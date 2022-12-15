cmake_minimum_required(VERSION 3.1)
project(spadger)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x")

include_directories(".")
include_directories("src")

get_property(dirs DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
message(${dirs}) # 打印一下目录情况

find_library(YAML-CPP yaml-cpp)
message(***${YAML-CPP})

# add library to ues
set(LIB_SRC
    src/log.cc
    src/util.cc
    src/thread.cc
    src/fiber.cc
    src/scheduler.cc
    src/config.cc
    src/env.cc
    src/iomanager.cc
    src/timer.cc
    src/hook.cc
    src/fd_manager.cc
    src/address.cc
    src/socket.cc
    src/bytearray.cc
)

add_library(spadger SHARED ${LIB_SRC})

# add_library(spadger STATIC ${LIB_SRC})
# SET_TARGET_PROPERTIES (spadger_static PROPERTIES OUTPUT_NAE "spadger")
set(LIB_LIB
    spadger
    pthread
    dl
    ${YAML-CPP}
)

# add executable and link lib
add_executable(log_test tests/test_log.cc)
add_dependencies(log_test spadger)
target_link_libraries(log_test ${LIB_LIB})

add_executable(config_test tests/test_config.cc)
add_dependencies(config_test spadger)
target_link_libraries(config_test ${LIB_LIB})

add_executable(thread_test tests/test_thread.cc)
add_dependencies(thread_test spadger)
target_link_libraries(thread_test ${LIB_LIB})

add_executable(util_test tests/test_util.cc)
add_dependencies(util_test spadger)
target_link_libraries(util_test ${LIB_LIB})

add_executable(fiber_test tests/test_fiber.cc)
add_dependencies(fiber_test spadger)
target_link_libraries(fiber_test ${LIB_LIB})

add_executable(scheduler_test tests/test_scheduler.cc)
add_dependencies(scheduler_test spadger)
target_link_libraries(scheduler_test ${LIB_LIB})

add_executable(iomanager_test tests/test_iomanager.cc)
add_dependencies(iomanager_test spadger)
target_link_libraries(iomanager_test ${LIB_LIB})

add_executable(hook_test tests/test_hook.cc)
add_dependencies(hook_test spadger)
target_link_libraries(hook_test ${LIB_LIB})

add_executable(address_test tests/test_address.cc)
add_dependencies(address_test spadger)
target_link_libraries(address_test ${LIB_LIB})

add_executable(socket_test tests/test_socket.cc)
add_dependencies(socket_test spadger)
target_link_libraries(socket_test ${LIB_LIB})

# set generate path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
seT(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
