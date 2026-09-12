export interface NativeRpcServer {
  start(host: string, port: number | string): void;
  stop(): void;
  setMessageCallback(callback: (body: string, messageId: number) => void): void;
  sendMessageSingle(body: string, messageId?: number): void;
  sendMessageSync(body: string): any;
  blockUntilNextMessage(): void;
}

export interface RpcRequest {
  type: 'constructor' | 'static' | 'dynamic' | 'dynamicProperty' | 'registerCallback';
  clazz: string;
  action: string;
  data: {
    instanceId?: number;
    clazz?: string;
    callbackId?: number | string;
    asyncCallback?: boolean;
    params?: any[];
    propertyAction?: 'get' | 'set';
  };
}
