cmake_minimum_required(VERSION 2.8)

add_definitions(-std=gnu99)

include (../cmake/FatalWarnings.cmake)
ADM_EXTRA_WARNINGS()

find_package(Protobuf 3 REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED 1.0.1)
find_package(LibXml2 REQUIRED)

find_library(AVAHI_LIBRARY-COMMON NAMES avahi-common)
find_library(AVAHI_LIBRARY-CLIENT NAMES avahi-client)

if(AVAHI_LIBRARY-CLIENT)
  add_definitions(-DWITH_MDNS)
  set(MDNS_LIBS ${AVAHI_LIBRARY-COMMON} ${AVAHI_LIBRARY-CLIENT})
endif()

FIND_LIBRARY(READLINE_LIB NAMES readline)
FIND_LIBRARY(ZMQ_LIB NAMES zmq)
FIND_LIBRARY(MATRIX_LIB NAMES matrix_malos)
FIND_LIBRARY(MATRIX_ZMQ_LIB NAMES matrix_malos_zmq)
FIND_LIBRARY(ZWAVEIP_LIB NAMES zwaveip)
FIND_LIBRARY(ZWCMDTOOL_LIB NAMES zw_cmd_tool)

message(STATUS "ZMQ found => " "${ZMQ_LIB}")
message(STATUS "MATRIX LIB found => " "${MATRIX_LIB}")
message(STATUS "MATRIX ZMQ LIB found => " "${MATRIX_ZMQ_LIB}")
message(STATUS "ZWAVEIP found => " "${ZWAVEIP_LIB}")
message(STATUS "XML2 found =>" "${LIBXML2_LIBRARIES}")


PROTOBUF_GENERATE_CPP(MALOS_PROTO_SRC MALOS_PROTO_HEADER ../protocol-buffers/malos/driver.proto)


include_directories(
  ${LIBXML2_INCLUDE_DIR}
  ${PROJECT_BINARY_DIR} # Protos.
  ${PROTOBUF_INCLUDE_DIRS})

set(malos_zwave_src
  malos_zwave.c 
  tokenizer.c
  hexchar.c 
  util.c 
  tokquote/tokquote.c
  xml/zw_cmd_tool.c
  xml/parse_xml.c
  ${MALOS_PROTO_SRC} ${MALOS_PROTO_HEADER}
)

add_executable(malos_zwave ${malos_zwave_src})
  set_property(TARGET malos_zwave PROPERTY CXX_STANDARD 11)
  target_link_libraries(malos_zwave ${PROTOBUF_LIBRARY})
  target_link_libraries(malos_zwave ${ZWAVEIP_LIB})
  target_link_libraries(malos_zwave ${ZWCMDTOOL_LIB})
  target_link_libraries(malos_zwave ${CMAKE_THREAD_LIBS_INIT})
  target_link_libraries(malos_zwave ${MATRIX_LIB})
  target_link_libraries(malos_zwave ${MATRIX_ZMQ_LIB} ${ZMQ_LIB})
  target_link_libraries(malos_zwave ${OPENSSL_LIBRARIES})
  target_link_libraries(malos_zwave ${READLINE_LIB})
  target_link_libraries(malos_zwave ${LIBXML2_LIBRARIES})
