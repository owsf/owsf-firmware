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

    uint8_t num_sensors;

    void new_sensor(JsonVariant &);

public:
    void register_sensor_class(const char *, SensorFactory *);

    bool upload_requested();
    bool sensors_done();

    void publish(InfluxDBClient *, String *, char *, const char *);
    uint8_t get_num_sensors();
    void loop();

    explicit SensorManager(const JsonArray &);
    ~SensorManager() {}
};


class Sensor {
public:
    virtual Sensor_State sample() = 0;
    virtual void publish(Point &) = 0;

    virtual const char *get_sensor_type() = 0;
    virtual String &get_tags() = 0;

    Sensor() {}
    virtual ~Sensor() {};
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

bool threshold_helper_float(float, float, float);
#endif
