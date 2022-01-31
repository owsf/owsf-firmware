/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifndef _RTCMEM_MAP_H_
#define _RTCMEM_MAP_H_

#define RTCMEM_GO_ONLINE         0
#define RTCMEM_REBOOT_COUNTER    1
#define RTCMEM_NET_CFG_IP        2
#define RTCMEM_NET_CFG_MASK      3
#define RTCMEM_NET_CFG_GW        4
#define RTCMEM_NET_CFG_DNS       5
#define RTCMEM_NET_CFG_BSSID     6
#define RTCMEM_NET_CFG_CHAN      8
#define RTCMEM_NET_CFG_MAGIC     9
#define RTCMEM_SENSOR_BASE       32

#define RTCMEM_SENSOR_SLOT_SIZE  8
#define RTCMEM_SENSOR_SLOT_COUNT 5
#define RTCMEM_SENSOR(i) \
    (\
     (i) < 0 || (i) >= RTCMEM_SENSOR_SLOT_COUNT ? \
     -1 : (RTCMEM_SENSOR_BASE + RTCMEM_SENSOR_SLOT_SIZE * (i))\
    )

#endif
