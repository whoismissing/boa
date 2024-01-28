/*
 *      Web server handler routines for firewall
 *
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *
 *      $Id: fmfwall.c,v 1.20 2009/07/09 03:21:23 keith_huang Exp $
 *
 */

/*-- System inlcude files --*/
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>

#include "boa.h"
#include "globals.h"
#include "apform.h"
#include "apmib.h"
#include "utility.h"
#include "asp_page.h"

#if defined (QOS_BY_BANDWIDTH) || defined (MULTI_WAN_QOS)
static const char s4dashes[] = "----";
#endif

#if defined (MULTI_WAN_QOS)
#if defined (CONFIG_RTL_8198)|| defined (CONFIG_RTL_8198C) || defined (CONFIG_RTL_8367R_SUPPORT) || defined(CONFIG_RTL_83XX_SUPPORT)
#define MAX_BAND_WIDTH 1000000
#else
#define MAX_BAND_WIDTH 100000
#endif
#elif defined (QOS_BY_BANDWIDTH)
#if defined (CONFIG_RTL_8198)|| defined (CONFIG_RTL_8198C) || defined (CONFIG_RTL_8367R_SUPPORT) || defined(CONFIG_RTL_83XX_SUPPORT)
#define MAX_BAND_WIDTH 1024000
#else
#define MAX_BAND_WIDTH 102400
#endif
#endif

#if defined(VLAN_CONFIG_SUPPORTED)
struct nameMapping
{
	char display[32];
	char ifname[16];
};
static struct nameMapping vlanNameMapping[15] =
{
	{"Ethernet Port1","eth0"},
	{"Ethernet Port2","eth2"},
	{"Ethernet Port3","eth3"},
	{"Ethernet Port4","eth4"},
	{"Ethernet Port5","eth1"},
	{"Wireless 1 Primary AP","wlan0"},
	{"Wireless 1 Virtual AP1","wlan0-va0"},
	{"Wireless 1 Virtual AP2","wlan0-va1"},
	{"Wireless 1 Virtual AP3","wlan0-va2"},
	{"Wireless 1 Virtual AP4","wlan0-va3"},
	{"Wireless 2 Primary AP","wlan1"},
	{"Wireless 2 Virtual AP1","wlan1-va0"},
	{"Wireless 2 Virtual AP2","wlan1-va1"},
	{"Wireless 2 Virtual AP3","wlan1-va2"},
	{"Wireless 2 Virtual AP4","wlan1-va3"},
};

static struct nameMapping* findNameMapping(const char *display)
{
	int i;
	for(i = 0; i < MAX_IFACE_VLAN_CONFIG;i++)
	{
		if(strcmp(display,vlanNameMapping[i].display) == 0)
			return &vlanNameMapping[i];
	}
	return NULL;
}

int vlanList(request *wp, int idx)
{
	VLAN_CONFIG_T entry;
	char *strToken;
	int cmpResult=0;
	//char *tmpStr0;
	int  index=0;
	char IfaceName[32];
	OPMODE_T opmode=-1;
	char wanLan[8];
	char bufStr[128];

#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT) ||defined(CONFIG_RTL_HW_VLAN_SUPPORT)
	unsigned char forwarding_rule;
#endif

	memset(IfaceName,0x00,sizeof(IfaceName));
	memset(wanLan,0x00,sizeof(wanLan));
	memset(bufStr,0x00,sizeof(bufStr));

	index = idx;

	if( index <= MAX_IFACE_VLAN_CONFIG && index != 0) /* ignore item 0 */
	{

    #ifdef RTK_USB3G_PORT5_LAN
        DHCP_T wan_dhcp = -1;
        apmib_get( MIB_DHCP, (void *)&wan_dhcp);
    #endif

		*((char *)&entry) = (char)index;

		if ( !apmib_get(MIB_VLANCONFIG_TBL, (void *)&entry))
		{
			fprintf(stderr,"Get vlan entry fail\n");
			return -1;
		}
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT) ||defined(CONFIG_RTL_HW_VLAN_SUPPORT)
		forwarding_rule = entry.forwarding_rule;
#endif
		apmib_get( MIB_OP_MODE, (void *)&opmode);

		switch(index)
		{
			case 1:
			case 2:
			case 3:
			case 4:
				sprintf(IfaceName,"%s%d","Ethernet Port",index);
				sprintf(wanLan,"%s","LAN");
				break;
			case 5:
				sprintf(IfaceName,"%s","Wireless 1 Primary AP");
				if(opmode == WISP_MODE)
				{
					sprintf(wanLan,"%s","WAN");
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT)
					forwarding_rule = VLAN_FORWARD_NAT;
#endif
				}
				else
				{
					sprintf(wanLan,"%s","LAN");
				}
				break;
			case 6:
			case 7:
			case 8:
			case 9:
				sprintf(IfaceName,"%s%d","Wireless 1 Virtual AP",index-5);
				sprintf(wanLan,"%s","LAN");
				break;
			case 10:
				sprintf(IfaceName,"%s","Wireless 2 Primary AP");
				sprintf(wanLan,"%s","LAN");
				break;
			case 11:
			case 12:
			case 13:
			case 14:
				sprintf(IfaceName,"%s%d","Wireless 2 Virtual AP",index-10);
				sprintf(wanLan,"%s","LAN");
				break;

			case 15:
				sprintf(IfaceName,"%s","Ethernet Port5");
#ifdef RTK_USB3G_PORT5_LAN
				if(opmode == WISP_MODE || opmode == BRIDGE_MODE || wan_dhcp == USB3G)
#else
				if(opmode == WISP_MODE || opmode == BRIDGE_MODE)
#endif
				{
					sprintf(wanLan,"%s","LAN");
				}
				else
				{
					sprintf(wanLan,"%s","WAN");

#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT) ||defined(CONFIG_RTL_HW_VLAN_SUPPORT)
					forwarding_rule = VLAN_FORWARD_NAT;
#endif
				}
				break;
			case 16:
			sprintf(IfaceName,"%s","Local Host/WAN");
				sprintf(wanLan,"%s","LAN");
				break;
		}

		/* enabled/netIface/tagged/untagged/priority/cfi/groupId/vlanId/LanWan */
		//req_format_write(wp, ("%d|%s|%d|%d|%d|%d|%d|%d|%s"), entry.enabled,IfaceName,entry.tagged,0,entry.priority,entry.cfi,0,entry.vlanId,wanLan);
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT) ||defined(CONFIG_RTL_HW_VLAN_SUPPORT)
		sprintf(bufStr, "token[%d] =\'%d|%s|%d|%d|%d|%d|%d|%d|%s|%d\';\n",idx,entry.enabled,IfaceName,entry.tagged,0,entry.priority,entry.cfi,0,entry.vlanId,wanLan, forwarding_rule);
#else
		sprintf(bufStr, "token[%d] =\'%d|%s|%d|%d|%d|%d|%d|%d|%s\';\n",idx, entry.enabled,IfaceName,entry.tagged,0,entry.priority,entry.cfi,0,entry.vlanId,wanLan);
#endif
	}
	else
	{
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT) ||defined(CONFIG_RTL_HW_VLAN_SUPPORT)
		sprintf(bufStr, "token[%d] =\'0|none|0|0|0|0|0|0|LAN|0\';\n", idx);
#else
		sprintf(bufStr, "token[%d] =\'0|none|0|0|0|0|0|0|LAN\';\n", idx);
#endif
	}
	req_format_write(wp, bufStr);
	return 0;
}

int getVlanList(request *wp, int argc, char **argv)
{
	int i, maxWebVlanNum;

#if defined(CONFIG_RTL_8198_AP_ROOT) && defined(GMII_ENABLED)
	maxWebVlanNum = MAX_IFACE_VLAN_CONFIG-2;
#else
	maxWebVlanNum = MAX_IFACE_VLAN_CONFIG-1;
#endif
	for (i=0; i<=maxWebVlanNum; i++) {
		vlanList(wp, i);
	}
	return 0;
}

void formVlan(request *wp, char *path, char *query)
{
	VLAN_CONFIG_T entry;
	char *submitUrl,*strTmp;
	int	i, vlan_onoff;
	struct nameMapping *mapping;
	char tmpBuf[100];

	//displayPostDate(wp->post_data);
	//printf("--%s(%d)--\n", __FUNCTION__, __LINE__);

	strTmp= req_get_cstream_var(wp, ("vlan_onoff"), "");
	if(strTmp[0])
	{
		vlan_onoff = atoi(strTmp);
	}

	if (!apmib_set(MIB_VLANCONFIG_ENABLED, (void *)&vlan_onoff))
	{
		strcpy(tmpBuf, ("set  MIB_VLANCONFIG_ENABLED error!"));
	//	printf("--%s(%d)--\n", __FUNCTION__, __LINE__);
		goto setErr;
	}
	if(vlan_onoff == 1)
	{
		if ( !apmib_set(MIB_VLANCONFIG_DELALL, (void *)&entry))
		{
			strcpy(tmpBuf, ("Delete all table error!"));
		//	printf("--%s(%d)--\n", __FUNCTION__, __LINE__);
			goto setErr;
		}

		for(i=1; i<=MAX_IFACE_VLAN_CONFIG ; i++)
		{
			memset(&entry, '\0', sizeof(entry));
		//	printf("--%s(%d)--i is %d\n", __FUNCTION__, __LINE__, i);

			*((char *)&entry) = (char)i;
			apmib_get(MIB_VLANCONFIG_TBL, (void *)&entry);

			memset(tmpBuf,0x00, sizeof(tmpBuf));
			sprintf(tmpBuf,"vlan_iface_%d",i);
			strTmp = req_get_cstream_var(wp, tmpBuf, "");

			if(strTmp[0])
			{
				//strcpy(entry.netIface,strTmp);

				mapping = findNameMapping(strTmp);

				if(mapping)
				{
					strcpy((char *)entry.netIface,mapping->ifname);
				}
			}
			else
			{
		//	printf("--%s(%d)--\n", __FUNCTION__, __LINE__);
				if ( apmib_set(MIB_VLANCONFIG_ADD, (void *)&entry) == 0)
				{
					strcpy(tmpBuf, ("Add table entry error!"));
	//				printf("--%s(%d)--\n", __FUNCTION__, __LINE__);
					goto setErr;
				}
	//			printf("--%s(%d)--\n", __FUNCTION__, __LINE__);
				continue;
			}

			memset(tmpBuf,0x00, sizeof(tmpBuf));
			sprintf(tmpBuf,"vlan_enable_%d",i);
			strTmp = req_get_cstream_var(wp, tmpBuf, "");
			if(strTmp[0])
			{
				entry.enabled = atoi(strTmp);
			}

			memset(tmpBuf,0x00, sizeof(tmpBuf));
			sprintf(tmpBuf,"vlan_tag_%d",i);
			strTmp = req_get_cstream_var(wp, tmpBuf, "");
			if(strTmp[0])
			{
				entry.tagged = atoi(strTmp);
			}

			memset(tmpBuf,0x00, sizeof(tmpBuf));
			sprintf(tmpBuf,"vlan_cfg_%d",i);
			strTmp = req_get_cstream_var(wp, tmpBuf, "");
			if(strTmp[0])
			{
				entry.cfi = atoi(strTmp);
			}

			memset(tmpBuf,0x00, sizeof(tmpBuf));
			sprintf(tmpBuf,"vlan_id_%d",i);
			strTmp = req_get_cstream_var(wp, tmpBuf, "");
			if(strTmp[0])
			{
				entry.vlanId = atoi(strTmp);
			}

			memset(tmpBuf,0x00, sizeof(tmpBuf));
			sprintf(tmpBuf,"vlan_priority_%d",i);
			strTmp = req_get_cstream_var(wp, tmpBuf, "");
			if(strTmp[0])
			{
				entry.priority = atoi(strTmp);
			}
#if defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT) || defined(CONFIG_RTL_HW_VLAN_SUPPORT)

			memset(tmpBuf,0x00, sizeof(tmpBuf));
			sprintf(tmpBuf,"vlan_forward_%d",i);
			strTmp = req_get_cstream_var(wp, tmpBuf, "");
			if(strTmp[0])
			{
				entry.forwarding_rule = atoi(strTmp);
			}
#endif

			if ( apmib_set(MIB_VLANCONFIG_ADD, (void *)&entry) == 0)
			{
				strcpy(tmpBuf, ("Add table entry error!"));
//				printf("--%s(%d)--\n", __FUNCTION__, __LINE__);
				goto setErr;
			}




		}

	}

	apmib_update_web(CURRENT_SETTING);
#ifdef MODULES_REINIT
		set_reinit_flag(REINIT_FLAG_LAN|REINIT_FLAG_WAN);
#endif

#ifndef NO_ACTION
	run_init_script("all");
#endif

	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
	{
		OK_MSG(submitUrl);
	}
  	return;

setErr:
	ERR_MSG(tmpBuf);
	return;

}
#endif

#ifdef HOME_GATEWAY

/////////////////////////////////////////////////////////////////////////////
void formPortFw(request *wp, char *path, char *query)
{
	char *submitUrl, *strAddPort, *strDelPort, *strVal, *strDelAllPort;
	char *strIp, *strFrom, *strTo, *strComment;
	char tmpBuf[100];
	int entryNum, intVal, i, j;
	PORTFW_T entry;
	struct in_addr curIpAddr, curSubnet;
	unsigned long v1, v2, v3;
#ifndef NO_ACTION
	int pid;
#endif

	strAddPort = req_get_cstream_var(wp, ("addPortFw"), "");
#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strAddPort[0]==0){
		strAddPort = req_get_cstream_var(wp, ("addPortFwFlag"), "");
	}
#endif	
#if defined(MULTI_WAN_SUPPORT)
	unsigned short pfWanMask = 0;
#endif
#if defined(CONFIG_RTL_PORTFW_EXTEND)
	char *strExternelFrom, *strExternelTo,*strRmtIp;
#endif
	strDelPort = req_get_cstream_var(wp, ("deleteSelPortFw"), "");
	strDelAllPort = req_get_cstream_var(wp, ("deleteAllPortFw"), "");

	memset(&entry, '\0', sizeof(entry));

	/* Add new port-forwarding table */
	if (strAddPort[0]) {
		strVal = req_get_cstream_var(wp, ("enabled"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;
		if ( apmib_set( MIB_PORTFW_ENABLED, (void *)&intVal) == 0) {
			strcpy(tmpBuf, ("Set enabled flag error!"));
			goto setErr_portfw;
		}

		strIp = req_get_cstream_var(wp, ("ip"), "");
		strFrom = req_get_cstream_var(wp, ("fromPort"), "");
		strTo = req_get_cstream_var(wp, ("toPort"), "");

		#if defined(CONFIG_RTL_PORTFW_EXTEND)
		strExternelFrom = req_get_cstream_var(wp, ("externelFromPort"), "");
		strExternelTo= req_get_cstream_var(wp, ("externelToPort"), "");
		strRmtIp = req_get_cstream_var(wp, ("rmtip"), "");
		#endif
		
		strComment = req_get_cstream_var(wp, ("comment"), "");
	
		#if defined(CONFIG_RTL_PORTFW_EXTEND)
		if (!strIp[0] && !strFrom[0] && !strTo[0] && !strExternelFrom[0] && !strExternelTo[0]&& !strRmtIp[0] && !strComment[0])
		#else
		if (!strIp[0] && !strFrom[0] && !strTo[0] && !strComment[0])
		#endif
			goto setOk_portfw;
		
		#ifdef MULTI_WAN_SUPPORT
		for(i = 1; i <= WANIFACE_NUM; i++){
			char pfWan[32] = {0};
			char *pfWanVal = NULL;
			snprintf(pfWan, sizeof(pfWan), "WAN%d", i);
			pfWanVal = req_get_cstream_var(wp, pfWan, "");
			if(strcmp(pfWanVal, "ON") == 0){
				pfWanMask |= (1 << (i-1));
			}
		}
		
		if(!pfWanMask){
			strcpy(tmpBuf, ("Please select the port-forwarding wan."));
			goto setErr_portfw;
		}	
	//	printf("[%s:%d] pfWanMask = %d\n",__FUNCTION__, __LINE__,pfWanMask);
		#endif

		if (!strIp[0]) {
			strcpy(tmpBuf, ("Error! No ip address to set."));
			goto setErr_portfw;
		}

		inet_aton(strIp, (struct in_addr *)&entry.ipAddr);
		getInAddr(BRIDGE_IF, IP_ADDR, (void *)&curIpAddr);
		getInAddr(BRIDGE_IF, SUBNET_MASK, (void *)&curSubnet);

		v1 = *((unsigned long *)entry.ipAddr);
		v2 = *((unsigned long *)&curIpAddr);
		v3 = *((unsigned long *)&curSubnet);

		if ( (v1 & v3) != (v2 & v3) ) {
			strcpy(tmpBuf, ("Invalid IP address! It should be set within the current subnet."));
			goto setErr_portfw;
		}
		if ( v1 == v2 ) {
				strcpy(tmpBuf, "<script>dw(dmz_ip_error2)</script>");
				goto setErr_portfw;
		}

		if ( !strFrom[0] ) { // if port-forwarding, from port must exist
			strcpy(tmpBuf, ("Error! No from-port value to be set."));
			goto setErr_portfw;
		}
		
		if ( !string_to_dec(strFrom, &intVal) || intVal<1 || intVal>65535) {
			strcpy(tmpBuf, ("Error! Invalid value of from-port."));
			goto setErr_portfw;
		}
		entry.fromPort = (unsigned short)intVal;
		
		if(strTo[0]){
			if ( !string_to_dec(strTo, &intVal) || intVal<1 || intVal>65535) {
				strcpy(tmpBuf, ("Error! Invalid value of to-port."));
				goto setErr_portfw;
			}
			entry.toPort = (unsigned short)intVal;
		}else{
			entry.toPort = entry.fromPort;
		}
		
		if ( entry.fromPort  > entry.toPort ) {
			strcpy(tmpBuf, ("Error! Invalid port range."));
			goto setErr_portfw;
		}

		#if defined(CONFIG_RTL_PORTFW_EXTEND)
		if ( !strExternelFrom[0] ) { // if port-forwarding, from port must exist
			strcpy(tmpBuf, ("Error! No Externel from-port value to be set."));
			goto setErr_portfw;
		}
		if ( !string_to_dec(strExternelFrom, &intVal) || intVal<1 || intVal>65535) {
			strcpy(tmpBuf, ("Error! Invalid value of Externel from-port."));
			goto setErr_portfw;
		}
		entry.externelFromPort = (unsigned short)intVal;
		
		if(strExternelTo[0]){
			if ( !string_to_dec(strExternelTo, &intVal) || intVal<1 || intVal>65535) {
				strcpy(tmpBuf, ("Error! Invalid value of Externel to-port."));
				goto setErr_portfw;
			}
			entry.externelToPort =  (unsigned short)intVal;
		}else{
			entry.externelToPort =  entry.externelFromPort;
		}

		if ( entry.externelFromPort  > entry.externelToPort ) {
			strcpy(tmpBuf, ("Error! Invalid port range."));
			goto setErr_portfw;
		}
				
		if (strRmtIp[0]) {			
			inet_aton(strRmtIp, (struct in_addr *)&entry.rmtipAddr);
			v1 = *((unsigned long *)entry.rmtipAddr);
			if ( (v1 & v3) == (v2 & v3) ) {
				strcpy(tmpBuf, ("Invalid IP address! It should not be set within the current lan subnet."));
				goto setErr_portfw;
			}
		}
		#endif

		strVal = req_get_cstream_var(wp, ("protocol"), "");
		if (strVal[0]) {
			if ( strVal[0] == '0' )
				entry.protoType = PROTO_BOTH;
			else if ( strVal[0] == '1' )
				entry.protoType = PROTO_TCP;
			else if ( strVal[0] == '2' )
				entry.protoType = PROTO_UDP;
			else {
				strcpy(tmpBuf, ("Error! Invalid protocol type."));
				goto setErr_portfw;
			}
		}
		else {
			strcpy(tmpBuf, ("Error! Protocol type cannot be empty."));
			goto setErr_portfw;
		}

		if ( strComment[0] ) {
			if (strlen(strComment) > COMMENT_LEN-1) {
				strcpy(tmpBuf, ("Error! Comment length too long."));
				goto setErr_portfw;
			}
			strcpy((char *)entry.comment, strComment);
		}
		
#ifdef MULTI_WAN_SUPPORT	
		for(i = 1; i<= WANIFACE_NUM; i++){
			if( (pfWanMask&(1 << (i-1))) == 0)
				continue;
			entry.WANIfIndex = i;
			
		//	printf("[%s:%d] i = %d\n",__FUNCTION__, __LINE__,i);
#endif
			if ( !apmib_get(MIB_PORTFW_TBL_NUM, (void *)&entryNum)) {
				strcpy(tmpBuf, ("Get entry number error!"));
				goto setErr_portfw;
			}

			if ( (entryNum + 1) > MAX_FILTER_NUM) {
				strcpy(tmpBuf, ("Cannot add new entry because table is full!"));
				goto setErr_portfw;
			}			

			// Check if there is any port overlapped
			for (j=1; j<=entryNum; j++) {
				PORTFW_T checkEntry;
				*((char *)&checkEntry) = (char)j;
				if ( !apmib_get(MIB_PORTFW_TBL, (void *)&checkEntry)) {
					strcpy(tmpBuf, ("Get table entry error!"));
					goto setErr_portfw;
				}
				if ( ( (entry.fromPort <= checkEntry.fromPort && entry.toPort >= checkEntry.fromPort) ||
				       (entry.fromPort >= checkEntry.fromPort && entry.fromPort <= checkEntry.toPort))&&
				       (entry.protoType & checkEntry.protoType)
#ifdef MULTI_WAN_SUPPORT
						&&(entry.WANIfIndex == checkEntry.WANIfIndex)
#endif
				) {
					strcpy(tmpBuf, ("Setting port range has overlapped with used port numbers!"));
					goto setErr_portfw;
				}
			}

		#if defined(CONFIG_RTL_PORTFW_EXTEND)
			//currently set portforwarding by web ui default enable this entry.
			entry.enabled = 1;
		#endif
			// set to MIB. try to delete it first to avoid duplicate case
			apmib_set(MIB_PORTFW_DEL, (void *)&entry);
			if ( apmib_set(MIB_PORTFW_ADD, (void *)&entry) == 0) {
				strcpy(tmpBuf, ("Add table entry error!"));
				goto setErr_portfw;
			}
#ifdef MULTI_WAN_SUPPORT
		}
#endif	
	}

	/* Delete entry */
	if (strDelPort[0]) {
		if ( !apmib_get(MIB_PORTFW_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get entry number error!"));
			goto setErr_portfw;
		}

		for (i=entryNum; i>0; i--) {
			snprintf(tmpBuf, 20, "select%d", i);
			strVal = req_get_cstream_var(wp, tmpBuf, "");
			if ( !strcmp(strVal, "ON") ) {
				*((char *)&entry) = (char)i;
				if ( !apmib_get(MIB_PORTFW_TBL, (void *)&entry)) {
					strcpy(tmpBuf, ("Get table entry error!"));
					goto setErr_portfw;
				}
				if ( !apmib_set(MIB_PORTFW_DEL, (void *)&entry)) {
					strcpy(tmpBuf, ("Delete table entry error!"));
					goto setErr_portfw;
				}
			}
		}
	}

	/* Delete all entry */
	if ( strDelAllPort[0]) {
		if ( !apmib_set(MIB_PORTFW_DELALL, (void *)&entry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_portfw;
		}
	}

setOk_portfw:
	apmib_update_web(CURRENT_SETTING);

#if !defined(MULTI_WAN_SUPPORT)
#ifndef NO_ACTION
	pid = fork();
        if (pid) {
	      	waitpid(pid, NULL, 0);
	}
        else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
               	exit(1);
        }
#endif
#endif

	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
#if defined(MULTI_WAN_SUPPORT)
	OK_MSG(submitUrl);
	return ;
#else
#ifdef REBOOT_CHECK
	if(needReboot == 1)
	{
		OK_MSG(submitUrl);
		return;
	}
#endif

	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
  	return;
#endif
setErr_portfw:
	ERR_MSG(tmpBuf);
}


/////////////////////////////////////////////////////////////////////////////
void formFilter(request *wp, char *path, char *query)
{
	char *strAddIp, *strAddPort, *strAddMac, *strDelPort, *strDelIp, *strDelMac;
	char *strDelAllPort, *strDelAllIp, *strDelAllMac, *strVal, *strEndIpAddr, *strDstIpAddr, *strDstEndIpAddr, *strDMac, *submitUrl, *strComment;
	char *strFrom, *strTo;
	char *filterMode;
#ifdef CONFIG_IPV6
	char *strIP6;
	char *ipVer;
	char ip6Enable = 0;
	char ip4Enable = 0;
#endif
	char tmpBuf[100];
	int entryNum, intVal, i, j;
	IPFILTER_T ipEntry, ipentrytmp;
	PORTFILTER_T portEntry, entrytmp;
	MACFILTER_T macEntry, macEntrytmp;
	struct in_addr curIpAddr, curSubnet;
	void *pEntry;
	unsigned long v1, v2, v3,v4;
	int num_id, get_id, add_id, del_id, delall_id, enable_id;
	char *strAddUrl, *strDelUrl;
	char *strDelAllUrl,*strUrlMode,*strUsrMode;
	int mode;/*url mode:white list or black list*/
	int usrMode;/*user mode:for all,specific ip or specific mac*/
	URLFILTER_T urlEntry, urlEntrytmp;
#ifdef MACFILTER_MODE_SUPPORT
	char *strMacMode;
#endif


#if defined(IP_RANGE_MULTI_WAN_SUPPORT)
		unsigned short ipfWanMask = 0;
#endif
	
#ifndef NO_ACTION
	int pid;
#endif
	strAddIp = req_get_cstream_var(wp, ("addFilterIp"), "");
#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strAddIp[0]==0){
		strAddIp = req_get_cstream_var(wp, ("addFilterIpFlag"), "");
	}
#endif	
	strDelIp = req_get_cstream_var(wp, ("deleteSelFilterIp"), "");
	strDelAllIp = req_get_cstream_var(wp, ("deleteAllFilterIp"), "");

	strAddPort = req_get_cstream_var(wp, ("addFilterPort"), "");
#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strAddPort[0]==0){
		strAddPort = req_get_cstream_var(wp, ("addFilterPortFlag"), "");
	}
#endif	
	strDelPort = req_get_cstream_var(wp, ("deleteSelFilterPort"), "");
	strDelAllPort = req_get_cstream_var(wp, ("deleteAllFilterPort"), "");

	strAddMac = req_get_cstream_var(wp, ("addFilterMac"), "");
#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strAddMac[0]==0){
		strAddMac = req_get_cstream_var(wp, ("addFilterMacFlag"), "");
	}
#endif	
	strDelMac = req_get_cstream_var(wp, ("deleteSelFilterMac"), "");
	strDelAllMac = req_get_cstream_var(wp, ("deleteAllFilterMac"), "");

	strAddUrl = req_get_cstream_var(wp, ("addFilterUrl"), "");
#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strAddUrl[0]==0){
		strAddUrl = req_get_cstream_var(wp, ("addFilterUrlFlag"), "");
	}
#endif	
	strDelUrl = req_get_cstream_var(wp, ("deleteSelFilterUrl"), "");
	strDelAllUrl = req_get_cstream_var(wp, ("deleteAllFilterUrl"), "");


	if (strAddIp[0] || strDelIp[0] || strDelAllIp[0]) {
		num_id = MIB_IPFILTER_TBL_NUM;
		get_id = MIB_IPFILTER_TBL;
		add_id = MIB_IPFILTER_ADD;
		del_id = MIB_IPFILTER_DEL;
		delall_id = MIB_IPFILTER_DELALL;
		enable_id = MIB_IPFILTER_ENABLED;
		memset(&ipEntry, '\0', sizeof(ipEntry));
		pEntry = (void *)&ipEntry;
	}
	else if (strAddPort[0] || strDelPort[0] || strDelAllPort[0]) {
		num_id = MIB_PORTFILTER_TBL_NUM;
		get_id = MIB_PORTFILTER_TBL;
		add_id = MIB_PORTFILTER_ADD;
		del_id = MIB_PORTFILTER_DEL;
		delall_id = MIB_PORTFILTER_DELALL;
		enable_id = MIB_PORTFILTER_ENABLED;
		memset(&portEntry, '\0', sizeof(portEntry));
		pEntry = (void *)&portEntry;
	}
	else if (strAddMac[0] || strDelMac[0] || strDelAllMac[0]) {
		num_id = MIB_MACFILTER_TBL_NUM;
		get_id = MIB_MACFILTER_TBL;
		add_id = MIB_MACFILTER_ADD;
		del_id = MIB_MACFILTER_DEL;
		delall_id = MIB_MACFILTER_DELALL;
		enable_id = MIB_MACFILTER_ENABLED;
		memset(&macEntry, '\0', sizeof(macEntry));
		pEntry = (void *)&macEntry;
	}
	else {
		num_id = MIB_URLFILTER_TBL_NUM;
		get_id = MIB_URLFILTER_TBL;
		add_id = MIB_URLFILTER_ADD;
		del_id = MIB_URLFILTER_DEL;
		delall_id = MIB_URLFILTER_DELALL;
		enable_id = MIB_URLFILTER_ENABLED;
		memset(&urlEntry, '\0', sizeof(urlEntry));
		pEntry = (void *)&urlEntry;
	}
	// Set enable flag
	if ( strAddIp[0] || strAddPort[0] || strAddMac[0] || strAddUrl[0]) {
		strVal = req_get_cstream_var(wp, ("enabled"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;
		//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);

		if ( apmib_set(enable_id, (void *)&intVal) == 0) {
			strcpy(tmpBuf, ("Set enabled flag error!"));
			goto setErr_filter;
		}
	}

	strComment = req_get_cstream_var(wp, ("comment"), "");

	/* Add IP filter */
	if (strAddIp[0]) {
		strVal = req_get_cstream_var(wp, ("ip"), "");

#ifdef IP_RANGE_MULTI_WAN_SUPPORT
		strEndIpAddr = req_get_cstream_var(wp, ("ipend"), "");
		strDstIpAddr = req_get_cstream_var(wp, ("dst_ip"), "");
		strDstEndIpAddr = req_get_cstream_var(wp, ("dst_ipend"), "");

		for(i = 1; i <= WANIFACE_NUM; i++){
			char ipfWan[32] = {0};
			char *ipfWanVal = NULL;
			snprintf(ipfWan, sizeof(ipfWan), "WAN%d", i);
			ipfWanVal = req_get_cstream_var(wp, ipfWan, "");
			if(strcmp(ipfWanVal, "ON") == 0){
				ipfWanMask |= (1 << (i-1));
			}
		}
		
		if(!ipfWanMask&&(strVal[0]||strEndIpAddr[0]||strDstIpAddr[0]||strDstEndIpAddr[0])){
			 strcpy(tmpBuf, ("<script>dw(ipfilter_set_error1)</script>"));
			 goto setErr_filter;
		}
#endif


#ifdef CONFIG_IPV6
		strIP6 = req_get_cstream_var(wp, ("ip6addr"), "");
		ipVer = req_get_cstream_var(wp, ("ip_version"), "");
	//	ip4Enable = req_get_cstream_var(wp, ("ip_enabled"), "");
		//fprintf(stderr,"%s:%d ipVer=%s\n",__FUNCTION__,__LINE__,ipVer);
		if(atoi(ipVer) == 0){
			ip4Enable = 1;
			ip6Enable = 0;

		}else if(atoi(ipVer) == 1){
			ip4Enable = 0;
			ip6Enable = 1;
		}
		//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);
#endif
		if (!strVal[0] &&!strComment[0]
#ifdef IP_RANGE_MULTI_WAN_SUPPORT
			&&!strEndIpAddr[0]&&!strDstIpAddr[0]&&!strDstEndIpAddr[0]
#endif
#ifdef CONFIG_IPV6
			&& !strIP6[0]
#endif
			)
			{
#ifdef IP_RANGE_MULTI_WAN_SUPPORT
			filterMode = req_get_cstream_var(wp, ("ipFilterMode"), "");
		if ( !strcmp(filterMode, "on")){
			mode=1;
		}
        else{
			mode=0;
		}
		if ( apmib_set(MIB_IPFILTER_MODE, (void *)&mode) == 0) {
		        strcpy(tmpBuf, ("<script>dw(ipfilter_set_mode_error)</script>"));
			    goto setErr_filter;
		}
#endif
			goto setOk_filter;
            }
		//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);

		if (!strVal[0]
#ifdef IP_RANGE_MULTI_WAN_SUPPORT
			&&!strEndIpAddr[0]&&!strDstIpAddr[0]&&!strDstEndIpAddr[0]
#endif
#ifdef CONFIG_IPV6
			&& !strIP6[0]
#endif
			) {
			strcpy(tmpBuf, ("<script>dw(ipfilter_no_ipaddr)</script>"));
			goto setErr_filter;
		}


#ifdef CONFIG_IPV6
		if(strIP6[0]){
			ipEntry.ipVer=IPv6;
			strcpy(ipEntry.ip6Addr,strIP6);
		}
		else
			ipEntry.ipVer=IPv4;
#endif


		if((strVal[0]
#ifdef IP_RANGE_MULTI_WAN_SUPPORT
			&&strEndIpAddr[0]&&!strDstIpAddr[0]&&!strDstEndIpAddr[0])||
			(!strVal[0]&&!strEndIpAddr[0]&&strDstIpAddr[0]&&strDstEndIpAddr[0])||
			(strVal[0]&&strEndIpAddr[0]&&strDstIpAddr[0]&&strDstEndIpAddr[0]
#endif
			)){		
#ifdef IP_RANGE_MULTI_WAN_SUPPORT	
        if(strVal[0]&&strEndIpAddr[0]){
		inet_aton(strVal, (struct in_addr *)&ipEntry.ipAddr);	
		inet_aton(strEndIpAddr, (struct in_addr *)&ipEntry.ipAddrEnd);
		}
		else{
			strVal="0.0.0.0";
			strEndIpAddr="0.0.0.0";
		    inet_aton(strVal, (struct in_addr *)&ipEntry.ipAddr);
		    inet_aton(strEndIpAddr, (struct in_addr *)&ipEntry.ipAddrEnd);
			}
		
		if(strDstIpAddr[0]&&strDstEndIpAddr[0]){
		inet_aton(strDstIpAddr, (struct in_addr *)&ipEntry.dstipAddr);
		inet_aton(strDstEndIpAddr, (struct in_addr *)&ipEntry.dstipAddrEnd);
		}
		else{
		strDstIpAddr="0.0.0.0";
		strDstEndIpAddr="0.0.0.0";
		inet_aton(strDstIpAddr, (struct in_addr *)&ipEntry.dstipAddr);
		inet_aton(strDstEndIpAddr, (struct in_addr *)&ipEntry.dstipAddrEnd);
		}
#else
        inet_aton(strVal, (struct in_addr *)&ipEntry.ipAddr);	
#endif
		getInAddr(BRIDGE_IF, IP_ADDR, (void *)&curIpAddr);
		getInAddr(BRIDGE_IF, SUBNET_MASK, (void *)&curSubnet);
		//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);

		v1 = *((unsigned long *)ipEntry.ipAddr);
#ifdef IP_RANGE_MULTI_WAN_SUPPORT
		v4 = *((unsigned long *)ipEntry.ipAddrEnd);
#endif
		v2 = *((unsigned long *)&curIpAddr);
		v3 = *((unsigned long *)&curSubnet);
		
#ifdef IP_RANGE_MULTI_WAN_SUPPORT
        strVal = req_get_cstream_var(wp, ("ip"), "");
		strEndIpAddr = req_get_cstream_var(wp, ("ipend"), "");
		if(strVal[0]&&strEndIpAddr[0]){
#endif
		if ( ((v1 & v3) != (v2 & v3))
#ifdef IP_RANGE_MULTI_WAN_SUPPORT
			||((v4 & v3) != (v2 & v3))
#endif
			) {
			strcpy(tmpBuf, ("<script>dw(ipfilter_ipaddr_not_within_subnet)</script>"));
			goto setErr_filter;
		   }
		if ( (v1 == v2)
#ifdef IP_RANGE_MULTI_WAN_SUPPORT
			||(v4 ==v2)
#endif
			) 
		{
			strcpy(tmpBuf, "<script>dw(dmz_ip_error2)</script>");
			goto setErr_filter;
		}
#ifdef IP_RANGE_MULTI_WAN_SUPPORT
		}
#endif		
	   }
#ifdef IP_RANGE_MULTI_WAN_SUPPORT
		else{
		    strcpy(tmpBuf, ("<script>dw(ipfilter_miss_ipaddr)</script>"));
			goto setErr_filter;
		}
#endif	

#ifdef IP_RANGE_MULTI_WAN_SUPPORT		
		filterMode = req_get_cstream_var(wp, ("ipFilterMode"), "");
		if ( !strcmp(filterMode, "on")){
			mode=1;
			ipEntry.ipFilterMode=mode;
		}
        else{
			mode=0;
			ipEntry.ipFilterMode=mode;
		}
		if ( apmib_set(MIB_IPFILTER_MODE, (void *)&mode) == 0) {
		    	strcpy(tmpBuf, ("<script>dw(ipfilter_set_mode_error)</script>"));
			    goto setErr_filter;
		}
#endif
	}

	//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);


	/* Add port filter */
	if (strAddPort[0]) {
		strFrom = req_get_cstream_var(wp, ("fromPort"), "");
		strTo = req_get_cstream_var(wp, ("toPort"), "");
	
		if (!strFrom[0] && !strTo[0] && !strComment[0])
			goto setOk_filter;

		if (!strFrom[0]) { // if port-forwarding, from port must exist
			strcpy(tmpBuf, ("Error! No from-port value to be set."));
			goto setErr_filter;
		}
		if ( !string_to_dec(strFrom, &intVal) || intVal<1 || intVal>65535) {
			strcpy(tmpBuf, ("Error! Invalid value of from-port."));
			goto setErr_filter;
		}
		portEntry.fromPort = (unsigned short)intVal;

		if ( !strTo[0] )
			portEntry.toPort = portEntry.fromPort;
		else {
			if ( !string_to_dec(strTo, &intVal) || intVal<1 || intVal>65535) {
				strcpy(tmpBuf, ("Error! Invalid value of to-port."));
				goto setErr_filter;
			}
			portEntry.toPort = (unsigned short)intVal;
		}

		if ( portEntry.fromPort  > portEntry.toPort ) {
			strcpy(tmpBuf, ("Error! Invalid port range."));
			goto setErr_filter;
		}
#if 0
#ifdef CONFIG_IPV6
		ipVer = req_get_cstream_var(wp, ("ip6_enabled"), "");
		if(atoi(ipVer))
			portEntry.ipVer=IPv6;
		else
			portEntry.ipVer=IPv4;
#endif
#endif
	}

	if (strAddPort[0] || strAddIp[0]) {
		strVal = req_get_cstream_var(wp, ("protocol"), "");
		if (strVal[0]) {
			if ( strVal[0] == '0' ) {
				if (strAddPort[0])
					portEntry.protoType = PROTO_BOTH;
				else
					ipEntry.protoType = PROTO_BOTH;
			}
			else if ( strVal[0] == '1' ) {
				if (strAddPort[0])
					portEntry.protoType = PROTO_TCP;
				else
					ipEntry.protoType = PROTO_TCP;
			}
			else if ( strVal[0] == '2' ) {
				if (strAddPort[0])
					portEntry.protoType = PROTO_UDP;
				else
					ipEntry.protoType = PROTO_UDP;
			}
			#ifdef IP_RANGE_MULTI_WAN_SUPPORT
			else if ( strVal[0] == '3' ) {
				if (strAddPort[0]){
					strcpy(tmpBuf, ("Error! Invalid protocol type."));
				    goto setErr_filter;
					}
				else
					ipEntry.protoType = PROTO_ICMP;
			} 
			#endif
			else {
				strcpy(tmpBuf, ("Error! Invalid protocol type."));
				goto setErr_filter;
			}
		}
		else {
			strcpy(tmpBuf, ("Error! Protocol type cannot be empty."));
			goto setErr_filter;
		}
	}

	if (strAddMac[0]) {
		strVal = req_get_cstream_var(wp, ("mac"), "");
		#ifdef DST_MAC_FILTER
        strDMac = req_get_cstream_var(wp, ("dmac"), "");
		#endif

		#ifdef MACFILTER_MODE_SUPPORT
        strMacMode = req_get_cstream_var(wp, "macFilterMode", "");
		if(strMacMode[0]){
			mode=atoi(strMacMode);
			if ( apmib_set(MIB_MACFILTER_MODE, (void *)&mode) == 0) {
		    		strcpy(tmpBuf, ("Set mode flag error!"));
					goto setErr_filter;
		    }
       	}
		#endif
	
		if (!strVal[0] && 
	    #ifdef DST_MAC_FILTER
			!strDMac[0] &&
	    #endif
			!strComment[0])
			goto setOk_filter;

		if ( !strVal[0]
	    #ifdef DST_MAC_FILTER
			&& !strDMac[0]
	    #endif
			) {
			strcpy(tmpBuf, ("Error! No mac address to set."));
			goto setErr_filter;
		}
		/*if (strlen(strVal)!=12 || !string_to_hex(strVal, macEntry.macAddr, 12)) {
			strcpy(tmpBuf, ("Error! Invalid MAC address."));
			goto setErr_filter;
		}*/
		#ifdef DST_MAC_FILTER
		if(strVal[0])
		{
		#endif
		    if (strlen(strVal)!=12 || !string_to_hex(strVal, macEntry.macAddr, 12)) {
			strcpy(tmpBuf, ("Error! Invalid MAC address."));
			goto setErr_filter;
		#ifdef DST_MAC_FILTER
		    }
		}
		if(strDMac[0])
		{
		    if (strlen(strDMac)!=12 || !string_to_hex(strDMac, macEntry.dmacAddr, 12)) {
			strcpy(tmpBuf, ("Error! Invalid DMAC address."));
			goto setErr_filter;
		    }
		#endif
		}

		#ifdef MACFILTER_MODE_SUPPORT
		apmib_get(MIB_MACFILTER_MODE, (void *)&mode);
		macEntry.filterMode = mode;
		#endif
		//add same mac address check
		apmib_get(MIB_MACFILTER_TBL_NUM, (void *)&entryNum);
		for(j=1;j<=entryNum;j++)
		{
			memset(&macEntrytmp, 0x00, sizeof(macEntrytmp));
			*((char *)&macEntrytmp) = (char)j;
			if ( apmib_get(MIB_MACFILTER_TBL, (void *)&macEntrytmp))
			{
				if (!memcmp(macEntrytmp.macAddr, macEntry.macAddr, 6) 
            #ifdef DST_MAC_FILTER
				&& !memcmp(macEntrytmp.dmacAddr, macEntry.dmacAddr, 6)
			#endif
			#ifdef MACFILTER_MODE_SUPPORT
				&& (mode == macEntrytmp.filterMode)
			#endif
					)
				{
					strcpy(tmpBuf, ("rule already exist!"));
					goto setErr_filter;
				}
					
			}
		}
	}

	if (strAddUrl[0]) {
		strUrlMode = req_get_cstream_var(wp, "urlFilterMode", "");
		if(strUrlMode){
			mode=atoi(strUrlMode);
			if ( apmib_set(MIB_URLFILTER_MODE, (void *)&mode) == 0) {
		    	strcpy(tmpBuf, ("Set mode flag error!"));
			    goto setErr_filter;
		    }
        }
		strVal = req_get_cstream_var(wp, "url", "");
		if (!strVal[0])// && !strComment[0])
			goto setOk_filter;

		if ( !strVal[0] ) {
			strcpy(tmpBuf, ("Error! No url keyword to set."));
			goto setErr_filter;
		}
		else
		{
			for(j=0; strVal[j]!='\0'; j++)
				strVal[j] = tolower(strVal[j]);
			
			strcpy((char *)urlEntry.urlAddr, strVal);
			urlEntry.ruleMode=mode;
		}
		
#ifdef URL_FILTER_USER_MODE_SUPPORT
				strUsrMode = req_get_cstream_var(wp,"urlFilterUserMode", "");
				if(strUsrMode){
					usrMode=atoi(strUsrMode);
				}
				urlEntry.usrMode=(unsigned char)usrMode;
				if(usrMode==1){//ip mode
					strVal = req_get_cstream_var(wp, "ip", "");
					if (strVal[0])
					{
						inet_aton(strVal, (struct in_addr *)&urlEntry.ipAddr);
					}
				}
				else if(usrMode==2)//mac mode
				{
					strVal = req_get_cstream_var(wp, "mac","");
					if(strVal[0])
					{
						if (strlen(strVal)!=12 || !string_to_hex(strVal, urlEntry.macAddr, 12)) {
							strcpy(tmpBuf, ("Error! Invalid MAC address."));
							goto setErr_filter;
						}
					}
				}
#endif

		//add same url rule check
		apmib_get(MIB_URLFILTER_TBL_NUM, (void *)&entryNum);
		for(j=1;j<=entryNum;j++)
		{
			memset(&urlEntrytmp, 0x00, sizeof(urlEntrytmp));
			*((char *)&urlEntrytmp) = (char)j;
			if ( apmib_get(MIB_URLFILTER_TBL, (void *)&urlEntrytmp))
			{
				if(urlEntry.ruleMode == urlEntrytmp.ruleMode)
				if (strlen(urlEntry.urlAddr) == strlen(urlEntrytmp.urlAddr))
				{
					if (!memcmp(urlEntrytmp.urlAddr, urlEntry.urlAddr, strlen(urlEntry.urlAddr)))
					{
#ifdef URL_FILTER_USER_MODE_SUPPORT
						//printf("%s:%d=======================user mode=%d\n",__FUNCTION__,__LINE__,urlEntry.usrMode);
					    if((urlEntry.usrMode == urlEntrytmp.usrMode)&&
							(((urlEntry.usrMode==1)&&(!memcmp(urlEntrytmp.ipAddr, urlEntry.ipAddr, strlen(urlEntry.ipAddr))))
							||((urlEntry.usrMode==2)&&(!memcmp(urlEntrytmp.macAddr, urlEntry.macAddr, strlen(urlEntry.macAddr))))))
					    {
#endif
						strcpy(tmpBuf, ("rule already exist!"));
						goto setErr_filter;
#ifdef URL_FILTER_USER_MODE_SUPPORT
						}
#endif
					}
				}
			}
		}

	}
	if (strAddPort[0]) {
		apmib_get(MIB_PORTFILTER_TBL_NUM, (void *)&entryNum);
		for(j=1;j<=entryNum;j++)
		{
			memset(&entrytmp, 0x00, sizeof(entrytmp));
			*((char *)&entrytmp) = (char)j;
			if ( apmib_get(MIB_PORTFILTER_TBL, (void *)&entrytmp))
			{
				if ((entrytmp.fromPort == portEntry.fromPort) &&
					(entrytmp.toPort == portEntry.toPort)&&
					((entrytmp.protoType == portEntry.protoType)||
					((entrytmp.protoType==PROTO_BOTH)&&portEntry.protoType==PROTO_UDP)||
					((entrytmp.protoType==PROTO_BOTH)&&portEntry.protoType==PROTO_TCP)||
					((entrytmp.protoType==PROTO_TCP)&&portEntry.protoType==PROTO_BOTH)||
					((entrytmp.protoType==PROTO_UDP)&&portEntry.protoType==PROTO_BOTH)))
					{
						strcpy(tmpBuf, ("rule already exist!"));
						goto setErr_filter;
					}
					if ((((entrytmp.fromPort <= portEntry.fromPort) &&
					(entrytmp.toPort >= portEntry.fromPort))||
					((entrytmp.fromPort <= portEntry.toPort) &&
					(entrytmp.toPort >= portEntry.toPort)))&&
					((entrytmp.protoType == portEntry.protoType)||
					((entrytmp.protoType==PROTO_BOTH)&&portEntry.protoType==PROTO_UDP)||
					((entrytmp.protoType==PROTO_BOTH)&&portEntry.protoType==PROTO_TCP)||
					((entrytmp.protoType==PROTO_TCP)&&portEntry.protoType==PROTO_BOTH)||
					((entrytmp.protoType==PROTO_UDP)&&portEntry.protoType==PROTO_BOTH)))
					{
						strcpy(tmpBuf, ("port overlap!"));
						goto setErr_filter;
					}
					if ((((entrytmp.fromPort >= portEntry.fromPort) &&
					(entrytmp.fromPort <= portEntry.toPort))||
					((entrytmp.toPort >= portEntry.fromPort) &&
					(entrytmp.toPort <= portEntry.toPort)))&&
					((entrytmp.protoType == portEntry.protoType)||
					((entrytmp.protoType==PROTO_BOTH)&&portEntry.protoType==PROTO_UDP)||
					((entrytmp.protoType==PROTO_BOTH)&&portEntry.protoType==PROTO_TCP)||
					((entrytmp.protoType==PROTO_TCP)&&portEntry.protoType==PROTO_BOTH)||
					((entrytmp.protoType==PROTO_UDP)&&portEntry.protoType==PROTO_BOTH)))
					{
						strcpy(tmpBuf, ("port overlap!"));
						goto setErr_filter;
					}
			}
		}
	}
	
	if (strAddIp[0]) {
		apmib_get(MIB_IPFILTER_TBL_NUM, (void *)&entryNum);
		for(j=1;j<=entryNum;j++)
		{
			fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);
			memset(&ipentrytmp, 0x00, sizeof(ipentrytmp));
			*((char *)&ipentrytmp) = (char)j;
			if ( apmib_get(MIB_IPFILTER_TBL, (void *)&ipentrytmp))
			{
			#ifdef RTL_IPFILTER_SUPPORT_IP_RANGE		
			#ifdef CONFIG_IPV6
				if(ip4Enable == 1){
			#endif
			#ifdef IP_RANGE_MULTI_WAN_SUPPORT
			if(ipEntry.protoType != PROTO_ICMP){
			#endif
					if (strEndIpAddr[0])
					{
						if (((*((unsigned int*)ipentrytmp.ipAddr)) == (*((unsigned int*)ipEntry.ipAddr)))&&
							((*((unsigned int*)ipentrytmp.ipAddrEnd))==(*((unsigned int*)ipEntry.ipAddrEnd)))&&
							#ifdef IP_RANGE_MULTI_WAN_SUPPORT
							((*((unsigned int*)ipentrytmp.dstipAddr)) == (*((unsigned int*)ipEntry.dstipAddr)))&&
							((*((unsigned int*)ipentrytmp.dstipAddrEnd)) == (*((unsigned int*)ipEntry.dstipAddrEnd)))&&
							(ipentrytmp.WANIfIndex==ipEntry.WANIfIndex)&&
							#endif
							((ipentrytmp.protoType==ipEntry.protoType)||
							(ipentrytmp.protoType==PROTO_BOTH&&ipEntry.protoType==PROTO_TCP)||
							(ipentrytmp.protoType==PROTO_BOTH&&ipEntry.protoType==PROTO_UDP)||
							(ipentrytmp.protoType==PROTO_TCP&&ipEntry.protoType==PROTO_BOTH)||
							(ipentrytmp.protoType==PROTO_UDP&&ipEntry.protoType==PROTO_BOTH))
							#ifdef CONFIG_IPV6
							&&(ip4Enable==1)&&(ipentrytmp.ipVer == IPv4)
							#endif
							)
						{
							strcpy(tmpBuf, ("rule already exist!"));
							goto setErr_filter;
						}
						if (((((*((unsigned int*)ipentrytmp.ipAddrEnd))>=(*((unsigned int*)ipEntry.ipAddrEnd)))&&
							((*((unsigned int*)ipentrytmp.ipAddr))<=(*((unsigned int*)ipEntry.ipAddrEnd))))||
							(((*((unsigned int*)ipentrytmp.ipAddrEnd))>=(*((unsigned int*)ipEntry.ipAddr)))&&
							((*((unsigned int*)ipentrytmp.ipAddr))<=(*((unsigned int*)ipEntry.ipAddr)))))
							#ifdef IP_RANGE_MULTI_WAN_SUPPORT
							||((((*((unsigned int*)ipentrytmp.dstipAddrEnd))>=(*((unsigned int*)ipEntry.dstipAddrEnd)))&&
							((*((unsigned int*)ipentrytmp.dstipAddr))<=(*((unsigned int*)ipEntry.dstipAddrEnd))))||
							(((*((unsigned int*)ipentrytmp.dstipAddrEnd))>=(*((unsigned int*)ipEntry.dstipAddr)))&&
							((*((unsigned int*)ipentrytmp.dstipAddr))<=(*((unsigned int*)ipEntry.dstipAddr)))))
							#endif
							&&
							#ifdef IP_RANGE_MULTI_WAN_SUPPORT
							(ipentrytmp.WANIfIndex==ipEntry.WANIfIndex)&&
							#endif
							((ipentrytmp.protoType==ipEntry.protoType)||
							(ipentrytmp.protoType==PROTO_BOTH&&ipEntry.protoType==PROTO_TCP)||
							(ipentrytmp.protoType==PROTO_BOTH&&ipEntry.protoType==PROTO_UDP)||
							(ipentrytmp.protoType==PROTO_TCP&&ipEntry.protoType==PROTO_BOTH)||
							(ipentrytmp.protoType==PROTO_UDP&&ipEntry.protoType==PROTO_BOTH))
							#ifdef CONFIG_IPV6
							&&(ip4Enable==1)&&(ipentrytmp.ipVer == IPv4)
							#endif
							)
						{
							strcpy(tmpBuf, ("ip address overlap!"));
							goto setErr_filter;
						}
						if (((((*((unsigned int*)ipEntry.ipAddrEnd))>=(*((unsigned int*)ipentrytmp.ipAddrEnd)))&&
						((*((unsigned int*)ipEntry.ipAddr))<=(*((unsigned int*)ipentrytmp.ipAddrEnd))))||
						(((*((unsigned int*)ipEntry.ipAddrEnd))>=(*((unsigned int*)ipentrytmp.ipAddr)))&&
						((*((unsigned int*)ipEntry.ipAddr))<=(*((unsigned int*)ipentrytmp.ipAddr)))))
						#ifdef IP_RANGE_MULTI_WAN_SUPPORT
						||((((*((unsigned int*)ipEntry.dstipAddrEnd))>=(*((unsigned int*)ipentrytmp.dstipAddrEnd)))&&
						((*((unsigned int*)ipEntry.dstipAddr))<=(*((unsigned int*)ipentrytmp.dstipAddrEnd))))||
						(((*((unsigned int*)ipEntry.dstipAddrEnd))>=(*((unsigned int*)ipentrytmp.dstipAddr)))&&
						((*((unsigned int*)ipEntry.dstipAddr))<=(*((unsigned int*)ipentrytmp.dstipAddr)))))
						#endif
						&&
						#ifdef IP_RANGE_MULTI_WAN_SUPPORT
						(ipentrytmp.WANIfIndex==ipEntry.WANIfIndex)&&
						#endif
						((ipentrytmp.protoType==ipEntry.protoType)||
						(ipentrytmp.protoType==PROTO_BOTH&&ipEntry.protoType==PROTO_TCP)||
						(ipentrytmp.protoType==PROTO_BOTH&&ipEntry.protoType==PROTO_UDP)||
						(ipentrytmp.protoType==PROTO_TCP&&ipEntry.protoType==PROTO_BOTH)||
						(ipentrytmp.protoType==PROTO_UDP&&ipEntry.protoType==PROTO_BOTH))
						#ifdef CONFIG_IPV6
						&&(ip4Enable==1)&&(ipentrytmp.ipVer == IPv4)
						#endif
						)
						{
							strcpy(tmpBuf, ("ip address overlap!"));
							goto setErr_filter;
						}
					}
					else
					{
						if ((((*((unsigned int*)ipentrytmp.ipAddrEnd))>=(*((unsigned int*)ipEntry.ipAddr)))&&
							((*((unsigned int*)ipentrytmp.ipAddr))<=(*((unsigned int*)ipEntry.ipAddr))))||
							(((*((unsigned int*)ipentrytmp.ipAddrEnd))==(*((unsigned int*)ipEntry.ipAddr)))||
							((*((unsigned int*)ipentrytmp.ipAddr))==(*((unsigned int*)ipEntry.ipAddr))))&&
							#ifdef IP_RANGE_MULTI_WAN_SUPPORT
							(((*((unsigned int*)ipentrytmp.dstipAddrEnd))>=(*((unsigned int*)ipEntry.dstipAddr)))&&
							((*((unsigned int*)ipentrytmp.dstipAddr))<=(*((unsigned int*)ipEntry.dstipAddr))))||
							(((*((unsigned int*)ipentrytmp.dstipAddrEnd))==(*((unsigned int*)ipEntry.dstipAddr)))||
							((*((unsigned int*)ipentrytmp.dstipAddr))==(*((unsigned int*)ipEntry.dstipAddr))))&&
							(ipentrytmp.WANIfIndex==ipEntry.WANIfIndex)&&
							#endif
							((ipentrytmp.protoType==ipEntry.protoType)||
							(ipentrytmp.protoType==PROTO_BOTH&&ipEntry.protoType==PROTO_TCP)||
							(ipentrytmp.protoType==PROTO_BOTH&&ipEntry.protoType==PROTO_UDP)||
							(ipentrytmp.protoType==PROTO_TCP&&ipEntry.protoType==PROTO_BOTH)||
							(ipentrytmp.protoType==PROTO_UDP&&ipEntry.protoType==PROTO_BOTH))
							#ifdef CONFIG_IPV6
							&&(ip4Enable==1)&&(ipentrytmp.ipVer == IPv4)
							#endif
							)
						{
							strcpy(tmpBuf, ("ip address overlap!"));
							goto setErr_filter;
						}
					}
				#ifdef IP_RANGE_MULTI_WAN_SUPPORT	
				}
			    #endif
					
				#ifdef CONFIG_IPV6
				}
				else if(ip6Enable == 1){
					//printf("====(%s:%d)======\n",__FILE__,__LINE__);
					if(!strcmp(ipentrytmp.ip6Addr,ipEntry.ip6Addr) &&  
						((ipentrytmp.protoType==ipEntry.protoType)||
                        (ipentrytmp.protoType==PROTO_BOTH&&ipEntry.protoType==PROTO_TCP)||
                        (ipentrytmp.protoType==PROTO_BOTH&&ipEntry.protoType==PROTO_UDP)||
                        (ipentrytmp.protoType==PROTO_TCP&&ipEntry.protoType==PROTO_BOTH)||
                        (ipentrytmp.protoType==PROTO_UDP&&ipEntry.protoType==PROTO_BOTH))
                        &&(ipentrytmp.ipVer == IPv6)){
						strcpy(tmpBuf, ("ipv6 rule already exist!"));
						goto setErr_filter;
					}
				}
				#endif
			#else
				if (((*((unsigned int*)ipentrytmp.ipAddr)) == (*((unsigned int*)ipEntry.ipAddr)))&&
			        #ifdef IP_RANGE_MULTI_WAN_SUPPORT
					((*((unsigned int*)ipentrytmp.ipAddrEnd)) == (*((unsigned int*)ipEntry.ipAddrEnd)))&&
					((*((unsigned int*)ipentrytmp.dstipAddr)) == (*((unsigned int*)ipEntry.dstipAddr)))&&
					((*((unsigned int*)ipentrytmp.dstipAddrEnd)) == (*((unsigned int*)ipEntry.dstipAddrEnd)))&&
					#endif
					((ipentrytmp.protoType==ipEntry.protoType)||
					(ipentrytmp.protoType==PROTO_BOTH&&ipEntry.protoType==PROTO_TCP)||
					(ipentrytmp.protoType==PROTO_BOTH&&ipEntry.protoType==PROTO_UDP)||
					(ipentrytmp.protoType==PROTO_TCP&&ipEntry.protoType==PROTO_BOTH)||
					(ipentrytmp.protoType==PROTO_UDP&&ipEntry.protoType==PROTO_BOTH))
					#ifdef CONFIG_IPV6
					&&(ip4Enable==1)&&(ipentrytmp.ipVer == IPv4)
					#endif
					)
				{
					strcpy(tmpBuf, ("rule already exist!"));
					goto setErr_filter;
				}
					
				#ifdef CONFIG_IPV6
				 else if (!strcmp(ipentrytmp.ip6Addr,ipEntry.ip6Addr)&&
                    ((ipentrytmp.protoType==ipEntry.protoType)||
                    (ipentrytmp.protoType==PROTO_BOTH&&ipEntry.protoType==PROTO_TCP)||
                    (ipentrytmp.protoType==PROTO_BOTH&&ipEntry.protoType==PROTO_UDP)||
                    (ipentrytmp.protoType==PROTO_TCP&&ipEntry.protoType==PROTO_BOTH)||
                    (ipentrytmp.protoType==PROTO_UDP&&ipEntry.protoType==PROTO_BOTH))
					&&(ip6Enable==1)&&(ipentrytmp.ipVer == IPv6))
                    {
                        strcpy(tmpBuf, ("ipv6 rule already exist!"));
                        goto setErr_filter;
                    }
				#endif
			#endif
			}
		}
	}

	if (strAddIp[0] || strAddPort[0] || strAddMac[0] || strAddUrl[0]) {
		if ( strComment[0] ) {
			if (strlen(strComment) > COMMENT_LEN-1) {
				strcpy(tmpBuf, ("Error! Comment length too long."));
				goto setErr_filter;
			}
			if (strAddIp[0])
				strcpy((char *)ipEntry.comment, strComment);
			else if (strAddPort[0])
				strcpy((char *)portEntry.comment, strComment);
			else if (strAddMac[0])
				strcpy((char *)macEntry.comment, strComment);
		}

		if ( !apmib_get(num_id, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get entry number error!"));
			goto setErr_filter;
		}
		if (strAddUrl[0])
		{
			if ( (entryNum + 1) > MAX_URLFILTER_NUM) {
				strcpy(tmpBuf, ("Cannot add new URL entry because table is full!"));
				goto setErr_filter;
			}
		}
		else
		{
			if ( (entryNum + 1) > MAX_FILTER_NUM) {
				strcpy(tmpBuf, ("Cannot add new entry because table is full!"));
				goto setErr_filter;
			}
		}
		    

		#ifdef IP_RANGE_MULTI_WAN_SUPPORT
		if(strAddIp[0])
		{
		  for(i = 1; i<= WANIFACE_NUM; i++){
		  	if( (ipfWanMask&(1 << (i-1))) == 0)
			  continue;
			  ipEntry.WANIfIndex = i;		
			 
              apmib_set(del_id, pEntry);
		      if ( apmib_set(add_id, pEntry) == 0) {
			     strcpy(tmpBuf, ("Add table entry error!"));
			     goto setErr_filter;
			  }
		  }
		}
		else
		#endif
		{
		// set to MIB. try to delete it first to avoid duplicate case
		apmib_set(del_id, pEntry);
		if ( apmib_set(add_id, pEntry) == 0) {
			strcpy(tmpBuf, ("Add table entry error!"));
			goto setErr_filter;
		   }
		}
	}


	/* Delete entry */
	if (strDelPort[0] || strDelIp[0] || strDelMac[0] || strDelUrl[0]) {
		if ( !apmib_get(num_id, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get entry number error!"));
			goto setErr_filter;
		}
		for (i=entryNum; i>0; i--) {
			snprintf(tmpBuf, 20, "select%d", i);

			strVal = req_get_cstream_var(wp, tmpBuf, "");
			if ( !strcmp(strVal, "ON") ) {

				*((char *)pEntry) = (char)i;
				if ( !apmib_get(get_id, pEntry)) {
					strcpy(tmpBuf, ("Get table entry error!"));
					goto setErr_filter;
				}
				if ( !apmib_set(del_id, pEntry)) {
					strcpy(tmpBuf, ("Delete table entry error!"));
					goto setErr_filter;
				}
			}
		}
	}

	/* Delete all entry */
	if ( strDelAllPort[0] || strDelAllIp[0] || strDelAllMac[0] || strDelAllUrl[0]) {
		if ( !apmib_set(delall_id, pEntry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_filter;
		}
	}
setOk_filter:
	apmib_update_web(CURRENT_SETTING);
#if !defined(MULTI_WAN_SUPPORT)
#ifndef NO_ACTION
	pid = fork();
        if (pid) {
	      	waitpid(pid, NULL, 0);
	}
        else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
               	exit(1);
        }
#ifdef CONFIG_IPV6
	pid = fork();
		if (pid) {
			waitpid(pid, NULL, 0);
	}
		else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _IPV6_FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _IPV6_FIREWALL_SCRIPT_PROG, NULL);
				exit(1);
		}
#endif
#endif
#endif

	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	#if defined(MULTI_WAN_SUPPORT)
	OK_MSG(submitUrl);
	return;
	#else
#ifdef REBOOT_CHECK
	if(needReboot == 1)
	{
		OK_MSG(submitUrl);
		return;
	}
#endif

	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
  	return;
	#endif
setErr_filter:
	ERR_MSG(tmpBuf);
}

#if 0
/////////////////////////////////////////////////////////////////////////////
void formTriggerPort(request *wp, char *path, char *query)
{
	char *strAddPort, *strDelAllPort, *strDelPort, *strVal, *submitUrl;
	char *strTriFrom, *strTriTo, *strIncFrom, *strIncTo, *strComment;
	char tmpBuf[100];
	int entryNum, intVal, i;
	TRIGGERPORT_T entry;

	memset(&entry, '\0', sizeof(entry));

	/* Add port filter */
	strAddPort = req_get_cstream_var(wp, ("addPort"), "");
	if (strAddPort[0]) {
		strVal = req_get_cstream_var(wp, ("enabled"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;

		if ( apmib_set(MIB_TRIGGERPORT_ENABLED, (void *)&intVal) == 0) {
			strcpy(tmpBuf, ("Set enabled flag error!"));
			goto setErr_triPort;
		}

		strTriFrom = req_get_cstream_var(wp, ("triFromPort"), "");
		strTriTo = req_get_cstream_var(wp, ("triToPort"), "");
		strIncFrom = req_get_cstream_var(wp, ("incFromPort"), "");
		strIncTo = req_get_cstream_var(wp, ("incToPort"), "");
		strComment = req_get_cstream_var(wp, ("comment"), "");

		if (!strTriFrom[0] && !strTriTo[0] && !strIncFrom[0] &&
					!strIncTo[0] && !strComment[0])
			goto setOk_triPort;

		// get trigger port range and protocol
		if (!strTriFrom[0]) { // from port must exist
			strcpy(tmpBuf, ("Error! No from-port value to be set."));
			goto setErr_triPort;
		}
		if ( !string_to_dec(strTriFrom, &intVal) || intVal<1 || intVal>65535) {
			strcpy(tmpBuf, ("Error! Invalid value of trigger from-port."));
			goto setErr_triPort;
		}
		entry.tri_fromPort = (unsigned short)intVal;

		if ( !strTriTo[0] )
			entry.tri_toPort = entry.tri_fromPort;
		else {
			if ( !string_to_dec(strTriTo, &intVal) || intVal<1 || intVal>65535) {
				strcpy(tmpBuf, ("Error! Invalid value of trigger to-port."));
				goto setErr_triPort;
			}
			entry.tri_toPort = (unsigned short)intVal;
		}

		if ( entry.tri_fromPort  > entry.tri_toPort ) {
			strcpy(tmpBuf, ("Error! Invalid trigger port range."));
			goto setErr_triPort;
		}

		strVal = req_get_cstream_var(wp, ("triProtocol"), "");
		if (strVal[0]) {
			if ( strVal[0] == '0' ) {
				if (strAddPort[0])
					entry.tri_protoType = PROTO_BOTH;
				else
					entry.tri_protoType = PROTO_BOTH;
			}
			else if ( strVal[0] == '1' ) {
				if (strAddPort[0])
					entry.tri_protoType = PROTO_TCP;
				else
					entry.tri_protoType = PROTO_TCP;
			}
			else if ( strVal[0] == '2' ) {
				if (strAddPort[0])
					entry.tri_protoType = PROTO_UDP;
				else
					entry.tri_protoType = PROTO_UDP;
			}
			else {
				strcpy(tmpBuf, ("Error! Invalid trigger-port protocol type."));
				goto setErr_triPort;
			}
		}
		else {
			strcpy(tmpBuf, ("Error! trigger-port protocol type cannot be empty."));
			goto setErr_triPort;
		}

		// get incoming port range and protocol
		if (!strIncFrom[0]) { // from port must exist
			strcpy(tmpBuf, ("Error! No from-port value to be set."));
			goto setErr_triPort;
		}
		if ( !string_to_dec(strIncFrom, &intVal) || intVal<1 || intVal>65535) {
			strcpy(tmpBuf, ("Error! Invalid value of incoming from-port."));
			goto setErr_triPort;
		}
		entry.inc_fromPort = (unsigned short)intVal;

		if ( !strIncTo[0] )
			entry.inc_toPort = entry.inc_fromPort;
		else {
			if ( !string_to_dec(strIncTo, &intVal) || intVal<1 || intVal>65535) {
				strcpy(tmpBuf, ("Error! Invalid value of incoming to-port."));
				goto setErr_triPort;
			}
			entry.inc_toPort = (unsigned short)intVal;
		}

		if ( entry.inc_fromPort  > entry.inc_toPort ) {
			strcpy(tmpBuf, ("Error! Invalid incoming port range."));
			goto setErr_triPort;
		}


		strVal = req_get_cstream_var(wp, ("incProtocol"), "");
		if (strVal[0]) {
			if ( strVal[0] == '0' ) {
				if (strAddPort[0])
					entry.inc_protoType = PROTO_BOTH;
				else
					entry.inc_protoType = PROTO_BOTH;
			}
			else if ( strVal[0] == '1' ) {
				if (strAddPort[0])
					entry.inc_protoType = PROTO_TCP;
				else
					entry.inc_protoType = PROTO_TCP;
			}
			else if ( strVal[0] == '2' ) {
				if (strAddPort[0])
					entry.inc_protoType = PROTO_UDP;
				else
					entry.inc_protoType = PROTO_UDP;
			}
			else {
				strcpy(tmpBuf, ("Error! Invalid incoming-port protocol type."));
				goto setErr_triPort;
			}
		}
		else {
			strcpy(tmpBuf, ("Error! incoming-port protocol type cannot be empty."));
			goto setErr_triPort;
		}

		// get comment
		if ( strComment[0] ) {
			if (strlen(strComment) > COMMENT_LEN-1) {
				strcpy(tmpBuf, ("Error! Comment length too long."));
				goto setErr_triPort;
			}
			strcpy(entry.comment, strComment);
		}

		// get entry number to see if it exceeds max
		if ( !apmib_get(MIB_TRIGGERPORT_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get entry number error!"));
			goto setErr_triPort;
		}
		if ( (entryNum + 1) > MAX_FILTER_NUM) {
			strcpy(tmpBuf, ("Cannot add new entry because table is full!"));
			goto setErr_triPort;
		}

		// Check if there is any port overlapped
		for (i=1; i<=entryNum; i++) {
			TRIGGERPORT_T checkEntry;
			*((char *)&checkEntry) = (char)i;
			if ( !apmib_get(MIB_TRIGGERPORT_TBL, (void *)&checkEntry)) {
				strcpy(tmpBuf, ("Get table entry error!"));
				goto setErr_triPort;
			}
			if ( ( (entry.tri_fromPort <= checkEntry.tri_fromPort &&
					entry.tri_toPort >= checkEntry.tri_fromPort) ||
			       (entry.tri_fromPort >= checkEntry.tri_fromPort &&
				entry.tri_fromPort <= checkEntry.tri_toPort)
			     )&&
			       (entry.tri_protoType & checkEntry.tri_protoType) ) {
				strcpy(tmpBuf, ("Trigger port range has overlapped with used port numbers!"));
				goto setErr_triPort;
			}
		}

		// set to MIB. try to delete it first to avoid duplicate case
		apmib_set(MIB_TRIGGERPORT_DEL, (void *)&entry);
		if ( apmib_set(MIB_TRIGGERPORT_ADD, (void *)&entry) == 0) {
			strcpy(tmpBuf, ("Add table entry error!"));
			goto setErr_triPort;
		}
	}

	/* Delete entry */
	strDelPort = req_get_cstream_var(wp, ("deleteSelPort"), "");
	if (strDelPort[0]) {
		if ( !apmib_get(MIB_TRIGGERPORT_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get entry number error!"));
			goto setErr_triPort;
		}
		for (i=entryNum; i>0; i--) {
			snprintf(tmpBuf, 20, "select%d", i);

			strVal = req_get_cstream_var(wp, tmpBuf, "");
			if ( !strcmp(strVal, "ON") ) {

				*((char *)&entry) = (char)i;
				if ( !apmib_get(MIB_TRIGGERPORT_TBL, (void *)&entry)) {
					strcpy(tmpBuf, ("Get table entry error!"));
					goto setErr_triPort;
				}
				if ( !apmib_set(MIB_TRIGGERPORT_DEL, (void *)&entry)) {
					strcpy(tmpBuf, ("Delete table entry error!"));
					goto setErr_triPort;
				}
			}
		}
	}

	/* Delete all entry */
	strDelAllPort = req_get_cstream_var(wp, ("deleteAllPort"), "");
	if ( strDelAllPort[0]) {
		if ( !apmib_set(MIB_TRIGGERPORT_DELALL, (void *)&entry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_triPort;
		}
	}

setOk_triPort:
	apmib_update_web(CURRENT_SETTING);

	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
  	return;

setErr_triPort:
	ERR_MSG(tmpBuf);
}
#endif

#if defined(CONFIG_RTL_ACCESS_INTERNET_USER_NUM_LIMIT)
void formUserLimit(request *wp, char *path, char *query)
{
	char *submitUrl, *strSave, *strVal;
	char tmpBuf[100];
	int intVal;

	strSave = req_get_cstream_var(wp, ("save"), "");
#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strSave[0]==0){
		strSave = req_get_cstream_var(wp, ("save_apply"), "");
	}
#endif
	if (strSave[0]) {
		strVal = req_get_cstream_var(wp, ("enabled"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;

		if ( apmib_set(MIB_LIMIT_USER_ENABLE, (void *)&intVal) == 0) {
			strcpy(tmpBuf, ("Set enabled flag error!"));
			goto setErr_user_limit;
		}

		strVal = req_get_cstream_var(wp, ("num"), "");
		if (!strVal[0]) {
			intVal = 4;
		} else {
			intVal = atoi(strVal);
		}
		
		if ( apmib_set(MIB_LIMIT_USER_NUM, (void *)&intVal) == 0) {
			strcpy(tmpBuf, ("Set DMZ MIB error!"));
			goto setErr_user_limit;
		}
	}

setOk_user_limit:
	apmib_update_web(CURRENT_SETTING);

#if !defined(MULTI_WAN_SUPPORT)
#ifndef NO_ACTION
	pid = fork();
        if (pid) {
	      	waitpid(pid, NULL, 0);
	}
        else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
               	exit(1);
        }
#endif
#endif

	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
#if defined(MULTI_WAN_SUPPORT)
	OK_MSG(submitUrl);
	return ;
#else
#ifdef REBOOT_CHECK
	if(needReboot == 1)
	{
		OK_MSG(submitUrl);
		return;
	}
#endif

	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
  	return;
#endif
setErr_user_limit:
	ERR_MSG(tmpBuf);
}
#endif

#if defined(CONFIG_RTK_VLAN_WAN_TAG_SUPPORT)
void formVlanWAN(request *wp, char *path, char *query)
{
	VLAN_CONFIG_T entry;
	char *submitUrl,*strTmp;
	int	value;
	struct nameMapping *mapping;
	char tmpBuf[100];
	
	value = !strcmp(req_get_cstream_var(wp, ("vlan_wan_enable"), ("")), "on");
	if (!apmib_set(MIB_VLAN_WAN_ENALE, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLANCONFIG_ENABLED error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_tag"), ("0")));
	if(strcmp(req_get_cstream_var(wp, ("vlan_wan_enable"), ("")), "on"))
		value = 0;

	if (!apmib_set(MIB_VLAN_WAN_TAG, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_TAG error!"));
		goto setErr;
	}

	value = !strcmp(req_get_cstream_var(wp, ("vlan_wan_host_enable"), ("")), "on");
	if (!apmib_set(MIB_VLAN_WAN_HOST_ENABLE, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_HOST_ENALE error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_host_tag"), ("0")));
	if (!apmib_set(MIB_VLAN_WAN_HOST_TAG, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_HOST_TAG error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_host_pri"), ("0")));
	if(strcmp(req_get_cstream_var(wp, ("vlan_wan_enable"), ("")), "on"))
		value = 0;
	if (!apmib_set(MIB_VLAN_WAN_HOST_PRI, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_HOST_PRI error!"));
		goto setErr;
	}

	value = !strcmp(req_get_cstream_var(wp, ("vlan_wan_wifi_root_enable"), ("")), "on");
	if (!apmib_set(MIB_VLAN_WAN_WIFI_ROOT_ENABLE, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_ROOT_ENALE error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_wifi_root_tag"), ("0")));
	if (!apmib_set(MIB_VLAN_WAN_WIFI_ROOT_TAG, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_ROOT_TAG error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_wifi_root_pri"), ("0")));
	if (!apmib_set(MIB_VLAN_WAN_WIFI_ROOT_PRI, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_ROOT_PRI error!"));
		goto setErr;
	}
	
	value = !strcmp(req_get_cstream_var(wp, ("vlan_wan_wifi_vap0_enable"), ("")), "on");
	if (!apmib_set(MIB_VLAN_WAN_WIFI_VAP0_ENABLE, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_VAP0_ENALE error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_wifi_vap0_tag"), ("0")));
	if (!apmib_set(MIB_VLAN_WAN_WIFI_VAP0_TAG, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_VAP0_TAG error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_wifi_vap0_pri"), ("0")));
	if (!apmib_set(MIB_VLAN_WAN_WIFI_VAP0_PRI, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_VAP0_PRI error!"));
		goto setErr;
	}

	value = !strcmp(req_get_cstream_var(wp, ("vlan_wan_wifi_vap1_enable"), ("")), "on");
	if (!apmib_set(MIB_VLAN_WAN_WIFI_VAP1_ENABLE, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_VAP1_ENALE error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_wifi_vap1_tag"), ("0")));
	if (!apmib_set(MIB_VLAN_WAN_WIFI_VAP1_TAG, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_VAP1_TAG error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_wifi_vap1_pri"), ("0")));
	if (!apmib_set(MIB_VLAN_WAN_WIFI_VAP1_PRI, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_VAP1_PRI error!"));
		goto setErr;
	}

	value = !strcmp(req_get_cstream_var(wp, ("vlan_wan_wifi_vap2_enable"), ("")), "on");
	if (!apmib_set(MIB_VLAN_WAN_WIFI_VAP2_ENABLE, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_VAP2_ENALE error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_wifi_vap2_tag"), ("0")));
	if (!apmib_set(MIB_VLAN_WAN_WIFI_VAP2_TAG, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_VAP0_TAG error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_wifi_vap2_pri"), ("0")));
	if (!apmib_set(MIB_VLAN_WAN_WIFI_VAP2_PRI, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_VAP2_PRI error!"));
		goto setErr;
	}

	value = !strcmp(req_get_cstream_var(wp, ("vlan_wan_wifi_vap3_enable"), ("")), "on");
	if (!apmib_set(MIB_VLAN_WAN_WIFI_VAP3_ENABLE, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_VAP3_ENALE error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_wifi_vap3_tag"), ("0")));
	if (!apmib_set(MIB_VLAN_WAN_WIFI_VAP3_TAG, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_VAP3_TAG error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_wifi_vap3_pri"), ("0")));
	if (!apmib_set(MIB_VLAN_WAN_WIFI_VAP3_PRI, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_WIFI_VAP3_PRI error!"));
		goto setErr;
	}


	value = !strcmp(req_get_cstream_var(wp, ("vlan_wan_bridge_enable"), ("")), "on");
	if (!apmib_set(MIB_VLAN_WAN_BRIDGE_ENABLE, (void *)&value))
	{
		strcpy(tmpBuf, ("set  VLAN_WAN_BRIDGE_ENABLE error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_bridge_tag"), ("0")));
	if(strcmp(req_get_cstream_var(wp, ("vlan_wan_bridge_enable"), ("")), "on"))
		value = 0;

	if (!apmib_set(MIB_VLAN_WAN_BRIDGE_TAG, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_BRIDGE_TAG error!"));
		goto setErr;
	}
	value = !strcmp(req_get_cstream_var(wp, ("vlan_wan_bridge_multicast_enable"), ("")), "on");
	
	if (!apmib_set(MIB_VLAN_WAN_BRIDGE_MULTICAST_ENABLE, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_BRIDGE_MULTICAST_ENABLE error!"));
		goto setErr;
	}
	value =  atoi(req_get_cstream_var(wp, ("vlan_wan_bridge_multicast_tag"), ("0")));
	if(strcmp(req_get_cstream_var(wp, ("vlan_wan_bridge_multicast_enable"), ("")), "on"))
		value = 0;

	if (!apmib_set(MIB_VLAN_WAN_BRIDGE_MULTICAST_TAG, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_BRIDGE_MULTICAST_TAG error!"));
		goto setErr;
	}
	value = 0;
	value |= (!strcmp(req_get_cstream_var(wp, ("vlan_wan_bridge_port_0"), ("")), "on"))<<3;
	value |= (!strcmp(req_get_cstream_var(wp, ("vlan_wan_bridge_port_1"), ("")), "on"))<<2;
	value |= (!strcmp(req_get_cstream_var(wp, ("vlan_wan_bridge_port_2"), ("")), "on"))<<1;
	value |= (!strcmp(req_get_cstream_var(wp, ("vlan_wan_bridge_port_3"), ("")), "on"))<<0;
	value |= (!strcmp(req_get_cstream_var(wp, ("vlan_wan_bridge_port_wifi_root"), ("")), "on"))<<6;
	value |= (!strcmp(req_get_cstream_var(wp, ("vlan_wan_bridge_port_wifi_vap0"), ("")), "on"))<<7;
	value |= (!strcmp(req_get_cstream_var(wp, ("vlan_wan_bridge_port_wifi_vap1"), ("")), "on"))<<8;
	value |= (!strcmp(req_get_cstream_var(wp, ("vlan_wan_bridge_port_wifi_vap2"), ("")), "on"))<<9;
	value |= (!strcmp(req_get_cstream_var(wp, ("vlan_wan_bridge_port_wifi_vap3"), ("")), "on"))<<10;
	if (!apmib_set(MIB_VLAN_WAN_BRIDGE_PORT, (void *)&value))
	{
		strcpy(tmpBuf, ("set  MIB_VLAN_WAN_BRIDGEPORT error!"));
		goto setErr;
	}


	apmib_update_web(CURRENT_SETTING);

	#ifndef NO_ACTION
		run_init_script("all");
	#endif

	OK_MSG("/vlan_wan.htm");
	return;

	setErr:
	ERR_MSG(tmpBuf);

	return;
	
}
#endif

/////////////////////////////////////////////////////////////////////////////
void formDMZ(request *wp, char *path, char *query)
{
	char *submitUrl, *strSave, *strVal;
	char tmpBuf[100];
	int intVal;
	struct in_addr ipAddr, curIpAddr, curSubnet;
	unsigned long v1, v2, v3;
#ifndef NO_ACTION
	int pid;
#endif

	strSave = req_get_cstream_var(wp, ("save"), "");
#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strSave[0]==0){
		strSave = req_get_cstream_var(wp, ("save_apply"), "");
	}
#endif
	if (strSave[0]) {
		strVal = req_get_cstream_var(wp, ("enabled"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;

		if ( apmib_set(MIB_DMZ_ENABLED, (void *)&intVal) == 0) {
			strcpy(tmpBuf, ("Set enabled flag error!"));
			goto setErr_dmz;
		}

		strVal = req_get_cstream_var(wp, ("ip"), "");
		if (!strVal[0]) {
			goto setOk_dmz;
		}
		inet_aton(strVal, &ipAddr);
		getInAddr(BRIDGE_IF, IP_ADDR, (void *)&curIpAddr);
		getInAddr(BRIDGE_IF, SUBNET_MASK, (void *)&curSubnet);

		v1 = *((unsigned long *)&ipAddr);
		v2 = *((unsigned long *)&curIpAddr);
		v3 = *((unsigned long *)&curSubnet);
		if (v1) {
			if ( (v1 & v3) != (v2 & v3) ) {
				strcpy(tmpBuf, "<script>dw(dmz_ip_error)</script>");
				goto setErr_dmz;
			}
			if ( v1 == v2 ) {
				strcpy(tmpBuf, "<script>dw(dmz_ip_error2)</script>");
				goto setErr_dmz;
			}
		}
		if ( apmib_set(MIB_DMZ_HOST, (void *)&ipAddr) == 0) {
			strcpy(tmpBuf, ("Set DMZ MIB error!"));
			goto setErr_dmz;
		}
	}

setOk_dmz:
	apmib_update_web(CURRENT_SETTING);

#if !defined(MULTI_WAN_SUPPORT)
#ifndef NO_ACTION
	pid = fork();
        if (pid) {
	      	waitpid(pid, NULL, 0);
	}
        else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
               	exit(1);
        }
#endif
#endif
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page

	#if defined(MULTI_WAN_SUPPORT)
	OK_MSG(submitUrl);
	return ;
	#else	
#ifdef REBOOT_CHECK
	if(needReboot == 1)
	{
		OK_MSG(submitUrl);
		return;
	}
#endif
	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
  	return;
	#endif
setErr_dmz:
	ERR_MSG(tmpBuf);
}


/////////////////////////////////////////////////////////////////////////////

void pFwTableHeadLine(request *wp){
#ifdef MULTI_WAN_SUPPORT
#if defined(CONFIG_RTL_PORTFW_EXTEND)
	req_format_write(wp, ("<tr class=\"tbl_head\">"
		"<td align=center width=\"15%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_local_ipaddr)</script></b></font></td>\n"
		"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_local_portrange)</script></b></font></td>\n"
		"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_proto)</script></b></font></td>\n"	
		"<td align=center width=\"15%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_remote_ipaddr)</script></b></font></td>\n"
		"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_remote_portrange)</script></b></font></td>\n"	
		"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_wan)</script></b></font></td>\n"
		"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_status)</script></b></font></td>\n"
		"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_remote_comm)</script></b></font></td>\n"
		"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_select)</script></b></font></td></tr>\n"));
#else
	req_format_write(wp, ("<tr class=\"tbl_head\">"
		"<td align=center width=\"25%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_local_ipaddr)</script></b></font></td>\n"
		"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_portrange)</script></b></font></td>\n"
		"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_proto)</script></b></font></td>\n"
		"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_wan)</script></b></font></td>\n"
		"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_remote_comm)</script></b></font></td>\n"
		"<td align=center width=\"15%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_select)</script></b></font></td></tr>\n"));
#endif	
#else //single wan
#if defined(CONFIG_RTL_PORTFW_EXTEND)
	req_format_write(wp, ("<tr class=\"tbl_head\">"
		"<td align=center width=\"15%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_local_ipaddr)</script></b></font></td>\n"
		"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_local_portrange)</script></b></font></td>\n"
		"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_proto)</script></b></font></td>\n"	
		"<td align=center width=\"15%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_remote_ipaddr)</script></b></font></td>\n"
		"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_remote_portrange)</script></b></font></td>\n"	
		"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_status)</script></b></font></td>\n"
		"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_remote_comm)</script></b></font></td>\n"
		"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_select)</script></b></font></td></tr>\n"));
#else
	req_format_write(wp, ("<tr class=\"tbl_head\">"
		"<td align=center width=\"25%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_local_ipaddr)</script></b></font></td>\n"
		"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_portrange)</script></b></font></td>\n"
		"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_proto)</script></b></font></td>\n"
		"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_remote_comm)</script></b></font></td>\n"
		"<td align=center width=\"15%%\" ><font size=\"2\"><b><script>dw(portfw_tbl_select)</script></b></font></td></tr>\n"));
#endif	

#endif

	return;
}

int pFwTableContentList(request *wp, int argc, char **argv)
{
	int nBytesSent=0, entryNum, i;
	PORTFW_T entry;
	char	*type, portRange[20], *ip, wanBuf[20];
#if defined(CONFIG_RTL_PORTFW_EXTEND)
	char extPortRange[20];
	unsigned char *rmtip;
	unsigned char ip_buff[32] = {0}, rmt_ip_buff[32]={0};
#endif
	
	if ( !apmib_get(MIB_PORTFW_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}

	for (i=1; i<=entryNum; i++) {	
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_PORTFW_TBL, (void *)&entry))
			return -1;
		#if defined(CONFIG_RTL_BIND_IP_FOR_PORTFWD)
		#define BIND_IP_FOR_PORTFWD_KEYWORDS	"1195"
		//do not display 1195's rule
		if ((entry.comment[0] != '\0') && !strncmp(entry.comment, BIND_IP_FOR_PORTFWD_KEYWORDS, strlen(BIND_IP_FOR_PORTFWD_KEYWORDS)))
			continue;
		#endif
		
		ip = inet_ntoa(*((struct in_addr *)entry.ipAddr));
		if ( !strcmp(ip, "0.0.0.0"))
			ip = "----";

		if ( entry.protoType == PROTO_BOTH )
			type = "TCP+UDP";
		else if ( entry.protoType == PROTO_TCP )
			type = "TCP";
		else
			type = "UDP";

		if ( entry.fromPort == 0)
			strcpy(portRange, "----");
		else if ( entry.fromPort == entry.toPort )
			snprintf(portRange, 20, "%d", entry.fromPort);
		else
			snprintf(portRange, 20, "%d-%d", entry.fromPort, entry.toPort);
		

#if defined(CONFIG_RTL_PORTFW_EXTEND)
		if ( entry.externelFromPort == 0)
			strcpy(extPortRange, "----");
		else if ( entry.externelFromPort == entry.externelToPort )
			snprintf(extPortRange, 20, "%d", entry.externelFromPort);
		else
			snprintf(extPortRange, 20, "%d-%d", entry.externelFromPort, entry.externelToPort);	

		snprintf(ip_buff, sizeof(ip_buff), "%s", ip);
		rmtip = inet_ntoa(*((struct in_addr *)entry.rmtipAddr));			
		if ( !strcmp(rmtip, "0.0.0.0"))
			rmtip = "ANY";
		
		sprintf(rmt_ip_buff, "%s", rmtip);

#ifdef MULTI_WAN_SUPPORT
		snprintf(wanBuf, sizeof(wanBuf), "WAN%d", entry.WANIfIndex);

		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"15%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" ><font size=\"2\">%s</td>\n"
     			"<td align=center width=\"10%%\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"15%%\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"10%%\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"10%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				ip_buff, portRange, type, rmt_ip_buff, extPortRange, wanBuf, entry.enabled?"Enabled":"Disabled",entry.comment, i);
#else
		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"15%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" ><font size=\"2\">%s</td>\n"
     			"<td align=center width=\"10%%\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"15%%\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"10%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				ip_buff, portRange, type, rmt_ip_buff, extPortRange, entry.enabled?"Enabled":"Disabled", entry.comment, i);
#endif //#ifdef MULTI_WAN_SUPPORT
#else
#ifdef MULTI_WAN_SUPPORT
		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
     			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
     			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"15%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				ip, portRange, type, wanBuf, entry.comment, i);

#else
		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
     			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"15%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				ip, portRange, type, entry.comment, i);
#endif //#ifdef MULTI_WAN_SUPPORT
#endif //#if defined(CONFIG_RTL_PORTFW_EXTEND)
	}
	return nBytesSent;
}


int portFwList(request *wp, int argc, char **argv)
{
	int nBytesSent = 0;
	
	pFwTableHeadLine(wp);
	nBytesSent = pFwTableContentList(wp,argc,argv);

	return nBytesSent;
}

/////////////////////////////////////////////////////////////////////////////
int portFilterList(request *wp, int argc, char **argv)
{
	int	nBytesSent=0, entryNum, i;
	PORTFILTER_T entry;
	char	*type, portRange[20];

	if ( !apmib_get(MIB_PORTFILTER_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}

	nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
      	"<td align=center width=\"30%%\"><font size=\"2\"><b><script>dw(portfiletr_tbl_range)</script></b></font></td>\n"
      	"<td align=center width=\"25%%\"><font size=\"2\"><b><script>dw(portfiletr_tbl_protocol)</script></b></font></td>\n"
	"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(portfiletr_tbl_comm)</script></b></font></td>\n"
      	"<td align=center width=\"15%%\" ><font size=\"2\"><b><script>dw(portfiletr_tbl_select)</script></b></font></td></tr>\n"));

#if 0
#ifdef CONFIG_IPV6
			"<td align=center ><font size=\"2\"><b>IP Version</b></font></td>\n"
#endif
#endif

	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_PORTFILTER_TBL, (void *)&entry))
			return -1;

		if ( entry.protoType == PROTO_BOTH )
			type = "TCP+UDP";
		else if ( entry.protoType == PROTO_TCP )
			type = "TCP";
		else
			type = "UDP";

		if ( entry.fromPort == 0)
			strcpy(portRange, "----");
		else if ( entry.fromPort == entry.toPort )
			snprintf(portRange, 20, "%d", entry.fromPort);
		else
			snprintf(portRange, 20, "%d-%d", entry.fromPort, entry.toPort);

		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
   			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
     			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"15%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				portRange, type, 
				entry.comment, i);
	}
	
#if 0
#ifdef CONFIG_IPV6
			   "<td align=center ><font size=\"2\">IPv%d</td>\n"
#endif
#endif

#if 0			
#ifdef	CONFIG_IPV6
					entry.ipVer,
#endif
#endif

	return nBytesSent;
}


/////////////////////////////////////////////////////////////////////////////
int ipFilterList(request *wp, int argc, char **argv)
{
	int	nBytesSent=0, entryNum, i,iplen,ipendlen,dstiplen,dstipendlen, mode;
	IPFILTER_T entry;
	char	*type;
	
//#define IP_RANGE_MULTI_WAN_SUPPORT	
#ifdef IP_RANGE_MULTI_WAN_SUPPORT
	char wanBuf[20];
	char ip[16];
	char endip[16] ;
	char dstip[16] ;
	char dstendip[16] ;
#else
    char *ip;
#endif

	if ( !apmib_get(MIB_IPFILTER_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}

#ifdef IP_RANGE_MULTI_WAN_SUPPORT
	nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
      	"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(ipfilter_tbl_local_addr)</script></b></font></td>\n"
      	"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(ipfilter_tbl_local_end_addr)</script></b></font></td>\n"
      	"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(ipfilter_tbl_dst_addr)</script></b></font></td>\n"
      	"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(ipfilter_tbl_dst_end_addr)</script></b></font></td>\n"
      	"<td align=center width=\"25%%\" ><font size=\"2\"><b><script>dw(ipfilter_tbl_proto)</script></b></font></td>\n"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(ipfilter_tbl_wan)</script></b></font></td>\n"
      	"<td align=center width=\"25%%\" ><font size=\"2\"><b><script>dw(ipfilter_tbl_comm)</script></b></font></td>\n"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(ipfilter_tbl_select)</script></b></font></td></tr>\n"));
#else
    nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
      	"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(ipfilter_tbl_local_addr)</script></b></font></td>\n"
      	"<td align=center width=\"25%%\" ><font size=\"2\"><b><script>dw(ipfilter_tbl_proto)</script></b></font></td>\n"
      	"<td align=center width=\"25%%\" ><font size=\"2\"><b><script>dw(ipfilter_tbl_comm)</script></b></font></td>\n"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(ipfilter_tbl_select)</script></b></font></td></tr>\n"));
#endif

	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_IPFILTER_TBL, (void *)&entry))
			return -1;
		
#ifdef IP_RANGE_MULTI_WAN_SUPPORT
        apmib_get(MIB_IPFILTER_MODE, (void *)&mode);
		if(entry.ipFilterMode != mode)continue;

		      iplen = strlen(inet_ntoa(*((struct in_addr *)entry.ipAddr)));
		      memcpy(ip,inet_ntoa(*((struct in_addr *)entry.ipAddr)),iplen);
			  ip[iplen]='\0';
		      if ( !strcmp(ip, "0.0.0.0"))
			      strcpy(ip, "----");
			  
		      ipendlen=strlen(inet_ntoa(*((struct in_addr *)entry.ipAddrEnd)));
		      memcpy(endip,inet_ntoa(*((struct in_addr *)entry.ipAddrEnd)),ipendlen);
			  endip[ipendlen]='\0';
		      if ( !strcmp(endip, "0.0.0.0"))
			       strcpy(endip, "----");

			  dstiplen = strlen(inet_ntoa(*((struct in_addr *)entry.dstipAddr)));
		      memcpy(dstip,inet_ntoa(*((struct in_addr *)entry.dstipAddr)),dstiplen);
			  dstip[dstiplen]='\0';
		      if ( !strcmp(dstip, "0.0.0.0"))
			       strcpy(dstip, "----");

			  dstipendlen = strlen(inet_ntoa(*((struct in_addr *)entry.dstipAddrEnd)));
		      memcpy(dstendip,inet_ntoa(*((struct in_addr *)entry.dstipAddrEnd)),dstipendlen);
			  dstendip[dstipendlen]='\0';
		      if ( !strcmp(dstendip, "0.0.0.0"))
			       strcpy(dstendip, "----");
#else
		ip = inet_ntoa(*((struct in_addr *)entry.ipAddr));
		if ( !strcmp(ip, "0.0.0.0"))
			ip = "----";
#endif

		if ( entry.protoType == PROTO_BOTH )
			type = "TCP+UDP";
		else if ( entry.protoType == PROTO_TCP )
			type = "TCP";
#ifdef IP_RANGE_MULTI_WAN_SUPPORT
		else if ( entry.protoType == PROTO_ICMP )
			type = "ICMP";
#endif
		else
			type = "UDP";

#ifdef IP_RANGE_MULTI_WAN_SUPPORT
		snprintf(wanBuf, sizeof(wanBuf), "WAN%d", entry.WANIfIndex);
#endif
#ifdef CONFIG_IPV6
		if(entry.ipVer==IPv4)
		#ifdef IP_RANGE_MULTI_WAN_SUPPORT
			nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				ip, endip, dstip, dstendip, type, wanBuf, entry.comment, i);
		#else
		    nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				ip, type, entry.comment, i);
		#endif
		else
		#ifdef IP_RANGE_MULTI_WAN_SUPPORT
			nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				entry.ip6Addr, entry.ip6AddrEnd, entry.dstip6Addr, entry.dstip6AddrEnd, type, wanBuf, entry.comment, i);
		#else
			nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				entry.ip6Addr, type, entry.comment, i);
		#endif
#else
       #ifdef IP_RANGE_MULTI_WAN_SUPPORT
		nBytesSent += req_format_write(wp, ("<tr>"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				ip, endip, dstip, dstendip, type, wanBuf, entry.comment, i);
	   #else
	    nBytesSent += req_format_write(wp, ("<tr>"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				ip, type, entry.comment, i);
	   #endif
#endif
	}
	
	return nBytesSent;
}


/////////////////////////////////////////////////////////////////////////////
int macFilterList(request *wp, int argc, char **argv)
{
	int nBytesSent=0, entryNum, i;
	MACFILTER_T entry;
	char tmpBuf[100];
	char tmpDMACBuf[100];
	#ifdef MACFILTER_MODE_SUPPORT
	int mode=0;
	#endif

	if ( !apmib_get(MIB_MACFILTER_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}

	nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
      	"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(macfilter_smac)</script></b></font></td>\n"
      	#ifdef DST_MAC_FILTER
      	"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(macfilter_dmac)</script></b></font></td>\n"
      	#endif
      	"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(macfilter_comment)</script></b></font></td>\n"
      	"<td align=center width=\"15%%\" ><font size=\"2\"><b><script>dw(macfilter_select)</script></b></font></td></tr>\n"));

#ifdef MACFILTER_MODE_SUPPORT
	apmib_get(MIB_MACFILTER_MODE, (void *)&mode);
#endif

	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_MACFILTER_TBL, (void *)&entry))
			return -1;
		
#ifdef MACFILTER_MODE_SUPPORT
		if(entry.filterMode != mode)
			continue;
#endif

		snprintf(tmpBuf, 100, ("%02x:%02x:%02x:%02x:%02x:%02x"),
			entry.macAddr[0], entry.macAddr[1], entry.macAddr[2],
			entry.macAddr[3], entry.macAddr[4], entry.macAddr[5]);
		
		#ifdef DST_MAC_FILTER
		snprintf(tmpDMACBuf, 100, ("%02x:%02x:%02x:%02x:%02x:%02x"),
			entry.dmacAddr[0], entry.dmacAddr[1], entry.dmacAddr[2],
			entry.dmacAddr[3], entry.dmacAddr[4], entry.dmacAddr[5]);

		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
       			"<td align=center width=\"15%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				tmpBuf, tmpDMACBuf, entry.comment, i);
		#else
	    nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
       			"<td align=center width=\"15%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				tmpBuf, entry.comment, i);
		#endif
	}
	return nBytesSent;
}

/////////////////////////////////////////////////////////////////////////////
int urlFilterList(request *wp, int argc, char **argv)
{
	int nBytesSent=0, entryNum, i;
	URLFILTER_T entry;
	int mode,usrMode;
#ifdef URL_FILTER_USER_MODE_SUPPORT
	char tmpBuf[20],tmpBuf2[20];
	int defaultRulefound=0;
#endif
	if ( !apmib_get(MIB_URLFILTER_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}

	if ( !apmib_get(MIB_URLFILTER_MODE, (void *)&mode)) {
  		fprintf(stderr, "Get URL Filter mode error!\n");
		return -1;
	}
#ifdef URL_FILTER_USER_MODE_SUPPORT	
		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
			"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(urlfilter_urladdr)</script></b></font></td>\n"
			"<td align=center width=\"25%%\" ><font size=\"2\"><b><script>dw(urlfilter_ipaddr)</script></b></font></td>\n"
			"<td align=center width=\"25%%\" ><font size=\"2\"><b><script>dw(urlfilter_macaddr)</script></b></font></td>\n"
			"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(urlfilter_select)</script></b></font></td></tr>\n"));
#else
	nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
      	"<td align=center width=\"70%%\" ><font size=\"2\"><b><script>dw(urlfilter_urladdr)</script></b></font></td>\n"
      	"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(urlfilter_select)</script></b></font></td></tr>\n"));
#endif
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_URLFILTER_TBL, (void *)&entry))
			return -1;
		if(mode!=entry.ruleMode)
			continue;
#ifdef URL_FILTER_USER_MODE_SUPPORT
		usrMode=(int)entry.usrMode;
		if(usrMode==0)//default rule
		{
			defaultRulefound=1;
			continue;
		}
		switch(usrMode)
		{
			case 1://for specific ip
			{
				strcpy(tmpBuf,inet_ntoa(*((struct in_addr *)entry.ipAddr)));
				snprintf(tmpBuf2,20,"-");
				break;
			}
			case 2://for specific mac
			{
				snprintf(tmpBuf,20,"-");
				snprintf(tmpBuf2, 20, ("%02x:%02x:%02x:%02x:%02x:%02x"),
						 entry.macAddr[0], entry.macAddr[1], entry.macAddr[2],
					     entry.macAddr[3], entry.macAddr[4], entry.macAddr[5]);
				break;
			}
			default:
			{
				snprintf(tmpBuf,20,"-");
				snprintf(tmpBuf2,20,"-");
				break;
			}
		}
		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"20%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
			entry.urlAddr,tmpBuf, tmpBuf2, i); 
#else
		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"70%%\" ><font size=\"2\">%s</td>\n"
      			//"<td align=center width=\"30%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
       			"<td align=center width=\"30%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
			entry.urlAddr, i); //tmpBuf
			//entry.urlAddr, entry.comment, i); //tmpBuf
#endif
	}
#ifdef URL_FILTER_USER_MODE_SUPPORT //display default rules
	if(defaultRulefound==1)
	{
		for (i=1; i<=entryNum; i++) {
			*((char *)&entry) = (char)i;
			if ( !apmib_get(MIB_URLFILTER_TBL, (void *)&entry))
				return -1;
			if(mode!=entry.ruleMode)
				continue;
			if(0!=entry.usrMode)
				continue;
			snprintf(tmpBuf,20,"For all users");
			snprintf(tmpBuf2,20,"For all users");
			nBytesSent += req_format_write(wp, ("<tr>"
				"<td align=center width=\"30%%\" bgcolor=\"#FFBF00\"><font size=\"2\">%s</td>\n"
				"<td align=center width=\"25%%\" bgcolor=\"#FFBF00\"><font size=\"2\">%s</td>\n"
				"<td align=center width=\"25%%\" bgcolor=\"#FFBF00\"><font size=\"2\">%s</td>\n"
				"<td align=center width=\"20%%\" bgcolor=\"#FFBF00\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				entry.urlAddr,tmpBuf, tmpBuf2, i); 
		}	
	}
#endif
	return nBytesSent;

}

#if 0
/////////////////////////////////////////////////////////////////////////////
int triggerPortList(request *wp, int argc, char **argv)
{

	int	nBytesSent=0, entryNum, i;
	TRIGGERPORT_T entry;
	char	*triType, triPortRange[20], *incType, incPortRange[20];

	if ( !apmib_get(MIB_TRIGGERPORT_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}

	nBytesSent += req_format_write(wp, ("<tr>"
      	"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Trigger-port Range</b></font></td>\n"
      	"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Trigger-port Protocol</b></font></td>\n"
     	"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Incoming-port Range</b></font></td>\n"
      	"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Incoming-port Protocol</b></font></td>\n"
	"<td align=center width=\"14%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Comment</b></font></td>\n"
      	"<td align=center width=\"6%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td></tr>\n"));


#if 0
	nBytesSent += req_format_write(wp, ("<tr>"
	"<td align=center width=\"25%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Trigger-port Range</b></font></td>\n"
      	"<td align=center width=\"15%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Trigger-port Protocol</b></font></td>\n")
	"<td align=center width=\"25%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Incoming-port Range</b></font></td>\n"
      	"<td align=center width=\"15%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Incoming-port Protocol</b></font></td>\n"
	"<td align=center width=\"14%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Comment</b></font></td>\n"
      	"<td align=center width=\"6%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td></tr>\n"));

#endif
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_TRIGGERPORT_TBL, (void *)&entry))
			return -1;

		if ( entry.tri_protoType == PROTO_BOTH )
			triType = "TCP+UDP";
		else if ( entry.tri_protoType == PROTO_TCP )
			triType = "TCP";
		else
			triType = "UDP";

		if ( entry.tri_fromPort == 0)
			strcpy(triPortRange, "----");
		else if ( entry.tri_fromPort == entry.tri_toPort )
			snprintf(triPortRange, 20, "%d", entry.tri_fromPort);
		else
			snprintf(triPortRange, 20, "%d-%d", entry.tri_fromPort, entry.tri_toPort);

		if ( entry.inc_protoType == PROTO_BOTH )
			incType = "TCP+UDP";
		else if ( entry.inc_protoType == PROTO_TCP )
			incType = "TCP";
		else
			incType = "UDP";

		if ( entry.inc_fromPort == 0)
			strcpy(incPortRange, "----");
		else if ( entry.inc_fromPort == entry.inc_toPort )
			snprintf(incPortRange, 20, "%d", entry.inc_fromPort);
		else
			snprintf(incPortRange, 20, "%d-%d", entry.inc_fromPort, entry.inc_toPort);


		nBytesSent += req_format_write(wp, ("<tr>"
			"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
   			"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
			"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
   			"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
     			"<td align=center width=\"20%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"6%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				triPortRange, triType, incPortRange, incType, entry.comment, i);
	}
	return nBytesSent;
}
#endif


#if defined(CONFIG_CMCC_QOS)	// cmcc qos
#if defined(CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT)
void formMultiPhyWan(request *wp, char *path, char *query)
{
	unsigned int wan_idx;
	unsigned int wan_port;
	char *strVal;
	char *submitUrl;
	char tmpBuf[100] = {0};
	char wan_name[16] = {0};
	int ret = 1;
	
	//wan_port
	strVal = req_get_cstream_var(wp, ("Wan"), "");
	
	if(strVal[0]){
		//printf("set_qos_wan_port!\n");
		wan_idx = atoi(strVal);

		get_wan_logic_portid_from_idx(wan_idx,&wan_port);

		//get_wan_root_name_from_logic_port(wan_port,wan_name);

		//printf("[%s:%d]wan_name: %s\n",__FUNCTION__,__LINE__,wan_name);

#if 0
		if(ret !=0)
		{
			//printf("[%s:%d]port not wan port!\n",__FUNCTION__,__LINE__);
			strcpy(tmpBuf, ("port not wan port!"));
			goto setErr;
		}
#endif
		if(!apmib_set(MIB_QOS_WAN_INDEX, (void *)&wan_idx)) {
			strcpy(tmpBuf, ("set MIB_QOS_WAN_INDEX error!"));
			goto setErr;
		}
		
		if(!apmib_set(MIB_QOS_WAN_PORT, (void *)&wan_port)) {
			strcpy(tmpBuf, ("set MIB_QOS_WAN_PORT error!"));
			goto setErr;
		}
	}
	
setOk:
	apmib_update_web(CURRENT_SETTING);
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	 // hidden page
	OK_MSG(submitUrl);
	return ;
		
setErr:
	ERR_MSG(tmpBuf);

	
}
void formMultiPhyWanUplinkQoS(request *wp, char *path, char *query)
{
	char *submitUrl, *submitUrl_class;
	char *strAddCls, *strVal, *strApply;
	char tmpBuf[100] = {0};
	char qos_mode[64] = {0};
	char wan_name[16] = {0};
	unsigned int val = 0, index = 0, ret = 1;
	unsigned int qos_flag = 0, wan_port = 0, wan = 0,qos_mode_null = 0, wanPortFlag;
	unsigned int policy = 0,up_speed = 0, en_force_weight = 0, dscp_remark = 0, dot1p_remark = 0;

	
	// init flag of qos_rule.htm
	if(!apmib_set(MIB_UPLINK_QOS_OPERATION, (void *)&val)){
		strcpy(tmpBuf, ("set MIB_UPLINK_QOS_OPERATION error!"));
		goto setErr;
	}

	
	strAddCls = req_get_cstream_var(wp, ("AddCls"), "");
	submitUrl_class = req_get_cstream_var(wp, ("submitUrl_class"), "");
	//click add class, switch to uplinkQosRule webpage	
	if(strAddCls[0] && submitUrl_class[0]){
		send_redirect_perm(wp, submitUrl_class);
	}

	//wan_port_tbl
	strVal = req_get_cstream_var(wp, ("policy"), "");
	
	strApply = req_get_cstream_var(wp, ("save_apply"), "");
	if(strVal[0] ||strApply[0] ){
		//printf("change wan_port_tbl and queue_tbl!\n");

		if(!apmib_get(MIB_QOS_WAN_INDEX,(void *)&wan)){
			strcpy(tmpBuf,("get MIB_QOS_WAN_INDEX error!"));
			goto setErr;
		}

		ret = get_wan_logic_portid_from_idx(wan,&wan_port);
		
		ret = get_wan_root_name_from_logic_port(wan_port,wan_name);

		if(ret == 0){
			//printf("[%s:%d]port is wan port!\n",__FUNCTION__,__LINE__);
			wanPortFlag = 1;
		}
		else{
			//printf("[%s:%d]port is not wan port!\n",__FUNCTION__,__LINE__);
			wanPortFlag = 0;
		}
		
		if(strVal[0]){
			policy = atoi(strVal);
		}
		
		strVal = req_get_cstream_var(wp, ("enable"), "");
		if(strVal[0] && !strcmp(strVal, "ON")){
			qos_flag = 1;
		}

		strVal = req_get_cstream_var(wp, ("uplinkSpeed"), "");
		if(strVal[0]){
			up_speed = atoi(strVal);
		}

		strVal = req_get_cstream_var(wp, ("service"), "");
		if(strVal[0]){
			if(parseQosMode((unsigned char *)strVal) == 0){
				strcpy(tmpBuf, ("Invalid QoS Mode Input!"));
				goto setErr;
			}		
			strcpy(qos_mode,(unsigned char *)strVal);
		}
		else{
			//printf("[%s:%d] service is null!\n",__FUNCTION__,__LINE__);
			qos_mode_null = 1;
			//if(policy == 0){
				//if((empty_qosApp(wan_port)) == 0)
					//return 0;
			//}
		}
				
		strVal = req_get_cstream_var(wp, ("enForceWeight"), "");
		if(strVal[0]){
			if (!strcmp(strVal, "ON"))
				en_force_weight = 1;
			else
				en_force_weight = 0;
		}
				
		strVal = req_get_cstream_var(wp, ("dscpRemark"), "");
		if(strVal[0]){
			if (!strcmp(strVal, "ON"))
				dscp_remark = 1;
			else
				dscp_remark = 0;
		}
		
		strVal = req_get_cstream_var(wp, ("dot1pRemark"), "");
		if(strVal[0]){
			dot1p_remark = atoi(strVal);
		}
				
		if(!apmib_get(MIB_QOS_WANPORT_TBL_NUM, (void *)&val)){
		    strcpy(tmpBuf, ("get MIB_QOS_WANPORT_TBL_NUM error!"));
			goto setErr;
		}


		//printf("wan_port:%d!\n",wan_port);

		for(index = 1; index <= val; index++){
			QOSWANPORT_T wanPort_entry, qosArray[2];

			// need to modify wan port
			memset(&wanPort_entry, '\0', sizeof(QOSWANPORT_T));
			*((char *)&wanPort_entry) = (char)index;

			if((apmib_get(MIB_QOS_WANPORT_TBL, (void *)&wanPort_entry)) == 0){
					strcpy(tmpBuf, ("get MIB_QOS_WANPORT_TBL error!"));
					goto setErr;
			}

			if(wanPort_entry.WanPortInstanceNum != wan)
				continue;

			if(wanPortFlag){
				if(qos_flag){
					if(wanPort_entry.WanPortPlan != policy || wanPort_entry.WanPortEnable != qos_flag
					|| wanPort_entry.WanPortUpLinkSpeed != up_speed || wanPort_entry.WanPortMode != qos_mode
					|| wanPort_entry.WanPortEnforceWeight != en_force_weight || wanPort_entry.WanPortEnDscpRemark
					!= dscp_remark || wanPort_entry.WanPortEnDot1pRemark != dot1p_remark || 
					wanPort_entry.WanPortSet != wanPortFlag || (strcmp(wanPort_entry.WanPortName,wan_name)!=0)){
						printf("[%s:%d]wanPortFlag && qos_flag!\n",__FUNCTION__,__LINE__);
						memset(&qosArray[0], 0, sizeof(QOSWANPORT_T));
						memset(&qosArray[1], 0, sizeof(QOSWANPORT_T));
						memcpy(&qosArray[0], &wanPort_entry, sizeof(QOSWANPORT_T));

						if(qos_mode_null){
							//only change qos_mode when sp->sp
							if(0 == wanPort_entry.WanPortPlan){
								snprintf((char *)wanPort_entry.WanPortMode,sizeof(wanPort_entry.WanPortMode), "%s", qos_mode);
								if(policy == 0){
									printf("qos_mode NULL!");
									if((empty_qosApp(wan_port)) == 0)
									return 0;
								}
							}
						}else{
						//printf("qos_mode not NULL!");
						snprintf((char *)wanPort_entry.WanPortMode,sizeof(wanPort_entry.WanPortMode), "%s", qos_mode);
						}

						//don't change force_weighted from sp->wfq(for sp,weighted == "")
						if(!en_force_weight){
							if(1 == wanPort_entry.WanPortPlan)
								wanPort_entry.WanPortEnforceWeight = en_force_weight;
						}else{
							wanPort_entry.WanPortEnforceWeight = en_force_weight;
						}

						strncpy(wanPort_entry.WanPortName,wan_name,sizeof(wanPort_entry.WanPortName));
						wanPort_entry.WanPortSet = wanPortFlag;
						wanPort_entry.WanPortPlan = policy;
						wanPort_entry.WanPortEnable = qos_flag;
						wanPort_entry.WanPortUpLinkSpeed = up_speed;			
						wanPort_entry.WanPortEnDscpRemark = dscp_remark;
						wanPort_entry.WanPortEnDot1pRemark = dot1p_remark;
						
						memcpy(&qosArray[1], &wanPort_entry, sizeof(QOSWANPORT_T));
						if(!apmib_set(MIB_QOS_WANPORT_MOD, (void *)&qosArray)){
							strcpy(tmpBuf, ("mod QOS_QUEUE_TBL error!"));
							goto setErr;
			    		}
					}
				}else{
					if(wanPort_entry.WanPortEnable != qos_flag || wanPort_entry.WanPortSet != wanPortFlag
						|| (strcmp(wanPort_entry.WanPortName,wan_name)!=0)){
						printf("[%s:%d]wanPortFlag and not qos_flag!\n",__FUNCTION__,__LINE__);
						memset(&qosArray[0], 0, sizeof(QOSWANPORT_T));
						memset(&qosArray[1], 0, sizeof(QOSWANPORT_T));
						memcpy(&qosArray[0], &wanPort_entry, sizeof(QOSWANPORT_T));

						strncpy(wanPort_entry.WanPortName,wan_name,sizeof(wanPort_entry.WanPortName));
						wanPort_entry.WanPortEnable = qos_flag;
						wanPort_entry.WanPortSet = wanPortFlag;
					
						memcpy(&qosArray[1], &wanPort_entry, sizeof(QOSWANPORT_T));
						if(!apmib_set(MIB_QOS_WANPORT_MOD, (void *)&qosArray)){
							strcpy(tmpBuf, ("mod QOS_QUEUE_TBL error!"));
							goto setErr;
			    		}
					}
					goto setOk;
				}
			}else{
				if(wanPort_entry.WanPortSet != wanPortFlag || (strcmp(wanPort_entry.WanPortName,wan_name)!=0)){
					printf("[%s:%d]not wanPortFlag!\n",__FUNCTION__,__LINE__);
					memset(&qosArray[0], 0, sizeof(QOSWANPORT_T));
					memset(&qosArray[1], 0, sizeof(QOSWANPORT_T));
					memcpy(&qosArray[0], &wanPort_entry, sizeof(QOSWANPORT_T));

					strncpy(wanPort_entry.WanPortName,wan_name,sizeof(wanPort_entry.WanPortName));
					wanPort_entry.WanPortSet = wanPortFlag;
					
					memcpy(&qosArray[1], &wanPort_entry, sizeof(QOSWANPORT_T));
					if(!apmib_set(MIB_QOS_WANPORT_MOD, (void *)&qosArray)){
						strcpy(tmpBuf, ("mod QOS_QUEUE_TBL error!"));
						goto setErr;
			    	}
				}

				strcpy(tmpBuf,("port not wan_port!"));
				goto setErr;
			}

		}

		if(strApply[0]){
			if(!apmib_get(MIB_QOS_QUEUE_TBL_NUM, (void *)&val)){
		    	strcpy(tmpBuf, ("get MIB_QOS_QUEUE_TBL_NUM error!"));
				goto setErr;
			}

			for(index = 1; index <= val; index++){
				char str_tmp[16] = {0}, str_weight[16] = {0};
				QOSQUEUE_T queue_entry, qosArray[2];
				unsigned int flag1 = 0, flag2 = 0; 
				unsigned int queue_enable = 0, queue_weight = 0;
				unsigned int index1 = 0;

				index1 = (index-1)%MAX_PER_PORT_QOS_QUEUE_NUM;

				if(policy == 0){
					snprintf(str_tmp, sizeof(str_tmp), "select%d", index1);
				}
				else{
					snprintf(str_tmp, sizeof(str_tmp), "select%d", index1+10);
					snprintf(str_weight, sizeof(str_weight), "weight%d", index1);
				}

				// need to modify queue status
				memset(&queue_entry, '\0', sizeof(QOSQUEUE_T));
				*((char *)&queue_entry) = (char)index;
		
				if((apmib_get(MIB_QOS_QUEUE_TBL, (void *)&queue_entry)) == 0){
					strcpy(tmpBuf, ("get MIB_QOS_QUEUE_TBL error!"));
					goto setErr;
				}
				
				if(queue_entry.QueueWanPort != wan || (queue_entry.QueueInstanceNum != (index1+1)))
					continue;

				strVal = req_get_cstream_var(wp, (str_tmp), "");
				if(strVal[0]){
					if(queue_entry.QueueEnable != 1){
						flag1 = 1;
						queue_enable = 1;
					}
				}
				else{
					if(queue_entry.QueueEnable != 0){
						flag1 = 1;
						queue_enable = 0;
					}
				}

				strVal = req_get_cstream_var(wp, (str_weight), "");
					if(policy == 1 && strVal[0]){
					if(atoi(strVal) != queue_entry.Weight){
					//flag2 == 1,need to modify queue weight
						flag2 = 1;
						queue_weight = atoi(strVal);
					}
				}

				if(flag1 == 1 || flag2 == 1){
					memset(&qosArray[0], 0, sizeof(QOSQUEUE_T));
			    	memset(&qosArray[1], 0, sizeof(QOSQUEUE_T));
			   		memcpy(&qosArray[0], &queue_entry, sizeof(QOSQUEUE_T));
				
					if(flag1 == 1){
						queue_entry.QueueEnable = queue_enable;
					}
				
					if(flag2 == 1){
						queue_entry.Weight = queue_weight;
					}

					memcpy(&qosArray[1], &queue_entry, sizeof(QOSQUEUE_T));
					if(!apmib_set(MIB_QOS_QUEUE_MOD, (void *)&qosArray)){
						strcpy(tmpBuf, ("mod QOS_QUEUE_TBL error!"));
						goto setErr;
			    	}
				}	
			}
		}
	}
setOk:
	apmib_update_web(CURRENT_SETTING);
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	OK_MSG(submitUrl);
	return ;
	
setErr:
	ERR_MSG(tmpBuf);
}

void formUplinkMultiPhyWanQosRule(request *wp, char *path, char *query)
{
	char *submitUrl, *submitUrl_uplink, *returnUplink; 
	char *strApply, *strClsId, *strTypeId;
	char *strAddCls, *strDelCls, *strAddType, *strDelType;
	char *strQosOperation, *strQueue, *strWanPort, *strDscpRemark, *strDot1pRemark;
	char *strType, *strMin, *strMax, *strProto;
	char tmpBuf[100] = {0}, tmpMin[64] = {0}, tmpMax[64] = {0}, typeName[MAX_TYPE_NAME_LEN] = {0};
	int number = 0, index = 0;
	int class_id = 0, type_id = 0;
	int wan_port = 0;
	char wan_name[16] = {0};
	unsigned int qos_operation = 0;

	returnUplink = req_get_cstream_var(wp, ("ReturnUplink"), "");
	submitUrl_uplink = req_get_cstream_var(wp, ("submitUrl_uplink"), "");
	//click return button on webpage,returb to uplink webpage
	if(returnUplink[0] && submitUrl_uplink[0]){
		send_redirect_perm(wp, submitUrl_uplink);
	}

	//get current class id and type id
	strClsId = req_get_cstream_var(wp, ("current_classid"), "");
	if(strClsId[0]){
		class_id = atoi(strClsId);

		if(!apmib_set(MIB_CURRENT_CLASSID, (void *)&class_id)){
            strcpy(tmpBuf, ("set MIB_CURRENT_CLASSID error!"));
			goto setErr;
        }
	}
	else{
		if(!apmib_get(MIB_CURRENT_CLASSID, (void *)&class_id)){
            strcpy(tmpBuf, ("get MIB_CURRENT_CLASSID error!"));
			goto setErr;
        }
	}

	strTypeId = req_get_cstream_var(wp, ("current_typeid"), "");
	if(strTypeId[0]){
		type_id = atoi(strTypeId);

		if(!apmib_set(MIB_CURRENT_TYPEID, (void *)&type_id)){
            strcpy(tmpBuf, ("set MIB_CURRENT_TYPEID error!"));
			goto setErr;
        }
	}
	else{
		if(!apmib_get(MIB_CURRENT_TYPEID, (void *)&type_id)){
            strcpy(tmpBuf, ("get MIB_CURRENT_TYPEID error!"));
			goto setErr;
        }
	}
	//get current class id and type id end

	strAddCls = req_get_cstream_var(wp, ("AddCls"), "");
	if(strAddCls[0]){
		//click add class , only add classInstanceNumnow. 
		if(addQosClass() == -1){
			strcpy(tmpBuf, ("CLASS TBL FULL!"));
			goto setErr;
		}

		if(!apmib_set(MIB_UPLINK_QOS_OPERATION, (void *)&qos_operation)){
		    strcpy(tmpBuf, ("set MIB_UPLINK_QOS_OPERATION error!"));
			goto setErr;
		}
	}

	strDelCls = req_get_cstream_var(wp, ("DelCls"), "");
	if(strDelCls[0]){
		if(class_id > 0){
			delQosTypeByClassId(class_id);
			delQosClass(class_id);
			
			if(!apmib_set(MIB_UPLINK_QOS_OPERATION, (void *)&qos_operation)){
			    strcpy(tmpBuf, ("set MIB_UPLINK_QOS_OPERATION error!"));
				goto setErr;
			}
		}
		else{
			strcpy(tmpBuf, ("Get qos current class id error!"));
			goto setErr;
		}
	}

	strAddType = req_get_cstream_var(wp, ("AddType"), "");
	if(strAddType[0]){
		if(class_id > 0){
			//only add typeInstanceNum and typeClassId now
			if(addQosTypeByClassId(class_id) == -1){
				strcpy(tmpBuf, ("TYPE TBL FULL!"));
				goto setErr;
			}
		}
		else{
			strcpy(tmpBuf, ("Get qos current class id error!"));
			goto setErr;
		}
	}

	strDelType = req_get_cstream_var(wp, ("DelType"), "");
	if(strDelType[0]){
		if(class_id > 0 && type_id > 0){
			delQosType(class_id, type_id);
			
			if(!apmib_set(MIB_UPLINK_QOS_OPERATION, (void *)&qos_operation)){
			    strcpy(tmpBuf, ("set MIB_UPLINK_QOS_OPERATION error!"));
				goto setErr;
			}
		}
		else{
			strcpy(tmpBuf, ("Get qos current type id error!"));
			goto setErr;
		}
	}

	// 1: add type  2: edit class  3: edit type
	strQosOperation = req_get_cstream_var(wp, ("qos_operation"), "");
	if(strQosOperation[0]){
		qos_operation = atoi(strQosOperation);

		if(!apmib_set(MIB_UPLINK_QOS_OPERATION, (void *)&qos_operation)){
            strcpy(tmpBuf, ("set MIB_UPLINK_QOS_OPERATION error!"));
			goto setErr;
        }
	}

	strApply = req_get_cstream_var(wp, ("save_apply"), "");
	if(strApply[0]){
		if(!apmib_get(MIB_UPLINK_QOS_OPERATION, (void *)&qos_operation)){
            strcpy(tmpBuf, ("get MIB_UPLINK_QOS_OPERATION error!"));
			goto setErr;
        }

		if(qos_operation == 2){		// edit class
			QOSCLASS_T qosClass, qosArray[2];

			if(!apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&number)){
				strcpy(tmpBuf, ("get MIB_QOS_CLASS_TBL_NUM error!"));
				goto setErr;
			}

			for(index = 1; index <= number; index++){
				memset(&qosClass, '\0', sizeof(QOSCLASS_T));
				*((char *)&qosClass) = (char)index;

				if((apmib_get(MIB_QOS_CLASS_TBL, (void *)&qosClass)) == 0){
					strcpy(tmpBuf, ("get MIB_QOS_CLASS_TBL error!"));
					goto setErr;
				}

				if(qosClass.ClassInstanceNum == class_id){
					break;
				}
			}

			memset(&qosArray[0], 0, sizeof(QOSCLASS_T));
	    	memset(&qosArray[1], 0, sizeof(QOSCLASS_T));
	    	memcpy(&qosArray[0], &qosClass, sizeof(QOSCLASS_T));

			strWanPort = req_get_cstream_var(wp, ("wanport"), "");

			if(strWanPort[0]){
				wan_port = atoi(strWanPort);

				get_wan_eth_ifname_from_idx(wan_port,wan_name);

				if(wan_name[0]){ //port has setted to wan_port
					qosClass.ClassWanPort = atoi(strWanPort);
				}
				else{ //port not wan_port, show error info
					strcpy(tmpBuf, ("not wan!"));
					goto setErr;
				}								
			}
						
			strQueue = req_get_cstream_var(wp, ("queue"), "");
			if(strQueue[0]){
				qosClass.ClassQueue = atoi(strQueue);
			}

			strDscpRemark = req_get_cstream_var(wp, ("dscpRemark"), "");
			if(strDscpRemark[0]){
				qosClass.DSCPMark = atoi(strDscpRemark);
			}

			strDot1pRemark = req_get_cstream_var(wp, ("dot1pRemark"), "");
			if(strDot1pRemark[0]){
				qosClass.Dot1pMark = atoi(strDot1pRemark);
			}
			
	        memcpy(&qosArray[1], &qosClass, sizeof(QOSCLASS_T));
			
			//update class_entry for this classid
			if(!apmib_set(MIB_QOS_CLASS_MOD, (void *)&qosArray)){
	            strcpy(tmpBuf, ("mod QOS_CLASS_TBL error!"));
				goto setErr;
	        }
			updateQosRule(class_id);
			
			goto setOk;
		}
		else if(qos_operation == 3){	// edit type
			QOSTYPE_T qosType,qosTypeArray[2];
	
			if(!apmib_get(MIB_QOS_TYPE_TBL_NUM, (void *)&number)){
				strcpy(tmpBuf, ("get MIB_QOS_TYPE_TBL_NUM error!"));
				goto setErr;
			}

			for(index = 1; index <= number; index++){
				memset(&qosType, '\0', sizeof(QOSTYPE_T));
				*((char *)&qosType) = (char)index;

				if((apmib_get(MIB_QOS_TYPE_TBL, (void *)&qosType)) == 0){
					strcpy(tmpBuf, ("get MIB_QOS_TYPE_TBL error!"));
					goto setErr;
				}

				if(qosType.TypeClassId == class_id 
					&& qosType.TypeInstanceNum == type_id){
					memset(&qosTypeArray[0], 0, sizeof(QOSTYPE_T));
			    	memset(&qosTypeArray[1], 0, sizeof(QOSTYPE_T));
			    	memcpy(&qosTypeArray[0], &qosType, sizeof(QOSTYPE_T));

					strMin = req_get_cstream_var(wp, ("minimum"), "");
					if(strMin[0]){
						strncpy(tmpMin, strMin, strlen(strMin));
					}

					strMax = req_get_cstream_var(wp, ("maximum"), "");
					if(strMax[0]){
						strncpy(tmpMax, strMax, strlen(strMax));
					}

					strProto = req_get_cstream_var(wp, ("protocol"), "");
					if(strProto[0]){
						if(!protoCheck(class_id, strProto)){
							strcpy(tmpBuf, ("Multiple type entries of a specified class must share the same protocol!"));
							goto setErr;
						}
						
						strncpy(qosType.Protocol, strProto, MAX_PROTOCOL_LEN);
					}

					strType = req_get_cstream_var(wp, ("type"), "");
					if(strType[0]){
						if(strncmp(strType, QOS_SMAC, strlen(QOS_SMAC)) == 0 || strncmp(strType, QOS_DMAC, strlen(QOS_DMAC)) == 0){
							// TODO: validity check
							
							if(strlen(tmpMin) != 17 || strlen(tmpMax) != 17){
								strcpy(tmpBuf, ("MAC input fail!"));
								goto setErr;
							}
						}
						else if(strncmp(strType, QOS_SIP, strlen(QOS_SIP)) == 0 || strncmp(strType, QOS_DIP, strlen(QOS_DIP)) == 0){
							// TODO: validity check

							if(strlen(tmpMin) == 0 || strlen(tmpMax) == 0){
								strcpy(tmpBuf, ("ip input fail!"));
								goto setErr;
							}
						}
						else if(strncmp(strType, QOS_SPORT, strlen(QOS_SPORT)) == 0 || strncmp(strType, QOS_DPORT, strlen(QOS_DPORT)) == 0){
							if(atoi(tmpMin) < 0 || atoi(tmpMin) > 0xFFFF || atoi(tmpMax) < 0 || atoi(tmpMax) > 0xFFFF){
								strcpy(tmpBuf, ("port out of range!"));
								goto setErr;
							}
						}
						else if(strncmp(strType, QOS_DSCP, strlen(QOS_DSCP)) == 0){
							if(!dscpCheck(class_id, 1)){
								strcpy(tmpBuf, ("A class entry can only have either dscp or tos type entry!"));
								goto setErr;
							}
							
							if(atoi(tmpMin) < 0 || atoi(tmpMin) > 0x3F || atoi(tmpMax) < 0 || atoi(tmpMax) > 0x3F){
								strcpy(tmpBuf, ("dscp out of range!"));
								goto setErr;
							}
						}
						else if(strncmp(strType, QOS_TOS, strlen(QOS_TOS)) == 0){
							if(!dscpCheck(class_id, 2)){
								strcpy(tmpBuf, ("A class entry can only have either dscp or tos type entry!"));
								goto setErr;
							}
							
							if(atoi(tmpMin) < 0 || atoi(tmpMin) > 0xFF || atoi(tmpMax) < 0 || atoi(tmpMax) > 0xFF){
								strcpy(tmpBuf, ("tos out of range!"));
								goto setErr;
							}
						}
						else if(strncmp(strType, QOS_DOT1P, strlen(QOS_DOT1P)) == 0){
							if(atoi(tmpMin) < 0 || atoi(tmpMin) > 7 || atoi(tmpMax) < 0 || atoi(tmpMax) > 7){
								strcpy(tmpBuf, ("802.1p out of range!"));
								goto setErr;
							}
						}
					#if 0
						else if(strncmp(strType, QOS_WAN, strlen(QOS_WAN)) == 0){
							// TODO: validity check
						}
						else if(strncmp(strType, QOS_LAN, strlen(QOS_LAN)) == 0){
							// TODO: validity check
						}
					#endif
						else{
							strcpy(tmpBuf, ("invalid qos class type!"));
							goto setErr;
						}

						if(!typeValidCheck(class_id, type_id, strType)){
							snprintf(tmpBuf, sizeof(tmpBuf), "A class entry can only have one %s type entry!", strType);
							goto setErr;
						}
						
						strncpy(typeName, strType, strlen(strType));
						snprintf(qosType.TypeName, sizeof(qosType.TypeName), "%s", typeName);
						
						snprintf(qosType.TypeMin, sizeof(qosType.TypeMin), "%s", tmpMin);
						snprintf(qosType.TypeMax, sizeof(qosType.TypeMax), "%s", tmpMax);
					}

					memcpy(&qosTypeArray[1], &qosType, sizeof(QOSTYPE_T));		
					if(!apmib_set(MIB_QOS_TYPE_MOD, (void *)&qosTypeArray)){
			            strcpy(tmpBuf, ("mod QOS_TYPE_TBL error!"));
						goto setErr;
			        }
					
					updateQosRule(class_id);
					break;
				}
			}
		}
	}

setOk:
	apmib_update_web(CURRENT_SETTING);
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	 // hidden page
	OK_MSG(submitUrl);
	return ;
	
setErr:
	ERR_MSG(tmpBuf);
}
#else
void formUplinkQoS(request *wp, char *path, char *query)
{
	char *submitUrl, *submitUrl_class;
	char *strAddCls, *strVal, *strApply;
	char tmpBuf[100] = {0};
	unsigned int val = 0, index = 0;
	unsigned int qos_flag = 0, up_speed = 0, policy = 0;
	unsigned int en_force_weight = 0, dscp_remark = 0, dot1p_remark = 0;

	// init flag of qos_rule.htm
	if(!apmib_set(MIB_UPLINK_QOS_OPERATION, (void *)&val)){
        strcpy(tmpBuf, ("set MIB_UPLINK_QOS_OPERATION error!"));
		goto setErr;
    }

	strAddCls = req_get_cstream_var(wp, ("AddCls"), "");
	submitUrl_class = req_get_cstream_var(wp, ("submitUrl_class"), "");
	//click add class, switch to uplinkQosRule webpage  
	if(strAddCls[0] && submitUrl_class[0]){
		send_redirect_perm(wp, submitUrl_class);
	}
	
	//policy : sp(0) or wfq(1)
	strVal = req_get_cstream_var(wp, ("policy"), "");
	if(strVal[0]){
		policy = atoi(strVal);
		
		if(!apmib_set(MIB_QOS_PLAN, (void *)&policy)) {
			strcpy(tmpBuf, ("set MIB_QOS_PLAN error!"));
			goto setErr;
		}
	}
	
	strApply = req_get_cstream_var(wp, ("save_apply"), "");
	//if(strApply[0]){
	if(1){
		strVal = req_get_cstream_var(wp, ("enable"), "");
		if(strVal[0] && !strcmp(strVal, "ON")){
			qos_flag = 1;
		}

		if(!apmib_set(MIB_QOS_ENABLE, (void *)&qos_flag)) {
			strcpy(tmpBuf, ("set MIB_QOS_ENABLE error!"));
			goto setErr;
		}

		if(!qos_flag){
			goto setOk;
		}

		strVal = req_get_cstream_var(wp, ("uplinkSpeed"), "");
		if(strVal[0]){
			up_speed = atoi(strVal);
		}
		strVal = req_get_cstream_var(wp, ("service"), "");
#if 0 //change to code down 
		if(strVal[0]){
			if(strlen(strVal )){
				if(parseQosMode((unsigned char *)strVal) == 0){
					strcpy(tmpBuf, ("Invalid QoS Mode Input!"));
					goto setErr;
				}
			}
			else{
				if(!apmib_get(MIB_QOS_PLAN, (void *)&policy)) {
					strcpy(tmpBuf, ("get MIB_QOS_PLAN error!"));
					goto setErr;
				}
				
				if(policy == 0){
					snprintf(tmpBuf, sizeof(tmpBuf), "%s", "");
					if (!apmib_set(MIB_QOS_MODE, (void *)tmpBuf)) {
						printf("[%s:%d] set MIB_QOS_MODE Error!\n", __FUNCTION__, __LINE__);
						return 0;
					}
				}
			}
		}
#endif

	    if(strVal[0]){
	    	if(parseQosMode((unsigned char *)strVal) == 0){
					strcpy(tmpBuf, ("Invalid QoS Mode Input!"));
					goto setErr;
			}
		}
		else{
			//if qos service is empty,strVal = "\n"
			if(!apmib_get(MIB_QOS_PLAN, (void *)&policy)) {
				strcpy(tmpBuf, ("get MIB_QOS_PLAN error!"));
				goto setErr;
			}
				
			if(policy == 0){
				//sp mode,should empty qosApp and qosMode
				QOSAPP_T qosApp;
				snprintf(tmpBuf, sizeof(tmpBuf), "%s", "");
				if (!apmib_set(MIB_QOS_MODE, (void *)tmpBuf)) {
					printf("[%s:%d] set MIB_QOS_MODE Error!\n", __FUNCTION__, __LINE__);
					return 0;
				}	
				memset(&qosApp, '\0', sizeof(QOSAPP_T));
				if (!apmib_set(MIB_QOS_APP_DELALL, (void *)&qosApp)) {
					printf("[%s:%d] MIB_QOS_APP_DELALL Error!\qn", __FUNCTION__, __LINE__);
					return 0;
				}	
			}
		}
		
		strVal = req_get_cstream_var(wp, ("enForceWeight"), "");
		if(strVal[0]){
			if (!strcmp(strVal, "ON"))
				en_force_weight = 1;
			else
				en_force_weight = 0;
				
			if(!apmib_set(MIB_QOS_ENFORCE_WEIGHT, (void *)&en_force_weight)) {
				strcpy(tmpBuf, ("set MIB_QOS_ENFORCE_WEIGHT error!"));
				goto setErr;
			}
		}
		
		strVal = req_get_cstream_var(wp, ("dscpRemark"), "");
		if(strVal[0]){
			if (!strcmp(strVal, "ON"))
				dscp_remark = 1;
			else
				dscp_remark = 0;
		}

		strVal = req_get_cstream_var(wp, ("dot1pRemark"), "");
		if(strVal[0]){
			dot1p_remark = atoi(strVal);
		}

		if(!apmib_set(MIB_QOS_UPLINK_SPEED, (void *)&up_speed)) {
			strcpy(tmpBuf, ("set MIB_QOS_UPLINK_SPEED error!"));
			goto setErr;
		}

		if(!apmib_set(MIB_QOS_EN_DSCP_REAMRK, (void *)&dscp_remark)) {
			strcpy(tmpBuf, ("set MIB_QOS_EN_DSCP_REAMRK error!"));
			goto setErr;
		}

		if(!apmib_set(MIB_QOS_EN_DOT1P_REMARK, (void *)&dot1p_remark)) {
			strcpy(tmpBuf, ("set MIB_QOS_EN_DOT1P_REMARK error!"));
			goto setErr;
		}

		if(strApply[0]){
			//click save in the webpage,QUEUE_TBL_NUM always 8
			if(!apmib_get(MIB_QOS_QUEUE_TBL_NUM, (void *)&val)){
		        strcpy(tmpBuf, ("get MIB_QOS_QUEUE_TBL_NUM error!"));
				goto setErr;
		    }

			for(index = 1; index <= val; index++){
				char str_tmp[16] = {0}, str_weight[16] = {0};
				QOSQUEUE_T queue_entry, qosArray[2];
				unsigned int flag1 = 0, flag2 = 0; 
				unsigned int queue_enable = 0, queue_weight = 0;

				if(policy == 0){
					snprintf(str_tmp, sizeof(str_tmp), "select%d", index-1);
				}
				else{
					snprintf(str_tmp, sizeof(str_tmp), "select%d", index-1+10);
					snprintf(str_weight, sizeof(str_weight), "weight%d", index-1);
				}

				// need to modify queue status
				memset(&queue_entry, '\0', sizeof(QOSQUEUE_T));
				*((char *)&queue_entry) = (char)index;

				if((apmib_get(MIB_QOS_QUEUE_TBL, (void *)&queue_entry)) == 0){
					strcpy(tmpBuf, ("get MIB_QOS_QUEUE_TBL error!"));
					goto setErr;
				}

				strVal = req_get_cstream_var(wp, (str_tmp), "");
				if(strVal[0]){
					if(queue_entry.QueueEnable != 1){
						//flag1 ==1,need to modify queue status
						flag1 = 1;
						queue_enable = 1;
					}
				}
				else{
					if(queue_entry.QueueEnable != 0){
						flag1 = 1;
						queue_enable = 0;
					}
				}

				strVal = req_get_cstream_var(wp, (str_weight), "");
				if(policy == 1 && strVal[0]){
					if(atoi(strVal) != queue_entry.Weight){
						//flag2 == 1,need to modify queue weight
						flag2 = 1;
						queue_weight = atoi(strVal);
					}
				}

				if(flag1 == 1 || flag2 == 1){
					memset(&qosArray[0], 0, sizeof(QOSQUEUE_T));
			    	memset(&qosArray[1], 0, sizeof(QOSQUEUE_T));
			    	memcpy(&qosArray[0], &queue_entry, sizeof(QOSQUEUE_T));

					if(flag1 == 1){
						queue_entry.QueueEnable = queue_enable;
					}
				
					if(flag2 == 1){
						queue_entry.Weight = queue_weight;
					}

					memcpy(&qosArray[1], &queue_entry, sizeof(QOSQUEUE_T));
					if(!apmib_set(MIB_QOS_QUEUE_MOD, (void *)&qosArray)){
						strcpy(tmpBuf, ("mod QOS_QUEUE_TBL error!"));
						goto setErr;
			        }
				}	
			}
		}
	}

setOk:
	apmib_update_web(CURRENT_SETTING);
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	OK_MSG(submitUrl);
	return ;
	
setErr:
	ERR_MSG(tmpBuf);
}

void formUplinkQosRule(request *wp, char *path, char *query)
{
	char *submitUrl, *submitUrl_uplink, *returnUplink; 
	char *strApply, *strClsId, *strTypeId;
	char *strAddCls, *strDelCls, *strAddType, *strDelType;
	char *strQosOperation, *strQueue, *strDscpRemark, *strDot1pRemark;
	char *strType, *strMin, *strMax, *strProto;
	char tmpBuf[100] = {0}, tmpMin[64] = {0}, tmpMax[64] = {0}, typeName[MAX_TYPE_NAME_LEN] = {0};
	int number = 0, index = 0;
	int class_id = 0, type_id = 0;
	unsigned int qos_operation = 0;

	returnUplink = req_get_cstream_var(wp, ("ReturnUplink"), "");
	submitUrl_uplink = req_get_cstream_var(wp, ("submitUrl_uplink"), "");
	//click return button on webpage,returb to uplink webpage
	if(returnUplink[0] && submitUrl_uplink[0]){
		send_redirect_perm(wp, submitUrl_uplink);
	}

	//get current class id and type id
	strClsId = req_get_cstream_var(wp, ("current_classid"), "");
	if(strClsId[0]){
		class_id = atoi(strClsId);

		if(!apmib_set(MIB_CURRENT_CLASSID, (void *)&class_id)){
            strcpy(tmpBuf, ("set MIB_CURRENT_CLASSID error!"));
			goto setErr;
        }
	}
	else{
		if(!apmib_get(MIB_CURRENT_CLASSID, (void *)&class_id)){
            strcpy(tmpBuf, ("get MIB_CURRENT_CLASSID error!"));
			goto setErr;
        }
	}

	strTypeId = req_get_cstream_var(wp, ("current_typeid"), "");
	if(strTypeId[0]){
		type_id = atoi(strTypeId);

		if(!apmib_set(MIB_CURRENT_TYPEID, (void *)&type_id)){
            strcpy(tmpBuf, ("set MIB_CURRENT_TYPEID error!"));
			goto setErr;
        }
	}
	else{
		if(!apmib_get(MIB_CURRENT_TYPEID, (void *)&type_id)){
            strcpy(tmpBuf, ("get MIB_CURRENT_TYPEID error!"));
			goto setErr;
        }
	}
	//get current class id and type id end

	strAddCls = req_get_cstream_var(wp, ("AddCls"), "");
	if(strAddCls[0]){
		//click add class , only add classInstanceNum now. 
		if(addQosClass() == -1){
			strcpy(tmpBuf, ("CLASS TBL FULL!"));
			goto setErr;
		}

		if(!apmib_set(MIB_UPLINK_QOS_OPERATION, (void *)&qos_operation)){
		    strcpy(tmpBuf, ("set MIB_UPLINK_QOS_OPERATION error!"));
			goto setErr;
		}
	}

	strDelCls = req_get_cstream_var(wp, ("DelCls"), "");
	if(strDelCls[0]){
		if(class_id > 0){
			delQosTypeByClassId(class_id);
			delQosClass(class_id);
			
			if(!apmib_set(MIB_UPLINK_QOS_OPERATION, (void *)&qos_operation)){
			    strcpy(tmpBuf, ("set MIB_UPLINK_QOS_OPERATION error!"));
				goto setErr;
			}
		}
		else{
			strcpy(tmpBuf, ("Get qos current class id error!"));
			goto setErr;
		}
	}

	strAddType = req_get_cstream_var(wp, ("AddType"), "");
	if(strAddType[0]){
		if(class_id > 0){
			//only add typeInstanceNum and typeClassId now
			if(addQosTypeByClassId(class_id) == -1){
				strcpy(tmpBuf, ("TYPE TBL FULL!"));
				goto setErr;
			}
		}
		else{
			strcpy(tmpBuf, ("Get qos current class id error!"));
			goto setErr;
		}
	}

	strDelType = req_get_cstream_var(wp, ("DelType"), "");
	if(strDelType[0]){
		if(class_id > 0 && type_id > 0){
			delQosType(class_id, type_id);
			
			if(!apmib_set(MIB_UPLINK_QOS_OPERATION, (void *)&qos_operation)){
			    strcpy(tmpBuf, ("set MIB_UPLINK_QOS_OPERATION error!"));
				goto setErr;
			}
		}
		else{
			strcpy(tmpBuf, ("Get qos current type id error!"));
			goto setErr;
		}
	}

	// 1: add type  2: edit class  3: edit type
	strQosOperation = req_get_cstream_var(wp, ("qos_operation"), "");
	if(strQosOperation[0]){
		qos_operation = atoi(strQosOperation);

		if(!apmib_set(MIB_UPLINK_QOS_OPERATION, (void *)&qos_operation)){
            strcpy(tmpBuf, ("set MIB_UPLINK_QOS_OPERATION error!"));
			goto setErr;
        }
	}

	strApply = req_get_cstream_var(wp, ("save_apply"), "");
	if(strApply[0]){
		if(!apmib_get(MIB_UPLINK_QOS_OPERATION, (void *)&qos_operation)){
            strcpy(tmpBuf, ("get MIB_UPLINK_QOS_OPERATION error!"));
			goto setErr;
        }

		if(qos_operation == 2){		// edit class
			QOSCLASS_T qosClass, qosArray[2];

			if(!apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&number)){
				strcpy(tmpBuf, ("get MIB_QOS_CLASS_TBL_NUM error!"));
				goto setErr;
			}

			for(index = 1; index <= number; index++){
				memset(&qosClass, '\0', sizeof(QOSCLASS_T));
				*((char *)&qosClass) = (char)index;

				if((apmib_get(MIB_QOS_CLASS_TBL, (void *)&qosClass)) == 0){
					strcpy(tmpBuf, ("get MIB_QOS_CLASS_TBL error!"));
					goto setErr;
				}

				if(qosClass.ClassInstanceNum == class_id){
					break;
				}
			}

			memset(&qosArray[0], 0, sizeof(QOSCLASS_T));
	    	memset(&qosArray[1], 0, sizeof(QOSCLASS_T));
	    	memcpy(&qosArray[0], &qosClass, sizeof(QOSCLASS_T));
			
			strQueue = req_get_cstream_var(wp, ("queue"), "");
			if(strQueue[0]){
				qosClass.ClassQueue = atoi(strQueue);
			}

			strDscpRemark = req_get_cstream_var(wp, ("dscpRemark"), "");
			if(strDscpRemark[0]){
				qosClass.DSCPMark = atoi(strDscpRemark);
			}

			strDot1pRemark = req_get_cstream_var(wp, ("dot1pRemark"), "");
			if(strDot1pRemark[0]){
				qosClass.Dot1pMark = atoi(strDot1pRemark);
			}
			
	        memcpy(&qosArray[1], &qosClass, sizeof(QOSCLASS_T));
			
			//update class_entry for this classid
			if(!apmib_set(MIB_QOS_CLASS_MOD, (void *)&qosArray)){
	            strcpy(tmpBuf, ("mod QOS_CLASS_TBL error!"));
				goto setErr;
	        }
			updateQosRule(class_id);
			
			goto setOk;
		}
		else if(qos_operation == 3){	// edit type
			QOSTYPE_T qosType,qosTypeArray[2];
	
			if(!apmib_get(MIB_QOS_TYPE_TBL_NUM, (void *)&number)){
				strcpy(tmpBuf, ("get MIB_QOS_TYPE_TBL_NUM error!"));
				goto setErr;
			}

			for(index = 1; index <= number; index++){
				memset(&qosType, '\0', sizeof(QOSTYPE_T));
				*((char *)&qosType) = (char)index;

				if((apmib_get(MIB_QOS_TYPE_TBL, (void *)&qosType)) == 0){
					strcpy(tmpBuf, ("get MIB_QOS_TYPE_TBL error!"));
					goto setErr;
				}

				if(qosType.TypeClassId == class_id && qosType.TypeInstanceNum == type_id){
					memset(&qosTypeArray[0], 0, sizeof(QOSTYPE_T));
			    	memset(&qosTypeArray[1], 0, sizeof(QOSTYPE_T));
			    	memcpy(&qosTypeArray[0], &qosType, sizeof(QOSTYPE_T));

					strMin = req_get_cstream_var(wp, ("minimum"), "");
					if(strMin[0]){
						strncpy(tmpMin, strMin, strlen(strMin));
					}

					strMax = req_get_cstream_var(wp, ("maximum"), "");
					if(strMax[0]){
						strncpy(tmpMax, strMax, strlen(strMax));
					}

					strProto = req_get_cstream_var(wp, ("protocol"), "");
					if(strProto[0]){
						if(!protoCheck(class_id, strProto)){
							strcpy(tmpBuf, ("Multiple type entries of a specified class must share the same protocol!"));
							goto setErr;
						}
						
						strncpy(qosType.Protocol, strProto, MAX_PROTOCOL_LEN);
					}

					strType = req_get_cstream_var(wp, ("type"), "");
					if(strType[0]){
						if(strncmp(strType, QOS_SMAC, strlen(QOS_SMAC)) == 0 || strncmp(strType, QOS_DMAC, strlen(QOS_DMAC)) == 0){
							// TODO: validity check
							
							if(strlen(tmpMin) != 17 || strlen(tmpMax) != 17){
								strcpy(tmpBuf, ("MAC input fail!"));
								goto setErr;
							}
						}
						else if(strncmp(strType, QOS_SIP, strlen(QOS_SIP)) == 0 || strncmp(strType, QOS_DIP, strlen(QOS_DIP)) == 0){
							// TODO: validity check

							if(strlen(tmpMin) == 0 || strlen(tmpMax) == 0){
								strcpy(tmpBuf, ("ip input fail!"));
								goto setErr;
							}
						}
						else if(strncmp(strType, QOS_SPORT, strlen(QOS_SPORT)) == 0 || strncmp(strType, QOS_DPORT, strlen(QOS_DPORT)) == 0){
							if(atoi(tmpMin) < 0 || atoi(tmpMin) > 0xFFFF || atoi(tmpMax) < 0 || atoi(tmpMax) > 0xFFFF){
								strcpy(tmpBuf, ("port out of range!"));
								goto setErr;
							}
						}
						else if(strncmp(strType, QOS_DSCP, strlen(QOS_DSCP)) == 0){
							if(!dscpCheck(class_id, 1)){
								strcpy(tmpBuf, ("A class entry can only have either dscp or tos type entry!"));
								goto setErr;
							}
							
							if(atoi(tmpMin) < 0 || atoi(tmpMin) > 0x3F || atoi(tmpMax) < 0 || atoi(tmpMax) > 0x3F){
								strcpy(tmpBuf, ("dscp out of range!"));
								goto setErr;
							}
						}
						else if(strncmp(strType, QOS_TOS, strlen(QOS_TOS)) == 0){
							if(!dscpCheck(class_id, 2)){
								strcpy(tmpBuf, ("A class entry can only have either dscp or tos type entry!"));
								goto setErr;
							}
							
							if(atoi(tmpMin) < 0 || atoi(tmpMin) > 0xFF || atoi(tmpMax) < 0 || atoi(tmpMax) > 0xFF){
								strcpy(tmpBuf, ("tos out of range!"));
								goto setErr;
							}
						}
						else if(strncmp(strType, QOS_DOT1P, strlen(QOS_DOT1P)) == 0){
							if(atoi(tmpMin) < 0 || atoi(tmpMin) > 7 || atoi(tmpMax) < 0 || atoi(tmpMax) > 7){
								strcpy(tmpBuf, ("802.1p out of range!"));
								goto setErr;
							}
						}
					#if 0
						else if(strncmp(strType, QOS_WAN, strlen(QOS_WAN)) == 0){
							// TODO: validity check
						}
						else if(strncmp(strType, QOS_LAN, strlen(QOS_LAN)) == 0){
							// TODO: validity check
						}
					#endif
						else{
							strcpy(tmpBuf, ("invalid qos class type!"));
							goto setErr;
						}

						if(!typeValidCheck(class_id, type_id, strType)){
							snprintf(tmpBuf, sizeof(tmpBuf), "A class entry can only have one %s type entry!", strType);
							goto setErr;
						}
						
						strncpy(typeName, strType, strlen(strType));
						snprintf(qosType.TypeName, sizeof(qosType.TypeName), "%s", typeName);
						
						snprintf(qosType.TypeMin, sizeof(qosType.TypeMin), "%s", tmpMin);
						snprintf(qosType.TypeMax, sizeof(qosType.TypeMax), "%s", tmpMax);
					}

					memcpy(&qosTypeArray[1], &qosType, sizeof(QOSTYPE_T));		
					if(!apmib_set(MIB_QOS_TYPE_MOD, (void *)&qosTypeArray)){
			            strcpy(tmpBuf, ("mod QOS_TYPE_TBL error!"));
						goto setErr;
			        }
					
					updateQosRule(class_id);
					break;
				}
			}
		}
	}

setOk:
	apmib_update_web(CURRENT_SETTING);
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	 // hidden page
	OK_MSG(submitUrl);
	return ;
	
setErr:
	ERR_MSG(tmpBuf);
}
#endif
void formShapingQoS(request *wp, char *path, char *query)
{
	char *submitUrl, *strApply;
	char *strAddCar, *strDelCar;
	char *strUpMode, *strDownMode;
	char *strQosOperation, *strCarId;
	char *strRate, *strType, *strType2;
	char tmpBuf[100] = {0};
	unsigned int qos_flag = 0, upMode = -1, downMode = -1;
	unsigned int carNum = 0, index = 0, index2 = 0;
	unsigned int qosUpOp = 0, carUpId = 0, val = 0;
	unsigned int qosDownOp = 0, carDownId = 0;

	
	strApply = req_get_cstream_var(wp, ("save_apply"), "");
	#if 0
	//if(strApply[0]){
	if(1){
		if(strFlag[0] && !strcmp(strFlag, "ON"))
			qos_flag = 1;
		else
			qos_flag = 0;

		if(!apmib_set(MIB_QOS_ENABLE, (void *)&qos_flag)) {
			strcpy(tmpBuf, ("set MIB_QOS_ENABLE error!"));
			goto setErr;
		}

		if(!qos_flag){
			goto setOk;
		}
	}
	#endif

	strUpMode = req_get_cstream_var(wp, ("up_shaping"), "");
	strDownMode = req_get_cstream_var(wp, ("down_shaping"), "");
	if(strUpMode[0] || strDownMode[0]){
		//0:close 1: based on interface 3:based on ip
		QOSCAR_T qosCar, qosCarArray[2];
		memset(&qosCarArray[0], 0, sizeof(QOSCAR_T));
	    memset(&qosCarArray[1], 0, sizeof(QOSCAR_T));

		if(strUpMode[0]){
			//uplink shaping open
			upMode = atoi(strUpMode);
		}

		if(strDownMode[0]){
			//downlink shaping open
			downMode = atoi(strDownMode);
		}

		if(!apmib_get(MIB_QOS_CAR_TBL_NUM, (void *)&carNum)){
			strcpy(tmpBuf, ("get MIB_QOS_CAR_TBL_NUM error!"));
			goto setErr;
		}

		for(index = 1; index <= carNum; index++){
			memset(&qosCar, '\0', sizeof(QOSCAR_T));
			*((char *)&qosCar) = (char)index;

			if((apmib_get(MIB_QOS_CAR_TBL, (void *)&qosCar)) == 0){
				strcpy(tmpBuf, ("get MIB_QOS_CAR_TBL error!"));
				goto setErr;
			}

			if((upMode != -1 && qosCar.SpeedLimitMode_UP != upMode) || 
			(downMode != -1 && qosCar.SpeedLimitMode_DOWN != downMode)){
				//mode in qosCarTable != setting,update
				memcpy(&qosCarArray[0], &qosCar, sizeof(QOSCAR_T));

				if(strUpMode[0]){
					qosCar.SpeedLimitMode_UP = upMode;
				}

				if(strDownMode[0]){
					qosCar.SpeedLimitMode_DOWN = downMode;
				}
				
				memcpy(&qosCarArray[1], &qosCar, sizeof(QOSCAR_T));

				if(!apmib_set(MIB_QOS_CAR_MOD, (void *)&qosCarArray)){
		            strcpy(tmpBuf, ("mod QOS_CAR_TBL error!"));
					goto setErr;
		        }
				break;
			}
			else{
				//upMode && downMode == -1 ,mode no change ,use old mode
				upMode = qosCar.SpeedLimitMode_UP;
				downMode = qosCar.SpeedLimitMode_DOWN;
			}
		}

		//printf("[%s:%d] upMode: %d, downMode: %d\n", __FUNCTION__, __LINE__, upMode, downMode);
		
		if(upMode == 0 && downMode == 0){
			goto setOk;	// shaping disable
		}

		strAddCar = req_get_cstream_var(wp, ("AddUpCar"), "");
		if(strAddCar[0]){
			//record CAR_TBL id to SHAPING_TBL entry and set CarInstanceNum(reord cur_car_id) now
			if(addQosCar(upMode, OUT_BOUND, NULL, NULL, 0) == -1){
				strcpy(tmpBuf, ("SHAPING TBL FULL!"));
				goto setErr;
			}
			goto setOk;
		}

		strAddCar = req_get_cstream_var(wp, ("AddDownCar"), "");
		if(strAddCar[0]){
			if(addQosCar(downMode, IN_BOUND, NULL, NULL, 0) == -1){
				strcpy(tmpBuf, ("SHAPING TBL FULL!"));
				goto setErr;
			}
			goto setOk;
		}
		
		strQosOperation = req_get_cstream_var(wp, ("car_up_op"), "");
		//car_up_op : 1 //edit car
		if(strQosOperation[0]){
			qosUpOp = atoi(strQosOperation);
		}
		else{
			qosUpOp = 0;
		}
		
		strQosOperation = req_get_cstream_var(wp, ("car_down_op"), "");
		if(strQosOperation[0]){
			qosDownOp = atoi(strQosOperation);
		}
		else{
			qosDownOp = 0;
		}

		//printf("[%s:%d] qosUpOp: %d, qosDownOp: %d\n", __FUNCTION__, __LINE__, qosUpOp, qosDownOp);

		//make upOp and downOp to one variable
		val = ((qosUpOp & 0xF) << 4) | (qosDownOp & 0xF);
		if(!apmib_set(MIB_SHAPING_QOS_OEPRATION, (void *)&val)){
	        strcpy(tmpBuf, ("set MIB_SHAPING_QOS_OEPRATION error!"));
			goto setErr;
	    }

		strCarId = req_get_cstream_var(wp, ("current_carUpId"), "");
		if(strCarId[0]){
			carUpId  = atoi(strCarId);
		}
		else{
			if(!apmib_get(MIB_CURRENT_CARID, (void *)&carUpId)){
	            strcpy(tmpBuf, ("get MIB_CURRENT_CARID error!"));
				goto setErr;
	        }

			carUpId = (carUpId >> 4) & 0xF;
		}

		strCarId = req_get_cstream_var(wp, ("current_carDownId"), "");
		if(strCarId[0]){
			carDownId  = atoi(strCarId);
		}
		else{
			if(!apmib_get(MIB_CURRENT_CARID, (void *)&carDownId)){
				strcpy(tmpBuf, ("get MIB_CURRENT_CARID error!"));
				goto setErr;
			}

			carDownId = carDownId & 0xF;
		}

		val = ((carUpId & 0xF) << 4) | (carDownId & 0xF);

		//printf("[%s:%d] carUpId: %d, carDownId: %d\n", __FUNCTION__, __LINE__, carUpId, carDownId);
		if(!apmib_set(MIB_CURRENT_CARID, (void *)&val)){
            strcpy(tmpBuf, ("set MIB_CURRENT_CARID error!"));
			goto setErr;
        }

		//click delete button on webpage
		strDelCar = req_get_cstream_var(wp, ("DelUpCar"), "");
		if(strDelCar[0]){
			if(carUpId > 0){
				delQosCar(upMode, OUT_BOUND, carUpId);
				goto setOk;
			}
			else{
				strcpy(tmpBuf, ("Get qos current shaping id error!"));
				goto setErr;
			}
		}

		strDelCar = req_get_cstream_var(wp, ("DelDownCar"), "");
		if(strDelCar[0]){
			if(carDownId > 0){
				delQosCar(downMode, IN_BOUND, carDownId);
				goto setOk;
			}
			else{
				strcpy(tmpBuf, ("Get qos current shaping id error!"));
				goto setErr;
			}
		}

		if(strApply[0]){
			//click save on the webpage
			int up_rate = 0, down_rate = 0;
			int mib_num = 0, mib_tbl = 0, mib_mod = 0;
			int car_id = 0, rate = 0;

			strRate = req_get_cstream_var(wp, ("up_rate"), "");
			if(strRate[0]){
				up_rate = atoi(strRate);
			}

			strRate = req_get_cstream_var(wp, ("down_rate"), "");
			if(strRate[0]){
				down_rate = atoi(strRate);
			}

			//printf("[%s:%d] up_rate: %d, down_rate: %d\n", __FUNCTION__, __LINE__, up_rate, down_rate);
		
			if(upMode == 1 || downMode == 1){
				//up shaping based on interface
				QOSCAR_IFACE_T car_iface, carArray[2];

				for(index = OUT_BOUND; index <= IN_BOUND; index++){
					if(index == OUT_BOUND && upMode == 1){
						mib_num = MIB_QOS_CAR_UP_IFACE_TBL_NUM;
						mib_tbl = MIB_QOS_CAR_UP_IFACE_TBL;
						mib_mod = MIB_QOS_CAR_UP_IFACE_MOD;
						car_id = carUpId;
						rate = up_rate;
					}
					else if(index == IN_BOUND && downMode == 1){
						mib_num = MIB_QOS_CAR_DOWN_IFACE_TBL_NUM;
						mib_tbl = MIB_QOS_CAR_DOWN_IFACE_TBL;
						mib_mod = MIB_QOS_CAR_DOWN_IFACE_MOD;
						car_id = carDownId;
						rate = down_rate;
					}
					else{
						continue;
					}

					if(rate == 0)
						continue;

					if(!apmib_get(mib_num, (void *)&carNum)){
						strcpy(tmpBuf, ("get MIB_QOS_CAR_TBL_NUM fail!"));
						goto setErr;
					}

					for (index2 = 1; index2 <= carNum; index2++){
						*((char *)&car_iface) = (char)index2;
						if (!apmib_get(mib_tbl, (void *)&car_iface)){
							strcpy(tmpBuf, ("get MIB_QOS_CAR_TBL fail!"));
							goto setErr;
				        }

						if (car_iface.CarIfaceInstanceNum == car_id){
							memset(&carArray[0], 0, sizeof(QOSCAR_IFACE_T));
			    			memset(&carArray[1], 0, sizeof(QOSCAR_IFACE_T));
			    			memcpy(&carArray[0], &car_iface, sizeof(QOSCAR_IFACE_T));

							car_iface.CarIfaceShaping = rate;
							if(index == OUT_BOUND){
								//for based_on interface, type is lan1/lan2...
								strType = req_get_cstream_var(wp, ("type"), "");
							}
							else{
								strType = req_get_cstream_var(wp, ("down_type"), "");
							}
							
							if(strType[0]){
								strncpy(car_iface.CarIfaceName, strType, MAX_IFACE_NAME_LEN);
							}

			        		memcpy(&carArray[1], &car_iface, sizeof(QOSCAR_IFACE_T));
							if(!apmib_set(mib_mod, (void *)&carArray)){
					            strcpy(tmpBuf, ("mod QOS_CAR_IFACE_TBL error!"));
								goto setErr;
					        }

							if(index == OUT_BOUND){
								//update car tbl
								updateCarTbl(OUT_BOUND, upMode);
							}
							else{
								updateCarTbl(IN_BOUND, downMode);
							}
							break;
						}
					}
					
				}
			}

			if(upMode == 2 || downMode == 2){
				QOSCAR_VLAN_T car_vlan, carArray[2];

				for(index = OUT_BOUND; index <= IN_BOUND; index++){
					if(index == OUT_BOUND && upMode == 2){
						mib_num = MIB_QOS_CAR_UP_VLAN_TBL_NUM;
						mib_tbl = MIB_QOS_CAR_UP_VLAN_TBL;
						mib_mod = MIB_QOS_CAR_UP_VLAN_MOD;
						car_id = carUpId;
						rate = up_rate;
					}
					else if(index == IN_BOUND && downMode == 2){
						mib_num = MIB_QOS_CAR_DOWN_VLAN_TBL_NUM;
						mib_tbl = MIB_QOS_CAR_DOWN_VLAN_TBL;
						mib_mod = MIB_QOS_CAR_DOWN_VLAN_MOD;
						car_id = carDownId;
						rate = down_rate;
					}
					else{
						continue;
					}

					if(rate == 0)
						continue;

					if(!apmib_get(mib_num, (void *)&carNum)){
						strcpy(tmpBuf, ("get MIB_QOS_CAR_TBL_NUM fail!"));
						goto setErr;
					}

					for (index2 = 1; index2 <= carNum; index2++){
						*((char *)&car_vlan) = (char)index2;
						if (!apmib_get(mib_tbl, (void *)&car_vlan)){
							strcpy(tmpBuf, ("get MIB_QOS_CAR_TBL fail!"));
							goto setErr;
				        }

						if (car_vlan.CarVlanInstanceNum == car_id){
							memset(&carArray[0], 0, sizeof(QOSCAR_VLAN_T));
			    			memset(&carArray[1], 0, sizeof(QOSCAR_VLAN_T));
			    			memcpy(&carArray[0], &car_vlan, sizeof(QOSCAR_VLAN_T));

							car_vlan.CarVlanShaping = rate;
							if(index == OUT_BOUND){
								strType = req_get_cstream_var(wp, ("type"), "");
							}
							else{
								strType = req_get_cstream_var(wp, ("down_type"), "");
							}
							if(strType[0]){
								car_vlan.CarVlanid = atoi(strType);
							}

			        		memcpy(&carArray[1], &car_vlan, sizeof(QOSCAR_VLAN_T));
							if(!apmib_set(mib_mod, (void *)&carArray)){
					            strcpy(tmpBuf, ("mod QOS_CAR_VLAN_TBL error!"));
								goto setErr;
					        }

							if(index == OUT_BOUND){
								updateCarTbl(OUT_BOUND, upMode);
							}
							else{
								updateCarTbl(IN_BOUND, downMode);
							}
					
							break;
						}
					}
				}
			}

			if(upMode == 3 || downMode == 3){
				QOSCAR_IP_T car_ip, carArray[2];
				char strTmp[40] = {0};

				for(index = OUT_BOUND; index <= IN_BOUND; index++){
					if(index == OUT_BOUND && upMode == 3){
						mib_num = MIB_QOS_CAR_UP_IP_TBL_NUM;
						mib_tbl = MIB_QOS_CAR_UP_IP_TBL;
						mib_mod = MIB_QOS_CAR_UP_IP_MOD;
						car_id = carUpId;
						rate = up_rate;
					}
					else if(index == IN_BOUND && downMode == 3){
						mib_num = MIB_QOS_CAR_DOWN_IP_TBL_NUM;
						mib_tbl = MIB_QOS_CAR_DOWN_IP_TBL;
						mib_mod = MIB_QOS_CAR_DOWN_IP_MOD;
						car_id = carDownId;
						rate = down_rate;
					}
					else{
						continue;
					}

					if(rate == 0)
						continue;

					if(!apmib_get(mib_num, (void *)&carNum)){
						strcpy(tmpBuf, ("get MIB_QOS_CAR_TBL_NUM fail!"));
						goto setErr;
					}

					for (index2 = 1; index2 <= carNum; index2++){
						*((char *)&car_ip) = (char)index2;
						if (!apmib_get(mib_tbl, (void *)&car_ip)){
							strcpy(tmpBuf, ("get MIB_QOS_CAR_IP_TBL fail!"));
							goto setErr;
				        }

						if (car_ip.CarIpInstanceNum == car_id){
							memset(&carArray[0], 0, sizeof(QOSCAR_IP_T));
			    			memset(&carArray[1], 0, sizeof(QOSCAR_IP_T));
			    			memcpy(&carArray[0], &car_ip, sizeof(QOSCAR_IP_T));

							car_ip.CarIpShaping = rate;

							if(index == OUT_BOUND){
								strType = req_get_cstream_var(wp, ("type"), "");
							}
							else{
								strType = req_get_cstream_var(wp, ("down_type"), "");
							}
							if(strType[0]){
								strncpy(strTmp, strType, strlen(strType));
								inet_aton(strTmp, (struct in_addr *)&car_ip.CarIpStart);
							}

							if(index == OUT_BOUND){
								strType = req_get_cstream_var(wp, ("type2"), "");
							}
							else{
								strType = req_get_cstream_var(wp, ("down_type2"), "");
							}
							if(strType[0]){
								strncpy(strTmp, strType, strlen(strType));
								inet_aton(strTmp, (struct in_addr *)&car_ip.CarIpEnd);
							}

			        		memcpy(&carArray[1], &car_ip, sizeof(QOSCAR_IP_T));

							if(!apmib_set(mib_mod, (void *)&carArray)){
					            strcpy(tmpBuf, ("mod QOS_CAR_IP_TBL error!"));
								goto setErr;
					        }

							if(index == OUT_BOUND){
								updateCarTbl(OUT_BOUND, upMode);
							}
							else{
								updateCarTbl(IN_BOUND, downMode);
							}
					
							break;
						}
					}
				}
			}

		#if defined(CONFIG_MAC_SHAPING_QOS)
			if(upMode == 4 || downMode == 4){
				QOSCAR_MAC_T car_mac, carArray[2];

				for(index = OUT_BOUND; index <= IN_BOUND; index++){
					if(index == OUT_BOUND && upMode == 4){
						mib_num = MIB_QOS_CAR_UP_MAC_TBL_NUM;
						mib_tbl = MIB_QOS_CAR_UP_MAC_TBL;
						mib_mod = MIB_QOS_CAR_UP_MAC_MOD;
						car_id = carUpId;
						rate = up_rate;
					}
					else if(index == IN_BOUND && downMode == 4){
						mib_num = MIB_QOS_CAR_DOWN_MAC_TBL_NUM;
						mib_tbl = MIB_QOS_CAR_DOWN_MAC_TBL;
						mib_mod = MIB_QOS_CAR_DOWN_MAC_MOD;
						car_id = carDownId;
						rate = down_rate;
					}
					else{
						continue;
					}

					if(rate == 0)
						continue;

					if(!apmib_get(mib_num, (void *)&carNum)){
						strcpy(tmpBuf, ("get MIB_QOS_CAR_TBL_NUM fail!"));
						goto setErr;
					}

					for (index2 = 1; index2 <= carNum; index2++){
						*((char *)&car_mac) = (char)index2;
						if (!apmib_get(mib_tbl, (void *)&car_mac)){
							strcpy(tmpBuf, ("get MIB_QOS_CAR_TBL fail!"));
							goto setErr;
				        }

						if (car_mac.CarMacInstanceNum == car_id){
							memset(&carArray[0], 0, sizeof(QOSCAR_MAC_T));
			    			memset(&carArray[1], 0, sizeof(QOSCAR_MAC_T));
			    			memcpy(&carArray[0], &car_mac, sizeof(QOSCAR_MAC_T));

							car_mac.CarMacShaping = rate;
							if(index == OUT_BOUND){
								strType = req_get_cstream_var(wp, ("type"), "");
							}
							else{
								strType = req_get_cstream_var(wp, ("down_type"), "");
							}
							if(strType[0]){
								unsigned char *buf;
								unsigned char str_mac[13] = {0};
								unsigned int i = 0;

								buf = strType;
								if(strlen(buf) == 17){
									i = 0;
									while(*buf){
										if(*buf != ':'){
											str_mac[i++] = *buf;
										}
										++buf;
									}
									str_mac[i] = '\0';
									if(!string_to_hex(str_mac, car_mac.CarMacAddr, 12)){
										strcpy(tmpBuf, ("Mac Addr Invalid!"));
										goto setErr;
									}	
								}
							}

			        		memcpy(&carArray[1], &car_mac, sizeof(QOSCAR_MAC_T));
							if(!apmib_set(mib_mod, (void *)&carArray)){
					            strcpy(tmpBuf, ("mod QOS_CAR_MAC_TBL error!"));
								goto setErr;
					        }

							if(index == OUT_BOUND){
								updateCarTbl(OUT_BOUND, upMode);
							}
							else{
								updateCarTbl(IN_BOUND, downMode);
							}
					
							break;
						}
					}
				}
			}
		#endif
		}
	}

setOk:
	apmib_update_web(CURRENT_SETTING);
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	OK_MSG(submitUrl);
	return ;
	
setErr:
	ERR_MSG(tmpBuf);
}


#elif defined(MULTI_WAN_QOS)
#define outBound 		0x1
#define inBound 		0x2

#define QUE_TBL_EMPTY 	0x0
#define SP_POLICY 		0x1
#define WFQ_POLICY 		0x2
#define SHAPING_POLICY	0x3

int configWanShaping(int wanIndex, int autoUp, int autoDown, 
		unsigned long manuUp, unsigned long manuDown, char *tmpBuf)
{
#if 0	// updating wanIface tbl will raise wan event up.
	WANIFACE_T entry[2];
	memset(&entry[0], 0, sizeof(WANIFACE_T));
	memset(&entry[1], 0, sizeof(WANIFACE_T));

	*((char *)&entry[0]) = (char)wanIndex;
	if(!apmib_get(MIB_WANIFACE_TBL, (void *)&entry[0])){
		strncpy(tmpBuf, "get MIB_WANIFACE_TBL fail!", sizeof(tmpBuf)-1);
		return 0;
	}

	memcpy(&entry[1], &entry[0], sizeof(WANIFACE_T));

	entry[1].qosAutoUpLinkSpeed 	= autoUp;
	entry[1].qosAutoDownLinkSpeed	= autoDown;
	entry[1].qosManualUpLinkSpeed	= manuUp;
	entry[1].qosManualDownLinkSpeed = manuDown;

	if (apmib_set(MIB_WANIFACE_MOD, (void *)&entry) == 0) {
		strncpy(tmpBuf, "set MIB_WANIFACE_MOD fail!", sizeof(tmpBuf)-1);
		return 0;
	}
#else
	if(!apmib_set( MIB_WANIFACE_CURRENT_IDX, (void *)&wanIndex)){
		strcpy(tmpBuf, ("set wan interface item index fail!"));
		return 0;
	}

	if(!apmib_set(MIB_WANIFACE_QOS_AUTO_UL_SPEED,(void*)&autoUp)){
		strcpy(tmpBuf, "set wan interface qos auto uplink speed fail!");
		return 0;;
	}

	if(!apmib_set(MIB_WANIFACE_QOS_AUTO_DL_SPEED,(void*)&autoDown)){
		strcpy(tmpBuf, "set wan interface qos  auto downlink speed fail!");
		return 0;
	}

	if(!apmib_set(MIB_WANIFACE_QOS_MANUAL_UL_SPEED,(void*)&manuUp)){
		strcpy(tmpBuf, "set wan interface qos manual uplink speed  fail!");
		return 0;
	}

	if(!apmib_set(MIB_WANIFACE_QOS_MANUAL_DL_SPEED,(void*)&manuDown)){
		strcpy(tmpBuf, "set wan interface qos manual downlink speed   fail!");
		return 0;
	}
#endif
	return 1;
}

int getWanShaping(int wanIndex, int direction, char *tmpBuf)
{
	int shapingRate = 100000;	// kbps
#if defined (CONFIG_RTL_8198)|| defined (CONFIG_RTL_8198C) || defined (CONFIG_RTL_8367R_SUPPORT)
	shapingRate = 1000000;
#endif

#if 1
	WANIFACE_T entry;
	memset(&entry, 0, sizeof(WANIFACE_T));

	*((char *)&entry) = (char)wanIndex;
	if(!apmib_get(MIB_WANIFACE_TBL, (void *)&entry)){
		strncpy(tmpBuf, "get MIB_WANIFACE_TBL fail!", sizeof(tmpBuf)-1);
		return 0;
	}

	if(direction == outBound){
		if(entry.qosAutoUpLinkSpeed == 0){
			return entry.qosManualUpLinkSpeed;
		}
		else{
			return shapingRate;
		}
	}
	else{
		if(entry.qosAutoDownLinkSpeed == 0){
			return entry.qosManualDownLinkSpeed;
		}
		else{
			return shapingRate;
		}
	}
#else
	int autoRate = 0;

	if(!apmib_set( MIB_WANIFACE_CURRENT_IDX, (void *)&wanIndex)){
		strcpy(tmpBuf, ("set wan interface item index fail!"));
		return 0;
	}

	if(direction == outBound){
		if(!apmib_get(MIB_WANIFACE_QOS_AUTO_UL_SPEED,(void*)&autoRate)){
			strcpy(tmpBuf, "get wan interface qos auto uplink speed fail!");
			return 0;;
		}

		if(autoRate == 0){
			if(!apmib_get(MIB_WANIFACE_QOS_MANUAL_UL_SPEED,(void*)&shapingRate)){
				strcpy(tmpBuf, "get wan interface qos manual uplink speed  fail!");
				return 0;
			}
		}
	}

	if(direction == inBound){
		if(!apmib_get(MIB_WANIFACE_QOS_AUTO_DL_SPEED,(void*)&autoRate)){
			strcpy(tmpBuf, "get wan interface qos auto uplink speed fail!");
			return 0;;
		}

		if(autoRate == 0){
			if(!apmib_get(MIB_WANIFACE_QOS_MANUAL_DL_SPEED,(void*)&shapingRate)){
				strcpy(tmpBuf, "get wan interface qos manual uplink speed  fail!");
				return 0;
			}
		}
	}

	return shapingRate;
#endif
}
#if defined(CONFIG_IPV6)
int configWanQosStatus(int wanIndex, int wanQosFlag, int wanV6QosFlag)
#else
int configWanQosStatus(int wanIndex, int wanQosFlag)
#endif
{
#if 0	// updating wanIface tbl will raise wan event up.
	WANIFACE_T entry[2];
	memset(&entry[0], 0, sizeof(WANIFACE_T));
	memset(&entry[1], 0, sizeof(WANIFACE_T));

	*((char *)&entry[0]) = (char)wanIndex;
	if(!apmib_get(MIB_WANIFACE_TBL, (void *)&entry[0])){
		printf("[%s:%d] get MIB_WANIFACE_TBL fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	memcpy(&entry[1], &entry[0], sizeof(WANIFACE_T));
	
	entry[1].wanQosEnable = 0;
#if defined(CONFIG_IPV6)
	entry[1].wanQosV6Enable = 0;
#endif

	if(entry[1].enable == 1){
		entry[1].wanQosEnable = wanQosFlag;

#if defined(CONFIG_IPV6)
		if(entry[1].ipv6Enable == 1)
			entry[1].wanQosV6Enable = wanV6QosFlag;
#endif
	}

	if (apmib_set(MIB_WANIFACE_MOD, (void *)&entry) == 0) {
		printf("set MIB_WANIFACE_MOD fail!");
		return 0;
	}
#else
	int wanStatus = 0;
	
	if(!apmib_set( MIB_WANIFACE_CURRENT_IDX, (void *)&wanIndex)){
		printf("set wan interface item index fail!");
		return 0;
	}

	if(!apmib_get( MIB_WANIFACE_ENABLE, (void *)&wanStatus)){
		printf("get wan interface enable fail!");
		return 0;
	}

	if(wanStatus == 1){
		if(!apmib_set(MIB_WAN_QOS_ENABLE,(void*)&wanQosFlag)){
			printf("set wan interface qos enable fail!");
			return 0;
		}

		#if defined(CONFIG_IPV6)
		if(!apmib_set(MIB_WAN_QOS_V6_ENABLE,(void*)&wanV6QosFlag)){
			printf("set wan interface qos enable fail!");
			return 0;
		}
		#endif
	}
	else{
		//if wan is disabled, related wan qos flag should be disabled too.
		wanStatus = 0;
		if(!apmib_set(MIB_WAN_QOS_ENABLE,(void*)&wanStatus)){
			printf("set wan interface qos enable fail!");
			return 0;
		}

		#if defined(CONFIG_IPV6)
		if(!apmib_set(MIB_WAN_QOS_V6_ENABLE,(void*)&wanStatus)){
			printf("set wan interface qos enable fail!");
			return 0;
		}
		#endif
	}
#endif
	return 1;
}

int configQueueStatus(int wanIndex, int queStats)
{
	char wanIface[8] = {0};
	//int entryNum = 0, index = 0, wanStatus = 0;
	int entryNum = 0, index = 0;
	QOSQUEUE_T entry[2];
	WANIFACE_T wanEntry;

	snprintf(wanIface, sizeof(wanIface),  "WAN%d", wanIndex);

	if(!apmib_get(MIB_QOS_QUEUE_TBL_NUM, (void *)&entryNum)){
		return -1;
	}
	
	if(entryNum < 1)
		return 1;

#if 1
	memset(&wanEntry, 0, sizeof(WANIFACE_T));

	*((char *)&wanEntry) = (char)wanIndex;
	if(!apmib_get(MIB_WANIFACE_TBL, (void *)&wanEntry)){
		printf("[%s:%d] get MIB_WANIFACE_TBL fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}
#else
	if(!apmib_set( MIB_WANIFACE_CURRENT_IDX, (void *)&wanIndex)){
		printf("[%s:%d] set wan interface item index fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	if(!apmib_get( MIB_WANIFACE_ENABLE, (void *)&wanStatus)){
		printf("[%s:%d] get wan interface enable fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}
#endif

	for(index = 1; index<=entryNum; index++){
		memset(&entry[0], 0, sizeof(QOSQUEUE_T));
		memset(&entry[1], 0, sizeof(QOSQUEUE_T));
			
		*((char *)&entry[0]) = (char)index;
		if(!apmib_get(MIB_QOS_QUEUE_TBL, (void *)&entry[0])){
			printf("[%s:%d] get queue tbl err.\n", __FUNCTION__, __LINE__);
			return 0;
		}

		memcpy(&entry[1], &entry[0], sizeof(QOSQUEUE_T));

		if(strcmp(entry[1].QueueWanInterface, wanIface) == 0){
			//init QueueStatus 0
			entry[1].QueueEnable = 0;

			//if(wanStatus == 1){
			if(wanEntry.enable == 1){
				//if wan qos enable, then record queue status
				entry[1].QueueEnable = queStats;
			}

			if (!apmib_set(MIB_QOS_QUEUE_MOD, (void *)&entry)) {
				printf("[%s:%d] modify queue tbl err.\n", __FUNCTION__, __LINE__);
				return 0;
			}
		}
	}

	return 1;
}

int updateQueShaping(int queueType, unsigned long upRate, unsigned long downRate)
{
	int entryNum = 0, intVal = 1;
	QOSQUEUE_T entry[2];
	
	if(!apmib_get(MIB_QOS_QUEUE_TBL_NUM, (void *)&entryNum)){
		printf("get qos queue table number error!");
		return 0;
	}

	if(entryNum < 1)
		return 1;

	/* update queue shaping rate */
	for(intVal = 1; intVal<=entryNum; intVal++){
		memset(&entry[0], 0, sizeof(QOSQUEUE_T));
		memset(&entry[1], 0, sizeof(QOSQUEUE_T));
		
		*((char *)&entry[0]) = (char)intVal;
		if (!apmib_get(MIB_QOS_QUEUE_TBL, (void *)&entry[0])){
			printf("[%s:%d] get queue tbl err.\n", __FUNCTION__, __LINE__);
			return 0;
		}

		memcpy(&entry[1], &entry[0], sizeof(QOSQUEUE_T));

		/* uplink */
		if(strncmp(entry[1].QueueInterface, "WAN", 3) == 0){

			if((queueType != SHAPING_POLICY) || (entry[1].GuaranMinRate != 1000))	// SP, WFQ, GM need to be adjusted
				entry[1].ShapingRate = upRate * 1000;		// from kbps -> bps
		}
	
		/* downlink */
		if(strcmp(entry[1].QueueInterface, "LAN") == 0){
			if((queueType != SHAPING_POLICY) || (entry[1].GuaranMinRate != 1000))	// SP, WFQ, GM need to be adjusted
				entry[1].ShapingRate = downRate * 1000;
		}
		
		if (apmib_set(MIB_QOS_QUEUE_MOD, (void *)&entry) == 0) {
			printf("[%s:%d] modify queue tbl err.\n", __FUNCTION__, __LINE__);
			return 0;
		}
	}

	return 1;
}
#if defined(CONFIG_IPV6)
int configClassStatus(int wanIndex, int clsStats, int v6QosFlag)
#else
int configClassStatus(int wanIndex, int clsStats)
#endif
{
	char wanIface[8] = {0};
	//int entryNum = 0, index = 0, wanStatus = 0;
	int entryNum = 0, index = 0;
	QOSCLASS_T entry[2];
#if 0 //defined(CONFIG_IPV6)
	int wanV6Status = 0;
#endif
	WANIFACE_T wanEntry;

	snprintf(wanIface, sizeof(wanIface),  "WAN%d", wanIndex);

	if(!apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum)){
		return -1;
	}
	
	if(entryNum < 1)
		return 1;

#if 1
	memset(&wanEntry, 0, sizeof(WANIFACE_T));
	*((char *)&wanEntry) = (char)wanIndex;
	
	if(!apmib_get(MIB_WANIFACE_TBL, (void *)&wanEntry)){
		printf("[%s:%d] get MIB_WANIFACE_TBL fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}
#else
	if(!apmib_set( MIB_WANIFACE_CURRENT_IDX, (void *)&wanIndex)){
		printf("[%s:%d] set wan interface item index fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	if(!apmib_get( MIB_WANIFACE_ENABLE, (void *)&wanStatus)){
		printf("[%s:%d] get wan interface enable fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}

#if defined(CONFIG_IPV6)
	if(!apmib_get( MIB_WANIFACE_IPV6_ENABLE, (void *)&wanV6Status)){
		printf("[%s:%d] get wan interface v6 enable fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}
#endif
#endif

	for(index = 1; index<=entryNum; index++){
		memset(&entry[0], 0, sizeof(QOSCLASS_T));
		memset(&entry[1], 0, sizeof(QOSCLASS_T));
			
		*((char *)&entry[0]) = (char)index;
		if(!apmib_get(MIB_QOS_CLASS_TBL, (void *)&entry[0])){
			printf("[%s:%d] get class tbl err.\n", __FUNCTION__, __LINE__);
			return 0;
		}

		memcpy(&entry[1], &entry[0], sizeof(QOSCLASS_T));

		if(strcmp(entry[1].ClassWanInterface, wanIface) == 0){
			entry[1].ClassificationEnable = 0;
			if(entry[1].IpVersion == 1){	// ipv4
				//if(wanStatus == 1){
				if(wanEntry.enable == 1){
					entry[1].ClassificationEnable = clsStats;
				}
			}
		#if defined(CONFIG_IPV6)
			else if(entry[1].IpVersion == 2){		// ipv6
				//if(wanV6Status == 1){
				if(wanEntry.ipv6Enable == 1){
					entry[1].ClassificationEnable = v6QosFlag;
				}
			}
			else if(entry[1].IpVersion == 4){	// ipv4+ipv6
				//if(wanStatus || wanV6Status)
				if(wanEntry.enable && wanEntry.ipv6Enable){
					entry[1].ClassificationEnable = v6QosFlag;
				}
			}
		#endif

			if (!apmib_set(MIB_QOS_CLASS_MOD, (void *)&entry)) {
				printf("[%s:%d] modify class tbl err.\n", __FUNCTION__, __LINE__);
				return 0;
			}
		}
	}

	return 1;
}

int getClassInstanceNum(void)
{
	unsigned int entryNum = 0, index = 0, maxInsNum = 0;
	QOSCLASS_T entry;
		
	if(!apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum)){
		printf("[%s:%d]get MIB_QOS_CLASS_TBL_NUM error!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	if(entryNum >= 1){
		for(index = 1; index<=entryNum; index++){
			memset(&entry, '\0', sizeof(entry));
			*((char *)&entry) = (char)index;
			if (!apmib_get(MIB_QOS_CLASS_TBL, (void *)&entry)){
				printf("[%s:%d]get MIB_QOS_CLASS_TBL error!\n", __FUNCTION__, __LINE__);
				return 0;
			}

			if(entry.ClassInstanceNum > maxInsNum)
				maxInsNum = entry.ClassInstanceNum;
		}
	}

	return maxInsNum+1;
}

void flushQosStatus(void)
{
	int wanIndex = 0;

	for(wanIndex = 1; wanIndex <= WANIFACE_NUM; wanIndex++){
	#if defined(CONFIG_IPV6)
		configWanQosStatus(wanIndex, 0, 0);
		configClassStatus(wanIndex, 0, 0);
	#else
		configWanQosStatus(wanIndex, 0);
		configClassStatus(wanIndex, 0);
	#endif
		configQueueStatus(wanIndex, 0);
	}
}

int check_queue_type(void)
{
	int index = 1, entryNum = 0;
	char tmp_args[32]={0};
	QOSQUEUE_T entry;

	if(!apmib_get(MIB_QOS_QUEUE_TBL_NUM, (void *)&entryNum)){
		return -1;
	}

	if(entryNum){
		//for(index = 1; index<=entryNum; index++){	
			memset(&entry, '\0', sizeof(entry));
			*((char *)&entry) = (char)index;
			apmib_get(MIB_QOS_QUEUE_TBL, (void *)&entry);

			// entry may be disabled due to wan qos disable
			//if(entry.QueueEnable)		
			{
				snprintf(tmp_args, sizeof(tmp_args),  "%s", entry.SchedulerAlgorithm);

				if((strcmp(tmp_args, "WFQ") == 0) || (strcmp(tmp_args, "WRR") == 0))
					return WFQ_POLICY;
				else if(strcmp(tmp_args, "SP") == 0)
					return SP_POLICY;
				else if(strcmp(tmp_args, "SHAPING") == 0)
					return SHAPING_POLICY;
				else return -1;
			}
		//}
	}
	return QUE_TBL_EMPTY;
}

void formQosTraffic(request *wp, char *path, char *query)
{
	char *submitUrl, *strAdd, *strDel, *strDelAll;
	char *strVal;
	int wanIndex = -1, intVal = 0;
	int qosFlag = 0, wanQosFlag = 0, autoUp = 1, autoDown = 1;
	char tmpBuf[100] = {0};
	unsigned long manuUp = 512, manuDown = 512;
	QOSQUEUE_T entry;
	int entryNum = 0;
	int ret = -1, queueType = QUE_TBL_EMPTY;
#if defined(CONFIG_IPV6)
	int wanV6QosFlag = 0;	
#endif


	strAdd = req_get_cstream_var(wp, ("addQos"), "");
#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strAdd[0]==0){
		strAdd = req_get_cstream_var(wp, ("addQosFlag"), "");
	}
#endif
	strDel = req_get_cstream_var(wp, ("deleteSel"), "");
	strDelAll = req_get_cstream_var(wp, ("deleteAll"), "");

	if ( strAdd[0]) {
		strVal = req_get_cstream_var(wp, ("enable"), "");
		if ( !strcmp(strVal, "ON"))
			qosFlag = 1;
		else
			qosFlag = 0;
			
		if ( !apmib_set( MIB_QOS_ENABLE, (void *)&qosFlag)) {
			strcpy(tmpBuf, ("set qos enable flag error!"));
			goto setErr;
		}

		if(!qosFlag){
			flushQosStatus();
			goto setOk;
		}
		
		strVal = req_get_cstream_var(wp, ("wanQosEnable"), "");
		if ( !strcmp(strVal, "ON"))
			wanQosFlag = 1;
		else
			wanQosFlag = 0;

#if defined(CONFIG_IPV6)
		strVal = req_get_cstream_var(wp, ("wanV6QosEnable"), "");
		if ( !strcmp(strVal, "ON"))
			wanV6QosFlag = 1;
		else
			wanV6QosFlag = 0;
#endif

		strVal = req_get_cstream_var(wp, ("wanInterface"), "");
		wanIndex = atoi(strVal);

		if(wanQosFlag == 0 && wanIndex == 0)
			goto setOk;
		
		if(wanIndex < WAN1 || wanIndex > WANIFACE_NUM){
			strcpy(tmpBuf, ("wan interface index error!"));
			goto setErr;
		}

		if(wanQosFlag){
			strVal = req_get_cstream_var(wp, ("automaticUplinkSpeed"), "");
			if ( !strcmp(strVal, "ON"))
				autoUp = 1;
			else
				autoUp = 0;
			
			strVal = req_get_cstream_var(wp, ("automaticDownlinkSpeed"), "");
			if ( !strcmp(strVal, "ON"))
				autoDown = 1;
			else
				autoDown = 0;

			if(autoUp == 0){
				strVal = req_get_cstream_var(wp, ("manualUplinkSpeed"), "");
				if(strVal[0])
					string_to_dec(strVal, &manuUp);
		
				if(manuUp > MAX_BAND_WIDTH){
					strcpy(tmpBuf, ("mannual uplink exceed max range!"));
					goto setErr;
				}
			}
			else{
			#if defined (CONFIG_RTL_8198)|| defined (CONFIG_RTL_8198C) || defined (CONFIG_RTL_8367R_SUPPORT)
				manuUp = 1000000;	// kbps
			#else
				manuUp = 100000;	// kbps
			#endif
			}

			if(autoDown == 0){
				strVal = req_get_cstream_var(wp, ("manualDownlinkSpeed"), "");
				if(strVal[0])
					string_to_dec(strVal, &manuDown);
		
				if(manuDown > MAX_BAND_WIDTH){
					strcpy(tmpBuf, ("mannual downlink exceed max range!"));
					goto setErr;
				}
			}	
			else{
			#if defined (CONFIG_RTL_8198)|| defined (CONFIG_RTL_8198C) || defined (CONFIG_RTL_8367R_SUPPORT)
				manuDown = 1000000;	// kbps
			#else
				manuDown = 100000;	// kbps
			#endif
			}

			queueType = check_queue_type();
			
			if(queueType == SHAPING_POLICY){
				if(!apmib_get(MIB_QOS_QUEUE_TBL_NUM, (void *)&entryNum)){
					strcpy(tmpBuf, ("get qos queue table number error!"));
					goto setErr;
				}

				for(intVal = 1; intVal<=entryNum; intVal++){
					memset(&entry, '\0', sizeof(entry));
					*((char *)&entry) = (char)intVal;
					if (!apmib_get(MIB_QOS_QUEUE_TBL, (void *)&entry)){
						strcpy(tmpBuf, ("get qos queue table entry error!"));
						goto setErr;
					}

					if(strncmp(entry.QueueInterface, "WAN", 3) == 0){	/* uplink */
						if((entry.ShapingRate/1000) > manuUp){
							strcpy(tmpBuf, ("mannual uplink speed is less than queue shaping setting, remove queue setting first!"));
							goto setErr;
						}
					}
					else if(strcmp(entry.QueueInterface, "LAN") == 0){	/* downlink */
						if((entry.ShapingRate/1000) > manuDown){
							strcpy(tmpBuf, ("mannual downlink speed is less than queue shaping setting, remove queue setting first!"));
							goto setErr;
						}
					}
				}
			}

			for(intVal = 1; intVal <= WANIFACE_NUM; intVal++){
				ret = configWanShaping(intVal, autoUp, autoDown, manuUp, manuDown, tmpBuf);
				if(ret == 0)
					goto setErr;		
			}

			ret = updateQueShaping(queueType, manuUp, manuDown);
			if(ret == 0)
				goto setErr;
		}

	#if defined(CONFIG_IPV6)
		ret = configWanQosStatus(wanIndex, wanQosFlag, wanV6QosFlag);
		if(!ret)
			goto setErr;
	
		ret = configClassStatus(wanIndex, wanQosFlag, wanV6QosFlag);
	#else
		ret = configWanQosStatus(wanIndex, wanQosFlag);
		if(!ret)
			goto setErr;
	
		ret = configClassStatus(wanIndex, wanQosFlag);
	#endif
		if(!ret)
			goto setErr;

		ret = configQueueStatus(wanIndex, wanQosFlag);
		if(!ret)
			goto setErr;
	}

	intVal = 0;
	if ( strDel[0]) {	
		for (wanIndex = WANIFACE_NUM; wanIndex>=WAN1; wanIndex--){
			snprintf(tmpBuf, 20, "select%d", wanIndex);
			
			strVal = req_get_cstream_var(wp, tmpBuf, "");
			if ( !strcmp(strVal, "ON") ) {
				#if defined(CONFIG_IPV6)
				ret = configWanQosStatus(wanIndex, intVal, intVal);
				if(!ret)
					goto setErr;
				
				ret = configClassStatus(wanIndex, intVal, intVal);
				#else
				ret = configWanQosStatus(wanIndex, intVal);
				if(!ret)
					goto setErr;

				ret = configClassStatus(wanIndex, intVal);
				#endif
				if(!ret)
					goto setErr;

				ret = configQueueStatus(wanIndex, intVal);
				if(!ret)
					goto setErr;
			}
		}
	}

	if ( strDelAll[0]) {	
		for(wanIndex = WAN1; wanIndex <= WANIFACE_NUM; wanIndex++){	
			#if defined(CONFIG_IPV6)
			ret = configWanQosStatus(wanIndex, intVal, intVal);
			if(!ret)
				goto setErr;
			
			ret = configClassStatus(wanIndex, intVal, intVal);
			#else
			ret = configWanQosStatus(wanIndex, intVal);
			if(!ret)
				goto setErr;

			ret = configClassStatus(wanIndex, intVal);
			#endif
			if(!ret)
				goto setErr;

			ret = configQueueStatus(wanIndex, intVal);
			if(!ret)
				goto setErr;
		}
	}

setOk:
	apmib_update_web(CURRENT_SETTING);
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	OK_MSG(submitUrl);
	return ;
	
setErr:
	ERR_MSG(tmpBuf);
}

void formQosPolicy(request *wp, char *path, char *query)
{
	char *submitUrl, *strAdd, *strDel, *strDelAll;
	char *strVal, *strPriority;
	char *strShapingRate;
	int wanIndex = -1, upFlag = 0, queueIndex = 0, index = 0;
	QOSQUEUE_T entry, entry_tmp;
	char dev[8] = {0};
	char tmpBuf[100] = {0};
	int policy = -1;
	int priority = 0, weight = 0, brFlag = 0;
	int entryNum = 0;
	int typeConflict = 0, shapingRate = 0;
	int totalLinkSpeed = 0, totalGuaranMin = 0;

#if defined(MAIN_WAN_MAPPING)
	int mainWanIndex = get_main_wan_index();
#endif

	strAdd = req_get_cstream_var(wp, ("addQos"), "");
#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strAdd[0]==0){
		strAdd = req_get_cstream_var(wp, ("addQosFlag"), "");
	}
#endif
	strDel = req_get_cstream_var(wp, ("deleteSel"), "");
	strDelAll = req_get_cstream_var(wp, ("deleteAll"), "");
	if ( strAdd[0]) {
		memset(&entry, '\0', sizeof(entry));
		
		strVal = req_get_cstream_var(wp, ("interface"), "");
		wanIndex = atoi(strVal);

		if(wanIndex < WAN1 || wanIndex > WANIFACE_NUM){
			strcpy(tmpBuf, ("wan interface index error!"));
			goto setErr;
		}

		if(!apmib_set( MIB_WANIFACE_CURRENT_IDX, (void *)&wanIndex)){
			strcpy(tmpBuf, ("set wan interface item index fail!"));
			goto setErr;
		}

		if(!apmib_get( MIB_WANIFACE_ADDRESSTYPE, (void *)&brFlag)){
			strcpy(tmpBuf, ("get MIB_WANIFACE_ADDRESSTYPE fail!"));
			goto setErr;
		}

		strVal = req_get_cstream_var(wp, ("direction"), "");
		upFlag = atoi(strVal);

		sprintf(dev, "%s%d", "WAN", wanIndex);

		if(upFlag == 1){		// lan to wan	
			sprintf(entry.QueueInterface, "%s", dev);
			strVal = req_get_cstream_var(wp, ("upQueue"), "");

			totalLinkSpeed = getWanShaping(wanIndex, outBound, tmpBuf);	// kbps
		}
		else{
			sprintf(entry.QueueInterface, "%s", "LAN");
			strVal = req_get_cstream_var(wp, ("downQueue"), "");

			totalLinkSpeed = getWanShaping(wanIndex, inBound, tmpBuf);	// kbps
		}
		if(totalLinkSpeed == 0)
			goto setErr;
		entry.ShapingRate = totalLinkSpeed*1000;	// from kbps -> bps
		
		queueIndex = atoi(strVal);
		entry.QueueInstanceNum = queueIndex;
		sprintf(entry.QueueWanInterface, "%s", dev);
		strVal = req_get_cstream_var(wp, ("policyType"), "");	
		if(strVal[0]){
			policy = atoi(strVal);
			
			index = check_queue_type();
			if(policy == 0){	// SP
				if((index != QUE_TBL_EMPTY) && (index != SP_POLICY))	{	// queue tbl contains other type queue
					typeConflict = 1;
				}
				
				sprintf(entry.SchedulerAlgorithm, "%s", "SP");

			#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
				if(brFlag == BRIDGE){
					strPriority = req_get_cstream_var(wp, ("priority_br"), "");
				}
			#if defined(MAIN_WAN_MAPPING)
				else if(wanIndex == mainWanIndex)
			#else
				else if(wanIndex == 1)
			#endif
				{
					strPriority = req_get_cstream_var(wp, ("priority_nat"), "");
				}
				else
				{
					strPriority = req_get_cstream_var(wp, ("priority"), "");
				}
			#else
				strPriority = req_get_cstream_var(wp, ("priority"), "");
			#endif
				
				if(strPriority[0]){
					priority = atoi(strPriority);
					entry.QueuePrecedence = priority;
				}
				else{
					priority = -1;
					
					#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
						if(brFlag == BRIDGE)
						{
							entry.QueuePrecedence = 7;
						}
					#if defined(MAIN_WAN_MAPPING)
						else if(wanIndex == mainWanIndex)
					#else
						else if(wanIndex == 1)
					#endif
						{
							entry.QueuePrecedence = 15;
						}
						else
						{
							entry.QueuePrecedence = 23;
						}
					#else
						entry.QueuePrecedence = 7;
					#endif
				}
				
				entry.QueueWeight = 0;
			}
			else if(policy == 1){	//WFQ
				if((index != QUE_TBL_EMPTY) && (index != WFQ_POLICY))	{	// queue tbl contains other type queue
					typeConflict = 1;
				}
			
				sprintf(entry.SchedulerAlgorithm, "%s", "WFQ");

				#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
					if(brFlag == BRIDGE)
					{
						entry.QueuePrecedence = 7;		/* bridge 0-7 */
					}
				#if defined(MAIN_WAN_MAPPING)
					else if(wanIndex == mainWanIndex)
				#else
					else if(wanIndex == 1)
				#endif
					{
						entry.QueuePrecedence = 15;		/* hw nat 8-15 */
					}
					else
					{
						entry.QueuePrecedence = 23;		/* others 16-23 */
					}
				#else
					entry.QueuePrecedence = 7;
				#endif

			
				strPriority = req_get_cstream_var(wp, ("weight"), "");
				weight = atoi(strPriority);
				entry.QueueWeight = weight;
			}
			else if(policy == 2){	// SHAPING
				if((index != QUE_TBL_EMPTY) && (index != SHAPING_POLICY))	{	// queue tbl contains other type queue
					typeConflict = 1;
				}
		
				sprintf(entry.SchedulerAlgorithm, "%s", "SHAPING");

				#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
					if(brFlag == BRIDGE)
					{
						entry.QueuePrecedence = 7;		/* bridge 0-7 */
					}
				#if defined(MAIN_WAN_MAPPING)
					else if(wanIndex == mainWanIndex)
				#else
					else if(wanIndex == 1)
				#endif
					{
						entry.QueuePrecedence = 15;		/* hw nat 8-15 */
					}
					else
					{
						entry.QueuePrecedence = 23;		/* others 16-23 */
					}
				#else
					entry.QueuePrecedence = 7;
				#endif

				strVal = req_get_cstream_var(wp, ("mode"), "");
				if(strVal[0]){
					strShapingRate = req_get_cstream_var(wp, ("rate"), "");
					if(strShapingRate[0]){
						shapingRate = atoi(strShapingRate) * 1000;
					}
					
					if(atoi(strVal) == 1){	// guaran min
						entry.GuaranMinRate = shapingRate;
					}
					else{	// restrict max
						entry.GuaranMinRate = 1000;
						entry.ShapingRate = shapingRate;
					}
				}
			}
		}
		
		sprintf(entry.DropAlgorithm, "%s", "DT");	//default value

		strVal = req_get_cstream_var(wp, ("defQueue"), "");
		if ( !strcmp(strVal, "ON"))
			index = 2;	// default qidsc :2
		else
			index = 0xffffffff;

		if (!apmib_set(MIB_QOS_DEF_QUEUE, (void *)&index)) {
			strcpy(tmpBuf, ("set qos default queue error!"));
			goto setErr;
		}
		
		if(upFlag == 1 && !policy && (priority == -1) && !weight)	// assue no new rule
			goto setOk;

		if(typeConflict){
			strcpy(tmpBuf, ("queue type can only be one of PRIO, WRR or SHAPING!"));
			goto setErr;
		}

		if(!apmib_get(MIB_QOS_QUEUE_TBL_NUM, (void *)&entryNum)){
			strcpy(tmpBuf, ("get qos queue table number error!"));
			goto setErr;
		}

		totalGuaranMin = 0;
		for(index = 1; index<=entryNum; index++){
			memset(&entry_tmp, '\0', sizeof(entry_tmp));
			*((char *)&entry_tmp) = (char)index;
			if (!apmib_get(MIB_QOS_QUEUE_TBL, (void *)&entry_tmp)){
				strcpy(tmpBuf, ("get qos queue table entry error!"));
				goto setErr;
			}

			if(entry_tmp.QueueInstanceNum == queueIndex){
				strcpy(tmpBuf, ("qos queue entry already exist!"));
				goto setErr;
			}

			if(!strcmp(entry_tmp.SchedulerAlgorithm, "SHAPING")){
				if(((upFlag == 1) && (!strncmp(entry_tmp.QueueInterface, "WAN", 3))) ||
				((upFlag == 2) && (!strcmp(entry_tmp.QueueInterface, "LAN")))){
					totalGuaranMin += (entry_tmp.GuaranMinRate/1000);
				}
			}
		}
		
		if((totalGuaranMin + entry.GuaranMinRate/1000) >= totalLinkSpeed){
			if(upFlag == 1)
				strcpy(tmpBuf, ("Error: for guaranteed minimum bandwidth of uplink, the sum bandwidth of all qos rules shoud be less than the total uplink bandwidth!"));
			else
				strcpy(tmpBuf, ("Error: for guaranteed minimum bandwidth of downlink, the sum bandwidth of all qos rules shoud be less than the total downlink bandwidth!"));
			goto setErr;
		}

		entry.QueueEnable = 1;

		apmib_set(MIB_QOS_QUEUE_DEL, (void *)&entry);
		if ( apmib_set(MIB_QOS_QUEUE_ADD, (void *)&entry) == 0) {
			strcpy(tmpBuf, ("Add qos queue table entry error!"));
			goto setErr;
		}
	}

	if ( strDel[0]) {
		if(!apmib_get(MIB_QOS_QUEUE_TBL_NUM, (void *)&entryNum)){
			strcpy(tmpBuf, ("get qos queue table number error!"));
			goto setErr;
		}

		for (index = entryNum; index>=1; index--){
			snprintf(tmpBuf, 20, "select%d", index);
			
			strVal = req_get_cstream_var(wp, tmpBuf, "");
			if ( !strcmp(strVal, "ON") ) {
				*((char *)&entry) = (char)index;
				if ( !apmib_get(MIB_QOS_QUEUE_TBL, (void *)&entry)) {
					strcpy(tmpBuf, ("Get table entry error!"));
					goto setErr;
				}
				if ( !apmib_set(MIB_QOS_QUEUE_DEL, (void *)&entry)) {
					strcpy(tmpBuf, ("Delete table entry error!"));
					goto setErr;
				}
			}
		}
	}

	if ( strDelAll[0]) {
		if ( !apmib_set(MIB_QOS_QUEUE_DELALL, (void *)&entry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr;
		}	
	}

setOk:
	apmib_update_web(CURRENT_SETTING);

	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	OK_MSG(submitUrl);
	return ;
	
setErr:
	ERR_MSG(tmpBuf);
}

void formQosRule(request *wp, char *path, char *query)
{
	char *submitUrl, *strAdd, *strDel, *strDelAll;
	char *strQueue, *strProto, *strSip, *strDip, *strSport, *strDport;
	char *strSmac, *strDmac, *strPport, *strDscp, *strVpri, *strVid, *strVal;
	char *strReDscp, *strReVpri;
	int index = -1, entryNum = -1, l3PreType = 0;
	QOSCLASS_T entry, entry_tmp;
	QOSQUEUE_T queue_entry;
	char tmpBuf[100] = {0};
	char *strIpVersion, *strIpv4Version, *strL3Pre;
	int ipVersion = -1, ipv4Version = -1;
#if defined(CONFIG_IPV6)
	char *strFLabel;
#endif

	strAdd = req_get_cstream_var(wp, ("addQos"), "");
#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strAdd[0]==0){
		strAdd = req_get_cstream_var(wp, ("addQosFlag"), "");
	}
#endif
	strDel = req_get_cstream_var(wp, ("deleteSel"), "");
	strDelAll = req_get_cstream_var(wp, ("deleteAll"), "");

	if ( strAdd[0]) {
		memset(&entry, '\0', sizeof(entry));
		
		strIpv4Version = req_get_cstream_var(wp, ("v4Version"), "");
		strIpVersion = req_get_cstream_var(wp, ("version"), "");

		if(strIpv4Version[0]){
			ipv4Version = atoi(strIpv4Version);
		}

		if(strIpVersion[0]){
			ipVersion = atoi(strIpVersion);
		}

		strSip = req_get_cstream_var(wp, ("sip"), "");
		strDip = req_get_cstream_var(wp, ("dip"), "");
		strDscp = req_get_cstream_var(wp, ("dscp"), "");

		if((ipv4Version == 1) || (ipVersion == 1)){	// ipv4
			entry.IpVersion = 1;
			
			if(strSip[0])
				inet_aton(strSip, (struct in_addr *)&entry.SourceIP);

			if(strDip[0])
				inet_aton(strDip, (struct in_addr *)&entry.DestIP);

			strL3Pre = req_get_cstream_var(wp, ("l3v4Prece"), "");
			
			if(strL3Pre[0] && strDscp[0]){
				l3PreType = atoi(strL3Pre);
				if(l3PreType == 1){		// DSCP
					entry.DSCPCheck = atoi(strDscp);
					entry.V4TypeOfService = -1;
#if defined (CONFIG_IPV6)						
					entry.V6TrafficClassMatch = -1;
#endif
				}
				else if(l3PreType == 2){	// ToS
					entry.V4TypeOfService = atoi(strDscp);
					entry.DSCPCheck = -1;
#if defined (CONFIG_IPV6)						
					entry.V6TrafficClassMatch = -1;
#endif						
				}
			}
			else{
				entry.DSCPCheck = -1;
				entry.V4TypeOfService = -1;
#if defined (CONFIG_IPV6)						
				entry.V6TrafficClassMatch = -1;
#endif		
			}
		}
#if defined (CONFIG_IPV6)
		else if((ipVersion == 2) || (ipVersion == 4)){	// ipv6 or ipv4+ipv6
			entry.IpVersion = ipVersion;
			
			if(strSip[0])
				strncpy(entry.SourceIPv6, strSip, 40);

			if(strDip[0])
				strncpy(entry.DestIPv6, strDip, 40);

			strL3Pre = req_get_cstream_var(wp, ("l3v6Prece"), "");
			if(strL3Pre[0] && strDscp[0]){
				l3PreType = atoi(strL3Pre);
				if(l3PreType == 1){		// DSCP
					entry.DSCPCheck = atoi(strDscp);
					entry.V4TypeOfService = -1;					
					entry.V6TrafficClassMatch = -1;
				}
				else if(l3PreType == 2){	// Traffic Class
					entry.V6TrafficClassMatch = atoi(strDscp);
					entry.DSCPCheck = -1;					
					entry.V4TypeOfService = -1;					
				}
			}
			else{
				entry.DSCPCheck = -1;
				entry.V4TypeOfService = -1;
				entry.V6TrafficClassMatch = -1;
			}
		}
#endif
		else{
			entry.IpVersion = -1;
		}

		strProto = req_get_cstream_var(wp, ("protocol"), "");
		if(strProto[0])
			entry.Protocol = atoi(strProto);
		else
			entry.Protocol = -1;

		strSport = req_get_cstream_var(wp, ("sPortStart"), "");
		if(strSport[0]){
			entry.SourcePort = atoi(strSport);
		}
		else
			entry.SourcePort = -1;

		strSport = req_get_cstream_var(wp, ("sPortEnd"), "");
		if(strSport[0]){
			entry.SourcePortRangeMax = atoi(strSport);
		}
		else
			entry.SourcePortRangeMax = -1;

		strDport = req_get_cstream_var(wp, ("dPortStart"), "");
		if(strDport[0]){
			entry.DestPort = atoi(strDport);
		}
		else
			entry.DestPort = -1;

		strDport = req_get_cstream_var(wp, ("dPortEnd"), "");
		if(strDport[0]){
			entry.DestPortRangeMax = atoi(strDport);
		}
		else
			entry.DestPortRangeMax = -1;

		strSmac = req_get_cstream_var(wp, ("smac"), "");
		if (strSmac[0] && !string_to_hex(strSmac, entry.SourceMACAddress, 12))
		{
			strcpy(tmpBuf, ("Source MAC input fail!"));
			goto setErr;
		}	

		strDmac = req_get_cstream_var(wp, ("dmac"), "");
		if (strDmac[0] && !string_to_hex(strDmac, entry.DestMACAddress, 12))
		{
			strcpy(tmpBuf, ("Destination MAC input fail!"));
			goto setErr;
		}

		strPport = req_get_cstream_var(wp, ("phyport"), "");
		if(strPport[0])
			entry.OutOfBandInfo = atoi(strPport);
		else
			entry.OutOfBandInfo = -1;

		strVpri = req_get_cstream_var(wp, ("vlan_pri"), "");
		if(strVpri[0])
			entry.EthernetPriorityCheck = atoi(strVpri);
		else
			entry.EthernetPriorityCheck = -1;

#if defined (CONFIG_IPV6)
		strFLabel = req_get_cstream_var(wp, ("flow_label"), "");
		if(strFLabel[0])
			entry.FlowLabel = atoi(strFLabel);
		else
			entry.FlowLabel = -1;
#endif
/*
		strVid = req_get_cstream_var(wp, ("vlan_id"), "");
		if(strVid[0])
			entry.VLANIDCheck = atoi(strVid);
		else
			entry.VLANIDCheck = -1;
*/

		strReVpri = req_get_cstream_var(wp, ("remark_vlan_pri"), "");
		if(strReVpri[0])
			entry.EthernetPriorityMark = atoi(strReVpri);
		else
			entry.EthernetPriorityMark = -1;

		strReDscp = req_get_cstream_var(wp, ("remark_dscp"), "");
		if(strReDscp[0])
			entry.DSCPMark = atoi(strReDscp);
		else
			entry.DSCPMark = -1;
		
		strQueue = req_get_cstream_var(wp, ("queue"), "");
		if(strQueue[0])
			entry.ClassQueue = atoi(strQueue);

		if(!apmib_get(MIB_QOS_QUEUE_TBL_NUM, (void *)&entryNum)){
			strcpy(tmpBuf, ("get qos queue table number error!"));
			goto setErr;
		}
		for(index = 1; index<=entryNum; index++){
			memset(&queue_entry, '\0', sizeof(queue_entry));
			*((char *)&queue_entry) = (char)index;
			if (!apmib_get(MIB_QOS_QUEUE_TBL, (void *)&queue_entry)){
				strcpy(tmpBuf, ("get qos queue table entry error!"));
				goto setErr;
			}

			if(queue_entry.QueueInstanceNum == entry.ClassQueue){
				sprintf(entry.ClassWanInterface, "%s", queue_entry.QueueWanInterface);
				break;
			}	
		}

		if(entry.ClassQueue / 10)	// queue11 - queue18
			sprintf(entry.ClassInterface, "%s", entry.ClassWanInterface);
		else
			sprintf(entry.ClassInterface, "%s", "LAN");
			
		entry.ClassificationEnable = 1;
		
		if( entry.Protocol == -1 && !strSip[0] && !strDip[0]
		&& !strSport[0] && !strDport[0] && !strSmac[0] && !strDmac[0] && !strPport[0] 
		&& !strDscp[0] && !strVpri[0] && !strReDscp[0] && !strReVpri[0] /*&& !strVid[0] */
#if defined (CONFIG_IPV6)
		&& !strFLabel[0]
#endif
		)		// no match and target rule
			goto setOk;

		if(entry.IpVersion == -1){
			strcpy(tmpBuf, ("Invalid IpVersion!"));
			goto setErr;
		}
	
		if( strReDscp[0] || strReVpri[0]){	// if exist target rule, must have at least one match rule
			if( entry.Protocol == -1 && !strSip[0] && !strDip[0]
			&& !strSport[0] && !strDport[0] && !strSmac[0] && !strDmac[0] && !strPport[0] 
			&& !strDscp[0] && !strVpri[0] /*&& !strVid[0] */
#if defined (CONFIG_IPV6)
			&& !strFLabel[0] 
#endif
			){
				strcpy(tmpBuf, ("can't set remark without classification rules!"));
				goto setErr;
			}
		}

		/* default value */
		entry.Ethertype = -1;
		entry.SSAP = -1;
		entry.DSAP = -1;
		entry.LLCControl = -1;
		entry.SNAPOUI = -1;
		entry.TrafficClass = -1;
		entry.ClassPolicer = -1;
		entry.ClassApp = -1;
		strcpy(entry.SourceVendorClassIDMode, "Exact");
		strcpy(entry.DestVendorClassIDMode, "Exact");
		strcpy(entry.SourceVendorSpecificInfoMode, "Exact");
		strcpy(entry.DestVendorSpecificInfoMode, "Exact");

		if(!apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum)){
			strcpy(tmpBuf, ("get qos class table number error!"));
			goto setErr;
		}
		
		for(index = 1; index<=entryNum; index++){
			memset(&entry_tmp, '\0', sizeof(entry_tmp));
			*((char *)&entry_tmp) = (char)index;
			if (!apmib_get(MIB_QOS_CLASS_TBL, (void *)&entry_tmp)){
				strcpy(tmpBuf, ("get qos class table entry error!"));
				goto setErr;
			}

			if(strlen(entry.ClassInterface) == strlen(entry_tmp.ClassInterface) &&
			!memcmp(entry.ClassInterface,entry_tmp.ClassInterface,strlen(entry.ClassInterface))){

				if((entry.ClassQueue < 10) && (entry_tmp.ClassQueue < 10) ||
				   (entry.ClassQueue > 10) && (entry_tmp.ClassQueue > 10)){

					if(entry.ClassQueue == entry_tmp.ClassQueue){
				
						if(((entry.V4TypeOfService != -1) && (entry_tmp.DSCPCheck != -1) && (entry_tmp.IpVersion == 0)) || 
						((entry.DSCPCheck != -1) && (entry.IpVersion == 0) && (entry_tmp.V4TypeOfService != -1))){
							strcpy(tmpBuf, ("cannot config dscp and tos on the same queue!"));
							goto setErr;
						}
					#if defined(CONFIG_IPV6)
						if(((entry.V6TrafficClassMatch != -1) && (entry_tmp.DSCPCheck != -1) && (entry_tmp.IpVersion == 1)) ||
						(entry.DSCPCheck != -1) && (entry.IpVersion == 1) && (entry_tmp.V6TrafficClassMatch != -1)){
							strcpy(tmpBuf, ("cannot config dscp and traffic class on the same queue!"));
							goto setErr;
						}
					#endif
					}

					if((strlen(entry.SourceIP) != strlen(entry_tmp.SourceIP)) || 
					memcmp(entry.SourceIP, entry_tmp.SourceIP, 4)){
						continue;			
					}
					else{
						unsigned char temp[4] = {0};
						if(memcmp(entry.SourceIP, temp, 4)){
							goto conflict;
						}
					}

					if((strlen(entry.DestIP) != strlen(entry_tmp.DestIP)) || 
					memcmp(entry.DestIP,entry_tmp.DestIP, 4)){
						continue;			
					}
					else{
						unsigned char temp[4] = {0};
						if(memcmp(entry.DestIP, temp, 4)){
							goto conflict;
						}
					}

#if defined(CONFIG_IPV6)
					if((strlen(entry.SourceIPv6) != strlen(entry_tmp.SourceIPv6)) || 
					memcmp(entry.SourceIPv6,entry_tmp.SourceIPv6, 40)){
						continue;	
					}
					else{
						unsigned char temp[40] = {0};
						if(memcmp(entry.SourceIPv6, temp, 40)){	// sip6 conflict
							goto conflict;
						}
					}

					if((strlen(entry.DestIPv6) != strlen(entry_tmp.DestIPv6)) || 
					memcmp(entry.DestIPv6,entry_tmp.DestIPv6, 40)){
						continue;	
					}
					else{
						unsigned char temp[40] = {0};
						if(memcmp(entry.DestIPv6, temp, 40)){	// dip6 conflict
							goto conflict;
						}
					}

					if(entry.FlowLabel != entry_tmp.FlowLabel){
						continue;				
					}
					else if(entry.FlowLabel >= 0){	// flow label conflict
						goto conflict;
					}

					if(entry.V6TrafficClassMatch != entry_tmp.V6TrafficClassMatch){
						continue;				
					}
					else if(entry.V6TrafficClassMatch >= 0){	// traffic class conflict
						goto conflict;
					}
#endif

					if(entry.SourcePort != -1 && entry_tmp.SourcePort != -1){

						if(((entry.SourcePort < entry_tmp.SourcePort) && (entry.SourcePortRangeMax < entry_tmp.SourcePort))
						|| ((entry.SourcePort > entry_tmp.SourcePortRangeMax) && (entry.SourcePortRangeMax > entry_tmp.SourcePortRangeMax))){
							continue;
						}
						else{
							strcpy(tmpBuf, ("source port or source port range conflict!"));
							goto setErr;
						} 
					}
					else if((entry.SourcePort != -1 && entry_tmp.SourcePort == -1) ||
					(entry.SourcePort == -1 && entry_tmp.SourcePort != -1)){
						continue;				
					}

					if(entry.DestPort != -1 && entry_tmp.DestPort != -1){

						if(((entry.DestPort < entry_tmp.DestPort) && (entry.DestPortRangeMax < entry_tmp.DestPort))
						|| ((entry.DestPort > entry_tmp.DestPortRangeMax) && (entry.DestPortRangeMax > entry_tmp.DestPortRangeMax))){
							continue;
						}
						else{
							strcpy(tmpBuf, ("dest port or dest port range conflict!"));
							goto setErr;
						} 
					}
					else if((entry.DestPort != -1 && entry_tmp.DestPort == -1) ||
					(entry.DestPort == -1 && entry_tmp.DestPort != -1)){
						continue;				
					}

					if(entry.DSCPCheck != entry_tmp.DSCPCheck){
						continue;
					}
					else if(entry.DSCPCheck >= 0){	// dscp conflict
						goto conflict;
					}

					if(entry.V4TypeOfService != entry_tmp.V4TypeOfService){
						continue;				
					}
					else if(entry.V4TypeOfService >= 0){	// type of service conflict
						goto conflict;
					}

					if(entry.EthernetPriorityCheck != entry_tmp.EthernetPriorityCheck){
						continue;
					}
					else if(entry.EthernetPriorityCheck >= 0){
						goto conflict;
					}

					if(entry.OutOfBandInfo != entry_tmp.OutOfBandInfo){
						continue;				
					}
					else if(entry.OutOfBandInfo >= 0){
						goto conflict;
					}
					
					if(memcmp(entry.SourceMACAddress,entry_tmp.SourceMACAddress, MAC_ADDR_LEN)){
						continue;				
					}
					else{
						unsigned char temp[MAC_ADDR_LEN] = {0};
						if(memcmp(entry.SourceMACAddress, temp, MAC_ADDR_LEN))
							goto conflict;
					}
					
					if(memcmp(entry.DestMACAddress,entry_tmp.DestMACAddress, MAC_ADDR_LEN))	{
						continue;				
					}
					else{
						unsigned char temp[MAC_ADDR_LEN] = {0};
						if(memcmp(entry.DestMACAddress, temp, MAC_ADDR_LEN))
							goto conflict;
					}
					
					if(entry.Protocol != entry_tmp.Protocol){
						continue;
					}
					
				   	if(entry.IpVersion != entry_tmp.IpVersion){
						continue;				
					}
				}
conflict:
				strcpy(tmpBuf, (" qos classification entry already exist!"));
				goto setErr;
			}

		}

		entry.ClassInstanceNum = getClassInstanceNum();
		
		apmib_set(MIB_QOS_CLASS_DEL, (void *)&entry);

	#if 1
		if(add_classifyMIB(&entry) == 0)
	#else
		if (apmib_set(MIB_QOS_CLASS_ADD, (void *)&entry) == 0) 
	#endif
		{
			strcpy(tmpBuf, ("Add qos class table entry error!"));
			goto setErr;
		}
	}

	if ( strDel[0]) {
		if(!apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum)){
			strcpy(tmpBuf, ("get qos queue table number error!"));
			goto setErr;
		}

		for (index = entryNum; index>=1; index--){
			snprintf(tmpBuf, 20, "select%d", index);
			
			strVal = req_get_cstream_var(wp, tmpBuf, "");
			if ( !strcmp(strVal, "ON") ) {
				*((char *)&entry) = (char)index;
				if ( !apmib_get(MIB_QOS_CLASS_TBL, (void *)&entry)) {
					strcpy(tmpBuf, ("Get table entry error!"));
					goto setErr;
				}

			#if 1
				if(!del_classifyMIB(&entry))
			#else
				if ( !apmib_set(MIB_QOS_CLASS_DEL, (void *)&entry)) 
			#endif
				{
					strcpy(tmpBuf, ("Delete table entry error!"));
					goto setErr;
				}
			}
		}
	}

	if ( strDelAll[0]) {
		if ( !apmib_set(MIB_QOS_CLASS_DELALL, (void *)&entry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr;
		}	
	}

setOk:
	apmib_update_web(CURRENT_SETTING);

	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	OK_MSG(submitUrl);
	return ;
	
setErr:
	ERR_MSG(tmpBuf);
}

int qosTrafficList(request *wp, int argc, char **argv)
{
	int	nBytesSent = 0, wan_index = 0;
	WANIFACE_T WanIfaceEntry;
	char wanInterface[8] = {0}, status[16] = {0};
#if 0
	char manualUplinkSpeed[64] = {0}, manualDownlinkSpeed[64] = {0};
#endif
#if defined(CONFIG_IPV6)
	char statusV6[16] = {0};
#endif

	nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
		"<td align=center width=\"\" ><font size=\"2\"><b>Interface</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Ipv4 Status</b></font></td>\n"
#if defined(CONFIG_IPV6)
		"<td align=center width=\"\" ><font size=\"2\"><b>Ipv6 Status</b></font></td>\n"
#endif
#if 0
		"<td align=center width=\"\" ><font size=\"2\"><b>Uplink(kbps)</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Downlink(kbps)</b></font></td>\n"
#endif
		"<td align=center width=\"\" ><font size=\"2\"><b>Select</b></font></td>\n"
	));

	for(wan_index = WAN1; wan_index <= WANIFACE_NUM; wan_index++)
	{
		memset(&WanIfaceEntry, '\0', sizeof(WanIfaceEntry));
		if(!getWanIfaceEntry(wan_index,&WanIfaceEntry))
		{
			printf("%s.%d.get waniface entry fail\n",__FUNCTION__,__LINE__);
			return -1;
		}
	
		sprintf(wanInterface, "%s%d","WAN", wan_index);

		if(WanIfaceEntry.wanQosEnable)
			snprintf(status, sizeof(status), "%s", "Enable");
		else
			snprintf(status, sizeof(status), "%s", "Disable");

#if defined(CONFIG_IPV6)
		if(WanIfaceEntry.wanQosV6Enable)
			snprintf(statusV6, sizeof(statusV6), "%s", "Enable");
		else
			snprintf(statusV6, sizeof(statusV6), "%s", "Disable");
#endif
#if 0			
		if(WanIfaceEntry.qosAutoUpLinkSpeed == 1){
			#if defined (CONFIG_RTL_8198)|| defined (CONFIG_RTL_8198C) || defined (CONFIG_RTL_8367R_SUPPORT)
				sprintf(manualUplinkSpeed, "%s", "1000000");	// kbps
			#else
				sprintf(manualUplinkSpeed, "%s", "100000");		// kbps
			#endif
		}
		else{
			sprintf(manualUplinkSpeed, "%ld", WanIfaceEntry.qosManualUpLinkSpeed);	
		}
	
		if(WanIfaceEntry.qosAutoDownLinkSpeed == 1){
			#if defined (CONFIG_RTL_8198)|| defined (CONFIG_RTL_8198C) || defined (CONFIG_RTL_8367R_SUPPORT)
				sprintf(manualDownlinkSpeed, "%s", "1000000");	// kbps
			#else
				sprintf(manualDownlinkSpeed, "%s", "100000");		// kbps
			#endif
		}
		else{
			sprintf(manualDownlinkSpeed, "%ld", WanIfaceEntry.qosManualDownLinkSpeed);
		}
#endif
	#if defined(CONFIG_IPV6)
		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
      	#if 1
			"<td align=center width=\"\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"), 
			wanInterface, status, statusV6, wan_index
		#else
      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"), 
			wanInterface, status, statusV6, manualUplinkSpeed, manualDownlinkSpeed, wan_index
		#endif
		);
	#else
		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
		#if 1
			"<td align=center width=\"\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"), 
			wanInterface, status, wan_index
		#else
      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"), 
			wanInterface, status, manualUplinkSpeed, manualDownlinkSpeed, wan_index
		#endif
		);
	#endif
	}

	return nBytesSent;
}

int qosPolicyList(request *wp, int argc, char **argv)
{
	int	nBytesSent = 0, entryNum = 0, index = 0;
	QOSQUEUE_T queue_entry;
	char direction[8] = {0}, wan_iface[16] = {0}, str_policy[16] = {0};
	char queue[16] = {0}, status[16] = {0};
	char weight[8] = {0}, prio[8] = {0};
	char mode[64] = {0}, rate[16] = {0};
		
	nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
		"<td align=center width=\"\" ><font size=\"2\"><b>Queue</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Status</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Direction</b></font></td>\n"		
		"<td align=center width=\"\" ><font size=\"2\"><b>Wan Interface</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Policy</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Priority</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Weight</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Mode</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Bandwidth</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Select</b></font></td>\n"
	));

	if((apmib_get(MIB_QOS_QUEUE_TBL_NUM, (void *)&entryNum))==0){
		printf("get MIB_QOS_QUEUE_TBL_NUM fail!\n");
		return -1;
	}
	
	for(index = 1; index <= entryNum; index++){
		memset(&queue_entry, '\0', sizeof(queue_entry));
		*((char *)&queue_entry) = (char)index;

		if((apmib_get(MIB_QOS_QUEUE_TBL, (void *)&queue_entry)) == 0){
			printf("get MIB_QOS_QUEUE_TBL fail!\n");
			return -1;
		}

		snprintf(direction, sizeof(direction), "%s", s4dashes);
		snprintf(weight, sizeof(weight), "%s", s4dashes);
		snprintf(mode, sizeof(mode), "%s", s4dashes);
		snprintf(rate, sizeof(rate), "%s", s4dashes);
		snprintf(prio, sizeof(prio), "%s", s4dashes);

		snprintf(queue, sizeof(queue), "queue %d", queue_entry.QueueInstanceNum);
		
		if(queue_entry.QueueEnable)
			snprintf(status, sizeof(status), "%s", "enable");
		else
			snprintf(status, sizeof(status), "%s", "disable");

		if(strncmp(queue_entry.QueueInterface, "LAN", 3) == 0)
			snprintf(direction, sizeof(direction), "%s", "down");
		else if(strncmp(queue_entry.QueueInterface, "WAN", 3) == 0)
			snprintf(direction, sizeof(direction), "%s", "up");
			
		if(!strcmp(queue_entry.SchedulerAlgorithm, "SP")){
			sprintf(prio, "%d", queue_entry.QueuePrecedence);
		}
		else if(!strcmp(queue_entry.SchedulerAlgorithm, "WFQ")){
			sprintf(weight, "%d", queue_entry.QueueWeight);
		}
		else if(!strcmp(queue_entry.SchedulerAlgorithm, "SHAPING")){
			if(queue_entry.GuaranMinRate > 1000){
				sprintf(rate, "%d", queue_entry.GuaranMinRate/1000);
				sprintf(mode, "%s", "Guaranteed minimum bandwidth");
			}
			else{
				sprintf(rate, "%d", queue_entry.ShapingRate/1000);
				sprintf(mode, "%s", "Restricted maximum bandwidth");
			}
		}

		snprintf(wan_iface, sizeof(wan_iface), "%s", queue_entry.QueueWanInterface);
		if(!wan_iface[0])
			snprintf(wan_iface, sizeof(wan_iface), "%s", s4dashes);

		
		snprintf(str_policy, sizeof(str_policy), "%s", queue_entry.SchedulerAlgorithm);
		if(!str_policy[0])
			snprintf(str_policy, sizeof(str_policy), "%s", s4dashes);
	
		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"), 
			queue, status, direction, wan_iface, str_policy,
			prio, weight, mode, rate, index
		);
	}
		
	return nBytesSent;
}

int qosRuleList(request *wp, int argc, char **argv)
{
	int	nBytesSent = 0, entryNum = 0, index = 0;
	QOSCLASS_T class_entry;
	char protocol[16] = {0};
	char sip[20] = {0}, dip[20] = {0}, smac[32] = {0}, dmac[32] = {0};
	char sport[16] = {0}, dport[16] = {0};
	char phyport[8] = {0}, dscp[8] = {0}, vlan_pri[8] = {0}, vlan_id[8] = {0}, tos[8] = {0};
	char dscp_remark[8] = {0}, vlan_pri_remark[8] = {0}, class_queue[8] = {0};
	struct in_addr * ipaddr = NULL;
	char version[16] = {0}, status[16] = {0};
#if defined (CONFIG_IPV6)
	char sip6[40] = {0}, dip6[40] = {0}, trafCls[8] = {0}, flowLabel[20] = {0};
#endif

#if defined (CONFIG_IPV6)
	nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
		"<td align=center width=\"\" ><font size=\"2\"><b>Queue</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Status</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Version</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Protocol</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Source IP</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Source Port</b></font></td>\n"
      	"<td align=center width=\"\" ><font size=\"2\"><b>Dest IP</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Dest Port</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Source Mac</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Dest Mac</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Physic Port</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>802.1P</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Flow Label</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>DSCP</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>ToS</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Traffic Class</b></font></td>\n"
	/*	"<td align=center width=\"\" ><font size=\"2\"><b>VLAN ID</b></font></td>\n"	*/
		"<td align=center width=\"\" ><font size=\"2\"><b>Remark DSCP</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Remark 802.1P</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Select</b></font></td>\n"
	));
#else
		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
		"<td align=center width=\"\" ><font size=\"2\"><b>Queue</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Status</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Version</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Protocol</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Source IP</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Source Port</b></font></td>\n"
      	"<td align=center width=\"\" ><font size=\"2\"><b>Dest IP</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Dest Port</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Source Mac</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Dest Mac</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Physic Port</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>802.1P</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>DSCP</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>ToS</b></font></td>\n"
	/*	"<td align=center width=\"\" ><font size=\"2\"><b>VLAN ID</b></font></td>\n"	*/
		"<td align=center width=\"\" ><font size=\"2\"><b>Remark DSCP</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Remark 802.1P</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Select</b></font></td>\n"
	));
#endif

	if((apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum))==0){
			printf("get MIB_QOS_CLASS_TBL_NUM fail!\n");
			return -1;
	}
		
	for(index = 1; index <= entryNum; index++){
		memset(&class_entry, '\0', sizeof(class_entry));
		*((char *)&class_entry) = (char)index;

		if((apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry)) == 0){
			printf("get MIB_QOS_CLASS_TBL fail!\n");
			return -1;
		}

		snprintf(class_queue, sizeof(class_queue), "%s", s4dashes);
		snprintf(version, sizeof(version), "%s", s4dashes);
		
		snprintf(sip, sizeof(sip), "%s", s4dashes);
		snprintf(dip, sizeof(dip), "%s", s4dashes);
		snprintf(sport, sizeof(sport), "%s", s4dashes);
		snprintf(dport, sizeof(dport), "%s", s4dashes);
		snprintf(smac, sizeof(smac), "%s", s4dashes);
		snprintf(dmac, sizeof(dmac), "%s", s4dashes);
		snprintf(protocol, sizeof(protocol), "%s", s4dashes);
		
		snprintf(tos, sizeof(tos), "%s", s4dashes);
		snprintf(dscp, sizeof(dscp), "%s", s4dashes);
		snprintf(phyport, sizeof(phyport), "%s", s4dashes);
		snprintf(vlan_pri, sizeof(vlan_pri), "%s", s4dashes);

		snprintf(dscp_remark, sizeof(dscp_remark), "%s", s4dashes);
		snprintf(vlan_pri_remark, sizeof(vlan_pri_remark), "%s", s4dashes);

	#if defined (CONFIG_IPV6)
		snprintf(sip6, sizeof(sip6), "%s", s4dashes);
		snprintf(dip6, sizeof(dip6), "%s", s4dashes);
		snprintf(trafCls, sizeof(trafCls), "%s", s4dashes);
		snprintf(flowLabel, sizeof(flowLabel), "%s", s4dashes);
	#endif

		if(class_entry.ClassQueue != -1)
			snprintf(class_queue, sizeof(class_queue), "%d", class_entry.ClassQueue);

		if(class_entry.ClassificationEnable)
			snprintf(status, sizeof(status), "%s", "enable");
		else
			snprintf(status, sizeof(status), "%s", "disable");

		if(class_entry.IpVersion == 1){		// ipv4 only
			snprintf(version, sizeof(version), "%s", "IPv4");

			ipaddr=((struct in_addr *)class_entry.SourceIP);
	        if(ipaddr->s_addr)//SourceIP is not empty
	            strcpy(sip, inet_ntoa(*((struct in_addr*)class_entry.SourceIP)));
			
			ipaddr=((struct in_addr *)class_entry.DestIP);
	        if(ipaddr->s_addr)//SourceIP is not empty
	            strcpy(dip, inet_ntoa(*((struct in_addr*)class_entry.DestIP)));

			if(class_entry.V4TypeOfService != -1)
				snprintf(tos, sizeof(tos), "%d", class_entry.V4TypeOfService);
		}
#if defined (CONFIG_IPV6)	
		else if(class_entry.IpVersion == 2){	// ipv6 only
			snprintf(version, "%s", "IPv6");
			strncpy(sip6, class_entry.SourceIPv6, 40);
			strncpy(dip6, class_entry.DestIPv6, 40);

			if(!sip6[0])
				sprintf(sip6, "%s", s4dashes);

			if(!dip6[0])
				sprintf(dip6, "%s", s4dashes);

			if(class_entry.V6TrafficClassMatch != -1)
				sprintf(trafCls, "%d", class_entry.V6TrafficClassMatch);

			if(class_entry.FlowLabel != -1)
				sprintf(flowLabel, "%d", class_entry.FlowLabel);
		}
		else if(class_entry.IpVersion == 4){	// both
			sprintf(version, "%s", "Ipv4+Ipv6");
		}
#endif
		
		if(class_entry.DSCPCheck != -1)
			sprintf(dscp, "%d", class_entry.DSCPCheck);
		
		if(class_entry.Protocol == 0)
			sprintf(protocol, "%s", "tcp+udp");
		else if(class_entry.Protocol == 6)
			sprintf(protocol, "%s", "tcp");
		else if(class_entry.Protocol == 17)
			sprintf(protocol, "%s", "udp");

		if(class_entry.DestPort != -1){
			if(class_entry.DestPortRangeMax != -1)
				sprintf(dport,"%d-%d",class_entry.DestPort,class_entry.DestPortRangeMax);
			else
				sprintf(dport,"%d-%d",class_entry.DestPort,class_entry.DestPort);
		}

		if(class_entry.SourcePort != -1){
			if(class_entry.SourcePortRangeMax != -1)
				sprintf(sport,"%d-%d",class_entry.SourcePort,class_entry.SourcePortRangeMax);
			else
				sprintf(sport,"%d-%d",class_entry.SourcePort,class_entry.SourcePort);
		}

		
		if(!((class_entry.DestMACAddress[0]==0)&&(class_entry.DestMACAddress[1]==0)
		&&(class_entry.DestMACAddress[2]==0)&&(class_entry.DestMACAddress[3]==0)
		&&(class_entry.DestMACAddress[4]==0)&&(class_entry.DestMACAddress[5]==0))){
			sprintf(dmac, "%02x:%02x:%02x:%02x:%02x:%02x",
                class_entry.DestMACAddress[0], 
                class_entry.DestMACAddress[1], 
                class_entry.DestMACAddress[2], 
                class_entry.DestMACAddress[3], 
                class_entry.DestMACAddress[4],
                class_entry.DestMACAddress[5]);
		}
		
		if(!((class_entry.SourceMACAddress[0]==0)&&(class_entry.SourceMACAddress[1]==0)
		&&(class_entry.SourceMACAddress[2]==0)&&(class_entry.SourceMACAddress[3]==0)
		&&(class_entry.SourceMACAddress[4]==0)&&(class_entry.SourceMACAddress[5]==0))){
			sprintf(smac, "%02x:%02x:%02x:%02x:%02x:%02x",
                class_entry.SourceMACAddress[0], 
                class_entry.SourceMACAddress[1], 
                class_entry.SourceMACAddress[2], 
                class_entry.SourceMACAddress[3], 
                class_entry.SourceMACAddress[4],
                class_entry.SourceMACAddress[5]);
		}

		if(class_entry.OutOfBandInfo != -1)
			sprintf(phyport, "%d", class_entry.OutOfBandInfo);

		if(class_entry.EthernetPriorityCheck != -1)
			sprintf(vlan_pri, "%d", class_entry.EthernetPriorityCheck);

	/*
		if(class_entry.VLANIDCheck != -1)
			sprintf(vlan_id, "%d", class_entry.VLANIDCheck);
		else
			sprintf(vlan_id, "%s", s4dashes);
	*/
		if(class_entry.DSCPMark != -1)
			sprintf(dscp_remark, "%d", class_entry.DSCPMark);

		if(class_entry.EthernetPriorityMark != -1)
			sprintf(vlan_pri_remark, "%d", class_entry.EthernetPriorityMark);

#if defined (CONFIG_IPV6)
		if(class_entry.IpVersion == 1){
			nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
	      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
	      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
	      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
	      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
	      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
	      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"	
			/*	"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"	*/
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				class_queue, status, version, protocol, sip, sport, dip, dport, smac, dmac, 
				phyport, vlan_pri, flowLabel, dscp, tos, trafCls, /*vlan_id, */dscp_remark, vlan_pri_remark, index);
		}
		else if((class_entry.IpVersion == 2) || (class_entry.IpVersion == 4)){
			nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
	      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
	      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
	      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
	      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"	
			/*	"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"	*/
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				class_queue, status, version, protocol, sip6, sport, dip6, dport, smac, dmac, 
				phyport, vlan_pri, flowLabel, dscp, tos, trafCls, /*vlan_id, */dscp_remark, vlan_pri_remark, index);
		}
#else
		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
	      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
	      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
	      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
	      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
			/*	"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"	*/
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				class_queue, status, version, protocol, sip, sport, dip, dport, smac, dmac, 
				phyport, vlan_pri, dscp, tos, /*vlan_id, */dscp_remark, vlan_pri_remark, index);
#endif
	}
	return nBytesSent;
}

#elif defined(QOS_BY_BANDWIDTH)
static const char _md1[] = "Guaranteed minimum bandwidth", _md2[] = "Restricted maximum bandwidth";

#define QOS_BW_CHECK_FAIL				-1
#define QOS_BW_NOT_OVERSIZE			0
#define QOS_UPLINK_BW_OVERSIZE		0x1
#define QOS_DOWNLINK_BW_OVERSIZE		0x2
#define QOS_BOTHLINK_BW_OVERSIZE		0x3

// Only for "Guaranteed minimum bandwidth",
// to check current uplink or downlink bandwidth added uplink & downlink bandwidth at previous rules
// whether larger than totoal uplink or downlink bandwidth
int checkQosRuleBw(unsigned long curUplinkBw, unsigned long curDownlinkBw, unsigned long totalUplinkBw, unsigned long totalDownlinkBw)
{
	int	entryNum, i, ret;
	IPQOS_T entry;
	unsigned long tmpTotolUplinkBw, tmpTotalDownlinkBw;

	if ( !apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum)) {
		return QOS_BW_CHECK_FAIL;
	}

	tmpTotolUplinkBw=curUplinkBw;
	tmpTotalDownlinkBw=curDownlinkBw;
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_QOS_RULE_TBL, (void *)&entry))
			return QOS_BW_CHECK_FAIL;

		if ( (entry.mode & QOS_RESTRICT_MIN)  != 0){
			//Do check for "Guaranteed minimum bandwidth"
			tmpTotolUplinkBw += entry.bandwidth;
			tmpTotalDownlinkBw += entry.bandwidth_downlink;
		}
	}

	ret=QOS_BW_NOT_OVERSIZE;
	if(tmpTotolUplinkBw > totalUplinkBw)
		ret += QOS_UPLINK_BW_OVERSIZE;

	if(tmpTotalDownlinkBw > totalDownlinkBw)
		ret += QOS_DOWNLINK_BW_OVERSIZE;

	return ret;
}

/////////////////////////////////////////////////////////////////////////////
int ipQosList(request *wp, int argc, char **argv)
{
	int	nBytesSent=0, entryNum, i;
	int vlanSupport, staticIpSupport;
	IPQOS_T entry;
	char	*mode, bandwidth[10], bandwidth_downlink[10];
	char	mac[20], ip[40], *tmpStr;
	#ifdef QOS_CLASSIFY_BY_DSCP
	char    dscp[40];
	int dscpvalue=0;
	#endif
#ifdef CONFIG_IPV6
	char	ip6[40];
#endif
#if defined(ADVANCED_IPQOS)
	char entryName[32]={0};
	char protocol=0;
	char protocolBuff[16]={0};
	char localPort[64]={0};
	char rmtip[64]={0};
	char rmtPort[64]={0};
	char rmtipTmp[64]={0};
	char rmtMac[32]={0};
	#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
	char priority[16]={0};
	char weight[16]={0};
	#endif
	char ip_qos_phyport[16]={0};
	char ip_qos_dscp[16]={0};
	char ip_qos_vlan_pri[16]={0};
	char remark_ip_qos_dscp[16]={0};
	char remark_ip_qos_vlan_pri[16]={0};
	char ipversion[16]={0};
#endif

	if ( !apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}

#if defined(CONFIG_8021Q_VLAN_SUPPORTED)
	if(apmib_get(MIB_VLAN_ENABLED, (void *)&vlanSupport)){
	}
	else{
		fprintf(stderr, "Get MIB_VLAN_ENABLED error!\n");
		return -1;
	}
#else
	vlanSupport = 0;
#endif
	if(apmib_get( MIB_WAN_DHCP, (void *)&staticIpSupport)){
		if(staticIpSupport == 0)
			staticIpSupport = 1;
		else
			staticIpSupport = 0;
	}
	else{
		fprintf(stderr, "Get MIB_WAN_DHCP error!\n");
		return -1;
	}
	
	nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"

#if defined(ADVANCED_IPQOS)
		"<td align=center width=\"\" ><font size=\"2\"><b>Name</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Ipversion</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Protocol</b></font></td>\n"
#endif
      	"<td align=center width=\"\" ><font size=\"2\"><b>Local IP Address</b></font></td>\n"
#if defined(ADVANCED_IPQOS)
		"<td align=center width=\"\" ><font size=\"2\"><b>Local Port</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Remote IP Address</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>Remote Port</b></font></td>\n"
#endif
#if defined(CONFIG_IPV6)
			"<td align=center width=\"20%%\" ><font size=\"2\"><b>Local IPv6 addr</b></font></td>\n"
#endif
      	"<td align=center width=\"\" ><font size=\"2\"><b>MAC Address</b></font></td>\n"));
#if defined(ADVANCED_IPQOS)
		if(staticIpSupport)
			nBytesSent += req_format_write(wp, ("<td align=center width=\"\" ><font size=\"2\"><b>Remote MAC</b></font></td>\n"));

		nBytesSent += req_format_write(wp, ("<td align=center width=\"\" ><font size=\"2\"><b>Phyport</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>dscp</b></font></td>\n"));
		
		if(vlanSupport)
			nBytesSent += req_format_write(wp, ("<td align=center width=\"\" ><font size=\"2\"><b>802.1p</b></font></td>\n"));
#endif
		nBytesSent += req_format_write(wp, (
#if defined(CONFIG_NETFILTER_XT_MATCH_LAYER7)
		"<td align=center width=\"20%%\" ><font size=\"2\"><b>Layer 7 Rule</b></font></td>\n"
#endif
      	"<td align=center width=\"\" ><font size=\"2\"><b>Mode</b></font></td>\n"
      	"<td align=center width=\"\" ><font size=\"2\"><b>Uplink Bandwidth</b></font></td>\n"
      	"<td align=center width=\"\" ><font size=\"2\"><b>Downlink Bandwidth</b></font></td>\n"));
      	
#if defined(ADVANCED_IPQOS)
		nBytesSent += req_format_write(wp, (
#if defined(CONFIG_RTL_HW_QOS_SUPPORT) 
		"<td align=center width=\"\" ><font size=\"2\"><b>priority</b></font></td>\n"
		"<td align=center width=\"\" ><font size=\"2\"><b>weight</b></font></td>\n"
#endif
		"<td align=center width=\"\" ><font size=\"2\"><b>remark dscp</b></font></td>\n"));
		
		if(vlanSupport)
			nBytesSent += req_format_write(wp, (
			"<td align=center width=\"\" ><font size=\"2\"><b>remark 802.1p</b></font></td>\n"));
#else
		nBytesSent += req_format_write(wp, (
		"<td align=center width=\"\" ><font size=\"2\"><b>Comment</b></font></td>\n"));
#endif
		nBytesSent += req_format_write(wp, (
      	"<td align=center width=\"\" ><font size=\"2\"><b>Select</b></font></td></tr>\n"));

	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_QOS_RULE_TBL, (void *)&entry))
			return -1;
#if !defined(ADVANCED_IPQOS)
		if ( (entry.mode & QOS_RESTRICT_IP)  != 0) {
			tmpStr = inet_ntoa(*((struct in_addr *)entry.local_ip_start));
			strcpy(mac, tmpStr);
			tmpStr = inet_ntoa(*((struct in_addr *)entry.local_ip_end));
			sprintf(ip, "%s - %s", mac, tmpStr);
#ifdef CONFIG_IPV6
			strcpy(ip6, s4dashes);
#endif

			strcpy(mac, s4dashes);

#ifdef QOS_CLASSIFY_BY_DSCP
			strcpy(dscp, s4dashes);
#endif
		}
		else if ( (entry.mode & QOS_RESTRICT_MAC)  != 0) {
			sprintf(mac, "%02x%02x%02x%02x%02x%02x",
				entry.mac[0],entry.mac[1],entry.mac[2],entry.mac[3],entry.mac[4],entry.mac[5]);
			strcpy(ip, s4dashes);
#ifdef CONFIG_IPV6
			strcpy(ip6, s4dashes);
#endif
#ifdef QOS_CLASSIFY_BY_DSCP
			strcpy(dscp, s4dashes);
#endif

		}
#ifdef QOS_CLASSIFY_BY_DSCP
		else if ( (entry.mode & QOS_RESTRICT_DSCP)  != 0) {
			sprintf(dscp, "%d",entry.dscp);
			strcpy(ip, s4dashes);
			strcpy(mac, s4dashes);
#ifdef CONFIG_IPV6
			strcpy(ip6, s4dashes);
#endif

		}
#endif

#ifdef CONFIG_IPV6
		else if( (entry.mode & QOS_RESTRICT_IPV6)  != 0){
			strcpy(ip, s4dashes);
			strcpy(mac, s4dashes);
			strncpy(ip6,entry.ip6_src,40);
#ifdef QOS_CLASSIFY_BY_DSCP
			strcpy(dscp, s4dashes);
#endif
		}
#endif
		else //all
		{
			strcpy(ip, s4dashes);
			strcpy(mac, s4dashes);
#ifdef CONFIG_IPV6
			strcpy(ip6, s4dashes);
#endif
#ifdef QOS_CLASSIFY_BY_DSCP			
			strcpy(dscp, s4dashes);
#endif
		}

#else
	#ifdef CONFIG_IPV6
		strncpy(ip6,entry.ip6_src,40);
	#endif

		tmpStr = inet_ntoa(*((struct in_addr *)entry.local_ip_start));
		strcpy(mac, tmpStr);
		tmpStr = inet_ntoa(*((struct in_addr *)entry.local_ip_end));
		sprintf(ip, "%s-%s", mac, tmpStr);
		
		sprintf(localPort,"%d-%d",entry.local_port_start,entry.local_port_end);
		
		tmpStr = inet_ntoa(*((struct in_addr *)entry.remote_ip_start));
		strcpy(rmtipTmp,tmpStr);
		tmpStr = inet_ntoa(*((struct in_addr *)entry.remote_ip_end));
		sprintf(rmtip, "%s-%s", rmtipTmp, tmpStr);
		
		sprintf(rmtPort, "%d-%d", entry.remote_port_start, entry.remote_port_end);
		
		sprintf(mac, "%02x%02x%02x%02x%02x%02x",
					entry.mac[0],entry.mac[1],entry.mac[2],entry.mac[3],entry.mac[4],entry.mac[5]);
		if(staticIpSupport)
			sprintf(rmtMac, "%02x%02x%02x%02x%02x%02x",
					entry.dst_mac[0],entry.dst_mac[1],entry.dst_mac[2],entry.dst_mac[3],entry.dst_mac[4],entry.dst_mac[5]);

		// do not add remote mac qos rule in earlier static ip mode
		if(staticIpSupport == 0){
			if(entry.dst_mac[0] || entry.dst_mac[1] || entry.dst_mac[2] || entry.dst_mac[3] || entry.dst_mac[4] || entry.dst_mac[5])
				continue;	
		}
		switch(entry.protocol)
		{
			case 0:
				sprintf(protocolBuff,"tcp+udp");
				break;
			case 1:
				sprintf(protocolBuff,"tcp");
				break;
			case 2:
				sprintf(protocolBuff,"udp");
				break;
			default:
				sprintf(protocolBuff,s4dashes);
				break;
		}
		
		if( (entry.mode & QOS_RESTRICT_IPV6)  != 0)
			sprintf(ipversion,"%s","ipv6");
		else
			sprintf(ipversion,"%s","ipv4");
		
		if(entry.phyport==ADVANCED_IPQOS_DEF_CHAR_VALUE)
		{
			sprintf(ip_qos_phyport,s4dashes);
		}else
			sprintf(ip_qos_phyport,"%d",entry.phyport);
		if(entry.dscp==ADVANCED_IPQOS_DEF_CHAR_VALUE)
		{
			sprintf(ip_qos_dscp,s4dashes);
		}else
			sprintf(ip_qos_dscp,"%d",entry.dscp);

		if(entry.vlan_pri==ADVANCED_IPQOS_DEF_CHAR_VALUE)
		{
			sprintf(ip_qos_vlan_pri,s4dashes);
		}else{
			if(vlanSupport)
				sprintf(ip_qos_vlan_pri,"%d",entry.vlan_pri);
				
			else	// do not add 1p qos rule in earlier vlan mode
				continue;
		}
		
		if(entry.remark_dscp==ADVANCED_IPQOS_DEF_CHAR_VALUE)
		{
			sprintf(remark_ip_qos_dscp,s4dashes);
		}else
			sprintf(remark_ip_qos_dscp,"%d",entry.remark_dscp);

		if(entry.remark_vlan_pri==ADVANCED_IPQOS_DEF_CHAR_VALUE)
		{
			sprintf(remark_ip_qos_vlan_pri,s4dashes);
		}else{
			if(vlanSupport)
				sprintf(remark_ip_qos_vlan_pri,"%d",entry.remark_vlan_pri);	
				
			else	// do not add 1p qos rule in earlier vlan mode
				continue;
		}
#endif

		if ( (entry.mode & QOS_RESTRICT_MIN)  != 0)
			mode = (char *)_md1;
		else
			mode = (char *)_md2;

    if(entry.bandwidth == 0)
    	sprintf(bandwidth, "%s", "-");
		else
			snprintf(bandwidth, 10, "%ld", entry.bandwidth);

		if(entry.bandwidth_downlink == 0)
    	sprintf(bandwidth_downlink, "%s", "-");
		else
			snprintf(bandwidth_downlink, 10, "%ld", entry.bandwidth_downlink);
		
#if defined(ADVANCED_IPQOS)
		strcpy(entryName,entry.entry_name);
		#if defined(CONFIG_RTL_HW_QOS_SUPPORT)	
		if(entry.policy==0)
		{//priority
			if(entry.priority!=ADVANCED_IPQOS_DEF_CHAR_VALUE)
				sprintf(priority,"%d",entry.priority);
			else
				sprintf(priority,s4dashes);
			strcpy(weight, s4dashes);
		}
		else
		{//weight
			sprintf(weight,"%d",entry.weight);			
			strcpy(priority, s4dashes);
		}
		#endif
#endif

		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
#if defined(ADVANCED_IPQOS)
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
#endif
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
#if defined(ADVANCED_IPQOS)
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
#endif
#ifdef CONFIG_IPV6
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
#endif

      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"), 

#if defined(ADVANCED_IPQOS)
				entryName,ipversion,protocolBuff,
#endif
				ip,  
#if defined(ADVANCED_IPQOS)
				localPort,rmtip,rmtPort,
#endif
#ifdef CONFIG_IPV6
				ip6,
#endif
				mac
);

#if defined(ADVANCED_IPQOS)
			if(staticIpSupport)
				nBytesSent += req_format_write(wp, (
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"),
					rmtMac
				);

			nBytesSent += req_format_write(wp, (
      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"),

				ip_qos_phyport,
				ip_qos_dscp
			);
			
			if(vlanSupport)
				nBytesSent += req_format_write(wp, (
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"),
					ip_qos_vlan_pri
			);
#endif
			
#if defined(CONFIG_NETFILTER_XT_MATCH_LAYER7)
			nBytesSent += req_format_write(wp, (
      			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"), 
      			entry.l7_protocol
      		);
#endif     
			nBytesSent += req_format_write(wp, (
      			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"),
				mode, bandwidth, bandwidth_downlink
			);
			

#if defined(ADVANCED_IPQOS)
	
#ifdef CONFIG_RTL_HW_QOS_SUPPORT
		nBytesSent += req_format_write(wp, (
			"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"),
      		priority,weight
      	);
#endif
		nBytesSent += req_format_write(wp, (
		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"),
			remark_ip_qos_dscp
		);

		if(vlanSupport)
			nBytesSent += req_format_write(wp, (
  		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"),
  		remark_ip_qos_vlan_pri
  		);
#else
		nBytesSent += req_format_write(wp, (
		"<td align=center width=\"\" ><font size=\"2\">%s</td>\n"),
		entry.entry_name);
#endif
		nBytesSent += req_format_write(wp, (
		"<td align=center width=\"\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
		i);

	#if 0
#if defined(ADVANCED_IPQOS) && !defined(CONFIG_IPV6)
				entryName,ipversion,protocolBuff,
#endif
				ip,  
#if defined(ADVANCED_IPQOS) && !defined(CONFIG_IPV6)
				localPort,rmtip,rmtPort,
#endif
#ifdef CONFIG_IPV6
				ip6,
#endif

				mac,
#if defined(ADVANCED_IPQOS) && !defined(CONFIG_IPV6)
				if(staticIpSupport)
					rmtMac,

				ip_qos_phyport,
				ip_qos_dscp,
				if(vlanSupport)
					ip_qos_vlan_pri,
#endif

#if defined(CONFIG_NETFILTER_XT_MATCH_LAYER7)      	
				entry.l7_protocol,
#endif				
				mode, bandwidth, bandwidth_downlink,
#if defined(ADVANCED_IPQOS) && !defined(CONFIG_IPV6)		
#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
				priority,weight,
#endif
				remark_ip_qos_dscp,
				if(vlanSupport)
				remark_ip_qos_vlan_pri,
#else	
				entry.entry_name, 
#endif
				i
		);
#endif
	}
	return nBytesSent;
}

int l7QosList(request *wp, int argc, char **argv)
{
	int	nBytesSent=0;

	nBytesSent += req_format_write(wp, ("<option value=\"Disable\">Disable</option>"));

#if defined(CONFIG_NETFILTER_XT_MATCH_LAYER7)
	if(0)
	{
		nBytesSent += req_format_write(wp, ("<option value=\"http\">http</option>"
		"<option value=\"bittorrent\">bittorrent</option>"
		"<option value=\"msnmessenger\">msnmessenger</option>"
		"<option value=\"doom3\">doom3</option>"
		));
	}
	else
	{
	
		#define READ_BUF_SIZE 512
		DIR *dir;
		struct dirent *next;
		
		pid_t   *pidList;
		int i=0,n=0,j=0;
		
		dir = opendir("/etc/l7-protocols/protocols");
		if (!dir)
		{
		        printf("find_pid_by_name: Cannot open /proc");
		        exit(1);
		}
		pidList = malloc(sizeof(*pidList)*5);
		while ((next = readdir(dir)) != NULL) {
			FILE *status;
		  char filename[READ_BUF_SIZE];
		  char buffer[READ_BUF_SIZE];
		  char name[READ_BUF_SIZE];
		  
		  char *lineptr = NULL;
		  char *str;
		  
		  /* Must skip ".." since that is outside /proc */
		  if (strcmp(next->d_name, "..") == 0)
		  	continue;
		  	
		  if (strstr(next->d_name, ".pat") == NULL)
		  	continue;
		
			lineptr = next->d_name;
			
			str = strsep(&lineptr,".");
			
			nBytesSent += req_format_write(wp, ("<option value=\"%s\">%s</option>"),str,str);
		
		}
		closedir(dir);
	}
	
#endif //#if defined(CONFIG_NETFILTER_XT_MATCH_LAYER7)
	
	return nBytesSent;
}

/////////////////////////////////////////////////////////////////////////////
void formIpQoS(request *wp, char *path, char *query)
{
	char *submitUrl, *strAdd, *strDel, *strVal, *strDelAll;
	char *strIpStart, *strIpEnd, *strMac, *strBandwidth, *strBandwidth_downlink, *strComment, *strL7Protocol;
	#ifdef QOS_CLASSIFY_BY_DSCP
	#ifndef ADVANCED_IPQOS
	char *strDscp;
	#endif
	#endif
#ifdef CONFIG_IPV6
	char *ip6_src;
#endif
#if defined(ADVANCED_IPQOS)
	char * strName, *strProtocol, *strLocalPortStart,*strLocalPortEnd;
	char  *strRmtIpStart,*strRmtIpEnd,*strRmtPortStart,*strRmtPortEnd;
	char *strRmtMac,*strPhyport,*strDscp,*strVlan_pri;
	#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
	char *strPolicy,*strPriority,*strWeight;
	#endif
	char *strRemakDscp,*strReMarkVlanPri;
	char *strIpversion;
	struct in_addr rIps, rIpe;
	unsigned char rmac[6];
	unsigned short lPortS, lPortE, rPortS, rPortE;
	unsigned int ip3, ip4;
#endif
	char tmpBuf[100];
	int entryNum, intVal, i, auto_uplink, auto_downlink;
	IPQOS_T entry, entry_tmp;
#ifndef NO_ACTION
	int pid;
#endif
	unsigned long totalUplinkBw, totalDownlinkBw;
	int ret;
	int j=0;
	unsigned int ip1,ip2;
#ifdef QOS_CLASSIFY_BY_DSCP
	int dscp;
#endif
	unsigned char mac[6];
	struct in_addr ips,ipe;
	int no_rule_flag = 0;

//displayPostDate(wp->post_data);

	strAdd = req_get_cstream_var(wp, ("addQos"), "");
#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strAdd[0]==0){
		strAdd = req_get_cstream_var(wp, ("addQosFlag"), "");
	}
#endif
	
	strDel = req_get_cstream_var(wp, ("deleteSel"), "");
	strDelAll = req_get_cstream_var(wp, ("deleteAll"), "");

	memset(&entry, '\0', sizeof(entry));

	if (strAdd[0]) {
		strVal = req_get_cstream_var(wp, ("enabled"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;
		if ( apmib_set( MIB_QOS_ENABLED, (void *)&intVal) == 0) {
			strcpy(tmpBuf, ("Set enabled flag error!"));
			goto setErr;
		}

		if (intVal == 0)
			goto setOk;

		strVal = req_get_cstream_var(wp, ("automaticUplinkSpeed"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;
		auto_uplink = intVal;

		if (intVal == 0) {
			strVal = req_get_cstream_var(wp, ("manualUplinkSpeed"), "");
			string_to_dec(strVal, &intVal);
			#if 0
			if ( apmib_set( MIB_QOS_MANUAL_UPLINK_SPEED, (void *)&intVal) == 0) {
				strcpy(tmpBuf, ("Set mib error!"));
				goto setErr;
			}
			#endif
			totalUplinkBw=intVal;
		}
		else{
			// Auto uplink speed
#if defined (CONFIG_RTL_8198)|| defined (CONFIG_RTL_8198C) || defined (CONFIG_RTL_8367R_SUPPORT) || defined(CONFIG_RTL_83XX_SUPPORT)
			totalUplinkBw=1024000;		// 1000Mbps
#else
			totalUplinkBw=102400;		// 100Mbps
#endif
		}

		strVal = req_get_cstream_var(wp, ("automaticDownlinkSpeed"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;
		auto_downlink = intVal;

		if (intVal == 0) {
			strVal = req_get_cstream_var(wp, ("manualDownlinkSpeed"), "");
			string_to_dec(strVal, &intVal);
			#if 0
			if ( apmib_set( MIB_QOS_MANUAL_DOWNLINK_SPEED, (void *)&intVal) == 0) {
				strcpy(tmpBuf, ("Set mib error!"));
				goto setErr;
			}
			#endif
			totalDownlinkBw=intVal;
		}
		else{
			// Auto uplink speed
#if defined (CONFIG_RTL_8198)|| defined (CONFIG_RTL_8198C) || defined (CONFIG_RTL_8367R_SUPPORT) || defined(CONFIG_RTL_83XX_SUPPORT)
			totalDownlinkBw=1024000;		// 1000Mbps
#else
			totalDownlinkBw=102400;		// 100Mbps
#endif
		}

		if(totalUplinkBw > MAX_BAND_WIDTH){
			strcpy(tmpBuf, ("mannual uplink exceed max range!"));
			goto setErr;
		}

		if(totalDownlinkBw > MAX_BAND_WIDTH){
			strcpy(tmpBuf, ("mannual downlink exceed max range!"));
			goto setErr;
		}

		strIpStart = req_get_cstream_var(wp, ("ipStart"), "");
		strIpEnd = req_get_cstream_var(wp, ("ipEnd"), "");
		strMac = req_get_cstream_var(wp, ("mac"), "");
#ifdef QOS_CLASSIFY_BY_DSCP
		strDscp = req_get_cstream_var(wp, ("dscp"), "");
#endif
#ifdef CONFIG_IPV6
		ip6_src = req_get_cstream_var(wp, ("ip6_src"), "");
#endif
		strBandwidth = req_get_cstream_var(wp, ("bandwidth"), "");
		strBandwidth_downlink = req_get_cstream_var(wp, ("bandwidth_downlink"), "");
		strComment = req_get_cstream_var(wp, ("comment"), "");
		strL7Protocol = req_get_cstream_var(wp, ("l7_protocol"), "");
#if defined(ADVANCED_IPQOS)		
		strProtocol = req_get_cstream_var(wp, ("protocol"), "");
	#ifndef CONFIG_IPV6
		strIpversion= req_get_cstream_var(wp, ("ipversion"), "");
	#endif

		strLocalPortStart = req_get_cstream_var(wp, ("localPortStart"), "");
		strLocalPortEnd = req_get_cstream_var(wp, ("localPortEnd"), "");
		strRmtIpStart = req_get_cstream_var(wp, ("rmt_ipStart"), "");
		strRmtIpEnd = req_get_cstream_var(wp, ("rmt_ipEnd"), "");
		strRmtPortStart = req_get_cstream_var(wp, ("rmt_portStart"), "");
		strRmtPortEnd = req_get_cstream_var(wp, ("rmt_portEnd"), "");
		strRmtMac = req_get_cstream_var(wp, ("dst_mac"), "");
		strPhyport = req_get_cstream_var(wp, ("phyport"), "");
		
		if(strPhyport[0])
		{
			entry.phyport=atoi(strPhyport);
		}else
		{
			entry.phyport= ADVANCED_IPQOS_DEF_CHAR_VALUE;
		}
		
		
		strDscp = req_get_cstream_var(wp, ("dscp"), "");
		if(strDscp[0])
		{
			entry.dscp=atoi(strDscp);
		}
		else
		{
			entry.dscp= ADVANCED_IPQOS_DEF_CHAR_VALUE;
		}
		strVlan_pri = req_get_cstream_var(wp, ("vlan_pri"), "");
		if(strVlan_pri[0])
		{
			entry.vlan_pri=atoi(strVlan_pri);
		}
		else
		{
			entry.vlan_pri= ADVANCED_IPQOS_DEF_CHAR_VALUE;
		}

		if(strProtocol[0]=='0' && !strIpStart[0] && !strIpEnd[0] && !strMac[0]
		 &&!strLocalPortStart[0]&& !strLocalPortEnd[0] && !strRmtIpStart[0] 
		 && !strRmtIpEnd[0]&&!strRmtPortStart[0]&& !strRmtPortStart[0] && !strRmtMac[0]
		 && !strPhyport[0] && !strDscp[0] && !strVlan_pri[0]
	#ifdef CONFIG_IPV6
		 && !ip6_src[0]
	#endif
		)
		{
		 	#if 0
			strcpy(tmpBuf, ("can't set qos rule nothing!"));
			goto setErr;
			#else
			no_rule_flag = 1;
			#endif
		}
		
		if(strProtocol[0] && strProtocol[0]!='0')
		{
			entry.protocol=atoi(strProtocol);
		}
		if(strIpStart[0])
			inet_aton(strIpStart, (struct in_addr *)&entry.local_ip_start);

		if(strIpEnd[0])
			inet_aton(strIpEnd, (struct in_addr *)&entry.local_ip_end);
		
		if(strLocalPortStart[0])
				entry.local_port_start=atoi(strLocalPortStart);
		if(strLocalPortEnd[0])
			entry.local_port_end=atoi(strLocalPortEnd);

		if(strRmtIpStart[0])
			inet_aton(strRmtIpStart, (struct in_addr *)&entry.remote_ip_start);
		if(strRmtIpEnd[0])
			inet_aton(strRmtIpEnd, (struct in_addr *)&entry.remote_ip_end);

		if(strRmtPortStart[0])
			entry.remote_port_start=atoi(strRmtPortStart);
		if(strRmtPortEnd[0])
			entry.remote_port_end=atoi(strRmtPortEnd);

		#ifndef CONFIG_IPV6
		entry.ipversion=atoi(strIpversion);
		#endif
		
		if (strMac[0] && !string_to_hex(strMac, entry.mac, 12))
		{
			strcpy(tmpBuf, ("MAC input fail!"));
			goto setErr;
		}		

		if (strRmtMac[0] && !string_to_hex(strRmtMac, entry.dst_mac, 12))
		{
			strcpy(tmpBuf, ("remote MAC input fail!"));
			goto setErr;
		}
		
		strVal = req_get_cstream_var(wp, ("addressType"), "");
		string_to_dec(strVal, &intVal);
		
		if (intVal == 0) { // IPv4
			inet_aton(strIpStart, &ips);
			inet_aton(strIpEnd, &ipe);
			inet_aton(strRmtIpStart, &rIps);
			inet_aton(strRmtIpEnd, &rIpe);
			lPortS = atoi(strLocalPortStart);
			lPortE = atoi(strLocalPortEnd);
			rPortS = atoi(strRmtPortStart);
			rPortE = atoi(strRmtPortEnd);
			//printf("ips:%x,ipe:%x,[%s]:[%d].\n",ips.s_addr,ipe.s_addr,__FUNCTION__,__LINE__);
						
			apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum);
		
			for(j=1;j<=entryNum;j++){
				*((char *)&entry_tmp) = (char)j;
				if ( apmib_get(MIB_QOS_RULE_TBL, (void *)&entry_tmp)){
					if(entry_tmp.mode & QOS_RESTRICT_IP)
					{						
						if(entry_tmp.local_ip_start[0]){
							ip1=(*((struct in_addr *)entry_tmp.local_ip_start)).s_addr;
							ip2=(*((struct in_addr *)entry_tmp.local_ip_end)).s_addr;
						}
						if(entry_tmp.remote_ip_start[0]){
							ip3=(*((struct in_addr *)entry_tmp.remote_ip_start)).s_addr;
							ip4=(*((struct in_addr *)entry_tmp.remote_ip_end)).s_addr;
						}

						if(entry_tmp.local_ip_start[0] && entry_tmp.local_port_start &&
							strIpStart[0] && strLocalPortStart[0])
						{
							if(((ips.s_addr < ip1) && (ipe.s_addr < ip1)) || 
								((ips.s_addr > ip2) && (ipe.s_addr > ip2)) ||
								((lPortS < entry_tmp.local_port_start) && (lPortE < entry_tmp.local_port_start)) || 
								((lPortS > entry_tmp.local_port_end) && (lPortE > entry_tmp.local_port_end))){
							}
							else
							{
								strcpy(tmpBuf, (" local ip address or local port conflict!"));
								goto setErr;
							}
						}
						else if(entry_tmp.local_ip_start[0] && strIpStart[0])
						{
							if(((ips.s_addr < ip1) && (ipe.s_addr < ip1)) || 
								((ips.s_addr > ip2) && (ipe.s_addr > ip2))){
							}
							else{
								strcpy(tmpBuf, (" local ip conflict!"));
								goto setErr;
							}
						}
						else if(entry_tmp.local_port_start && strLocalPortStart[0])
						{
							if(((lPortS < entry_tmp.local_port_start) && (lPortE < entry_tmp.local_port_start)) || 
								((lPortS > entry_tmp.local_port_end) && (lPortE > entry_tmp.local_port_end))){
							}
							else{
								strcpy(tmpBuf, (" local port conflict!"));
								goto setErr;
							}
						}

						if(entry_tmp.remote_ip_start[0] && entry_tmp.remote_port_start &&
							strRmtIpStart[0] && strRmtPortStart[0])
						{
							if(((rIps.s_addr < ip3) && (rIpe.s_addr < ip3)) || 
								((rIps.s_addr > ip4) && (rIpe.s_addr > ip4)) ||
								((rPortS < entry_tmp.remote_port_start) && (rPortE < entry_tmp.remote_port_start)) || 
								((rPortS > entry_tmp.remote_port_end) && (rPortE > entry_tmp.remote_port_end))){
							}
							else
							{
								strcpy(tmpBuf, (" remote ip address or local port conflict!"));
								goto setErr;
							}
						}
						else if(entry_tmp.remote_ip_start[0] && strRmtIpStart[0])
						{
							if(((rIps.s_addr < ip3) && (rIpe.s_addr < ip3)) || 
								((rIps.s_addr > ip4) && (rIpe.s_addr > ip4))){
							}
							else{
								strcpy(tmpBuf, (" remote ip conflict!"));
								goto setErr;
							}
						}
						else if(entry_tmp.remote_port_start && strRmtPortStart[0])
						{
							if(((rPortS < entry_tmp.remote_port_start) && (rPortE < entry_tmp.remote_port_start)) || 
								((rPortS > entry_tmp.remote_port_end) && (rPortE > entry_tmp.remote_port_end))){
							}
							else{
								strcpy(tmpBuf, (" remote port conflict!"));
								goto setErr;
							}
						}

					}
				}
			}
			
			inet_aton(strIpStart, (struct in_addr *)&entry.local_ip_start);
			inet_aton(strIpEnd, (struct in_addr *)&entry.local_ip_end);
			inet_aton(strRmtIpStart, (struct in_addr *)&entry.remote_ip_start);
			inet_aton(strRmtIpEnd, (struct in_addr *)&entry.remote_ip_end);
			entry.mode = QOS_RESTRICT_IP;
		}
		else if (intVal == 1) { //MAC
			string_to_hex(strMac, mac, 12);
			string_to_hex(strRmtMac, rmac, 12);
						
			apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum);
			
			for(j=1;j<=entryNum;j++)
			{
				*((char *)&entry_tmp) = (char)j;
				if ( apmib_get(MIB_QOS_RULE_TBL, (void *)&entry_tmp))
				{
					if(entry_tmp.mode & QOS_RESTRICT_MAC)
					{						
						if((entry_tmp.dst_mac[0] || entry_tmp.dst_mac[1] || entry_tmp.dst_mac[2] || entry_tmp.dst_mac[3] || entry_tmp.dst_mac[4] || entry_tmp.dst_mac[5]) &&
							(rmac[0] || rmac[1] || rmac[2] || rmac[3] || rmac[4] || rmac[5]))
						{
							if((entry_tmp.dst_mac[0]==rmac[0])&&(entry_tmp.dst_mac[1]==rmac[1])
								&&(entry_tmp.dst_mac[2]==rmac[2])&&(entry_tmp.dst_mac[3]==rmac[3])
								&&(entry_tmp.dst_mac[4]==rmac[4])&&(entry_tmp.dst_mac[5]==rmac[5]))
							{
								strcpy(tmpBuf, (" remote mac address conflict!"));
								goto setErr;
							}
						}
						else if((entry_tmp.mac[0] || entry_tmp.mac[1] || entry_tmp.mac[2] || entry_tmp.mac[3] || entry_tmp.mac[4] || entry_tmp.mac[5]) &&
							(mac[0] || mac[1] || mac[2] || mac[3] || mac[4] || mac[5]))
						{					
							if((entry_tmp.mac[0]==mac[0])&&(entry_tmp.mac[1]==mac[1])
								&&(entry_tmp.mac[2]==mac[2])&&(entry_tmp.mac[3]==mac[3])
								&&(entry_tmp.mac[4]==mac[4])&&(entry_tmp.mac[5]==mac[5]))	
							{
								strcpy(tmpBuf, (" mac address conflict!"));
								goto setErr;
							}
						}				
					}
				}
			}
			if(strMac[0]){
				if (!string_to_hex(strMac, entry.mac, 12))
				{
					strcpy(tmpBuf, ("MAC input fail!"));
					goto setErr;
				}
			}
			if(strRmtMac[0]){
				if (!string_to_hex(strRmtMac, entry.dst_mac, 12))
				{
					strcpy(tmpBuf, ("remote MAC input fail!"));
					goto setErr;
				}
			}
			entry.mode = QOS_RESTRICT_MAC;
		}
		else if(intVal == 2) { 

			#ifndef CONFIG_IPV6		//phyport
			apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum);
			for(j=1;j<=entryNum;j++)
			{
				*((char *)&entry_tmp) = (char)j;
				if ( apmib_get(MIB_QOS_RULE_TBL, (void *)&entry_tmp))
				{
					if(entry_tmp.mode & QOS_RESTRICT_PHYPORT)
					{
						if(entry_tmp.phyport==atoi(strPhyport)){
							strcpy(tmpBuf, ("phyport conflict!"));
							goto setErr;
						}
					}
				}
			}			
			entry.mode = QOS_RESTRICT_PHYPORT;
			#else		//IPV6
			
			apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum);	
			for(j=1;j<=entryNum;j++)
			{
				*((char *)&entry_tmp) = (char)j;
				if ( apmib_get(MIB_QOS_RULE_TBL, (void *)&entry_tmp))
				{
					if(entry_tmp.mode & QOS_RESTRICT_IPV6)
					{
						if((strlen(ip6_src) == strlen(entry_tmp.ip6_src)) && (memcmp(ip6_src,entry_tmp.ip6_src,strlen(ip6_src))==0))	
						{
							strcpy(tmpBuf, (" ipv6 address conflict!"));
							goto setErr;
						}
					}
				}
			}	
			strncpy(entry.ip6_src,ip6_src,40);
			entry.mode = QOS_RESTRICT_IPV6;
			#endif
		}
		else if(intVal == 3) { 
			
			#ifndef CONFIG_IPV6		//dscp
			apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum);
			for(j=1;j<=entryNum;j++)
			{
				*((char *)&entry_tmp) = (char)j;
				if ( apmib_get(MIB_QOS_RULE_TBL, (void *)&entry_tmp))
				{
					if(entry_tmp.mode & QOS_RESTRICT_DSCP)
					{
						if(entry_tmp.dscp==atoi(strDscp)){
							strcpy(tmpBuf, ("DSCP conflict!"));
							goto setErr;
						}
					}
				}
			}			
			entry.mode = QOS_RESTRICT_DSCP;
			
			#else		//phyport
			apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum);
			for(j=1;j<=entryNum;j++)
			{
				*((char *)&entry_tmp) = (char)j;
				if ( apmib_get(MIB_QOS_RULE_TBL, (void *)&entry_tmp))
				{
					if(entry_tmp.mode & QOS_RESTRICT_PHYPORT)
					{
						if(entry_tmp.phyport==atoi(strPhyport)){
							strcpy(tmpBuf, ("phyport conflict!"));
							goto setErr;
						}
					}
				}
			}			
			entry.mode = QOS_RESTRICT_PHYPORT;
			#endif
		}
		else  if(intVal == 4) { 

		#ifndef CONFIG_IPV6		//1p
			apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum);
			for(j=1;j<=entryNum;j++)
			{
				*((char *)&entry_tmp) = (char)j;
				if ( apmib_get(MIB_QOS_RULE_TBL, (void *)&entry_tmp))
				{
					if(entry_tmp.mode & QOS_RESTRICT_1P)
					{
						if(entry_tmp.vlan_pri==atoi(strVlan_pri)){
							strcpy(tmpBuf, ("1P conflict!"));
							goto setErr;
						}
					}
				}
			}					
			entry.mode = QOS_RESTRICT_1P; 
			
		#else		//dscp
			apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum);
			for(j=1;j<=entryNum;j++)
			{
				*((char *)&entry_tmp) = (char)j;
				if ( apmib_get(MIB_QOS_RULE_TBL, (void *)&entry_tmp))
				{
					if(entry_tmp.mode & QOS_RESTRICT_DSCP)
					{
						if(entry_tmp.dscp==atoi(strDscp)){
							strcpy(tmpBuf, ("DSCP conflict!"));
							goto setErr;
						}
					}
				}
			}			
			entry.mode = QOS_RESTRICT_DSCP;
		#endif
		}
		else{
			#ifdef CONFIG_IPV6		//1p
			apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum);
			for(j=1;j<=entryNum;j++)
			{
				*((char *)&entry_tmp) = (char)j;
				if ( apmib_get(MIB_QOS_RULE_TBL, (void *)&entry_tmp))
				{
					if(entry_tmp.mode & QOS_RESTRICT_1P)
					{
						if(entry_tmp.vlan_pri==atoi(strVlan_pri)){
							strcpy(tmpBuf, ("1P conflict!"));
							goto setErr;
						}
					}
				}
			}					
			entry.mode = QOS_RESTRICT_1P; 
			#endif
		}
#else
#ifndef CONFIG_IPV6
#ifdef QOS_CLASSIFY_BY_DSCP
		if (!strIpStart[0] && !strIpEnd[0] && !strMac[0] && !strBandwidth[0] && !strBandwidth_downlink[0] && !strComment[0] && !strDscp[0])
#else
		if (!strIpStart[0] && !strIpEnd[0] && !strMac[0] && !strBandwidth[0] && !strBandwidth_downlink[0] && !strComment[0])
#endif
			goto setOk;
#endif

		if(!strIpStart[0] && !strIpEnd[0] && !strMac[0] && !ip6_src[0])
		{
		 	#if 0
			strcpy(tmpBuf, ("can't set qos rule nothing!"));
			goto setErr;
			#else
			no_rule_flag = 1;
			#endif
		}

		if ( strL7Protocol[0] ) {
			strcpy((char *)entry.l7_protocol, strL7Protocol);
		}

		strVal = req_get_cstream_var(wp, ("addressType"), "");
		string_to_dec(strVal, &intVal);		
		if (intVal == 0) { // IP
			inet_aton(strIpStart, &ips);
			inet_aton(strIpEnd, &ipe);
			//printf("ips:%x,ipe:%x,[%s]:[%d].\n",ips.s_addr,ipe.s_addr,__FUNCTION__,__LINE__);
						
			apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum);
						
			for(j=1;j<=entryNum;j++)
			{
				*((char *)&entry_tmp) = (char)j;
				if ( apmib_get(MIB_QOS_RULE_TBL, (void *)&entry_tmp))
				{
					if(entry_tmp.mode & QOS_RESTRICT_IP)
					{
						ip1=(*((struct in_addr *)entry_tmp.local_ip_start)).s_addr;
						ip2=(*((struct in_addr *)entry_tmp.local_ip_end)).s_addr;
						//printf("ip1:%x,ip2:%x,[%s]:[%d].\n",ip1,ip2,__FUNCTION__,__LINE__);
						if(((ips.s_addr >= ip1) && (ips.s_addr <= ip2))
							||((ipe.s_addr >= ip1) && (ipe.s_addr <=ip2))
							||((ips.s_addr < ip1) && (ipe.s_addr > ip2)))
						{
							strcpy(tmpBuf, (" ip address conflict!"));
							goto setErr;
						}
						
					}
				}
			}
			inet_aton(strIpStart, (struct in_addr *)&entry.local_ip_start);
			inet_aton(strIpEnd, (struct in_addr *)&entry.local_ip_end);
			entry.mode = QOS_RESTRICT_IP;
		}
		else if (intVal == 1) { //MAC
			string_to_hex(strMac, mac, 12);
			apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum);
			
			for(j=1;j<=entryNum;j++)
			{
				*((char *)&entry_tmp) = (char)j;
				if ( apmib_get(MIB_QOS_RULE_TBL, (void *)&entry_tmp))
				{
					if(entry_tmp.mode & QOS_RESTRICT_MAC)
					{
						/*printf("[%s]:[%d]%02x%02x%02x%02x%02x%02x\n",__FUNCTION__,__LINE__,
						entry.mac[0],entry.mac[1],entry.mac[2],entry.mac[3],entry.mac[4],entry.mac[5]);*/
						if((entry_tmp.mac[0]==mac[0])&&(entry_tmp.mac[1]==mac[1])
						&&(entry_tmp.mac[2]==mac[2])&&(entry_tmp.mac[3]==mac[3])
						&&(entry_tmp.mac[4]==mac[4])&&(entry_tmp.mac[5]==mac[5]))
						{
							strcpy(tmpBuf, (" mac address conflict!"));
							goto setErr;
						}
						
					}
				}
			}
			if (!string_to_hex(strMac, entry.mac, 12))
			{
				strcpy(tmpBuf, ("MAC input fail!"));
				goto setErr;
			}
			entry.mode = QOS_RESTRICT_MAC;
		}
#ifdef QOS_CLASSIFY_BY_DSCP
		else if(intVal == 2){
			printf("strDscp:%s,[%s]:[%d].\n",strDscp,__FUNCTION__,__LINE__);
			if(strDscp==NULL)
			{
				strcpy(tmpBuf, ("dscp can't be null!should be(0~63)"));
				goto setErr;
			}
			string_to_dec(strDscp, &dscp);
			
			printf("strDscp:%s,:%d,[%s]:[%d].\n",strDscp,dscp,__FUNCTION__,__LINE__);
			if((dscp>63)||dscp<0)
			{
				strcpy(tmpBuf, ("dscp input fail!should be(0~63)"));
				goto setErr;
			}
			entry.mode = QOS_RESTRICT_DSCP;
			entry.dscp = dscp;
		}
#endif
#ifdef CONFIG_IPV6
		else if(intVal == 2){
			
			apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum);	
			
			for(j=1;j<=entryNum;j++)
			{
				*((char *)&entry_tmp) = (char)j;
				if ( apmib_get(MIB_QOS_RULE_TBL, (void *)&entry_tmp))
				{
					if(entry_tmp.mode & QOS_RESTRICT_IPV6)
					{
						if((strlen(ip6_src) == strlen(entry_tmp.ip6_src)) && (memcmp(ip6_src,entry_tmp.ip6_src,strlen(ip6_src))==0))	
						{
							strcpy(tmpBuf, (" ipv6 address conflict!"));
							goto setErr;
						}
						
					}
				}
			}	
			strncpy(entry.ip6_src,ip6_src,40);
			entry.mode = QOS_RESTRICT_IPV6;
		}
#endif
		else
		{
			entry.mode = QOS_RESTRICT_ALL;
		}
#endif
		strVal = req_get_cstream_var(wp, ("mode"), "");
		if (strVal[0] == '1')
			entry.mode |= QOS_RESTRICT_MIN;
		else
			entry.mode |= QOS_RESTRICT_MAX;

		string_to_dec(strBandwidth, &intVal);
		entry.bandwidth = (unsigned long)intVal;

		string_to_dec(strBandwidth_downlink, &intVal);
		entry.bandwidth_downlink = (unsigned long)intVal;

		if(entry.bandwidth==0 && entry.bandwidth_downlink==0 && no_rule_flag==0)
		{
			strcpy(tmpBuf, ("can't set qos rule without bandwith!"));
				goto setErr;
		}

		if((entry.bandwidth!=0 || entry.bandwidth_downlink!=0) && no_rule_flag==1)
		{
			strcpy(tmpBuf, ("can't set qos rule with nothing!"));
				goto setErr;
		}
	
		//To check uplink & downlink guaranteed minimum bandwidth
		if(entry.mode &  QOS_RESTRICT_MIN){
			ret=checkQosRuleBw(entry.bandwidth, entry.bandwidth_downlink, totalUplinkBw, totalDownlinkBw);
			if(ret==QOS_BW_CHECK_FAIL){
				strcpy(tmpBuf, ("checkQosRuleBw fail!"));
				goto setErr;
			}
			else if(ret==QOS_BOTHLINK_BW_OVERSIZE){
				strcpy(tmpBuf, ("Error: for guaranteed minimum bandwidth of both uplink and downlink, the sum bandwidth of all qos rules are larger than the total bandwidth!"));
				goto setErr;
			}
			else if(ret==QOS_DOWNLINK_BW_OVERSIZE){
				strcpy(tmpBuf, ("Error: for guaranteed minimum bandwidth of downlink, the sum bandwidth of all qos rules is larger than the total downlink bandwidth!"));
				goto setErr;
			}
			else if(ret==QOS_UPLINK_BW_OVERSIZE){
				strcpy(tmpBuf, ("Error: for guaranteed minimum bandwidth of uplink, the sum bandwidth of all qos rules is larger than the total uplink bandwidth!"));
				goto setErr;
			}
		}
		
		if(no_rule_flag==0)
		{
#if defined(CONFIG_IPV6) || !defined(ADVANCED_IPQOS)
		if ( strComment[0] ) {
			strcpy((char *)entry.entry_name, strComment);
		}
#endif
#if defined(ADVANCED_IPQOS)
		if(entry.bandwidth==0 && entry.bandwidth_downlink==0)
		{
			strcpy(tmpBuf, ("uplink and downlink both zero!"));
				goto setErr;
		}
		strName = req_get_cstream_var(wp, ("entry_name"), "");
		if(strName[0])
		{
			apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum);	
			
			for(j=1;j<=entryNum;j++)
			{
				*((char *)&entry_tmp) = (char)j;
				if ( apmib_get(MIB_QOS_RULE_TBL, (void *)&entry_tmp))
				{
					if((strlen(strName) == strlen(entry_tmp.entry_name)) && (memcmp(strName,entry_tmp.entry_name,strlen(strName))==0))	
					{
						strcpy(tmpBuf, (" entry name conflict!"));
						goto setErr;
					}
				}
			}	
			strcpy(entry.entry_name,strName);
		}

		#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
		strPolicy = req_get_cstream_var(wp, ("policyType"), "");
		if(strPolicy[0])
		{
			entry.policy=atoi(strPolicy);
		}
		strPriority = req_get_cstream_var(wp, ("priority"), "");

		/*priority:UI->MIB, 0->0xF0000000, 1->...,7->0x10000000, not set->0xFFFFFFFF, because the priority of tr098 is ranged from 1~0xFFFFFFFF*/
		if(entry.policy==0x0){	//sp 
			if(strPriority[0])
			{
				entry.priority=atoi(strPriority);
				entry.weight=ADVANCED_IPQOS_DEF_CHAR_VALUE;
			}
			else
			{
				#ifndef CONFIG_RTL_HARDWARE_IPV6_SUPPORT
				if(entry.mode & QOS_RESTRICT_IPV6)
				{
					entry.priority=ADVANCED_IPQOS_DEF_CHAR_VALUE;
					entry.weight=ADVANCED_IPQOS_DEF_CHAR_VALUE;
				}
				else
				#endif
				{
					strcpy(tmpBuf, (" priority cannot be empty under PRIO policy!"));
					goto setErr;
				}
			}
		}
		else {	//wfq
			strWeight = req_get_cstream_var(wp, ("weight"), "");
			if(strWeight[0]){
				entry.weight=atoi(strWeight);
				entry.priority=ADVANCED_IPQOS_DEF_CHAR_VALUE;
			}
			else{
				#ifndef CONFIG_RTL_HARDWARE_IPV6_SUPPORT
				if(entry.mode & QOS_RESTRICT_IPV6)
				{
					entry.priority=ADVANCED_IPQOS_DEF_CHAR_VALUE;
					entry.weight=ADVANCED_IPQOS_DEF_CHAR_VALUE;
				}
				else
				#endif
				{
					strcpy(tmpBuf, (" weight cannot be empty under WRR policy!"));
					goto setErr;
				}
			}
		}
		#endif
		
		strRemakDscp = req_get_cstream_var(wp, ("remark_dscp"), "");
		if(strRemakDscp[0])
		{
			entry.remark_dscp=atoi(strRemakDscp);
		}
		else
		{
			entry.remark_dscp=ADVANCED_IPQOS_DEF_CHAR_VALUE;
		}
		strReMarkVlanPri = req_get_cstream_var(wp, ("remark_vlan_pri"), "");
		if(strReMarkVlanPri[0])
		{
			entry.remark_vlan_pri=atoi(strReMarkVlanPri);
		}
		else
		{
			entry.remark_vlan_pri=ADVANCED_IPQOS_DEF_CHAR_VALUE;
		}
#endif
		entry.enabled = 1;
		if ( !apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get entry number error!"));
			goto setErr;
		}

		if ( (entryNum + 1) > MAX_QOS_RULE_NUM) {
			strcpy(tmpBuf, ("Cannot add new entry because table is full!"));
			goto setErr;
		}

		// set to MIB. try to delete it first to avoid duplicate case		
		apmib_set(MIB_QOS_DEL, (void *)&entry);
		if ( apmib_set(MIB_QOS_ADD, (void *)&entry) == 0) {
			strcpy(tmpBuf, ("Add table entry error!"));
			goto setErr;
		}
		else{
		
			if ( apmib_set( MIB_QOS_AUTO_UPLINK_SPEED, (void *)&auto_uplink) == 0) {
				strcpy(tmpBuf, ("Set mib error!"));
				goto setErr;
			}
		
			if ( apmib_set( MIB_QOS_AUTO_DOWNLINK_SPEED, (void *)&auto_downlink) == 0) {	// set auto & mannual status after rule are taken effects
				strcpy(tmpBuf, ("Set mib error!"));
				goto setErr;
			}

			if(auto_uplink == 0){
				if ( apmib_set( MIB_QOS_MANUAL_UPLINK_SPEED, (void *)&totalUplinkBw) == 0) {
					strcpy(tmpBuf, ("Set mib error!"));
					goto setErr;
				}
			}
		
			if(auto_downlink == 0){
				if ( apmib_set( MIB_QOS_MANUAL_DOWNLINK_SPEED, (void *)&totalDownlinkBw) == 0) {
					strcpy(tmpBuf, ("Set mib error!"));
					goto setErr;
				}
			}	
		}
		}
		else {		// no rule
		
			if ( apmib_set( MIB_QOS_AUTO_UPLINK_SPEED, (void *)&auto_uplink) == 0) {
				strcpy(tmpBuf, ("Set mib error!"));
				goto setErr;
			}
		
			if ( apmib_set( MIB_QOS_AUTO_DOWNLINK_SPEED, (void *)&auto_downlink) == 0) {	// set auto & mannual status after rule are taken effects
				strcpy(tmpBuf, ("Set mib error!"));
				goto setErr;
			}

			if(auto_uplink == 0){
				if ( apmib_set( MIB_QOS_MANUAL_UPLINK_SPEED, (void *)&totalUplinkBw) == 0) {
					strcpy(tmpBuf, ("Set mib error!"));
					goto setErr;
				}
			}
		
			if(auto_downlink == 0){
				if ( apmib_set( MIB_QOS_MANUAL_DOWNLINK_SPEED, (void *)&totalDownlinkBw) == 0) {
					strcpy(tmpBuf, ("Set mib error!"));
					goto setErr;
				}
			}
		}
	}

	/* Delete entry */
	if (strDel[0]) {
		if ( !apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get entry number error!"));
			goto setErr;
		}	

		for (i=entryNum; i>0; i--) {
			snprintf(tmpBuf, 20, "select%d", i);

			strVal = req_get_cstream_var(wp, tmpBuf, "");
			if ( !strcmp(strVal, "ON") ) {
				*((char *)&entry) = (char)i;
				if ( !apmib_get(MIB_QOS_RULE_TBL, (void *)&entry)) {
					strcpy(tmpBuf, ("Get table entry error!"));
					goto setErr;
				}
				if ( !apmib_set(MIB_QOS_DEL, (void *)&entry)) {
					strcpy(tmpBuf, ("Delete table entry error!"));
					goto setErr;
				}
			}
		}
	}

	/* Delete all entry */
	if ( strDelAll[0]) {
		if ( !apmib_set(MIB_QOS_DELALL, (void *)&entry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr;
		}	
	}

setOk:
	apmib_update_web(CURRENT_SETTING);

#ifndef NO_ACTION
	pid = fork();
        if (pid) {
	      	waitpid(pid, NULL, 0);
	}
        else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _QOS_SCRIPT_PROG);
		execl( tmpBuf, _QOS_SCRIPT_PROG, NULL);
             exit(1);
        }
#ifdef CONFIG_IPV6
			pid = fork();
				if (pid) {
					waitpid(pid, NULL, 0);
			}
				else if (pid == 0) {
				snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _IPV6_FIREWALL_SCRIPT_PROG);
				execl( tmpBuf, _IPV6_FIREWALL_SCRIPT_PROG, NULL);
						exit(1);
				}
#endif
#endif

	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	
#ifdef MODULES_REINIT
		set_reinit_flag(REINIT_FLAG_WAN);
#endif
#ifdef REBOOT_CHECK
	if(needReboot == 1
		)
	{
		OK_MSG(submitUrl);
		return;
	}
#endif

	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
  	return;

setErr:
	ERR_MSG(tmpBuf);
}

#elif defined(GW_QOS_ENGINE)
/////////////////////////////////////////////////////////////////////////////
int qosList(request *wp, int argc, char **argv)
{
	int	entryNum;
	QOS_T entry;
	char buffer[120];
	char tmpBuf[80];
	int index;

	if ( !apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&entryNum)) {
		goto ret_empty;
	}
	index= atoi(argv[0]); // index shoud be 0 ~ 9
	index += 1;

	if( index <= entryNum)
	{
		*((char *)&entry) = (char)index;
		if ( !apmib_get(MIB_QOS_RULE_TBL, (void *)&entry))
		{
			goto ret_empty;
		}

              strcpy(tmpBuf, inet_ntoa(*((struct in_addr*)entry.local_ip_start)));
              strcpy(&tmpBuf[20], inet_ntoa(*((struct in_addr*)entry.local_ip_end)));
              strcpy(&tmpBuf[40], inet_ntoa(*((struct in_addr*)entry.remote_ip_start)));
              strcpy(&tmpBuf[60], inet_ntoa(*((struct in_addr*)entry.remote_ip_end)));
		 sprintf(buffer, "%d-%d-%d-%s-%s-%d-%d-%s-%s-%d-%d-%s", entry.enabled, entry.priority, entry.protocol,
                        tmpBuf, &tmpBuf[20],entry.local_port_start, entry.local_port_end,
                        &tmpBuf[40], &tmpBuf[60], entry.remote_port_start, entry.remote_port_end, entry.entry_name );

		req_format_write(wp, ("%s"), buffer);
	      return 0;
	}

ret_empty:
	req_format_write(wp, ("%s"), "");
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
#define _PROTOCOL_TCP   6
#define _PROTOCOL_UDP   17
#define _PROTOCOL_BOTH   257
#define _PORT_MIN       0
#define _PORT_MAX       65535

static QOS_T entry_for_save[MAX_QOS_RULE_NUM];

void formQoS(request *wp, char *path, char *query)
{
#ifndef NO_ACTION
    int pid;
#endif

    char *submitUrl;
    char tmpBuf[100];

    char *strIp, *endIp, *tmpStr, *strEnabled;
    char varName[48];
    int index=1, protocol_others;
    int intVal, valid_num;
    QOS_T entry;
    struct in_addr curIpAddr, curSubnet;
    unsigned long v1, v2, v3, v4;

    strEnabled = req_get_cstream_var(wp, ("config.qos_enabled"), "");
    if( !strcmp(strEnabled, "true"))
    {
        intVal=1;
    }
    else
        intVal=0;
    if ( apmib_set( MIB_QOS_ENABLED, (void *)&intVal) == 0) {
        strcpy(tmpBuf, ("Set QoS enabled flag error!"));
        goto setErr_qos;
    }
    if (intVal==0)
         goto setOk_qos;
    strEnabled = req_get_cstream_var(wp, ("config.qos_auto_trans_rate"), "");
    if( !strcmp(strEnabled, "true"))
        intVal=1;
    else
        intVal=0;
    if ( apmib_set( MIB_QOS_AUTO_UPLINK_SPEED, (void *)&intVal) == 0) {
        strcpy(tmpBuf, ("Set QoS error!"));
        goto setErr_qos;
    }

    if( intVal == 0)
    {
        tmpStr = req_get_cstream_var(wp, ("config.qos_max_trans_rate"), "");
          string_to_dec(tmpStr, &intVal);
        if ( apmib_set(MIB_QOS_MANUAL_UPLINK_SPEED, (void *)&intVal) == 0) {
            strcpy(tmpBuf, ("Set QoS error!"));
            goto setErr_qos;
        }
    }


/*    if ( !apmib_set(MIB_QOS_DELALL, (void *)&entry)) {
        strcpy(tmpBuf, ("Delete all table error!"));
        goto setErr_qos;
    } */

    for(index=0, valid_num=0; index<MAX_QOS_RULE_NUM; index++)
    {
        sprintf(varName, "config.qos_rules[%d].enabled", index);
        tmpStr = req_get_cstream_var(wp, varName, "");
        if( !strcmp(tmpStr, "true"))
            intVal=1;
        else
            intVal=0;
        entry.enabled = (unsigned char)intVal;

        sprintf(varName, "config.qos_rules[%d].entry_name", index);
        tmpStr = req_get_cstream_var(wp, varName, "");
        strcpy(entry.entry_name, tmpStr);

        if (intVal == 0 && tmpStr[0] == 0)
             continue;

        sprintf(varName, "config.qos_rules[%d].priority", index);
        tmpStr = req_get_cstream_var(wp, varName, "");
        string_to_dec(tmpStr, &intVal);
        entry.priority = (unsigned char)intVal;

        sprintf(varName, "config.qos_rules[%d].protocol_menu", index);
        tmpStr = req_get_cstream_var(wp, varName, "");
        if (!strcmp(tmpStr, "-1"))
            protocol_others = 1;
        else
            protocol_others = 0;

        sprintf(varName, "config.qos_rules[%d].protocol", index);
        tmpStr = req_get_cstream_var(wp, varName, "");
        string_to_dec(tmpStr, &intVal);
        entry.protocol = (unsigned short)intVal;

        sprintf(varName, "config.qos_rules[%d].local_ip_start", index);
        strIp = req_get_cstream_var(wp, varName, "");
        inet_aton(strIp, (struct in_addr *)&entry.local_ip_start);
        sprintf(varName, "config.qos_rules[%d].local_ip_end", index);
        endIp = req_get_cstream_var(wp, varName, "");
        inet_aton(endIp, (struct in_addr *)&entry.local_ip_end);
        getInAddr(BRIDGE_IF, IP_ADDR, (void *)&curIpAddr);
        getInAddr(BRIDGE_IF, SUBNET_MASK, (void *)&curSubnet);

        v1 = *((unsigned long *)entry.local_ip_start);
        v2 = *((unsigned long *)&curIpAddr);
        v3 = *((unsigned long *)&curSubnet);
        if ( (v1 & v3) != (v2 & v3) ) {
            sprintf(tmpBuf, "\'%s\': Local IP start \'%s\' is not in the LAN subnet",
                        entry.entry_name, strIp);
            goto setErr_qos;
        }
        v4 = *((unsigned long *)entry.local_ip_end);
        if ( (v4 & v3) != (v2 & v3) ) {
            sprintf(tmpBuf, "\'%s\': Local IP end \'%s\' is not in the LAN subnet",
                        entry.entry_name, endIp);
            goto setErr_qos;
        }
        if ( v1 > v4 ) {
            sprintf(tmpBuf, "\'%s\': Local IP start, \'%s\', must be less than or equal to local IP end, \'%s\'",
                        entry.entry_name, strIp, endIp);
            goto setErr_qos;
        }


        sprintf(varName, "config.qos_rules[%d].remote_ip_start", index);
        strIp = req_get_cstream_var(wp, varName, "");
        inet_aton(strIp, (struct in_addr *)&entry.remote_ip_start);
        sprintf(varName, "config.qos_rules[%d].remote_ip_end", index);
        endIp = req_get_cstream_var(wp, varName, "");
        inet_aton(endIp, (struct in_addr *)&entry.remote_ip_end);
        v1 = *((unsigned long *)entry.remote_ip_start);
        v4 = *((unsigned long *)entry.remote_ip_end);
        if ( (v1 & v3) == (v2 & v3) ) {
            sprintf(tmpBuf, "\'%s\': Remote IP start \'%s\' is in the LAN subnet",
                        entry.entry_name, strIp);
            goto setErr_qos;
        }
        if ( (v4 & v3) == (v2 & v3) ) {
            sprintf(tmpBuf, "\'%s\': Remote IP end \'%s\' is in the LAN subnet",
                        entry.entry_name, endIp);
            goto setErr_qos;
        }
        if ( v1 > v4 ) {
            sprintf(tmpBuf, "\'%s\': Remote IP start, \'%s\', must be less than or equal to remote IP end, \'%s\'",
                        entry.entry_name, strIp, endIp);
            goto setErr_qos;
        }

/*        if ((!protocol_others) &&
            ( entry.protocol  == _PROTOCOL_TCP || entry.protocol  == _PROTOCOL_UDP ||entry.protocol  == _PROTOCOL_BOTH)) */
        {
            sprintf(varName, "config.qos_rules[%d].local_port_start", index);
            tmpStr = req_get_cstream_var(wp, varName, "");
            string_to_dec(tmpStr, &intVal);
            entry.local_port_start = (unsigned short)intVal;
            sprintf(varName, "config.qos_rules[%d].local_port_end", index);
            tmpStr = req_get_cstream_var(wp, varName, "");
            string_to_dec(tmpStr, &intVal);
            entry.local_port_end = (unsigned short)intVal;

            sprintf(varName, "config.qos_rules[%d].remote_port_start", index);
            tmpStr = req_get_cstream_var(wp, varName, "");
            string_to_dec(tmpStr, &intVal);
            entry.remote_port_start = (unsigned short)intVal;
            sprintf(varName, "config.qos_rules[%d].remote_port_end", index);
            tmpStr = req_get_cstream_var(wp, varName, "");
            string_to_dec(tmpStr, &intVal);
            entry.remote_port_end = (unsigned short)intVal;

        }

/*        *((char *)&entry_existed) = (char)index;
        if ( !apmib_get(MIB_QOS_RULE_TBL, (void *)&entry_existed)) {
		strcpy(tmpBuf, ("Get table entry error!"));
		goto setErr_qos;
        }
        if ( !apmib_set(MIB_QOS_DEL, (void *)&entry_existed)) {
		strcpy(tmpBuf, ("Delete table entry error!"));
		goto setErr_qos;
        } */

/*        if ( apmib_set(MIB_QOS_ADD, (void *)&entry) == 0) {
            strcpy(tmpBuf, ("Add table entry error!"));
            goto setErr_qos;
        } */
        memcpy(&entry_for_save[valid_num], &entry, sizeof(QOS_T));
        valid_num++;

    }


    if ( !apmib_set(MIB_QOS_DELALL, (void *)&entry)) {
        strcpy(tmpBuf, ("Delete all table error!"));
        goto setErr_qos;
    }

    for(index=0; index<valid_num; index++)
    {
        if ( apmib_set(MIB_QOS_ADD, (void *)&entry_for_save[index]) == 0) {
            strcpy(tmpBuf, ("Add table entry error!"));
            goto setErr_qos;
        }
    }

setOk_qos:
    apmib_update_web(CURRENT_SETTING);

#ifndef NO_ACTION
    pid = fork();
    if (pid) {
        waitpid(pid, NULL, 0);
    }
    else if (pid == 0) {
        snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _QOS_SCRIPT_PROG);
        execl( tmpBuf, _QOS_SCRIPT_PROG, NULL);
        exit(1);
    }
#endif

    submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
    if (submitUrl[0])
        send_redirect_perm(wp, submitUrl);
    return;

setErr_qos:
    ERR_MSG(tmpBuf);
}
#endif

#ifdef CONFIG_CMCC
#ifdef RTL_NF_ALG_CTL
void formALGOnOff(request * wp, char *path, char *query)
{
	char	*str, *submitUrl;
	char	*str_alg;
	char tmpBuf[100];
	bool enable;
	str_alg = req_get_cstream_var(wp, "apply", "");
	if (str_alg[0]) {
		str = req_get_cstream_var(wp, "l2tp_algonoff", "");
		if(str[0])
		{
			if ((atoi(str)==1)||!strcmp(str, "on"))
				enable = 1;
			else
				enable = 0;
			rtk_alg_set("l2tp",enable);
		}else
			rtk_alg_set("l2tp",0);
		str = req_get_cstream_var(wp, "ipsec_algonoff", "");
		
		if(str[0])
		{
			if ((atoi(str)==1)||!strcmp(str, "on"))
				enable = 1;
			else
				enable = 0;
			rtk_alg_set("ipsec",enable);
		}else
			rtk_alg_set("ipsec",0);
		
		str = req_get_cstream_var(wp, "h323_algonoff", "");
		
		if(str[0])
		{
			if ((atoi(str)==1)||!strcmp(str, "on"))
				enable = 1;
			else
				enable = 0;
			rtk_alg_set("h323",enable);
		}else
			rtk_alg_set("h323",0);
		
		str = req_get_cstream_var(wp, "rtsp_algonoff", "");
		
		if(str[0])
		{
			if ((atoi(str)==1)||!strcmp(str, "on"))
				enable = 1;
			else
				enable = 0;
			rtk_alg_set("rtsp",enable);
		}else
			rtk_alg_set("rtsp",0);
		
		str = req_get_cstream_var(wp, "sip_algonoff", "");
	
		if(str[0])
		{
			if ((atoi(str)==1)||!strcmp(str, "on"))
				enable = 1;
			else
				enable = 0;
			rtk_alg_set("sip",enable);
		}else
			rtk_alg_set("sip",0);
		
		str = req_get_cstream_var(wp, "ftp_algonoff", "");
		
		if(str[0])
		{
			if ((atoi(str)==1)||!strcmp(str, "on"))
				enable = 1;
			else
				enable = 0;
			rtk_alg_set("ftp",enable);
		}else
			rtk_alg_set("ftp",0);
	
		str = req_get_cstream_var(wp, "pptp_algonoff", "");
		
		if(str[0])
		{
			if ((atoi(str)==1)||!strcmp(str, "on"))
				enable = 1;
			else
				enable = 0;
			rtk_alg_set("pptp",enable);
		}else
			rtk_alg_set("pptp",0);
		
		str = req_get_cstream_var(wp, "tftp_algonoff", "");
		
		if(str[0])
		{
			if ((atoi(str)==1)||!strcmp(str, "on"))
				enable = 1;
			else
				enable = 0;
			rtk_alg_set("tftp",enable);
		}else
			rtk_alg_set("tftp",0);
		
	
	}
	apmib_update_web(CURRENT_SETTING);
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	//printf("%s:%d get submit=%s \n",__FUNCTION__,__LINE__,submitUrl);
	if (submitUrl[0])
		OK_MSG(submitUrl);

return;

setErr:
	ERR_MSG(tmpBuf);


}

int rtk_alg_get(unsigned char *protocol,bool *enable)
{
	FILE *fp;
	char buf[64];
	unsigned char *saveStr;
	unsigned char *token;
	memset(buf,0,sizeof(buf));
	if((protocol == NULL)||enable == NULL)
		return -1;
	if ((fp = fopen("/proc/alg", "r")) != NULL)
	{
			while(fgets(buf, sizeof(buf),fp)){
				saveStr = buf;
				token = strsep(&saveStr," ");
				if(token != NULL)
				{
					if(strcmp(token, "|")==0)
					{
						token = strsep(&saveStr,"=");
						if(token != NULL)
							if(strcmp(token,protocol)==0)
							{
								if(saveStr != NULL)
								{
									*enable=atoi(saveStr);
									fclose(fp);
									return 0;
								}

							}
							
					}
				}
				memset(buf,0,sizeof(buf));
			}
			fclose(fp);
			return -1;
	}
	return -1;
	
}


int rtk_alg_set(unsigned char *protocol,bool enable)
{
	char buf[64];
	memset(buf,0,sizeof(buf));
	bool enable_tmp;
	if(protocol == NULL)
		return -1;
	enable=!enable;
	if(strcmp("ftp",protocol)==0)
		if(rtk_alg_get("ftp",&enable_tmp) == 0){
			apmib_set(MIB_ALG_FTP_DISABLE, (void *) &enable);
		}else{
			return -1;
		}
	else if(strcmp("tftp",protocol)==0)
		if(rtk_alg_get("tftp",&enable_tmp) == 0){
			apmib_set(MIB_ALG_TFTP_DISABLE, (void *) &enable);
		}else{

			return -1;
		}
	else if(strcmp("rtsp",protocol)==0)
		if(rtk_alg_get("rtsp",&enable_tmp) == 0){
			apmib_set(MIB_ALG_RTSP_DISABLE, (void *) &enable);
		}else{
			return -1;
		}
	else if(strcmp("pptp",protocol)==0)
		if(rtk_alg_get("pptp",&enable_tmp) == 0){
			apmib_set(MIB_ALG_PPTP_DISABLE, (void *) &enable);
		}else{
			return -1;
		}
	else if(strcmp("l2tp",protocol)==0)
		if(rtk_alg_get("l2tp",&enable_tmp) == 0){
			apmib_set(MIB_ALG_L2TP_DISABLE, (void *) &enable);
		}else{
			return -1;
		}
	else if(strcmp("ipsec",protocol)==0)
		if(rtk_alg_get("ipsec",&enable_tmp) == 0){
			apmib_set(MIB_ALG_IPSEC_DISABLE, (void *) &enable);
		}else{
			return -1;
		}
	else if(strcmp("sip",protocol)==0)
		if(rtk_alg_get("sip",&enable_tmp) == 0){
			apmib_set(MIB_ALG_SIP_DISABLE, (void *) &enable);
		}else{
			return -1;
		}
	else if(strcmp("h323",protocol)==0)
		if(rtk_alg_get("h323",&enable_tmp) == 0){
			apmib_set(MIB_ALG_H323_DISABLE, (void *) &enable);
		}else{
			return -1;
		}
	else
		return -1;
	sprintf(buf,"echo %s %d > /proc/alg\n",protocol,!enable);
	system(buf);
	return 0;
}
#endif

#endif
#if 0//def SAMBA_WEB_SUPPORT
int UserEditName(request *wp, int argc, char **argv)
{
	int 			nBytesSent = 0;
	int				index;
	STORAGE_USER_T	s_user;

	apmib_get(MIB_STORAGE_USER_EDIT_INDEX,(void*)&index);
	*((char*)&s_user) = (char)index;
	apmib_get(MIB_STORAGE_USER_TBL,(void*)&s_user);
	
	nBytesSent += req_format_write(wp, ("<tr>"
		"<td width=\"20%%\"><font size=2><b>Name:</b></td>\n"
		"<td width=\"50%%\"><font size=2>%s</td></tr>\n"),
		s_user.storage_user_name);
	
	return nBytesSent;
}

int GroupEditName(request *wp, int argc, char **argv)
{
	int 			nBytesSent = 0;
	int				index;
	STORAGE_GROUP_T	s_group;
	
	apmib_get(MIB_STORAGE_GROUP_EDIT_INDEX,(void*)&index);
	*((char*)&s_group) = (char)index;
	apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);
	
	nBytesSent += req_format_write(wp, ("<tr>"
		"<td width=\"20%%\"><font size=2><b>Group Name</b></td>\n"
		"<td width=\"50%%\"><font size=2>%s</td></tr>\n"),
		s_group.storage_group_name);

	return nBytesSent;
}

int ShareFolderList(request *wp, int argc, char **argv)
{
	int 			nBytesSent = 0,len = 0;
	int				number,i;
	STORAGE_GROUP_T	s_group;
	
	nBytesSent += req_format_write(wp, ("<tr>"
      	"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Diaplay Name</b></font></td>\n"
      	"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Shared Folder</b></font></td>\n"
      	"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Group</b></font></td>\n"
      	"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Access</b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Delete</b></font></td></tr>\n"));

	apmib_get(MIB_STORAGE_GROUP_TBL_NUM,(void*)&number);
	for(i = 0;i < number;i++)
	{
		memset(&s_group,'\0',sizeof(STORAGE_GROUP_T));
		*((char*)&s_group) = (char)(i+1);
		apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);

		if(s_group.storage_group_sharefolder_flag == 1){
			nBytesSent += req_format_write(wp, ("<tr>"
      			"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
      			"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
      			"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
      			"<td align=center width=\"20%%\" bgcolor=\"#808080\"><font size=\"2\"><b>%s</b></font></td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b><input type=\"checkbox\" value=\"%s\" name=\"delete%d\"></b></font></td></tr>\n"),
      			s_group.storage_group_displayname,s_group.storage_group_sharefolder,s_group.storage_group_name,s_group.storage_group_access,
      			s_group.storage_group_name,i);
		}
	}

	return nBytesSent;
}

int Storage_GeDirRoot(request *wp, int argc, char **argv)
{
	int 			nBytesSent = 0;
	char*			dir_name;
	char			tmpBuff[30];
	char			tmpBuff2[30];

	memset(tmpBuff,'\0',30);
	memset(tmpBuff2,'\0',30);
	apmib_get(MIB_STORAGE_FOLDER_LOCAL,(void*)tmpBuff);

	dir_name = strstr(tmpBuff,"sd");
	strcpy(tmpBuff2,"/tmp/usb/");
	strcat(tmpBuff2,dir_name);
	
	nBytesSent += req_format_write(wp, ("<tr>"
		"<td width=\"20%%\"><font size=2><b>Location</b></td>\n"
		"<td width=\"50%%\"><font size=2>%s</td></tr>\n"
		"<input type=\"hidden\" name=\"Location\" value=\"%s\">\n"),
		tmpBuff2,tmpBuff2);
	
	return nBytesSent;
}

int FolderList(request *wp, int argc, char **argv)
{
	int 			nBytesSent = 0,len;
	FILE 			*fp,*fp2;
	char			tmpBuff[100],tmpBuff2[100];
	char			strLocal[30],Location[30];
	char*			strRootDir;
	int				i = 0,index = 0,flag = 0,number;
	char			*p,*p2;
	STORAGE_GROUP_T	s_group;


	memset(tmpBuff,'\0',100);
	memset(tmpBuff2,'\0',100);
	memset(strLocal,'\0',30);
	
	apmib_get(MIB_STORAGE_FOLDER_LOCAL,(void*)strLocal);
	strRootDir = strstr(strLocal,"sd");
	snprintf(tmpBuff2,100,"ls /tmp/usb/%s >/tmp/tmp.txt",strRootDir);
	system(tmpBuff2);

	nBytesSent += req_format_write(wp, ("<tr>"
      	"<td align=center width=\"25%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Folder</b></font></td>\n"
      	"<td align=center width=\"25%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Group</b></font></td>\n"
      	"<td align=center width=\"25%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td>\n"
      	"<td align=center width=\"25%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Delete</b></font></td></tr>\n"));

	memset(tmpBuff,'\0',100);
	fp = fopen("/tmp/tmp.txt","r");
	if(fp == NULL)
	{
		return nBytesSent;
	}

	while(fgets(tmpBuff, 100, fp)){
		len = strlen(tmpBuff);
		tmpBuff[len-1] = '\0';
		snprintf(tmpBuff2,100,"ls -ld /tmp/usb/%s/%s >/tmp/tmp2.txt",strRootDir,tmpBuff);
		system(tmpBuff2);

		memset(tmpBuff2,'\0',100);
		fp2 = fopen("/tmp/tmp2.txt","r");
		if(fp2 == NULL){
			return nBytesSent;
		}
		
		if(fgets(tmpBuff2,100,fp2)){
			if(tmpBuff2[0] != 'd'){
				memset(tmpBuff,'\0',100);
				memset(tmpBuff2,'\0',100);
				fclose(fp2);
				continue;
			}
			p = tmpBuff2;

			while(i < 3){
				while(*p == ' '){
					p++;
				}
				p = strstr(p," ");
				i++;
			}

			while(*p == ' ')
				p++;

			p2 = strstr(p," ");
			*p2 = '\0';
			i  = 0;
		}

		apmib_get(MIB_STORAGE_GROUP_TBL_NUM,(void*)&number);
		for(i = 0;i < number;i++)
		{
			memset(&s_group,'\0',sizeof(STORAGE_GROUP_T));
			*((char*)&s_group) = (char)(i+1);
			apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);

			if(s_group.storage_group_sharefolder_flag == 1){
				memset(Location,'\0',30);
				snprintf(Location,30,"/tmp/usb/%s/%s",strRootDir,tmpBuff);
				if(!strcmp(Location,s_group.storage_group_sharefolder)){
					flag = 1;
					break;
				}
			}
		}			
		
		if(flag == 0){
			nBytesSent += req_format_write(wp, ("<tr>"
				"<td align=center width=\"25%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">/tmp/usb/%s/%s</td>\n"
      			"<td align=center width=\"25%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">--</td>\n"
      			"<td align=center width=\"25%%\" bgcolor=\"#C0C0C0\"><font size=\"2\"><input type=\"checkbox\" value=\"/tmp/usb/%s/%s\" name=\"select%d\" onClick=\"SelectClick(%d)\"></td>\n"
      			"<td align=center width=\"25%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" value=\"/tmp/usb/%s/%s\" name=\"delete%d\" onClick=\"DeleteClick(%d)\"></td></tr>\n"),
				strRootDir,tmpBuff,strRootDir,tmpBuff,index,index,strRootDir,tmpBuff,index,index);
			index++;
		}
		
		fclose(fp2);
		memset(tmpBuff,'\0',100);
		memset(tmpBuff2,'\0',100);
		flag = 0;
	}
	fclose(fp);

	nBytesSent += req_format_write(wp,(
		"<input type=\"hidden\"  name=\"DirNum\" value=\"%d\">\n"),
		index);
	return nBytesSent;
		
}

int DiskList(request *wp, int argc, char **argv)
{
	int 			nBytesSent = 0,len = 0;
	int				i,j = 0;
	char			capability[20],freeSize[20];
	long long		num1,num2;
	char			*ptr;
	FILE 			*fp;
	int				total_size,free_size;
	char			tmpBuff[100];
	unsigned char	local[10]; 
	
	nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
      	"<td align=center width=\"25%%\" ><font size=\"2\"><b>Partition</b></font></td>\n"
      	"<td align=center width=\"25%%\" ><font size=\"2\"><b>Capacity</b></font></td>\n"
		"<td align=center width=\"25%%\" ><font size=\"2\"><b>Free Space</b></font></td>\n"
      	/*"<td align=center width=\"25%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Create Share</b></font></td></tr>\n"*/));

	memset(tmpBuff,0,100);
	system("df >/tmp/df_tmp.txt");
	sleep(5);
	fp = fopen("/tmp/df_tmp.txt","r");
	if(fp == NULL)
		return nBytesSent;
	
	while (fgets(tmpBuff, 100, fp)) {
		ptr = strstr(tmpBuff, "/dev/sd");
		if (ptr) {
			local[j] =  ptr - tmpBuff;
			while(j++ < 4)
			{
				ptr = strstr(ptr," ");
				while(*ptr == ' '){
					*ptr++ = '\0';
				}
				local[j] = ptr - tmpBuff;
			}
			local[j] = ptr - tmpBuff;

			memset(capability,'\0',20);
			memset(freeSize,'\0',20);
			num1 = atoll(tmpBuff+local[1])/(1000*1000);
			num2 = (atoll(tmpBuff+local[1])/1000)%1000;
			snprintf(capability,20,"%lld.%lld(G)",num1,num2);
			num1 = atoll(tmpBuff+local[3])/(1000*1000);
			num2 = (atoll(tmpBuff+local[3])/1000)%1000;
			snprintf(freeSize,20,"%lld.%lld(G)",num1,num2);
			
			nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
				"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
     			"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      			/* "<td align=center width=\"25%%\" ><input type=\"submit\" name=\"create_share\" value=\"Create Share\" onClick=\"CreateShare('%s')\"></td></tr>\n" */),
				tmpBuff+local[0], capability, freeSize/* ,tmpBuff+local[0] */);

			memset(tmpBuff,0,100);
		}
		j = 0;
	}
	fclose(fp);
	system("rm /tmp/df_tmp.txt");

	return nBytesSent;
}

int Storage_DispalyUser(request *wp, int argc, char **argv)
{
	int nBytesSent = 0;
	STORAGE_USER_T s_user;
	int i;
	int number;
	
	nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
      	"<td align=center width=\"50%%\" ><font size=\"2\"><b>User Name</b></font></td>\n"
      	#if 0
      	"<td align=center width=\"25%%\" ><font size=\"2\"><b>Group</b></font></td>\n"
      	"<td align=center width=\"25%%\" ><font size=\"2\"><b>Edit</b></font></td>\n"
		#endif
      	"<td align=center width=\"50%%\" ><font size=\"2\"><b>Select</b></font></td></tr>\n"));

	apmib_get(MIB_STORAGE_USER_TBL_NUM,(void*)&number);
	
	for(i = 0;i < number;i++)
	{
		*((char*)&s_user) = (char)(i+1);
		apmib_get(MIB_STORAGE_USER_TBL,(void*)&s_user);
		
		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"50%%\" ><font size=\"2\">%s</td>\n"
			#if 0
      		"<td align=center width=\"25%%\" ><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"25%%\" ><input type=\"submit\" value=\"Edit\" onclick=\"UserEditClick('%d')\"></td>\n"
			#endif
			"<td align=center width=\"50%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
			s_user.storage_user_name, /*s_user.storage_user_group,(i+1),*/(i+1));
	}
	return nBytesSent;
}

int Storage_DispalyGroup(request *wp, int argc, char **argv)
{
	int nBytesSent = 0;
	STORAGE_GROUP_T s_group;
	int i;
	int number;
	
	nBytesSent += req_format_write(wp, ("<tr>"
      	"<td align=center width=\"25%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Group Name</b></font></td>\n"
      	"<td align=center width=\"25%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Access</b></font></td>\n"
      	"<td align=center width=\"25%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Edit</b></font></td>\n"
      	"<td align=center width=\"25%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Delete</b></font></td></tr>\n"));

	apmib_get(MIB_STORAGE_GROUP_TBL_NUM,(void*)&number);

	for(i = 0;i < number;i++)
	{
		*((char*)&s_group) = (char)(i+1);
		apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);
		
		nBytesSent += req_format_write(wp, ("<tr>"
			"<td align=center width=\"25%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"25%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"25%%\" bgcolor=\"#C0C0C0\"><input type=\"submit\" value=\"Edit\" onClick=\"GroupEditClick('%d')\"></td>\n"
      		"<td align=center width=\"25%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td>\n"),
			s_group.storage_group_name, s_group.storage_group_access,(i+1),(i+1));
		
	}
	return nBytesSent;
}

int Storage_GetGroupMember(request *wp, int argc, char **argv)
{
	int nBytesSent = 0;
	STORAGE_GROUP_T s_group;
	int i;
	int number;

	nBytesSent += req_format_write(wp,
		("<select name=\"Group\">\n"));
	
	apmib_get(MIB_STORAGE_GROUP_TBL_NUM,(void*)&number);

	for(i = 0;i < number;i++)
	{
		*((char*)&s_group) = (char)(i+1);
		apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);

		nBytesSent += req_format_write(wp,
			("<option value=\"%d\">%s</option>\n"),
			(i+1),s_group.storage_group_name);

	}

	nBytesSent += req_format_write(wp,
		("</select>\n"));

	return nBytesSent;
}
#endif

#endif // HOME_GATEWAY

