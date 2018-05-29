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
configSocket.connect('tcp://' + creator_ip + ':' + creator_zwave_base_port /* config */)

var updateSocket = zmq.socket('sub')
updateSocket.connect('tcp://' + creator_ip + ':' + (creator_zwave_base_port + 3))
updateSocket.subscribe('')
updateSocket.on('message', (zwave_buffer) => {
	  var  zwaveObject = matrix_io.malos.v1.comm.ZWaveMsg.decode(zwave_buffer)
	   _.filter(zwaveObject.node, function(o){console.log(o.serviceName)})
	  console.log("");
});

function listNodes(){
		var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
						zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
													operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.LIST
										})
				})
return 	configSocket.send(
				  matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());
}



function addNodes(){
	var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
			zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
						operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.ADDNODE
			})
	})
	console.log("Turn off and then on your device for it to join the network \n\n")
console.log("The following devices have already joined in the Zwave network")
return configSocket.send(
		  matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());
}


console.log("The following devices have already joined in the Zwave network")
listNodes();
setTimeout(addNodes,1000)
setTimeout(listNodes, 10000);
