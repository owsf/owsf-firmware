/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef _BME280_H_
#define _BME280_H_

#include <BME280I2C.h>

#include "sensor.h"

class Sensor_BME280 : public Sensor {
private:
    static constexpr const int addr = 0x76;
    int sda, scl;
    float temp, hum, pres;
    bool initialized;
    BME280I2C bme;
    Sensor_State state = SENSOR_NOT_INIT;

public:
    Sensor_State sample() override;
    void publish(Point &) override;

    explicit Sensor_BME280(const JsonVariant &);
    Sensor_BME280() : sda(2), scl(14), temp(21.), hum(50.), pres(1080.), initialized(false), bme() {}
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
