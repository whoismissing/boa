#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "apmib.h"
#include "genfcksum.h"

#ifdef CONFIG_RTL_FAST_CHECKSUM_ENABLE
/* adler32 */
typedef unsigned long  uLong;
typedef unsigned char Bytef;
typedef unsigned int uInt;

#define BASE 65521      /* largest prime smaller than 65536 */
#define NMAX 5552
#define Z_NULL NULL
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define DO1(buf,i)  {adler += (buf)[i]; sum2 += adler;}
#define DO2(buf,i)  {DO1(buf,i); DO1(buf,i+1);}
#define DO4(buf,i)  {DO2(buf,i); DO2(buf,i+2);}
#define DO8(buf,i)  {DO4(buf,i); DO4(buf,i+4);}
#define DO16(buf)   {DO8(buf,0); DO8(buf,8);}
#define MOD(a) a %= BASE
#define MOD28(a) a %= BASE
#define MOD63(a) a %= BASE

/* ========================================================================= */
static uLong adler32(adler, buf, len)
    uLong adler;
    const Bytef *buf;
    uInt len;
{
    unsigned long sum2;
    unsigned n;

    /* split Adler-32 into component sums */
    sum2 = (adler >> 16) & 0xffff;
    adler &= 0xffff;

    /* in case user likes doing a byte at a time, keep it fast */
    if (len == 1) {
        adler += buf[0];
        if (adler >= BASE)
            adler -= BASE;
        sum2 += adler;
        if (sum2 >= BASE)
            sum2 -= BASE;
        return adler | (sum2 << 16);
    }   

    /* initial Adler-32 value (deferred check for len == 1 speed) */
    if (buf == Z_NULL)
        return 1L; 

    /* in case short lengths are provided, keep it somewhat fast */
    if (len < 16) {
        while (len--) {
            adler += *buf++;
            sum2 += adler;
        }   
        if (adler >= BASE)
            adler -= BASE;
        MOD28(sum2);            /* only added so many BASE's */
        return adler | (sum2 << 16);
    }

    /* do length NMAX blocks -- requires just one modulo operation */
    while (len >= NMAX) {
        len -= NMAX;
        n = NMAX / 16;          /* NMAX is divisible by 16 */
        do {
            DO16(buf);          /* 16 sums unrolled */
            buf += 16;
        } while (--n);
        MOD(adler);
        MOD(sum2);
    }

    /* do remaining bytes (less than NMAX, still just one modulo) */
    if (len) {                  /* avoid modulos if none remaining */
        while (len >= 16) {
            len -= 16;
            DO16(buf);
            buf += 16;
        }
        while (len--) {
            adler += *buf++;
            sum2 += adler;
        }
        MOD(adler);
        MOD(sum2);
    }

    /* return recombined sums */
    return adler | (sum2 << 16);
}

/*
 * gen_fastcksum():
 *  Generate fast checksum.
 *    Buffer contains of payload length(int) + old checksum of linux or root(short) + magic number(int).
 *   Use this buffer to perform Adler-32 and get an unsigned int fast checksum.
 *  Note:
 *     payload_len: the exact len field value of IMG_HEADER_T stored in flash(should with big endian, check cvimg.c)
 *        old_cksum: the exact old checksum value stored in flash(should with big endian, check cvimg.c).
 */
static int gen_fastcksum(unsigned int *cksum, int payload_len, short old_cksum)
{
    unsigned char buf[16] = {0};
    int at = 0, i = 0;
    unsigned int tmp = 0;

    if(NULL==cksum)
        return FASTCKSUM_FAILED;

    printf("[%s:%d]payload len: 0x%x, old_cksum:0x%x\n", __FUNCTION__, __LINE__, payload_len, old_cksum);

    *((int *)(buf+at)) = payload_len;
    at += sizeof(int);
    *((short *)(buf+at)) = old_cksum;
    at += sizeof(short);
    *((int *)(buf+at)) = htonl(RTL_FASTCKSUM_ALGO_MAGIC);
    at += sizeof(int);

    printf("sizeof(int):%d\n", sizeof(int));
    printf("sizeof(short):%d\n", sizeof(short));
    tmp = adler32(0L, NULL, 0); 
    tmp = adler32((unsigned long *)tmp, buf, at);
    *cksum = DWORD_SWAP(tmp);

    printf("[%s:%d]buf:(total %d)\n", __FUNCTION__, __LINE__, at);
    for(; i < at; i++)
    {   
        printf("%02x ", buf[i]);
    }   
    printf("\n[%s:%d]cksum:0x%x\n", __FUNCTION__, __LINE__, *cksum);

    return FASTCKSUM_SUCCESS;
}

/* utilites */
static void usage()
{
    printf("genfcksum:\n");
    printf("\tgenfcksum $(OUT) $(LINUX1) $(ROOT1) { $(LINUX2) $(ROOT2) }\n");
    printf("$(OUT):\tThe output filename.\n");
    printf("$(LINUX1):\tBank1 linux.bin file path.\n");
    printf("$(ROOT1):\tBank1 root.bin file path.\n");
    printf("$(LINUX2):\t(Optional)Bank2 linux.bin file path.(Only used when CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE=y)\n");
    printf("$(ROOT2):\t(Optional)Bank2 root.bin file path.(Only used when CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE=y)\n");
}

static int is_file_exist(char *file)
{
    int ret = 0;
    struct stat st;

    if(file)
    {
        if( 0==stat(file, &st) && st.st_size )
            ret = 1;
    }

    return ret;
}

static void dump_fastcksum_body(FASTCKSUM_HEADER_Tp hdr)
{
    FASTCKSUM_PAYLOAD_Tp body = NULL;
    int i=0, count=0;

    if(hdr==NULL)
        return ;

    printf("Dump fastchecksum structure:\n");
    printf("\tlen:%d, sizeof(FASTCKSUM_PAYLOAD_T):%d\n", hdr->len, sizeof(FASTCKSUM_PAYLOAD_T));
    if(hdr->len%sizeof(FASTCKSUM_PAYLOAD_T))
    {
        printf("WARNING: invalid length, exit dump!!\n");
        return;
    }
    count = hdr->len/sizeof(FASTCKSUM_PAYLOAD_T);
    for(i=0; i<count; i++)
    {
        body = ((FASTCKSUM_PAYLOAD_Tp)hdr->payload)+i;
        printf("\t[%d]:\n", i);
        printf("\t\t Bank:%d\n", body->bank);
        printf("\t\t kernel sig:%02x%02x%02x%02x\n", body->linux_sig[0], body->linux_sig[1], body->linux_sig[2], body->linux_sig[3]);
        printf("\t\t kernel cksum:0x%x\n", body->linux_cksum);
        printf("\t\t root sig:%02x%02x%02x%02x\n", body->root_sig[0], body->root_sig[1], body->root_sig[2], body->root_sig[3]);
        printf("\t\t root cksum:0x%x\n", body->root_cksum);
        printf("\t\t reserved bitvalue:0x%x\n", body->reserved);
    }
    return ;
}

static int fill_fcksum_body_linux(FASTCKSUM_PAYLOAD_Tp pBody, char *infile)
{
    int ret = FASTCKSUM_FAILED, buflen = 0, tmp = 0, new_cksum = 0;
    short old_cksum = 0;
    struct stat st;
    FILE *fd = NULL;
    char *buf = NULL;
    IMG_HEADER_T *hdr = NULL;

    if( NULL==pBody || NULL==infile )
    {
        printf("[%s:%d] invalid args\n", __FUNCTION__, __LINE__);
        goto EXIT_FL;
    }

    if(stat(infile, &st)==-1)
    {
        printf("[%s:%d] stat %s failed\n", __FUNCTION__, __LINE__, infile);
        goto EXIT_FL;
    }
    buflen = st.st_size;

    if( (fd = fopen(infile, "r")) == NULL)
    {
        printf("[%s:%d] fopen %s failed\n", __FUNCTION__, __LINE__, infile);
        goto EXIT_FL;
    }

    if( (buf = (char *)malloc(buflen)) == NULL)
    {
        printf("[%s:%d] malloc %d failed\n", __FUNCTION__, __LINE__, buflen);
        goto FREE_AND_EXIT_FL;
    }
   
    if( fread(buf, sizeof(char), buflen, fd) != buflen )
    {
        printf("[%s:%d] fread %d failed\n", __FUNCTION__, __LINE__, buflen);
        goto FREE_AND_EXIT_FL;
    }

    hdr = (IMG_HEADER_T *)buf;
    //sig
    memcpy((void *)pBody->linux_sig, hdr->signature, SIG_LEN);
    printf("Linux sig: %02x%02x%02x%02x\n", pBody->linux_sig[0], pBody->linux_sig[1], pBody->linux_sig[2], pBody->linux_sig[3]);
    //old cksum
    tmp = sizeof(IMG_HEADER_T) + DWORD_SWAP(hdr->len) - SIZE_OF_CHECKSUM;
    memcpy(&old_cksum, &buf[tmp], SIZE_OF_CHECKSUM);
    printf("Linux old cksum offset:%x, value:%x, len:%x\n", tmp, old_cksum, hdr->len);
    //new cksum
    if( FASTCKSUM_FAILED==gen_fastcksum(&new_cksum, hdr->len, old_cksum) )
    {
        printf("[%s:%d] gen fcksum failed!\n", __FUNCTION__, __LINE__);
        goto FREE_AND_EXIT_FL;
    }
    pBody->linux_cksum = new_cksum;
    printf("Linux new ecksum:%x\n", pBody->linux_cksum);

    ret = FASTCKSUM_SUCCESS;

FREE_AND_EXIT_FL:
    if(buf)
    {
        free(buf);
        buf = NULL;
    }
    if(fd)
    {
        fclose(fd);
        fd = NULL;
    }
EXIT_FL:
    return ret;
}

static int fill_fcksum_body_squashfs(FASTCKSUM_PAYLOAD_Tp pBody, char *infile)
{
    int ret = FASTCKSUM_FAILED, buflen = 0, tmp = 0;
    short old_cksum = 0;
    int new_cksum = 0;
    char *buf = NULL;
    FILE *fd = NULL;
    struct stat st;
    IMG_HEADER_T *pImgHdr = NULL;
    struct SQUASH_HDR_T {
        unsigned char sig[SIG_LEN];
        unsigned int reserved1;
        unsigned int len;
        unsigned int reserved2;
    } *pSqhHdr = NULL;

    if( NULL==pBody || NULL==infile )
    {
        printf("[%s:%d] invalid args\n", __FUNCTION__, __LINE__);
        goto EXIT_FS;
    }

    if(stat(infile, &st)==-1)
    {
        printf("[%s:%d] stat %s failed\n", __FUNCTION__, __LINE__, infile);
        goto EXIT_FS;
    }
    buflen = st.st_size;

    if( (fd = fopen(infile, "r")) == NULL)
    {
        printf("[%s:%d] fopen %s failed\n", __FUNCTION__, __LINE__, infile);
        goto EXIT_FS;
    }

    if( (buf = (char *)malloc(buflen)) == NULL)
    {
        printf("[%s:%d] malloc %d failed\n", __FUNCTION__, __LINE__, buflen);
        goto FREE_AND_EXIT_FS;
    }
   
    if( fread(buf, sizeof(char), buflen, fd) != buflen )
    {
        printf("[%s:%d] fread %d failed\n", __FUNCTION__, __LINE__, buflen);
        goto FREE_AND_EXIT_FS;
    }

    pImgHdr = (IMG_HEADER_T *)buf;
    pSqhHdr = (struct SQUASH_HDR_T *)&buf[sizeof(IMG_HEADER_T)];
    //sig
    memcpy(pBody->root_sig, pSqhHdr->sig, SIG_LEN);
    printf("Root sig: %02x%02x%02x%02x\n", pBody->root_sig[0], pBody->root_sig[1], pBody->root_sig[2], pBody->root_sig[3]);
    //old cksum
    tmp = sizeof(IMG_HEADER_T) + DWORD_SWAP(pImgHdr->len) - SIZE_OF_CHECKSUM;
    memcpy(&old_cksum, &buf[tmp], SIZE_OF_CHECKSUM);
    printf("Root old cksum offset:%x, value:%x, len:%x\n", tmp, old_cksum, pSqhHdr->len);
    //new cksum
    if( FASTCKSUM_FAILED==gen_fastcksum(&new_cksum, pSqhHdr->len, old_cksum) )
    {
        printf("[%s:%d] gen fcksum failed!\n", __FUNCTION__, __LINE__);
        goto FREE_AND_EXIT_FS;
    }
    pBody->root_cksum = new_cksum;
    printf("Root new ecksum:%x\n", pBody->root_cksum);

    ret = FASTCKSUM_SUCCESS;

FREE_AND_EXIT_FS:
    if(buf)
    {
        free(buf);
        buf = NULL;
    }
    if(fd)
    {
        fclose(fd);
        fd = NULL;
    }
EXIT_FS:
    return ret;
}

static int fill_fcksum_body(FASTCKSUM_PAYLOAD_Tp pBody, char *fname, enum e_rtl_fast_checksum_type type)
{
    int ret = FASTCKSUM_FAILED;

    switch(type)
    {
    case FCKSUM_KERNEL:
        ret = fill_fcksum_body_linux(pBody, fname);
        break;
    case FCKSUM_SQASHFS:
        ret = fill_fcksum_body_squashfs(pBody, fname);
        break;
    default:
        printf("genfcksum: unknown file type %d\n", type);
        break;
    }

    return ret;
}

/* main */
int main(int argc, char *argv[])
{
    int bank = 0, ret = FASTCKSUM_FAILED;
    FILE *out = NULL;
    FASTCKSUM_HEADER_Tp fcksum_hdr = NULL;
    FASTCKSUM_PAYLOAD_Tp fcksum_body = NULL;

    printf("genfcksum start, argc:%d\n", argc);

    /* args validatation */
    if( !(argc==4||argc==6) )
    {
        usage();
        goto RET;
    }else{
        if( is_file_exist(argv[2])==0 || is_file_exist(argv[3])==0 )
        {
            printf("genfcksum: file not exist. arg2:%s, arg3:%s\n", argv[2], argv[3]);
            goto RET;
        }
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
        if( (argc==6) && (is_file_exist(argv[4])==0 || is_file_exist(argv[5])==0) )
        {
            printf("genfcksum: file not exist. arg4:%s, arg5:%s\n", argv[4], argv[5]);
            goto RET;
        }
#endif
    }
    printf("genfcksum validation: OK\n");

    /* initial */
    out = fopen(argv[1], "w+");
    if(out==NULL)
    {
        printf("genfcksum: fopen %s failed\n", argv[1]);
        goto FREE_AND_RET;
    }
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
    bank = 2;
#else
    bank = 1;
#endif
    fcksum_hdr = (FASTCKSUM_HEADER_Tp)malloc(sizeof(FASTCKSUM_HEADER_T) + bank*sizeof(FASTCKSUM_PAYLOAD_T));
    if(fcksum_hdr==NULL)
    {
        printf("genfcksum: malloc fcksum_hdr failed\n");
        goto FREE_AND_RET;
    }
    fcksum_hdr->len = bank*sizeof(FASTCKSUM_PAYLOAD_T); //body length
    fcksum_body = fcksum_hdr->payload;
    printf("genfcksum initialization: OK\n");
    
    /* fill in content */
    //linux1
    fcksum_body->bank = 1;
    if(FASTCKSUM_FAILED==fill_fcksum_body(fcksum_body, argv[2], FCKSUM_KERNEL))
    {
        printf("genfcksum: generate fast checksum for %s failed\n", argv[2]);
        goto FREE_AND_RET;
    }
    //root1
    if(FASTCKSUM_FAILED==fill_fcksum_body(fcksum_body, argv[3], FCKSUM_SQASHFS))
    {
        printf("genfcksum: generate fast checksum for %s failed\n", argv[3]);
        goto FREE_AND_RET;
    }
    fcksum_body->reserved |= (RTL_FASTCKSUM_FIELD_ROOT_VALID | RTL_FASTCKSUM_FIELD_LINUX_VALID);
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
    if(argc==6)
    {
        fcksum_body++;
        fcksum_body->bank = 2;
        //linux2
        if(FASTCKSUM_FAILED==fill_fcksum_body(fcksum_body, argv[4], FCKSUM_KERNEL))
        {
            printf("genfcksum: generate fast checksum for %s failed\n", argv[4]);
            goto FREE_AND_RET;
        }
        //root2
        if(FASTCKSUM_FAILED==fill_fcksum_body(fcksum_body, argv[5], FCKSUM_SQASHFS))
        {
            printf("genfcksum: generate fast checksum for %s failed\n", argv[5]);
            goto FREE_AND_RET;
        }
        fcksum_body->reserved |= (RTL_FASTCKSUM_FIELD_ROOT_VALID | RTL_FASTCKSUM_FIELD_LINUX_VALID);
    }
#endif
    printf("genfcksum fillin content: OK\n");

    /* write to out file */
    dump_fastcksum_body(fcksum_hdr);
    if( fwrite(fcksum_hdr, sizeof(char), sizeof(FASTCKSUM_HEADER_T) + bank*sizeof(FASTCKSUM_PAYLOAD_T), out) <=0 )
    {
        printf("genfcksum: write to file failed\n");
        goto FREE_AND_RET;
    }
    printf("genfcksum write out file: OK\n");

    ret = FASTCKSUM_SUCCESS;

    /* exit */
FREE_AND_RET:
    if(fcksum_hdr)
    {
        free(fcksum_hdr);
        fcksum_hdr = NULL;
    }
    if(out)
    {
        fclose(out);
        out = NULL;
    }
RET:
    printf("genfcksum return %d\n", ret);
    return ret;
}
#endif/*CONFIG_RTL_FAST_CHECKSUM_ENABLE*/
