import { useInstanceManage, useObjectManage } from "../server/object-manage";
import { useLogger } from "./log";

const clazzList = [
    'AsyncStylesheets',
    'ViewShadowNode',
    'GridViewShadowNode',
    'ScrollViewShadowNode',
    'ListViewShadowNode',
    'StickySectionShadowNode',
    'StickyHeaderShadowNode',
    'FragmentBinding',
    'TextShadowNode',
    'InputShadowNode',
    'ImageShadowNode',
    'SwiperShadowNode',
    // 就是少了一个w
    'SwiperItemShadoNode',
    'HeroShadowNode',
    'MutableValue',
]
const log = useLogger('HookArgument')
/**
 * 处理Callback的参数
 * 
 * Server -> Client
 * @param arg 
 * @returns 
 */
const hookCallbackArgument = (arg: any) => {
    if (Array.isArray(arg)) {
        for (let i = 0; i < arg.length; i++) {
            const element = arg[i];
            arg[i] = hookCallbackArgument(element)
        }
    }
    else if (typeof arg === 'object') {
        // 对象处理
        const name = arg?.constructor?.name
        // 特定实例，转换为自定义对象
        if (clazzList.includes(name)) {
            const { getInstanceId, setInstance } = useInstanceManage()
            const instanceId = getInstanceId(arg) || setInstance(arg);
            arg = {
                instanceId: instanceId,
                instanceType: name,
            }
        }
        else {
            // 其他对象，直接转换为普通对象
            const g = global
            if (!g.clazzSet)
                g.clazzSet = new Set()
            if (!g.clazzSet.has(name)) {
                g.clazzSet.add(name)
            }
            log.warn('hookCallbackArgument type not found!', name, arg)
            for (const key in arg) {
                arg[key] = hookCallbackArgument(arg[key])
            }
        }
    }
    return arg;
}

const hookArgumentItem = (action: string, arg: any) => {
    if (!arg) return arg
    if (Array.isArray(arg)) {
        // Array 处理
        for (let i = 0; i < arg.length; i++) {
            const element = arg[i];
            arg[i] = hookArgumentItem(action, element)
        }
    }
    else if (typeof arg === 'object') {
        // Object 处理
        if (arg.hasOwnProperty('instanceId')) {
            // 实例替换
            const { getInstance } = useInstanceManage()
            const instance = getInstance(arg.instanceId)
            if (instance) {
                arg = instance
            }
            else {
                throw new Error(`Instance not found: ${arg.instanceId}`)
            }
        }
        else if (arg.hasOwnProperty('callbackId')) {
            // 回调替换
            const callbackId = arg.callbackId
            const asyncCallback = arg.asyncCallback
            const temp: any = (...args1: any[]) => {
                // 替换参数中的具体对象
                hookCallbackArgument(args1)
                log.info('callback emit', action, args1)
                if (asyncCallback) {
                    log.info('callback emit async', action, args1)
                    // 异步回调
                    global.send(JSON.stringify({
                        type: 'emitCallback',
                        callbackId,
                        data: {
                            args: args1,
                            block: false,
                        },
                    }))
                    return;
                }
                log.info('callback emit sync', action, args1)
                // 同步回调
                const result = global.sendMessageSync(JSON.stringify({
                    type: 'emitCallback',
                    callbackId,
                    data: {
                        args: args1,
                        block: true,
                    },
                }))
                log.info('callback emit sync result:', result)
                return hookResult(`${action}_syncResult`, result)
            }
            // worklet 处理
            if (arg.__worklet) {
                temp.asString = arg.asString
                temp.__workletHash = arg.__workletHash
                temp.__location = arg.__location
                temp.__worklet = arg.__worklet
                temp._closure = hookArgumentItem(action, arg._closure)
            }
            arg = temp
        }
        else {
            for (const k in arg) {
                if (arg.hasOwnProperty(k)) {
                    arg[k] = hookArgumentItem(action, arg[k])
                }
            }
        }
    }
    return arg
}

export const hookArgument = (action: string, args: any[]) => {
    args = hookArgumentItem(action, args)
    if (action === 'createWindow') {
        // 创建窗口时，传入的bufferKey参数需要转换为ArrayBuffer
        const sharedMemory = require('sharedMemory/sharedMemory.node')
        args[6] = sharedMemory.getMemory(args[6])
    }
    else if (action === 'notifyHttpRequestComplete') {
        // http资源替换buffer
        const sharedMemory = require('sharedMemory/sharedMemory.node')
        log.info('notifyHttpRequestComplete', args[4])
        const buf = sharedMemory.getMemory(args[4]) as ArrayBuffer
        args[4] = new Uint8Array(buf)
        // if (args[4] != 'resource_0') {
        //     throw new Error('break!')
        // }
    }
    else if (action === 'notifyResourceLoad') {
        // http资源替换buffer
        args[1] = new Uint8Array(args[1])
    }
    else if (action === 'registerEventHandler') {
        console.info('registerEventHandler', args)
    }
}
let functionDataId = 1;
export const hookResult = (action: string, result: any) => {
    log.info('result before:', result)
    if (action === 'setLoadResourceCallback_syncResult') {
        return new Uint8Array(result);
    }
    else if (typeof result === 'function') {
        const id = functionDataId++
        const { getClazz } = useObjectManage()
        const functionData = getClazz('functionData')
        functionData[id] = result;
        return { instanceId: id, instanceType: 'function' };
    }
    else if (Array.isArray(result)) {
        // 是个Array，转为自定义Object
        const { setInstance } = useInstanceManage()
        for (let i = 0; i < result.length; i++) {
            const element = result[i];
            const name = element?.constructor?.name
            if (clazzList.includes(name)) {
                result[i] = {
                    instanceId: setInstance(element),
                    instanceType: name,
                }
            }
            else {
                const g = global as any
                if (!g.clazzSet)
                    g.clazzSet = new Set()
                if (!g.clazzSet.has(name)) {
                    g.clazzSet.add(name)
                }
            }
        }
    }
    else if (typeof result === 'object') {
        const name = result?.constructor?.name
        if (clazzList.includes(name)) {
            const { setInstance } = useInstanceManage()
            result = {
                instanceId: setInstance(result),
                instanceType: name,
            }
        }
        else {
            const g = global as any
            if (!g.clazzSet)
                g.clazzSet = new Set()
            if (!g.clazzSet.has(name)) {
                g.clazzSet.add(name)
            }
        }
    }
    return result;
}