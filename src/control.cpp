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

#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

void FirmwareControl::set_clock() {
    char buffer[64];

    configTime(TZ_INFO, "pool.ntp.org", "time.nist.gov");

    Serial.print(F("Waiting for NTP time sync: "));
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        yield();
        delay(500);
        Serial.print(F("."));
        now = time(nullptr);
    }
    Serial.printf("\n");

    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print(F("Current time: "));
    strftime(buffer, 64, "%c", &timeinfo);
    Serial.println(buffer);
}

void FirmwareControl::OTA() {
    if (update_url.length() < 11)
        return;

    Updater upd;
    upd.update(https, update_url, VERSION);
}

void FirmwareControl::go_online() {
    Serial.printf("going online ...");
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

    if (online) {
        set_clock();
    } else {
        Serial.printf("Failed to go online");
        return;
    }

    wcs = new BearSSL::WiFiClientSecure();
    wcs->setCertStore(&cert_store);

    influx = new InfluxDBClient(influx_url, influx_org, influx_bucket, influx_token, InfluxDbCloud2CACert);
    if (influx->validateConnection()) {
        Serial.print("Connected to InfluxDB: ");
        Serial.println(influx->getServerUrl());
    } else {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(influx->getLastErrorMessage());
    }
}

void FirmwareControl::deep_sleep() {
    Serial.printf(" -> deep sleep for %u seconds\n", sleep_time_s);

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
                    Serial.printf("Could not load config file");
                update_url = doc["firmware_update"] | "";
            }
        }
    }
    https.end();
}

void FirmwareControl::read_global_config() {
    File file = LittleFS.open("/global_config.js", "r");
    if (!file)
        return;

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error != DeserializationError::Ok) {
        Serial.printf("Could not global load config file");
        return;
    }
    file.close();

    wifi_ssid = doc["wifi_ssid"] | "NO SSID";
    wifi_pass = doc["wifi_pass"] | "NO PSK";
    ctrl_url  = doc["ctrl_url"]  | "https://example.com";

    influx_url = strdup(doc["influx_url"] | "https://example.com");
    influx_token = strdup(doc["influx_token"] | "ABCDEFG");
    influx_bucket = strdup(doc["influx_bucket"] | "sensor_bucket");
    influx_org = strdup(doc["influx_org"] | "influx org");
}

void FirmwareControl::read_config() {
    File file = LittleFS.open("/config.js", "r");
    if (file) {
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, file);
        if (error != DeserializationError::Ok) {
            Serial.printf("Could not load config file");
            return;
        }
        file.close();

        sleep_time_s = doc["sleep_time_s"] | 60;
        device_name = doc["device_name"] | chip_id;

        JsonArray ja = doc["sensors"].as<JsonArray>();
        sensor_manager = new SensorManager(ja);
    } else {
        go_online_request = true;
    }
}

void FirmwareControl::setup() {
    snprintf(chip_id, 11, "0x%08x", ESP.getChipId());

    Serial.printf("ESP8266 Firmware Version %s (%s)\n", VERSION, BUILD_DATE);
    Serial.printf("  Chip ID: %s\n", chip_id);

    LittleFS.begin();

    wss = (WiFiState *)RTCMEM_WSS;

    int numCerts = cert_store.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
    Serial.print(F("Number of CA certs read: "));
    Serial.println(numCerts);
    if (numCerts == 0)
        Serial.printf("No certs found");

    read_global_config();
    read_config();

#if ENABLE_OTA
    if (ESP.getResetReason() == "Power On")
        go_online_request = true;
#endif
}

void FirmwareControl::loop() {
    if (!online && go_online_request)
        go_online();

#if ENABLE_OTA
    if (online && (ESP.getResetReason() == "Power On" || !sensor_manager)) {
        query_ctrl();
        OTA();
    }
#endif

    if (!sensor_manager->sensors_done())
        sensor_manager->loop();

    if (sensor_manager->sensors_done())
        go_online_request = sensor_manager->upload_requested();

    if (sensor_manager->sensors_done()) {
        if (online) {
            Point p("sensor_data");
            p.addTag("device", device_name);
            p.addTag("chip_id", chip_id);
            p.addTag("firmware_version", VERSION);
            p.setTime(time(nullptr));
            sensor_manager->publish(p);
            Serial.println(influx->pointToLineProtocol(p));
            influx->writePoint(p);
            deep_sleep();
        } else if (!sensor_manager->upload_requested()) {
            deep_sleep();
        }
    }
}

FirmwareControl::FirmwareControl() :
    influx_url(nullptr), influx_org(nullptr), influx_bucket(nullptr), influx_token(nullptr),
    sleep_time_s(600), go_online_request(false), online(false), wcs(nullptr), wss(nullptr),
    sensor_manager(nullptr), influx(nullptr) {
}
