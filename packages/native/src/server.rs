use crate::binding::{error, function, invalid, port, Reference, State};
use napi::{JsObject, Result};
use std::rc::Rc;

pub fn init(state: &Rc<State>, exports: &mut JsObject) -> Result<()> {
    let weak = Rc::downgrade(state);
    exports.set_named_property(
        "start",
        state
            .env
            .create_function_from_closure("start", move |ctx| {
                let state = weak
                    .upgrade()
                    .ok_or_else(|| error("Environment has closed"))?;
                let host = ctx.get::<String>(0)?;
                let port = port(&ctx, 1)?;
                if state.endpoint.borrow().is_some() {
                    return Err(error("Server is already started"));
                }
                let endpoint = state.make_endpoint()?;
                endpoint.listen(&host, port).map_err(error)?;
                *state.endpoint.borrow_mut() = Some(endpoint);
                ctx.env.create_uint32(0)
            })?,
    )?;

    let weak = Rc::downgrade(state);
    exports.set_named_property(
        "stop",
        state.env.create_function_from_closure("stop", move |ctx| {
            if let Some(state) = weak.upgrade() {
                state.close();
            }
            ctx.env.get_undefined()
        })?,
    )?;

    let weak = Rc::downgrade(state);
    exports.set_named_property(
        "setMessageCallback",
        state
            .env
            .create_function_from_closure("setMessageCallback", move |ctx| {
                let state = weak
                    .upgrade()
                    .ok_or_else(|| error("Environment has closed"))?;
                let callback = function(&ctx, 0)?;
                *state.message_callback.borrow_mut() = Some(Reference::new(state.env, callback)?);
                state.drain()?;
                ctx.env.get_undefined()
            })?,
    )?;

    let weak = Rc::downgrade(state);
    exports.set_named_property(
        "sendMessageSync",
        state
            .env
            .create_function_from_closure("sendMessageSync", move |ctx| {
                let state = weak
                    .upgrade()
                    .ok_or_else(|| error("Environment has closed"))?;
                let body = ctx.get::<String>(0)?;
                if state.message_callback.borrow().is_none() {
                    return Err(error("Message callback is not registered"));
                }
                let result = state.request(&body)?;
                state.decode(&result)
            })?,
    )?;

    let weak = Rc::downgrade(state);
    exports.set_named_property(
        "sendMessageSingle",
        state
            .env
            .create_function_from_closure("sendMessageSingle", move |ctx| {
                let state = weak
                    .upgrade()
                    .ok_or_else(|| error("Environment has closed"))?;
                let body = ctx.get::<String>(0)?;
                let id = if ctx.length > 1 {
                    ctx.get::<napi::JsNumber>(1)?.get_double()?
                } else {
                    0.0
                };
                if !id.is_finite()
                    || id.fract() != 0.0
                    || !(0.0..=9_007_199_254_740_991.0).contains(&id)
                {
                    return Err(invalid("Message ID must be a non-negative safe integer"));
                }
                let endpoint = state.socket()?;
                endpoint
                    .send(id as u64, &body, endpoint.generation())
                    .map_err(error)?;
                ctx.env.get_undefined()
            })?,
    )?;

    let weak = Rc::downgrade(state);
    exports.set_named_property(
        "blockUntilNextMessage",
        state
            .env
            .create_function_from_closure("blockUntilNextMessage", move |ctx| {
                let state = weak
                    .upgrade()
                    .ok_or_else(|| error("Environment has closed"))?;
                state.socket()?.wait_for_message().map_err(error)?;
                ctx.env.get_undefined()
            })?,
    )?;
    Ok(())
}
