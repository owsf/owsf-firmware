/*
 * (C) Copyright 2020, 2022 Tillmann Heidsieck
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
#include <IPAddress.h>
#include <LittleFS.h>
#include <TZ.h>
#include <time.h>

#include "control.h"
#include "rtcmem_map.h"
#include "updater.h"
#include "version.h"

#include "influxca.h"


NetCfg::NetCfg(bool load) {
    if (!load)
        return;

    uint32_t mem;
    ESP.rtcUserMemoryRead(RTCMEM_NET_CFG_MAGIC, &mem, sizeof(mem));
    if (mem != 0xdeadbeef)
        return;

    ok = true;
    ESP.rtcUserMemoryRead(RTCMEM_NET_CFG_IP,    &this->ip_addr, sizeof(this->ip_addr));
    ESP.rtcUserMemoryRead(RTCMEM_NET_CFG_MASK,  &this->netmask, sizeof(this->netmask));
    ESP.rtcUserMemoryRead(RTCMEM_NET_CFG_GW,    &this->gateway, sizeof(this->gateway));
    ESP.rtcUserMemoryRead(RTCMEM_NET_CFG_DNS,   &this->namesrv, sizeof(this->namesrv));
    ESP.rtcUserMemoryRead(RTCMEM_NET_CFG_BSSID, (uint32_t *)&this->bssid, 8);
    ESP.rtcUserMemoryRead(RTCMEM_NET_CFG_CHAN,  (uint32_t *)&this->chan, 4);
}

void NetCfg::save() {
    ok = true;
    ESP.rtcUserMemoryWrite(RTCMEM_NET_CFG_IP,    &this->ip_addr, sizeof(this->ip_addr));
    ESP.rtcUserMemoryWrite(RTCMEM_NET_CFG_MASK,  &this->netmask, sizeof(this->netmask));
    ESP.rtcUserMemoryWrite(RTCMEM_NET_CFG_GW,    &this->gateway, sizeof(this->gateway));
    ESP.rtcUserMemoryWrite(RTCMEM_NET_CFG_DNS,   &this->namesrv, sizeof(this->namesrv));
    ESP.rtcUserMemoryWrite(RTCMEM_NET_CFG_BSSID, (uint32_t *)&this->bssid, 8);
    ESP.rtcUserMemoryWrite(RTCMEM_NET_CFG_CHAN,  (uint32_t *)&this->chan, 4);

    uint32_t mem = 0xdeadbeef;
    ESP.rtcUserMemoryWrite(RTCMEM_NET_CFG_MAGIC, &mem, sizeof(mem));
}

void NetCfg::clear() {
    uint32_t mem = 0xffffffff;
    ESP.rtcUserMemoryWrite(RTCMEM_NET_CFG_MAGIC, &mem, sizeof(mem));
}

void NetCfg::update() {
    struct station_config conf;

    wifi_station_get_config(&conf);
    for (uint8_t i = 0; i < sizeof(conf.bssid); i++)
        bssid[i] = conf.bssid[i];

    chan    = wifi_get_channel();
    ip_addr = WiFi.localIP();
    gateway = WiFi.gatewayIP();
    netmask = WiFi.subnetMask();
    namesrv = WiFi.dnsIP();

    save();
}

void FirmwareControl::set_clock() {
    char buffer[64];

    Serial.print(F("NTP Server: "));
    Serial.println(ntp_server);
    configTzTime(TZ_Europe_Berlin, ntp_server);

    Serial.print(F("Waiting for NTP time sync: "));
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        yield();
        delay(500);
        Serial.print(F("."));
        now = time(nullptr);
    }
    Serial.println();

    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print(F("Current time: "));
    strftime(buffer, 64, "%c", &timeinfo);
    Serial.println(buffer);
}

bool FirmwareControl::update_config(const char *name) {
    Serial.print(F("Update configuration: "));
    Serial.println(name);

    DynamicJsonDocument doc(1024);

    String url = ctrl_url + "/" + name;
    if (!https->begin(*wifi_client, url))
        return false;

    https->setUserAgent(F("ESP8266-OTA"));
    https->addHeader(F("X-chip-id"), chip_id);

    String filename;
    if (!strncasecmp(name, "global_config", 13)) {
        https->addHeader(F("X-global-config-version"), String(global_config_version));
        https->addHeader(F("X-global-config-key"), String(global_config_key));
        filename = String("/") + name + ".json";
    } else if (!strncasecmp(name, "local_config", 12)) {
        https->addHeader(F("X-config-version"), String(config_version));
        filename = F("/config.json");
    } else {
        https->end();
        return false;
    }

    int http_code = https->GET();
    if (http_code < 0) {
        https->end();
        return false;
    }

    /* Note: we do not use streaming API here! Because we try to be a little more
     * safe here
     * - First download new config
     * - Second test if config is JSON format
     * - Third write to file
     */
    String payload;
    if (http_code == HTTP_CODE_OK) {
        payload = https->getString();
        DeserializationError error = deserializeJson(doc, payload.c_str());
        if (error != DeserializationError::Ok) {
            Serial.printf("Could not load config file");
            https->end();
            return false;
        }
    } else {
        Serial.print(https->getString());
        Serial.println();
        https->end();
        return false;
    }
    https->end();

    File file = LittleFS.open(filename, "w");
    if (!file)
        return false;

    file.write(payload.c_str(), payload.length());

    file.close();

    return true;
}

bool FirmwareControl::OTA() {
    if (ctrl_url.length() < 11) {
        Serial.println(F("Invalid CTRL_URL"));
        return false;
    }

    BearSSL::WiFiClientSecure *wcs = new BearSSL::WiFiClientSecure;
    if (!wcs) {
        Serial.println(F("OOM: could not allocate WiFiClientSecure"));
        return false;
    }

    wcs->setCertStore(&cert_store);

    String server = ctrl_url;
    server.remove(0, server.indexOf(":") + 3);
    bool mfln = wcs->probeMaxFragmentLength(server, 443, 1024);
    if (mfln) {
        Serial.println(F("MFLN supported"));
        wcs->setBufferSizes(1024, 1024);
    }

    wifi_client = wcs;

    https = new HTTPClient;
    if (!https) {
        Serial.println(F("OOM: could not allocate HTTPClient"));
        return false;
    }

    https->setReuse(true);
    https->setTimeout(20000);

    ctrl_url += "/api/v1";

    Serial.print(F("control server: "));
    Serial.println(ctrl_url);

    bool new_cfg = update_config("global_config");
    new_cfg = update_config("local_config") || new_cfg;

    ctrl_url += "/firmware";
    Updater upd;
    int r = upd.update(*https, ctrl_url, VERSION);

    return new_cfg || r > 0 ? true : false;
}

void FirmwareControl::publish_trace_data() {
    Point point("trace_data");
    point.addTag("device", device_name);
    point.addTag("chip_id", chip_id);
    point.addTag("firmware_version", VERSION);
    point.setTime(time(nullptr));
    point.addField("connect_time", connect_time);
    point.addField("sample_time", sample_time);
    point.addTag("valid_net_cfg", valid_net_cfg ? "true" : "false");
    Serial.println(influx->pointToLineProtocol(point));
    influx->writePoint(point);
}

void FirmwareControl::publish_data() {
    uint8_t num_sensors;

    num_sensors = sensor_manager->get_num_sensors();
    influx = new InfluxDBClient(influx_url, influx_org, influx_bucket,
                                influx_token, influxCA);
    HTTPOptions opt;
    opt.connectionReuse(true);
    opt.httpReadTimeout(10000);
    influx->setHTTPOptions(opt);
    // NOTE: check batchSize with respect to num_sensors if there are memory
    // consumption errors upcomming
    influx->setWriteOptions(WriteOptions().writePrecision(WritePrecision::S).batchSize(num_sensors).bufferSize(2*num_sensors).maxRetryInterval(10));

    if (influx->validateConnection()) {
        Serial.print("Connected to InfluxDB: ");
        Serial.println(influx->getServerUrl());
    } else {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(influx->getLastErrorMessage());
    }

    sensor_manager->publish(influx, &device_name, chip_id, VERSION);
    publish_trace_data();

    influx_retry = 0;
    while (!influx->isBufferEmpty() && influx_retry < 30) {
        influx->flushBuffer();
        Serial.print("InfluxDB flush failed: ");
        Serial.println(influx->getLastErrorMessage());
        Serial.print("Full buffer: ");
        Serial.println(influx->isBufferFull() ? "Yes" : "No");
        influx_retry++;
        delay(1000);
    }
}

void FirmwareControl::go_online() {
    bool error = false;
    int8_t status;
    uint32_t tmp = 0, start_time = millis();
    uint32_t sleep_factor = 1;
    
    ESP.rtcUserMemoryWrite(RTCMEM_GO_ONLINE, &tmp, sizeof(tmp));

    if (!rf_active)
        goto sleep;

    WiFi.persistent(false);
    if (!WiFi.mode(WIFI_STA)) {
        WiFi.mode(WIFI_OFF);
        Serial.println("Cannot WIFI_STA!");
        Serial.flush();
        error = true;
    }

    this->netcfg = NetCfg(true);
    valid_net_cfg = this->netcfg.valid();
    if (!error && valid_net_cfg) {
        Serial.println("... found valid network config");
        Serial.flush();
        if (!WiFi.config(netcfg.IP(), netcfg.GW(), netcfg.Netmask(), netcfg.DNS())) {
            error = true;
            Serial.println("Cannot config!");
            Serial.flush();
        }

        if (!error && !WiFi.begin(wifi_ssid, wifi_pass, netcfg.channel(), netcfg.BSSID())) {
            Serial.println("Cannot begin from config!");
            Serial.flush();
            error = true;
        }
    } else if (!error && !WiFi.begin(wifi_ssid, wifi_pass)) {
        Serial.println("Cannot begin!");
        Serial.flush();
        error = true;
    }

    status = WiFi.waitForConnectResult(10000);
    if (!error && status != WL_CONNECTED) {
        Serial.printf("Cannot connect (%d)!", status);
        Serial.flush();
        error = true;
    }

    if (!error && !WiFi.localIP().isSet()) {
        Serial.println("Cannot localIP!");
        Serial.flush();
        error = true;
    }

    this->online = !error;

    if (!online) {
        netcfg.clear();
        sleep_factor = 60;
        Serial.println(F("Failed to go online"));
sleep:
        tmp = 1;
        ESP.rtcUserMemoryWrite(RTCMEM_GO_ONLINE, &tmp, sizeof(tmp));
        Serial.flush();
        ESP.deepSleepInstant(sleep_factor * 1E6, WAKE_RF_DEFAULT);
        delay(100);
    }

    connect_time = millis() - start_time;
    netcfg.update();

    set_clock();
}

void FirmwareControl::deep_sleep() {
    Serial.print(F(" -> deep sleep for "));
    Serial.println(sleep_time_s);
    Serial.flush();

    ++reboot_count;
    ESP.rtcUserMemoryWrite(RTCMEM_REBOOT_COUNTER, &reboot_count,
                           sizeof(reboot_count));

    if (online)
        WiFi.mode(WIFI_OFF);

    ESP.deepSleepInstant(sleep_time_s * 1E6, WAKE_RF_DISABLED);
    delay(100);
}

void FirmwareControl::read_global_config() {
    File file = LittleFS.open("/global_config.json", "r");
    if (!file)
        return;

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error != DeserializationError::Ok) {
        Serial.println(F("Could not global load config file"));
        return;
    }
    file.close();

    global_config_key = doc["global_config_key"] | "ABCDEF";
    global_config_version = doc["global_config_version"] | 0;

    wifi_ssid = doc["wifi_ssid"] | "NO SSID";
    wifi_pass = doc["wifi_pass"] | "NO PSK";
    ctrl_url  = doc["ctrl_url"]  | "https://example.com";
    int idx = ctrl_url.indexOf("/", 8);
    if (idx)
        ctrl_url.remove(idx);

    influx_url = strdup(doc["influx_url"] | "https://example.com");
    influx_token = strdup(doc["influx_token"] | "ABCDEFG");
    influx_bucket = strdup(doc["influx_bucket"] | "sensor_bucket");
    influx_org = strdup(doc["influx_org"] | "influx org");

    ntp_server = strdup(doc["ntp_server"] | "pool.ntp.org");
}

void FirmwareControl::read_config() {
    File file = LittleFS.open("/config.json", "r");
    StaticJsonDocument<1024> doc;
    JsonArray ja;

    if (file) {
        DeserializationError error = deserializeJson(doc, file);
        if (error != DeserializationError::Ok) {
            Serial.printf("Could not load config file");
            return;
        }
        file.close();

        sleep_time_s = doc["sleep_time_s"] | 60;
        ota_check_after = doc["ota_check_after"] | 10000;
        forced_data_after = doc["forced_data_after"] | 0;
        device_name = doc["device_name"] | chip_id;
        config_version = doc["config_version"] | 0;

        ja = doc["sensors"].as<JsonArray>();
        sensor_manager = new SensorManager(ja);
    } else {
        ota_request = true;
        Serial.println(F("OTA Request: No local config found"));
        deserializeJson(doc, "{}");
        ja = doc.as<JsonArray>();
        sensor_manager = new SensorManager(ja);
    }
}

void FirmwareControl::setup() {
    snprintf(chip_id, 11, "0x%08x", ESP.getChipId());

    Serial.printf("ESP8266 Firmware Version %s (%s)\n", VERSION, BUILD_DATE);
    Serial.printf("  Chip ID: %s\n", chip_id);
    Serial.printf("  CPU Freq: %hhu\n", ESP.getCpuFreqMHz());
    Serial.print(F("  Reset Reason: "));
    Serial.println(ESP.getResetReason());
    LittleFS.begin();

    read_global_config();
    read_config();

    if (ESP.getResetReason() == F("Power On") || ESP.getResetReason() == F("External System")) {
        Serial.print(F("OTA Request: "));
        Serial.println(ESP.getResetReason());
        ota_request = true;
        reboot_count = 0;
        ESP.rtcUserMemoryWrite(RTCMEM_REBOOT_COUNTER,
                               &reboot_count,
                               sizeof(reboot_count));
    }

    ESP.rtcUserMemoryRead(RTCMEM_REBOOT_COUNTER,
                          &reboot_count,
                          sizeof(reboot_count));
    if (!(reboot_count % ota_check_after)) {
        Serial.println("OTA Request: Reboot counter");
        ota_request = true;
    }

    if (forced_data_after && !(reboot_count % forced_data_after)) {
        Serial.println("GoOnline Request: Reboot counter");
        go_online_request = true;
    }

    uint32_t tmp;
    ESP.rtcUserMemoryRead(RTCMEM_GO_ONLINE, &tmp, sizeof(tmp));
    if (tmp) {
        rf_active = true;
        go_online_request = true;
    }

    int num_certs = cert_store.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
    Serial.print("Number of CA certs read: ");
    Serial.println(num_certs);
    if (!num_certs)
        Serial.println("No certs found");
}

void FirmwareControl::loop() {
    bool ota_effect = false;
    uint32_t start_time;

    if (!online && (go_online_request || ota_request))
        go_online();

    if (online && ota_request) {
        ota_effect = OTA();
        if (ota_effect)
            ESP.reset();

        /* we did try OTA but either it had no effect or did fail,
         * either way we clear the request
         */
        ota_request = false;
    }

    if (sensor_manager->sensors_done()) {
        go_online_request = sensor_manager->upload_requested();

        if (online) {
            publish_data();
            deep_sleep();
        } else if (!go_online_request) {
            deep_sleep();
        }
    } else {
        start_time = millis();
        sensor_manager->loop();
        sample_time += millis() - start_time;
    }
}

FirmwareControl::FirmwareControl() :
    influx_url(nullptr),
    influx_org(nullptr),
    influx_bucket(nullptr),
    influx_token(nullptr),
    influx_retry(0),
    device_name("OWSF Sensor ESP8266"),
    sleep_time_s(600),
    config_version(0),
    rf_active(false),
    go_online_request(false),
    ota_request(false),
    online(false),
    reboot_count(0),
    ota_check_after(10000),
    forced_data_after(0),
    sensor_manager(nullptr),
    influx(nullptr),
    connect_time(0),
    sample_time(0),
    valid_net_cfg(false)
{
}
