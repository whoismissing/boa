#ifndef _COMMON_UTILITY_H
#define _COMMON_UTILITY_H
#include "../apmib/apmib.h"
#ifdef CONFIG_FWD_UNIFIED_FRAMEWORK
#include "fwd.h"
#endif

#define TRUE 1
#define FALSE 0

#define NULL_FILE 0
#define NULL_STR ""
#define IFACE_NUM_MAX 32
#define IFACE_NAME_MAX 32
#define READ_BUF_SIZE	50

#define ADD_NEW_LINE_TO_HEAD 3
#define ADD_NEW_LINE_TO_END 2
#define ADD_NEW_LINE_TO_NEW_FILE 1

#define MULTI_WAN_IFNAME_FILE "/var/run/multi_wan_ifname"


#define DNS_BUF_SIZE 1024
#define DNS_SRV_PORT 53

#define SEND_DNS_REQ_NUM 3
#define MAX_DNS_NUM 3

#define ALIASNAME_WLAN0_VA  			"wlan0-va"
#define ALIASNAME_WLAN1_VA 			"wlan1-va"
#define ALIASNAME_WLAN2_VA 			"wlan2-va"


#define ELAN1_DEV_NAME 			"eth0"
#define ELAN2_DEV_NAME 			"eth2"
#define ELAN3_DEV_NAME 			"eth3" 
#define ELAN4_DEV_NAME 			"eth4"
#ifdef CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT
#define ELAN5_DEV_NAME 			"eth5"
#endif
#define WLAN0_DEV_NAME 			"wlan0"
#define WLAN1_DEV_NAME 			"wlan1"
#define WLAN2_DEV_NAME 			"wlan2"


#define ETH_LAN_MAX_NUM		8
//It means the real phy port on customer board(show on UI)
#define ETH_LAN_NUM WAN_INTERFACE_LAN_PORT_NUM
#define ETH_LAN1_BIND_MASK		(0x1ULL<<0)
#define ETH_LAN2_BIND_MASK		(0x1ULL<<1)
#define ETH_LAN3_BIND_MASK		(0x1ULL<<2)
#define ETH_LAN4_BIND_MASK		(0x1ULL<<3)
#ifdef CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT
#define ETH_LAN5_BIND_MASK		(0x1ULL<<4)
#endif


#define WLAN_MAX_NUM 		9
#define WLAN_NUM 	WAN_INTERFACE_EACH_WLAN_PORT_NUM
#define WLAN1_BIND_MASK			(0x1ULL<<ETH_LAN_NUM)
#define WLAN1_VA1_BIND_MASK		(WLAN1_BIND_MASK<<1)
#define WLAN1_VA2_BIND_MASK		(WLAN1_BIND_MASK<<2)
#define WLAN1_VA3_BIND_MASK		(WLAN1_BIND_MASK<<3)
#define WLAN1_VA4_BIND_MASK		(WLAN1_BIND_MASK<<4)
#define WLAN1_VA5_BIND_MASK		(WLAN1_BIND_MASK<<5)
#define WLAN1_VA6_BIND_MASK		(WLAN1_BIND_MASK<<6)
#define WLAN1_VA7_BIND_MASK		(WLAN1_BIND_MASK<<7)
#define WLAN1_VXD_BIND_MASK		(WLAN1_BIND_MASK<<(WLAN_NUM-1))

#define WLAN2_BIND_MASK			(WLAN1_BIND_MASK<<WLAN_NUM)
#define WLAN2_VA1_BIND_MASK		(WLAN2_BIND_MASK<<1)
#define WLAN2_VA2_BIND_MASK		(WLAN2_BIND_MASK<<2)
#define WLAN2_VA3_BIND_MASK		(WLAN2_BIND_MASK<<3)
#define WLAN2_VA4_BIND_MASK		(WLAN2_BIND_MASK<<4)
#define WLAN2_VA5_BIND_MASK		(WLAN2_BIND_MASK<<5)
#define WLAN2_VA6_BIND_MASK		(WLAN2_BIND_MASK<<6)
#define WLAN2_VA7_BIND_MASK		(WLAN2_BIND_MASK<<7)
#define WLAN2_VXD_BIND_MASK		(WLAN2_BIND_MASK<<(WLAN_NUM-1))


#define WLAN3_BIND_MASK			(WLAN2_BIND_MASK<<WLAN_NUM)
#define WLAN3_VA1_BIND_MASK		(WLAN3_BIND_MASK<<1)
#define WLAN3_VA2_BIND_MASK		(WLAN3_BIND_MASK<<2)
#define WLAN3_VA3_BIND_MASK		(WLAN3_BIND_MASK<<3)
#define WLAN3_VA4_BIND_MASK		(WLAN3_BIND_MASK<<4)
#define WLAN3_VA5_BIND_MASK		(WLAN3_BIND_MASK<<5)
#define WLAN3_VA6_BIND_MASK		(WLAN3_BIND_MASK<<6)
#define WLAN3_VA7_BIND_MASK		(WLAN3_BIND_MASK<<7)
#define WLAN3_VXD_BIND_MASK		(WLAN3_BIND_MASK<<(WLAN_NUM-1))


#define DEV_NAME_NUM(name,num)	name#num //fix compile error when tro09 enabled

#define WLAN0_VA0_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN0_VA, 0)
#define WLAN0_VA1_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN0_VA, 1)
#define WLAN0_VA2_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN0_VA, 2)
#define WLAN0_VA3_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN0_VA, 3)
#define WLAN0_VA4_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN0_VA, 4)
#define WLAN0_VA5_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN0_VA, 5)
#define WLAN0_VA6_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN0_VA, 6)
#define WLAN0_VXD_DEV_NAME 	"wlan0-vxd"
#define WLAN1_VA0_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN1_VA, 0)
#define WLAN1_VA1_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN1_VA, 1)
#define WLAN1_VA2_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN1_VA, 2)
#define WLAN1_VA3_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN1_VA, 3)
#define WLAN1_VA4_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN1_VA, 4)
#define WLAN1_VA5_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN1_VA, 5)
#define WLAN1_VA6_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN1_VA, 6)
#define WLAN1_VXD_DEV_NAME 	"wlan1-vxd"
#define WLAN2_VA0_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN2_VA, 0)
#define WLAN2_VA1_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN2_VA, 1)
#define WLAN2_VA2_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN2_VA, 2)
#define WLAN2_VA3_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN2_VA, 3)
#define WLAN2_VA4_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN2_VA, 4)
#define WLAN2_VA5_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN2_VA, 5)
#define WLAN2_VA6_DEV_NAME 	DEV_NAME_NUM(ALIASNAME_WLAN2_VA, 6)
#define WLAN2_VXD_DEV_NAME 	"wlan2-vxd"

#ifdef CONFIG_FWD_UNIFIED_FRAMEWORK
struct fwd_partition {
	unsigned char name[FWD_MAX_TARGET_NAMESIZE];
};
#endif

struct lanPortBindIfname{
	uint64_t bindMask;// bindmask of  wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM]
	char ifname[IFNAMSIZE];
};

typedef unsigned short U16;

typedef struct _DNS_HDR
{
	U16 id;
	U16 tag;
	U16 numq;
	U16 numa;
	U16 numa1;
	U16 numa2;
}DNS_HDR;

#define RR_NAMESIZE 512

typedef struct _rr {
    unsigned short      flags;
    char	  name[RR_NAMESIZE];
    
    unsigned int  type;
    unsigned int  class;

    unsigned long ttl;
    int		  len;
    char	  data[RR_NAMESIZE];
} rr_t;


typedef struct _header {
    unsigned short int	id;
    unsigned short      u;

    short int	qdcount;
    short int	ancount;
    short int	nscount;
    short int	arcount;	/* Till here it would fit perfectly to a real
    				 * DNS packet if we had big endian. */

    char	*here;		/* For packet parsing. */
    char	*packet;	/* The actual data packet ... */
    int		len;		/* ... with this size in bytes. */

    char	*rdata;		/* For packet assembly. */
} dnsheader_t;

typedef struct _DNS_QER
{
    U16 type;
    U16 classes;
}DNS_QER;

int DoCmd(char *const argv[], char *file);
int RunSystemCmd(char *filepath, ...);
int isFileExist(char *file_name);

int setPid_toFile(char *file_name);
int getPid_fromFile(char *file_name);
int killDaemonByPidFile(char *pidFile);
int find_pid_by_name( char* pidName);
int write_line_to_file(char *filename, int mode, char *line_data);
#ifdef CONFIG_AUTO_DHCP_CHECK
int discovery_dhcp(char *wan_if);
#endif
int getLanPortIfName(char* name,int index);

#ifdef WLAN_VDEV_SUPPORT
// keep the same as rtl8192cd/8192cd_cfg.h
#define WLAN_VDEV_NONE				0
#define WLAN_VDEV_FOR_V4			(1<<0)	//BIT 0
#define WLAN_VDEV_FOR_V6			(1<<1)	//BIT 1
#define WLAN_VDEV_IPPROTO_V4		(1<<2)	//BIT 2
#define WLAN_VDEV_IPPROTO_V6		(1<<3)	//BIT 3
#define WLAN_VDEV_OPT_IP_DROP		(1<<4)	//BIT 4: if packet ip version mismatch bridge ip version, 1-drop, 0-continue
#define WLAN_VDEV_OPT_DHCP_TRAP		(1<<5)	//BIT 5: if packet is DHCP packet, 1-trap to br0, 0-passthrough

void wlan_vdev_enable(char *dev_name, char *br_name);
void wlan_vdev_disable(char *dev_name, char *br_name, int force);
void wlan_vdev_action_all(int root_wlan_idx, int action, int force);
#endif /*WLAN_VDEV_SUPPORT*/

#endif
