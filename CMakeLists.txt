# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(MscAndBLE)

target_include_directories(app PRIVATE inc)
target_sources(app PRIVATE
 src/main.c
 src/sensor.c
 src/user_gpio.c
 src/user_workQueue.c
 src/user_rtc.c
 src/user_storage.c
 src/lcd.c
 src/bat_ssadc.c
 src/ble_nus_order.c
 src/button.c
 )
