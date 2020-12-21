/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifndef _UPDATER_H_
#define _UPDATER_H_

#if SIGNED_UPDATES
#include <CertStoreBearSSL.h>
#include <BearSSLHelpers.h>
#endif

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include "pubkey.h"

class Updater : public ESP8266HTTPUpdate {
private:
#if SIGNED_UPDATES
    BearSSL::PublicKey sign_pubkey;
    BearSSL::HashSHA256 hash;
    BearSSL::SigningVerifier sign;
#endif

public:
    int update(HTTPClient &, String &);

#if SIGNED_UPDATES
    Updater() : sign_pubkey(pubkey), sign(&sign_pubkey) {}
#else
    Updater() {}
#endif
    ~Updater() {}
};

#endif
