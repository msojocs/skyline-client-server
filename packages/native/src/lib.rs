#![allow(dead_code)]

#[cfg(all(feature = "client", feature = "server"))]
compile_error!("Build client and server separately with --no-default-features");

#[cfg(not(test))]
mod binding;
#[cfg(all(feature = "client", not(test)))]
mod client;
#[cfg(all(feature = "server", not(test)))]
mod server;
mod transport;

#[cfg(not(test))]
#[napi_derive::module_exports]
fn init(mut exports: napi::JsObject, mut env: napi::Env) -> napi::Result<()> {
    let state = binding::State::new(env);
    #[cfg(feature = "client")]
    client::init(&state, &mut exports)?;
    #[cfg(feature = "server")]
    server::init(&state, &mut exports)?;
    env.add_env_cleanup_hook(state, |state| state.cleanup())?;
    Ok(())
}
