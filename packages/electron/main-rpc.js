'use strict';

// main 层 server 应用层。
//
// 与 render 的 server.ts 共用同一套 RPC：transport 仍是 native 的 render-server.node
// （start / stop / setMessageCallback / sendMessageSingle / blockUntilNextMessage），
// 请求 type（constructor / static / dynamic / dynamicProperty）与回复格式也完全一致，
// 差异只在应用层——这里注册的是 main 层的 Electron 命名空间，而不是 webview 那套 Controller。
//
//   devtools main 层（client）              Skyline main 进程（本文件，server）
//   mainController.electron
//     .webContents.fromId(e)          →     require('electron').webContents.fromId(e)
//       .loadURL(url)                 →       webContents.loadURL(url)
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
  return typeof name === 'string' && name !== '' ? name : 'Object';
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
  const instanceIds = new WeakMap();
  let nextInstanceId = 1;
  let listening = false;

  // 同一真实对象复用同一 instanceId，client 侧的代理才能保持同一性。
  const register = (value) => {
    const existing = instanceIds.get(value);
    if (existing !== undefined) return existing;
    const id = nextInstanceId++;
    instances.set(id, value);
    instanceIds.set(value, id);
    return id;
  };

  /** 把 main 层的真实值编成 wire 值：普通数据走 JSON，Electron 对象走 instanceId 代理。 */
  const encode = (value, ancestors = new Set()) => {
    if (value === null || value === undefined) return null;
    const type = typeof value;
    if (type === 'string' || type === 'number' || type === 'boolean') return value;
    if (type === 'bigint') return Number(value);
    // 与 binding.rs 的 encode 对齐：函数不做远端代理。
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

  const clazzMap = new Map([['electron', electronModule]]);

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
      console.info('[main-rpc] client disconnected');
      return;
    }

    const data = request.data || {};
    const params = data.params || [];
    const instance = instances.get(data.instanceId);
    const missingInstance = () => {
      console.error('[main-rpc] InstanceId not found', request.type, request.action, data.instanceId);
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
    },
  };
}

function startMainRpc(options) {
  const rpc = createMainRpc(options);
  rpc.start();
  return rpc;
}

module.exports = { createMainRpc, startMainRpc };
