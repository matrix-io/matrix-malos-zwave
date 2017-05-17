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

#include <fstream>
#include <set>
#include <string>
#include <valarray>

extern "C" {
#include <zwaveip/zw_cmd_tool.h>
}

int main(int argc, char* argv[]) {
  std::ofstream os;
  os.open("zwave_commands.json", std::ofstream::out | std::ofstream::trunc);

  std::valarray<const char*> names(512);
  std::set<std::string> class_names;

  int number_of_classes = zw_cmd_tool_get_command_class_names(&names[0]);

  for (auto& class_name :
       std::valarray<const char*>(names[std::slice(0, number_of_classes, 1)])) {
    class_names.insert(std::string(class_name));
  }

  os << "{";

  int class_index = 0;
  for (auto& class_name : class_names) {
    os << std::endl;
    os << "  \"" << class_name << "\": {" << std::endl;
    ++class_index;

    const zw_command_class* p_class_class =
        zw_cmd_tool_get_class_by_name(class_name.c_str());

    int number_of_commands =
        zw_cmd_tool_get_cmd_names(p_class_class, &names[0]);
    int cmd_index = 0;
    for (auto& cmd_name : std::valarray<const char*>(
             names[std::slice(0, number_of_commands, 1)])) {
      os << "    \"" << cmd_name << "\" : ";
      ++cmd_index;

      const zw_command* p_zw_command =
          zw_cmd_tool_get_cmd_by_name(p_class_class, cmd_name);
      int number_of_params =
          zw_cmd_tool_get_param_names(p_zw_command, &names[0]);

      int param_index = 0;
      os << "[";
      for (auto& param_name : std::valarray<const char*>(
               names[std::slice(0, number_of_params, 1)])) {
        os << "\"" << param_name << "\"";
        ++param_index;
        if (param_index != number_of_params) os << ", ";
      }
      os << "]";
      if (cmd_index != number_of_commands) os << ", ";
      os << std::endl;
    }

    os << "  }";
    if (class_index != class_names.size()) os << ", ";
    os << std::endl;
  }
  os << "}";
  os.close();

  return 0;
}
