/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * SBI routines (RISCV64)
 *
 * Copyright 2018, 2020, 2024 Phoenix Systems
 * Author: Pawel Pisarczyk, Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "sbi.h"

/* Base extension */
#define SBI_EXT_BASE 0x10

#define SBI_BASE_SPEC_VER      0x0
#define SBI_BASE_IMPL_ID       0x1
#define SBI_BASE_IMPL_VER      0x2
#define SBI_BASE_PROBE_EXT     0x3
#define SBI_BASE_GET_MVENDORID 0x4
#define SBI_BASE_GET_MARCHID   0x5
#define SBI_BASE_GET_MIMPLID   0x6

/* Timer extension */
#define SBI_EXT_TIME 0x54494D45

#define SBI_TIME_SETTIMER 0x0

/* Legacy extensions */
#define SBI_LEGACY_SETTIMER               0x0
#define SBI_LEGACY_PUTCHAR                0x1
#define SBI_LEGACY_GETCHAR                0x2
#define SBI_LEGACY_CLEARIPI               0x3
#define SBI_LEGACY_SENDIPI                0x4
#define SBI_LEGACY_REMOTE_FENCE_I         0x5
#define SBI_LEGACY_REMOTE_SFENCE_VMA      0x6
#define SBI_LEGACY_REMOTE_SFENCE_VMA_ASID 0x7
#define SBI_LEGACY_SHUTDOWN               0x8

/* clang-format off */
#define SBI_MINOR(x) ((x) & 0xffffff)
/* clang-format on */
#define SBI_MAJOR(x) ((x) >> 24)


static struct {
	u32 specVersion;
	void (*setTimer)(u64);
} sbi_common;


static sbiret_t sbi_ecall(int ext, int fid, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
	sbiret_t ret;

	register u64 a0 asm("a0") = arg0;
	register u64 a1 asm("a1") = arg1;
	register u64 a2 asm("a2") = arg2;
	register u64 a3 asm("a3") = arg3;
	register u64 a4 asm("a4") = arg4;
	register u64 a5 asm("a5") = arg5;
	register u64 a6 asm("a6") = fid;
	register u64 a7 asm("a7") = ext;

	/* clang-format off */
	__asm__ volatile (
		"ecall"
		: "+r" (a0), "+r" (a1)
		: "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
		: "memory");
	/* clang-format on */

	ret.error = a0;
	ret.value = a1;

	return ret;
}

/* Legacy SBI v0.1 calls */

static void sbi_setTimerv01(u64 stime)
{
	sbi_ecall(SBI_LEGACY_SETTIMER, 0, stime, 0, 0, 0, 0, 0);
}


long sbi_putchar(int ch)
{
	return sbi_ecall(SBI_LEGACY_PUTCHAR, 0, ch, 0, 0, 0, 0, 0).error;
}


long sbi_getchar(void)
{
	return sbi_ecall(SBI_LEGACY_GETCHAR, 0, 0, 0, 0, 0, 0, 0).error;
}

/* SBI v0.2+ calls */

sbiret_t sbi_getSpecVersion(void)
{
	return sbi_ecall(SBI_EXT_BASE, SBI_BASE_SPEC_VER, 0, 0, 0, 0, 0, 0);
}


sbiret_t sbi_probeExtension(long extid)
{
	return sbi_ecall(SBI_EXT_BASE, SBI_BASE_PROBE_EXT, extid, 0, 0, 0, 0, 0);
}


static void sbi_setTimerv02(u64 stime)
{
	sbi_ecall(SBI_EXT_TIME, SBI_TIME_SETTIMER, stime, 0, 0, 0, 0, 0);
}


void sbi_setTimer(u64 stime)
{
	sbi_common.setTimer(stime);
}


void sbi_init(void)
{
	sbiret_t ret = sbi_getSpecVersion();
	sbi_common.specVersion = ret.value;

	ret = sbi_probeExtension(SBI_EXT_TIME);
	if (ret.error == 0) {
		sbi_common.setTimer = sbi_setTimerv02;
	}
	else {
		sbi_common.setTimer = sbi_setTimerv01;
	}
}
