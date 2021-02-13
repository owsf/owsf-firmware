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
#define RTCMEM_SENSOR0 (128 - 32)
/* sensors require 128bytes total */
#define RTCMEM_SENSOR1 (RTCMEM_SENSOR0 + 8)
#define RTCMEM_SENSOR2 (RTCMEM_SENSOR1 + 8)
#define RTCMEM_SENSOR3 (RTCMEM_SENSOR2 + 8)
#define RTCMEM_SENSOR4 (RTCMEM_SENSOR3 + 8)

inline int get_rtc_addr(const JsonVariant &j)
{
	int addr;
	int mem_no;

	addr = -1;
	mem_no = j["RTCMEM_SENSOR"] | -1;

	switch(mem_no)
	{
	case 0:
		addr = RTCMEM_SENSOR0;
		break;
	case 1:
		addr = RTCMEM_SENSOR1;
		break;
	case 2:
		addr = RTCMEM_SENSOR2;
		break;
	case 3:
		addr = RTCMEM_SENSOR3;
		break;
	case 4:
		addr = RTCMEM_SENSOR4;
		break;
	default:
		break;
	}

	return addr;
}

#endif
