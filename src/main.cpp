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

#include "updater.h"

BearSSL::CertStore cert_store;

static char *wifi_ssid;
static char *wifi_pass;
static char *ctrl_url;
static char *update_url;

void setClock() {
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

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.printf("ESP8266 Firmware Version %s (%s)\n", VERSION, DATE);

    LittleFS.begin();

    int numCerts = cert_store.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
    Serial.print(F("Number of CA certs read: "));
    Serial.println(numCerts);
    if (numCerts == 0)
        Serial.println(F("No certs found"));

    File file = LittleFS.open("/config.js", "r");

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error != DeserializationError::Ok) {
        Serial.println(F("Could not load config file"));
        return;
    }
    file.close();

    wifi_ssid = strdup(doc["wifi_ssid"] | "NO SSID");
    wifi_pass = strdup(doc["wifi_pass"] | "NO PSK");
    ctrl_url  = strdup(doc["ctrl_url"] | "https://example.com");

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_pass);
}

void loop() {
    /* work around problems of WiFi status not always showing WL_CONNECTED */
    if ((WiFi.status() == WL_CONNECTED) || WiFi.localIP().isSet()) {
        static char c;
        if (!c)
            setClock();
        c = 1;

        {
            std::unique_ptr<BearSSL::WiFiClientSecure>wcs(new BearSSL::WiFiClientSecure);
            wcs->setCertStore(&cert_store);

            StaticJsonDocument<1024> doc;
            bool mfln = wcs->probeMaxFragmentLength(remote_server, 443, 512);
            if (mfln)
                wcs->setBufferSizes(512, 512);
            else
                wcs->setBufferSizes(16384, 512);

            HTTPClient https;
            if (https.begin(*wcs, ctrl_url)) {
                int http_code = https.GET();
                if (http_code > 0) {
                    if (http_code == HTTP_CODE_OK) {
                        String payload = https.getString();
                        DeserializationError error = deserializeJson(doc, payload.c_str());
                        if (error != DeserializationError::Ok) {
                            Serial.println(F("Could not load config file"));
                        }
                        if (!doc["firmware_update"].isNull()) {
                            update_url = strdup(doc["firmware_update"]);
                        }
                    }
                }
            }
            https.end();

            if (update_url) {
                Updater upd;
                upd.update(https, update_url);
            }
            wcs->stop();
        }

        Serial.println(F("loop end"));
    }

    delay(500);
}
