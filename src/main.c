/*
 * Based on the smp_svr sample of Zephyr:
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>

LOG_MODULE_REGISTER(repro, LOG_LEVEL_INF);

static struct k_work advertise_work;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, SMP_BT_SVC_UUID_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void advertise(struct k_work *work)
{
	int rc = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (rc) {
		LOG_ERR("Advertising failed to start (rc %d)", rc);
		return;
	}
	LOG_INF("Advertising as \"%s\"", CONFIG_BT_DEVICE_NAME);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
		k_work_submit(&advertise_work);
		return;
	}
	LOG_INF("Connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected, reason 0x%02x %s", reason, bt_hci_err_to_str(reason));
}

static void recycled(void)
{
	/* The connection object is free again, so the device can advertise again. */
	k_work_submit(&advertise_work);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = recycled,
};

int main(void)
{
	LOG_INF("Image %s started", CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION);

	if (IS_ENABLED(CONFIG_REPRO_REBOOT_ON_BOOT)) {
		/* Like an image that crashes on boot, before anything can confirm it. */
		LOG_ERR("Simulating a crash on boot, rebooting");
		/* The log is deferred and would not be printed before the reboot, so flush it now. */
		LOG_PANIC();
		k_sleep(K_MSEC(300));
		sys_reboot(SYS_REBOOT_COLD);
	}

	k_work_init(&advertise_work, advertise);

	int rc = bt_enable(NULL);

	if (rc) {
		LOG_ERR("Bluetooth failed to start (rc %d)", rc);
		return 0;
	}
	k_work_submit(&advertise_work);

	/* MCUmgr requests are handled by the SMP transport, so main() has nothing more to do. */
	return 0;
}
