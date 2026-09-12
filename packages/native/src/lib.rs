#![allow(dead_code)]

#[cfg(all(feature = "client", feature = "server"))]
compile_error!("Build client and server separately with --no-default-features");

#[cfg(not(test))]
mod binding;
// `main-client` 隐含 `client`：client.rs 仍要编译，因为它的 `define_class` / `remote`
// 是 main 层应用层共用的代理机制，只是不再从 init 导出 Controller。
#[cfg(all(feature = "client", not(test)))]
mod client;
#[cfg(all(feature = "main-client", not(test)))]
mod main_client;
#[cfg(all(feature = "server", not(test)))]
mod server;
mod transport;

#[cfg(not(test))]
#[napi_derive::module_exports]
fn init(mut exports: napi::JsObject, mut env: napi::Env) -> napi::Result<()> {
    let state = binding::State::new(env);
    #[cfg(all(feature = "client", not(feature = "main-client")))]
    client::init(&state, &mut exports)?;
    #[cfg(feature = "main-client")]
    main_client::init(&state, &mut exports)?;
    #[cfg(feature = "server")]
    server::init(&state, &mut exports)?;
    env.add_env_cleanup_hook(state, |state| state.cleanup())?;
    Ok(())
}
