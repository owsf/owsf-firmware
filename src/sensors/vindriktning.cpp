/*
 * (C) Copyright 2021 Sören Beye
 * (C) Copyright 2022 Dominik Laton
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <SoftwareSerial.h>

#include "sensors/vindriktning.h"
const char START_BYTES_DETECTED = 1;
const char END_BYTES_DETECTED = 2;
const uint32_t START_MASK = 0x0016110B;

bool Sensor_VINDRIKTNING::sample_process() {
	/**
	 *         MSB  DF 3     DF 4  LSB
	 * uint16_t = xxxxxxxx xxxxxxxx
	 */
	bool done = false;
	uint16_t pm25_cur;
	float pm25_calc;

	pm25_cur = (vind_message[5] << 8) | vind_message[6];
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

	vind_message.clear();
	return done;
}

char Sensor_VINDRIKTNING::check_start_end_bytes(uint8_t byte) {
  incoming_mask = (incoming_mask << 8) | byte;

  if ((incoming_mask & START_MASK) == START_MASK)
    return START_BYTES_DETECTED;
  if (record && vind_message.size() > 19)
    return END_BYTES_DETECTED;
  return 0;
}

bool Sensor_VINDRIKTNING::checksum_valid() {
	uint8_t checksum = 0;

	for (uint8_t i = 0; i < 20; i++) {
		checksum += vind_message[i];
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

	while (sensor_serial->available()) {
		const char c = sensor_serial->read();

		delay(1);
		if (record)
			vind_message.emplace_back(c);

		switch (check_start_end_bytes(c)) {
		case START_BYTES_DETECTED: {
			Serial.printf("vindriktning: start detected\n");
			record = true;
			vind_message.clear();
			vind_message.emplace_back(0x16);
			vind_message.emplace_back(0x11);
			vind_message.emplace_back(0x0B);
			break;
		};
		case END_BYTES_DETECTED: {
			if (record) {
				record = false;
				finished = true;
				Serial.printf("vindriktning: end detected\n");
			}
			break;
		};
		};
	}

	if (finished == false)
		return state;
	finished = false;

	Serial.printf("vindriktning: checking checksum\n");
	if (!checksum_valid()) {
		Serial.printf("vindriktning: checksum incorrect\n");
		vind_message.clear();
		return state;
	}

	if (!sample_process()) {
		return state;
	}

	if (mem < 0) {
		state = SENSOR_DONE_UPDATE;
		return state;
	}

	state = SENSOR_DONE_NOUPDATE;
	ESP.rtcUserMemoryRead(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
	if (threshold_helper_float(pm25, rtc_data.pm25, threshold_pm25)) {
		state = SENSOR_DONE_UPDATE;
		rtc_data.pm25 = pm25;
	}
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
