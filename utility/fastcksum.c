#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "fastcksum.h"

#ifdef CONFIG_RTL_FAST_CHECKSUM_ENABLE
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
		MOD28(sum2);			/* only added so many BASE's */
		return adler | (sum2 << 16);
	}

	/* do length NMAX blocks -- requires just one modulo operation */
	while (len >= NMAX) {
		len -= NMAX;
		n = NMAX / 16;			/* NMAX is divisible by 16 */
		do {
			DO16(buf);			/* 16 sums unrolled */
			buf += 16;
		} while (--n);
		MOD(adler);
		MOD(sum2);
	}

	/* do remaining bytes (less than NMAX, still just one modulo) */
	if (len) {					/* avoid modulos if none remaining */
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

static int get_fastcksum_mtd_name(char *name)
{
	if(NULL==name)
		return FASTCKSUM_FAILED;

#ifdef MTD_NAME_MAPPING
	rtl_name_to_mtdblock(RTL_FASTCKSUM_MTD_NAME, name);
#else
#error "get_fastcksum_mtd_name: CONFIG_RTL_FAST_CHECKSUM_ENABLE=y, MUST turn on CONFIG_MTD_NAME_MAPPING!!!"
#endif

	FCS_DBG("[%s:%d] fastcksum mtd name:%s\n", __FUNCTION__, __LINE__, name);
	return FASTCKSUM_SUCCESS;
}

/*
 * gen_fastcksum():
 *  Generate fast checksum.
 *	Buffer contains of payload length(int) + old checksum of linux or root(short) + magic number(int).
 *	Use this buffer to perform Adler-32 and get an unsigned int fast checksum.
 *	Note:
 *		payload_len: the exact len field value of IMG_HEADER_T stored in flash(should with big endian, check cvimg.c)
 *		old_cksum: the exact old checksum value stored in flash(should with big endian, check cvimg.c).
 */
int gen_fastcksum(unsigned int *cksum, int payload_len, short old_cksum)
{
	unsigned char buf[16] = {0};
	int at = 0, i = 0;
	unsigned int tmp = 0;

	if(NULL==cksum)
		return FASTCKSUM_FAILED;

	FCS_DBG("[%s:%d]payload len: 0x%x, old_cksum:0x%x\n", __FUNCTION__, __LINE__, payload_len, old_cksum);
	
	*((int *)(buf+at)) = payload_len;
	at += sizeof(int);
	*((short *)(buf+at)) = old_cksum;
	at += sizeof(short);
	*((int *)(buf+at)) = htonl(RTL_FASTCKSUM_ALGO_MAGIC);
	at += sizeof(int);

	tmp = adler32(0L, NULL, 0); 
    tmp = adler32((unsigned long *)tmp, buf, at);
	*cksum = DWORD_SWAP(tmp);

    FCS_DBG("[%s:%d]buf:\n", __FUNCTION__, __LINE__);
    for(; i < at; i++)
    {   
        FCS_DBG("%02x ", buf[i]);
    }   
    FCS_DBG("\n[%s:%d]cksum:0x%x\n", __FUNCTION__, __LINE__, *cksum);

	return FASTCKSUM_SUCCESS;
}

int read_fastcksum_from_flash(unsigned char *buf, int len)
{
	char mtd[32]={0};
	int fd=-1, read_num=0, ret=FASTCKSUM_FAILED;
	int i=0;

	if(NULL==buf || len<=0)
		goto RETURN;

	if(get_fastcksum_mtd_name(mtd)==FASTCKSUM_FAILED)
		goto RETURN;
	
	fd = open(mtd, O_RDONLY);
	if(fd<0)
		goto RETURN;

	read_num = read(fd, buf, len);
	if(read_num<0)
	{
		perror("read fastcksum mtd: ");
		goto CLOSE_AND_RETURN;
	}
	FCS_DBG("[%s:%d] read %d bytes\n", __FUNCTION__, __LINE__, read_num);
	FCS_DBG("buf:\n");
	for(i=0; i<read_num; i++)
		FCS_DBG("%02x ", buf[i]);
	FCS_DBG("\n");

	ret = FASTCKSUM_SUCCESS;
CLOSE_AND_RETURN:
	close(fd);
RETURN:
	return ret;
}

int write_fastcksum_to_flash(unsigned char *buf, int len)
{
	char mtd[32]={0};
	int fd=-1, write_num=0, ret=FASTCKSUM_FAILED;
	int i=0;

	if(NULL==buf || len<=0)
		goto RETURN;

	if(get_fastcksum_mtd_name(mtd)==FASTCKSUM_FAILED)
		goto RETURN;
	
	fd = open(mtd, O_RDWR);
	if(fd<0)
		goto RETURN;

	write_num = write(fd, buf, len);
	if(write_num<0)
	{
		perror("read fastcksum mtd: ");
		goto CLOSE_AND_RETURN;
	}
	FCS_DBG("[%s:%d] write_num %d bytes\n", __FUNCTION__, __LINE__, write_num);
	FCS_DBG("buf:\n");
	for(i=0; i<write_num; i++)
		FCS_DBG("%02x ", buf[i]);
	FCS_DBG("\n");

	ret = FASTCKSUM_SUCCESS;
CLOSE_AND_RETURN:
	close(fd);
RETURN:
	return ret;
}

void dump_fastcksum_struct(FASTCKSUM_HEADER_Tp hdr)
{
	FASTCKSUM_PAYLOAD_Tp body = NULL;
	int i=0, count=0;
	
	if(hdr==NULL)
		return ;

	FCS_DBG("Dump fastchecksum structure:\n");
	FCS_DBG("\tlen:%d, sizeof(FASTCKSUM_PAYLOAD_T):%d\n", hdr->len, sizeof(FASTCKSUM_PAYLOAD_T));
	if(hdr->len%sizeof(FASTCKSUM_PAYLOAD_T))
	{
		printf("WARNING: invalid length, exit dump!!\n");
		return;
	}
	count = hdr->len/sizeof(FASTCKSUM_PAYLOAD_T);
	for(i=0; i<count; i++)
	{
		body = ((FASTCKSUM_PAYLOAD_Tp)hdr->payload)+i;
		FCS_DBG("\t[%d]:\n", i);
		FCS_DBG("\t\t Bank:%d\n", body->bank);
		FCS_DBG("\t\t kernel sig:%02x%02x%02x%02x\n", body->linux_sig[0], body->linux_sig[1], body->linux_sig[2], body->linux_sig[3]);
		FCS_DBG("\t\t kernel cksum:0x%x\n", body->linux_cksum);
		FCS_DBG("\t\t root sig:%02x%02x%02x%02x\n", body->root_sig[0], body->root_sig[1], body->root_sig[2], body->root_sig[3]);
		FCS_DBG("\t\t root cksum:0x%x\n", body->root_cksum);
		FCS_DBG("\t\t reserved bitvalue:0x%x\n", body->reserved);
	}
	return ;
}

int revalidation(unsigned char *mtd, int offset, int size, int *valid)
{
	unsigned char *buf = NULL;
	int fh=-1, ret=FASTCKSUM_FAILED, i=0;
	unsigned short sum = 0;

	if(mtd==NULL || size<=0 || offset<0 || valid==NULL)
		goto REVALIDAION_END;
	else
		FCS_DBG("[%s:%d] mtd:%s, offset:0x%x, size:0x%x\n", __FUNCTION__, __LINE__, mtd, offset, size);
	
	if((fh=open(mtd, O_RDONLY))<0)
	{
		printf("[%s:%d] open %s failed!\n", __FUNCTION__, __LINE__, mtd);
		goto REVALIDAION_END;
	}

	if(lseek(fh, offset, SEEK_SET) == -1)//fix converity error:CHECKED_RETURN
	{
		printf("[%s:%d] lseek %s failed!\n", __FUNCTION__, __LINE__, mtd);
		goto REVALIDAION_END;
	}

	buf = (unsigned char *)malloc(size);
	if(buf==NULL)
	{
		printf("[%s:%d] malloc 0x%x bytes failed!\n", __FUNCTION__, __LINE__, size);
		goto REVALIDAION_END;
	}

	if(read(fh, buf, size)<=0)
	{
		perror("revalidation read:");
		goto REVALIDAION_END;
	}

	for(i=0; i<size; i+=2)
		sum +=((unsigned short)(((*(buf+1+i))|((*(buf+i))<<8))&0xffff));

	if(sum)
		*valid = 0;
	else
		*valid = 1;
	ret = FASTCKSUM_SUCCESS;

	FCS_DBG("[%s:%d] valid:%d\n", __FUNCTION__, __LINE__, *valid);
REVALIDAION_END:
	if(fh>0)
		close(fh);
	if(buf)
		free(buf);
	return ret;
}

#endif /*CONFIG_RTL_FAST_CHECKSUM_ENABLE*/
