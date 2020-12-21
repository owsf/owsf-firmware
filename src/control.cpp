/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <Arduino.h>

#include <ArduinoJson.h>
#include <CertStoreBearSSL.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <LittleFS.h>
#include <time.h>

#include "control.h"
#include "rtcmem_map.h"
#include "updater.h"
#include "version.h"

void FirmwareControl::set_clock() {
    char buffer[64];

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    Serial.print(F("Waiting for NTP time sync: "));
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        yield();
        delay(500);
        Serial.print(F("."));
        now = time(nullptr);
    }

    Serial.println(F(""));
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print(F("Current time: "));
    strftime(buffer, 64, "%c", &timeinfo);
    Serial.println(buffer);
}

void FirmwareControl::OTA() {
    if (!update_url.length() < 11)
        return;

    Updater upd;
    upd.update(https, update_url, VERSION);
}

void FirmwareControl::go_online() {
    WiFi.forceSleepWake();
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_pass);

    for (int i = 0; i < 1000; i++) {
        /* work around problems of WiFi status not always showing WL_CONNECTED */
        if ((WiFi.status() == WL_CONNECTED) || WiFi.localIP().isSet()) {
            this->online = true;
            break;
        }
        delay(100);
    }

    if (online)
        set_clock();
}

void FirmwareControl::deep_sleep() {
    if (online)
        WiFi.mode(WIFI_SHUTDOWN, wss);

    ESP.deepSleepInstant(sleep_time_s * 1E6, WAKE_RF_DISABLED);
}

void FirmwareControl::query_ctrl() {
    StaticJsonDocument<1024> doc;

    String server = ctrl_url;
    server.remove(0, server.indexOf(":") + 3);
    server.remove(server.indexOf("/"));
    bool mfln = wcs->probeMaxFragmentLength(server, 443, 512);
    if (mfln)
        wcs->setBufferSizes(512, 512);
    else
        wcs->setBufferSizes(16384, 512);

    if (https.begin(*wcs, ctrl_url)) {
        int http_code = https.GET();
        if (http_code > 0) {
            if (http_code == HTTP_CODE_OK) {
                String payload = https.getString();
                DeserializationError error = deserializeJson(doc, payload.c_str());
                if (error != DeserializationError::Ok)
                    Serial.println(F("Could not load config file"));
                update_url = doc["firmware_update"] | "";
            }
        }
    }
    https.end();
}

void FirmwareControl::setup() {
    Serial.printf("ESP8266 Firmware Version %s (%s)\n", VERSION, BUILD_DATE);

    LittleFS.begin();

    wss = (WiFiState *)RTCMEM_WSS;

    int numCerts = cert_store.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
    Serial.print(F("Number of CA certs read: "));
    Serial.println(numCerts);
    if (numCerts == 0)
        Serial.println(F("No certs found"));
    wcs->setCertStore(&cert_store);

    File file = LittleFS.open("/config.js", "r");

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error != DeserializationError::Ok) {
        Serial.println(F("Could not load config file"));
        return;
    }
    file.close();

    wifi_ssid = doc["wifi_ssid"] | "NO SSID";
    wifi_pass = doc["wifi_pass"] | "NO PSK";
    ctrl_url  = doc["ctrl_url"]  | "https://example.com";
    sleep_time_s = doc["sleep_time_s"] | 60;
}

void FirmwareControl::loop() {
}

FirmwareControl::FirmwareControl() : go_online_request(false), online(false), wcs(new BearSSL::WiFiClientSecure), wss(nullptr) {
}
