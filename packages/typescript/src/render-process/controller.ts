import { useInstanceManage } from "./object-manage"

export type DialogType = 'prompt' | 'alert' | 'confirm'

export interface DialogRequest {
    requestId: string
    type: DialogType
    args: unknown[]
    guestWebContentsId?: number
}

export type DialogCallback = (
    webview: HTMLElement,
    requestId: string,
    type: DialogType,
    ...args: unknown[]
) => void

// The renderer owns one dialog route even though native clients may construct
// replacement Controller instances. Keep the callback shared across them.
let dialogCallback: DialogCallback | null = null

export class Controller {
    private container: HTMLElement
    private _webview: HTMLElement | null
    constructor(){
        const container = document.getElementById('container')
        if (!container) {
            throw new Error('Container element not found')
        }
        const webview = document.createElement('webview')
        this._webview = webview
        this.container = container
        const { removeInstanceOfType } = useInstanceManage()
        removeInstanceOfType('Controller')
    }
    get webview() {
        return this._webview
    }
    mount() {
        if (this._webview) {
            this.container.hasChildNodes() && this.container.childNodes.forEach(child => {
                this.container.removeChild(child)
            })
            this.container.appendChild(this._webview)
        } else {
            throw new Error('Webview is not initialized')
        }
    }
    unmount() {
        if (this._webview) {
            this.container.removeChild(this._webview)
            this._webview = null
        }
    }

    setDialogCallback(callback: DialogCallback | null | undefined) {
        if (callback !== null && callback !== undefined && typeof callback !== 'function') {
            throw new TypeError('Dialog callback must be a function')
        }
        console.info('[Controller] set dialog callback', { registered: typeof callback === 'function' })
        dialogCallback = callback || null
    }

    /**
     * Dispatch a dialog request without waiting on the callback. The original
     * guest renderer remains blocked in sendSync until resolveDialog is called.
     */
    dialog(webview: HTMLElement, request: DialogRequest) {
        if (!dialogCallback) return false
        const callback = dialogCallback
        console.info('[Controller] dispatch dialog', request)
        queueMicrotask(() => {
            try {
                callback(webview, request.requestId, request.type, ...request.args)
            } catch (error) {
                console.error('[Controller] dialog callback failed', request, error)
                globalThis.__skylineResolveDialog?.({
                    requestId: request.requestId,
                    error: error instanceof Error ? error.message : String(error),
                })
            }
        })
        return true
    }

    resolveDialog(requestId: string, result?: unknown) {
        if (!requestId || typeof requestId !== 'string') {
            throw new TypeError('Dialog requestId must be a non-empty string')
        }
        console.info('[Controller] resolve dialog', { requestId, result })
        globalThis.__skylineResolveDialog?.({ requestId, result })
    }
}
