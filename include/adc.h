/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifndef _ADC_H_
#define _ADC_H_

#include "sensor.h"

class Sensor_ADC : public Sensor {
private:
   float current_value; 
   float r1;
   float r2;
   float offset;
   float factor;
   Sensor_State state = SENSOR_INIT;

public:
    Sensor_State sample() override;
    void publish(Point &) override;

    explicit Sensor_ADC(const JsonVariant &j) : current_value(0) {
        r1 = j["R1"] | 9100.;
        r2 = j["R2"] | 47000.;
        offset = j["offset"] | 0.;
        factor = j["factor"] | 1.;
    }

    Sensor_ADC() : current_value(0), r1(9100.), r2(47000.), offset(0.), factor(1.) {}
    ~Sensor_ADC() {}
};

class ADCFactory : public SensorFactory {
    static constexpr const char *type_str = "ADC";

public:
    Sensor *new_instance(JsonVariant &cfg) override {
        return new Sensor_ADC(cfg);
    }

    void register_factory(SensorManager *sm) override {
        this->manager = sm;
        this->manager->register_sensor_class(type_str, this);
    }

    ADCFactory() {}
    ~ADCFactory() {}
};
#endif
