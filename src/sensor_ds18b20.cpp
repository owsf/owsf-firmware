/*
 * (C) Copyright 2021 Tillmann Heidsieck
 * (C) Copyright 2021 Dominik Laton
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "sensor_ds18b20.h"

Sensor_State Sensor_DS18B20::sample() {
    if (state != SENSOR_NOT_INIT && state != SENSOR_INIT)
        return state;
    state = SENSOR_DONE_NOUPDATE;

    Serial.printf("  Sampling sensor DS18B20\n");
    if (!initialized) {
        Serial.printf("  DS18B20 not initialized");
        return SENSOR_NOT_INIT;
    }

    temp = ds->getTempC();

    if (mem < 0) {
        state = SENSOR_DONE_UPDATE;
        return state;
    }

    ESP.rtcUserMemoryRead(mem, (uint32_t*) &rtc_data, sizeof(rtc_data));
    if (threshold_helper_float(temp, &rtc_data.temp, 0.1))
        state = SENSOR_DONE_UPDATE;
    ESP.rtcUserMemoryWrite(mem, (uint32_t*) &rtc_data, sizeof(rtc_data));

    return state;
}

void Sensor_DS18B20::publish(Point &p) {
    if (!initialized)
        return;

    p.addField("temperature", temp);
}

Sensor_DS18B20::Sensor_DS18B20(const JsonVariant &j) :
    temp(21.), ds(nullptr), mem(-1)
{
    uint8_t pin;

    Serial.println(F("Initializing DS18B20 "));

    initialized = false;
    pin = j["pin"] | 12;

    mem = get_rtc_addr(j);

    ds = new DS18B20(pin);
    if (! ds->selectNext())
        return;

    initialized = true;
}
