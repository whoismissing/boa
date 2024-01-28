#ifndef RTK_EVENT_H_
#define RTK_EVENT_H_

typedef struct BASE_DATA_
{
	int wlan_idx;
	int vwlan_idx;
	int wan_idx;
	int lan_idx;
	int isStartUp;//
//	void* parm;
} BASE_DATA_T, *BASE_DATA_Tp;

typedef enum
{
	REINIT_EVENT_BEGIN=0,
	REINIT_EVENT_INIT,				//init
	REINIT_EVENT_MIBCHANGE, 			//mib change
	REINIT_EVENT_APPLYCAHNGES, 			//apply change	
	REINIT_EVENT_PPP_MANUAL_CONNECT,
	REINIT_EVENT_WAN_CONNECT, 		//wan connect
	REINIT_EVENT_WAN_DISCONNECT,	//wan disconnect
	REINIT_EVENT_AP_CONNECT,		//AP connect
	REINIT_EVENT_LAN_DHCP_CONNECT, //lan dhcp connect	
#if 0  //no used
	REINIT_EVENT_WAN_POWER_ON = 3,		//wan power on 	
	REINIT_EVENT_FIREWALL	 = 6,		//firewall 		
	REINIT_EVENT_PPPOE_START = 7,		//pppoe start
	REINIT_EVENT_PPPOE_EXIT	 = 8,		//pppoe exit	
	REINIT_EVENT_PPTP_START = 11,		//pptp start
	REINIT_EVENT_PPTP_EXIT	 = 12,		//pptp exit	
	REINIT_EVENT_L2TP_START = 13,		//pppoe start
	REINIT_EVENT_L2TP_EXIT	 = 14,		//pppoe exit	
#endif
#ifdef CONFIG_IPV6
	REINIT_EVENT_IPV6CP_CONNECT, //ipv6cp connect
	REINIT_EVENT_IPV6_WAN_CONNECT = 16,	//ipv6 wan connect
	REINIT_EVENT_IPV6_WAN_DISCONNECT = 17, //ipv6 wan disconnect
	REINIT_EVENT_IPV6_FIREWALL = 18,	//ipv6 firewall
	REINIT_EVENT_IPV6_DHCP6C_PD = 19,   //ipv6 dhcp6c get pd
#ifdef CONFIG_DSLITE_SUPPORT
	REINIT_EVENT_IPV6_DSLITE_AUTO = 20, //ipv6 auto dslite tunnel
#endif
	REINIT_EVENT_IPV6CP_DISCONNECT,  //ipv6cp disconnect
	REINIT_EVENT_IPV6_POLICY_ROUTE,  //IPv6 policy route
#endif
	REINIT_EVENT_START_L2TP_CONNECT,
	REINIT_EVENT_END
} REINIT_EVENT_ID;
typedef struct EventHandleFuncItem_
{
	REINIT_EVENT_ID eventId;
	int (*event_handle)(char* data,int dataLen,char*errmsg);
	char * eventName;
} EventHandleFuncItem_t, *EventHandleFuncItem_tp;


#endif
