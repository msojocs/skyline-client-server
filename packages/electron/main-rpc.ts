// Electron main-process integration is bundled by Vite. The RPC protocol is
// intentionally dynamic because it mirrors values crossing the native bridge.
// @ts-nocheck
'use strict';

// main 层 server 应用层。
//
// 与 render-server.ts 共用同一套 RPC：transport 仍是 native 的 render-server.node
// （start / stop / setMessageCallback / sendMessageSingle / blockUntilNextMessage），
// 请求 type（constructor / static / dynamic / dynamicProperty）与回复格式也完全一致，
// 差异只在应用层——这里注册的是 main 层的 Electron 命名空间，而不是 webview 那套 Controller。
//
//   devtools main 层（client）              Skyline main 进程（本文件，server）
//   mainController.electron
//     .webContents.fromId(e)          →     require('electron').webContents.fromId(e)
//       .loadURL(url)                 →       webContents.loadURL(url)
//       .session.webRequest[name](filter, listener)
//                                     →       ses.webRequest[name](filter, listener)
//
// 函数参数两个方向都要过桥：
//
// - client → server：参数里的 `{callbackId}` 还原成真实函数（`decodeArgument`），
//   函数被调用时把参数编成 emitCallback 发回 client 并取回返回值。
// - server → client：Electron 回调里拿到的函数（webRequest 的 callback 之类）编成
//   `{instanceId, instanceType: 'function'}` 交给 client，client 调用时走
//   `static / clazz "functionData"` 回到这里。
//
// 注意：client 侧的同步调用会阻塞它自己的 JS 线程。同进程（例如测试里把 server 跑在
// Worker）时，返回 Promise 的方法仍能被 resolve；若 client 与 server 同处一个线程，
// 异步方法会等到 client 的 RPC 超时。

const DEFAULT_HOST = '127.0.0.1';
const DEFAULT_PORT = 3002;

/** 普通数据对象走 JSON，其余对象（Electron 的类实例）一律代理成远端实例。 */
function isPlainData(value) {
  const prototype = Object.getPrototypeOf(value);
  return prototype === Object.prototype || prototype === null;
}

// 远端代理的 wire 类名取 constructor.name，必须与 client 侧 CLASSES 表里的 wire_name 一致
// （main_client.rs 里是 "WebContents"）。Electron 内部类名不稳定时，在这里做一次归一化。
function instanceTypeOf(value) {
  const name = value?.constructor?.name;
  if (typeof name === 'string' && name !== '') return name;
  return 'Object';
}

/**
 * 建一个 main 层 RPC 服务端。
 *
 * @param {object} [options]
 * @param {object} [options.electron] 注入的 electron 模块，默认 require('electron')。
 * @param {string} [options.host]
 * @param {number} [options.port] 默认 3002，避开 renderer 已占用的 3001。
 * @param {object} [options.server] 注入的 native RPC 模块，默认 render-server.node。
 */
function createMainRpc(options = {}) {
  const electronModule = options.electron || require('electron');
  const rpc = options.server || require('skyline-server/render-server.node');
  const host = options.host || DEFAULT_HOST;
  const port = options.port || process.env.SKYLINE_MAIN_RPC_PORT || DEFAULT_PORT;

  const instances = new Map();
  let instanceIds = new WeakMap();
  let nextInstanceId = 1;
  let listening = false;

  // 客户端用 {callbackId} 表示一个函数参数（见 binding.rs 的 encode_object）。这里把它还原成
  // 真实函数再交给 Electron（例如 webRequest 的监听器）；函数被调用时把参数编成 emitCallback
  // 发回客户端。同一个 callbackId 复用同一个函数，与客户端那边的函数身份保持一致。
  const callbacks = new Map();

  // 反方向：Electron 传给我们的函数（webRequest 的 callback 之类）编成
  // {instanceId, instanceType: 'function'}，客户端 remote() 会复活成可调用代理，调用落到
  // clazzMap 里的 functionData[instanceId]。
  const functionData = {};
  let functionIds = new WeakMap();
  let nextFunctionId = 1;

  // 同一真实对象复用同一 instanceId，client 侧的代理才能保持同一性。
  const register = (value) => {
    const existing = instanceIds.get(value);
    if (existing !== undefined) return existing;
    const id = nextInstanceId++;
    instances.set(id, value);
    instanceIds.set(value, id);
    return id;
  };

  // 函数单独一张表：客户端 functionData[id] 即调用服务端持有的函数，见 encode 的函数分支。
  const registerFunction = (value) => {
    const existing = functionIds.get(value);
    if (existing !== undefined) return existing;
    const id = nextFunctionId++;
    functionData[id] = value;
    functionIds.set(value, id);
    return id;
  };

  /** 客户端函数参数的本地替身：调用时把参数发回客户端；同步模式下取回返回值。 */
  const callbackFunction = (callbackId, asyncCallback) => {
    const existing = callbacks.get(callbackId);
    if (existing) return existing;
    const callback = (...args) => {
      const body = JSON.stringify({
        type: 'emitCallback',
        callbackId,
        data: { args: args.map((arg) => encode(arg)), block: !asyncCallback },
      });
      if (asyncCallback) {
        // 与 render server 的 hookArgument 一致：__asyncCallback 的回调不等结果。
        rpc.sendMessageSingle(body, 0);
        return undefined;
      }
      return rpc.sendMessageSync(body);
    };
    callbacks.set(callbackId, callback);
    return callback;
  };

  /** 把客户端编来的参数还原成服务端真实值：{callbackId} → 函数，{instanceId} → 之前的实例。 */
  const decodeArgument = (value) => {
    if (value === null || typeof value !== 'object') return value;
    if (Array.isArray(value)) return value.map((item) => decodeArgument(item));
    if (typeof value.callbackId === 'number') {
      return callbackFunction(value.callbackId, value.asyncCallback === true);
    }
    if (value.instanceId !== undefined) {
      const instance = instances.get(value.instanceId);
      if (instance === undefined) throw new Error(`InstanceId not found: ${value.instanceId}`);
      return instance;
    }
    const result = {};
    for (const [key, item] of Object.entries(value)) result[key] = decodeArgument(item);
    return result;
  };

  /** 把 main 层的真实值编成 wire 值：普通数据走 JSON，Electron 对象走 instanceId 代理。 */
  const encode = (value, ancestors = new Set()) => {
    if (value === null || value === undefined) return null;
    const type = typeof value;
    if (type === 'string' || type === 'number' || type === 'boolean') return value;
    if (type === 'bigint') return Number(value);
    // 函数代理成 functionData：与 render server 的 hookResult 对齐，客户端解码成可调用对象。
    if (type === 'function') {
      return { instanceId: registerFunction(value), instanceType: 'function' };
    }
    if (type !== 'object') return null;

    // 已经是远端代理的对象按现状透传，与 binding.rs 的 encode 一致。
    if (value.instanceId !== undefined) return { instanceId: value.instanceId };
    if (Buffer.isBuffer(value)) return [...value];
    if (value instanceof ArrayBuffer) return [...new Uint8Array(value)];
    if (Array.isArray(value)) return value.map((item) => encode(item, ancestors));
    if (!isPlainData(value)) {
      return { instanceId: register(value), instanceType: instanceTypeOf(value) };
    }

    if (ancestors.has(value)) throw new Error('Cannot serialize a circular result');
    if (ancestors.size >= 128) throw new Error('Result nesting exceeds 128 levels');
    ancestors.add(value);
    const result = {};
    for (const [key, item] of Object.entries(value)) result[key] = encode(item, ancestors);
    ancestors.delete(value);
    return result;
  };

  const clazzMap = new Map([
    ['electron', electronModule],
    // 客户端调用 functionData[id] 即调用服务端持有的函数，见 encode 的函数分支。
    ['functionData', functionData],
  ]);

  /** 支持 `webContents.fromId` 这类点分 action，并保留 owner 作为 this。 */
  const resolveStatic = (root, action) => {
    let owner = root;
    let value = root;
    for (const key of action.split('.')) {
      owner = value;
      value = value?.[key];
      if (value === undefined) break;
    }
    return { owner, value };
  };

  const dispatch = (body, messageId) => {
    let replied = false;
    const reply = (payload) => {
      if (replied || messageId <= 0) return;
      replied = true;
      rpc.sendMessageSingle(JSON.stringify(payload), messageId);
    };
    // 方法可能返回 Promise（如 executeJavaScript）；等它落地再回复。
    const replyWith = (operation) => {
      const finish = (value) => reply({ result: { returnValue: encode(value) } });
      const fail = (error) => {
        console.error('[main-rpc] dispatch failed', request.type, request.action, error);
        reply({ error: error?.message || String(error) });
      };
      try {
        const value = operation();
        if (value && typeof value.then === 'function') Promise.resolve(value).then(finish, fail);
        else finish(value);
      } catch (error) {
        fail(error);
      }
    };

    let request;
    try {
      request = JSON.parse(body);
    } catch (error) {
      console.error('[main-rpc] invalid request body', body, error);
      reply({ error: 'Request body is not valid JSON' });
      return;
    }
    if (request.action === 'disconnected') {
      instances.clear();
      callbacks.clear();
      for (const id of Object.keys(functionData)) delete functionData[id];
      // Remote IDs are valid only for the current connection. Rebuild the identity
      // caches as well, otherwise a reconnect can reuse an ID that no longer exists
      // in `instances` / `functionData`.
      instanceIds = new WeakMap();
      functionIds = new WeakMap();
      console.info('[main-rpc] client disconnected');
      return;
    }

    const data = request.data || {};
    // 参数里的 {callbackId} / {instanceId} 在交给 Electron 之前要还原成真实函数/实例。
    let params;
    try {
      params = decodeArgument(data.params || []);
    } catch (error) {
      console.error('[main-rpc] invalid params', request.type, request.action, error);
      reply({ error: error?.message || String(error) });
      return;
    }
    const instance = instances.get(data.instanceId);
    const missingInstance = () => {
      console.error('[main-rpc] InstanceId not found', request.type, request.action, data.instanceId, 'instances size:', instances.size);
      reply({ error: 'InstanceId not found' });
    };

    if (request.type === 'constructor') {
      const clazz = clazzMap.get(request.clazz);
      if (!clazz) {
        reply({ error: 'Class not found' });
        return;
      }
      replyWith(() => register(new clazz(...params)));
      return;
    }
    if (request.type === 'static') {
      const clazz = clazzMap.get(request.clazz);
      if (!clazz) {
        reply({ error: 'Class not found' });
        return;
      }
      const { owner, value } = resolveStatic(clazz, request.action);
      if (typeof value !== 'function') {
        reply({ error: 'Method not found or instance invalid' });
        return;
      }
      replyWith(() => value.apply(owner, params));
      return;
    }
    if (request.type === 'dynamicProperty') {
      if (instance === undefined) {
        missingInstance();
        return;
      }
      replyWith(() => {
        if (data.propertyAction === 'set') {
          instance[request.action] = params[0];
          return undefined;
        }
        return instance[request.action];
      });
      return;
    }
    if (request.type === 'dynamic') {
      if (instance === undefined) {
        missingInstance();
        return;
      }
      const method = instance[request.action];
      if (typeof method !== 'function') {
        reply({ error: 'Method not found or instance invalid' });
        return;
      }
      replyWith(() => method.apply(instance, params));
      return;
    }
    reply({ error: 'Request type not recognized' });
  };

  return {
    start() {
      if (listening) return;
      rpc.setMessageCallback(dispatch);
      rpc.start(host, port);
      listening = true;
      console.info(`[main-rpc] listening on ${host}:${port}`);
    },
    stop() {
      if (!listening) return;
      rpc.stop();
      listening = false;
      instances.clear();
      callbacks.clear();
      for (const id of Object.keys(functionData)) delete functionData[id];
      instanceIds = new WeakMap();
      functionIds = new WeakMap();
    },
  };
}

function startMainRpc(options) {
  const rpc = createMainRpc(options);
  rpc.start();
  return rpc;
}

export { createMainRpc, startMainRpc };
