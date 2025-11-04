declare global {
    var sendMessageSync: (message: string) => string
    var send: (message: string) => void
    var blockUntilNextMessage: () => void
    var clazzSet: Set<string>
    var clazzMap: Map<string, any>
    var instanceMap: Map<number, any>
}
export {};