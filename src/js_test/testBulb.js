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

// Import MATRIX Proto messages
var matrix_io = require('matrix-protos').matrix_io
var async = require("async");

var zmq = require('zmq')
var configSocket = zmq.socket('push')
configSocket.connect('tcp://' + creator_ip + ':' + creator_zwave_base_port /* config */)

//------------ Starting to ping the driver -----------------------

var pingSocket = zmq.socket('push');
pingSocket.connect('tcp://' + creator_ip + ':' + (creator_zwave_base_port + 1));
pingSocket.send('');  // Ping the first time.

setInterval(function() { pingSocket.send(''); }, 1000);

function setOperation(operation) {
    var zwave_cmd = new matrixMalosBuilder.ZWaveMsg;
    zwave_cmd.set_operation(operation);
    
    var config = new matrixMalosBuilder.DriverConfig;
    config.set_zwave(zwave_cmd);
    
    configSocket.send(matrix_io.malos.v1.driver.DriverConfig.encode(zb_toggle_msg).finish());
}

function sendCommand(command) {
    
}

console.log(matrix_io.malos.v1.comm.ZWaveMsg);
async.waterfall([
    function(cb) {
        setOperation(matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.ADDNODE);
        configSocket()
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
