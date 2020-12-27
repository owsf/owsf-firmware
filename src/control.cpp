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

#include "influxca.h"

#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

void FirmwareControl::set_clock() {
    char buffer[64];

    configTzTime(TZ_INFO, "pool.ntp.org", "time.nist.gov");

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

void FirmwareControl::update_config(const char *name) {
    StaticJsonDocument<1024> doc;
    String url, filename;

    https.useHTTP10(true);
    https.setTimeout(5000);
    https.addHeader(F("X-chip-id"), String(ESP.getChipId()));
    if (!strncasecmp(name, "global_config", 13)) {
        https.addHeader(F("X-global-config-version"), String(global_config_version));
        https.addHeader(F("X-global-config-key"), String(global_config_key));
        url = ctrl_url + "/global_config";
        filename = String("/") + name + ".json";
    } else if (!strncasecmp(name, "config", 6) ||
               !strncasecmp(name, "local_config", 12)) {
        https.addHeader(F("X-config-version"), String(config_version));
        url = ctrl_url + "/local_config";
        filename = F("/config.json");
    }

    if (!https.begin(*wcs, url))
        return;

    int http_code = https.GET();
    if (http_code < 0) {
        https.end();
        return;
    }

    /* Note: we do not use streaming API here! Because we try to be a little more
     * safe here
     * - First download new config
     * - Second test if config is JSON format
     * - Third write to file
     */
    String payload;
    if (http_code == HTTP_CODE_OK) {
        payload = https.getString();
        DeserializationError error = deserializeJson(doc, payload.c_str());
        if (error != DeserializationError::Ok) {
            Serial.printf("Could not load config file");
            https.end();
            return;
        }
    } else {
        Serial.print(https.getString());
        Serial.println();
        https.end();
        return;
    }
    https.end();

    File file = LittleFS.open(filename, "w");
    if (!file)
        return;

    file.write(payload.c_str(), payload.length());

    file.close();
}

void FirmwareControl::OTA() {
    if (ctrl_url.length() < 11)
        return;

    int idx = ctrl_url.indexOf("/", 7);
    if (idx)
        ctrl_url.remove(idx);

    String server = ctrl_url;
    server.remove(0, server.indexOf(":") + 3);
    bool mfln = wcs->probeMaxFragmentLength(server, 443, 1024);
    if (mfln)
        wcs->setBufferSizes(1024, 1024);
    else
        wcs->setBufferSizes(16384, 1024);

    ctrl_url += "/api/v1";

    update_config("global_config");
    update_config("local_config");

    Updater upd;
    upd.update(https, ctrl_url + "/firmware", VERSION);
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

    influx = new InfluxDBClient(influx_url, influx_org, influx_bucket,
                                influx_token, influxCA);
    HTTPOptions opt;
    opt.connectionReuse(true);
    opt.httpReadTimeout(10000);
    influx->setHTTPOptions(opt);
    influx->setWriteOptions(WriteOptions().writePrecision(WritePrecision::S));

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

void FirmwareControl::read_global_config() {
    File file = LittleFS.open("/global_config.json", "r");
    if (!file)
        return;

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error != DeserializationError::Ok) {
        Serial.printf("Could not global load config file");
        return;
    }
    file.close();

    global_config_key = doc["global_config_key"] | "ABCDEF";
    global_config_version = doc["global_config_version"] | 0;

    wifi_ssid = doc["wifi_ssid"] | "NO SSID";
    wifi_pass = doc["wifi_pass"] | "NO PSK";
    ctrl_url  = doc["ctrl_url"]  | "https://example.com";

    influx_url = strdup(doc["influx_url"] | "https://example.com");
    influx_token = strdup(doc["influx_token"] | "ABCDEFG");
    influx_bucket = strdup(doc["influx_bucket"] | "sensor_bucket");
    influx_org = strdup(doc["influx_org"] | "influx org");
}

void FirmwareControl::read_config() {
    File file = LittleFS.open("/config.json", "r");
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
        config_version = doc["config_version"] | 0;

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

    int numCerts = cert_store.initCertStore(LittleFS, PSTR("/certs.idx"),
                                            PSTR("/certs.ar"));
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
    if (!online && (go_online_request || !sensor_manager))
        go_online();

#if ENABLE_OTA
    if (online && (ESP.getResetReason() == "Power On" || !sensor_manager)) {
        OTA();

        delay(1000);
        pinMode(16, OUTPUT);
        digitalWrite(16, 0);
        /* we _should_ never reach this point ... but who knows */
        delay(100000);
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
