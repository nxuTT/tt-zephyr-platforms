# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(clock_control_tests)

include_directories(../../../include/)

FILE(GLOB SOURCES src/*.c)
target_sources(app PRIVATE ${SOURCES})
