/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <Arduino.h>
#include <time.h>

#include "sensor.h"

/* sensor specific includes */
#include "sensors/adc.h"
#include "sensors/bme280.h"
#include "sensors/ds18b20.h"

bool threshold_helper_float(float val_new, float *val_old, float threshold)
{
    float val_diff;

    val_diff = val_new - *val_old;
    if(val_diff < 0)
        val_diff *= -1;
    if(val_diff >= threshold) {
        *val_old = val_new;
        return true;
    } else {
        return false;
    }
}

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
DS18B20Factory ds18b20_factory;

SensorManager::SensorManager(const JsonArray &j) {
    num_sensors = 0;
    bme280_factory.register_factory(this);
    adc_factory.register_factory(this);
    ds18b20_factory.register_factory(this);

    for (JsonVariant v : j) {
        Serial.printf("found sensor type %s\n", v["type"].as<const char *>());
        new_sensor(v);
        num_sensors++;
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

void SensorManager::publish(InfluxDBClient *c, String *device_name,
                            char *chip_id, const char *version) {
    for (Sensor *sensor : sensors) {
        Point point("sensor_data");
        point.addTag("device", *device_name);
        point.addTag("chip_id", chip_id);
        point.addTag("firmware_version", version);
        point.setTime(time(nullptr));
        point.addTag("sensor_type", sensor->get_sensor_type());
        if (sensor->get_tags() != "")
            point.addTag("sensor_tags", sensor->get_tags());
        sensor->publish(point);
        Serial.println(c->pointToLineProtocol(point));
        c->writePoint(point);
    }
}

uint8_t SensorManager::get_num_sensors() {
    return num_sensors;
}
