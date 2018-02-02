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

#ifndef SRC_DRIVER_ZWAVE_H_
#define SRC_DRIVER_ZWAVE_H_

extern "C" {
#include <zwaveip/libzwaveip.h>
#include <zwaveip/network_management.h>
#include <zwaveip/parse_xml.h>
#include <zwaveip/zresource.h>
#include <zwaveip/zw_cmd_tool.h>
}

#include <matrix_io/malos/v1/driver.pb.h>
#include <matrix_malos/malos_base.h>

#include <memory>
#include <string>
#include <thread>
#include <valarray>

const char kZWaveDriverName[] = "ZWave";

namespace pb = matrix_io::malos::v1;

namespace matrix_malos {

class ZWaveDriver : public MalosBase {
 public:
  ZWaveDriver();

  // Read configuration of LEDs (from the outside world).
  bool ProcessConfig(const pb::driver::DriverConfig& config) override;

  // Send updates. Checks for new messages from ZWave.
  bool SendUpdate() override;

 private:
  // MALOS exposed methods
  void Send(const pb::comm::ZWaveMsg& msg);
  void AddNode();
  void RemoveNode();
  void SetDefault();
  void List();

 private:
  zconnection* ZipConnect(const std::string& address);
  bool ServiceToAddress(const std::string& service_name, std::string* addr);
  static void TransmitDonePan(zconnection* zc,
                              transmission_status_code_t status);

  static void ApplicationCommandHandler(zconnection* connection,
                                        const uint8_t* data, uint16_t datalen);
  void ParsePsk(const char* psk);

 public:
  static bool pan_bonnection_busy_;
  static uint8_t requested_keys_;
  static uint8_t csa_inclusion_requested_;
  // workaround for C callback compatibility
  static ZmqPusher* static_zqm_push_update_;

 private:
  std::thread mdns_thread_;
  zconnection* gw_zip_connection_;
  zconnection* pan_connection_;
  std::string dest_address_;
  std::string server_ip_;

  std::valarray<uint8_t> cfg_psk_;  // fixed size = 64
  uint8_t cfg_psk_len_;
};
}  // namespace matrix_malos

#endif  // SRC_DRIVER_ZWAVE_H_
