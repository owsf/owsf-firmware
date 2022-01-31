/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <Arduino.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <IPAddress.h>
#include <include/WiFiState.h>
#include <InfluxDbClient.h>

#include "sensor.h"

class NetCfg {
private:
    uint32_t ip_addr = 0;
    uint32_t netmask = 0xffffffff;
    uint32_t gateway = 0;
    uint32_t namesrv = 0;
    uint8_t  bssid[6] = {0};
    uint8_t  chan = 0;
    bool     ok = false;
public:
    NetCfg(bool load = false);
    void clear();
    void update();
    void save();

    IPAddress IP() { return IPAddress(ip_addr); }
    IPAddress Netmask() { return IPAddress(netmask); }
    IPAddress GW() { return IPAddress(gateway); }
    IPAddress DNS() { return IPAddress(namesrv); }
    uint8_t channel() { return chan; }
    uint8_t * BSSID() { return bssid; }
    bool valid() { return ok; }
};

class FirmwareControl {
private:
    String wifi_ssid;
    String wifi_pass;

    NetCfg netcfg;

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
    bool OTA();
    bool update_config(const char *);
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
