const server = require('../build/skylineServer.node')
server.start('127.0.0.1', 3001)
server.setMessageCallback((ws, msg) => {
    console.log('Message received:', msg)
    const data = JSON.parse(msg) // Updated to use msg instead of args[0]
    if (data.action === 'start') {
        const result = server.sendMessageSync(JSON.stringify({
            type: 'pong',
            data: 'pong'
        }))
        console.info('Message sent:', result)
    }
})

process.on('SIGINT', () => {
    console.log('SIGINT received, shutting down...')
    server.stop()
    process.exit(0)
})

const sleep = (ms) => new Promise(resolve => setTimeout(resolve, ms))
;(async () => {
while (true) {
    await sleep(1000)
}
})()