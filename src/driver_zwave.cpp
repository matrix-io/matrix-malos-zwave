/*
 * Copyright 2017 <Admobilize>
 * MATRIX Labs  [http://creator.matrix.one]
 * This file is part of MATRIX Creator MALOS
 *
 * Author: Andres Calderon <andres.calderon@admobilize.com>
 *         github.com/eljuguetero
 *
 * MATRIX Creator MALOS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include <gflags/gflags.h>

DEFINE_int32(port, 41230, "ZWaveIP gateway port");
DEFINE_string(server, "::1", "ZWaveIP Gateway ");
DEFINE_string(psk, "123456789012345678901234567890aa",
              "ZWave Pre Shared Key (PSK)");
DEFINE_string(xml, "ZWave_custom_cmd_classes.xml", "XML ZWave classes");

#include "./driver_zwave.h"

#include "./src/driver.pb.h"

#define MAX_ADDRESS_SIZE 100

#define SECURITY_0_NETWORK_KEY_BIT 0x80
#define SECURITY_2_ACCESS_CLASS_KEY 0x04
#define SECURITY_2_AUTHENTICATED_CLASS_KEY 0x02
#define SECURITY_2_UNAUTHENTICATED_CLASS_KEY 0x01

extern "C" {
uint8_t get_unique_seq_no(void) {
  static uint8_t uniq_seqno = 0;
  return uniq_seqno++;
}
}

void net_mgmt_command_handler(union evt_handler_struct evt) {
  switch (evt.dsk_report.type) {
    case APPROVE_REQUESTED_KEYS: {
      matrix_malos::ZWaveDriver::requested_keys_ =
          evt.requested_keys.requested_keys;
      matrix_malos::ZWaveDriver::csa_inclusion_requested_ =
          evt.requested_keys.csa_requested;

      std::cout << "The joining node requests these keys:\n" << std::endl;
      if (evt.requested_keys.requested_keys & SECURITY_2_ACCESS_CLASS_KEY) {
        std::cout << " * Security 2 Access/High Security key" << std::endl;
        if (evt.requested_keys.requested_keys &
            SECURITY_2_AUTHENTICATED_CLASS_KEY) {
          std::cout << " * Security 2 Authenticated/Normal key" << std::endl;
        }
        if (evt.requested_keys.requested_keys &
            SECURITY_2_UNAUTHENTICATED_CLASS_KEY) {
          std::cout << " * Security 2 Unauthenticated/Ad-hoc key" << std::endl;
        }
        if (evt.requested_keys.requested_keys & SECURITY_0_NETWORK_KEY_BIT) {
          std::cout << " * Security S0 key" << std::endl;
        }
        std::cout << "" << std::endl;
        if (evt.requested_keys.csa_requested) {
          std::cout << "and client side authentication" << std::endl;
        }
        std::cout << "Enter \'grantkeys\' to accept or \'abortkeys\' to cancel."
                  << std::endl;
      }
      break;
      case APPROVE_DSK: {
        std::cout << "The joining node is reporting this device specific key:"
                  << std::endl;
        std::cout << "Please approve by typing \'acceptdsk 12345\' where 12345 "
                     "is the "
                     "first part of the DSK.\n12345 may be omitted if the "
                     "device does "
                     "not require the Access or Authenticated keys."
                  << std::endl;

      } break;
      default:
        break;
    }
  }
}

void transmit_done(zconnection* /*zc*/, transmission_status_code_t status) {
  switch (status) {
    case TRANSMIT_OK:
      break;
    case TRANSMIT_NOT_OK:
      std::cout << "\nTransmit failed\n";
      break;
    case TRANSMIT_TIMEOUT:
      std::cout << "\nTransmit attempt timed out\n";
      break;
  }
}

namespace matrix_malos {

/* Static members of ZWaveDriver class */
bool ZWaveDriver::pan_bonnection_busy_;
uint8_t ZWaveDriver::requested_keys_;
uint8_t ZWaveDriver::csa_inclusion_requested_;
ZmqPusher* ZWaveDriver::static_zqm_push_update_;

void ZresourceMDNSHelper() { zresource_mdns_thread_func(NULL); }

ZWaveDriver::ZWaveDriver()
    : MalosBase(kZWaveDriverName),
      mdns_thread_(ZresourceMDNSHelper),
      cfg_psk_(64) /* fixed size = 64 */ {
  SetNeedsKeepalives(true);
  SetMandatoryConfiguration(true);
  SetNotesForHuman("ZWave Driver v1.0");
  pan_bonnection_busy_ = false;

  server_ip_ = FLAGS_server;

  ParsePsk(FLAGS_psk.c_str());
  std::cout << "FLAGS_psk : " << FLAGS_psk << std::endl;

  if (!initialize_xml(FLAGS_xml.c_str())) {
    std::cerr << "Could not load Command Class definitions" << std::endl;
    return;
  }

  gw_zip_connection_ = ZipConnect(server_ip_.c_str());

  zconnection_set_transmit_done_func(gw_zip_connection_, transmit_done);

  requested_keys_ = 0;
  csa_inclusion_requested_ = 0;
  net_mgmt_init(gw_zip_connection_);
}

bool ZWaveDriver::ProcessConfig(const DriverConfig& config) {
  ZwaveMsg zwave(config.zwave());

  static_zqm_push_update_ = zqm_push_update_.get();

  switch (zwave.operation()) {
    case ZwaveMsg::SEND:
      Send(zwave);
      break;
    case ZwaveMsg::ADDNODE:
      AddNode();
      break;
    case ZwaveMsg::REMOVENODE:
      RemoveNode();
      break;
    case ZwaveMsg::SETDEFAULT:
      SetDefault();
      break;
    case ZwaveMsg::LIST:
      List();
      break;
    case ZwaveMsg::UNDEF:
    default:
      // If this happens the program has to be fixed.
      std::cerr << "Invalid enum conversion. EnumMalosEyeDetectionType."
                << std::endl;
      std::exit(1);
  }

  return true;
}

bool ZWaveDriver::SendUpdate() { return true; }

void ZWaveDriver::Send(const ZwaveMsg& msg) {
  std::string cmdName;
  std::string className;

  const zw_command_class* pClass;
  const zw_command* pCmd;

  cmdName = ZwaveCmdType_Name(msg.zwave_cmd().cmd());
  className = ZwaveClassType_Name(msg.zwave_cmd().zwclass());

  pClass = zw_cmd_tool_get_class_by_name(className.c_str());
  pCmd = zw_cmd_tool_get_cmd_by_name(pClass, cmdName.c_str());

  if (!pClass || !pCmd) {
    std::cerr << "Invalid <ZWave class, command> pair." << std::endl;
    return;
  }

  const int binary_command_buffer_size = 2000;
  static unsigned char binary_command[binary_command_buffer_size];

  memset(binary_command, 0, binary_command_buffer_size);
  binary_command[0] = pClass->cmd_class_number;
  binary_command[1] = pCmd->cmd_number;

  memcpy(&binary_command[2], msg.zwave_cmd().params().c_str(),
         msg.zwave_cmd().params().length());

  int binary_commandLen =
      2 +
      msg.zwave_cmd()
          .params()
          .length();  // sizeof([class_number,cmd_number,params])

  if (0 != pan_bonnection_busy_) {
    std::cerr << "Busy, cannot send right now." << std::endl;
    return;
  }

  if (msg.device() != dest_address_) {
    if (pan_connection_) {
      zclient_stop(pan_connection_);
      pan_connection_ = NULL;
    }
    // FIXME: Use thread synchronization instead of sleep to avoid "Socket
    // Read
    // Error"
    std::this_thread::sleep_for(std::chrono::seconds(1));
    pan_connection_ = ZipConnect(msg.device().c_str());
  }
  if (!pan_connection_) {
    std::cerr << "Failed to connect to PAN node" << std::endl;
    dest_address_[0] = 0;
    return;
  }
  dest_address_ = msg.device();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  zconnection_set_transmit_done_func(pan_connection_, TransmitDonePan);
  if (zconnection_send_async(pan_connection_, binary_command, binary_commandLen,
                             0)) {
    pan_bonnection_busy_ = true;
  }
}

void ZWaveDriver::AddNode() {
  net_mgmt_learn_mode_start();

  int idx = 0;
  static uint8_t buf[200];

  const uint8_t COMMAND_CLASS_NETWORK_MANAGEMENT_INCLUSION = 0x34;
  const uint8_t NODE_ADD = 0x01;

  buf[idx++] = COMMAND_CLASS_NETWORK_MANAGEMENT_INCLUSION;
  buf[idx++] = NODE_ADD;
  buf[idx++] = get_unique_seq_no();
  buf[idx++] = 0;
  buf[idx++] = 0x07; /* ADD_NODE_S2 */
  buf[idx++] = 0;    /* Normal power, no NWI */

  zconnection_send_async(gw_zip_connection_, buf, idx, 0);
}

void ZWaveDriver::RemoveNode() {
  int idx = 0;
  static uint8_t buf[200];

  const uint8_t COMMAND_CLASS_NETWORK_MANAGEMENT_INCLUSION = 0x34;
  const uint8_t NODE_REMOVE = 0x03;

  buf[idx++] = COMMAND_CLASS_NETWORK_MANAGEMENT_INCLUSION;
  buf[idx++] = NODE_REMOVE;
  buf[idx++] = get_unique_seq_no();
  buf[idx++] = 0;
  buf[idx++] = 0x01; /* REMOVE_NODE_ANY */

  zconnection_send_async(gw_zip_connection_, buf, idx, 0);
}

void ZWaveDriver::SetDefault() {
  int idx = 0;
  static uint8_t buf[200];

  const uint8_t COMMAND_CLASS_NETWORK_MANAGEMENT_BASIC = 0x4D;
  const uint8_t DEFAULT_SET = 0x06;

  buf[idx++] = COMMAND_CLASS_NETWORK_MANAGEMENT_BASIC;
  buf[idx++] = DEFAULT_SET;
  buf[idx++] = get_unique_seq_no();

  zconnection_send_async(gw_zip_connection_, buf, idx, 0);
}

void ZWaveDriver::List() {
  ZwaveMsg msg;

  std::cout << "List of discovered Z/IP services:" << std::endl;
  for (zip_service* n = zresource_get(); n; n = n->next) {
    std::cout << " host_name:    " << n->host_name << std::endl;
    std::cout << " service_name: " << n->service_name << std::endl;
    std::cout << " infolen: " << std::dec << n->infolen << std::endl;
    std::cout << " info: " << std::endl;

    ZwaveMsg_ZwaveNode* node = msg.add_node();
    node->set_service_name(n->service_name);

    for (int i = 0; i < n->infolen; i++) {
      if (ZwaveClassType_IsValid(n->info[i])) {
        const std::string& className =
            ZwaveClassType_Name(static_cast<ZwaveClassType>(n->info[i]));

        ZwaveClassType zwave_class;
        ZwaveClassType_Parse(className, &zwave_class);

        node->add_zwave_class(zwave_class);

        std::cout << "  " << std::hex << (int)n->info[i] << " " << className
                  << std::endl;
      } else {
        std::cout << "  " << std::hex << static_cast<int>(n->info[i])
                  << " not found in the ZwaveClassType enum." << std::endl;
      }
    }
    std::cout << std::endl;
  }
  msg.set_result(true);

  std::string buffer;
  msg.SerializeToString(&buffer);
  zqm_push_update_->Send(buffer);
}

zconnection* ZWaveDriver::ZipConnect(const char* remote_addr) {
  if (cfg_psk_len_ == 0) {
    std::cerr << "PSK not configured - unable to connect to " << remote_addr
              << std::endl;
    return 0;
  }

  zconnection* zc;

  zc = zclient_start(remote_addr, 41230, reinterpret_cast<char*>(&cfg_psk_[0]),
                     cfg_psk_len_, ApplicationCommandHandler);
  if (zc == 0) {
    std::cout << "Error connecting to " << remote_addr << std::endl;
  } else {
    std::cout << "ZWaveDriver connected to " << remote_addr << std::endl;
  }
  return zc;
}

void print_hex_string(const uint8_t* data, unsigned int datalen) {
  unsigned int i;

  for (i = 0; i < datalen; i++) {
    std::cout << " " << std::hex << int(data[i]);
    if ((i & 0xf) == 0xf) {
      std::cout << std::endl;
    }
  }
  std::cout.flush();
}

void ZWaveDriver::ApplicationCommandHandler(zconnection* /*zc*/,
                                            const uint8_t* data,
                                            uint16_t datalen) {
  int i;
  int len;
  const uint8_t COMMAND_CLASS_NETWORK_MANAGEMENT_INCLUSION = 0x34;

  unsigned char cmd_classes[400][MAX_LEN_CMD_CLASS_NAME];
  std::cout << "ApplicationCommandHandler datalen=" << datalen << std::endl;

  print_hex_string(data, datalen);

  switch (data[0]) {
    case COMMAND_CLASS_NETWORK_MANAGEMENT_INCLUSION:
      parse_network_mgmt_inclusion_packet(data, datalen);
      break;

    default:
      memset(cmd_classes, 0, sizeof(cmd_classes));
      /* decode() clobbers data - but we are not using it afterwards, hence
       * the
       * typecast */
      decode((uint8_t*)data, datalen, cmd_classes, &len);
      std::cout << std::endl;
      for (i = 0; i < len; i++) {
        std::cout << " +++ " << cmd_classes[i] << std::endl;
      }
      std::cout << std::endl;
      break;
  }

  ZwaveMsg msg;

  if (datalen >= 3 && data[2])
    msg.set_result(true);
  else
    msg.set_result(false);

  std::string buffer;
  msg.SerializeToString(&buffer);
  static_zqm_push_update_->Send(buffer);
}

void ZWaveDriver::TransmitDonePan(zconnection* /*zc*/,
                                  transmission_status_code_t status) {
  std::cout << "ZWaveDriver::transmit_done_pan" << std::endl;

  switch (status) {
    case TRANSMIT_OK:
      break;
    case TRANSMIT_NOT_OK:
      std::cerr << "Transmit failed" << std::endl;
      break;
    case TRANSMIT_TIMEOUT:
      std::cerr << "Transmit attempt timed out" << std::endl;
      break;
  }
  ZWaveDriver::pan_bonnection_busy_ = false;
}

static int hex2int(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 0xa;
  } else if (c >= 'A' && c <= 'F') {
    return c - 'A' + 0xa;
  } else {
    return -1;
  }
}

void ZWaveDriver::ParsePsk(const char* psk) {
  int val;
  cfg_psk_len_ = 0;
  const char* s = psk;
  while (*s && cfg_psk_len_ < cfg_psk_.size()) {
    val = hex2int(*s++);
    if (val < 0) break;
    cfg_psk_[cfg_psk_len_] = ((val)&0xf) << 4;
    val = hex2int(*s++);
    if (val < 0) break;
    cfg_psk_[cfg_psk_len_] |= (val & 0xf);
    cfg_psk_len_++;
  }
  std::cout << std::endl;
}

}  // namespace matrix_malos
