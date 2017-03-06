/*
 * Copyright 2016 <Admobilize>
 * MATRIX Labs  [http://creator.matrix.one]
 * This file is part of MATRIX Creator MALOS
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

#include <matrix_malos/malos_base.h>
#include "./src/driver.pb.h"

const char kZWaveDriverName[] = "ZWave";

namespace matrix_malos {


class ZWaveDriver : public MalosBase {
 public:
  ZWaveDriver() : MalosBase(kZWaveDriverName) {
    SetNeedsKeepalives(true);
    SetMandatoryConfiguration(true);
    SetNotesForHuman("ZWave Driver v1.0");
  }

  // Read configuration of LEDs (from the outside world).
  bool ProcessConfig(const DriverConfig& config) override;

  // Send updates. Checks for new messages from Zigbee.
  bool SendUpdate() override;

 private:
  ZwaveParams zw_msg;
};

}  // namespace matrix_malos

#endif  // SRC_DRIVER_ZWAVE_H_ 
