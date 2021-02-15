/*
 * (C) Copyright 2021 Tillmann Heidsieck
 * (C) Copyright 2021 Dominik Laton
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef _DS18B20_H_
#define _DS18B20_H_

#include <DS18B20.h>

#include "rtcmem_map.h"
#include "sensor.h"

struct ds18b20_rtc_data {
    float temp;
}__attribute__ ((packed));

class Sensor_DS18B20 : public Sensor {
private:
    float temp;
    bool initialized;
    DS18B20 *ds;
    int mem;
    ds18b20_rtc_data rtc_data;
    String sensor_type;
    String tags;
    Sensor_State state = SENSOR_NOT_INIT;

public:
    Sensor_State sample() override;
    void publish(Point &) override;

    String &get_sensor_type() override;
    String &get_tags() override;

    explicit Sensor_DS18B20(const JsonVariant &);
    Sensor_DS18B20() : temp(21.), initialized(false), ds(nullptr), mem(-1),
    sensor_type(), tags() {}
    ~Sensor_DS18B20() {}
};


class DS18B20Factory : public SensorFactory {
    static constexpr const char *type_str = "DS18B20";

public:
    Sensor *new_instance(JsonVariant &cfg) override {
        return new Sensor_DS18B20(cfg);
    }

    void register_factory(SensorManager *sm) override {
        this->manager = sm;
        this->manager->register_sensor_class(type_str, this);
    }

    DS18B20Factory() {}
    ~DS18B20Factory() {}
};
#endif
