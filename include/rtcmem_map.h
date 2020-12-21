/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifndef _RTCMEM_MAP_H_
#define _RTCMEM_MAP_H_

#include <ESP8266WiFi.h>

#define RTCMEM_WSS RTC_USER_MEM
#define RTCMEM_SENSOR0 (RTCMEM_WSS + sizeof(struct WiFiState))
/* sensors require 128bytes total */
#define RTCMEM_SENSOR1 (RTCMEM_SENSOR0 + 8 * sizeof(uint32_t))
#define RTCMEM_SENSOR2 (RTCMEM_SENSOR1 + 8 * sizeof(uint32_t))
#define RTCMEM_SENSOR3 (RTCMEM_SENSOR2 + 8 * sizeof(uint32_t))
#define RTCMEM_SENSOR4 (RTCMEM_SENSOR3 + 8 * sizeof(uint32_t))

#endif
