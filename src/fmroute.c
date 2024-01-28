/*
 *      Web server handler routines for management (password, save config, f/w update)
 *
 *      Authors: sc_yang <sc_yang@realtek.com.tw>
 *
 *      $Id
 *
 */
#ifdef ROUTE_SUPPORT
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/reboot.h>
#include <unistd.h>
#include <net/route.h>
#include "boa.h"
#include "asp_page.h"
#include "apmib.h"
#include "apform.h"
#include "utility.h"

#ifdef HOME_GATEWAY

int interface_list(request *wp, int argc, char **argv)
{

	int	nBytesSent=0, wan_num = 0, i = 0;
	unsigned char buffer[64] = {0};
	#if defined(MULTI_WAN_SUPPORT)
	WANIFACE_T entry = {0};

	nBytesSent += req_format_write(wp, "<select size=\"1\" id=\"iface\" name=\"iface\">"
									   "<option value=\"0\">LAN</option>\n"
									   );

	wan_num = WANIFACE_NUM;
	for(i = 1;i <= wan_num;i++)
	{
		memset(&entry, '\0', sizeof(entry));
		*((char *)&entry) = (char)i;
		if(!apmib_get(MIB_WANIFACE_TBL, (void *)&entry))
		{
			printf("%s:%d get MIB_WANIFACE_TBL fail!\n",__FUNCTION__,__LINE__);
			return -1;
		}
		if(entry.enable)
		{
			////wan index start from 1....
			sprintf(buffer,"<option value=\"%d\">WAN%d</option>\n",i,i);
			nBytesSent += req_format_write(wp, buffer);
		}	

	}
	
	nBytesSent += req_format_write(wp, "</select>\n");
	
	#else
	nBytesSent += req_format_write(wp, "<select size=\"1\" id=\"iface\" name=\"iface\">"
								 	   "<option value=\"0\">LAN</option>\n"
								 	   "<option value=\"1\">WAN</option>\n"
								 	   "</select>\n");
	#endif

	return nBytesSent;
}


#if defined(MULTI_WAN_SUPPORT)

static char *find_name_mapping(const char *if_name)
{
	int i = 0;
	int stridx = 0;
	unsigned char real_name[32] = {0};
	unsigned char display_name[32] = {0};
	int wan_idx = 0;
	
	if(if_name == NULL)
		return NULL;
	
	strcpy(real_name, if_name);
	for(stridx=0; stridx<32; stridx++){	
		if(real_name[stridx] == '.')
		{
			real_name[stridx] = '\0';
		}
	}
	
	wan_idx = getWanIdx(real_name);
	if(wan_idx < 0){
		if( !strcmp(real_name, RTL_BR0_DEV_NAME) ){
			snprintf(display_name, sizeof(display_name), "%s", "LAN" );
		}else
			return NULL;
	}else{
		snprintf(display_name, sizeof(display_name), "WAN%d", wan_idx);
	}
	
	return display_name;
}
#endif

int CheckkernelRouteList(STATICROUTE_Tp checkentry)
{
	char buff[256];
	char iface[30];
	int nl = 0;
	struct in_addr dest;
	struct in_addr gw;
	struct in_addr mask;
	int flgs, ref, use, metric, mtu, win, ir;
	char flags[64];
	unsigned long int d, g, m;
	STATICROUTE_T entry;
	DHCP_T dhcp;
	char *interface_WAN=NULL;
	char *interface_LAN=NULL;
	int isFound=0;
	int opmode=0;
	memset(&entry, '\0', sizeof(entry));
	apmib_get( MIB_WAN_DHCP, (void *)&dhcp);
	apmib_get(MIB_OP_MODE, (void *)&opmode);
	if ( dhcp == PPPOE || dhcp == PPTP || dhcp == L2TP) {
			interface_WAN = PPPOE_IF;
	}else
			interface_WAN = WAN_IF;
	if(opmode==2)
		interface_WAN = ("wlan0");
			
	interface_LAN = BRIDGE_IF;
	
	FILE *fp = fopen("/proc/net/route", "r");

	while (fgets(buff, sizeof(buff), fp) != NULL) {
		if (nl) {
			int ifl = 0;
			while (buff[ifl] != ' ' && buff[ifl] != '\t' && buff[ifl] != '\0')
				ifl++;
			strncpy(iface, buff, ifl);
			iface[ifl]='\0';
			if(!strcmp(iface, interface_WAN)){
				entry.interface=1;
			}
			if(!strcmp(iface, interface_LAN)){
				entry.interface=0;	
			}	
				
			buff[ifl] = 0;	/* interface */
			if (sscanf(buff + ifl + 1, "%lx%lx%X%d%d%d%lx%d%d%d",
					   &d, &g, &flgs, &ref, &use, &metric, &m, &mtu, &win,
					   &ir) != 10) {
				printf("Unsuported kernel route format\n");
			}
			ifl = 0;	/* parse flags */
			if (flgs & RTF_UP) {
				if (flgs & RTF_REJECT)
					flags[ifl++] = '!';
				else
					flags[ifl++] = 'U';
				if (flgs & RTF_GATEWAY)
					flags[ifl++] = 'G';
				if (flgs & RTF_HOST)
					flags[ifl++] = 'H';
				if (flgs & RTF_REINSTATE)
					flags[ifl++] = 'R';
				if (flgs & RTF_DYNAMIC)
					flags[ifl++] = 'D';
				if (flgs & RTF_MODIFIED)
					flags[ifl++] = 'M';
				flags[ifl] = 0;
				dest.s_addr = d;
				memcpy(&(entry.dstAddr), &(dest.s_addr), 4); 
				gw.s_addr = g;
				memcpy(&(entry.gateway), &(gw.s_addr), 4); 
				mask.s_addr = m;
			}
			
				if ( dhcp == PPPOE || dhcp == PPTP || dhcp == L2TP){
					if(checkentry->interface==1){
						if(!memcmp(checkentry->dstAddr, &entry.dstAddr, 4) && entry.interface == checkentry->interface ){
							isFound=1;		
							break;
						}
					}else{
						if(!memcmp(checkentry->dstAddr, &entry.dstAddr, 4) && !memcmp(checkentry->gateway, &entry.gateway, 4) && entry.interface == checkentry->interface ){
							isFound=1;		
							break;
						}
					}
				}else{//for lan interface
					if(!memcmp(checkentry->dstAddr, &entry.dstAddr, 4) && !memcmp(checkentry->gateway, &entry.gateway, 4) && entry.interface == checkentry->interface ){
						isFound=1;		
						break;
					}
				}
		}
		nl++;
		
	}
	if(fp)
		fclose(fp);
	if(isFound ==1)
		return 1;
	else
		return 0;
}


void formRoute(request *wp, char *path, char *query)
{
	char *submitUrl, *strAddRoute, *strDelRoute, *strVal, *strDelAllRoute;
	char *strIp, *strMask, *strGateway, *strRip, *tmpStr, *strMetric;
	char *strRip_Mode;
	char *strInterface;
	char *iface=NULL;
	char tmpBuf[100];
	char str1[30], str2[30], str3[30];
	int entryNum, intVal, i;
	STATICROUTE_T entry, checkentry;
	unsigned int * a1, *a2, *b1, *b2;
	int ok_msg =0 ;
#ifndef NO_ACTION
	int pid;
#endif
	int opmode=-1;
	int nat_mode=0;
	int get_wanip=0;
	DHCP_T dhcp;
	struct in_addr intaddr;
	RIP_OPMODE_T ripmode_tx;
	RIP_OPMODE_T ripmode_rx;
#ifdef RIP6_SUPPORT
	char *strRip6;
	RIP6_OPMODE_T rip6_set;
#endif
	unsigned long v1, v2, v3;
	int check_ip1=0;
	int check_ip2=0;
	unsigned long ipAddr, curIpAddr, curSubnet;
	int isExist=0;
	int enable_igmpproxy=0;	
#ifdef CONFIG_IPV6
	int enable_mldproxy=0;
#endif
	apmib_get( MIB_WAN_DHCP, (void *)&dhcp);
	apmib_get(MIB_OP_MODE, (void *)&opmode);
	strAddRoute = req_get_cstream_var(wp, ("addRoute"), "");
#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strAddRoute[0]==0){
		strAddRoute = req_get_cstream_var(wp, ("addRouteFlag"), "");
	}
#endif
	strDelRoute = req_get_cstream_var(wp, ("deleteSelRoute"), "");
	strDelAllRoute = req_get_cstream_var(wp, ("deleteAllRoute"), "");
	strRip	= req_get_cstream_var(wp, ("ripSetup"), "");
	memset(&entry, '\0', sizeof(entry));
	if(strRip[0]) {
		
		strVal = req_get_cstream_var(wp, ("enabled"), "");
		if ( !strcmp(strVal, "ON"))
                        intVal = 1;
                else
                        intVal = 0;	

		if ( apmib_set( MIB_RIP_ENABLED, (void *)&intVal) == 0) {
			strcpy(tmpBuf, ("Set enabled flag error!"));
			goto setErr_route;
		}
		
		if(intVal==0){
			nat_mode=1;
			if ( apmib_set(MIB_NAT_ENABLED, (void *)&nat_mode) == 0) {
				strcpy(tmpBuf, ("\"Set NAT enabled error!\""));
				goto setErr_route;
			}
			enable_igmpproxy = 0;	
			if ( apmib_set(MIB_IGMP_PROXY_DISABLED, (void *)&enable_igmpproxy) == 0) {
				strcpy(tmpBuf, ("\"Set IGMP PROXY disabled error!\""));
				goto setErr_route;
			}
			goto setOk_route;
		
#ifdef CONFIG_IPV6
			enable_mldproxy = 0;	
			if ( apmib_set(MIB_MLD_PROXY_DISABLED, (void *)&enable_mldproxy) == 0) {
				strcpy(tmpBuf, ("\"Set MLD PROXY disabled error!\""));
				goto setErr_route;
			}
#endif
		}
		
		tmpStr = req_get_cstream_var(wp, ("nat_enabled"), "");  
		if(tmpStr[0]){
			if(tmpStr[0] == '0')
				nat_mode=1;
			else
				nat_mode=0;
				
			if ( apmib_set(MIB_NAT_ENABLED, (void *)&nat_mode) == 0) {
				strcpy(tmpBuf, ("\"Set NAT enabled error!\""));
				goto setErr_route;
			}
			if(nat_mode==1){
				enable_igmpproxy = 0;
#ifdef CONFIG_IPV6
				enable_mldproxy=0;
#endif
			}
			else{
				enable_igmpproxy = 1;	
				
#ifdef CONFIG_IPV6
				enable_mldproxy=1;
#endif
			}	
			if ( apmib_set(MIB_IGMP_PROXY_DISABLED, (void *)&enable_igmpproxy) == 0) {
				strcpy(tmpBuf, ("\"Set IGMP PROXY disabled error!\""));
				goto setErr_route;
			}

			
#ifdef CONFIG_IPV6
			if ( apmib_set(MIB_MLD_PROXY_DISABLED, (void *)&enable_mldproxy) == 0) {
				strcpy(tmpBuf, ("\"Set MLD PROXY disabled error!\""));
				goto setErr_route;
			}
#endif
			
			
		}	
		
		strRip_Mode= req_get_cstream_var(wp, ("rip_tx"), "");
		if(strRip_Mode[0]) {
			ripmode_tx = strRip_Mode[0] - '0' ;
		}else
			ripmode_tx = DISABLE_MODE;	
		if (apmib_set( MIB_RIP_LAN_TX, (void *)&ripmode_tx) == 0) {
				strcpy(tmpBuf, ("\"Set RIP LAN Tx error!\""));
				goto setErr_route;
		}
		if(nat_mode ==0){
			if (apmib_set( MIB_RIP_WAN_TX, (void *)&ripmode_tx) == 0) {
					strcpy(tmpBuf, ("\"Set RIP WAN Tx error!\""));
				goto setErr_route;
			}
		}

		strRip_Mode= req_get_cstream_var(wp, ("rip_rx"), "");
		if(strRip_Mode[0]) {
			ripmode_rx = strRip_Mode[0] - '0' ;
		}else
			ripmode_rx = DISABLE_MODE;
		if (apmib_set( MIB_RIP_LAN_RX, (void *)&ripmode_rx) == 0) {
			strcpy(tmpBuf, ("\"Set RIP LAN Rx error!\""));
				goto setErr_route;
		}
		if(nat_mode ==0){
			if ( apmib_set( MIB_RIP_WAN_RX, (void *)&ripmode_rx) == 0) {
					strcpy(tmpBuf, ("\"Set RIP WAN Rx error!\""));
				goto setErr_route;
			}
		}

#ifdef RIP6_SUPPORT
		strRip6= req_get_cstream_var(wp, ("rip_v6"), "");
		//printf("strRip6: %s\n", strRip6);
		if(strRip6[0]) {
			rip6_set = strRip6[0] - '0' ;
		}else
			rip6_set = RIP6_DISABLE;
		if (apmib_set( MIB_RIP6_ENABLED, (void *)&rip6_set) == 0) {
			strcpy(tmpBuf, ("\"Set RIP LAN Rx error!\""));
				goto setErr_route;
		}
#endif
		ok_msg = 1;
	}
	/* Add new static route table */
	if (strAddRoute[0]) {
		strVal = req_get_cstream_var(wp, ("enabled"), "");
		if ( !strcmp(strVal, "ON"))
			intVal = 1;
		else
			intVal = 0;
		if ( apmib_set( MIB_STATICROUTE_ENABLED, (void *)&intVal) == 0) {
			strcpy(tmpBuf, ("Set enabled flag error!"));
			goto setErr_route;
		}

		strIp = req_get_cstream_var(wp, ("ipAddr"), "");
		strMask= req_get_cstream_var(wp, ("subnet"), "");
		strGateway= req_get_cstream_var(wp, ("gateway"), "");
		strInterface=req_get_cstream_var(wp, ("iface"), "");
		strMetric=req_get_cstream_var(wp, ("metric"), "");
		if (!strIp[0] && !strMask[0] && !strGateway[0])
			goto setOk_route;

		entry.interface = strInterface[0] -'0';
		#if 0
		if(strInterface[0]=='0')
			entry.interface=0;
		else if( strInterface[0]=='1')
			entry.interface=1;
		#endif
		#if 0
		if(entry.interface==1){
			if ( dhcp == PPPOE || dhcp == PPTP || dhcp == L2TP) {
				iface = PPPOE_IF;
			}else
				iface = WAN_IF;
		if(opmode==2)
				iface = ("wlan0");		
		}else
			iface=BRIDGE_IF;	
		#endif
		#if 0
		if(!strcmp(iface, "ppp0"))
			get_wanip = isConnectPPP();
		else
			get_wanip = getInAddr(iface, IP_ADDR, (void *)&intaddr);	
		#endif
		
		inet_aton(strIp,(struct in_addr *)&entry.dstAddr);
		inet_aton(strMask, (struct in_addr *)&entry.netmask);
		inet_aton(strGateway, (struct in_addr *)&entry.gateway);
		entry.metric=(unsigned char)atoi(strMetric);
		memcpy((void *)&v1, (void *)entry.dstAddr, 4);
		memcpy((void *)&v2, (void *)entry.netmask, 4);
		v2 = ~ntohl(v2);
		if (v2 & (v2 + 1)) {
			sprintf(tmpBuf, "\"<script>dw(route_invalid_netmask)</script> %s \"", strMask);
			goto setErr_route;
		}
		memcpy((void *)&v2, (void *)entry.netmask, 4);	
		if (v1 & ~v2) {
			strcpy(tmpBuf, "\"<script>dw(route_invalid_netmask_nomatch)</script>\"");
			goto setErr_route;
		} 

		#if 0
		if(strcmp(iface, "ppp0"))
		{
			memcpy((void *)&ipAddr, (void *)entry.gateway, 4);
			check_ip1= getInAddr(iface, IP_ADDR, (void *)&curIpAddr);
			check_ip2= getInAddr(iface, SUBNET_MASK, (void *)&curSubnet);
			v1 = ipAddr;
			v2 = curIpAddr;
			v3 = curSubnet;
			if (v1 && check_ip1 && check_ip2) 
			{
				if ( (v1 & v3) != (v2 & v3) ) 
				{
					if(entry.interface==1)
					{
						strcpy(tmpBuf, ("Invalid Gateway address! It should be set within wan subnet."));
					}else if(entry.interface ==2)
						strcpy(tmpBuf, ("Invalid Gateway address! It should be set within wan physical subnet."));
					 else
						strcpy(tmpBuf, ("Invalid Gateway address! It should be set within lan subnet."));
					goto setErr_route;
				}
			}
		}
		#endif
		
		if ( !apmib_get(MIB_STATICROUTE_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get entry number error!"));
			goto setErr_route;
		}

		if ( (entryNum + 1) > MAX_ROUTE_NUM) {
			strcpy(tmpBuf, ("Cannot add new entry because table is full!"));
			goto setErr_route;
		}
		a1 = (unsigned int *) entry.dstAddr ;
		a2 = (unsigned int *) entry.netmask;
		for(i=1; i <= entryNum ; i++) {
			*((char *)&checkentry) = (char)i;
			if ( !apmib_get(MIB_STATICROUTE_TBL, (void *)&checkentry)){
				strcpy(tmpBuf, ("get entry error!"));
				goto setErr_route;
			}
			b1 = (unsigned int *) checkentry.dstAddr ;
			b2 = (unsigned int *) checkentry.netmask;
			if((*a1 & *a2) == (*b1&*b2)){
				sprintf(tmpBuf, ("<script>dw(route_duplicate)</script> %d!"),i);
				goto setErr_route;
			}
		}
#ifndef REBOOT_CHECK
		sprintf(tmpBuf, "route add -net %s netmask %s gw %s dev %s metric %d", strIp, strMask, strGateway, iface, entry.metric);
		if(system(tmpBuf) != 0){
			strcpy(tmpBuf, "<script>dw(route_error)</script>");
			goto setErr_route ;
		}
#endif
		#if defined(CONFIG_APP_TR069)
		entry.Enable = 1;
		entry.InstanceNum=0;
		if (entry.interface){
			//wan interface
			entry.ifIndex = entry.interface;
		}
		else{
			//currently only lan interface, bit24 set as 1, fix me when multiple lan support layer3forward? 
			entry.ifIndex = 1<<24;
		}
		#endif
		// set to MIB. try to delete it first to avoid duplicate case
		apmib_set(MIB_STATICROUTE_DEL, (void *)&entry);
		if ( apmib_set(MIB_STATICROUTE_ADD, (void *)&entry) == 0) {
			strcpy(tmpBuf, ("Add table entry error!"));
			goto setErr_route;
		}
	}

	/* Delete entry */
	if (strDelRoute[0]) {
		
		
		if ( !apmib_get(MIB_STATICROUTE_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get entry number error!"));
			goto setErr_route;
		}

		for (i=entryNum; i>0; i--) {
			snprintf(tmpBuf, 20, "select%d", i);
			strVal = req_get_cstream_var(wp, tmpBuf, "");
			if ( !strcmp(strVal, "ON") ) {
				*((char *)&entry) = (char)i;
				if ( !apmib_get(MIB_STATICROUTE_TBL, (void *)&entry)) {
					strcpy(tmpBuf, ("Get table entry error!"));
					goto setErr_route;
				}
						
				strcpy(str1, inet_ntoa(*((struct in_addr *)entry.dstAddr)));
				strcpy(str2, inet_ntoa(*((struct in_addr *)entry.netmask)));
				strcpy(str3, inet_ntoa(*((struct in_addr *)entry.gateway)));
				
						isExist = CheckkernelRouteList(&entry);
						if(isExist){	
							if(dhcp == PPPOE || dhcp ==PPTP || dhcp == L2TP){
								if(entry.interface == 1)	 
									sprintf(tmpBuf, "route del -net %s netmask %s dev %s metric %d", str1, str2, iface, entry.metric);	
							else
								sprintf(tmpBuf, "route del -net %s netmask %s gw %s metric %d", str1, str2, str3, entry.metric);
							}else
								sprintf(tmpBuf, "route del -net %s netmask %s gw %s metric %d", str1, str2, str3, entry.metric);
							
				system(tmpBuf);
						}	
				if ( !apmib_set(MIB_STATICROUTE_DEL, (void *)&entry)) {
					strcpy(tmpBuf, ("Delete table entry error!"));
					goto setErr_route;
				}
			}
		}
	}

	/* Delete all entry */
	if ( strDelAllRoute[0]) {
		if ( !apmib_get(MIB_STATICROUTE_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get entry number error!"));
			goto setErr_route;
		}
		for(i=1; i <= entryNum ; i++) {
			*((char *)&entry) = (char)i;
			if ( !apmib_get(MIB_STATICROUTE_TBL, (void *)&entry)){
				strcpy(tmpBuf, ("get entry error!"));
				goto setErr_route;
			}
			strcpy(str1, inet_ntoa(*((struct in_addr *)entry.dstAddr)));
			strcpy(str2, inet_ntoa(*((struct in_addr *)entry.netmask)));
			strcpy(str3, inet_ntoa(*((struct in_addr *)entry.gateway)));
			sprintf(tmpBuf, "route del -net %s netmask %s gw %s metric %d", str1, str2, str3, entry.metric);
			system(tmpBuf);

		}
		if ( !apmib_set(MIB_STATICROUTE_DELALL, (void *)&entry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_route;
		}
	}

setOk_route:
	apmib_update_web(CURRENT_SETTING);
#ifdef MODULES_REINIT
	set_reinit_flag(REINIT_FLAG_ALL);
#endif
#ifdef REBOOT_CHECK
	run_init_script_flag = 1;
#endif
#ifndef NO_ACTION
	run_init_script("all");
#endif

	submitUrl = req_get_cstream_var(wp, ("submit-url"), "");   // hidden page
		OK_MSG(submitUrl);	
		return ;

setErr_route:
	ERR_MSG(tmpBuf);
}

#if defined(CONFIG_APP_PIMD) || defined(CONFIG_APP_RTL_QUAGGA)|| defined(CONFIG_APP_MROUTED)
extern unsigned char  interfacelist[20][20];
#ifdef REBOOT_CHECK
extern int	 run_init_script_flag;
#endif	
#endif
#ifdef CONFIG_APP_MROUTED
void formmroutedroute(request * wp, char *path, char *query)
{
	char	*str, *submitUrl;
	char	*str_MroutedSet,*str_restart,*str_AddIf,*str_AddTunnel,*str_DelIf,*str_DelAllIf,*str_DelTunnel,*str_DelAllTunnel;
	char *strVal,*str_AddAltnet,*str_DelAltnet,*str_DelAllAltnet;
	MROUTED_IF_T IfEntry,IfCheckEntry;
	MROUTED_TUNNEL_T TunnelEntry,TunnelCheckEntry;
	MROUTED_ALTNET_T AltnetEntry,AltnetCheckEntry;
	char tmpBuf[100];
	int entryNum,i;
	unsigned int ifindex,zero;
	unsigned int * a1, *a2, *b1, *b2;
	int MroutedEnable;
	
	str_MroutedSet = req_get_cstream_var(wp, "mroutedset", "");
	str_restart = req_get_cstream_var(wp, "restart", "");
	str_AddIf = req_get_cstream_var(wp, "add_If", "");
	str_AddTunnel = req_get_cstream_var(wp, "addtunnel", "");
	str_DelIf = req_get_cstream_var(wp, "delete_If", "");
	str_DelAllIf = req_get_cstream_var(wp, "deleteall_If", "");
	str_DelTunnel = req_get_cstream_var(wp, "deleteTunnel", "");
	str_DelAllTunnel = req_get_cstream_var(wp, "deleteAllTunnel", "");
	
	str_AddAltnet = req_get_cstream_var(wp, "addAltnet", "");
	str_DelAltnet = req_get_cstream_var(wp, "deleteAltnet", "");
	str_DelAllAltnet = req_get_cstream_var(wp, "deleteAllAltnet", "");
	
	if (str_MroutedSet[0]) {
	
		strVal = req_get_cstream_var(wp, "mrouted_on", "");
		
		if (!strcmp(strVal, "ON"))
				MroutedEnable = 1;
			else
				MroutedEnable = 0;
		if (!apmib_set(MIB_MROUTED_ENABLE, (void *)&MroutedEnable)) {
				strcpy(tmpBuf, "Set mrouted error!");
				goto setErr_route;
			}
		if(!MroutedEnable){
				if ( !apmib_set(MIB_MROUTED_IF_DELALL, (void *)&IfEntry)) {
					strcpy(tmpBuf, ("Delete all IfEntry table error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_MROUTED_TUNNEL_DELALL, (void *)&TunnelEntry)) {
					strcpy(tmpBuf, ("Delete all TunnelEntry table error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_MROUTED_IF_ALTNET_DELALL, (void *)&AltnetEntry)) {
					strcpy(tmpBuf, ("Delete all AltnetEntry table error!"));
					goto setErr_route;
				}
				goto setOk_change;
			}
	}

	if (str_restart[0]) {

			goto setOk_reatart;

		}
	
	if (str_AddIf[0]) {

		strVal = req_get_cstream_var(wp, ("mrouted_if"), "");
		ifindex=atoi(strVal);
		strcpy(IfEntry.MroutedIfName, interfacelist[ifindex]);
		strVal = req_get_cstream_var(wp, ("If_Metric"), "");
		if(strVal[0])
		{
			IfEntry.MroutedIfMetric=atoi(strVal);
			
		}else{
			IfEntry.MroutedIfMetric=0;

		}
		strVal = req_get_cstream_var(wp, ("If_Shreshold"), "");
		if(strVal[0])
		{
			IfEntry.MroutedIfThreshold=atoi(strVal);
			
		}else{
			IfEntry.MroutedIfThreshold=0;

		}
		strVal = req_get_cstream_var(wp, ("If_RateLimit"), "");
		if(strVal[0])
		{
			IfEntry.MroutedIfRateLimit=atoi(strVal);
			
		}else{
			IfEntry.MroutedIfRateLimit=0;

		}
		strVal = req_get_cstream_var(wp, ("If_Boundray_Ip"), "");
		if(strVal[0])
		{
			inet_aton(strVal,(struct in_addr *)&IfEntry.MroutedIfBoundryIp);
		}
		strVal = req_get_cstream_var(wp, ("If_Boundary_Mask_Len"), "");
		if(strVal[0])
		{
			IfEntry.MroutedIfBoundryIpLen=atoi(strVal);
		}else{
			IfEntry.MroutedIfBoundryIpLen=0;

		}
		

		if ( !apmib_get(MIB_MROUTED_IF_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get mrouted entry number error!"));
			goto setErr_route;
		}	

		if ( (entryNum + 1) > MAX_MROUTED_IF_NUM) {
			strcpy(tmpBuf, ("Cannot add new mrouted entry because table is full!"));
			goto setErr_route;
		}
		
		for(i=1; i <= entryNum ; i++) {
			*((char *)&IfCheckEntry) = (char)i;
			if ( !apmib_get(MIB_MROUTED_IF_TBL, (void *)&IfCheckEntry)){
				strcpy(tmpBuf, ("get mrouted entry error!"));
				goto setErr_route;
			}
			if(!strcmp(IfCheckEntry.MroutedIfName,IfEntry.MroutedIfName)){
				apmib_set(MIB_MROUTED_IF_DEL, (void *)&IfCheckEntry);
				break;
			}
				
		
		}
		if ( apmib_set(MIB_MROUTED_IF_ADD, (void *)&IfEntry) == 0) {
					strcpy(tmpBuf, ("Add mrouted  table entry error!"));
					goto setErr_route;
		}
		
		
	}
	if(str_DelIf[0]){
		if ( !apmib_get(MIB_MROUTED_IF_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get mrouted entry number error!"));
					goto setErr_route;
				}
				for (i=entryNum; i>0; i--) {
				snprintf(tmpBuf, 20, "select%d", i);
				strVal = req_get_cstream_var(wp, tmpBuf, "");
				if ( !strcmp(strVal, "ON") ) {
					*((char *)&IfEntry) = (char)i;
					if ( !apmib_get(MIB_MROUTED_IF_TBL, (void *)&IfEntry)) {
						strcpy(tmpBuf, ("Get mrouted table entry error!"));
						goto setErr_route;
					}
					if ( !apmib_set(MIB_MROUTED_IF_DEL, (void *)&IfEntry)) {
						strcpy(tmpBuf, ("Delete mrouted table entry error!"));
					goto setErr_route;
					}
				}
				}
		
	}
	if(str_DelAllIf[0]){
		if ( !apmib_set(MIB_MROUTED_IF_DELALL, (void *)&IfEntry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_route;
		}

		
	}

	if (str_AddAltnet[0]) {
		strVal = req_get_cstream_var(wp, ("Altnet_if"), "");
		ifindex=atoi(strVal);
		strcpy(AltnetEntry.MroutedIfAltnetName, interfacelist[ifindex]);
		strVal = req_get_cstream_var(wp, ("If_AltNet_Ip"), "");
		if(strVal[0])
		{
			inet_aton(strVal,(struct in_addr *)&AltnetEntry.MroutedIfAltnetIp);
		}
		strVal = req_get_cstream_var(wp, ("If_AltNet_Mask_Len"), "");
		if(strVal[0])
		{
			AltnetEntry.MroutedIfAltnetIpLen=atoi(strVal);
		}else{
			AltnetEntry.MroutedIfAltnetIpLen=0;

		}
		if ( !apmib_get(MIB_MROUTED_IF_ALTNET_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get mrouted entry number error!"));
			goto setErr_route;
		}	

		if ( (entryNum + 1) > MAX_MROUTED_ALTNET_NUM) {
			strcpy(tmpBuf, ("Cannot add new mrouted entry because table is full!"));
			goto setErr_route;
		}
		
		for(i=1; i <= entryNum ; i++) {
			*((char *)&AltnetCheckEntry) = (char)i;
			if ( !apmib_get(MIB_MROUTED_IF_ALTNET_TBL, (void *)&AltnetCheckEntry)){
				strcpy(tmpBuf, ("get mrouted entry error!"));
				goto setErr_route;
			}
			a1=(unsigned int *) AltnetCheckEntry.MroutedIfAltnetIp;
			b1 = (unsigned int *) AltnetEntry.MroutedIfAltnetIp ;
			if((!strcmp(AltnetCheckEntry.MroutedIfAltnetName,AltnetEntry.MroutedIfAltnetName))&&((*a1) == (*b1))){
				apmib_set(MIB_MROUTED_IF_ALTNET_DEL, (void *)&AltnetCheckEntry);
				break;
			}
				
		
		}
		if ( apmib_set(MIB_MROUTED_IF_ALTNET_ADD, (void *)&AltnetEntry) == 0) {
					strcpy(tmpBuf, ("Add mrouted  table entry error!"));
					goto setErr_route;
		}
		
	}
	if(str_DelAltnet[0]){
		if ( !apmib_get(MIB_MROUTED_IF_ALTNET_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get mrouted entry number error!"));
					goto setErr_route;
				}
				for (i=entryNum; i>0; i--) {
				snprintf(tmpBuf, 20, "select%d", i);
				strVal = req_get_cstream_var(wp, tmpBuf, "");
				if ( !strcmp(strVal, "ON") ) {
					*((char *)&AltnetEntry) = (char)i;
					if ( !apmib_get(MIB_MROUTED_IF_ALTNET_TBL, (void *)&AltnetEntry)) {
						strcpy(tmpBuf, ("Get mrouted table entry error!"));
						goto setErr_route;
					}
					if ( !apmib_set(MIB_MROUTED_IF_ALTNET_DEL, (void *)&AltnetEntry)) {
						strcpy(tmpBuf, ("Delete mrouted table entry error!"));
					goto setErr_route;
					}
				}
				}
		
	}
	if(str_DelAllAltnet[0]){
		if ( !apmib_set(MIB_MROUTED_IF_ALTNET_DELALL, (void *)&AltnetEntry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_route;
		}

		
	}


	
	if (str_AddTunnel[0]) {

		strVal = req_get_cstream_var(wp, ("Tunnel_Local_Ip"), "");
		if(strVal[0])
		{
			inet_aton(strVal,(struct in_addr *)&TunnelEntry.MroutedTunnelLocalIp);
		}
		strVal = req_get_cstream_var(wp, ("Tunnel_Remote_Ip"), "");
		if(strVal[0])
		{
			inet_aton(strVal,(struct in_addr *)&TunnelEntry.MroutedTunnelRemoteIp);
		}
		
		strVal = req_get_cstream_var(wp, ("Tunnel_Metric"), "");
		if(strVal[0])
		{
			TunnelEntry.MroutedTunnelMetric=atoi(strVal);
			
		}else{
			TunnelEntry.MroutedTunnelMetric=0;

		}
		strVal = req_get_cstream_var(wp, ("Tunnel_Shreshold"), "");
		if(strVal[0])
		{
			TunnelEntry.MroutedTunnelThreshold=atoi(strVal);
			
		}else{
			TunnelEntry.MroutedTunnelThreshold=0;

		}
		strVal = req_get_cstream_var(wp, ("Tunnel_RateLimit"), "");
		if(strVal[0])
		{
			TunnelEntry.MroutedTunnelRateLimit=atoi(strVal);
			
		}else{
			TunnelEntry.MroutedTunnelRateLimit=0;

		}
		strVal = req_get_cstream_var(wp, ("Tunnel_Boundray_Ip"), "");
		if(strVal[0])
		{
			inet_aton(strVal,(struct in_addr *)&TunnelEntry.MroutedTunnelBoundryIp);
		}
		strVal = req_get_cstream_var(wp, ("Tunnel_Boundray_Ip_Len"), "");
		if(strVal[0])
		{
			TunnelEntry.MroutedTunnelBoundryIpLen=atoi(strVal);
			
		}else{
			TunnelEntry.MroutedTunnelBoundryIpLen=0;

		}

		if ( !apmib_get(MIB_MROUTED_TUNNEL_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get mrouted entry number error!"));
			goto setErr_route;
		}	

		if ( (entryNum + 1) > MAX_MROUTED_TUNNEL_NUM) {
			strcpy(tmpBuf, ("Cannot add new mrouted entry because table is full!"));
			goto setErr_route;
		}
		
		for(i=1; i <= entryNum ; i++) {
			*((char *)&TunnelCheckEntry) = (char)i;
			if ( !apmib_get(MIB_MROUTED_TUNNEL_TBL, (void *)&TunnelCheckEntry)){
				strcpy(tmpBuf, ("get mrouted entry error!"));
				goto setErr_route;
			}

			a1=(unsigned int *) TunnelEntry.MroutedTunnelLocalIp;
			b1 = (unsigned int *) TunnelCheckEntry.MroutedTunnelLocalIp ;
			a2=(unsigned int *) TunnelEntry.MroutedTunnelRemoteIp;
			b2 = (unsigned int *) TunnelCheckEntry.MroutedTunnelRemoteIp ;
			if(((*a1) == (*b1))&&((*a2) == (*b2))){
				apmib_set(MIB_MROUTED_TUNNEL_DEL, (void *)&TunnelCheckEntry);
				break;
			}
				
		
		}
		if ( apmib_set(MIB_MROUTED_TUNNEL_ADD, (void *)&TunnelEntry) == 0) {
					strcpy(tmpBuf, ("Add mrouted  table entry error!"));
					goto setErr_route;
		}
		
		
	}
	if(str_DelTunnel[0]){
		if ( !apmib_get(MIB_MROUTED_TUNNEL_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get mrouted entry number error!"));
					goto setErr_route;
				}
				for (i=entryNum; i>0; i--) {
				snprintf(tmpBuf, 20, "select%d", i);
				strVal = req_get_cstream_var(wp, tmpBuf, "");
				if ( !strcmp(strVal, "ON") ) {
					*((char *)&TunnelEntry) = (char)i;
					if ( !apmib_get(MIB_MROUTED_TUNNEL_TBL, (void *)&TunnelEntry)) {
						strcpy(tmpBuf, ("Get mrouted table entry error!"));
						goto setErr_route;
					}
					if ( !apmib_set(MIB_MROUTED_TUNNEL_DEL, (void *)&TunnelEntry)) {
						strcpy(tmpBuf, ("Delete mrouted table entry error!"));
					goto setErr_route;
					}
				}
				}
		
	}
	if(str_DelAllTunnel[0]){
		if ( !apmib_set(MIB_MROUTED_TUNNEL_DELALL, (void *)&TunnelEntry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_route;
		}

		
	}
setOk_change:
	submitUrl = req_get_cstream_var(wp, ("submit-url"), "");   // hidden page
	apmib_update_web(CURRENT_SETTING);
	send_redirect_perm(wp, submitUrl);
	return;
setOk_reatart:

submitUrl = req_get_cstream_var(wp, ("submit-url"), "");   // hidden page
#ifdef REBOOT_CHECK
	{
		char tmpMsg[300];
		char lan_ip_buf[30], lan_ip[30];
#ifdef RTK_REINIT_SUPPORT
		char cmdBuf[64]={0};
#endif		
		sprintf(tmpMsg, "%s","<script>dw(route_change)</script>");
		apmib_get( MIB_IP_ADDR,  (void *)lan_ip_buf) ;
		sprintf(lan_ip,"%s",inet_ntoa(*((struct in_addr *)lan_ip_buf)) );
#ifdef RTK_REINIT_SUPPORT
		sprintf(cmdBuf,"reinitCli -e %d",REINIT_EVENT_INIT);
		system(cmdBuf);
#endif
		OK_MSG_FW(tmpMsg, submitUrl,APPLY_COUNTDOWN_TIME,lan_ip);
#ifdef REBOOT_CHECK
		run_init_script_flag = 1;
#endif		
#ifndef NO_ACTION
		run_init_script("all");

#endif	
	}
#else
	OK_MSG(submitUrl);
#endif

	
#ifdef MULTI_WAN_SUPPORT
//system("reboot\n");
#endif	
	return;

setErr_route:
	ERR_MSG(tmpBuf);

}

#endif
#ifdef CONFIG_APP_PIMD
void formpimdroute(request * wp, char *path, char *query)
{
	char	*str, *submitUrl;
	char	*str_pimdset,*str_restart,*str_rpsetup,*str_RpCdGroup,*str_AddInterface;
	char *str_delaterp,*str_deleteallrp,*str_deleteIf,*str_deleteAllIf;
	char *strVal;
	PIMD_RP_GROUP_T RpGroupentry,RpGroupcheckentry;
	PIMD_IF_T Ifentry,Ifcheckentry;
	char tmpBuf[100];
	int entryNum,i;
	int ifindex,len;
	unsigned int * a1, *a2, *b1, *b2,StaticRpIpGroupMaskLen,BspPriority,RpPriority,RpTime;
	unsigned char StaticRpIpAddr[4],StaticRpIpGroupAddr[4];
	//unsigned char ospfEnable;
	int PimdEnable,bgpdebug,routeinput,ospfinput;
	
	str_pimdset = req_get_cstream_var(wp, "pimdset", "");
	str_restart = req_get_cstream_var(wp, "restart", "");
	str_rpsetup = req_get_cstream_var(wp, "rpapply", "");
	str_RpCdGroup = req_get_cstream_var(wp, "add_rp_candidate_group", "");
	str_AddInterface = req_get_cstream_var(wp, "addinterface", "");
	str_delaterp=req_get_cstream_var(wp, "deleterp_group", "");
	str_deleteallrp=req_get_cstream_var(wp, "deleteallrp_group", "");
	str_deleteIf=req_get_cstream_var(wp, "deleteif", "");
	str_deleteAllIf=req_get_cstream_var(wp, "deleteallif", "");
	if (str_pimdset[0]) {
	
		strVal = req_get_cstream_var(wp, "pimd_on", "");
		
		if (!strcmp(strVal, "ON"))
				PimdEnable = 1;
			else
				PimdEnable = 0;
		if (!apmib_set(MIB_PIMD_ENABLE, (void *)&PimdEnable)) {
				strcpy(tmpBuf, "Set pimd error!");
				goto setErr_route;
			}
		if(!PimdEnable){
				StaticRpIpAddr[0]=0;
				StaticRpIpGroupAddr[0]=0;
				StaticRpIpGroupMaskLen=0;
				BspPriority=256;
				RpPriority=256;
				RpTime=0;
				if ( !apmib_set(MIB_PIMD_BSR_PRIORITY, (void *)&BspPriority)) {
					strcpy(tmpBuf, ("set StaticRpIpGroupMaskLen  error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_PIMD_RP_PRIORITY, (void *)&RpPriority)) {
					strcpy(tmpBuf, ("set StaticRpIpGroupMaskLen  error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_PIMD_RP_TIME, (void *)&RpTime)) {
					strcpy(tmpBuf, ("set StaticRpIpGroupMaskLen  error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_PIMD_STATIC_RP_IP_ADDRESS, (void *)StaticRpIpAddr)) {
					strcpy(tmpBuf, ("set StaticRpIpAddr  error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_PIMD_STATIC_RP_GROUP_ADDRESS, (void *)StaticRpIpGroupAddr)) {
					strcpy(tmpBuf, ("set StaticRpIpGroupAddr  error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_PIMD_STATIC_RP_GROUP_MASK_LEN, (void *)&StaticRpIpGroupMaskLen)) {
					strcpy(tmpBuf, ("set StaticRpIpGroupMaskLen  error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_PIMD_RP_GROUP_DELALL, (void *)&RpGroupentry)) {
					strcpy(tmpBuf, ("Delete all RpGroupentry table error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_PIMD_IF_DELALL, (void *)&Ifentry)) {
					strcpy(tmpBuf, ("Delete all Ifentry table error!"));
					goto setErr_route;
				}
				goto setOk_change;
			}
		
		
		strVal = req_get_cstream_var(wp, "bsr_candidate_priority", "");
		
		if (strVal[0]) {
		BspPriority=atoi(strVal);

		if (!apmib_set(MIB_PIMD_BSR_PRIORITY, (void *)&BspPriority)){
				strcpy(tmpBuf, "Set bsr_candidate_priority error!");
				goto setErr_route;
			}			
			}
		strVal = req_get_cstream_var(wp, "rp_candidate_priority", "");
		
		if (strVal[0]) {
		RpPriority=atoi(strVal);

		if (!apmib_set(MIB_PIMD_RP_PRIORITY, (void *)&RpPriority)){
				strcpy(tmpBuf, "Set rp_candidate_priority  error!");
				goto setErr_route;
			}			
			}
		strVal = req_get_cstream_var(wp, "rp_candidate_time", "");
		
		if (strVal[0]) {
		RpTime=atoi(strVal);

		if (!apmib_set(MIB_PIMD_RP_TIME, (void *)&RpTime)){
				strcpy(tmpBuf, "Set rp_candidate_time error!");
				goto setErr_route;
			}			
			}

		
		
		
		
	}

	if (str_restart[0]) {

			goto setOk_reatart;

		}
	if (str_rpsetup[0]) {

		strVal = req_get_cstream_var(wp, ("ip_address"), "");
		if(strVal[0])
		{

			inet_aton(strVal,(struct in_addr *)StaticRpIpAddr);
			if ( apmib_set(MIB_PIMD_STATIC_RP_IP_ADDRESS, (void *)StaticRpIpAddr) == 0) {
					strcpy(tmpBuf, ("Add StaticRpIpAddr error!"));
					goto setErr_route;
			}
		}
		strVal = req_get_cstream_var(wp, ("group_address"), "");
		if(strVal[0])
		{

			inet_aton(strVal,(struct in_addr *)StaticRpIpGroupAddr);
			if ( apmib_set(MIB_PIMD_STATIC_RP_GROUP_ADDRESS, (void *)StaticRpIpGroupAddr) == 0) {
					strcpy(tmpBuf, ("Add StaticRpIpGroupAddr error!"));
					goto setErr_route;
			}

		}


		
		strVal = req_get_cstream_var(wp, ("group_mask_len"), "");
		if(strVal[0])
		{
			StaticRpIpGroupMaskLen=atoi(strVal);
			if ( apmib_set(MIB_PIMD_STATIC_RP_GROUP_MASK_LEN, (void *)&StaticRpIpGroupMaskLen) == 0) {
					strcpy(tmpBuf, ("Add group_mask_len error!"));
					goto setErr_route;
			}

		}
		
	}
	if (str_RpCdGroup[0]) {
		strVal = req_get_cstream_var(wp, ("rp_group_address"), "");
		//strAddnetmask= req_get_cstream_var(wp, ("rp_group_mask_len"), "");
		if (!strVal[0])
			goto setOk_change;					
		inet_aton(strVal,(struct in_addr *)&RpGroupentry.PimdRpGroup);
		strVal = req_get_cstream_var(wp, ("rp_group_mask_len"), "");
		RpGroupentry.PimdRpGroupMaskLen=atoi(strVal);
			
		

		if ( !apmib_get(MIB_PIMD_RP_GROUP_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get pimdentry number error!"));
			goto setErr_route;
		}	

		if ( (entryNum + 1) > MAX_PIMD_RP_GROUP_NUM) {
			strcpy(tmpBuf, ("Cannot add new pimd entry because table is full!"));
			goto setErr_route;
		}
		for(i=1; i <= entryNum ; i++) {
			*((char *)&RpGroupcheckentry) = (char)i;
			if ( !apmib_get(MIB_PIMD_RP_GROUP_TBL, (void *)&RpGroupcheckentry)){
				strcpy(tmpBuf, ("get pimd entry error!"));
				goto setErr_route;
			}
			a1=(unsigned int *) RpGroupentry.PimdRpGroup;
			b1 = (unsigned int *) RpGroupcheckentry.PimdRpGroup ;
			if((*a1) == (*b1)){
				apmib_set(MIB_PIMD_RP_GROUP_DEL, (void *)&RpGroupcheckentry);
				break;
			}
				
		
		}
		if ( apmib_set(MIB_PIMD_RP_GROUP_ADD, (void *)&RpGroupentry) == 0) {
					strcpy(tmpBuf, ("Add pimd  table entry error!"));
					goto setErr_route;
		}


		

	}
	if(str_delaterp[0]){
		if ( !apmib_get(MIB_PIMD_RP_GROUP_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get pimd entry number error!"));
					goto setErr_route;
				}
				for (i=entryNum; i>0; i--) {
				snprintf(tmpBuf, 20, "select%d", i);
				strVal = req_get_cstream_var(wp, tmpBuf, "");
				if ( !strcmp(strVal, "ON") ) {
					*((char *)&RpGroupentry) = (char)i;
					if ( !apmib_get(MIB_PIMD_RP_GROUP_TBL, (void *)&RpGroupentry)) {
						strcpy(tmpBuf, ("Get pimd table entry error!"));
						goto setErr_route;
					}
					if ( !apmib_set(MIB_PIMD_RP_GROUP_DEL, (void *)&RpGroupentry)) {
						strcpy(tmpBuf, ("Delete pimd table entry error!"));
					goto setErr_route;
					}
				}
				}
		
	}
	if(str_deleteallrp[0]){
		if ( !apmib_set(MIB_PIMD_RP_GROUP_DELALL, (void *)&RpGroupentry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_route;
		}

		
	}


	if(str_AddInterface[0]){
		strVal = req_get_cstream_var(wp, ("pimd_if"), "");
		ifindex=atoi(strVal);
		strcpy(Ifentry.PimdIfName, interfacelist[ifindex ]);
		strVal = req_get_cstream_var(wp, ("dr_priority"), "");
		Ifentry.PimdIfDrPriroity=atoi(strVal);
		strVal = req_get_cstream_var(wp, ("alternative_network"), "");
		if(strVal[0])
		{
			inet_aton(strVal,(struct in_addr *)&Ifentry.PimdIfAlNetworkAddr);
			strVal = req_get_cstream_var(wp, ("alternative_network_mask_len"), "");
			if(strVal[0]){
				Ifentry.PimdIfAlNetworkMaskLen=atoi(strVal);
				}
		}else{

			Ifentry.PimdIfAlNetworkMaskLen=0;
		}
		strVal = req_get_cstream_var(wp, ("filter_network"), "");
		if(strVal[0])
		{
			inet_aton(strVal,(struct in_addr *)&Ifentry.PimdIfNetworkAddr);
			strVal = req_get_cstream_var(wp, ("filter_network_mask_len"), "");
			if(strVal[0]){
				Ifentry.PimdIfFlNetworkMaskLen=atoi(strVal);
				}
		}else{
			Ifentry.PimdIfFlNetworkMaskLen=0;
		}


		
		if ( !apmib_get(MIB_PIMD_IF_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get pimdentry number error!"));
			goto setErr_route;
		}	

		if ( (entryNum + 1) > MAX_PIMD_IF_NUM) {
			strcpy(tmpBuf, ("Cannot add new pimd entry because table is full!"));
			goto setErr_route;
		}
		for(i=1; i <= entryNum ; i++) {
			*((char *)&Ifcheckentry) = (char)i;
			if ( !apmib_get(MIB_PIMD_IF_TBL, (void *)&Ifcheckentry)){
				strcpy(tmpBuf, ("get pimd entry error!"));
				goto setErr_route;
			}
			if(!strcmp(Ifentry.PimdIfName,Ifcheckentry.PimdIfName)){
				apmib_set(MIB_PIMD_IF_DEL, (void *)&Ifcheckentry);
				break;
			}
				
		
		}
		if ( apmib_set(MIB_PIMD_IF_ADD, (void *)&Ifentry) == 0) {
					strcpy(tmpBuf, ("Add pimd  table entry error!"));
					goto setErr_route;
		}



		



		
	}
	if(str_deleteIf[0]){
		if ( !apmib_get(MIB_PIMD_IF_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get pimd entry number error!"));
					goto setErr_route;
				}
				for (i=entryNum; i>0; i--) {
				snprintf(tmpBuf, 20, "select%d", i);
				strVal = req_get_cstream_var(wp, tmpBuf, "");
				if ( !strcmp(strVal, "ON") ) {
					*((char *)&Ifentry) = (char)i;
					if ( !apmib_get(MIB_PIMD_IF_TBL, (void *)&Ifentry)) {
						strcpy(tmpBuf, ("Get pimd table entry error!"));
						goto setErr_route;
					}
					if ( !apmib_set(MIB_PIMD_IF_DEL, (void *)&Ifentry)) {
						strcpy(tmpBuf, ("Delete pimd table entry error!"));
					goto setErr_route;
					}
				}
				}
		
	}
	if(str_deleteAllIf[0]){
		if ( !apmib_set(MIB_PIMD_IF_DELALL, (void *)&Ifentry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_route;
		}

		
	}
setOk_change:
	submitUrl = req_get_cstream_var(wp, ("submit-url"), "");   // hidden page
	apmib_update_web(CURRENT_SETTING);
	send_redirect_perm(wp, submitUrl);
	return;
setOk_reatart:

submitUrl = req_get_cstream_var(wp, ("submit-url"), "");   // hidden page
#ifdef REBOOT_CHECK
	{
		char tmpMsg[300];
		char lan_ip_buf[30], lan_ip[30];
#ifdef RTK_REINIT_SUPPORT
		char cmdBuf[64]={0};
#endif		
		sprintf(tmpMsg, "%s","<script>dw(route_change)</script>");
		apmib_get( MIB_IP_ADDR,  (void *)lan_ip_buf) ;
		sprintf(lan_ip,"%s",inet_ntoa(*((struct in_addr *)lan_ip_buf)) );
#ifdef RTK_REINIT_SUPPORT
		sprintf(cmdBuf,"reinitCli -e %d",REINIT_EVENT_INIT);
		system(cmdBuf);
#endif
		OK_MSG_FW(tmpMsg, submitUrl,APPLY_COUNTDOWN_TIME,lan_ip);
#ifdef REBOOT_CHECK
		run_init_script_flag = 1;
#endif		
#ifndef NO_ACTION
		run_init_script("all");
#endif	
	}
#else
	OK_MSG(submitUrl);
#endif

	
#ifdef MULTI_WAN_SUPPORT
//system("reboot\n");
#endif		
	
	return;

setErr_route:
	ERR_MSG(tmpBuf);

}

#endif


#ifdef CONFIG_APP_RTL_QUAGGA

void formisisroute(request * wp, char *path, char *query)
{	
	char	*str, *submitUrl;
	char	*str_isisset,*str_restart,*str_netautype;
	char *str_deletenet,*str_deleteallnet;
	char *strVal;
	char *str_addinterface,*str_deleteif,*str_deleteallif;
	ISIS_NET_T isisentry,isischeckentry;
	ISIS_IF_T isisifentry,isisifcheckentry;
	char tmpBuf[100];
	int entryNum,i;
	int ifindex;
	unsigned int * a1, *a2, *b1, *b2,route_id,as_number;
	//unsigned char ospfEnable;
	int isisEnable,isisDebug,netlevel,netautype;
	unsigned int processid;
	char *strnet="";
	unsigned char str_net[50];

	
	
	str_isisset = req_get_cstream_var(wp, "isisset", "");
	str_restart = req_get_cstream_var(wp, "restart", "");
	str_netautype= req_get_cstream_var(wp, "addnet", "");
	str_deletenet = req_get_cstream_var(wp, "deletenetAuthentication", "");
	str_deleteallnet = req_get_cstream_var(wp, "deleteallnetAuthentication", "");
	str_addinterface= req_get_cstream_var(wp, "addinterface", "");
	str_deleteif = req_get_cstream_var(wp, "deleteif", "");
	str_deleteallif = req_get_cstream_var(wp, "deleteallif", "");
	if (str_isisset[0]) {	
		strVal = req_get_cstream_var(wp, "isis_on", "");
		
		if (!strcmp(strVal, "ON"))
				isisEnable = 1;
			else
				isisEnable = 0;
		if (!apmib_set(MIB_ISIS_ENABLE, (void *)&isisEnable)) {
				strcpy(tmpBuf, "Set ISIS error!");
				goto setErr_route;
			}
		if(!isisEnable){
				processid=0;
				strcpy(str_net,strnet);
				//strnet="";
				if ( !apmib_set(MIB_ISIS_PROCESS_ID, (void *)&processid)) {
					strcpy(tmpBuf, "set process id error");
					goto setErr_route;
				}
				if ( !apmib_set(MIB_ISIS_NET, (void *)str_net)) {
					strcpy(tmpBuf, "set isis net error");
					goto setErr_route;
				}
				if ( !apmib_set(MIB_ISIS_NET_AU_TYPE_DELALL, (void *)&isisentry)) {
					strcpy(tmpBuf, "Delete all network table error!");
					goto setErr_route;
				}
				if ( !apmib_set(MIB_ISIS_IF_DELALL, (void *)&isisifentry)) {
					strcpy(tmpBuf, "Delete all if table error!");
					goto setErr_route;
				}
				goto setOk_change;
			}
		
		strVal = req_get_cstream_var(wp, "isis_debug", "");


		if (!strcmp(strVal, "ON"))
				isisDebug = 1;
			else
				isisDebug = 0;
		if (!apmib_set(MIB_ISIS_DEBUG, (void *)&isisDebug)) {
				strcpy(tmpBuf, "Set isis debug error!");
				goto setErr_route;
			}
		strVal = req_get_cstream_var(wp, "processid", "");
		if (strVal[0]) {
		processid=atoi(strVal);

		if (!apmib_set(MIB_ISIS_PROCESS_ID, (void *)&processid)){
				strcpy(tmpBuf, "Set isis process error!");
				goto setErr_route;
			}			
			}
	
		else{
			processid=0;
			if (!apmib_set(MIB_ISIS_PROCESS_ID, (void *)&processid)){
				strcpy(tmpBuf, "Set isis process error!");
				goto setErr_route;
			}
			
			}	
		strVal = req_get_cstream_var(wp, "level_type", "");
		netlevel=atoi(strVal);
		if (!apmib_set(MIB_ISIS_NET_LEVEL, (void *)&netlevel)){
				strcpy(tmpBuf, "Set net level error!");
				goto setErr_route;
			}

		strVal = req_get_cstream_var(wp, "net", "");
		strcpy(str_net,strVal);
			if (!apmib_set(MIB_ISIS_NET, (void *)str_net)){
				strcpy(tmpBuf, "Set net error!");
				goto setErr_route;
			}	
	
	}

	if (str_restart[0]) {

			goto setOk_reatart;

		}
	if (str_netautype[0]) {
	
		strVal = req_get_cstream_var(wp, "net_authentication", "");
		isisentry.netautype=atoi(strVal);
		strVal = req_get_cstream_var(wp, "net_key", "");
		strcpy(isisentry.netkey,strVal);		
		if ( !apmib_get(MIB_ISIS_NET_AU_TYPE_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get isisentry number error!"));
			goto setErr_route;
		}

		if ( (entryNum + 1) > MAX_ISIS_NET_AU_TYPE) {
			strcpy(tmpBuf, ("Cannot add new isis entry because table is full!"));
			goto setErr_route;
		}
		for(i=1; i <= entryNum ; i++) {
			*((char *)&isischeckentry) = (char)i;
			if ( !apmib_get(MIB_ISIS_NET_AU_TYPE_TBL, (void *)&isischeckentry)){
				strcpy(tmpBuf, ("get isis entry error!"));
				goto setErr_route;
			}
	
			if(isisentry.netautype==isischeckentry.netautype){
				apmib_set(MIB_ISIS_NET_AU_TYPE_DEL, (void *)&isischeckentry);
				break;
			}
				
		
		}
		if ( apmib_set(MIB_ISIS_NET_AU_TYPE_ADD, (void *)&isisentry) == 0) {
					strcpy(tmpBuf, ("Add isis net table entry error!"));
					goto setErr_route;
		}



			
	}

	
	if (str_deletenet[0]) {
		if ( !apmib_get(MIB_ISIS_NET_AU_TYPE_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get isis entry number error!"));
					goto setErr_route;
				}
				for (i=entryNum; i>0; i--) {
				snprintf(tmpBuf, 20, "select%d", i);
				strVal = req_get_cstream_var(wp, tmpBuf, "");
				if ( !strcmp(strVal, "ON") ) {
					*((char *)&isisentry) = (char)i;
					if ( !apmib_get(MIB_ISIS_NET_AU_TYPE_TBL, (void *)&isisentry)) {
						strcpy(tmpBuf, ("Get isis table entry error!"));
						goto setErr_route;
					}
					if ( !apmib_set(MIB_ISIS_NET_AU_TYPE_DEL, (void *)&isisentry)) {
						strcpy(tmpBuf, ("Delete isis table entry error!"));
					goto setErr_route;
					}
				}
				}

	}
	if (str_deleteallnet[0]) {
		if ( !apmib_get(MIB_ISIS_NET_AU_TYPE_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get isis entry number error!"));
					goto setErr_route;
		}
		for(i=1; i <= entryNum ; i++) {
			*((char *)&isisentry) = (char)i;
			if ( !apmib_get(MIB_ISIS_NET_AU_TYPE_TBL, (void *)&isisentry)){
				strcpy(tmpBuf, ("get entry error!"));
				goto setErr_route;
			}	

		}
		if ( !apmib_set(MIB_ISIS_NET_AU_TYPE_DELALL, (void *)&isisentry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_route;
		}

	}

	if (str_addinterface[0]) {

		ifindex = atoi(req_get_cstream_var(wp, "isis_if", ""));

		strcpy(isisifentry.interfacename, interfacelist[ifindex ]);
		strVal = req_get_cstream_var(wp, "level_type", "");

		isisifentry.interfacelevel=atoi(strVal);
		strVal = req_get_cstream_var(wp, "metric", "");
	
		isisifentry.metric=atoi(strVal);
		strVal = req_get_cstream_var(wp, "if_authentication_type", "");
	
		isisifentry.ifautype=atoi(strVal);
		strVal = req_get_cstream_var(wp, "interface_key", "");

		strcpy(isisifentry.ifkey, strVal);
	
	
		if ( !apmib_get(MIB_ISIS_IF_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get isis if entry number error!"));
			goto setErr_route;
		}

		if ( (entryNum + 1) > MAX_ISIS_IF_NUM) {
			strcpy(tmpBuf, ("Cannot add new isis  if entry because table is full!"));
			goto setErr_route;
		}
		for(i=1; i <= entryNum ; i++) {
			*((char *)&isisifcheckentry) = (char)i;
			if ( !apmib_get(MIB_ISIS_IF_TBL, (void *)&isisifcheckentry)){
				strcpy(tmpBuf, ("get isis entry error!"));
				goto setErr_route;
			}
	
			if(!strcmp(isisifentry.interfacename,isisifcheckentry.interfacename)){
				apmib_set(MIB_ISIS_IF_DEL, (void *)&isisifcheckentry);
				break;
			}
				
		
		}


		if ( apmib_set(MIB_ISIS_IF_ADD, (void *)&isisifentry) == 0) {
					strcpy(tmpBuf, ("Add isis if table entry error!"));
					goto setErr_route;
		}

		}

	if (str_deleteif[0]) {
		if ( !apmib_get(MIB_ISIS_IF_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get isis entry number error!"));
					goto setErr_route;
				}
				for (i=entryNum; i>0; i--) {
				snprintf(tmpBuf, 20, "select%d", i);
				strVal = req_get_cstream_var(wp, tmpBuf, "");
				if ( !strcmp(strVal, "ON") ) {
					*((char *)&isisifentry) = (char)i;
					if ( !apmib_get(MIB_ISIS_IF_TBL, (void *)&isisifentry)) {
						strcpy(tmpBuf, ("Get isis table entry error!"));
						goto setErr_route;
					}
					if ( !apmib_set(MIB_ISIS_IF_DEL, (void *)&isisifentry)) {
						strcpy(tmpBuf, ("Delete isis table entry error!"));
					goto setErr_route;
					}
				}
				}

	}
	if (str_deleteallif[0]) {
		if ( !apmib_get(MIB_ISIS_IF_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get isis entry number error!"));
					goto setErr_route;
		}
		for(i=1; i <= entryNum ; i++) {
			*((char *)&isisifentry) = (char)i;
			if ( !apmib_get(MIB_ISIS_IF_TBL, (void *)&isisifentry)){
				strcpy(tmpBuf, ("get entry error!"));
				goto setErr_route;
			}	

		}
		if ( !apmib_set(MIB_ISIS_IF_DELALL, (void *)&isisifentry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_route;
		}

	}
setOk_change:
	submitUrl = req_get_cstream_var(wp, ("submit-url"), "");   // hidden page
	apmib_update_web(CURRENT_SETTING);
	send_redirect_perm(wp, submitUrl);
	return;
setOk_reatart:

submitUrl = req_get_cstream_var(wp, ("submit-url"), "");   // hidden page
#ifdef REBOOT_CHECK
	{
		char tmpMsg[300];
		char lan_ip_buf[30], lan_ip[30];
#ifdef RTK_REINIT_SUPPORT
		char cmdBuf[64]={0};
#endif		
		sprintf(tmpMsg, "%s","<script>dw(route_change)</script>");
		apmib_get( MIB_IP_ADDR,  (void *)lan_ip_buf) ;
		sprintf(lan_ip,"%s",inet_ntoa(*((struct in_addr *)lan_ip_buf)) );
#ifdef RTK_REINIT_SUPPORT
		sprintf(cmdBuf,"reinitCli -e %d",REINIT_EVENT_INIT);
		system(cmdBuf);
#endif
		OK_MSG_FW(tmpMsg, submitUrl,APPLY_COUNTDOWN_TIME,lan_ip);
#ifdef REBOOT_CHECK
		run_init_script_flag = 1;
#endif		
#ifndef NO_ACTION
		run_init_script("all");
#endif	
	}
#else
	OK_MSG(submitUrl);
#endif

	
#ifdef MULTI_WAN_SUPPORT
//system("reboot\n");
#endif	
	
	return;

setErr_route:
	ERR_MSG(tmpBuf);

	
	

}
void formbgproute(request * wp, char *path, char *query)
{
	char	*str, *submitUrl;
	char	*str_bgpfset,*str_restart;
	char *str_delatepeer,*str_deleteallpeer,*str_addpeer;
	char *strVal;
	char *str_bgpAddnet, *str_bgpdelnet, *str_bgpdelallnet,*str_routeinput;
	char *strAddnet ,*strAddnetmask;
	BGP_NET_T bgpentry,bgpcheckentry;
	BGP_PEER_T bgppeerentry,bgppeercheckentry;
	char tmpBuf[100];
	int entryNum,i;
	int ifindex;
	unsigned int * a1, *a2, *b1, *b2,route_id,as_number;
	//unsigned char ospfEnable;
	int bgpEnable,bgpdebug,routeinput,ospfinput;
	
	str_bgpfset = req_get_cstream_var(wp, "bgpset", "");
	str_restart = req_get_cstream_var(wp, "restart", "");
	str_bgpAddnet = req_get_cstream_var(wp, "add_network", "");
	str_bgpdelnet = req_get_cstream_var(wp, "deletenet", "");
	str_bgpdelallnet = req_get_cstream_var(wp, "deleteallnet", "");
	str_routeinput = req_get_cstream_var(wp, "input_static_route", "");
	str_addpeer = req_get_cstream_var(wp, "add_peer", "");
	str_delatepeer = req_get_cstream_var(wp, "deletepeer", "");
	str_deleteallpeer = req_get_cstream_var(wp, "deleteallpeer", "");



	
	if (str_bgpfset[0]) {
	
		strVal = req_get_cstream_var(wp, "bgp_on", "");
		
		if (!strcmp(strVal, "ON"))
				bgpEnable = 1;
			else
				bgpEnable = 0;
		if (!apmib_set(MIB_BGP_ENABLE, (void *)&bgpEnable)) {
				strcpy(tmpBuf, "Set BGP error!");
				goto setErr_route;
			}
		if(!bgpEnable){
				route_id=0;
				as_number=0;
				if ( !apmib_set(MIB_BGP_ROUTERID, (void *)&route_id)) {
					strcpy(tmpBuf, ("set route_id  error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_BGP_AS_NUMBER, (void *)&as_number)) {
					strcpy(tmpBuf, ("set As  error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_BGP_NETWORK_DELALL, (void *)&bgpentry)) {
					strcpy(tmpBuf, ("Delete all network table error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_BGP_PEER_DELALL, (void *)&bgppeerentry)) {
					strcpy(tmpBuf, ("Delete all peer table error!"));
					goto setErr_route;
				}
			}
		
		strVal = req_get_cstream_var(wp, "bgp_debug", "");


		if (!strcmp(strVal, "ON"))
				bgpdebug = 1;
			else
				bgpdebug = 0;
		if (!apmib_set(MIB_BGP_DEBUG, (void *)&bgpdebug)) {
				strcpy(tmpBuf, "Set BGP debug error!");
				goto setErr_route;
			}
		strVal = req_get_cstream_var(wp, "bgprouteid", "");
		
		if (strVal[0]) {
		route_id=atoi(strVal);

		if (!apmib_set(MIB_BGP_ROUTERID, (void *)&route_id)){
				strcpy(tmpBuf, "Set BGP routeid error!");
				goto setErr_route;
			}			
			}
	
		else{
			route_id=0;
			if (!apmib_set(MIB_BGP_ROUTERID, (void *)&route_id)){
				strcpy(tmpBuf, "Set BGP routeid error!");
				goto setErr_route;
			}
			
			}

		strVal = req_get_cstream_var(wp, "AS", "");
		
		if (strVal[0]) {
		as_number=atoi(strVal);

		if (!apmib_set(MIB_BGP_AS_NUMBER, (void *)&as_number)){
				strcpy(tmpBuf, "Set BGP AS error!");
				goto setErr_route;
			}			
			}
	
		else{
			as_number=0;
			if (!apmib_set(MIB_BGP_AS_NUMBER, (void *)&as_number)){
				strcpy(tmpBuf, "Set BGP AS error!");
				goto setErr_route;
			}
			
			}
		
	}

	if (str_restart[0]) {

			goto setOk_reatart;

		}



	if (str_bgpAddnet[0]) {
		strAddnet = req_get_cstream_var(wp, ("bgpnetwork"), "");
		strAddnetmask= req_get_cstream_var(wp, ("bgpsubnet"), "");
		if (!strAddnet[0] && !strAddnetmask[0])
			goto setOk_change;					
		inet_aton(strAddnet,(struct in_addr *)&bgpentry.bgpnet);
		inet_aton(strAddnetmask, (struct in_addr *)&bgpentry.bgpnetmask);		
		if ( !apmib_get(MIB_BGP_NETWORK_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get bgpentry number error!"));
			goto setErr_route;
		}

		if ( (entryNum + 1) > MAX_BGP_NETWORK_NUM) {
			strcpy(tmpBuf, ("Cannot add new bgp entry because table is full!"));
			goto setErr_route;
		}
		a1 = (unsigned int *) bgpentry.bgpnet ;
		a2 = (unsigned int *) bgpentry.bgpnetmask;
		for(i=1; i <= entryNum ; i++) {
			*((char *)&bgpcheckentry) = (char)i;
			if ( !apmib_get(MIB_BGP_NETWORK_TBL, (void *)&bgpcheckentry)){
				strcpy(tmpBuf, ("get bgp entry error!"));
				goto setErr_route;
			}
			b1 = (unsigned int *) bgpcheckentry.bgpnet ;
			b2 = (unsigned int *) bgpcheckentry.bgpnetmask;
			if((*a1 & *a2) == (*b1&*b2)){
				apmib_set(MIB_BGP_NETWORK_DEL, (void *)&bgpcheckentry);
				break;
			}
				
		
		}
		if ( apmib_set(MIB_BGP_NETWORK_ADD, (void *)&bgpentry) == 0) {
					strcpy(tmpBuf, ("Add bgp net table entry error!"));
					goto setErr_route;
		}
	}
	if (str_bgpdelnet[0]) {
		if ( !apmib_get(MIB_BGP_NETWORK_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get bgp entry number error!"));
					goto setErr_route;
				}
				for (i=entryNum; i>0; i--) {
				snprintf(tmpBuf, 20, "select%d", i);
				strVal = req_get_cstream_var(wp, tmpBuf, "");
				if ( !strcmp(strVal, "ON") ) {
					*((char *)&bgpentry) = (char)i;
					if ( !apmib_get(MIB_BGP_NETWORK_TBL, (void *)&bgpentry)) {
						strcpy(tmpBuf, ("Get bgp table entry error!"));
						goto setErr_route;
					}
					if ( !apmib_set(MIB_BGP_NETWORK_DEL, (void *)&bgpentry)) {
						strcpy(tmpBuf, ("Delete bgp table entry error!"));
					goto setErr_route;
					}
				}
				}

	}
	if (str_bgpdelallnet[0]) {
		if ( !apmib_get(MIB_BGP_NETWORK_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get bgp entry number error!"));
					goto setErr_route;
		}
		for(i=1; i <= entryNum ; i++) {
			*((char *)&bgpentry) = (char)i;
			if ( !apmib_get(MIB_BGP_NETWORK_TBL, (void *)&bgpentry)){
				strcpy(tmpBuf, ("get entry error!"));
				goto setErr_route;
			}	

		}
		if ( !apmib_set(MIB_BGP_NETWORK_DELALL, (void *)&bgpentry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_route;
		}

	}


	if (str_routeinput[0]) {
	
		strVal = req_get_cstream_var(wp, "routeinput", "");
		
		if (!strcmp(strVal, "ON"))
				routeinput = 1;
			else
				routeinput = 0;
		if (!apmib_set(MIB_BGP_ROUTE_INPUT, (void *)&routeinput)) {
				strcpy(tmpBuf, "Set route error!");
				goto setErr_route;
			}
		
		strVal = req_get_cstream_var(wp, "ospfimport", "");


		if (!strcmp(strVal, "ON"))
				ospfinput = 1;
			else
				ospfinput = 0;
		if (!apmib_set(MIB_BGP_OSPF_ROUTE_INPUT, (void *)&ospfinput)) {
				strcpy(tmpBuf, "Set ospf route input  error!");
				goto setErr_route;
			}
	
		
	}

	if (str_addpeer[0]) {
		strAddnet = req_get_cstream_var(wp, ("ip_address"), "");
		strVal= req_get_cstream_var(wp, ("peer_as"), "");
		if (!strAddnet[0])
			goto setOk_change;					
		inet_aton(strAddnet,(struct in_addr *)&bgppeerentry.bgppeerip);	

		as_number=atoi(strVal);
		bgppeerentry.asnumber=as_number;
	
		if ( !apmib_get(MIB_BGP_PEER_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get bgpentry number error!"));
			goto setErr_route;
		}

		if ( (entryNum + 1) > MAX_BGP_PEER_NUM) {
			strcpy(tmpBuf, ("Cannot add new bgp peer entry because table is full!"));
			goto setErr_route;
		}
		a1 = (unsigned int *) bgppeerentry.bgppeerip ;
		for(i=1; i <= entryNum ; i++) {
			*((char *)&bgpcheckentry) = (char)i;
			if ( !apmib_get(MIB_BGP_PEER_TBL, (void *)&bgppeercheckentry)){
				strcpy(tmpBuf, ("get bgp entry error!"));
				goto setErr_route;
			}
			b1 = (unsigned int *) bgppeercheckentry.bgppeerip ;
			if((*a1) == (*b1)){
				apmib_set(MIB_BGP_PEER_DEL, (void *)&bgppeercheckentry);
				break;
			}
				
		
		}
		if ( apmib_set(MIB_BGP_PEER_ADD, (void *)&bgppeerentry) == 0) {
					strcpy(tmpBuf, ("Add bgp net table entry error!"));
					goto setErr_route;
		}
	}
	if (str_delatepeer[0]) {
		if ( !apmib_get(MIB_BGP_PEER_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get bgp entry number error!"));
					goto setErr_route;
				}
				for (i=entryNum; i>0; i--) {
				snprintf(tmpBuf, 20, "select%d", i);
				strVal = req_get_cstream_var(wp, tmpBuf, "");
				if ( !strcmp(strVal, "ON") ) {
					*((char *)&bgppeerentry) = (char)i;
					if ( !apmib_get(MIB_BGP_PEER_TBL, (void *)&bgppeerentry)) {
						strcpy(tmpBuf, ("Get bgp table entry error!"));
						goto setErr_route;
					}
					if ( !apmib_set(MIB_BGP_PEER_DEL, (void *)&bgppeerentry)) {
						strcpy(tmpBuf, ("Delete bgp table entry error!"));
					goto setErr_route;
					}
				}
				}

	}
	if (str_deleteallpeer[0]) {
		if ( !apmib_get(MIB_BGP_NETWORK_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get bgp entry number error!"));
					goto setErr_route;
		}
		for(i=1; i <= entryNum ; i++) {
			*((char *)&bgppeerentry) = (char)i;
			if ( !apmib_get(MIB_BGP_PEER_TBL, (void *)&bgppeerentry)){
				strcpy(tmpBuf, ("get entry error!"));
				goto setErr_route;
			}	

		}
		if ( !apmib_set(MIB_BGP_PEER_DELALL, (void *)&bgppeerentry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_route;
		}

	}



setOk_change:
	submitUrl = req_get_cstream_var(wp, ("submit-url"), "");   // hidden page
	apmib_update_web(CURRENT_SETTING);
	send_redirect_perm(wp, submitUrl);
	return;
setOk_reatart:

submitUrl = req_get_cstream_var(wp, ("submit-url"), "");   // hidden page
#ifdef REBOOT_CHECK
	{
		char tmpMsg[300];
		char lan_ip_buf[30], lan_ip[30];
#ifdef RTK_REINIT_SUPPORT
		char cmdBuf[64]={0};
#endif		
		sprintf(tmpMsg, "%s","<script>dw(route_change)</script>");
		apmib_get( MIB_IP_ADDR,  (void *)lan_ip_buf) ;
		sprintf(lan_ip,"%s",inet_ntoa(*((struct in_addr *)lan_ip_buf)) );
#ifdef RTK_REINIT_SUPPORT
		sprintf(cmdBuf,"reinitCli -e %d",REINIT_EVENT_INIT);
		system(cmdBuf);
#endif
		OK_MSG_FW(tmpMsg, submitUrl,APPLY_COUNTDOWN_TIME,lan_ip);
#ifdef REBOOT_CHECK
		run_init_script_flag = 1;
#endif		
#ifndef NO_ACTION
		run_init_script("all");
#endif	
	}
#else
	OK_MSG(submitUrl);
#endif

	
#ifdef MULTI_WAN_SUPPORT
//system("reboot\n");
#endif	
	
	return;

setErr_route:
	ERR_MSG(tmpBuf);
	
}

void formquaggaroute(request * wp, char *path, char *query)
{
	char	*str, *submitUrl;
	char	*str_ospfset,*str_restart,*str_addinterface,*str_deleteospfinterface,*str_deleteallospfinterface,*str_addarea;
	char *str_delateospfarea,*str_deleteallospfarea,*str_addnet,*str_deletenet,*str_delateallnet;
	char *strVal,*strospfaddnet;
	char *strospfAddnet, *strospfdelnet, *strospfdelallnet,*strospfAddnetmask, *strospfarea;
	OSPF_NET_T ospfentry,ospfcheckentry;
	OSPF_INTERFACE_T ospfifentry,ospfifcheckentry;
	OSPF_AREA_T ospfareaentry,ospfareacheckentry;
	char tmpBuf[100];
	int entryNum,i;
	int ifindex;
	unsigned int * a1, *a2, *b1, *b2,route_id;
	//unsigned char ospfEnable;
	int ospfEnable,ospfdebug;
	
	str_ospfset = req_get_cstream_var(wp, "ospfset", "");
	str_restart = req_get_cstream_var(wp, "restart", "");
	str_addinterface = req_get_cstream_var(wp, "addospfinterface", "");
	str_deleteospfinterface = req_get_cstream_var(wp, "deleteospfinterface", "");
	str_deleteallospfinterface = req_get_cstream_var(wp, "deleteallospfinterface", "");
	str_addarea = req_get_cstream_var(wp, "addospfarea", "");
	str_delateospfarea = req_get_cstream_var(wp, "deleteospfarea", "");
	str_deleteallospfarea = req_get_cstream_var(wp, "deleteallospfarea", "");
	str_addnet = req_get_cstream_var(wp, "addospfnet", "");
	str_deletenet = req_get_cstream_var(wp, "deleteospfnetwork", "");
	str_delateallnet = req_get_cstream_var(wp, "deleteallospfnetwork", "");
	if (str_ospfset[0]) {
	
		strVal = req_get_cstream_var(wp, "ospf_on", "");
		
		if (!strcmp(strVal, "ON"))
				ospfEnable = 1;
			else
				ospfEnable = 0;

		if (!apmib_set(MIB_OSPF_ENABLE, (void *)&ospfEnable)) {
				strcpy(tmpBuf, "Set OSPF error!");
				goto setErr_route;
			}
		if(!ospfEnable){
				route_id=0;
				if ( !apmib_set(MIB_OSPF_ROUTERID, (void *)&route_id)) {
					strcpy(tmpBuf, ("set route_id  error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_OSPF_NETWORK_DELALL, (void *)&ospfentry)) {
					strcpy(tmpBuf, ("Delete all network table error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_OSPF_INTERFACE_DELALL, (void *)&ospfifentry)) {
					strcpy(tmpBuf, ("Delete all interface table error!"));
					goto setErr_route;
				}
				if ( !apmib_set(MIB_OSPF_AREA_DELALL, (void *)&ospfareaentry)) {
					strcpy(tmpBuf, ("Delete all area table error!"));
					goto setErr_route;
				}

			}
		
		strVal = req_get_cstream_var(wp, "ospf_debug", "");


		if (!strcmp(strVal, "ON"))
				ospfdebug = 1;
			else
				ospfdebug = 0;
		if (!apmib_set(MIB_OSPF_DEBUG, (void *)&ospfdebug)) {
				strcpy(tmpBuf, "Set OSPF debug error!");
				goto setErr_route;
			}
		strVal = req_get_cstream_var(wp, "ospfrouteid", "");
		
		if (strVal[0]) {
		route_id=atoi(strVal);

		if (!apmib_set(MIB_OSPF_ROUTERID, (void *)&route_id)){
				strcpy(tmpBuf, "Set OSPF routeid error!");
				goto setErr_route;
			}			
			}
	
		else{
			route_id=0;
			if (!apmib_set(MIB_OSPF_ROUTERID, (void *)&route_id)){
				strcpy(tmpBuf, "Set OSPF routeid error!");
				goto setErr_route;
			}
			
			}
		
	}
	if (str_restart[0]) {

			goto setOk_reatart;

		}
	if (str_addinterface[0]) {
		strVal = req_get_cstream_var(wp, "interface_authentication", "");
		ospfifentry.authenticationtype=atoi(strVal);
		strVal = req_get_cstream_var(wp, "interface_key", "");
		strcpy(ospfifentry.interfacekey, strVal);
		ifindex = atoi(req_get_cstream_var(wp, "ospf_if", ""));
		strcpy(ospfifentry.interfacename, interfacelist[ifindex ]);
		strVal = req_get_cstream_var(wp, "Cost", "");
		if(strVal!=""){
			ospfifentry.interfacecost=atoi(strVal);	
		}
		else
			ospfifentry.interfacecost=0;
		if ( !apmib_get(MIB_OSPF_INTERFACE_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get ospf interface entry number error!"));
					goto setErr_route;
				}

				if ( (entryNum + 1) > MAX_OSPF_INTERFACE_NUM) {
					strcpy(tmpBuf, ("Cannot add new ospf interface entry because table is full!"));
					goto setErr_route;
				}
				for(i=1; i <= entryNum ; i++) {
					*((char *)&ospfifcheckentry) = (char)i;
					if ( !apmib_get(MIB_OSPF_INTERFACE_TBL, (void *)&ospfifcheckentry)){
						strcpy(tmpBuf, ("get ospf interface entry error!"));
						goto setErr_route;
					}


					if(!strcmp(ospfifcheckentry.interfacename,ospfifentry.interfacename)){
						apmib_set(MIB_OSPF_INTERFACE_DEL, (void *)&ospfifcheckentry);
						break;
						}
					
				}
				if ( apmib_set(MIB_OSPF_INTERFACE_ADD, (void *)&ospfifentry) == 0) {
					strcpy(tmpBuf, ("Add ospf interface table entry error!"));
					goto setErr_route;
				}

	}
	if (str_deleteospfinterface[0]) {
		if ( !apmib_get(MIB_OSPF_INTERFACE_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get ospf interface entry number error!"));
					goto setErr_route;
		}
		for (i=entryNum; i>0; i--) {
			snprintf(tmpBuf, 20, "select%d", i);
			strVal = req_get_cstream_var(wp, tmpBuf, "");
			if ( !strcmp(strVal, "ON") ) {
					*((char *)&ospfifentry) = (char)i;
					if ( !apmib_get(MIB_OSPF_INTERFACE_TBL, (void *)&ospfifentry)) {
						strcpy(tmpBuf, ("Get ospf interface table entry error!"));
						goto setErr_route;
					}
					if ( !apmib_set(MIB_OSPF_INTERFACE_DEL, (void *)&ospfifentry)) {
						strcpy(tmpBuf, ("Delete ospf interface table entry error!"));
					goto setErr_route;
					}
				}
			}


	}
	if (str_deleteallospfinterface[0]) {
		if ( !apmib_get(MIB_OSPF_INTERFACE_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get ospf interface entry number error!"));
					goto setErr_route;
		}
		for(i=1; i <= entryNum ; i++) {
			*((char *)&ospfifentry) = (char)i;
			if ( !apmib_get(MIB_OSPF_INTERFACE_TBL, (void *)&ospfifentry)){
				strcpy(tmpBuf, ("get ospf interface entry error!"));
				goto setErr_route;
			}	

		}
		if ( !apmib_set(MIB_OSPF_INTERFACE_DELALL, (void *)&ospfifentry)) {
			strcpy(tmpBuf, ("Delete all ospf interface table error!"));
			goto setErr_route;
		}

	}

	if (str_addarea[0]) {
		strVal = req_get_cstream_var(wp, "area_id", "");
		ospfareaentry.areaid=atoi(strVal);
		strVal = req_get_cstream_var(wp, "area_authentication", "");
		ospfareaentry.authenticationtype=atoi(strVal);
		if(ospfareaentry.authenticationtype!=0){
			if ( !apmib_get(MIB_OSPF_AREA_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get ospf area entry number error!"));
					goto setErr_route;
				}

				if ( (entryNum + 1) > MAX_OSPF_AREA_NUM) {
					strcpy(tmpBuf, ("Cannot add new ospf area entry because table is full!"));
					goto setErr_route;
				}
				for(i=1; i <= entryNum ; i++) {
					*((char *)&ospfareacheckentry) = (char)i;
					if ( !apmib_get(MIB_OSPF_AREA_TBL, (void *)&ospfareacheckentry)){
						strcpy(tmpBuf, ("get ospf area entry error!"));
						goto setErr_route;
					}


					if(ospfareaentry.areaid==ospfareacheckentry.areaid){
						apmib_set(MIB_OSPF_AREA_DEL, (void *)&ospfareacheckentry);
						break;
						}
					
				}
				if ( apmib_set(MIB_OSPF_AREA_ADD, (void *)&ospfareaentry) == 0) {
					strcpy(tmpBuf, ("Add ospf area table entry error!"));
					goto setErr_route;
				}
		}

	}
	if (str_delateospfarea[0]) {
		if ( !apmib_get(MIB_OSPF_AREA_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get ospf area entry number error!"));
					goto setErr_route;
		}
		for (i=entryNum; i>0; i--) {
			snprintf(tmpBuf, 20, "select%d", i);
			strVal = req_get_cstream_var(wp, tmpBuf, "");
			if ( !strcmp(strVal, "ON") ) {
					*((char *)&ospfareaentry) = (char)i;
					if ( !apmib_get(MIB_OSPF_AREA_TBL, (void *)&ospfareaentry)) {
						strcpy(tmpBuf, ("Get ospf area table entry error!"));
						goto setErr_route;
					}
					if ( !apmib_set(MIB_OSPF_AREA_DEL, (void *)&ospfareaentry)) {
						strcpy(tmpBuf, ("Delete ospf area table entry error!"));
					goto setErr_route;
					}
				}
			}
	}
	if (str_deleteallospfarea[0]) {
		if ( !apmib_get(MIB_OSPF_AREA_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get ospf area entry number error!"));
					goto setErr_route;
		}
		for(i=1; i <= entryNum ; i++) {
			*((char *)&ospfareaentry) = (char)i;
			if ( !apmib_get(MIB_OSPF_AREA_TBL, (void *)&ospfareaentry)){
				strcpy(tmpBuf, ("get ospf area entry error!"));
				goto setErr_route;
			}	

		}
		if ( !apmib_set(MIB_OSPF_AREA_DELALL, (void *)&ospfareaentry)) {
			strcpy(tmpBuf, ("Delete all ospf area table error!"));
			goto setErr_route;
		}

	}


	str_addnet = req_get_cstream_var(wp, "addospfnet", "");
	str_deletenet = req_get_cstream_var(wp, "deleteospfnetwork", "");
	str_delateallnet = req_get_cstream_var(wp, "deleteallospfnetwork", "");
	if (str_addnet[0]) {
		strospfAddnet = req_get_cstream_var(wp, ("ospfnetwork"), "");
		strospfAddnetmask= req_get_cstream_var(wp, ("ospfsubnet"), "");
		strospfarea= req_get_cstream_var(wp, ("ospfnetarea"), "");
		if (!strospfAddnet[0] && !strospfAddnetmask[0] && !strospfarea[0])
			goto setOk_change;					
		inet_aton(strospfAddnet,(struct in_addr *)&ospfentry.ospfnet);
		inet_aton(strospfAddnetmask, (struct in_addr *)&ospfentry.ospfnetmask);
		ospfentry.ospfarea=(unsigned char)atoi(strospfarea);
			
		if ( !apmib_get(MIB_OSPF_NET_TBL_NUM, (void *)&entryNum)) {
			strcpy(tmpBuf, ("Get ospfentry number error!"));
			goto setErr_route;
		}

		if ( (entryNum + 1) > MAX_OSPF_NETWORK_NUM) {
			strcpy(tmpBuf, ("Cannot add new ospf entry because table is full!"));
			goto setErr_route;
		}
		a1 = (unsigned int *) ospfentry.ospfnet ;
		a2 = (unsigned int *) ospfentry.ospfnetmask;
		for(i=1; i <= entryNum ; i++) {
			*((char *)&ospfcheckentry) = (char)i;
			if ( !apmib_get(MIB_OSPF_NETWORK_TBL, (void *)&ospfcheckentry)){
				strcpy(tmpBuf, ("get ospf entry error!"));
				goto setErr_route;
			}
			b1 = (unsigned int *) ospfcheckentry.ospfnet ;
			b2 = (unsigned int *) ospfcheckentry.ospfnetmask;
			if((*a1 & *a2) == (*b1&*b2)){
				apmib_set(MIB_OSPF_NETWORK_DEL, (void *)&ospfcheckentry);
				break;
			}
				
		
		}
		if ( apmib_set(MIB_OSPF_NETWORK_ADD, (void *)&ospfentry) == 0) {
					strcpy(tmpBuf, ("Add ospf net table entry error!"));
					goto setErr_route;
		}
	}
	if (str_deletenet[0]) {
		if ( !apmib_get(MIB_OSPF_NET_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get ospf entry number error!"));
					goto setErr_route;
				}
				for (i=entryNum; i>0; i--) {
				snprintf(tmpBuf, 20, "select%d", i);
				strVal = req_get_cstream_var(wp, tmpBuf, "");
				if ( !strcmp(strVal, "ON") ) {
					*((char *)&ospfentry) = (char)i;
					if ( !apmib_get(MIB_OSPF_NETWORK_TBL, (void *)&ospfentry)) {
						strcpy(tmpBuf, ("Get ospf table entry error!"));
						goto setErr_route;
					}
					if ( !apmib_set(MIB_OSPF_NETWORK_DEL, (void *)&ospfentry)) {
						strcpy(tmpBuf, ("Delete ospf table entry error!"));
					goto setErr_route;
					}
				}
				}

	}
	if (str_delateallnet[0]) {
		if ( !apmib_get(MIB_OSPF_NET_TBL_NUM, (void *)&entryNum)) {
					strcpy(tmpBuf, ("Get ospf entry number error!"));
					goto setErr_route;
		}
		for(i=1; i <= entryNum ; i++) {
			*((char *)&ospfentry) = (char)i;
			if ( !apmib_get(MIB_OSPF_NETWORK_TBL, (void *)&ospfentry)){
				strcpy(tmpBuf, ("get entry error!"));
				goto setErr_route;
			}	

		}
		if ( !apmib_set(MIB_OSPF_NETWORK_DELALL, (void *)&ospfentry)) {
			strcpy(tmpBuf, ("Delete all table error!"));
			goto setErr_route;
		}

	}

setOk_change:
	submitUrl = req_get_cstream_var(wp, ("submit-url"), "");   // hidden page
	apmib_update_web(CURRENT_SETTING);
	send_redirect_perm(wp, submitUrl);
	return;
setOk_reatart:
submitUrl = req_get_cstream_var(wp, ("submit-url"), "");   // hidden page
#ifdef REBOOT_CHECK
	{
		char tmpMsg[300];
		char lan_ip_buf[30], lan_ip[30];
#ifdef RTK_REINIT_SUPPORT
		char cmdBuf[64]={0};
#endif		
		sprintf(tmpMsg, "%s","<script>dw(route_change)</script>");
		apmib_get( MIB_IP_ADDR,  (void *)lan_ip_buf) ;
		sprintf(lan_ip,"%s",inet_ntoa(*((struct in_addr *)lan_ip_buf)) );
#ifdef RTK_REINIT_SUPPORT
		sprintf(cmdBuf,"reinitCli -e %d",REINIT_EVENT_INIT);
		system(cmdBuf);
#endif
		OK_MSG_FW(tmpMsg, submitUrl,APPLY_COUNTDOWN_TIME,lan_ip);
#ifdef REBOOT_CHECK
		run_init_script_flag = 1;
#endif		
#ifndef NO_ACTION
		run_init_script("all");
#endif	
	}
#else
	OK_MSG(submitUrl);
#endif

	
#ifdef MULTI_WAN_SUPPORT
//system("reboot\n");
#endif	

	
	return;

setErr_route:
	ERR_MSG(tmpBuf);
}
#endif


int staticRouteList(request *wp, int argc, char **argv)
{
	int	nBytesSent=0, entryNum, i;
	STATICROUTE_T entry;
	char	ip[30], netmask[30], gateway[30], *tmpStr;
	#if defined(MULTI_WAN_SUPPORT)
	unsigned char display_name[16] = {'\0'};
	#endif
	
	if ( !apmib_get(MIB_STATICROUTE_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}
	#if defined(CONFIG_APP_TR069)
	nBytesSent += req_format_write(wp, "<tr class=\"tbl_head\">"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(route_tbl_destip)</script></b></font></td>\n"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(route_tbl_mask)</script></b></font></td>\n"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(route_tbl_gateway)</script></b></font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(route_tbl_metric)</script></b></font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(route_tbl_inter)</script></b></font></td>\n"
		"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(route_tbl_status)</script></b></font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(route_tbl_select)</script></b></font></td></tr>\n");
	#else
	nBytesSent += req_format_write(wp, "<tr class=\"tbl_head\">"
      	"<td align=center width=\"23%%\" ><font size=\"2\"><b><script>dw(route_tbl_destip)</script></b></font></td>\n"
      	"<td align=center width=\"23%%\" ><font size=\"2\"><b><script>dw(route_tbl_mask)</script> </b></font></td>\n"
      	"<td align=center width=\"23%%\" ><font size=\"2\"><b><script>dw(route_tbl_gateway)</script> </b></font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(route_tbl_metric)</script> </b></font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(route_tbl_inter)</script> </b></font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(route_tbl_select)</script></b></font></td></tr>\n");
	#endif
	
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_STATICROUTE_TBL, (void *)&entry))
			return -1;

		tmpStr = inet_ntoa(*((struct in_addr *)entry.dstAddr));
		strcpy(ip, tmpStr);
		tmpStr = inet_ntoa(*((struct in_addr *)entry.netmask));
		strcpy(netmask, tmpStr);
		tmpStr = inet_ntoa(*((struct in_addr *)entry.gateway));
		strcpy(gateway, tmpStr);

		#if defined(MULTI_WAN_SUPPORT)
		memset(display_name, '\0', sizeof(display_name));
		/* currently entry.interface 
		  * wan interface: value 1/2/3/4.....
		  * lan interface: value 0....
		 */
		if (entry.interface){
			snprintf(display_name, sizeof(display_name), "WAN%u", entry.interface);
		}
		else
		{
			snprintf(display_name, sizeof(display_name), "LAN");
		}
		#endif
		
		#if defined(CONFIG_APP_TR069)
		nBytesSent += req_format_write(wp, "<tr class=\"tbl_body\">"
			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" ><font size=\"2\">%d</td>\n"
      			"<td align=center width=\"10%%\" ><font size=\"2\">%s</td>\n"
				"<td align=center width=\"10%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				ip, netmask, gateway,entry.metric,
				#if defined(MULTI_WAN_SUPPORT)
				display_name,
				#else
				(entry.interface?"WAN":"LAN"),
				#endif
				entry.Enable?"<script>dw(route_tbl_enable)</script>":"<script>dw(route_tbl_disable)</script>",i);
		#else
		nBytesSent += req_format_write(wp, "<tr class=\"tbl_body\">"
			"<td align=center width=\"23%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"23%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"23%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" ><font size=\"2\">%d</td>\n"
      			"<td align=center width=\"10%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				ip, netmask, gateway,entry.metric,
				#if defined(MULTI_WAN_SUPPORT)
				display_name,
				#else
				(entry.interface?"WAN":"LAN"), 
				#endif
				i);
		#endif
	}
	return nBytesSent;
}

#ifdef CONFIG_APP_MROUTED
int mroutedIfList(request *wp, int argc, char **argv)
{
	int	nBytesSent=0, entryNum, i;
	MROUTED_IF_T entry;
	char	ip[30], netmask[30], gateway[30], *tmpStr;
	if ( !apmib_get(MIB_MROUTED_IF_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}
	char *nothing="null";
	char *str="Default";
	nBytesSent += req_format_write(wp, "<tr>"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Interface</b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>metric </b></font></td>\n"
      		"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>threshold </b></font></td>\n"
      		"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>rate limit(kbps)</b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Boundary Ip</b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>mask len </b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td></tr>\n");
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_MROUTED_IF_TBL, (void *)&entry))
			return -1;
		nBytesSent += req_format_write(wp, "<tr>"
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n",
				entry.MroutedIfName);

		if(entry.MroutedIfMetric==0)
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n",
				str);
		else{
			nBytesSent += req_format_write(wp,
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n",
				entry.MroutedIfMetric);
		}
		if(entry.MroutedIfThreshold==0)
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n",
				str);
		else{
			nBytesSent += req_format_write(wp,
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n",
				entry.MroutedIfThreshold);
		}
		if(entry.MroutedIfRateLimit==0)
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n",
				str);
		else{
			nBytesSent += req_format_write(wp,
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%u</td>\n",
				entry.MroutedIfRateLimit);
		}

		
		if(entry.MroutedIfBoundryIpLen==0)
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				nothing,nothing,i);
		else{
			tmpStr = inet_ntoa(*((struct in_addr *)entry.MroutedIfBoundryIp));
			strcpy(ip, tmpStr);
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				ip,entry.MroutedIfBoundryIpLen,i);
		}
		
		}	
	return nBytesSent;
}
int mroutedAltnetList(request *wp, int argc, char **argv)
{
	int	nBytesSent=0, entryNum, i;
	MROUTED_ALTNET_T entry;
	char	ip[30], netmask[30], gateway[30], *tmpStr;
	if ( !apmib_get(MIB_MROUTED_IF_ALTNET_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}
	nBytesSent += req_format_write(wp, "<tr>"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Interface</b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Altnet Ip</b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>mask len </b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td></tr>\n");
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_MROUTED_IF_ALTNET_TBL, (void *)&entry))
			return -1;
		nBytesSent += req_format_write(wp, "<tr>"
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n",
				entry.MroutedIfAltnetName);
		
			tmpStr = inet_ntoa(*((struct in_addr *)entry.MroutedIfAltnetIp));
			strcpy(ip, tmpStr);
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				ip,entry.MroutedIfAltnetIpLen,i);
			
		}	
	return nBytesSent;

}
int mroutedTunnelList(request *wp, int argc, char **argv)
{
	int	nBytesSent=0, entryNum, i;
	MROUTED_TUNNEL_T entry;
	char	ip[30], netmask[30], gateway[30], *tmpStr;
	if ( !apmib_get(MIB_MROUTED_TUNNEL_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}
	char *nothing="null";
	char *str="Default";
	nBytesSent += req_format_write(wp, "<tr>"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Local Ip</b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Remote Ip </b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>metric </b></font></td>\n"
      		"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>threshold </b></font></td>\n"
      		"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>rate limit(kbps)</b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Boundary Ip</b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>mask len </b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td></tr>\n");
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_MROUTED_TUNNEL_TBL, (void *)&entry))
			return -1;
		tmpStr = inet_ntoa(*((struct in_addr *)entry.MroutedTunnelLocalIp));
		strcpy(ip, tmpStr);
		nBytesSent += req_format_write(wp,"<tr>"
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n",
				ip);
		tmpStr = inet_ntoa(*((struct in_addr *)entry.MroutedTunnelRemoteIp));
		strcpy(ip, tmpStr);
		nBytesSent += req_format_write(wp,
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n",
				ip);
		if(entry.MroutedTunnelMetric==0)
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n",
				str);
		else{
			nBytesSent += req_format_write(wp,
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n",
				entry.MroutedTunnelMetric);
		}
		if(entry.MroutedTunnelThreshold==0)
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n",
				str);
		else{
			nBytesSent += req_format_write(wp,
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n",
				entry.MroutedTunnelThreshold);
		}
		if(entry.MroutedTunnelRateLimit==0)
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n",
				str);
		else{
			nBytesSent += req_format_write(wp,
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%u</td>\n",
				entry.MroutedTunnelRateLimit);
		}

		
		if(entry.MroutedTunnelBoundryIpLen==0)
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				nothing,nothing,i);
		else{
			tmpStr = inet_ntoa(*((struct in_addr *)entry.MroutedTunnelBoundryIp));
			strcpy(ip, tmpStr);
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				ip,entry.MroutedTunnelBoundryIpLen,i);
		}	
	}
	return nBytesSent;
}


#endif
#ifdef CONFIG_APP_PIMD

int PimdRpList(request *wp, int argc, char **argv)
{
	int	nBytesSent=0, entryNum, i;
	PIMD_RP_GROUP_T entry;
	char	ip[30], netmask[30], gateway[30], *tmpStr;
	if ( !apmib_get(MIB_PIMD_RP_GROUP_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}
	nBytesSent += req_format_write(wp, "<tr>"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>group address</b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>mask len </b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td></tr>\n");
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_PIMD_RP_GROUP_TBL, (void *)&entry))
			return -1;

		tmpStr = inet_ntoa(*((struct in_addr *)entry.PimdRpGroup));
		strcpy(ip, tmpStr);
		nBytesSent += req_format_write(wp, "<tr>"
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				ip, entry.PimdRpGroupMaskLen,i);
	}
	return nBytesSent;
}

int PimdInterfaceList(request *wp, int argc, char **argv)
{
	int	nBytesSent=0, entryNum, i;
	PIMD_IF_T entry;
	char	ip[30], netmask[30], gateway[30], *tmpStr;
	if ( !apmib_get(MIB_PIMD_IF_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}
	char *nothing="null";
	nBytesSent += req_format_write(wp, "<tr>"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Interface</b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>dr-priority </b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>alternative network</b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>mask len </b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>filter network</b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>mask len </b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td></tr>\n");
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_PIMD_IF_TBL, (void *)&entry))
			return -1;
		nBytesSent += req_format_write(wp, "<tr>"
			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n",
				entry.PimdIfName,entry.PimdIfDrPriroity);
		if(entry.PimdIfAlNetworkMaskLen==0)
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n",
				nothing,nothing);
		else{
			tmpStr = inet_ntoa(*((struct in_addr *)entry.PimdIfAlNetworkAddr));
			strcpy(ip, tmpStr);
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n",
				ip,entry.PimdIfAlNetworkMaskLen);
		}
		if(entry.PimdIfFlNetworkMaskLen==0)
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				nothing,nothing,i);
		else{
			tmpStr = inet_ntoa(*((struct in_addr *)entry.PimdIfNetworkAddr));
			strcpy(ip, tmpStr);
			nBytesSent += req_format_write(wp,
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				ip,entry.PimdIfFlNetworkMaskLen,i);
		}	
	}
	return nBytesSent;
}

#endif
int read_pid(const char *filename)
{
	FILE *fp;
	int pid;

	if ((fp = fopen(filename, "r")) == NULL)
		return -1;
	if(fscanf(fp, "%d", &pid) != 1)
		pid = -1;
	fclose(fp);

	return pid;
}
#ifdef CONFIG_APP_RTL_QUAGGA
int ospfnetworkList(request *wp, int argc, char **argv)
{
	int	nBytesSent=0, entryNum, i;
	OSPF_NET_T entry;
	char	ip[30], netmask[30], gateway[30], *tmpStr;
	if ( !apmib_get(MIB_OSPF_NET_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}
	nBytesSent += req_format_write(wp, "<tr>"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>network</b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Netmask </b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>area </b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td></tr>\n");
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_OSPF_NETWORK_TBL, (void *)&entry))
			return -1;

		tmpStr = inet_ntoa(*((struct in_addr *)entry.ospfnet));
		strcpy(ip, tmpStr);
		tmpStr = inet_ntoa(*((struct in_addr *)entry.ospfnetmask));
		strcpy(netmask, tmpStr);

		nBytesSent += req_format_write(wp, "<tr>"
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				ip, netmask, entry.ospfarea, i);
	}
	return nBytesSent;
}
int ospfinterfaceList(request *wp, int argc, char **argv)
{
	int	nBytesSent=0, entryNum, i;
	OSPF_INTERFACE_T entry;
	char *tmpStr;
	char *str1="None";
	char *str2="Simple password";
	char *str3="MD5";
	char *cost_none="no set";
	char type[20];
	if ( !apmib_get(MIB_OSPF_INTERFACE_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get interface table entry error!\n");
		return -1;
	}
	nBytesSent += req_format_write(wp, "<tr>"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Interface</b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Authentication type </b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Cost </b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td></tr>\n");
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_OSPF_INTERFACE_TBL, (void *)&entry)){
			fprintf(stderr, "Get interface table entry error!\n");
			return -1;
		}

		if(entry.authenticationtype==0)
			strcpy(type, str1);
		if(entry.authenticationtype==1)
			strcpy(type, str2);
		if(entry.authenticationtype==2)
			strcpy(type, str3);	
		if(entry.interfacecost)
		{
		nBytesSent += req_format_write(wp, "<tr>"
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%d</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				entry.interfacename, type, entry.interfacecost, i);
		}
		else{
			nBytesSent += req_format_write(wp, "<tr>"
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				entry.interfacename, type,cost_none, i);

		}
			
	}
	return nBytesSent;
}

int bgppeerList(request *wp, int argc, char **argv)
{
int	nBytesSent=0, entryNum, i;
	BGP_PEER_T entry;
	char	ip[30], netmask[30], gateway[30], *tmpStr;
	if ( !apmib_get(MIB_BGP_PEER_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}
	nBytesSent += req_format_write(wp, "<tr>"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>peer ip</b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>peer as</b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td></tr>\n");
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_BGP_PEER_TBL, (void *)&entry))
			return -1;

		tmpStr = inet_ntoa(*((struct in_addr *)entry.bgppeerip));
		strcpy(ip, tmpStr);


		nBytesSent += req_format_write(wp, "<tr>"
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%u</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				ip, entry.asnumber,i);
	}
	return nBytesSent;

}
int isisinterfaceList(request *wp, int argc, char **argv)
{
int	nBytesSent=0, entryNum, i;
	ISIS_IF_T entry;
	char	 *tmpStr;
	char *str1="Level 1-2";
	char *str2="Level 1";
	char *str3="Level 2";
	char level[20];
	char Autype[20];
	char *str5="None";
	char *str6="Simple Word";
	int metric;
	if ( !apmib_get(MIB_ISIS_IF_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}
	nBytesSent += req_format_write(wp, "<tr>"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Interface</b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Level Type</b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Metric</b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Authenticate Type</b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td></tr>\n");
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;

		if ( !apmib_get(MIB_ISIS_IF_TBL, (void *)&entry))
			return -1;

		if(entry.interfacelevel==0)
		{
			strcpy(level,str1);
		}
		if(entry.interfacelevel==1)
		{
			strcpy(level,str2);
		}
		if(entry.interfacelevel==2)
		{
			strcpy(level,str3);
		}
		if(entry.metric==0)
			metric=10;
		else
			metric=entry.metric;
		if(entry.ifautype==0)
			strcpy(Autype,str5);
		else
			strcpy(Autype,str6);
		nBytesSent += req_format_write(wp, "<tr>"
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%u</td>\n"
      			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				entry.interfacename,level,metric,Autype,i);
	}
	return nBytesSent;

}


int netAuthenticationList(request *wp, int argc, char **argv)
{
	int nBytesSent=0, entryNum, i;
	ISIS_NET_T entry;
	char	ip[30], netmask[30], gateway[30], *tmpStr;
	char *str1="Level 1";
	char *str2="Level 2";
	if ( !apmib_get(MIB_ISIS_NET_AU_TYPE_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}
	nBytesSent += req_format_write(wp, "<tr>"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Authenticate Net Level</b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td></tr>\n");
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_ISIS_NET_AU_TYPE_TBL, (void *)&entry))
			return -1;
		if(entry.netautype==1)
		{

			nBytesSent += req_format_write(wp, "<tr>"
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				str1,i);
		}
		if(entry.netautype==2)
		{

			nBytesSent += req_format_write(wp, "<tr>"
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				str2,i);
		}
	}
	return nBytesSent;

}



int bgpnetList(request *wp, int argc, char **argv)
{
int	nBytesSent=0, entryNum, i;
	BGP_NET_T entry;
	char	ip[30], netmask[30], gateway[30], *tmpStr;
	if ( !apmib_get(MIB_BGP_NETWORK_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}
	nBytesSent += req_format_write(wp, "<tr>"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>network</b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Netmask </b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td></tr>\n");
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_BGP_NETWORK_TBL, (void *)&entry))
			return -1;

		tmpStr = inet_ntoa(*((struct in_addr *)entry.bgpnet));
		strcpy(ip, tmpStr);
		tmpStr = inet_ntoa(*((struct in_addr *)entry.bgpnetmask));
		strcpy(netmask, tmpStr);

		nBytesSent += req_format_write(wp, "<tr>"
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				ip, netmask,i);
	}
	return nBytesSent;

}

int ospfareaList(request *wp, int argc, char **argv)
{
	int	nBytesSent=0, entryNum, i;
	OSPF_AREA_T entry;
	char *str1="None";
	char *str2="Simple password";
	char *str3="MD5";
	char *tmpStr;
	char type[20];
	if ( !apmib_get(MIB_OSPF_AREA_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get  area table entry error!\n");
		return -1;
	}
	nBytesSent += req_format_write(wp, "<tr>"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Area ID</b></font></td>\n"
      	"<td align=center width=\"23%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Authentication type </b></font></td>\n"
      	"<td align=center width=\"10%%\" bgcolor=\"#808080\"><font size=\"2\"><b>Select</b></font></td></tr>\n");
	for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_OSPF_AREA_TBL, (void *)&entry)){
			fprintf(stderr, "Get  area table entry error!\n");
			return -1;
		}
		if(entry.authenticationtype==0)
			strcpy(type, str1);
		if(entry.authenticationtype==1)
			strcpy(type, str2);
		if(entry.authenticationtype==2)
			strcpy(type, str3);	
		nBytesSent += req_format_write(wp, "<tr>"
			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%u</td>\n"
      			"<td align=center width=\"23%%\" bgcolor=\"#C0C0C0\"><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"10%%\" bgcolor=\"#C0C0C0\"><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n",
				 entry.areaid,type, i);
	}
	return nBytesSent;
}


//extern int RunSystemCmd();
static const char ZEBRA_CONF[] = "/var/zebra.conf";
static const char OSPFD_CONF[] = "/var/ospfd.conf";
static const char BGPD_CONF[] = "/var/bgpd.conf";
static const char ISISD_CONF[] = "/var/isisd.conf";
const char ZEBRA_PID[] = "/var/run/zebra.pid";
const char OSPFD_PID[] = "/var/run/ospfd.pid";
const char BGPD_PID[] = "/var/run/bgpd.pid";
const char ISIS_PID[] = "/var/run/isisd.pid";

// OSPF server configuration
// return value:
// 0  : not enabled
// 1  : successful
// -1 : startup failed
int startquagga(void)
{
	FILE *fp;
	char *argv[6];
	int pid;
	int  ospfOn,ospfDebug,bgpon,bgpdebug,bgprouteinput,ospfrouteinput;
	int isison,isisdebug,isisprocessid,isisnetlevel;
	char isisnet[70];
	unsigned int routeid,bgprouteid,asnumber;
	char *router_id;
	unsigned int entryNum, i, j;
	char *netIp[20];
	char *str_level1="level-1";
	char *str_level2="level-2-only";
	char *str_level12="level-1-2";
	unsigned int uMask;
	unsigned char temp[4];
	unsigned int uIp;
	ISIS_NET_T isisnetentry;
	ISIS_IF_T isisifentry;
	BGP_NET_T bgpnetentry;
	BGP_PEER_T bgppeerentry;
	OSPF_NET_T ospfentry;
	OSPF_INTERFACE_T ospfifentry;
	OSPF_AREA_T ospfareaentry;
	apmib_get(MIB_OSPF_ENABLE, (void *)&ospfOn);
	apmib_get(MIB_OSPF_DEBUG, (void *)&ospfDebug);
	apmib_get(MIB_OSPF_ROUTERID, (void *)&routeid);

	apmib_get(MIB_BGP_ENABLE, (void *)&bgpon);
	apmib_get(MIB_BGP_DEBUG, (void *)&bgpdebug);
	apmib_get(MIB_BGP_ROUTERID, (void *)&bgprouteid);
	apmib_get(MIB_BGP_ROUTE_INPUT, (void *)&bgprouteinput);
	apmib_get(MIB_BGP_OSPF_ROUTE_INPUT, (void *)&ospfrouteinput);
	apmib_get(MIB_BGP_AS_NUMBER, (void *)&asnumber);
	apmib_get(MIB_ISIS_ENABLE, (void *)&isison);
	apmib_get(MIB_ISIS_DEBUG, (void *)&isisdebug);
	apmib_get(MIB_ISIS_PROCESS_ID, (void *)&isisprocessid);
	apmib_get(MIB_ISIS_NET_LEVEL, (void *)&isisnetlevel);
	apmib_get(MIB_ISIS_NET, (void *)isisnet);
	//kill old zebra
	pid = read_pid((char *)ZEBRA_PID);
	if (pid >= 1) {
		if (kill(pid, SIGTERM) != 0) {
			printf("Could not kill pid '%d'\n", pid);
		}
	}
	//kill old ospfd
	pid = read_pid((char *)OSPFD_PID);
	if (pid >= 1) {
		if (kill(pid, SIGTERM) != 0)
			printf("Could not kill pid '%d'\n", pid);
	}
	
	pid = read_pid((char *)BGPD_PID);
	if (pid >= 1) {
		if (kill(pid, SIGTERM) != 0)
			printf("Could not kill pid '%d'\n", pid);
	}
	pid = read_pid((char *)ISIS_PID);
	if (pid >= 1) {
		if (kill(pid, SIGTERM) != 0)
			printf("Could not kill pid '%d'\n", pid);
	}
	

	system("rm /var/zebra.conf");
	system("rm /var/bgpd.conf");
	system("rm /var/ospfd.conf");
	system("rm /var/isisd.conf");
	system("rm /var/ospfd.log");
	system("rm /var/bgpd.log");
	system("rm /var/isisd.log");
	
	//create zebra.conf
	if ((fp = fopen(ZEBRA_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", ZEBRA_CONF);
		return -1;
	}
	fprintf(fp, "hostname Router\n");
	fprintf(fp, "password zebra\n");
	fprintf(fp, "enable password zebra\n");
	fclose(fp);
	sleep(1);
	system("zebra -d -f /var/zebra.conf");
	sleep(1);
	//create ospfd.conf
	if (bgpon)
	{
		if ((fp = fopen(BGPD_CONF, "w")) == NULL)
		{
			printf("Open file %s failed !\n", BGPD_CONF);
			return -1;
		}
		fprintf(fp, "hostname bgpd\n");
		fprintf(fp, "password zebra\n");
		if(bgpdebug==1)
		{
			fprintf(fp, "log file /var/bgpd.log\n");
			fprintf(fp, "log stdout\n");
			fprintf(fp, "!\n");
			fprintf(fp, "debug bgp events\n");
			fprintf(fp, "debug bgp as4\n");
			fprintf(fp, "debug bgp filters\n");
			fprintf(fp, "debug bgp fsm\n");
			fprintf(fp, "debug bgp updates\n");
			fprintf(fp, "debug bgp keepalives\n");
			fprintf(fp, "!\n");
		}
		if(asnumber!=0)
		{
			fprintf(fp, "router bgp %u\n",asnumber);
			if(bgprouteid!=0)
			{	temp[0]=(bgprouteid>>24)&0xff;
				temp[1]=(bgprouteid>>16)&0xff;
				temp[2]=(bgprouteid>>8)&0xff;
				temp[3]=(bgprouteid)&0xff;
				bgprouteid=temp[0]|(temp[1]<<8)|(temp[2]<<16)|(temp[3]<<24);
				router_id=inet_ntoa(*((struct in_addr *)&bgprouteid)); 
				fprintf(fp, " bgp router-id %s\n",router_id);
			}
		apmib_get(MIB_BGP_NETWORK_TBL_NUM, (void *)&entryNum);
		for(i=1; i <= entryNum ; i++) {
			*((char *)&bgpnetentry) = (char)i;
			if ( !apmib_get(MIB_BGP_NETWORK_TBL, (void *)&bgpnetentry)){
					printf("bgp entry read error");
					return 1;
					}	
		uIp = *(unsigned int *)bgpnetentry.bgpnet;
		uMask = *(unsigned int *)bgpnetentry.bgpnetmask;
		uIp = uIp & uMask;
		strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));

		for (j=0; j<32; j++)
		{
		#if defined(_LITTLE_ENDIAN_)	
			if (!((uMask>>j) & 0x01))
		#elif defined(_BIG_ENDIAN_)
			if ((uMask>>j) & 0x01)
		#endif		
				break;
		}
		#if defined(_LITTLE_ENDIAN_)	
			uMask = j;
		#elif defined(_BIG_ENDIAN_)
			uMask = 32 - j;
		#endif

		sprintf(netIp, "%s/%d", netIp, uMask); 
		fprintf(fp, " network %s\n", netIp);

		}
		if(bgprouteinput)
		{
			if(ospfrouteinput)
			{
				fprintf(fp, " redistribute ospf\n");
			}

		
		}
		apmib_get(MIB_BGP_PEER_TBL_NUM, (void *)&entryNum);
		for(i=1; i <= entryNum ; i++) {
			*((char *)&bgppeerentry) = (char)i;
			if ( !apmib_get(MIB_BGP_PEER_TBL, (void *)&bgppeerentry)){
					printf("bgp entry read error");
					return 1;
					}	
		uIp = *(unsigned int *)bgppeerentry.bgppeerip;
		strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));
		fprintf(fp, " neighbor %s remote-as %u\n", netIp,bgppeerentry.asnumber);

		}
		fprintf(fp, "!\n");
		}
		fclose(fp);
		system("bgpd -d -f /var/bgpd.conf");
	}

	if(isison)
	{
		if ((fp = fopen(ISISD_CONF, "w")) == NULL)
		{
			printf("Open file %s failed !\n", ISISD_CONF);
			return -1;
		}
		fprintf(fp, "hostname isisd\n");
		fprintf(fp, "password zebra\n");
		if(isisdebug==1)
		{
			fprintf(fp, "log file /var/isisd.log\n");
			fprintf(fp, "log stdout\n");
			fprintf(fp, "!\n");
			fprintf(fp, "debug isis adj-packets\n");
			fprintf(fp, "debug isis checksum-errors\n");
			fprintf(fp, "debug isis events\n");
			fprintf(fp, "debug isis local-updates\n");
			fprintf(fp, "debug isis protocol-errors\n");
			fprintf(fp, "debug isis route-events\n");
			fprintf(fp, "debug isis snp-packets\n");
			fprintf(fp, "debug isis spf-events\n");
			fprintf(fp, "debug isis spf-statistics\n");
			fprintf(fp, "debug isis spf-triggers\n");
			fprintf(fp, "debug isis update-packets\n");
			fprintf(fp, "!\n");
		}
		fprintf(fp, "router isis %u\n",isisprocessid);
		fprintf(fp, " net %s\n",isisnet);
		if(isisnetlevel==0)
			fprintf(fp, " is-type %s\n",str_level12);
		if(isisnetlevel==1)
			fprintf(fp, " is-type %s\n",str_level1);
		if(isisnetlevel==2)
			fprintf(fp, " is-type %s\n",str_level2);
		apmib_get(MIB_ISIS_NET_AU_TYPE_TBL_NUM, (void *)&entryNum);
		for(i=1; i <= entryNum ; i++) {
			*((char *)&isisnetentry) = (char)i;
			if ( !apmib_get(MIB_ISIS_NET_AU_TYPE_TBL, (void *)&isisnetentry)){
					printf("isis entry read error");
					return 1;
					}	
			if(isisnetentry.netautype==1)
				fprintf(fp, " area-password %s\n", isisnetentry.netkey);		
			if(isisnetentry.netautype==2)
				fprintf(fp, " domain-password %s\n", isisnetentry.netkey);	


		}
		fprintf(fp, "!\n");
		apmib_get(MIB_ISIS_IF_TBL_NUM, (void *)&entryNum);
		for(i=1; i <= entryNum ; i++) {
			*((char *)&isisifentry) = (char)i;
			if ( !apmib_get(MIB_ISIS_IF_TBL, (void *)&isisifentry)){
					printf("isis entry read error");
					return 1;
					}
			fprintf(fp, "interface %s\n", isisifentry.interfacename);	
			fprintf(fp, " ip router isis %u\n", isisprocessid);
			if(isisifentry.interfacelevel==0)
				fprintf(fp, " isis circuit-type %s\n",str_level12);				
			if(isisifentry.interfacelevel==1)
				fprintf(fp, " isis circuit-type %s\n",str_level1);	
			if(isisifentry.interfacelevel==2)
				fprintf(fp, " isis circuit-type %s\n",str_level2);	
			if(isisifentry.metric!=0)
				fprintf(fp, " isis metric %u\n",isisifentry.metric);
			if(isisifentry.ifautype!=0)
				fprintf(fp, " isis password %s\n",isisifentry.ifkey);
			fprintf(fp, "!\n");
	


		}

		
		
		fclose(fp);
		system("isisd -d -f /var/isisd.conf");



	
	}



	if (ospfOn)
	{
	if ((fp = fopen(OSPFD_CONF, "w")) == NULL)
	{
		printf("Open file %s failed !\n", OSPFD_CONF);
		return -1;
	}


	if ( !apmib_get(MIB_OSPF_NET_TBL_NUM, (void *)&entryNum)) {
					printf("ospf entry read error");
					return 1;
				}

	
	fprintf(fp, "hostname ospfd\n");
	fprintf(fp, "password zebra\n");
	if(ospfDebug==1)
	{
		fprintf(fp, "log file /var/ospfd.log\n");
		fprintf(fp, "log stdout\n");
		fprintf(fp, "!\n");
		fprintf(fp, "debug ospf ism\n");
		fprintf(fp, "debug ospf nsm\n");
		fprintf(fp, "debug ospf lsa\n");
		fprintf(fp, "debug ospf nssa\n");
		fprintf(fp, "debug ospf event\n");
		fprintf(fp, "debug ospf packet all\n");
		fprintf(fp, "!\n");
	}
	if(routeid!=0)
	{
		fprintf(fp, "router ospf\n");
		temp[0]=(routeid>>24)&0xff;
		temp[1]=(routeid>>16)&0xff;
		temp[2]=(routeid>>8)&0xff;
		temp[3]=(routeid)&0xff;
		routeid=temp[0]|(temp[1]<<8)|(temp[2]<<16)|(temp[3]<<24);
		router_id=inet_ntoa(*((struct in_addr *)&routeid)); 
		fprintf(fp, " ospf router-id %s\n",router_id);
		fprintf(fp, "!\n");
	}
	apmib_get(MIB_OSPF_INTERFACE_TBL_NUM, (void *)&entryNum);
		for(i=1; i <= entryNum ; i++) {
			*((char *)&ospfifentry) = (char)i;
			if ( !apmib_get(MIB_OSPF_INTERFACE_TBL, (void *)&ospfifentry)){
					printf("ospf entry read error");
					return 1;
					}	
			if(ospfifentry.authenticationtype==0)
			{
				fprintf(fp, "interface %s\n",ospfifentry.interfacename);
				fprintf(fp, " ip ospf cost %u\n",ospfifentry.interfacecost);
				fprintf(fp, "!\n");


			}
			if(ospfifentry.authenticationtype==1)
			{
			
				fprintf(fp, "interface %s\n",ospfifentry.interfacename);
				fprintf(fp, " ip ospf authentication-key %s\n",ospfifentry.interfacekey);
				fprintf(fp, " ip ospf authentication\n");
				if(ospfifentry.interfacecost>0)
				{
					fprintf(fp, " ip ospf cost %u\n",ospfifentry.interfacecost);
				}
				fprintf(fp, "!\n");


			
			}
			if(ospfifentry.authenticationtype==2)
			{
				fprintf(fp, "interface %s\n",ospfifentry.interfacename);
				
				fprintf(fp, " ip ospf authentication message-digest\n");
				fprintf(fp, " ip ospf message-digest-key 1 md5 %s\n",ospfifentry.interfacekey);
				if(ospfifentry.interfacecost>0)
				{
					fprintf(fp, " ip ospf cost %u\n",ospfifentry.interfacecost);
				}
				fprintf(fp, "!\n");
			}

		}
	fprintf(fp, "router ospf\n");
	apmib_get(MIB_OSPF_NET_TBL_NUM, (void *)&entryNum);
	for(i=1; i <= entryNum ; i++) {
			*((char *)&ospfentry) = (char)i;
			if ( !apmib_get(MIB_OSPF_NETWORK_TBL, (void *)&ospfentry)){
					printf("ospf entry read error");
					return 1;
					}	
		uIp = *(unsigned int *)ospfentry.ospfnet;
		uMask = *(unsigned int *)ospfentry.ospfnetmask;
		uIp = uIp & uMask;
		strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));

		for (j=0; j<32; j++)
		#if defined(_LITTLE_ENDIAN_)	
			if (!((uMask>>j) & 0x01))
		#elif defined(_BIG_ENDIAN_)
			if ((uMask>>j) & 0x01)
		#endif	
				break;
		#if defined(_LITTLE_ENDIAN_)	
			uMask = j;
		#elif defined(_BIG_ENDIAN_)
			uMask = 32 - j;
		#endif
	

		sprintf(netIp, "%s/%d", netIp, uMask); 
		fprintf(fp, " network %s area %d\n", netIp,ospfentry.ospfarea);

				}

	fprintf(fp, "!\n");

	apmib_get(MIB_OSPF_AREA_TBL_NUM, (void *)&entryNum);
	for(i=1; i <= entryNum ; i++) {
			*((char *)&ospfareaentry) = (char)i;
			if ( !apmib_get(MIB_OSPF_AREA_TBL, (void *)&ospfareaentry)){
					printf("ospf entry read error");
					return 1;
					}	
			if(ospfareaentry.authenticationtype==1)
			{
				fprintf(fp, "router ospf\n");
			
				fprintf(fp, " area %u authentication\n",ospfareaentry.areaid);
				fprintf(fp, "!\n");


			
			}
			if(ospfareaentry.authenticationtype==2)
			{
				fprintf(fp, "router ospf\n");
				fprintf(fp, " area %u authentication message-digest\n",ospfareaentry.areaid);
				fprintf(fp, "!\n");
			}




	}

	
	fclose(fp);
	system("ospfd -d -f /var/ospfd.conf");
	}

	
		
	return 0;
}

#endif

#ifdef CONFIG_APP_PIMD

//extern int RunSystemCmd();
static const char PIMD_CONF[] = "/var/pimd.conf";
const char PIMD_PID[] = "/var/run/V1.pid";

// OSPF server configuration
// return value:
// 0  : not enabled
// 1  : successful
// -1 : startup failed
int startpimd(void)
{
	FILE *fp;
	char *argv[6];
	int pid;
	int  pimdOn;
	char isisnet[70];
	unsigned int BsrPriority,RpPriority,RpTime,MaskLen;
	char *router_id;
	unsigned int entryNum, i, j;
	char netIp[10];
	char netIp1[10];
	char RpIp[4],RpGroup[4];
	unsigned int uMask;
	unsigned int uIp,uGroup;
	PIMD_RP_GROUP_T Rpentry;
	PIMD_IF_T Ifentry;
	apmib_get(MIB_PIMD_ENABLE, (void *)&pimdOn);
	apmib_get(MIB_PIMD_BSR_PRIORITY, (void *)&BsrPriority);
	apmib_get(MIB_PIMD_RP_PRIORITY, (void *)&RpPriority);
	apmib_get(MIB_PIMD_RP_TIME, (void *)&RpTime);
	apmib_get(MIB_PIMD_STATIC_RP_IP_ADDRESS, (void *)RpIp);
	apmib_get(MIB_PIMD_STATIC_RP_GROUP_ADDRESS, (void *)RpGroup);
	apmib_get(MIB_PIMD_STATIC_RP_GROUP_MASK_LEN, (void *)&MaskLen);
	//kill old pimd
	pid = read_pid((char *)PIMD_PID);
	if (pid >= 1) {
		if (kill(pid, SIGTERM) != 0) {
			printf("Could not kill pid '%d'\n", pid);
		}
	}

	system("rm /var/pimd.conf");
	if (pimdOn)
	{
		if ((fp = fopen(PIMD_CONF, "w")) == NULL)
		{
			printf("Open file %s failed !\n", PIMD_CONF);
			return -1;
		}
		fprintf(fp, "bsr-candidate priority %u\n",BsrPriority);
		fprintf(fp, "rp-candidate time %u priority %u\n",RpTime,RpPriority);
		apmib_get(MIB_PIMD_RP_GROUP_TBL_NUM, (void *)&entryNum);
		for(i=1; i <= entryNum ; i++) {
			*((char *)&Rpentry) = (char)i;
			if ( !apmib_get(MIB_PIMD_RP_GROUP_TBL, (void *)&Rpentry)){
					printf("pimd entry read error");
					return 1;
					}	
			uIp = *(unsigned int *)Rpentry.PimdRpGroup;
			strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));
			fprintf(fp, " group-prefix %s/%u\n", netIp,Rpentry.PimdRpGroupMaskLen);

		}
		if((int)RpIp[0]!=0)
		{
			uIp = *(unsigned int *)RpIp;
			strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));
			fprintf(fp, "rp-address %s",netIp);
			if(MaskLen!=0)
			{
				uGroup=*(unsigned int *)RpGroup;
				strcpy(netIp1, inet_ntoa(*((struct in_addr *)&uGroup)));
				fprintf(fp, " %s/%u",netIp1,MaskLen);
			}
			fprintf(fp, "\n");
		}
		
		apmib_get(MIB_PIMD_IF_TBL_NUM, (void *)&entryNum);
		for(i=1; i <= entryNum ; i++) {
			*((char *)&Ifentry) = (char)i;
			if ( !apmib_get(MIB_PIMD_IF_TBL, (void *)&Ifentry)){
					printf("pimd entry read error");
					return 1;
					}
			fprintf(fp, "phyint %s",Ifentry.PimdIfName);
			fprintf(fp, " dr-priority %u",Ifentry.PimdIfDrPriroity);
			if(!((Ifentry.PimdIfAlNetworkMaskLen==0)||(Ifentry.PimdIfAlNetworkMaskLen>32)))
			{
				uIp = *(unsigned int *)Ifentry.PimdIfAlNetworkAddr;
				strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));
				fprintf(fp, " altnet %s/%u",netIp,Ifentry.PimdIfAlNetworkMaskLen);
			
			}
			if(!((Ifentry.PimdIfAlNetworkMaskLen==0)||(Ifentry.PimdIfAlNetworkMaskLen>32)))
			{
				uIp = *(unsigned int *)Ifentry.PimdIfNetworkAddr;
				strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));
				fprintf(fp, " scoped %s/%u",netIp,Ifentry.PimdIfFlNetworkMaskLen);

			
			}
			fprintf(fp, "\n");

		}

		fclose(fp);
		system("pimd -c /var/pimd.conf");
	}
	
		
	return 0;
}

#endif
#ifdef CONFIG_APP_MROUTED
static const char MROUTED_CONF[] = "/var/mrouted.conf";
const char MROUTED_PID[] = "/var/run/mrouted.pid";
int startmrouted(void)
{
	FILE *fp;
	char *argv[6];
	int pid;
	int  MroutedOn;

	unsigned int BsrPriority,RpPriority,RpTime,MaskLen;
	char *router_id;
	unsigned int entryNum, entryNum1,i, j;
	char netIp[20];
	unsigned int uMask;
	unsigned int uIp,uGroup;
	MROUTED_IF_T Ifentry;
	MROUTED_TUNNEL_T Tunnelentry;
	MROUTED_ALTNET_T Altnetentry;
	apmib_get(MIB_MROUTED_ENABLE, (void *)&MroutedOn);
	//kill old pimd
	pid = read_pid((char *)MROUTED_PID);
	if (pid >= 1) {
		if (kill(pid, SIGTERM) != 0) {
			printf("Could not kill pid '%d'\n", pid);
		}
	}

	
	if (MroutedOn)
	{	
		system("rm /var/mrouted.conf");
		if ((fp = fopen(MROUTED_CONF, "w")) == NULL)
		{
			printf("Open file %s failed !\n", MROUTED_CONF);
			return -1;
		}
		apmib_get(MIB_MROUTED_IF_TBL_NUM, (void *)&entryNum);
		for(i=1; i <= entryNum ; i++) {
			*((char *)&Ifentry) = (char)i;
			if ( !apmib_get(MIB_MROUTED_IF_TBL, (void *)&Ifentry)){
					printf("mrouted entry read error");
					return 1;
					}	
			fprintf(fp, "phyint %s",Ifentry.MroutedIfName);
			if(Ifentry.MroutedIfMetric==0)
			{
			}else{

				fprintf(fp, " metric %u",Ifentry.MroutedIfMetric);
	
			}
			if(Ifentry.MroutedIfThreshold==0)
			{	
			}else{
				fprintf(fp, " threshold %u",Ifentry.MroutedIfThreshold);
			}
			if(Ifentry.MroutedIfRateLimit==0)
			{
			}else{
				fprintf(fp, " rate_limit %u",Ifentry.MroutedIfRateLimit);
			}

			if(Ifentry.MroutedIfBoundryIpLen==0)
			{
			}else{
				uIp = *(unsigned int *)Ifentry.MroutedIfBoundryIp;
				strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));
				fprintf(fp, " boundary %s/%u",netIp,Ifentry.MroutedIfBoundryIpLen);
		
			}
			apmib_get(MIB_MROUTED_IF_ALTNET_TBL_NUM, (void *)&entryNum1);
			for(j=1; j <= entryNum1 ; j++) {
				*((char *)&Altnetentry) = (char)j;
				if ( !apmib_get(MIB_MROUTED_IF_ALTNET_TBL, (void *)&Altnetentry)){
					printf("mrouted entry read error");
					return 1;
					}
				
				if(!strcmp(Ifentry.MroutedIfName,Altnetentry.MroutedIfAltnetName)){
				uIp = *(unsigned int *)Altnetentry.MroutedIfAltnetIp;
				strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));
				fprintf(fp, " altnet %s/%u",netIp,Altnetentry.MroutedIfAltnetIpLen);
				}
			}
			fprintf(fp,"\n");

		}
		
		apmib_get(MIB_MROUTED_TUNNEL_TBL_NUM, (void *)&entryNum);
		for(i=1; i <= entryNum ; i++) {
			*((char *)&Tunnelentry) = (char)i;
			if ( !apmib_get(MIB_MROUTED_TUNNEL_TBL, (void *)&Tunnelentry)){
					printf("mrouted entry read error");
					return 1;
					}
			uIp = *(unsigned int *)Tunnelentry.MroutedTunnelLocalIp;
			strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));
			fprintf(fp, "tunnel %s",netIp);
			uIp = *(unsigned int *)Tunnelentry.MroutedTunnelRemoteIp;
			strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));
			fprintf(fp, " %s",netIp);
			if(Tunnelentry.MroutedTunnelMetric==0)
			{
			}else{

				fprintf(fp, " metric %u",Tunnelentry.MroutedTunnelMetric);
	
			}
			if(Tunnelentry.MroutedTunnelThreshold==0)
			{	
			}else{
				fprintf(fp, " threshold %u",Tunnelentry.MroutedTunnelThreshold);
			}
			if(Tunnelentry.MroutedTunnelRateLimit==0)
			{
			}else{
				fprintf(fp, " rate_limit %u",Tunnelentry.MroutedTunnelRateLimit);
			}

			if(Tunnelentry.MroutedTunnelBoundryIpLen==0)
			{
			}else{
				uIp = *(unsigned int *)Tunnelentry.MroutedTunnelBoundryIp;
				strcpy(netIp, inet_ntoa(*((struct in_addr *)&uIp)));
				fprintf(fp, " boundary %s/%u",netIp,Tunnelentry.MroutedTunnelBoundryIpLen);
		
			}
						
			fprintf(fp,"\n");
		}
	
		fclose(fp);
		system("mrouted -c /var/mrouted.conf");
	}
	
		
	return 0;
}
#endif

int CheckRouteType(STATICROUTE_Tp checkentry)
{
	int	entryNum, i;
	STATICROUTE_T entry;
	DHCP_T dhcp;
	
	int isFound=1;
	
	if ( !apmib_get(MIB_STATICROUTE_TBL_NUM, (void *)&entryNum)) {
		return -1;
	}
	apmib_get( MIB_WAN_DHCP, (void *)&dhcp);
	for (i=1; i<=entryNum; i++) {
		memset(&entry, '\0', sizeof(entry));
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_STATICROUTE_TBL, (void *)&entry)){
			fprintf(stderr,"Get route entry fail\n");
			return -1;
		}
		if (dhcp == PPPOE || dhcp == PPTP || dhcp == L2TP) {
			if(entry.interface ==1){
				if(!memcmp(checkentry->dstAddr, &entry.dstAddr, 4) && !memcmp(checkentry->netmask, &entry.netmask, 4) && entry.interface == checkentry->interface ){
					isFound=0;		
					break;
				}
			}else{
				if(!memcmp(checkentry->dstAddr, &entry.dstAddr, 4) && !memcmp(checkentry->netmask, &entry.netmask, 4) && !memcmp(checkentry->gateway, &entry.gateway, 4) && entry.interface == checkentry->interface){
					isFound=0;		
					break;
				}
			}
		}else{
			if(!memcmp(checkentry->dstAddr, &entry.dstAddr, 4) && !memcmp(checkentry->netmask, &entry.netmask, 4) && !memcmp(checkentry->gateway, &entry.gateway, 4) && entry.interface == checkentry->interface){
				isFound=0;		
				break;
			}
		}
	}
	if(isFound==0)
		return 1;
	else
		return 0;
}
int kernelRouteList(request *wp, int argc, char **argv)
{
	char buff[256];
	char iface[32];
	int nl = 0;
	struct in_addr dest;
	struct in_addr gw;
	struct in_addr mask;
	int flgs, ref, use, metric, mtu, win, ir;
	char flags[64];
	unsigned long int d, g, m;
	STATICROUTE_T entry;
	char sdest[16], sgw[16];
	int	nBytesSent=0;
	char *interface_WAN=NULL;
	char *interface_LAN=NULL;
	int opmode=0;
	int entry_type=0;
	int static_route_enabled=0;
	DHCP_T dhcp;
	char *interface_WANPhy="eth1"; 
	#if defined(MULTI_WAN_SUPPORT)
	char *display_name = NULL;
	#endif
	
	apmib_get( MIB_WAN_DHCP, (void *)&dhcp);
	apmib_get(MIB_OP_MODE, (void *)&opmode);
	apmib_get(MIB_STATICROUTE_ENABLED, (void *)&static_route_enabled);
	if ( dhcp == PPPOE || dhcp == PPTP || dhcp == L2TP) {
			interface_WAN = PPPOE_IF;
	}else
			interface_WAN = WAN_IF;
		
		
	interface_LAN = BRIDGE_IF;	
	
	if(opmode==2)
		interface_WAN = ("wlan0");


	FILE *fp = fopen("/proc/net/route", "r");
	nBytesSent += req_format_write(wp, "<tr class=\"tbl_head\">"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(routetbl_dst)</script></b></font></td>\n"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(routetbl_gw)</script></b></b></font></td>\n"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(routetbl_mask)</script></b></font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(routetbl_metric)</script></b></font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(routetbl_iface)</script></b></font></td>"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(routetbl_type)</script></b></font></td></tr>\n");
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		if (nl) {
			int ifl = 0;
			while (buff[ifl] != ' ' && buff[ifl] != '\t' && buff[ifl] != '\0')
				ifl++;
			memset(&entry, '\0', sizeof(entry));	
			strncpy(iface, buff, ifl);
			iface[ifl]='\0';
				
			if(!strcmp(iface, interface_WAN)){
					entry.interface=1;
			}
			if ( dhcp ==PPTP || dhcp == L2TP) {
					if(!strcmp(iface, interface_WANPhy)){
						entry.interface=1;
					}
			}
			if(!strcmp(iface, interface_LAN)){
				entry.interface=0;	
			}

			#if defined(MULTI_WAN_SUPPORT)
			display_name = find_name_mapping(iface);
			//printf("%s %d display_name=%s\n", __func__, __LINE__, display_name?display_name:"null");
			#endif
			
			buff[ifl] = 0;	/* interface */
			
			if (sscanf(buff + ifl + 1, "%lx%lx%X%d%d%d%lx%d%d%d",
					   &d, &g, &flgs, &ref, &use, &metric, &m, &mtu, &win,
					   &ir) != 10) {
				printf("Unsuported kernel route format\n");
			}
			ifl = 0;	/* parse flags */
			if (flgs & RTF_UP) {
				if (flgs & RTF_REJECT)
					flags[ifl++] = '!';
				else
					flags[ifl++] = 'U';
				if (flgs & RTF_GATEWAY)
					flags[ifl++] = 'G';
				if (flgs & RTF_HOST)
					flags[ifl++] = 'H';
				if (flgs & RTF_REINSTATE)
					flags[ifl++] = 'R';
				if (flgs & RTF_DYNAMIC)
					flags[ifl++] = 'D';
				if (flgs & RTF_MODIFIED)
					flags[ifl++] = 'M';
				flags[ifl] = 0;
				dest.s_addr = d;
				gw.s_addr = g;
				mask.s_addr = m;
				strcpy(sdest, inet_ntoa(dest));
				strcpy(sgw, inet_ntoa(gw));
				
				memcpy(&(entry.dstAddr), &(dest.s_addr), 4); 
				memcpy(&(entry.gateway), &(gw.s_addr), 4); 
				memcpy(&(entry.netmask), &(mask.s_addr), 4); 
				
				if(static_route_enabled==1)
					entry_type=CheckRouteType(&entry);
				else
					entry_type=0;

				nBytesSent += req_format_write(wp, "<tr class=\"tbl_body\">"
      			"<td align=center width=\"20%%\" ><font size=\"2\">%-16s</td>\n"
      			"<td align=center width=\"20%%\" ><font size=\"2\">%-16s</td>\n"
      			"<td align=center width=\"20%%\" ><font size=\"2\">%-16s</td>\n"
      			"<td align=center width=\"10%%\" ><font size=\"2\">%d</td>\n"
      			"<td align=center width=\"10%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td></tr>\n",
			sdest, sgw, inet_ntoa(mask), metric, 
			#if defined(MULTI_WAN_SUPPORT)
			(display_name!=NULL?display_name:"LAN"),
			#else
			(entry.interface?"WAN":"LAN"),
			#endif
			(entry_type?"Static":"Dynamic"));
			}
		}
		nl++;
	}
	if(fp)
		fclose(fp);
	return nBytesSent;
}

#ifdef RIP6_SUPPORT //Modified from net-tools-1.60 by lynn_pu

#define _PATH_PROCNET_ROUTE6 "/proc/net/ipv6_route"
/* Like strncpy but make sure the resulting string is always 0 terminated. */  
char *safe_strncpy(char *dst, const char *src, size_t size)
{   
    dst[size-1] = '\0';
    return strncpy(dst,src,size-1);   
}

static int INET6_getsock(char *bufp, struct sockaddr *sap)
{
    struct sockaddr_in6 *sin6;

    sin6 = (struct sockaddr_in6 *) sap;
    sin6->sin6_family = AF_INET6;
    sin6->sin6_port = 0;

    if (inet_pton(AF_INET6, bufp, sin6->sin6_addr.s6_addr) <= 0)
	return (-1);

    return 16;			/* ?;) */
}

static int INET6_input(int type, char *bufp, struct sockaddr *sap)
{
    switch (type) {
    	case 1:
			return (INET6_getsock(bufp, sap));
    	default:
			return -1;
    }
}

static int INET6_rresolve(char *name, struct sockaddr_in6 *sin6, int numeric)
{
    int s;

    /* Grmpf. -FvK */
    if (sin6->sin6_family != AF_INET6) {
#ifdef DEBUG
	fprintf(stderr, "rresolve: unsupport address family %d !\n",
		sin6->sin6_family);
#endif
	errno = EAFNOSUPPORT;
	return (-1);
    }
    if (numeric & 0x7FFF) {
	inet_ntop(AF_INET6, &sin6->sin6_addr, name, 80);
	return (0);
    }
    if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
        if (numeric & 0x8000)
	    strcpy(name, "default");
	else
	    strcpy(name, "*");
	return (0);
    }

    if ((s = getnameinfo((struct sockaddr *) sin6, sizeof(struct sockaddr_in6),
			 name, 255 /* !! */ , NULL, 0, 0))) {
	fputs("getnameinfo failed\n", stderr);
	return -1;
    }
    return (0);
}


/* Display an Internet socket address. */
/* dirty! struct sockaddr usually doesn't suffer for inet6 addresses, fst. */
static char *INET6_sprint(struct sockaddr *sap, int numeric)
{
    static char buff[128];

    if (sap->sa_family == 0xFFFF || sap->sa_family == 0)
		return safe_strncpy(buff, "[NONE SET]", sizeof(buff));
    if (INET6_rresolve(buff, (struct sockaddr_in6 *) sap, numeric) != 0)
		return safe_strncpy(buff, "[UNKNOWN]", sizeof(buff));
    return (buff);
}

int kernelRoute6List(request *wp, int argc, char **argv)
{
	char buff[4096], iface[16], flags[16];
	char addr6[128], naddr6[128];
	struct sockaddr_in6 saddr6, snaddr6;
	int num, iflags, metric, refcnt, use, prefix_len, slen;
	FILE *fp = fopen(_PATH_PROCNET_ROUTE6, "r");
	char addr6p[8][5], saddr6p[8][5], naddr6p[8][5];
	int	nBytesSent=0;

	if (!fp) {
		perror(_PATH_PROCNET_ROUTE6);
		printf("INET6 (IPv6) not configured in this system.\n");
		return 1;
	}
	//printf("Kernel IPv6 routing table\n");
	
	//printf("Destination 								"
	//		 "Next Hop								  "
	//		 "Flags Metric Ref	  Use Iface\n");

	nBytesSent += req_format_write(wp, "<tr class=\"tbl_head\">"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(routetbl_dst)</script></b></font></td>\n"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b><script>dw(routetbl_nhop)</script></b></font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(routetbl_flag)</script></b></font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(routetbl_metric)</script></b></font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(routetbl_ref)</script></b></font></td>"
      	"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(routetbl_use)</script></b></font></td>"
      	"<td align=center width=\"10%%\" ><font size=\"2\"><b><script>dw(routetbl_if)</script></b></font></td></tr>\n");
	
	while (fgets(buff, 1023, fp)) {
		num = sscanf(buff, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %4s%4s%4s%4s%4s%4s%4s%4s %02x %4s%4s%4s%4s%4s%4s%4s%4s %08x %08x %08x %08x %s\n",
		addr6p[0], addr6p[1], addr6p[2], addr6p[3],
		addr6p[4], addr6p[5], addr6p[6], addr6p[7],
		&prefix_len,
		saddr6p[0], saddr6p[1], saddr6p[2], saddr6p[3],
		saddr6p[4], saddr6p[5], saddr6p[6], saddr6p[7],
		&slen,
		naddr6p[0], naddr6p[1], naddr6p[2], naddr6p[3],
		naddr6p[4], naddr6p[5], naddr6p[6], naddr6p[7],
		&metric, &use, &refcnt, &iflags, iface);

		if(!strcmp(iface, "lo"))
			continue;

#if 0
		if (num < 23)
			continue;
#endif
		if (!(iflags & RTF_UP))
			continue;
		/* Fetch and resolve the target address. */
		snprintf(addr6, sizeof(addr6), "%s:%s:%s:%s:%s:%s:%s:%s",
			 addr6p[0], addr6p[1], addr6p[2], addr6p[3],
			 addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
		INET6_input(1, addr6, (struct sockaddr *) &saddr6);
		snprintf(addr6, sizeof(addr6), "%s/%d",
			 INET6_sprint((struct sockaddr *) &saddr6, 1),
			 prefix_len);
	
		/* Fetch and resolve the nexthop address. */
		snprintf(naddr6, sizeof(naddr6), "%s:%s:%s:%s:%s:%s:%s:%s",
			 naddr6p[0], naddr6p[1], naddr6p[2], naddr6p[3],
			 naddr6p[4], naddr6p[5], naddr6p[6], naddr6p[7]);
		INET6_input(1, naddr6, (struct sockaddr *) &snaddr6);
		snprintf(naddr6, sizeof(naddr6), "%s",
			 INET6_sprint((struct sockaddr *) &snaddr6, 1));
	
		/* Decode the flags. */
		strcpy(flags, "U");
		if (iflags & RTF_GATEWAY)
			strcat(flags, "G");
		if (iflags & RTF_HOST)
			strcat(flags, "H");
		if (iflags & RTF_DEFAULT)
			strcat(flags, "D");
		if (iflags & RTF_ADDRCONF)
			strcat(flags, "A");
		if (iflags & RTF_CACHE)
			strcat(flags, "C");
	
		/* Print the info. */
		//printf("%-43s %-39s %-5s %-6d %-2d %7d %-8s\n",
		//	   addr6, naddr6, flags, metric, refcnt, use, iface);
		
		nBytesSent += req_format_write(wp, "<tr class=\"tbl_body\">"
      	"<td align=center width=\"20%%\" ><font size=\"2\">%-43s</font></td>\n"
      	"<td align=center width=\"20%%\" ><font size=\"2\">%-39s</font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\">%-5s</font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\">%-6d</font></td>\n"
      	"<td align=center width=\"10%%\" ><font size=\"2\">%-2d</font></td>"
      	"<td align=center width=\"10%%\" ><font size=\"2\">%7d</font></td>"
      	"<td align=center width=\"10%%\" ><font size=\"2\">%-8s</font></td></tr>\n",
			   addr6, naddr6, flags, metric, refcnt, use, (strcmp(iface,"eth1"))?"LAN":"WAN");
	
	}

	fclose(fp);

	return nBytesSent;
}
#endif
#endif

#endif //ROUTE_SUPPORT
