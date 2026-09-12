import { Controller } from "./render-process/controller"

declare global {
    var __skylineResolveDialog: ((response: {
        requestId: string
        result?: unknown
        error?: string
    }) => void) | undefined
}

declare global {
    var sendMessageSync: (message: string) => string
    var send: (message: string, messageId?: number) => void
    var blockUntilNextMessage: () => void
    var controller: Controller
    var clazzSet: Set<string>
    var clazzMap: Map<string, any>
    var instanceMap: Map<number, any>
}
export {};
