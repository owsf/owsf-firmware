#!/usr/bin/python3
#
# (C) Copyright 2020 Tillmann Heidsieck
#
# SPDX-License-Identifier: MIT
#
import os
import subprocess

#Import("env") # pylint:

ca_paths = [
    "/usr/share/ca-certificates/mozilla",
    "/etc/ca-certificates",
    "data"
]

cert_list = "misc/cert_list.txt"
if "CERT_LIST" in os.environ.keys():
    cert_list = os.environ["CERT_LIST"]

pems = []
with open(cert_list, "r") as f:
    pems = f.readlines()

idx = 0
certs = []
for p in pems:
    found = None 
    p = p.lstrip().rstrip().replace("\n", "")
    for cap in ca_paths:
        if os.path.exists(cap + "/" + p):
            found = cap + "/" + p
            break
    if not found:
        continue

    cert = "data/ca_%03d.der" % (idx)
    cmd = ['openssl', 'x509', '-inform', 'PEM', '-in', found, '-outform', 'DER', '-out', cert]
    subprocess.run(" ".join(cmd), shell=True)

    if os.path.exists(cert):
        certs.append(cert)
        idx = idx + 1

subprocess.run(['ar', 'q', 'data/certs.ar'] + certs)

# cleanup temporary files
for cert in certs:
    os.unlink(cert)
