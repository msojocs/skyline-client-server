#include "skyline_global.hh"
#include "include/page_context.hh"
#include "include/runtime.hh"
#include "napi.h"

namespace SkylineGlobal {
  void Init(Napi::Env env) {
    auto skylineGlobal = Napi::Object::New(env);
    {
      skylineGlobal.Set(Napi::String::New(env, "userAgent"), Napi::String::New(env, "skyline/1.4.0 (;8f190450e6301587ce41e08afcaa983db4dc712e;)"));
    }
    {
      auto features = Napi::Object::New(env);
      features.Set(Napi::String::New(env, "contextOperation"), Napi::Number::New(env, 1));
      features.Set(Napi::String::New(env, "eventDefaultPrevented"), Napi::Number::New(env, 1));
      skylineGlobal.Set(Napi::String::New(env, "features"), features);
    }
    {
      Skyline::PageContext::Init(env, skylineGlobal);
    }
    {
      Skyline::Runtime::Init(env, skylineGlobal);
    }
    env.Global().Set(Napi::String::New(env, "SkylineGlobal"), skylineGlobal);
  }
}
