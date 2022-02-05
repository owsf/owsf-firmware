/*
 * (C) Copyright 2021 Sören Beye
 * (C) Copyright 2022 Dominik Laton
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef _VINDRIKTNING_H_
#define _VINDRIKTNING_H_

#include <SoftwareSerial.h>

#include "rtcmem_map.h"
#include "sensor.h"

struct vindriktning_rtc_data {
	uint32_t data_upload;
	float pm25;
}__attribute__ ((packed));

class Sensor_VINDRIKTNING : public Sensor {
private:
	int rx, tx;
	float pm25;
	bool initialized;
	std::shared_ptr<SoftwareSerial> sensor_serial;
	float threshold_pm25;
	int mem;
	vindriktning_rtc_data rtc_data;
	static constexpr const char *sensor_type = "VINDRIKTNING";
	String tags;
	Sensor_State state = SENSOR_NOT_INIT;
	uint8_t rx_buf[255];
	uint8_t rx_buf_idx;
	uint16_t pm25_meas[5];
	uint8_t pm25_meas_idx;
	bool data_upload;

	void rx_buf_clear();
	bool sample_process();
	bool header_valid();
	bool checksum_valid();

public:
	Sensor_State sample() override;
	void publish(Point &) override;

	const char *get_sensor_type() override;
	String &get_tags() override;

	explicit Sensor_VINDRIKTNING(const JsonVariant &);
	Sensor_VINDRIKTNING() : rx(4), tx(5), pm25(1.0),
	initialized(false), sensor_serial(), mem(-1), tags(),
	rx_buf_idx(0), pm25_meas_idx(0), data_upload(false) {}
	~Sensor_VINDRIKTNING() {}
};


class VINDRIKTNINGFactory : public SensorFactory {
	static constexpr const char *type_str = "VINDRIKTNING";

public:
	Sensor *new_instance(JsonVariant &cfg) override {
		return new Sensor_VINDRIKTNING(cfg);
	}

	void register_factory(SensorManager *sm) override {
		this->manager = sm;
		this->manager->register_sensor_class(type_str, this);
	}

	VINDRIKTNINGFactory() {}
	~VINDRIKTNINGFactory() {}
};
#endif
