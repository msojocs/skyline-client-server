import { Controller } from "./controller";
import { useCustomHandle } from "./custom-handle"
import { createInstanceManage, createObjectManage } from "../common/object-manage"

const clazzMap = new Map<string, any>();
const instanceMap = new Map<number, any>();
globalThis.clazzMap = clazzMap
globalThis.instanceMap = instanceMap

const objects = createObjectManage(clazzMap)
const instances = createInstanceManage(instanceMap)
export const useObjectManage = () => objects
export const useInstanceManage = () => instances

export const registerDefaultClazz = (g: any) => {
    objects.setClazz('Controller', Controller)
    objects.setClazz('global', g)
    objects.setClazz('customHandle', useCustomHandle())
    objects.setClazz('functionData', {})
}
