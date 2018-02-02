// This is how we connect to the creator. IP and port.
// The IP is the IP I'm using and you need to edit it.
// By default, MALOS has its 0MQ ports open to the world.

// Every device is identified by a base port. Then the mapping works
// as follows:
// BasePort     => Configuration port. Used to config the device.
// BasePort + 1 => Keepalive port. Send pings to this port.
// BasePort + 2 => Error port. Receive errros from device.
// BasePort + 3 => Data port. Receive data from device.


// ------------ Z-Wave ----------------------------------------
const creator_ip = '127.0.0.1'
const creator_zwave_base_port = 50001 // port for ZWave MALOS

// ------------- IMU ------------------------------------------
const creator_imu_base_port = 20013


// -------------- Stuffs needed -------------------------------
var matrix_io = require('matrix-protos').matrix_io
var zmq = require('zmq')
var configImuSocket = zmq.socket('push')
var configZWaveSocket = zmq.socket('push')

// ----------------- Socket connection -----------------------
configImuSocket.connect('tcp://' + creator_ip + ':' + creator_imu_base_port)
configZWaveSocket.connect('tcp://' + creator_ip + ':' + creator_zwave_base_port /* config */)

// -------------- IMU Configuration --------------------------
var config = matrix_io.malos.v1.driver.DriverConfig.create({
  delayBetweenUpdates: 2.0,  // 2 seconds between updates
  timeoutAfterLastPing: 6.0  // Stop sending updates 6 seconds after pings.
})

configImuSocket.send(matrix_io.malos.v1.driver.DriverConfig.encode(config).finish())


// ------------ Starting to ping the driver -----------------------

var pingZWave = zmq.socket('push');
var pingImu = zmq.socket('push');

pingZWave.connect('tcp://' + creator_ip + ':' + (creator_zwave_base_port + 1));
pingZWave.send('');  // Ping the first time.


pingImu.connect('tcp://' + creator_ip + ':' + (creator_imu_base_port + 1));
pingImu.send(''); // Ping the first time.
setInterval(() => {
  pingImu.send('');
  pingZWave.send('');
}, 5000);

//-----  Print the errors that the Zwave driver sends ------------

var errorZWave = zmq.socket('sub'); 
errorZWave.connect('tcp://' + creator_ip + ':' +
	                    (creator_zwave_base_port + 2));
errorZWave.subscribe('');
errorZWave.on('message', function(error_message) {
	  process.stdout.write('Message received: ' + error_message.toString('utf8') +
		                         "\n");
});

//-----  Print the errors that the IMU driver sends ------------
var errorImu = zmq.socket('sub')
errorImu.connect('tcp://' + creator_ip + ':' + (creator_imu_base_port + 2))
errorImu.subscribe('')
errorImu.on('message', (error_message) => {
  console.log('Message received: IMU error: ', error_message.toString('utf8'))
});

function listNodes(){
    var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
        zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
            operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.LIST
            })
        });	

    return configZWaveSocket.send(
        matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());
}

function addNodes(){
    var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
        zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
            operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.ADDNODE
            })  
        });	

    return configZWaveSocket.send(
        matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());
}

function removeNode(){
    var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
        zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
            operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.REMOVENODE
            })
        });	

    return configZWaveSocket.send(
        matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());
}

let off = new Uint8Array(1);
let on = new Uint8Array(1);

off[0] = 0x00;
on[0] = 0xFF;

function toggle(){
    var receiveData = zmq.socket('sub')

    receiveData.connect('tcp://' + creator_ip + ':' + (creator_imu_base_port + 3))
    receiveData.subscribe('')
    receiveData.on('message', (imu_buffer) => {
        var imuData = matrix_io.malos.v1.sense.Imu.decode(imu_buffer);
        if (imuData.accelX < 0 && imuData.accelY < 0 && imuData.accelZ < 0) {
            var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
                zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
                    operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.SEND,
                    serviceToSend: "Switch Multilevel [f02e99ae2000]",
                    zwaveCmd: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveCommand.create({
                        zwclass: matrix_io.malos.v1.comm.ZWaveClassType.COMMAND_CLASS_SWITCH_MULTILEVEL,
                        cmd: matrix_io.malos.v1.comm.ZWaveCmdType.SWITCH_MULTILEVEL_SET,
                        params: off
                    })
                })                 
            });
            return configZWaveSocket.send(matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());            
        } else {
            var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
                zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
                    operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.SEND,
                    serviceToSend: "Switch Multilevel [f02e99ae2000]",
                    zwaveCmd: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveCommand.create({
                        zwclass: matrix_io.malos.v1.comm.ZWaveClassType.COMMAND_CLASS_SWITCH_MULTILEVEL,
                        cmd: matrix_io.malos.v1.comm.ZWaveCmdType.SWITCH_MULTILEVEL_SET,
                        params: on
                    })
                })                 
            });
            return configZWaveSocket.send(matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());        
        }
    });
}
var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
	zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
		operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.SEND,
//		serviceToSend: "10.0.0.137",
		serviceToSend: "Switch Binary [f7abf7fc0600]",
		zwaveCmd: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveCommand.create({
			zwclass: matrix_io.malos.v1.comm.ZWaveClassType.COMMAND_CLASS_SWITCH_BINARY,
			cmd: matrix_io.malos.v1.comm.ZWaveCmdType.SWITCH_BINARY_SET,
			params: param
		})
	})
});

//listNodes()
addNodes()
toggle()