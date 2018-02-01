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

var protoBuf = require("protobufjs");
var protoBuilder = protoBuf.loadProtoFile('../../protocol-buffers/malos/driver.proto')
var matrixMalosBuilder = protoBuilder.build("matrix_malos")
var async = require("async");

var zmq = require('zmq')
var configSocket = zmq.socket('push')
configSocket.connect('tcp://' + creator_ip + ':' + creator_servo_base_port /* config */)

function send(operation) {
    var zwave_cmd = new matrixMalosBuilder.ZWaveMsg;
    zwave_cmd.set_operation(operation);
    
    var config = new matrixMalosBuilder.DriverConfig;
    config.set_zwave(zwave_cmd);
    return configSocket.send(config.encode().toBuffer());
}

async.waterfall([
    function(cb) {
        send(matrixMalosBuilder.ZWaveMsg.ZWaveOperations.ADDNODE);
        cb(null);
    }, function(cb) {
        console.log("NODE ADDED!");
        console.log(util.inspect(matrixMalosBuilder));
        //send()
        cb(null);
    }
], function(err) {
    if (err) console.log(err);
});
