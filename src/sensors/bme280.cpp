/*
 * (C) Copyright 2021 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <Wire.h>

#include "sensors/bme280.h"

Sensor_State Sensor_BME280::sample() {
    if (state != SENSOR_NOT_INIT && state != SENSOR_INIT)
        return state;
    state = SENSOR_DONE_NOUPDATE;

    Serial.printf("  Sampling sensor BME280\n");
    if (!initialized) {
        Serial.printf("  BME280 not initialized\n");
        return SENSOR_NOT_INIT;
    }

    if (data_upload)
	return SENSOR_DONE_UPDATE;

    Wire.begin(sda, scl);
    bme.takeForcedMeasurement();
    pres = bme.readPressure() / 100.;
    hum = bme.readHumidity();
    temp = bme.readTemperature();

    if (mem < 0) {
        state = SENSOR_DONE_UPDATE;
        return state;
    }

    ESP.rtcUserMemoryRead(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
    if (threshold_helper_float(pres, &rtc_data.pres, threshold_pres))
        state = SENSOR_DONE_UPDATE;
    if (threshold_helper_float(hum, &rtc_data.hum, threshold_hum))
        state = SENSOR_DONE_UPDATE;
    if (threshold_helper_float(temp, &rtc_data.temp, threshold_temp))
        state = SENSOR_DONE_UPDATE;
    if (state == SENSOR_DONE_UPDATE) {
	rtc_data.data_upload = 0xdeadbeef;
    } else {
	rtc_data.data_upload = 0;
    }
    ESP.rtcUserMemoryWrite(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));

    return state;
}

void Sensor_BME280::publish(Point &p) {
    if (!initialized)
        return;

    ESP.rtcUserMemoryRead(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
    rtc_data.data_upload = 0;
    ESP.rtcUserMemoryWrite(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
    p.addField("temperature", rtc_data.temp);
    p.addField("humidity", rtc_data.hum);
    p.addField("pressure", rtc_data.pres);
}

Sensor_BME280::Sensor_BME280(const JsonVariant &j) :
    temp(21.), hum(50.), pres(1080.), bme(), mem(-1), data_upload(false)
{
    int rtcmem;

    Serial.println(F("Initializing BME280 "));

    initialized = false;
    sda = j["sda"] | 2;
    scl = j["scl"] | 14;

    threshold_pres = j["threshold_pres"] | 5.0;
    threshold_hum = j["threshold_hum"] | 0.5;
    threshold_temp = j["threshold_temp"] | 0.3;

    rtcmem = j["rtcmem_slot"] | -1;
    mem = RTCMEM_SENSOR(rtcmem);

    tags = j["tags"] | "";

    ESP.rtcUserMemoryRead(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
    if (rtc_data.data_upload == 0xdeadbeef) {
	rtc_data.data_upload = 0;
	ESP.rtcUserMemoryWrite(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
	data_upload = true;
	initialized = true;
	return;
    }
    Wire.begin(sda, scl);
    delay(500);
    if (!bme.begin(addr, &Wire))
        return;

    delay(500);
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF);
    delay(500);

    initialized = true;
}

const char *Sensor_BME280::get_sensor_type() {
    return sensor_type;
}

String &Sensor_BME280::get_tags() {
    return tags;
}
