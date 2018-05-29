// This is how we connect to the creator. IP and port.
// The IP is the IP I'm using and you need to edit it.
// By default, MALOS has its 0MQ ports open to the world.

// Every device is identified by a base port. Then the mapping works
// as follows:
// BasePort     => Configuration port. Used to config the device.
// BasePort + 1 => Keepalive port. Send pings to this port.
// BasePort + 2 => Error port. Receive errros from device.
// BasePort + 3 => Data port. Receive data from device.

// -------------- Dependencies -------------------------------
var matrix_io = require('matrix-protos').matrix_io;
var zmq = require('zmq');
var _ = require('lodash');
var async = require('async');

// ------------ Z-Wave ----------------------------------------

const DEFAULT_PING_INTERVAL = 500;
const CREATOR_IP = '127.0.0.1';
const CREATOR_ZWAVE_BASE_PORT = 50001; // port for ZWave MALOS
const CREATOR_ZWAVE_PING_PORT = CREATOR_ZWAVE_BASE_PORT + 1;
const CREATOR_ZWAVE_ERROR_PORT = CREATOR_ZWAVE_BASE_PORT + 2;
const CREATOR_ZWAVE_DATA_PORT = CREATOR_ZWAVE_BASE_PORT + 3;

/*
const DEVICE_TO_USE = {
  name: 'Switch Multilevel',
  class: 'COMMAND_CLASS_SWITCH_MULTILEVEL',
  command: 'SWITCH_MULTILEVEL_SET'
};
*/

const DEVICE_TO_USE = {
  name: 'Switch Binary',
  class: 'COMMAND_CLASS_SWITCH_BINARY',
  command: 'SWITCH_BINARY_SET'
};

DEVICE_TO_USE.classNumber = matrix_io.malos.v1.comm.ZWaveClassType[DEVICE_TO_USE.class];

let serviceToSend = ''; // The device we want to send this to, needs to be found

// ------------- IMU ------------------------------------------
const CREATOR_IMU_BASE_PORT = 20013;
const CREATOR_IMU_PING_PORT = CREATOR_IMU_BASE_PORT + 1;
const CREATOR_IMU_ERROR_PORT = CREATOR_IMU_BASE_PORT + 2;
const CREATOR_IMU_DATA_PORT = CREATOR_IMU_BASE_PORT + 3;

const HEX_ON = 0xFF;
const HEX_OFF = 0x00;

/**
 * @param {string} ip Device IP
 * @param {number} port Port
 * @param {object} options Object to specify parameters. { pingInterval: number}
 * @param {string} type Socket type ('push'|'sub')
 */
function createSocket(ip, port, options, type) {
  if (!type) type = 'push';
  var socket = zmq.socket(type);

  socket.connect('tcp://' + ip + ':' + (port));
  socket.send(''); //Pings once

  if (type === 'sub') socket.subscribe('');

  if (options && options.pingInterval) {
    setInterval(() => {
      socket.send('');
    }, options.pingInterval);
  }

  return socket;
}


//Socket collection used to send and read ZWave commands
var zwave = {
  config: createSocket(CREATOR_IP, CREATOR_ZWAVE_BASE_PORT),
  ping: createSocket(CREATOR_IP, CREATOR_ZWAVE_PING_PORT, {
    pingInterval: DEFAULT_PING_INTERVAL
  }),
  error: createSocket(CREATOR_IP, CREATOR_ZWAVE_ERROR_PORT, {}, 'sub'),
  data: createSocket(CREATOR_IP, CREATOR_ZWAVE_DATA_PORT, {}, 'sub'),
};

//-----  Print the errors that the Zwave driver sends ------------
zwave.error.on('message', function(err) {
  process.stdout.write('Message received: ' + err.toString('utf8') + "\n");
});


function list() {
  console.log('List!');
  var list_config = matrix_io.malos.v1.driver.DriverConfig.create({
    zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
      operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.LIST
    })
  });

  return zwave.config.send(matrix_io.malos.v1.driver.DriverConfig.encode(list_config).finish());
}

//Used to send and read IMU commands
var imu = {
  config: createSocket(CREATOR_IP, CREATOR_IMU_BASE_PORT),
  ping: createSocket(CREATOR_IP, CREATOR_IMU_PING_PORT, {
    pingInterval: DEFAULT_PING_INTERVAL
  }),
  error: createSocket(CREATOR_IP, CREATOR_IMU_ERROR_PORT, {}, 'sub'),
  data: createSocket(CREATOR_IP, CREATOR_IMU_DATA_PORT, {}, 'sub'),
};

// -------------- IMU Configuration --------------------------
var imuListenCommand = matrix_io.malos.v1.driver.DriverConfig.create({
  delayBetweenUpdates: 2.0, // 2 seconds between updates
  timeoutAfterLastPing: 6.0 // Stop sending updates 6 seconds after pings.
});

//Start listening for IMU updates (MALOS)
imu.config.send(matrix_io.malos.v1.driver.DriverConfig.encode(imuListenCommand).finish());

//Print IMU errors
imu.error.on('message', (err) => {
  console.log('Message received: IMU error: ', err.toString('utf8'));
});


//Handle IMU data
function set_state(value) {
  console.log('Updating state! (' + value + ')');
  if (!_.isNull(serviceToSend)) {
    var paramsData = new Uint8Array(1);
    paramsData[0] = value;

    var init_config = matrix_io.malos.v1.driver.DriverConfig.create({
      zwave: matrix_io.malos.v1.comm.ZWaveMsg.create({
        operation: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveOperations.SEND, //A message to be sent
        serviceToSend: serviceToSend,
        zwaveCmd: matrix_io.malos.v1.comm.ZWaveMsg.ZWaveCommand.create({
          zwclass: matrix_io.malos.v1.comm.ZWaveClassType[DEVICE_TO_USE.class],
          cmd: matrix_io.malos.v1.comm.ZWaveCmdType[DEVICE_TO_USE.command],
          params: paramsData
        })
      })
    });

    return zwave.config.send(matrix_io.malos.v1.driver.DriverConfig.encode(init_config).finish());
  } else {
    console.log('No', DEVICE_TO_USE.name, 'detected!');
  }

}

var pastState;

function toggle() {
  console.log('Enabled Z axis triggering!');

  imu.data.on('message', (buffer) => {
    var newState;
    var data = matrix_io.malos.v1.sense.Imu.decode(buffer);
    newState = (data.accelZ < 0) ? HEX_OFF : HEX_ON;
    if (newState != pastState) {
      console.log('Changing to:', newState);
      pastState = newState;
      set_state(newState);
    }
  });
}

//Listen to ZWave activity
zwave.data.on('message', (data) => {

  console.log('ZWave data!');
  var zw = matrix_io.malos.v1.comm.ZWaveMsg.decode(data);

  //Look for the device we want to use
  async.map(zw.node, function(node, cb) {
    if (node.serviceName.includes(DEVICE_TO_USE.name)) {
      console.log('Service found:', node.serviceName);

      //Use just this if you don't want to list the commands ///
      //cb(node.serviceName);
      //////////////////////////////////////////////////////////

      //Use this instead to list the commands ///////////////////////////
      async.some(node.zwaveClass, function(zwaveClass, cb) {
        if (zwaveClass.zwaveClass === DEVICE_TO_USE.classNumber) {
          //Class found, list the commands
          console.log('Commands found:');
          async.each(zwaveClass.command, function(command, next) {
            console.log(JSON.stringify(command));
            next();
          }, cb);
        } else {
          cb(null, false);
        }
      }, () => {
        cb(node.serviceName);
      });
      ////////////////////////////////////////////////////////////////////
    }
  }, function(result) {
    serviceToSend = result;
  });
});

list(); //List ZWave devices
toggle();