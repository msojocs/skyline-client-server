// Node.js 22+; attach to the existing sessions, without restarting or changing profiles.
// Run from the repository root:
// node docs/skyline-startup-stall/collect-snapshot.mjs > snapshot.json
async function connect(port, title) {
  const targets = await (await fetch(`http://127.0.0.1:${port}/json/list`)).json();
  const target = targets.find((target) => target.title.includes(title));
  if (!target) throw new Error(`Target missing on ${port}: ${title}`);
  const socket = new WebSocket(target.webSocketDebuggerUrl);
  await new Promise((resolve, reject) => {
    socket.onopen = resolve;
    socket.onerror = reject;
  });
  let sequence = 0;
  const pending = new Map();
  socket.onmessage = ({ data }) => {
    const message = JSON.parse(data);
    pending.get(message.id)?.(message);
  };
  return {
    socket,
    call(method, params = {}) {
      return new Promise((resolve, reject) => {
        const id = ++sequence;
        const timer = setTimeout(() => {
          pending.delete(id);
          reject(new Error(`Timeout: ${method}`));
        }, 15000);
        pending.set(id, (message) => {
          clearTimeout(timer);
          pending.delete(id);
          if (message.error) reject(new Error(JSON.stringify(message.error)));
          else resolve(message.result);
        });
        socket.send(JSON.stringify({ id, method, params }));
      });
    },
  };
}

function value(response) {
  if (response.exceptionDetails) {
    throw new Error(response.exceptionDetails.exception?.description || response.exceptionDetails.text);
  }
  return response.result.value;
}

async function evaluate(connection, expression) {
  return value(await connection.call('Runtime.evaluate', {
    expression, returnByValue: true, awaitPromise: true, timeout: 12000,
  }));
}

// Read the preload wrapper's closure through debugger metadata. No functions are replaced.
async function preloadMessager(connection) {
  const evaluated = await connection.call('Runtime.evaluate', {
    expression: '__global__.__messager__.setLogicReady', objectGroup: 'skyline-analysis',
  });
  const properties = await connection.call('Runtime.getProperties', {
    objectId: evaluated.result.objectId, ownProperties: true,
  });
  const scopes = properties.internalProperties.find((entry) => entry.name === '[[Scopes]]');
  const entries = await connection.call('Runtime.getProperties', {
    objectId: scopes.value.objectId, ownProperties: true,
  });
  for (const scope of entries.result.filter((entry) => /^\d+$/.test(entry.name))) {
    if (!scope.value.description.startsWith('Closure')) continue;
    const variables = await connection.call('Runtime.getProperties', {
      objectId: scope.value.objectId, ownProperties: true,
    });
    for (const variable of variables.result) {
      if (variable.value?.type !== 'object' || !variable.value.objectId) continue;
      const members = await connection.call('Runtime.getProperties', {
        objectId: variable.value.objectId, ownProperties: true,
      });
      if (!members.result.some((entry) => entry.name === '_pendingEvents')) continue;
      return value(await connection.call('Runtime.callFunctionOn', {
        objectId: variable.value.objectId,
        functionDeclaration: `function() { return {
          name: this.name, logicReady: this._logicReady,
          pendingCount: this._pendingEvents.length,
          skylineListenerCount: this._cmdListeners.get('23')?.length || 0,
          clients: [...this._clientNameMap.keys()]
        }; }`,
        returnByValue: true,
      }));
    }
  }
  throw new Error('Preload messager closure not found; the bundle may have changed');
}

async function linuxSnapshot() {
  const electron = process.mainModule.require('electron');
  const contents = electron.webContents.getAllWebContents();
  const project = contents.find((entry) => entry.getURL().includes('electron-project.html'));
  const pageframe = contents.find((entry) => entry.getURL().includes('/__pageframe__/'));
  const state = await project.executeJavaScript(`(() => {
    const c = require('../js/93fb707457ce375a8b560c655057c27f.js').skylineController;
    return {
      barriers: [...c.windowBarriers].map(([id, barrier]) => ({ id, open: barrier.isOpen() })),
      appRoute: c.getState().simulator.appRoute,
      pages: Object.values(c.getState().simulator.webviewInfos).map(p => ({
        id: p.id, path: p.pathName, ready: p.ready, renderer: p.renderer
      }))
    };
  })()`);
  const attached = project.debugger.isAttached();
  if (!attached) project.debugger.attach('1.3');
  let proxy;
  try {
    const fn = await project.debugger.sendCommand('Runtime.evaluate', {
      expression: `require('../js/9eee66f818065fa6881814a83bcfe0cf.js').default(
        require('../js/36bdd018ddae3596c5b9230a8f320bb2.js').IEnvMessagerService2).send`,
      objectGroup: 'skyline-analysis',
    });
    const properties = await project.debugger.sendCommand('Runtime.getProperties', {
      objectId: fn.result.objectId, ownProperties: true,
    });
    const bound = properties.internalProperties.find((entry) => entry.name === '[[BoundThis]]');
    const response = await project.debugger.sendCommand('Runtime.callFunctionOn', {
      objectId: bound.value.objectId,
      functionDeclaration: `function() {
        const p = this._clientNameMap.get('WEBVIEW_APPSERVICE')[0];
        const element = p._sender.getOriginElement();
        return {
          messagerName: this.name, logicReady: this._logicReady,
          clientWaiters: [...this._clientWaiters.keys()], proxyReady: p._ready,
          messageQueueLength: p._msgList.length, flushTimerActive: !!p._flushTimer,
          origin: {
            isConnected: element.isConnected, parentElementType: typeof element.parentElement,
            hasParentElement: !!element.parentElement, sendType: typeof element.send
          },
          detachedQueue: p._detachedQueue.map(entry => ({
            channel: entry.channel, cmd: entry.message?.cmd || null,
            command: entry.message?.data?.command || null,
            createWindow: entry.message?.data?.command === 'SAC0' ? entry.message.data.data : null
          }))
        };
      }`,
      returnByValue: true,
    });
    if (response.exceptionDetails) throw new Error(response.exceptionDetails.text);
    proxy = response.result.value;
  } finally {
    try { await project.debugger.sendCommand('Runtime.releaseObjectGroup', { objectGroup: 'skyline-analysis' }); }
    finally { if (!attached) project.debugger.detach(); }
  }
  return {
    platform: process.platform, pid: process.pid, userData: electron.app.getPath('userData'),
    project: state, proxy,
    pageframe: await pageframe.executeJavaScript(`({
      title: document.title, ready: document.readyState,
      canvasCount: document.querySelectorAll('canvas').length,
      rendererInstanceType: typeof SkylineRenderer.instance
    })`),
  };
}

const connections = [];
try {
  const host = await connect(9222, 'Skyline Server'); connections.push(host);
  const guest = await connect(9222, '小程序逻辑层'); connections.push(guest);
  const main = await connect(9229, 'browser_init'); connections.push(main);
  const snapshot = {
    collectedAt: new Date().toISOString(),
    host: await evaluate(host, `(() => {
      const w = document.querySelector('#appservice');
      return { platform: process.platform, electron: process.versions.electron, ready: document.readyState,
        guest: { id: w.getWebContentsId(), isConnected: w.isConnected,
          parentId: w.parentElement?.id, sendType: typeof w.send,
          partition: w.partition, preload: w.preload }
      };
    })()`),
    guest: await evaluate(guest, `({
      ready: document.readyState, platform: __global.platform,
      preloadGlobalType: typeof __global__, messagerType: typeof __global__.__messager__,
      appExists: typeof getApp === 'function' && !!getApp(),
      currentPageCount: getCurrentPages().length, skylineShellType: typeof SkylineShell,
      frames: [window, ...Array.from(document.querySelectorAll('iframe'), f => f.contentWindow)]
        .map(w => ({ path: w.location.pathname,
          skylineWindowCount: w.__global?.skylineManager?.skylineWindows?.size ?? null }))
    })`),
    preloadMessager: await preloadMessager(guest),
    linux: await evaluate(main, `(${linuxSnapshot.toString()})()`),
  };
  console.log(JSON.stringify(snapshot, null, 2));
} finally {
  for (const connection of connections) connection.socket.close();
}
