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

#include <iostream>
#include <string>
#include <thread>

#include "./driver_zwave.h"

#include "./src/driver.pb.h"

namespace matrix_malos {
ZwaveParams_ZwaveOperations i;

bool ZWaveDriver::ProcessConfig(const DriverConfig& config) {
  ZwaveParams zwave(config.zwave());

  if (zwave.operation() == ZwaveParams::SEND) {
    Send(zwave);
  }

  /*
     UNDEF=0;
     SEND=1;
     ADDNODE=2;
     REMOVENODE=3;
     SETDEFAULT=4;
     LIST=5;
  */
  return true;
}

bool ZWaveDriver::SendUpdate() { return true; }

void ZWaveDriver::Send(ZwaveParams& msg) {}

void ZWaveDriver::AddNode(ZwaveParams& msg) {}

void ZWaveDriver::RemoveNode(ZwaveParams& msg) {}

void ZWaveDriver::SetDefault(ZwaveParams& msg) {}

void ZWaveDriver::List(ZwaveParams& msg) {}
}
