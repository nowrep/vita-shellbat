cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(ShellBat)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

if (NOT ${RELEASE})
  add_definitions(-DENABLE_LOGGING)
endif()

add_executable(shellbat
  main.c
  log.c
)

target_link_libraries(shellbat
  taihen_stub
  SceLibKernel_stub
  SceIofilemgr_stub
  k
  gcc
  ScePower_stub
  SceKernelModulemgr_stub
)

set_target_properties(shellbat
  PROPERTIES LINK_FLAGS "-nostdlib"
)

vita_create_self(shellbat.suprx shellbat
  UNSAFE
  CONFIG ${CMAKE_SOURCE_DIR}/shellbat.yml
)
