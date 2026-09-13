import { once } from 'node:events';
import { createConnection, createServer, type AddressInfo } from 'node:net';
import path from 'node:path';
import { afterEach, expect, test, vi } from 'vitest';
import type {} from '../packages/typescript/src/global';
import { hookArgument } from '../packages/typescript/src/common/hook-argument';
import type { NativeRpcServer } from '../packages/typescript/src/common/rpc';
import { useCallback } from '../packages/typescript/src/render-process/callback';
import { Controller } from '../packages/typescript/src/render-process/controller';
import { useInstanceManage } from '../packages/typescript/src/render-process/object-manage';

const directory = process.env.SKYLINE_NATIVE_TEST_DIR || path.resolve(__dirname, '../packages/native/build',
  process.platform === 'win32' ? 'x86_64-pc-windows-gnu' : 'x86_64-unknown-linux-gnu');
const server: NativeRpcServer = require(path.join(directory, 'render-server.node'));

afterEach(() => {
  server.stop();
  useInstanceManage().clearInstance();
  useCallback().clearCallback();
  vi.restoreAllMocks();
  vi.unstubAllGlobals();
});

async function availablePort() {
  const listener = createServer();
  listener.listen(0, '127.0.0.1');
  await once(listener, 'listening');
  const { port } = listener.address() as AddressInfo;
  await new Promise<void>((resolve, reject) => listener.close(error => error ? reject(error) : resolve()));
  return port;
}

test.each(['prompt', 'alert', 'confirm'] as const)('%s dispatch reaches the native transport without blocking', async type => {
  class WebViewElement {}
  vi.stubGlobal('document', {
    getElementById: () => ({}),
    createElement: () => new WebViewElement(),
  });
  vi.stubGlobal('send', server.sendMessageSingle);
  const sendSync = vi.fn();
  const resolveDialog = vi.fn();
  vi.stubGlobal('sendMessageSync', sendSync);
  vi.stubGlobal('__skylineResolveDialog', resolveDialog);

  const controller = new Controller();
  const webview = controller.webview!;
  const instanceId = useInstanceManage().setInstance(webview);
  const params: any[] = [{ callbackId: 7 }];
  hookArgument('setDialogCallback', params);
  controller.setDialogCallback(params[0]);

  const port = await availablePort();
  server.setMessageCallback(() => {});
  server.start('127.0.0.1', port);
  const socket = createConnection(port, '127.0.0.1');
  const chunks: Buffer[] = [];
  socket.on('data', chunk => chunks.push(chunk));
  try {
    await once(socket, 'data');
    expect(Buffer.concat(chunks).readUInt32BE()).toBe(114514);
    expect(controller.dialog(webview, {
      requestId: 'dialog-test', type, args: ['message', 'default'], guestWebContentsId: 2,
    })).toBe(true);
    await new Promise<void>(resolve => queueMicrotask(resolve));
    expect(resolveDialog).not.toHaveBeenCalled();
    expect(sendSync).not.toHaveBeenCalled();

    await vi.waitFor(() => {
      const frame = Buffer.concat(chunks).subarray(4);
      expect(frame.length).toBeGreaterThanOrEqual(12);
      expect(frame.readUInt32BE()).toBe(frame.length - 12);
      expect(frame.readBigUInt64BE(4)).toBe(0n);
      expect(JSON.parse(frame.subarray(12).toString())).toEqual({
        type: 'emitCallback',
        callbackId: 7,
        data: {
          args: [{ instanceId, instanceType: 'ChromeWebViewElement' }, 'dialog-test', type, 'message', 'default'],
          block: false,
        },
      });
    });
  } finally {
    controller.setDialogCallback(null);
    socket.destroy();
  }
});
