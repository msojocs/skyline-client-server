'use strict';

// Small adapter for the subset of the NW.js webview API used by Skyline.
// Electron keeps the webview element, while the adapter normalizes method and
// event names expected by the native bridge.
const { ipcRenderer } = require('electron');
const fs = require('fs');
const path = require('path');
const { pathToFileURL } = require('url');
const webviewPrototype = document.createElement('webview').constructor.prototype;
const requestEvents = new WeakMap();
const eventListenerWrappers = new WeakMap();
let listenerId = 0;

// NW.js injected documentstart.js for every guest page. Electron expresses the
// same behavior with the webview preload attribute, which must be present
// before the element is attached to the DOM.
const documentStartPath = path.join(__dirname, 'documentstart', 'index.js');
const originalCreateElement = document.createElement.bind(document);
document.createElement = function (tagName, options) {
  const element = originalCreateElement(tagName, options);
  if (String(tagName).toLowerCase() === 'webview' && fs.existsSync(documentStartPath)) {
    element.setAttribute('preload', pathToFileURL(documentStartPath).toString());
  }
  if (String(tagName).toLowerCase() === 'webview' && !element.hasAttribute('partition')) {
    element.setAttribute('partition', 'skyline_appservice_0');
  }
  return element;
};

const eventMap = {
  loadcommit: 'did-frame-navigate',
  loadstart: 'did-start-loading',
  loadstop: 'did-stop-loading',
  contentload: 'dom-ready',
  loadabort: 'did-fail-load',
};

function eventForElectron(event, type) {
  if (event.type === type) return event;
  return Object.assign({}, event, {
    type,
    isTopLevel: event.isMainFrame,
  });
}

function getRequestState(view) {
  let state = requestEvents.get(view);
  if (state) return state;
  const events = new Map();
  const partition = view.getAttribute('partition') || 'skyline_appservice_0';
  const names = ['onBeforeRequest', 'onBeforeSendHeaders', 'onSendHeaders', 'onHeadersReceived', 'onResponseStarted', 'onBeforeRedirect', 'onCompleted', 'onErrorOccurred', 'onAuthRequired'];
  for (const name of names) events.set(name, new Map());
  state = { partition, events, request: null };
  requestEvents.set(view, state);
  return state;
}

class WebRequestEvent {
  constructor(view, eventName) {
    this.view = view;
    this.eventName = eventName;
  }
  addListener(callback, filter) {
    const state = getRequestState(this.view);
    state.partition = this.view.getAttribute('partition') || 'skyline_appservice_0';
    const id = ++listenerId;
    state.events.get(this.eventName).set(callback, { id, filter });
    const register = () => {
      let webContentsId;
      try { webContentsId = this.view.getWebContentsId(); } catch { return false; }
      ipcRenderer.send('webview-request-listener', { partition: state.partition, eventName: this.eventName, webContentsId, filter, listenerId: id, add: true });
      return true;
    };
    if (!register()) {
      let retries = 0;
      const retry = () => { if (!register() && retries++ < 20) setTimeout(retry, 100); };
      setTimeout(retry, 0);
    }
    return true;
  }
  removeListener(callback) {
    const state = getRequestState(this.view);
    const listener = state.events.get(this.eventName).get(callback);
    if (!listener) return false;
    state.events.get(this.eventName).delete(callback);
    try {
      ipcRenderer.send('webview-request-listener', { partition: state.partition, eventName: this.eventName, webContentsId: this.view.getWebContentsId(), listenerId: listener.id, add: false });
    } catch { /* The guest may not have been attached yet. */ }
    return true;
  }
  hasListener(callback) { return getRequestState(this.view).events.get(this.eventName).has(callback); }
  hasListeners() { return getRequestState(this.view).events.get(this.eventName).size > 0; }
}

class RequestMessageEvent extends WebRequestEvent {
  constructor(view) { super(view, 'onMessage'); }
  addListener(callback) { return super.addListener(callback); }
}

class RequestRule {
  constructor(view) { this.view = view; this.rules = []; }
  addRules(rules) { this.rules.push(...(Array.isArray(rules) ? rules : [rules])); return true; }
  getRules() { return this.rules; }
  removeRules(rules) { this.rules = this.rules.filter((rule) => !rules.includes(rule)); return true; }
}

function requestFor(view) {
  const state = getRequestState(view);
  if (state.request) return state.request;
  const result = {};
  for (const name of ['onBeforeRequest', 'onBeforeSendHeaders', 'onSendHeaders', 'onHeadersReceived', 'onResponseStarted', 'onBeforeRedirect', 'onCompleted', 'onErrorOccurred', 'onAuthRequired']) {
    Object.defineProperty(result, name, { enumerable: true, value: new WebRequestEvent(view, name) });
  }
  Object.defineProperty(result, 'onMessage', { enumerable: true, value: new RequestMessageEvent(view) });
  Object.defineProperty(result, 'onRequest', { enumerable: true, value: new RequestRule(view) });
  state.request = result;
  return result;
}

Object.defineProperty(webviewPrototype, 'request', {
  configurable: true,
  get() { return requestFor(this); },
});

const originalAddEventListener = webviewPrototype.addEventListener;
webviewPrototype.addEventListener = function (type, callback, options) {
  const mapped = eventMap[type];
  if (!mapped) return originalAddEventListener.call(this, type, callback, options);
  let wrappers = eventListenerWrappers.get(this);
  if (!wrappers) eventListenerWrappers.set(this, wrappers = new Map());
  const wrapped = (event) => callback.call(this, eventForElectron(event, type));
  wrappers.set(`${type}:${callback}`, wrapped);
  return originalAddEventListener.call(this, mapped, wrapped, options);
};
const originalRemoveEventListener = webviewPrototype.removeEventListener;
webviewPrototype.removeEventListener = function (type, callback, options) {
  const mapped = eventMap[type];
  if (!mapped) return originalRemoveEventListener.call(this, type, callback, options);
  const wrappers = eventListenerWrappers.get(this);
  const wrapped = wrappers && wrappers.get(`${type}:${callback}`);
  if (wrappers) wrappers.delete(`${type}:${callback}`);
  return originalRemoveEventListener.call(this, mapped, wrapped || callback, options);
};

if (!webviewPrototype.executeScript) {
  webviewPrototype.executeScript = function (details, callback) {
    const code = typeof details === 'string'
      ? details
      : details && details.code
        ? details.code
        : details && details.file
          ? fs.readFileSync(details.file, 'utf8')
          : '';
    const promise = this.executeJavaScript(code || '', Boolean(details && details.userGesture));
    promise.then((result) => { if (typeof callback === 'function') callback([result]); }).catch(() => {});
    return true;
  };
}

const originalSetUserAgent = webviewPrototype.setUserAgent;
webviewPrototype.setUserAgentOverride = function (userAgent) {
  this.__skylineUserAgent = userAgent;
  if (originalSetUserAgent) originalSetUserAgent.call(this, userAgent);
  return true;
};
webviewPrototype.getUserAgent = function () { return this.__skylineUserAgent || navigator.userAgent; };
const originalReload = webviewPrototype.reload;
webviewPrototype.reload = function () { originalReload.call(this); return true; };

ipcRenderer.on('webview-request-event', (_event, message) => {
  for (const view of document.querySelectorAll('webview')) {
    let id;
    try { id = view.getWebContentsId(); } catch { continue; }
    if (id !== message.details.webContentsId) continue;
    const state = getRequestState(view);
    const listeners = state.events.get(message.eventName);
    let result = {};
    for (const [callback, listener] of listeners) {
      try {
        const value = callback(message.details);
        if (value && typeof value === 'object') result = value;
      } catch (error) { console.error(error); }
    }
    if (message.token) ipcRenderer.send(`webview-request-response:${message.token}`, result);
  }
});
