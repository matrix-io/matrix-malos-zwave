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

#include "./driver_zwave.h"
#include <arpa/inet.h>
#include <gflags/gflags.h>
#include <matrix_io/malos/v1/driver.pb.h>

#include <valarray>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

DEFINE_int32(port, 41230, "ZWaveIP gateway port");
DEFINE_string(server, "::1", "ZWaveIP Gateway ");
DEFINE_string(psk, "123456789012345678901234567890aa",
              "ZWave Pre Shared Key (PSK)");
DEFINE_string(xml, "ZWave_custom_cmd_classes.xml", "XML ZWave classes");

#define MAX_ADDRESS_SIZE 100

#define SECURITY_0_NETWORK_KEY_BIT 0x80
#define SECURITY_2_ACCESS_CLASS_KEY 0x04
#define SECURITY_2_AUTHENTICATED_CLASS_KEY 0x02
#define SECURITY_2_UNAUTHENTICATED_CLASS_KEY 0x01

namespace pb = matrix_io::malos::v1;

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

  pan_connection_ = NULL;
}

bool ZWaveDriver::ProcessConfig(const pb::driver::DriverConfig& config) {
  pb::comm::ZWaveMsg zwave(config.zwave());

  static_zqm_push_update_ = zqm_push_update_.get();

  switch (zwave.operation()) {
    case pb::comm::ZWaveMsg::SEND:
      Send(zwave);
      break;
    case pb::comm::ZWaveMsg::ADDNODE:
      AddNode();
      break;
    case pb::comm::ZWaveMsg::REMOVENODE:
      RemoveNode();
      break;
    case pb::comm::ZWaveMsg::SETDEFAULT:
      SetDefault();
      break;
    case pb::comm::ZWaveMsg::LIST:
      List();
      break;
    case pb::comm::ZWaveMsg::UNDEF:
    default:
      // If this happens the program has to be fixed.
      std::cerr << "Invalid enum conversion. EnumMalosEyeDetectionType."
                << std::endl;
      std::exit(1);
  }

  return true;
}

bool ZWaveDriver::SendUpdate() {
  /* unimplemented yet */
  return true;
}

void ZWaveDriver::Send(const pb::comm::ZWaveMsg& msg) {
  std::string cmd_name = ZWaveCmdType_Name(msg.zwave_cmd().cmd());
  std::string class_name = ZWaveClassType_Name(msg.zwave_cmd().zwclass());

  const zw_command_class* p_class =
      zw_cmd_tool_get_class_by_name(class_name.c_str());
  const zw_command* p_cmd =
      zw_cmd_tool_get_cmd_by_name(p_class, cmd_name.c_str());

  if (!p_class || !p_cmd) {
    std::cerr << "Invalid <ZWave class, command> pair." << std::endl;
    return;
  }

  const int binary_command_buffer_size = 2000;
  static unsigned char binary_command[binary_command_buffer_size];

  memset(binary_command, 0, binary_command_buffer_size);
  binary_command[0] = p_class->cmd_class_number;
  binary_command[1] = p_cmd->cmd_number;

  memcpy(&binary_command[2], msg.zwave_cmd().params().c_str(),
         msg.zwave_cmd().params().length());

  int binary_commandLen =
      2 +
      msg.zwave_cmd()
          .params()
          .length();  // sizeof([class_number,cmd_number,params])

  if (pan_bonnection_busy_) {
    std::cerr << "Busy, cannot send right now." << std::endl;
    return;
  }

  if (msg.service_to_send() != dest_address_) {
    if (pan_connection_) {
      zclient_stop(pan_connection_);
      pan_connection_ = NULL;
    }
    // FIXME: Use thread synchronization instead of sleep to avoid "Socket
    //        Read Error"
    std::this_thread::sleep_for(std::chrono::seconds(1));
    pan_connection_ = ZipConnect(msg.service_to_send().c_str());
  }
  if (!pan_connection_) {
    std::cerr << "Failed to connect to PAN node" << std::endl;
    dest_address_[0] = 0;
    return;
  }
  dest_address_ = msg.service_to_send();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  zconnection_set_transmit_done_func(pan_connection_, TransmitDonePan);
  if (zconnection_send_async(pan_connection_, binary_command, binary_commandLen,
                             0)) {
    pan_bonnection_busy_ = true;
  }
}

void ZWaveDriver::AddNode() {
  net_mgmt_learn_mode_start();

  const uint8_t COMMAND_CLASS_NETWORK_MANAGEMENT_INCLUSION = 0x34;
  const uint8_t NODE_ADD = 0x01;

  static uint8_t buf[] = {
      COMMAND_CLASS_NETWORK_MANAGEMENT_INCLUSION, NODE_ADD, get_unique_seq_no(),
      0, 0x07 /* ADD_NODE_S2 */, 0 /* Normal power, no NWI */
  };

  zconnection_send_async(gw_zip_connection_, buf, sizeof(buf), 0);
}

void ZWaveDriver::RemoveNode() {
  const uint8_t COMMAND_CLASS_NETWORK_MANAGEMENT_INCLUSION = 0x34;
  const uint8_t NODE_REMOVE = 0x03;

  static uint8_t buf[] = {
      COMMAND_CLASS_NETWORK_MANAGEMENT_INCLUSION, NODE_REMOVE,
      get_unique_seq_no(), 0, 0x01 /* REMOVE_NODE_ANY */
  };
  zconnection_send_async(gw_zip_connection_, buf, sizeof(buf), 0);
}

void ZWaveDriver::SetDefault() {
  const uint8_t COMMAND_CLASS_NETWORK_MANAGEMENT_BASIC = 0x4D;
  const uint8_t DEFAULT_SET = 0x06;

  static uint8_t buf[] = {COMMAND_CLASS_NETWORK_MANAGEMENT_BASIC, DEFAULT_SET,
                          get_unique_seq_no()};

  zconnection_send_async(gw_zip_connection_, buf, sizeof(buf), 0);
}

void ZWaveDriver::List() {
  pb::comm::ZWaveMsg msg;

  std::cout << "List of discovered Z/IP services:" << std::endl;
  for (zip_service* n = zresource_get(); n; n = n->next) {
    std::cout << " host_name:    " << n->host_name << std::endl;
    std::cout << " service_name: " << n->service_name << std::endl;
    std::cout << " infolen: " << std::dec << n->infolen << std::endl;
    std::cout << " info: " << std::endl;

    pb::comm::ZWaveMsg_ZWaveNode* node = msg.add_node();
    node->set_service_name(n->service_name);

    for (int i = 0; i < n->infolen; i++) {
      if (pb::comm::ZWaveClassType_IsValid(n->info[i])) {
        const std::string& class_name = pb::comm::ZWaveClassType_Name(
            static_cast<pb::comm::ZWaveClassType>(n->info[i]));

        pb::comm::ZWaveClassType zwave_class_type;
        pb::comm::ZWaveClassType_Parse(class_name, &zwave_class_type);

        pb::comm::ZWaveMsg_ZWaveClassInfo* p_zwave_class =
            node->add_zwave_class();

        p_zwave_class->set_zwave_class(zwave_class_type);

        const zw_command_class* p_cmd_class =
            zw_cmd_tool_get_class_by_name(class_name.c_str());

        std::valarray<const char*> cmd_names(512);

        int number_of_commands =
            zw_cmd_tool_get_cmd_names(p_cmd_class, &cmd_names[0]);

        for (auto& cmd_name : std::valarray<const char*>(
                 cmd_names[std::slice(0, number_of_commands, 1)])) {
          pb::comm::ZWaveCmdType zwave_cmd_type;
          pb::comm::ZWaveCmdType_Parse(cmd_name, &zwave_cmd_type);

          pb::comm::ZWaveMsg_ZWaveCommandInfo* zwave_cmd_info =
              p_zwave_class->add_command();
          zwave_cmd_info->set_cmd(zwave_cmd_type);

          std::valarray<const char*> param_names(512);

          const zw_command* p_zw_command =
              zw_cmd_tool_get_cmd_by_name(p_cmd_class, cmd_name);

          int number_of_params =
              zw_cmd_tool_get_param_names(p_zw_command, &param_names[0]);

          for (auto& param_name : std::valarray<const char*>(
                   param_names[std::slice(0, number_of_params, 1)])) {
            zwave_cmd_info->add_param(param_name);
          }
        }

        std::cout << "  " << std::hex << static_cast<int>(n->info[i]) << " "
                  << class_name << std::endl;
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
  char address[256];
  memset(address, 0, 256);
  memcpy(address, remote_addr, strlen(remote_addr));

  for (zip_service* n = zresource_get(); n; n = n->next) {
    const char* res;
    /* Try connecting via IPv6 first */
    res = inet_ntop(n->addr6.sin6_family, &n->addr6.sin6_addr, address,
                    sizeof(struct sockaddr_in6));
    if (!res) {
      /* fallback to IPv4 */
      res = inet_ntop(n->addr.sin_family, &n->addr.sin_addr, address,
                      sizeof(struct sockaddr_in));
      if (!res) {
        std::cerr << "Invalid destination address." << std::endl;
        return 0;
      }
    }
  }
  zconnection* zc;

  zc = zclient_start(address, 41230, reinterpret_cast<char*>(&cfg_psk_[0]),
                     cfg_psk_len_, ApplicationCommandHandler);
  if (zc == 0) {
    std::cout << "Error connecting to " << remote_addr << std::endl;
  } else {
    std::cout << "ZWaveDriver connected to " << remote_addr << "-" << address
              << std::endl;
  }
  return zc;
}

void print_hex_string(const uint8_t* data, unsigned int datalen) {
  for (unsigned int i = 0; i < datalen; i++) {
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
       * the typecast */
      int len;
      decode((uint8_t*)(data), datalen, cmd_classes, &len);
      std::cout << std::endl;
      for (int i = 0; i < len; i++) {
        std::cout << " +++ " << cmd_classes[i] << std::endl;
      }
      std::cout << std::endl;
      break;
  }

  pb::comm::ZWaveMsg msg;

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
  cfg_psk_len_ = 0;
  const char* s = psk;
  while (*s && cfg_psk_len_ < cfg_psk_.size()) {
    int val = hex2int(*s++);
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
