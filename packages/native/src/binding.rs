use crate::transport::{Endpoint, Message, Pending};
use napi::threadsafe_function::{ErrorStrategy, ThreadsafeFunction, ThreadsafeFunctionCallMode};
use napi::{
    CallContext, Env, Error, JsFunction, JsObject, JsUnknown, NapiRaw, NapiValue, Ref, Result,
    Status, ValueType,
};
use serde_json::{json, Value};
use std::cell::{Cell, RefCell};
use std::collections::HashMap;
use std::rc::Rc;
use std::sync::Arc;
use std::time::{Duration, Instant};

pub fn error(message: impl ToString) -> Error {
    Error::from_reason(message.to_string())
}
pub fn invalid(message: &str) -> Error {
    Error::new(Status::InvalidArg, message)
}

pub struct Reference {
    env: Env,
    reference: Ref<()>,
}

impl Reference {
    pub fn new<T: NapiRaw>(env: Env, value: T) -> Result<Self> {
        Ok(Self {
            env,
            reference: env.create_reference(value)?,
        })
    }
    pub fn get<T: NapiValue>(&self) -> Result<T> {
        self.env.get_reference_value(&self.reference)
    }
}

impl Drop for Reference {
    fn drop(&mut self) {
        let _ = self.reference.unref(self.env);
    }
}

pub struct State {
    pub env: Env,
    pub endpoint: RefCell<Option<Arc<Endpoint>>>,
    pub callbacks: RefCell<HashMap<u64, Reference>>,
    pub instances: RefCell<HashMap<(String, u64), Reference>>,
    pub constructors: RefCell<HashMap<String, Reference>>,
    pub error_callback: RefCell<Option<Reference>>,
    pub message_callback: RefCell<Option<Reference>>,
    pub epoch: Cell<u64>,
    next_callback: Cell<u64>,
    next_request: Cell<u64>,
}

impl State {
    pub fn new(env: Env) -> Rc<Self> {
        Rc::new(Self {
            env,
            endpoint: RefCell::new(None),
            callbacks: RefCell::new(HashMap::new()),
            instances: RefCell::new(HashMap::new()),
            constructors: RefCell::new(HashMap::new()),
            error_callback: RefCell::new(None),
            message_callback: RefCell::new(None),
            epoch: Cell::new(0),
            next_callback: Cell::new(1),
            next_request: Cell::new(if cfg!(feature = "client") { 1 } else { 2 }),
        })
    }

    pub fn cleanup(&self) {
        self.close();
        self.constructors.borrow_mut().clear();
        self.message_callback.borrow_mut().take();
    }

    fn stop_endpoint(&self) {
        let endpoint = self.endpoint.borrow_mut().take();
        if let Some(endpoint) = endpoint {
            endpoint.stop();
        }
    }

    pub fn close(&self) {
        self.stop_endpoint();
        self.instances.borrow_mut().clear();
        self.callbacks.borrow_mut().clear();
        self.error_callback.borrow_mut().take();
        self.epoch.set(self.epoch.get() + 1);
    }

    pub fn make_endpoint(self: &Rc<Self>) -> Result<Arc<Endpoint>> {
        let weak = Rc::downgrade(self);
        let dispatch = self
            .env
            .create_function_from_closure("dispatch", move |ctx| {
                if let Some(state) = weak.upgrade() {
                    state.drain()?;
                }
                ctx.env.get_undefined()
            })?;
        let mut wake: ThreadsafeFunction<(), ErrorStrategy::Fatal> =
            dispatch.create_threadsafe_function(0, |_| Ok(Vec::<JsUnknown>::new()))?;
        // Background sockets must not keep an otherwise idle Node environment alive.
        wake.unref(&self.env)?;
        let endpoint = Endpoint::new(if cfg!(feature = "client") { 1 } else { 0 }, move || {
            wake.call((), ThreadsafeFunctionCallMode::NonBlocking);
        });
        // Cleanup hooks run in reverse registration order. This hook is registered after the
        // module-level State hook, so Endpoint drops its TSFN while the N-API resource is valid.
        let weak = Rc::downgrade(self);
        let mut env = self.env;
        env.add_env_cleanup_hook(weak, |weak| {
            if let Some(state) = weak.upgrade() {
                state.stop_endpoint();
            }
        })?;
        Ok(endpoint)
    }

    pub fn socket(&self) -> Result<Arc<Endpoint>> {
        self.endpoint
            .borrow()
            .clone()
            .ok_or_else(|| error("Not connected. Call connect() or start() first."))
    }

    pub fn start_request(&self, body: &str) -> Result<(Arc<Endpoint>, Pending)> {
        let endpoint = self.socket()?;
        let id = self.next_request.get();
        self.next_request.set(if id >= 9_007_199_254_740_989 {
            if cfg!(feature = "client") {
                1
            } else {
                2
            }
        } else {
            id + 2
        });
        let pending = endpoint.request(id);
        endpoint.send(id, body, pending.generation).map_err(error)?;
        Ok((endpoint, pending))
    }

    pub fn request(self: &Rc<Self>, body: &str) -> Result<Value> {
        let (endpoint, pending) = self.start_request(body)?;
        let deadline =
            Instant::now() + Duration::from_secs(if cfg!(feature = "client") { 5 } else { 3 });
        loop {
            self.drain_endpoint(&endpoint)?;
            if let Some(body) = pending.poll(deadline).map_err(error)? {
                let response: Value = serde_json::from_str(&body).map_err(error)?;
                if let Some(message) = response.get("error") {
                    return Err(error(message.as_str().unwrap_or("Remote request failed")));
                }
                return Ok(response["result"].clone());
            }
        }
    }

    pub fn drain(self: &Rc<Self>) -> Result<()> {
        let endpoint = self.endpoint.borrow().clone();
        if let Some(endpoint) = endpoint {
            self.drain_endpoint(&endpoint)?;
        }
        Ok(())
    }

    fn drain_endpoint(self: &Rc<Self>, endpoint: &Arc<Endpoint>) -> Result<()> {
        #[cfg(feature = "server")]
        if self.message_callback.borrow().is_none() {
            return Ok(());
        }
        while let Some(message) = endpoint.pop() {
            self.dispatch(endpoint, message)?;
        }
        Ok(())
    }

    fn dispatch(self: &Rc<Self>, endpoint: &Arc<Endpoint>, message: Message) -> Result<()> {
        #[cfg(feature = "server")]
        {
            let callback: JsFunction = self
                .message_callback
                .borrow()
                .as_ref()
                .ok_or_else(|| error("Message callback is not registered"))?
                .get()?;
            callback.call(
                None,
                &[
                    self.env.create_string(&message.body)?.into_unknown(),
                    self.env.create_double(message.id as f64)?.into_unknown(),
                ],
            )?;
            let _ = endpoint;
        }
        #[cfg(feature = "client")]
        {
            let payload: Value = serde_json::from_str(&message.body).map_err(error)?;
            if payload["type"] != "emitCallback" {
                return Ok(());
            }
            let id = payload["callbackId"]
                .as_u64()
                .ok_or_else(|| error("Invalid callbackId"))?;
            let result = (|| {
                let callback: JsFunction = self
                    .callbacks
                    .borrow()
                    .get(&id)
                    .ok_or_else(|| error(format!("Unknown callbackId: {id}")))?
                    .get()?;
                let args = payload["data"]["args"]
                    .as_array()
                    .ok_or_else(|| error("Invalid callback arguments"))?
                    .iter()
                    .map(|arg| self.decode(arg))
                    .collect::<Result<Vec<_>>>()?;
                let value = callback.call(None, &args)?;
                self.encode(value)
            })();
            match result {
                Ok(value) if message.id > 0 => endpoint
                    .send(
                        message.id,
                        &json!({"type": "callbackReply", "result": value}).to_string(),
                        message.generation,
                    )
                    .map_err(error)?,
                Err(error) => {
                    if message.id > 0 {
                        let _ = endpoint.send(
                            message.id,
                            &json!({"type": "callbackReply", "error": error.reason}).to_string(),
                            message.generation,
                        );
                    }
                    return Err(error);
                }
                _ => (),
            }
        }
        Ok(())
    }

    pub fn encode(&self, value: JsUnknown) -> Result<Value> {
        self.encode_inner(value, &mut Vec::new())
    }

    fn encode_inner(
        &self,
        value: JsUnknown,
        ancestors: &mut Vec<napi::sys::napi_value>,
    ) -> Result<Value> {
        if ancestors.len() >= 128 {
            return Err(invalid("Argument nesting exceeds 128 levels"));
        }
        match value.get_type()? {
            ValueType::String => Ok(Value::String(
                value.coerce_to_string()?.into_utf8()?.as_str()?.into(),
            )),
            ValueType::Number => Ok(json!(value.coerce_to_number()?.get_double()?)),
            ValueType::Boolean => Ok(json!(value.coerce_to_bool()?.get_value()?)),
            ValueType::Function | ValueType::Object => {
                // All handles here belong to this call's environment and remain rooted by its arguments.
                let raw = unsafe { value.raw() };
                for ancestor in ancestors.iter() {
                    let ancestor =
                        unsafe { JsUnknown::from_raw_unchecked(self.env.raw(), *ancestor) };
                    let current = unsafe { JsUnknown::from_raw_unchecked(self.env.raw(), raw) };
                    if self.env.strict_equals(ancestor, current)? {
                        return Err(invalid("Cannot serialize a circular argument"));
                    }
                }
                ancestors.push(raw);
                let result = self.encode_object(value, ancestors);
                ancestors.pop();
                result
            }
            _ => Ok(Value::Null),
        }
    }

    fn encode_object(
        &self,
        value: JsUnknown,
        ancestors: &mut Vec<napi::sys::napi_value>,
    ) -> Result<Value> {
        if value.get_type()? == ValueType::Function {
            let mut function = value.coerce_to_object()?;
            let mut id = None;
            for (key, reference) in self.callbacks.borrow().iter() {
                let existing: JsUnknown = reference.get()?;
                // Functions are valid objects for strict equality, without invoking user code.
                let current = borrow_object(self.env, &function)?;
                if self.env.strict_equals(existing, current)? {
                    id = Some(*key);
                    break;
                }
            }
            let id = if let Some(id) = id {
                id
            } else {
                let id = self.next_callback.get();
                self.next_callback.set(id + 1);
                self.callbacks.borrow_mut().insert(
                    id,
                    Reference::new(self.env, borrow_object(self.env, &function)?)?,
                );
                // Preserve the public marker used by existing clients, including frozen callbacks.
                let extensible: JsUnknown = function.get_named_property("__callbackId")?;
                if extensible.get_type()? == ValueType::Undefined {
                    let _ = function
                        .set_named_property("__callbackId", self.env.create_double(id as f64)?);
                }
                id
            };
            let async_flag: JsUnknown = function.get_named_property("__asyncCallback")?;
            let mut encoded = json!({"callbackId": id, "asyncCallback": async_flag.get_type()? == ValueType::Boolean && async_flag.coerce_to_bool()?.get_value()?});
            let worklet: JsUnknown = function.get_named_property("__worklet")?;
            if worklet.get_type()? == ValueType::Boolean {
                for key in [
                    "__worklet",
                    "asString",
                    "__workletHash",
                    "__location",
                    "_closure",
                ] {
                    encoded[key] =
                        self.encode_inner(function.get_named_property(key)?, ancestors)?;
                }
            }
            return Ok(encoded);
        }
        if value.is_buffer()? {
            return bytes(self.env, &value, true);
        }
        let object = value.coerce_to_object()?;
        let mut is_arraybuffer = false;
        // The N-API predicate also handles ArrayBuffers from another JS realm.
        napi::check_status!(unsafe {
            napi::sys::napi_is_arraybuffer(self.env.raw(), object.raw(), &mut is_arraybuffer)
        })?;
        if is_arraybuffer {
            return bytes(self.env, &object, false);
        }
        if object.is_array()? {
            return (0..object.get_array_length()?)
                .map(|i| self.encode_inner(object.get_element(i)?, ancestors))
                .collect::<Result<Vec<_>>>()
                .map(Value::Array);
        }
        let instance_id: JsUnknown = object.get_named_property("instanceId")?;
        if instance_id.get_type()? == ValueType::Number {
            return Ok(json!({"instanceId": instance_id.coerce_to_number()?.get_int64()?}));
        }
        let keys = object.get_property_names()?;
        let mut result = serde_json::Map::new();
        for i in 0..keys.get_array_length()? {
            let key = keys
                .get_element::<napi::JsString>(i)?
                .into_utf8()?
                .as_str()?
                .to_owned();
            if !key.is_empty() {
                result.insert(
                    key.clone(),
                    self.encode_inner(object.get_named_property(&key)?, ancestors)?,
                );
            }
        }
        Ok(Value::Object(result))
    }

    pub fn decode(self: &Rc<Self>, value: &Value) -> Result<JsUnknown> {
        match value {
            Value::Null => Ok(self.env.get_undefined()?.into_unknown()),
            Value::Bool(value) => Ok(self.env.get_boolean(*value)?.into_unknown()),
            Value::Number(value) => Ok(self
                .env
                .create_double(value.as_f64().unwrap())?
                .into_unknown()),
            Value::String(value) => Ok(self.env.create_string(value)?.into_unknown()),
            Value::Array(values) => {
                let mut array = self.env.create_array_with_length(values.len())?;
                for (i, value) in values.iter().enumerate() {
                    array.set_element(i as u32, self.decode(value)?)?;
                }
                Ok(array.into_unknown())
            }
            Value::Object(values) => {
                #[cfg(feature = "client")]
                if let (Some(id), Some(kind)) =
                    (value["instanceId"].as_u64(), value["instanceType"].as_str())
                {
                    return crate::client::remote(self, kind, id);
                }
                let object = self.env.create_object()?;
                for (key, value) in values {
                    define_value(self.env, &object, key, self.decode(value)?, true)?;
                }
                Ok(object.into_unknown())
            }
        }
    }
}

pub fn arguments(state: &State, ctx: &CallContext) -> Result<Value> {
    ctx.get_all()
        .into_iter()
        .map(|value| state.encode(value))
        .collect::<Result<Vec<_>>>()
        .map(Value::Array)
}

pub fn port(ctx: &CallContext, index: usize) -> Result<u16> {
    let value = napi::JsNumber::try_from(ctx.get::<JsUnknown>(index)?)?.get_double()?;
    if !value.is_finite() || value.fract() != 0.0 || !(0.0..=65535.0).contains(&value) {
        return Err(invalid("Port must be an integer between 0 and 65535"));
    }
    Ok(value as u16)
}

pub fn function(ctx: &CallContext, index: usize) -> Result<JsFunction> {
    if index >= ctx.length {
        return Err(invalid("Expected a function argument"));
    }
    JsFunction::try_from(ctx.get::<JsUnknown>(index)?)
}

fn bytes(env: Env, value: &impl NapiRaw, buffer: bool) -> Result<Value> {
    let mut data = std::ptr::null_mut();
    let mut length = 0;
    // The caller has checked the type. Copy while the JS argument is rooted; empty buffers may have null data.
    napi::check_status!(unsafe {
        if buffer {
            napi::sys::napi_get_buffer_info(env.raw(), value.raw(), &mut data, &mut length)
        } else {
            napi::sys::napi_get_arraybuffer_info(env.raw(), value.raw(), &mut data, &mut length)
        }
    })?;
    if length == 0 {
        return Ok(json!([]));
    }
    if data.is_null() {
        return Err(invalid("Buffer is detached"));
    }
    Ok(json!(unsafe {
        std::slice::from_raw_parts(data.cast::<u8>(), length)
    }))
}

pub fn define_value(
    env: Env,
    object: &JsObject,
    name: &str,
    value: JsUnknown,
    writable: bool,
) -> Result<()> {
    let mut descriptor = env.create_object()?;
    descriptor.set_named_property("value", value)?;
    descriptor.set_named_property("writable", writable)?;
    descriptor.set_named_property("configurable", writable)?;
    descriptor.set_named_property("enumerable", writable)?;
    define_property(env, object, name, descriptor)
}

pub fn define_property(
    env: Env,
    object: &JsObject,
    name: &str,
    descriptor: JsObject,
) -> Result<()> {
    let constructor = env
        .get_global()?
        .get_named_property::<JsFunction>("Object")?
        .coerce_to_object()?;
    let define: JsFunction = constructor.get_named_property("defineProperty")?;
    define.call(
        Some(&constructor),
        &[
            borrow_object(env, object)?.into_unknown(),
            env.create_string(name)?.into_unknown(),
            descriptor.into_unknown(),
        ],
    )?;
    Ok(())
}

pub fn borrow_object(env: Env, value: &impl NapiRaw) -> Result<JsObject> {
    // Duplicate a handle within its existing scope; this does not extend its lifetime.
    unsafe { JsUnknown::from_raw_unchecked(env.raw(), value.raw()) }.coerce_to_object()
}
