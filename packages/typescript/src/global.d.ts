declare global {
    var sendMessageSync: (message: string) => string
    var send: (message: string) => void
}
export {};