/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "sensor.h"

Sensor_State Sensor_ADC::sample() {
    if (state != SENSOR_INIT)
        return state;

    Serial.printf("  Sampling sensor ADC\n");

    current_value = factor * (1. * analogRead(0)) * (1 + r1 / r2) / 1024 + offset;

    state = SENSOR_DONE_UPDATE;

    return state;
}

void Sensor_ADC::publish(Point &p) {
    p.addField("voltage", current_value);
}

/* sensor specific includes */
#include "bme280.h"

void SensorManager::register_sensor_class(const char *type, SensorFactory *sf) {
    if (!type || !sf) {
        Serial.printf("register_sensor_class: invalid parameters %p %p\n", type, sf);
        return;
    }

    Serial.printf("Registering factory %s -> %p\n", type, sf);

    factories[type] = sf;
}

void SensorManager::new_sensor(JsonVariant &j) {
    SensorFactory *factory = nullptr;

    if (j["type"].isNull())
        return;

    const char *type = j["type"].as<const char*>();
    for (std::pair<const char *, SensorFactory *> f: factories) {
        if (!strncasecmp(f.first, type, 64)) {
            factory = f.second;
            break;
        }
    }

    if (!factory) {
        Serial.printf("No factory for sensor type %s\n", type);
        return;
    }

    Sensor *sensor = factory->new_instance(j);
    if (!sensor)
        return;

    sensors.push_back(sensor);
}

ADCFactory adc_factory;
BME280Factory bme280_factory;

SensorManager::SensorManager(const JsonArray &j) {
    bme280_factory.register_factory(this);
    adc_factory.register_factory(this);

    for (JsonVariant v : j) {
        Serial.printf("found sensor type %s\n", v["type"].as<const char *>());
        new_sensor(v);
    }

    Serial.println(F("SensorManger() done"));
}

bool SensorManager::upload_requested() {
    return upload_request;
}

bool SensorManager::sensors_done() {
    return done;
}

void SensorManager::loop() {
    done = true;
    Serial.printf("Sampling ... \n");
    for (Sensor *s : sensors) {
        Sensor_State state = s->sample();

        if (state == SENSOR_DONE_UPDATE)
            upload_request = true;

        if (state == SENSOR_INIT)
            done = false;
    }
}

void SensorManager::publish(Point &p) {
    for (Sensor *s : sensors)
        s->publish(p);
}
