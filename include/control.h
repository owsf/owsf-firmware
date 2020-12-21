/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <Arduino.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

class FirmwareControl {
private:
    String wifi_ssid;
    String wifi_pass;

    String ctrl_url;
    String update_url;
    uint32_t sleep_time_s;

    BearSSL::CertStore cert_store;

    bool go_online_request;
    bool online;

    std::unique_ptr<BearSSL::WiFiClientSecure>wcs;
    HTTPClient https;

    WiFiState *wss;

protected:
    void OTA();
    void query_ctrl();
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
