当前系统有两个程序,已经启动
1. /home/msojocs/github/skyline-client-server/（wine）
2. /home/msojocs/github/wechat-web-devtools-linux/（linux）

分析当前项目启动的electron（linux）右侧渲染方块不能显示的原因
1. 你可以连接启动中的electron debugger 127.0.0.1:9222
2. 可以杀死进程重新启动来查看情况；先wine层再linux层，然后点击“打开留言”。
3. 禁止使用临时数据目录启动，会导致需要登陆，流程不对。

当前系统有两个程序
1. /home/msojocs/github/skyline-client-server/（wine）
2. /home/msojocs/github/wechat-web-devtools-linux/（linux）

分析wine端关闭后，出现报错的原因
1. 先启动wine层，再启动linux；等待linux层右侧渲染完成，关闭linux层，再关闭wine层。执行操作分析原因。
2. 禁止使用临时数据目录启动，会导致需要登陆，流程不对。