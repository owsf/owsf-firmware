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
    BearSSL::PublicKey *signPubKey = nullptr;
    BearSSL::HashSHA256 *hash;
    BearSSL::SigningVerifier *sign;
    BearSSL::CertStore *certStore;
public:
    int run(std::string &remote_server, std::string &remote_file);

    Updater(BearSSL::CertStore *certStore); 
    ~Updater();
};

Updater::Updater(BearSSL::CertStore *certStore) { 
    this->certStore = certStore;

#if SIGNED_UPDATES
    this->signPubKey = new BearSSL::PublicKey(pubkey);
    this->hash = new BearSSL::HashSHA256();
    this->sign = new BearSSL::SigningVerifier(signPubKey);
#endif
}

Updater::~Updater() {
    delete this->signPubKey;
    delete this->hash;
    delete this->sign;
}

int Updater::run(std::string &remote_server, std::string &remote_file) {
    BearSSL::WiFiClientSecure client;
    bool mfln;
    int r;

#if SIGNED_UPDATES
    Update.installSignature(this->hash, this->sign);
#endif

    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

    mfln = client.probeMaxFragmentLength(remote_server.c_str(), 443, 1024);
    Serial.printf("MFLN supported: %s\n", mfln ? "yes" : "no");
    if (mfln)
        client.setBufferSizes(1024, 1024);
    client.setCertStore(certStore);

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
