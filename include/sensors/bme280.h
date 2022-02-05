/*
 * (C) Copyright 2021 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef _BME280_H_
#define _BME280_H_

#include <Adafruit_BME280.h>
#include <Wire.h>

#include "rtcmem_map.h"
#include "sensor.h"

struct bme280_rtc_data {
    uint32_t data_upload;
    float temp;
    float hum;
    float pres;
}__attribute__ ((packed));

class Sensor_BME280 : public Sensor {
private:
    static constexpr const int addr = 0x76;
    int sda, scl;
    float temp, hum, pres;
    bool initialized;
    TwoWire i2c;
    Adafruit_BME280 bme;
    float threshold_pres;
    float threshold_hum;
    float threshold_temp;
    int mem;
    bme280_rtc_data rtc_data;
    static constexpr const char *sensor_type = "BME280";
    String tags;
    bool data_upload;
    Sensor_State state = SENSOR_NOT_INIT;

public:
    Sensor_State sample() override;
    void publish(Point &) override;

    const char *get_sensor_type() override;
    String &get_tags() override;

    explicit Sensor_BME280(const JsonVariant &);
    Sensor_BME280() : sda(2), scl(14), temp(21.), hum(50.), pres(1080.),
    initialized(false), bme(), mem(-1), tags(), data_upload(false) {}
    ~Sensor_BME280() {}
};


class BME280Factory : public SensorFactory {
    static constexpr const char *type_str = "BME280";

public:
    Sensor *new_instance(JsonVariant &cfg) override {
        return new Sensor_BME280(cfg);
    }

    void register_factory(SensorManager *sm) override {
        this->manager = sm;
        this->manager->register_sensor_class(type_str, this);
    }

    BME280Factory() {}
    ~BME280Factory() {}
};
#endif
