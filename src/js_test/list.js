// This is how we connect to the creator. IP and port.
// The IP is the IP I'm using and you need to edit it.
// By default, MALOS has its 0MQ ports open to the world.

// Every device is identified by a base port. Then the mapping works
// as follows:
// BasePort     => Configuration port. Used to config the device.
// BasePort + 1 => Keepalive port. Send pings to this port.
// BasePort + 2 => Error port. Receive errros from device.
// BasePort + 3 => Data port. Receive data from device.

var _ = require('lodash');

var creator_ip = '127.0.0.1'
var creator_zwave_base_port = 50001 // port for ZWave MALOS

var matrix_io = require('matrix-protos').matrix_io

var zmq = require('zmq')
var configSocket = zmq.socket('push')
configSocket.connect('tcp://' + creator_ip + ':' + creator_zwave_base_port /* config */ )

//-----  Print the errors that the Zwave driver sends ------------

var errorSocket = zmq.socket('sub');
errorSocket.connect('tcp://' + creator_ip + ':' +
  (creator_zwave_base_port + 2));
errorSocket.subscribe('');
errorSocket.on('message', function(error_message) {
  process.stdout.write('Message received: ' + error_message.toString('utf8') +
    "\n");
});

var updateSocket = zmq.socket('sub')
updateSocket.connect('tcp://' + creator_ip + ':' + (creator_zwave_base_port + 3))
updateSocket.subscribe('')
updateSocket.on('message', (zwave_buffer) => {
  var zwaveObject = matrix_io.malos.v1.comm.ZWaveMsg.decode(zwave_buffer)
  _.filter(zwaveObject.node, function(o) {
    console.log(o.serviceName)
  })
  process.exit(0)
});

function listNodes() {
  var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
    zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
      operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.LIST
    })
  })
  return configSocket.send(
    matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());
}

listNodes();