/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifndef _ADC_H_
#define _ADC_H_

#include "rtcmem_map.h"
#include "sensor.h"

struct adc_rtc_data {
    uint32_t data_upload;
    float current_value;
}__attribute__ ((packed));

class Sensor_ADC : public Sensor {
private:
   float current_value;
   float r1;
   float r2;
   float offset;
   float factor;
   float threshold_voltage;
   int mem;
   bool data_upload;
   adc_rtc_data rtc_data;
   static constexpr const char *sensor_type = "ADC";
   String tags;
   Sensor_State state = SENSOR_INIT;

public:
    Sensor_State sample() override;
    void publish(Point &) override;

    const char *get_sensor_type() override;
    String &get_tags() override;

    explicit Sensor_ADC(const JsonVariant &);

    Sensor_ADC() : current_value(0), r1(9100.), r2(47000.), offset(0.), factor(1.), mem(-1), data_upload(false), tags() {}
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
