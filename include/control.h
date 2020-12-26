/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <Arduino.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>

#include "sensor.h"

class FirmwareControl {
private:
    String wifi_ssid;
    String wifi_pass;

    String global_config_key;
    uint32_t global_config_version;

    const char *influx_url;
    const char *influx_org;
    const char *influx_bucket;
    const char *influx_token;

    String device_name;

    char chip_id[11] = {0};

    String ctrl_url;
    uint32_t sleep_time_s;
    uint32_t config_version;

    BearSSL::CertStore cert_store;

    bool go_online_request;
    bool online;

    //std::unique_ptr<BearSSL::WiFiClientSecure> wcs;
    BearSSL::WiFiClientSecure *wcs;
    HTTPClient https;

    WiFiState *wss;

    SensorManager *sensor_manager;
    InfluxDBClient *influx;

protected:
    void read_global_config();
    void read_config();
    void OTA();
    void update_config(const char *);
    void go_online();
    void set_clock();
    void deep_sleep();

public:
    bool is_online() { return this->online; }
    void online_request() { go_online_request = true; }

    void loop();
    void setup();

    FirmwareControl();
    ~FirmwareControl() {};
};
