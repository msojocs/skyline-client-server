import { Controller } from "./controller";
import { useCustomHandle } from "./custom-handle"

const clazzMap = new Map<string, any>();
globalThis.clazzMap = clazzMap
export const useObjectManage = () => ({
    getClazz: (clazzName: string) => {
        return clazzMap.get(clazzName)
    },
    setClazz: (clazzName: string, clazz: any) => {
        clazzMap.set(clazzName, clazz)
    },
    removeClazz: (clazzName: string) => {
        clazzMap.delete(clazzName)
    },
    clearClazz: () => {
        clazzMap.clear()
    },
    getAllClazz: () => {
        return Array.from(clazzMap.values())
    },
})

export const registerDefaultClazz = (g: any) => {
    clazzMap.set('Controller', Controller)
    // SkylineDebugInfo使用
    clazzMap.set('global', g)
    const customHandle = useCustomHandle()
    clazzMap.set('customHandle', customHandle)
    clazzMap.set('functionData', {})
}

const instanceMap = new Map<number, any>();
const instanceObjectIdMap = new WeakMap<object, number>();
const instancePrimitiveIdMap = new Map<any, number>();
globalThis.instanceMap = instanceMap;
let instanceId = 1;

const isWeakMapKey = (value: any): value is object => {
    return (typeof value === 'object' && value !== null) || typeof value === 'function'
}
export const useInstanceManage = () => ({
    getInstance: (instanceId: number) => {
        return instanceMap.get(instanceId)
    },
    getInstanceId: (instance: any) => {
        if (isWeakMapKey(instance)) {
            return instanceObjectIdMap.get(instance) ?? null
        }
        return instancePrimitiveIdMap.get(instance) ?? null
    },
    setInstance: (instance: any) => {
        const id = instanceId++
        instanceMap.set(id, instance)
        if (isWeakMapKey(instance)) {
            if (!instanceObjectIdMap.has(instance)) {
                instanceObjectIdMap.set(instance, id)
            }
        } else if (!instancePrimitiveIdMap.has(instance)) {
            instancePrimitiveIdMap.set(instance, id)
        }
        return id
    },
    removeInstance: (instanceId: number) => {
        const instance = instanceMap.get(instanceId)
        if (!instanceMap.delete(instanceId)) {
            return
        }

        if (isWeakMapKey(instance)) {
            const mappedId = instanceObjectIdMap.get(instance)
            if (mappedId === instanceId) {
                let fallbackId: number | null = null
                for (const [key, value] of instanceMap.entries()) {
                    if (value === instance) {
                        fallbackId = key
                        break
                    }
                }
                if (fallbackId === null) {
                    instanceObjectIdMap.delete(instance)
                } else {
                    instanceObjectIdMap.set(instance, fallbackId)
                }
            }
            return
        }

        const mappedId = instancePrimitiveIdMap.get(instance)
        if (mappedId === instanceId) {
            let fallbackId: number | null = null
            for (const [key, value] of instanceMap.entries()) {
                if (value === instance) {
                    fallbackId = key
                    break
                }
            }
            if (fallbackId === null) {
                instancePrimitiveIdMap.delete(instance)
            } else {
                instancePrimitiveIdMap.set(instance, fallbackId)
            }
        }
    },
    removeInstanceOfType: (type: string) => {
        for (const [id, instance] of instanceMap.entries()) {
            if (instance?.constructor?.name === type) {
                instanceMap.delete(id)
                if (isWeakMapKey(instance)) {
                    const mappedId = instanceObjectIdMap.get(instance)
                    if (mappedId === id) {
                        instanceObjectIdMap.delete(instance)
                    }
                } else {
                    const mappedId = instancePrimitiveIdMap.get(instance)
                    if (mappedId === id) {
                        instancePrimitiveIdMap.delete(instance)
                    }
                }
            }
        }
    },
    clearInstance: () => {
        instanceMap.clear()
        instancePrimitiveIdMap.clear()
    },
})
