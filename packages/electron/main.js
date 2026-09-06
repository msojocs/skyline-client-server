'use strict';

const { app, BrowserWindow, ipcMain, session } = require('electron');
const path = require('path');

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
  ['js-flags', '--harmony-weak-refs'],
  ['enable-experimental-webassembly-features'],
  ['remote-debugging-port', process.env.REMOTE_DEBUGGING_PORT || '9222'],
];

for (const [name, value] of chromiumSwitches) {
  app.commandLine.appendSwitch(name, value);
}

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
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});
