import path from "path"
import { registerSkylineGlobalClazz, useInstanceManage, useObjectManage } from "./object-manage"
import { useLogger } from "../common/log"

const log = useLogger('CustomHandle')

export const useCustomHandle = () => ({
    registerSkylineGlobalClazzRequest: () => {
        registerSkylineGlobalClazz(global)
    },
    getSkylineAddonPath: () => {
        const buildPath = require.cache[require.resolve('skyline-addon/build/skyline.node')]?.path
        if (!buildPath) {
            throw new Error('skyline-addon not found')
        }
        return path.resolve(buildPath, '..')
    },
})