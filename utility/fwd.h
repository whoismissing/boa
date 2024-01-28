/*
 * fwd general definition header file
 * Author: rui_cai <rui_cai@realsil.com.cn>
 * Copyright (c) Realtek Ltd.Co
 */
#ifndef __FWD_H__
#define __FWD_H__

/* i/o files */
#define FWD_CONFFILE			"/var/fwd.conf"		//fwd configuration file
#define FWD_READYFILE			"/var/fwd.ready"	//fwd trigger
#define FWD_STATFILE_PREFIX		"/var/fwd_status_"	//append with callee's PID
#define FWD_LOCKFILE			"/var/fwd.lock"		//fwd proceed lock file

/* keys */
//fwd.conf
#define FWD_CONF_FROM			"from"
#define FWD_CONF_FILENAME		"filename"
#define FWD_CONF_SHMID			"shmid"
#define FWD_CONF_REBOOT			"reboot_delay"
#define FWD_CONF_TYPE			"type"
#define FWD_CONF_TARGET			"target"
#define FWD_CONF_BURN_ADDR		"burn_address"
#define FWD_CONF_BURN_SIZE		"burn_size"
#define FWD_CONF_DATA_OFFSET	"data_offset"
//fwd.ready
#define FWD_READY_PID			"pid"
//result
#define FWD_STAT_STATUS			"status"
#define FWD_STAT_MSG			"msg"

/* fwd behaviour control flag */
#define BIT(X)					(1<<(X))
#define IS_BIT_SET(X, bit)		(((X)&(bit))?1:0)
#define SET_BIT(X, bit)			((X)|=(bit))
#define CLR_BIT(X, bit)			((X)&=(~(bit)))

#define FWD_DATA_SOURCE			(BIT(0))		// if set, fwd data is in file, otherwise, in shm.
#define FWD_REBOOT_DELAY		(BIT(1))		// if set, fwd will delay reboot after succeed, otherwise, watchdog reboot!
#define FWD_SUPPORT_OFFSET_CHANGE (BIT(2))		// if set, fwd support flash layout change in new firmware.
#define FWD_FORCE_BURN			(BIT(3))		// if set, fwd will burn correct section to flash, even if there's invalid section in upgrade data.

#define FWD_MAX_LINE_SIZE		(128)
#define FWD_MAX_TARGET_NAMESIZE	(64)


#define MACIE5_CFGSTR	"/plain\x0d\x0a\0x0d\0x0a"
#define WINIE6_STR	"/octet-stream\x0d\x0a\0x0d\0x0a"
#define MACIE5_FWSTR	"/macbinary\x0d\x0a\0x0d\0x0a"
#define OPERA_FWSTR	"/x-macbinary\x0d\x0a\0x0d\0x0a"
#define LINE_FWSTR	"\x0d\x0a\0x0d\0x0a"
#define LINUXFX36_FWSTR "/x-ns-proxy-autoconfig\x0d\x0a\0x0d\0x0a"
#define CWMP_DL_HTTP_PREFIX		"cwmp http download filelen="		//keep it same as cwmp-tr069_v2

#define PLUGIN_INSTALL_DIRNAME	"installed"		//DIR Fullpath: $PLUGIN_INST_DEF_PATH/$PLUGIN_INST_INFO_DIRNAME

enum FWD_RESULT
{
	FWD_FAILED=0,
	FWD_SUCCEED=1
};
enum FWD_DATA_SRC
{
	FWD_DATA_IN_SHM=0,
	FWD_DATA_IN_FILE
};

enum FWD_DATA_TYPE
{
	FWD_DATA_TYPE_BOOT=0,
	FWD_DATA_TYPE_WEB,
	FWD_DATA_TYPE_LINUX,
	FWD_DATA_TYPE_ROOTFS,
	FWD_DATA_TYPE_HS,
	FWD_DATA_TYPE_DS,
	FWD_DATA_TYPE_CS,
	FWD_DATA_TYPE_PLUGIN,
	FWD_DATA_TYPE_SIZE
};

enum FWD_DATA_PRIO
{
	FWD_PRIO_IG = 0,	/*lowest priority, ignore*/
	FWD_PRIO_BT,		/*Bootcode*/
	FWD_PRIO_RO,		/*Rootfs*/
	FWD_PRIO_KE,		/*Kernel*/
	FWD_PRIO_WB,		/*Web*/
	FWD_PRIO_CO,		/*CS/DS*/
	FWD_PRIO_HW,		/*HS*/
	FWD_PRIO_PL,		/*Plugin, require tar command, must upgrade before rootfs*/
};

enum FWD_STATUS
{
	FWD_INIT=0,
	FWD_PARSING,
	FWD_BURNING,
	FWD_DONE,
	FWD_ERROR
};

struct fwd_conf_payload
{
	int type;
	unsigned char target[FWD_MAX_TARGET_NAMESIZE];
	unsigned int burn_address;
	unsigned int burn_size;
	unsigned int data_offset;
	enum FWD_DATA_PRIO prio;
};

struct fwd_conf
{
	//Raw Data Source
	int from;
	unsigned char filename[FWD_MAX_LINE_SIZE];
	int shmid;

	//Behavior after flash write
	int reboot_delay;

	//Raw Data Analysis
	int section_num;
	struct fwd_conf_payload *payload;
};

//#define FWD_DBG 1
#ifdef FWD_DBG
#define BDBG_BSN(format, arg...) printf(format , ## arg)
#else
#define BDBG_BSN(format, arg...) do{}while(0)
#endif

//--------------------------------------------------------------------
#if CHECK_BURN_SERIAL
/* Firmware image file header */
typedef struct img_header {
	unsigned char signature[SIG_LEN];
	unsigned int startAddr;
	unsigned int burnAddr;
	unsigned int len;
}__attribute__ ((packed)) IMG_HEADER_T, *IMG_HEADER_Tp;

struct squashfs_super_block_partial {
	unsigned int		s_magic;
	unsigned int		inodes;
	unsigned int		mkfs_time /* time of filesystem creation */;
	unsigned int		block_size;
#if 0
	unsigned int		fragments;
	unsigned short		compression;
	unsigned short		block_log;
	unsigned short		flags;
	unsigned short		no_ids;
	unsigned short		s_major;
	unsigned short		s_minor;
	long long		root_inode;//squashfs_inode_t	root_inode;
	long long		bytes_used;
	long long		id_table_start;
	long long		xattr_table_start;
	long long		inode_table_start;
	long long		directory_table_start;
	long long		fragment_table_start;
	long long		lookup_table_start;
#endif
}__attribute__ ((packed));

struct _rootfs_padding {
	unsigned char  zero_pad[2];
	unsigned short chksum;
	unsigned char  signature[SIG_LEN];
	unsigned long  len;
}__attribute__ ((packed));

unsigned long gen_burn_serial()
{
	unsigned long burn_serial = 0;

	/* generate burn_serial */
	srand(time(NULL));
	burn_serial=rand() | (1<<31);

	return burn_serial;
}
#endif /* #if CHECK_BURN_SERIAL */
//--------------------------------------------------------------------

#endif /* __FWD_H__ */
