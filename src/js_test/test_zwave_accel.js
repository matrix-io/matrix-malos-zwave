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
let serviceToSend = ''; // Only searching for Switch Multilevel devices

// ------------- IMU ------------------------------------------
const creator_imu_base_port = 20013


// -------------- Stuffs needed -------------------------------
var matrix_io = require('matrix-protos').matrix_io
var zmq = require('zmq')
var _ = require('lodash');
var configImuSocket = zmq.socket('push')
var configZWaveSocket = zmq.socket('push')
var receiveListInfo = zmq.socket('sub');

// ----------------- Socket connection -----------------------
configImuSocket.connect('tcp://' + creator_ip + ':' + creator_imu_base_port)
configZWaveSocket.connect('tcp://' + creator_ip + ':' + creator_zwave_base_port /* config */)
receiveListInfo.connect('tcp://' + creator_ip + ':' + creator_zwave_base_port + 3)

// -------------- IMU Configuration --------------------------
var config = matrix_io.malos.v1.driver.DriverConfig.create({
  delayBetweenUpdates: 1.0,  // 2 seconds between updates
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
}, 500);

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

var data = new Uint8Array(1);

function list(){
    var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
        zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
            operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.LIST
        })
    });	
    
    return configZWaveSocket.send(
            matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());        
}


function set_state(value){
    receiveListInfo.subscribe('');

    receiveListInfo.on('message', (data) => {
        var zw = matrix_io.malos.v1.comm.ZWaveMsg.decode(data);
        async.map(zw.node, function(node, cb){
            if (node.serviceName.contains('Switch Multilevel')) // change here if working with different device
                cb(node.serviceName);
        }, function(result){
            serviceToSend = result;
        })
    })
    if(!_.isNull(serviceToSend)){
        data[0] = value
        // if working with different device needs to change the zwclass and cmd
        var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
                zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
                    operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.SEND,
                    serviceToSend: serviceToSend,
                    zwaveCmd: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveCommand.create({
                        zwclass: matrix_io.malos.v1.comm.ZWaveClassType.COMMAND_CLASS_SWITCH_MULTILEVEL,
			            cmd: matrix_io.malos.v1.comm.ZWaveCmdType.SWITCH_MULTILEVEL_SET,
                        params: data
                    })
                })                 
            });

            return configZWaveSocket.send(matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());
    } else console.log("No Switch Multilevel detected!");
    
}

function toggle(){
    var receiveData = zmq.socket('sub')

    receiveData.connect('tcp://' + creator_ip + ':' + (creator_imu_base_port + 3))
    receiveData.subscribe('')
    receiveData.on('message', (imu_buffer) => {
        var imuData = matrix_io.malos.v1.sense.Imu.decode(imu_buffer);
        if (imuData.accelZ < 0) {
            set_state(0x00)
        } else {
            set_state(0xFF)
        }
    });
}
list()
toggle()