#include "./controller.hh"
#include "./html/controller.hh"
#include "../common/convert.hh"
#include "client_action.hh"
#include <spdlog/spdlog.h>
#include <string>


namespace Webview {
void Init(Napi::Env env, Napi::Object exports) {

    auto func = HTML::Controller::GetClazz(env);
    env.SetInstanceData(func);
    exports.Set("Controller", func->Value());

}
} // namespace Webview
