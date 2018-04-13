// This is how we connect to the creator. IP and port.
// The IP is the IP I'm using and you need to edit it.
// By default, MALOS has its 0MQ ports open to the world.

// Every device is identified by a base port. Then the mapping works
// as follows:
// BasePort     => Configuration port. Used to config the device.
// BasePort + 1 => Keepalive port. Send pings to this port.
// BasePort + 2 => Error port. Receive errros from device.
// BasePort + 3 => Data port. Receive data from device.

// -------------- Dependencies --------------------------------
const matrix_io = require('matrix-protos').matrix_io;
const zmq = require('zmq');
const _ = require('lodash');
const async = require('async');

const CREATOR_IP = '127.0.0.1';
const CREATOR_ZWAVE_BASE_PORT = 50001; // port for ZWave MALOS
const CREATOR_ZWAVE_PING_PORT = CREATOR_ZWAVE_BASE_PORT + 1;
const CREATOR_ZWAVE_ERROR_PORT = CREATOR_ZWAVE_BASE_PORT + 2;
const CREATOR_ZWAVE_DATA_PORT = CREATOR_ZWAVE_BASE_PORT + 3;
const CREATOR_IMU_BASE_PORT = 20013;
const CREATOR_IMU_PING_PORT = CREATOR_IMU_BASE_PORT + 1;
const CREATOR_IMU_ERROR_PORT = CREATOR_IMU_BASE_PORT + 2;
const CREATOR_IMU_DATA_PORT = CREATOR_IMU_BASE_PORT + 3;
const IMU_PROTO = matrix_io.malos.v1.sense.Imu;
const ZWAVE_MESSAGE_PROTO = matrix_io.malos.v1.comm.ZWaveMsg;
const DRIVER_CONFIG_PROTO = matrix_io.malos.v1.driver.DriverConfig;

// For this sample we are only going to use a Multilevel Switch, change values accordingly
const ZWAVE_CLASS = matrix_io.malos.v1.comm.ZWaveClassType.COMMAND_CLASS_SWITCH_MULTILEVEL;
const ZWAVE_CMD = matrix_io.malos.v1.comm.ZWaveCmdType.SWITCH_MULTILEVEL_SET;
const DEVICE_SERVICE_NAME = 'Switch Multilevel';
let service_to_send = '';

// States, Hex values to set the Multilevel Switch
const ON = 0xFF; 
const OFF = 0x00;

var imu, zwave; //Socket objects

/**
 * Creates a zmq socket object
 * @param {String} ip Socket ip
 * @param {Number} port Socket port
 * @param {String} [type='push'] Socket type such as 'push', 'pull', sub', etc...
 * @param {Object} message_function Function to trigger for subscriptions
 */
function create_socket(ip, port, type, message_function) {
  if (!type) type = 'push';
  let socket = zmq.socket(type);
  socket.connect('tcp://' + ip + ':' + port);

  //Used for error and data sockets
  if (type === 'sub') socket.subscribe('');
  if (message_function) socket.on('message', message_function);

  return socket;
}

/**
 * Executes a function and then executes it again every ms milliseconds
 * @param {Object} a_function Function to execute
 * @param {Number} ms Time in milliseconds
 */
function do_once_then_interval(a_function, ms) {
  a_function();
  return setInterval(a_function, ms);
}

/**
 * Changes the state of the ZWave switch
 * @param {Number} value Hexadecimal value to set
 */
function set_state(value) {
  var data_storage = new Uint8Array(1);
  if (!_.isNull(service_to_send)) {
    data_storage[0] = value
    // if working with different device needs to change the zwclass and cmd
    var init_config = DRIVER_CONFIG_PROTO.create({
      zwave: ZWAVE_MESSAGE_PROTO.create({
        operation: ZWAVE_MESSAGE_PROTO.ZWaveOperations.SEND,
        serviceToSend: service_to_send,
        zwaveCmd: ZWAVE_MESSAGE_PROTO.ZWaveCommand.create({
          zwclass: ZWAVE_CLASS,
          cmd: ZWAVE_CMD,
          params: data_storage
        })
      })
    });

    return zwave.config.send(DRIVER_CONFIG_PROTO.encode(init_config).finish());
  } else console.log("No Switch Multilevel detected!");

}

// ----------------- Sockets ------------------------
imu = {
  config: create_socket(CREATOR_IP, CREATOR_IMU_BASE_PORT), //Config
  ping: create_socket(CREATOR_IP, CREATOR_IMU_PING_PORT), //Keep alive
  error: create_socket(CREATOR_IP, CREATOR_IMU_ERROR_PORT, 'sub', () => {
    console.log('Message received: IMU error: ', error_message.toString('utf8'));
   }) //Errors
};

zwave = {
  config: create_socket(CREATOR_IP, CREATOR_ZWAVE_BASE_PORT), //Config
  ping: create_socket(CREATOR_IP, CREATOR_ZWAVE_PING_PORT), //Keep alive
  error: create_socket(CREATOR_IP, CREATOR_ZWAVE_ERROR_PORT, 'sub', () => { 
    console.log('Message received: ZWave error: ', error_message.toString('utf8'));
  }) //Errors
};

//Ping once, then keep pinging every 500ms
do_once_then_interval(() => {
  zwave.ping.send('');
  imu.ping.send('');
}, 500);


// -------------- IMU Configuration --------------------------
var imu_base_config_params = DRIVER_CONFIG_PROTO.create({
  delayBetweenUpdates: 1.0,  // 2 seconds between updates
  timeoutAfterLastPing: 6.0  // Stop sending updates 6 seconds after pings.
});

// -------------- ZWAVE Configuration --------------------------
//Prepare a list command
var zwave_list_config_params = DRIVER_CONFIG_PROTO.create({
  zwave: ZWAVE_MESSAGE_PROTO.create({
    operation: ZWAVE_MESSAGE_PROTO.ZWaveOperations.LIST
  })
});

// -------------- IMU data handler --------------------------

//According to the accelerometer Z value we are going to change the device value
imu.data = create_socket(CREATOR_IP, CREATOR_IMU_DATA_PORT, 'sub', (buffer) => {
  var imu_data = IMU_PROTO.decode(buffer);
  if (imu_data.accelZ < 0) set_state(OFF);
  else set_state(ON);
}); //Used to receive IMU data

// -------------- ZWave data handler --------------------------

zwave.data = create_socket(CREATOR_IP, CREATOR_ZWAVE_DATA_PORT, 'sub', (buffer) => {
  var zwave_data = ZWAVE_MESSAGE_PROTO.decode(buffer);
  
  async.map(zwave_data.node, (node, cb) => { //Find the matching node
    if (node.serviceName.includes(DEVICE_SERVICE_NAME)) cb(node.serviceName);
  }, (result) => {
    service_to_send = result;
  })
}); //Used to receive ZWave data


// -------------- Execute code --------------------------

//Send list command
console.log('ZWave list');
zwave.config.send(DRIVER_CONFIG_PROTO.encode(zwave_list_config_params).finish());

//Configure IMU so we can interact with accelerometer
console.log('Change device state on accelerometer changes');
imu.config.send(DRIVER_CONFIG_PROTO.encode(imu_base_config_params).finish());