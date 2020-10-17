// SPDX-License-Identifier: GPL-2.0
/*
 * Virtual block swap configuration helper based on userland_worker
 *
 * Copyright (C) 2020 Vlad Adumitroaie <celtare21@gmail.com>.
 *               2021 Adam W. Willis <return.of.octobot@gmail.com>
 */

#define pr_fmt(fmt) "vbswap_helper: " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/security.h>
#include <linux/delay.h>

#include "../security/selinux/include/security.h"

#define STANDARD_SIZE 4
#define MAX_CHAR 128
#define DELAY 125

static char** argv;

static struct delayed_work userland_work;

static void free_memory(char** argv, int size)
{
	int i;

	for (i = 0; i < size; i++)
		kfree(argv[i]);
	kfree(argv);
}

static char** alloc_memory(int size)
{
	char** argv;
	int i;

	argv = kmalloc(size * sizeof(char*), GFP_KERNEL);
	if (!argv)
		return NULL;

	for (i = 0; i < size; i++) {
		argv[i] = kmalloc(MAX_CHAR * sizeof(char), GFP_KERNEL);
		if (!argv[i]) {
			kfree(argv);
			return NULL;
		}
	}

	return argv;
}

static int call_userland(char** argv)
{
	static char* envp[] = {
		"SHELL=/bin/sh",
		"HOME=/",
		"USER=shell",
		"TERM=xterm-256color",
		"PATH=/product/bin:/apex/com.android.runtime/bin:/apex/com.android.art/bin:/system_ext/bin:/system/bin:/system/xbin:/odm/bin:/vendor/bin:/vendor/xbin",
		"DISPLAY=:0",
		NULL
	};

	return call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
}

static inline int linux_sh(const char* command)
{
	int ret;

	strcpy(argv[0], "/system/bin/sh");
	strcpy(argv[1], "-c");
	strcpy(argv[2], command);
	argv[3] = NULL;

	ret = call_userland(argv);
	if (!ret)
		pr_info("Calling %s", command);

	return ret;
}

static void vbswap_helper(void)
{
	int ret, retries = 0;

	do {
		ret = linux_sh("/system/bin/printf 4294967296 > /sys/devices/virtual/block/vbswap0/disksize");
		if (ret)
			msleep(DELAY);
	} while (ret && (retries++ < 25));

	linux_sh("/system/bin/mkswap /dev/block/vbswap0");
	linux_sh("/system/bin/swapon /dev/block/vbswap0");
}

static void vbswap_init(struct work_struct *work)
{
	bool is_enforcing;

	argv = alloc_memory(STANDARD_SIZE);
	if (!argv)
		return;

	is_enforcing = get_enforce_value();
	if (is_enforcing) {
		pr_info("Setting selinux state: permissive");
		set_selinux(0);
	}

	vbswap_helper();

	if (is_enforcing) {
		pr_info("Setting selinux state: enforcing");
		set_selinux(1);
	}
	
	free_memory(argv, STANDARD_SIZE);
}

static int __init vbswap_helper_entry(void)
{
	INIT_DELAYED_WORK(&userland_work, vbswap_init);
	queue_delayed_work(system_power_efficient_wq,
			&userland_work, DELAY * 4);

	return 0;
}

module_init(vbswap_helper_entry);
