use crate::binding::{
    arguments, borrow_object, define_property, define_value, error, function, invalid, port,
    Reference, State,
};
use crate::transport::Pending;
use napi::bindgen_prelude::{AsyncTask, ToNapiValue};
use napi::{
    CallContext, Env, JsFunction, JsNumber, JsObject, JsUnknown, NapiValue, Result, Task, ValueType,
};
use serde_json::{json, Value};
use std::rc::{Rc, Weak};
use std::time::{Duration, Instant};

/// 返回 Promise 的远端方法走 AsyncTask 分支：等待在 worker 线程上进行，不阻塞 JS 线程，
/// 失败表现为 Promise reject 而不是同步抛出。列表按方法名匹配，render 与 main 两个应用层共用。
const ASYNC_METHODS: &[&str] = &["executeJavaScript", "loadExtension"];

struct AsyncCallTask {
    state: Weak<State>,
    pending: Option<Pending>,
    initial_error: Option<napi::Error>,
}

// napi-rs runs compute on a worker and resolve/reject/drop back on the JS
// thread. compute only touches Pending; the non-Send Weak<State> is accessed
// after the task returns to its owning JS thread.
unsafe impl Send for AsyncCallTask {}

impl AsyncCallTask {
    fn state(&self) -> Result<Rc<State>> {
        self.state
            .upgrade()
            .ok_or_else(|| error("Environment has closed"))
    }
}

impl Task for AsyncCallTask {
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

pub(crate) struct Class {
    pub(crate) wire_name: &'static str,
    pub(crate) name: &'static str,
    pub(crate) methods: &'static [&'static str],
    pub(crate) properties: &'static [(&'static str, bool, bool)],
    /// render 客户端的 DOM 元素类额外挂载 reload / setAttribute / removeAttribute / isConnected。
    /// main 客户端代理的是 Electron 对象，不挂这些。
    pub(crate) webview_element: bool,
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
        webview_element: false,
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
            "send",
            "setUserAgent",
            "openDevTools",
            "getWebContentsId",
        ],
        properties: &[
            ("src", true, true),
            ("style", true, true),
            ("parentElement", true, false),
        ],
        webview_element: true,
    },
    Class {
        wire_name: "HTMLDivElement",
        name: "HTMLDivElement",
        methods: &[],
        properties: &[("id", true, false)],
        webview_element: false,
    },
    Class {
        wire_name: "CSSStyleDeclaration",
        name: "CSSStyleDeclaration",
        methods: &["setText"],
        properties: &[("display", true, true), ("pointerEvents", true, true)],
        webview_element: true,
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
        webview_element: true,
    },
    Class {
        wire_name: "WebRequestEvent",
        name: "WebRequestEvent",
        methods: &["addListener", "hasListener", "removeListener"],
        properties: &[],
        webview_element: true,
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
        webview_element: true,
    },
    Class {
        wire_name: "RequestRule",
        name: "RequestRule",
        methods: &["getRules", "addRules", "removeRules"],
        properties: &[],
        webview_element: true,
    },
];

pub fn init(state: &Rc<State>, exports: &mut JsObject) -> Result<()> {
    for class in CLASSES {
        let constructor = define_class(state, class)?;
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
            define_value(
                state.env,
                &constructor,
                "connect",
                connect.into_unknown(),
                true,
            )?;
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
                &constructor,
                "disconnect",
                disconnect.into_unknown(),
                true,
            )?;
            exports.set_named_property("Controller", constructor)?;
        }
    }
    Ok(())
}

/// 按 `Class` 表建一个 JS 构造函数：挂上方法/属性原型，并登记到 `state.constructors`，
/// 供 `binding.rs` 的 `decode` 依 `instanceType` 复活远端对象（见 `client::remote`）。
/// render 与 main 两个客户端应用层共用这段逻辑，差异只在各自的 `Class` 表。
pub(crate) fn define_class(state: &Rc<State>, class: &'static Class) -> Result<JsObject> {
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
            let js_id = ctx.get::<napi::JsNumber>(0)?;
            let id = js_id.get_double()?;
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
    if class.webview_element {
        for method in ["reload", "setAttribute", "removeAttribute"] {
            add_method(state, &prototype, method)?;
        }
        add_property(state, &prototype, "isConnected", true, false)?;
    }
    for (name, get, set) in class.properties {
        add_property(state, &prototype, name, *get, *set)?;
    }
    let object = borrow_object(state.env, &constructor)?;
    state
        .constructors
        .borrow_mut()
        .insert(class.wire_name.into(), Reference::new(state.env, &object)?);
    Ok(object)
}

/// 读句柄上的数字字段。缺失、`undefined` 或非数字一律当作没有：`get_named_property::<f64>`
/// 遇到 `undefined` 会抛 napi 的 "Expect value to be Number, but received Undefined"。
fn number_property(object: &JsObject, name: &str) -> Result<Option<f64>> {
    let value: JsUnknown = object.get_named_property(name)?;
    match JsNumber::try_from(value) {
        Ok(value) => Ok(Some(value.get_double()?)),
        Err(_) => Ok(None),
    }
}

/// 从句柄读 `(instanceId, __skylineEpoch)`。两者都是建句柄时用 `define_value` 写上的非负整数，
/// 外来对象上的同名值不算句柄。
fn handle_binding(object: &JsObject) -> Result<Option<(u64, u64)>> {
    let field = |name: &str| -> Result<Option<u64>> {
        Ok(number_property(object, name)?
            .filter(|value| value.is_finite() && value.fract() == 0.0 && *value >= 0.0)
            .map(|value| value as u64))
    };
    Ok(match (field("instanceId")?, field("__skylineEpoch")?) {
        (Some(id), Some(epoch)) => Some((id, epoch)),
        _ => None,
    })
}

fn instance(state: &State, ctx: &CallContext) -> Result<u64> {
    let object = ctx.this::<JsObject>()?;
    let Some((id, epoch)) = handle_binding(&object)? else {
        // 方法被摘下来单独调用（`const f = webview.getURL; f()`、`[id].map(webview.getId)`）
        // 时接收者会丢，这里给一条明确的错误，而不是让 napi 报属性类型不匹配。
        return Err(error("Remote method receiver is not a Skyline instance"));
    };
    if epoch != state.epoch.get() {
        return Err(error("Remote object belongs to a closed connection"));
    }
    Ok(id)
}

/// 一次远端调用：`ASYNC_METHODS` 里的方法走 AsyncTask（不阻塞 JS 线程，失败表现为 Promise
/// reject），其余方法同步阻塞并直接抛出。
fn invoke(state: &Rc<State>, ctx: &CallContext, name: &str, id: u64) -> Result<JsUnknown> {
    if ASYNC_METHODS.contains(&name) {
        let request = (|| {
            let body = json!({"type": "dynamic", "action": name,
                "data": {"instanceId": id, "params": arguments(state, ctx)?}}).to_string();
            state.start_request(&body).map(|(_, pending)| pending)
        })();
        let task = match request {
            Ok(pending) => AsyncCallTask {
                state: Rc::downgrade(state),
                pending: Some(pending),
                initial_error: None,
            },
            Err(initial_error) => AsyncCallTask {
                state: Rc::downgrade(state),
                pending: None,
                initial_error: Some(initial_error),
            },
        };
        let promise = unsafe {
            <AsyncTask<AsyncCallTask> as ToNapiValue>::to_napi_value(
                state.env.raw(),
                AsyncTask::new(task),
            )?
        };
        return Ok(unsafe { JsUnknown::from_raw_unchecked(state.env.raw(), promise) });
    }
    if name == "showDevTools" { return Err(error("Not implemented")); }
    let result = state.request(&json!({"type": "dynamic", "action": name,
        "data": {"instanceId": id, "params": arguments(state, ctx)?}}).to_string())?;
    state.decode(&result["returnValue"])
}

/// 造一个方法闭包。`binding` 为 `Some` 时身份在建方法时就固定下来、调用时不看接收者，
/// 与 `remote()` 的函数代理同一套语义；为 `None` 时退回调用点传入的 `this`。
fn method_function(
    state: &Rc<State>,
    name: &str,
    binding: Option<(u64, u64)>,
) -> Result<JsFunction> {
    let weak = Rc::downgrade(state);
    let method_name = name.to_owned();
    state.env.create_function_from_closure(name, move |ctx| {
        let state = weak.upgrade().ok_or_else(|| error("Environment has closed"))?;
        let (id, epoch) = match binding {
            Some(binding) => binding,
            None => (instance(&state, &ctx)?, state.epoch.get()),
        };
        if epoch != state.epoch.get() {
            return Err(error("Remote object belongs to a closed connection"));
        }
        report(&state, || invoke(&state, &ctx, &method_name, id))
    })
}

/// 原型上挂的是访问器：取值时按当前句柄现绑一个方法闭包。于是
/// `const f = webview.getWebContentsId; f()`、`[id].map(webview.getId)` 这类把方法从句柄上
/// 摘下来的写法仍然作用在原句柄上，而不会丢掉 `this`。取值本身不校验连接世代，
/// 保持"读方法不报错、调用才报错"的语义。
fn add_method(state: &Rc<State>, prototype: &JsObject, name: &'static str) -> Result<()> {
    let mut descriptor = state.env.create_object()?;
    descriptor.set_named_property("configurable", true)?;
    let weak = Rc::downgrade(state);
    let getter = state.env.create_function_from_closure(name, move |ctx| {
        let state = weak
            .upgrade()
            .ok_or_else(|| error("Environment has closed"))?;
        let binding = handle_binding(&ctx.this::<JsObject>()?)?;
        Ok(method_function(&state, name, binding)?.into_unknown())
    })?;
    descriptor.set_named_property("get", getter)?;
    // 方法原本是原型上的可写数据属性，赋值会在句柄上落一个自有属性把它盖住；换成访问器后
    // 必须显式保留这套语义（例如 `webview.getWebContentsId = () => 114514 + getId()`）。
    let weak = Rc::downgrade(state);
    let setter = state.env.create_function_from_closure(name, move |ctx| {
        let state = weak
            .upgrade()
            .ok_or_else(|| error("Environment has closed"))?;
        define_value(
            state.env,
            &ctx.this::<JsObject>()?,
            name,
            ctx.get::<JsUnknown>(0)?,
            true,
        )?;
        ctx.env.get_undefined()
    })?;
    descriptor.set_named_property("set", setter)?;
    define_property(state.env, prototype, name, descriptor)
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

pub(crate) fn report<T>(state: &State, operation: impl FnOnce() -> Result<T>) -> Result<T> {
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

// Anonymous Electron objects arrive as Object handles without a fixed class API.
// Decode their properties recursively, binding function members to the owning handle.
fn remote_object(state: &Rc<State>, id: u64) -> Result<JsUnknown> {
    let object = state.env.create_object()?;
    let epoch = state.epoch.get();
    define_value(
        state.env, &object, "instanceId",
        state.env.create_double(id as f64)?.into_unknown(), false,
    )?;
    define_value(
        state.env, &object, "__skylineEpoch",
        state.env.create_double(epoch as f64)?.into_unknown(), false,
    )?;

    let mut handler = state.env.create_object()?;
    let weak = Rc::downgrade(state);
    let getter = state.env.create_function_from_closure("get", move |ctx| {
        let state = weak
            .upgrade()
            .ok_or_else(|| error("Environment has closed"))?;
        let target = ctx.get::<JsObject>(0)?;
        let key = ctx.get::<JsUnknown>(1)?;
        if key.get_type()? != ValueType::String {
            return target.get_property::<_, JsUnknown>(key);
        }
        let name = key.coerce_to_string()?.into_utf8()?.as_str()?.to_owned();
        if target.has_named_property(&name)? {
            return target.get_named_property::<JsUnknown>(&name);
        }
        if epoch != state.epoch.get() {
            return Err(error("Remote object belongs to a closed connection"));
        }
        report(&state, || {
            let result = state.request(&json!({
                "type": "dynamicProperty", "action": name,
                "data": {"instanceId": id, "propertyAction": "get"}
            }).to_string())?;
            let value = &result["returnValue"];
            if value["instanceType"].as_str() == Some("function") {
                let method = method_function(&state, &name, Some((id, epoch)))?;
                define_value(state.env, &target, &name, method.into_unknown(), true)?;
                target.get_named_property::<JsUnknown>(&name)
            } else {
                state.decode(value)
            }
        })
    })?;
    handler.set_named_property("get", getter)?;
    let proxy: JsFunction = state.env.get_global()?.get_named_property("Proxy")?;
    Ok(proxy.new_instance(&[object, handler])?.into_unknown())
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
    } else if kind == "Object" {
        remote_object(state, id)?
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
