/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <BME280I2C.h>
#include <Wire.h>

#include "bme280.h"

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
    p.addField("pressure", pres / 100.0);
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
