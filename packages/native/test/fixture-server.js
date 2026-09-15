const { parentPort, workerData } = require('node:worker_threads');
const server = require(workerData.module);
const objects = new Map();
let nextId = 10;

const remote = (instanceType, instanceId) => ({ instanceType, instanceId });
const put = (kind, value = {}) => {
  const id = nextId++;
  objects.set(id, value);
  return remote(kind, id);
};

server.setMessageCallback((body, id) => {
  const request = JSON.parse(body);
  if (request.action === 'disconnected') return;
  const reply = result => server.sendMessageSingle(JSON.stringify({ result }), id);
  try {
    if (request.type === 'constructor') {
      const style = put('CSSStyleDeclaration', { display: '', pointerEvents: '' });
      const webview = put('ChromeWebViewElement', { src: '', style, parentElement: null });
      const container = put('HTMLDivElement', { id: 'container' });
      const controller = put('Controller', { webview, container });
      reply({ instanceId: controller.instanceId });
      return;
    }
    const { params = [], instanceId } = request.data;
    const object = objects.get(instanceId);
    let value;
    if (request.type === 'dynamicProperty') {
      if (request.data.propertyAction === 'set') object[request.action] = params[0];
      else value = object[request.action];
    } else if (request.type === 'static') {
      value = { called: request.action, params };
    } else {
      switch (request.action) {
        case 'mount':
          objects.get(object.webview.instanceId).parentElement = object.container;
          break;
        case 'unmount':
          objects.get(object.webview.instanceId).parentElement = null;
          break;
        case 'setText': break;
        case 'send':
          if (typeof params[0] !== 'string') throw new Error('Channel must be a string');
          parentPort.postMessage({ type: 'webview-message', params });
          break;
        case 'setDialogCallback':
          object.dialogCallback = params[0];
          break;
        case 'dialog': {
          const [webview, request] = params;
          const callback = object.dialogCallback;
          value = Boolean(callback);
          if (callback) {
            setImmediate(() => {
              const payload = JSON.stringify({
                type: 'emitCallback',
                callbackId: callback.callbackId,
                data: {
                  block: false,
                  args: [
                    remote('ChromeWebViewElement', webview.instanceId),
                    request.requestId,
                    request.type,
                    ...request.args,
                  ],
                },
              });
              server.sendMessageSingle(payload);
            });
          }
          break;
        }
        case 'resolveDialog':
          object.dialogResponse = params;
          value = true;
          break;
        case 'setAttribute': object[params[0]] = params[1]; break;
        case 'getAttribute': value = object[params[0]]; break;
        case 'removeAttribute': delete object[params[0]]; break;
        case 'getUserAgent': value = object.userAgent || 'Fixture'; break;
        case 'setUserAgentOverride': object.userAgent = params[0]; value = true; break;
        case 'executeJavaScript': {
          const [options, callback] = params;
          if (options.error) throw new Error('fixture remote error');
          if (options.noReply) return;
          if (options.disconnect) { server.stop(); return; }
          if (options.function) { value = remote('function', 71); break; }
          if (options.controller) { value = remote('Controller', instanceId); break; }
          if (options.echo) { value = options.value; break; }
          if (options.blockUntilNext) {
            reply({ returnValue: true });
            server.blockUntilNextMessage();
            return;
          }
          if (callback) {
            const emit = () => {
              const payload = JSON.stringify({ type: 'emitCallback', callbackId: callback.callbackId,
                data: { block: !callback.asyncCallback, args: options.args || [put('Event', options.event || {})] } });
              return callback.asyncCallback ? server.sendMessageSingle(payload) : server.sendMessageSync(payload);
            };
            if (options.later) setTimeout(emit, 10);
            else value = emit();
          } else value = true;
          break;
        }
        case 'reload': value = true; break;
        default: throw new Error(`Unknown method: ${request.action}`);
      }
    }
    reply({ returnValue: value });
  } catch (error) {
    server.sendMessageSingle(JSON.stringify({ error: error.message }), id);
  }
});

server.start('127.0.0.1', workerData.port);
parentPort.postMessage('ready');
parentPort.on('message', message => {
  if (message === 'stop') { server.stop(); parentPort.close(); }
});
