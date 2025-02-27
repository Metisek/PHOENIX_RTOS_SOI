/*
 * Phoenix-RTOS
 *
 * GR716 flash server tests for meterfs
 *
 * Copyright 2020, 2023 Phoenix Systems
 * Author: Hubert Buczynski, Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/msg.h>

#include <meterfs.h>
#include <gr716-flashsrv.h>


#define LOG_ERROR(str, ...) \
	do { \
		fprintf(stderr, __FILE__ ":%d error: " str "\n", __LINE__, ##__VA_ARGS__); \
	} while (0)


/* Internal functions */

static int mfs_allocate(oid_t oid, const char *name, size_t sectors, size_t filesz, size_t recordsz)
{
	int err;
	msg_t msg;
	meterfs_i_devctl_t *i = (meterfs_i_devctl_t *)msg.i.raw;
	meterfs_o_devctl_t *o = (meterfs_o_devctl_t *)msg.o.raw;

	msg.type = mtDevCtl;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = NULL;
	msg.o.size = 0;

	i->type = meterfs_allocate;
	strncpy(i->allocate.name, name, sizeof(i->allocate.name) - 1);
	i->allocate.sectors = sectors;
	i->allocate.filesz = filesz;
	i->allocate.recordsz = recordsz;

	if ((err = msgSend(oid.port, &msg)) != 0) {
		LOG_ERROR("Cannot send msg.\n");
	}

	if ((err = o->err) < 0) {
		LOG_ERROR("Cannot allocate file %s, error: (%s).", name, strerror(err));
	}

	return err;
}


static int mfs_lookup(const char *file, oid_t *poid, oid_t *foid)
{
	int err;
	msg_t msg;

	msg.type = mtLookup;
	msg.i.io.oid = *poid;
	msg.i.data = (void *)file;

	if ((err = msgSend(poid->port, &msg)) != 0) {
		LOG_ERROR("Cannot send msg.\n");
		return err;
	}

	if ((err = msg.o.lookup.err) < 0) {
		LOG_ERROR("Cannot lookup file, error: (%s).", strerror(err));
		return err;
	}

	foid->id = msg.o.lookup.fil.id;
	foid->port = msg.o.lookup.fil.port;

	return EOK;
}


static int mfs_open(const char *file, oid_t *oid, oid_t *foid)
{
	int err;
	msg_t msg;

	err = mfs_lookup(file, oid, foid);
	if (err != EOK) {
		return err;
	}

	msg.type = mtOpen;
	msg.i.openclose.oid = *foid;
	msg.i.openclose.flags = 0;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = NULL;
	msg.o.size = 0;

	if ((err = msgSend(oid->port, &msg)) != 0) {
		LOG_ERROR("Cannot send msg.\n");
	}

	if ((err = msg.o.io.err) < 0) {
		LOG_ERROR("Cannot open %s, error: (%s).", file, strerror(err));
	}

	return err;
}


static int mfs_close(oid_t *oid)
{
	int err;
	msg_t msg;

	msg.type = mtClose;
	msg.i.openclose.oid = *oid;
	msg.i.openclose.flags = 0;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = NULL;
	msg.o.size = 0;

	if ((err = msgSend(oid->port, &msg)) != 0) {
		LOG_ERROR("Cannot send msg.\n");
	}

	if ((err = msg.o.io.err) < 0) {
		LOG_ERROR("Cannot close file, error: (%s).", strerror(err));
	}

	return err;
}


static int mfs_fileInfo(oid_t *oid, struct _info *info)
{
	int err;
	msg_t msg;
	meterfs_i_devctl_t *i = (meterfs_i_devctl_t *)msg.i.raw;
	meterfs_o_devctl_t *o = (meterfs_o_devctl_t *)msg.o.raw;

	msg.type = mtDevCtl;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = NULL;
	msg.o.size = 0;
	i->type = meterfs_info;
	i->id = oid->id;

	if ((err = msgSend(oid->port, &msg)) != 0) {
		LOG_ERROR("Cannot send msg.\n");
	}

	if (info != NULL) {
		memcpy(info, &o->info, sizeof(*info));
	}

	if ((err = o->err) < 0) {
		LOG_ERROR("Cannot get file info, error: (%s).", strerror(err));
	}

	return err;
}


static int mfs_resizeFile(oid_t *oid, size_t filesz, size_t recordsz)
{
	int err;
	msg_t msg;
	meterfs_i_devctl_t *i = (meterfs_i_devctl_t *)msg.i.raw;
	meterfs_o_devctl_t *o = (meterfs_o_devctl_t *)msg.o.raw;

	msg.type = mtDevCtl;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = NULL;
	msg.o.size = 0;
	i->type = meterfs_resize;
	i->resize.id = oid->id;
	i->resize.filesz = filesz;
	i->resize.recordsz = recordsz;

	if ((err = msgSend(oid->port, &msg)) != 0) {
		LOG_ERROR("Cannot send msg.\n");
	}

	if ((err = o->err) < 0) {
		LOG_ERROR("Cannot get file info, error: (%s).", strerror(err));
	}

	return 1;
}


static int mfs_partitionErase(oid_t oid)
{
	msg_t msg;
	meterfs_i_devctl_t *i = (meterfs_i_devctl_t *)msg.i.raw;
	meterfs_o_devctl_t *o = (meterfs_o_devctl_t *)msg.o.raw;
	int err;

	msg.type = mtDevCtl;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = NULL;
	msg.o.size = 0;
	i->type = meterfs_chiperase;

	if ((err = msgSend(oid.port, &msg)) != 0) {
		LOG_ERROR("Cannot send msg.\n");
	}

	if ((err = o->err) < 0) {
		LOG_ERROR("Cannot erase partition, error: (%s).", strerror(err));
	}

	return 1;
}


static int mfs_write(oid_t *oid, void *buff, size_t len)
{
	int err;
	msg_t msg;

	msg.type = mtWrite;
	msg.i.io.oid = *oid;
	msg.i.io.offs = 0;
	msg.i.io.len = len;
	msg.i.io.mode = 0;
	msg.i.data = buff;
	msg.i.size = len;
	msg.o.data = NULL;
	msg.o.size = 0;

	if ((err = msgSend(oid->port, &msg)) != 0) {
		LOG_ERROR("Cannot send msg.\n");
	}

	if ((err = msg.o.io.err) < 0) {
		LOG_ERROR("Cannot write data to file, error: (%s).", strerror(err));
	}

	return err;
}


static int mfs_read(oid_t *oid, offs_t offs, void *buff, size_t len)
{
	int err;
	msg_t msg;

	msg.type = mtRead;
	msg.i.io.oid = *oid;
	msg.i.io.offs = offs;
	msg.i.io.len = len;
	msg.i.io.mode = 0;
	msg.i.data = NULL;
	msg.i.size = 0;
	msg.o.data = buff;
	msg.o.size = len;

	if ((err = msgSend(oid->port, &msg)) != 0) {
		LOG_ERROR("Cannot send msg.\n");
	}

	if ((err = msg.o.io.err) < 0) {
		LOG_ERROR("Cannot read data from file, error: (%s).", strerror(err));
	}

	return err;
}


/* Auxiliary functions */

int test_flashsrv_mfsAllocate(void)
{
	oid_t poid, foid;
	struct _info info;
	const char *partitionPath = "/dev/flash0.mfs1";
	const char *file = "file0";

	const uint32_t fileSz = 2000;
	const uint32_t sectorsCnt = 2;
	const uint32_t recordsSz = 20;

	while (lookup(partitionPath, NULL, &poid) < 0) { usleep(100000); }

	if (mfs_partitionErase(poid) < 0) {
		return -1;
	}

	if (mfs_allocate(poid, file, sectorsCnt, fileSz, recordsSz) < 0) {
		return -1;
	}

	if (mfs_open(file, &poid, &foid) < 0) {
		return -1;
	}

	if (mfs_fileInfo(&foid, &info) < 0) {
		return -1;
	}

	if (sectorsCnt != info.sectors || fileSz != info.filesz || recordsSz != info.recordsz || 0 != info.recordcnt) {
		return -1;
	}

	if (mfs_close(&foid) < 0) {
		return -1;
	}

	return 0;
}

#define LOG_FAIL fprintf(stderr, "FAIL: %s:%d\n", __FILE__, __LINE__)


int test_flashsrv_mfsAllocateWriteAndRead(void)
{
	int res = EOK, i;
	oid_t poid, foid;
	const char *partitionPath = "/dev/flash0.mfs1";
	const char *filePath = "file0";

	const uint8_t BUFF_SIZE = 10u;
	const uint8_t RECORD_SIZE = 20u;

	uint8_t writeBuff[BUFF_SIZE];
	uint8_t rcvBuff[RECORD_SIZE];

	while (lookup(partitionPath, NULL, &poid) < 0) {
		usleep(100000);
	}

	/* Prepare partition for test */
	if (mfs_partitionErase(poid) < 0) {
		return -1;
	}

	if (mfs_allocate(poid, "file0", 2, 2000, RECORD_SIZE) < 0) {
		return -1;
	}

	if (mfs_open(filePath, &poid, &foid) < 0) {
		return -1;
	}

	/* Write, read and verify data from the first record */
	memset(writeBuff, 0xa, BUFF_SIZE);

	if (mfs_write(&foid, writeBuff, BUFF_SIZE) < 0) {
		return -1;
	}

	if (mfs_read(&foid, 0, rcvBuff, RECORD_SIZE) < 0) {
		return -1;
	}

	if (memcmp(rcvBuff, writeBuff, BUFF_SIZE) != 0) {
		return -1;
	}

	for (i = BUFF_SIZE; i < RECORD_SIZE; ++i) {
		if (rcvBuff[i] != (uint8_t)0xff) {
			LOG_FAIL;
			printf("rcvBuff[%d] = %d\n", i, rcvBuff[i]);
			return -1;
		}
	}

	/* Write, read and verify data from the second record */
	memset(writeBuff, 0xef, BUFF_SIZE);

	if (mfs_write(&foid, writeBuff, BUFF_SIZE) < 0) {
		return -1;
	}

	if (mfs_read(&foid, RECORD_SIZE, rcvBuff, RECORD_SIZE) < 0) {
		return -1;
	}

	if (memcmp(rcvBuff, writeBuff, BUFF_SIZE) != 0) {
		return -1;
	}

	for (i = BUFF_SIZE; i < RECORD_SIZE; ++i) {
		if (rcvBuff[i] != (uint8_t)0xff) {
			LOG_FAIL;
			printf("rcvBuff[%d] = %d\n", i, rcvBuff[i]);
			return -1;
		}
	}

	if (mfs_close(&foid) < 0) {
		return -1;
	}

	return res;
}


int test_flashsrv_mfsFileResize(void)
{
	oid_t poid, foid;
	struct _info info;
	const char *partitionPath = "/dev/flash0.mfs1";
	const char *filePath = "file0";

	const uint32_t fileSz = 1000u;
	const uint32_t sectorsCnt = 2u;
	const uint32_t recordsSz = 100u;

	while (lookup(partitionPath, NULL, &poid) < 0) {
		usleep(100000);
	}

	/* Prepare partition for test */
	if (mfs_partitionErase(poid) < 0) {
		return -1;
	}

	if (mfs_allocate(poid, "file0", 2, 2000, 20) < 0) {
		return -1;
	}

	if (mfs_open(filePath, &poid, &foid) < 0) {
		return -1;
	}

	if (mfs_resizeFile(&foid, 1000, 100) < 0) {
		return -1;
	}

	if (mfs_fileInfo(&foid, &info) < 0) {
		return -1;
	}

	if (sectorsCnt != info.sectors || fileSz != info.filesz || recordsSz != info.recordsz) {
		return -1;
	}

	if (mfs_close(&foid) < 0) {
		return -1;
	}

	return EOK;
}
