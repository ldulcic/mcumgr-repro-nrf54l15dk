#!/usr/bin/env bash
# Builds the three images for the nRF54L15 DK and copies them to prebuilt/.
# Run it in an nRF Connect SDK v3.4.1 environment, with west and the toolchain on the PATH.
set -euo pipefail

cd "$(dirname "$0")"
board=nrf54l15dk/nrf54l15/cpuapp
# Sysbuild names the application image after the directory it's built from.
app=$(basename "$PWD")
zephyr_base=${ZEPHYR_BASE:-$(west topdir)/zephyr}

west build -p always -b "$board" -d build/v1 .
west build -p always -b "$board" -d build/v2-reboots . -- -DEXTRA_CONF_FILE=v2-reboots.conf
west build -p always -b "$board" -d build/no-params . -- -DEXTRA_CONF_FILE=no-params.conf

# Sysbuild doesn't write a merged hex here, so merge MCUboot and the signed application into one
# file to flash.
merge() {
  python3 "$zephyr_base/scripts/build/mergehex.py" -o "$2" \
    "$1/mcuboot/zephyr/zephyr.hex" "$1/$app/zephyr/zephyr.signed.hex"
}

mkdir -p prebuilt
merge build/v1 prebuilt/repro-1.0.0.hex
cp "build/v2-reboots/$app/zephyr/zephyr.signed.bin" prebuilt/repro-2.0.0-reboots.bin
merge build/no-params prebuilt/repro-no-params.hex
ls -l prebuilt
