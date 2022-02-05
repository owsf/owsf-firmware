/*
 * (C) Copyright 2021 Sören Beye
 * (C) Copyright 2022 Dominik Laton
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <SoftwareSerial.h>

#include "sensors/vindriktning.h"

void Sensor_VINDRIKTNING::rx_buf_clear() {
	// Clear everything for the next message
	memset(rx_buf, 0, sizeof(rx_buf));
	rx_buf_idx = 0;
}

bool Sensor_VINDRIKTNING::sample_process() {
	/**
	 *         MSB  DF 3     DF 4  LSB
	 * uint16_t = xxxxxxxx xxxxxxxx
	 */
	bool done = false;
	uint16_t pm25_cur;
	float pm25_calc;

	pm25_cur = (rx_buf[5] << 8) | rx_buf[6];
	pm25_meas[pm25_meas_idx] = pm25_cur;
	pm25_meas_idx = (pm25_meas_idx + 1) % 5;

	if (pm25_meas_idx == 0) {
		pm25_calc = 0;
		for (uint8_t i = 0; i < 5; ++i) {
			pm25_calc += pm25_meas[i] / 5.0f;
		}
		pm25 = pm25_calc;
		done = true;
	}

	rx_buf_clear();
	return done;
}

bool Sensor_VINDRIKTNING::header_valid() {
	bool headerValid = rx_buf[0] == 0x16 && rx_buf[1] == 0x11 && rx_buf[2] == 0x0B;

	return headerValid;
}

bool Sensor_VINDRIKTNING::checksum_valid() {
	uint8_t checksum = 0;

	for (uint8_t i = 0; i < 20; i++) {
		checksum += rx_buf[i];
	}

	return checksum == 0;
}

Sensor_State Sensor_VINDRIKTNING::sample() {
	if (state != SENSOR_NOT_INIT && state != SENSOR_INIT)
		return state;
	state = SENSOR_INIT;

	if (!initialized) {
		Serial.printf("  vindriktning not initialized");
		return SENSOR_NOT_INIT;
	}

	if (data_upload)
		return SENSOR_DONE_UPDATE;

	if (!sensor_serial->available()) {
		return state;
	}

	Serial.printf("Sampling vindriktning data: ");
	while (sensor_serial->available()) {
		rx_buf[rx_buf_idx++] = sensor_serial->read();
		Serial.printf(".");

		delay(15);
		if (rx_buf_idx >= 64) {
			rx_buf_clear();
		}
	}
	Serial.printf("\n");

	if (header_valid() && checksum_valid()) {
		Serial.printf("Found vindriktning data\n");
		if (!sample_process())
			return state;
	} else {
		rx_buf_clear();
		return state;
	}

	if (mem < 0) {
		state = SENSOR_DONE_UPDATE;
		return state;
	}

	state = SENSOR_DONE_NOUPDATE;
	ESP.rtcUserMemoryRead(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
	if (threshold_helper_float(pm25, &rtc_data.pm25, threshold_pm25))
		state = SENSOR_DONE_UPDATE;
	if (state == SENSOR_DONE_UPDATE) {
		rtc_data.data_upload = 0xdeadbeef;
	} else {
		rtc_data.data_upload = 0;
	}
	ESP.rtcUserMemoryWrite(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));

	return state;
}

void Sensor_VINDRIKTNING::publish(Point &p) {
	if (!initialized)
		return;

	ESP.rtcUserMemoryRead(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
	rtc_data.data_upload = 0;
	ESP.rtcUserMemoryWrite(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
	p.addField("pm2.5", rtc_data.pm25);
}

Sensor_VINDRIKTNING::Sensor_VINDRIKTNING(const JsonVariant &j) :
	pm25(-1), mem(-1), data_upload(false)
{
	int rtcmem;

	Serial.println(F("Initializing VINDRIKTNING "));

	initialized = false;

	rx = j["rx"] | 4;
	tx = j["tx"] | 5;

	threshold_pm25 = j["threshold_pm25"] | 5.0;

	rtcmem = j["rtcmem_slot"] | -1;
	mem = RTCMEM_SENSOR(rtcmem);

	tags = j["tags"] | "";

	ESP.rtcUserMemoryRead(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
	if (rtc_data.data_upload == 0xdeadbeef) {
		rtc_data.data_upload = 0;
		ESP.rtcUserMemoryWrite(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
		data_upload = true;
		initialized = true;
		return;
	}

	rx_buf_idx = 0;
	pm25_meas_idx = 0;

	sensor_serial = std::make_shared<SoftwareSerial>(rx, tx);
	sensor_serial->begin(9600);

	initialized = true;
}

const char *Sensor_VINDRIKTNING::get_sensor_type() {
	return sensor_type;
}

String &Sensor_VINDRIKTNING::get_tags() {
	return tags;
}
