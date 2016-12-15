var net = require('net')
var options = {
  port : 5012,
  host: "0.0.0.0"
}

var server = net.createServer( (sock) => {
  log ("new connection: "+sock.address().address +":"+sock.address().port);
  sock.setEncoding('utf8')
  sock.on('data', (data) => {
    handleData(sock, data)
  })
  sock.on('error', (err) => { log(err.toString())})
  //sock.on('close', () => {})
  //sock.on('end', () => {})
  //sock.on('timeout', () =>{})

})

server.on ( 'error', (err) => { log(err.toString()) })
server.on ( 'listening', () => { log(JSON.stringify(server.address())) })
server.listen(options)
console.log("listening?")



var partial_data = {}
function handleData (sock, data) {
  var _data = ""
  if (partial_data[sock]) {
    _data += partial_data[sock]
    delete(partial_data[sock])
  }
  _data += data
  var packets = _data.split(/\n/)
  for (var i =0; i!=packets.length-1; ++i) {
    handlePacket(packets[i])
  }
  // last (or only str in packets is incomplete, use it to add to subsequent data
  partial_data[sock] = packets[packets.length-1]
}

var count = 0
function handlePacket(packet) {
  var match = packet.match(/(\d+) ([a-f0-9:]+) (\d+) (\d+)/)
  if (match) {
    // "S <sampleNum> <ts> <mac> <vnox> <vred>"
    log (`S ${++count} ${match[1]} ${match[2]} ${match[3]} ${match[4]}`)
  } else {
    log (`${packet}`)
  }
}

function log (msg) {
  console.log(`${Date.now()} : ${msg}`)
}
