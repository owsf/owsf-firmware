/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <Arduino.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <include/WiFiState.h>
#include <InfluxDbClient.h>

#include "sensor.h"

class FirmwareControl {
private:
    String wifi_ssid;
    String wifi_pass;

    String global_config_key;
    uint32_t global_config_version;

    const char *influx_url = nullptr;
    const char *influx_org = nullptr;
    const char *influx_bucket = nullptr;
    const char *influx_token = nullptr;

    String device_name;

    char chip_id[11] = {0};

    String ctrl_url;
    uint32_t sleep_time_s;
    uint32_t config_version;

    BearSSL::CertStore cert_store;

    bool go_online_request;
    bool ota_request;
    bool rf_enabled;
    bool online;
    uint32_t reboot_count;
    uint32_t ota_check_after;

    WiFiClient *wifi_client;
    BearSSL::X509List *_cert = nullptr;
    HTTPClient *https = nullptr;

    SensorManager *sensor_manager = nullptr;
    InfluxDBClient *influx = nullptr;

protected:
    void publish_sensor_data();
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
