/*
 * Copyright 2017 <Admobilize>
 * MATRIX Labs  [http://creator.matrix.one]
 * This file is part of MATRIX Creator MALOS
 *
 * Author: Andres Calderon <andres.calderon@admobilize.com>
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
DEFINE_string(psk, "123456789012345678901234567890aa", "PSK");

extern "C" {
#include <zwaveip/libzwaveip.h>
#include <zwaveip/zresource.h>
#include <zwaveip/zw_cmd_tool.h>
#include <zwaveip/network_management.h>
}



#include "./driver_zwave.h"

#include "./src/driver.pb.h"

#define binaryCommand_BUFFER_SIZE 2000
#define MAX_ADDRESS_SIZE 100

#define SECURITY_0_NETWORK_KEY_BIT 0x80
#define SECURITY_2_ACCESS_CLASS_KEY 0x04
#define SECURITY_2_AUTHENTICATED_CLASS_KEY 0x02
#define SECURITY_2_UNAUTHENTICATED_CLASS_KEY 0x01


void net_mgmt_command_handler(union evt_handler_struct evt) {
/*
  switch (evt.dsk_report.type) {
    case APPROVE_REQUESTED_KEYS: {
      inclusion_context.requested_keys = evt.requested_keys.requested_keys;
      inclusion_context.csa_inclusion_requested =
          evt.requested_keys.csa_requested;

      printf("The joining node requests these keys:\n\n");
      if (evt.requested_keys.requested_keys & SECURITY_2_ACCESS_CLASS_KEY) {
        printf(" * Security 2 Access/High Security key\n");
      }
      if (evt.requested_keys.requested_keys &
          SECURITY_2_AUTHENTICATED_CLASS_KEY) {
        printf(" * Security 2 Authenticated/Normal key\n");
      }
      if (evt.requested_keys.requested_keys &
          SECURITY_2_UNAUTHENTICATED_CLASS_KEY) {
        printf(" * Security 2 Unauthenticated/Ad-hoc key\n");
      }
      if (evt.requested_keys.requested_keys & SECURITY_0_NETWORK_KEY_BIT) {
        printf(" * Security S0 key\n");
      }
      printf("\n");
      if (evt.requested_keys.csa_requested) {
        printf("and client side authentication\n");
      }
      printf("Enter \'grantkeys\' to accept or \'abortkeys\' to cancel.\n");
    } break;
    case APPROVE_DSK: {
      printf("The joining node is reporting this device specific key:\n");
      print_hex_string(evt.dsk_report.dsk, 16);
      printf(
          "Please approve by typing \'acceptdsk 12345\' where 12345 is the "
          "first part of the DSK.\n12345 may be omitted if the device does not "
          "require the Access or Authenticated keys.\n");

    } break;
    default:
      break;
  }
*/
}

namespace matrix_malos {

bool ZWaveDriver::panConnectionBusy_;

void ZresourceMDNSHelper() { zresource_mdns_thread_func(NULL); }

ZWaveDriver::ZWaveDriver() : MalosBase(kZWaveDriverName), cfgPsk_(64) {
  SetNeedsKeepalives(true);
  SetMandatoryConfiguration(true);
  SetNotesForHuman("ZWave Driver v1.0");
  panConnectionBusy_ = false;

  serverIP_ = FLAGS_server;

  std::thread MDNSThread(ZresourceMDNSHelper);

  ConnectToGateway();
}

// ZwaveParams_ZwaveOperations i;

bool ZWaveDriver::ProcessConfig(const DriverConfig& config) {
  ZwaveParams zwave(config.zwave());

  if (zwave.operation() == ZwaveParams::SEND) {
    Send(zwave);
  } else if (zwave.operation() == ZwaveParams::ADDNODE) {
    AddNode(zwave);
  } else if (zwave.operation() == ZwaveParams::REMOVENODE) {
    RemoveNode(zwave);
  } else if (zwave.operation() == ZwaveParams::SETDEFAULT) {
    SetDefault(zwave);
  } else if (zwave.operation() == ZwaveParams::LIST) {
    List(zwave);
  }

  return true;
}

bool ZWaveDriver::SendUpdate() { return true; }

void ZWaveDriver::Send(ZwaveParams& msg) {
  std::string cmdName;
  std::string className;

  const struct zw_command_class* pClass;
  const struct zw_command* pCmd;

  cmdName = ZWaveCommand_CmdType_Name(msg.zwave_cmd().cmd());
  className = ZWaveCommand_ClassType_Name(msg.zwave_cmd().zwclass());

  pClass = zw_cmd_tool_get_class_by_name(className.c_str());
  pCmd = zw_cmd_tool_get_cmd_by_name(pClass, cmdName.c_str());

  if (!pClass || !pCmd) {
    std::cerr << "Invalid <ZWave class, command> pair." << std::endl;
    return;
  }

  static unsigned char binaryCommand[binaryCommand_BUFFER_SIZE];

  memset(binaryCommand, 0, binaryCommand_BUFFER_SIZE);
  binaryCommand[0] = pClass->cmd_class_number;
  binaryCommand[1] = pCmd->cmd_number;

  memcpy(&binaryCommand[2], msg.zwave_cmd().params().c_str(),
         msg.zwave_cmd().params().length());

  int binaryCommandLen =
      2 +
      msg.zwave_cmd()
          .params()
          .length();  // sizeof([class_number,cmd_number,params])

  if (0 != panConnectionBusy_) {
    std::cerr << "Busy, cannot send right now." << std::endl;
    return;
  }

  if (msg.device() != destAddress_) {
    if (panConnection_) {
      zclient_stop(panConnection_);
      panConnection_ = NULL;
    }
    // FIXME: Use thread synchronization instead of sleep to avoid "Socket Read
    // Error"
    std::this_thread::sleep_for(std::chrono::seconds(1));
    panConnection_ = ZipConnect(msg.device().c_str());
  }
  if (!panConnection_) {
    fprintf(stderr, "Failed to connect to PAN node\n");
    destAddress_[0] = 0;
    return;
  }
  destAddress_ = msg.device();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  zconnection_set_transmit_done_func(panConnection_, TransmitDonePan);
  if (zconnection_send_async(panConnection_, binaryCommand, binaryCommandLen,
                             0)) {
    panConnectionBusy_ = true;
  }
}

void ZWaveDriver::AddNode(ZwaveParams& /*msg*/) { net_mgmt_learn_mode_start(); }

void ZWaveDriver::RemoveNode(ZwaveParams& /*msg*/) {}

void ZWaveDriver::SetDefault(ZwaveParams& /*msg*/) {}

void ZWaveDriver::List(ZwaveParams& /*msg*/) {
  std::cout << "List of discovered Z/IP services:" << std::endl;
  for (zip_service* n = zresource_get(); n; n = n->next) {
    std::cout << n->host_name << " " << n->service_name;
  }
}

bool ZWaveDriver::ConnectToGateway() {
  gwZipconnection_ = ZipConnect(serverIP_.c_str());

  if (gwZipconnection_) return true;
  return false;
}

zconnection* ZWaveDriver::ZipConnect(const char* remote_addr) {
  static uint8_t psk[] = {0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56,
                          0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0xAA};

  std::cout << "ZWaveDriver::ZipConnect" << std::endl;

  if (cfgPskLen_ == 0) {
    memcpy(&cfgPsk_[0], psk, sizeof(psk));
    cfgPskLen_ = sizeof(psk);
    std::cerr << "PSK not configured - using default." << std::endl;
  }

  struct zconnection* zc;

  zc = zclient_start(remote_addr, 41230, reinterpret_cast<char*>(&cfgPsk_[0]),
                     cfgPskLen_, ApplicationCommandHandler);
  if (zc == 0) {
    std::cerr << "Error connecting." << std::endl;
  }
  return zc;
}

void ZWaveDriver::ApplicationCommandHandler(struct zconnection* connection,
                                            const uint8_t* data,
                                            uint16_t datalen) {}

void ZWaveDriver::TransmitDonePan(struct zconnection* zc,
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
  ZWaveDriver::panConnectionBusy_ = false;
}

}  // namespace matrix_malos
