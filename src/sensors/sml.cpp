/*
 * (C) Copyright 2022 Dominik Laton
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <SoftwareSerial.h>
#include <sml_parser.h>
#include <constants.h>

#include "sensors/sml.h"

static std::string obis_energy_total_positive("1-0:1.8.0");
static std::string obis_energy_total_negative("1-0:2.8.0");
static std::string obis_power_current("1-0:16.7.0");
const char START_BYTES_DETECTED = 1;
const char END_BYTES_DETECTED = 2;

uint8_t get_code(uint8_t byte) {
	switch (byte) {
	case 0x1b:
		return 1;
	case 0x01:
		return 2;
	case 0x1a:
		return 3;
	default:
		return 0;
	}
}

bool Sensor_SML::sample_process() {
	bool done;

	done = false;

	Serial.printf("sml: started processing message\n");

	esphome::sml::SmlFile sml_file = esphome::sml::SmlFile(sml_message);
	std::vector<esphome::sml::ObisInfo> obis_info_vec =
		sml_file.get_obis_info();
	for (auto const &obis_info : obis_info_vec) {
		Serial.printf("OBIS-code: %s\n", obis_info.code_repr().c_str());
		if(obis_info.code_repr() == obis_energy_total_positive) {
			energy_total_positive =
				(float)((double)esphome::sml::bytes_to_uint(obis_info.value) / 10000.0);
			done = true;
			Serial.printf("sml: positive active energy, total: %4.2f kWh\n", energy_total_positive);
		}
		if(obis_info.code_repr() == obis_energy_total_negative) {
			energy_total_negative =
				(float)((double)esphome::sml::bytes_to_uint(obis_info.value) / 10000.0);
			done = true;
			Serial.printf("sml: negative active energy, total: %4.2f kWh\n", energy_total_negative);
		}
		if(obis_info.code_repr() == obis_power_current) {
			power_current =
				(float)((double)esphome::sml::bytes_to_int(obis_info.value) / 1.0);
			done = true;
			Serial.printf("sml: absolute active instantaneous power: %4.2f W\n", power_current);
		}
	}

	return done;
}

uint16_t calc_crc16_p1021(esphome::sml::bytes::const_iterator begin,
			  esphome::sml::bytes::const_iterator end,
			  uint16_t crcsum) {
	for (auto it = begin; it != end; it++) {
		crcsum = (crcsum >> 8) ^
			esphome::sml::CRC16_X25_TABLE[(crcsum & 0xff) ^ *it];
	}
	return crcsum;
}

uint16_t calc_crc16_x25(esphome::sml::bytes::const_iterator begin,
			esphome::sml::bytes::const_iterator end,
			uint16_t crcsum = 0) {
	crcsum = calc_crc16_p1021(begin, end, crcsum ^ 0xffff) ^ 0xffff;
	return (crcsum >> 8) | ((crcsum & 0xff) << 8);
}

uint16_t calc_crc16_kermit(esphome::sml::bytes::const_iterator begin,
			   esphome::sml::bytes::const_iterator end,
			   uint16_t crcsum = 0) {
	return calc_crc16_p1021(begin, end, crcsum);
}

bool Sensor_SML::checksum_check() {
	uint16_t crc_received;

	crc_received = (sml_message.at(sml_message.size() - 2) << 8) |
		sml_message.at(sml_message.size() - 1);
	if (crc_received == calc_crc16_x25(sml_message.begin(),
					   sml_message.end() - 2, 0x6e23)) {
		return true;
	}

	if (crc_received == calc_crc16_kermit(sml_message.begin(),
					      sml_message.end() - 2, 0xed50)) {
		return true;
	}

	return false;
}

char Sensor_SML::check_start_end_bytes(uint8_t byte) {
  incoming_mask = (incoming_mask << 2) | get_code(byte);

  if (incoming_mask == esphome::sml::START_MASK)
    return START_BYTES_DETECTED;
  if ((incoming_mask >> 6) == esphome::sml::END_MASK)
    return END_BYTES_DETECTED;
  return 0;
}

Sensor_State Sensor_SML::sample() {
	if (state != SENSOR_NOT_INIT && state != SENSOR_INIT)
		return state;
	state = SENSOR_INIT;

	if (!initialized) {
		Serial.printf("sml: not initialized");
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
			sml_message.emplace_back(c);

		switch (check_start_end_bytes(c)) {
		case START_BYTES_DETECTED: {
			Serial.printf("sml: start detected\n");
			record = true;
			sml_message.clear();
			break;
		};
		case END_BYTES_DETECTED: {
			if (record) {
				record = false;
				finished = true;
				Serial.printf("sml: end detected\n");
			}
			break;
		};
		};
	}

	if (finished == false)
		return state;
	finished = false;

	Serial.printf("sml: checking checksum\n");
	if (!checksum_check()) {
		Serial.printf("sml: checksum incorrect\n");
		sml_message.clear();
		return state;
	}

	sml_message.resize(sml_message.size() - 8);

	// Now a complete SML message is captured
	// Lets do the evaluation stuff
	if (!sample_process()) {
		return state;
	}

	if (mem < 0) {
		state = SENSOR_DONE_UPDATE;
		return state;
	}

	state = SENSOR_DONE_NOUPDATE;
	ESP.rtcUserMemoryRead(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));

	if (threshold_helper_float(energy_total_positive,
				   rtc_data.energy_total_positive,
				   threshold_energy)) {
		state = SENSOR_DONE_UPDATE;
		rtc_data.energy_total_positive = energy_total_positive;
	}
	if (threshold_helper_float(energy_total_negative,
				   rtc_data.energy_total_negative,
				   threshold_energy)) {
		state = SENSOR_DONE_UPDATE;
		rtc_data.energy_total_negative = energy_total_negative;
	}
	if (threshold_helper_float(power_current,
				   rtc_data.power_current,
				   threshold_power)) {
		state = SENSOR_DONE_UPDATE;
		rtc_data.power_current = power_current;
	}

	if (state == SENSOR_DONE_UPDATE) {
		rtc_data.data_upload = 0xdeadbeef;
	} else {
		rtc_data.data_upload = 0;
	}
	ESP.rtcUserMemoryWrite(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));

	return state;
}

void Sensor_SML::publish(Point &p) {
	if (!initialized)
		return;

	ESP.rtcUserMemoryRead(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
	rtc_data.data_upload = 0;
	ESP.rtcUserMemoryWrite(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
	p.addField("en_tot_pos", rtc_data.energy_total_positive);
	p.addField("en_tot_neg", rtc_data.energy_total_negative);
	p.addField("pow_cur", rtc_data.power_current);
}

Sensor_SML::Sensor_SML(const JsonVariant &j) :
	energy_total_positive(-1), energy_total_negative(-1),
	power_current(-1), mem(-1), data_upload(false)
{
	int rtcmem;

	Serial.println(F("Initializing SML "));

	initialized = false;

	rx = j["rx"] | 4;
	tx = j["tx"] | 5;

	threshold_energy = j["threshold_energy"] | 1;
	threshold_power = j["threshold_power"] | 50.0;

	rtcmem = j["rtcmem_slot"] | -1;
	mem = RTCMEM_SENSOR(rtcmem);

	tags = j["tags"] | "";

	ESP.rtcUserMemoryRead(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
	if (rtc_data.data_upload == 0xdeadbeef) {
		rtc_data.data_upload = 0;
		ESP.rtcUserMemoryWrite(mem, (uint32_t *)&rtc_data,
				       sizeof(rtc_data));
		data_upload = true;
		initialized = true;
		return;
	}

	sensor_serial = std::make_shared<SoftwareSerial>(rx, tx);
	sensor_serial->begin(9600);

	initialized = true;
}

const char *Sensor_SML::get_sensor_type() {
	return sensor_type;
}

String &Sensor_SML::get_tags() {
	return tags;
}
