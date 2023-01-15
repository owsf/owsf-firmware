/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "sensors/adc.h"
extern "C" {
    #include "user_interface.h"
}

Sensor_ADC::Sensor_ADC(const JsonVariant &j) : current_value(0), mem(-1),
	data_upload(false) {
    int rtcmem;

    r1 = j["R1"] | 9100.;
    r2 = j["R2"] | 47000.;
    offset = j["offset"] | 0.;
    factor = j["factor"] | 1.;

    threshold_voltage = j["threshold_voltage"] | 0.01;

    rtcmem = j["rtcmem_slot"] | -1;
    mem = RTCMEM_SENSOR(rtcmem);

    tags = j["tags"] | "";

    ESP.rtcUserMemoryRead(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
    if (rtc_data.data_upload == 0xdeadbeef) {
        rtc_data.data_upload = 0;
	ESP.rtcUserMemoryWrite(mem, (uint32_t *)&rtc_data,
			       sizeof(rtc_data));
        data_upload = true;
        return;
    }
}

void Sensor_ADC::publish(Point &p) {
    ESP.rtcUserMemoryRead(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
    rtc_data.data_upload = 0;
    ESP.rtcUserMemoryWrite(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
    p.addField("voltage", rtc_data.current_value);
}

Sensor_State Sensor_ADC::sample() {
    uint32_t adc_val;
    if (state != SENSOR_INIT)
        return state;
    state = SENSOR_DONE_NOUPDATE;

    if (data_upload)
	return SENSOR_DONE_UPDATE;

    Serial.printf("  Sampling sensor ADC\n");

    system_soft_wdt_stop();
    ets_intr_lock();
    noInterrupts();
    adc_val = system_adc_read();
    interrupts();
    ets_intr_unlock();
    system_soft_wdt_restart();

    current_value = factor * (1. * adc_val) * (1 + r1 / r2) / 1024 + offset;

    if (mem < 0) {
        state = SENSOR_DONE_UPDATE;
        return state;
    }

    state = SENSOR_DONE_NOUPDATE;
    ESP.rtcUserMemoryRead(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));
    if (threshold_helper_float(current_value, rtc_data.current_value,
			       threshold_voltage)) {
        state = SENSOR_DONE_UPDATE;
	rtc_data.data_upload = 0xdeadbeef;
	data_upload = true;
	rtc_data.current_value = current_value;
    } else {
	    rtc_data.data_upload = 0;
    }
    ESP.rtcUserMemoryWrite(mem, (uint32_t *)&rtc_data, sizeof(rtc_data));

    return state;
}

const char *Sensor_ADC::get_sensor_type() {
    return sensor_type;
}

String &Sensor_ADC::get_tags() {
    return tags;
}
