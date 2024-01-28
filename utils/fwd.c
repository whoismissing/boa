#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>  // MSE compiler shutup
#include <dirent.h>
//keith_fwd
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
//keith_fwd
#include <unistd.h>
#ifdef KERNEL_3_10
#include <linux/fs.h>
#endif

#ifdef CONFIG_RTL_FAST_CHECKSUM_ENABLE
#include "common_utility.h"
#include "fastcksum.h"
#endif

#define FW_SIGNATURE			((char *)"cs6c")	// fw signature
#define FW_SIGNATURE_WITH_ROOT	((char *)"cr6c")	// fw signature with root fs
#define ROOT_SIGNATURE			((char *)"r6cr")
#define SQSH_SIGNATURE			((char *)"sqsh")
#define SQSH_SIGNATURE_LE       ((char *)"hsqs")
#define SIG_LEN					4
#define SQUASHFS_MAGIC			0x73717368
#define SQUASHFS_MAGIC_SWAP 	0x68737173
#define SIZE_OF_SQFS_SUPER_BLOCK 640
#define SIZE_OF_CHECKSUM 		2
#define OFFSET_OF_LEN 			2

#ifdef CONFIG_BOOT_FAIL_CHECK
/*
 * Dummy Register Definitions
 */
#ifdef CONFIG_RTL_8197F
#define BSP_DUMMY_REG_BASE				0xb8000f00
#define BSP_DUMMP_REG_POST_ENABLE		(BSP_DUMMY_REG_BASE + 0x0)		//reserved for CONFIG_POST_ENABLE, used in POSTRW_API().
#define BSP_DUMMY_REG_BANK1_BOOTSTATUS	(BSP_DUMMY_REG_BASE + 0x4)		//reserved for bootup failure check
#define BSP_DUMMY_REG_BANK2_BOOTSTATUS	(BSP_DUMMY_REG_BASE + 0x8)		//reserved for bootup failure check
#define BSP_DUMMY_REG_END				(BSP_DUMMY_REG_BASE + 0x27)		//included
#define BOOTFAIL_CHECK_BIT_ISBOOTING	(1<<31)	//0x80000000
#else
#error "Boot fail check is not supported in this platform. Please disable CONFIG_BOOT_FAIL_CHECK!!!"
#endif
#endif/*CONFIG_BOOT_FAIL_CHECK*/


#ifdef CONFIG_FWD_UNIFIED_FRAMEWORK
#include <sys/mman.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

#include "./../utility/fwd.h"

//#define CHECK_BURN_SERIAL 1

typedef struct __fwd_ops {
	int sectype;
	int (*prepare)(struct fwd_conf_payload *payload, unsigned char *shm, unsigned char *msg);
	int (*write)(struct fwd_conf_payload *payload, unsigned char *shm, unsigned char *msg);
	int (*validate)(struct fwd_conf_payload *payload, unsigned char *shm, unsigned char *msg);
}FWD_OP_T, *FWD_OP_Tp;

//declaration
static int __linux_prepare(struct fwd_conf_payload *payload, unsigned char *shm, unsigned char *msg);
static int __linux_validate(struct fwd_conf_payload *payload, unsigned char *shm, unsigned char *msg);
static int __root_prepare(struct fwd_conf_payload *payload, unsigned char *shm, unsigned char *msg);
static int __root_validate(struct fwd_conf_payload *payload, unsigned char *shm, unsigned char *msg);
static int __fwd(struct fwd_conf_payload *payload, unsigned char *shm_memory, unsigned char *msg);
#ifdef CONFIG_FWD_UNIFIED_SUPPORT_PLUGIN
static int __fwd_plugin(struct fwd_conf_payload *payload, unsigned char *shm_memory, unsigned char *msg);
#endif

FWD_OP_T FWD_OP_TABLE[]=
{
/*  {sectype, 			 	prepare,		write,		validate}  */
	{FWD_DATA_TYPE_BOOT, 	NULL, 			__fwd, 		NULL},
	{FWD_DATA_TYPE_WEB,		NULL,			__fwd,		NULL},
	{FWD_DATA_TYPE_LINUX,	__linux_prepare,__fwd,		__linux_validate},
	{FWD_DATA_TYPE_ROOTFS,	__root_prepare,	__fwd,		__root_validate},
	{FWD_DATA_TYPE_DS,		NULL,			__fwd,		NULL},
	{FWD_DATA_TYPE_CS,		NULL,			__fwd,		NULL},
#ifdef CONFIG_FWD_UNIFIED_SUPPORT_PLUGIN
	{FWD_DATA_TYPE_PLUGIN,	NULL,			__fwd_plugin,		NULL},
#endif
	{-1,					NULL,			NULL,		NULL}//keep it the last (sectype=-1)
};

//implementation
#ifdef CONFIG_RTL_FAST_CHECKSUM_ENABLE
static int alloc_and_read_fchdr(FASTCKSUM_HEADER_Tp *ppfc_hdr, int *size, unsigned char *msg)
{
	int ret = FWD_FAILED;
	int total_bank=0;

	if(ppfc_hdr==NULL || size==NULL || msg==NULL)
	{
		goto RETURN;
	}

#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	total_bank = 2;
#else
	total_bank = 1;
#endif
	*ppfc_hdr = NULL;

	*size = sizeof(FASTCKSUM_HEADER_T) + total_bank*sizeof(FASTCKSUM_PAYLOAD_T);
	*ppfc_hdr = (FASTCKSUM_HEADER_Tp)malloc( *size );
	if(NULL==*ppfc_hdr)
	{
		sprintf(msg, "cannot malloc for fastcksum_hdr, abandon upgrade!!!\n");
		goto RETURN;
	}
	memset(*ppfc_hdr, 0x0, *size);

	//read from flash mtd to structure
	if(FASTCKSUM_FAILED==read_fastcksum_from_flash((unsigned char *)*ppfc_hdr, *size))
	{
		sprintf(msg, "cannot read flash data for fastcksum_hdr, abandon upgrade!!!\n");
		goto RETURN;
	}
	
	dump_fastcksum_struct(*ppfc_hdr);
	ret = FWD_SUCCEED;

RETURN:
	if(ret==FWD_FAILED && ppfc_hdr && *ppfc_hdr!=NULL)
	{
		free(*ppfc_hdr);
		*ppfc_hdr = NULL;
	}
	return ret;
}

static int free_fchdr(FASTCKSUM_HEADER_Tp *ppfc_hdr)
{
	if(ppfc_hdr && *ppfc_hdr)
		free(*ppfc_hdr);
	if(ppfc_hdr)
		*ppfc_hdr = NULL;

	return FWD_SUCCEED;
}
#endif

#ifdef CONFIG_BOOT_FAIL_CHECK
static void reset_target_bank_bootstatus_register(struct fwd_conf_payload *payload)
{
	int target_bank = 0;
	char buf[64]={0};
	
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	if( payload->burn_address >= (CONFIG_RTL_FLASH_DUAL_IMAGE_OFFSET) )
	{
		//will write to 2nd bank, move forward fastcksum_body ptr.
		target_bank = 2;
	}else
#endif
		target_bank = 1;

	if(target_bank==1)
		sprintf(buf, "ew 0x%x 0", BSP_DUMMY_REG_BANK1_BOOTSTATUS);
	else if(target_bank==2)
		sprintf(buf, "ew 0x%x 0", BSP_DUMMY_REG_BANK2_BOOTSTATUS);
	else{
		printf("[%s:%d] invalid bank value %d.\n", __FUNCTION__, __LINE__, target_bank);
		return ;
	}

	printf("clear target bank boot status register, bank:%d, cmd:%s\n", target_bank, buf);
	system(buf);

	return;
}
#endif

static int __linux_prepare(struct fwd_conf_payload *payload, unsigned char *shm, unsigned char *msg)
{
	int ret = FWD_SUCCEED;

	if(payload==NULL || shm==NULL || msg==NULL)
	{
		ret = FWD_FAILED;
		goto RETURN;
	}

#ifdef CONFIG_RTL_FAST_CHECKSUM_ENABLE
	//clear fast checksum linux VALID field
	FASTCKSUM_HEADER_Tp fastcksum_hdr = NULL, *ppfc_hdr = &fastcksum_hdr;
	FASTCKSUM_PAYLOAD_Tp fastcksum_body = NULL;
	int size = 0;

	//init structure
	if(FWD_FAILED==alloc_and_read_fchdr(ppfc_hdr, &size, msg))
	{
		ret = FWD_FAILED;
		goto RETURN;
	}
	fastcksum_body = (FASTCKSUM_PAYLOAD_Tp)fastcksum_hdr->payload;
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	if( payload->burn_address >= (CONFIG_RTL_FLASH_DUAL_IMAGE_OFFSET) )
	{
		//will write to 2nd bank, move forward fastcksum_body ptr.
		fastcksum_body++;
	}
#endif

	//maybe should init fields
	if(fastcksum_hdr->len != (size-sizeof(FASTCKSUM_HEADER_T)))
	{
		BDBG_BSN("WARNING: fastcksum_hdr->len is updated!!! Old:%d, new:%d, sizeof(FASTCKSUM_PAYLOAD_T):%d\n", fastcksum_hdr->len, size-sizeof(FASTCKSUM_HEADER_T), sizeof(FASTCKSUM_PAYLOAD_T));
		fastcksum_hdr->len = size-sizeof(FASTCKSUM_HEADER_T);
	}
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	if( payload->burn_address >= (CONFIG_RTL_FLASH_DUAL_IMAGE_OFFSET) )
	{
		fastcksum_body->bank = 2;
	}else
#endif
	{
		fastcksum_body->bank = 1;
	}

	//clear linux VALID bits, write fast cksum structure to flash
	{
		BDBG_BSN("[%s:%d] clear linux VALID field of bank %d!\n", __FUNCTION__, __LINE__, fastcksum_body->bank);
		fastcksum_body->reserved &= (~RTL_FASTCKSUM_FIELD_LINUX_VALID);
		dump_fastcksum_struct(fastcksum_hdr);
		if(FASTCKSUM_FAILED==write_fastcksum_to_flash((unsigned char *)fastcksum_hdr, size))
		{
			sprintf(msg, "[%s:%d] write_fastcksum_to_flash() failed, abandon upgrade!!!\n", __FUNCTION__, __LINE__);
			ret = FWD_FAILED;
			goto RETURN;
		}
	}
#endif/*CONFIG_RTL_FAST_CHECKSUM_ENABLE*/

RETURN:
#ifdef CONFIG_RTL_FAST_CHECKSUM_ENABLE
	//house keeping
	free_fchdr(ppfc_hdr);
#endif/*CONFIG_RTL_FAST_CHECKSUM_ENABLE*/

	return ret;
}

static int __linux_validate(struct fwd_conf_payload *payload, unsigned char *shm, unsigned char *msg)
{
	int ret = FWD_SUCCEED;

	if(payload==NULL || shm==NULL || msg==NULL)
	{
		ret = FWD_FAILED;
		goto RETURN;
	}

#ifdef CONFIG_RTL_FAST_CHECKSUM_ENABLE
	//clear fast checksum linux VALID field
	FASTCKSUM_HEADER_Tp fastcksum_hdr = NULL, *ppfc_hdr = &fastcksum_hdr;
	FASTCKSUM_PAYLOAD_Tp fastcksum_body = NULL;
	int size = 0;

	//init structure
	if(FWD_FAILED==alloc_and_read_fchdr(ppfc_hdr, &size, msg))
	{
		ret = FWD_FAILED;
		goto RETURN;
	}
	fastcksum_body = (FASTCKSUM_PAYLOAD_Tp)fastcksum_hdr->payload;
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	if( payload->burn_address >= (CONFIG_RTL_FLASH_DUAL_IMAGE_OFFSET) )
	{
		//will write to 2nd bank, move forward fastcksum_body ptr.
		fastcksum_body++;
	}
#endif

	//maintain fast cksum structure
	{
		BDBG_BSN("[%s:%d] validate linux.bin...\n", __FUNCTION__, __LINE__);
		IMG_HEADER_Tp img_hdr = (IMG_HEADER_Tp)&shm[payload->data_offset];
		int len = img_hdr->len, cksum_offset=0, valid = 0;
		unsigned int new_checksum = 0;
		short old_checksum = 0;

		//linux_sig
		memcpy(fastcksum_body->linux_sig, img_hdr->signature, SIG_LEN);
		BDBG_BSN("[%s:%d]linux sig:%02x%02x%02x%02x\n", __FUNCTION__, __LINE__, fastcksum_body->linux_sig[0], fastcksum_body->linux_sig[1], 
			fastcksum_body->linux_sig[2], fastcksum_body->linux_sig[3]);
		
		//linux_cksum
		cksum_offset = DWORD_SWAP(len) + sizeof(IMG_HEADER_T) - SIZE_OF_CHECKSUM;
		memcpy(&old_checksum, &shm[payload->data_offset+cksum_offset], SIZE_OF_CHECKSUM);
		BDBG_BSN("[%s:%d]cksum_offset:0x%x, old_checksum:%x\n", __FUNCTION__, __LINE__, cksum_offset, old_checksum);
		if(FASTCKSUM_FAILED==gen_fastcksum(&new_checksum, len, old_checksum))
		{
			sprintf(msg, "[%s:%d]Generate fastchecksum for rootfs failed!\n", __FUNCTION__, __LINE__);
			ret = FWD_FAILED;
			goto RETURN;
		}
		fastcksum_body->linux_cksum = new_checksum;
		BDBG_BSN("[%s:%d]linux fast checksum:%x\n", __FUNCTION__, __LINE__, fastcksum_body->linux_cksum);

		//re-valid flash content
		if(revalidation(payload->target, payload->burn_address+sizeof(IMG_HEADER_T), payload->burn_size-sizeof(IMG_HEADER_T), &valid)==FASTCKSUM_SUCCESS)
		{
			if(valid)
				fastcksum_body->reserved |= RTL_FASTCKSUM_FIELD_LINUX_VALID;
			else
				fastcksum_body->reserved &= (~RTL_FASTCKSUM_FIELD_LINUX_VALID);
			BDBG_BSN("[%s:%d] reserved:0x%x\n", __FUNCTION__, __LINE__, fastcksum_body->reserved);
		}else{
			fastcksum_body->reserved &= (~RTL_FASTCKSUM_FIELD_LINUX_VALID);
			BDBG_BSN("[%s:%d] revalidation failed, clear valid bits!\n", __FUNCTION__, __LINE__);
		}
	}

	//write fast cksum structure to flash
	dump_fastcksum_struct(fastcksum_hdr);
	if(FASTCKSUM_FAILED==write_fastcksum_to_flash((unsigned char *)fastcksum_hdr, size))
	{
		sprintf(msg, "[%s:%d] write_fastcksum_to_flash() failed, abandon upgrade!!!\n", __FUNCTION__, __LINE__);
		ret = FWD_FAILED;
		goto RETURN;
	}
#endif/*CONFIG_RTL_FAST_CHECKSUM_ENABLE*/

#ifdef CONFIG_BOOT_FAIL_CHECK
	reset_target_bank_bootstatus_register(payload);
#endif/*CONFIG_BOOT_FAIL_CHECK*/

RETURN:
#ifdef CONFIG_RTL_FAST_CHECKSUM_ENABLE
	//house keeping
	free_fchdr(ppfc_hdr);
#endif/*CONFIG_RTL_FAST_CHECKSUM_ENABLE*/

	return ret;
}

static int __root_prepare(struct fwd_conf_payload *payload, unsigned char *shm, unsigned char *msg)
{
	int ret = FWD_SUCCEED;
	
	if(payload==NULL || shm==NULL || msg==NULL)
	{
		ret = FWD_FAILED;
		goto RETURN;
	}
	
#ifdef CONFIG_RTL_FAST_CHECKSUM_ENABLE
	//clear fast checksum linux VALID field
	FASTCKSUM_HEADER_Tp fastcksum_hdr = NULL, *ppfc_hdr = &fastcksum_hdr;
	FASTCKSUM_PAYLOAD_Tp fastcksum_body = NULL;
	int size = 0;

	//init structure
	if(FWD_FAILED==alloc_and_read_fchdr(ppfc_hdr, &size, msg))
	{
		ret = FWD_FAILED;
		goto RETURN;
	}
	fastcksum_body = (FASTCKSUM_PAYLOAD_Tp)fastcksum_hdr->payload;
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	if( payload->burn_address >= (CONFIG_RTL_FLASH_DUAL_IMAGE_OFFSET) )
	{
		//will write to 2nd bank, move forward fastcksum_body ptr.
		fastcksum_body++;
	}
#endif

	//maybe should init fields
	if(fastcksum_hdr->len != (size-sizeof(FASTCKSUM_HEADER_T)))
	{
		BDBG_BSN("WARNING: fastcksum_hdr->len is updated!!! Old:%d, new:%d, sizeof(FASTCKSUM_PAYLOAD_T):%d\n", fastcksum_hdr->len, size-sizeof(FASTCKSUM_HEADER_T), sizeof(FASTCKSUM_PAYLOAD_T));
		fastcksum_hdr->len = size-sizeof(FASTCKSUM_HEADER_T);
	}
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	if( payload->burn_address >= (CONFIG_RTL_FLASH_DUAL_IMAGE_OFFSET) )
	{
		fastcksum_body->bank = 2;
	}else
#endif
	{
		fastcksum_body->bank = 1;
	}

	//clear root VALID bits, write fast cksum structure to flash
	{
		BDBG_BSN("[%s:%d] clear root VALID field of bank %d!\n", __FUNCTION__, __LINE__, fastcksum_body->bank);
		fastcksum_body->reserved &= (~RTL_FASTCKSUM_FIELD_ROOT_VALID);
		dump_fastcksum_struct(fastcksum_hdr);
		if(FASTCKSUM_FAILED==write_fastcksum_to_flash((unsigned char *)fastcksum_hdr, size))
		{
			sprintf(msg, "[%s:%d] write_fastcksum_to_flash() failed, abandon upgrade!!!\n", __FUNCTION__, __LINE__);
			ret = FWD_FAILED;
			goto RETURN;
		}
	}
#endif/*CONFIG_RTL_FAST_CHECKSUM_ENABLE*/

RETURN:
#ifdef CONFIG_RTL_FAST_CHECKSUM_ENABLE
	//house keeping
	free_fchdr(ppfc_hdr);
#endif/*CONFIG_RTL_FAST_CHECKSUM_ENABLE*/

	return ret;
}

static int __root_validate(struct fwd_conf_payload *payload, unsigned char *shm, unsigned char *msg)
{
	int ret = FWD_SUCCEED;
	
	if(payload==NULL || shm==NULL || msg==NULL)
	{
		ret = FWD_FAILED;
		goto RETURN;
	}
	
#ifdef CONFIG_RTL_FAST_CHECKSUM_ENABLE
	//clear fast checksum linux VALID field
	FASTCKSUM_HEADER_Tp fastcksum_hdr = NULL, *ppfc_hdr = &fastcksum_hdr;
	FASTCKSUM_PAYLOAD_Tp fastcksum_body = NULL;
	int size = 0;

	//init structure
	if(FWD_FAILED==alloc_and_read_fchdr(ppfc_hdr, &size, msg))
	{
		ret = FWD_FAILED;
		goto RETURN;
	}
	fastcksum_body = (FASTCKSUM_PAYLOAD_Tp)fastcksum_hdr->payload;
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	if( payload->burn_address >= (CONFIG_RTL_FLASH_DUAL_IMAGE_OFFSET) )
	{
		//will write to 2nd bank, move forward fastcksum_body ptr.
		fastcksum_body++;
	}
#endif

	//maintain fast cksum structure
	{
		FCS_DBG("[%s:%d] validate root.bin...\n", __FUNCTION__, __LINE__);
		struct SQUASH_HDR_T {
			unsigned char sig[SIG_LEN];
			unsigned int reserved1;
			unsigned int len;
			unsigned int reserved2;
		} *squash_hdr = (struct SQUASH_HDR_T *)&shm[payload->data_offset];
		int len = squash_hdr->len, cksum_offset = 0, valid = 0;
		unsigned int new_checksum = 0;
		short old_checksum = 0;

		//root_sig
		memcpy(fastcksum_body->root_sig, squash_hdr->sig, SIG_LEN);
		FCS_DBG("[%s:%d]root sig:%02x%02x%02x%02x\n", __FUNCTION__, __LINE__, fastcksum_body->root_sig[0], fastcksum_body->root_sig[1], 
			fastcksum_body->root_sig[2], fastcksum_body->root_sig[3]);
		
		//root_cksum
		cksum_offset = DWORD_SWAP(len) + SIZE_OF_SQFS_SUPER_BLOCK;
		memcpy(&old_checksum, &shm[payload->data_offset+cksum_offset], SIZE_OF_CHECKSUM);
		FCS_DBG("[%s:%d]cksum_offset:0x%x, old_checksum:%x\n", __FUNCTION__, __LINE__, cksum_offset, old_checksum);
		if(FASTCKSUM_FAILED==gen_fastcksum(&new_checksum, len, old_checksum))
		{
			sprintf(msg, "[%s:%d]Generate fastchecksum for rootfs failed!\n", __FUNCTION__, __LINE__);
			ret = FWD_FAILED;
			goto RETURN;
		}
		fastcksum_body->root_cksum = new_checksum;
		FCS_DBG("[%s:%d]root fast checksum:%x\n", __FUNCTION__, __LINE__, fastcksum_body->root_cksum);
		
		//re-valid
		if(revalidation(payload->target, payload->burn_address, payload->burn_size, &valid)==FASTCKSUM_SUCCESS)
		{
			if(valid)
				fastcksum_body->reserved |= RTL_FASTCKSUM_FIELD_ROOT_VALID;
			else
				fastcksum_body->reserved &= (~RTL_FASTCKSUM_FIELD_ROOT_VALID);
			FCS_DBG("[%s:%d] reserved:0x%x\n", __FUNCTION__, __LINE__, fastcksum_body->reserved);
		}else{
			fastcksum_body->reserved &= (~RTL_FASTCKSUM_FIELD_ROOT_VALID);
			FCS_DBG("[%s:%d] revalidation failed, clear valid bits!\n", __FUNCTION__, __LINE__);
		}
	}

	//write fast cksum structure to flash
	dump_fastcksum_struct(fastcksum_hdr);
	if(FASTCKSUM_FAILED==write_fastcksum_to_flash((unsigned char *)fastcksum_hdr, size))
	{
		sprintf(msg, "[%s:%d] write_fastcksum_to_flash() failed, abandon upgrade!!!\n", __FUNCTION__, __LINE__);
		ret = FWD_FAILED;
		goto RETURN;
	}
#endif

RETURN:
#ifdef CONFIG_RTL_FAST_CHECKSUM_ENABLE
	//house keeping
	free_fchdr(ppfc_hdr);
#endif/*CONFIG_RTL_FAST_CHECKSUM_ENABLE*/

	return ret;
}

static void clear_status_file(int pid)
{
	unsigned char cmd[FWD_MAX_TARGET_NAMESIZE+4]={0};
	sprintf(cmd, "rm %s%d", FWD_STATFILE_PREFIX, pid);
	system(cmd);
	
	return FWD_SUCCEED;
}

static enum FWD_DATA_PRIO get_fwd_prio(int type)
{
	switch(type)
	{
		case FWD_DATA_TYPE_BOOT:
			return FWD_PRIO_BT;
			
		case FWD_DATA_TYPE_WEB:
			return FWD_PRIO_WB;
			
		case FWD_DATA_TYPE_LINUX:
			return FWD_PRIO_KE;
			
		case FWD_DATA_TYPE_ROOTFS:
			return FWD_PRIO_RO;
			
		case FWD_DATA_TYPE_HS:
			return FWD_PRIO_HW;
			
		case FWD_DATA_TYPE_DS:
		case FWD_DATA_TYPE_CS:
			return FWD_PRIO_CO;
		
		case FWD_DATA_TYPE_PLUGIN:
			return FWD_PRIO_PL;
	}

	return FWD_PRIO_IG;
}

static int get_fwd_conf(struct fwd_conf *conf)
{
	unsigned char line[160] = {0};
	int section_num = 0;
	FILE *fp = NULL;
	conf->payload = NULL;
	
	if(conf==NULL)
	{
		BDBG_BSN("[%s:%d] conf is NULL!\n", __FUNCTION__, __LINE__);
		return FWD_FAILED;
	}

	BDBG_BSN("open %s\n", FWD_CONFFILE);
	if((fp=fopen(FWD_CONFFILE, "r"))==NULL)
	{
		BDBG_BSN("[%s:%d] fopen %s error!\n", __FUNCTION__, __LINE__, FWD_CONFFILE);
		return FWD_FAILED;
	}
	
	BDBG_BSN("parse %s\n", FWD_CONFFILE);
	while(fgets(line, (sizeof(line)-1), fp)>0)
	{
		if(strlen(line)<=0)
			continue;
		
		if( memcmp(line, FWD_CONF_FROM, strlen(FWD_CONF_FROM))==0 )
		{
			sscanf(line, "%*s :%d", &conf->from);
			BDBG_BSN("from:%d\n", conf->from);
		}else if( memcmp(line, FWD_CONF_FILENAME, strlen(FWD_CONF_FILENAME))==0 )
		{
			sscanf(line, "%*s :%s", conf->filename);
			BDBG_BSN("filename:%s\n", conf->filename);
		}else if( memcmp(line, FWD_CONF_SHMID, strlen(FWD_CONF_SHMID))==0 )
		{
			sscanf(line, "%*s :%d", &conf->shmid);
			BDBG_BSN("shmid:%d\n", conf->shmid);
		}else if( memcmp(line, FWD_CONF_REBOOT, strlen(FWD_CONF_REBOOT))==0 )
		{
			sscanf(line, "%*s :%d", &conf->reboot_delay);
			BDBG_BSN("reboot_delay:%d\n", conf->reboot_delay);
		}else
		{
			/* payload */
			struct fwd_conf_payload *ptr = (struct fwd_conf_payload *)conf->payload + section_num;
			if( memcmp(line, FWD_CONF_TYPE, strlen(FWD_CONF_TYPE))==0 )
			{
				// the start of new 'Raw Data Analysis' section
				conf->payload = (struct fwd_conf_payload *)realloc(conf->payload, (1+section_num)*sizeof(struct fwd_conf_payload));
				// update ptr
				ptr = (struct fwd_conf_payload *)conf->payload + section_num;
				sscanf(line, "%*s :%d", &ptr->type);
				ptr->prio = get_fwd_prio(ptr->type);
				BDBG_BSN("[section %d] ptr:%p\n", section_num, ptr);
				BDBG_BSN("[section %d] type:%d(%p)\n", section_num, ptr->type, ptr);
				BDBG_BSN("[section %d] prio:%d\n", section_num, ptr->prio);
			}else if( memcmp(line, FWD_CONF_TARGET, strlen(FWD_CONF_TARGET))==0 && ptr!=NULL)
			{
				sscanf(line, "%*s :%s", ptr->target);
				BDBG_BSN("[section %d] target:%s(%p)\n", section_num, ptr->target, ptr);
			}else if( memcmp(line, FWD_CONF_BURN_ADDR, strlen(FWD_CONF_BURN_ADDR))==0 && ptr!=NULL)
			{
				sscanf(line, "%*s :%d", &ptr->burn_address);
				BDBG_BSN("[section %d] burn_address:0x%x(%p)\n", section_num, ptr->burn_address, ptr);
			}else if( memcmp(line, FWD_CONF_BURN_SIZE, strlen(FWD_CONF_BURN_SIZE))==0 && ptr!=NULL)
			{
				sscanf(line, "%*s :%d", &ptr->burn_size);
				BDBG_BSN("[section %d] burn_size:%d(%p)\n", section_num, ptr->burn_size, ptr);
			}else if( memcmp(line, FWD_CONF_DATA_OFFSET, strlen(FWD_CONF_DATA_OFFSET))==0 && ptr!=NULL)
			{
				sscanf(line, "%*s :%d", &ptr->data_offset);
				BDBG_BSN("[section %d] data_offset:0x%x(%p)\n", section_num, ptr->data_offset, ptr);
				// the end of this'Raw Data Analysis' section
				section_num++;
			}
			
			conf->section_num = section_num;
		}
		
		memset(line, 0x0, sizeof(line));
		continue;
	}
	
	BDBG_BSN("close %s\n", FWD_CONFFILE);
	fclose(fp);

	return FWD_SUCCEED;
}

static int clear_fwd_conf(struct fwd_conf *conf, int pid)
{
	if(conf && conf->payload)
	{
		free(conf->payload);
		conf->payload = NULL;
		memset(conf, 0x0, sizeof(conf));
	}
	
	unsigned char cmd[FWD_MAX_TARGET_NAMESIZE+4]={0};
	sprintf(cmd, "rm %s", FWD_CONFFILE);
	system(cmd);
	
	return FWD_SUCCEED;
}

static int get_fwd_ready(int *pid)
{
	unsigned char line[16] = {0};
	FILE *fp = NULL;
	
	if( pid==NULL )
	{
		BDBG_BSN("[%s:%d] pid is NULL!\n", __FUNCTION__, __LINE__);
		return FWD_FAILED;
	}

	BDBG_BSN("open %s\n", FWD_READYFILE);
	if( (fp=fopen(FWD_READYFILE, "r"))==NULL )
	{
		BDBG_BSN("[%s:%d] fopen %s error!\n", __FUNCTION__, __LINE__, FWD_READYFILE);
		return FWD_FAILED;
	}

	BDBG_BSN("parse %s\n", FWD_READYFILE);
	if( fgets(line, (sizeof(line)-1), fp)>0 )
	{
		if( memcmp(line, FWD_READY_PID, strlen(FWD_READY_PID))==0 )
		{
			sscanf(line, "%*s :%d", pid);
			BDBG_BSN("pid:%d\n", *pid);
		}
	}

	BDBG_BSN("close %s\n", FWD_READYFILE);
	fclose(fp);

	return FWD_SUCCEED;
}

static int clear_fwd_ready()
{
	unsigned char cmd[FWD_MAX_TARGET_NAMESIZE+4]={0};
	sprintf(cmd, "rm %s", FWD_READYFILE);
	system(cmd);
	
	return FWD_SUCCEED;
}

static int write_status_file(unsigned int status, unsigned char *msg, int pid)
{
	unsigned char filename[64] = {0};
	unsigned char line[FWD_MAX_LINE_SIZE] = {0};
	int fd = -1, ret = FWD_FAILED;

	if(strlen(msg)>(FWD_MAX_LINE_SIZE-strlen(FWD_STAT_MSG)-4))
	{
		BDBG_BSN("message too long!\n");
		return FWD_FAILED;
	}
	
	sprintf(filename, "%s%d", FWD_STATFILE_PREFIX, pid);
	BDBG_BSN("open %s!\n", filename);
	fd = open(filename, O_CREAT | O_RDWR);
	if(-1==fd)
	{
		BDBG_BSN("open %s error!\n", filename);
		return FWD_FAILED;
	}

	/* file lock */
	flock(fd, LOCK_EX);
	
	BDBG_BSN("write %s!\n", filename);
	sprintf(line, "%s :%d\n", FWD_STAT_STATUS, status);
	if(write(fd, line, strlen(line))<0)
	{
		ret = FWD_FAILED;
		BDBG_BSN("write status %d error!\n", status);
		goto WRITE_STAT_FILE_DONE;
	}else{
		ret = FWD_SUCCEED;
		BDBG_BSN("write %s to %s!\n", line, filename);
	}

	if(msg)
	{
		sprintf(line, "%s :%s\n", FWD_STAT_MSG, msg);
		if(write(fd, line, strlen(line))<0)
		{
			ret = FWD_FAILED;
			BDBG_BSN("write msg %s error!\n", msg);
			goto WRITE_STAT_FILE_DONE;
		}else{
			ret = FWD_SUCCEED;
			BDBG_BSN("write %s to %s!\n", line, filename);
		}
	}
	
WRITE_STAT_FILE_DONE:
	/* file unlock */
	flock(fd, LOCK_UN);
	
	close(fd);
	return ret;
}

static char *get_share_memory(struct fwd_conf *conf, int *fd, int *flen)
{
	unsigned int shm_id=-1;
	char *memory=NULL;

	if(conf==NULL || fd==NULL || flen==NULL)
	{
		BDBG_BSN("invalid argument for %s\n", __FUNCTION__);
		return NULL;
	}
	
	if(conf->from==FWD_DATA_IN_SHM)
	{
		shm_id = conf->shmid;
		BDBG_BSN("data source: in share memory id %d\n", shm_id);
		memory = (char *)shmat(shm_id, NULL, 0); // MSE shutup compiler
	}else if(conf->from==FWD_DATA_IN_FILE){
		struct stat st;
		
		if ( stat(conf->filename, &st) < 0)
		{
			BDBG_BSN("data source file %s not exist!\n", conf->filename);
			return NULL;
		}else{
			*flen = st.st_size;
			BDBG_BSN("data source: in file %s(%d bytes)\n", conf->filename, *flen);
			
			*fd = open(conf->filename, O_RDONLY);
			if(*fd<=0)
			{
				*fd = -1;
				BDBG_BSN("open data source file %s failed!\n", conf->filename);
				return NULL;
			}else{
				memory = (char *)mmap(NULL, *flen, PROT_READ, MAP_SHARED, *fd, 0);
				if(memory==MAP_FAILED)
				{
					BDBG_BSN("mmap() source file %s failed!\n", conf->filename);
					perror("fwd:");
					return NULL;
				}
			}
		}
	}else{
		BDBG_BSN("data source invalid %d!\n", conf->from);
		return NULL;
	}
	
	if(memory==NULL)
	{
		BDBG_BSN("get shm_memory failed!\n");
		return NULL;
	}else{
		BDBG_BSN("get shm_memory at %p!\n", memory);
		return memory;
	}

}

static int clear_share_memory(struct fwd_conf *conf, unsigned char *memory, int fd, unsigned int flen)
{
	if( conf==NULL )
		return FWD_SUCCEED;
	
	if(conf->from==FWD_DATA_IN_SHM)
	{
		if(memory)
			shmdt(memory);
	}else if(conf->from==FWD_DATA_IN_FILE){
		if(memory!=MAP_FAILED && memory!=NULL)
			munmap(memory, flen);
		if(fd>0)
			close(fd);
		flen = 0;
	}

	return FWD_SUCCEED;
}

static int __fwd(struct fwd_conf_payload *payload, unsigned char *shm_memory, unsigned char *msg)
{
	if( payload==NULL || shm_memory==NULL || msg==NULL )
	{
		sprintf(msg, "payload:%p, memory:%p, msg:%p\n", payload, shm_memory, msg);
		return FWD_FAILED;
	}
	
	int fh=-1, numWrite=0;
	unsigned int offset = payload->burn_address;
	unsigned int size = payload->burn_size;
	unsigned int fw_offset = payload->data_offset;
#if CHECK_BURN_SERIAL
	struct _rootfs_padding  rootfs_padding;
	IMG_HEADER_T Header;
	unsigned long burn_serial = 0;
	int padding_len = 0;
	unsigned char* read_buf = NULL;
	int trueorfalse = 0;
#endif
	
	BDBG_BSN("burn type:%d, target:%s, burn address:0x%x, burn size:%d, data offset:0x%x:\n",
		payload->type, payload->target, offset, size, fw_offset);

	fh = open((char *)payload->target, O_RDWR); 
	if(-1==fh)
	{
		sprintf(msg, "open %s failed!\n", payload->target);
		return FWD_FAILED;
	}
	
#if CHECK_BURN_SERIAL
#ifndef CONFIG_MTD_NAND
	if (size > sizeof(IMG_HEADER_T))
	{
		memcpy((void *)(&Header), (void *)(&(shm_memory[fw_offset])), sizeof(IMG_HEADER_T));

		if (!memcmp(Header.signature, FW_SIGNATURE, SIG_LEN) ||
			!memcmp(Header.signature, FW_SIGNATURE_WITH_ROOT, SIG_LEN))
		{
			Header.burnAddr = burn_serial = gen_burn_serial();
			memcpy((void *)(&(shm_memory[fw_offset])), (void *)(&Header), sizeof(IMG_HEADER_T));
			memcpy((void *)(&(shm_memory[fw_offset+size])), (void *)(&burn_serial), sizeof(burn_serial));
			size += sizeof(burn_serial);
		}
		else if (!memcmp(Header.signature, SQSH_SIGNATURE, SIG_LEN) ||
			!memcmp(Header.signature, SQSH_SIGNATURE_LE, SIG_LEN))
		{
			struct squashfs_super_block_partial sblk;
			lseek(fh, offset, SEEK_SET);
			read(fh, (char *)(&sblk), sizeof (sblk));

			if (size == sblk.mkfs_time+SIZE_OF_SQFS_SUPER_BLOCK+SIZE_OF_CHECKSUM)
				padding_len = sizeof(rootfs_padding.zero_pad);

			memset(&rootfs_padding, 0, sizeof(rootfs_padding));
			memcpy((void *)(&rootfs_padding.chksum), (void *)(&(shm_memory[fw_offset]) + size - SIZE_OF_CHECKSUM), SIZE_OF_CHECKSUM);
			memcpy((char *)(&rootfs_padding.signature), ROOT_SIGNATURE, SIG_LEN);
			rootfs_padding.chksum -= padding_len;
			rootfs_padding.len = sblk.mkfs_time = burn_serial = size + padding_len - SIZE_OF_SQFS_SUPER_BLOCK - SIZE_OF_CHECKSUM;
			memcpy((void *)(&(shm_memory[fw_offset])), (void *)(&sblk), sizeof(struct squashfs_super_block_partial));
		}
		else {
			BDBG_BSN("\n[%s:%d] not handle!!!\n", __FUNCTION__, __LINE__);
		}
	}
#endif
#endif /* #if CHECK_BURN_SERIAL */

	if(lseek(fh, offset, SEEK_SET) == -1)
 	{
		sprintf(msg, "lseek to 0x%x failed!\n", offset);
		close(fh);
		return FWD_FAILED;
 	}
	BDBG_BSN("first 4 bytes raw data at offset 0x%x:\n", fw_offset);
	BDBG_BSN("%02x %02x %02x %02x\n", shm_memory[fw_offset], shm_memory[fw_offset+1], shm_memory[fw_offset+2], shm_memory[fw_offset+3]);
 	numWrite = write(fh, &(shm_memory[fw_offset]), size);

#if CHECK_BURN_SERIAL
#ifndef CONFIG_MTD_NAND
	if (burn_serial) {
		if (!memcmp(Header.signature, SQSH_SIGNATURE, SIG_LEN) ||
			!memcmp(Header.signature, SQSH_SIGNATURE_LE, SIG_LEN))
		{						
			lseek(fh, offset+size-SIZE_OF_CHECKSUM, SEEK_SET);

			if (padding_len) {
				BDBG_BSN("[%s:%d] pad\n", __FUNCTION__, __LINE__);
				write(fh, &rootfs_padding, sizeof(rootfs_padding));
			}
			else {
				BDBG_BSN("[%s:%d] nopad\n", __FUNCTION__, __LINE__);
				write(fh, (void *)(&rootfs_padding)+sizeof(rootfs_padding.zero_pad), sizeof(rootfs_padding)-sizeof(rootfs_padding.zero_pad));
			}
		}
	}
#endif
#endif /* #if CHECK_BURN_SERIAL */

	if(numWrite != size)
 	{
		sprintf(msg, "write failed, should write %d, actually %d!\n", size, numWrite);
		close(fh);
		return FWD_FAILED;
 	}else{
	 	sync();
#ifdef KERNEL_3_10
#ifdef CONFIG_MTD_NAND
		if(payload->type <= FWD_DATA_TYPE_ROOTFS)
		{
#endif //CONFIG_MTD_NAND
			if(ioctl(fh,BLKFLSBUF,NULL) < 0){
				sprintf(msg, "flush mtd system cache error\n");
				close(fh);
				return FWD_FAILED;
			}
#ifdef CONFIG_MTD_NAND
		}
#endif //CONFIG_MTD_NAND
#endif
		BDBG_BSN("write %d bytes succeed\n", numWrite);
 	}

#if CHECK_BURN_SERIAL
#ifndef CONFIG_MTD_NAND
	/* compare flash/ram data */
	if (!memcmp(Header.signature, FW_SIGNATURE, SIG_LEN) ||
			!memcmp(Header.signature, FW_SIGNATURE_WITH_ROOT, SIG_LEN)){
		//printf("[%s]%d\n",__func__,__LINE__);
		read_buf = (unsigned char*)malloc(size);
		if(read_buf == NULL){
			/* error */
			trueorfalse = 1;
			goto FINISH_COMPARE;
		}
			
		lseek(fh, offset, SEEK_SET);
		read(fh,read_buf,size);
		//if(memcmp(read_buf,&(shm_memory[fw_offset]),size+0x100) !=0){
		if(memcmp(read_buf,&(shm_memory[fw_offset]),size) !=0){
			/* error */
			trueorfalse = 1;
			goto FINISH_COMPARE;
		}
	}else if (!memcmp(Header.signature, SQSH_SIGNATURE, SIG_LEN) ||
			!memcmp(Header.signature, SQSH_SIGNATURE_LE, SIG_LEN)){
		//printf("[%s]%d\n",__func__,__LINE__);
		read_buf = (unsigned char*)malloc(size+sizeof(rootfs_padding));
		if(read_buf == NULL){
			/* error */
			trueorfalse = 1;
			goto FINISH_COMPARE;
		}

		lseek(fh, offset, SEEK_SET);
		read(fh,read_buf,size+sizeof(rootfs_padding));

		if(memcmp(read_buf,&(shm_memory[fw_offset]),size-SIZE_OF_CHECKSUM) != 0){
			/* error */
			trueorfalse = 1;
			goto FINISH_COMPARE;
		
		}

		if (padding_len) {
			BDBG_BSN("[%s:%d] pad\n", __FUNCTION__, __LINE__);
			if(memcmp(read_buf+size-SIZE_OF_CHECKSUM,&rootfs_padding,sizeof(rootfs_padding)) != 0){
				/* error */
				trueorfalse =1;
				goto FINISH_COMPARE;
			}
		}
		else {
			BDBG_BSN("[%s:%d] nopad\n", __FUNCTION__, __LINE__);
			if(memcmp(read_buf+size-SIZE_OF_CHECKSUM,(void *)(&rootfs_padding)+sizeof(rootfs_padding.zero_pad),
				sizeof(rootfs_padding)-sizeof(rootfs_padding.zero_pad)) != 0){
				/* error */
				trueorfalse = 1;
				goto FINISH_COMPARE;
			}
		}
	}else {
		//printf("[%s]%d\n",__func__,__LINE__);
		BDBG_BSN("\n[%s:%d] not handle!!!\n", __FUNCTION__, __LINE__);
	}

FINISH_COMPARE:
	if(read_buf){
		free(read_buf);
		read_buf = NULL;
	}

	if(trueorfalse == 1){
		memset(Header.signature,0,SIG_LEN);
		lseek(fh, offset, SEEK_SET);
		write(fh,&Header,sizeof(Header));

		sync(); 
#ifdef KERNEL_3_10
		if(ioctl(fh,BLKFLSBUF,NULL) < 0){
			sprintf(msg, "flush mtd system cache error\n");
			return FWD_FAILED;
		}
#endif
		sprintf(msg, "flash/memory data is different!\n");
		return FWD_FAILED;
	}
#endif
#endif				

	return FWD_SUCCEED;
}

#ifdef CONFIG_FWD_UNIFIED_SUPPORT_PLUGIN
static int __fwd_plugin(struct fwd_conf_payload *payload, unsigned char *shm_memory, unsigned char *msg)
{
	char decomp_dirname[FWD_MAX_TARGET_NAMESIZE+8]={0}, root_dir[FWD_MAX_TARGET_NAMESIZE+1]={0}, plugin_name[FWD_MAX_TARGET_NAMESIZE]={0}, cmd[512] = {0};
	char *LEFT_SLASH = "/", *ptr = NULL, *ptr2 = NULL;
	
	if( payload==NULL || shm_memory==NULL || msg==NULL )
	{
		sprintf(msg, "payload:%p, memory:%p, msg:%p\n", payload, shm_memory, msg);
		return FWD_FAILED;
	}

	int nwrite = 0;
	unsigned int size = payload->burn_size;
	unsigned int fw_offset = payload->data_offset;
	FILE *fp = NULL;
	
	BDBG_BSN("burn type:%d, target:%s, burn size:%d, data offset:0x%x:\n",
		payload->type, payload->target, size, fw_offset);

	//generate dir name
	snprintf(decomp_dirname, sizeof(decomp_dirname), "%s_decomp", payload->target);
	snprintf(root_dir, sizeof(root_dir), "%s", payload->target);
	BDBG_BSN("[%s:%d] decomp dir:%s, root dir:%s\n", __FUNCTION__, __LINE__, decomp_dirname, root_dir);
	ptr = ptr2 = root_dir;
	while(ptr)
	{
		if( (ptr = strstr(ptr, LEFT_SLASH))==NULL )
		{
			snprintf(plugin_name, sizeof(plugin_name), "%s", ptr2+1);	//strip '/' at ptr2
			*ptr2 = '\0';
			break;
		}else{
			ptr2 = ptr;
			ptr++;
		}
	}
	BDBG_BSN("[%s:%d] root dir:%s, plugin_name:%s\n", __FUNCTION__, __LINE__, root_dir, plugin_name);
	
	fp = fopen(payload->target, "w+");
	if(fp==NULL)
	{
		sprintf(msg, "create plugin file %s failed\n", payload->target);
		return FWD_FAILED;
	}

	if((nwrite=fwrite(shm_memory+fw_offset, sizeof(char), size, fp))<=0)
	{
		sprintf(msg, "write plugin file %s failed\n", payload->target);
		return FWD_FAILED;
	}else
		BDBG_BSN("write %d bytes to %s\n", nwrite, payload->target);

	fclose(fp);

	//decompress file
	if(access(decomp_dirname, F_OK)!=0) {
		snprintf(cmd, sizeof(cmd), "mkdir %s", decomp_dirname);
		BDBG_BSN("cmd:%s\n", cmd);
		system(cmd);
	}
	snprintf(cmd, sizeof(cmd), "tar -xf %s -C %s", payload->target, decomp_dirname);
	BDBG_BSN("cmd:%s\n", cmd);
	if(system(cmd)==0)
	{
		//succeed
		BDBG_BSN("[%s;%d] decompress succeed to %s\n", __FUNCTION__, __LINE__, decomp_dirname);
		snprintf(cmd, sizeof(cmd), "rm %s -f", payload->target);	//remove compressed file, to save space.
		BDBG_BSN("cmd:%s\n", cmd);
		system(cmd);
		snprintf(cmd, sizeof(cmd), "/bin/plugin.validate %s %s %s/%s", plugin_name, decomp_dirname, root_dir, PLUGIN_INSTALL_DIRNAME);
		BDBG_BSN("cmd:%s\n", cmd);
		if(system(cmd)==0)
		{
			BDBG_BSN("[%s:%d] upgrade plugin succeed\n", __FUNCTION__, __LINE__);
		}else{
			sprintf(msg, "upgrade plugin file %s failed\n", payload->target);
			return FWD_FAILED;
		}
	}else{
		sprintf(msg, "decompress plugin file %s failed\n", payload->target);
		return FWD_FAILED;
	}
	
	return FWD_SUCCEED;
}
#endif

int main(int argc, char *argv[])
{
	unsigned char *shm_memory=NULL;
	struct fwd_conf conf = {0};
	int pid=-1, source_file_fd=-1, source_file_length=0, fwd_status=FWD_INIT, reboot_delay=0;
	FILE *fp=NULL, *fp2=NULL;
	unsigned char  buf[FWD_MAX_LINE_SIZE]={0};
#if defined(CONFIG_ELINK_SUPPORT) || defined(CONFIG_ANDLINK_SUPPORT)
    struct stat status;
#endif

	while(1)
	{
		sleep(3);
		
		fp = fopen(FWD_READYFILE, "r");
   		if (fp)
		{
			BDBG_BSN("fwd status %d->", fwd_status);
			fwd_status = FWD_PARSING;
			BDBG_BSN("%d\n", fwd_status);
			
			if(FWD_FAILED==get_fwd_ready(&pid))
			{
				sprintf(buf, "get_fwd_ready() failed!\n");
				BDBG_BSN("%s", buf);
				goto NEXT_ROUND;
			}

			write_status_file(fwd_status, buf, pid);
			fp2 = fopen(FWD_CONFFILE, "r");
			if(fp2)
			{
				if(FWD_FAILED==get_fwd_conf(&conf))
				{
					BDBG_BSN("get_fwd_conf() failed!\n");
					goto NEXT_ROUND;
				}else{
					int i;
					for(i=0; i<conf.section_num; i++)
					{
						struct fwd_conf_payload *payload = (struct fwd_conf_payload *)conf.payload + i;
						
						BDBG_BSN("[section %d] ptr:%p\n", i, payload);
						BDBG_BSN("[section %d] type:%d(%p)\n", i, payload->type, payload);
						BDBG_BSN("[section %d] prio:%d(%p)\n", i, payload->prio, payload);
						BDBG_BSN("[section %d] target:%s(%p)\n", i, payload->target, payload);
						BDBG_BSN("[section %d] burn_address:0x%x(%p)\n", i, payload->burn_address, payload);
						BDBG_BSN("[section %d] burn_size:%d(%p)\n", i, payload->burn_size, payload);
						BDBG_BSN("[section %d] data_offset:0x%x(%p)\n", i, payload->data_offset, payload);
					}
				}
				
				// get raw upgrade data in pointer shm_memory
				if(NULL==(shm_memory=get_share_memory(&conf, &source_file_fd, &source_file_length)))
				{
					BDBG_BSN("get shm_memory failed!\n");
					goto NEXT_ROUND;
				}

				// get behaviour
				reboot_delay = conf.reboot_delay;
				BDBG_BSN("delay reboot: %d\n", reboot_delay);

				// get each 'Raw Data Analyze' section
				if(0==conf.section_num)
				{
					BDBG_BSN("No any section in conf!\n");
					goto NEXT_ROUND;
				}else{
					unsigned int i = 0, j = 0, k = 0;
					struct fwd_conf_payload *payload = NULL;
					enum FWD_DATA_PRIO prio = FWD_PRIO_IG;
					
					BDBG_BSN("fwd status %d->", fwd_status);
					fwd_status = FWD_BURNING;
					BDBG_BSN("%d\n", fwd_status);
					
					while(1)
					{
						for(k=0; k<conf.section_num; k++)
						{
							payload = (struct fwd_conf_payload *)conf.payload + k;
							if(payload->prio > prio)
							{
								prio = payload->prio;
								i = k;
							}
						}
						if(prio==FWD_PRIO_IG)
						{
							BDBG_BSN("All sections upgraded !!!\n");
							fwd_status = FWD_SUCCEED;
							break;
						}
						payload = (struct fwd_conf_payload *)conf.payload + i;
						fwd_status = FWD_BURNING;
						BDBG_BSN("[section %d] __fwd() start, payload %p\n", i+1, payload);
						sprintf(buf, "burning %d (type %d)\n", i+1, payload->type);
						write_status_file(fwd_status, buf, pid);

						//get the section's ops
						while( (FWD_OP_TABLE[j].sectype!=-1) && (FWD_OP_TABLE[j].sectype!=payload->type) )					
							j++;
						if(FWD_OP_TABLE[j].sectype==-1)
						{
							sprintf(buf, "section %d type %d is not supported by fwd!!!\n", i+1, payload->type);
							BDBG_BSN("[section %d] __fwd failed, reason:%s!\n", i+1, buf);
							fwd_status = FWD_FAILED;
							break;
						}else{
							BDBG_BSN("[section %d] callback table ID:%d\n", i+1, j);
						}
						
						//prepare
						if(FWD_OP_TABLE[j].prepare)
						{
							memset(buf, 0x0, sizeof(buf));
							if(FWD_FAILED==(fwd_status=FWD_OP_TABLE[j].prepare(payload, shm_memory, buf)))
							{
								buf[sizeof(buf)-1] = '\0';
								BDBG_BSN("[section %d] prepare:\tFailed, reason:%s!\n", i+1, buf);
								break;
							}else{
								BDBG_BSN("[section %d] prepare:\tOK.\n", i+1);
							}
						}else{
							BDBG_BSN("[section %d] prepare:\tIgnore.\n", i+1);
						}

						//do burn
						if(FWD_OP_TABLE[j].write)
						{
							memset(buf, 0x0, sizeof(buf));
							if(FWD_FAILED==(fwd_status=FWD_OP_TABLE[j].write(payload, shm_memory, buf)))
							{
								buf[sizeof(buf)-1] = '\0';
								BDBG_BSN("[section %d] burn:\tFailed, reason:%s!\n", i+1, buf);
								break;
							}else{
								BDBG_BSN("[section %d] burn:\tOK.\n", i+1);
							}
						}else{
							BDBG_BSN("[section %d] burn:\tIgnore.\n", i+1);
						}

						//validate
						if(FWD_OP_TABLE[j].validate)
						{
							memset(buf, 0x0, sizeof(buf));
							if(FWD_FAILED==(fwd_status=FWD_OP_TABLE[j].validate(payload, shm_memory, buf)))
							{
								buf[sizeof(buf)-1] = '\0';
								BDBG_BSN("[section %d] validate:\tFailed, reason:%s!\n", i+1, buf);
								break;
							}else{
								BDBG_BSN("[section %d] validate:\tOK.\n", i+1);
							}
						}else{
							BDBG_BSN("[section %d] validate:\tIgnore.\n", i+1);
						}

						//reset
						j = 0;
						prio = payload->prio = FWD_PRIO_IG;
					}
				}
				
				BDBG_BSN("fwd status %d->", FWD_BURNING);
				if(fwd_status==FWD_FAILED)
					fwd_status = FWD_ERROR;
				else
					fwd_status = FWD_DONE;
				BDBG_BSN("%d\n", fwd_status);
				
				write_status_file(fwd_status, buf, pid);

				goto NEXT_ROUND;
			}	else
				continue;
		}else
			continue;
		
NEXT_ROUND:
		write_status_file(fwd_status, buf, pid);
		
		clear_share_memory(&conf, shm_memory, source_file_fd, source_file_length);
        shm_memory = NULL;
		
		if(fp!=NULL){
			fclose(fp);
			fp = NULL;
		}
		clear_fwd_ready();
		
		if(fp2!=NULL){
			fclose(fp2);
			fp2 = NULL;
		}
		clear_fwd_conf(&conf, pid);

		memset(buf, 0x0, sizeof(buf));
		pid = -1;
		if(reboot_delay==0 && fwd_status==FWD_DONE)
		{
			// finish burning successfully, do watchdog reboot!!!
			FILE *fp_watchdog = NULL;

			fp_watchdog = fopen("/proc/watchdog_reboot","w+"); //reboot
			if(fp_watchdog)
			{
				fprintf(stderr, "FWD watchdog reboot!!!\n");
				fflush(stderr);
				sleep(1);
				fputs("111", fp_watchdog);
				fclose(fp_watchdog);
			}

			for(;;);
		}else{
			BDBG_BSN("fwd status %d->", fwd_status);
			fwd_status = FWD_INIT;
			BDBG_BSN("%d (delay reboot).\n", fwd_status);
		}
	}
}



#else

#if defined(CONFIG_ANDLINK_SUPPORT)||defined(CONFIG_ELINK_SUPPORT)
#include "../../rtl_link/fwupgrade.h"
#define RTL_LINK_FW_DIR "/var/rtl_link_fw_dir"
#if defined(RTL_LINK_UPGRADE_SHRINKMEM)
#include <sys/mman.h>
#endif
#endif

//--------------------------------------------------------------------
//#define CHECK_BURN_SERIAL 1

#if 0
  #define BDBG_BSN(format, arg...) 	  \
		  printf(format , ## arg)
#else
  #define BDBG_BSN(format, arg...)
#endif

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
#if defined(CONFIG_ANDLINK_SUPPORT)||defined(CONFIG_ELINK_SUPPORT)
#if defined(RTL_LINK_UPGRADE_SHRINKMEM)
static int isFileExist(char *file_name)
{
	struct stat status;

	if ( stat(file_name, &status) < 0)
		return 0;

	return 1;
}
#endif

static int daemonKilled = 0;
//static void killDaemon()
void upgrade_killDaemons()
{
 		
    if (daemonKilled)
    	return;

    daemonKilled = 1;

#if 1	
	FILE *stream;
	system("ps > /var/alive_daemons");
	stream = fopen ( "/var/alive_daemons", "r" );
	if ( stream != NULL ) { 	
		char *strtmp, *pid_token=NULL;
		char line[100], cmdBuf[50];

		while (fgets(line, sizeof(line), stream))
		{
			strtmp = line;
			while(*strtmp == ' ')
				strtmp++;
            if(	(strstr(strtmp,"boa") != 0) 
            		|| (strstr(strtmp,"/bin/sh") != 0) 
            		|| (strstr(strtmp,"ps") != 0) 
            		|| (strstr(strtmp,"testcase") != 0) 
            		|| (strstr(strtmp,"edp_client") != 0) 
            		|| (strstr(strtmp, "root") == 0)
#if defined(CONFIG_APP_FWD) // fwd is daemon to write firmware in flash at last.
            		|| (strstr(strtmp, "fwd") != 0)
#endif            		
#if defined(APP_WATCHDOG)
					|| (strstr(strtmp,"watchdog") != 0)
#endif
#if defined(CONFIG_ELINK_SUPPORT)
                    || (strstr(strtmp,"elink") != 0)
#endif
            )
				continue;
			else
			{

				pid_token = strtok(strtmp, " ");
				if((pid_token != NULL) && (strlen(pid_token) >= 3))
				{
					sprintf(cmdBuf,"kill -9 %s > /dev/null", pid_token);
					system(cmdBuf);
				}
			}
		}
		fclose(stream);
	}
	system("rm -rf /var/alive_daemons");

#endif
	
// added by rock /////////////////////////////////////////
#ifdef VOIP_SUPPORT
	system("killall -9 snmpd 2> /dev/null");
	system("killall -9 solar_monitor 2> /dev/null");
	system("killall -9 solar 2> /dev/null");
	system("killall -9 dns_task 2> /dev/null");
	system("killall -9 ivrserver 2> /dev/null");
	system("killall -9 fwupdate 2> /dev/null");
	system("killall -9 netlink 2> /dev/null");
#endif

#ifdef CONFIG_SNMP
	system("killall -9 snmpd 2> /dev/null");
#endif
}

char *get_ifname(char *name, char *p)
{
	while (isspace(*p))
		p++;
	while (*p) {
		if (isspace(*p))
			break;
		if (*p == ':') {	/* could be an alias */
			char *dot = p, *dotname = name;

			*name++ = *p++;
			while (isdigit(*p))
				*name++ = *p++;
			if (*p != ':') {	/* it wasn't, backup */
				p = dot;
				name = dotname;
			}
			if (*p == '\0')
				return NULL;
			p++;
			break;
		}
		*name++ = *p++;
	}
	*name++ = '\0';
	return p;
}

#define _PATH_PROCNET_DEV "/proc/net/dev"

int shutdown_netdev(void)
{
	FILE *fh;
	char buf[512];
	char *s, name[16], tmp_str[64];
	int iface_num=0;
	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh) {
		return 0;
	}
	fgets(buf, sizeof buf, fh);	/* eat line */
	fgets(buf, sizeof buf, fh);
	while (fgets(buf, sizeof buf, fh)) {
		s = get_ifname(name, buf);
		if(strstr(name, "eth") || strstr(name, "wlan") || strstr(name, "ppp")|| strstr(name, "sit")){
			iface_num++;
			snprintf(tmp_str,sizeof(tmp_str),"ifconfig %s down",name);
			system(tmp_str);
		}
	}
	
	fclose(fh);
	return iface_num;
}
pid_t find_pid_by_name( char* pidName)
{
	DIR *dir;
	struct dirent *next;
	pid_t pid;
	
	if ( strcmp(pidName, "init")==0)
		return 1;
	
	dir = opendir("/proc");
	if (!dir) {
		printf("Cannot open /proc");
		return 0;
	}

	while ((next = readdir(dir)) != NULL) {
		FILE *status;
		char filename[64];
		char buffer[64];
		char name[64];

		/* Must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, "..") == 0)
			continue;

		/* If it isn't a number, we don't want it */
		if (!isdigit(*next->d_name))
			continue;

		sprintf(filename, "/proc/%s/status", next->d_name);
		if (! (status = fopen(filename, "r")) ) {
			continue;
		}
		if (fgets(buffer, 63, status) == NULL) {
			fclose(status);
			continue;
		}
		fclose(status);

		/* Buffer should contain a string like "Name:   binary_name" */
		sscanf(buffer, "%*s %s", name);
		if (strcmp(name, pidName) == 0) {
		//	pidList=xrealloc( pidList, sizeof(pid_t) * (i+2));
			pid=(pid_t)strtol(next->d_name, NULL, 0);
			closedir(dir);
			return pid;
		}
	}	
	closedir(dir);
	return 0;
}

#endif

int main(int argc, char *argv[])
{
	unsigned char *shm_memory=NULL;
	int shm_id = 0;
	FILE *fp, *fp2;
	FILE *fp_watchdog;
	char  buf[150]={0};
#if defined(CONFIG_ANDLINK_SUPPORT)||defined(CONFIG_ELINK_SUPPORT)
    struct stat status;
#if defined(RTL_LINK_UPGRADE_SHRINKMEM)   
    int fileLen, fd;
    char fwdir[256];
#endif
#endif
				
#if CHECK_BURN_SERIAL
	struct _rootfs_padding  rootfs_padding;
	IMG_HEADER_T Header;
	unsigned long burn_serial = 0;
	int padding_len = 0;
	unsigned char* read_buf = NULL;
	int trueorfalse = 0;
#endif

	while(1)
	{  	  


		sleep(3);
		

		fp = fopen("/var/fwd.ready", "r");
    if (fp)
		{
#if defined(CONFIG_ANDLINK_SUPPORT)||defined(CONFIG_ELINK_SUPPORT)
#if defined(RTL_LINK_UPGRADE_SHRINKMEM)
            if(isFileExist("/var/rtl_link_shrinkmem")){
                fgets(fwdir,sizeof(fwdir),fp);
                stat(fwdir, &status);
                fileLen = status.st_size;

                fd = open(fwdir, O_RDONLY);
                if(fd == -1){
                    printf("FWD (%s)%d open fw %s failed\n", __FUNCTION__, __LINE__, fwdir);
                }

                shm_memory = (char *)mmap(NULL, fileLen, PROT_READ, MAP_SHARED, fd, 0);
                if(shm_memory == (void *)-1){
                    printf("FWD (%s)%d mmap failed\n", __FUNCTION__, __LINE__);
                }
#if 1
                upgrade_killDaemons();
#if defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F)
                // do nothing
#else
                shutdown_netdev();
                system("ifconfig br0 down 2> /dev/null");
#endif
#endif
            }else
#endif
#endif	
            {
       		    fgets(buf,150,fp);
    			shm_id = atoi(buf);
    			
    			/* Attach the shared memory segment */
				shm_memory = (unsigned char *)shmat(shm_id, NULL, 0); // MSE shutup compiler
			}
			
			fp2 = fopen("/var/fwd.conf", "r");
			if(fp2)
			{
//				int head_offset=0;// MSE shutup compiler
				unsigned char mtd_name[20]={0};
				unsigned char offset_str[20]={0};
				unsigned char size_str[20]={0};
				unsigned char fw_offset_str[20]={0};
				
				int numWrite;
//				int fw_offset=0;  // MSE shutup compiler
				
				
				fgets((char *)mtd_name,20,fp2); 		// MSE shutup compiler 			
				fgets((char *)offset_str,20,fp2);		// MSE shutup compiler 			
				fgets((char *)size_str,20,fp2); 		// MSE shutup compiler 			
				fgets((char *)fw_offset_str,20,fp2);	// MSE shutup compiler

								
				do{
					
					int fh;
					int offset = atoi((const char *)offset_str);            // MSE shutup compiler
				 	int size = atoi((const char *)size_str);                // MSE shutup compiler
				 	int fw_offset = atoi((const char *)fw_offset_str);      // MSE shutup compiler				 	

				 	mtd_name[strlen((const char *)mtd_name)-1]='\0';        // MSE shutup compiler
				 	
				 	fh = open((char *)mtd_name, O_RDWR);                    // MSE shutup compiler
				 	if(fh == -1)
				 	{
						//printf("\r\n Open [%s] fail.__[%s-%u]\r\n",mtd_name,__FILE__,__LINE__);				 		
						fp_watchdog = fopen("/proc/watchdog_reboot","w+"); //reboot
						if(fp_watchdog)
						{
							fputs("111", fp_watchdog);
							fclose(fp_watchdog);
						}
						
						for(;;);
				 	}

#if CHECK_BURN_SERIAL
#ifndef CONFIG_MTD_NAND
					if (size > sizeof(IMG_HEADER_T))
					{
				 		memcpy((void *)(&Header), (void *)(&(shm_memory[fw_offset])), sizeof(IMG_HEADER_T));

						if (!memcmp(Header.signature, FW_SIGNATURE, SIG_LEN) ||
							!memcmp(Header.signature, FW_SIGNATURE_WITH_ROOT, SIG_LEN))
						{
							Header.burnAddr = burn_serial = gen_burn_serial();
							memcpy((void *)(&(shm_memory[fw_offset])), (void *)(&Header), sizeof(IMG_HEADER_T));
							memcpy((void *)(&(shm_memory[fw_offset+size])), (void *)(&burn_serial), sizeof(burn_serial));
							size += sizeof(burn_serial);
						}
						else if (!memcmp(Header.signature, SQSH_SIGNATURE, SIG_LEN) ||
							!memcmp(Header.signature, SQSH_SIGNATURE_LE, SIG_LEN))
						{
							struct squashfs_super_block_partial sblk;
						 	lseek(fh, offset, SEEK_SET);
							read(fh, (char *)(&sblk), sizeof (sblk));

							if (size == sblk.mkfs_time+SIZE_OF_SQFS_SUPER_BLOCK+SIZE_OF_CHECKSUM)
								padding_len = sizeof(rootfs_padding.zero_pad);

							memset(&rootfs_padding, 0, sizeof(rootfs_padding));
							memcpy((void *)(&rootfs_padding.chksum), (void *)(&(shm_memory[fw_offset]) + size - SIZE_OF_CHECKSUM), SIZE_OF_CHECKSUM);
							memcpy((char *)(&rootfs_padding.signature), ROOT_SIGNATURE, SIG_LEN);
							rootfs_padding.chksum -= padding_len;
							rootfs_padding.len = sblk.mkfs_time = burn_serial = size + padding_len - SIZE_OF_SQFS_SUPER_BLOCK - SIZE_OF_CHECKSUM;
							memcpy((void *)(&(shm_memory[fw_offset])), (void *)(&sblk), sizeof(struct squashfs_super_block_partial));
						}
						else {
							BDBG_BSN("\n[%s:%d] not handle!!!\n", __FUNCTION__, __LINE__);
						}
					}
#endif
#endif /* #if CHECK_BURN_SERIAL */

				 	if(lseek(fh, offset, SEEK_SET) == -1)//fix converity error:CHECKED_RETURN
				 	{
				 		printf("error to call lseek. FILE:%s LINE:%d\n",__FILE__,__LINE__);
				 		fp_watchdog = fopen("/proc/watchdog_reboot","w+"); //reboot
						if(fp_watchdog)
						{
							fputs("111", fp_watchdog);
							fclose(fp_watchdog);
						}
						
						for(;;);
				 	}
				 	numWrite = write(fh, &(shm_memory[fw_offset]), size);
				 	
#if CHECK_BURN_SERIAL
#ifndef CONFIG_MTD_NAND
					if (burn_serial) {
						if (!memcmp(Header.signature, SQSH_SIGNATURE, SIG_LEN) ||
							!memcmp(Header.signature, SQSH_SIGNATURE_LE, SIG_LEN))
						{						
							lseek(fh, offset+size-SIZE_OF_CHECKSUM, SEEK_SET);

							if (padding_len) {
								BDBG_BSN("[%s:%d] pad\n", __FUNCTION__, __LINE__);
								write(fh, &rootfs_padding, sizeof(rootfs_padding));
							}
							else {
								BDBG_BSN("[%s:%d] nopad\n", __FUNCTION__, __LINE__);
								write(fh, (void *)(&rootfs_padding)+sizeof(rootfs_padding.zero_pad), sizeof(rootfs_padding)-sizeof(rootfs_padding.zero_pad));
							}
						}
					}
#endif
#endif /* #if CHECK_BURN_SERIAL */

				 	if(numWrite != size)
				 	{
						//printf("\r\n Write firmware size incorrect.__[%s-%u]\r\n",__FILE__,__LINE__);				 		
						fp_watchdog = fopen("/proc/watchdog_reboot","w+"); //reboot
						if(fp_watchdog)
						{
							fputs("111", fp_watchdog);
							fclose(fp_watchdog);
						}
						
						for(;;);			 		
				 	}
				 	sync();	
					#ifdef KERNEL_3_10
					if(ioctl(fh,BLKFLSBUF,NULL) < 0){
						printf("flush mtd system cache error\n");
					}
					#endif
#if CHECK_BURN_SERIAL
#ifndef CONFIG_MTD_NAND
					/* compare flash/ram data */
					if (!memcmp(Header.signature, FW_SIGNATURE, SIG_LEN) ||
							!memcmp(Header.signature, FW_SIGNATURE_WITH_ROOT, SIG_LEN)){
						//printf("[%s]%d\n",__func__,__LINE__);
						read_buf = (unsigned char*)malloc(size);
						if(read_buf == NULL){
							/* error */
							trueorfalse = 1;
							goto FINISH_COMPARE;
						}
							
						lseek(fh, offset, SEEK_SET);
						read(fh,read_buf,size);
						//if(memcmp(read_buf,&(shm_memory[fw_offset]),size+0x100) !=0){
						if(memcmp(read_buf,&(shm_memory[fw_offset]),size) !=0){
							/* error */
							trueorfalse = 1;
							goto FINISH_COMPARE;
						}
					}else if (!memcmp(Header.signature, SQSH_SIGNATURE, SIG_LEN) ||
							!memcmp(Header.signature, SQSH_SIGNATURE_LE, SIG_LEN)){
						//printf("[%s]%d\n",__func__,__LINE__);
						read_buf = (unsigned char*)malloc(size+sizeof(rootfs_padding));
						if(read_buf == NULL){
							/* error */
							trueorfalse = 1;
							goto FINISH_COMPARE;
						}

						lseek(fh, offset, SEEK_SET);
						read(fh,read_buf,size+sizeof(rootfs_padding));

						if(memcmp(read_buf,&(shm_memory[fw_offset]),size-SIZE_OF_CHECKSUM) != 0){
							/* error */
							trueorfalse = 1;
							goto FINISH_COMPARE;
						
						}

						if (padding_len) {
							BDBG_BSN("[%s:%d] pad\n", __FUNCTION__, __LINE__);
							if(memcmp(read_buf+size-SIZE_OF_CHECKSUM,&rootfs_padding,sizeof(rootfs_padding)) != 0){
								/* error */
								trueorfalse =1;
								goto FINISH_COMPARE;
							}
						}
						else {
							BDBG_BSN("[%s:%d] nopad\n", __FUNCTION__, __LINE__);
							if(memcmp(read_buf+size-SIZE_OF_CHECKSUM,(void *)(&rootfs_padding)+sizeof(rootfs_padding.zero_pad),
								sizeof(rootfs_padding)-sizeof(rootfs_padding.zero_pad)) != 0){
								/* error */
								trueorfalse = 1;
								goto FINISH_COMPARE;
							}
						}
					}else {
						//printf("[%s]%d\n",__func__,__LINE__);
						BDBG_BSN("\n[%s:%d] not handle!!!\n", __FUNCTION__, __LINE__);
					}

FINISH_COMPARE:
					if(read_buf){
						free(read_buf);
						read_buf = NULL;
					}
		
					if(trueorfalse == 1){
						printf("[%s]:%d,flash/memory data is different\n",__func__,__LINE__);
						memset(Header.signature,0,SIG_LEN);
						lseek(fh, offset, SEEK_SET);
						write(fh,&Header,sizeof(Header));

						sync();	
						#ifdef KERNEL_3_10
						if(ioctl(fh,BLKFLSBUF,NULL) < 0){
							printf("flush mtd system cache error\n");
						}
						#endif

						fp_watchdog = fopen("/proc/watchdog_reboot","w+"); //reboot
						if(fp_watchdog)
						{
							fputs("111", fp_watchdog);
							fclose(fp_watchdog);
						}
						
						for(;;);
					}
#endif
#endif				
					close(fh);					 
				 	
				 	memset(mtd_name,0x00,sizeof(mtd_name));
				 	memset(offset_str,0x00,sizeof(offset_str));
				 	memset(size_str,0x00,sizeof(size_str));
				 	memset(fw_offset_str,0x00,sizeof(fw_offset_str));
				 	
					fgets((char *)mtd_name,20,fp2); 	// MSE shutup compiler 
					fgets((char *)offset_str,20,fp2);	// MSE shutup compiler 
					fgets((char *)size_str,20,fp2); 	// MSE shutup compiler
					fgets((char *)fw_offset_str,20,fp2);// MSE shutup compiler

					
				}while( !feof(fp2) );
				
				fclose(fp2);
#if defined(CONFIG_ELINK_SUPPORT) || defined(CONFIG_ANDLINK_SUPPORT)
                if(stat("/var/rtl_link_restart_block", &status) < 0){
                    fp_watchdog = fopen("/proc/watchdog_reboot","w+"); //reboot
                    if(fp_watchdog)
                    {
                        fputs("111", fp_watchdog);
                        fclose(fp_watchdog);
                    }
                    for(;;);
                }else{
                    unlink("/var/rtl_link_restart_block");
                }
#else												
				fp_watchdog = fopen("/proc/watchdog_reboot","w+"); //reboot
				if(fp_watchdog)
				{
					fputs("111", fp_watchdog);
					fclose(fp_watchdog);
				}
				for(;;);
#endif
			}
			else
			{
				fp_watchdog = fopen("/proc/watchdog_reboot","w+"); //reboot
				if(fp_watchdog)
				{
					fputs("111", fp_watchdog);
					fclose(fp_watchdog);
				}
				
				for(;;);
				
			}

#if defined(CONFIG_ELINK_SUPPORT) || defined(CONFIG_ANDLINK_SUPPORT)	
             unlink("/var/fwd.ready");
             if(shm_memory){
                 shmdt(shm_memory);
                 shm_memory = NULL;
             }
             if(shm_id){
                 shmctl(shm_id, IPC_RMID, 0);
                 shm_id = 0;
             }
#endif			
				
			fclose(fp);
		}
#ifdef CONFIG_ELINK_SUPPORT
            unlink("/var/fwd.ready");
#endif
	}
}


#endif //CONFIG_FWD_UNIFIED_FRAMEWORK
