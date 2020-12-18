/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include "pubkey.h"

#include <string>

class Updater {
private:
#if SIGNED_UPDATES
    BearSSL::PublicKey sign_pubkey;
    BearSSL::HashSHA256 hash;
    BearSSL::SigningVerifier sign;
#endif

    BearSSL::CertStore *cert_store = nullptr;
public:
    int run(std::string &remote_server, std::string &remote_file);
    //void set_cert_store(BearSSL::CertStore *cert_store) { this->cert_store = cert_store; }

    explicit Updater(BearSSL::CertStore *);
    ~Updater();
};

#if SIGNED_UPDATES
Updater::Updater(BearSSL::CertStore *cs) : sign_pubkey(pubkey), sign(&sign_pubkey), cert_store(cs) {}
#else
Updater::Updater(BearSSL::CertStore *cs) : cert_store(cs) {}
#endif

Updater::~Updater() {}

int Updater::run(std::string &remote_server, std::string &remote_file) {
    BearSSL::WiFiClientSecure client;
    bool mfln;
    int r;

#if SIGNED_UPDATES
    Update.installSignature(&this->hash, &this->sign);
#endif

    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

    mfln = client.probeMaxFragmentLength(remote_server.c_str(), 443, 1024);
    Serial.printf("MFLN supported: %s\n", mfln ? "yes" : "no");
    if (mfln)
        client.setBufferSizes(1024, 1024);
    client.setCertStore(cert_store);

    t_httpUpdate_return ret = ESPhttpUpdate.update(client, remote_server.c_str(), 443, remote_file.c_str());

    switch (ret) {
    case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        r = -1;
        break;

    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        r = 0;
        break;

    case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        r = 0;
        break;

    default:
        r = 0;
        break;
    }

    return r;
}
