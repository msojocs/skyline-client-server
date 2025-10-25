import { useLogger } from "./common/log"
import { registerDefaultClazz, useInstanceManage, useObjectManage } from "./server/object-manage"
import { hookArgument, hookResult } from "./common/hook-argument"
const log = useLogger('Server')

interface RequestMessage {
    id: string
    type: 'constructor' | 'static' | 'dynamic' | 'dynamicProperty' | 'registerCallback'
    data: {
        clazz: string
        action: string
        instanceId?: string
        callbackId?: string
        syncCallback?: boolean
        params?: any[]
        propertyAction?: 'set' | 'get'
    }
}
interface ResponseMessage {
    id: string
    type: 'response'
    data: {
        result?: any
        error?: string
    }
}

try {
  log.info('Hi rpc server!')
  log.info(process.version)

  process.on('unhandledRejection', (err) => {
    log.error('unhandledRejection:', err)
    // process.exit(1)
  })
  require('skyline-addon/build/skyline.node')
  const skylineServerFile = process.env.SKYLINE_SERVER_FILE || 'skyline-server/skylineServer.node'
  const server = require(skylineServerFile)
  const g = global as any
  g.window = g
  window = g
  registerDefaultClazz(g)
  const port = 3001
  server.start('127.0.0.1', port)
  g.send = server.sendMessageSingle
  const reply = server.sendMessageSingle as (message: ResponseMessage) => void
  server.setMessageCallback((message: RequestMessage) => {
    g.sendMessageSync = server.sendMessageSync
    const req = message
    if (req.data.action === 'disconnected') {
      (globalThis as any).parentSource.postMessage('disconnected')
      return
    }
    try {
      log.info(`Received message => ${JSON.stringify(message)}`);
      if (req.type === 'constructor') {
        // 构造对象请求
        const { getClazz } = useObjectManage()
        const clazz = getClazz(req.data.clazz)
        if (!clazz) {
          log.error('Class not found', req.data.clazz)
          reply({
            id: req.id,
            type: 'response',
            data: {
                error: 'Class not found'
            }
          })
          return
        }
        // 实例化对象
        const { setInstance } = useInstanceManage()
        const params = req.data.params || []
        const instanceId = setInstance(new clazz(...params))
        // log.info('constructor end', req.data.clazz, instanceId)
        reply({
          id: req.id,
          type: 'response',
          data: {
            result: {
              instanceId: instanceId
            }
          }
        })
      }
      else if (req.type == 'static') {
        // 静态对象调用请求
        const { getClazz } = useObjectManage()
        const clazz = getClazz(req.data.clazz)
        if (!clazz) {
          reply({
            id: req.id,
            type: 'response',
            data: {
              error: 'Class not found'
            }
          })
          return
        }
        if (typeof clazz[req.data.action] === 'function') {
          const params = req.data.params || []
          hookArgument(req.data.action, params)
        //   log.info('static call', req.data.action, params)
          let result = clazz[req.data.action](...params);
          result = hookResult(`${req.data.action}_staticResult`, result)
        //   log.info("static call result", req.data.action, result);
          reply({
            id: req.id,
            type: 'response',
            data: {
                result: {
                    returnValue: result,
                },
            }
          });
        } else if (typeof clazz[req.data.action] !== 'undefined') {
          const result = clazz[req.data.action]
          reply({
            id: req.id,
            type: 'response',
            data: {
                result: {
                    returnValue: result
                },
            }
          });
        } else {
          log.error('Method not found or instance invalid', req.data.action, clazz[req.data.action])
          reply({
            id: req.id,
            type: 'response',
            data: {
                error: 'Method not found or instance invalid',
            }
          });
        }
      }
      else if (req.type == 'dynamic') {
        // 动态对象调用请求
        if (!req.data.instanceId) {
          log.error('InstanceId not found')
          reply({
            id: req.id,
            type: 'response',
            data: {
                error: 'InstanceId not found'
            }
          });
          return
        }
        const { getInstance } = useInstanceManage()
        const instance = getInstance(req.data.instanceId);
        if (instance && typeof instance[req.data.action] === 'function') {
          const params = req.data.params || []
        //   log.info("dynamic call", instance, req.data.action, params);
          hookArgument(req.data.action, params)
          let result = instance[req.data.action](...params);
        //   log.info("dynamic call result", req.data.action, result);
          result = hookResult(`${req.data.action}_dynamicResult`, result)
        //   log.info("dynamic call result hooked", req.data.action, result);

          if (req.id) {
            reply({
                id: req.id,
                type: 'response',
                data: {
                    result: {
                        returnValue: result,
                    },
                },
            });
            if (req.data.action === 'matches' && result === true) {
              console.info('matches:', instance, params, result)
            }
          }
        } else {
          reply({
            id: req.id,
            type: 'response',
            data: {
                error: 'Method not found or instance invalid',
            }
          });
        }
      }
      else if (req.type == 'dynamicProperty') {
        // 动态对象调用请求
        if (!req.data.instanceId) {
          reply({
            id: req.id,
            type: 'response',
            data: {
              error: 'InstanceId not found'
            }
          })
          return
        }
        const { getInstance } = useInstanceManage()
        const instance = getInstance(req.data.instanceId);
        // 动态属性调用请求
        const type = req.data.propertyAction
        const params = req.data.params || []
        // log.info("dynamic property", req.data.action, params);
        hookArgument(req.data.action, params)
        let result = undefined
        if (type === 'set') {
          // 设置属性
          instance[req.data.action] = params[0]
        //   log.info("dynamic property set", req.data.action, params[0]);
        }
        else if (type === 'get') {
          // 获取属性
        //   log.info("dynamic property get", req.data.action, instance[req.data.action]);
          result = instance[req.data.action];
        }
        // log.info("dynamic property result", req.data.action, result);
        result = hookResult(`${req.data.action}_dynamicResult`, result)
        // log.info("dynamic property result hooked", req.data.action, result);

        if (req.id) {
          reply({
            id: req.id,
            type: 'response',
            data: {
              result: {
                returnValue: result,
              },
            },
          });
        }
      }
      else {
        reply({
          id: req.id,
          type: 'response',
          data: {
            error: 'Request type not recognized',
          }
        });
      }
    }
    catch (err: any) {
      log.error('Error:', err)
      if (req.id) {
        reply({
          id: req.id,
          type: 'response',
          data: {
            error: err.message,
          }
        });
      }
    }
  });
  log.info(`✅ Server listening on *:${port}`);
  log.info('end....')
}
catch (err) {
  log.error('Error:', err)
}