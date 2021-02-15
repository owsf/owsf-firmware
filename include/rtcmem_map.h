/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifndef _RTCMEM_MAP_H_
#define _RTCMEM_MAP_H_
#include <ArduinoJson.h>

#define RTCMEM_WSS RTC_USER_MEM
#define RTCMEM_SENSOR_BASE (128 - 40)
#define RTCMEM_SENSOR(i) (i < 0 || i > 4 ? -1 : (RTCMEM_SENSOR_BASE + 8 * (i)))

#endif
