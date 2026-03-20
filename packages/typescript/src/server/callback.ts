import { useLogger } from "../common/log"

const callbackMap = new Map<number, Function>()
const log = useLogger('Callback')
export const useCallback = () => ({
    getCallback: (callbackId: number, cb: Function) => {
        if (!callbackMap.has(callbackId)) {
            callbackMap.set(callbackId, cb)
            log.debug('callback registered', callbackId)
            return cb
        }
        return callbackMap.get(callbackId)
    },
})