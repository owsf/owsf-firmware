/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <Wire.h>

#include "bme280.h"

Sensor_State Sensor_BME280::sample() {
    if (state != SENSOR_NOT_INIT && state != SENSOR_INIT)
        return state;

    Serial.printf("  Sampling sensor BME280\n");
    if (!initialized) {
        Serial.printf("  BME280 not initialized");
        return SENSOR_NOT_INIT;
    }

    Wire.begin(sda, scl);
    bme.takeForcedMeasurement();
    pres = bme.readPressure();
    hum = bme.readHumidity();
    temp = bme.readTemperature();

    state = SENSOR_DONE_UPDATE;

    return state;
}

void Sensor_BME280::publish(Point &p) {
    if (!initialized)
        return;

    p.addField("temperature", temp);
    p.addField("humidity", hum);
    p.addField("pressure", pres / 100.0);
}

Sensor_BME280::Sensor_BME280(const JsonVariant &j) :
    temp(21.), hum(50.), pres(1080.), bme()
{
    Serial.println(F("Initializing BME280 "));

    initialized = false;
    sda = j["sda"] | 2;
    scl = j["scl"] | 14;

    Wire.begin(sda, scl);
    if (!bme.begin(addr, &Wire))
        return;

    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF);

    initialized = true;
}
