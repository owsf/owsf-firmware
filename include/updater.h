/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifndef _UPDATER_H_
#define _UPDATER_H_

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

class Updater : public ESP8266HTTPUpdate {
private:
#if SIGNED_UPDATES
    BearSSL::PublicKey sign_pubkey;
    BearSSL::HashSHA256 hash;
    BearSSL::SigningVerifier sign;
#endif

public:
    int update(HTTPClient &, String &);

    Updater() {};
    ~Updater() {};
};

#endif
