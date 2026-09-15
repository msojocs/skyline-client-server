import { afterEach, expect, test, vi } from 'vitest';
import type {} from '../packages/typescript/src/global';
import { createCallbackManage } from '../packages/typescript/src/common/callback';
import { createInstanceManage, createObjectManage } from '../packages/typescript/src/common/object-manage';
import { hookArgument, hookResult } from '../packages/typescript/src/common/hook-argument';
import { createMainRpc } from '../packages/typescript/src/main-process/controller';
import { useCallback } from '../packages/typescript/src/render-process/callback';
import { registerDefaultClazz, useInstanceManage } from '../packages/typescript/src/render-process/object-manage';

afterEach(() => {
  useInstanceManage().clearInstance();
  useCallback().clearCallback();
  vi.restoreAllMocks();
  vi.unstubAllGlobals();
});

test.each([{}, () => {}, 'value', null, 7])('instance identities survive alias removal and expire on clear: %s', (value) => {
  const instances = createInstanceManage();
  const first = instances.setInstance(value);
  const second = instances.setInstance(value);
  expect(instances.getInstanceId(value)).toBe(first);
  instances.removeInstance(first);
  expect(instances.getInstanceId(value)).toBe(second);
  expect(instances.getInstance(second)).toBe(value);
  instances.clearInstance();
  expect(instances.getInstanceId(value)).toBeNull();
  expect(instances.getInstance(second)).toBeUndefined();
  expect(instances.setInstance(value)).toBeGreaterThan(second);
});

test('removing a class clears all its aliases and preserves unrelated instances', () => {
  class Controller {}
  const instances = createInstanceManage();
  const controller = new Controller();
  instances.setInstance(controller);
  instances.setInstance(controller);
  const unrelated = {};
  const unrelatedId = instances.setInstance(unrelated);
  instances.removeInstanceOfType('Controller');
  expect(instances.getInstanceId(controller)).toBeNull();
  expect(instances.instanceCount).toBe(1);
  expect(instances.getInstance(unrelatedId)).toBe(unrelated);
});

test('class registries and callback caches belong to their factory instances', () => {
  const first = createObjectManage();
  const second = createObjectManage();
  first.setClazz('electron', { name: 'first' });
  second.setClazz('electron', { name: 'second' });
  first.clearClazz();
  expect(second.getClazz('electron')).toEqual({ name: 'second' });
  const firstCallbacks = createCallbackManage();
  const secondCallbacks = createCallbackManage();
  const callback = () => 'first';
  const other = () => 'second';
  expect(firstCallbacks.getCallback(1, callback)).toBe(callback);
  expect(firstCallbacks.getCallback(1, other)).toBe(callback);
  expect(secondCallbacks.getCallback(1, other)).toBe(other);
  firstCallbacks.clearCallback();
  expect(firstCallbacks.getCallback(1, other)).toBe(other);
});

function fixture(electron: any) {
  let dispatch: (body: string, messageId: number) => void;
  let nextMessageId = 1;
  const server = {
    start: vi.fn(),
    stop: vi.fn(),
    blockUntilNextMessage: vi.fn(),
    setMessageCallback: vi.fn((callback: typeof dispatch) => { dispatch = callback; }),
    sendMessageSingle: vi.fn((_body: string, _messageId?: number) => {}),
    sendMessageSync: vi.fn((_body: string) => 'client-result'),
  };
  const rpc = createMainRpc({ electron, server });
  rpc.start();
  const request = (body: object) => {
    const messageId = nextMessageId++;
    dispatch(JSON.stringify(body), messageId);
    const reply = server.sendMessageSingle.mock.calls.find(([, id]) => id === messageId);
    return reply ? JSON.parse(reply[0]) : undefined;
  };
  const call = (action: string, params: any[] = [], clazz = 'electron') => request({
    type: 'static', clazz, action, data: { params },
  });
  return { rpc, server, request, call };
}

test('main servers isolate handles and route the same callback ID through their own transport', () => {
  class Target {
    constructor(public name: string) {}
    getName() { return this.name; }
  }
  const makeNamespace = (name: string) => ({
    target: new Target(name),
    getTarget() { return this.target; },
    listen: vi.fn(),
  });
  const firstApi = makeNamespace('first');
  const secondApi = makeNamespace('second');
  const first = fixture({ api: firstApi });
  const second = fixture({ api: secondApi });
  const firstHandle = first.call('api.getTarget').result.returnValue;
  const secondHandle = second.call('api.getTarget').result.returnValue;
  expect(firstHandle.instanceId).toBe(secondHandle.instanceId);
  first.call('api.listen', [{ callbackId: 1 }]);
  second.call('api.listen', [{ callbackId: 1 }]);
  firstApi.listen.mock.calls[0][0]('first');
  secondApi.listen.mock.calls[0][0]('second');
  expect(JSON.parse(first.server.sendMessageSync.mock.calls[0][0]).data.args).toEqual(['first']);
  expect(JSON.parse(second.server.sendMessageSync.mock.calls[0][0]).data.args).toEqual(['second']);
  first.request({ action: 'disconnected' });
  expect(second.request({
    type: 'dynamic', action: 'getName', data: { instanceId: secondHandle.instanceId },
  }).result.returnValue).toBe('second');
  first.rpc.stop();
  second.rpc.stop();
});

test.each(['disconnect', 'restart'])('main %s clears object, function and callback identities', (reset) => {
  vi.spyOn(console, 'error').mockImplementation(() => {});
  class Target {}
  const target = new Target();
  const fn = () => 'function-result';
  const listen = vi.fn();
  const { rpc, call, request, server } = fixture({ getTarget: () => target, getFunction: () => fn, listen });
  const object = call('getTarget').result.returnValue;
  const func = call('getFunction').result.returnValue;
  call('listen', [{ callbackId: 1 }]);
  const callback = listen.mock.calls[0][0];
  expect(call('getTarget').result.returnValue).toEqual(object);
  expect(call('getFunction').result.returnValue).toEqual(func);
  expect(call(String(func.instanceId), [], 'functionData').result.returnValue).toBe('function-result');
  if (reset === 'disconnect') request({ action: 'disconnected' });
  else { rpc.stop(); rpc.start(); }
  expect(request({
    type: 'dynamicProperty', action: 'name', data: { instanceId: object.instanceId, propertyAction: 'get' },
  }).error).toBe('InstanceId not found');
  expect(call(String(func.instanceId), [], 'functionData').error).toBe('Method not found or instance invalid');
  expect(call('getTarget').result.returnValue.instanceId).toBeGreaterThan(object.instanceId);
  expect(call('getFunction').result.returnValue.instanceId).toBeGreaterThan(func.instanceId);
  call('listen', [{ callbackId: 1 }]);
  expect(listen.mock.calls[1][0]).not.toBe(callback);
  const starts = server.start.mock.calls.length;
  rpc.start();
  expect(server.start).toHaveBeenCalledTimes(starts);
  rpc.stop();
  const stops = server.stop.mock.calls.length;
  rpc.stop();
  expect(server.stop).toHaveBeenCalledTimes(stops);
});

test('main hooks decode nested handles and encode asynchronous callback arguments', () => {
  class Target {}
  const target = new Target();
  const accept = vi.fn();
  const { rpc, call, server } = fixture({ getTarget: () => target, accept });
  const handle = call('getTarget').result.returnValue;
  call('accept', [{ nested: [handle, { callbackId: 9, asyncCallback: true }] }]);
  const [decoded, callback] = accept.mock.calls[0][0].nested;
  expect(decoded).toBe(target);
  const done = vi.fn(() => 'accepted');
  expect(callback({ target, bytes: Buffer.from([1, 2]), buffer: new Uint8Array([3, 4]).buffer, done })).toBeUndefined();
  expect(server.sendMessageSync).not.toHaveBeenCalled();
  const [body, messageId] = server.sendMessageSingle.mock.calls.at(-1)!;
  const message = JSON.parse(body);
  expect(messageId).toBe(0);
  expect(message).toMatchObject({ type: 'emitCallback', callbackId: 9, data: { block: false } });
  expect(message.data.args[0]).toMatchObject({ target: handle, bytes: [1, 2], buffer: [3, 4] });
  const remoteFunction = message.data.args[0].done;
  expect(call(String(remoteFunction.instanceId), [], 'functionData').result.returnValue).toBe('accepted');
  expect(done).toHaveBeenCalledOnce();
  rpc.stop();
});

test('renderer adapters retain webview handles, worklet metadata and replaceable async dialog callbacks', () => {
  class WebViewElement {}
  registerDefaultClazz(globalThis);
  const webview = new WebViewElement();
  const instanceId = useInstanceManage().setInstance(webview);
  const send = vi.fn();
  const sendSync = vi.fn((_body: string) => 'client-result');
  vi.stubGlobal('send', send);
  vi.stubGlobal('sendMessageSync', sendSync);
  const worklet: any[] = [{ callbackId: 1, __worklet: true, asString: 'worklet', _closure: { webview: { instanceId } } }];
  hookArgument('registerCallback', worklet);
  expect(worklet[0].__worklet).toBe(true);
  expect(worklet[0].asString).toBe('worklet');
  expect(worklet[0]._closure.webview).toBe(webview);
  expect(worklet[0](webview)).toBe('client-result');
  expect(JSON.parse(sendSync.mock.calls[0][0]).data.args).toEqual([{ instanceId, instanceType: 'ChromeWebViewElement' }]);
  expect(hookResult('webview_propertyResult', webview).instanceType).toBe('ChromeWebViewElement');
  const first: any[] = [{ callbackId: 1 }];
  const second: any[] = [{ callbackId: 1 }];
  hookArgument('setDialogCallback', first);
  hookArgument('setDialogCallback', second);
  expect(first[0]).not.toBe(second[0]);
  expect(second[0]('request')).toBeUndefined();
  expect(send.mock.calls[0][1]).toBe(0);
  expect(JSON.parse(send.mock.calls[0][0]).data.block).toBe(false);
  expect(sendSync).toHaveBeenCalledOnce();
});

test.each([
  'parentElement_propertyResult',
  'querySelector_dynamicResult',
])('renderer HTMLDivElement results preserve their type regardless of action: %s', (action) => {
  class HTMLDivElement {
    id = 'container';
    children: unknown[] = [];
  }
  const parent = new HTMLDivElement();
  const webview = { parentElement: parent };
  parent.children.push(webview);

  const handle = hookResult(action, parent);
  expect(handle.instanceType).toBe('HTMLDivElement');
  expect(useInstanceManage().getInstance(handle.instanceId)).toBe(parent);
  expect(hookResult('parentElement_propertyResult', parent)).toEqual(handle);
  expect(hookResult('nodes_dynamicResult', [parent, parent])).toEqual([handle, handle]);
  expect(hookResult('payload_staticResult', { parent, count: 2, empty: null })).toEqual({
    parent: handle, count: 2, empty: null,
  });
  expect(useInstanceManage().instanceCount).toBe(1);
  expect(hookArgument('send', [handle])).toEqual([parent]);
  expect(hookResult(action, null)).toBeNull();
});

test('renderer HTMLDivElement callbacks reuse the same typed handle as return values', () => {
  class HTMLDivElement {}
  const element = new HTMLDivElement();
  const handle = hookResult('element_propertyResult', element);
  expect(handle.instanceType).toBe('HTMLDivElement');
  const send = vi.fn();
  vi.stubGlobal('send', send);
  const params: any[] = ['event', { callbackId: 1, asyncCallback: true }];
  hookArgument('addEventListener', params);
  params[1](element);
  expect(JSON.parse(send.mock.calls[0][0]).data.args).toEqual([handle]);
  expect(useInstanceManage().instanceCount).toBe(1);
});

test('renderer asynchronous event callbacks use message ID zero', () => {
  const send = vi.fn();
  const sendSync = vi.fn();
  vi.stubGlobal('send', send);
  vi.stubGlobal('sendMessageSync', sendSync);
  const params: any[] = ['load', { callbackId: 2, asyncCallback: true }];
  hookArgument('addEventListener', params);

  expect(params[1]('loaded')).toBeUndefined();
  expect(send).toHaveBeenCalledExactlyOnceWith(JSON.stringify({
    type: 'emitCallback',
    callbackId: 2,
    data: { args: ['loaded'], block: false },
  }), 0);
  expect(sendSync).not.toHaveBeenCalled();
});

test('main calls with renderer action names keep their original parameters and callback behavior', () => {
  const createWindow = vi.fn();
  const notifyResourceLoad = vi.fn();
  const setDialogCallback = vi.fn();
  const { rpc, call, server } = fixture({ createWindow, notifyResourceLoad, setDialogCallback });
  const windowParams = [1, 2, 3, 4, 5, 6, 'memory-key'];
  call('createWindow', windowParams);
  expect(createWindow).toHaveBeenCalledWith(...windowParams);
  call('notifyResourceLoad', ['resource', [1, 2], { callbackId: 'ordinary-data' }]);
  expect(notifyResourceLoad).toHaveBeenCalledWith('resource', [1, 2], { callbackId: 'ordinary-data' });
  call('setDialogCallback', [{ callbackId: 1 }]);
  call('setDialogCallback', [{ callbackId: 1 }]);
  const callback = setDialogCallback.mock.calls[0][0];
  expect(setDialogCallback.mock.calls[1][0]).toBe(callback);
  expect(callback('request')).toBe('client-result');
  expect(JSON.parse(server.sendMessageSync.mock.calls[0][0]).data.block).toBe(true);
  rpc.stop();
});

test('renderer hooks retain resource conversion and request object types', () => {
  registerDefaultClazz(globalThis);
  const params = ['resource', [1, 2]];
  hookArgument('notifyResourceLoad', params);
  expect(params[1]).toEqual(new Uint8Array([1, 2]));
  expect(hookResult('setLoadResourceCallback_syncResult', [3, 4])).toEqual(new Uint8Array([3, 4]));

  const request = { onMessage: {}, onRequest: {} };
  const result = hookResult('request_propertyResult', request);
  expect(result.onMessage.instanceType).toBe('RequestMessageEvent');
  expect(result.onRequest.instanceType).toBe('RequestRule');
  expect(useInstanceManage().getInstance(result.onMessage.instanceId)).toBe(request.onMessage);
  expect(useInstanceManage().getInstance(result.onRequest.instanceId)).toBe(request.onRequest);

  class WebViewElement {}
  const webview = new WebViewElement();
  const data = { nested: true };
  const items = [webview, data];
  expect(hookResult('array_staticResult', items)).toBe(items);
  expect(items[0]).toMatchObject({ instanceType: 'ChromeWebViewElement' });
  expect(items[1]).toBe(data);
});
