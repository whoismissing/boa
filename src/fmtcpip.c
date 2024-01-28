/*
 *      Web server handler routines for TCP/IP stuffs
 *
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *
 *      $Id: fmtcpip.c,v 1.24 2009/08/24 10:31:08 bradhuang Exp $
 *
 */

/*-- System inlcude files --*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <time.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>

/*-- Local inlcude files --*/
#include "boa.h"
#include "globals.h"
#include "apmib.h"
#include "apform.h"
#include "utility.h"
#include "mibtbl.h"
#include "asp_page.h"

#define _DHCPD_PROG_NAME	"udhcpd"
#define _DHCPD_PID_PATH		"/var/run"
#define _DHCPC_PROG_NAME	"udhcpc"
#define _DHCPC_PID_PATH		"/etc/udhcpc"
#define _PATH_DHCPS_LEASES	"/var/lib/misc/udhcpd.leases"


/*-- Macro declarations --*/
#ifdef _LITTLE_ENDIAN_
#define ntohdw(v) ( ((v&0xff)<<24) | (((v>>8)&0xff)<<16) | (((v>>16)&0xff)<<8) | ((v>>24)&0xff) )

#else
#define ntohdw(v) (v)
#endif

#define RECONNECT_MSG(url) { \
	req_format_write(wp, "<html><body><blockquote><h4>Change setting successfully!<BR><BR>If IP address was modified, you have to re-connect the WebServer" \
		"<BR>with the new address.<BR><BR>" \
                "<form><input type=button value=\"  OK  \" OnClick=window.location.replace(\"%s\")></form></blockquote></body></html>", url);\
}

/*-- Forward declarations --*/
static int getOneDhcpClient(char **ppStart, unsigned long *size, char *ip, char *mac, char *liveTime, char *hostname);

#if 0
static DHCP_T wanDhcpTmp=(DHCP_T)-1;
#endif

//////////////////////////////////////////////////////////////////////////////
static int isValidName(char *str)
{
	int i, len=strlen(str);

	for (i=0; i<len; i++) {
		if (str[i] == ' ' || str[i] == '"' || str[i] == '\x27' || str[i] == '\x5c' || str[i] == '$')
			return 0;
	}
	return 1;
}

//////////////////////////////////////////////////////////////////////////////
static int getOneDhcpClient(char **ppStart, unsigned long *size, char *ip, char *mac, char *liveTime, char *hostname)
{
	struct dhcpOfferedAddr {
        	u_int8_t chaddr[16];
        	u_int32_t yiaddr;       /* network order */
        	u_int32_t expires;      /* host order */
		char hostname[64]; 
	};

	struct dhcpOfferedAddr entry;
	 u_int8_t empty_haddr[16]; 
    
     	memset(empty_haddr, 0, 16); 
	if ( *size < sizeof(entry) )
		return -1;

	entry = *((struct dhcpOfferedAddr *)*ppStart);
	*ppStart = *ppStart + sizeof(entry);
	*size = *size - sizeof(entry);

	if (entry.expires == 0)
		return 0;

	if(!memcmp(entry.chaddr, empty_haddr, 16)){
		//fprintf(stderr, "got a unavailable entry for ip=%s\n",inet_ntoa(*((struct in_addr *)&entry.yiaddr)));
		return 0;
	}
	strcpy(ip, inet_ntoa(*((struct in_addr *)&entry.yiaddr)) );
	snprintf(mac, 20, "%02x:%02x:%02x:%02x:%02x:%02x",
			entry.chaddr[0],entry.chaddr[1],entry.chaddr[2],entry.chaddr[3],
			entry.chaddr[4], entry.chaddr[5]);
	if(entry.expires == 0xffffffff)
    	sprintf(liveTime,"%s", "-");
   else
		snprintf(liveTime, 10, "%lu", (unsigned long)ntohl(entry.expires));

	if(entry.hostname[0])
		strcpy(hostname, entry.hostname);
	else
		strcpy(hostname, "unkown");

	return 1;
}


///////////////////////////////////////////////////////////
int getPid(char *filename)
{
	struct stat status;
	char buff[100];
	FILE *fp;

	if ( stat(filename, &status) < 0)
		return -1;
	fp = fopen(filename, "r");
	if (!fp) {
        	fprintf(stderr, "Read pid file error!\n");
		return -1;
   	}
	fgets(buff, 100, fp);
	fclose(fp);

	return (atoi(buff));
}
int tcpipLanHandler(request *wp, char *tmpBuf)
{
	char	*strIp, *strMask, *strGateway, *strDhcp, *strStp, *strMac, *strDNS, *strDomain, *strDhcpLeaseTime, *strWacBand;
    char    wlan_name[16]={0};
	struct in_addr inIp, inMask, inGateway;
	DHCP_T dhcp, curDhcp;	
	
	int stp;
	char	*strdhcpRangeStart, *strdhcpRangeEnd;
	struct in_addr dhcpRangeStart, dhcpRangeEnd;
	struct in_addr dns1, dns2, dns3;	
	int call_from_wizard = 0;
	int lan_dhcp_mode=0;
	int dhcp_lease_time=0;
	strDhcp = req_get_cstream_var(wp, ("dhcp"), "");
	if (!strDhcp[0])
		call_from_wizard = 1;

	// Set STP
	strStp = req_get_cstream_var(wp, ("stp"), "");
	if (strStp[0]) {
		if (strStp[0] == '0')
			stp = 0;
		else
			stp = 1;
		if ( !apmib_set(MIB_STP_ENABLED, (void *)&stp)) {
			strcpy(tmpBuf, ("Set STP mib error!"));
			goto setErr_tcpip;
		}
	}

#if 0 // Move to formStaticDHCP
	// Set static DHCP
	strStp = req_get_cstream_var(wp, ("static_dhcp"), "");
	if (strStp[0]) {
		if (strStp[0] == '0')
			stp = 0;
		else
			stp = 1;
		if ( !apmib_set(MIB_DHCPRSVDIP_ENABLED, (void *)&stp)) {
			strcpy(tmpBuf, ("Set static DHCP mib error!"));
			goto setErr_tcpip;
		}
	}
#endif

#if defined(CONFIG_APP_APPLE_MFI_WAC)
    // Set MFI WAC Band
    strWacBand = req_get_cstream_var(wp, ("mfiWacId"), "");  
    //printf("[%s:%d] tmpWacBand %s\n", __FUNCTION__, __LINE__, strWacBand);
    if(strWacBand)
    {
        int wacBand = 0, orig_wacBand;
        apmib_get(MIB_MFI_WAC_BAND, (void *) &orig_wacBand);
        if(!strcmp(strWacBand, "0"))
            // 5G
            wacBand = 5;
        else if(!strcmp(strWacBand, "1"))
            // 2.4G
            wacBand = 2;
        apmib_set(MIB_MFI_WAC_BAND,  (void *) &wacBand);
    }

#endif

	// Set clone MAC address
	strMac = req_get_cstream_var(wp, ("lan_macAddr"), "");
	if (strMac[0]) {
		int orig_wlan_idx=0;
		int orig_vwlan_idx=0;
		int i;
		int j;
		if (strlen(strMac)!=12 || !string_to_hex(strMac, tmpBuf, 12)) {
			strcpy(tmpBuf, ("<script>dw(tcpip_lan_clone_mac_error)</script>"));
			goto setErr_tcpip;
		}
		if ( !apmib_set(MIB_ELAN_MAC_ADDR, (void *)tmpBuf)) {
			strcpy(tmpBuf, ("Set MIB_ELAN_MAC_ADDR mib error!"));
			goto setErr_tcpip;
		}
		
		orig_wlan_idx=wlan_idx;
		orig_vwlan_idx=vwlan_idx;
#if defined(CONFIG_RTL_8197F) && defined(CONFIG_APP_APPLE_MFI_WAC)
            int interface_selected=0, wac_band_selected=0;
            apmib_get(MIB_MFI_WAC_BAND, (void *)&wac_band_selected);
            interface_selected = (wac_band_selected==2)?1:0;
#endif
		if( !memcmp(strMac,"000000000000",12))
		{
			for(i=0;i<NUM_WLAN_INTERFACE;i++)
			{
				wlan_idx=i;
#if defined(CONFIG_APP_APPLE_MFI_WAC)//set wlan ssid
                sprintf(wlan_name, "wlan%s", wlan_idx);
				unsigned char ssid_str[64];
				unsigned char* ptr = NULL;
				unsigned char hwmac_addr[6];
                if(SetWlan_idx(wlan_name))
                {
                    apmib_get(MIB_HW_WLAN_ADDR,  (void *)hwmac_addr);	
                    apmib_get(MIB_WLAN_SSID,ssid_str);
                    if(ptr = strstr(ssid_str,"WAC_"))
                        *ptr = '\0';
                    sprintf(ssid_str+strlen(ssid_str),"WAC_%02X%02X%02X",(unsigned char)hwmac_addr[3],(unsigned char)hwmac_addr[4],(unsigned char)hwmac_addr[5]);
                    if ( !apmib_set(MIB_WLAN_SSID, (void *)ssid_str)) {
                        strcpy(tmpBuf, ("Set MIB_WLAN_SSID mib error!"));
                        goto setErr_tcpip;
                    }
                }
#ifdef CONFIG_RTL_8197F
                if(i==interface_selected)
#else
				if(0 == i)
#endif
                {
					sprintf(ssid_str,"Realtek_WAC_%02X%02X%02X",(unsigned char)hwmac_addr[3],(unsigned char)hwmac_addr[4],(unsigned char)hwmac_addr[5]);
					if ( !apmib_set(MIB_MFI_WAC_DEVICE_NAME, (void *)ssid_str)) {
							strcpy(tmpBuf, ("Set MIB_MFI_WAC_DEVICE_NAME mib error!"));
							goto setErr_tcpip;
					}
				}
#endif					
				for(j=0;j<NUM_VWLAN_INTERFACE;j++)
				{
					vwlan_idx=j;
					if ( !apmib_set(MIB_WLAN_WLAN_MAC_ADDR, (void *)tmpBuf)) {
						strcpy(tmpBuf, ("Set MIB_WLAN_WLAN_MAC_ADDR mib error!"));
						goto setErr_tcpip;
					}
				}
			}
		}
		else
		{
			for(i=0;i<NUM_WLAN_INTERFACE;i++)
			{
				wlan_idx=i;
#if defined(CONFIG_APP_APPLE_MFI_WAC)//set wlan ssid
				unsigned char ssid_str[64];
				unsigned char* ptr = NULL;
                sprintf(wlan_name, "wlan%d", wlan_idx);
                
                if(SetWlan_idx(wlan_name))
                {
                    apmib_get(MIB_WLAN_SSID,ssid_str);
                    if(ptr = strstr(ssid_str,"WAC_"))
                        *ptr = '\0';
                    sprintf(ssid_str+strlen(ssid_str),"WAC_%02X%02X%02X",(unsigned char)tmpBuf[3],(unsigned char)tmpBuf[4],(unsigned char)tmpBuf[5]);
                    if ( !apmib_set(MIB_WLAN_SSID, (void *)ssid_str)) {
                        strcpy(tmpBuf, ("Set MIB_WLAN_SSID mib error!"));
                        goto setErr_tcpip;
                    }
                }
#ifdef CONFIG_RTL_8197F
                if(i==interface_selected)
#else
				if(0 == i)
#endif
                {
					sprintf(ssid_str,"Realtek_WAC_%02X%02X%02X",(unsigned char)tmpBuf[3],(unsigned char)tmpBuf[4],(unsigned char)tmpBuf[5]);
                    if ( !apmib_set(MIB_MFI_WAC_DEVICE_NAME, (void *)ssid_str)) {
							strcpy(tmpBuf, ("Set MIB_MFI_WAC_DEVICE_NAME mib error!"));
							goto setErr_tcpip;
					}
				}
#endif					
				for(j=0;j<NUM_VWLAN_INTERFACE;j++)
				{
					vwlan_idx=j;
					if ( !apmib_set(MIB_WLAN_WLAN_MAC_ADDR, (void *)tmpBuf)) {
						strcpy(tmpBuf, ("Set MIB_WLAN_WLAN_MAC_ADDR mib error!"));
						goto setErr_tcpip;
					}
				tmpBuf[5]++;
			}
			tmpBuf[5]-=NUM_VWLAN_INTERFACE;
			tmpBuf[5]+=0x10;
			}
		}
		
		wlan_idx=orig_wlan_idx;
		vwlan_idx=orig_vwlan_idx;
	}

	// Read current DHCP setting for reference later
	if ( !apmib_get( MIB_DHCP, (void *)&curDhcp) ) {
		strcpy(tmpBuf, ("Get DHCP MIB error!"));
		goto setErr_tcpip;
	}

	strDhcp = req_get_cstream_var(wp, ("dhcp"), "");
	if ( strDhcp[0] ) {
		lan_dhcp_mode = atoi(strDhcp);
		
		if(lan_dhcp_mode != 0 && lan_dhcp_mode != 1 && lan_dhcp_mode != 2 && lan_dhcp_mode != DHCP_LAN_RELAY && lan_dhcp_mode != 15  && lan_dhcp_mode != 19){
			strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_dhcp_value)</script>"));
			goto setErr_tcpip;
		}

		if ( !apmib_set(MIB_DHCP, (void *)&lan_dhcp_mode)) {
	  		strcpy(tmpBuf, ("Set DHCP error!"));
			goto setErr_tcpip;
		}
		dhcp = lan_dhcp_mode;
	}
	else
		dhcp = curDhcp;

	if ( dhcp == DHCP_DISABLED || dhcp == DHCP_SERVER || dhcp == DHCP_LAN_RELAY || dhcp == DHCP_AUTO || DHCP_AUTO_WAN) {
		strIp = req_get_cstream_var(wp, ("lan_ip"), "");
		if ( strIp[0] ) {
			if ( !inet_aton(strIp, &inIp) ) {
				strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_ip_value)</script>"));
				goto setErr_tcpip;
			}
			if ( !apmib_set( MIB_IP_ADDR, (void *)&inIp)) {
				strcpy(tmpBuf, ("Set IP-address error!"));
				goto setErr_tcpip;
			}
		}
		else { // get current used IP
			if ( !getInAddr(BRIDGE_IF, IP_ADDR, (void *)&inIp) ) {
				strcpy(tmpBuf, ("Get IP-address error!"));
				goto setErr_tcpip;
			}
		}

		strMask = req_get_cstream_var(wp, ("lan_mask"), "");
		if ( strMask[0] ) {
			if ( !inet_aton(strMask, &inMask) ) {
				strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_subnet_value)</script>"));
				goto setErr_tcpip;
			}
			if ( !apmib_set(MIB_SUBNET_MASK, (void *)&inMask)) {
				strcpy(tmpBuf, ("Set subnet-mask error!"));
				goto setErr_tcpip;
			}
		}
		else { // get current used netmask
			if ( !getInAddr(BRIDGE_IF, SUBNET_MASK, (void *)&inMask )) {
				strcpy(tmpBuf, ("Get subnet-mask error!"));
				goto setErr_tcpip;
			}
		}
		strGateway = req_get_cstream_var(wp, ("lan_gateway"), "");
		if ( (dhcp == DHCP_DISABLED && strGateway[0]) || 
			(dhcp == DHCP_SERVER && strGateway[0]) ||
			(dhcp == DHCP_LAN_RELAY && strGateway[0])  ) {
			if ( !inet_aton(strGateway, &inGateway) ) {
				strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_defgw_value)</script>"));
				goto setErr_tcpip;
			}
			if ( !apmib_set(MIB_DEFAULT_GATEWAY, (void *)&inGateway)) {
				strcpy(tmpBuf, ("Set default-gateway error!"));
				goto setErr_tcpip;
			}
		}
			
#ifdef CONFIG_APP_DHCPRELAY
		if(dhcp == DHCP_LAN_RELAY)
		{
			char *strAccessServerWanIdx=NULL, *strServerIp=NULL;
			unsigned char wan_idx=0;
			struct in_addr serverIP;
			strAccessServerWanIdx = req_get_cstream_var(wp, ("access_relay_server_wan_idx"), "");
			if(strAccessServerWanIdx[0])
			{
				wan_idx = atoi(strAccessServerWanIdx);
				if(wan_idx<1 || wan_idx>WANIFACE_NUM)
				{
					strcpy(tmpBuf, "Invalid wan index!");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_DHCP_RELAY_WAN_IDX, (void *)&wan_idx)) 
				{
	  				strcpy(tmpBuf, "Set DHCP Relay WAN index error!");
					goto setErr_tcpip;
				}				
			}
			
			strServerIp = req_get_cstream_var(wp, ("relay_dhcp_server_ip"), "");
			if(strServerIp[0])
			{
				if ( !inet_aton(strServerIp, &serverIP) ) 
				{
					strcpy(tmpBuf, "Invalid DHCP Server IP");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_DHCP_RELAY_SERVER_IP, (void *)&serverIP)) 
				{
					strcpy(tmpBuf, ("Set DHCP Relay Server IP error!"));
					goto setErr_tcpip;
				}
			}			
		}
#endif

		if ( dhcp == DHCP_SERVER|| dhcp == DHCP_AUTO || DHCP_AUTO_WAN) {
			// Get/Set DHCP client range
			strdhcpRangeStart = req_get_cstream_var(wp, ("dhcpRangeStart"), "");
			if ( strdhcpRangeStart[0] ) {
				if ( !inet_aton(strdhcpRangeStart, &dhcpRangeStart) ) {
					strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_dhcpstart)</script>"));
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_DHCP_CLIENT_START, (void *)&dhcpRangeStart)) {
					strcpy(tmpBuf, ("Set DHCP client start address error!"));
					goto setErr_tcpip;
				}
			}
			strdhcpRangeEnd = req_get_cstream_var(wp, ("dhcpRangeEnd"), "");
			if ( strdhcpRangeEnd[0] ) {
				if ( !inet_aton(strdhcpRangeEnd, &dhcpRangeEnd) ) {
					strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_dhcpend)</script>"));
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_DHCP_CLIENT_END, (void *)&dhcpRangeEnd)) {
					strcpy(tmpBuf, ("Set DHCP client end address error!"));
					goto setErr_tcpip;
				}
			}

			if ( strdhcpRangeStart[0] && strdhcpRangeEnd[0] ) {
				unsigned long start, end, mask, ip;
				int diff;

				start = *((unsigned long *)&dhcpRangeStart);
				end = *((unsigned long *)&dhcpRangeEnd);
				diff = (int) ( ntohdw(end) - ntohdw(start) );
				ip = *((unsigned long *)&inIp);
				mask = *((unsigned long *)&inMask);
				if (diff <= 0 ||
					diff > (~mask) ||
					(ip&mask) != (start&mask) ||
					(ip&mask) != (end& mask) ) {
					strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_dhcprange)</script>"));
					goto setErr_tcpip;
				}
			}

			// If DHCP server is enabled in LAN, update dhcpd.conf
			strDNS = req_get_cstream_var(wp, ("dns1"), "");
			if ( strDNS[0] ) {
				if ( !inet_aton(strDNS, &dns1) ) {
					strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_dns_value)</script>"));
					goto setErr_tcpip;
				}

				if ( !apmib_set(MIB_DNS1, (void *)&dns1)) {
	  				strcpy(tmpBuf, "Set DNS MIB error!");
					goto setErr_tcpip;
				}
			}

			strDNS = req_get_cstream_var(wp, ("dns2"), "");
			if ( strDNS[0] ) {
				if ( !inet_aton(strDNS, &dns2) ) {
					strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_dns_value)</script>"));
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_DNS2, (void *)&dns2)) {
	  				strcpy(tmpBuf, "Set DNS MIB error!");
					goto setErr_tcpip;
				}
			}

			strDNS = req_get_cstream_var(wp, ("dns3"), "");
			if ( strDNS[0] ) {
				if ( !inet_aton(strDNS, &dns3) ) {
					strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_dns_value)</script>"));
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_DNS3, (void *)&dns3)) {
	  				strcpy(tmpBuf, "Set DNS MIB error!");
					goto setErr_tcpip;
				}
			}

			if (!call_from_wizard) {
				strDhcpLeaseTime = req_get_cstream_var(wp, ("dhcpLeaseTime"), "");
	                if ( strDhcpLeaseTime ) {
				dhcp_lease_time = atoi(strDhcpLeaseTime);
                    		if ( !apmib_set(MIB_DHCP_LEASE_TIME, (void *)&dhcp_lease_time)) {
		                        strcpy(tmpBuf, ("Set MIB_DHCP_LEASE_TIME MIB error!"));
                	        goto setErr_tcpip;
	                    }
        	        }
				strDomain = req_get_cstream_var(wp, ("domainName"), "");
				if ( strDomain[0] ) {
					if (!isValidName(strDomain)) {
  						strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_domain)</script>"));
						goto setErr_tcpip;				
					}							
					if ( !apmib_set(MIB_DOMAIN_NAME, (void *)strDomain)) {
	  					strcpy(tmpBuf, ("Set MIB_DOMAIN_NAME MIB error!"));
						goto setErr_tcpip;
					}
				}
				#if 0
				else{
					 if ( !apmib_set(MIB_DOMAIN_NAME, (void *)"")){
	  					strcpy(tmpBuf, ("\"Set MIB_DOMAIN_NAME MIB error!\""));
						goto setErr_tcpip;
					}	
				}
				#endif
			}			
		}
	}
	return 0 ;
setErr_tcpip:
	return -1 ;	
}

///////////////////////////////////////////////////////////////////
#if defined(MIB_TLV)
extern int mib_search_by_id(const mib_table_entry_T *mib_tbl, unsigned short mib_id, unsigned char *pmib_num, const mib_table_entry_T **ppmib, unsigned int *offset);
extern mib_table_entry_T mib_root_table[];
#else
extern int update_linkchain(int fmt, void *Entry_old, void *Entry_new, int type_size);
#endif

int checkStaticIpIsValid(char *tmpBuf)
{
	int i, entryNum=0, enabled=0;
	DHCPRSVDIP_T entry;
	struct in_addr start_ip, end_ip, router_ip;
	
	apmib_get(MIB_DHCPRSVDIP_ENABLED, (void *)&enabled);
	if(enabled==0)
		return 0;
	
	apmib_get(MIB_DHCPRSVDIP_TBL_NUM, (void *)&entryNum);
	apmib_get(MIB_DHCP_CLIENT_START,  (void *)&start_ip);
	apmib_get(MIB_DHCP_CLIENT_END,  (void *)&end_ip);
	apmib_get(MIB_IP_ADDR,  (void *)&router_ip);
	
	for (i=1; i<=entryNum; i++) 
	{
		*((char *)&entry) = (char)i;
		if(!apmib_get(MIB_DHCPRSVDIP_TBL, (void *)&entry))
		{
			printf("get mib MIB_DHCPRSVDIP_TBL fail!\n");
			return 0;
		}		
		if((*((struct in_addr*)entry.ipAddr)).s_addr<start_ip.s_addr || (*((struct in_addr*)entry.ipAddr)).s_addr>end_ip.s_addr || (*((struct in_addr*)entry.ipAddr)).s_addr==router_ip.s_addr)
		{
			strcpy(tmpBuf, ("<script>dw(tcpip_check_dhcp)</script>"));
			return 1;	
		}
	}	
	return 0;
}
#if defined(MULTI_WAN_SUPPORT)&& defined(CONFIG_IPV6)
void radvdHandler(request *wp, char *path, char *query)
{
	int pid;
	char tmpBuf[256];
	char *submitUrl;
	char* value;
	radvdCfgParam_t radvdCfgParam;
	/*Get parameters*/
	getRadvdInfo(&radvdCfgParam);
	
	/*Set parameters*/ 
	set_RadvdParam(wp, path, query,&radvdCfgParam); 
	
	/*Set to pMIb*/
	apmib_set(MIB_IPV6_RADVD_PARAM,&radvdCfgParam);

	return;
	
}

void dhcp6sHandler(request *wp, char *path, char *query)
{
	dhcp6sCfgParam_t dhcp6sCfgParam;
	addr6CfgParam_t addr6;
	char tmpBuf[256];
	char *submitUrl;
	char* value;
	int val;
#ifdef CONFIG_IPV6_CE_ROUTER_SUPPORT
	addr6CfgParam_t ula_addr6;
#endif
	
	/*Get parameters**/
	getDhcpv6sInfo(&dhcp6sCfgParam);
 	/*Set to Parameters*/
 	value=req_get_cstream_var(wp,"ipv6lanOrigin","");
	val = atoi(value);//0:auto 1:mode
	apmib_set(MIB_IPV6_LAN_ORIGIN,&val);
 
	set_lan_addr6(wp, path, query,&addr6);
	apmib_set(MIB_IPV6_ADDR_LAN_PARAM,&addr6);
	
	set_DhcpSParam(wp, path, query,&dhcp6sCfgParam);
	apmib_set(MIB_IPV6_DHCPV6S_PARAM,&dhcp6sCfgParam);

#ifdef CONFIG_IPV6_CE_ROUTER_SUPPORT
	/* ula address */
	value=req_get_cstream_var(wp,"enable_ula","");
	val = atoi(value);
	apmib_set(MIB_IPV6_ULA_ENABLE,&val);

	value=req_get_cstream_var(wp,"ula_mode","");
	if(!strcmp(value,"ulaManual"))
		val = 0;
	else
		val = 1;
	apmib_set(MIB_IPV6_ULA_MODE,&val);

	if(val == 0)
	{
		set_ula_addr6(wp, path, query,&ula_addr6);
		apmib_set(MIB_IPV6_ADDR_ULA_PARAM,&ula_addr6);
	}
	else
	{
		memset(&ula_addr6, 0, sizeof(addr6CfgParam_t));
		create_ula_prefix(&ula_addr6);
		apmib_set(MIB_IPV6_ADDR_ULA_PARAM,&ula_addr6);
	}
#endif 
 
	return ;
}
#endif

void formTcpipSetup(request *wp, char *path, char *query)
{

	char tmpBuf[100];
	char buffer[200];
	char *submitUrl, *strDhcp, *strIp, *strMask, *strStartIp, *strEndIp;
	DHCP_T dhcp;
#ifdef MIB_TLV
	char pmib_num[10]={0};
	mib_table_entry_T *pmib_tl = NULL;
	unsigned int offset;
#endif
	struct in_addr inLanaddr_orig, inLanaddr_new;
	struct in_addr inLanmask_orig, inLanmask_new;
	struct in_addr private_host, tmp_private_host, update;
	int	entryNum_resvdip, i;
	DHCPRSVDIP_T entry_resvdip, checkentry_resvdip;
	int link_type;
#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
	int opmode=0, wlan0_mode=0, check_flag=0;
	int lan_dhcp_mode_orig=0;
	int lan_dhcp_mode=0;
	char lan_domain_name[	MAX_NAME_LEN]={0};
	char lan_domain_name_orig[	MAX_NAME_LEN]={0};
#endif	
	apmib_get( MIB_IP_ADDR,  (void *)buffer); //save the orig lan subnet
	memcpy((void *)&inLanaddr_orig, buffer, 4);
	
	apmib_get( MIB_SUBNET_MASK,  (void *)buffer); //save the orig lan mask
	memcpy((void *)&inLanmask_orig, buffer, 4);
	
#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
	apmib_get( MIB_DHCP, (void *)&lan_dhcp_mode_orig);
	apmib_get( MIB_DOMAIN_NAME, (void *)lan_domain_name_orig);
#endif

////////////////////////////////////////

	strDhcp = req_get_cstream_var(wp, ("dhcp"), "");
	if ( strDhcp[0] ) 
		dhcp = atoi(strDhcp);
	
	if(dhcp == DHCP_LAN_SERVER)
	{
		int entryNum;
		DHCPRSVDIP_T entry;
		struct in_addr start_ip, end_ip, router_ip, netmask;
		unsigned int staticIp;
		apmib_get(MIB_DHCPRSVDIP_TBL_NUM, (void *)&entryNum);
		
		strIp = req_get_cstream_var(wp, ("lan_ip"), "");
		if ( strIp[0] ) 
			inet_aton(strIp, &router_ip);

		strMask = req_get_cstream_var(wp, ("lan_mask"), "");
		if ( strMask[0] ) 
			inet_aton(strMask, &netmask);

		strStartIp = req_get_cstream_var(wp, ("dhcpRangeStart"), "");
		if ( strStartIp[0] ) 
			inet_aton(strStartIp, &start_ip);

		strEndIp = req_get_cstream_var(wp, ("dhcpRangeEnd"), "");
		if ( strEndIp[0] ) 
			inet_aton(strEndIp, &end_ip);		
	
		for (i=1; i<=entryNum; i++) 
		{
			*((char *)&entry) = (char)i;			
			apmib_get(MIB_DHCPRSVDIP_TBL, (void *)&entry);
			memcpy(&staticIp, entry.ipAddr, 4);

			if((staticIp & (~netmask.s_addr)) < ((start_ip.s_addr) & (~netmask.s_addr)) 
				|| (staticIp & (~netmask.s_addr)) > ((end_ip.s_addr) & (~netmask.s_addr)) 
				//|| (staticIp & (~netmask.s_addr))==((router_ip.s_addr) & (~netmask.s_addr))
			  )
			{
				strcpy(tmpBuf, "<script>dw(tcpip_dhcp_error)</script>");
				goto setErr_end ; 
			}
		}	
	}
///////////////////////////////////////

	if(tcpipLanHandler(wp, tmpBuf) < 0){
		//back to the orig lan subnet and mask
		apmib_set(MIB_IP_ADDR, (void *)&inLanaddr_orig);
		apmib_set(MIB_SUBNET_MASK, (void *)&inLanmask_orig);
		goto setErr_end ;
	}	
#if defined(MULTI_WAN_SUPPORT)&& defined(CONFIG_IPV6)
	radvdHandler(wp, path, query);
	dhcp6sHandler(wp, path, query);
#endif

#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
	apmib_get(MIB_OP_MODE,(void *)&opmode);
	apmib_get( MIB_WLAN_MODE, (void *)&wlan0_mode);
	if(opmode ==1 && (wlan0_mode == 1 || wlan0_mode == 0)){ //when wlan is client mode or ap mode, user change lan setting
		check_flag=1;
	}
	apmib_set(MIB_AUTO_DISCOVERY_ENABLED,(void *)&check_flag); //lan ipaddress has been changed from web page 
#endif

	apmib_update_web(CURRENT_SETTING);	// update configuration to flash

	
	apmib_get( MIB_IP_ADDR,  (void *)buffer); //check the new lan subnet
	memcpy((void *)&inLanaddr_new, buffer, 4);
	
	apmib_get( MIB_SUBNET_MASK,  (void *)buffer); //check the new lan mask
	memcpy((void *)&inLanmask_new, buffer, 4);

	//if(checkStaticIpIsValid(tmpBuf)>0)
	//	goto setErr_end ;
	
#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
	apmib_get( MIB_DHCP, (void *)&lan_dhcp_mode);
	apmib_get( MIB_DOMAIN_NAME, (void *)lan_domain_name);
#endif
	if((inLanaddr_orig.s_addr & inLanmask_orig.s_addr) != (inLanaddr_new.s_addr & inLanmask_new.s_addr)){
		//check static dhcp ip 
 		apmib_get(MIB_DHCPRSVDIP_TBL_NUM, (void *)&entryNum_resvdip);
		link_type = 8; //DHCPRSVDIP_ARRY_T
		for (i=1; i<=entryNum_resvdip; i++) {
			memset(&checkentry_resvdip, '\0', sizeof(checkentry_resvdip));
			*((char *)&entry_resvdip) = (char)i;
			apmib_get(MIB_DHCPRSVDIP_TBL, (void *)&entry_resvdip);
			memcpy(&checkentry_resvdip, &entry_resvdip, sizeof(checkentry_resvdip));
			memcpy((void *)&private_host, &(entry_resvdip.ipAddr), 4);
			if((inLanaddr_new.s_addr & inLanmask_new.s_addr) != (private_host.s_addr & inLanmask_new.s_addr)){
				update.s_addr = inLanaddr_new.s_addr & inLanmask_new.s_addr;
				tmp_private_host.s_addr  = ~(inLanmask_new.s_addr) & private_host.s_addr;
				update.s_addr = update.s_addr | tmp_private_host.s_addr;
				memcpy((void *)&(checkentry_resvdip.ipAddr), &(update), 4);
#if defined(MIB_TLV)
				offset=0;//must initial first for mib_search_by_id
				mib_search_by_id(mib_root_table, MIB_DHCPRSVDIP_TBL, pmib_num, &pmib_tl, &offset);
				update_tblentry(pMib,offset,entryNum_resvdip,pmib_tl,&entry_resvdip, &checkentry_resvdip);
#else
				update_linkchain(link_type, &entry_resvdip, &checkentry_resvdip , sizeof(checkentry_resvdip));
#endif
				
			}
		}
		apmib_update_web(CURRENT_SETTING);	// update configuration to flash
	}	

#ifdef MODULES_REINIT
	set_reinit_flag(REINIT_FLAG_ALL);
#endif

#ifndef NO_ACTION
#if defined(VOIP_SUPPORT) && defined(ATA867x)
	run_init_script("all");
#else
	run_init_script("bridge");
#endif
#endif

	submitUrl = req_get_cstream_var(wp, "submit-url", "");

#ifdef REBOOT_CHECK
#if !defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
	if(memcmp(&inLanaddr_orig,&inLanaddr_new,4) == 0)
#else
	if((memcmp(&inLanaddr_orig,&inLanaddr_new,4) == 0) && (lan_dhcp_mode_orig==lan_dhcp_mode) && (lan_domain_name[0] && !strcmp(lan_domain_name, lan_domain_name_orig)))
#endif
	{
		OK_MSG(submitUrl);
	}
	else
	{
		char tmpBuf[200];
		char lan_ip_buf[30], lan_ip[30];

		//apmib_reinit();
			
		//apmib_update_web(CURRENT_SETTING);	// update configuration to flash
		run_init_script_flag = 1;	
#ifndef NO_ACTION
		run_init_script("all");
#endif		
		apmib_get( MIB_IP_ADDR,  (void *)lan_ip_buf) ;
	  	sprintf(lan_ip,"%s",inet_ntoa(*((struct in_addr *)lan_ip_buf)) );
#ifndef RTK_REINIT_SUPPORT
	  	sprintf(tmpBuf,"%s","<script>dw(tcpip_change)</script>");
		OK_MSG_FW(tmpBuf, submitUrl, APPLY_COUNTDOWN_TIME+5, lan_ip);
#else
		struct in_addr  lan_ip_orig={0};
		struct in_addr  lan_mask_orig={0};
		struct in_addr  lan_mask={0};
		struct in_addr  remote_ip={0};
#ifdef CONFIG_IPV6
		int isLanPcIpv6 = 0;
		struct in6_addr remote_ip6 = {0};	
#endif

		if(!getInAddr(BRIDGE_IF,IP_ADDR,&lan_ip_orig))
			goto REBOOTCHECK_FAIL;
		if(!getInAddr(BRIDGE_IF,SUBNET_MASK,&lan_mask_orig))
			goto REBOOTCHECK_FAIL;
		if(!apmib_get(MIB_SUBNET_MASK,((void *)&lan_mask)))
			goto REBOOTCHECK_FAIL;
		if(!inet_aton(wp->remote_ip_addr, &remote_ip)){
	#ifdef CONFIG_IPV6		
		if(!inet_pton(AF_INET6,wp->remote_ip_addr,(void *)&remote_ip6)){
			goto REBOOTCHECK_FAIL;
		}else
			isLanPcIpv6 = 1;
	#else
		goto REBOOTCHECK_FAIL;
	#endif
			
		}
			

		fprintf(stderr,"send apply change event!\n");
		sprintf(tmpBuf,"reinitCli -e %d &", REINIT_EVENT_APPLYCAHNGES);
		system(tmpBuf);   

#ifdef CONFIG_IPV6
	if(isLanPcIpv6)
		send_redirect_perm(wp,submitUrl);
	else
#endif
		if(((lan_ip_orig.s_addr&lan_mask_orig.s_addr)==(remote_ip.s_addr&lan_mask_orig.s_addr))//lan visit
	   && (memcmp(&lan_ip_orig,&lan_ip_buf,sizeof(lan_ip_orig))!=0) //lan ip change
	   )
		{
			//fprintf(stderr,"%s:%d local_ip_addr=%s remote_ip_addr=%s\n",__FUNCTION__,__LINE__,wp->local_ip_addr,wp->remote_ip_addr);
			OK_MSG_FW("<script>dw(tcpip_change)</script>", submitUrl, 8, lan_ip);    
		}else
		{
			send_redirect_perm(wp,submitUrl);
		}
	
		//fprintf(stderr,"%s:%d redirect to %s\n",__FUNCTION__,__LINE__,tmpBuf);
#endif
	}
#else
	RECONNECT_MSG(submitUrl);	// display reconnect msg to remote
#endif

#ifndef APPLY_CHANGE_DIRECT_SUPPORT
	needReboot = 0;
#endif
	return;
 REBOOTCHECK_FAIL:
 //	printf("========%s:%s:%d========\n",__FILE__,__FUNCTION__,__LINE__);
	ERR_MSG("<script>dw(tcpip_reboot)</script>");
	return;
setErr_end:
	ERR_MSG(tmpBuf);
}

#ifdef RTK_USB3G
void kill_3G_ppp_inet(void)
{
    system("killall -15 ppp_inet 2> /dev/null");
    system("killall -15 pppd 2> /dev/null");
    system("rm /etc/ppp/connectfile >/dev/null 2>&1");
    system("rm /etc/ppp/link >/dev/null 2>&1");
}
#endif

#ifdef HOME_GATEWAY
int tcpipWanHandler(request *wp, char * tmpBuf, int *dns_changed)
{
	
	char	*strIp, *strMask, *strGateway, *strDNS, *strMode, *strConnect, *strMac;
	char  *strVal, *strType;
	int intVal;
	struct in_addr inIp, inMask,dns1, dns2, dns3, inGateway;
	DHCP_T dhcp, curDhcp;

	char tmpbuf[16];
#if defined(ROUTE_SUPPORT)	
	int orig_nat=0;
	int curr_nat=0;
#endif	
	DNS_TYPE_T dns, dns_old;

	char *submitUrl;
#ifndef NO_ACTION
	int pid;
#endif
#ifdef MULTI_PPPOE
	int flag = 0;
#endif
	int buttonState=0, call_from_wizard=0;
	
#if defined(CONFIG_DYNAMIC_WAN_IP)
	char *strPPPGateway, *strWanIpType;
	struct in_addr inPPPGateway;
	WAN_IP_TYPE_T wanIpType;
#if defined(CONFIG_GET_SERVER_IP_BY_DOMAIN)
	char *strGetServByDomain=NULL;
	char *strGatewayDomain;
#endif
#endif
#ifdef CONFIG_RTL_ETH_802DOT1X_CLIENT_MODE_SUPPORT
	int intVal2,dot1x_mode,val,dot1x_enable;
#endif
#if defined(CONFIG_4G_LTE_SUPPORT)
	int lte = 0;
#endif
#if defined(CONFIG_RTL_WEB_WAN_ACCESS_PORT)
	unsigned int WebPort = 0;
#endif


	strVal = req_get_cstream_var(wp, ("lan_ip"), "");
	if (strVal[0])
		call_from_wizard = 1;	
	
	strVal = req_get_cstream_var(wp, ("isPocketWizard"), "");
	if (strVal[0])
	{
		if ( atoi(strVal) == 1 )
		{
			call_from_wizard = 1;
		}
	}
			
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page

	strConnect = req_get_cstream_var(wp, ("pppConnect"), "");
	if (strConnect && strConnect[0]) {
		buttonState = 1;
#ifdef MULTI_PPPOE		
		flag = 1;
#endif
		strMode = "ppp";
		goto set_ppp;
	}

        strConnect = req_get_cstream_var(wp, ("pppDisconnect"), "");
	if (strConnect && strConnect[0]) {
		buttonState = 2;
#ifdef MULTI_PPPOE		
		flag = 1;
#endif	
		strMode = "ppp";
		goto set_ppp;
	}
#ifdef  MULTI_PPPOE
	//second
	strConnect = req_get_cstream_var(wp, ("pppConnect2"), "");
	if (strConnect && strConnect[0]) {
		buttonState = 1;
#ifdef MULTI_PPPOE		
		flag = 2;
#endif	
		strMode = "ppp";
		goto set_ppp;
	}
		strConnect = req_get_cstream_var(wp, ("pppDisconnect2"), "");
	if (strConnect && strConnect[0]) {
		buttonState = 2;
#ifdef MULTI_PPPOE		
		flag = 2;
#endif			
		strMode = "ppp";
		goto set_ppp;
	}
	//thrid 
		strConnect = req_get_cstream_var(wp, ("pppConnect3"), "");
	if (strConnect && strConnect[0]) {
		buttonState = 1;
#ifdef MULTI_PPPOE		
		flag = 3;
#endif	
		strMode = "ppp";
		goto set_ppp;
	}

		strConnect = req_get_cstream_var(wp, ("pppDisconnect3"), "");
	if (strConnect && strConnect[0]) {
		buttonState = 2;
#ifdef MULTI_PPPOE		
		flag = 3;
#endif				
		strMode = "ppp";
		goto set_ppp;
	}
	//forth
		strConnect = req_get_cstream_var(wp, ("pppConnect4"), "");
	if (strConnect && strConnect[0]) {
		buttonState = 1;
#ifdef MULTI_PPPOE		
		flag = 4;
#endif			
		strMode = "ppp";
		goto set_ppp;
	}

		strConnect = req_get_cstream_var(wp, ("pppDisconnect4"), "");
	if (strConnect && strConnect[0]) {
		buttonState = 2;
#ifdef MULTI_PPPOE		
		flag = 4;
#endif			
		strMode = "ppp";
		goto set_ppp;
	}
#endif
	
	strConnect = req_get_cstream_var(wp, ("pptpConnect"), "");
	if (strConnect && strConnect[0]) {
		buttonState = 1;
		strMode = "pptp";
		goto set_ppp;
	}

        strConnect = req_get_cstream_var(wp, ("pptpDisconnect"), "");
	if (strConnect && strConnect[0]) {
		buttonState = 2;
		strMode = "pptp";
		goto set_ppp;
	}
	strConnect = req_get_cstream_var(wp, ("l2tpConnect"), "");
	if (strConnect && strConnect[0]) {
		buttonState = 1;
		strMode = "l2tp";
		goto set_ppp;
	}

        strConnect = req_get_cstream_var(wp, ("l2tpDisconnect"), "");
	if (strConnect && strConnect[0]) {
		buttonState = 2;
		strMode = "l2tp";
		goto set_ppp;
	}

#ifdef RTK_USB3G
    strConnect = req_get_cstream_var(wp, ("USB3GConnect"), "");
    if (strConnect && strConnect[0]) {
        buttonState = 1;
        strMode = ("USB3G");
        goto set_ppp;
    }

    strConnect = req_get_cstream_var(wp, ("USB3GDisconnect"), "");
    if (strConnect && strConnect[0]) {
        buttonState = 2;
        strMode = ("USB3G");
        goto set_ppp;
    }
#endif /* #ifdef RTK_USB3G */

#if 0 //sc_yang
	strVal = req_get_cstream_var(wp, ("save"), "");
	if (!strVal || !strVal[0]) { // not save, wan type is changed
		strVal = req_get_cstream_var(wp, ("wanType"), "");
		wanDhcpTmp = (DHCP_T)(strVal[0] - '0');

		if (submitUrl && submitUrl[0])
			send_redirect_perm(wp, submitUrl);
		return;
	}
#endif
 	// Set clone MAC address
	strMac = req_get_cstream_var(wp, ("wan_macAddr"), "");
	if (strMac[0]) {
		if (strlen(strMac)!=12 || !string_to_hex(strMac, tmpBuf, 12)) {
			strcpy(tmpBuf, ("<script>dw(tcpip_wan_clone_mac_error)</script>"));
			goto setErr_tcpip;
		}
		if ( !apmib_set(MIB_WAN_MAC_ADDR, (void *)tmpBuf)) {
			strcpy(tmpBuf, ("Set MIB_WAN_MAC_ADDR mib error!"));
			goto setErr_tcpip;
		}
	}

	strMode = req_get_cstream_var(wp, ("dnsMode"), "");
	if ( strMode && strMode[0] ) {
		if (!strcmp(strMode, ("dnsAuto")))
			dns = DNS_AUTO;
		else if (!strcmp(strMode, ("dnsManual")))
			dns = DNS_MANUAL;
		else {
			strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_dns_mode)</script>"));
			goto setErr_tcpip;
		}

		if ( !apmib_get(MIB_DNS_MODE, (void *)&dns_old)) {
	  		strcpy(tmpBuf, ("Get DNS MIB error!"));
			goto setErr_tcpip;
		}
		if (dns != dns_old)
			*dns_changed = 1;

		// Set DNS to MIB
		if ( !apmib_set(MIB_DNS_MODE, (void *)&dns)) {
	  		strcpy(tmpBuf, "Set DNS MIB error!");
			goto setErr_tcpip;
		}

		if ( dns == DNS_MANUAL ) {
			struct in_addr dns1_old, dns2_old, dns3_old;
			if ( !apmib_get(MIB_DNS1, (void *)&dns1_old)) {
	  			strcpy(tmpBuf, "Get DNS1 MIB error!");
				goto setErr_tcpip;
			}
			if ( !apmib_get(MIB_DNS2, (void *)&dns2_old)) {
	  			strcpy(tmpBuf, "Get DNS1 MIB error!");
				goto setErr_tcpip;
			}
			if ( !apmib_get(MIB_DNS3, (void *)&dns3_old)) {
	  			strcpy(tmpBuf, "Get DNS1 MIB error!");
				goto setErr_tcpip;
			}

			// If DHCP server is enabled in LAN, update dhcpd.conf
			strDNS = req_get_cstream_var(wp, ("dns1"), "");
			if ( strDNS[0] ) {
				if ( !inet_aton(strDNS, &dns1) ) {
					strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_dns_value)</script>"));
					goto setErr_tcpip;
				}

				if ( !apmib_set(MIB_DNS1, (void *)&dns1)) {
	  				strcpy(tmpBuf, "Set DNS MIB error!");
					goto setErr_tcpip;
				}
			}
			else {
				if ( !apmib_get(MIB_DNS1, (void *)&dns1) ) {
					strcpy(tmpBuf, "Get DNS1 MIB error!");
					goto setErr_tcpip;
				}
			}
			strDNS = req_get_cstream_var(wp, ("dns2"), "");
			if ( strDNS[0] ) {
				if ( !inet_aton(strDNS, &dns2) ) {
					strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_dns_value)</script>"));
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_DNS2, (void *)&dns2)) {
	  				strcpy(tmpBuf, "Set DNS MIB error!");
					goto setErr_tcpip;
				}
			}
			else {
				if ( !apmib_get(MIB_DNS2, (void *)&dns2) ) {
					strcpy(tmpBuf, ("Get DNS2 MIB error!"));
					goto setErr_tcpip;
				}
			}
			strDNS = req_get_cstream_var(wp, ("dns3"), "");
			if ( strDNS[0] ) {
				if ( !inet_aton(strDNS, &dns3) ) {
					strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_dns_value)</script>"));
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_DNS3, (void *)&dns3)) {
	  				strcpy(tmpBuf, "Set DNS MIB error!");
					goto setErr_tcpip;
				}
			}
			else {
				if ( !apmib_get(MIB_DNS3, (void *)&dns3) ) {
					strcpy(tmpBuf, "Get DNS3 MIB error!");
					goto setErr_tcpip;
				}
			}

			if ( *((long *)&dns1) != *((long *)&dns1_old) ||
				 *((long *)&dns2) != *((long *)&dns2_old) ||
				  *((long *)&dns3) != *((long *)&dns3_old) )
				*dns_changed = 1;
		}
	}

	// Read current ip mode setting for reference later
	if ( !apmib_get( MIB_WAN_DHCP, (void *)&curDhcp) ) {
		strcpy(tmpBuf, ("Get WAN DHCP MIB error!"));
		goto setErr_tcpip;
	}
#if defined(ROUTE_SUPPORT)	
	if ( !apmib_get( MIB_NAT_ENABLED, (void *)&orig_nat) ) {
		strcpy(tmpBuf, ("Get NAT MIB error!"));
		goto setErr_tcpip;
	}
	
#endif	
	//sc_yang
	//strMode = req_get_cstream_var(wp, ("ipMode"), "");
	strMode = req_get_cstream_var(wp, ("wanType"), "");

#if defined(CONFIG_RTL_ULINKER)
	if ( strMode && strMode[0] )
		;
	else
	{
		strMode = req_get_cstream_var(wp, ("otg_wan_type"), "");
	}
#endif

set_ppp:
	if ( strMode && strMode[0] ) {
		if ( !strcmp(strMode, ("autoIp")))
			dhcp = DHCP_CLIENT;
		else if ( !strcmp(strMode, ("fixedIp")))
			dhcp = DHCP_DISABLED;
		else if ( !strcmp(strMode, "ppp")) {
			char	*strName, *strPassword, *strService;
			char 	*strConnectNumber;
			char  *strsubnetNumber;
			char	*strIp;
			char  *strSubNet;
			struct in_addr  inIp;
			int count;
			dhcp = PPPOE;
			strConnectNumber = req_get_cstream_var(wp, "pppoeNumber", "");
			count = strtol(strConnectNumber, (char**)NULL, 10);
			
			if(strConnectNumber[0]){				
				if ( apmib_set(MIB_PPP_CONNECT_COUNT, (void *)&count) == 0) {
					strcpy(tmpBuf, "Set PPPoE Number MIB error!");
					goto setErr_tcpip;
				}				
			}			
			strSubNet = req_get_cstream_var(wp, "pppSubNet_1", "");
			if(strSubNet[0]){
				if ( apmib_set(MIB_PPP_SUBNET1, (void *)strSubNet) == 0) {
					strcpy(tmpBuf, "Set PPPoE Number MIB error!");
					goto setErr_tcpip;
				}				
			}	
			strSubNet = req_get_cstream_var(wp, "pppSubNet_2", "");
			if(strSubNet[0]){
				if ( apmib_set(MIB_PPP_SUBNET2, (void *)strSubNet) == 0) {
					strcpy(tmpBuf, "Set PPPoE Number MIB error!");
					goto setErr_tcpip;
				}				
			}
			strSubNet = req_get_cstream_var(wp, "pppSubNet_3", "");
			if(strSubNet[0]){
				if ( apmib_set(MIB_PPP_SUBNET3, (void *)strSubNet) == 0) {
					strcpy(tmpBuf, "Set PPPoE Number MIB error!");
					goto setErr_tcpip;
				}				
			}
			strSubNet = req_get_cstream_var(wp, "pppSubNet_4","");
			if(strSubNet[0]){
				if ( apmib_set(MIB_PPP_SUBNET4, (void *)strSubNet) == 0) {
					strcpy(tmpBuf, "Set PPPoE Number MIB error!");
					goto setErr_tcpip;
				}				
			}
			strsubnetNumber = req_get_cstream_var(wp, "pppSubNet1","");
			count = strtol(strsubnetNumber, (char**)NULL, 10);
			if(strsubnetNumber[0]){
				if ( apmib_set(MIB_SUBNET1_COUNT, (void *)&count) == 0) {
					strcpy(tmpBuf, "Set PPPoE Number MIB error!");
					goto setErr_tcpip;
				}				
			}		

			strsubnetNumber = req_get_cstream_var(wp, "pppSubNet2", "");
			count = strtol(strsubnetNumber, (char**)NULL, 10);
			if(strsubnetNumber[0]){
				if ( apmib_set(MIB_SUBNET2_COUNT, (void *)&count) == 0) {
					strcpy(tmpBuf, "Set PPPoE Number MIB error!");
					goto setErr_tcpip;
				}				
			}

			strsubnetNumber = req_get_cstream_var(wp, "pppSubNet3", "");
			count = strtol(strsubnetNumber, (char**)NULL, 10);
			if(strsubnetNumber[0]){
				if ( apmib_set(MIB_SUBNET3_COUNT, (void *)&count) == 0) {
					strcpy(tmpBuf, "Set PPPoE Number MIB error!");
					goto setErr_tcpip;
				}				
			}

			strsubnetNumber = req_get_cstream_var(wp, "pppSubNet4", "");
			count = strtol(strsubnetNumber, (char**)NULL, 10);
			if(strsubnetNumber[0]){
				if ( apmib_set(MIB_SUBNET4_COUNT, (void *)&count) == 0) {
					strcpy(tmpBuf, "Set PPPoE Number MIB error!");
					goto setErr_tcpip;
				}				
			}			
			// four ip seting,the first one
			strIp = req_get_cstream_var(wp, "S1_F1_start", "");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf, "<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET1_F1_START, (void *)&inIp)) {
					strcpy(tmpBuf, "Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}			
			strIp = req_get_cstream_var(wp, "S1_F1_end", "");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf, "<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET1_F1_END, (void *)&inIp)) {
					strcpy(tmpBuf, "Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			strIp = req_get_cstream_var(wp, "S1_F2_start", "");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf, "<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET1_F2_START, (void *)&inIp)) {
					strcpy(tmpBuf, "Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			strIp = req_get_cstream_var(wp, "S1_F2_end", "");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf, "<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET1_F2_END, (void *)&inIp)) {
					strcpy(tmpBuf, "Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}			
			strIp = req_get_cstream_var(wp, "S1_F3_start","");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET1_F3_START, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			strIp = req_get_cstream_var(wp,"S1_F3_end", "");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET1_F3_END, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			
			//the second
			strIp = req_get_cstream_var(wp,"S2_F1_start","");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET2_F1_START, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}			
			strIp = req_get_cstream_var(wp,"S2_F1_end","");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET2_F1_END, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			strIp = req_get_cstream_var(wp,"S2_F2_start","");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET2_F2_START, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			strIp = req_get_cstream_var(wp,"S2_F2_end","");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET2_F2_END, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}			
			strIp = req_get_cstream_var(wp,"S2_F3_start","");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET2_F3_START, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			strIp = req_get_cstream_var(wp,"S2_F3_end","");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET2_F3_END, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			//the third 
			strIp = req_get_cstream_var(wp,"S3_F1_start","");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET3_F1_START, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}			
			strIp = req_get_cstream_var(wp,"S3_F1_end","");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET3_F1_END, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			strIp = req_get_cstream_var(wp,"S3_F2_start","");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET3_F2_START, (void *)&inIp)) {
					strcpy(tmpBuf, "Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			strIp = req_get_cstream_var(wp,"S3_F2_end","");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET3_F2_END, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}			
			strIp = req_get_cstream_var(wp,"S3_F3_start","");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET3_F3_START, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			strIp = req_get_cstream_var(wp,"S3_F3_end","");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf, "<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET3_F3_END, (void *)&inIp)) {
					strcpy(tmpBuf, "Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			//the forth
			strIp = req_get_cstream_var(wp,"S4_F1_start", "");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf, "<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET4_F1_START, (void *)&inIp)) {
					strcpy(tmpBuf, "Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}			
			strIp = req_get_cstream_var(wp, "S4_F1_end", "");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET4_F1_END, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			strIp = req_get_cstream_var(wp,"S4_F2_start", "");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf, "<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET4_F2_START, (void *)&inIp)) {
					strcpy(tmpBuf, "Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			strIp = req_get_cstream_var(wp, "S4_F2_end", "");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET4_F2_END, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}			
			strIp = req_get_cstream_var(wp,"S4_F3_start", "");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf, "<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET4_F3_START, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}
			strIp = req_get_cstream_var(wp,"S4_F3_end","");
			if ( strIp[0] ) {
				if ( !inet_aton(strIp, &inIp) ) {
					strcpy(tmpBuf,"<script>dw(tcpip_lan_invalid_subnet_value)</script>");
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_SUBNET4_F3_END, (void *)&inIp)) {
					strcpy(tmpBuf,"Set subnet-mask error!");
					goto setErr_tcpip;
				}
			}			

  			strName = req_get_cstream_var(wp, ("pppUserName"), "");
			if ( strName[0] ) {
				if ( apmib_set(MIB_PPP_USER_NAME, (void *)strName) == 0) {
					strcpy(tmpBuf, ("Set PPP user name MIB error!"));
					goto setErr_tcpip;
				}
			}

 			strPassword = req_get_cstream_var(wp, ("pppPassword"), "");
			if ( strPassword[0] ) {
				if ( apmib_set(MIB_PPP_PASSWORD, (void *)strPassword) == 0) {
					strcpy(tmpBuf, ("Set PPP user password MIB error!"));
					goto setErr_tcpip;
				}
			}
			strName = req_get_cstream_var(wp,"pppUserName2" ,"");
			if ( strName[0] ) {
				if ( apmib_set(MIB_PPP_USER_NAME2, (void *)strName) == 0) {
					strcpy(tmpBuf,"Set PPP user name MIB error!");
					goto setErr_tcpip;
				}
			}

			strPassword = req_get_cstream_var(wp,"pppPassword2" ,"");
			if ( strPassword[0] ) {
				if ( apmib_set(MIB_PPP_PASSWORD2, (void *)strPassword) == 0) {
					strcpy(tmpBuf,"Set PPP user password MIB error!");
					goto setErr_tcpip;
				}
			}
			strName = req_get_cstream_var(wp,"pppUserName3","");
			if ( strName[0] ) {
				if ( apmib_set(MIB_PPP_USER_NAME3, (void *)strName) == 0) {
					strcpy(tmpBuf, "Set PPP user name MIB error!");
					goto setErr_tcpip;
				}
			}

			strPassword = req_get_cstream_var(wp,"pppPassword3","");
			if ( strPassword[0] ) {
				if ( apmib_set(MIB_PPP_PASSWORD3, (void *)strPassword) == 0) {
					strcpy(tmpBuf, "Set PPP user password MIB error!");
					goto setErr_tcpip;
				}
			}
			strName = req_get_cstream_var(wp, "pppUserName4","");
			if ( strName[0] ) {
				if ( apmib_set(MIB_PPP_USER_NAME4, (void *)strName) == 0) {
					strcpy(tmpBuf, "Set PPP user name MIB error!");
					goto setErr_tcpip;
				}
			}

			strPassword = req_get_cstream_var(wp,"pppPassword4", "");
			if ( strPassword[0] ) {
				if ( apmib_set(MIB_PPP_PASSWORD4, (void *)strPassword) == 0) {
					strcpy(tmpBuf, "Set PPP user password MIB error!");
					goto setErr_tcpip;
				}
			}

			strService = req_get_cstream_var(wp, ("pppServiceName"), "");
			if ( strService[0] ) {
				if ( apmib_set(MIB_PPP_SERVICE_NAME, (void *)strService) == 0) {
					strcpy(tmpBuf, ("Set PPP serice name MIB error!"));
					goto setErr_tcpip;
				}
			}else{
				if ( apmib_set(MIB_PPP_SERVICE_NAME, (void *)"") == 0) {
					strcpy(tmpBuf, ("Set PPP serice name MIB error!"));
					goto setErr_tcpip;
				}
			}
			strService = req_get_cstream_var(wp, "pppServiceName2","");
			if ( strService[0] ) {
				if ( apmib_set(MIB_PPP_SERVICE_NAME2, (void *)strService) == 0) {
					strcpy(tmpBuf,"Set PPP serice name MIB error!");
					goto setErr_tcpip;
				}
			}else{
				if ( apmib_set(MIB_PPP_SERVICE_NAME2, (void *)"") == 0) {
					strcpy(tmpBuf,"Set PPP serice name MIB error!");
					goto setErr_tcpip;
				}
			}
			strService = req_get_cstream_var(wp,"pppServiceName3","");
			if ( strService[0] ) {
				if ( apmib_set(MIB_PPP_SERVICE_NAME3, (void *)strService) == 0) {
					strcpy(tmpBuf, "Set PPP serice name MIB error!");
					goto setErr_tcpip;
				}
			}else{
				if ( apmib_set(MIB_PPP_SERVICE_NAME3, (void *)"") == 0) {
					strcpy(tmpBuf, "Set PPP serice name MIB error!");
					goto setErr_tcpip;
				}
			}
			strService = req_get_cstream_var(wp,"pppServiceName4" ,"");
			if ( strService[0] ) {
				if ( apmib_set(MIB_PPP_SERVICE_NAME4, (void *)strService) == 0) {
					strcpy(tmpBuf, "Set PPP serice name MIB error!");
					goto setErr_tcpip;
				}
			}else{
				if ( apmib_set(MIB_PPP_SERVICE_NAME4, (void *)"") == 0) {
					strcpy(tmpBuf, "Set PPP serice name MIB error!");
					goto setErr_tcpip;
				}
			}
			
			strType = req_get_cstream_var(wp, ("pppConnectType"), "");
			if ( strType[0] ) {
				PPP_CONNECT_TYPE_T type;
				if ( strType[0] == '0' )
					type = CONTINUOUS;
				else if ( strType[0] == '1' )
					type = CONNECT_ON_DEMAND;
				else if ( strType[0] == '2' )
					type = MANUAL;
				else {
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_ppp_type)</script>"));
					goto setErr_tcpip;
				}
				if ( apmib_set(MIB_PPP_CONNECT_TYPE, (void *)&type) == 0) {
   					strcpy(tmpBuf, ("Set PPP type MIB error!"));
					goto setErr_tcpip;
				}
				if (type != CONTINUOUS) {
					char *strTime;
					strTime = req_get_cstream_var(wp, ("pppIdleTime"), "");
					if ( strTime[0] ) {
						int time;
 						time = strtol(strTime, (char**)NULL, 10) * 60;
						if ( apmib_set(MIB_PPP_IDLE_TIME, (void *)&time) == 0) {
   							strcpy(tmpBuf, ("Set PPP idle time MIB error!"));
							goto setErr_tcpip;
						}
					}
				}
			}
			strType = req_get_cstream_var(wp, "pppConnectType2","");
			if ( strType[0] ) {
				PPP_CONNECT_TYPE_T type;
				if ( strType[0] == '0' )
					type = CONTINUOUS;
				else if ( strType[0] == '1' )
					type = CONNECT_ON_DEMAND;
				else if ( strType[0] == '2' )
					type = MANUAL;
				else {
					strcpy(tmpBuf, "<script>dw(tcpip_wan_invalid_ppp_type)</script>");
					goto setErr_tcpip;
				}
				if ( apmib_set(MIB_PPP_CONNECT_TYPE2, (void *)&type) == 0) {
   					strcpy(tmpBuf, "Set PPP type MIB error!");
					goto setErr_tcpip;
				}
				if (type != CONTINUOUS) {
					char *strTime;
					strTime = req_get_cstream_var(wp, "pppIdleTime2","");
					if ( strTime[0] ) {
						int time;
 						time = strtol(strTime, (char**)NULL, 10) * 60;
						if ( apmib_set(MIB_PPP_IDLE_TIME2, (void *)&time) == 0) {
   							strcpy(tmpBuf, "Set PPP idle time MIB error!");
							goto setErr_tcpip;
						}
					}
				}
			}
						
			strType = req_get_cstream_var(wp, "pppConnectType3", "");
			if ( strType[0] ) {
				PPP_CONNECT_TYPE_T type;
				if ( strType[0] == '0' )
					type = CONTINUOUS;
				else if ( strType[0] == '1' )
					type = CONNECT_ON_DEMAND;
				else if ( strType[0] == '2' )
					type = MANUAL;
				else {
					strcpy(tmpBuf, "<script>dw(tcpip_wan_invalid_ppp_type)</script>");
					goto setErr_tcpip;
				}
				if ( apmib_set(MIB_PPP_CONNECT_TYPE3, (void *)&type) == 0) {
   					strcpy(tmpBuf, "Set PPP type MIB error!");
					goto setErr_tcpip;
				}
				if (type != CONTINUOUS) {
					char *strTime;
					strTime = req_get_cstream_var(wp, "pppIdleTime3", "");
					if ( strTime[0] ) {
						int time;
 						time = strtol(strTime, (char**)NULL, 10) * 60;
						if ( apmib_set(MIB_PPP_IDLE_TIME3, (void *)&time) == 0) {
   							strcpy(tmpBuf, "Set PPP idle time MIB error!");
							goto setErr_tcpip;
						} 
					}
				}
			}	
			strType = req_get_cstream_var(wp, "pppConnectType4" ,"");
			if ( strType[0] ) {
				PPP_CONNECT_TYPE_T type;
				if ( strType[0] == '0' )
					type = CONTINUOUS;
				else if ( strType[0] == '1' )
					type = CONNECT_ON_DEMAND;
				else if ( strType[0] == '2' )
					type = MANUAL;
				else {
					strcpy(tmpBuf, "<script>dw(tcpip_wan_invalid_ppp_type)</script>");
					goto setErr_tcpip;
				}
				if ( apmib_set(MIB_PPP_CONNECT_TYPE4, (void *)&type) == 0) {
   					strcpy(tmpBuf, "Set PPP type MIB error!");
					goto setErr_tcpip;
				}
				if (type != CONTINUOUS) {
					char *strTime;
					strTime = req_get_cstream_var(wp, "pppIdleTime4", "");
					if ( strTime[0] ) {
						int time;
 						time = strtol(strTime, (char**)NULL, 10) * 60;
						if ( apmib_set(MIB_PPP_IDLE_TIME4, (void *)&time) == 0) {
   							strcpy(tmpBuf, "Set PPP idle time MIB error!");
							goto setErr_tcpip;
						}
					}
				}
			}			
#ifdef _ALPHA_DUAL_WAN_SUPPORT_
			strVal = req_get_cstream_var(wp, ("pppVlanId"), "");
			if ( strVal[0] ) {
				int vlanId;
 				vlanId = strtol(strVal, (char**)NULL, 10);
				if ( apmib_set(MIB_CWMP_PPPOE_WAN_VLANID, (void *)&vlanId) == 0) {
					strcpy(tmpBuf, ("Set PPP vlan id MIB error!"));
					goto setErr_tcpip;
				}
			}
#endif
			strVal = req_get_cstream_var(wp, ("pppMtuSize"), "");
			if ( strVal[0] ) {
				int mtuSize;
 				mtuSize = strtol(strVal, (char**)NULL, 10);
				if ( apmib_set(MIB_PPP_MTU_SIZE, (void *)&mtuSize) == 0) {
					strcpy(tmpBuf, ("Set PPP mtu size MIB error!"));
					goto setErr_tcpip;
				}
			}
			strVal = req_get_cstream_var(wp,"pppMtuSize2","");
			if ( strVal[0] ) {
				int mtuSize;
				mtuSize = strtol(strVal, (char**)NULL, 10);
				if ( apmib_set(MIB_PPP_MTU_SIZE2, (void *)&mtuSize) == 0) {
					strcpy(tmpBuf, "Set PPP mtu size MIB error!");
					goto setErr_tcpip;
				}
			}
			
			strVal = req_get_cstream_var(wp, "pppMtuSize3","");
			if ( strVal[0] ) {
				int mtuSize;
				mtuSize = strtol(strVal, (char**)NULL, 10);
				if ( apmib_set(MIB_PPP_MTU_SIZE3, (void *)&mtuSize) == 0) {
					strcpy(tmpBuf, "Set PPP mtu size MIB error!");
					goto setErr_tcpip;
				}
			}
			strVal = req_get_cstream_var(wp, "pppMtuSize4", "");
			if ( strVal[0] ) {
				int mtuSize;
				mtuSize = strtol(strVal, (char**)NULL, 10);
				if ( apmib_set(MIB_PPP_MTU_SIZE4, (void *)&mtuSize) == 0) {
					strcpy(tmpBuf, "Set PPP mtu size MIB error!");
					goto setErr_tcpip;
				}
			}
		}
		else if ( !strcmp(strMode, "pptp")) {
			char	*strName, *strPassword;
			dhcp = PPTP;
  			strName = req_get_cstream_var(wp, ("pptpUserName"), "");
			if ( strName[0] ) {
				if ( apmib_set(MIB_PPTP_USER_NAME, (void *)strName) == 0) {
					strcpy(tmpBuf, ("Set PPTP user name MIB error!"));
					goto setErr_tcpip;
				}
			}
 			strPassword = req_get_cstream_var(wp, ("pptpPassword"), "");
			if ( strPassword[0] ) {
				if ( apmib_set(MIB_PPTP_PASSWORD, (void *)strPassword) == 0) {
					strcpy(tmpBuf, ("Set PPTP user password MIB error!"));
					goto setErr_tcpip;
				}
			}
#if defined(CONFIG_DYNAMIC_WAN_IP)
			strWanIpType = req_get_cstream_var(wp, ("wan_pptp_use_dynamic_carrier_radio"), (""));
			if ( strWanIpType[0] ) {
				if (!strcmp(strWanIpType, ("dynamicIP")))
				{
					wanIpType= DYNAMIC_IP;
					
				}
				else if (!strcmp(strWanIpType, ("staticIP")))
				{
					wanIpType = STATIC_IP;
				}
				else {
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_pptp_type)</script>"));
					goto setErr_tcpip;
				}

				if ( !apmib_set(MIB_PPTP_WAN_IP_DYNAMIC, (void *)&wanIpType)) {
			  		strcpy(tmpBuf, ("Set MIB_PPTP_WAN_IP_DYNAMIC error!"));
					goto setErr_tcpip;
				}
			}
			
			strPPPGateway = req_get_cstream_var(wp, ("pptpDefGw"), (""));
			if ( strPPPGateway[0] ) {
				if ( !inet_aton(strPPPGateway, &inPPPGateway) ) {
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_defgw)</script>"));
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_PPTP_DEFAULT_GW, (void *)&inPPPGateway)) {
					strcpy(tmpBuf, ("Set pptp default gateway error!"));
					goto setErr_tcpip;
				}
			}
#endif

#if defined(CONFIG_DYNAMIC_WAN_IP)
			if(wanIpType==STATIC_IP){
#endif
				strIp = req_get_cstream_var(wp, ("pptpIpAddr"), "");
				if ( strIp[0] ) {
					if ( !inet_aton(strIp, &inIp) ) {
						strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_ip_value)</script>"));
						goto setErr_tcpip;
					}
					if ( !apmib_set(MIB_PPTP_IP_ADDR, (void *)&inIp)) {
						strcpy(tmpBuf, ("Set IP-address error!"));
						goto setErr_tcpip;
					}
				}

				strMask = req_get_cstream_var(wp, ("pptpSubnetMask"), "");
				if ( strMask[0] ) {
					if ( !inet_aton(strMask, &inMask) ) {
						strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_subnet_value)</script>"));
						goto setErr_tcpip;
					}
					if ( !apmib_set(MIB_PPTP_SUBNET_MASK, (void *)&inMask)) {
						strcpy(tmpBuf, ("Set subnet-mask error!"));
						goto setErr_tcpip;
					}
				}
#if defined(CONFIG_DYNAMIC_WAN_IP)
			}
#endif

			strGateway = req_get_cstream_var(wp, ("pptpServerIpAddr"), "");
			if ( strGateway[0] ) {
				if ( !inet_aton(strGateway, &inGateway) ) {
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_pptp_server)</script>"));
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_PPTP_SERVER_IP_ADDR, (void *)&inGateway)) {
					strcpy(tmpBuf, ("Set pptp server ip error!"));
					goto setErr_tcpip;
				}
			}

#if defined(CONFIG_GET_SERVER_IP_BY_DOMAIN)
			strGetServByDomain = req_get_cstream_var(wp,"pptpGetServMode","");
			if(strGetServByDomain[0])
			{
				if(!strcmp(strGetServByDomain,"pptpGetServByDomainName"))
				{
					intVal=1;
					if(!apmib_set(MIB_PPTP_GET_SERV_BY_DOMAIN,(void*)&intVal))
					{
						strcpy(tmpBuf, ("Set pptp get server by domain error!"));
							goto setErr_tcpip;
					}
					strGatewayDomain = req_get_cstream_var(wp, ("pptpServerDomainName"), "");
					if(strGatewayDomain[0])
					{
						if ( !apmib_set(MIB_PPTP_SERVER_DOMAIN, (void *)strGatewayDomain)) {
							strcpy(tmpBuf, ("Set pptp server domain error!"));
							goto setErr_tcpip;
						}
					}
				}else
				{
					intVal=0;
					if(!apmib_set(MIB_PPTP_GET_SERV_BY_DOMAIN,(void*)&intVal))
					{
						strcpy(tmpBuf, ("Set pptp get server by domain error!"));
							goto setErr_tcpip;
					}
				}
			}
			
#endif

		strType = req_get_cstream_var(wp, ("pptpConnectType"), "");
			if ( strType[0] ) {
				PPP_CONNECT_TYPE_T type;
				if ( strType[0] == '0' )
					type = CONTINUOUS;
				else if ( strType[0] == '1' )
					type = CONNECT_ON_DEMAND;
				else if ( strType[0] == '2' )
					type = MANUAL;
				else {
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_pptp_type)</script>"));
					goto setErr_tcpip;
				}
				if ( apmib_set(MIB_PPTP_CONNECTION_TYPE, (void *)&type) == 0) {
   					strcpy(tmpBuf, ("Set PPTP type MIB error!"));
					goto setErr_tcpip;
				}
				if (type != CONTINUOUS) {
					char *strTime;
					strTime = req_get_cstream_var(wp, ("pptpIdleTime"), "");
					if ( strTime[0] ) {
						int time;
 						time = strtol(strTime, (char**)NULL, 10) * 60;
						if ( apmib_set(MIB_PPTP_IDLE_TIME, (void *)&time) == 0) {
   							strcpy(tmpBuf, ("Set PPTP idle time MIB error!"));
							goto setErr_tcpip;
						}
					}
				}
			}
			strVal = req_get_cstream_var(wp, ("pptpMtuSize"), "");
			if ( strVal[0] ) {
				int mtuSize;
 				mtuSize = strtol(strVal, (char**)NULL, 10);
				if ( apmib_set(MIB_PPTP_MTU_SIZE, (void *)&mtuSize) == 0) {
					strcpy(tmpBuf, ("Set PPTP mtu size MIB error!"));
					goto setErr_tcpip;
				}
			}
			if (!call_from_wizard) { // not called from wizard
				strVal = req_get_cstream_var(wp, ("pptpSecurity"), "");
				if ( !strcmp(strVal, "ON"))
					intVal = 1;
				else
					intVal = 0;
				apmib_set(MIB_PPTP_SECURITY_ENABLED, (void *)&intVal);

				strVal = req_get_cstream_var(wp, ("pptpCompress"), "");
				if ( !strcmp(strVal, "ON"))
					intVal = 1;
				else
					intVal = 0;
				apmib_set(MIB_PPTP_MPPC_ENABLED, (void *)&intVal);				
			}			
		}
		/* # keith: add l2tp support. 20080515 */
		else if ( !strcmp(strMode, "l2tp")) {
			char	*strName, *strPassword;
			dhcp = L2TP;
  			strName = req_get_cstream_var(wp, ("l2tpUserName"), "");
			if ( strName[0] ) {
				if ( apmib_set(MIB_L2TP_USER_NAME, (void *)strName) == 0) {
					strcpy(tmpBuf, ("Set L2TP user name MIB error!"));
					goto setErr_tcpip;
				}
			}
 			strPassword = req_get_cstream_var(wp, ("l2tpPassword"), "");
			if ( strPassword[0] ) {
				if ( apmib_set(MIB_L2TP_PASSWORD, (void *)strPassword) == 0) {
					strcpy(tmpBuf, ("Set L2TP user password MIB error!"));
					goto setErr_tcpip;
				}
			}
#if defined(CONFIG_DYNAMIC_WAN_IP)
			strWanIpType = req_get_cstream_var(wp, ("wan_l2tp_use_dynamic_carrier_radio"), (""));
			if ( strWanIpType[0] ) {
				if (!strcmp(strWanIpType, ("dynamicIP")))
					wanIpType= DYNAMIC_IP;
				else if (!strcmp(strWanIpType, ("staticIP")))
					wanIpType = STATIC_IP;
				else {
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_l2tp_type)</script>"));
					goto setErr_tcpip;
				}

				if ( !apmib_set(MIB_L2TP_WAN_IP_DYNAMIC, (void *)&wanIpType)) {
			  		strcpy(tmpBuf, ("Set MIB_L2TP_WAN_IP_DYNAMIC error!"));
					goto setErr_tcpip;
				}
			}
			
			strPPPGateway = req_get_cstream_var(wp, ("l2tpDefGw"), (""));
			if ( strPPPGateway[0] ) {
				if ( !inet_aton(strPPPGateway, &inPPPGateway) ) {
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_l2tp_defgw)</script>"));
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_L2TP_DEFAULT_GW, (void *)&inPPPGateway)) {
					strcpy(tmpBuf, ("Set l2tp default gateway error!"));
					goto setErr_tcpip;
				}
			}
#endif	

#if defined(CONFIG_DYNAMIC_WAN_IP)
			if(wanIpType==STATIC_IP){
#endif		
				strIp = req_get_cstream_var(wp, ("l2tpIpAddr"), "");
				if ( strIp[0] ) {
					if ( !inet_aton(strIp, &inIp) ) {
						strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_ip_value)</script>"));
						goto setErr_tcpip;
					}
					if ( !apmib_set(MIB_L2TP_IP_ADDR, (void *)&inIp)) {
						strcpy(tmpBuf, ("Set IP-address error!"));
						goto setErr_tcpip;
					}
				}

				strMask = req_get_cstream_var(wp, ("l2tpSubnetMask"), "");
				if ( strMask[0] ) {
					if ( !inet_aton(strMask, &inMask) ) {
						strcpy(tmpBuf, ("<script>dw(tcpip_lan_invalid_subnet_value)</script>"));
						goto setErr_tcpip;
					}
					if ( !apmib_set(MIB_L2TP_SUBNET_MASK, (void *)&inMask)) {
						strcpy(tmpBuf, ("Set subnet-mask error!"));
						goto setErr_tcpip;
					}
				}
#if defined(CONFIG_DYNAMIC_WAN_IP)
			}
#endif		
			strGateway = req_get_cstream_var(wp, ("l2tpServerIpAddr"), "");
			if ( strGateway[0] ) {
				if ( !inet_aton(strGateway, &inGateway) ) {
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_l2tp_server)</script>"));
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_L2TP_SERVER_IP_ADDR, (void *)&inGateway)) {
					strcpy(tmpBuf, ("Set pptp server ip error!"));
					goto setErr_tcpip;
				}
			}
			
#if defined(CONFIG_GET_SERVER_IP_BY_DOMAIN)
			strGetServByDomain = req_get_cstream_var(wp,"l2tpGetServMode","");
			if(strGetServByDomain[0])
			{
				if(!strcmp(strGetServByDomain,"l2tpGetServByDomainName"))
				{
					intVal=1;
					if(!apmib_set(MIB_L2TP_GET_SERV_BY_DOMAIN,(void*)&intVal))
					{
						strcpy(tmpBuf, ("Set l2tp get server by domain error!"));
							goto setErr_tcpip;
					}
					strGatewayDomain = req_get_cstream_var(wp, ("l2tpServerDomainName"), "");
					if(strGatewayDomain[0])
					{
						if ( !apmib_set(MIB_L2TP_SERVER_DOMAIN, (void *)strGatewayDomain)) {
							strcpy(tmpBuf, ("Set l2tp server domain error!"));
							goto setErr_tcpip;
						}
					}
				}else
				{
					intVal=0;
					if(!apmib_set(MIB_L2TP_GET_SERV_BY_DOMAIN,(void*)&intVal))
					{
						strcpy(tmpBuf, ("Set l2tp get server by domain error!"));
							goto setErr_tcpip;
					}
				}
			}
			
#endif

		strType = req_get_cstream_var(wp, ("l2tpConnectType"), "");
			if ( strType[0] ) {
				PPP_CONNECT_TYPE_T type;
				if ( strType[0] == '0' )
					type = CONTINUOUS;
				else if ( strType[0] == '1' )
					type = CONNECT_ON_DEMAND;
				else if ( strType[0] == '2' )
					type = MANUAL;
				else {
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_l2tp)</script>"));
					goto setErr_tcpip;
				}
				if ( apmib_set(MIB_L2TP_CONNECTION_TYPE, (void *)&type) == 0) {
   					strcpy(tmpBuf, ("Set L2TP type MIB error!"));
					goto setErr_tcpip;
				}
				if (type != CONTINUOUS) {
					char *strTime;
					strTime = req_get_cstream_var(wp, ("l2tpIdleTime"), "");
					if ( strTime[0] ) {
						int time;
 						time = strtol(strTime, (char**)NULL, 10) * 60;
						if ( apmib_set(MIB_L2TP_IDLE_TIME, (void *)&time) == 0) {
   							strcpy(tmpBuf, ("Set L2TP idle time MIB error!"));
							goto setErr_tcpip;
						}
					}
				}
			}
			strVal = req_get_cstream_var(wp, ("l2tpMtuSize"), "");
			if ( strVal[0] ) {
				int mtuSize;
 				mtuSize = strtol(strVal, (char**)NULL, 10);
				if ( apmib_set(MIB_L2TP_MTU_SIZE, (void *)&mtuSize) == 0) {
					strcpy(tmpBuf, ("Set L2TP mtu size MIB error!"));
					goto setErr_tcpip;
				}
			}
				
		}

#ifdef RTK_USB3G
        else if ( !strcmp(strMode, ("USB3G"))) {
            char  *strName, *strPassword, *strPIN, *strAPN, *strDialnum;
            dhcp = USB3G;

#if defined(CONFIG_4G_LTE_SUPPORT)
        {
        	int val = 0;
        	apmib_set(MIB_LTE4G, (void *)&val);
        }
#endif

            strName = req_get_cstream_var(wp, ("USB3G_USER"), "");
            //if ( strName[0] ) {
                if ( apmib_set(MIB_USB3G_USER, (void *)strName) == 0) {
                    strcpy(tmpBuf, ("Set USB3G user name MIB error!"));
                    goto setErr_tcpip;
                }
            //}
            strPassword = req_get_cstream_var(wp, ("USB3G_PASS"), "");
            //if ( strPassword[0] ) {
                if ( apmib_set(MIB_USB3G_PASS, (void *)strPassword) == 0) {
                    strcpy(tmpBuf, ("Set USB3G user password MIB error!"));
                    goto setErr_tcpip;
                }
            //}
            strPIN = req_get_cstream_var(wp, ("USB3G_PIN"), "");
            //if ( strPIN[0] ) {
                if ( apmib_set(MIB_USB3G_PIN, (void *)strPIN) == 0) {
                    strcpy(tmpBuf, ("Set USB3G PIN MIB error!"));
                    goto setErr_tcpip;
                }
            //}            
            strAPN = req_get_cstream_var(wp, ("USB3G_APN"), "");
            if ( strAPN[0] ) {
                if ( apmib_set(MIB_USB3G_APN, (void *)strAPN) == 0) {
                    strcpy(tmpBuf, ("Set USB3G APN MIB error!"));
                    goto setErr_tcpip;
                }
            }
            strDialnum = req_get_cstream_var(wp, ("USB3G_DIALNUM"), "");
            if ( strDialnum[0] ) {
                if ( apmib_set(MIB_USB3G_DIALNUM, (void *)strDialnum) == 0) {
                    strcpy(tmpBuf, ("Set USB3G Dial number MIB error!"));
                    goto setErr_tcpip;
                }
            }

            strDialnum = req_get_cstream_var(wp, ("USB3GMtuSize"), "");
            if ( strDialnum[0] ) {
                if ( apmib_set(MIB_USB3G_MTU_SIZE, (void *)strDialnum) == 0) {
                    strcpy(tmpBuf, ("Set USB3G mtu size MIB error!"));
                    goto setErr_tcpip;
                }
            }

            strType = req_get_cstream_var(wp, ("USB3GConnectType"), "");
            if ( strType[0] ) {
                PPP_CONNECT_TYPE_T type;
                if (!strcmp(strType, "0"))
                    type = CONTINUOUS;
                else if (!strcmp(strType, "1"))
                    type = CONNECT_ON_DEMAND;
                else if (!strcmp(strType, "2"))
                    type = MANUAL;
                else {
                    strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_usb)</script>"));
                    goto setErr_tcpip;
                }
                if ( apmib_set(MIB_USB3G_CONN_TYPE, (void *)strType) == 0) {
                    strcpy(tmpBuf, ("Set USB3G type MIB error!"));
                    goto setErr_tcpip;
                }
                if (type != CONTINUOUS) {
                    char *strTime;
                    strTime = req_get_cstream_var(wp, ("USB3GIdleTime"), "");
                    if ( strTime[0] ) {
                        int time;
                        char buffer[8];
                        time = atoi(strTime) * 60;
                        sprintf(buffer, "%d", time);
                        if ( apmib_set(MIB_USB3G_IDLE_TIME, (void *)buffer) == 0) {
                            strcpy(tmpBuf, ("Set USB3G idle time MIB error!"));
                            goto setErr_tcpip;
                        }
                    }
                }
            }
        }
#endif /* #ifdef RTK_USB3G */
#if defined(CONFIG_4G_LTE_SUPPORT)
		else if ( !strcmp(strMode, ("LTE4G"))) {
			dhcp = DHCP_CLIENT;
			lte = 1;
		}
#endif
#ifdef CONFIG_IPV6
#ifdef CONFIG_DSLITE_SUPPORT
	else if ( !strcmp(strMode, ("dslite")))
	{
		int dslite;
		addr6CfgParam_t ipaddr6;
		char *strAFTR;
		dhcp = AFTR;
		strMode = req_get_cstream_var(wp, ("dsliteMode"), "");

		if ( strMode && strMode[0] ) 
		{
			if (!strcmp(strMode, ("dsliteAuto")))
				dslite = 0;
			else if (!strcmp(strMode, ("dsliteManual")))
				dslite = 1;
			else {
				strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_ds)</script>"));
				goto setErr_tcpip;
			}

			if ( !apmib_set(MIB_DSLITE_MODE, (void *)&dslite)) {
	  			strcpy(tmpBuf, "Set DSLITE MODE MIB error!");
				goto setErr_tcpip;
			}

			if(dslite == 1)
			{
				strAFTR = req_get_cstream_var(wp, ("dsliteAftrIpAddr6"), "");
				if(strAFTR[0])
				{
					if(inet_pton6(strAFTR, ipaddr6.addrIPv6) == 0)
					{
						strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_aftr)</script>"));
						goto setErr_tcpip;
					}
					
					if ( !apmib_set(MIB_IPV6_ADDR_AFTR_PARAM, (void *)&ipaddr6))
					{
	  					strcpy(tmpBuf, "Set AFTR MIB error!");
						goto setErr_tcpip;
					}
				}
				else
				{
					if ( !apmib_get(MIB_IPV6_ADDR_AFTR_PARAM, (void *)&ipaddr6) )
					{
						strcpy(tmpBuf, "Get AFTR MIB error!");
						goto setErr_tcpip;
					}
				}
			}
		}
	}
#endif
#endif

		else {
			strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_ip_mode)</script>"));
			goto setErr_tcpip;
		}

#if defined(CONFIG_4G_LTE_SUPPORT)
		if ( !apmib_set(MIB_LTE4G, (void *)&lte)) {
			strcpy(tmpBuf, ("Set MIB_LTE4G error!"));
			goto setErr_tcpip;
		}
#endif

		if ( !apmib_set(MIB_WAN_DHCP, (void *)&dhcp)) {
	  		strcpy(tmpBuf, ("Set DHCP error!"));
			goto setErr_tcpip;
		}
#if defined(ROUTE_SUPPORT)		
	if ( (dhcp == PPPOE) || (dhcp == PPTP) || (dhcp == L2TP) || (dhcp == USB3G) ) {
		curr_nat=1;
		
		if(curr_nat !=orig_nat){//force NAT is enabled when pppoe/pptp/l2tp
			if ( !apmib_set( MIB_NAT_ENABLED, (void *)&curr_nat) ) {
				strcpy(tmpBuf, ("Get NAT MIB error!"));
				goto setErr_tcpip;
			}
			intVal=0;
			if (apmib_set( MIB_RIP_LAN_TX, (void *)&intVal) == 0) {
					strcpy(tmpBuf, ("\"Set RIP LAN Tx error!\""));
					goto setErr_tcpip;
			}
			if (apmib_set( MIB_RIP_WAN_TX, (void *)&intVal) == 0) {
					strcpy(tmpBuf, ("\"Set RIP WAN Tx error!\""));
					goto setErr_tcpip;
			}
			if (!apmib_set(MIB_IGMP_PROXY_DISABLED, (void *)&intVal)) {
				strcpy(tmpBuf, ("Set MIB_IGMP_PROXY_DISABLED error!"));
				goto setErr_tcpip;
			}
		}
	}
#endif	

        if ( buttonState == 1 && (dhcp == PPPOE || dhcp == PPTP || dhcp == L2TP || dhcp == USB3G) ) { // connect button is pressed
			int wait_time=45;  // FOR WISP MODE
			int opmode=0;
			apmib_update_web(CURRENT_SETTING);	// update to flash
			apmib_get(MIB_OP_MODE, (void *)&opmode);
			
#ifdef MULTI_PPPOE
			if(buttonState == 1 && dhcp == PPPOE)
			{
				extern int PPPoE_Number;
				int ppp_num;
				FILE *pF;
				system("ifconfig |grep 'ppp'| cut -d ' ' -f 1 |  wc -l > /etc/ppp/lineNumber");	

				if(flag ==1){
					PPPoE_Number = 1;
					system("echo 1 > /etc/ppp/connfile1");
					system("rm /etc/ppp/disconnect_trigger1 >/dev/null 2>&1");
				}else if(flag ==2){					
					PPPoE_Number = 2;				
					system("echo 1 > /etc/ppp/connfile2");
					system("rm /etc/ppp/disconnect_trigger2 >/dev/null 2>&1");		
				}else if(flag ==3){
					PPPoE_Number = 3;				
					system("echo 1 > /etc/ppp/connfile3");
					system("rm /etc/ppp/disconnect_trigger3 >/dev/null 2>&1");
				}else if(flag ==4){								
					PPPoE_Number = 4;				
					system("echo 1 > /etc/ppp/connfile4");
					system("rm /etc/ppp/disconnect_trigger4 >/dev/null 2>&1");											
				}		
				system("rm /etc/ppp/connectfile >/dev/null 2>&1");
				if((pF = fopen("/etc/ppp/lineNumber","r+")) != NULL)
				{
					fscanf(pF,"%d",&ppp_num);
					if(ppp_num == 0)
					{						
						system("killall -9 ppp_inet 2> /dev/null");
						goto End;
					}
				}
				while (wait_time-- >0) {
					if (isConnectPPP()){
						printf("PPP is connected\n");
						break;
					}
					sleep(1);
				}
				if (isConnectPPP())
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_con_su)</script>"));
				else
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_con_fail)</script>"));	
				OK_MSG1(tmpBuf, submitUrl);			
				return 1;				
			}
End:			
#endif
////			if(opmode==2)
////				WAN_IF = ("wlan0");
			if(opmode==2)
			{
				int wisp_wan_id, wlan_mode;
				char wlan_name[16];
				apmib_get(MIB_WISP_WAN_ID,(void *)&wisp_wan_id);

				sprintf(wlan_name,"wlan%d",wisp_wan_id);
				if(SetWlan_idx(wlan_name))
				{
					apmib_get(MIB_WLAN_MODE,(void *)&wlan_mode);
					if(wlan_mode == CLIENT_MODE)			
						sprintf(tmpbuf, "wlan%d", wisp_wan_id);
					else			
						sprintf(tmpbuf, "wlan%d-vxd", wisp_wan_id);
				}	
				WAN_IF=tmpbuf;	
//				printf("%s:%d wan_if=%s\n",__FUNCTION__,__LINE__,WAN_IF);
			}
			else if(opmode ==0)
				WAN_IF = ("eth1");

			system("killall -9 igmpproxy 2> /dev/null");
			system("echo 1,1 > /proc/br_mCastFastFwd");
			system("killall -9 dnrd 2> /dev/null");
			if(dhcp == PPPOE || dhcp == PPTP)
			{
				//system("killall -15 pppd 2> /dev/null");
			}
        #ifdef RTK_USB3G
            else if (dhcp == USB3G)
                kill_3G_ppp_inet();
        #endif
			else
			{
				//system("killall -9 pppd 2> /dev/null");
			}
				
				system("disconnect.sh option");
#ifndef NO_ACTION
        #ifdef RTK_USB3G
            if (dhcp == USB3G)
                system("ppp_inet -t 16 -c 0 -x");
            else {
        #endif
			pid = fork();
        		if (pid)
	        		waitpid(pid, NULL, 0);
			else if (pid == 0) {
				if(dhcp == PPPOE){
					snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _PPPOE_SCRIPT_PROG);
					execl( tmpBuf, _PPPOE_SCRIPT_PROG, "connect", WAN_IF, NULL);
				}else if(dhcp == PPTP){
					snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _PPTP_SCRIPT_PROG);
					execl( tmpBuf, _PPTP_SCRIPT_PROG, "connect", WAN_IF, NULL);
				}else if(dhcp == L2TP){
					system("killall -9 l2tpd 2> /dev/null");
					system("rm -f /var/run/l2tpd.pid 2> /dev/null");
					snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _L2TP_SCRIPT_PROG);
					execl( tmpBuf, _L2TP_SCRIPT_PROG, "connect", WAN_IF, NULL);
				}
       				exit(1);
			}
        #ifdef RTK_USB3G
            }
        #endif
			while (wait_time-- >0) {
				if (isConnectPPP()){
					printf("PPP is connected\n");
					break;
				}
				sleep(1);
			}
			if (isConnectPPP())
				strcpy(tmpBuf, ("<script>dw(tcpip_wan_con_su)</script>"));
			else
				strcpy(tmpBuf, ("<script>dw(tcpip_wan_con_fail)</script>"));

			OK_MSG1(tmpBuf, submitUrl);
#endif
			return 1;
		}

		if ( buttonState == 2 && (dhcp == PPPOE || dhcp == PPTP || dhcp == L2TP || dhcp == USB3G) ) { // disconnect button is pressed
			apmib_update_web(CURRENT_SETTING);	// update to flash


#ifdef MULTI_PPPOE
		if ( buttonState == 2 && dhcp == PPPOE)
		{
			char ppp_name[5];
			int orderNumber,pppNumbers,index;
			FILE *order,*number;			
			int wait_time=30;
			if((order=fopen("/etc/ppp/ppp_order_info","r+"))==NULL)
			{
				printf("Cannot open this file\n");
				goto end;
			}
			if((number=fopen("/etc/ppp/lineNumber","r+"))==NULL)
			{
				printf("Cannot open this file\n");
				goto end;
			}
			fscanf(number,"%d",&pppNumbers);
			close(order);
			close(number);										
			for( index = 0 ; index < pppNumbers ; ++index)
			{			
				fscanf(order,"%d--%s",&orderNumber,ppp_name);
				if(flag == orderNumber)
				{
					int pid;
					char path[50],cmd[50];
					FILE *pidF;				
					extern int PPPoE_Number;
					sprintf(path,"/var/run/%s.pid",ppp_name);
					if((pidF=fopen(path,"r+")) == NULL)
						goto end;									
					fscanf(pidF,"%d",&pid);
					if(flag ==1){
						system("echo 1 > /etc/ppp/disconnect_trigger1");
						PPPoE_Number = 1;
					}
					else if(flag == 2){
						system("echo 1 > /etc/ppp/disconnect_trigger2");
						PPPoE_Number = 2;
					}
					else if(flag ==3){
						system("echo 1 > /etc/ppp/disconnect_trigger3");
						PPPoE_Number = 3;
					}
					else if(flag ==4){
						system("echo 1 > /etc/ppp/disconnect_trigger4");
						PPPoE_Number = 4;
					}
					sprintf(cmd,"kill %d  2> /dev/null",pid);
					system(cmd);									
					system("rm /etc/ppp/connectfile >/dev/null 2>&1");	
					while (wait_time-- >0) {
						if (!isConnectPPP()){
							printf("PPP is disconnected\n");
							break;
						}
						sleep(1);
					}
					if (!isConnectPPP())
						strcpy(tmpBuf, ("<script>dw(tcpip_wan_ppp_discon)</script>"));
					else
						strcpy(tmpBuf, ("<script>dw(tcpip_wan_unkonwn)</script>"));
	
					OK_MSG1(tmpBuf, submitUrl);			
					return 1;			
				}
			}

		}
end:
#endif

#ifndef NO_ACTION
        #ifdef RTK_USB3G
            if (dhcp == USB3G)
                kill_3G_ppp_inet();
            else
        #endif
			//if(dhcp != PPTP)
			if(1)
			{
			pid = fork();
        		if (pid)
	             		waitpid(pid, NULL, 0);
        		else if (pid == 0) {
				snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _PPPOE_DC_SCRIPT_PROG);
				execl( tmpBuf, _PPPOE_DC_SCRIPT_PROG, "all", NULL);
                		exit(1);
        		}
        	}else{
        		system("killall -15 ppp_inet 2> /dev/null");
        		system("killall -15 pppd 2> /dev/null");
        	}

        		if(dhcp == PPPOE)	
			strcpy(tmpBuf, ("<script>dw(tcpip_wan_ppp_discon)</script>"));
			if(dhcp == PPTP)	
			strcpy(tmpBuf, ("<script>dw(tcpip_wan_pptp_discon)</script>"));
			if(dhcp == L2TP)	
			strcpy(tmpBuf, ("<script>dw(tcpip_wan_l2tp_discon)</script>"));
            if(dhcp == USB3G)    
                strcpy(tmpBuf, ("<script>dw(tcpip_wan_usb3g_discon)</script>"));

			OK_MSG1(tmpBuf, submitUrl);
#endif
			return 1;
		}
	}
	else
		dhcp = curDhcp;

	if ( dhcp == DHCP_DISABLED ) {
		strIp = req_get_cstream_var(wp, ("wan_ip"), "");
		if ( strIp[0] ) {
			if ( !inet_aton(strIp, &inIp) ) {
				strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
				goto setErr_tcpip;
			}
			if ( !apmib_set(MIB_WAN_IP_ADDR, (void *)&inIp)) {
				strcpy(tmpBuf, ("Set IP-address error!"));
				goto setErr_tcpip;
			}
		}

		strMask = req_get_cstream_var(wp, ("wan_mask"), "");
		if ( strMask[0] ) {
			if ( !inet_aton(strMask, &inMask) ) {
				strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_subnet)</script>"));
				goto setErr_tcpip;
			}
			if ( !apmib_set(MIB_WAN_SUBNET_MASK, (void *)&inMask)) {
				strcpy(tmpBuf, ("Set subnet-mask error!"));
				goto setErr_tcpip;
			}
		}

		strGateway = req_get_cstream_var(wp, ("wan_gateway"), "");
		if ( strGateway[0] ) {
			if ( !inet_aton(strGateway, &inGateway) ) {
				strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_gw)</script>"));
				goto setErr_tcpip;
			}
			if ( !apmib_set(MIB_WAN_DEFAULT_GATEWAY, (void *)&inGateway)) {
				strcpy(tmpBuf, ("Set default-gateway error!"));
				goto setErr_tcpip;
			}
		}

		strVal = req_get_cstream_var(wp, ("fixedIpMtuSize"), "");
		if ( strVal[0] ) {
			int mtuSize;
			mtuSize = strtol(strVal, (char**)NULL, 10);
			if ( apmib_set(MIB_FIXED_IP_MTU_SIZE, (void *)&mtuSize) == 0) {
				strcpy(tmpBuf, ("Set FIXED-IP mtu size MIB error!"));
				goto setErr_tcpip;
			}
		}		
	}
	
	if (!call_from_wizard) { // not called from wizard
		if (dhcp == DHCP_CLIENT) {
			strVal = req_get_cstream_var(wp, ("dhcpMtuSize"), "");
			if ( strVal ) {
				int mtuSize;
				mtuSize = strtol(strVal, (char**)NULL, 10);
				if ( apmib_set(MIB_DHCP_MTU_SIZE, (void *)&mtuSize) == 0) {
					strcpy(tmpBuf, ("Set DHCP mtu size MIB error!"));
					goto setErr_tcpip;
				}
			}
			
			strVal = req_get_cstream_var(wp, ("hostName"), "");
			if (strVal) {
				if (!isValidName(strVal)) {
  					strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_host)</script>"));
					goto setErr_tcpip;				
				}			
				if ( !apmib_set(MIB_HOST_NAME, (void *)strVal)) {
  					strcpy(tmpBuf, ("Set MIB_HOST_NAME MIB error!"));
					goto setErr_tcpip;
				}
			}else{
				 if ( !apmib_set(MIB_HOST_NAME, (void *)"")){
	  					strcpy(tmpBuf, ("\"Set MIB_HOST_NAME MIB error!\""));
						goto setErr_tcpip;
				}	
			}					
		}	
#ifdef _ALPHA_DUAL_WAN_SUPPORT_
		strVal = req_get_cstream_var(wp, ("pppoeWithDhcpEnabled"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;
		if ( !apmib_set(MIB_PPPOE_DHCP_ENABLED, (void *)&intVal)) {
			strcpy(tmpBuf, ("Set MIB_PPPOE_DHCP_ENABLED error!"));
			goto setErr_tcpip;
		}
#endif
		strVal = req_get_cstream_var(wp, ("upnpEnabled"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;
		if ( !apmib_set(MIB_UPNP_ENABLED, (void *)&intVal)) {
			strcpy(tmpBuf, ("Set MIB_UPNP_ENABLED error!"));
			goto setErr_tcpip;
		}
//Brad add for igmpproxy
		strVal = req_get_cstream_var(wp, ("igmpproxyEnabled"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 0;
		else
			intVal = 1;
#if defined(ROUTE_SUPPORT)	
		int nat_mode = 0;
		if(!apmib_get(MIB_NAT_ENABLED, (void *)&nat_mode))
		{
			strcpy(tmpBuf, ("Get MIB_NAT_ENABLED error!"));
			goto setErr_tcpip;
		}
		if(nat_mode == 0 && intVal == 0)
		{
			strcpy(tmpBuf, ("<script>dw(tcpip_wan_igmpproxy)</script>"));
			goto setErr_tcpip;
		}
#endif
		if ( !apmib_set(MIB_IGMP_PROXY_DISABLED, (void *)&intVal)) {
			strcpy(tmpBuf, ("Set MIB_IGMP_PROXY_DISABLED error!"));
			goto setErr_tcpip;
		}
//Brad add end
		strVal = req_get_cstream_var(wp, ("webWanAccess"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;
		if ( !apmib_set(MIB_WEB_WAN_ACCESS_ENABLED, (void *)&intVal)) {
			strcpy(tmpBuf, ("Set WEB_WAN_ACCESS_ENABLED error!"));
			goto setErr_tcpip;
		}
		#if defined(CONFIG_RTL_WEB_WAN_ACCESS_PORT)
		if (intVal){
			strVal = req_get_cstream_var(wp, ("webAccessPort"), "");
			if (!string_to_dec(strVal, &WebPort) || WebPort<1 || WebPort>65535) {
				strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_port)</script>"));
				goto setErr_tcpip;
			}
			if ( !apmib_set(MIB_WEB_WAN_ACCESS_PORT, (void *)&WebPort)) {
				strcpy(tmpBuf, ("Set WEB_WAN_ACCESS_ENABLED error!"));
				goto setErr_tcpip;
			}
		}
		#endif
		strVal = req_get_cstream_var(wp, ("pingWanAccess"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;
		if ( !apmib_set(MIB_PING_WAN_ACCESS_ENABLED, (void *)&intVal)) {
			strcpy(tmpBuf, ("Set PING_WAN_ACCESS_ENABLED error!"));
			goto setErr_tcpip;
		}		
			
		strVal = req_get_cstream_var(wp, ("WANPassThru1"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;	
		if ( !apmib_set(MIB_VPN_PASSTHRU_IPSEC_ENABLED, (void *)&intVal)) {
			strcpy(tmpBuf, ("Set VPN_PASSTHRU_IPSEC_ENABLED error!"));
			goto setErr_tcpip;
		}

		strVal = req_get_cstream_var(wp, ("WANPassThru2"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;
		if ( !apmib_set(MIB_VPN_PASSTHRU_PPTP_ENABLED, (void *)&intVal)) {
			strcpy(tmpBuf, ("Set VPN_PASSTHRU_PPTP_ENABLED error!"));
			goto setErr_tcpip;
		}
		
		strVal = req_get_cstream_var(wp, ("WANPassThru3"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;
		if ( !apmib_set(MIB_VPN_PASSTHRU_L2TP_ENABLED, (void *)&intVal)) {
			strcpy(tmpBuf, ("Set VPN_PASSTHRU_L2TP_ENABLED error!"));
			goto setErr_tcpip;
		}
#if !defined(CONFIG_IPV6)
		strVal = req_get_cstream_var(wp, ("ipv6_passthru_enabled"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;
	        if ( !apmib_set(MIB_CUSTOM_PASSTHRU_ENABLED, (void *)&intVal))
	        {
	                strcpy(tmpBuf, ("Set custom passthru enabled error!"));
	                goto setErr_tcpip;
	        }
#endif
#ifdef CONFIG_RTL_ETH_802DOT1X_CLIENT_MODE_SUPPORT
		strVal = req_get_cstream_var(wp, ("WANDot1xEnabled"), "");
		apmib_get(MIB_ELAN_DOT1X_MODE,(void *)&dot1x_mode);
		apmib_get(MIB_ELAN_ENABLE_1X,(void *)&dot1x_enable);
		#if 0
		if(strcmp(strVal,"ON")){
			intVal = 1;
			dot1x_mode |= ETH_DOT1X_CLIENT_MODE;
		}
		else{
			dot1x_mode &= (~ETH_DOT1X_CLIENT_MODE);
			if(dot1x_mode)
				intVal = 1;
			else
				intVal = 0;
		}
		#else
		if(strcmp(strVal,"ON"))
		{
			dot1x_enable |= ETH_DOT1X_CLIENT_MODE_ENABLE_BIT;
			dot1x_mode |= ETH_DOT1X_CLIENT_MODE_BIT;
		}
		else
		{
			dot1x_enable &= ~ETH_DOT1X_CLIENT_MODE_ENABLE_BIT;
			dot1x_mode &= (~ETH_DOT1X_CLIENT_MODE_BIT);
		}
		#endif		
		apmib_set(MIB_ELAN_DOT1X_MODE,(void *)&dot1x_mode);
		apmib_set(MIB_ELAN_ENABLE_1X,(void *)&dot1x_enable);
		
		strVal = req_get_cstream_var(wp, "eapType", "");
		if (strVal[0]) {
				if ( !string_to_dec(strVal, &intVal) ) {
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_8021x)</script>"));
					goto setErr_tcpip;
				}
				if ( !apmib_set(MIB_ELAN_EAP_TYPE, (void *)&intVal)) {
					strcpy(tmpBuf, ("Set MIB_ELAN_EAP_TYPE error!"));
					goto setErr_tcpip;
				}
			}
			else{
				strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x)</script>"));
				goto setErr_tcpip;
			}

			if(intVal == EAP_MD5){
				strVal = req_get_cstream_var(wp, "eapUserId", "");
				if (strVal[0]) {
					if(strlen(strVal)>MAX_EAP_USER_ID_LEN){
						strcpy(tmpBuf, ("EAP user ID too long!"));
						goto setErr_tcpip;
					}
					if ( !apmib_set(MIB_ELAN_EAP_USER_ID, (void *)strVal)) {
						strcpy(tmpBuf, ("Set MIB_ELAN_EAP_USER_ID error!"));
						goto setErr_tcpip;
					}
				}
				else{
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_user)</script>"));
					goto setErr_tcpip;
				}
				
				strVal = req_get_cstream_var(wp, "radiusUserName", "");
				if (strVal[0]) {
					if(strlen(strVal)>MAX_RS_USER_NAME_LEN){
						strcpy(tmpBuf, ("RADIUS user name too long!"));
						goto setErr_tcpip;
					}
					if ( !apmib_set(MIB_ELAN_RS_USER_NAME, (void *)strVal)) {
						strcpy(tmpBuf, ("Set MIB_ELAN_RS_USER_NAME error!"));
						goto setErr_tcpip;
					}
				}
				else{
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_user2)</script>"));
					goto setErr_tcpip;
				}

				
				strVal = req_get_cstream_var(wp, "radiusUserPass", "");
				if (strVal[0]) {
					if(strlen(strVal)>MAX_RS_USER_PASS_LEN){
						strcpy(tmpBuf, ("RADIUS user password too long!"));
						goto setErr_tcpip;
					}
					if ( !apmib_set(MIB_ELAN_RS_USER_PASSWD, (void *)strVal)) {
						strcpy(tmpBuf, ("Set MIB_ELAN_RS_USER_PASSWD error!"));
						goto setErr_tcpip;
					}
				}
				else{
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_user3)</script>"));
					goto setErr_tcpip;
				}
			}
			else if(intVal == EAP_TLS){
			
				strVal = req_get_cstream_var(wp, "eapUserId", "");
				if (strVal[0]) {
					if(strlen(strVal)>MAX_EAP_USER_ID_LEN){
						strcpy(tmpBuf, ("EAP user ID too long!"));
						goto setErr_tcpip;
					}
					if ( !apmib_set(MIB_ELAN_EAP_USER_ID, (void *)strVal)) {
						strcpy(tmpBuf, ("Set MIB_ELAN_EAP_USER_ID error!"));
						goto setErr_tcpip;
					}
				}
				else{
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_user)</script>"));
					goto setErr_tcpip;
				}
				
				strVal = req_get_cstream_var(wp, "radiusUserCertPass", "");
				if (strVal[0]) {
					if(strlen(strVal)>MAX_RS_USER_CERT_PASS_LEN){
						strcpy(tmpBuf, ("RADIUS user cert password too long!"));
						goto setErr_tcpip;
					}
					if ( !apmib_set(MIB_ELAN_RS_USER_CERT_PASSWD, (void *)strVal)) {
						strcpy(tmpBuf, ("Set MIB_ELAN_RS_USER_CERT_PASSWD error!"));
						goto setErr_tcpip;
					}
				}
				else{
					if ( !apmib_set(MIB_ELAN_RS_USER_CERT_PASSWD, (void *)strVal)) {
						strcpy(tmpBuf, ("Clear MIB_ELAN_RS_USER_CERT_PASSWD error!"));
						goto setErr_tcpip;
					}
					//strcpy(tmpBuf, ("No 802.1x RADIUS user cert password!"));
					//goto setErr_encrypt;
				}

				
					if(isFileExist(RS_USER_CERT_ETH) != 1){
						strcpy(tmpBuf, ("No 802.1x RADIUS ethernet user cert!\nPlease upload it."));
						goto setErr_tcpip;
					}
					
					if(isFileExist(RS_ROOT_CERT_ETH) != 1){
						strcpy(tmpBuf, ("No 802.1x RADIUS ethernet root cert!\nPlease upload it."));
						goto setErr_tcpip;
					}
				
				
			}
			else if(intVal == EAP_PEAP){
				strVal = req_get_cstream_var(wp, "eapInsideType", "");
				if (strVal[0]) {
					if ( !string_to_dec(strVal, &intVal2) ) {
						strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_tunnel)</script>"));
						goto setErr_tcpip;
					}
					if ( !apmib_set(MIB_ELAN_EAP_INSIDE_TYPE, (void *)&intVal2)) {
						strcpy(tmpBuf, ("Set MIB_ELAN_EAP_INSIDE_TYPE error!"));
						goto setErr_tcpip;
					}
				}
				else{
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_tunnel2)</script>"));
					goto setErr_tcpip;
				}

				if(intVal2 == INSIDE_MSCHAPV2){
					strVal = req_get_cstream_var(wp, "eapUserId", "");
					if (strVal[0]) {
						if(strlen(strVal)>MAX_EAP_USER_ID_LEN){
							strcpy(tmpBuf, ("EAP user ID too long!"));
							goto setErr_tcpip;
						}
						if ( !apmib_set(MIB_ELAN_EAP_USER_ID, (void *)strVal)) {
							strcpy(tmpBuf, ("Set MIB_ELAN_EAP_USER_ID error!"));
							goto setErr_tcpip;
						}
					}
					else{
						strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_user)</script>"));
						goto setErr_tcpip;
					}
					
					strVal = req_get_cstream_var(wp, "radiusUserName", "");
					if (strVal[0]) {
						if(strlen(strVal)>MAX_RS_USER_NAME_LEN){
							strcpy(tmpBuf, ("RADIUS user name too long!"));
							goto setErr_tcpip;
						}
						if ( !apmib_set(MIB_ELAN_RS_USER_NAME, (void *)strVal)) {
							strcpy(tmpBuf, ("Set MIB_ELAN_RS_USER_NAME error!"));
							goto setErr_tcpip;
						}
					}
					else{
						strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_user2)</script>"));
						goto setErr_tcpip;
					}

					strVal = req_get_cstream_var(wp, "radiusUserPass", "");
					if (strVal[0]) {
						if(strlen(strVal)>MAX_RS_USER_PASS_LEN){
							strcpy(tmpBuf, ("RADIUS user password too long!"));
							goto setErr_tcpip;
						}
						if ( !apmib_set(MIB_ELAN_RS_USER_PASSWD, (void *)strVal)) {
							strcpy(tmpBuf, ("Set MIB_ELAN_RS_USER_PASSWD error!"));
							goto setErr_tcpip;
						}
					}
					else{
						strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_user3)</script>"));
						goto setErr_tcpip;
					}

//					if(isFileExist(RS_USER_CERT) == 1){
						strVal = req_get_cstream_var(wp, "radiusUserCertPass", "");
						if (strVal[0]) {
							if(strlen(strVal)>MAX_RS_USER_CERT_PASS_LEN){
								strcpy(tmpBuf, ("RADIUS user cert password too long!"));
								goto setErr_tcpip;
							}
							if ( !apmib_set(MIB_ELAN_RS_USER_CERT_PASSWD, (void *)strVal)) {
								strcpy(tmpBuf, ("Set MIB_ELAN_RS_USER_CERT_PASSWD error!"));
								goto setErr_tcpip;
							}
						}
						else{
							if ( !apmib_set(MIB_ELAN_RS_USER_CERT_PASSWD, (void *)strVal)) {
								strcpy(tmpBuf, ("[1] Clear MIB_ELAN_RS_USER_CERT_PASSWD error!"));
								goto setErr_tcpip;
							}
							//strcpy(tmpBuf, ("No 802.1x RADIUS user cert password!"));
							//goto setErr_encrypt;
						}
//					}
				}
				else{
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_tunnel3)</script>"));
					goto setErr_tcpip;
				}
			}
			else if (intVal == EAP_TTLS){
				strVal = req_get_cstream_var(wp, "eapPhase2Type", "");
				if (strVal[0]) {
					if ( !string_to_dec(strVal, &intVal2) ) {
						strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_phase)</script>"));
						goto setErr_tcpip;
					}
					if ( !apmib_set(MIB_ELAN_EAP_PHASE2_TYPE, (void *)&intVal2)) {
						strcpy(tmpBuf, ("Set MIB_ELAN_EAP_INSIDE_TYPE error!"));
						goto setErr_tcpip;
					}
				}
				else{
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_phase2)</script>"));
					goto setErr_tcpip;
				}

				if(intVal2 == TTLS_PHASE2_EAP){
					val = TTLS_PHASE2_EAP_MD5;
					apmib_set(MIB_ELAN_PHASE2_EAP_METHOD,(void *)&val);
					strVal = req_get_cstream_var(wp, "eapUserId", "");
					if (strVal[0]) {
						if(strlen(strVal)>MAX_EAP_USER_ID_LEN){
							strcpy(tmpBuf, ("<script>dw(tcpip_wan_eap_long)</script>"));
							goto setErr_tcpip;
						}
						if ( !apmib_set(MIB_ELAN_EAP_USER_ID, (void *)strVal)) {
							strcpy(tmpBuf, ("Set MIB_ELAN_EAP_USER_ID error!"));
							goto setErr_tcpip;
						}
					}
					else{
						strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_user)</script>"));
						goto setErr_tcpip;
					}
					
					strVal = req_get_cstream_var(wp, "radiusUserName", "");
					if (strVal[0]) {
						if(strlen(strVal)>MAX_RS_USER_NAME_LEN){
							strcpy(tmpBuf, ("RADIUS user name too long!"));
							goto setErr_tcpip;
						}
						if ( !apmib_set(MIB_ELAN_RS_USER_NAME, (void *)strVal)) {
							strcpy(tmpBuf, ("Set MIB_ELAN_RS_USER_NAME error!"));
							goto setErr_tcpip;
						}
					}
					else{
						strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_user2)</script>"));
						goto setErr_tcpip;
					}

					strVal = req_get_cstream_var(wp, "radiusUserPass", "");
					if (strVal[0]) {
						if(strlen(strVal)>MAX_RS_USER_PASS_LEN){
							strcpy(tmpBuf, ("RADIUS user password too long!"));
							goto setErr_tcpip;
						}
						if ( !apmib_set(MIB_ELAN_RS_USER_PASSWD, (void *)strVal)) {
							strcpy(tmpBuf, ("Set MIB_ELAN_RS_USER_PASSWD error!"));
							goto setErr_tcpip;
						}
					}
					else{
						strcpy(tmpBuf, ("<script>dw(tcpip_wan_8021x_user3)</script>"));
						goto setErr_tcpip;
					}
					if(isFileExist(RS_ROOT_CERT_ETH) != 1){
						strcpy(tmpBuf, ("No 802.1x RADIUS ethernet root cert!\nPlease upload it."));
						goto setErr_tcpip;
					}
//					
				}
				else{
					strcpy(tmpBuf, ("<script>dw(tcpip_wan_ttls_phase2)</script>"));
					goto setErr_tcpip;
				}
			}
			else{
				strcpy(tmpBuf, ("<script>dw(tcpip_wan_eap_nsupport)</script>"));
				goto setErr_tcpip;
			}
#endif
		
	}
	return 0 ;
setErr_tcpip:
	return -1 ;	
}	



////////////////////////////////////////////////////////////////////////////////

void formWanTcpipSetup(request *wp, char *path, char *query)
{


	char tmpBuf[100];
	int dns_changed=0;
	char *arg;
	char *submitUrl;
	int val ;
		
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page

	if((val = tcpipWanHandler(wp, tmpBuf, &dns_changed)) < 0 )
		goto setErr_end ;
	else if (val == 1) // return ok
		return ;

	apmib_update_web(CURRENT_SETTING);	// update to flash

#ifdef MODULES_REINIT
		set_reinit_flag(REINIT_FLAG_NAT|REINIT_FLAG_WAN|REINIT_FLAG_APP);
#endif

	// run script
	if ( dns_changed )
		arg = "all";
	else
		arg = "wan";

#ifdef UNIVERSAL_REPEATER
	apmib_get(MIB_REPEATER_ENABLED1, (void *)&val);
	if (val) 
		arg = "all";	
#endif	

#ifndef NO_ACTION
	run_init_script(arg);                
#endif

	OK_MSG(submitUrl);

	return;
setErr_end:
	ERR_MSG(tmpBuf);
}
#endif
#ifdef SUPPORT_DHCP_PORT_IP_BIND
int checkSameIpOrPort(struct in_addr *IpAddr, int port_num, int entryNum)
{
	if(IpAddr==NULL || entryNum<1)
		return 0;
	int i;
	DHCPRSVDIP_T entry;
	
	for (i=1; i<=entryNum; i++) 
	{
		*((char *)&entry) = (char)i;
		if(!apmib_get(MIB_DHCPRSVDIP_TBL, (void *)&entry))
		{
			printf("get mib MIB_DHCPRSVDIP_TBL fail!\n");
			return -1;
		}
		if(memcmp(IpAddr, entry.ipAddr, 4)==0)
			return 1;
		if(port_num==entry.portId)
			return 3;
	}
	return 0;
}
#endif
int checkSameIpOrMac(struct in_addr *IpAddr, char *macAddr, int entryNum)
{
	if(IpAddr==NULL || macAddr==NULL)
		return 4;

	int i;
	DHCPRSVDIP_T entry;
	struct in_addr start_ip, end_ip, router_ip, netmask;
	
	for (i=1; i<=entryNum; i++) 
	{
		*((char *)&entry) = (char)i;
		if(!apmib_get(MIB_DHCPRSVDIP_TBL, (void *)&entry))
		{
			printf("get mib MIB_DHCPRSVDIP_TBL fail!\n");
			return -1;
		}
		if(memcmp(IpAddr, entry.ipAddr, 4)==0)
			return 1;
		if(memcmp(macAddr, entry.macAddr, 6)==0)
			return 2;
	}
	
	apmib_get(MIB_DHCP_CLIENT_START,  (void *)&start_ip);
	apmib_get(MIB_DHCP_CLIENT_END,  (void *)&end_ip);	
	apmib_get(MIB_IP_ADDR,  (void *)&router_ip);
	apmib_get(MIB_SUBNET_MASK,  (void *)&netmask);

	if(((IpAddr->s_addr & netmask.s_addr) != (router_ip.s_addr & netmask.s_addr)))
		return 5;
	
	if(((IpAddr->s_addr) & (~netmask.s_addr))<((start_ip.s_addr) & (~netmask.s_addr)) 
		|| ((IpAddr->s_addr) & (~netmask.s_addr)) >((end_ip.s_addr) & (~netmask.s_addr)) 
		|| ((IpAddr->s_addr) & (~netmask.s_addr))==((router_ip.s_addr) & (~netmask.s_addr))
		)
		return 5;
	
	return 0;
}
//////////////////////////////////////////////////////////////////////////////
//Static DHCP 
void formStaticDHCP(request *wp, char *path, char *query)
{
	char *strStp, *strIp, *strHostName, *strAddRsvIP, *strDelRsvIP, *strDelAllRsvIP, *strVal, *submitUrl;
	char tmpBuf[100];
	char buffer[100];
	int entryNum, i, stp;
	DHCPRSVDIP_T staticIPEntry, delEntry;
	struct in_addr inIp;
	struct in_addr inLanaddr_orig;
	struct in_addr inLanmask_orig;
	int retval;
	strAddRsvIP = req_get_cstream_var(wp, ("addRsvIP"), "");
	strDelRsvIP = req_get_cstream_var(wp, ("deleteSelRsvIP"), "");
	strDelAllRsvIP = req_get_cstream_var(wp, ("deleteAllRsvIP"), "");

//displayPostDate(wp->post_data);

	apmib_get( MIB_IP_ADDR,  (void *)buffer); //save the orig lan subnet
	memcpy((void *)&inLanaddr_orig, buffer, 4);
	
	apmib_get( MIB_SUBNET_MASK,  (void *)buffer); //save the orig lan mask
	memcpy((void *)&inLanmask_orig, buffer, 4);
	
	// Set static DHCP
	strStp = req_get_cstream_var(wp, ("static_dhcp"), "");
	if (strStp[0]) {
		if (strStp[0] == '0')
			stp = 0;
		else
			stp = 1;
		if ( !apmib_set(MIB_DHCPRSVDIP_ENABLED, (void *)&stp)) {
			strcpy(tmpBuf, ("Set static DHCP mib error!"));
			goto setErr_rsv;
		}
	}
#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strAddRsvIP[0]==0){
		strAddRsvIP = req_get_cstream_var(wp, ("addRsvIPFlag"), "");
	}
#endif
	if (strAddRsvIP[0]) {
		memset(&staticIPEntry, '\0', sizeof(staticIPEntry));	
		strHostName = (char *)req_get_cstream_var(wp, ("hostname"), "");	
	    if (strlen(strHostName)>MAX_NAME_LEN){
			strcpy(tmpBuf, ("<script>dw(tcpip_lan_host_error)</script>"));
	        goto setErr_rsv;
	    }
		if (strHostName[0])
			snprintf((char *)staticIPEntry.hostName, sizeof(staticIPEntry.hostName), "%s", strHostName);				
		strIp = req_get_cstream_var(wp,( "ip_addr"), "");
		if (strIp[0]) {
			inet_aton(strIp, &inIp);
			memcpy(staticIPEntry.ipAddr, &inIp, 4);
		}
		strVal = req_get_cstream_var(wp, ("mac_addr"), "");
		if ( !strVal[0] ) {
	//		strcpy(tmpBuf, ("Error! No mac address to set."));
			goto setac_ret;
		}
#ifdef  SUPPORT_DHCP_PORT_IP_BIND
		if(strlen(strVal)==1)
			staticIPEntry.portId=atoi(strVal);
		else
#endif
		if (strlen(strVal)!=12 || !string_to_hex(strVal, staticIPEntry.macAddr, 12)) {
			strcpy(tmpBuf, ("<script>dw(tcpip_wan_clone_mac_error)</script>"));
			goto setErr_rsv;
		}
		if ( !apmib_get(MIB_DHCPRSVDIP_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get entry number error!"));
			goto setErr_rsv;
		}
		if ( (entryNum + 1) > MAX_DHCP_RSVD_IP_NUM) {
			strcpy(tmpBuf, ("<script>dw(tcpip_entry_table_full)</script>"));
			goto setErr_rsv;
		}
		if((inLanaddr_orig.s_addr & inLanmask_orig.s_addr) != (inIp.s_addr & inLanmask_orig.s_addr)){
			strcpy(tmpBuf, ("<script>dw(tcpip_entry_ip_error)</script>"));
			goto setErr_rsv;
		}
	
#ifdef SUPPORT_DHCP_PORT_IP_BIND
		if(staticIPEntry.portId>0)
			retval=checkSameIpOrPort(&inIp, staticIPEntry.portId, entryNum);
		else
#endif	
		retval=checkSameIpOrMac(&inIp, staticIPEntry.macAddr, entryNum);
		if(retval>0)
		{
			if(retval==1)
				strcpy(tmpBuf, ("<script>dw(tcpip_ip_setted)</script>"));
			if(retval==2)				
				strcpy(tmpBuf, ("<script>dw(tcpip_mac_setted)</script>"));
			if(retval==3)
				strcpy(tmpBuf, ("<script>dw(tcpip_port_setted)</script>"));
			if(retval==5)
				strcpy(tmpBuf, ("<script>dw(tcpip_ip_error)</script>"));
			if(retval==4)
				strcpy(tmpBuf, ("<script>dw(tcpip_ip_mac_error)</script>"));
			
			goto setErr_rsv;
		}

#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
		DHCPRSVDIP_T tmp_staticIPEntry = {0};
		int tmp_num = 1;
		for(i = 1; i <= entryNum; i++){
			*(char *)&tmp_staticIPEntry = (char)i;
			apmib_get(MIB_DHCPRSVDIP_TBL, (void *)&tmp_staticIPEntry);
			if(tmp_staticIPEntry.usedFor == eUsedFor_DHCPServer){
				tmp_num++;
			//	printf("[%s:%d] i = %d, tmp_num = %d\n", __FILE__, __LINE__, i, tmp_num);
			}
		}
		staticIPEntry.dhcpRsvdIpEntryEnabled = 1;
		staticIPEntry.usedFor = eUsedFor_DHCPServer;
		staticIPEntry.InstanceNum = tmp_num;
#endif
	
		// set to MIB. try to delete it first to avoid  case
		apmib_set(MIB_DHCPRSVDIP_DEL, (void *)&staticIPEntry);
		if ( apmib_set(MIB_DHCPRSVDIP_ADD, (void *)&staticIPEntry) == 0) {
			strcpy(tmpBuf, ("Add table entry error!"));
			goto setErr_rsv;
		}
	}

	/* Delete entry */
	if (strDelRsvIP[0]) {
		if ( !apmib_get(MIB_DHCPRSVDIP_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get entry number error!"));
			goto setErr_rsv;
		}
		for (i=entryNum; i>0; i--) {
			snprintf(tmpBuf, 20, "select%d", i);
			memset(&delEntry, '\0', sizeof(delEntry));	
			strVal = req_get_cstream_var(wp, tmpBuf, "");
			if ( !strcmp(strVal, "ON") ) {

				*((char *)&delEntry) = (char)i;
				if ( !apmib_get(MIB_DHCPRSVDIP_TBL, (void *)&delEntry)) {
					strcpy(tmpBuf, ("Get table entry error!"));
					goto setErr_rsv;
				}
				if ( !apmib_set(MIB_DHCPRSVDIP_DEL, (void *)&delEntry)) {
					strcpy(tmpBuf, ("Delete table entry error!"));
					goto setErr_rsv;
				}
		#ifdef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
				DHCPRSVDIP_T mod_staticIPEntry[2] = {0};
				int j;
				apmib_get(MIB_DHCPRSVDIP_TBL_NUM, (void *)&entryNum);
				for(j = 1; j <= entryNum; j++){
					*(char *)&mod_staticIPEntry[0] = (char)j;
					apmib_get(MIB_DHCPRSVDIP_TBL, (void *)&mod_staticIPEntry[0]);
					/*
					printf("[%s:%d] j = %d, i = %d, mod_staticIPEntry[0].InstanceNum = %d\n", __FILE__,
						__LINE__, j, i, mod_staticIPEntry[0].InstanceNum);
					*/			
					if(mod_staticIPEntry[0].usedFor == eUsedFor_DHCPServer &&
						mod_staticIPEntry[0].dhcpConSPInstNum == 0 && 
						mod_staticIPEntry[0].InstanceNum > i){
						memcpy( &mod_staticIPEntry[1], &mod_staticIPEntry[0], sizeof(DHCPRSVDIP_T) );
						mod_staticIPEntry[1].InstanceNum--;
						/*
						printf("[%s:%d] mod_staticIPEntry[1].InstanceNum = %d\n", __FILE__,
						__LINE__, mod_staticIPEntry[1].InstanceNum);
						*/
						
						apmib_set(MIB_DHCPRSVDIP_MOD, (void *)&mod_staticIPEntry);
					}
				}
		#endif
			}
		}
	}

	/* Delete all entry */
	if ( strDelAllRsvIP[0]) {
		if ( !apmib_set(MIB_DHCPRSVDIP_DELALL, (void *)&staticIPEntry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_rsv;
		}
	}

setac_ret:
	apmib_update_web(CURRENT_SETTING);

#ifndef NO_ACTION
	run_init_script("all");
#endif

	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	OK_MSG( submitUrl );
  	return;

setErr_rsv:
	ERR_MSG(tmpBuf);
}


int dhcpRsvdIp_List(request *wp, int argc, char **argv)
{
	int	entryNum, i;
	int nBytesSent=0;
	DHCPRSVDIP_T entry;
	char macaddr[30];
	apmib_get(MIB_DHCPRSVDIP_TBL_NUM, (void *)&entryNum);
	nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
      	"<td align=center width=\"25%%\" ><font size=\"2\"><b><script>dw(dhcp_ip)</script></b></font></td>\n"
#ifdef SUPPORT_DHCP_PORT_IP_BIND
      	"<td align=center width=\"45%%\" ><font size=\"2\"><b><script>dw(tcpip_dhcp_mac_port2)</script></b></font></td>\n"
#else	
	"<td align=center width=\"45%%\" ><font size=\"2\"><b><script>dw(tcpip_dhcp_mac_addr2)</script></b></font></td>\n"
#endif
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(tcpip_dhcp_comment)</script></b></font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(tcpip_dhcp_select)</script></b></font></td></tr>\n"));
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		apmib_get(MIB_DHCPRSVDIP_TBL, (void *)&entry);
		
#ifdef SUPPORT_DHCP_PORT_IP_BIND
		if(entry.portId>0)
		{	
			nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"30%%\" ><font size=\"2\">%d</td>\n"
      			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
       			"<td align=center width=\"10%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
			inet_ntoa(*((struct in_addr*)entry.ipAddr)), entry.portId, entry.hostName, i);	
		}
		else 
#endif
		{
#ifndef _PRMT_X_TELEFONICA_ES_DHCPOPTION_
			if (memcmp(entry.macAddr, "\x0\x0\x0\x0\x0\x0", 6))
#endif
			{				
				sprintf(macaddr," %02x-%02x-%02x-%02x-%02x-%02x", entry.macAddr[0], entry.macAddr[1], entry.macAddr[2], entry.macAddr[3], entry.macAddr[4], entry.macAddr[5]);
		
		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
				"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
      				"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
       			"<td align=center width=\"10%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				inet_ntoa(*((struct in_addr*)entry.ipAddr)), macaddr,entry.hostName, i);
			}		
		}
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
int dhcpClientList(request *wp, int argc, char **argv)
{
	FILE *fp;	
	int nBytesSent=0;
	int element=0, ret;
	char ipAddr[40], macAddr[40], liveTime[80], *buf=NULL, *ptr, tmpBuf[100];
	struct stat status;
	int pid;
	unsigned long fileSize=0;
	char hostname[64];
	// siganl DHCP server to update lease file
	snprintf(tmpBuf, 100, "%s/%s.pid", _DHCPD_PID_PATH, _DHCPD_PROG_NAME);
	pid = getPid(tmpBuf);
	snprintf(tmpBuf, 100, "kill -SIGUSR1 %d\n", pid);
	
	if ( pid > 0)
	{
		//kill(pid, SIGUSR1);
		system(tmpBuf);
	}
	usleep(1000);

	if ( stat(_PATH_DHCPS_LEASES, &status) < 0 )
		goto err;

	fileSize=status.st_size;
	buf = malloc(fileSize);
	if ( buf == NULL )
		goto err;
	fp = fopen(_PATH_DHCPS_LEASES, "r");
	if ( fp == NULL )
		goto err;

	#ifdef CONFIG_APP_ADAPTER_API_ISP
	int fd;
	fd = fileno(fp);
	rtk_set_lock(fd, F_WRLCK);
	#endif

	fread(buf, 1, fileSize, fp);

	#ifdef CONFIG_APP_ADAPTER_API_ISP
	rtk_set_lock(fd, F_UNLCK);
	#endif
	
	fclose(fp);

	ptr = buf;
	while (1) {
		ret = getOneDhcpClient(&ptr, &fileSize, ipAddr, macAddr, liveTime, hostname);

		if (ret < 0)
			break;
		if (ret == 0)
			continue;		
		nBytesSent += req_format_write(wp,
			("<tr class=\"tbl_body\"><td><font size=2>%s</td><td><font size=2>%s</td><td><font size=2>%s</td><td><font size=2>%s</td></tr>"),
			hostname,ipAddr, macAddr, liveTime);
		element++;
	}
err:
	if (element == 0) {
		nBytesSent += req_format_write(wp,
			("<tr class=\"tbl_body\"><td><font size=2><script>dw(dhcp_none)</script></td><td><font size=2><script>dw(dhcp_none)</script></td><td><font size=2>----</td><td><font size=2>----</td></tr>"));
	}
	if (buf)
		free(buf);

	return nBytesSent;
}

////////////////////////////////////////////////////////////////////////////
#if 1 
#define WIRELESS_CONN 0
#define WIRED_CONN 1

int wireless_dhcp_client_num=0;
int wired_dhcp_client_num=0;

int isWiredDhcpClient(char *client_mac)
{
	FILE *fp;
	char line_buffer[512];	
	char mac_str[13];
	unsigned char mac_addr[6];
	int idx=0, i, j;	
	char *pchar, *pstart;
	int conn_type=0;

	char maclist[64][18];	
	
	if((fp= fopen("/proc/rtl865x/l2", "r"))==NULL)
		return -1;

	while(fgets(line_buffer, sizeof(line_buffer), fp))
	{			
		line_buffer[strlen(line_buffer)-1]='\0';

		if(strstr(line_buffer, "ff:ff:ff:ff:ff:ff") || strstr(line_buffer, "CPU") || strstr(line_buffer, "FID:1") || strstr(line_buffer, "mbr(4") || strstr(line_buffer, "mbr(8"))
		//if(strstr(line_buffer, "ff:ff:ff:ff:ff:ff") || strstr(line_buffer, "CPU") || strstr(line_buffer, "FID:1"))
			continue;	
		
		pchar=strchr(line_buffer, ':');
		pstart=pchar-2;
		strncpy(maclist[idx], pstart, 17);
		maclist[idx][17]=0;
		idx++;	
	}
	fclose(fp);

	if(idx>0)
	{
		for(i=0;i<idx;i++)
		{
			if(strncasecmp(maclist[i], client_mac, 17)==0)
			{
				conn_type=1;
				break;
			}
		}
	}
	if(conn_type==1)
		return 1;
	else 
		return 0;	
}

int findClientAssoWlanIf(char *filename, char *client_mac)
{
	FILE *fp;
	char line_buffer[512];	
	int idx=0, i=0, j=0;	
	char *pchar=NULL;
	int conn_type=-1;

	char maclist[64][13];
	if((fp= fopen(filename, "r"))!=NULL)
	{	
		while(fgets(line_buffer, sizeof(line_buffer), fp) && (idx<64))
		{			
			line_buffer[strlen(line_buffer)-1]='\0';		
			if((pchar=strstr(line_buffer, "hwaddr: "))!=NULL)
			{
				pchar+=strlen("hwaddr: ");
				strncpy(maclist[idx], pchar, 12);
				maclist[idx][12]=0;
				idx++;		
			}
		}
		fclose(fp);

		if(idx>0)
		{			
			for(i=0;i<idx;i++)
			{
				if(strncasecmp(maclist[i], client_mac, 12)==0)
				{
					conn_type=WIRELESS_CONN;
					break;
				}
			}
		}
	}
	if(conn_type==WIRELESS_CONN)
		return 1;
	
	return 0;
}


int isWirelessDhcpClient(char *client_mac)
{
	int i=0, j=0;
	char tmp_client_mac[13]={0};

	for(i=0;i<17;i++)
		if(client_mac[i]!=':')
			tmp_client_mac[j++]=client_mac[i];


	if(findClientAssoWlanIf("/proc/wlan0/sta_info", tmp_client_mac))
		return 1;

	if(findClientAssoWlanIf("/proc/wlan1/sta_info", tmp_client_mac))
		return 1;

	return 0;
}

int checkClientConnType(char *client_mac, int connType)
{

	if(connType==WIRED_CONN)
		return isWiredDhcpClient(client_mac);
	else if(connType==WIRELESS_CONN)
		return isWirelessDhcpClient(client_mac);
}

int dhcpWiredClientList(request *wp, int argc, char **argv)
{
	FILE *fp;
	int nBytesSent=0;
	int element=0, ret;
	char ipAddr[40], macAddr[40], liveTime[80], *buf=NULL, *ptr, tmpBuf[100];
	struct stat status;
	int pid;
	unsigned long fileSize=0;
	char hostname[64]={0};
	// siganl DHCP server to update lease file
	snprintf(tmpBuf, 100, "%s/%s.pid", _DHCPD_PID_PATH, _DHCPD_PROG_NAME);
	pid = getPid(tmpBuf);
	snprintf(tmpBuf, 100, "kill -SIGUSR1 %d\n", pid);
	int device_type=5;
	
	if ( pid > 0)
	{
		//kill(pid, SIGUSR1);
		system(tmpBuf);
	}
	usleep(1000);

	if ( stat(_PATH_DHCPS_LEASES, &status) < 0 )
		goto err;

	fileSize=status.st_size;
	buf = malloc(fileSize);
	if ( buf == NULL )
		goto err;
	fp = fopen(_PATH_DHCPS_LEASES, "r");
	if ( fp == NULL )
		goto err;

	#ifdef CONFIG_APP_ADAPTER_API_ISP
	int fd;
	fd = fileno(fp);
	rtk_set_lock(fd, F_WRLCK);
	#endif

	fread(buf, 1, fileSize, fp);

	#ifdef CONFIG_APP_ADAPTER_API_ISP
	rtk_set_lock(fd, F_UNLCK);
	#endif
	
	fclose(fp);

	ptr = buf;
	while (1) {
		ret = getOneDhcpClient(&ptr, &fileSize, ipAddr, macAddr, liveTime, hostname);

		if (ret < 0)
			break;
		if (ret == 0)
			continue;
		if(checkClientConnType(macAddr, WIRED_CONN)!=1)
			continue;

		element++;

		nBytesSent += req_format_write(wp, ("clts.push(new it_nr(\"%d\", new it('macAddr', \"%s\"), new it('ipAddr', \"%s\")));"), 
			element-1, macAddr, ipAddr);
        
	}
err:	
	if (buf)
		free(buf);

	wired_dhcp_client_num=element;

	return nBytesSent;
}


int dhcpWirelessClientList(request *wp, int argc, char **argv)
{
	FILE *fp;
	int nBytesSent=0;
	int element=0, ret;
	char ipAddr[40], macAddr[40], liveTime[80], *buf=NULL, *ptr, tmpBuf[100];
	struct stat status;
	int pid;
	unsigned long fileSize=0;
	char hostname[64]={0};
	// siganl DHCP server to update lease file
	snprintf(tmpBuf, 100, "%s/%s.pid", _DHCPD_PID_PATH, _DHCPD_PROG_NAME);
	pid = getPid(tmpBuf);
	snprintf(tmpBuf, 100, "kill -SIGUSR1 %d\n", pid);
	int device_type=2;
	
	if ( pid > 0)
	{
		//kill(pid, SIGUSR1);
		system(tmpBuf);
	}
	usleep(1000);

	if ( stat(_PATH_DHCPS_LEASES, &status) < 0 )
		goto err;

	fileSize=status.st_size;
	buf = malloc(fileSize);
	if ( buf == NULL )
		goto err;
	fp = fopen(_PATH_DHCPS_LEASES, "r");
	if ( fp == NULL )
		goto err;

	#ifdef CONFIG_APP_ADAPTER_API_ISP
	int fd;
	fd = fileno(fp);
	rtk_set_lock(fd, F_WRLCK);
	#endif

	fread(buf, 1, fileSize, fp);
	
	#ifdef CONFIG_APP_ADAPTER_API_ISP
	rtk_set_lock(fd, F_UNLCK);
	#endif
	
	fclose(fp);

	ptr = buf;
	while (1) {
	
		ret = getOneDhcpClient(&ptr, &fileSize, ipAddr, macAddr, liveTime, hostname);

		if (ret < 0)
			break;
		if (ret == 0)
			continue;
		
		if(checkClientConnType(macAddr, WIRELESS_CONN)!=1)
			continue;

		if(strstr(hostname, "Mac"))
			device_type=1;
		else if(strstr(hostname, "iPhone"))
			device_type=3;
		else if(strstr(hostname, "android"))
			device_type=4;
		else if(strstr(hostname, "iPad"))
			device_type=6;
		else
			device_type=2;

		element++;
		
		if(argv[0]==NULL)
			nBytesSent += req_format_write(wp, ("wlanlist.push(new it_nr(\"%d\", new it('macAddr', \"%s\"), new it('ipAddr', \"%s\")));"), 
			element-1, macAddr, ipAddr);
        
	}
err:	
	if (buf)
		free(buf);

	wireless_dhcp_client_num=element;
	return nBytesSent;
}


#endif

/////////////////////////////////////////////////////////////////////////////
void formReflashClientTbl(request *wp, char *path, char *query)
{
	char *submitUrl;

	submitUrl = req_get_cstream_var(wp, "submit-url", "");
	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
}


//////////////////////////////////////////////////////////////////////////////
int isDhcpClientExist(char *name)
{
	char tmpBuf[100];
	struct in_addr intaddr;

	if ( getInAddr(name, IP_ADDR, (void *)&intaddr ) ) {
		snprintf(tmpBuf, 100, "%s/%s-%s.pid", _DHCPC_PID_PATH, _DHCPC_PROG_NAME, name);
		if ( getPid(tmpBuf) > 0)
			return 1;
	}
	return 0;
}

#ifdef MULTI_WAN_SUPPORT
///////////////////////////////////////////////////////////////////////////

int SetDefaultRouteIface( void )
{
	int total,i;
	int found =0;
	WANIFACE_T *pEntry,wan_entity;
	WANIFACE_T target[2];

	apmib_get(MIB_WANIFACE_TBL_NUM, (void *)&total);

	for( i=1; i<=total; i++ )
	{
		pEntry = &wan_entity;
		
		*((char *)pEntry) = (char)i;
		if(!apmib_get(MIB_WANIFACE_TBL, (void *)pEntry))
			continue;
		
		if(pEntry->enable == 0 
#ifdef CONFIG_IPV6
			&& pEntry->ipv6Enable== 0
#endif
			)
			continue;
			
		memcpy(&target[0], &wan_entity, sizeof(WANIFACE_T));
		pEntry->dgw=0;
		if((pEntry->cmode != IP_BRIDGE) && found==0){
			if((pEntry->ServiceList&X_CT_SRV_TR069) != 0 && (pEntry->ServiceList&X_CT_SRV_INTERNET) !=0 ){
				pEntry->dgw=1;
				found=1;
			}else if((pEntry->ServiceList&X_CT_SRV_INTERNET) !=0 ){
				pEntry->dgw=1;
				found=1;
			}
		}
		memcpy(&target[1], &wan_entity, sizeof(WANIFACE_T));
		apmib_set(MIB_WANIFACE_MOD, (void *)&target);
	}
	if(found == 0){
		for( i=1; i<=total; i++ )
		{
			pEntry = &wan_entity;
		
			*((char *)pEntry) = (char)i;
			if(!apmib_get(MIB_WANIFACE_TBL, (void *)pEntry))
				continue;
				
			if(pEntry->enable == 0 
#ifdef CONFIG_IPV6
				&& pEntry->ipv6Enable== 0
#endif
				)
				continue;
				
			memcpy(&target[0], &wan_entity, sizeof(WANIFACE_T));
		
			if(pEntry->cmode != IP_BRIDGE){
				pEntry->dgw=1;
				found=1;
				memcpy(&target[1], &wan_entity, sizeof(WANIFACE_T));
				apmib_set(MIB_WANIFACE_MOD, (void *)&target);
				break;
			}
		}
	}
		
}


void formMultiWanListTcpip(request *wp, char *path, char *query)
{

	char *buffer=NULL,*submitUrl=NULL,*submitUrl_wanIntface=NULL,*strVal=NULL;
	int wan_index =0,wan_num=0,i=0,intVal=0;
	char tmpBuf[128]={0};
	WANIFACE_T *p,wan_entity;
	WANIFACE_T target[2];
	int instanceNum;	
	#if defined(CONFIG_RTL_WEB_WAN_ACCESS_PORT)
	unsigned int WebPort = 0;
	#endif
#if 0
	strVal = req_get_cstream_var(wp, ("upnpEnabled"), "");
	if ( !strcmp(strVal, "ON"))
		intVal = 1;
	else
		intVal = 0;
	if ( !apmib_set(MIB_UPNP_ENABLED, (void *)&intVal)) {
		strcpy(tmpBuf, ("Set MIB_WANIFACE_UPNP_ENABLED error!"));
		goto setErr_tcpip;
	}
#endif
#ifndef CONFIG_ISP_IGMPPROXY_MULTIWAN)
	strVal = req_get_cstream_var(wp, ("igmpproxyEnabled"), "");
	if ( !strcmp(strVal, "ON"))
		intVal = 0;
	else
		intVal = 1;
	if ( !apmib_set(MIB_IGMP_PROXY_DISABLED, (void *)&intVal)) {
		strcpy(tmpBuf, ("Set MIB_WANIFACE_IGMP_PRXY_ENABLED error!"));
		goto setErr_tcpip;
	}
#endif
	strVal = req_get_cstream_var(wp, ("webWanAccess"), "");
	if ( !strcmp(strVal, "ON"))
		intVal = 1;
	else
		intVal = 0;
	if ( !apmib_set(MIB_WEB_WAN_ACCESS_ENABLED, (void *)&intVal)) {
		strcpy(tmpBuf, ("Set WEB_WAN_ACCESS_ENABLED error!"));
		goto setErr_tcpip;
	}
	
	#if defined(CONFIG_RTL_WEB_WAN_ACCESS_PORT)
	if (intVal){
		strVal = req_get_cstream_var(wp, ("webAccessPort"), "");
		if (!string_to_dec(strVal, &WebPort) || WebPort<1 || WebPort>65535) {
			strcpy(tmpBuf, ("Error! Invalid value of port."));
			goto setErr_tcpip;
		}
		if ( !apmib_set(MIB_WEB_WAN_ACCESS_PORT, (void *)&WebPort)) {
			strcpy(tmpBuf, ("Set WEB_WAN_ACCESS_ENABLED error!"));
			goto setErr_tcpip;
		}
	}
	#endif
	
	strVal = req_get_cstream_var(wp, ("pingWanAccess"), "");
	if ( !strcmp(strVal, "ON"))
		intVal = 1;
	else
		intVal = 0;
	if ( !apmib_set(MIB_PING_WAN_ACCESS_ENABLED, (void *)&intVal)) {
		strcpy(tmpBuf, ("Set PING_WAN_ACCESS_ENABLED error!"));
		goto setErr_tcpip;
	}		
		
	strVal = req_get_cstream_var(wp, ("WANPassThru1"), "");
	if ( !strcmp(strVal, "ON"))
		intVal = 1;
	else
		intVal = 0;	
	if ( !apmib_set(MIB_VPN_PASSTHRU_IPSEC_ENABLED, (void *)&intVal)) {
		strcpy(tmpBuf, ("Set MIB_VPN_PASSTHRU_IPSEC_ENABLED error!"));
		goto setErr_tcpip;
	}

	
	strVal = req_get_cstream_var(wp, ("WANPassThru2"), "");
	if ( !strcmp(strVal, "ON"))
		intVal = 1;
	else
		intVal = 0;	
	if ( !apmib_set(MIB_VPN_PASSTHRU_PPTP_ENABLED, (void *)&intVal)) {
		strcpy(tmpBuf, ("Set MIB_VPN_PASSTHRU_PPTP_ENABLED error!"));
		goto setErr_tcpip;
	}

	
	strVal = req_get_cstream_var(wp, ("WANPassThru3"), "");
	if ( !strcmp(strVal, "ON"))
		intVal = 1;
	else
		intVal = 0;	
	if ( !apmib_set(MIB_VPN_PASSTHRU_L2TP_ENABLED, (void *)&intVal)) {
		strcpy(tmpBuf, ("Set MIB_VPN_PASSTHRU_L2TP_ENABLED error!"));
		goto setErr_tcpip;
	}
	
	strVal = req_get_cstream_var(wp, ("ipv6_passthru_enabled"), "");
	if ( !strcmp(strVal, "ON"))
		intVal = 1;
	else
		intVal = 0;
    if ( !apmib_set(MIB_CUSTOM_PASSTHRU_ENABLED, (void *)&intVal))
    {
            strcpy(tmpBuf, ("Set custom passthru enabled error!"));
            goto setErr_tcpip;
    }

	
	
//currentWanIndex =-1 when apply change,and when edit,currentWanIndex=0,1,2,3
	buffer = req_get_cstream_var(wp, "currentWanIndex", "");
	wan_index = atoi(buffer)+1;
	

	if(wan_index)
	{
		if ( !apmib_set( MIB_WANIFACE_CURRENT_IDX, (void *)&wan_index) )
		{
			fprintf(stderr,"%s:%d get MIB_WANIFACE_CURRENT_IDX fail!wan_index=%d\n",__FUNCTION__,__LINE__,wan_index);
			return;
		}
		//setMutiWanMibValue(MIB_WANIFACE_CURRENT_IDX,(void*)&wan_index);
		//printf("%s:%d wan_index=%d\n",__FUNCTION__,__LINE__,wan_index);
		submitUrl_wanIntface = req_get_cstream_var(wp, ("submit-url-wanIntface"), "");
		//printf("%s:%d submitUrl_wanIntface=%s\n",__FUNCTION__,__LINE__,submitUrl_wanIntface);
		if (submitUrl_wanIntface[0])
				send_redirect_perm(wp, submitUrl_wanIntface);
	}else
	{//wan_index=0, appaly
		SetDefaultRouteIface();
		submitUrl = req_get_cstream_var(wp, ("submit-url"), "");
		if (submitUrl[0])
			OK_MSG(submitUrl);
	}


	return;
setErr_tcpip:
	ERR_MSG(tmpBuf);
	return; 

	
}

/*******************************
	save tcpipWan modify
*********************************/
int wanTcpipSaveStaticIp(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	char * strIp=NULL;
	struct in_addr inIp;
	char *strVal=NULL;
	strIp = req_get_cstream_var(wp, ("wan_ip"), "");
	if ( strIp[0] ) {
		if ( !inet_aton(strIp, &inIp) ) {
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			return -1;
		}
		memcpy(pEntry->ipAddr,&inIp,sizeof(unsigned char)*4);
	}

	strIp = req_get_cstream_var(wp, ("wan_mask"), "");
	if ( strIp[0] ) {
		if ( !inet_aton(strIp, &inIp) ) {
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_subnet)</script>"));
			return -1;
		}
		memcpy(pEntry->netMask,&inIp,sizeof(unsigned char)*4);
	}

	strIp = req_get_cstream_var(wp, ("wan_gateway"), "");
	if ( strIp[0] ) {
		if ( !inet_aton(strIp, &inIp) ) {
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_gw)</script>"));
			return -1;
		}
		memcpy(pEntry->remoteIpAddr,&inIp,sizeof(unsigned char)*4);
	}

	strVal = req_get_cstream_var(wp, ("fixedIpMtuSize"), "");
	if ( strVal[0] ) {
		int mtuSize;
		mtuSize = strtol(strVal, (char**)NULL, 10);

		pEntry->staticIpMtu=mtuSize;
	}		
	return 0;
}
int wanTcpipSaveDhcp(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	char *strVal=NULL;	

	strVal = req_get_cstream_var(wp, ("hostName"), "");
	if (strVal[0]) {
		if (!isValidName(strVal)) {
				strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_host)</script>"));
			return -1;				
		}			
		strcpy(pEntry->dhcpHostName,strVal);
	}else{
		 strcpy(pEntry->dhcpHostName,"");
	}	

	strVal = req_get_cstream_var(wp, ("dhcpMtuSize"), "");
	if ( strVal[0] ) {
		int mtuSize;
		mtuSize = strtol(strVal, (char**)NULL, 10);
		pEntry->dhcpMtu = mtuSize;
	}
	return 0;
}
int wanTcpipSavePppoe(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	char *strbuf=NULL;
	strbuf = req_get_cstream_var(wp, ("pppUserName"), "");
	if ( strbuf[0] ) {
		strcpy(pEntry->pppUsername,strbuf);
	}

	strbuf = req_get_cstream_var(wp, ("pppPassword"), "");
	if ( strbuf[0] ) {
		strcpy(pEntry->pppPassword,strbuf);
	}

	strbuf = req_get_cstream_var(wp, ("pppServiceName"), "");
	if ( strbuf[0] ) {
		strcpy(pEntry->pppServiceName,strbuf);
	}else
		strcpy(pEntry->pppServiceName,"");

	strbuf = req_get_cstream_var(wp, ("pppAcName"), "");
	if ( strbuf[0] ) {
		strcpy(pEntry->pppACName,strbuf);
	}else
		strcpy(pEntry->pppACName,"");

	strbuf = req_get_cstream_var(wp, ("pppConnectType"), "");
	if ( strbuf[0] ) {

		pEntry->pppCtype = strtol(strbuf, (char**)NULL, 10);
	}
	strbuf = req_get_cstream_var(wp, ("pppConnect"), "");
	if ( strbuf[0] ) {

		pEntry->pppCStatus = strtol(strbuf, (char**)NULL, 10);
	}
	strbuf = req_get_cstream_var(wp, ("pppIdleTime"), "");
	if ( strbuf[0] ) {

		pEntry->pppIdleTime = strtol(strbuf, (char**)NULL, 10);
	}
	strbuf = req_get_cstream_var(wp, ("pppMtuSize"), "");
	if ( strbuf[0] ) {

		pEntry->pppoeMtu = strtol(strbuf, (char**)NULL, 10);
	}
	return 0;
}
int wanTcpipSaveBridge(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	return 0;
}

#if  defined(SINGLE_WAN_SUPPORT)
int wanTcpipSaveL2TP(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	char *strbuf=NULL;
	struct in_addr inIp;
	
	strbuf = req_get_cstream_var(wp, "l2tpIpAddr", "");
	if (strbuf[0]){
		if (!inet_aton(strbuf,&inIp)){
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			return 0;
		}
		memcpy(pEntry->l2tpIpAddr,&inIp,sizeof(unsigned char)*4);
	}

	strbuf = req_get_cstream_var(wp, "l2tpSubnetMask", "");
	if (strbuf[0]){
		if (!inet_aton(strbuf,&inIp)){
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			return 0;
		}
		memcpy(pEntry->l2tpSubnetMask,&inIp,sizeof(unsigned char)*4);
	}	

	strbuf = req_get_cstream_var(wp, "l2tpDefGw", "");
	if (strbuf[0]){
		if (!inet_aton(strbuf,&inIp)){
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			return 0;
		}
		memcpy(pEntry->l2tpGateway,&inIp,sizeof(unsigned char)*4);
	}	
	strbuf = req_get_cstream_var(wp, "l2tpServerIpAddr", "");
	if (strbuf[0]){
		if (!inet_aton(strbuf,&inIp)){
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			return 0;
		}
		memcpy(pEntry->l2tpServerIpAddr,&inIp,sizeof(unsigned char)*4);
	}	
	
	strbuf = req_get_cstream_var(wp, ("l2tpUserName"), "");
	if ( strbuf[0] ) {
		strcpy(pEntry->l2tpUserName,strbuf);
	}

	strbuf = req_get_cstream_var(wp, ("l2tpPassword"), "");
	if ( strbuf[0] ) {
		strcpy(pEntry->l2tpPassword,strbuf);
	}

	strbuf = req_get_cstream_var(wp, ("l2tpConnectType"), "");
	if ( strbuf[0] ) {

		pEntry->l2tpConnectType = strtol(strbuf, (char**)NULL, 10);
	}

	strbuf = req_get_cstream_var(wp, ("l2tpIdleTime"), "");
	if ( strbuf[0] ) {

		pEntry->l2tpIdleTime = strtol(strbuf, (char**)NULL, 10);
	}
	strbuf = req_get_cstream_var(wp, ("l2tpMtuSize"), "");
	if ( strbuf[0] ) {

		pEntry->l2tpMtuSize = strtol(strbuf, (char**)NULL, 10);
	}
	printf("%s.%d. l2tp handle\n",__FUNCTION__,__LINE__);	
	return 0;
}

int wanTcpipSavePPTP(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	char *strbuf=NULL;
	struct in_addr inIp;
	int intVal = 0;
	
	strbuf = req_get_cstream_var(wp, "pptpIpAddr", "");
	if (strbuf[0]){
		if (!inet_aton(strbuf,&inIp)){
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			return 0;
		}
		memcpy(pEntry->pptpIpAddr,&inIp,sizeof(unsigned char)*4);
	}

	strbuf = req_get_cstream_var(wp, "pptpSubnetMask", "");
	if (strbuf[0]){
		if (!inet_aton(strbuf,&inIp)){
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			return 0;
		}
		memcpy(pEntry->pptpSubnetMask,&inIp,sizeof(unsigned char)*4);
	}	

	strbuf = req_get_cstream_var(wp, "pptpDefGw", "");
	if (strbuf[0]){
		if (!inet_aton(strbuf,&inIp)){
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			return 0;
		}
		memcpy(pEntry->pptpGateway,&inIp,sizeof(unsigned char)*4);
	}	
	strbuf = req_get_cstream_var(wp, "pptpServerIpAddr", "");
	if (strbuf[0]){
		if (!inet_aton(strbuf,&inIp)){
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			return 0;
		}
		memcpy(pEntry->pptpServerIpAddr,&inIp,sizeof(unsigned char)*4);
	}	
	
	strbuf = req_get_cstream_var(wp, ("pptpUserName"), "");
	if ( strbuf[0] ) {
		strcpy(pEntry->pptpUserName,strbuf);
	}

	strbuf = req_get_cstream_var(wp, ("pptpPassword"), "");
	if ( strbuf[0] ) {
		strcpy(pEntry->pptpPassword,strbuf);
	}

	strbuf = req_get_cstream_var(wp, ("pptpConnectType"), "");
	if ( strbuf[0] ) {

		pEntry->pptpConnectType = strtol(strbuf, (char**)NULL, 10);
	}

	strbuf = req_get_cstream_var(wp, ("pptpIdleTime"), "");
	if ( strbuf[0] ) {

		pEntry->pptpIdleTime = strtol(strbuf, (char**)NULL, 10);
	}
	strbuf = req_get_cstream_var(wp, ("pptpMtuSize"), "");
	if ( strbuf[0] ) {

		pEntry->pptpMtuSize = strtol(strbuf, (char**)NULL, 10);
	}
	strbuf = req_get_cstream_var(wp, ("pptpSecurity"), "");
	{
		if (!strcmp(strbuf, "ON"))
			intVal = 1;
		else
			intVal = 0;		
		pEntry->pptpSecurityEnabled = intVal;
	}
	
	strbuf = req_get_cstream_var(wp, ("pptpCompress"), "");
	{	
		if ( !strcmp(strbuf, "ON"))
			intVal = 1;
		else
			intVal = 0;
		pEntry->pptpMppcEnabled = intVal;
	}
	return 0;
}

#endif

int wanTcpipSaveUPNP(request*wp, int current_wanIdx, WANIFACE_Tp wan_entry,char* erroMsgBuf)
{
	WANIFACE_T *pWan_entity,wan_entity;
	int i,total,val,found = 0;
	char * pBuf=NULL;
	apmib_get(MIB_WANIFACE_TBL_NUM, (void *)&total);
	if(!wan_entry) return -1;
	
	pBuf = req_get_cstream_var(wp, ("upnpEnabled"), "");
	if ( !strcmp(pBuf, "ON"))
		val = 1;
	else
		val = 0;
	for( i=1; i<=total; i++ )
	{
		pWan_entity = &wan_entity;
		
		*((char *)pWan_entity) = (char)i;
		if(!apmib_get(MIB_WANIFACE_TBL, (void *)pWan_entity))
			continue;
		if(pWan_entity->enable && pWan_entity->enableUPNP){
			found = 1;
			break;
		}
	}

//	printf("[%s:%d]current_wanIdx = %d, i = %d\n",__FUNCTION__,__LINE__,current_wanIdx,i);
	if(val==1 && found &&  wan_entry->enable && current_wanIdx != i  ){
		strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_upnp)</script>"));
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_ENABLEUPNP,(void*)&val))
	{
		strcpy(erroMsgBuf,"set wan MIB_WANIFACE_ENABLEUPNP value fail!");
		return -1;
	}	
	wan_entry->enableUPNP = val;
	
	return 0;
}

int getAndCheckDigitVal(request*wp,char* varName,int * pVal,char* erroMsgBuf,char notNull)
{
	char * pEndPos=NULL;
	char * pbuf = NULL;
	int val =0;

	pbuf = req_get_cstream_var(wp, varName, "");
	if(notNull && !pbuf[0])
	{
		sprintf(erroMsgBuf,"%s can't be null!",varName);
		return -1;
	}
	if ( pbuf[0] ) 
	{
		val = strtol(pbuf,&pEndPos, 10);
		if(pEndPos && *pEndPos)
		{
			sprintf(erroMsgBuf,"%s have invalid character!",varName);
			return -1;
		}
		//printf("%s:%d pbuf=%s val=%d *pVal=%d\n",__FUNCTION__,__LINE__,pbuf,val,*pVal);
		*pVal = val;
		//printf("%s:%d pbuf=%s val=%d *pVal=%d\n",__FUNCTION__,__LINE__,pbuf,val,*pVal);
	}
	return 0;
}

int releaseBindingPorts(WanIntfacesType wan_idx,char*erroMsgBuf)
{
	int j=0;			
	WanIntfacesType wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM]={0};
	

	if(!apmib_get(MIB_WANIFACE_BINDING_LAN_PORTS,(void*)wanBindingLanPorts))
	{
		strcpy(erroMsgBuf,"get MIB_WANIFACE_BINDING_LAN_PORTS fail!");
		//fprintf(stderr,"%s:%d get MIB_WANIFACE_BINDING_LAN_PORTS fail!\n",__FUNCTION__,__LINE__);
		return -1;
	}
	
	for(j=0;j<WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM;j++)
	{
		if(wanBindingLanPorts[j] == wan_idx)
			wanBindingLanPorts[j] = 0;
	}
	/*for(j=0;j<WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM;j++)
	{
		printf("%s:%d wanBindingLanPorts[%d]=%d\n",__FUNCTION__,__LINE__,j,wanBindingLanPorts[j]);
	}*/
	if(!apmib_set(MIB_WANIFACE_BINDING_LAN_PORTS,(void*)wanBindingLanPorts))
	{
		strcpy(erroMsgBuf,"set MIB_WANIFACE_BINDING_LAN_PORTS fail!");
		//fprintf(stderr,"%s:%d set MIB_WANIFACE_BINDING_LAN_PORTS fail!\n",__FUNCTION__,__LINE__);
		return -1;
	}
	/*apmib_get(MIB_WANIFACE_BINDING_LAN_PORTS,(void*)wanBindingLanPorts);
	for(j=0;j<WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM;j++)
	{
		printf("%s:%d wanBindingLanPorts[%d]=%d\n",__FUNCTION__,__LINE__,j,wanBindingLanPorts[j]);
	}*/
	return 0;
}
//avoid conflict
int wanTcpipGetNoConflictVid(int vlanIdArry[])
{
	int tmpVid=0,j=0;
	while(true)
	{
		tmpVid = random()%4094 + 1;
		for(j=0;j<WANIFACE_NUM;j++)
		{
			if(vlanIdArry[j]==tmpVid)//conflict,retry
				break;
		}
		if(j==WANIFACE_NUM)//no conflict
			break;
	}
	return tmpVid;
}

void wanTcpipSetIfindex(WANIFACE_Tp pEntry,WanIntfacesType wan_idx)
{
	if(pEntry->ifIndex<0x1ff05 || pEntry->ifIndex>(0x1ff04 + WANIFACE_NUM))
	{
		pEntry->ifIndex = 0x1ff04 +wan_idx;
	}
	
	
	if(pEntry->ConDevInstNum == 0){
		pEntry->ConDevInstNum = wan_idx;
	}
}
int wanTcpipSaveVlan(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	char * pBuf = NULL;
	int val =0,i=0,j=0;
	int currentWanIdx_bak=0;
	int enable=0,vlanEnable=0,vlanid=0,tmpVid=0,vlanPri=0,addrType=0;
	int vlanEnabledArry[WANIFACE_NUM]={0};
	int vlanIdArry[WANIFACE_NUM]={0};
	int wanEnableArry[WANIFACE_NUM]={0};
	int addrTypeArry[WANIFACE_NUM]={0};

	pBuf = req_get_cstream_var(wp,"checkbox_vlanEnable","");
	if(strstr(pBuf,"on"))
	{		
		val = getAndCheckDigitVal(wp,"INPUT_VlanId",&vlanid,erroMsgBuf,1);
		if(val<0)
			return val;

		if(vlanid == 1 || (vlanid >= 7 && vlanid <= 12)) 
		{
			sprintf(erroMsgBuf, "<script>dw(tcpip_vid)</script> %d <script>dw(tcpip_vid_reserve)</script>", vlanid);
			return -1;
		}
		
		pEntry->vlanid=vlanid;
		//printf("%s:%d pEntry->vlanid=%d\n",__FUNCTION__,__LINE__,pEntry->vlanid);
		val = getAndCheckDigitVal(wp,"INPUT_vlan_priority",&vlanPri,erroMsgBuf,1);
		if(val<0)
			return val;
		
		if(vlanPri < 0 || vlanPri > 7) 
		{
			sprintf(erroMsgBuf, "<script>dw(tcpip_invalid_pri)</script>");
			return -1;
		}
		pEntry->vlanpriority=vlanPri;
		val=1;
		//setMutiWanMibValue(MIB_WANIFACE_VPRIORITY,(void*)&val);
	}else
	{//vlan disable
		val=0;
	}
	pEntry->vlan =val;	

	apmib_get(MIB_WANIFACE_CURRENT_IDX,(void*)&currentWanIdx_bak);
//get all vlan enable and vid
	for(i=1;i<=WANIFACE_NUM;i++)
	{	
		if(i==currentWanIdx_bak)
		{
			wanEnableArry[i-1]		=	pEntry->enable;
#ifdef CONFIG_IPV6
			if(pEntry->ipv6Enable)
				wanEnableArry[i-1]		=	pEntry->ipv6Enable;
#endif
			vlanEnabledArry[i-1]	=	pEntry->vlan;
			vlanIdArry[i-1]			=	pEntry->vlanid;
			addrTypeArry[i-1]       =	pEntry->AddressType;
			//printf("wanEnableArry[%d]=%d,vlanEnabledArry[%d]=%d,vlanIdArry[%d]=%d\n",
		//i-1,wanEnableArry[i-1],i-1,vlanEnabledArry[i-1],i-1,vlanIdArry[i-1]);
			continue;
		}
		apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i);//for interface i		
		apmib_get(MIB_WANIFACE_ENABLE,(void*)&enable);
		wanEnableArry[i-1]=enable;
#ifdef CONFIG_IPV6
		apmib_get(MIB_WANIFACE_IPV6_ENABLE,(void*)&enable);
		if(enable)
			wanEnableArry[i-1]=enable;			
#endif

		apmib_get(MIB_WANIFACE_VLAN,(void*)&vlanEnable);
		vlanEnabledArry[i-1]=vlanEnable;
		apmib_get(MIB_WANIFACE_VLANID,(void*)&vlanid);
		vlanIdArry[i-1] = vlanid;

		apmib_get(MIB_WANIFACE_ADDRESSTYPE,(void*)&addrType);
		addrTypeArry[i-1]       =	addrType;
		//printf("wanEnableArry[%d]=%d,vlanEnabledArry[%d]=%d,vlanIdArry[%d]=%d\n",
	//	i-1,wanEnableArry[i-1],i-1,vlanEnabledArry[i-1],i-1,vlanIdArry[i-1]);
	}	
	apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&currentWanIdx_bak);//revert currentIdx
	
//not init
	if(!pEntry->vlan&&(pEntry->vlanid<1 || pEntry->vlanid >4094))
	{
		pEntry->vlanid = wanTcpipGetNoConflictVid(vlanIdArry);
	}

	//check vid conflict
	for(i=1;i<=WANIFACE_NUM;i++)
	{
		if(i==currentWanIdx_bak)//current interface
			continue;		
		if(!wanEnableArry[i-1]) //disbaled interface
			continue;

		#if 0
		if(!pEntry->vlan && !vlanEnabledArry[i-1])
		{
			if((pEntry->AddressType == BRIDGE && addrTypeArry[i-1] == BRIDGE) ||
				(pEntry->AddressType != BRIDGE && addrTypeArry[i-1] != BRIDGE))
			{
				sprintf(erroMsgBuf, "Only 1 routing/NAT wan + 1 bridge wan can be VLAN untagged!");
				return -1;
			}
		}
		#endif

		if((vlanIdArry[i-1]==pEntry->vlanid))
		{//current vid conflict with wan[i]
			if(pEntry->vlan)
			{//current vlan enable
				if(vlanEnabledArry[i-1])
				{//wan[i] enable
					if(pEntry->AddressType == BRIDGE && addrTypeArry[i-1] == BRIDGE)
					{
						sprintf(erroMsgBuf, "vlanid conflict with wan %d !",i);
						return -1;
					}
					apmib_get(MIB_WANIFACE_CURRENT_IDX,(void*)&currentWanIdx_bak);
					apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i);
					apmib_set(MIB_WANIFACE_VLANID,(void*)&(vlanIdArry[i-1]));
					apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&currentWanIdx_bak);
				}else
				{//wan[i] vlan disbale,wan[i] vid need to change
					vlanIdArry[i-1]=wanTcpipGetNoConflictVid(vlanIdArry);
					//set wan[i] vlan new vid
					apmib_get(MIB_WANIFACE_CURRENT_IDX,(void*)&currentWanIdx_bak);
					apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i);
					apmib_set(MIB_WANIFACE_VLANID,(void*)&(vlanIdArry[i-1]));
					apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&currentWanIdx_bak);
				}
			}else
			{//current vlan disable,current vlan need to change
				pEntry->vlanid = wanTcpipGetNoConflictVid(vlanIdArry);
			}
		}
	
	}	
	val=pEntry->vlan;
	apmib_set(MIB_WANIFACE_VLAN,(void*)&(val));
	val=pEntry->vlanid;
	apmib_set(MIB_WANIFACE_VLANID,(void*)&(val));
	val=pEntry->vlanpriority;
	apmib_set(MIB_WANIFACE_VPRIORITY,(void*)&(val));

	return 0;
}

int wanTcpipSaveWanMac(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	char *pBuf=NULL;
	pBuf = req_get_cstream_var(wp, "wan_macAddr", "");
	if(pBuf[0])
	{
	
		unsigned char tmpBuf[64]={0};
		//printf("%s:%d pBuf=%s \n",__FUNCTION__,__LINE__,pBuf);
		
		if (strlen(pBuf)!=12 || !string_to_hex(pBuf, tmpBuf, 12)) {
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_clone_mac_error)</script>"));
			return -1;
		}
		//printf("%s:%d get tmpBuf=%x \n",__FUNCTION__,__LINE__,tmpBuf);
		memcpy(pEntry->wanMacAddr,tmpBuf,sizeof(pEntry->wanMacAddr));
		//printf("%s:%d get wanMacAddr=%x \n",__FUNCTION__,__LINE__,entry.wanMacAddr);
	}
	return 0;	
}

int wanTcpipSaveBindingPorts(request*wp,WanIntfacesType wan_idx,char* erroMsgBuf)
{
	int tmpVal=0;
	int val=0,i=0,j=0;
	char tmpBuf[64]={0};
	char* pBuf=NULL;
	WanIntfacesType wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM]={0};
	

	if(!apmib_get(MIB_WANIFACE_BINDING_LAN_PORTS,(void*)wanBindingLanPorts))
	{
		strcpy(erroMsgBuf,"get MIB_WANIFACE_BINDING_LAN_PORTS fail!");
//		fprintf(stderr,"%s:%d get MIB_WANIFACE_BINDING_LAN_PORTS fail!\n",__FUNCTION__,__LINE__);
		return -1;
	}
	/*for(i=0;i<WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM;i++)
	{
		printf("%s:%d wanBindingLanPorts[%d]=%d\n",__FUNCTION__,__LINE__,i,wanBindingLanPorts[i]);
	}*/
	
	for(i=0;i<WAN_INTERFACE_LAN_PORT_NUM;i++)
	{
		sprintf(tmpBuf,"INPUT_LanPort%d",i+1);
		pBuf = req_get_cstream_var(wp, tmpBuf, "");
		if(strstr(pBuf,"on"))
		{
			wanBindingLanPorts[i]=wan_idx;
		}else if(wanBindingLanPorts[i]==wan_idx)
		{
			wanBindingLanPorts[i]=0;
		}
		//printf("%s:%d pBuf=%s wanBindingLanPorts[%d]\n",__FUNCTION__,__LINE__,pBuf,i);
		
	}
	
	//return 0;
	for(i=WAN_INTERFACE_LAN_PORT_NUM;i<WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM;i++)
	{
		j=i-WAN_INTERFACE_LAN_PORT_NUM;
		if(j%WAN_INTERFACE_EACH_WLAN_PORT_NUM!=(WAN_INTERFACE_EACH_WLAN_PORT_NUM-1))
			snprintf(tmpBuf,sizeof(tmpBuf),"INPUT_Wlan%dSsid%d",(j/WAN_INTERFACE_EACH_WLAN_PORT_NUM)+1,j%WAN_INTERFACE_EACH_WLAN_PORT_NUM);
		else			
			snprintf(tmpBuf,sizeof(tmpBuf),"INPUT_Wlan%dVxd",(j/WAN_INTERFACE_EACH_WLAN_PORT_NUM)+1);
		pBuf = req_get_cstream_var(wp, tmpBuf, "");
		if(strstr(pBuf,"on"))
		{
			wanBindingLanPorts[i]=wan_idx;
		}else if(wanBindingLanPorts[i]==wan_idx)
		{
			wanBindingLanPorts[i]=0;
		}		
		
		//printf("%s:%d pBuf=%s wanBindingLanPorts[%d]=%d\n",__FUNCTION__,__LINE__,pBuf,i,wanBindingLanPorts[i]);
	}
	//setMutiWanMibValue(MIB_WANIFACE_BINDING_LAN_PORTS,(void*)wanBindingLanPorts);
	/*for(i=0;i<WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM;i++)
	{
		printf("%s:%d wanBindingLanPorts[%d]=%d\n",__FUNCTION__,__LINE__,i,wanBindingLanPorts[i]);
	}*/
	if(!apmib_set(MIB_WANIFACE_BINDING_LAN_PORTS,(void*)wanBindingLanPorts))
	{		
		strcpy(erroMsgBuf,"set MIB_WANIFACE_BINDING_LAN_PORTS fail!");
		//fprintf(stderr,"%s:%d set MIB_WANIFACE_BINDING_LAN_PORTS fail!\n",__FUNCTION__,__LINE__);
		return -1;
	}
	/*apmib_get(MIB_WANIFACE_BINDING_LAN_PORTS,(void*)wanBindingLanPorts);
	for(i=0;i<WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM;i++)
	{
		printf("%s:%d wanBindingLanPorts[%d]=%d\n",__FUNCTION__,__LINE__,i,wanBindingLanPorts[i]);
	}*/
	return 0;
}


int ppp_manual_handle(request*wp ,int wan_index)
{
	unsigned char command[128],tmpbuf[64];
	int buf_len;	
	char *strConnect=NULL;
	
	sprintf(tmpbuf,"wan_index=%d",wan_index);
	buf_len += strlen("wan_index=")+1;


	/*pppoe manual connect click **********/
	strConnect = req_get_cstream_var(wp, ("pppConnect"), "");
	if (strConnect && strConnect[0]) {
		sprintf(command,"reinitCli -e %d -l %d -d %s",REINIT_EVENT_PPP_MANUAL_CONNECT,buf_len,tmpbuf);		
		system(command);
//		printf("%s.%d.command(%s)\n",__FUNCTION__,__LINE__,command);
		return ;
	}
	
	/*pppoe manual disconnect click **********/	
    strConnect = req_get_cstream_var(wp, ("pppDisconnect"), "");
	if (strConnect && strConnect[0]) {
		sprintf(command,"reinitCli -e %d -l %d -d %s",REINIT_EVENT_WAN_DISCONNECT,buf_len,tmpbuf);		
		system(command);	
//		printf("%s.%d.command(%s)\n",__FUNCTION__,__LINE__,command);		
		return ;
	}	
	
}


int pptp_manual_handle(request*wp ,int wan_index)
{
	
	
	unsigned char command[128],tmpbuf[64];
	int buf_len;	
	char *strConnect=NULL;
	
	sprintf(tmpbuf,"wan_index=%d",wan_index);
	buf_len += strlen("wan_index=")+1;


	/*pppoe manual connect click **********/
	strConnect = req_get_cstream_var(wp, ("pptpConnect"), "");
	if (strConnect && strConnect[0]) {
		sprintf(command,"reinitCli -e %d -l %d -d %s",REINIT_EVENT_WAN_CONNECT,buf_len,tmpbuf);		
		system(command);
		printf("%s.%d.command(%s)\n",__FUNCTION__,__LINE__,command);
		return ;
	}
	
	/*pppoe manual disconnect click **********/	
    strConnect = req_get_cstream_var(wp, ("pptpDisconnect"), "");
	if (strConnect && strConnect[0]) {
		sprintf(command,"reinitCli -e %d -l %d -d %s",REINIT_EVENT_WAN_DISCONNECT,buf_len,tmpbuf);		
		system(command);	
		printf("%s.%d.command(%s)\n",__FUNCTION__,__LINE__,command);		
		return ;
	}	
	
}

int l2tp_manual_handle(request*wp ,int wan_index)
{
	unsigned char command[128],tmpbuf[64];
	int buf_len;	
	char *strConnect=NULL;
	
	sprintf(tmpbuf,"wan_index=%d",wan_index);
	buf_len += strlen("wan_index=")+1;


	/*pppoe manual connect click **********/
	strConnect = req_get_cstream_var(wp, ("l2tpConnect"), "");
	if (strConnect && strConnect[0]) {
		sprintf(command,"reinitCli -e %d -l %d -d %s",REINIT_EVENT_WAN_CONNECT,buf_len,tmpbuf);		
		system(command);
		printf("%s.%d.command(%s)\n",__FUNCTION__,__LINE__,command);
		return ;
	}
	
	/*pppoe manual disconnect click **********/	
    strConnect = req_get_cstream_var(wp, ("l2tpDisconnect"), "");
	if (strConnect && strConnect[0]) {
		sprintf(command,"reinitCli -e %d -l %d -d %s",REINIT_EVENT_WAN_DISCONNECT,buf_len,tmpbuf);		
		system(command);	
		printf("%s.%d.command(%s)\n",__FUNCTION__,__LINE__,command);		
		return ;
	}	
	
}

#ifdef CONFIG_IPV6
int getAndCheckIpv6Addr(request*wp,char* varNameBegin,char* varNamePrefix_len,addr6CfgParam_Tp pIpv6AddrVal,char* erroMsgBuf)
{
	char * pbuf=NULL;
	int i=0;
	char ipv6StrName[32]={0};
	char * pEndPos=NULL;
	char * pPrefix_len = NULL;

	for(i=0;i<8;i++)
	{
		sprintf(ipv6StrName,"%s%d",varNameBegin,i);
		pbuf = req_get_cstream_var(wp, ipv6StrName, "");	
		pIpv6AddrVal->addrIPv6[i] = strtol(pbuf,&pEndPos,16);
		if(pEndPos && *pEndPos)
		{//have invalid string 
			sprintf(erroMsgBuf,"%s have invalid character!",ipv6StrName);
			return -1;
		}		
//printf("%s:%d get pbuf[%d]=%s val=0x%x\n",__FUNCTION__,__LINE__,i,pbuf,pIpv6AddrVal->addrIPv6[i]);
		bzero(ipv6StrName,sizeof(ipv6StrName));
	}
	pPrefix_len = req_get_cstream_var(wp, varNamePrefix_len, "");
	pIpv6AddrVal->prefix_len = atoi(pPrefix_len);
//	printf("%s:%d pPrefix_len=%s val=%d\n",__FUNCTION__,__LINE__,pPrefix_len,pIpv6AddrVal->prefix_len);
	return 0;
}

int wanTcpipSaveDhcpV6c(request*wp)
{
	char *strVal=NULL;
	int enable =0;
	int ipv6DhcpReqAddrEnable=0,ipv6DhcpPdEnable=0,ipv6DhcpRapidCommitEnable=0;
	strVal = req_get_cstream_var(wp, ("dhcpv6Addr"), "");
	if(strstr(strVal,"on"))
	{
		ipv6DhcpReqAddrEnable = 1;
	}else
	{
		ipv6DhcpReqAddrEnable = 0;
	}
	if(!apmib_set(MIB_WANIFACE_IPV6_DHCP_REQ_ADDR_ENABLE,(void*)&ipv6DhcpReqAddrEnable))
	{
		fprintf(stderr,"Set MIB_WANIFACE_IPV6_DHCP_REQ_ADDR_ENABLE error!\n");
	}	
	strVal = req_get_cstream_var(wp, ("dhcpv6Pd"), "");
	if(strstr(strVal,"on"))
	{
		ipv6DhcpPdEnable = 1;
	}else
	{
		ipv6DhcpPdEnable = 0;
	}
	if(!apmib_set(MIB_WANIFACE_IPV6_DHCP_PD_ENABLE,(void*)&ipv6DhcpPdEnable))
	{
		fprintf(stderr,"Set MIB_WANIFACE_IPV6_DHCP_PD_ENABLE error!\n");
	}	
	strVal = req_get_cstream_var(wp, ("rapidEnable"), "");
	if(strstr(strVal,"on"))
	{
		ipv6DhcpRapidCommitEnable = 1;
	}else
	{
		ipv6DhcpRapidCommitEnable = 0;
	}
	if(!apmib_set(MIB_WANIFACE_IPV6_DHCP_RAPID_COMMIT_ENABLE,(void*)&ipv6DhcpRapidCommitEnable))
	{
		fprintf(stderr,"Set MIB_WANIFACE_IPV6_DHCP_RAPID_COMMIT_ENABLE error!\n");
	}
	return 0;
}

int wanTcpipSaveIpv6Static(request*wp,char* erroMsgBuf)
{
	int val=0;	
	addr6CfgParam_t ipv6Addr,ipv6Gw,ipv6dns1,ipv6dns2;	
	val = getAndCheckIpv6Addr(wp,"wan_ip_","prefix_len_ip",&ipv6Addr,erroMsgBuf);
	if(val==0)
	{
		if(!apmib_set(MIB_WANIFACE_IPV6_ADDR,(void*)&ipv6Addr))
		{
			fprintf(stderr,"Set MIB_WANIFACE_IPV6_ADDR error!\n");
		}
	}
	else
		return val;
		
	val = getAndCheckIpv6Addr(wp,"wan_gw_","prefix_len_gw",&ipv6Gw,erroMsgBuf);
	if(val==0)
	{
		if(!apmib_set(MIB_WANIFACE_IPV6_GW,(void*)&ipv6Gw))
		{
			fprintf(stderr,"Set MIB_WANIFACE_IPV6_GW error!\n");
		}
	}
	else
		return val;
	val = getAndCheckIpv6Addr(wp,"wan_dns1_","prefix_len_dns1",&ipv6dns1,erroMsgBuf);
	if(val==0)
	{
		if(!apmib_set(MIB_WANIFACE_IPV6_DNS1,(void*)&ipv6dns1))
		{
			fprintf(stderr,"Set MIB_WANIFACE_IPV6_DNS1 error!\n");
		}
	}
	else
		return val;
	
	val = getAndCheckIpv6Addr(wp,"wan_dns2_","prefix_len_dns2",&ipv6dns2,erroMsgBuf);
	if(val==0)
	{
		if(!apmib_set(MIB_WANIFACE_IPV6_DNS2,(void*)&ipv6dns2))
		{
			fprintf(stderr,"Set MIB_WANIFACE_IPV6_DNS2 error!\n");
		}
	}
	else
		return val;
	return 0; 	
}
#ifdef CONFIG_DSLITE_SUPPORT
int wanTcpipSaveDslite(request*wp, char* erroMsgBuf)
{
		int dslite;
		int val=0;
		addr6CfgParam_t ipv6Addr;
		char *strAFTR;
		char * strMode = NULL;
		char tmpBuf[64];
		strMode = req_get_cstream_var(wp, ("dsliteMode"), "");

		if ( strMode && strMode[0] ) 
		{
			if (!strcmp(strMode, ("dsliteAuto")))
				dslite = 1;
			else if (!strcmp(strMode, ("dsliteManual")))
				dslite = 0;
			else {
				strcpy(tmpBuf, ("Invalid ds-lite mode value!"));
				return -1;
			}

			if ( !apmib_set(MIB_WANIFACE_IPV6_DSLITE_MODE, (void *)&dslite)) {
	  			strcpy(tmpBuf, "Set DSLITE MODE MIB error!");
				return -1;
			}
			if(dslite == 0)
			{
				if(wanTcpipSaveIpv6Static(wp,erroMsgBuf)<0)
					return -1;
				val = getAndCheckIpv6Addr(wp,"wan_aftr_","prefix_len_aftr",&ipv6Addr,erroMsgBuf);
				if(val==0)
				{	
					if(!apmib_set(MIB_WANIFACE_IPV6_ADDR_AFTR_PARAM,(void*)&ipv6Addr))
					{
						fprintf(stderr,"Set MIB_WANIFACE_IPV6_ADDR error!\n");
					}
				}
				else
					return val;
			}
		}
		return 0;
}
#endif
#ifdef CONFIG_SIXRD_SUPPORT
int wanTcpipSaveIPv6rd(request*wp, char* erroMsgBuf)
{
		int val=0;
		addr6CfgParam_t ipv6rdprefix;
		char *strVal=NULL;
		struct in_addr Ip4addr={0};
		val = getAndCheckIpv6Addr(wp,"IPv6_6rd_ip_","prefix_len_6rd",&ipv6rdprefix,erroMsgBuf);
		if(val==0)
		{
			if(!apmib_set(MIB_WANIFACE_IPV6_6RD_PREFIX_PARAM,(void*)&ipv6rdprefix))
			{
				fprintf(stderr,"Set MIB_WANIFACE_IPV6_6RD_PREFIX_PARAM error!\n");
				return -1;
			}
		}
		else
			return -1;
		strVal = req_get_cstream_var(wp, ("wanMask"), "");
		if(strVal)
		{
			val=atoi(strVal);
			if(!apmib_set(MIB_WANIFACE_IPV4_6RD_MASK_LEN,(void*)&val))
			{
				fprintf(stderr,"Set MIB_WANIFACE_IPV4_6RD_MASK_LEN error!\n");
				return -1;
			}			
		}
		else
			return -1;
		strVal=req_get_cstream_var(wp,"IPv6_6rd_BR_IP","");
		if (strVal)
		{
			if ( !inet_aton(strVal, &Ip4addr) )
			{
				fprintf (stderr,"Set 6rd BR IP error!\n");
				return -1;
			}
			if ( !apmib_set(MIB_WANIFACE_IPV4_6RD_BR_ADDR, (void *)&Ip4addr))
			{
  				fprintf(stderr,"Set MIB_IPV4_6RD_BR_ADDR error!\n");
				return -1;
			}
		}
		else
			return -1;
		return 0;
}
#endif

int wanTcpipSaveIpv6Dns(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	char * pBuf = NULL;
	int val=0;
#if 0
	pBuf = req_get_cstream_var(wp, "dnsMode", "");
	if(strstr(pBuf,"dnsManual"))
		pEntry->ipv6_dnsAuto=0;
	else
		pEntry->ipv6_dnsAuto=1;
	//printf("%s:%d get pBuf=%s entry.dnsAuto=%d \n",__FUNCTION__,__LINE__,pBuf,entry.dnsAuto);
#endif
	val = getAndCheckIpv6Addr(wp,"wan_dns1_","prefix_len_dns1",&pEntry->ipv6dns1,erroMsgBuf);
	if(val<0)
		return val;
	val = getAndCheckIpv6Addr(wp,"wan_dns2_","prefix_len_dns2",&pEntry->ipv6dns2,erroMsgBuf);
	if(val<0)
		return val;
	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////

#if defined(MULTI_WAN_QOS)
	extern int configQueueStatus(int wanIndex, int queStats);
#if defined(CONFIG_IPV6)
	extern int configWanQosStatus(int wanIndex, int wanQosFlag, int wanV6QosFlag);
	extern int configClassStatus(int wanIndex, int clsStats, int v6QosFlag);
#else	
	extern int configWanQosStatus(int wanIndex, int wanQosFlag);
	extern int configClassStatus(int wanIndex, int clsStats);
#endif
#endif

//check if has set one wan interface service type to tr069
int checkSingleTr069If(WanIntfacesType wan_idx,char*erroMsgBuf){
	int i =0 ;
	WANIFACE_T entry_tmp;
	memset(&entry_tmp, '\0', sizeof(entry_tmp));	
	for(i = 1; i <= WANIFACE_NUM; i++){ 	
		*((char *)&entry_tmp) = (char)i;
		if(i != wan_idx){
			if(!apmib_get(MIB_WANIFACE_TBL, (void *)&entry_tmp))
			{
				strcpy(erroMsgBuf,"get wan interface item wrong!");
				return -1;
			}
			if(entry_tmp.enable && (entry_tmp.applicationtype == APPTYPE_TR069 || 
				entry_tmp.applicationtype == APPTYPE_TR069_INTERNET|| 
				entry_tmp.applicationtype == APPTYPE_TR069_BRIDGE)){
				strcpy(erroMsgBuf,"<script>dw(tcpip_wan_has_tr069)</script>");
				return -1;
			}
		}		
	}
	return 1;
}
#ifdef MAIN_WAN_MAPPING

int update_wan_hwnat_enabled(int wan_idx,int val)
{
	int i=0,wanidx_bak=0;
	int mappingWanIfIdx=0,hwnat_enabled=0,theWanMappingWanIfIdex=0;
	dump_mappingWanIdx_info();
	apmib_get(MIB_WANIFACE_CURRENT_IDX,(void*)&wanidx_bak);
	apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&wan_idx);
	if(apmib_get(MIB_WANIFACE_HW_NAT_ENABLED,(void*)&hwnat_enabled)==0) return -1;
	if(apmib_get(MIB_WANIFACE_MAPPING_WANIFIDX,(void*)&theWanMappingWanIfIdex)==0) return -1;
	if(theWanMappingWanIfIdex<1 || theWanMappingWanIfIdex>WANIFACE_NUM){		
		fprintf(stderr,"%s:%d wan%d mappingWanIfIdx=%d\n",__FUNCTION__,__LINE__,wan_idx,theWanMappingWanIfIdex);
		if(update_uninited_mappingWanIdx()<0)return -1;
		dump_mappingWanIdx_info();
		if(apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&wan_idx)==0) return -1;
		if(apmib_get(MIB_WANIFACE_MAPPING_WANIFIDX,(void*)&theWanMappingWanIfIdex)==0) return -1;
	}
	
	if(hwnat_enabled==val) {
		if(apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&wan_idx)==0) return -1;
		return 1;// not change
	}
	if(val==0){//disable hwnat on the wan, just set hw_nat disbale, wan interface mapping neednot change
		if(apmib_set(MIB_WANIFACE_HW_NAT_ENABLED,(void*)&val)==0) return -1;
		if(apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&wan_idx)==0) return -1;
		return 1;
	}
	//enable hwnat on the wan, should disable it on another wan(if exist), and make the wan mapping wanIfIdx to eth1 
	for(i=1;i<=WANIFACE_NUM;i++){
		if(i==wan_idx) continue;
		if(apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i)==0) return -1;
		if(apmib_get(MIB_WANIFACE_HW_NAT_ENABLED,(void*)&hwnat_enabled)==0) return -1;
		if(hwnat_enabled){//this wan hw_nat should be set disbale
			hwnat_enabled=0;
			if(apmib_set(MIB_WANIFACE_HW_NAT_ENABLED,(void*)&hwnat_enabled)==0) return -1;			
		}

		if(apmib_get(MIB_WANIFACE_MAPPING_WANIFIDX,(void*)&mappingWanIfIdx)==0) return -1;
		if(mappingWanIfIdx<1 || mappingWanIfIdx>WANIFACE_NUM){
			fprintf(stderr,"%s:%d wan%d mappingWanIfIdx=%d\n",__FUNCTION__,__LINE__,i,mappingWanIfIdx);
			if(update_uninited_mappingWanIdx()<0)return -1;
			i=0;
			continue;
		}
		if(mappingWanIfIdx==1){//swap this wan and the wan's mapping wanifidx
			mappingWanIfIdx=theWanMappingWanIfIdex;
			if(apmib_set(MIB_WANIFACE_MAPPING_WANIFIDX,(void*)&mappingWanIfIdx)==0) return -1;			
		}
	}
	//make the wan's mapping wanif to be eth1
	if(apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&wan_idx)==0) return -1;	
	theWanMappingWanIfIdex=1;			
	if(apmib_set(MIB_WANIFACE_MAPPING_WANIFIDX,(void*)&theWanMappingWanIfIdex)==0) return -1;
	if(apmib_set(MIB_WANIFACE_HW_NAT_ENABLED,(void*)&val)==0) return -1;
	updateWanIfName();
	dump_mappingWanIdx_info();
	if(wanidx_bak!=0)
		apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&wanidx_bak);
	return 1;
}
#endif
void formMultiWanTcpipSetup(request *wp, char *path, char *query)
{
	char erroMsgBuf[128]={0};
	int dns_changed=0;
	char *strIp=NULL;
	struct in_addr inIp;
	char *submitUrl;
	int val=0,i=0;
	int WANConDevInstNum=0;
	WanIntfacesType wan_idx=0 ;	
	char * pBuf=NULL, *temppBuf=NULL;
	WANIFACE_T entry;
	memset(&entry, '\0', sizeof(entry));	
	pBuf = req_get_cstream_var(wp, "ChangeNotSave", "");
	if(strstr(pBuf,"yes"))
	{//abort, save nothing
		submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
		if (submitUrl[0])
			send_redirect_perm(wp, submitUrl);
		return;
	}
	//apply,save changes


	if ( !apmib_get( MIB_WANIFACE_CURRENT_IDX, (void *)&wan_idx) )
	{
		strcpy(erroMsgBuf,"<script>dw(tcpip_mwan_err0)</script>");
		goto setErr_end;
	}
	if(wan_idx<1||wan_idx>WANIFACE_NUM)
	{
		strcpy(erroMsgBuf,"<script>dw(tcpip_mwan_err1)</script>");
		goto setErr_end;
	}
	
	//printf("%s:%d wan_index=%d\n",__FUNCTION__,__LINE__,wan_idx);
//get saved wanInterface entry to modify
	*((char *)&entry) = (char)wan_idx;
	if(!apmib_get(MIB_WANIFACE_TBL, (void *)&entry))
	{
		strcpy(erroMsgBuf,"get wan interface item wrong!");
		goto setErr_end;
	}
	//printf("%s:%d idx=%d \n",__FUNCTION__,__LINE__,wan_idx);

	pBuf = req_get_cstream_var(wp, "wan_enable", "");
	if(strstr(pBuf,"on"))
	{
		entry.connDisable=0;
		entry.enable=1;
	}else
	{//disabled
		if(!entry.enable) //disable->disable
			goto DO_MULTIWAN_END;
		//entry.connDisable=1;
		//entry.enable=0;

		val = 0;
		if(!apmib_set(MIB_WANIFACE_ENABLE,(void*)&val))
		{
			strcpy(erroMsgBuf,"set wan MIB_WANIFACE_ENABLE value fail!");
			goto setErr_end;
		}
	#if defined(CONFIG_IPV6)
		if(!apmib_set(MIB_WANIFACE_IPV6_ENABLE,(void *)&val))
		{
			fprintf(stderr,"Set MIB_WANIFACE_IPV6_ENABLE error!\n");
			goto setErr_end;
		}
	#endif
	#if defined(MULTI_WAN_QOS) && !defined(CONFIG_CMCC_QOS)
		configQueueStatus(wan_idx, 0);
	
		#if defined(CONFIG_IPV6)
		configWanQosStatus(wan_idx, 0, 0);
		configClassStatus(wan_idx, 0, 0);
		#else	
		configWanQosStatus(wan_idx, 0);
		configClassStatus(wan_idx, 0);
		#endif
	#endif

		//release binding ports
		if(releaseBindingPorts(wan_idx,erroMsgBuf)<0)
			goto setErr_end;
		goto DO_MULTIWAN_END;
	}

//set ifindex
	wanTcpipSetIfindex(&entry,wan_idx);
	/**************		save wan		**************************/
	pBuf = req_get_cstream_var(wp, "wanType", "");
//	printf("%s:%d set wanType=%s \n",__FUNCTION__,__LINE__,pBuf);
	val=atoi(pBuf);
	if(!apmib_set(MIB_WANIFACE_ADDRESSTYPE,(void*)&val))
	{
		strcpy(erroMsgBuf,"set wan interface addressType fail!");
		goto setErr_end;
	}
	entry.AddressType=val;

	//printf("%s:%d get wanType=%d \n",__FUNCTION__,__LINE__,val);


	pBuf = req_get_cstream_var(wp, "serviceType", "");	
	//printf("%s:%d get serviceType=%s \n",__FUNCTION__,__LINE__,pBuf);
	val=atoi(pBuf);
	entry.ServiceList=val;
	//setMutiWanMibValue(MIB_WANIFACE_SERVICE_TYPE,(void*)&val);
	if(entry.ServiceList == X_CT_SRV_INTERNET){
		entry.applicationtype = APPTYPE_INTERNET;
	}else if(entry.ServiceList ==X_CT_SRV_TR069){
		entry.applicationtype = APPTYPE_TR069;

		//check if has set one wan interface service type to tr069
		if(checkSingleTr069If(wan_idx,erroMsgBuf) < 0){
			goto setErr_end;
		}
	}else if(entry.ServiceList ==X_CT_SRV_OTHER){
		entry.applicationtype = APPTYPE_OTHER;
	}else if(entry.ServiceList ==X_CT_SRV_VOICE){
		entry.applicationtype = APPTYPE_VOICE;
	}else if(entry.ServiceList ==X_CT_SRV_IPTV){
		entry.applicationtype = APPTYPE_IPTV;
	}else if(entry.ServiceList ==X_CT_SRV_TR069_BRIDGE){
		entry.applicationtype = APPTYPE_TR069_BRIDGE;
	}else if(entry.ServiceList ==X_CT_SRV_TR069_INTERNET){
		entry.applicationtype = APPTYPE_TR069_INTERNET;

		//check if has set one wan interface service type to tr069
		if(checkSingleTr069If(wan_idx,erroMsgBuf) < 0){
			goto setErr_end;
		}
	}
	val=entry.applicationtype;
	if(!apmib_set(MIB_WANIFACE_APPLICATIONTYPE,(void*)&val))
	{
		strcpy(erroMsgBuf,"set wan interface addressType fail!");
		goto setErr_end;
	}

	switch(entry.AddressType)
	{
		case DHCP_DISABLED://static ip saves
			if(wanTcpipSaveStaticIp(wp,&entry,erroMsgBuf)<0)
				goto setErr_end;
			entry.cmode=IP_ROUTE;
			entry.brmode=BRIDGE_DISABLE;
			//printf("%s:%d inx=%d, cmode=%d\n",__FUNCTION__,__LINE__,wan_idx,entry.cmode);
		
			//entry.ConIPInstNum = findWANMaxIPConInstNum()+1;
			break;
		case DHCP_CLIENT://dhcp saves
			if(wanTcpipSaveDhcp(wp,&entry,erroMsgBuf)<0)
				goto setErr_end;
			entry.cmode=IP_ROUTE;
			entry.brmode=BRIDGE_DISABLE;
			//entry.ConIPInstNum = findWANMaxIPConInstNum()+1;
			//printf("%s:%d inx=%d, cmode=%d\n",__FUNCTION__,__LINE__,wan_idx,entry.cmode);
			break;
		case PPPOE:
			if(wanTcpipSavePppoe(wp,&entry,erroMsgBuf)<0)
				goto setErr_end;
			entry.cmode=IP_PPP;
			entry.brmode=BRIDGE_DISABLE;
			
			ppp_manual_handle(wp,wan_idx);
			//entry.ConPPPInstNum = findWANMaxPPPConInstNum()+1;
			//printf("%s:%d inx=%d, cmode=%d\n",__FUNCTION__,__LINE__,wan_idx,entry.cmode);
			break;
		case BRIDGE:
			if(wanTcpipSaveBridge(wp,&entry,erroMsgBuf)<0)
				goto setErr_end;
			entry.cmode=IP_BRIDGE;
			entry.brmode=BRIDGE_ETHERNET;
			//entry.ConIPInstNum = findWANMaxIPConInstNum()+1;
			///printf("%s:%d inx=%d, cmode=%d\n",__FUNCTION__,__LINE__,wan_idx,entry.cmode);
			break;
#if  defined(SINGLE_WAN_SUPPORT)
		case PPTP:
			if(wanTcpipSavePPTP(wp,&entry,erroMsgBuf)<0)
				goto setErr_end;
			entry.cmode=IP_PPP;
			entry.brmode=BRIDGE_DISABLE;	

			
			pptp_manual_handle(wp,wan_idx);
			break;
		case L2TP:
			if(wanTcpipSaveL2TP(wp,&entry,erroMsgBuf)<0)
				goto setErr_end;
			entry.cmode=IP_PPP;
			entry.brmode=BRIDGE_DISABLE;

			
			l2tp_manual_handle(wp,wan_idx);
			break;
#endif			
		default:
			strcpy(erroMsgBuf,"<script>dw(tcpip_mwan_err2)</script>");
			goto setErr_end;
	}
	
	/**************		DNS setting 	**************************/
	pBuf = req_get_cstream_var(wp, "dnsMode", "");
	if(strstr(pBuf,"dnsManual"))
		entry.dnsAuto=0;
	else
		entry.dnsAuto=1;
	//printf("%s:%d get pBuf=%s entry.dnsAuto=%d \n",__FUNCTION__,__LINE__,pBuf,entry.dnsAuto);

	strIp = req_get_cstream_var(wp, "dns1", "");
	if ( strIp[0] ) {
		if ( !inet_aton(strIp, &inIp) ) {
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			goto setErr_end;
		}
		memcpy(entry.wanIfDns1,&inIp,sizeof(unsigned char)*4);
	}
	strIp = req_get_cstream_var(wp, "dns2", "");
	if ( strIp[0] ) {
		if ( !inet_aton(strIp, &inIp) ) {
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			goto setErr_end;
		}
		memcpy(entry.wanIfDns2,&inIp,sizeof(unsigned char)*4);
	}
	//printf("%s:%d get entry.wanIfDns2=%s \n",__FUNCTION__,__LINE__,entry.wanIfDns2);
	strIp = req_get_cstream_var(wp, "dns3", "");
	if ( strIp[0] ) {
		if ( !inet_aton(strIp, &inIp) ) {
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			goto setErr_end;
		}
		memcpy(entry.wanIfDns3,&inIp,sizeof(unsigned char)*4);
	}

	/**************		vlan 	**************************/
	if(wanTcpipSaveVlan(wp,&entry,erroMsgBuf)<0)
		goto setErr_end;
	/**************		others 	**************************/
	if(wanTcpipSaveWanMac(wp,&entry,erroMsgBuf)<0)
		goto setErr_end;
		
	//added for igmpproxy
#ifdef CONFIG_ISP_IGMPPROXY_MULTIWAN
	pBuf = req_get_cstream_var(wp, ("igmpproxyEnabled"), "");
	if ( !strcmp(pBuf, "ON"))
		val = 1;
	else
		val = 0;
	entry.enableIGMP=val;
#if defined(CONFIG_ISP_ON_PHY)
	pBuf = req_get_cstream_var(wp, ("onPhysicalInterface"), "");
	if ( !strcmp(pBuf, "ON"))
		val = 1;
	else 
		val = 0;
	entry.onPhyIf = val;
#endif
#endif

#ifdef MAIN_WAN_MAPPING
	pBuf = req_get_cstream_var(wp, ("natEnabled"), "");
	if ( !strcmp(pBuf, "ON"))
		entry.nat_disable=0;
	else
		entry.nat_disable=1;
	

	pBuf = req_get_cstream_var(wp, ("setMainWan"), "");
	if ( !strcmp(pBuf, "ON"))
		val = 1;
	else
		val = 0;
	if(update_wan_hwnat_enabled(wan_idx,val)<0){
		fprintf(stderr,"Set setMainWan fail!\n");
		goto setErr_end;
	}
	entry.hw_nat_enabled=val;
	if(val==1){
		entry.mappingWanIfIdx=1;
		entry.nat_disable=0;
	}	
#endif

	if(wanTcpipSaveUPNP(wp,wan_idx, &entry, erroMsgBuf) < 0)
		goto setErr_end;
	//setMutiWanMibValue(MIB_WANIFACE_VLAN,(void*)&val);
	if(setMutiWanIntfaceValue(&entry,wan_idx)<0)
	{
		strcpy(erroMsgBuf,"<script>dw(tcpip_wan_set_fail)</script>");
		goto setErr_end;
	}
#ifdef CONFIG_APP_TR069
		else//sync tr069 mibs
		{
			val=entry.enable;
			apmib_set(MIB_WANIFACE_ENABLE,(void*)&val);
		}
#endif
#ifdef CONFIG_IPV6
		/************** 	save ipv6 link	type	**************************/
		pBuf = req_get_cstream_var(wp, "linkType", "");
		val = atoi(pBuf);
		//printf("%s:%d set linkType=%d \n",__FUNCTION__,__LINE__,val);
		if(!apmib_set(MIB_WANIFACE_IPV6_LINK_TYPE,(void *)&val))
		{
			fprintf(stderr,"Set MIB_WANIFACE_IPV6_LINK_TYPE error!\n");
		}
		switch(val)
		{
			case IPV6_LINKTYPE_IP:
				break;
			case IPV6_LINKTYPE_PPP:
				break;
			default:
				fprintf(stderr,"wrong ipv6LinkType");
				goto setErr_end;
		}
				

		pBuf = req_get_cstream_var(wp, "ipv6wan_enable", "");
		val=-1;
		if(strstr(pBuf,"on"))
			val = 1;
		else
			val = 0;
		if(!apmib_set(MIB_WANIFACE_IPV6_ENABLE,(void *)&val))
		{
			fprintf(stderr,"Set MIB_WANIFACE_IPV6_ENABLE error!\n");
			goto setErr_end;
		}

	#if defined(MULTI_WAN_QOS) && !defined(CONFIG_CMCC_QOS)
		if(val == 0){
			configWanQosStatus(wan_idx, entry.wanQosEnable, 0);
			configClassStatus(wan_idx, entry.wanQosEnable, 0);
		}
	#endif

		if(val == 1)
		{
		
		/************** save ipv6 wan	type	**************************/
		pBuf = req_get_cstream_var(wp, "ipv6WanType", "");
		//printf("%s:%d set ipv6wanType=%s \n",__FUNCTION__,__LINE__,pBuf);
		val = atoi(pBuf);
		if(!apmib_set(MIB_WANIFACE_IPV6_ORIGIN,(void *)&val))
		{
			fprintf(stderr,"Set MIB_WANIFACE_IPV6_ORIGIN error!\n");
		}
		switch(val)
		{
			case IPV6_ORIGIN_AUTO:
				break;
			case IPV6_ORIGIN_STATIC:
				if(wanTcpipSaveIpv6Static(wp,erroMsgBuf)<0)
					goto setErr_end;
				break;
			case IPV6_ORIGIN_SLAAC:
				break;
			case IPV6_ORIGIN_DHCP6C:
				if(wanTcpipSaveDhcpV6c(wp)<0)
					goto setErr_end;
				break;
#ifdef CONFIG_DSLITE_SUPPORT
			case IPV6_ORIGIN_DSLITE:
				if(wanTcpipSaveDslite(wp,erroMsgBuf)<0)
					goto setErr_end;
				break;
#endif
			case IPV6_ORIGIN_BRIDGE:
				break;
#ifdef CONFIG_SIXRD_SUPPORT
			case IPV6_ORIGIN_6RD:
				if(wanTcpipSaveIPv6rd(wp,erroMsgBuf)<0)
					goto setErr_end;
				break;
#endif				
			default:
				fprintf(stderr,"wrong ipv6WanType");
				goto setErr_end;
		}
	
		
		
#if defined(CONFIG_ISP_MLDPROXY_MULTIWAN)
		pBuf = req_get_cstream_var(wp, ("ispMldProxyEnabled"), "");
		if ( !strcmp(pBuf, "ON"))
			val = 1;
		else
			val = 0;
		#ifndef CONFIG_CMCC
		//MIB_WANIFACE_CURRENT_IDX is known, so that MIB_WANIFACE_ENABLE_MLDPROXY can be set successfully 
		if(!apmib_set(MIB_WANIFACE_ENABLE_MLDPROXY,(void *)&val))
		{
			fprintf(stderr,"Set MIB_WANIFACE_ENABLE_MLDPROXY error!\n");
		}	
		#endif
		//printf("val = %d, [%s:%d]\n", entry.enableMLD, __FUNCTION__, __LINE__);
#endif
		}
#endif
	if(wanTcpipSaveBindingPorts(wp,wan_idx,erroMsgBuf)<0)
		goto setErr_end;
	//goto DO_MULTIWAN_END;
	apmib_update_web(CURRENT_SETTING);	// update to flash
		

DO_MULTIWAN_END:
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	//printf("%s:%d get submit=%s \n",__FUNCTION__,__LINE__,submitUrl);
	if (submitUrl[0])
		OK_MSG(submitUrl);
			//send_redirect_perm(wp, submitUrl);
	/*if((val = tcpipWanHandler(wp, tmpBuf, &dns_changed)) < 0 )
		goto setErr_end ;
	else if (val == 1) // return ok
		return ;

	apmib_update_web(CURRENT_SETTING);	// update to flash

	// run script
	if ( dns_changed )
		arg = "all";
	else
		arg = "wan";

#ifdef UNIVERSAL_REPEATER
	apmib_get(MIB_REPEATER_ENABLED1, (void *)&val);
	if (val) 
		arg = "all";	
#endif	

#ifndef NO_ACTION
	run_init_script(arg);                
#endif

	OK_MSG(submitUrl);*/

	return;
setErr_end:
	ERR_MSG(erroMsgBuf);
}

#if 0
def CONFIG_IPV6
void formIpv6MultiWanListTcpip(request *wp, char *path, char *query)
{
	char *buffer=NULL,*submitUrl=NULL,*submitUrl_wanIntface=NULL;
	int wan_index =0,val=0,wan_num=0,i=0,strVal=0,intVal=0;
	char tmpBuf[128]={0};
	int instanceNum;
	
	//currentWanIndex =-1 when apply change,and when edit,currentWanIndex=0,1,2,3
	buffer = req_get_cstream_var(wp, "currentWanIndex", "");
	wan_index = atoi(buffer)+1;
	

	if(wan_index)
	{
		if ( !apmib_set( MIB_WANIFACE_CURRENT_IDX, (void *)&wan_index) )
		{
			fprintf(stderr,"%s:%d get MIB_WANIFACE_CURRENT_IDX fail!wan_index=%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		//setMutiWanMibValue(MIB_WANIFACE_CURRENT_IDX,(void*)&wan_index);
		//printf("%s:%d wan_index=%d\n",__FUNCTION__,__LINE__,wan_index);
		submitUrl_wanIntface = req_get_cstream_var(wp, ("submit-url-wanIntface"), "");
		//printf("%s:%d submitUrl_wanIntface=%s\n",__FUNCTION__,__LINE__,submitUrl_wanIntface);
		if (submitUrl_wanIntface[0])
				send_redirect_perm(wp, submitUrl_wanIntface);
	}else
	{//wan_index=0, appaly
		SetDefaultRouteIface();
		submitUrl = req_get_cstream_var(wp, ("submit-url"), "");
		if (submitUrl[0])
		{
		
			OK_MSG(submitUrl);
		}
	}
}
void formIpv6MultiLanListTcpip(request *wp, char *path, char *query)
{
	char *buffer=NULL,*submitUrl=NULL,*submitUrl_wanIntface=NULL;
	int wan_index =0,val=0,wan_num=0,i=0,strVal=0,intVal=0;
	char tmpBuf[128]={0};
	int instanceNum;
	//currentWanIndex =-1 when apply change,and when edit,currentWanIndex=0,1,2,3
	buffer = req_get_cstream_var(wp, "currentLanIndex", "");
	wan_index = atoi(buffer)+1;
	if(wan_index)
	{
		if ( !apmib_set( MIB_WANIFACE_CURRENT_IDX, (void *)&wan_index) )
		{
			fprintf(stderr,"%s:%d get MIB_WANIFACE_CURRENT_IDX fail!wan_index=%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		//setMutiWanMibValue(MIB_WANIFACE_CURRENT_IDX,(void*)&wan_index);
		//printf("%s:%d wan_index=%d\n",__FUNCTION__,__LINE__,wan_index);
		submitUrl_wanIntface = req_get_cstream_var(wp, ("submit-url-lanIntface"), "");
		//printf("%s:%d submitUrl_wanIntface=%s\n",__FUNCTION__,__LINE__,submitUrl_wanIntface);
		if (submitUrl_wanIntface[0])
				send_redirect_perm(wp, submitUrl_wanIntface);
	}else
	{//wan_index=0, appaly
		submitUrl = req_get_cstream_var(wp, ("submit-url"), "");
		if (submitUrl[0])
		{
			OK_MSG(submitUrl);
		}
	}
}

int getAndCheckIpv6Addr(request*wp,char* varNameBegin,char* varNamePrefix_len,addr6CfgParam_Tp pIpv6AddrVal,char* erroMsgBuf)
{
	char * pbuf=NULL;
	int i=0;
	char ipv6StrName[32]={0};
	char * pEndPos=NULL;
	char * pPrefix_len = NULL;

	for(i=0;i<8;i++)
	{
		sprintf(ipv6StrName,"%s%d",varNameBegin,i);
		pbuf = req_get_cstream_var(wp, ipv6StrName, "");	
		pIpv6AddrVal->addrIPv6[i] = strtol(pbuf,&pEndPos,16);
		if(pEndPos && *pEndPos)
		{//have invalid string 
			sprintf(erroMsgBuf,"%s have invalid character!",ipv6StrName);
			return -1;
		}		
//printf("%s:%d get pbuf[%d]=%s val=0x%x\n",__FUNCTION__,__LINE__,i,pbuf,pIpv6AddrVal->addrIPv6[i]);
		bzero(ipv6StrName,sizeof(ipv6StrName));
	}
	pPrefix_len = req_get_cstream_var(wp, varNamePrefix_len, "");
	pIpv6AddrVal->prefix_len = atoi(pPrefix_len);
	//printf("%s:%d pPrefix_len=%s val=%d\n",__FUNCTION__,__LINE__,pPrefix_len,pIpv6AddrVal->prefix_len);
	return 0;
}

int wanTcpipSaveDhcpV6c(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	char *strVal=NULL;
	int enable =0;

	strVal = req_get_cstream_var(wp, ("ifname_dhcpv6c"), "");
	if (strVal[0]) 
	{
		if (!isValidName(strVal)) {
				strcpy(erroMsgBuf, ("Invalid Name! Please enter characters in A(a)~Z(z) or 0-9 without spacing."));
			return -1;				
		}			
		strcpy(pEntry->dhcp6cCfgParam.ifName,strVal);
	}else{
		 strcpy(pEntry->dhcp6cCfgParam.ifName,"");
	}	

	strVal = req_get_cstream_var(wp, ("enable_dhcpv6pd"), "");
	
	//printf("%s:%d dhcp6cCfgParam.enabled=%s\n",__FUNCTION__,__LINE__,strVal);
	if(strstr(strVal,"on"))
	{
		pEntry->dhcp6cCfgParam.enabled |= DHCP6C_PREFIX_ENABLE;
	}else
	{
		pEntry->dhcp6cCfgParam.enabled &= ~DHCP6C_PREFIX_ENABLE;
	}
	//printf("%s:%d dhcp6cCfgParam.enabled=0x%x\n",__FUNCTION__,__LINE__,pEntry->dhcp6cCfgParam.enabled);

	strVal = req_get_cstream_var(wp, ("interfacename_pd"), "");
	if(strVal[0])
	{
		if (!isValidName(strVal)) {
				strcpy(erroMsgBuf, ("Invalid Name! Please enter characters in A(a)~Z(z) or 0-9 without spacing."));
			return -1;				
		}			
		strcpy(pEntry->dhcp6cCfgParam.dhcp6pd.ifName,strVal);
	}

	strVal = req_get_cstream_var(wp, ("sla_len"), "");
	if(strVal[0])
	{
		pEntry->dhcp6cCfgParam.dhcp6pd.sla_len=atoi(strVal);
	}

	strVal = req_get_cstream_var(wp, ("sla_id"), "");
	if(strVal[0])
	{
		pEntry->dhcp6cCfgParam.dhcp6pd.sla_id=atoi(strVal);
	}
	return 0;
}

int wanTcpipSaveIpv6Static(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	int val=0;	
	
	val = getAndCheckIpv6Addr(wp,"wan_ip_","prefix_len_ip",&(pEntry->ipv6Addr),erroMsgBuf);
	if(val<0)
		return val;
		
	//printf("%s:%d ipv6val[0]=0x%x \n",__FUNCTION__,__LINE__,pEntry->ipv6Addr.addrIPv6[0]);
	val = getAndCheckIpv6Addr(wp,"wan_gw_","prefix_len_gw",&(pEntry->ipv6Gw),erroMsgBuf);
	if(val<0)
		return val;
		
	val = getAndCheckIpv6Addr(wp,"child_prefix_","prefix_len_childPrefixAddr",&(pEntry->ipv6ChildPrefix),erroMsgBuf);
	//val = getAndCheckIpv6Addr(wp,"prefix_","prefix_len_prefixAddr",&(pEntry->ipv6PrefixAddr),erroMsgBuf);
	if(val<0)
		return val;
	//val = getAndCheckIpv6Addr(wp,"child_prefix_mask_","prefix_len_childPrefixMask",&(pEntry->ipv6PrefixChildPrefix),erroMsgBuf);
	//if(val<0)
	//	return val;
	//printf("%s:%d ipv6val[0]=0x%x \n",__FUNCTION__,__LINE__,pEntry->ipv6Addr.addrIPv6[0]);
	return 0; 	
}
int wanTcpipSaveIpv6Dns(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	char * pBuf = NULL;
	int val=0;
	pBuf = req_get_cstream_var(wp, "dnsMode", "");
	if(strstr(pBuf,"dnsManual"))
		pEntry->ipv6_dnsAuto=0;
	else
		pEntry->ipv6_dnsAuto=1;
	//printf("%s:%d get pBuf=%s entry.dnsAuto=%d \n",__FUNCTION__,__LINE__,pBuf,entry.dnsAuto);
	val = getAndCheckIpv6Addr(wp,"wan_dns1_","prefix_len_dns1",&pEntry->ipv6dns1,erroMsgBuf);
	if(val<0)
		return val;
	val = getAndCheckIpv6Addr(wp,"wan_dns2_","prefix_len_dns2",&pEntry->ipv6dns2,erroMsgBuf);
	if(val<0)
		return val;
	return 0;
}



void formIpv6MultiWanTcpipSetup(request *wp, char *path, char *query)
{
	char erroMsgBuf[128]={0};
	int dns_changed=0;
	char *strIp=NULL;
	struct in_addr inIp;
	char *submitUrl;
	int val=0,i=0;
	int WANConDevInstNum=0;
	WanIntfacesType wan_idx=0 ;	
	char * pBuf=NULL;
	WANIFACE_T entry;
	
	memset(&entry, '\0', sizeof(entry));	
	pBuf = req_get_cstream_var(wp, "ChangeNotSave", "");
	if(strstr(pBuf,"yes"))
	{//abort, save nothing
		submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
		if (submitUrl[0])
			send_redirect_perm(wp, submitUrl);
		return;
	}
	//apply,save changes


	if ( !apmib_get( MIB_WANIFACE_CURRENT_IDX, (void *)&wan_idx) )
	{
		strcpy(erroMsgBuf,"get wan interface item index fail!");
		goto setErr_end;
	}
	if(wan_idx<1||wan_idx>WANIFACE_NUM)
	{
		strcpy(erroMsgBuf,"get wan interface item index wrong!");
		goto setErr_end;
	}
//get wanif entry
	*((char *)&entry) = (char)wan_idx;
	if(!apmib_get(MIB_WANIFACE_TBL, (void *)&entry))
	{
		strcpy(erroMsgBuf,"get wan interface item wrong!");
		goto setErr_end;
	}

	pBuf = req_get_cstream_var(wp, "wan_enable", "");
	if(strstr(pBuf,"on"))
	{
		val=1;		
	}else
	{
		val=0;
		apmib_set(MIB_WANIFACE_IPV6_ENABLE,(void*)&val);
		apmib_get(MIB_WANIFACE_ENABLE,(void*)&val);
		if(!val)//the interface disable for ipv4 and ipv6, release binding ports		
			if(releaseBindingPorts(wan_idx,erroMsgBuf)<0)
				goto setErr_end;
		goto DO_MULTIWAN_END;
	}

	entry.ipv6Enable = val;
	entry.connDisable=!val;

//set ifindex
	wanTcpipSetIfindex(&entry,wan_idx);

//save wan
	pBuf = req_get_cstream_var(wp, "OriginType", "");
	entry.ipv6Origin = atoi(pBuf);

	pBuf = req_get_cstream_var(wp, "linkType", "");
	entry.ipv6LinkType = atoi(pBuf);
	entry.brmode=BRIDGE_DISABLE;	
	pBuf = req_get_cstream_var(wp, "serviceType", "");
	//printf("%s:%d get serviceType=%s \n",__FUNCTION__,__LINE__,pBuf);
	entry.ServiceList=atoi(pBuf);
	if(entry.ServiceList == X_CT_SRV_INTERNET){
		entry.applicationtype = APPTYPE_INTERNET;
	}else if(entry.ServiceList ==X_CT_SRV_TR069){
		entry.applicationtype = APPTYPE_TR069;
	}else if(entry.ServiceList ==X_CT_SRV_OTHER){
		entry.applicationtype = APPTYPE_OTHER;
	}else if(entry.ServiceList ==X_CT_SRV_VOICE){
		entry.applicationtype = APPTYPE_VOICE;
	}
	switch(entry.ipv6Origin)
	{
	#if 0
		case IPV6_ORIGIN_AUTO:
			val = getAndCheckIpv6Addr(wp,"child_prefix_","prefix_len_childPrefixAddr",&(entry.ipv6ChildPrefix),erroMsgBuf);
			if(val<0)
				goto setErr_end;
			entry.cmode=IP_ROUTE;
			entry.AddressType = DHCP_CLIENT;
			break;
	#endif
		case IPV6_ORIGIN_AUTO:
			if(wanTcpipSaveDhcpV6c(wp,&entry,erroMsgBuf)<0)
				goto setErr_end;
			entry.cmode=IP_ROUTE;
			entry.AddressType = DHCP_CLIENT;
			break;
		case IPV6_ORIGIN_STATIC:
			if(wanTcpipSaveIpv6Static(wp,&entry,erroMsgBuf)<0)
				goto setErr_end;
			
			//printf("%s:%d ipv6val[0]=0x%x \n",__FUNCTION__,__LINE__,entry.ipv6Addr.addrIPv6[0]);
			entry.cmode=IP_ROUTE;
			entry.AddressType = DHCP_DISABLED;
			break;
		default:
			strcpy(erroMsgBuf,"get ipv6Origin wrong!");
			break;
	}
	if(entry.ipv6Origin==IPV6_ORIGIN_AUTO)
	{		
		entry.dhcp6cCfgParam.enabled |= DHCP6C_ENABLE;
	}else
	{
		entry.dhcp6cCfgParam.enabled &= ~DHCP6C_ENABLE;
	}
	switch(entry.ipv6LinkType)
	{
		case IPV6_LINKTYPE_IP:
			break;
		case IPV6_LINKTYPE_PPP:
			if(wanTcpipSavePppoe(wp,&entry,erroMsgBuf)<0)
				goto setErr_end;
			entry.cmode=IP_PPP;
			entry.AddressType = PPPOE;
			break;
		default:
			break;
	}
//set DNS
	if(wanTcpipSaveIpv6Dns(wp,&entry,erroMsgBuf)<0)
		goto setErr_end;

	/************** 	vlan	**************************/
	if(wanTcpipSaveVlan(wp,&entry,erroMsgBuf)<0)
		goto setErr_end;
	/************** 	others	**************************/
	if(wanTcpipSaveWanMac(wp,&entry,erroMsgBuf)<0)
		goto setErr_end;
	//added for mldproxy
	pBuf = req_get_cstream_var(wp, ("mldproxyEnabled"), "");
	if ( !strcmp(pBuf, "ON"))
		val = 0;
	else
		val = 1;
	entry.ipv6MldproxyDisabled=val;
	
	//printf("%s:%d ipv6val[0]=0x%x \n",__FUNCTION__,__LINE__,entry.ipv6Addr.addrIPv6[0]);

	if(setMutiWanIntfaceValue(&entry,wan_idx)<0)
	{
		strcpy(erroMsgBuf,"set wan interface item value fail!");
		goto setErr_end;
	}
#ifdef CONFIG_APP_TR069
	else//sync tr069 mibs
	{
		val=entry.ipv6Enable;
		apmib_set(MIB_WANIFACE_IPV6_ENABLE,(void*)&val);
	}
#endif
	if(wanTcpipSaveBindingPorts(wp,wan_idx,erroMsgBuf)<0)
		goto setErr_end;
	
DO_MULTIWAN_END:
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	 // hidden page
	//printf("%s:%d get submit=%s \n",__FUNCTION__,__LINE__,submitUrl);
	if (submitUrl[0])
			send_redirect_perm(wp, submitUrl);
	return;		
setErr_end:
		ERR_MSG(erroMsgBuf);

}

int lanTcpipSaveDhcpV6s(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	char *strVal=NULL;
	//pEntry->dhcp6sCfgParam.enabled = 1;
	strVal = req_get_cstream_var(wp, "dhcp_dnsaddr", "");
	if(strVal[0])
		strcpy(pEntry->dhcp6sCfgParam.DNSaddr6,strVal);

	strVal = req_get_cstream_var(wp, "dhcpv6s_interfacenameds", "");
	if(strVal[0])
		strcpy(pEntry->dhcp6sCfgParam.interfaceNameds,strVal);

	strVal = req_get_cstream_var(wp, "dhcp_addrPoolStart", "");
	if(strVal[0])
		strcpy(pEntry->dhcp6sCfgParam.addr6PoolS,strVal);

	strVal = req_get_cstream_var(wp, "dhcp_addrPoolEnd", "");
	if(strVal[0])
		strcpy(pEntry->dhcp6sCfgParam.addr6PoolE,strVal);

	return 0;
}
int lanTcpipSaveIpv6Radvd(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	char *strVal=NULL;
	int val=0;
	
	strVal = req_get_cstream_var(wp, "interfacename", "");
	if(strVal[0])
		strcpy(pEntry->radvdCfgParam.interface.Name,strVal);

	strVal = req_get_cstream_var(wp, "MaxRtrAdvInterval", "");
	if(strVal[0])
		pEntry->radvdCfgParam.interface.MaxRtrAdvInterval=atoi(strVal);

	strVal = req_get_cstream_var(wp, "MinRtrAdvInterval", "");
	if(strVal[0])
		pEntry->radvdCfgParam.interface.MinRtrAdvInterval=atoi(strVal);

	strVal = req_get_cstream_var(wp, "MinDelayBetweenRAs", "");
	if(strVal[0])
		pEntry->radvdCfgParam.interface.MinDelayBetweenRAs=atoi(strVal);

	strVal = req_get_cstream_var(wp, "AdvManagedFlag", "");
	if(strstr(strVal,"on"))
		pEntry->radvdCfgParam.interface.AdvManagedFlag=1;
	else
		pEntry->radvdCfgParam.interface.AdvManagedFlag=0;	

	strVal = req_get_cstream_var(wp, "AdvOtherConfigFlag", "");
	if(strstr(strVal,"on"))
		pEntry->radvdCfgParam.interface.AdvOtherConfigFlag=1;
	else
		pEntry->radvdCfgParam.interface.AdvOtherConfigFlag=0;

	strVal = req_get_cstream_var(wp, "AdvLinkMTU", "");
	if(strVal[0])
		pEntry->radvdCfgParam.interface.AdvLinkMTU=atoi(strVal);

	strVal = req_get_cstream_var(wp, "AdvReachableTime", "");
	if(strVal[0])
		pEntry->radvdCfgParam.interface.AdvReachableTime=atoi(strVal);

	strVal = req_get_cstream_var(wp, "AdvRetransTimer", "");
	if(strVal[0])
		pEntry->radvdCfgParam.interface.AdvRetransTimer=atoi(strVal);

	strVal = req_get_cstream_var(wp, "AdvCurHopLimit", "");
	if(strVal[0])
		pEntry->radvdCfgParam.interface.AdvCurHopLimit=atoi(strVal);

	strVal = req_get_cstream_var(wp, "AdvDefaultLifetime", "");
	if(strVal[0])
		pEntry->radvdCfgParam.interface.AdvDefaultLifetime=atoi(strVal);

	strVal = req_get_cstream_var(wp, "AdvDefaultPreference", "");
	if(strstr(strVal,"high"))
		strcpy(pEntry->radvdCfgParam.interface.AdvDefaultPreference,"high");
	else if(strstr(strVal,"medium"))
		strcpy(pEntry->radvdCfgParam.interface.AdvDefaultPreference,"medium");
	else if(strstr(strVal,"low"))
		strcpy(pEntry->radvdCfgParam.interface.AdvDefaultPreference,"low");
		
	strVal = req_get_cstream_var(wp, "AdvSourceLLAddress", "");
	if(strstr(strVal,"on"))
		pEntry->radvdCfgParam.interface.AdvSourceLLAddress=1;
	else
		pEntry->radvdCfgParam.interface.AdvSourceLLAddress=0;

	strVal = req_get_cstream_var(wp, "UnicastOnly", "");
	if(strstr(strVal,"on"))
		pEntry->radvdCfgParam.interface.UnicastOnly=1;
	else
		pEntry->radvdCfgParam.interface.UnicastOnly=0;


//prefix
	strVal = req_get_cstream_var(wp, "Enabled_0", "");
	if(strstr(strVal,"on"))
		pEntry->radvdCfgParam.interface.prefix[0].enabled=1;
	else
		pEntry->radvdCfgParam.interface.prefix[0].enabled=0;

	{
		int i=0;
		char ipv6StrName[32]={0};
		char * pEndPos=NULL;

		for(i=0;i<8;i++)
		{
			sprintf(ipv6StrName,"radvdprefix0_%d",i);
			strVal = req_get_cstream_var(wp, ipv6StrName, "");	
			pEntry->radvdCfgParam.interface.prefix[0].Prefix[i] = strtol(strVal,&pEndPos,16);
			if(pEndPos && *pEndPos)
			{//have invalid string 
				sprintf(erroMsgBuf,"%s have invalid character!",ipv6StrName);
				return -1;
			}		
			bzero(ipv6StrName,sizeof(ipv6StrName));
		}
		strVal = req_get_cstream_var(wp, "radvdprefix0_len", "");
		if(strVal[0])
			pEntry->radvdCfgParam.interface.prefix[0].PrefixLen = atoi(strVal);
	}	
		
	strVal = req_get_cstream_var(wp, "AdvOnLinkFlag_0", "");
	if(strstr(strVal,"on"))
		pEntry->radvdCfgParam.interface.prefix[0].AdvOnLinkFlag=1;
	else
		pEntry->radvdCfgParam.interface.prefix[0].AdvOnLinkFlag=0;

	strVal = req_get_cstream_var(wp, "AdvAutonomousFlag_0", "");
	if(strstr(strVal,"on"))
		pEntry->radvdCfgParam.interface.prefix[0].AdvAutonomousFlag=1;
	else
		pEntry->radvdCfgParam.interface.prefix[0].AdvAutonomousFlag=0;

	strVal = req_get_cstream_var(wp, "AdvValidLifetime_0", "");
	if(strVal[0])
		pEntry->radvdCfgParam.interface.prefix[0].AdvValidLifetime=atoi(strVal);

	strVal = req_get_cstream_var(wp, "AdvPreferredLifetime_0", "");
	if(strVal[0])
		pEntry->radvdCfgParam.interface.prefix[0].AdvPreferredLifetime=atoi(strVal);

	strVal = req_get_cstream_var(wp, "AdvRouterAddr_0", "");
	if(strstr(strVal,"on"))
		pEntry->radvdCfgParam.interface.prefix[0].AdvRouterAddr=1;
	else
		pEntry->radvdCfgParam.interface.prefix[0].AdvRouterAddr=0;
	strVal = req_get_cstream_var(wp, "if6to4_0", "");
	if(strVal[0])
		strcpy(pEntry->radvdCfgParam.interface.prefix[0].if6to4,strVal);
	return 0;
}

void formIpv6MultiLanTcpipSetup(request *wp, char *path, char *query)
{
	char erroMsgBuf[128]={0};	
	char *submitUrl;
	//int val=0,i=0;
	WanIntfacesType wan_idx=0 ;	
	char * pBuf=NULL;
	int val=0;
	WANIFACE_T entry;
	memset(&entry, '\0', sizeof(entry));	
	pBuf = req_get_cstream_var(wp, "ChangeNotSave", "");
	if(strstr(pBuf,"yes"))
	{//abort, save nothing
		submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
		if (submitUrl[0])
			send_redirect_perm(wp, submitUrl);
		return;
	}
	//apply,save changes
	if ( !apmib_get( MIB_WANIFACE_CURRENT_IDX, (void *)&wan_idx) )
	{
		strcpy(erroMsgBuf,"get wan interface item index fail!");
		goto setErr_end;
	}
	if(wan_idx<1||wan_idx>WANIFACE_NUM)
	{
		strcpy(erroMsgBuf,"get wan interface item index wrong!");
		goto setErr_end;
	}
//get wanif entry
	*((char *)&entry) = (char)wan_idx;
	if(!apmib_get(MIB_WANIFACE_TBL, (void *)&entry))
	{
		strcpy(erroMsgBuf,"get wan interface item wrong!");
		goto setErr_end;
	}

	pBuf = req_get_cstream_var(wp, "lanMode", "");
	if(strstr(pBuf,"lanManual"))
	{
		entry.ipv6LanManualConfig = 1;
	
	//save lan
		val = getAndCheckIpv6Addr(wp,"lan_ip_","prefix_len_lanIp",&(entry.ipv6AddrLan),erroMsgBuf);
		if(val<0)
				goto setErr_end;
	//save dhcpv6
		if(lanTcpipSaveDhcpV6s(wp,&entry,erroMsgBuf)<0)
					goto setErr_end;
	//save radvd
		if(lanTcpipSaveIpv6Radvd(wp,&entry,erroMsgBuf)<0)
					goto setErr_end;
	}
	else if(strstr(pBuf,"lanAuto"))
	{
		entry.ipv6LanManualConfig = 0;		
	}

	if(setMutiWanIntfaceValue(&entry,wan_idx)<0)
	{
		strcpy(erroMsgBuf,"set wan interface item value fail!");
		goto setErr_end;
	}
	
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	 // hidden page
	//printf("%s:%d get submit=%s \n",__FUNCTION__,__LINE__,submitUrl);
	if (submitUrl[0])
			send_redirect_perm(wp, submitUrl);
	return;		
setErr_end:
		ERR_MSG(erroMsgBuf);
}
#endif//CONFIG_IPV6


//#else //MULTI_WAN_SUPPORT



#if 0
void formWanTcpipSetup(request *wp, char *path, char *query)
{


	char tmpBuf[100];
	int dns_changed=0;
	char *arg;
	char *submitUrl;
	int val ;
		
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page

	if((val = tcpipWanHandler(wp, tmpBuf, &dns_changed)) < 0 )
		goto setErr_end ;
	else if (val == 1) // return ok
		return ;

	apmib_update_web(CURRENT_SETTING);	// update to flash

	// run script
	if ( dns_changed )
		arg = "all";
	else
		arg = "wan";

#ifdef UNIVERSAL_REPEATER
	apmib_get(MIB_REPEATER_ENABLED1, (void *)&val);
	if (val) 
		arg = "all";	
#endif	

#ifndef NO_ACTION
	run_init_script(arg);                
#endif

	OK_MSG(submitUrl);

	return;
setErr_end:
	ERR_MSG(tmpBuf);
}
#endif

#ifdef CONFIG_CMCC
//if find, return wan idx, else return 0; if fail, return -1;
int get_wanEntry_by_wanName(WANIFACE_Tp pEntry,char* wan_name)
{
	int index=0;
#if 1
	char * strIdx=NULL;
	if(!wan_name) return 0;
	if((strIdx=strsep(&wan_name,"_"))==NULL){
		return 0;
	}
	index=strtoul(strIdx,NULL,10);
#else
	for(index=1;index<=WANIFACE_NUM;index++){
		if(!getWanIfaceEntry(index,pEntry)){
			return -1;
		}
		if(strcmp(wan_name,pEntry->WanName)==0) break;
	}
#endif
	if(index>WANIFACE_NUM) return 0;
	if(!getWanIfaceEntry(index,pEntry)){
			return -1;
		}
	return index;
}
int cmccWanSetStaticIp(request*wp,WANIFACE_Tp pEntry,char* erroMsgBuf)
{
	char * strIp=NULL;
	struct in_addr inIp={0};
	char *strVal=NULL;
	strIp = req_get_cstream_var(wp, ("wanIpAddress"), "");
	if ( strIp[0] ) {
		if ( !inet_aton(strIp, &inIp) ) {
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			return -1;
		}
		memcpy(pEntry->ipAddr,&inIp,sizeof(unsigned char)*4);
	}

	strIp = req_get_cstream_var(wp, ("wanSubnetMask"), "");
	if ( strIp[0] ) {
		if ( !inet_aton(strIp, &inIp) ) {
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_subnet)</script>"));
			return -1;
		}
		memcpy(pEntry->netMask,&inIp,sizeof(unsigned char)*4);
	}

	strIp = req_get_cstream_var(wp, ("defaultGateway"), "");
	if ( strIp[0] ) {
		if ( !inet_aton(strIp, &inIp) ) {
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_gw)</script>"));
			return -1;
		}
		memcpy(pEntry->remoteIpAddr,&inIp,sizeof(unsigned char)*4);
	}

	strVal = req_get_cstream_var(wp, ("MTU"), "");
	if ( strVal[0] ) {
		int mtuSize;
		mtuSize = strtol(strVal, (char**)NULL, 10);

		pEntry->staticIpMtu=mtuSize;
	}		
	pEntry->dnsAuto=0;
	
	strIp = req_get_cstream_var(wp, "dnsPrimary", "");
	if ( strIp[0] ) {
		if ( !inet_aton(strIp, &inIp) ) {
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			return -1;
		}
		memcpy(pEntry->wanIfDns1,&inIp,sizeof(unsigned char)*4);
	}
	strIp = req_get_cstream_var(wp, "dnsSecondary", "");
	if ( strIp[0] ) {
		if ( !inet_aton(strIp, &inIp) ) {
			strcpy(erroMsgBuf, ("<script>dw(tcpip_wan_invalid_ip)</script>"));
			return -1;
		}
		memcpy(pEntry->wanIfDns2,&inIp,sizeof(unsigned char)*4);
	}
	return 0;
}

int cmcc_setbinding(request *wp,int wanIdx,char*erroMsgBuf,int len)
{
	char *strVal=NULL;
	int i=0;
	WanIntfacesType wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM]={0};
	apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&wanIdx);
	if(!apmib_get(MIB_WANIFACE_BINDING_LAN_PORTS, (void *)wanBindingLanPorts))
	{
		snprintf(erroMsgBuf,len,"%s:%d get MIB_WANIFACE_BINDING_LAN_PORTS fail!\n",__FUNCTION__,__LINE__);
		return -1;
	}
	for(i=0;i<WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM;i++){
		if(wanBindingLanPorts[i]==wanIdx)
			wanBindingLanPorts[i]=0;
	}
	strVal = req_get_cstream_var(wp, ("cb_bindlan1"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[0]=wanIdx;
	strVal = req_get_cstream_var(wp, ("cb_bindlan2"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[1]=wanIdx;
	strVal = req_get_cstream_var(wp, ("cb_bindlan3"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[2]=wanIdx;
	strVal = req_get_cstream_var(wp, ("cb_bindlan4"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[3]=wanIdx;
#ifdef	CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT
	strVal = req_get_cstream_var(wp, ("cb_bindlan5"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[4]=wanIdx;
#endif
	strVal = req_get_cstream_var(wp, ("cb_bindwireless1"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM]=wanIdx;
	strVal = req_get_cstream_var(wp, ("cb_bindwireless1_va1"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+1]=wanIdx;
	strVal = req_get_cstream_var(wp, ("cb_bindwireless1_va2"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+2]=wanIdx;
	strVal = req_get_cstream_var(wp, ("cb_bindwireless1_va3"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+3]=wanIdx;
	strVal = req_get_cstream_var(wp, ("cb_bindwireless1_va4"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+4]=wanIdx;
	strVal = req_get_cstream_var(wp, ("cb_bindwireless1_vxd"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+5]=wanIdx;
	strVal = req_get_cstream_var(wp, ("cb_bindwireless2"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_EACH_WLAN_PORT_NUM]=wanIdx;
	strVal = req_get_cstream_var(wp, ("cb_bindwireless2_va1"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_EACH_WLAN_PORT_NUM+1]=wanIdx;
	strVal = req_get_cstream_var(wp, ("cb_bindwireless2_va2"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_EACH_WLAN_PORT_NUM+2]=wanIdx;
	strVal = req_get_cstream_var(wp, ("cb_bindwireless2_va3"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_EACH_WLAN_PORT_NUM+3]=wanIdx;
	strVal = req_get_cstream_var(wp, ("cb_bindwireless2_va4"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_EACH_WLAN_PORT_NUM+4]=wanIdx;
	strVal = req_get_cstream_var(wp, ("cb_bindwireless2_vxd"), "");
	if(strcmp(strVal, "on") == 0 ) wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_EACH_WLAN_PORT_NUM+5]=wanIdx;

	if(!apmib_set(MIB_WANIFACE_BINDING_LAN_PORTS, (void *)wanBindingLanPorts))
	{
		snprintf(erroMsgBuf,len,"%s:%d set MIB_WANIFACE_BINDING_LAN_PORTS fail!\n",__FUNCTION__,__LINE__);
		return -1;
	}
	return 0;
}

int cmcc_setWanValues(int wanIdx,WANIFACE_Tp pWanEntry,char*erroMsgBuf,int len)
{
	apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&wanIdx);
	if(!apmib_set(MIB_WANIFACE_ENABLE,(void*)&(pWanEntry->enable))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_ENABLE value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_CONNDISABLE,(void*)&(pWanEntry->connDisable)))
	{
		snprintf(erroMsgBuf,len,"set wan MIB_WANIFACE_CONNDISABLE value fail!");
			return -1;
	}
	if(!apmib_set(MIB_WANIFACE_CMODE,(void*)&(pWanEntry->cmode))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_CMODE value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_ADDRESSTYPE,(void*)&(pWanEntry->AddressType))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_ADDRESSTYPE value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_BRMODE,(void*)&(pWanEntry->brmode))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_BRMODE value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_IPADDR,(void*)&(pWanEntry->ipAddr))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_IPADDR value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_NETMASK,(void*)&(pWanEntry->netMask))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_NETMASK value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_REMOTEIPADDR,(void*)&(pWanEntry->remoteIpAddr))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_REMOTEIPADDR value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_STATIC_IP_MTU,(void*)&(pWanEntry->staticIpMtu))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_STATIC_IP_MTU value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_DHCP_MTU,(void*)&(pWanEntry->dhcpMtu))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_DHCP_MTU value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_PPPUSERNAME,(void*)(pWanEntry->pppUsername))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_PPPUSERNAME value fail!");
		return -1;
	}
	//printf("%s:%d pppUsername=%s\n",__FUNCTION__,__LINE__,pWanEntry->pppUsername);
	if(!apmib_set(MIB_WANIFACE_PPPPASSWD,(void*)(pWanEntry->pppPassword))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_PPPPASSWD value fail!");
		return -1;
	}
	//printf("%s:%d pppPassword=%s\n",__FUNCTION__,__LINE__,pWanEntry->pppPassword);
	if(!apmib_set(MIB_WANIFACE_PPPOE_MTU,(void*)&(pWanEntry->pppoeMtu))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_PPPOE_MTU value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_ENABLEIGMP,(void*)&(pWanEntry->enableIGMP))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_ENABLEIGMP value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_VLAN,(void*)&(pWanEntry->vlan))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_VLAN value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_VLANID,(void*)&(pWanEntry->vlanid))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_VLANID value fail!");
		return -1;
	}
	
	if(!apmib_set(MIB_WANIFACE_VPRIORITY,(void*)&(pWanEntry->vlanpriority))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_VPRIORITY value fail!");
		return -1;
	}
	
	if(!apmib_set(MIB_WANIFACE_MULTICAST_VLAN,(void*)&(pWanEntry->multicastVlan))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_VLANID value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_DNS_AUTO,(void*)&(pWanEntry->dnsAuto))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_DNS_AUTO value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_DNS1,(void*)&(pWanEntry->wanIfDns1))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_DNS1 value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_DNS2,(void*)&(pWanEntry->wanIfDns2))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_DNS2 value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_NAT_DISABLE,(void*)&(pWanEntry->nat_disable))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_NAT_DISABLE value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_WANNAME,(void*)&(pWanEntry->WanName))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_WANNAME value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_APPLICATIONTYPE,(void*)&(pWanEntry->applicationtype))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_APPLICATIONTYPE value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_SERVICELIST,(void*)&(pWanEntry->ServiceList))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_APPLICATIONTYPE value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_IPPROTOCOL,(void*)&(pWanEntry->IpProtocol))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_IPPROTOCOL value fail!");
		return -1;
	}

#ifdef CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT
	if(!apmib_set(MIB_WANIFACE_PORT_ID,(void*)&(pWanEntry->portId))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_PORT_ID value fail!");
		return -1;
	}	
	if(!apmib_set(MIB_WANIFACE_WAN_ETH_NAME,(void*)&(pWanEntry->wan_eth_name))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_WAN_ETH_NAME value fail!");
		return -1;
	}
	if(pWanEntry->AddressType==PPPOE)
	if(!apmib_set(MIB_WANIFACE_WAN_PPP_NAME,(void*)&(pWanEntry->wan_ppp_name))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_WAN_PPP_NAME value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_HW_NAT_ENABLED,(void*)&(pWanEntry->hw_nat_enabled))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_HW_NAT_ENABLED value fail!");
		return -1;
	}	
#endif
	
#ifdef CONFIG_IPV6
	if(!apmib_set(MIB_WANIFACE_IPV6_ENABLE,(void*)&(pWanEntry->ipv6Enable))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_IPV6_ENABLE value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_IPV6_LINK_TYPE,(void*)&(pWanEntry->ipv6LinkType))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_IPV6_LINK_TYPE value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_IPV6_ORIGIN,(void*)&(pWanEntry->ipv6Origin))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_IPV6_ORIGIN value fail!");
		return -1;
	}
	#ifdef CONFIG_CMCC
	if(!apmib_set(MIB_WANIFACE_IPV6_MANUAL_CONFIG,(void*)&(pWanEntry->ipv6WanManualConfig))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_IPV6_MANUAL_CONFIG value fail!");
		return -1;
	}
	#endif
	if(!apmib_set(MIB_WANIFACE_IPV6_ADDR,(void*)&(pWanEntry->ipv6Addr))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_IPV6_ADDR value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_IPV6_GW,(void*)&(pWanEntry->ipv6Gw))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_IPV6_GW value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_IPV6_DNS1,(void*)&(pWanEntry->ipv6dns1))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_IPV6_DNS1 value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_IPV6_DNS2,(void*)&(pWanEntry->ipv6dns2))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_IPV6_DNS2 value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_IPV6_DHCP_PD_ENABLE,(void*)&(pWanEntry->ipv6DhcpPdEnable))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_IPV6_DHCP_PD_ENABLE value fail!");
		return -1;
	}
#ifdef CONFIG_DSLITE_SUPPORT
	if(!apmib_set(MIB_WANIFACE_IPV6_DSLITE_ENABLE,(void*)&(pWanEntry->dsliteEnable))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_APPLICATIONTYPE value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_IPV6_DSLITE_MODE,(void*)&(pWanEntry->dsliteMode))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_APPLICATIONTYPE value fail!");
		return -1;
	}
	if(!apmib_set(MIB_WANIFACE_IPV6_ADDR_AFTR_PARAM,(void*)&(pWanEntry->addr6AftrParam))){
		snprintf(erroMsgBuf,len,"set MIB_WANIFACE_APPLICATIONTYPE value fail!");
		return -1;
	}
#endif
#endif
	return 0;
}
#if defined (CONFIG_ISP_IGMPPROXY_MULTIWAN)
#if defined(CONFIG_CMCC)
int checkAppTypeForIgmp(int appType)
{
	int ret = 0;
	
	switch(appType)
	{
		case APPTYPE_TR069_INTERNET:
		case APPTYPE_INTERNET:
		case APPTYPE_VOICE_INTERNET:
		case APPTYPE_TR069_VOICE_INTERNET:
		case APPTYPE_IPTV:
			ret = 1;
			break;
		case APPTYPE_TR069:
		case APPTYPE_OTHER:
		case APPTYPE_VOICE:
		case APPTYPE_TR069_VOICE:
		case APPTYPE_TR069_BRIDGE:
			ret = 0;
			break;
		default:
			ret =0;
			break;
	}
	return ret;
			
}
#endif
#endif

void formEthernet_cmcc(request *wp, char *path, char *query)
{
	char * pBuf=NULL, *temppBuf=NULL;
	WANIFACE_T entry;

	char *submitUrl=NULL,*strVal=NULL,*strIp=NULL, *tmpStr=NULL,*wanName=NULL;
	char erroMsgBuf[256]={0},cmdBuf[64]={0};
	WANIFACE_T wanEntry={0};
	int index=0,enable=0,i=0,lan_dhcp=0,val=0,prefixLen=64,disable=0;
	struct in6_addr ip6Addr={0};
	int retVal;
	
		
	strVal = req_get_cstream_var(wp, ("OperatorStyle"), ""); 	
	wanName=req_get_cstream_var(wp,("wanName"),"");
	tmpStr=req_get_cstream_var(wp, "cb_enblService", "");
	
	index=get_wanEntry_by_wanName(&wanEntry,wanName);
	if(strcmp(strVal,"Del")==0 || strcmp(tmpStr,"on")!=0 ){
		if(index<=0){
			snprintf(erroMsgBuf,sizeof(erroMsgBuf),"Can't find %s in wan setting!\n",wanName);
				goto setformEthernet_cmcc_err_end;
		}
//del the wan means set the wan disable
		enable=0;
		disable=1;
		apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&index);
		if(!apmib_set(MIB_WANIFACE_ENABLE,(void*)&enable))
		{
			snprintf(erroMsgBuf,sizeof(erroMsgBuf),"set wan MIB_WANIFACE_ENABLE value fail!");
				goto setformEthernet_cmcc_err_end;
		}
		if(!apmib_set(MIB_WANIFACE_CONNDISABLE,(void*)&disable))
		{
			snprintf(erroMsgBuf,sizeof(erroMsgBuf),"set wan MIB_WANIFACE_CONNDISABLE value fail!");
				goto setformEthernet_cmcc_err_end;
		}
		
		if(releaseBindingPorts(index,erroMsgBuf)<0)
			goto setformEthernet_cmcc_err_end;
		
		goto setformEthernet_cmcc_ok;
	}
	else if(strcmp(strVal,"Add")==0){
		/*if(index!=0){
			snprintf(erroMsgBuf,sizeof(erroMsgBuf),"Can't add %s for wan with the same name already exist!\n",wanName);
				goto setformEthernet_cmcc_err_end;
		}*/
		for(i=1;i<=WANIFACE_NUM;i++){
			if(!getWanIfaceEntry(i,&wanEntry)){
				return -1;
			}
			if(!wanEntry.enable) {
				index=i;
				break;
			}
		}
		if(i>WANIFACE_NUM){
			snprintf(erroMsgBuf,sizeof(erroMsgBuf),"Can't add %s for max support %d wan!\n",wanName,WANIFACE_NUM);
				goto setformEthernet_cmcc_err_end;
		}
		bzero(&wanEntry,sizeof(wanEntry));
		wanEntry.enable=1;		
		wanEntry.connDisable=0;
		
	}
	else if(strcmp(strVal,"Modify")==0){
		if(index<=0){
			snprintf(erroMsgBuf,sizeof(erroMsgBuf),"Can't Modify %s in wan setting!\n",wanName);
				goto setformEthernet_cmcc_err_end;
		}
	}

	wanEntry.dnsAuto=1;
	tmpStr = req_get_cstream_var(wp, "wanMode", "");
	if(strcmp(tmpStr,"Bridge")==0){
		wanEntry.AddressType=BRIDGE;
		wanEntry.cmode=IP_BRIDGE;
		wanEntry.brmode=BRIDGE_ETHERNET;
	#ifdef CONFIG_CMCC
		wanEntry.enableIGMP=0;
	#endif

#ifdef CONFIG_IPV6
	#ifdef CONFIG_CMCC
		wanEntry.enableMLD=0;
		apmib_set(MIB_WANIFACE_ENABLE_MLDPROXY,(void *)&wanEntry.enableMLD);
		#endif
		tmpStr = req_get_cstream_var(wp, "bridgeMode", "");
		if (strcmp(tmpStr, "PPPoE_Bridged") == 0) //PPPoE_Bridged
				wanEntry.ipv6LinkType = IPV6_LINKTYPE_PPP;
			else //IP_Bridged
				wanEntry.ipv6LinkType= IPV6_LINKTYPE_IP;
#endif
	}else{
//route
		tmpStr = req_get_cstream_var(wp, "linkMode", "");
		if(strcmp(tmpStr,"linkPPP")==0){
			wanEntry.AddressType=PPPOE;
#ifdef CONFIG_IPV6
			/* sync ipv4 linktype with ipv6 linktype */
			wanEntry.ipv6LinkType=IPV6_LINKTYPE_PPP;
#endif
			wanEntry.cmode=IP_PPP;
			wanEntry.brmode=BRIDGE_DISABLE;
			tmpStr=req_get_cstream_var(wp, "pppUserName", "");
			strncpy(wanEntry.pppUsername,tmpStr,MAX_PPP_NAME_LEN+1);
			//printf("%s:%d pppUserName=%s\n",__FUNCTION__,__LINE__,tmpStr);
			tmpStr=req_get_cstream_var(wp, "pppPassword", "");
			strncpy(wanEntry.pppPassword,tmpStr,MAX_PPP_NAME_LEN+1);
			//printf("%s:%d pppPassword=%s\n",__FUNCTION__,__LINE__,tmpStr);
			strVal=req_get_cstream_var(wp, "MTU", "");
			if ( strVal[0] ) {
				int mtuSize;
				mtuSize = strtol(strVal, (char**)NULL, 10);
				wanEntry.pppoeMtu = mtuSize;
			}
		}else
		if(strcmp(tmpStr,"linkIP")==0){
			wanEntry.cmode=IP_ROUTE;
			wanEntry.brmode=BRIDGE_DISABLE;
#ifdef CONFIG_IPV6
			/* sync ipv4 linktype with ipv6 linktype */
			wanEntry.ipv6LinkType=IPV6_LINKTYPE_IP;
#endif			
			tmpStr=req_get_cstream_var(wp, "IpMode", "");
			if(strcmp(tmpStr,"DHCP")==0){
				wanEntry.AddressType=DHCP_CLIENT;
				wanEntry.dnsAuto=1;
				strVal=req_get_cstream_var(wp, "MTU", "");
				if ( strVal[0] ) {
					int mtuSize;
					mtuSize = strtol(strVal, (char**)NULL, 10);
					wanEntry.dhcpMtu = mtuSize;
				}
			}
			else if(strcmp(tmpStr,"Static")==0){
				wanEntry.AddressType=DHCP_DISABLED;
				if(cmccWanSetStaticIp(wp,&wanEntry,erroMsgBuf)<0){
					goto setformEthernet_cmcc_err_end;
				}
			}
				
		}
	}

	if(cmcc_setbinding(wp,index,erroMsgBuf,sizeof(erroMsgBuf))<0){
		goto setformEthernet_cmcc_err_end;
	}
	tmpStr = req_get_cstream_var(wp, "serviceList", "");
	if(strcmp(tmpStr,"TR069_INTERNET")==0){
		wanEntry.applicationtype = APPTYPE_TR069_INTERNET;
		wanEntry.ServiceList = X_CT_SRV_TR069_INTERNET;
		}
	else if(strcmp(tmpStr,"INTERNET")==0){
		wanEntry.applicationtype = APPTYPE_INTERNET;
		wanEntry.ServiceList = X_CT_SRV_INTERNET;
		}
	else if(strcmp(tmpStr,"TR069")==0){
		wanEntry.applicationtype = APPTYPE_TR069;
		wanEntry.ServiceList = X_CT_SRV_TR069;
		}
	else if(strcmp(tmpStr,"Other")==0 || strcmp(tmpStr,"OTHER")==0){
		wanEntry.applicationtype = APPTYPE_OTHER;
		wanEntry.ServiceList = X_CT_SRV_OTHER;
		}
	else if(strcmp(tmpStr,"VOICE")==0){
		wanEntry.applicationtype = APPTYPE_VOICE;
		wanEntry.ServiceList = X_CT_SRV_VOICE;
		}
	else if(strcmp(tmpStr,"TR069_VOICE")==0){
		wanEntry.applicationtype = APPTYPE_TR069_VOICE;
		wanEntry.ServiceList = X_CT_SRV_VOICE;
		}
	else if(strcmp(tmpStr,"VOICE_INTERNET")==0){
		wanEntry.applicationtype = APPTYPE_VOICE_INTERNET;
		wanEntry.ServiceList = X_CT_SRV_VOICE;
		}
	else if(strcmp(tmpStr,"TR069_VOICE_INTERNET")==0){
		wanEntry.applicationtype = APPTYPE_TR069_VOICE_INTERNET;
		wanEntry.ServiceList = X_CT_SRV_VOICE;
		}
		
	#ifdef CONFIG_CMCC
	if(checkAppTypeForIgmp(wanEntry.applicationtype))
		wanEntry.enableIGMP=1;
	else
		wanEntry.enableIGMP=0;
	#endif

	//don't show DHCP Server enable on wan webpage
#if 0
	tmpStr = req_get_cstream_var(wp, "cb_enabledhcp", "");
	if(strcmp(tmpStr,"on")==0){
		lan_dhcp=DHCP_SERVER;
	}else
		lan_dhcp=DHCP_DISABLED;
	if(!apmib_set(MIB_DHCP,(void*)&lan_dhcp)){
		snprintf(erroMsgBuf,sizeof(erroMsgBuf),"set MIB_DHCP value fail!");
		goto setformEthernet_cmcc_err_end;
	}
#endif

#ifdef CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT
	tmpStr = req_get_cstream_var(wp, "WanPortSelect", "");
	wanEntry.portId=atoi(tmpStr);

	tmpStr = req_get_cstream_var(wp, "hw_nat", "");
	if(atoi(tmpStr)==1)
		wanEntry.hw_nat_enabled=1;
	else
		wanEntry.hw_nat_enabled=0;	
#endif

#ifdef MAIN_WAN_MAPPING
			
		pBuf = req_get_cstream_var(wp, ("cb_mainwan"), "");
		if ( !strcmp(pBuf, "ON"))
			val = 1;
		else
			val = 0;
		if(update_wan_hwnat_enabled(wan_idx,val)<0){
			fprintf(stderr,"Set setMainWan fail!\n");
			goto setformEthernet_cmcc_err_end;
		}
/*
		entry.hw_nat_enabled=val;	
		if(val==1){
			entry.mappingWanIfIdx=1;
			entry.nat_disable=0;
		}	
*/
#endif
	//set mainwan ebnable 

	tmpStr = req_get_cstream_var(wp, "IpVersion", "");	
	if(strcmp(tmpStr, "IPv6")==0)
		wanEntry.IpProtocol=IP_PROTOCOL_MODE_IPV6;
	else if(strcmp(tmpStr, "IPv4/IPv6")==0)
		wanEntry.IpProtocol=IP_PROTOCOL_MODE_BOTH;
	else
		wanEntry.IpProtocol=IP_PROTOCOL_MODE_IPV4;

	//printf("\n%s:%d wanEntry.IpProtocol=%d\n",__FUNCTION__,__LINE__,wanEntry.IpProtocol);
	
#ifdef CONFIG_IPV6
	if(strstr(tmpStr,"IPv6")){
		wanEntry.ipv6Enable=1;
	}else
		wanEntry.ipv6Enable=0;
#endif
	tmpStr = req_get_cstream_var(wp, "VLANMode", "");
	if(strcmp(tmpStr,"UNTAG")==0){
		wanEntry.vlan=0;
		
#ifdef CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT
	int vlanid=0;
	val = getAndCheckDigitVal(wp,"vlan_untag",&vlanid,erroMsgBuf,1);
	if(val<0)
		goto setformEthernet_cmcc_err_end;

	if(vlanid == 1 || (vlanid >= 7 && vlanid <= 12)) 
	{
		sprintf(erroMsgBuf, "VLAN id %d is reserverd for internal use!", vlanid);
		goto setformEthernet_cmcc_err_end;
	}

	wanEntry.vlanid=vlanid;
#endif	
	}else
	if(strcmp(tmpStr,"TAG")==0){
		int vlanid=0, priority = 0;
		wanEntry.vlan=2; //2 wan tag, 1 vlan transparent

		val = getAndCheckDigitVal(wp,"vlan",&vlanid,erroMsgBuf,1);
		if(val<0)
			goto setformEthernet_cmcc_err_end;

		if(vlanid == 1 || (vlanid >= 7 && vlanid <= 12)) 
		{
			sprintf(erroMsgBuf, "VLAN id %d is reserverd for internal use!", vlanid);
			goto setformEthernet_cmcc_err_end;
		}
		
		wanEntry.vlanid=vlanid;

		//vlan priority
		val = getAndCheckDigitVal(wp,"v8021P",&priority,erroMsgBuf,1);
		if(val<0)
			goto setformEthernet_cmcc_err_end;

		if(priority < 0 || priority > 7) 
		{
			sprintf(erroMsgBuf, "8021p vlan priority invalid!");
			goto setformEthernet_cmcc_err_end;
		}
		wanEntry.vlanpriority = priority;
	}

#ifdef CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT	

	retVal=check_port_vid_is_used(wanEntry.portId, wanEntry.vlanid);
	if(retVal>=1 && retVal!=index)
	{
		sprintf(erroMsgBuf, "vlan id %d has been used by port %d!",wanEntry.vlanid, wanEntry.portId);
		goto setformEthernet_cmcc_err_end;
	}

	retVal=generate_wan_eth_ifname(wanEntry.portId, wanEntry.vlanid, wanEntry.wan_eth_name);
	if(retVal>=1 && retVal!=index)
	{
		sprintf(erroMsgBuf, "wan ethernet interface name has been used!");
		goto setformEthernet_cmcc_err_end;
	}

	if(wanEntry.AddressType==PPPOE)
		generate_wan_ppp_ifname(index, wanEntry.wan_ppp_name);
#endif

	int mcastVid = 0;
	val = getAndCheckDigitVal(wp,"MulticastVID",&mcastVid,erroMsgBuf,0);

	
	if(val<0)
		goto setformEthernet_cmcc_err_end;

	if(mcastVid == 1 || (mcastVid >= 7 && mcastVid <= 12)) 
	{
		sprintf(erroMsgBuf, "Multicast VLAN id %d is reserverd for internal use!", mcastVid);
		goto setformEthernet_cmcc_err_end;
	}
	
	wanEntry.multicastVlan = mcastVid;
	
	
	tmpStr = req_get_cstream_var(wp, "cb_nat", "");
	if(strcmp(tmpStr,"on")==0){
		wanEntry.nat_disable=0;
	}else
		wanEntry.nat_disable=1;
	#ifndef CONFIG_CMCC
	tmpStr = req_get_cstream_var(wp, "enblIgmp", "");
	if(strcmp(tmpStr,"Yes")==0){
		wanEntry.enableIGMP=0;
	}else
		wanEntry.enableIGMP=1;
	#endif
#ifdef CONFIG_IPV6
	tmpStr = req_get_cstream_var(wp, "IdIpv6AddrType", "");  
	//printf("[%s:%d] IdIpv6AddrType = %s\n",__FUNCTION__,__LINE__,tmpStr);
	if(strcmp(tmpStr,"SLAAC")==0){
		wanEntry.ipv6Origin=IPV6_ORIGIN_SLAAC;
		#ifdef CONFIG_CMCC
		wanEntry.enableMLD=0;
		apmib_set(MIB_WANIFACE_ENABLE_MLDPROXY,(void *)&wanEntry.enableMLD);
		wanEntry.ipv6WanManualConfig=IPV6_ORIGIN_SLAAC;
		#endif
	}else
	if(strcmp(tmpStr,"AUTO")==0){
		wanEntry.ipv6Origin=IPV6_ORIGIN_AUTO;
		#ifdef CONFIG_CMCC
		wanEntry.enableMLD=0;
		apmib_set(MIB_WANIFACE_ENABLE_MLDPROXY,(void *)&wanEntry.enableMLD);
		wanEntry.ipv6WanManualConfig=IPV6_ORIGIN_AUTO;
		#endif
	}else
	if(strcmp(tmpStr,"DHCP")==0){
		wanEntry.ipv6Origin=IPV6_ORIGIN_DHCP6C;
		#ifdef CONFIG_CMCC
		wanEntry.enableMLD=1;
		apmib_set(MIB_WANIFACE_ENABLE_MLDPROXY,(void *)&wanEntry.enableMLD);
		wanEntry.ipv6WanManualConfig=IPV6_ORIGIN_DHCP6C;
		#endif
	}else
	if(strcmp(tmpStr,"Static")==0){
		#ifdef CONFIG_CMCC
		wanEntry.enableMLD=1;
		apmib_set(MIB_WANIFACE_ENABLE_MLDPROXY,(void *)&wanEntry.enableMLD);
		wanEntry.ipv6WanManualConfig=IPV6_ORIGIN_STATIC;
		#endif
		wanEntry.ipv6Origin=IPV6_ORIGIN_STATIC;
		strIp = req_get_cstream_var(wp, "IdIpv6Addr", "");
		if(strIp[0]){
			inet_pton(PF_INET6, strIp, &ip6Addr);
			for(i=0;i<8;i++)
				ip6Addr.s6_addr16[i]=htons(ip6Addr.s6_addr16[i]);
			memcpy(wanEntry.ipv6Addr.addrIPv6,&ip6Addr,sizeof(struct in6_addr));
		}
		strVal= req_get_cstream_var(wp, "IdIpv6PrefixLen", "");
		if(strVal[0]){
			prefixLen=strtol(strVal, (char**)NULL, 10);
			wanEntry.ipv6Addr.prefix_len=prefixLen;
		}
		strIp = req_get_cstream_var(wp, "IdIpv6Gateway", "");
		if(strIp[0]){
			inet_pton(PF_INET6, strIp, &ip6Addr);
			for(i=0;i<8;i++)
				ip6Addr.s6_addr16[i]=htons(ip6Addr.s6_addr16[i]);
			memcpy(wanEntry.ipv6Gw.addrIPv6,&ip6Addr,sizeof(struct in6_addr));
			wanEntry.ipv6Gw.prefix_len=prefixLen;
		}
		strIp = req_get_cstream_var(wp, "IdIpv6Dns1", "");
		if(strIp[0]){
			inet_pton(PF_INET6, strIp, &ip6Addr);
			for(i=0;i<8;i++)
				ip6Addr.s6_addr16[i]=htons(ip6Addr.s6_addr16[i]);
			memcpy(wanEntry.ipv6dns1.addrIPv6,&ip6Addr,sizeof(struct in6_addr));
			wanEntry.ipv6dns1.prefix_len=prefixLen;
		}
		strIp = req_get_cstream_var(wp, "IdIpv6Dns2", "");
		if(strIp[0]){
			inet_pton(PF_INET6, strIp, &ip6Addr);
			for(i=0;i<8;i++)
				ip6Addr.s6_addr16[i]=htons(ip6Addr.s6_addr16[i]);
			memcpy(wanEntry.ipv6dns2.addrIPv6,&ip6Addr,sizeof(struct in6_addr));
			wanEntry.ipv6dns2.prefix_len=prefixLen;
		}
	}

		else
	{
		sprintf(erroMsgBuf, "Invalid ipv6 addr type %s!", tmpStr);
		goto setformEthernet_cmcc_err_end;
	}
	tmpStr = req_get_cstream_var(wp, "enablepd", "");
	if(strcmp(tmpStr,"Yes")==0){
		wanEntry.ipv6DhcpPdEnable=1;
	}else
		wanEntry.ipv6DhcpPdEnable=0;
#ifdef CONFIG_DSLITE_SUPPORT
	tmpStr = req_get_cstream_var(wp, "cb_enabledslite", "");
	if(strcmp(tmpStr,"on")==0){
		wanEntry.dsliteEnable=1;
		strVal = req_get_cstream_var(wp, "dslitemode", "");
		if(strVal[0]){
			wanEntry.dsliteMode = strtol(strVal,(char**)NULL, 10);  //  dslitemode value : manu =0, auto =1			
			if(wanEntry.dsliteMode==0){  
				strIp = req_get_cstream_var(wp, "dsliteaddress", "");
				if(strIp[0]){
					inet_pton(PF_INET6, strIp, &ip6Addr);
					for(i=0;i<8;i++)
						ip6Addr.s6_addr16[i]=htons(ip6Addr.s6_addr16[i]);
					memcpy(wanEntry.addr6AftrParam.addrIPv6,&ip6Addr,sizeof(struct in6_addr));
					wanEntry.addr6AftrParam.prefix_len=prefixLen;
				}
			}
		}
	}
#endif

	if( (wanEntry.ipv6Enable == 1) && (wanEntry.brmode == BRIDGE_ETHERNET)){ //select IPv6 bridge
		wanEntry.ipv6Origin=IPV6_ORIGIN_BRIDGE;		
#ifdef CONFIG_CMCC
		wanEntry.enableMLD=0;
		apmib_set(MIB_WANIFACE_ENABLE_MLDPROXY,(void *)&wanEntry.enableMLD);
#endif
	}

#ifdef CONFIG_CMCC
	tmpStr = req_get_cstream_var(wp, "wanMode", "");
	if(strcmp(tmpStr,"Bridge")==0){
		wanEntry.ipv6Origin=IPV6_ORIGIN_BRIDGE;		//cmcc webpage cannot select v4/v6 bridge/nat seperately, if set bridge, set v4 and v6 together.
	}
#endif

#endif
	if(cmcc_genWanName(index,&wanEntry)<0){
			snprintf(erroMsgBuf,sizeof(erroMsgBuf),"cmcc_genWanName fail!\n");
				goto setformEthernet_cmcc_err_end;
	}

	if(cmcc_setWanValues(index,&wanEntry,erroMsgBuf,sizeof(erroMsgBuf))<0)
		goto setformEthernet_cmcc_err_end;
	
setformEthernet_cmcc_ok:
	apmib_update_web(CURRENT_SETTING);
	
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	 // hidden page
	//printf("%s:%d get submit=%s \n",__FUNCTION__,__LINE__,submitUrl);
	fprintf(stderr,"send apply change event!\n");
	snprintf(cmdBuf,sizeof(cmdBuf),"reinitCli -e %d &", REINIT_EVENT_APPLYCAHNGES);
	system(cmdBuf);  
	if (submitUrl[0])
		OK_MSG(submitUrl);
	return;
setformEthernet_cmcc_err_end:
	ERR_MSG(erroMsgBuf);
	return;
}
void formFastConf(request *wp, char *path, char *query)
{
	char *submitUrl=NULL,*strVal=NULL,*strIp=NULL, *tmpStr=NULL,*wanName=NULL;
	char erroMsgBuf[256]={0},cmdBuf[64]={0};
	WANIFACE_T wanEntry={0};
	int wanIdx=1,intVal=0;

	strVal=req_get_cstream_var(wp, "wanIdx", "");
	if(strVal[0]){
		wanIdx=strtol(strVal,(char**)NULL, 10);
	}
	if(wanIdx<=0||wanIdx>WANIFACE_NUM) wanIdx=1;
	
	wanEntry.enable=1;
	wanEntry.connDisable=0;
	strVal=req_get_cstream_var(wp, "ipmode", "");
	if(strVal[0]){
		intVal=strtol(strVal,(char**)NULL, 10);
		if(intVal==0)//dhcp
		{
			wanEntry.cmode=IP_ROUTE;
			wanEntry.AddressType=DHCP_CLIENT;
			wanEntry.brmode=BRIDGE_DISABLE;
			wanEntry.dhcpMtu=1500;
			wanEntry.dnsAuto=1;
		}else if(intVal==2)//pppoe
		{
			wanEntry.cmode=IP_PPP;
			wanEntry.AddressType=PPPOE;
			wanEntry.brmode=BRIDGE_DISABLE;
			tmpStr=req_get_cstream_var(wp, "username", "");
			if(tmpStr[0]){
				strcpy(wanEntry.pppUsername,tmpStr);
			}else
			{
				snprintf(erroMsgBuf,sizeof(erroMsgBuf),"pppoe username should not be null!");
				goto setformFastConf_end;
			}
			tmpStr=req_get_cstream_var(wp, "password", "");
			if(tmpStr[0]){
				strcpy(wanEntry.pppPassword,tmpStr);
			}else
			{
				snprintf(erroMsgBuf,sizeof(erroMsgBuf),"pppoe password should not be null!");
				goto setformFastConf_end;
			}
			tmpStr=req_get_cstream_var(wp, "serviceName", "");
			if(tmpStr[0]){
				strcpy(wanEntry.pppServiceName,tmpStr);
			}
			wanEntry.pppoeMtu=1492;
			wanEntry.dnsAuto=1;
			wanEntry.IpProtocol=1;
		}
	}
	else{
		snprintf(erroMsgBuf,sizeof(erroMsgBuf),"ipmode should not be null!");
				goto setformFastConf_end;
	}
	tmpStr=req_get_cstream_var(wp, "wanname", "");
	if(tmpStr[0]){
		strcpy(wanEntry.WanName,tmpStr);

	}
	if(cmcc_setWanValues(wanIdx,&wanEntry,erroMsgBuf,sizeof(erroMsgBuf))<0)
		goto setformFastConf_end;
	
setformFastConf_ok:
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	 // hidden page
	//printf("%s:%d get submit=%s \n",__FUNCTION__,__LINE__,submitUrl);
	fprintf(stderr,"send apply change event!\n");
	snprintf(cmdBuf,sizeof(cmdBuf),"reinitCli -e %d &", REINIT_EVENT_APPLYCAHNGES);
	system(cmdBuf);  
	if (submitUrl[0])
		OK_MSG(submitUrl);
	return;
setformFastConf_end:
	ERR_MSG(erroMsgBuf);
	return;
}

void formL2tpCfg(request *wp, char *path, char *query)
{		
	char *strIp, *strMask, *strGateway, *strMode;
	char *strVal, *strType;
	char tmpBuf[256];
	int intVal;
	struct in_addr inIp, inMask, inGateway;
	char *submitUrl;
#if defined(CONFIG_DYNAMIC_WAN_IP)
	char *strPPPGateway, *strWanIpType;
	struct in_addr inPPPGateway;
	WAN_IP_TYPE_T wanIpType;
#if defined(CONFIG_GET_SERVER_IP_BY_DOMAIN)
	char *strGetServByDomain=NULL;
	char *strGatewayDomain;
#endif
#endif
	//DHCP_T dhcp, curDhcp;
	char	*strName, *strPassword;
	//dhcp = L2TP;

	strName = req_get_cstream_var(wp, ("username"), "");
	if ( strName[0] ) {
		if ( apmib_set(MIB_L2TP_USER_NAME, (void *)strName) == 0) {
			strcpy(tmpBuf, ("Set L2TP user name MIB error!"));
			goto setErr_l2tp;
		}
	}
	strPassword = req_get_cstream_var(wp, ("password"), "");
	if ( strPassword[0] ) {
		if ( apmib_set(MIB_L2TP_PASSWORD, (void *)strPassword) == 0) {
			strcpy(tmpBuf, ("Set L2TP user password MIB error!"));
			goto setErr_l2tp;
		}
	}
	
	strGateway = req_get_cstream_var(wp, ("ServerIpAddr"), "");
	if ( strGateway[0] ) {
		if ( !inet_aton(strGateway, &inGateway) ) {
			strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_l2tp_server)</script>"));
			goto setErr_l2tp;
		}
		if ( !apmib_set(MIB_L2TP_SERVER_IP_ADDR, (void *)&inGateway)) {
			strcpy(tmpBuf, ("Set pptp server ip error!"));
			goto setErr_l2tp;
		}
	}

#if defined(CONFIG_GET_SERVER_IP_BY_DOMAIN)
	strGetServByDomain = req_get_cstream_var(wp,"GetServMode","");
	if(strGetServByDomain[0])
	{
		if(!strcmp(strGetServByDomain,"GetServByDomainName"))
		{
			intVal=1;
			if(!apmib_set(MIB_L2TP_GET_SERV_BY_DOMAIN,(void*)&intVal))
			{
				strcpy(tmpBuf, ("Set l2tp get server by domain error!"));
					goto setErr_l2tp;
			}
			strGatewayDomain = req_get_cstream_var(wp, ("ServerDomainName"), "");
			if(strGatewayDomain[0])
			{
				if ( !apmib_set(MIB_L2TP_SERVER_DOMAIN, (void *)strGatewayDomain)) {
					strcpy(tmpBuf, ("Set l2tp server domain error!"));
					goto setErr_l2tp;
				}
			}
		}else
		{
			intVal=0;
			if(!apmib_set(MIB_L2TP_GET_SERV_BY_DOMAIN,(void*)&intVal))
			{
				strcpy(tmpBuf, ("Set l2tp get server by domain error!"));
					goto setErr_l2tp;
			}
		}
	}
	
#endif

strType = req_get_cstream_var(wp, ("pppconntype"), "");
	if ( strType[0] ) {
		PPP_CONNECT_TYPE_T type;
		if ( strType[0] == '0' )
			type = CONTINUOUS;
		else if ( strType[0] == '1' )
			type = CONNECT_ON_DEMAND;
		else if ( strType[0] == '2' )
			type = MANUAL;
		else {
			strcpy(tmpBuf, ("<script>dw(tcpip_wan_invalid_l2tp)</script>"));
			goto setErr_l2tp;
		}
		if ( apmib_set(MIB_L2TP_CONNECTION_TYPE, (void *)&type) == 0) {
			strcpy(tmpBuf, ("Set L2TP type MIB error!"));
			goto setErr_l2tp;
		}
		if (type != CONTINUOUS) {
			char *strTime;
			strTime = req_get_cstream_var(wp, ("idletime"), "");
			if ( strTime[0] ) {
				int time;
				time = strtol(strTime, (char**)NULL, 10) * 60;
				if ( apmib_set(MIB_L2TP_IDLE_TIME, (void *)&time) == 0) {
					strcpy(tmpBuf, ("Set L2TP idle time MIB error!"));
					goto setErr_l2tp;
				}
			}
		}
	}
	strVal = req_get_cstream_var(wp, ("MtuSize"), "");
	if ( strVal[0] ) {
		int mtuSize;
		mtuSize = strtol(strVal, (char**)NULL, 10);
		if ( apmib_set(MIB_L2TP_MTU_SIZE, (void *)&mtuSize) == 0) {
			strcpy(tmpBuf, ("Set L2TP mtu size MIB error!"));
			goto setErr_l2tp;
		}
	}

	apmib_update_web(CURRENT_SETTING);

	submitUrl = req_get_cstream_var(wp, ("submit-url"), "");
	if (submitUrl[0]){
		//OK_MSG(submitUrl);
		req_format_write(wp, "<html><head>"); \
        req_format_write(wp, "<meta http-equiv=\"refresh\" content=\"0;url=%s\"></head></html>", submitUrl); 
	}
		return;
	
setErr_l2tp:
	ERR_MSG(tmpBuf);
	return; 

}
#endif //CONFIG_CMCC
#endif //MULTI_WAN_SUPPORT

