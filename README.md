# MCUmgr repro firmware for the nRF54L15 DK

A minimal Zephyr application that reproduces two issues of
[nRF Connect Device Manager for Android](https://github.com/nordicsemi/Android-nRF-Connect-Device-Manager)
on an nRF54L15 DK:

1. [Requests fail on a device without the MCUmgr params command](#1-requests-fail-without-the-mcumgr-params-command)
   (`mcumgr-ble2`).
2. [Test and Confirm confirms an image that failed its test boot](#2-test-and-confirm-confirms-an-image-that-failed-its-test-boot)
   (`mcumgr-core`).

The application is an MCUmgr (SMP) server over Bluetooth LE with the image and OS management
groups, behind MCUboot in swap-using-move mode. It's based on Zephyr's
[`smp_svr`](https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/mgmt/mcumgr/smp_svr)
sample, without the parts the repros don't need.

## What you need

- An nRF54L15 DK.
- An Android phone with the Device Manager sample app (the `sample` module), built from `main`.
- [nRF Util](https://www.nordicsemi.com/Products/Development-tools/nRF-Util) with the `device`
  command, to flash the DK.
- Optional: a serial terminal on the DK's VCOM1 port at 115200 baud. It shows the logs of MCUboot
  and the application.

## Images

`prebuilt/` has the images, built by `build.sh` with nRF Connect SDK v3.3.0. They're signed with
MCUboot's development key (`root-ed25519.pem`), so upload `repro-2.0.0-reboots.bin` only to a DK
flashed with `repro-1.0.0.hex`.

| File | What it is |
|---|---|
| `repro-1.0.0.hex` | MCUboot and version 1.0.0 of the application. Advertises as "SMP repro". |
| `repro-2.0.0-reboots.bin` | Version 2.0.0, signed for upload. It reboots 300 ms after it starts, like an image that crashes on boot, so it never confirms itself. |
| `repro-no-params.hex` | MCUboot and version 1.0.0 without the MCUmgr params command (`CONFIG_MCUMGR_GRP_OS_MCUMGR_PARAMS=n`, Zephyr's default). Advertises as "SMP repro no params". |

To flash a `.hex` file (add `--serial-number` if more than one DK is connected):

```sh
nrfutil device program --firmware prebuilt/repro-1.0.0.hex --options chip_erase_mode=ERASE_ALL
```

## 1. Requests fail without the MCUmgr params command

Affects `mcumgr-ble2`, which the sample app uses on `main`.

1. Flash `repro-no-params.hex`.
2. In the sample app, connect to "SMP repro no params" and tap READ on the Image tab.

When the SMP service opens, `mcumgr-ble2` asks for the MCUmgr params. This device replies
`{"rc": 8}` (not supported). `mcumgr-ble2` takes that as a reassembly buffer of 0 bytes, so every
request fails:

```
SMP reassembly supported with buffer size: 0 bytes, and count: 0
Payload (9 bytes) too long for MTU: 0
```

Expected: the images are read, as with `mcumgr-ble`, which ignores such a reply.

## 2. Test and Confirm confirms an image that failed its test boot

Affects `ConfirmAfterReset` in `mcumgr-core`.

1. Flash `repro-1.0.0.hex`.
2. Copy `repro-2.0.0-reboots.bin` to the phone, for example with
   `adb push prebuilt/repro-2.0.0-reboots.bin /sdcard/Download/`.
3. In the sample app, connect to "SMP repro". On the Image tab, select the file and choose Test
   and Confirm. Set the estimated swap time to 20 s: it's in the BASIC mode, under the expand
   arrow of the Firmware Upgrade card. Start the upgrade.
4. When it's done, tap READ, then reset the DK.

What happens:

- MCUboot swaps in 2.0.0 for its test boot ("Swap type: test"). 2.0.0 reboots before anything can
  confirm it, so MCUboot swaps 1.0.0 back ("Swap type: revert").
- The app reconnects and confirms 2.0.0 by its hash, although 2.0.0 isn't running. Zephyr allows
  confirming an image that isn't active (`CONFIG_MCUMGR_GRP_IMG_ALLOW_CONFIRM_NON_ACTIVE_SLOT` is
  `y` by default in the swap modes) and marks it pending and permanent. The app reports that the
  upgrade is complete.
- READ shows 2.0.0 in slot 1 as pending and permanent.
- On the reset, MCUboot installs 2.0.0 for good ("Swap type: perm"), and the DK keeps rebooting.
  Flash `repro-1.0.0.hex` to recover it.

Expected: the upgrade fails because the new image isn't running, and nothing is confirmed.

## Building

In an nRF Connect SDK v3.3.0 environment:

```sh
./build.sh
```

It builds the three images into `build/` and copies them to `prebuilt/`. Each image is one
`west build`:

```sh
west build -b nrf54l15dk/nrf54l15/cpuapp -d build/v1 .
west build -b nrf54l15dk/nrf54l15/cpuapp -d build/v2-reboots . -- -DEXTRA_CONF_FILE=v2-reboots.conf
west build -b nrf54l15dk/nrf54l15/cpuapp -d build/no-params . -- -DEXTRA_CONF_FILE=no-params.conf
```

## License

Apache-2.0, like the Zephyr sample it's based on.
