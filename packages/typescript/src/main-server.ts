// Electron loads the JavaScript emitted from this TypeScript entry by Vite.
// @ts-nocheck
'use strict';

const electronModule = require('electron');
const { app, BrowserWindow, ipcMain, session } = electronModule;
const path = require('path');
import { createMainRpc, startMainRpc } from './main-process/controller.ts';

// The combined bundle also exposes the RPC factory to native integration tests.
// Node resolves the Electron package to its executable path outside Electron;
// in that context only the RPC module is needed and main-process setup is skipped.
if (app) {
console.info('Electron version', process.versions.electron, 'Chrome version', process.versions.chrome, 'Node.js version', process.versions.node);

// These switches were previously supplied through NW.js package.json.
const chromiumSwitches = [
  ['disable-features', 'CrossSiteDocumentBlockingIfIsolating,CrossSiteDocumentBlockingAlways,BlockInsecurePrivateNetworkRequests,RendererCodeIntegrity'],
  ['enable-features', 'ExperimentalWebPlatformFeatures'],
  ['enable-experimental-web-platform-features'],
  ['ignore-certificate-errors'],
  ['ignore-certificate-errors-spki-list', 'TWToqKvMjzR/4BKV/Dv8/bwL0BIdU5bClW2BMayCYB8='],
  ['disable-quic'],
  ['allow-insecure-localhost'],
  ['ignore-gpu-blacklist'],
  ['enable-experimental-webassembly-features'],
  ['remote-debugging-port', process.env.REMOTE_DEBUGGING_PORT || '9222'],
  ['remote-allow-origins', '*'],
];

for (const [name, value] of chromiumSwitches) {
  app.commandLine.appendSwitch(name, value);
}

const dialogTimeoutMs = Math.max(
  1000,
  Number.parseInt(process.env.SKYLINE_DIALOG_TIMEOUT_MS || '30000', 10) || 30000,
);
let dialogRequestId = 0;
const pendingDialogs = new Map();

function completeDialog(requestId, value, reason) {
  const pending = pendingDialogs.get(requestId);
  if (!pending || pending.completed) return false;
  const senderDestroyed = pending.sender && pending.sender.isDestroyed();
  const error = reason ? String(reason) : '';
  pending.completed = true;
  pendingDialogs.delete(requestId);
  clearTimeout(pending.timeout);
  if (pending.sender && !senderDestroyed) {
    pending.sender.removeListener('destroyed', pending.onDestroyed);
  }

  if (error) {
    console.error(`[dialog ${requestId}] ${error}`);
  } else {
    console.info(`[dialog ${requestId}] response`, value);
  }
  if (senderDestroyed) {
    return true;
  }
  pending.event.returnValue = value;
  return true;
}

function sendDeferredDialog(event, type, args) {
  const requestId = `dialog-${Date.now()}-${++dialogRequestId}`;
  const guest = event.sender;
  const host = guest && guest.hostWebContents;
  const pending = {
    event,
    type,
    sender: guest,
    hostWebContentsId: host && host.id,
    guestWebContentsId: guest && guest.id,
    completed: false,
    timeout: null,
    onDestroyed: () => completeDialog(requestId, undefined, 'guest webContents was destroyed'),
  };
  pending.timeout = setTimeout(() => {
    completeDialog(requestId, undefined, `request timed out after ${dialogTimeoutMs}ms`);
  }, dialogTimeoutMs);
  pendingDialogs.set(requestId, pending);
  if (guest && typeof guest.once === 'function') guest.once('destroyed', pending.onDestroyed);

  console.info(`[dialog ${requestId}] request`, {
    type,
    args,
    guestWebContentsId: pending.guestWebContentsId,
    hostWebContentsId: pending.hostWebContentsId,
  });

  if (!host || typeof host.send !== 'function' || host.isDestroyed()) {
    completeDialog(requestId, undefined, 'host webContents is unavailable');
    return;
  }
  host.send('skyline-dialog-request', {
    requestId,
    type,
    args,
    guestWebContentsId: pending.guestWebContentsId,
  });
}

ipcMain.on('prompt', (event, type, ...args) => {
  console.info('[dialog] received renderer request', { type: 'prompt', args: [type, ...args] });
  sendDeferredDialog(event, 'prompt', [type, ...args]);
});

ipcMain.on('alert', (event, type, ...args) => {
  console.info('[dialog] received renderer request', { type: 'alert', args: [type, ...args] });
  sendDeferredDialog(event, 'alert', [type, ...args]);
});

ipcMain.on('confirm', (event, type, ...args) => {
  console.info('[dialog] received renderer request', { type: 'confirm', args: [type, ...args] });
  sendDeferredDialog(event, 'confirm', [type, ...args]);
});

ipcMain.on('skyline-dialog-response', (event, response) => {
  console.info('[dialog] received renderer response', response);
  if (!response || typeof response.requestId !== 'string') return;
  const pending = pendingDialogs.get(response.requestId);
  if (!pending) return;
  if (pending.hostWebContentsId !== undefined && pending.hostWebContentsId !== event.sender.id) {
    console.error(`[dialog ${response.requestId}] response came from an unexpected renderer`);
    return;
  }
  completeDialog(response.requestId, response.result, response.error);
});

const eventNames = [
  'onBeforeRequest',
  'onBeforeSendHeaders',
  'onSendHeaders',
  'onHeadersReceived',
  'onResponseStarted',
  'onBeforeRedirect',
  'onCompleted',
  'onErrorOccurred',
  'onAuthRequired',
];
const requestSessions = new Map();
let requestToken = 0;

function matchesFilter(details, filter) {
  if (!filter || !Array.isArray(filter.urls) || filter.urls.length === 0) return true;
  return filter.urls.some((pattern) => {
    if (pattern === '<all_urls>' || pattern === '*') return true;
    const escaped = pattern.replace(/[.+?^${}()|[\]\\]/g, '\\$&').replace(/\*/g, '.*');
    try { return new RegExp(`^${escaped}$`).test(details.url); } catch { return true; }
  });
}

function installRequestSession(partition) {
  if (requestSessions.has(partition)) return;
  const ses = session.fromPartition(partition);
  const listeners = new Map(eventNames.map((name) => [name, new Map()]));
  requestSessions.set(partition, listeners);

  for (const name of eventNames) {
    const electronName = name;
    const handler = (details, callback) => {
      const candidates = [...listeners.get(name).values()]
        .filter((listener) => listener.webContentsId === details.webContentsId && matchesFilter(details, listener.filter));
      if (candidates.length === 0) {
        if (callback) callback({});
        return;
      }
      const token = `${Date.now()}-${++requestToken}`;
      const sender = candidates[0].sender;
      if (!callback) {
        sender.send('webview-request-event', { eventName: name, details });
        return;
      }
      const responseChannel = `webview-request-response:${token}`;
      let finished = false;
      const finish = (result) => {
        if (finished) return;
        finished = true;
        ipcMain.removeListener(responseChannel, responseHandler);
        if (callback) callback(result && typeof result === 'object' ? result : {});
      };
      const responseHandler = (_event, result) => finish(result);
      ipcMain.once(responseChannel, responseHandler);
      sender.send('webview-request-event', { token, eventName: name, details });
      if (callback) setTimeout(() => finish({}), 30000);
    };
    // Electron exposes the same names without the NW.js "on" prefix.
    ses.webRequest[electronName]({ urls: ['<all_urls>'] }, handler);
  }
}

ipcMain.on('webview-request-listener', (event, message) => {
  const { partition, eventName, webContentsId, filter, listenerId, add } = message || {};
  if (!partition || !eventNames.includes(eventName) || !Number.isInteger(webContentsId)) return;
  installRequestSession(partition);
  const listeners = requestSessions.get(partition).get(eventName);
  if (add) listeners.set(listenerId, { sender: event.sender, webContentsId, filter });
  else listeners.delete(listenerId);
});

function createWindow() {
  const window = new BrowserWindow({
    width: 1100,
    height: 760,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
      sandbox: false,
      webviewTag: true,
    },
  });
  window.loadFile(path.join(__dirname, 'index.html'));
  return window;
}

app.whenReady().then(() => {
  createWindow();
  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
  // main 层 RPC 服务端：把真实 Electron 对象（webContents 等）暴露给 devtools main 层的
  // mainController.electron.*。端口默认 3002，renderer 的 render-server 仍占 3001。
  const electron = require('electron');
  const list = electron.webContents.getAllWebContents();
  const item = list[0]
  try {
    startMainRpc();
  } catch (error) {
    console.error('[main-rpc] failed to start', error);
  }
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});

}

export { createMainRpc, startMainRpc };
