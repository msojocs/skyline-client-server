//! main 层 client 应用层。
//!
//! 与 `client.rs` 共用同一套 RPC（`transport.rs` + `binding.rs`）和同一套远端对象代理机制
//! （`client::define_class` 建类表、`client::remote` 依 `instanceType` 复活代理、`State::decode`
//! 负责反序列化），差异只在应用层：这里代理的是 main 层的 Electron API，入口是
//! `mainController.electron.*`，而不是 webview 那套 Controller。
//!
//! 首条打通的方法：
//!
//! ```js
//! const { mainController } = require('main-client.node')
//! mainController.connect('127.0.0.1', 3002)
//! const webContents = mainController.electron.webContents.fromId(7)
//! webContents.loadURL('https://example.com/')
//! ```
//!
//! 两点沿用 render client 的既有语义：
//!
//! - 名为 `executeJavaScript` 的方法走 `client.rs` 的 AsyncTask 分支，不阻塞 JS 线程，
//!   失败表现为 Promise reject；其余方法同步阻塞并直接抛出。
//! - 返回 `{instanceId, instanceType}` 的对象由 `client::remote` 依 `instanceType` 复活成
//!   代理，因此 [`CLASSES`] 里的 `wire_name` 必须与服务端回的 `instanceType` 一致
//!   （服务端取 `constructor.name`，见 `packages/electron/main-rpc.js`）。

use crate::binding::{arguments, define_value, error, port, State};
use crate::client::{define_class, report, Class};
use napi::{JsObject, Result};
use serde_json::json;
use std::rc::Rc;

/// main 层可远程调用的类。`wire_name` 必须与服务端回传的 `instanceType` 一致。
const CLASSES: &[Class] = &[Class {
    wire_name: "WebContents",
    name: "WebContents",
    methods: &[
        "loadURL",
        "getURL",
        "reload",
        "executeJavaScript",
        "getId",
        "isDestroyed",
        "close",
        "openDevTools",
    ],
    properties: &[("id", true, false), ("url", true, false)],
    webview_element: false,
}];

pub fn init(state: &Rc<State>, exports: &mut JsObject) -> Result<()> {
    for class in CLASSES {
        define_class(state, class)?;
    }

    // mainController.electron.webContents.fromId(e)
    // 与服务端 server.ts 的 `static` 分支对应：clazz 为命名空间根，action 为点分路径。
    let web_contents = state.env.create_object()?;
    let weak = Rc::downgrade(state);
    let from_id = state
        .env
        .create_function_from_closure("fromId", move |ctx| {
            let state = weak
                .upgrade()
                .ok_or_else(|| error("Environment has closed"))?;
            report(&state, || {
                let result = state.request(
                    &json!({"type": "static", "clazz": "electron", "action": "webContents.fromId",
                    "data": {"params": arguments(&state, &ctx)?}})
                    .to_string(),
                )?;
                state.decode(&result["returnValue"])
            })
        })?;
    define_value(
        state.env,
        &web_contents,
        "fromId",
        from_id.into_unknown(),
        true,
    )?;

    let electron = state.env.create_object()?;
    define_value(
        state.env,
        &electron,
        "webContents",
        web_contents.into_unknown(),
        true,
    )?;

    let controller = state.env.create_object()?;
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
            // main 层的 RPC 服务端默认在 3002，避开 renderer 已占用的 3001。
            let port = if ctx.length > 1 { port(&ctx, 1)? } else { 3002 };
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
        &controller,
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
        &controller,
        "disconnect",
        disconnect.into_unknown(),
        true,
    )?;
    define_value(
        state.env,
        &controller,
        "electron",
        electron.into_unknown(),
        true,
    )?;

    exports.set_named_property("mainController", controller)?;
    Ok(())
}
