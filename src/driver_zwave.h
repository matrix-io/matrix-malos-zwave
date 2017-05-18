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

#ifndef SRC_DRIVER_ZWAVE_H_
#define SRC_DRIVER_ZWAVE_H_

#include <memory>
#include <valarray>

extern "C" {
#include <zwaveip/libzwaveip.h>
#include <zwaveip/zw_cmd_tool.h>
}

#include <matrix_malos/malos_base.h>
#include "./src/driver.pb.h"

const char kZWaveDriverName[] = "ZWave";

namespace matrix_malos {


class ZWaveDriver : public MalosBase {
 public:
  ZWaveDriver();

  // Read configuration of LEDs (from the outside world).
  bool ProcessConfig(const DriverConfig& config) override;

  // Send updates. Checks for new messages from Zigbee.
  bool SendUpdate() override;

 private:
  // MALOS exposed methods
  void Send(ZwaveParams& msg);
  void AddNode(ZwaveParams& msg);
  void RemoveNode(ZwaveParams& msg);
  void SetDefault(ZwaveParams& msg);
  void List(ZwaveParams& msg);

 private:
  zconnection*  ZipConnect(const char* remote_addr);
  static void TransmitDonePan(struct zconnection* zc,
                                transmission_status_code_t status);

  static void ApplicationCommandHandler(struct zconnection* connection,
                                          const uint8_t* data,
                                          uint16_t datalen);
  bool ConnectToGateway();

 private:
  zconnection* gwZipconnection_;
  std::string destAddress_;
  std::string serverIP_;
  zconnection* panConnection_;
  static bool panConnectionBusy_;

  std::valarray<uint8_t> cfgPsk_;  // fixed size = 64
  uint8_t cfgPskLen_;
};
}  // namespace matrix_malos

#endif  // SRC_DRIVER_ZWAVE_H_
