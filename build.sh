#!/usr/bin/env bash
# Builds the three images for the nRF54L15 DK and copies them to prebuilt/.
# Run it in an nRF Connect SDK v3.3.0 environment, with west and the toolchain on the PATH.
set -euo pipefail

cd "$(dirname "$0")"
board=nrf54l15dk/nrf54l15/cpuapp
# Sysbuild names the application image after the directory it's built from.
app=$(basename "$PWD")

west build -p always -b "$board" -d build/v1 .
west build -p always -b "$board" -d build/v2-reboots . -- -DEXTRA_CONF_FILE=v2-reboots.conf
west build -p always -b "$board" -d build/no-params . -- -DEXTRA_CONF_FILE=no-params.conf

mkdir -p prebuilt
cp build/v1/merged.hex prebuilt/repro-1.0.0.hex
cp "build/v2-reboots/$app/zephyr/zephyr.signed.bin" prebuilt/repro-2.0.0-reboots.bin
cp build/no-params/merged.hex prebuilt/repro-no-params.hex
ls -l prebuilt
