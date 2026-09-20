const path = require('path')
const p = path.resolve(__dirname, "../../packages/native/build/x86_64-unknown-linux-gnu/main-client.node")
const { mainController } = require(p)

console.info(mainController)

async function main() {
  await mainController.connect('127.0.0.1', 3002)

  // Try fromId first
  console.info('\nTesting fromId...')
  const wc = mainController.electron.webContents.fromId(1)
  console.info('fromId(1).id:', wc.id)

  // Try getAllWebContents
  console.info('\nTesting getAllWebContents...')
  const list = mainController.electron.webContents.getAllWebContents()
  console.info('allWebContents:', list.length)
  const target = list[0]
  console.info('target.id:', target.id)
  console.info('target.session:', target.session)
  console.info('target.session.webRequest:', target.session.webRequest)

  // Test webRequest methods
  console.info('\nTesting webRequest methods...')
  const webRequest = target.session.webRequest
  console.info('webRequest.onBeforeRequest:', typeof webRequest.onBeforeRequest)
  console.info('webRequest.onBeforeSendHeaders:', typeof webRequest.onBeforeSendHeaders)

  console.info('\n✅ All tests passed!')
}

main().catch((error) => {
  console.error(error)
  process.exitCode = 1
})
