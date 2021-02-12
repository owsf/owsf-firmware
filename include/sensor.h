/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifndef _SENSOR_H_
#define _SENSOR_H_

#include <ArduinoJson.h>
#include <InfluxDbClient.h>
#include <map>
#include <list>

enum Sensor_State {
    SENSOR_NOT_INIT,
    SENSOR_INIT,
    SENSOR_DONE_UPDATE,
    SENSOR_DONE_NOUPDATE,
};

class Sensor;
class SensorFactory;

class SensorManager {
private:
    std::map<const char *, SensorFactory *> factories;
    std::list<Sensor *> sensors;
    bool upload_request = false;
    bool done = false;

    void new_sensor(JsonVariant &);

public:
    void register_sensor_class(const char *, SensorFactory *);

    bool upload_requested();
    bool sensors_done();

    void publish(Point &);
    void loop();

    explicit SensorManager(const JsonArray &);
    ~SensorManager() {}
};


class Sensor {
public:
    virtual Sensor_State sample() = 0;
    virtual void publish(Point &) = 0;

    Sensor() {}
    virtual ~Sensor() {};
};


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


class SensorFactory {
protected:
    SensorManager *manager = nullptr;

public:
    virtual Sensor *new_instance(JsonVariant &) = 0;
    virtual void register_factory(SensorManager *) = 0;
    SensorFactory() {};
    ~SensorFactory() {};
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
