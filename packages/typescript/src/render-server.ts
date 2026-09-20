import { useLogger } from "./common/log"
import { registerDefaultClazz, useInstanceManage, useObjectManage } from "./render-process/object-manage"
import { hookArgument, hookResult } from "./common/hook-argument"
import { Controller } from "./render-process/controller"
import { useCallback } from "./render-process/callback"
import { ipcRenderer } from 'electron'
import type { NativeRpcServer, RpcRequest } from './common/rpc'
const log = useLogger('Server')
try {
  log.info('Hi rpc server!')
  log.info(process.version)

  process.on('unhandledRejection', (err) => {
    log.error('unhandledRejection:', err)
    // process.exit(1)
  })
  const server: NativeRpcServer = require('skyline-server/render-server.node')
  global.sendMessageSync = server.sendMessageSync
  global.send = server.sendMessageSingle
  global.blockUntilNextMessage = server.blockUntilNextMessage
  global.controller = new Controller()
  global.__skylineResolveDialog = (response) => {
    log.info('Sending dialog response', response)
    ipcRenderer.send('skyline-dialog-response', response)
  }

  type ElectronWebviewElement = HTMLElement & { getWebContentsId(): number }

  const findWebviewForDialog = (guestWebContentsId: number) => {
    for (const webview of Array.from(document.querySelectorAll<ElectronWebviewElement>('webview'))) {
      try {
        if (webview.getWebContentsId() === guestWebContentsId) return webview
      } catch {
        // The webview can be detached while a navigation is being replaced.
      }
    }
    return undefined
  }

  ipcRenderer.on('skyline-dialog-request', (_event, request) => {
    log.info('Received dialog request', request)
    try {
      const webview = findWebviewForDialog(request?.guestWebContentsId)
      if (!webview) {
        ipcRenderer.send('skyline-dialog-response', {
          requestId: request?.requestId,
          error: 'Skyline dialog target webview is unavailable',
        })
        return
      }
      const handled = global.controller.dialog(webview, request)
      if (!handled) {
        ipcRenderer.send('skyline-dialog-response', {
          requestId: request.requestId,
          error: 'Skyline dialog callback is not registered',
        })
      }
    } catch (error: any) {
      ipcRenderer.send('skyline-dialog-response', {
        requestId: request?.requestId,
        error: error?.message || String(error),
      })
    }
  })

  const g = global as any
  registerDefaultClazz(g)
  const port = 3001
  server.start('127.0.0.1', port)
  server.setMessageCallback((message: string, messageId: number) => {
    let replied = false
    const reply = (payload: any) => {
      if (replied || messageId <= 0) return
      replied = true
      console.info('Reply message <====', payload, messageId)
      global.send(JSON.stringify(payload), messageId)
    }
    const req = JSON.parse(message) as RpcRequest
    if (req.action === 'disconnected') {
      useCallback().clearCallback()
      global.controller.setDialogCallback(null)
      log.error('disconnected')
      return
    }
    try {
      log.info(`Received message => ${message}`);
      if (req.type === 'constructor') {
        // 构造对象请求
        const { getClazz } = useObjectManage()
        const clazz = getClazz(req.clazz)
        if (!clazz) {
          log.error('Class not found', req.clazz)
          reply({ error: 'Class not found' })
          return
        }
        // 实例化对象
        const { setInstance } = useInstanceManage()
        const params = req.data.params || []
        const instanceId = setInstance(new clazz(...params))
        log.debug('constructor end', req.clazz, instanceId)
        reply({ result: { instanceId: instanceId } })
      }
      else if (req.type == 'static') {
        // 静态对象调用请求
        const { getClazz } = useObjectManage()
        const clazz = getClazz(req.clazz)
        if (!clazz) {
          reply({ error: 'Class not found' })
          return
        }
        if (typeof clazz[req.action] === 'function') {
          const params = req.data.params || []
          hookArgument(req.action, params)
          log.debug('static call', req.action, params)
          let result = clazz[req.action](...params);
          result = hookResult(`${req.action}_staticResult`, result)
          log.debug("static call result", req.action, result);
          reply({ result: { returnValue: result } });
        } else if (typeof clazz[req.action] === 'object') {
          let result = clazz[req.action]
          result = hookResult(`${req.action}_staticResult`, result)
          reply({ result: { returnValue: result } });
        } else if (typeof clazz[req.action] !== 'undefined') {
          const result = clazz[req.action]
          reply({ result: { returnValue: result } });
        } else {
          log.error('Method not found or instance invalid', req.action, clazz[req.action])
          reply({ error: 'Method not found or instance invalid' });
        }
      }
      else if (req.type == 'dynamic') {
        // 动态对象调用请求
        if (!req.data.instanceId) {
          log.error('InstanceId not found')
          reply({ error: 'InstanceId not found' })
          return
        }
        const { getInstance } = useInstanceManage()
        const instance = getInstance(req.data.instanceId);
        if (instance && typeof instance[req.action] === 'function') {
          const params = req.data.params || []
          log.debug("dynamic call", instance, req.action, params);
          hookArgument(req.action, params)
          const finishDynamicCall = (value: any) => {
            log.debug("dynamic call result", req.action, value);
            const result = hookResult(`${req.action}_dynamicResult`, value)
            log.debug("dynamic call result hooked", req.action, result);

            if (messageId > 0) {
              reply({
                result: {
                  returnValue: result,
                },
              });
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
          }
          const result = instance[req.action](...params);
          if (result && typeof result.then === 'function') {
            Promise.resolve(result).then(finishDynamicCall, (error: any) => {
              log.error('Async dynamic call failed:', req.action, error)
              reply({ error: error?.message || String(error) })
            })
          } else {
            finishDynamicCall(result)
          }
        } else {
          console.error('Method not found or instance invalid', req.action, instance[req.action])
          reply({ error: 'Method not found or instance invalid' });
        }
      }
      else if (req.type == 'dynamicProperty') {
        // 动态对象调用请求
        if (!req.data.instanceId) {
          console.error('InstanceId not found')
          reply({ error: 'InstanceId not found' })
          return
        }
        const { getInstance } = useInstanceManage()
        const instance = getInstance(req.data.instanceId);
        console.debug("dynamic property", req.action, instance, req.data)
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
        result = hookResult(`${req.action}_propertyResult`, result)
        log.debug("dynamic property result hooked", req.action, result);

        if (messageId > 0) {
          reply({
            result: {
              returnValue: result,
            },
          });
        }
      }
      else {
        reply({ error: 'Request type not recognized' });
      }
    }
    catch (err: any) {
      log.error('Error:', err)
      if (messageId > 0 && !replied) {
        reply({ error: err.message })
      }
    }
  });
  log.info(`✅ WebSocket Server listening on ws://localhost:${port}`);
  log.info('end....')
}
catch (err) {
  log.error('Error:', err)
}
/* The renderer entry is bundled by Vite; native RPC values are intentionally dynamic. */
