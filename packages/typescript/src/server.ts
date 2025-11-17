import { useLogger } from "./common/log"
import { registerDefaultClazz, useInstanceManage, useObjectManage } from "./server/object-manage"
import { hookArgument, hookResult } from "./common/hook-argument"
const log = useLogger('Server')
try {
  log.info('Hi rpc server!')
  log.info(process.version)

  process.on('unhandledRejection', (err) => {
    log.error('unhandledRejection:', err)
    // process.exit(1)
  })
  require('skyline-addon/build/skyline.node')
  const server = require('skyline-server/server.node')
  global.sendMessageSync = server.sendMessageSync
  global.send = server.sendMessageSingle
  global.blockUntilNextMessage = server.blockUntilNextMessage
  const g = global as any
  g.window = g
  window = g
  registerDefaultClazz(g)
  const port = 3001
  server.start('127.0.0.1', port)
  server.setMessageCallback((message: string) => {
    const req = JSON.parse(message) as {
      id: number
      type: 'constructor' | 'static' | 'dynamic' | 'dynamicProperty' | 'registerCallback'
      clazz: string
      action: string
      data: {
        instanceId?: number
        clazz?: string
        callbackId?: string
        asyncCallback?: boolean
        params?: any[]
        propertyAction?: 'set' | 'get'
      }
    }
    if (req.action === 'disconnected') {
      log.error('disconnected')
      process.exit(1)
      return
    }
    try {
      log.debug(`Received message => ${message}`);
      if (req.type === 'constructor') {
        // 构造对象请求
        const { getClazz } = useObjectManage()
        const clazz = getClazz(req.clazz)
        if (!clazz) {
          log.error('Class not found', req.clazz)
          global.send(JSON.stringify({
            id: req.id,
            error: 'Class not found'
          }))
          return
        }
        // 实例化对象
        const { setInstance } = useInstanceManage()
        const params = req.data.params || []
        const instanceId = setInstance(new clazz(...params))
        log.debug('constructor end', req.clazz, instanceId)
        global.send(JSON.stringify({
          id: req.id,
          result: {
            instanceId: instanceId
          }
        }))
      }
      else if (req.type == 'static') {
        // 静态对象调用请求
        const { getClazz } = useObjectManage()
        const clazz = getClazz(req.clazz)
        if (!clazz) {
          global.send(JSON.stringify({
            id: req.id,
            error: 'Class not found'
          }))
          return
        }
        if (typeof clazz[req.action] === 'function') {
          const params = req.data.params || []
          hookArgument(req.action, params)
          log.debug('static call', req.action, params)
          let result = clazz[req.action](...params);
          result = hookResult(`${req.action}_staticResult`, result)
          log.debug("static call result", req.action, result);
          global.send(JSON.stringify({
            id: req.id,
            result: {
              returnValue: result,
            },
          }));
        } else if (typeof clazz[req.action] !== 'undefined') {
          const result = clazz[req.action]
          global.send(JSON.stringify({
            id: req.id,
            result: {
              returnValue: result
            },
          }));
        } else {
          log.error('Method not found or instance invalid', req.action, clazz[req.action])
          global.send(JSON.stringify({
            id: req.id,
            error: 'Method not found or instance invalid',
          }));
        }
      }
      else if (req.type == 'dynamic') {
        // 动态对象调用请求
        if (!req.data.instanceId) {
          log.error('InstanceId not found')
          global.send(JSON.stringify({
            id: req.id,
            error: 'InstanceId not found'
          }))
          return
        }
        const { getInstance } = useInstanceManage()
        const instance = getInstance(req.data.instanceId);
        if (instance && typeof instance[req.action] === 'function') {
          const params = req.data.params || []
          log.debug("dynamic call", instance, req.action, params);
          hookArgument(req.action, params)
          let result = instance[req.action](...params);
          log.debug("dynamic call result", req.action, result);
          result = hookResult(`${req.action}_dynamicResult`, result)
          log.debug("dynamic call result hooked", req.action, result);

          if (req.id) {
            global.send(JSON.stringify({
              id: req.id,
              result: {
                returnValue: result,
              },
            }));
            if (req.action === 'matches' && result === true) {
              console.info('matches:', instance, params, result)
            } else if (req.action === 'appendCompiledStyleSheets') {
              /**
               * 阻塞当前线程，直到有新消息到来
               * appendCompiledStyleSheets执行后，必须立即执行appendStyleSheets，否则崩溃。
               * 
               * 崩溃情况：
               * 1. appendCompiledStyleSheets执行后，还未执行appendStyleSheets
               * 2. 渲染线程开始新一轮渲染，此时样式表存在异常，由于官方程序未做异常处理，程序崩溃
               * 
               * 解决方法：
               * 1. appendCompiledStyleSheets执行后，立即阻塞当前线程
               * 2. 由于线程阻塞，渲染线程无法开始新一轮渲染
               * 3. 收到appendStyleSheets，解除阻塞
               * 4. 执行appendStyleSheets，此时优先级高于渲染线程
               * 5. 渲染线程继续渲染
               */
              global.blockUntilNextMessage()
            }
          }
        } else {
          global.send(JSON.stringify({
            id: req.id,
            error: 'Method not found or instance invalid',
          }));
        }
      }
      else if (req.type == 'dynamicProperty') {
        // 动态对象调用请求
        if (!req.data.instanceId) {
          global.send(JSON.stringify({
            id: req.id,
            error: 'InstanceId not found'
          }))
          return
        }
        const { getInstance } = useInstanceManage()
        const instance = getInstance(req.data.instanceId);
        // 动态属性调用请求
        const type = req.data.propertyAction
        const params = req.data.params || []
        log.debug("dynamic property", req.action, params);
        hookArgument(req.action, params)
        let result = undefined
        if (type === 'set') {
          // 设置属性
          instance[req.action] = params[0]
          log.debug("dynamic property set", req.action, params[0]);
        }
        else if (type === 'get') {
          // 获取属性
          log.debug("dynamic property get", req.action, instance[req.action]);
          result = instance[req.action];
        }
        log.debug("dynamic property result", req.action, result);
        result = hookResult(`${req.action}_dynamicResult`, result)
        log.debug("dynamic property result hooked", req.action, result);

        if (req.id) {
          global.send(JSON.stringify({
            id: req.id,
            result: {
              returnValue: result,
            },
          }));
        }
      }
      else {
        global.send(JSON.stringify({
          id: req.id,
          error: 'Request type not recognized',
        }));
      }
    }
    catch (err: any) {
      log.error('Error:', err)
      if (req.id) {
        global.send(JSON.stringify({
          id: req.id,
          error: err.message,
        }))
      }
    }
  });
  log.info(`✅ WebSocket Server listening on ws://localhost:${port}`);
  log.info('end....')
}
catch (err) {
  log.error('Error:', err)
}