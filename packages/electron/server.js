"use strict";
var __create = Object.create;
var __defProp = Object.defineProperty;
var __getOwnPropDesc = Object.getOwnPropertyDescriptor;
var __getOwnPropNames = Object.getOwnPropertyNames;
var __getProtoOf = Object.getPrototypeOf;
var __hasOwnProp = Object.prototype.hasOwnProperty;
var __copyProps = (to, from, except, desc) => {
  if (from && typeof from === "object" || typeof from === "function") {
    for (let key of __getOwnPropNames(from))
      if (!__hasOwnProp.call(to, key) && key !== except)
        __defProp(to, key, { get: () => from[key], enumerable: !(desc = __getOwnPropDesc(from, key)) || desc.enumerable });
  }
  return to;
};
var __toESM = (mod, isNodeMode, target) => (target = mod != null ? __create(__getProtoOf(mod)) : {}, __copyProps(
  // If the importer is in node compatibility mode or this is not an ESM
  // file that has been converted to a CommonJS file using a Babel-
  // compatible transform (i.e. "__esModule" has not been set), then set
  // "default" to the CommonJS "module.exports" for node compatibility.
  isNodeMode || !mod || !mod.__esModule ? __defProp(target, "default", { value: mod, enumerable: true }) : target,
  mod
));
const path = require("node:path");
const electron = require("electron");
const _console = {
  debug: console.debug,
  info: console.info,
  warn: console.warn,
  error: console.error,
  trace: console.trace
};
const color = {
  red: 31,
  green: 32,
  yellow: 33,
  blue: 34,
  white: 37
};
const Styles = [
  `\x1B[${color.white}m%s\x1B[0m`,
  `\x1B[${color.blue}m%s\x1B[0m`,
  `\x1B[${color.green}m%s\x1B[0m`,
  `\x1B[${color.yellow}m%s\x1B[0m`,
  `\x1B[${color.red}m%s\x1B[0m`
];
const Methods = [
  "trace",
  "debug",
  "info",
  "warn",
  "error"
];
const CurrentLogLevel = 1;
class Logger {
  constructor(namespace = "unknown") {
    this.beforeFuncs = [];
    this.afterFuncs = [];
    this.config = {
      namespace: ""
    };
    this.config.namespace = `[${namespace}]`;
  }
  /**
   * 创建新的 Logger 实例
   *
   * @param namespace 命名空间
   * @returns Logger
   */
  create(namespace = "unknown") {
    return new Logger(namespace);
  }
  _log(level, args) {
    if (level < CurrentLogLevel) return;
    this.beforeFuncs.forEach((e) => e(this.config));
    const now = /* @__PURE__ */ new Date();
    const fix = (d, len = 2) => (d + "").padStart(len, "0");
    const time = `${now.getFullYear()}-${fix(now.getMonth() + 1)}-${fix(now.getDate())} ${fix(now.getHours())}:${fix(now.getMinutes())}:${fix(now.getSeconds())}.${fix(now.getMilliseconds(), 3)}`;
    _console[Methods[level]](`[${time}] [${Styles[level]}] ${this.config.namespace}`, Methods[level], ...args);
    this.afterFuncs.forEach((e) => e(this.config));
  }
  /**
   * 添加拦截器
   * @param func 拦截器
   * @param isBefore 是否日志之前
   * @returns this
   */
  addInterceptor(func, isBefore = true) {
    if (typeof func !== "function")
      return this.error("拦截器函数不符合规范");
    if (isBefore) {
      this.beforeFuncs.push(func);
      return this;
    }
    this.afterFuncs.push(func);
    return this;
  }
  /**
   * 添加日志打印之前的拦截函数
   *
   * @param func 拦截器
   * @returns this
   */
  addBeforeInterceptor(func) {
    this.beforeFuncs.push(func);
    return this;
  }
  /**
   * 添加日志打印之后的拦截函数
   *
   * @param func 拦截器
   * @returns this
   */
  addAfterInterceptor(func) {
    this.afterFuncs.push(func);
    return this;
  }
  /**
   * 打印追踪信息 🐛
   *
   * @param args 任意参数
   */
  trace(...args) {
    this._log(0, args);
    return this;
  }
  /**
   * 打印调试信息 🐛
   *
   * @param args 任意参数
   */
  debug(...args) {
    this._log(1, args);
    return this;
  }
  /**
   * 打印输出信息 🐛
   *
   * @param args 任意参数
   */
  info(...args) {
    this._log(2, args);
    return this;
  }
  /**
   * 打印输出警告信息 ❕
   *
   * @param args 任意参数
   */
  warn(...args) {
    this._log(3, args);
    return this;
  }
  /**
   * 打印输出错误信息 ❌
   *
   * @param args 任意参数
   */
  error(...args) {
    this._log(4, args);
    return this;
  }
  /**
   * 设置命名空间（日志前缀）
   * @param namespace
   */
  setNamespace(namespace = "") {
    this.config.namespace = `[${namespace}]`;
    return this;
  }
}
const useLogger = (namespace) => {
  return new Logger(namespace);
};
let dialogCallback = null;
class Controller {
  constructor() {
    const container = document.getElementById("container");
    if (!container) {
      throw new Error("Container element not found");
    }
    const webview = document.createElement("webview");
    this._webview = webview;
    this.container = container;
    const { removeInstanceOfType } = useInstanceManage();
    removeInstanceOfType("Controller");
  }
  get webview() {
    return this._webview;
  }
  mount() {
    if (this._webview) {
      this.container.hasChildNodes() && this.container.childNodes.forEach((child) => {
        this.container.removeChild(child);
      });
      this.container.appendChild(this._webview);
    } else {
      throw new Error("Webview is not initialized");
    }
  }
  unmount() {
    if (this._webview) {
      this.container.removeChild(this._webview);
      this._webview = null;
    }
  }
  setDialogCallback(callback) {
    if (callback !== null && callback !== void 0 && typeof callback !== "function") {
      throw new TypeError("Dialog callback must be a function");
    }
    console.info("[Controller] set dialog callback", { registered: typeof callback === "function" });
    dialogCallback = callback || null;
  }
  /**
   * Dispatch a dialog request without waiting on the callback. The original
   * guest renderer remains blocked in sendSync until resolveDialog is called.
   */
  dialog(webview, request) {
    if (!dialogCallback) return false;
    const callback = dialogCallback;
    console.info("[Controller] dispatch dialog", request);
    queueMicrotask(() => {
      try {
        callback(webview, request.requestId, request.type, ...request.args);
      } catch (error) {
        console.error("[Controller] dialog callback failed", request, error);
        globalThis.__skylineResolveDialog?.({
          requestId: request.requestId,
          error: error instanceof Error ? error.message : String(error)
        });
      }
    });
    return true;
  }
  resolveDialog(requestId, result) {
    if (!requestId || typeof requestId !== "string") {
      throw new TypeError("Dialog requestId must be a non-empty string");
    }
    console.info("[Controller] resolve dialog", { requestId, result });
    globalThis.__skylineResolveDialog?.({ requestId, result });
  }
}
useLogger("CustomHandle");
const useCustomHandle = () => ({
  getSkylineAddonPath: () => {
    const buildPath = require.cache[require.resolve("skyline-addon/build/skyline.node")]?.path;
    if (!buildPath) {
      throw new Error("skyline-addon not found");
    }
    return path.resolve(buildPath, "..");
  }
});
function createObjectManage(clazzMap2 = /* @__PURE__ */ new Map()) {
  return {
    getClazz: (name) => clazzMap2.get(name),
    setClazz: (name, clazz) => {
      clazzMap2.set(name, clazz);
    },
    removeClazz: (name) => {
      clazzMap2.delete(name);
    },
    clearClazz: () => clazzMap2.clear(),
    getAllClazz: () => Array.from(clazzMap2.values())
  };
}
function createInstanceManage(instanceMap2 = /* @__PURE__ */ new Map()) {
  let objectIds = /* @__PURE__ */ new WeakMap();
  const primitiveIds = /* @__PURE__ */ new Map();
  let nextInstanceId = 1;
  const identityMap = (value) => typeof value === "object" && value !== null || typeof value === "function" ? objectIds : primitiveIds;
  const removeInstance = (id) => {
    const instance = instanceMap2.get(id);
    if (!instanceMap2.delete(id)) return;
    const ids = identityMap(instance);
    if (ids.get(instance) !== id) return;
    ids.delete(instance);
    for (const [otherId, value] of instanceMap2) {
      if (value === instance) {
        ids.set(instance, otherId);
        break;
      }
    }
  };
  return {
    getInstance: (id) => instanceMap2.get(id),
    getInstanceId: (instance) => identityMap(instance).get(instance) ?? null,
    get instanceCount() {
      return instanceMap2.size;
    },
    setInstance(instance) {
      const id = nextInstanceId++;
      instanceMap2.set(id, instance);
      const ids = identityMap(instance);
      if (!ids.has(instance)) ids.set(instance, id);
      return id;
    },
    removeInstance,
    removeInstanceOfType(type) {
      for (const [id, instance] of instanceMap2) {
        if (instance?.constructor?.name !== type) continue;
        instanceMap2.delete(id);
        const ids = identityMap(instance);
        if (ids.get(instance) === id) ids.delete(instance);
      }
    },
    clearInstance() {
      instanceMap2.clear();
      primitiveIds.clear();
      objectIds = /* @__PURE__ */ new WeakMap();
    }
  };
}
const clazzMap = /* @__PURE__ */ new Map();
const instanceMap = /* @__PURE__ */ new Map();
globalThis.clazzMap = clazzMap;
globalThis.instanceMap = instanceMap;
const objects = createObjectManage(clazzMap);
const instances = createInstanceManage(instanceMap);
const useObjectManage = () => objects;
const useInstanceManage = () => instances;
const registerDefaultClazz = (g) => {
  objects.setClazz("Controller", Controller);
  objects.setClazz("global", g);
  objects.setClazz("customHandle", useCustomHandle());
  objects.setClazz("functionData", {});
};
const log$2 = useLogger("Callback");
function createCallbackManage() {
  const callbacks2 = /* @__PURE__ */ new Map();
  return {
    getCallback(callbackId, callback) {
      const existing = callbacks2.get(callbackId);
      if (existing) return existing;
      callbacks2.set(callbackId, callback);
      log$2.debug("callback registered", callbackId);
      return callback;
    },
    clearCallback: () => callbacks2.clear()
  };
}
const callbacks = createCallbackManage();
const useCallback = () => callbacks;
const rendererContext = {
  instances: useInstanceManage(),
  objects: useObjectManage(),
  callbacks: useCallback(),
  server: {
    sendMessageSingle: (body, messageId) => global.send(body, messageId),
    sendMessageSync: (body) => global.sendMessageSync(body)
  },
  renderer: true
};
const remoteInstanceTypes = {
  CSSStyleDeclaration: "CSSStyleDeclaration",
  ChromeWebViewElement: "ChromeWebViewElement",
  WebViewElement: "ChromeWebViewElement",
  WebRequestEvent: "WebRequestEvent",
  Event: "Event",
  Controller: "Controller"
};
const log$1 = useLogger("HookArgument");
const getRemoteInstanceType = (instance, context) => {
  if (context.renderer) return remoteInstanceTypes[instance?.constructor?.name];
  if (instance === null || typeof instance !== "object") return void 0;
  const prototype = Object.getPrototypeOf(instance);
  if (prototype === Object.prototype || prototype === null) return void 0;
  const name = instance.constructor?.name;
  return typeof name === "string" && name !== "" ? name : "Object";
};
const hookCallbackArgument = (arg, context) => {
  if (!context.renderer) return hookResult("callbackArgument", arg, context);
  if (Array.isArray(arg)) {
    for (let i = 0; i < arg.length; i++) {
      arg[i] = hookCallbackArgument(arg[i], context);
    }
  } else if (typeof arg === "object") {
    const name = arg?.constructor?.name;
    const instanceType = getRemoteInstanceType(arg, context);
    if (instanceType) {
      const { getInstanceId, setInstance } = context.instances;
      arg = {
        instanceId: getInstanceId(arg) ?? setInstance(arg),
        instanceType
      };
    } else {
      if (!global.clazzSet) global.clazzSet = /* @__PURE__ */ new Set();
      global.clazzSet.add(name);
      log$1.warn("hookCallbackArgument type not found!", name, arg);
      for (const key in arg) {
        arg[key] = hookCallbackArgument(arg[key], context);
      }
    }
  }
  return arg;
};
const hookArgumentItem = (action, arg, context) => {
  if (!arg) return arg;
  if (Array.isArray(arg)) {
    for (let i = 0; i < arg.length; i++) {
      arg[i] = hookArgumentItem(action, arg[i], context);
    }
  } else if (typeof arg === "object") {
    if (Object.prototype.hasOwnProperty.call(arg, "instanceId")) {
      const instance = context.instances.getInstance(arg.instanceId);
      if (instance === void 0) {
        const label = context.renderer ? "Instance not found" : "InstanceId not found";
        throw new Error(`${label}: ${arg.instanceId}`);
      }
      arg = instance;
    } else if (Object.prototype.hasOwnProperty.call(arg, "callbackId") && (context.renderer || typeof arg.callbackId === "number")) {
      const callbackId = arg.callbackId;
      const dialogCallback2 = context.renderer && action === "setDialogCallback";
      const asyncCallback = (context.renderer ? Boolean(arg.asyncCallback) : arg.asyncCallback === true) || dialogCallback2;
      const callback = (...args) => {
        const body = JSON.stringify({
          type: "emitCallback",
          callbackId,
          data: {
            args: hookCallbackArgument(args, context),
            block: !asyncCallback
          }
        });
        if (asyncCallback) {
          context.server.sendMessageSingle(body, context.renderer ? void 0 : 0);
          return;
        }
        const result = context.server.sendMessageSync(body);
        return context.renderer ? hookResult(`${action}_syncResult`, result, context) : result;
      };
      if (context.renderer && arg.__worklet) {
        callback.asString = arg.asString;
        callback.__workletHash = arg.__workletHash;
        callback.__location = arg.__location;
        callback.__worklet = arg.__worklet;
        callback._closure = hookArgumentItem(action, arg._closure, context);
      }
      arg = dialogCallback2 ? callback : context.callbacks.getCallback(callbackId, callback);
    } else {
      for (const key of Object.keys(arg)) {
        arg[key] = hookArgumentItem(action, arg[key], context);
      }
    }
  }
  return arg;
};
const hookArgument = (action, args, context = rendererContext) => {
  args = hookArgumentItem(action, args, context);
  if (!context.renderer) return args;
  if (action === "createWindow") {
    const sharedMemory = require("sharedMemory/sharedMemory.node");
    args[6] = sharedMemory.getMemory(args[6]);
  } else if (action === "notifyHttpRequestComplete") {
    const sharedMemory = require("sharedMemory/sharedMemory.node");
    args[4] = new Uint8Array(sharedMemory.getMemory(args[4]));
  } else if (action === "notifyResourceLoad") {
    args[1] = new Uint8Array(args[1]);
  } else if (action === "registerEventHandler") {
    console.info("registerEventHandler", args);
  }
  return args;
};
let functionDataId = 1;
const hookResult = (action, result, context = rendererContext, ancestors = /* @__PURE__ */ new Set()) => {
  if (context.renderer) log$1.debug("result before:", result);
  if (context.renderer && action === "setLoadResourceCallback_syncResult") {
    return new Uint8Array(result);
  }
  if (typeof result === "function") {
    const id = context.functions ? context.functions.getInstanceId(result) ?? context.functions.setInstance(result) : functionDataId++;
    context.objects.getClazz("functionData")[id] = result;
    return { instanceId: id, instanceType: "function" };
  }
  if (!context.renderer) {
    if (result === null || result === void 0) return null;
    const type = typeof result;
    if (type === "string" || type === "number" || type === "boolean") return result;
    if (type === "bigint") return Number(result);
    if (type !== "object") return null;
    if (result.instanceId !== void 0) return { instanceId: result.instanceId };
    if (Buffer.isBuffer(result)) return [...result];
    if (result instanceof ArrayBuffer) return [...new Uint8Array(result)];
  }
  if (Array.isArray(result)) {
    if (!context.renderer) return result.map((item) => hookResult(action, item, context, ancestors));
    for (let i = 0; i < result.length; i++) {
      const element = result[i];
      const instanceType = getRemoteInstanceType(element, context);
      if (instanceType) {
        result[i] = {
          instanceId: context.instances.setInstance(element),
          instanceType
        };
      } else {
        if (!global.clazzSet) global.clazzSet = /* @__PURE__ */ new Set();
        global.clazzSet.add(element?.constructor?.name);
      }
    }
  } else if (typeof result === "object") {
    const instanceType = getRemoteInstanceType(result, context);
    if (instanceType) {
      const { getInstanceId, setInstance } = context.instances;
      const id = context.renderer ? setInstance(result) : getInstanceId(result) ?? setInstance(result);
      return { instanceId: id, instanceType };
    }
    if (context.renderer) {
      if (action === "request_propertyResult_onMessage_propertyResult") {
        return { instanceId: context.instances.setInstance(result), instanceType: "RequestMessageEvent" };
      }
      if (action === "request_propertyResult_onRequest_propertyResult") {
        return { instanceId: context.instances.setInstance(result), instanceType: "RequestRule" };
      }
      if (!global.clazzSet) global.clazzSet = /* @__PURE__ */ new Set();
      global.clazzSet.add(result?.constructor?.name);
    } else {
      if (ancestors.has(result)) throw new Error("Cannot serialize a circular result");
      if (ancestors.size >= 128) throw new Error("Result nesting exceeds 128 levels");
      ancestors.add(result);
    }
    const output = {};
    for (const key in result) {
      if (context.renderer || Object.prototype.hasOwnProperty.call(result, key)) {
        output[key] = hookResult(`${action}_${key}_propertyResult`, result[key], context, ancestors);
      }
    }
    if (!context.renderer) ancestors.delete(result);
    return output;
  }
  return result;
};
const log = useLogger("Server");
try {
  log.info("Hi rpc server!");
  log.info(process.version);
  process.on("unhandledRejection", (err) => {
    log.error("unhandledRejection:", err);
  });
  const server = require("skyline-server/render-server.node");
  global.sendMessageSync = server.sendMessageSync;
  global.send = server.sendMessageSingle;
  global.blockUntilNextMessage = server.blockUntilNextMessage;
  global.controller = new Controller();
  global.__skylineResolveDialog = (response) => {
    log.info("Sending dialog response", response);
    electron.ipcRenderer.send("skyline-dialog-response", response);
  };
  const findWebviewForDialog = (guestWebContentsId) => {
    for (const webview of Array.from(document.querySelectorAll("webview"))) {
      try {
        if (webview.getWebContentsId() === guestWebContentsId) return webview;
      } catch {
      }
    }
    return void 0;
  };
  electron.ipcRenderer.on("skyline-dialog-request", (_event, request) => {
    log.info("Received dialog request", request);
    try {
      const webview = findWebviewForDialog(request?.guestWebContentsId);
      if (!webview) {
        electron.ipcRenderer.send("skyline-dialog-response", {
          requestId: request?.requestId,
          error: "Skyline dialog target webview is unavailable"
        });
        return;
      }
      const handled = global.controller.dialog(webview, request);
      if (!handled) {
        electron.ipcRenderer.send("skyline-dialog-response", {
          requestId: request.requestId,
          error: "Skyline dialog callback is not registered"
        });
      }
    } catch (error) {
      electron.ipcRenderer.send("skyline-dialog-response", {
        requestId: request?.requestId,
        error: error?.message || String(error)
      });
    }
  });
  const g = global;
  g.window = g;
  window = g;
  registerDefaultClazz(g);
  const port = 3001;
  server.start("127.0.0.1", port);
  server.setMessageCallback((message, messageId) => {
    let replied = false;
    const reply = (payload) => {
      if (replied || messageId <= 0) return;
      replied = true;
      console.info("Reply message <====", payload, messageId);
      global.send(JSON.stringify(payload), messageId);
    };
    const req = JSON.parse(message);
    if (req.action === "disconnected") {
      global.controller.setDialogCallback(null);
      log.error("disconnected");
      return;
    }
    try {
      log.info(`Received message => ${message}`);
      if (req.type === "constructor") {
        const { getClazz } = useObjectManage();
        const clazz = getClazz(req.clazz);
        if (!clazz) {
          log.error("Class not found", req.clazz);
          reply({ error: "Class not found" });
          return;
        }
        const { setInstance } = useInstanceManage();
        const params = req.data.params || [];
        const instanceId = setInstance(new clazz(...params));
        log.debug("constructor end", req.clazz, instanceId);
        reply({ result: { instanceId } });
      } else if (req.type == "static") {
        const { getClazz } = useObjectManage();
        const clazz = getClazz(req.clazz);
        if (!clazz) {
          reply({ error: "Class not found" });
          return;
        }
        if (typeof clazz[req.action] === "function") {
          const params = req.data.params || [];
          hookArgument(req.action, params);
          log.debug("static call", req.action, params);
          let result = clazz[req.action](...params);
          result = hookResult(`${req.action}_staticResult`, result);
          log.debug("static call result", req.action, result);
          reply({ result: { returnValue: result } });
        } else if (typeof clazz[req.action] === "object") {
          let result = clazz[req.action];
          result = hookResult(`${req.action}_staticResult`, result);
          reply({ result: { returnValue: result } });
        } else if (typeof clazz[req.action] !== "undefined") {
          const result = clazz[req.action];
          reply({ result: { returnValue: result } });
        } else {
          log.error("Method not found or instance invalid", req.action, clazz[req.action]);
          reply({ error: "Method not found or instance invalid" });
        }
      } else if (req.type == "dynamic") {
        if (!req.data.instanceId) {
          log.error("InstanceId not found");
          reply({ error: "InstanceId not found" });
          return;
        }
        const { getInstance } = useInstanceManage();
        const instance = getInstance(req.data.instanceId);
        if (instance && typeof instance[req.action] === "function") {
          const params = req.data.params || [];
          log.debug("dynamic call", instance, req.action, params);
          hookArgument(req.action, params);
          const finishDynamicCall = (value) => {
            log.debug("dynamic call result", req.action, value);
            const result2 = hookResult(`${req.action}_dynamicResult`, value);
            log.debug("dynamic call result hooked", req.action, result2);
            if (messageId > 0) {
              reply({
                result: {
                  returnValue: result2
                }
              });
              if (req.action === "matches" && result2 === true) {
                console.info("matches:", instance, params, result2);
              } else if (req.action === "appendCompiledStyleSheets") {
                global.blockUntilNextMessage();
              }
            }
          };
          const result = instance[req.action](...params);
          if (result && typeof result.then === "function") {
            Promise.resolve(result).then(finishDynamicCall, (error) => {
              log.error("Async dynamic call failed:", req.action, error);
              reply({ error: error?.message || String(error) });
            });
          } else {
            finishDynamicCall(result);
          }
        } else {
          console.error("Method not found or instance invalid", req.action, instance[req.action]);
          reply({ error: "Method not found or instance invalid" });
        }
      } else if (req.type == "dynamicProperty") {
        if (!req.data.instanceId) {
          console.error("InstanceId not found");
          reply({ error: "InstanceId not found" });
          return;
        }
        const { getInstance } = useInstanceManage();
        const instance = getInstance(req.data.instanceId);
        console.debug("dynamic property", req.action, instance, req.data);
        const type = req.data.propertyAction;
        const params = req.data.params || [];
        log.debug("dynamic property", req.action, params);
        hookArgument(req.action, params);
        let result = void 0;
        if (type === "set") {
          instance[req.action] = params[0];
          log.debug("dynamic property set", req.action, params[0]);
        } else if (type === "get") {
          log.debug("dynamic property get", req.action, instance[req.action]);
          result = instance[req.action];
        }
        log.debug("dynamic property result", req.action, result);
        result = hookResult(`${req.action}_propertyResult`, result);
        log.debug("dynamic property result hooked", req.action, result);
        if (messageId > 0) {
          reply({
            result: {
              returnValue: result
            }
          });
        }
      } else {
        reply({ error: "Request type not recognized" });
      }
    } catch (err) {
      log.error("Error:", err);
      if (messageId > 0 && !replied) {
        reply({ error: err.message });
      }
    }
  });
  log.info(`✅ WebSocket Server listening on ws://localhost:${port}`);
  log.info("end....");
} catch (err) {
  log.error("Error:", err);
}
