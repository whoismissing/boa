#ifndef __FASTCKSUM_H__
#define __FASTCKSUM_H__

#ifdef CONFIG_RTL_FAST_CHECKSUM_ENABLE
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define _LITTLE_ENDIAN_
#elif __BYTE_ORDER == __BIG_ENDIAN
#define _BIG_ENDIAN_
#endif

//#define RTL_FASTCKSUM_DBG			1

#define FASTCKSUM_FAILED			-1
#define FASTCKSUM_SUCCESS			0
#define RTL_FASTCKSUM_HDR_MAGIC 	"checksum"
#define RTL_FASTCKSUM_ALGO_MAGIC	0xdeadbeaf
#define RTL_FASTCKSUM_MTD_NAME		"cksum"
#define RTL_FASTCKSUM_FIELD_LINUX_VALID		(1<<0)
#define RTL_FASTCKSUM_FIELD_ROOT_VALID		(1<<1)

#if defined(_LITTLE_ENDIAN_)
#define DWORD_SWAP(v) ( (((v&0xff)<<24)&0xff000000) | ((((v>>8)&0xff)<<16)&0xff0000) | \
        ((((v>>16)&0xff)<<8)&0xff00) | (((v>>24)&0xff)&0xff) )
#define WORD_SWAP(v) ((unsigned short)(((v>>8)&0xff) | ((v<<8)&0xff00)))
#elif defined(_BIG_ENDIAN_)
#define DWORD_SWAP(v) (v)
#define WORD_SWAP(v) (v)
#else
#error "Endian is not defined"
#endif

#ifdef RTL_FASTCKSUM_DBG
#define FCS_DBG(fmt, arg...)		do{printf(fmt, ##arg);}while(0);
#else
#define FCS_DBG(fmt, arg...)		do{}while(0);
#endif

typedef struct _rtl_fast_checksum_hdr
{
    int  len;				//n*sizeof(FASTCKSUM_PAYLOAD_T), n>=0
    char payload[0];		//FASTCKSUM_PAYLOAD_T
}FASTCKSUM_HEADER_T, *FASTCKSUM_HEADER_Tp;

typedef struct _rtl_fast_checksum_content
{
    int  bank;
    unsigned char linux_sig[8];
    int  linux_cksum;
    unsigned char root_sig[8];
    int  root_cksum;
    int  reserved;
}FASTCKSUM_PAYLOAD_T, *FASTCKSUM_PAYLOAD_Tp;

int gen_fastcksum(unsigned int *cksum, int payload_len, short old_cksum);
int read_fastcksum_from_flash(unsigned char *buf, int len);
int write_fastcksum_to_flash(unsigned char *buf, int len);
#endif

#endif
