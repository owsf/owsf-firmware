/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <BME280I2C.h>
#include <Wire.h>

#include "sensor.h"

const BME280I2C::Settings bme280_settings(BME280::OSR_X1,
                                          BME280::OSR_X1,
                                          BME280::OSR_X1,
                                          BME280::Mode_Forced,
                                          BME280::StandbyTime_1000ms,
                                          BME280::Filter_Off,
                                          BME280::SpiEnable_False,
                                          BME280I2C::I2CAddr_0x76 // I2C address. I2C specific.
                                         );

TwoWire i2c;

Sensor_State Sensor_ADC::sample() {
    if (state != SENSOR_INIT)
        return state;

    Serial.printf("  Sampling sensor ADC\n");

    current_value = factor * (1. * analogRead(0)) * (1 + r1 / r2) / 1024 + offset;

    state = SENSOR_DONE_UPDATE;

    return state;
}

void Sensor_ADC::publish(Point &p) {
    p.addField("voltage", current_value * 0.01);
}

Sensor_State Sensor_BME280::sample() {
    if (state != SENSOR_NOT_INIT && state != SENSOR_INIT)
        return state;

    Serial.printf("  Sampling sensor BME280\n");
    if (!initialized) {
        Serial.printf("  BME280 not initialized");
        return SENSOR_NOT_INIT;
    }

    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);
    bme.read(pres, temp, hum, tempUnit, presUnit);

    state = SENSOR_DONE_UPDATE;

    return state;
}

void Sensor_BME280::publish(Point &p) {
    if (!initialized)
        return;

    p.addField("temperature", temp);
    p.addField("humidity", hum);
    p.addField("pressure", pres);
}

Sensor_BME280::Sensor_BME280(const JsonVariant &j) :
    temp(21.), hum(50.), pres(1080.), bme(bme280_settings)
{
    initialized = false;
    sda = j["sda"] | 2;
    scl = j["scl"] | 14;

    i2c.begin(sda, scl, 0x76);
    Serial.printf("Initializing BME280 ");
    for (int i = 0; i < 20; i++) {
        Serial.printf(".");
        if (bme.begin()) {
            initialized = true;
            state = SENSOR_INIT;
            break;
        }
        delay(1000);
    }

    if (!initialized)
        Serial.printf(" failed");
    else
        Serial.print(" done");
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
