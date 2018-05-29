// This is how we connect to the creator. IP and port.
// The IP is the IP I'm using and you need to edit it.
// By default, MALOS has its 0MQ ports open to the world.

// Every device is identified by a base port. Then the mapping works
// as follows:
// BasePort     => Configuration port. Used to config the device.
// BasePort + 1 => Keepalive port. Send pings to this port.
// BasePort + 2 => Error port. Receive errros from device.
// BasePort + 3 => Data port. Receive data from device.

var creator_ip = '127.0.0.1'
var creator_servo_base_port = 50001 // port for ZWave MALOS

var matrix_io = require('matrix-protos').matrix_io

var zmq = require('zmq')
var configSocket = zmq.socket('push')
configSocket.connect('tcp://' + creator_ip + ':' + creator_servo_base_port /* config */)

function addNodes(){
	var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
			zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
						operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.ADDNODE
			})
	})
return configSocket.send(
		  matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());
}

addNodes()
