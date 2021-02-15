/*
 * (C) Copyright 2021 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <Wire.h>

#include "bme280.h"

Sensor_State Sensor_BME280::sample() {
    if (state != SENSOR_NOT_INIT && state != SENSOR_INIT)
        return state;
    state = SENSOR_DONE_NOUPDATE;

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

    if (mem < 0) {
        state = SENSOR_DONE_UPDATE;
        return state;
    }

    ESP.rtcUserMemoryRead(mem, (uint32_t*) &rtc_data, sizeof(rtc_data));
    if (threshold_helper_float(pres, &rtc_data.temp, threshold_pres))
        state = SENSOR_DONE_UPDATE;
    if (threshold_helper_float(hum, &rtc_data.temp, threshold_hum))
        state = SENSOR_DONE_UPDATE;
    if (threshold_helper_float(temp, &rtc_data.temp, threshold_temp))
        state = SENSOR_DONE_UPDATE;
    ESP.rtcUserMemoryWrite(mem, (uint32_t*) &rtc_data, sizeof(rtc_data));

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
    temp(21.), hum(50.), pres(1080.), bme(), mem(-1), sensor_type("BME280")
{
    Serial.println(F("Initializing BME280 "));

    initialized = false;
    sda = j["sda"] | 2;
    scl = j["scl"] | 14;

    threshold_pres = j["threshold_pres"] | 20.0;
    threshold_hum = j["threshold_hum"] | 0.5;
    threshold_temp = j["threshold_temp"] | 0.1;

    mem = get_rtc_addr(j);

    tags = j["tags"] | "";

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

String &Sensor_BME280::get_sensor_type() {
    return sensor_type;
}

String &Sensor_BME280::get_tags() {
    return tags;
}
