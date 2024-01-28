#ifndef _CLUSTER_COMMON_H
#define _CLUSTER_COMMON_H
#include <stdio.h>
#include "apmib.h"

extern unsigned char debug_level;
#define CLUSTER_DEBUG_LEVEL_FILE	"/var/cluster_debug"
#define debug_print(f,args...) \
do{\
    if(debug_level){\
		printf("[%s %d] ",__func__,__LINE__);\
    	printf(f,##args);\
    }\
}while(0)
extern int wlan_idx;	// interface index
//interface for sending cluster packet
#define CLU_CLIENT_INTERFACE "br0"

/*

 ***************************************************
 ***           cluster packet format             ***
 
 | DMAC | SMAC | PROTO NUM | CLU_HDR |   PAYLOAD   |
 |   6  |   6  |     2     | sizeof()| CLU_PAYLOAD |
 |                                                 |
 | <----------- CLUSTER_PKT_MAX_LEN -------------> |
 ***************************************************

 Note:
 	1. CLU_PAYLOAD in each packet is for the wlan/vwlan idx specified in CLU_HDR. 
 	2. checksum and totlen in CLU_HDR covers the whole packet excluding DMAC/SMAC/PROTO NUM.
 */
#define CLUSTER_PKT_MAX_LEN 	(1400)
#define CLUSTER_MANAGE_PROTOCOL (0xDCBA)
//cluster header format
typedef struct cluster_header_s{
    unsigned short checksum;
    unsigned short totlen;
    unsigned short sequence;
    unsigned short wlan_idx;
    unsigned short vwlan_idx;
    unsigned short more_fragment;
    unsigned short frag_id;
}CLU_HDR_T, *CLU_HDR_Tp;
//cluster payload format, matching data structure in MIBDEF
typedef struct cluster_payload_s{
	// ssid
	unsigned char ssid[MAX_SSID_LEN];
	
	// channel
	unsigned char channel;
	
	// bandwith
	unsigned char channelbonding;
	unsigned char CoexistEnabled;
	
	// encrypt
	unsigned char authType;
	unsigned char enable1X;
	// encrypt(wep)
	unsigned char wep;
	unsigned char encrypt;
	unsigned char wepKeyType;
	unsigned char wep64Key1[WEP64_KEY_LEN];
	unsigned char wep64Key2[WEP64_KEY_LEN];
	unsigned char wep64Key3[WEP64_KEY_LEN];
	unsigned char wep64Key4[WEP64_KEY_LEN];
	unsigned char wep128Key1[WEP128_KEY_LEN];
	unsigned char wep128Key2[WEP128_KEY_LEN];
	unsigned char wep128Key3[WEP128_KEY_LEN];
	unsigned char wep128Key4[WEP128_KEY_LEN];
	// encrypt(wpa)
	unsigned char macAuthEnabled;
	unsigned char wpaAuth;
	unsigned char wpaCipher;
	unsigned char wpa2Cipher;
	unsigned char wpa2PreAuth;
	unsigned char wpaPSKFormat;
	unsigned char wpaPSK[MAX_PSK_LEN+1];
	// encrypt(1X)?
	
	// filter
	unsigned char acEnabled;
	unsigned char acNum;
	MACFILTER_T	  acAddrArray[MAX_WLAN_AC_NUM];
}CLU_PAYLOAD_T, *CLU_PAYLOAD_Tp;

unsigned short get_checksum(unsigned char *ptr, unsigned short len);

#endif
