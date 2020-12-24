/*
 * (C) Copyright 2020 Tillmann Heidsieck
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef _PUBKEY_H_
#define _PUBKEY_H_

#if SIGNED_UPDATES
#include "signing_pubkey.h"
#else
extern const char pubkey[];
#endif

#endif
