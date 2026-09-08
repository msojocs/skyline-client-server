use crate::binding::{
    arguments, borrow_object, define_property, define_value, error, function, invalid, port,
    Reference, State,
};
use crate::transport::Pending;
use napi::bindgen_prelude::{AsyncTask, ToNapiValue};
use napi::{CallContext, Env, JsFunction, JsObject, JsUnknown, NapiValue, Result, Task};
use serde_json::{json, Value};
use std::rc::{Rc, Weak};
use std::time::{Duration, Instant};

struct ExecuteJavaScriptTask {
    state: Weak<State>,
    pending: Option<Pending>,
    initial_error: Option<napi::Error>,
}

// napi-rs runs compute on a worker and resolve/reject/drop back on the JS
// thread. compute only touches Pending; the non-Send Weak<State> is accessed
// after the task returns to its owning JS thread.
unsafe impl Send for ExecuteJavaScriptTask {}

impl ExecuteJavaScriptTask {
    fn state(&self) -> Result<Rc<State>> {
        self.state
            .upgrade()
            .ok_or_else(|| error("Environment has closed"))
    }
}

impl Task for ExecuteJavaScriptTask {
    type Output = Value;
    type JsValue = JsUnknown;

    fn compute(&mut self) -> Result<Self::Output> {
        if let Some(error) = self.initial_error.take() {
            return Err(error);
        }
        let pending = self
            .pending
            .as_ref()
            .ok_or_else(|| error("RPC request was not initialized"))?;
        let body = pending
            .wait(Instant::now() + Duration::from_secs(300))
            .map_err(error)?;
        let response: Value = serde_json::from_str(&body).map_err(error)?;
        if let Some(message) = response.get("error") {
            return Err(error(message.as_str().unwrap_or("Remote request failed")));
        }
        Ok(response["result"]["returnValue"].clone())
    }

    fn resolve(&mut self, _env: Env, output: Self::Output) -> Result<Self::JsValue> {
        let state = self.state()?;
        report(&state, || state.decode(&output))
    }

    fn reject(&mut self, _env: Env, err: napi::Error) -> Result<Self::JsValue> {
        let state = self.state()?;
        report(&state, || Err(err))
    }
}

struct Class {
    wire_name: &'static str,
    name: &'static str,
    methods: &'static [&'static str],
    properties: &'static [(&'static str, bool, bool)],
}

const CLASSES: &[Class] = &[
    Class {
        wire_name: "Controller",
        name: "Controller",
        methods: &[
            "mount",
            "unmount",
            "setDialogCallback",
            "dialog",
            "resolveDialog",
        ],
        properties: &[("webview", true, false)],
    },
    Class {
        wire_name: "ChromeWebViewElement",
        name: "WebviewElement",
        methods: &[
            "addEventListener",
            "executeJavaScript",
            "getAttribute",
            "getUserAgent",
            "removeEventListener",
            "setUserAgentOverride",
            "showDevTools",
            "getWebContentsId",
        ],
        properties: &[
            ("request", true, false),
            ("src", true, true),
            ("style", true, true),
            ("ondialog", false, true),
        ],
    },
    Class {
        wire_name: "CSSStyleDeclaration",
        name: "CSSStyleDeclaration",
        methods: &["setText"],
        properties: &[("display", true, true), ("pointerEvents", true, true)],
    },
    Class {
        wire_name: "Event",
        name: "Event",
        methods: &["preventDefault"],
        properties: &[
            ("dialog", true, false),
            ("messageText", true, false),
            ("messageType", true, false),
            ("returnValue", false, true),
            ("type", true, false),
        ],
    },
    Class {
        wire_name: "WebRequestEvent",
        name: "WebRequestEvent",
        methods: &["addListener", "hasListener", "removeListener"],
        properties: &[],
    },
    Class {
        wire_name: "RequestMessageEvent",
        name: "RequestMessageEvent",
        methods: &[
            "addListener",
            "dispatch",
            "dispatchNW",
            "getListeners",
            "hasListener",
            "hasListeners",
            "removeListener",
        ],
        properties: &[],
    },
    Class {
        wire_name: "RequestRule",
        name: "RequestRule",
        methods: &["getRules", "addRules", "removeRules"],
        properties: &[],
    },
];

pub fn init(state: &Rc<State>, exports: &mut JsObject) -> Result<()> {
    for class in CLASSES {
        let weak = Rc::downgrade(state);
        let constructor = state.env.create_function_from_closure(class.name, move |ctx| {
            ctx.get_new_target::<JsFunction>()?;
            let state = weak.upgrade().ok_or_else(|| error("Environment has closed"))?;
            let object = ctx.this::<JsObject>()?;
            let id = if class.name == "Controller" {
                let callback = function(&ctx, 0)?;
                *state.error_callback.borrow_mut() = Some(Reference::new(state.env, callback)?);
                let result = report(&state, || state.request(&json!({
                    "type": "constructor", "clazz": "Controller", "data": {"params": arguments(&state, &ctx)?}
                }).to_string()))?;
                result["instanceId"].as_u64().ok_or_else(|| error("No instanceId in constructor response"))?
            } else {
                let id = ctx.get::<napi::JsNumber>(0)?.get_double()?;
                if !id.is_finite() || id <= 0.0 || id.fract() != 0.0 || id > 9_007_199_254_740_991.0 {
                    return Err(invalid("Invalid remote instanceId"));
                }
                id as u64
            };
            define_value(state.env, &object, "instanceId", state.env.create_double(id as f64)?.into_unknown(), false)?;
            define_value(state.env, &object, "__skylineEpoch", state.env.create_double(state.epoch.get() as f64)?.into_unknown(), false)?;
            Ok(object)
        })?;
        let prototype: JsObject =
            borrow_object(state.env, &constructor)?.get_named_property("prototype")?;
        for method in class.methods {
            add_method(state, &prototype, method)?;
        }
        if class.name != "Controller" {
            for method in ["reload", "setAttribute", "removeAttribute"] {
                add_method(state, &prototype, method)?;
            }
            add_property(state, &prototype, "isConnected", true, false)?;
        }
        for (name, get, set) in class.properties {
            add_property(state, &prototype, name, *get, *set)?;
        }
        if class.name == "Controller" {
            let weak = Rc::downgrade(state);
            let connect = state
                .env
                .create_function_from_closure("connect", move |ctx| {
                    let state = weak
                        .upgrade()
                        .ok_or_else(|| error("Environment has closed"))?;
                    let host = if ctx.length > 0 {
                        ctx.get::<String>(0)?
                    } else {
                        "127.0.0.1".into()
                    };
                    let port = if ctx.length > 1 { port(&ctx, 1)? } else { 3001 };
                    if state
                        .endpoint
                        .borrow()
                        .as_ref()
                        .is_some_and(|endpoint| endpoint.connected())
                    {
                        return ctx.env.get_undefined();
                    }
                    state.close();
                    let endpoint = state.make_endpoint()?;
                    endpoint.connect(&host, port).map_err(error)?;
                    *state.endpoint.borrow_mut() = Some(endpoint);
                    ctx.env.get_undefined()
                })?;
            let object = borrow_object(state.env, &constructor)?;
            define_value(state.env, &object, "connect", connect.into_unknown(), true)?;
            let weak = Rc::downgrade(state);
            let disconnect = state
                .env
                .create_function_from_closure("disconnect", move |ctx| {
                    if let Some(state) = weak.upgrade() {
                        state.close();
                    }
                    ctx.env.get_undefined()
                })?;
            define_value(
                state.env,
                &object,
                "disconnect",
                disconnect.into_unknown(),
                true,
            )?;
            exports.set_named_property("Controller", borrow_object(state.env, &constructor)?)?;
        }
        state.constructors.borrow_mut().insert(
            class.wire_name.into(),
            Reference::new(state.env, constructor)?,
        );
    }
    Ok(())
}

fn instance(state: &State, ctx: &CallContext) -> Result<u64> {
    let object = ctx.this::<JsObject>()?;
    let epoch: f64 = object.get_named_property("__skylineEpoch")?;
    if epoch as u64 != state.epoch.get() {
        return Err(error("Remote object belongs to a closed connection"));
    }
    Ok(object.get_named_property::<f64>("instanceId")? as u64)
}

fn add_method(state: &Rc<State>, prototype: &JsObject, name: &'static str) -> Result<()> {
    let weak = Rc::downgrade(state);
    let function = state.env.create_function_from_closure(name, move |ctx| {
        let state = weak.upgrade().ok_or_else(|| error("Environment has closed"))?;
        if name == "executeJavaScript" {
            let request = (|| {
                let body = json!({"type": "dynamic", "action": name,
                    "data": {"instanceId": instance(&state, &ctx)?, "params": arguments(&state, &ctx)?}}).to_string();
                state.start_request(&body).map(|(_, pending)| pending)
            })();
            let task = match request {
                Ok(pending) => ExecuteJavaScriptTask {
                    state: Rc::downgrade(&state),
                    pending: Some(pending),
                    initial_error: None,
                },
                Err(initial_error) => ExecuteJavaScriptTask {
                    state: Rc::downgrade(&state),
                    pending: None,
                    initial_error: Some(initial_error),
                },
            };
            let promise = unsafe {
                <AsyncTask<ExecuteJavaScriptTask> as ToNapiValue>::to_napi_value(
                    state.env.raw(),
                    AsyncTask::new(task),
                )?
            };
            return Ok(unsafe { JsUnknown::from_raw_unchecked(state.env.raw(), promise) });
        }
        report(&state, || {
            if name == "showDevTools" { return Err(error("Not implemented")); }
            let result = state.request(&json!({"type": "dynamic", "action": name,
                "data": {"instanceId": instance(&state, &ctx)?, "params": arguments(&state, &ctx)?}}).to_string())?;
            state.decode(&result["returnValue"])
        })
    })?;
    define_value(state.env, prototype, name, function.into_unknown(), true)
}

fn add_property(
    state: &Rc<State>,
    prototype: &JsObject,
    name: &'static str,
    get: bool,
    set: bool,
) -> Result<()> {
    let mut descriptor = state.env.create_object()?;
    descriptor.set_named_property("configurable", true)?;
    if get {
        let weak = Rc::downgrade(state);
        let getter = state.env.create_function_from_closure(name, move |ctx| {
            let state = weak
                .upgrade()
                .ok_or_else(|| error("Environment has closed"))?;
            report(&state, || {
                let result = state.request(
                    &json!({"type": "dynamicProperty", "action": name,
                    "data": {"instanceId": instance(&state, &ctx)?, "propertyAction": "get"}})
                    .to_string(),
                )?;
                state.decode(&result["returnValue"])
            })
        })?;
        descriptor.set_named_property("get", getter)?;
    }
    if set {
        let weak = Rc::downgrade(state);
        let setter = state.env.create_function_from_closure(name, move |ctx| {
            let state = weak.upgrade().ok_or_else(|| error("Environment has closed"))?;
            report(&state, || {
                state.request(&json!({"type": "dynamicProperty", "action": name,
                    "data": {"instanceId": instance(&state, &ctx)?, "propertyAction": "set", "params": arguments(&state, &ctx)?}}).to_string())?;
                ctx.env.get_undefined()
            })
        })?;
        descriptor.set_named_property("set", setter)?;
    }
    define_property(state.env, prototype, name, descriptor)
}

fn report<T>(state: &State, operation: impl FnOnce() -> Result<T>) -> Result<T> {
    let result = operation();
    if let Err(ref error) = result {
        let callback = state
            .error_callback
            .borrow()
            .as_ref()
            .map(|reference| reference.get::<JsFunction>())
            .transpose()?;
        if let Some(callback) = callback {
            callback.call(None, &[state.env.create_string(&error.reason)?])?;
        }
    }
    result
}

pub fn remote(state: &Rc<State>, kind: &str, id: u64) -> Result<JsUnknown> {
    let key = (kind.into(), id);
    if let Some(reference) = state.instances.borrow().get(&key) {
        return reference.get();
    }
    let object = if kind == "function" {
        let weak = Rc::downgrade(state);
        let epoch = state.epoch.get();
        state
            .env
            .create_function_from_closure("remoteFunction", move |ctx| {
                let state = weak
                    .upgrade()
                    .ok_or_else(|| error("Environment has closed"))?;
                if epoch != state.epoch.get() {
                    return Err(error("Remote function belongs to a closed connection"));
                }
                let result = state.request(
                    &json!({"type": "static", "clazz": "functionData", "action": id.to_string(),
                "data": {"params": arguments(&state, &ctx)?}})
                    .to_string(),
                )?;
                state.decode(&result["returnValue"])
            })?
            .into_unknown()
    } else {
        let constructor = state
            .constructors
            .borrow()
            .get(kind)
            .map(|reference| reference.get::<JsFunction>())
            .transpose()?;
        let Some(constructor) = constructor else {
            return Ok(state.env.get_undefined()?.into_unknown());
        };
        if kind == "Controller" {
            // A returned Controller already exists remotely; its public constructor would issue another RPC.
            let object_ctor = state
                .env
                .get_global()?
                .get_named_property::<JsFunction>("Object")?
                .coerce_to_object()?;
            let create: JsFunction = object_ctor.get_named_property("create")?;
            let prototype: JsObject =
                borrow_object(state.env, &constructor)?.get_named_property("prototype")?;
            let object = create
                .call(Some(&object_ctor), &[prototype])?
                .coerce_to_object()?;
            define_value(
                state.env,
                &object,
                "instanceId",
                state.env.create_double(id as f64)?.into_unknown(),
                false,
            )?;
            define_value(
                state.env,
                &object,
                "__skylineEpoch",
                state
                    .env
                    .create_double(state.epoch.get() as f64)?
                    .into_unknown(),
                false,
            )?;
            object.into_unknown()
        } else {
            constructor
                .new_instance(&[state.env.create_double(id as f64)?])?
                .into_unknown()
        }
    };
    state.instances.borrow_mut().insert(
        key,
        Reference::new(state.env, borrow_object(state.env, &object)?)?,
    );
    Ok(object)
}
