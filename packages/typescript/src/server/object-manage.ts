import { useCustomHandle } from "./custom-handle"

const clazzMap = new Map<string, any>();
(globalThis as any).clazzMap = clazzMap
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
    clazzMap.set('SkylineShell', g.SkylineShell)
    // SkylineDebugInfo使用
    clazzMap.set('global', g)
    const customHandle = useCustomHandle()
    clazzMap.set('customHandle', customHandle)
    clazzMap.set('functionData', {})
}

export const registerSkylineGlobalClazz = (g: any) => {
    clazzMap.set('SkylineGlobal', g.SkylineGlobal)
    clazzMap.set('PageContext', g.SkylineGlobal.PageContext)
    clazzMap.set('SkylineRuntime', g.SkylineGlobal.runtime)
    clazzMap.set('SkylineWorkletModule', g.SkylineGlobal.workletModule)
    clazzMap.set('SkylineGestureModule', g.SkylineGlobal.gestureHandlerModule)
}

const instanceMap = new Map<number, any>();
(globalThis as any).instanceMap = instanceMap;
let instanceId = 1;
export const useInstanceManage = () => ({
    getInstance: (instanceId: number) => {
        return instanceMap.get(instanceId)
    },
    getInstanceId: (instance: any) => {
        for (const [key, value] of instanceMap.entries()) {
            if (value === instance) {
                return key
            }
        }
        return null
    },
    setInstance: (instance: any) => {
        const id = instanceId++
        instanceMap.set(id, instance)
        return id
    },
    removeInstance: (instanceId: number) => {
        instanceMap.delete(instanceId)
    },
    clearInstance: () => {
        instanceMap.clear()
    },
})