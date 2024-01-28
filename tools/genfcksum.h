#ifndef __GEN_FCKSUM_H__
#define __GEN_FCKSUM_H__

#ifdef CONFIG_RTL_FAST_CHECKSUM_ENABLE

#define FASTCKSUM_FAILED                    -1
#define FASTCKSUM_SUCCESS                   0

#define SIG_LEN                 			4
#define SIZE_OF_CHECKSUM        			2

#define RTL_FASTCKSUM_ALGO_MAGIC            0xdeadbeaf
#define RTL_FASTCKSUM_FIELD_LINUX_VALID     (1<<0)
#define RTL_FASTCKSUM_FIELD_ROOT_VALID      (1<<1)

enum e_rtl_fast_checksum_type {
    FCKSUM_KERNEL,
    FCKSUM_SQASHFS,
};

typedef struct _rtl_fast_checksum_hdr
{
    int  len;               //n*sizeof(FASTCKSUM_PAYLOAD_T), n>=0
    char payload[0];        //FASTCKSUM_PAYLOAD_T
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

#endif/*CONFIG_RTL_FAST_CHECKSUM_ENABLE*/

#endif
