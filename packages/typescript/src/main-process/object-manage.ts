import type { ObjectManage } from "../common/object-manage"

/**
 * main 进程的命名空间注册。与 render-process/object-manage.ts 的 registerDefaultClazz 对应，
 * 但这里不持有模块级单例：main 侧的 Controller 可以同时存在多个（测试里就建了两个），
 * 状态必须跟着 Controller 实例走，所以 objects 与 functionData 都显式传入。
 */
export const registerDefaultClazz = (
    objects: ObjectManage,
    electronModule: any,
    functionData: Record<string, Function>,
) => {
    objects.setClazz('electron', electronModule)
    objects.setClazz('functionData', functionData)
}
