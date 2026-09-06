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
      const webview = put('ChromeWebViewElement', { src: '', style, request: {
        onAuthRequired: put('WebRequestEvent', { listeners: new Set() }),
        onMessage: put('RequestMessageEvent', { listeners: new Set() }),
        onRequest: put('RequestRule', { rules: [] }),
      }});
      const controller = put('Controller', { webview });
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
        case 'mount': case 'unmount': case 'setText': break;
        case 'setAttribute': object[params[0]] = params[1]; break;
        case 'getAttribute': value = object[params[0]]; break;
        case 'removeAttribute': delete object[params[0]]; break;
        case 'getUserAgent': value = object.userAgent || 'Fixture'; break;
        case 'setUserAgentOverride': object.userAgent = params[0]; value = true; break;
        case 'addListener': object.listeners.add(params[0].callbackId); break;
        case 'hasListener': value = object.listeners.has(params[0].callbackId); break;
        case 'hasListeners': value = object.listeners.size > 0; break;
        case 'removeListener': object.listeners.delete(params[0].callbackId); break;
        case 'getListeners': value = [...object.listeners]; break;
        case 'addRules': object.rules.push(...params[0]); break;
        case 'getRules': value = object.rules; break;
        case 'removeRules': object.rules = []; break;
        case 'executeScript': {
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
                data: { block: !callback.asyncCallback, args: options.args || [remote('Event', objects.size + 1000)] } });
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
