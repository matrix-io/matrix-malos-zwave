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
var zwaveDeviceIP = ""; // Device ip 
var turnOn = "COMMAND_CLASS_SWITCH_MULTILEVEL";

var protoBuf = require("protobufjs");
var protoBuilder = protoBuf.loadProtoFile('../../protocol-buffers/malos/driver.proto')

// Import MATRIX Proto messages
var matrix_io = require('matrix-protos').matrix_io
var async = require("async");

var zmq = require('zmq')
var configSocket = zmq.socket('push')
configSocket.connect('tcp://' + creator_ip + ':' + creator_servo_base_port /* config */)

var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
    delayBetweenUpdates: 1.0,  // 1 seconds between updates.
    timeoutAfterLastPing: 1.0 // Stop sending updates 6 seconds after pings.
  });


function setOperation(operation) {
    var zwave_cmd = new matrix_io.malos.v1.comm.ZWaveMsg.create({ operation: operation });  
    configSocket.send(matrix_io.malos.v1.driver.DriverConfig.encode(zwave_cmd).finish());
}

function sendCommand(command, params) {
    var zwave_cmd = new matrix_io.malos.v1.comm.ZWaveMsg.create({
        operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.SEND,
        zwaveCmd: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveCommand.create({
            //zwclass: ,
            cmd: command,
            params: params
        })
    });

    configSocket.send(matrix_io.malos.v1.driver.DriverConfig.encode(zwave_cmd).finish());
}

console.log(matrix_io.malos.v1.comm.ZWaveMsg);
async.waterfall([
    function(cb) {
        setOperation(matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.ADDNODE);
        configSocket()
        cb(null);
    }, function(cb) {
        console.log("NODE ADDED!");
        sendCommand(command, "FF");        
        //send()
        cb(null);
    }, function(cb) {
        console.log("TURNING OFF");
        sendCommand(command, "00");
    }
], function(err) {
    if (err) console.log(err);
});
