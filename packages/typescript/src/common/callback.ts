import { useLogger } from './log';

const log = useLogger('Callback');

export function createCallbackManage() {
  const callbacks = new Map<number, Function>();
  const callbackFunctions = new Set<Function>();
  let generation = 0;
  return {
    get generation() { return generation; },
    hasCallback: (callback: Function) => callbackFunctions.has(callback),
    getCallback(callbackId: number, callback: Function) {
      const existing = callbacks.get(callbackId);
      if (existing) return existing;
      callbacks.set(callbackId, callback);
      callbackFunctions.add(callback);
      log.debug('callback registered', callbackId);
      return callback;
    },
    clearCallback() {
      // Event emitters and native APIs can retain callbacks after the cache is cleared.
      generation++;
      callbacks.clear();
      callbackFunctions.clear();
    },
  };
}
