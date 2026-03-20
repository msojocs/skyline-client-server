import path from "path"
import { useLogger } from "../common/log"

const log = useLogger('CustomHandle')

export const useCustomHandle = () => ({
    getSkylineAddonPath: () => {
        const buildPath = require.cache[require.resolve('skyline-addon/build/skyline.node')]?.path
        if (!buildPath) {
            throw new Error('skyline-addon not found')
        }
        return path.resolve(buildPath, '..')
    },
})