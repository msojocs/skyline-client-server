import { describe, it, expect } from 'vitest'
import path from 'path'

const clientNode = process.env['SKYLINE_DEV_PATH']
    ? `${process.env['SKYLINE_DEV_PATH']}/skyline.node` 
    : path.resolve(__dirname, "../packages/native/build/skyline.node")
const skylineClient = require(clientNode);



describe('基础数据获取', () => {
  it('Webview available', () => {
    expect(skylineClient.webview).toBeDefined()
  })

  it('Webview src 可用', () => {
    expect(skylineClient.webview.src).toBeDefined()
  })
})

describe('简单数据修改', () => {
  it('src修改', () => {
    const wv = skylineClient.webview
    wv.src = 'https://www.example.com'
    expect(wv.src).toBe('https://www.example.com')
  })

  it('style修改', () => {
    const wv = skylineClient.webview
    wv.style.display = 'block'
    wv.style.pointerEvents = 'none'
    expect(wv.style.display).toBe('block')
    expect(wv.style.pointerEvents).toBe('none')
  })

})

describe('请求拦截', () => {
    it('onAuthRequired', () => {
        const wv = skylineClient.webview
        const listener = (details: any) => {
            console.log('onAuthRequired', details)
            return { cancel: true }
        }
        expect(wv.request).toBeDefined()
        expect(wv.request.onAuthRequired).toBeDefined()
        expect(wv.request.onAuthRequired.constructor.name).toBe('WebRequestEvent')
        const onAuthRequired = wv.request.onAuthRequired
        onAuthRequired.addListener(listener, { urls: ['<all_urls>'] })
        const exists = onAuthRequired.hasListener(listener)
        expect(exists).toBe(true)
        onAuthRequired.removeListener(listener)
        expect(onAuthRequired.hasListener(listener)).toBe(false)
    })
    it('onMessage', () => {
        const wv = skylineClient.webview
        const listener = (details: any) => {
            console.log('onMessage', details)
            return { cancel: true }
        }
        expect(wv.request).toBeDefined()
        expect(wv.request.onMessage).toBeDefined()
        expect(wv.request.onMessage.constructor.name).toBe('RequestMessageEvent')
        const onMessage = wv.request.onMessage
        onMessage.addListener(listener)
        const exists = onMessage.hasListener(listener)
        expect(exists).toBe(true)
        onMessage.removeListener(listener)
        expect(onMessage.hasListener(listener)).toBe(false)
    })
    // it('onRequest', () => {
    //     const wv = skylineClient.webview
    //     expect(wv.request).toBeDefined()
    //     expect(wv.request.onRequest).toBeDefined()
    //     expect(wv.request.onRequest.constructor.name).toBe('RequestRule')
    //     const onRequest = wv.request.onRequest
    //     const rule = {}
    //     onRequest.addRules(rule)
    //     const exists = onRequest.getRules()
    //     expect(exists).toBe(true)
    //     onRequest.removeRules(rule)
    //     expect(onRequest.getRules()).toBe(false)
    // })
})

describe('Webview API', () => {
    it('executeScript', async () => {
        const wv = skylineClient.webview
        expect(wv.executeScript).toBeDefined()
        wv.src = 'https://www.baidu.com'
        await new Promise(resolve => setTimeout(resolve, 1000))
        const result = wv.executeScript({ code: 'console.log("Hello from Webview")' })
        expect(result).toBe(true)
    })
    it('reload', async () => {
        const wv = skylineClient.webview
        expect(wv.reload).toBeDefined()
        wv.src = 'https://www.baidu.com'
        await new Promise(resolve => setTimeout(resolve, 1000))
        const result = wv.reload()
        expect(result).toBe(true)
    })
    it('UserAgentOverride', async () => {
        const wv = skylineClient.webview
        expect(wv.setUserAgentOverride).toBeDefined()
        wv.src = 'https://ip2.us'
        await new Promise(resolve => setTimeout(resolve, 1000))
        wv.reload()
        const result = wv.setUserAgentOverride('MyCustomUserAgent/1.0')
        await new Promise(resolve => setTimeout(resolve, 1000))
        expect(result).toBe(true)
        expect(wv.getUserAgent()).toBe('MyCustomUserAgent/1.0')
        const code = new Promise((resolve, reject) => {
           const code = wv.executeScript({ code: `document.body.textContent.includes('MyCustomUserAgent/1.0')`, mainWorld: true }, (result: Array<boolean>) => {
              resolve(result?.[0])
          })
          if (!code) {
              reject(new Error('executeScript failed'))
          }
        })
        expect(await code).toBe(true)
    })
    it('eventListener', async () => {
        const wv = skylineClient.webview
        expect(wv.addEventListener).toBeDefined()
        const promise = new Promise<Event>((resolve) => {
          const listener = (event: any) => {
              console.log('Webview event:', event)
              resolve(event)
          }
          wv.addEventListener('loadcommit', listener)
        })
        // 触发事件
        wv.src = 'https://www.baidu.com'
        await new Promise(resolve => setTimeout(resolve, 500))
        wv.reload()
        await new Promise(resolve => setTimeout(resolve, 500))
        const result = await promise
        expect(result?.type).toBe('loadcommit')
    })
    it('Attribute', async () => {
        const wv = skylineClient.webview
        expect(wv.getAttribute).toBeDefined()
        expect(wv.setAttribute).toBeDefined()
        wv.setAttribute('data-test', 'testValue')
        const attrValue = wv.getAttribute('data-test')
        expect(attrValue).toBe('testValue')
    })
})

describe('dialog', () => {
    it('prompt', async () => {
        const wv = skylineClient.webview
        expect(wv.addEventListener).toBeDefined()
        wv.setAttribute('partition', 'trusted')
        const promise = new Promise<any>((resolve) => {
          const listener = (event: any) => {
                event.preventDefault()
                console.log('Webview event:', event)
                if (event.type === 'dialog') {
                    resolve(event)
                    event.returnValue = 'test input'
                    event.dialog.ok('111') // 关闭对话框
                }
            }
            wv.ondialog = listener
        })
        // 触发事件
        wv.src = 'https://www.baidu.com'
        await new Promise(resolve => setTimeout(resolve, 500))
        wv.executeScript({ code: `prompt('请输入内容')`, mainWorld: true }, (result: Array<any>) => {
            console.log('prompt result:', result)
        })
        const result = await promise
        expect(result?.type).toBe('dialog')
        expect(result?.dialog).toBeDefined()
        expect(result?.messageText).toBe('请输入内容')
    })
})