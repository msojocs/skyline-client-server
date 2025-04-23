import { useLogger } from "./common/log"
import { registerDefaultClazz, useInstanceManage, useObjectManage } from "./server/object-manage"
import { hookArgument, hookResult } from "./common/hook-argument"
import { TwitterSnowflake } from "@sapphire/snowflake"
const log = useLogger('Server')
try {
  log.info('Hi rpc server!')
  log.info(process.version)

  process.on('unhandledRejection', (err) => {
    log.error('unhandledRejection:', err)
    // process.exit(1)
  })
  require('skyline-addon/build/skyline.node')
  const server = require('D:/github/skyline-client/packages/native/build/skylineServer.node')
  const g = global as any
  g.window = g
  window = g
  registerDefaultClazz(g)
  server.start('127.0.0.1', 3001)
  server.setMessageCallback((ws: { send: (data: string) => void}, message: string) => {
    g.send = ws.send
    g.sendMessageSync = server.sendMessageSync
    const req = JSON.parse(message) as {
      id: string
      type: 'constructor' | 'static' | 'dynamic' | 'dynamicProperty' | 'registerCallback'
      clazz: string
      action: string
      data: {
        instanceId?: string
        clazz?: string
        callbackId?: string
        syncCallback?: boolean
        params?: any[]
        propertyAction?: 'set' | 'get'
      }
    }
    try {
      log.info(`Received message => ${message}`);
      if (req.type === 'constructor') {
        // 构造对象请求
        const { getClazz } = useObjectManage()
        const clazz = getClazz(req.clazz)
        if (!clazz) {
          log.error('Class not found', req.clazz)
          ws.send(JSON.stringify({
            id: req.id,
            error: 'Class not found'
          }))
          return
        }
        // 实例化对象
        const { setInstance } = useInstanceManage()
        const params = req.data.params || []
        const instanceId = setInstance(new clazz(...params))
        log.info('constructor end', req.clazz, instanceId)
        ws.send(JSON.stringify({
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
          ws.send(JSON.stringify({
            id: req.id,
            error: 'Class not found'
          }))
          return
        }
        if (typeof clazz[req.action] === 'function') {
          const params = req.data.params || []
          hookArgument(req.action, params)
          log.info('static call', req.action, params)
          let result = clazz[req.action](...params);
          result = hookResult(`${req.action}_staticResult`, result)
          log.info("static call result", req.action, result);
          ws.send(JSON.stringify({
            id: req.id,
            result: {
              returnValue: result,
            },
          }));
        } else if (typeof clazz[req.action] !== 'undefined') {
          const result = clazz[req.action]
          ws.send(JSON.stringify({
            id: req.id,
            result: {
              returnValue: result
            },
          }));
        } else {
          log.error('Method not found or instance invalid', req.action, clazz[req.action])
          ws.send(JSON.stringify({
            id: req.id,
            error: 'Method not found or instance invalid',
          }));
        }
      }
      else if (req.type == 'dynamic') {
        // 动态对象调用请求
        if (!req.data.instanceId) {
          log.error('InstanceId not found')
          ws.send(JSON.stringify({
            id: req.id,
            error: 'InstanceId not found'
          }))
          return
        }
        const { getInstance } = useInstanceManage()
        const instance = getInstance(req.data.instanceId);
        if (instance && typeof instance[req.action] === 'function') {
          const params = req.data.params || []
          log.info("dynamic call", instance, req.action, params);
          hookArgument(req.action, params)
          let result = instance[req.action](...params);
          log.info("dynamic call result", req.action, result);
          result = hookResult(`${req.action}_dynamicResult`, result)
          log.info("dynamic call result hooked", req.action, result);

          if (req.id) {
            ws.send(JSON.stringify({
              id: req.id,
              result: {
                returnValue: result,
              },
            }));
            if (req.action === 'matches' && result === true) {
              console.info('matches:', instance, params, result)
            }
          }
        } else {
          ws.send(JSON.stringify({
            id: req.id,
            error: 'Method not found or instance invalid',
          }));
        }
      }
      else if (req.type == 'dynamicProperty') {
        // 动态对象调用请求
        if (!req.data.instanceId) {
          ws.send(JSON.stringify({
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
        log.info("dynamic property", req.action, params);
        hookArgument(req.action, params)
        let result = undefined
        if (type === 'set') {
          // 设置属性
          instance[req.action] = params[0]
          log.info("dynamic property set", req.action, params[0]);
        }
        else if (type === 'get') {
          // 获取属性
          log.info("dynamic property get", req.action, instance[req.action]);
          result = instance[req.action];
        }
        log.info("dynamic property result", req.action, result);
        result = hookResult(`${req.action}_dynamicResult`, result)
        log.info("dynamic property result hooked", req.action, result);

        if (req.id) {
          ws.send(JSON.stringify({
            id: req.id,
            result: {
              returnValue: result,
            },
          }));
        }
      }
      else {
        ws.send(JSON.stringify({
          id: req.id,
          error: 'Request type not recognized',
        }));
      }
    }
    catch (err: any) {
      log.error('Error:', err)
      if (req.id) {
        ws.send(JSON.stringify({
          id: req.id,
          error: err.message,
        }))
      }
    }
  });
  log.info('✅ WebSocket Server listening on ws://localhost:3001');
  log.info('end....')
}
catch (err) {
  log.error('Error:', err)
}