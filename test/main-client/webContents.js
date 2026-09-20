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
  wc.once("destroyed", () => {
      console.info('webContents destroyed')
  })
}

main().catch((error) => {
  console.error(error)
  process.exitCode = 1
})
