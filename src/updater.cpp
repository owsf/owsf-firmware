/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <Arduino.h>
#include <string>

#include "updater.h"

int Updater::update(HTTPClient &http, String &update_url, const char *version) {
#if SIGNED_UPDATES
    Update.installSignature(&this->hash, &this->sign);
    Serial.println(F("Installed signature for update verification"));
#endif

    this->setLedPin(LED_BUILTIN, LOW);

    http.setURL(update_url);
    auto ret = this->handleUpdate(http, version, false);

    int r;
    switch (ret) {
    case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n",
                      getLastError(), getLastErrorString().c_str());
        r = -1;
        break;

    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        r = 0;
        break;

    case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        r = 1;
        break;

    default:
        r = 0;
        break;
    }

    return r;
}
