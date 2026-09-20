import { createInstanceManage, createObjectManage } from '../common/object-manage';
import type { InstanceManage, ObjectManage } from '../common/object-manage';
import { createCallbackManage } from '../common/callback';
import { hookArgument, hookResult } from '../common/hook-argument';
import type { HookContext } from '../common/hook-argument';
import type { NativeRpcServer, RpcRequest } from '../common/rpc';
import { registerDefaultClazz } from './object-manage';

const DEFAULT_HOST = '127.0.0.1';
const DEFAULT_PORT = 3002;

export interface MainProcessOptions {
  electron?: any;
  host?: string;
  port?: number;
  server?: NativeRpcServer;
}

// Dotted Electron actions must retain their owner as the method receiver.
function resolveStatic(root: any, action: string) {
  let owner = root;
  let value = root;
  for (const key of action.split('.')) {
    owner = value;
    value = value?.[key];
    if (value === undefined) break;
  }
  return { owner, value };
}

/**
 * main 进程应用层控制器。
 *
 * 与 render 进程的 Controller（render-process/controller.ts，协议级注册、由客户端 `new Controller()`
 * 实例化的 DOM webview 包装类）实现不同：main 侧没有协议级 Controller，客户端拿到的
 * `mainController.electron.*` 是一个纯客户端对象，服务端的对应物就是这里——持有 Electron 命名空间
 * 与分发循环。
 *
 * 每个实例自带一套 object / instance / function / callback 表，所以多个 Controller 之间互不串状态
 * （见 test/rpc-modules.test.ts 的隔离用例）。
 */
export class Controller {
  private rpc: NativeRpcServer;
  private host: string;
  private port: number | string;
  private objects: ObjectManage;
  private instances: InstanceManage;
  private functions: InstanceManage;
  private callbacks: ReturnType<typeof createCallbackManage>;
  private functionData: Record<string, Function> = {};
  private hookContext: HookContext;
  private listening = false;

  constructor(options: MainProcessOptions = {}) {
    const electronModule = options.electron || require('electron');
    this.rpc = options.server || require('skyline-server/render-server.node');
    this.host = options.host || DEFAULT_HOST;
    this.port = options.port || process.env.SKYLINE_MAIN_RPC_PORT || DEFAULT_PORT;
    this.objects = createObjectManage();
    this.instances = createInstanceManage();
    this.functions = createInstanceManage();
    this.callbacks = createCallbackManage();
    registerDefaultClazz(this.objects, electronModule, this.functionData);
    this.hookContext = {
      server: this.rpc,
      instances: this.instances,
      objects: this.objects,
      functions: this.functions,
      callbacks: this.callbacks,
    };
  }

  private clearConnection() {
    this.instances.clearInstance();
    this.functions.clearInstance();
    this.callbacks.clearCallback();
    for (const id of Object.keys(this.functionData)) delete this.functionData[id];
  }

  // Arrow property: setMessageCallback receives the function unbound, so `this` must be captured here.
  private dispatch = (body: string, messageId: number) => {
    console.info('[main-rpc] dispatch received', body)
    let replied = false;
    const reply = (payload: any) => {
      if (replied || messageId <= 0) return;
      replied = true;
      this.rpc.sendMessageSingle(JSON.stringify(payload), messageId);
    };
    const replyWith = (operation: () => any) => {
      const finish = (value: any) => reply({ result: { returnValue: hookResult(request.action, value, this.hookContext) } });
      const fail = (error: any) => {
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

    let request: RpcRequest;
    try {
      request = JSON.parse(body);
    } catch (error) {
      console.error('[main-rpc] invalid request body', body, error);
      reply({ error: 'Request body is not valid JSON' });
      return;
    }
    if (request.action === 'disconnected') {
      this.clearConnection();
      console.info('[main-rpc] client disconnected');
      return;
    }

    const data = request.data || {};
    let params: any[];
    try {
      params = hookArgument(request.action, data.params || [], this.hookContext);
    } catch (error: any) {
      console.error('[main-rpc] invalid params', request.type, request.action, error);
      reply({ error: error?.message || String(error) });
      return;
    }
    const instance = data.instanceId === undefined ? undefined : this.instances.getInstance(data.instanceId);
    const missingInstance = () => {
      console.error('[main-rpc] InstanceId not found', request.type, request.action, data.instanceId, 'instances size:', this.instances.instanceCount);
      reply({ error: 'InstanceId not found' });
    };

    if (request.type === 'constructor') {
      const clazz = this.objects.getClazz(request.clazz);
      if (!clazz) {
        reply({ error: 'Class not found' });
        return;
      }
      replyWith(() => {
        const value = new clazz(...params);
        return this.instances.getInstanceId(value) ?? this.instances.setInstance(value);
      });
      return;
    }
    if (request.type === 'static') {
      const clazz = this.objects.getClazz(request.clazz);
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

  start() {
    if (this.listening) return;
    this.rpc.setMessageCallback(this.dispatch);
    this.rpc.start(this.host, this.port);
    this.listening = true;
    console.info(`[main-rpc] listening on ${this.host}:${this.port}`);
  }

  stop() {
    if (!this.listening) return;
    this.rpc.stop();
    this.listening = false;
    this.clearConnection();
  }
}

export function createMainRpc(options: MainProcessOptions = {}) {
  return new Controller(options);
}

export function startMainRpc(options?: MainProcessOptions) {
  const rpc = createMainRpc(options);
  rpc.start();
  return rpc;
}
