## 创建窗口

createWindow 的路径要与当前环境一致，否则会崩溃。

## Launch

notifyLaunch 要在ready之后，否则会崩溃。

## Mutable

给value赋值前，`instannCoreFunctions` 和 `makeShareable` 要正确，否则会崩溃。

## 点击事件

需要 `__wxElement` 配合，因此在 `Convert` 中添加了 `instanceCache`。