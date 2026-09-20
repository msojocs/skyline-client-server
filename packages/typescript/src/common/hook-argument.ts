import { useCallback } from "../render-process/callback";
import { useInstanceManage, useObjectManage } from "../render-process/object-manage";
import { useLogger } from "./log";
import type { createCallbackManage } from './callback';
import type { InstanceManage, ObjectManage } from './object-manage';
import type { NativeRpcServer } from './rpc';

export interface HookContext {
    instances: InstanceManage;
    objects: ObjectManage;
    callbacks: ReturnType<typeof createCallbackManage>;
    server: Pick<NativeRpcServer, 'sendMessageSingle' | 'sendMessageSync'>;
    functions?: InstanceManage;
    renderer?: boolean;
}

const rendererContext: HookContext = {
    instances: useInstanceManage(),
    objects: useObjectManage(),
    callbacks: useCallback(),
    server: {
        sendMessageSingle: (body, messageId) => global.send(body, messageId),
        sendMessageSync: (body) => global.sendMessageSync(body),
    },
    renderer: true,
};

const remoteInstanceTypes: Record<string, string> = {
    CSSStyleDeclaration: 'CSSStyleDeclaration',
    ChromeWebViewElement: 'ChromeWebViewElement',
    WebViewElement: 'ChromeWebViewElement',
    HTMLDivElement: 'HTMLDivElement',
    WebRequestEvent: 'WebRequestEvent',
    Event: 'Event',
    Controller: 'Controller',
}
const log = useLogger('HookArgument')

const getRemoteInstanceType = (instance: any, context: HookContext) => {
    if (context.renderer) return remoteInstanceTypes[instance?.constructor?.name]
    if (instance === null || typeof instance !== 'object') return undefined
    const prototype = Object.getPrototypeOf(instance)
    if (prototype === Object.prototype || prototype === null) return undefined
    const name = instance.constructor?.name
    return typeof name === 'string' && name !== '' ? name : 'Object'
}

const hookCallbackArgument = (arg: any, context: HookContext): any => {
    if (!context.renderer) return hookResult('callbackArgument', arg, context)
    if (Array.isArray(arg)) {
        for (let i = 0; i < arg.length; i++) {
            arg[i] = hookCallbackArgument(arg[i], context)
        }
    }
    else if (typeof arg === 'object') {
        const name = arg?.constructor?.name
        const instanceType = getRemoteInstanceType(arg, context)
        if (instanceType) {
            const { getInstanceId, setInstance } = context.instances
            arg = {
                instanceId: getInstanceId(arg) ?? setInstance(arg),
                instanceType,
            }
        }
        else {
            if (!global.clazzSet) global.clazzSet = new Set()
            global.clazzSet.add(name)
            log.warn('hookCallbackArgument type not found!', name, arg)
            for (const key in arg) {
                arg[key] = hookCallbackArgument(arg[key], context)
            }
        }
    }
    return arg;
}

const hookArgumentItem = (action: string, arg: any, context: HookContext): any => {
    if (!arg) return arg
    if (Array.isArray(arg)) {
        for (let i = 0; i < arg.length; i++) {
            arg[i] = hookArgumentItem(action, arg[i], context)
        }
    }
    else if (typeof arg === 'object') {
        if (Object.prototype.hasOwnProperty.call(arg, 'instanceId')) {
            const instance = context.instances.getInstance(arg.instanceId)
            if (instance === undefined) {
                const label = context.renderer ? 'Instance not found' : 'InstanceId not found'
                throw new Error(`${label}: ${arg.instanceId}`)
            }
            arg = instance
        }
        else if (Object.prototype.hasOwnProperty.call(arg, 'callbackId') && (context.renderer || typeof arg.callbackId === 'number')) {
            const callbackId = arg.callbackId
            // Dialog callbacks may resolve a deferred synchronous request.
            const dialogCallback = context.renderer && action === 'setDialogCallback'
            const asyncCallback = (context.renderer ? Boolean(arg.asyncCallback) : arg.asyncCallback === true) || dialogCallback
            const generation = context.callbacks.generation
            const callback: any = (...args: any[]) => {
                // Old listeners must neither serialize objects nor send to a later connection.
                if (context.callbacks.generation !== generation) return
                const body = JSON.stringify({
                    type: 'emitCallback',
                    callbackId,
                    data: {
                        args: hookCallbackArgument(args, context),
                        block: !asyncCallback,
                    },
                })
                if (asyncCallback) {
                    // Native bindings require a numeric ID when the argument is supplied.
                    context.server.sendMessageSingle(body, 0)
                    return
                }
                const result = context.server.sendMessageSync(body)
                return context.renderer ? hookResult(`${action}_syncResult`, result, context) : result
            }
            if (context.renderer && arg.__worklet) {
                callback.asString = arg.asString
                callback.__workletHash = arg.__workletHash
                callback.__location = arg.__location
                callback.__worklet = arg.__worklet
                callback._closure = hookArgumentItem(action, arg._closure, context)
            }
            arg = dialogCallback ? callback : context.callbacks.getCallback(callbackId, callback)
        }
        else {
            for (const key of Object.keys(arg)) {
                arg[key] = hookArgumentItem(action, arg[key], context)
            }
        }
    }
    return arg
}

export const hookArgument = (action: string, args: any[], context = rendererContext): any[] => {
    args = hookArgumentItem(action, args, context)
    if (!context.renderer) return args
    // if (action === 'createWindow') {
    //     const sharedMemory = require('sharedMemory/sharedMemory.node')
    //     args[6] = sharedMemory.getMemory(args[6])
    // }
    // else if (action === 'notifyHttpRequestComplete') {
    //     const sharedMemory = require('sharedMemory/sharedMemory.node')
    //     args[4] = new Uint8Array(sharedMemory.getMemory(args[4]) as ArrayBuffer)
    // }
    // else 
    if (action === 'notifyResourceLoad') {
        args[1] = new Uint8Array(args[1])
    }
    else if (action === 'registerEventHandler') {
        console.info('registerEventHandler', args)
    }
    return args
}

let functionDataId = 1;
export const hookResult = (action: string, result: any, context = rendererContext, ancestors = new Set<object>()): any => {
    if (context.renderer) log.debug('result before:', result)
    if (context.renderer && action === 'setLoadResourceCallback_syncResult') {
        return new Uint8Array(result);
    }
    if (typeof result === 'function') {
        const id = context.functions
            ? context.functions.getInstanceId(result) ?? context.functions.setInstance(result)
            : functionDataId++
        context.objects.getClazz('functionData')[id] = result
        return { instanceId: id, instanceType: 'function' }
    }
    if (!context.renderer) {
        if (result === null || result === undefined) return null
        const type = typeof result
        if (type === 'string' || type === 'number' || type === 'boolean') return result
        if (type === 'bigint') return Number(result)
        if (type !== 'object') return null
        if (result.instanceId !== undefined) return { instanceId: result.instanceId }
        if (Buffer.isBuffer(result)) return [...result]
        if (result instanceof ArrayBuffer) return [...new Uint8Array(result)]
    }
    if (Array.isArray(result)) {
        if (!context.renderer) return result.map((item) => hookResult(action, item, context, ancestors))
        for (let i = 0; i < result.length; i++) {
            const element = result[i]
            const instanceType = getRemoteInstanceType(element, context)
            if (instanceType) {
                result[i] = {
                    instanceId: context.instances.getInstanceId(element) ?? context.instances.setInstance(element),
                    instanceType,
                }
            }
            else {
                if (!global.clazzSet) global.clazzSet = new Set()
                global.clazzSet.add(element?.constructor?.name)
            }
        }
    }
    else if (typeof result === 'object') {
        if (result === null) return null
        const instanceType = getRemoteInstanceType(result, context)
        if (instanceType) {
            const { getInstanceId, setInstance } = context.instances
            const id = getInstanceId(result) ?? setInstance(result)
            return { instanceId: id, instanceType }
        }
        if (context.renderer) {
            if (action === 'request_propertyResult_onMessage_propertyResult') {
                return { instanceId: context.instances.setInstance(result), instanceType: 'RequestMessageEvent' }
            }
            if (action === 'request_propertyResult_onRequest_propertyResult') {
                return { instanceId: context.instances.setInstance(result), instanceType: 'RequestRule' }
            }
            if (!global.clazzSet) global.clazzSet = new Set()
            global.clazzSet.add(result?.constructor?.name)
        }
        else {
            if (ancestors.has(result)) throw new Error('Cannot serialize a circular result')
            if (ancestors.size >= 128) throw new Error('Result nesting exceeds 128 levels')
            ancestors.add(result)
        }
        const output: Record<string, any> = {}
        for (const key in result) {
            if (context.renderer || Object.prototype.hasOwnProperty.call(result, key)) {
                output[key] = hookResult(`${action}_${key}_propertyResult`, result[key], context, ancestors)
            }
        }
        if (!context.renderer) ancestors.delete(result)
        return output
    }
    return result;
}
