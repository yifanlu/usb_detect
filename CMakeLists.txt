cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

set(SHORT_NAME usb_detect)
project(${SHORT_NAME})
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "USB Detect")
set(VITA_TITLEID  "USBDETECT")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")

link_directories(
  ${CMAKE_BINARY_DIR}/kernel_stubs
  ${CMAKE_BINARY_DIR}/user_stubs
)

add_executable(${SHORT_NAME}
  main.c
)

target_link_libraries(${SHORT_NAME}
  taihen_stub
  SceDisplay_stub
  SceCtrl_stub
  SceGxm_stub
  SceAppUtil_stub
  SceCommonDialog_stub
  usb_detect_helper_user_stub
)

add_dependencies(${SHORT_NAME} user_stubs)

vita_create_self(${SHORT_NAME}.self ${SHORT_NAME} UNSAFE)

add_executable(kernel
  kernel.c
)

target_link_libraries(kernel
  SceUsbdForDriver_stub
  SceDebugForDriver_stub
  SceSysmemForDriver_stub
  SceSysclibForDriver_stub
  taihenForKernel_stub
)

set_target_properties(kernel
  PROPERTIES LINK_FLAGS "-nostdlib"
  COMPILE_FLAGS "-D__VITA_KERNEL__"
)

vita_create_self(kernel.skprx kernel
  UNSAFE
  CONFIG ${CMAKE_SOURCE_DIR}/kernel.yml
)

vita_create_stubs(kernel_stubs kernel ${CMAKE_SOURCE_DIR}/kernel.yml KERNEL)

add_executable(user
  user.c
)

target_link_libraries(user
  usb_detect_helper_stub
)

set_target_properties(user
  PROPERTIES LINK_FLAGS "-nostdlib"
)

add_dependencies(user kernel_stubs)

vita_create_self(user.suprx user
  UNSAFE
  CONFIG ${CMAKE_SOURCE_DIR}/user.yml
)

vita_create_stubs(user_stubs user ${CMAKE_SOURCE_DIR}/user.yml)

# second kernel helper to work around unloading a kernel module with exported funcs
add_executable(kernel2
  kernel2.c
)
set_target_properties(kernel2
  PROPERTIES LINK_FLAGS
  -nostdlib
)
target_link_libraries(kernel2
  gcc
  taihenForKernel_stub
)
vita_create_self(kernel2.skprx kernel2
  CONFIG ${CMAKE_SOURCE_DIR}/kernel2.yml
  UNSAFE
)

vita_create_vpk(${SHORT_NAME}.vpk ${VITA_TITLEID} ${SHORT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE ${CMAKE_CURRENT_BINARY_DIR}/kernel.skprx kernel.skprx
  FILE ${CMAKE_CURRENT_BINARY_DIR}/kernel2.skprx kernel2.skprx
  FILE ${CMAKE_CURRENT_BINARY_DIR}/user.suprx user.suprx
)

add_dependencies(${SHORT_NAME}.vpk kernel.skprx)
add_dependencies(${SHORT_NAME}.vpk user.suprx)
add_dependencies(${SHORT_NAME}.vpk kernel2.skprx)
