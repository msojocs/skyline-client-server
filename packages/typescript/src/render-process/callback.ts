import { createCallbackManage } from "../common/callback"

const callbacks = createCallbackManage()
export const useCallback = () => callbacks
