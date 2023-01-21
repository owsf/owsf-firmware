/*
 * (C) Copyright 2022 Dominik Laton
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef _SML_H_
#define _SML_H_

#include <SoftwareSerial.h>

#include "rtcmem_map.h"
#include "sensor.h"

struct sml_rtc_data {
	uint32_t data_upload;
	float energy_total_positive;
	float energy_total_negative;
	float power_current;
}__attribute__ ((aligned(4)));

class Sensor_SML : public Sensor {
private:
	int rx, tx;
	float energy_total_positive;
	float energy_total_negative;
	float power_current;
	bool initialized;
	std::shared_ptr<SoftwareSerial> sensor_serial;
	float threshold_energy;
	float threshold_power;
	int mem;
	sml_rtc_data rtc_data;
	static constexpr const char *sensor_type = "SML";
	String tags;
	Sensor_State state = SENSOR_NOT_INIT;
	std::vector<uint8_t> sml_message;
	bool data_upload;
	uint16_t incoming_mask = 0;
	bool record = false;
	bool finished = false;

	void rx_buf_clear();
	bool sample_process();
	bool checksum_check();
	char check_start_end_bytes(uint8_t);

public:
	Sensor_State sample() override;
	void publish(Point &) override;

	const char *get_sensor_type() override;
	String &get_tags() override;

	explicit Sensor_SML(const JsonVariant &);
	Sensor_SML() : rx(4), tx(5), energy_total_positive(0.0),
	energy_total_negative(0.0), power_current(0.0),
	initialized(false), sensor_serial(), mem(-1), tags(),
	data_upload(false) {}
	~Sensor_SML() {}
};


class SMLFactory : public SensorFactory {
	static constexpr const char *type_str = "SML";

public:
	Sensor *new_instance(JsonVariant &cfg) override {
		return new Sensor_SML(cfg);
	}

	void register_factory(SensorManager *sm) override {
		this->manager = sm;
		this->manager->register_sensor_class(type_str, this);
	}

	SMLFactory() {}
	~SMLFactory() {}
};
#endif
