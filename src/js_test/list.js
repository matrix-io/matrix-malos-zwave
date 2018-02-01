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
var creator_zwave_base_port = 50001 // port for ZWave MALOS

var matrix_io = require('matrix-protos').matrix_io

var zmq = require('zmq')
var configSocket = zmq.socket('push')
configSocket.connect('tcp://' + creator_ip + ':' + creator_zwave_base_port /* config */)

// ------------ Starting to ping the driver -----------------------

var pingSocket = zmq.socket('push');
pingSocket.connect('tcp://' + creator_ip + ':' + (creator_zwave_base_port + 1));
pingSocket.send('');  // Ping the first time.

setInterval(function() { pingSocket.send(''); }, 1000);

//-----  Print the errors that the ZigBee driver sends ------------

var errorSocket = zmq.socket('sub'); 
errorSocket.connect('tcp://' + creator_ip + ':' +
	                    (creator_zwave_base_port + 2));
errorSocket.subscribe('');
errorSocket.on('message', function(error_message) {
	  process.stdout.write('Message received: ' + error_message.toString('utf8') +
		                         "\n");
});

// ------------ Starting to ping the driver -----------------------

var pingSocket = zmq.socket('push');
pingSocket.connect('tcp://' + creator_ip + ':' + (creator_zwave_base_port + 1));
pingSocket.send('');  // Ping the first time.

setInterval(function() { pingSocket.send(''); }, 1000);

function listNodes(){
var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
	zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
		operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.LIST
})
});	

return configSocket.send(
	  matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());
}

function addNodes(){
var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
	zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
		operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.ADDNODE
})
});	

return configSocket.send(
	  matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());
}

function removeNode(){
var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
	zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
		operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.REMOVENODE
})
});	

return configSocket.send(
	  matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());
}

var param = new Uint8Array(1);
param[0]=0x00;

function test(){
var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
	zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
		operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.SEND,
		serviceToSend: "10.0.0.137",
		zwaveCmd: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveCommand.create({
			zwclass: matrix_io.malos.v1.comm.ZWaveClassType.COMMAND_CLASS_SWITCH_BINARY,
			cmd: matrix_io.malos.v1.comm.ZWaveCmdType.SWITCH_BINARY_SET,
			params: param
		})
	})
});

return configSocket.send(
	  matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());
}
listNodes()
test()
