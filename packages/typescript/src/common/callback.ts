import { useLogger } from './log';

const log = useLogger('Callback');

export function createCallbackManage() {
  const callbacks = new Map<number, Function>();
  return {
    getCallback(callbackId: number, callback: Function) {
      const existing = callbacks.get(callbackId);
      if (existing) return existing;
      callbacks.set(callbackId, callback);
      log.debug('callback registered', callbackId);
      return callback;
    },
    clearCallback: () => callbacks.clear(),
  };
}
