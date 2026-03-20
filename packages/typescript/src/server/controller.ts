import { useInstanceManage } from "./object-manage"

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
}