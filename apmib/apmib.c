/*
 *      Routines to handle MIB operation
 *
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *
 *      $Id: apmib.c,v 1.16 2009/09/03 05:04:41 keith_huang Exp $
 *
 */

// include file
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <net/if.h>
#ifdef CONFIG_IPV6
#include <linux/if_addr.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#include "apmib.h"
#include "mibtbl.h"

/* Shared Memory */
#if CONFIG_APMIB_SHARED_MEMORY == 1
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#endif
#ifdef RTL_DEF_SETTING_IN_FW
#include "def_setting_config.h"
#endif
// MAC address filtering
typedef struct _filter {
	struct _filter *prev, *next;
	char val[1];
} FILTER_T, *FILTER_Tp;

typedef struct _linkChain {
	FILTER_Tp pUsedList, pFreeList;
	int size, num, usedNum, compareLen, realsize;
	char *buf;
} LINKCHAIN_T, *LINKCHAIN_Tp;


// macro to remove a link list entry
#define REMOVE_LINK_LIST(entry) { \
	if ( entry ) { \
		if ( entry->prev ) \
			entry->prev->next = entry->next; \
		if ( entry->next ) \
			entry->next->prev = entry->prev; \
	} \
}

// macro to add a link list entry
#define ADD_LINK_LIST(list, entry) { \
	if ( list == NULL ) { \
		list = entry; \
		list->prev = list->next = entry; \
	} \
	else { \
		entry->prev = list; \
		entry->next = list->next; \
		list->next = entry; \
		entry->next->prev = entry; \
	} \
}

// local routine declaration
static int flash_read(char *buf, int offset, int len);
static int flash_write(char *buf, int offset, int len);

#if (defined BACKUP_CURRENT_SETTING_SUPPORT)||(defined BACKUP_DEFAULT_SETTING_SUPPORT)||(defined BACKUP_HARDWARE_SETTING_SUPPORT)
static int flash_read_withtype(char *buf, int offset, int len,CONFIG_DATA_T srcSetting);
static int flash_write_withtype(char *buf, int offset, int len,CONFIG_DATA_T srcSetting);
#endif
#ifndef MIB_TLV
static int init_linkchain(LINKCHAIN_Tp pLinkChain, int size, int num);
static int add_linkchain(LINKCHAIN_Tp pLinkChain, char *val);
static int delete_linkchain(LINKCHAIN_Tp pLinkChain, char *val);
static void delete_all_linkchain(LINKCHAIN_Tp pLinkChain);
static int get_linkchain(LINKCHAIN_Tp pLinkChain, char *val, int index);
static int mibtbl_check(const mib_table_entry_T *mib_tbl, int *size);
#endif

#ifdef COMPRESS_MIB_SETTING
unsigned int mib_compress_write(CONFIG_DATA_T type, unsigned char *data);
int mib_updateDef_compress_write(CONFIG_DATA_T type, char *data, PARAM_HEADER_T *pheader);
#endif

#ifdef MIB_TLV
int get_tblentry(void *pmib,unsigned int offset,int num,const mib_table_entry_T *mib_tbl,void *val, int index);
int add_tblentry(void *pmib, unsigned offset, int num,const mib_table_entry_T *mib_tbl,void *val);
int delete_tblentry(void *pmib, unsigned offset, int num,const mib_table_entry_T *mib_tbl,void *val);
int delete_all_tblentry(void *pmib, unsigned int offset, int num,const mib_table_entry_T *mib_tbl);
int Decode(unsigned char *ucInput, unsigned int inLen, unsigned char *ucOutput);
unsigned int mib_tlv_init(const mib_table_entry_T *mib_tbl, unsigned char *from_data, void *pfile, unsigned int tlv_content_len);
unsigned int mib_get_setting_len(CONFIG_DATA_T type);
unsigned int mib_tlv_save(CONFIG_DATA_T type, void *mib_data, unsigned char *mib_tlvfile, unsigned int *tlv_content_len);
void mib_display_data_content(CONFIG_DATA_T type, unsigned char * pdata, unsigned int mib_data_len);
void mib_display_tlv_content(CONFIG_DATA_T type, unsigned char * ptlv, unsigned int mib_tlv_len);
int mib_search_by_id(const mib_table_entry_T *mib_tbl, unsigned short mib_id, unsigned char *pmib_num, const mib_table_entry_T **ppmib, unsigned int *offset);
#endif
mib_table_entry_T* mib_get_table(CONFIG_DATA_T type);
int mib_write_to_raw(const mib_table_entry_T *mib_tbl, void *data, unsigned char *pfile, unsigned int *idx);

// local & global variable declaration
APMIB_Tp pMib=NULL;
APMIB_Tp pMibDef;
#ifdef HEADER_LEN_INT
HW_PARAM_HEADER_T hsHeader;
PARAM_HEADER_T dsHeader, csHeader;
#else
PARAM_HEADER_T hsHeader, dsHeader, csHeader;
#endif
HW_SETTING_Tp pHwSetting;
#ifdef DEFSETTING_AUTO_UPDATE
APMIB_Tp pMibLastDef;
PARAM_HEADER_T lastDsHeader;
#endif

#ifdef BLUETOOTH_HW_SETTING_SUPPORT
BLUETOOTH_HW_SETTING_Tp pBlueToothHwSetting;
HW_PARAM_HEADER_T blueToothHwHeader;
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
CUSTOMER_HW_SETTING_Tp pCustomerHwSetting;
PARAM_HEADER_T customerHwHeader;
#endif
#ifdef RF_DPK_SETTING_SUPPORT
RF_DPK_SETTING_Tp pRfDpkSetting;
HW_PARAM_HEADER_T rfDpkHeader;
#endif /* RF_DPK_SETTING_SUPPORT */
int compress_hw_setting = 1;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
int bluetooth_compress_hw_setting = 1;
#endif
int wlan_idx=0;	// interface index 
int vwlan_idx=0;	// initially set interface index to root
int wlan_idx_bak=0;
int vwlan_idx_bak=0;


// ------------------------ RTL CODE - START ------------------------
#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
char * RTL_LOG_CALLER[] = {
        [RTL_LOG_CALLER__SYS  ] = "safe-syslog",
        [RTL_LOG_CALLER__SS   ] = "safe-script",
        [RTL_LOG_CALLER__SM   ] = "safe-mode  ",
        [RTL_LOG_CALLER__AP   ] = "safe-apmib ",
        [RTL_LOG_CALLER__LIST ] = "safe-list  ",
        [RTL_LOG_CALLER__DBG  ] = "rtl-debug  ",
};
#if CONFIG_APMIB_SHARED_MEMORY == 1
//#define RTL_LOCK_ALREADY_TAKEN 0xDEADBEAF
unsigned int val_RTL_LOCK_ALREADY_TAKEN = 0;
#endif
#endif
// ------------------------ RTL CODE - END ------------------------



#ifdef AP_CONTROLER_SUPPORT
char *virtual_flash=NULL;
#endif
#ifdef MULTI_WAN_SUPPORT
int wan_idx=0;
int wan_idx_bak=0;
#endif
#ifdef HTTP_FILE_SERVER_HTM_UI
char httpfile_dirpath[256]={0};
char httpfile_type=0;
char httpfile_order=0;
#endif

#ifdef MIB_TLV
#else
static LINKCHAIN_T wlanMacChain[NUM_WLAN_INTERFACE][NUM_VWLAN_INTERFACE+1];
static LINKCHAIN_T wdsChain[NUM_WLAN_INTERFACE][NUM_VWLAN_INTERFACE+1];
#if defined(WIFI_SIMPLE_CONFIG) && defined(CONFIG_APP_TR069)
static LINKCHAIN_T wscERChain[NUM_WLAN_INTERFACE][NUM_VWLAN_INTERFACE+1];
#endif
static LINKCHAIN_T scheduleRuleChain[NUM_WLAN_INTERFACE][NUM_VWLAN_INTERFACE+1];

//### edit by sen_liu 2011.5.13 sync to meshAclChain in apmib_get()
//#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)
static LINKCHAIN_T meshAclChain;
//#endif
//### end

#ifdef HOME_GATEWAY
static LINKCHAIN_T portFwChain, ipFilterChain, portFilterChain, macFilterChain, triggerPortChain;
static LINKCHAIN_T urlFilterChain;
#ifdef ROUTE_SUPPORT
static LINKCHAIN_T staticRouteChain;
#endif
#ifdef CHECK_IPV6_DHCP_OPTION
static LINKCHAIN_T dhcp6Option16Chain;
static LINKCHAIN_T dhcp6Option17Chain;
#endif
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
static LINKCHAIN_T capPortalAllowChain;
#endif


#if defined(CONFIG_CMCC_QOS)
#if defined(CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT)
static LINKCHAIN_T qoswanPortChain;
#endif
static LINKCHAIN_T qosclassChain;
static LINKCHAIN_T qostypeChain;
static LINKCHAIN_T qosruleChain;
static LINKCHAIN_T qosqueueChain;
static LINKCHAIN_T qosappChain;
static LINKCHAIN_T qoscarChain;
static LINKCHAIN_T qoscarUpIfaceChain;
static LINKCHAIN_T qoscarUpVlanChain;
static LINKCHAIN_T qoscarUpIpChain;
static LINKCHAIN_T qoscarDownIfaceChain;
static LINKCHAIN_T qoscarDownVlanChain;
static LINKCHAIN_T qoscarDownIpChain;


#elif defined(QOS_OF_TR069) || defined(MULTI_WAN_QOS)
#if defined(CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT)
static LINKCHAIN_T qoswanPortChain;
#endif
static LINKCHAIN_T qosqueueChain;
static LINKCHAIN_T qosclassChain;
static LINKCHAIN_T Tr098QOSAppChain;
static LINKCHAIN_T Tr098QOSFlowChain;
static LINKCHAIN_T qospolicerChain;
static LINKCHAIN_T qosqueuestatsChain;

#elif defined(GW_QOS_ENGINE) || defined(QOS_BY_BANDWIDTH)
static LINKCHAIN_T qosChain;
#endif

#ifdef CONFIG_RTL_AIRTIME
static LINKCHAIN_T airtimeChain;
static LINKCHAIN_T airtimeChain2;
#endif
#endif
static LINKCHAIN_T dhcpRsvdIpChain;
#if defined(VLAN_CONFIG_SUPPORTED)
static LINKCHAIN_T vlanConfigChain;
#endif

#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
static LINKCHAIN_T dhcpServerOptionChain;
static LINKCHAIN_T dhcpClientOptionChain;
static LINKCHAIN_T dhcpsServingPoolChain;
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */

#endif

#ifdef HOME_GATEWAY
#ifdef VPN_SUPPORT
static LINKCHAIN_T  ipsecTunnelChain;
#endif
#endif

#ifdef TLS_CLIENT
static LINKCHAIN_T  certRootChain;
static LINKCHAIN_T  certUserChain;
#endif

#ifndef MIB_TLV
#ifdef RTK_CAPWAP
static LINKCHAIN_T  capwapWtpConfigChain;
#endif
#endif

#if CONFIG_APMIB_SHARED_MEMORY == 1

#ifdef __mips__
#define FTOK_PATH "/var"
#else
#define FTOK_PATH "flash"
#endif

char *shm_name[] = {
	"/var/HWCONF",	/* HWCONF_SHM_KEY */
	"/var/DSCONF",	/* DSCONF_SHM_KEY */
	"/var/CSCONF",	/* CSCONF_SHM_KEY */
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
	"/var/BLUETOOTH_HWCONF",
#else
	""
#endif
#ifdef	CUSTOMER_HW_SETTING_SUPPORT
	"/var/CUSTOMER_HWCONF",
#else
	"",
#endif
#ifdef BACKUP_CURRENT_SETTING_SUPPORT
	"/var/BACKUP_CSCONF",
#else
	"",
#endif
#ifdef	RF_DPK_SETTING_SUPPORT
	"/var/RF_DPKCONF",
#else
    ""
#endif /* RF_DPK_SETTING_SUPPORT */
#ifdef BACKUP_DEFAULT_SETTING_SUPPORT
	"/var/BACKUP_DSCONF",
#else
	"",
#endif
};
static int apmib_sem_id = -1;

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                (Linux specific) */
};


/*
* Convert ifIndex to system interface name, e.g. eth0,vc0...
*/

char *ifGetName(int ifindex, char *buffer, unsigned int len)
{
#ifdef MULTI_WAN_SUPPORT

	MEDIA_TYPE_T mType;

	if ( ifindex == DUMMY_IFINDEX )
		return 0;
	if (PPP_INDEX(ifindex) == DUMMY_PPP_INDEX)
	{
		mType = MEDIA_INDEX(ifindex);
		if (mType == MEDIA_ATM)
			snprintf( buffer, len,  "%s%u",ALIASNAME_VC, VC_INDEX(ifindex) );
		else if (mType == MEDIA_ETH)
#ifdef CONFIG_RTL_MULTI_ETH_WAN
			snprintf( buffer, len, "%s%d",ALIASNAME_MWNAS, ETH_INDEX(ifindex));
#else
			snprintf( buffer, len,  "%s%u",ALIASNAME_ETH, ETH_INDEX(ifindex) );
#endif
		else
			return 0;
	}else{
		snprintf( buffer, len,  "%s%u",ALIASNAME_PPP, PPP_INDEX(ifindex) );
	}
	return buffer;
#else
	return NULL;
#endif
}

void apmib_save_wlanIdx()
{
	wlan_idx_bak	=	wlan_idx;
	vwlan_idx_bak	=	vwlan_idx;	
}
void apmib_recov_wlanIdx()
{
	wlan_idx	=	wlan_idx_bak;
	vwlan_idx	=	vwlan_idx_bak;
}
#ifdef RTK_REINIT_SUPPORT
void apmib_save_Idx()
{
	wlan_idx_bak	=	wlan_idx;
	vwlan_idx_bak	=	vwlan_idx;
#ifdef MULTI_WAN_SUPPORT	
	wan_idx_bak		=	wan_idx;
#endif
}
void apmib_recov_Idx()
{
	wlan_idx	=	wlan_idx_bak;
	vwlan_idx	=	vwlan_idx_bak;
#ifdef MULTI_WAN_SUPPORT
	wan_idx		=	wan_idx_bak;
#endif
}

#endif
static int apmib_sem_create(void)
{
	int sem_id;
	
	/* Generate a System V IPC key */ 
	key_t key;
	key = ftok(FTOK_PATH, 0xD4);
	if (key == -1) {
	#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
		RTL_LOG_DBG_A("Semaphore ftok() failed !! [%s]\n", strerror(errno));
	#else
		printf("APMIB Semaphore ftok() failed !! [%s]\n", strerror(errno));
	#endif
		return -1;
	}
	
	/* Get a semaphore set with 1 semaphore */
	sem_id = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666);
	if (sem_id == -1) {
		if (errno == EEXIST) {
			sem_id = semget(key, 1, 0666);
			if (sem_id == -1) {
			#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
				RTL_LOG_DBG_A("Semaphore semget() failed !! [%s]\n", strerror(errno));
			#else
				printf("APMIB Semaphore semget() failed !! [%s]\n", strerror(errno));
			#endif
				return -1;
			}
		} else {
		#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
			RTL_LOG_DBG_A("Semaphore semget() failed !! [%s]\n", strerror(errno));
		#else
				printf("APMIB Semaphore semget() failed !! [%s]\n", strerror(errno));
		#endif
			return -1;
		}
	} else {
		/* Initialize semaphore #0 to 1 */
		union semun arg;
		
		arg.val = 1;
		if (semctl(sem_id, 0, SETVAL, arg) == -1) {
		#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
			RTL_LOG_DBG_A("Semaphore semctl() failed !! [%s]\n", strerror(errno));
		#else
			printf("APMIB Semaphore semctl() failed !! [%s]\n", strerror(errno));
		#endif
			
			return -1;
		}
	}

	apmib_sem_id = sem_id;
	return 0;
}

int apmib_sem_lock(void)
{
#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
	if(RTL_LOCK_ALREADY_TAKEN != val_RTL_LOCK_ALREADY_TAKEN){
#endif
		struct sembuf sop[1];
		
		if (apmib_sem_id == -1) {
			if (apmib_sem_create() == -1) {
			#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
				RTL_LOG_DBG_A("apmib_sem_create fail\n");
			#else
	 			printf("apmib_sem_create fail\n");
			#endif			
				return -1;
			}
		}
		
		sop[0].sem_num = 0;
		sop[0].sem_op = -1;
		sop[0].sem_flg = SEM_UNDO;
	try_again:
		if (semop(apmib_sem_id, sop, 1) == -1) {
			if (errno == EINTR) {
				//printf("APMIB Semaphore Lock semop() failed !! [%s]\n", strerror(errno));
				goto try_again;
			}
		#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
			RTL_LOG_DBG_A("Semaphore Lock semop() failed !! [%s]", strerror(errno));
		#else
			printf("APMIB Semaphore Lock semop() failed !! [%s]\n", strerror(errno));
		#endif		
			return -1;
#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
	}
#endif
	   //printf("APMIB Semaphore Lock semop() success !! \n");
	}
#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
	else
	{
	    RTL_LOG_DBG_A("Semaphore Lock semop() ALREADY TAKEN BY SAFE_MODE !!!");
	}
#endif
	return 0;		
}

int apmib_sem_unlock(void)
{
#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
	if(RTL_LOCK_ALREADY_TAKEN != val_RTL_LOCK_ALREADY_TAKEN)
	{
#endif
		struct sembuf sop[1];
		
		sop[0].sem_num = 0;
		sop[0].sem_op = 1;
		sop[0].sem_flg = SEM_UNDO;
		if (semop(apmib_sem_id, sop, 1) == -1) {
			printf("APMIB Semaphore Unlock semop() failed !! [%s]\n", strerror(errno));
			return -1;
		}	
		// printf("APMIB Semaphore Unlock semop() success !!\n");
		return 0;
#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
	}else{
		RTL_LOG_DBG_A("Semaphore Unlock semop() ALREADY TAKEN BY SAFE_MODE !!!");
	}
#endif
}
	
int apmib_shm_free(void *shm_memory, int shm_key)
{
	return (shmdt(shm_memory));
}

static char *apmib_shm_calloc(size_t nmemb, size_t size, int shm_key, int *created)
{
	int shm_id, shm_size;
	char *shm_memory; //, *shm_name;	
	
	*created = 0;
	shm_size = (nmemb * size);

	/* Generate a System V IPC key */ 
	key_t key;
	key = ftok(FTOK_PATH, (0x3C + shm_key));
	if (key == -1) {
		printf("%s ftok() failed !! [%s]\n", shm_name[shm_key], strerror(errno));
		return NULL;
	}
		 
	/* Allocate a shared memory segment */
	shm_id = shmget(key, shm_size, IPC_CREAT | IPC_EXCL | 0666);
	if (shm_id == -1) {
		if (errno == EEXIST) {
			*created = 1;
			shm_id = shmget(key, shm_size, 0666);
			if (shm_id == -1) {
				printf("%s shmget() failed !! [%s]\n", shm_name[shm_key], strerror(errno));
				return NULL;
			}
		} else {
			printf("%s shmget() failed !! [%s]\n", shm_name[shm_key], strerror(errno));
			return NULL;
		}
	}
		 
	/* Attach the shared memory segment */
	shm_memory = (char *)shmat(shm_id, NULL, 0);
	if ((int)shm_memory == -1) {
		printf("%s shmat() failed [%s]\n", shm_name[shm_key], strerror(errno));
		return NULL;
	}
		
	if (*created) {
		return shm_memory;
	}

	memset(shm_memory, 0, shm_size);
	return shm_memory;
}
#endif


#ifdef _LITTLE_ENDIAN_
static int _mib_swap_value(const mib_table_entry_T *mib, void *data)
{
	short *pShort;
	int *pInt;

	switch (mib->type)
	{
	case WORD_T:
		pShort = (short *) data;
		*pShort = WORD_SWAP(*pShort);
		break;
	case DWORD_T:
		pInt = (int *) data;
		*pInt = DWORD_SWAP(*pInt);
		break;
#ifdef CONFIG_IPV6
	case RADVDPREFIX_T:
		{
			radvdCfgParam_Tp radvdCfg=(radvdCfgParam_Tp)data;
			radvdCfg->interface.MaxRtrAdvInterval=DWORD_SWAP(radvdCfg->interface.MaxRtrAdvInterval);
			radvdCfg->interface.MinRtrAdvInterval=DWORD_SWAP(radvdCfg->interface.MinRtrAdvInterval);
			radvdCfg->interface.MinDelayBetweenRAs=DWORD_SWAP(radvdCfg->interface.MinDelayBetweenRAs);
			radvdCfg->interface.AdvLinkMTU=DWORD_SWAP(radvdCfg->interface.AdvLinkMTU);
			radvdCfg->interface.AdvReachableTime=DWORD_SWAP(radvdCfg->interface.AdvReachableTime);
			radvdCfg->interface.AdvRetransTimer=DWORD_SWAP(radvdCfg->interface.AdvRetransTimer);
			radvdCfg->interface.AdvDefaultLifetime=WORD_SWAP(radvdCfg->interface.AdvDefaultLifetime);
			break;
		}
	case DNSV6_T:
		{
			int i=0,j=0;
			dnsv6CfgParam_Tp dnsv6Cfg=(dnsv6CfgParam_Tp)data;
			for(i=0;i<RR_MAX_NUM;i++){
				for(j=0;j<8;j++){
					dnsv6Cfg->rr[i].address[j]=WORD_SWAP(dnsv6Cfg->rr[i].address[j]);
				}
			}
			break;
		}
	case DHCPV6S_T:
		{				
			break;
		}
	case DHCPV6C_T:
		{	
			dhcp6cCfgParam_Tp dhcp6cCfg=(dhcp6cCfgParam_Tp)data;
			dhcp6cCfg->dhcp6pd.sla_id=DWORD_SWAP(dhcp6cCfg->dhcp6pd.sla_id);
			dhcp6cCfg->dhcp6pd.sla_len=DWORD_SWAP(dhcp6cCfg->dhcp6pd.sla_len);
			break;
		}
	case ADDR6_T:
		{	
			int i=0,j=0;
			daddrIPv6CfgParam_Tp daddrV6Cfg=(daddrIPv6CfgParam_Tp)data; 	
			daddrV6Cfg->enabled=DWORD_SWAP(daddrV6Cfg->enabled);
			for(i=0;i<2;i++)
				daddrV6Cfg->prefix_len[i]=DWORD_SWAP(daddrV6Cfg->prefix_len[i]);
			for(i=0;i<2;i++)
				for(j=0;j<8;j++)
					daddrV6Cfg->addrIPv6[i][j]=WORD_SWAP(daddrV6Cfg->addrIPv6[i][j]);
			break;
		}
	case ADDRV6_T:
		{	
			int i=0;
			addr6CfgParam_Tp addr6Cfg=(addr6CfgParam_Tp)data;
			addr6Cfg->prefix_len=DWORD_SWAP(addr6Cfg->prefix_len);
			for(i=0;i<8;i++)
				addr6Cfg->addrIPv6[i]=WORD_SWAP(addr6Cfg->addrIPv6[i]);
			break;
		}
	case TUNNEL6_T:
		break;		
#endif

	default:
		break;
	}

	return 0;
}

static int _mibtbl_swap_value(const mib_table_entry_T *mib_tbl, void *data, int offset)
{
	int i, j;
	const mib_table_entry_T *mib;
	int new_offset;

	for (i=0; mib_tbl[i].id; i++)
	{
		mib = &mib_tbl[i];
		new_offset = offset + mib->offset;
		for (j=0; j<(mib->total_size / mib->unit_size); j++)
		{
			if (mib->type >= TABLE_LIST_T)
			{
				if (_mibtbl_swap_value(mib->next_mib_table, data, new_offset) != 0)
				{
					fprintf(stderr, "MIB (%s, %d, %d) Error: swap failed\n",
						mib_tbl[i].name, mib_tbl[i].total_size, mib_tbl[i].unit_size);
					return -1;
				}
			}
			else
			{
				_mib_swap_value(mib, (void *)((unsigned int) data + new_offset));
			}
			new_offset += mib->unit_size;
		}
	}

	return 0;
}

void swap_mib_word_value(APMIB_Tp pMib)
{
	mib_table_entry_T *pmib_tl;

	pmib_tl = mib_get_table(CURRENT_SETTING);
	_mibtbl_swap_value(pmib_tl, pMib, 0);
#ifdef VOIP_SUPPORT
	voip_mibtbl_swap_value(&pMib->voipCfgParam);
#endif
}

void swap_mib_value(void *pMib, CONFIG_DATA_T type)
{
	mib_table_entry_T *pmib_tl;
	pmib_tl = mib_get_table(type);
	_mibtbl_swap_value(pmib_tl, pMib, 0);
}
#endif // _LITTLE_ENDIAN_


////////////////////////////////////////////////////////////////////////////////
char *apmib_hwconf(void)
{
	int ver;
	char *buff;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	int created;
#endif

	char hw_setting_start[6];
	
#if 1		
	if ( flash_read(hw_setting_start, HW_SETTING_OFFSET, 6)==0 ) {
			printf("Read hw setting header failed!\n");					
	}
	else 
	{
		//printf("===,the start char from HW_SETTING_OFFSET is %c%c%c%c%c%c\n",
		//		hw_setting_start[0], hw_setting_start[1], hw_setting_start[2], 
		//		hw_setting_start[3], hw_setting_start[4], hw_setting_start[5]);
		
		if (!memcmp(hw_setting_start, HW_SETTING_HEADER_TAG, TAG_LEN) ||
			!memcmp(hw_setting_start, HW_SETTING_HEADER_FORCE_TAG, TAG_LEN) ||
			!memcmp(hw_setting_start, HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN))
		{
			compress_hw_setting = 0;
		}
		else
		{
			compress_hw_setting = 1;
		}
	}
#endif
//fprintf(stderr,"\r\n compress_hw_setting = %u",compress_hw_setting);

	
#if defined(COMPRESS_MIB_SETTING) 		
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;
#endif



#if defined(COMPRESS_MIB_SETTING)
	if(compress_hw_setting)
	{
	flash_read((char *)&compHeader, HW_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T));
	if(memcmp(compHeader.signature, COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);
		if ( (compLen > 0) && (compLen <= HW_SETTING_SECTOR_LEN) ) {
			compFile=malloc(compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}
			expFile=malloc(zipRate*compLen);
			if(expFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
				free(compFile);
				return 0;
			}

			flash_read((char *)compFile, HW_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T));

			expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);
			memcpy((char *)&hsHeader, expFile, sizeof(hsHeader));
			hsHeader.len=WORD_SWAP(hsHeader.len);
		} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}

#if defined(MIB_TLV)
		mib_table_entry_T *pmib_tl = NULL;
		unsigned char *hwMibData;
		unsigned int tlv_content_len = hsHeader.len - 1; // last one is checksum

		hwMibData = malloc(sizeof(HW_SETTING_T)+1); // 1: checksum

		if(hwMibData != NULL)
			memset(hwMibData, 0x00, sizeof(HW_SETTING_T)+1);
	
		pmib_tl = mib_get_table(HW_SETTING);
		unsigned int tlv_checksum = 0;

		if(expFile != NULL)
			tlv_checksum = CHECKSUM_OK(expFile+sizeof(hsHeader), hsHeader.len);

//mib_display_tlv_content(HW_SETTING, expFile+sizeof(PARAM_HEADER_T), hsHeader.len);

		if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(hsHeader), (void*)hwMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
		{
			sprintf((char *)&hsHeader.signature[TAG_LEN], "%02d", HW_SETTING_VER);
			hsHeader.len = sizeof(HW_SETTING_T)+1;
			hwMibData[hsHeader.len-1]  = CHECKSUM(hwMibData, hsHeader.len-1);

			if(expFile!= NULL)
				free(expFile);

			expFile = malloc(sizeof(hsHeader)+hsHeader.len);
			memcpy(expFile, &hsHeader, sizeof(hsHeader));
			memcpy(expFile+sizeof(hsHeader), hwMibData, hsHeader.len);

//mib_display_data_content(HW_SETTING, expFile+sizeof(PARAM_HEADER_T), hsHeader.len-1);

			
		}	

		if(hwMibData != NULL)
			free(hwMibData);
		
#endif // #ifdef MIB_TLV

	}
	}
	else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
	// Read hw setting
{
	if ( flash_read((char *)&hsHeader, HW_SETTING_OFFSET, sizeof(hsHeader))==0 ) {
//		printf("Read hw setting header failed!\n");
		return NULL;
	}
	hsHeader.len=WORD_SWAP(hsHeader.len);
}
	if ( sscanf((char *)&hsHeader.signature[TAG_LEN], "%02d", &ver) != 1)
		ver = -1;

//fprintf(stderr,"\r\n ver = %u",ver);

	if ( memcmp(hsHeader.signature, HW_SETTING_HEADER_TAG, TAG_LEN)) // invalid signatur
	{
		printf("Invalid hw setting signature [sig=%c%c]!\n", hsHeader.signature[0],hsHeader.signature[1]);
#if defined(COMPRESS_MIB_SETTING)
	if(compress_hw_setting)
	{

		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
	}
#endif
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
		return NULL;
	}
//	fprintf(stderr,"\r\n ver=%d len=%d size=%d__[%s-%u]",ver, hsHeader.len,sizeof(HW_SETTING_T),__FILE__,__LINE__);

	if ( (ver != HW_SETTING_VER) || // version not equal to current
		(hsHeader.len < (sizeof(HW_SETTING_T)+1)) ) { // length is less than current
		printf("Invalid hw setting version number or data length[ver=%d, len=%d size=%d]!\n", ver, hsHeader.len,sizeof(HW_SETTING_T));
#if defined(COMPRESS_MIB_SETTING)
	if(compress_hw_setting)
	{

		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
	}
#endif
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
		return NULL;
	}
//	if (ver > HW_SETTING_VER)
//		printf("HW setting version is greater than current [f:%d, c:%d]!\n", ver, HW_SETTING_VER);

#if CONFIG_APMIB_SHARED_MEMORY == 1
	buff = apmib_shm_calloc(1, hsHeader.len, HWCONF_SHM_KEY, &created);
#else
	buff = calloc(1, hsHeader.len);
#endif
	if ( buff == 0 ) {
//		printf("Allocate buffer failed!\n");
#if defined(COMPRESS_MIB_SETTING)
	if(compress_hw_setting)
	{
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
	}
#endif
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
		return NULL;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
    if (created) {
        //printf("No need to read hw setting!\n");
#if defined(COMPRESS_MIB_SETTING)
	if(compress_hw_setting)
	{
	if(compFile != NULL)
		free(compFile);
	if(expFile != NULL)
		free(expFile);
	}
#endif        
        return buff;
    }
#endif

#if defined(COMPRESS_MIB_SETTING)
	if(compress_hw_setting)
	{
	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
		memcpy(buff, expFile+sizeof(hsHeader), hsHeader.len);
		free(expFile);
		free(compFile);
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);		
	}
	}
	else // not compress mib data, copy mibdata from flash
#endif
	if ( flash_read(buff, HW_SETTING_OFFSET+sizeof(hsHeader), hsHeader.len)==0 ) {
//		printf("Read hw setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, HWCONF_SHM_KEY);
#else
		free(buff);
#endif		
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
		return NULL;
	}
	if ( !CHECKSUM_OK((unsigned char *)buff, hsHeader.len) ) {
//		printf("Invalid checksum of hw setting!\n");
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, HWCONF_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}

	if(!compress_hw_setting){
#ifdef MIB_TLV
		int index=0;
		unsigned short *wVal=NULL;
		unsigned int *dwVal=NULL;
		mib_table_entry_T * pmib_tl = mib_get_table(HW_SETTING);//hwmib_root_table
		pmib_tl=pmib_tl->next_mib_table;//hwmib_table
		while(pmib_tl[index].id){
			switch(pmib_tl[index].type)
			{
			case WORD_T:
				wVal=buff+pmib_tl[index].offset;
				*wVal=WORD_SWAP(*wVal);
				break;
			case DWORD_T:
				dwVal=buff+pmib_tl[index].offset;
				*dwVal=DWORD_SWAP(*dwVal);
				break;
			default:
				break;
			}
			index++;
		}
#endif
	}
	return buff;
}
#ifdef BACKUP_HARDWARE_SETTING_SUPPORT
char *apmib_backuphwconf(void)
{
	int ver;
	char *buff;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	int created;
#endif
	
	char hw_setting_start[6];
	
	if ( flash_read_withtype(hw_setting_start, BACKUP_HARDWARE_SETTING_OFFSET, 6,BACKUP_HW_SETTING)==0 ) {
		printf("Read BACKUP hw setting header failed!\n"); 	
	}
	else 
	{
		//printf("===,the start char from HW_SETTING_OFFSET is %c%c%c%c%c%c\n",
		//		hw_setting_start[0], hw_setting_start[1], hw_setting_start[2], 
		//		hw_setting_start[3], hw_setting_start[4], hw_setting_start[5]);
		if (!memcmp(hw_setting_start, BACKUP_HW_SETTING_HEADER_TAG, TAG_LEN) ||
			!memcmp(hw_setting_start, HW_SETTING_HEADER_FORCE_TAG, TAG_LEN) ||
			!memcmp(hw_setting_start, HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN)){
			compress_hw_setting = 0;
		}
		else
		{
			compress_hw_setting = 0;
		}
	}

		
#if defined(COMPRESS_MIB_SETTING) 		
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;
#endif
	
	
	
#if defined(COMPRESS_MIB_SETTING)
	if(compress_hw_setting)
	{
		flash_read((char *)&compHeader, BACKUP_HARDWARE_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T));
		if(memcmp(compHeader.signature, COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
		{
			zipRate = WORD_SWAP(compHeader.compRate);
			compLen = DWORD_SWAP(compHeader.compLen);
			if ( (compLen > 0) && (compLen <= HW_SETTING_SECTOR_LEN) ) 
			{
				compFile=malloc(compLen+sizeof(COMPRESS_MIB_HEADER_T));
				if(compFile==NULL)
				{
					COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
					return 0;
				}
				expFile=malloc(zipRate*compLen);
				if(expFile==NULL)
				{
					COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
					free(compFile);
					return 0;
				}
	
				flash_read((char *)compFile, BACKUP_HARDWARE_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T));	
				expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);
				memcpy((char *)&hsHeader, expFile, sizeof(hsHeader));
				hsHeader.len=WORD_SWAP(hsHeader.len);
			} 
			else 
			{
				COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
				return 0;
			}
	
#if defined(MIB_TLV)
			mib_table_entry_T *pmib_tl = NULL;
			unsigned char *hwMibData;
			unsigned int tlv_content_len = hsHeader.len - 1; // last one is checksum	
			hwMibData = malloc(sizeof(HW_SETTING_T)+1); // 1: checksum	
			if(hwMibData != NULL)
				memset(hwMibData, 0x00, sizeof(HW_SETTING_T)+1);		
			pmib_tl = mib_get_table(HW_SETTING);
			unsigned int tlv_checksum = 0;	
			if(expFile != NULL)
				tlv_checksum = CHECKSUM_OK(expFile+sizeof(hsHeader), hsHeader.len);	
			if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(hsHeader), (void*)hwMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
			{
				sprintf((char *)&hsHeader.signature[TAG_LEN], "%02d", HW_SETTING_VER);
				hsHeader.len = sizeof(HW_SETTING_T)+1;
				hwMibData[hsHeader.len-1]  = CHECKSUM(hwMibData, hsHeader.len-1);
	
				if(expFile!= NULL)
					free(expFile);
	
				expFile = malloc(sizeof(hsHeader)+hsHeader.len);
				memcpy(expFile, &hsHeader, sizeof(hsHeader));
				memcpy(expFile+sizeof(hsHeader), hwMibData, hsHeader.len);
			}		
			if(hwMibData != NULL)
				free(hwMibData);
			
#endif // #ifdef MIB_TLV	
		}
	}
	else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
		// Read hw setting
	{
		if ( flash_read_withtype((char *)&hsHeader, BACKUP_HARDWARE_SETTING_OFFSET, sizeof(hsHeader),BACKUP_HW_SETTING)==0 ) {
			printf("Read backuphw setting header failed!\n");
			return NULL;
		}
		hsHeader.len=WORD_SWAP(hsHeader.len);
	}
		if ( sscanf((char *)&hsHeader.signature[TAG_LEN], "%02d", &ver) != 1)
			ver = -1;

		if ( memcmp(hsHeader.signature, BACKUP_HW_SETTING_HEADER_TAG, TAG_LEN)) // invalid signatur
		{
		printf("Invalid backuphw setting signature [sig=%c%c]!\n", hsHeader.signature[0],hsHeader.signature[1]);

#if defined(COMPRESS_MIB_SETTING)
		if(compress_hw_setting)
		{
	
			if(compFile != NULL)
				free(compFile);
			if(expFile != NULL)
				free(expFile);
		}
#endif
		return NULL;
		}
	
		if ( (ver != HW_SETTING_VER) || // version not equal to current
			(hsHeader.len < (sizeof(HW_SETTING_T)+1)) ) { // length is less than current
			printf("Invalid backuphw setting version number or data length[ver=%d, len=%d size=%d]!\n", ver, hsHeader.len,sizeof(HW_SETTING_T));

#if defined(COMPRESS_MIB_SETTING)
			if(compress_hw_setting)
			{	
				if(compFile != NULL)
					free(compFile);
				if(expFile != NULL)
					free(expFile);
			}
#endif
			return NULL;
		}
	
#if CONFIG_APMIB_SHARED_MEMORY == 1
		buff = apmib_shm_calloc(1, hsHeader.len, BACKUP_HWCONF_SHM_KEY, &created);
#else
		buff = calloc(1, hsHeader.len);
#endif
		if ( buff == 0 ) {
			printf("Allocate buffer failed![%s-%u]\n",__FILE__,__LINE__);
#if defined(COMPRESS_MIB_SETTING)
		if(compress_hw_setting)
		{
			if(compFile != NULL)
				free(compFile);
			if(expFile != NULL)
				free(expFile);
		}
#endif
		return NULL;
		}
	
#if CONFIG_APMIB_SHARED_MEMORY == 1
		if (created) {
			//printf("No need to read hw setting!\n");
#if defined(COMPRESS_MIB_SETTING)
		if(compress_hw_setting)
		{
			if(compFile != NULL)
				free(compFile);
			if(expFile != NULL)
				free(expFile);
		}
#endif
		return buff;
		}
#endif
	
#if defined(COMPRESS_MIB_SETTING)
		if(compress_hw_setting)
		{
			if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
			{
				memcpy(buff, expFile+sizeof(hsHeader), hsHeader.len);
				free(expFile);
				free(compFile);
				//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);		
			}
		}
		else // not compress mib data, copy mibdata from flash
#endif
		if ( flash_read_withtype(buff, BACKUP_HARDWARE_SETTING_OFFSET+sizeof(hsHeader), hsHeader.len,BACKUP_HW_SETTING)==0 ) {
	 		printf("Read backuphw setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(buff, BACKUP_HWCONF_SHM_KEY);
#else
			free(buff);
#endif		
			return NULL;
		}
		if ( !CHECKSUM_OK((unsigned char *)buff, hsHeader.len) ) {
	 		printf("Invalid checksum of hw setting!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(buff, BACKUP_HWCONF_SHM_KEY);
#else
			free(buff);
#endif		
			return NULL;
		}
	
		if(!compress_hw_setting){
#ifdef MIB_TLV
			int index=0;
			unsigned short *wVal=NULL;
			unsigned int *dwVal=NULL;
			mib_table_entry_T * pmib_tl = mib_get_table(HW_SETTING);//hwmib_root_table
			pmib_tl=pmib_tl->next_mib_table;//hwmib_table
			while(pmib_tl[index].id){
				switch(pmib_tl[index].type)
				{
				case WORD_T:
					wVal=buff+pmib_tl[index].offset;
					*wVal=WORD_SWAP(*wVal);
					break;
				case DWORD_T:
					dwVal=buff+pmib_tl[index].offset;
					*dwVal=DWORD_SWAP(*dwVal);
					break;
				default:
					break;
				}
				index++;
			}
#endif
		}
		return buff;
		printf("apmib backuphwconf init ok \n");
}

#endif



#ifdef BLUETOOTH_HW_SETTING_SUPPORT
char *apmib_bluetoothHwconf(void)
{
	int ver;
	char *buff;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	int created;
#endif
	char bluetooth_hw_setting_start[6];
	if ( flash_read(bluetooth_hw_setting_start, BLUETOOTH_HW_SETTING_OFFSET, 6)==0 ) {
			printf("Read hw setting header failed!\n");					
	}
	else 
	{
		//printf("===,the start char from HW_SETTING_OFFSET is %c%c%c%c%c%c\n",
		//		hw_setting_start[0], hw_setting_start[1], hw_setting_start[2], 
		//		hw_setting_start[3], hw_setting_start[4], hw_setting_start[5]);
		
		if (!memcmp(bluetooth_hw_setting_start, BLUETOOTH_HW_SETTING_HEADER_TAG, TAG_LEN))
		{
			bluetooth_compress_hw_setting = 0;
		}
		else
		{
			bluetooth_compress_hw_setting = 1;
		}
	}

#ifdef COMPRESS_MIB_SETTING
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;
		
	if(bluetooth_compress_hw_setting){
		

		flash_read((char *)&compHeader, BLUETOOTH_HW_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T));
		//COMP_TRACE(stderr,"\r\n sig=%s __[%s-%u]",compHeader.signature,__FILE__,__LINE__);
		if(memcmp(compHeader.signature, COMP_BT_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
		{
			zipRate = WORD_SWAP(compHeader.compRate);
			compLen = DWORD_SWAP(compHeader.compLen);

			if ( (compLen > 0) && (compLen <= BLUETOOTH_HW_SETTING_SECTOR_LEN) ) 
			{
				compFile=malloc(compLen+sizeof(COMPRESS_MIB_HEADER_T));
				if(compFile==NULL)
				{
					COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
					return 0;
				}
				expFile=malloc(zipRate*compLen);
				if(expFile==NULL)
				{
					COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
					free(compFile);
					return 0;
				}

				flash_read((char *)compFile, BLUETOOTH_HW_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T));

				expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);
			//	printf("expandLen read len: %d\n", expandLen);

				// copy the mib header from expFile
				memcpy((char *)&blueToothHwHeader, expFile, sizeof(blueToothHwHeader));
				blueToothHwHeader.len=HEADER_SWAP(blueToothHwHeader.len);

				//check mib ver and tag
				if ( sscanf((char *)&blueToothHwHeader.signature[TAG_LEN], "%02d", &ver) != 1)
					ver = -1;
				
				if ( memcmp(blueToothHwHeader.signature, BLUETOOTH_HW_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
					(ver != BLUETOOTH_HW_SETTING_VER) // version not equal
					) { 
					
					printf("Invalid default setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",
						blueToothHwHeader.signature[0], blueToothHwHeader.signature[1], ver, blueToothHwHeader.len);
					if(compFile != NULL)
						free(compFile);
					if(expFile != NULL)
						free(expFile);
					return NULL;
				}
	
			} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}
#ifdef MIB_TLV
		mib_table_entry_T *pmib_tl = NULL;
		unsigned char *bluetoothHwMibData;
		unsigned int tlv_content_len = blueToothHwHeader.len - 1; // last one is checksum

		bluetoothHwMibData = malloc(sizeof(BLUETOOTH_HW_SETTING_T)+1); // 1: checksum

		if(bluetoothHwMibData != NULL)
			memset(bluetoothHwMibData, 0x00, sizeof(BLUETOOTH_HW_SETTING_T)+1);
	
		pmib_tl = mib_get_table(BLUETOOTH_HW_SETTING);
		unsigned int tlv_checksum = 0;

		if(expFile != NULL)
			tlv_checksum = CHECKSUM_OK(expFile+sizeof(PARAM_HEADER_T), blueToothHwHeader.len);

//mib_display_tlv_content(DEFAULT_SETTING, expFile+sizeof(PARAM_HEADER_T), dsHeader.len);
		
		if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(PARAM_HEADER_T), (void*)bluetoothHwMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
		{
			sprintf((char *)&blueToothHwHeader.signature[TAG_LEN], "%02d", BLUETOOTH_HW_SETTING_VER);
			blueToothHwHeader.len = sizeof(BLUETOOTH_HW_SETTING_T)+1;
			bluetoothHwMibData[blueToothHwHeader.len-1]  = CHECKSUM(bluetoothHwMibData, sizeof(BLUETOOTH_HW_SETTING_T));

			if(expFile!= NULL)
				free(expFile);

			expFile = malloc(sizeof(PARAM_HEADER_T)+blueToothHwHeader.len);
			memcpy(expFile, &blueToothHwHeader, sizeof(PARAM_HEADER_T));
			memcpy(expFile+sizeof(PARAM_HEADER_T), bluetoothHwMibData, blueToothHwHeader.len);
			
//mib_display_data_content(DEFAULT_SETTING, expFile+sizeof(PARAM_HEADER_T), dsHeader.len-1);
		}	
		else
		{
			COMP_TRACE(stderr,"\r\n ERR!Invalid checksum[%u] or mib_tlv_init() fail! __[%s-%u]",tlv_checksum,__FILE__,__LINE__);
		}
		
		if(bluetoothHwMibData != NULL)
			free(bluetoothHwMibData);
		
#endif // #ifdef MIB_TLV		
	}
	else
	{
		return NULL;
	}
  }
  else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
	{
		if ( flash_read((char *)&blueToothHwHeader, BLUETOOTH_HW_SETTING_OFFSET, sizeof(blueToothHwHeader))==0 ) {
	//		printf("Read default setting header failed!\n");
			return NULL;
		}
		blueToothHwHeader.len=WORD_SWAP(blueToothHwHeader.len);
	}
	if ( sscanf((char *)&blueToothHwHeader.signature[TAG_LEN], "%02d", &ver) != 1)
		ver = -1;


	if ( memcmp(blueToothHwHeader.signature, BLUETOOTH_HW_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
		(ver != BLUETOOTH_HW_SETTING_VER) || // version not equal 
		(blueToothHwHeader.len < (sizeof(BLUETOOTH_HW_SETTING_T)+1)) ) { // length is less 
		printf("Invalid bluetooth hw setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",	blueToothHwHeader.signature[0], blueToothHwHeader.signature[1], ver, blueToothHwHeader.len);
		printf("Expect [sig=%s, ver=%d, len=%d]!\n", BLUETOOTH_HW_SETTING_HEADER_TAG, BLUETOOTH_HW_SETTING_VER, sizeof(BLUETOOTH_HW_SETTING_T)+1);
		return NULL;
	}
//	if (ver > DEFAULT_SETTING_VER)
//		printf("Default setting version is greater than current [f:%d, c:%d]!\n", ver, DEFAULT_SETTING_VER);

#if CONFIG_APMIB_SHARED_MEMORY == 1
	buff = apmib_shm_calloc(1, blueToothHwHeader.len, BLUETOOTH_HW_SHM_KEY, &created);
#else
	buff = calloc(1, blueToothHwHeader.len);
#endif	
	if ( buff == 0 ) {
//		printf("Allocate buffer failed!\n");
		return NULL;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
    if (created) {
        return buff;
    }
#endif
#ifdef COMPRESS_MIB_SETTING

	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
		memcpy(buff, expFile+sizeof(PARAM_HEADER_T), blueToothHwHeader.len);
		free(expFile);
		free(compFile);
	}
	else // not compress mib data, copy mibdata from flash
#endif
	if ( flash_read(buff, BLUETOOTH_HW_SETTING_OFFSET+sizeof(blueToothHwHeader), blueToothHwHeader.len)==0 ) {
		printf("Read bluetooth hw setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, BLUETOOTH_HW_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}
	if ( !CHECKSUM_OK((unsigned char *)buff, blueToothHwHeader.len) ) {
		printf("Invalid checksum of bluetooth hw setting! \n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, BLUETOOTH_HW_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}
	return buff;
}

#ifdef BACKUP_HARDWARE_SETTING_SUPPORT
char *apmib_backupbluetoothHwconf(void)
{
	int ver;
	char *buff;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	int created;
#endif
	char bluetooth_hw_setting_start[6];
	if ( flash_read_withtype (bluetooth_hw_setting_start,BACKUP_BLUETOOTH_HW_SETTING_OFFSET, 6,BACKUP_BLUETOOTH_HW_SETTING)==0 ) {
		printf("Read backup_bluetooth_hw_setting header failed!\n");	
		bluetooth_compress_hw_setting = 1;
	}
	else 
	{
		//printf("===,the start char from bluetooth_hw_setting is %c%c%c%c%c%c\n",
		//bluetooth_hw_setting_start[0], bluetooth_hw_setting_start[1], bluetooth_hw_setting_start[2], 
		//bluetooth_hw_setting_start[3], bluetooth_hw_setting_start[4], bluetooth_hw_setting_start[5]);
		
		if (!memcmp(bluetooth_hw_setting_start, BACKUP_BLUETOOTH_HW_SETTING_HEADER_TAG, TAG_LEN))
		{
			bluetooth_compress_hw_setting = 0;
		}
		else
		{
			bluetooth_compress_hw_setting = 1;
		}
	}

#ifdef COMPRESS_MIB_SETTING
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;
		
	if(bluetooth_compress_hw_setting){
		

		flash_read_withtype((char *)&compHeader, BACKUP_BLUETOOTH_HW_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T),BACKUP_BLUETOOTH_HW_SETTING);
		COMP_TRACE(stderr,"\r\n sig=%s __[%s-%u]",compHeader.signature,__FILE__,__LINE__);
		if(memcmp(compHeader.signature, COMP_BB_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
		{
			zipRate = WORD_SWAP(compHeader.compRate);
			compLen = DWORD_SWAP(compHeader.compLen);

			if ( (compLen > 0) && (compLen <= BLUETOOTH_HW_SETTING_SECTOR_LEN) ) 
			{
				compFile=malloc(compLen+sizeof(COMPRESS_MIB_HEADER_T));
				if(compFile==NULL)
				{
					COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
					return 0;
				}
				expFile=malloc(zipRate*compLen);
				if(expFile==NULL)
				{
					COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
					free(compFile);
					return 0;
				}

				flash_read_withtype ((char *)compFile, BACKUP_BLUETOOTH_HW_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T),BACKUP_BLUETOOTH_HW_SETTING);

				expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);

				// copy the mib header from expFile
				memcpy((char *)&blueToothHwHeader, expFile, sizeof(blueToothHwHeader));
				blueToothHwHeader.len=HEADER_SWAP(blueToothHwHeader.len);//unsigned short length

				//check mib ver and tag
				if ( sscanf((char *)&blueToothHwHeader.signature[TAG_LEN], "%02d", &ver) != 1)
					ver = -1;
				
				if ( memcmp(blueToothHwHeader.signature, BACKUP_BLUETOOTH_HW_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
					(ver != BLUETOOTH_HW_SETTING_VER) // version not equal
					) { 					
					printf("Invalid default setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",
					blueToothHwHeader.signature[0], blueToothHwHeader.signature[1], ver, blueToothHwHeader.len);
					if(compFile != NULL)
						free(compFile);
					if(expFile != NULL)
						free(expFile);
					return NULL;
				}
	
			} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}
#ifdef MIB_TLV
	mib_table_entry_T *pmib_tl = NULL;
	unsigned char *bluetoothHwMibData;
	unsigned int tlv_content_len = blueToothHwHeader.len - 1; // last one is checksum

	bluetoothHwMibData = malloc(sizeof(BLUETOOTH_HW_SETTING_T)+1); // 1: checksum

	if(bluetoothHwMibData != NULL)
		memset(bluetoothHwMibData, 0x00, sizeof(BLUETOOTH_HW_SETTING_T)+1);
	
	pmib_tl = mib_get_table(BLUETOOTH_HW_SETTING);
	unsigned int tlv_checksum = 0;

	if(expFile != NULL)
		tlv_checksum = CHECKSUM_OK(expFile+sizeof(PARAM_HEADER_T), blueToothHwHeader.len);

//mib_display_tlv_content(DEFAULT_SETTING, expFile+sizeof(PARAM_HEADER_T), dsHeader.len);
		
	if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(PARAM_HEADER_T), (void*)bluetoothHwMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
	{
		sprintf((char *)&blueToothHwHeader.signature[TAG_LEN], "%02d", BLUETOOTH_HW_SETTING_VER);
		blueToothHwHeader.len = sizeof(BLUETOOTH_HW_SETTING_T)+1;
		bluetoothHwMibData[blueToothHwHeader.len-1]  = CHECKSUM(bluetoothHwMibData, sizeof(BLUETOOTH_HW_SETTING_T));

		if(expFile!= NULL)
			free(expFile);

		expFile = malloc(sizeof(PARAM_HEADER_T)+blueToothHwHeader.len);
		memcpy(expFile, &blueToothHwHeader, sizeof(PARAM_HEADER_T));
		memcpy(expFile+sizeof(PARAM_HEADER_T), bluetoothHwMibData, blueToothHwHeader.len);

	}	
	else
	{
		COMP_TRACE(stderr,"\r\n ERR!Invalid checksum[%u] or mib_tlv_init() fail! __[%s-%u]",tlv_checksum,__FILE__,__LINE__);
	}
		
	if(bluetoothHwMibData != NULL)
		free(bluetoothHwMibData);
		
#endif // #ifdef MIB_TLV		
	}
	else
	{
		return NULL;
	}
  }
  else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
{
	if ( flash_read_withtype ((char *)&blueToothHwHeader, BACKUP_BLUETOOTH_HW_SETTING_OFFSET, sizeof(blueToothHwHeader),BACKUP_BLUETOOTH_HW_SETTING)==0 ) {
		//printf("Read blueToothHwHeader setting header failed!\n");
		return NULL;
		}
	blueToothHwHeader.len=WORD_SWAP(blueToothHwHeader.len);
	}
	if ( sscanf((char *)&blueToothHwHeader.signature[TAG_LEN], "%02d", &ver) != 1)
		ver = -1;


	if ( memcmp(blueToothHwHeader.signature, BACKUP_BLUETOOTH_HW_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
		(ver != BLUETOOTH_HW_SETTING_VER) || // version not equal 
		(blueToothHwHeader.len < (sizeof(BLUETOOTH_HW_SETTING_T)+1)) ) { // length is less 
		printf("Invalid bluetooth hw setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",	blueToothHwHeader.signature[0], blueToothHwHeader.signature[1], ver, blueToothHwHeader.len);
		printf("Expect [sig=%s, ver=%d, len=%d]!\n", BACKUP_BLUETOOTH_HW_SETTING_HEADER_TAG, BLUETOOTH_HW_SETTING_VER, sizeof(BLUETOOTH_HW_SETTING_T)+1);
		return NULL;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
	buff = apmib_shm_calloc(1, blueToothHwHeader.len, BACKUP_BLUETOOTH_HW_SHM_KEY, &created);
#else
	buff = calloc(1, blueToothHwHeader.len);
#endif	
	if ( buff == 0 ) {
//		printf("Allocate buffer failed!\n");
		return NULL;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
    if (created) {
        return buff;
    }
#endif
#ifdef COMPRESS_MIB_SETTING

	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
		memcpy(buff, expFile+sizeof(PARAM_HEADER_T), blueToothHwHeader.len);
		free(expFile);
		free(compFile);
	}
	else // not compress mib data, copy mibdata from flash
#endif
	if ( flash_read_withtype(buff, BACKUP_BLUETOOTH_HW_SETTING_OFFSET+sizeof(blueToothHwHeader), blueToothHwHeader.len,BACKUP_BLUETOOTH_HW_SETTING)==0 ) {
		printf("Read bluetooth hw setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, BACKUP_BLUETOOTH_HW_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}
	if ( !CHECKSUM_OK((unsigned char *)buff, blueToothHwHeader.len) ) {
		printf("Invalid checksum of bluetooth hw setting! \n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, BACKUP_BLUETOOTH_HW_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}
	return buff;
	//printf("Bluetooth apmib init ok! \n");
}

#endif
#endif

#ifdef CUSTOMER_HW_SETTING_SUPPORT
////////////////////////////////////////////////////////////////////////////////
char *apmib_customerHwconf(void)
{
	int ver;
	char *buff;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	int created;
#endif
#ifdef COMPRESS_MIB_SETTING
		int zipRate=0;
		unsigned char *compFile=NULL, *expFile=NULL;
		unsigned int expandLen=0, compLen=0;
		COMPRESS_MIB_HEADER_T compHeader;

		flash_read((char *)&compHeader, CUSTOMER_HW_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T));
		//COMP_TRACE(stderr,"\r\n sig=%s __[%s-%u]",compHeader.signature,__FILE__,__LINE__);
		if(memcmp(compHeader.signature, COMP_US_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
		{
			zipRate = WORD_SWAP(compHeader.compRate);
			compLen = DWORD_SWAP(compHeader.compLen);

			if ( (compLen > 0) && (compLen <= CUSTOMER_HW_SETTING_SECTOR_LEN) ) 
			{
				compFile=malloc(compLen+sizeof(COMPRESS_MIB_HEADER_T));
				if(compFile==NULL)
				{
					COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
					return 0;
				}
				expFile=malloc(zipRate*compLen);
				if(expFile==NULL)
				{
					COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
					free(compFile);
					return 0;
				}

				flash_read((char *)compFile, CUSTOMER_HW_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T));

				expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);
			//	printf("expandLen read len: %d\n", expandLen);

				// copy the mib header from expFile
				memcpy((char *)&customerHwHeader, expFile, sizeof(customerHwHeader));
				customerHwHeader.len=HEADER_SWAP(customerHwHeader.len);

				//check mib ver and tag
				if ( sscanf((char *)&customerHwHeader.signature[TAG_LEN], "%02d", &ver) != 1)
					ver = -1;
				
				if ( memcmp(customerHwHeader.signature, CUSTOMER_HW_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
					(ver != CUSTOMER_HW_SETTING_VER) // version not equal
					) { 
					
					printf("Invalid default setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",
						customerHwHeader.signature[0], customerHwHeader.signature[1], ver, customerHwHeader.len);
					if(compFile != NULL)
						free(compFile);
					if(expFile != NULL)
						free(expFile);
					return NULL;
				}
	
			} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}
#ifdef MIB_TLV
		mib_table_entry_T *pmib_tl = NULL;
		unsigned char *customerHwMibData;
		unsigned int tlv_content_len = customerHwHeader.len - 1; // last one is checksum

		customerHwMibData = malloc(sizeof(CUSTOMER_HW_SETTING_T)+1); // 1: checksum

		if(customerHwMibData != NULL)
			memset(customerHwMibData, 0x00, sizeof(CUSTOMER_HW_SETTING_T)+1);
	
		pmib_tl = mib_get_table(CUSTOMER_HW_SETTING);
		unsigned int tlv_checksum = 0;

		if(expFile != NULL)
			tlv_checksum = CHECKSUM_OK(expFile+sizeof(PARAM_HEADER_T), customerHwHeader.len);

//mib_display_tlv_content(DEFAULT_SETTING, expFile+sizeof(PARAM_HEADER_T), dsHeader.len);
		
		if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(PARAM_HEADER_T), (void*)customerHwMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
		{
			sprintf((char *)&customerHwHeader.signature[TAG_LEN], "%02d", CUSTOMER_HW_SETTING_VER);
			customerHwHeader.len = sizeof(CUSTOMER_HW_SETTING_T)+1;
			customerHwMibData[customerHwHeader.len-1]  = CHECKSUM(customerHwMibData, sizeof(CUSTOMER_HW_SETTING_T));

			if(expFile!= NULL)
				free(expFile);

			expFile = malloc(sizeof(PARAM_HEADER_T)+customerHwHeader.len);
			memcpy(expFile, &customerHwHeader, sizeof(PARAM_HEADER_T));
			memcpy(expFile+sizeof(PARAM_HEADER_T), customerHwMibData, customerHwHeader.len);
			
//mib_display_data_content(DEFAULT_SETTING, expFile+sizeof(PARAM_HEADER_T), dsHeader.len-1);
		}	
		else
		{
			COMP_TRACE(stderr,"\r\n ERR!Invalid checksum[%u] or mib_tlv_init() fail! __[%s-%u]",tlv_checksum,__FILE__,__LINE__);
		}
		
		if(customerHwMibData != NULL)
			free(customerHwMibData);
		
#endif // #ifdef MIB_TLV		
	}
	else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
	{
		if ( flash_read((char *)&customerHwHeader, CUSTOMER_HW_SETTING_OFFSET, sizeof(customerHwHeader))==0 ) {
	//		printf("Read default setting header failed!\n");
			return NULL;
		}
		customerHwHeader.len=HEADER_SWAP(customerHwHeader.len);
	}
	if ( sscanf((char *)&customerHwHeader.signature[TAG_LEN], "%02d", &ver) != 1)
		ver = -1;

//fprintf(stderr,"\r\n (sizeof(CUSTOMER_HW_SETTING_T)=%u ",(sizeof(CUSTOMER_HW_SETTING_T)));
//printf("customer setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",	customerHwHeader.signature[0], customerHwHeader.signature[1], ver, customerHwHeader.len);

	if ( memcmp(customerHwHeader.signature, CUSTOMER_HW_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
		(ver != CUSTOMER_HW_SETTING_VER) || // version not equal 
		(customerHwHeader.len < (sizeof(CUSTOMER_HW_SETTING_T)+1)) ) { // length is less 
		printf("Invalid customer setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",	customerHwHeader.signature[0], customerHwHeader.signature[1], ver, customerHwHeader.len);
		printf("Expect [sig=%s, ver=%d, len=%d]!\n", CUSTOMER_HW_SETTING_HEADER_TAG, CUSTOMER_HW_SETTING_VER, sizeof(CUSTOMER_HW_SETTING_T)+1);
		return NULL;
	}
//	if (ver > DEFAULT_SETTING_VER)
//		printf("Default setting version is greater than current [f:%d, c:%d]!\n", ver, DEFAULT_SETTING_VER);

#if CONFIG_APMIB_SHARED_MEMORY == 1
	buff = apmib_shm_calloc(1, customerHwHeader.len, CUSTOMER_HW_SHM_KEY, &created);
#else
	buff = calloc(1, customerHwHeader.len);
#endif	
	if ( buff == 0 ) {
//		printf("Allocate buffer failed!\n");
		return NULL;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
    if (created) {
        return buff;
    }
#endif
#ifdef COMPRESS_MIB_SETTING

	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
		memcpy(buff, expFile+sizeof(PARAM_HEADER_T), customerHwHeader.len);
		free(expFile);
		free(compFile);
	}
	else // not compress mib data, copy mibdata from flash
#endif
	if ( flash_read(buff, CUSTOMER_HW_SETTING_OFFSET+sizeof(customerHwHeader), customerHwHeader.len)==0 ) {
		printf("Read customer hw setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, CUSTOMER_HW_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}
	if ( !CHECKSUM_OK((unsigned char *)buff, customerHwHeader.len) ) {
		printf("Invalid checksum of customer hw setting! \n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, CUSTOMER_HW_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}
	return buff;
}
#ifdef BACKUP_HARDWARE_SETTING_SUPPORT
char *apmib_backupcustomerHwconf(void)
{
	int ver;
	char *buff;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	int created;
#endif
#ifdef COMPRESS_MIB_SETTING
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;

	flash_read_withtype((char *)&compHeader, BACKUP_CUSTOMER_HW_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T),BACKUP_CUSTOMER_HW_SETTING);
	
	if(memcmp(compHeader.signature, COMP_BU_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether backup compress mib data
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);

		if ( (compLen > 0) && (compLen <= CUSTOMER_HW_SETTING_SECTOR_LEN) ) 
		{
			compFile=malloc(compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}
			expFile=malloc(zipRate*compLen);
			if(expFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
				free(compFile);
				return 0;
			}

			flash_read_withtype((char *)compFile, BACKUP_CUSTOMER_HW_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T),BACKUP_CUSTOMER_HW_SETTING);
			expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);

			// copy the mib header from expFile
			memcpy((char *)&customerHwHeader, expFile, sizeof(customerHwHeader));
			customerHwHeader.len=HEADER_SWAP(customerHwHeader.len);

			//check mib ver and tag
			if ( sscanf((char *)&customerHwHeader.signature[TAG_LEN], "%02d", &ver) != 1)
				ver = -1;
				
			if ( memcmp(customerHwHeader.signature, CUSTOMER_HW_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
				(ver != CUSTOMER_HW_SETTING_VER) // version not equal
				) { 
					
				printf("Invalid default setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",
				customerHwHeader.signature[0], customerHwHeader.signature[1], ver, customerHwHeader.len);
				if(compFile != NULL)
					free(compFile);
				if(expFile != NULL)
					free(expFile);
				return NULL;
			}
		} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}
#ifdef MIB_TLV
	mib_table_entry_T *pmib_tl = NULL;
	unsigned char *customerHwMibData;
	unsigned int tlv_content_len = customerHwHeader.len - 1; // last one is checksum
	customerHwMibData = malloc(sizeof(CUSTOMER_HW_SETTING_T)+1); // 1: checksum

	if(customerHwMibData != NULL)
		memset(customerHwMibData, 0x00, sizeof(CUSTOMER_HW_SETTING_T)+1);
	
	pmib_tl = mib_get_table(CUSTOMER_HW_SETTING);
	unsigned int tlv_checksum = 0;

	if(expFile != NULL)
		tlv_checksum = CHECKSUM_OK(expFile+sizeof(PARAM_HEADER_T), customerHwHeader.len);
		
	if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(PARAM_HEADER_T), (void*)customerHwMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
	{
		sprintf((char *)&customerHwHeader.signature[TAG_LEN], "%02d", CUSTOMER_HW_SETTING_VER);
		customerHwHeader.len = sizeof(CUSTOMER_HW_SETTING_T)+1;
		customerHwMibData[customerHwHeader.len-1]  = CHECKSUM(customerHwMibData, sizeof(CUSTOMER_HW_SETTING_T));

		if(expFile!= NULL)
			free(expFile);

		expFile = malloc(sizeof(PARAM_HEADER_T)+customerHwHeader.len);
		memcpy(expFile, &customerHwHeader, sizeof(PARAM_HEADER_T));
		memcpy(expFile+sizeof(PARAM_HEADER_T), customerHwMibData, customerHwHeader.len);
	}	
	else
	{
		COMP_TRACE(stderr,"\r\n ERR!Invalid checksum[%u] or mib_tlv_init() fail! __[%s-%u]",tlv_checksum,__FILE__,__LINE__);
	}
		
	if(customerHwMibData != NULL)
		free(customerHwMibData);
		
#endif // #ifdef MIB_TLV		
	}
	else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
	{
		printf("[%s:%d] flash read at %x\n", __FUNCTION__, __LINE__,BACKUP_CUSTOMER_HW_SETTING_OFFSET);
		if ( flash_read_withtype((char *)&customerHwHeader, BACKUP_CUSTOMER_HW_SETTING_OFFSET, sizeof(customerHwHeader),BACKUP_CUSTOMER_HW_SETTING)==0 ) {
			printf("Read customer hw setting header failed!\n");
			return NULL;
		}
		customerHwHeader.len=HEADER_SWAP(customerHwHeader.len);

		if ( sscanf((char *)&customerHwHeader.signature[TAG_LEN], "%02d", &ver) != 1)
			ver = -1;

		//fprintf(stderr,"\r\n (sizeof(CUSTOMER_HW_SETTING_T)=%u ",(sizeof(CUSTOMER_HW_SETTING_T)));
		//printf("customer setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",	customerHwHeader.signature[0], customerHwHeader.signature[1], ver, customerHwHeader.len);

		if ( memcmp(customerHwHeader.signature,CUSTOMER_HW_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
		(ver != CUSTOMER_HW_SETTING_VER) || // version not equal 
		(customerHwHeader.len < (sizeof(CUSTOMER_HW_SETTING_T)+1)) ) { // length is less 
			printf("Invalid customer setting signature or version number [sig=%c%c, ver=%d, len=%x]!\n",customerHwHeader.signature[0], customerHwHeader.signature[1], ver, customerHwHeader.len);
			printf("Expect [sig=%s, ver=%d, len=%x]!\n", CUSTOMER_HW_SETTING_HEADER_TAG, CUSTOMER_HW_SETTING_VER, sizeof(CUSTOMER_HW_SETTING_T)+1);
			return NULL;
		}
	}
#if CONFIG_APMIB_SHARED_MEMORY == 1
	buff = apmib_shm_calloc(1, customerHwHeader.len, BACKUP_CUSTOMER_HW_SHM_KEY, &created);
#else
	buff = calloc(1, customerHwHeader.len);
#endif	
	if ( buff == 0 ) {
//		printf("Allocate buffer failed!\n");
		return NULL;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
	if (created) {
		return buff;
	}
#endif
#ifdef COMPRESS_MIB_SETTING

	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
		memcpy(buff, expFile+sizeof(PARAM_HEADER_T), customerHwHeader.len);
		free(expFile);
		free(compFile);
	}
	else // not compress mib data, copy mibdata from flash
#endif
	if ( flash_read_withtype(buff, BACKUP_CUSTOMER_HW_SETTING_OFFSET+sizeof(customerHwHeader), customerHwHeader.len,BACKUP_CUSTOMER_HW_SETTING)==0 ) {
		printf("Read customer hw setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, BACKUP_CUSTOMER_HW_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}
	if ( !CHECKSUM_OK((unsigned char *)buff, customerHwHeader.len) ) {
		printf("Invalid checksum of customer hw setting! \n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, BACKUP_CUSTOMER_HW_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}
	return buff;
}

#endif
#endif

#ifdef RF_DPK_SETTING_SUPPORT
////////////////////////////////////////////////////////////////////////////////
char *apmib_rfDPKconf(void)
{
	int ver;
	char *buff;

#if CONFIG_APMIB_SHARED_MEMORY == 1
	int created;

	/* check shared memory */
#endif /* CONFIG_APMIB_SHARED_MEMORY == 1 */

	memset(&rfDpkHeader, 0, sizeof(rfDpkHeader));

	/* Read RF DPK setting */
	if ( flash_read((char *)&rfDpkHeader, RF_DPK_SETTING_OFFSET, sizeof(rfDpkHeader))==0 ) {
		//printf("Read RF DPK setting header failed!\n");
		return NULL;
	}
	//printf("flash read DPK data length=%x!\n", rfDpkHeader.len);
	rfDpkHeader.len=WORD_SWAP(rfDpkHeader.len);

	if ( sscanf((char *)&rfDpkHeader.signature[TAG_LEN], "%02d", &ver) != 1)
		ver = -1;

	/* check signature */
	if ( memcmp(rfDpkHeader.signature, RF_DPK_SETTING_HEADER_TAG, TAG_LEN)) { /* invalid signature */
		printf("Invalid RF DPK setting signature [sig=%c%c]!\n", rfDpkHeader.signature[0],rfDpkHeader.signature[1]);
		return NULL;
	}

	/* check version & length */
	if ((ver != RF_DPK_SETTING_VER) || /* version not equal to current */
		(rfDpkHeader.len < (sizeof(RF_DPK_SETTING_T)+1))) { /* length is less than current */
		printf("Invalid RF DPK setting version number or data length[ver=%d, len=%d size=%d]!\n", ver, rfDpkHeader.len,sizeof(RF_DPK_SETTING_T));
		return NULL;
	}

	#if 0
	if (ver > RF_DPK_SETTING_VER)
		printf("RF DPK setting version is greater than current [f:%d, c:%d]!\n", ver, RF_DPK_SETTING_VER);
	#endif

#if CONFIG_APMIB_SHARED_MEMORY == 1
	buff = apmib_shm_calloc(1, rfDpkHeader.len, RF_DPK_SHM_KEY, &created);
#else
	buff = calloc(1, rfDpkHeader.len);
#endif
	if ( buff == 0 ) {
		printf("rfDpk Allocate buffer failed!\n");
		return NULL;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
	if (created) {
		//printf("No need to read hw setting!\n");     
		return buff;
	}
#endif

     {
         DPK_DEBUG("Load DPK flash MIB\n");
        if ( flash_read(buff, RF_DPK_SETTING_OFFSET+sizeof(rfDpkHeader), rfDpkHeader.len)==0 ) {    
    		printf("Read RF DPK setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
    		apmib_shm_free(buff, RF_DPK_SHM_KEY);
#else
    		free(buff);
#endif		
    		return NULL;
    	}
    	if ( !CHECKSUM_OK((unsigned char *)buff, rfDpkHeader.len) ) {
    		printf("Invalid checksum of RF DPK setting!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
    		apmib_shm_free(buff, RF_DPK_SHM_KEY);
#else
    		free(buff);
#endif		
    		return NULL;
    	}
    }
	return buff;
}
#endif /* RF_DPK_SETTING_SUPPORT */
#ifdef DEFSETTING_AUTO_UPDATE

////////////////////////////////////////////////////////////////////////////////
char *apmib_last_dsconf(void)
{
	int ver;
	char *buff;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	int created;
#endif

#ifdef COMPRESS_MIB_SETTING
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;

	flash_read((char *)&compHeader, DEFAULT_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T));

	if(memcmp(compHeader.signature, COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);

		if ( (compLen > 0) && (compLen <= DEFAULT_SETTING_SECTOR_LEN) ) 
		{
			compFile=malloc(compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}
			expFile=malloc(zipRate*compLen);
			if(expFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
				free(compFile);
				return 0;
			}


			flash_read((char *)compFile, DEFAULT_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T));

			expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);

			// copy the mib header from expFile
			memcpy((char *)&lastDsHeader, expFile, sizeof(lastDsHeader));
			lastDsHeader.len=HEADER_SWAP(lastDsHeader.len);
	

			//check mib ver and tag
			if ( sscanf((char *)&lastDsHeader.signature[TAG_LEN], "%02d", &ver) != 1)
				ver = -1;
			
			if ( memcmp(lastDsHeader.signature, CURRENT_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
				(ver != CURRENT_SETTING_VER) // version not equal to current
				) { 
				
				printf("Invalid default setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",
					lastDsHeader.signature[0], lastDsHeader.signature[1], ver, lastDsHeader.len);
				if(compFile != NULL)
					free(compFile);
				if(expFile != NULL)
					free(expFile);
				return NULL;
			}
	
		} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}
#ifdef MIB_TLV
		mib_table_entry_T *pmib_tl = NULL;
		unsigned char *defMibData;
		unsigned int tlv_content_len = lastDsHeader.len - 1; // last one is checksum

		defMibData = malloc(sizeof(APMIB_T)+1); // 1: checksum

		if(defMibData != NULL)
			memset(defMibData, 0x00, sizeof(APMIB_T)+1);
	
		pmib_tl = mib_get_table(DEFAULT_SETTING);
		unsigned int tlv_checksum = 0;

		if(expFile != NULL)
			tlv_checksum = CHECKSUM_OK(expFile+sizeof(PARAM_HEADER_T), lastDsHeader.len);

//mib_display_tlv_content(DEFAULT_SETTING, expFile+sizeof(PARAM_HEADER_T), dsHeader.len);
		
		if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(PARAM_HEADER_T), (void*)defMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
		{
			sprintf((char *)&lastDsHeader.signature[TAG_LEN], "%02d", CURRENT_SETTING_VER);
			lastDsHeader.len = sizeof(APMIB_T)+1;
			defMibData[lastDsHeader.len-1]	= CHECKSUM(defMibData, sizeof(APMIB_T));

			if(expFile!= NULL)
				free(expFile);

			expFile = malloc(sizeof(PARAM_HEADER_T)+lastDsHeader.len);
			memcpy(expFile, &lastDsHeader, sizeof(PARAM_HEADER_T));
			memcpy(expFile+sizeof(PARAM_HEADER_T), defMibData, lastDsHeader.len);
			
//mib_display_data_content(DEFAULT_SETTING, expFile+sizeof(PARAM_HEADER_T), dsHeader.len-1);
		}	
		else
		{
			COMP_TRACE(stderr,"\r\n ERR!Invalid checksum[%u] or mib_tlv_init() fail! __[%s-%u]",tlv_checksum,__FILE__,__LINE__);
		}
		
		if(defMibData != NULL)
			free(defMibData);
		
#endif // #ifdef MIB_TLV		
	}
	else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
	// Read default s/w mib
	{

		if ( flash_read((char *)&lastDsHeader, DEFAULT_SETTING_OFFSET, sizeof(lastDsHeader))==0 ) {
	//		printf("Read default setting header failed!\n");
			return NULL;
		}
		lastDsHeader.len=HEADER_SWAP(lastDsHeader.len);
	}

	if ( sscanf((char *)&lastDsHeader.signature[TAG_LEN], "%02d", &ver) != 1)
		ver = -1;

	if (

	 memcmp(lastDsHeader.signature, CURRENT_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
		(ver != CURRENT_SETTING_VER) || // version not equal to current
		(lastDsHeader.len < (sizeof(APMIB_T)+1)) ) { // length is less than current
		printf("Invalid default setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n", lastDsHeader.signature[0], lastDsHeader.signature[1], ver, lastDsHeader.len);

		printf("Expect [sig=%s, ver=%d, len=%d]!\n", CURRENT_SETTING_HEADER_TAG, CURRENT_SETTING_VER, sizeof(APMIB_T)+1);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
//	if (ver > DEFAULT_SETTING_VER)
//		printf("Default setting version is greater than current [f:%d, c:%d]!\n", ver, DEFAULT_SETTING_VER);

#if CONFIG_APMIB_SHARED_MEMORY == 1
	buff = apmib_shm_calloc(1, lastDsHeader.len, LAST_DSCONF_SHM_KEY, &created);
#else
	buff = calloc(1, lastDsHeader.len);
#endif	
	if ( buff == 0 ) {
//		printf("Allocate buffer failed!\n");
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
	if (created) {
		//printf("No need to read default setting!\n");
#ifdef COMPRESS_MIB_SETTING
	if(compFile != NULL)
		free(compFile);
	if(expFile != NULL)
		free(expFile);
#endif        
		return buff;
	}
#endif

#ifdef COMPRESS_MIB_SETTING

	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
		memcpy(buff, expFile+sizeof(PARAM_HEADER_T), lastDsHeader.len);
		free(expFile);
		free(compFile);
	}
	else // not compress mib data, copy mibdata from flash
#endif
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);

	if ( flash_read(buff, DEFAULT_SETTING_OFFSET+sizeof(lastDsHeader), lastDsHeader.len)==0 ) {
//		printf("Read default setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, LAST_DSCONF_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}
	if ( !CHECKSUM_OK((unsigned char *)buff, lastDsHeader.len) ) {
//		printf("Invalid checksum of current setting!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, LAST_DSCONF_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}	
	return buff;
}

int apmib_dsAutoUpdate(void)
{
	unsigned int i=0,j=0,wlanIdx=0,vwlanIdx=0,offset=0,size=0;

	if ((pMibLastDef=(APMIB_Tp)apmib_last_dsconf()) == NULL){
			printf("get last dsconf fail!\n");
			return -1;
		}
	//fprintf(stderr," __[%s-%u] pMibLastDef=%p\n",__FILE__,__LINE__,pMibLastDef); 
	//fprintf(stderr," __[%s-%u] pMib=%p\n",__FILE__,__LINE__,pMib); 
	//fprintf(stderr," __[%s-%u] pMibDef=%p\n",__FILE__,__LINE__,pMibDef); 

	for(i=0;mib_table[i].id;i++){
		
		//fprintf(stderr," __[%s-%u] i=%d id=%d  mib_table address=%p\n",__FILE__,__LINE__,i,mib_table[i].id,&mib_table[i]); 
		offset=mib_table[i].offset;
		size=mib_table[i].total_size;
		//fprintf(stderr," __[%s-%u] name=%s offset=%x size=%d\n",__FILE__,__LINE__,mib_table[i].name,offset,size); 
		if(memcmp(((void *)pMibDef)+offset,((void *)pMibLastDef)+offset,size)!=0){//new version this mib have changed
			//fprintf(stderr," __[%s-%u] name=%s offset=%x size=%d\n",__FILE__,__LINE__,mib_table[i].name,offset,size); 
			if(memcmp(((void *)pMib)+offset,((void *)pMibLastDef)+offset,size)==0){//this mib customer have not changed
				//fprintf(stderr," __[%s-%u] name=%s offset=%x size=%d\n",__FILE__,__LINE__,mib_table[i].name,offset,size); 
				memcpy(((void *)pMib)+offset,((void *)pMibDef)+offset,size);
				printf("update mib %s",mib_table[i].name);
			}
		}
		//fprintf(stderr," __[%s-%u]\n",__FILE__,__LINE__); 
	}
	//fprintf(stderr," __[%s-%u]\n",__FILE__,__LINE__); 

	for(wlanIdx=0;wlanIdx<NUM_WLAN_INTERFACE;wlanIdx++){
		for(vwlanIdx=0;vwlanIdx<NUM_VWLAN_INTERFACE+1;vwlanIdx++){
			for(i=0;mib_wlan_table[i].id;i++){
				offset=mib_wlan_table[i].offset;
				size=mib_wlan_table[i].total_size;
				if(memcmp(
					(void *)&(pMibDef->wlan[wlanIdx][vwlanIdx])+offset,
					(void *)&(pMibLastDef->wlan[wlanIdx][vwlanIdx])+offset,
					size)!=0
					)
				{//new version this mib have changed
					if(memcmp(
						(void *)&(pMib->wlan[wlanIdx][vwlanIdx])+offset,
						(void *)&(pMibLastDef->wlan[wlanIdx][vwlanIdx])+offset,
						size)==0
						)
					{//this mib customer have not changed
						memcpy(
							(void *)&(pMib->wlan[wlanIdx][vwlanIdx])+offset,
							(void *)&(pMibDef->wlan[wlanIdx][vwlanIdx])+offset,
							size);
						
						printf("update wlan mib %s",mib_wlan_table[i].name);
					}
				}
			}
		}
	}
	//fprintf(stderr," __[%s-%u]\n",__FILE__,__LINE__); 
	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////
char *apmib_dsconf(void)//return data_structure APMIB_T with checksum
{
	int ver;
	char *buff;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	int created;
#endif

#ifdef COMPRESS_MIB_SETTING
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;
#endif

#ifdef COMPRESS_MIB_SETTING

//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
#ifndef RTL_DEF_SETTING_IN_FW
	flash_read((char *)&compHeader, DEFAULT_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T));
#else
	if(sizeof(def_setting_data)<sizeof(COMPRESS_MIB_HEADER_T))
	{
		fprintf(stderr,"\r\n ERR:def_setting_data error! __[%s-%u]",__FILE__,__LINE__);
		return 0;
	}
	memcpy((char *)&compHeader,def_setting_data,sizeof(COMPRESS_MIB_HEADER_T));
#endif
//fprintf(stderr,"\r\n __[%s-%u] compHeader.signature=%s",__FILE__,__LINE__,compHeader.signature);	
#ifndef RTL_DEF_SETTING_IN_FW
	if(memcmp(compHeader.signature, COMP_DS_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
#else
	if(memcmp(compHeader.signature, COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
#endif
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);
#ifndef RTL_DEF_SETTING_IN_FW
		if ( (compLen > 0) && (compLen <= DEFAULT_SETTING_SECTOR_LEN) ) 
#else
		if ( (compLen > 0) && (compLen <= CURRENT_SETTING_SECTOR_LEN) ) 
#endif
		{
			compFile=malloc(compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}
			expFile=malloc(zipRate*compLen);
			if(expFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
				free(compFile);
				return 0;
			}
#ifndef RTL_DEF_SETTING_IN_FW

			flash_read((char *)compFile, DEFAULT_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T));
#else
			if(sizeof(def_setting_data)<compLen+sizeof(COMPRESS_MIB_HEADER_T))
			{
				fprintf(stderr,"\r\n ERR:def_setting_data error! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}
			memcpy((char *)compFile,def_setting_data,compLen+sizeof(COMPRESS_MIB_HEADER_T));
#endif

			expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);
			//printf("expandLen read len: %d\n", expandLen);

			// copy the mib header from expFile
			memcpy((char *)&dsHeader, expFile, sizeof(dsHeader));
			dsHeader.len=HEADER_SWAP(dsHeader.len);
	
#ifndef RTL_DEF_SETTING_IN_FW
			//check mib ver and tag
			if ( sscanf((char *)&dsHeader.signature[TAG_LEN], "%02d", &ver) != 1)
				ver = -1;
			
			if ( memcmp(dsHeader.signature, DEFAULT_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
				(ver != DEFAULT_SETTING_VER) // version not equal to current
				) { 
				
				printf("Invalid default setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",
					dsHeader.signature[0], dsHeader.signature[1], ver, dsHeader.len);
				if(compFile != NULL)
					free(compFile);
				if(expFile != NULL)
					free(expFile);
				return NULL;
			}
#endif		
		} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}
#ifdef MIB_TLV
		mib_table_entry_T *pmib_tl = NULL;
		unsigned char *defMibData;
		unsigned int tlv_content_len = dsHeader.len - 1; // last one is checksum

		defMibData = malloc(sizeof(APMIB_T)+1); // 1: checksum

		if(defMibData != NULL)
			memset(defMibData, 0x00, sizeof(APMIB_T)+1);
	
		pmib_tl = mib_get_table(DEFAULT_SETTING);
		unsigned int tlv_checksum = 0;

		if(expFile != NULL)
			tlv_checksum = CHECKSUM_OK(expFile+sizeof(PARAM_HEADER_T), dsHeader.len);

//mib_display_tlv_content(DEFAULT_SETTING, expFile+sizeof(PARAM_HEADER_T), dsHeader.len);
		
		if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(PARAM_HEADER_T), (void*)defMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
		{
			sprintf((char *)&dsHeader.signature[TAG_LEN], "%02d", DEFAULT_SETTING_VER);
			dsHeader.len = sizeof(APMIB_T)+1;
			defMibData[dsHeader.len-1]  = CHECKSUM(defMibData, sizeof(APMIB_T));

			if(expFile!= NULL)
				free(expFile);

			expFile = malloc(sizeof(PARAM_HEADER_T)+dsHeader.len);
			memcpy(expFile, &dsHeader, sizeof(PARAM_HEADER_T));
			memcpy(expFile+sizeof(PARAM_HEADER_T), defMibData, dsHeader.len);
			
//mib_display_data_content(DEFAULT_SETTING, expFile+sizeof(PARAM_HEADER_T), dsHeader.len-1);
		}	
		else
		{
			COMP_TRACE(stderr,"\r\n ERR!Invalid checksum[%u] or mib_tlv_init() fail! __[%s-%u]",tlv_checksum,__FILE__,__LINE__);
			if(compFile != NULL)
			{
				free(compFile);
			}
			if(expFile != NULL)
			{
				free(expFile);
			}
			if(defMibData != NULL)
			{
				free(defMibData);
			}
			return NULL;
		}
		
		if(defMibData != NULL)
			free(defMibData);
		
#endif // #ifdef MIB_TLV		
	}
	else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
	// Read default s/w mib
	{
#ifndef RTL_DEF_SETTING_IN_FW
		if ( flash_read((char *)&dsHeader, DEFAULT_SETTING_OFFSET, sizeof(dsHeader))==0 ) {
	//		printf("Read default setting header failed!\n");
			return NULL;
		}
#else
		{
			if(sizeof(def_setting_data)<sizeof(dsHeader))
			{
				fprintf(stderr,"\r\n ERR:def_setting_data error! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}	
			memcpy((char *)&dsHeader,def_setting_data,sizeof(dsHeader));
		}

#endif
		dsHeader.len=HEADER_SWAP(dsHeader.len);
	}

	if ( sscanf((char *)&dsHeader.signature[TAG_LEN], "%02d", &ver) != 1)
		ver = -1;

//fprintf(stderr,"\r\n (sizeof(APMIB_T)=%u ",(sizeof(APMIB_T)));
//printf("default setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",	dsHeader.signature[0], dsHeader.signature[1], ver, dsHeader.len);
	if (
#ifndef RTL_DEF_SETTING_IN_FW
	 memcmp(dsHeader.signature, DEFAULT_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
		(ver != DEFAULT_SETTING_VER) || // version not equal to current
#else
	memcmp(dsHeader.signature, CURRENT_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
		(ver != CURRENT_SETTING_VER) || // version not equal to current
#endif
		(dsHeader.len < (sizeof(APMIB_T)+1)) ) { // length is less than current
		printf("Invalid default setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",	dsHeader.signature[0], dsHeader.signature[1], ver, dsHeader.len);
#ifndef RTL_DEF_SETTING_IN_FW
		printf("Expect [sig=%s, ver=%d, len=%d]!\n", DEFAULT_SETTING_HEADER_TAG, DEFAULT_SETTING_VER, sizeof(APMIB_T)+1);
#else
		printf("Expect [sig=%s, ver=%d, len=%d]!\n", CURRENT_SETTING_HEADER_TAG, DEFAULT_SETTING_VER, sizeof(APMIB_T)+1);
#endif
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
//	if (ver > DEFAULT_SETTING_VER)
//		printf("Default setting version is greater than current [f:%d, c:%d]!\n", ver, DEFAULT_SETTING_VER);

#if CONFIG_APMIB_SHARED_MEMORY == 1
	buff = apmib_shm_calloc(1, dsHeader.len, DSCONF_SHM_KEY, &created);
#else
	buff = calloc(1, dsHeader.len);
#endif	
	if ( buff == 0 ) {
//		printf("Allocate buffer failed!\n");
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
    if (created) {
        //printf("No need to read default setting!\n");
#ifdef COMPRESS_MIB_SETTING
	if(compFile != NULL)
		free(compFile);
	if(expFile != NULL)
		free(expFile);
#endif        
        return buff;
    }
#endif

#ifdef COMPRESS_MIB_SETTING

	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
		memcpy(buff, expFile+sizeof(PARAM_HEADER_T), dsHeader.len);
		free(expFile);
		free(compFile);
	}
	else // not compress mib data, copy mibdata from flash
#endif
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
#ifndef RTL_DEF_SETTING_IN_FW

	if ( flash_read(buff, DEFAULT_SETTING_OFFSET+sizeof(dsHeader), dsHeader.len)==0 ) {
//		printf("Read default setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, DSCONF_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}
#else
	{
		if(sizeof(def_setting_data)<dsHeader.len+sizeof(dsHeader))
		{
			fprintf(stderr,"\r\n ERR:def_setting_data error! __[%s-%u]",__FILE__,__LINE__);
			
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(buff, DSCONF_SHM_KEY);
#else
			free(buff);
#endif		
			return 0;
		}
		memcpy(buff,def_setting_data+sizeof(dsHeader),dsHeader.len);
	}
#endif
	if ( !CHECKSUM_OK((unsigned char *)buff, dsHeader.len) ) {
//		printf("Invalid checksum of current setting!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, DSCONF_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}	
	return buff;
}


#ifdef DOWNLOAD_UPLOAD_DEFAULT_SETTING
int copy_ds_from_flash_to_file(void)
{
//#ifdef COMPRESS_MIB_SETTING
	int zipRate = 0;
	unsigned char *compFile = NULL;
	unsigned int compLen = 0;
	COMPRESS_MIB_HEADER_T compHeader;

	flash_read((char *)&compHeader, DEFAULT_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T));


	if(memcmp(compHeader.signature, COMP_DS_SIGNATURE, COMP_SIGNATURE_LEN) == 0)//check whether compress mib data
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);

		if((compLen > 0) && (compLen <= DEFAULT_SETTING_SECTOR_LEN))
		{
			compFile = malloc(compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile == NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}

			flash_read((char *)compFile, DEFAULT_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T));
		}
	}
//#endif

	int fh = open("/var/defconfig.dat", O_RDWR|O_CREAT|O_TRUNC);
	if(fh == -1)
	{
		free(compFile);
		fprintf(stderr,"Create defconfig.dat error!%s:%d\n",__FUNCTION__,__LINE__);
		return 0;
	}
	if(write(fh, compFile, compLen+sizeof(COMPRESS_MIB_HEADER_T)) != compLen+sizeof(COMPRESS_MIB_HEADER_T))
	{
		close(fh);
		free(compFile);
		fprintf(stderr,"Write config file error!%s %d\n",__FUNCTION__,__LINE__);
		return 0;
	}
	close(fh);
	sync();
	
	free(compFile);
	return 1;
}
#endif

#ifdef BACKUP_DEFAULT_SETTING_SUPPORT
char *apmib_backupdsconf(void)//return data_structure APMIB_T with checksum
{
	PARAM_HEADER_T backupdsHeader;
	int ver;
	char *buff;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	int created;
#endif

#ifdef COMPRESS_MIB_SETTING
	int zipRate = 0;
	unsigned char *compFile = NULL, *expFile = NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;

#ifndef RTL_DEF_SETTING_IN_FW
	flash_read_withtype((char *)&compHeader, BACKUP_DEFAULT_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T),BACKUP_DEFAULT_SETTING);
#else
	if(sizeof(def_setting_data) < sizeof(COMPRESS_MIB_HEADER_T))
	{
		fprintf(stderr,"\r\n ERR:def_setting_data error! [File:%s Line:%u]",__FILE__,__LINE__);
		return NULL;
	}
	memcpy((char *)&compHeader, def_setting_data, sizeof(COMPRESS_MIB_HEADER_T));
#endif

#ifndef RTL_DEF_SETTING_IN_FW
	if(memcmp(compHeader.signature, COMP_BD_SIGNATURE, COMP_SIGNATURE_LEN) == 0) //check whether compress mib data
#else
	if(memcmp(compHeader.signature, COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN) == 0) //check whether compress mib data
#endif
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);
#ifndef RTL_DEF_SETTING_IN_FW
		if((compLen > 0) && (compLen <= BACKUP_DEFAULT_SETTING_SECTOR_LEN))
#else
		if((compLen > 0) && (compLen <= CURRENT_SETTING_SECTOR_LEN))
#endif
		{
			compFile = calloc(1, compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile == NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! [File:%s Line:%u]",__FILE__,__LINE__);
				return NULL;
			}
			expFile = calloc(1, zipRate*compLen);
			if(expFile == NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:expFile malloc fail! [File:%s Line:%u]",__FILE__,__LINE__);			
				free(compFile);
				return NULL;
			}
			
#ifndef RTL_DEF_SETTING_IN_FW
			flash_read_withtype((char *)compFile, BACKUP_DEFAULT_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T),BACKUP_DEFAULT_SETTING);
			//printf(" flash_read_BACKUPDS successfully :srcOffset=%x[File:%s Line:%d]\n",BACKUP_DEFAULT_SETTING_OFFSET,__FILE__,__LINE__);
#else
			if(sizeof(def_setting_data) < compLen+sizeof(COMPRESS_MIB_HEADER_T))
			{
				fprintf(stderr,"\r\n ERR:def_setting_data error! [File:%s Line:%u]",__FILE__,__LINE__);
				return NULL;
			}
			memcpy((char *)compFile, def_setting_data, compLen+sizeof(COMPRESS_MIB_HEADER_T));
#endif
			expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);
			free(compFile);
			
			//copy the mib header from expFile
			memcpy((char *)&backupdsHeader, expFile, sizeof(backupdsHeader));
			backupdsHeader.len = HEADER_SWAP(backupdsHeader.len);
	
#ifndef RTL_DEF_SETTING_IN_FW
			//check version and tag
			if(sscanf((char *)&backupdsHeader.signature[TAG_LEN], "%02d", &ver) != 1)
			{
				ver = -1;
			}
			if((memcmp(backupdsHeader.signature, DEFAULT_SETTING_HEADER_TAG, TAG_LEN) != 0) || // invalid signature
			   (ver != DEFAULT_SETTING_VER) // version not equal to DEFAULT_SETTING_VER
			  ) 
			{
				printf("Invalid default setting signature or version number![File:%s Line:%d]\n", __FILE__, __LINE__);
				printf("[sig = %c%c]\n", backupdsHeader.signature[0], backupdsHeader.signature[1]);
				printf("DEFAULT_SETTING_HEADER_TAG:%s\n",DEFAULT_SETTING_HEADER_TAG);

				printf("[ver = %d]\n", ver);
				printf("DEFAULT_SETTING_VER:%s\n",DEFAULT_SETTING_VER);
				
				if(expFile != NULL)
				{
					free(expFile);
				}
				return NULL;
			}
#endif		
		} 
		else 
		{
			COMP_TRACE(stderr, "\r\n ERR:compLen = %u [File:%s Line:%u]", compLen, __FILE__, __LINE__);
			return NULL;
		}
#ifdef MIB_TLV
		mib_table_entry_T *pmib_tl = NULL;
		pmib_tl = mib_get_table(BACKUP_DEFAULT_SETTING);

		unsigned int is_checksum_ok = 0;
		unsigned int tlv_content_len = backupdsHeader.len - 1; // last one is checksum

		unsigned char *defMibData;
		defMibData = malloc(sizeof(APMIB_T)+1); // 1: checksum

		if(defMibData != NULL)
		{
			memset(defMibData, 0x00, sizeof(APMIB_T)+1);
		}
		else
		{
			printf("malloc defMibData error! File:%s  Line:%d\n",__FILE__,__LINE__);
			if(expFile != NULL)
			{
				free(expFile);
			}
			return NULL;
		}

		if(expFile != NULL)
		{
			is_checksum_ok = CHECKSUM_OK(expFile+sizeof(PARAM_HEADER_T), backupdsHeader.len);
		}
		if(is_checksum_ok == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(PARAM_HEADER_T), (void*)defMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
		{
			sprintf((char *)&backupdsHeader.signature[TAG_LEN], "%02d", DEFAULT_SETTING_VER);
			backupdsHeader.len = sizeof(APMIB_T)+1;//change backupcsHeader.len from 'tlv+checksum' to 'APMIB_T+1'
			defMibData[backupdsHeader.len-1] = CHECKSUM(defMibData, sizeof(APMIB_T));//write checksum to the last byte of defMibData

			free(expFile);
			expFile = malloc(sizeof(PARAM_HEADER_T)+backupdsHeader.len);//malloc expFile: sizeof(APMIB_T) + 1+sizeof(PARAM_HEADER_T)
			if(expFile != NULL)
			{
				memcpy(expFile, &backupdsHeader, sizeof(PARAM_HEADER_T));
				memcpy(expFile+sizeof(PARAM_HEADER_T), defMibData, backupdsHeader.len);
			}
			else
			{
				printf("malloc expFile error! File:%s  Line:%d\n",__FILE__,__LINE__);
				if(defMibData != NULL)
				{
					free(defMibData);
				}
				return NULL;
			}
		}
		else
		{
			printf("mib_tlv_init fail!! File:%s Line:%d\n",__FILE__,__LINE__);
			if(expFile != NULL)
			{
				free(expFile);
			}
			if(defMibData != NULL)
			{
				free(defMibData);
			}
			return NULL;
		}
		
		if(defMibData != NULL)
		{
			free(defMibData);
		}
#endif //MIB_TLV
	}
	else //not compress mib-data, get mib header from flash
#endif //COMPRESS_MIB_SETTING
	{// Read default s/w mib
#ifndef RTL_DEF_SETTING_IN_FW
		if(flash_read_withtype((char *)&backupdsHeader, BACKUP_DEFAULT_SETTING_OFFSET, sizeof(backupdsHeader),BACKUP_DEFAULT_SETTING) == 0)
		{
			printf("read backup default setting header failed! File:%s Line:%d\n",__FILE__,__LINE__);
			return NULL;
		}
#else
		{
			if(sizeof(def_setting_data) < sizeof(backupdsHeader))
			{
				fprintf(stderr,"\r\n ERR:def_setting_data error! [File:%s Line:%u]",__FILE__,__LINE__);
				return NULL;
			}	
			memcpy((char *)&backupdsHeader,def_setting_data,sizeof(backupdsHeader));
		}
#endif
		backupdsHeader.len = HEADER_SWAP(backupdsHeader.len);
	}

	if(sscanf((char *)&backupdsHeader.signature[TAG_LEN], "%02d", &ver) != 1)
	{
		ver = -1;
	}
	
	if(
#ifndef RTL_DEF_SETTING_IN_FW
	 	(memcmp(backupdsHeader.signature, DEFAULT_SETTING_HEADER_TAG, TAG_LEN) != 0) || // invalid signatur
		(ver != DEFAULT_SETTING_VER) || // version not equal to current
#else
		(memcmp(backupdsHeader.signature, CURRENT_SETTING_HEADER_TAG, TAG_LEN) != 0) || // invalid signatur
		(ver != CURRENT_SETTING_VER) || // version not equal to current
#endif
		(backupdsHeader.len < (sizeof(APMIB_T)+1))// length is less than current
	   ) 
	{
		printf("Invalid backup default setting signature or version number [File:%s Line:%d]!\n", __FILE__,__LINE__);
		printf("In flash: [sig=%c%c, ver=%d, len=%d]!\n", backupdsHeader.signature[0], backupdsHeader.signature[1], ver, backupdsHeader.len);
#ifndef RTL_DEF_SETTING_IN_FW
		printf("Expect  :[sig=%s, ver=%d, len=%d]!\n", DEFAULT_SETTING_HEADER_TAG, DEFAULT_SETTING_VER, sizeof(APMIB_T)+1);
#else
		printf("Expect  :[sig=%s, ver=%d, len=%d]!\n", CURRENT_SETTING_HEADER_TAG, DEFAULT_SETTING_VER, sizeof(APMIB_T)+1);
#endif
#ifdef COMPRESS_MIB_SETTING
		if(expFile != NULL)
		{
			free(expFile);
		}
#endif
		return NULL;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
	//created==1:has been created and assigned by expFile; created==0: has not been created and assigned by expFile
	buff = apmib_shm_calloc(1, backupdsHeader.len, BACKUP_DSCONF_SHM_KEY, &created);
#else
	buff = calloc(1, backupdsHeader.len);
#endif	
	if(buff == NULL)
	{
		printf("allocate buffer failed! File:%s  Line:%d\n",__FILE__,__LINE__);
#ifdef COMPRESS_MIB_SETTING
		if(expFile != NULL)
		{
			free(expFile);
		}
#endif
		return NULL;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
	if(created == 1) //No need to read default setting
	{
#ifdef COMPRESS_MIB_SETTING
		if(expFile != NULL)
		{
			free(expFile);
		}
#endif        
		return buff;
	}
#endif

#ifdef COMPRESS_MIB_SETTING

	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
		memcpy(buff, expFile+sizeof(PARAM_HEADER_T), backupdsHeader.len);
		free(expFile);
	}
	else // not compress mib data, copy mibdata from flash
#endif
#ifndef RTL_DEF_SETTING_IN_FW
	if(flash_read_withtype(buff, BACKUP_DEFAULT_SETTING_OFFSET+sizeof(backupdsHeader), backupdsHeader.len,BACKUP_DEFAULT_SETTING) == 0) 
	{
		printf("Read default setting failed! [File:%s Line:%s]\n",__FILE__,__LINE__);
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, BACKUP_DSCONF_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}
#else
	{
		if(sizeof(def_setting_data) < backupdsHeader.len+sizeof(backupdsHeader))
		{
			fprintf(stderr,"\r\n ERR:def_setting_data error! [File:%s  Line:%u]\n",__FILE__,__LINE__);
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(buff, BACKUP_DSCONF_SHM_KEY);
#else
			free(buff);
#endif		
			return NULL;
		}
		memcpy(buff,def_setting_data+sizeof(backupdsHeader),backupdsHeader.len);
	}
#endif
	if(CHECKSUM_OK((unsigned char *)buff, backupdsHeader.len) != 1)
	{
		printf("Invalid checksum of backup default setting! [File:%s  Line:%u]\n",__FILE__,__LINE__);
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, BACKUP_DSCONF_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}	
	return buff;
}
#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef COMPRESS_MIB_SETTING
int flash_read_raw_mib(unsigned char **compFile)
{
	int zipRate=0, i = 0;
	unsigned int compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;
	void *pDst = (void*)(CURRENT_SETTING_OFFSET);

	if (compFile && *compFile)
		free(*compFile);
	else
		return 0;

	flash_read(&compHeader, pDst, sizeof(COMPRESS_MIB_HEADER_T));
	if(memcmp(compHeader.signature, COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);
		if ( (compLen > 0) && (compLen <= CURRENT_SETTING_SECTOR_LEN) ) {
			*compFile=calloc(1,compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(*compFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}
			flash_read(*compFile, pDst, compLen+sizeof(COMPRESS_MIB_HEADER_T));		
		}
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}
	}

	return 1;
}

int flash_write_raw_mib(unsigned char **compFile)
{
	COMPRESS_MIB_HEADER_T *compHeader = (COMPRESS_MIB_HEADER_T *)(*compFile);
	void *pDst = (void*)(CURRENT_SETTING_OFFSET);
	int ret = 1;

	if ( flash_write(*compFile, pDst, compHeader->compLen+sizeof(COMPRESS_MIB_HEADER_T))==0 )
	{
		printf("Write flash compress setting failed![%s-%u]\n",__FILE__,__LINE__);			
		ret = 0;
	}

	if (compFile && *compFile)
		free(*compFile);

	return ret;
}
#else
int flash_read_raw_mib(unsigned char **compFile)
{
	return 0;
}

int flash_write_raw_mib(unsigned char **compFile)
{
	return 0;
}
#endif /* #ifdef COMPRESS_MIB_SETTING */


char *apmib_csconf(void)
{
	int ver=0;
	char *buff=NULL;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	int created;
#endif

#ifdef COMPRESS_MIB_SETTING
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader={0};
#endif

#ifdef COMPRESS_MIB_SETTING

//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
	flash_read((char *)&compHeader, CURRENT_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T));
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);	
	if(memcmp(compHeader.signature, COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);
		if ( (compLen > 0) && (compLen <= CURRENT_SETTING_SECTOR_LEN) ) {
			compFile=calloc(1,compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}
			expFile=calloc(1,zipRate*compLen);
			if(expFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
				free(compFile);
				return 0;
			}

			flash_read((char *)compFile, CURRENT_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T));

			expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);
			//printf("expandLen read len: %d\n", expandLen);

			// copy the mib header from expFile
			memcpy((char *)&csHeader, expFile, sizeof(csHeader));
			csHeader.len=HEADER_SWAP(csHeader.len);

			// check version and tag
			if ( sscanf((char *)&csHeader.signature[TAG_LEN], "%02d", &ver) != 1)
				ver = -1;

			if ( memcmp(csHeader.signature, CURRENT_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
				(ver != CURRENT_SETTING_VER)  // version not equal to current
			       ) {
			       printf("Invalid current setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",
				   	csHeader.signature[0], csHeader.signature[1], ver, csHeader.len);
				if(compFile != NULL)
					free(compFile);
				if(expFile != NULL)
					free(expFile);
				return NULL;
			}	
		} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}
#ifdef MIB_TLV
		mib_table_entry_T *pmib_tl = NULL;
		unsigned char *curMibData;
		unsigned int tlv_content_len = csHeader.len - 1; // last one is checksum
		
		curMibData = malloc(sizeof(APMIB_T)+1); // 1: checksum

		if(curMibData != NULL)
			memset(curMibData, 0x00, sizeof(APMIB_T)+1);

		pmib_tl = mib_get_table(CURRENT_SETTING);
		unsigned int tlv_checksum = 0;

		if(expFile != NULL)
			tlv_checksum = CHECKSUM_OK(expFile+sizeof(PARAM_HEADER_T), csHeader.len);

//mib_display_tlv_content(CURRENT_SETTING, expFile+sizeof(PARAM_HEADER_T), csHeader.len);
 		if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(PARAM_HEADER_T), (void*)curMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
		{
			sprintf((char *)&csHeader.signature[TAG_LEN], "%02d", CURRENT_SETTING_VER);
			csHeader.len = sizeof(APMIB_T)+1;
			curMibData[csHeader.len-1]  = CHECKSUM(curMibData, csHeader.len-1);

			if(expFile!= NULL)
				free(expFile);

			expFile = malloc(sizeof(PARAM_HEADER_T)+csHeader.len);
			memcpy(expFile, &csHeader, sizeof(PARAM_HEADER_T));
			memcpy(expFile+sizeof(PARAM_HEADER_T), curMibData, csHeader.len);

//mib_display_data_content(CURRENT_SETTING, expFile+sizeof(PARAM_HEADER_T), csHeader.len-1);
		}else
		{
			printf("mib_tlv_init fail!!\n");
			if(compFile != NULL)
			{
				free(compFile);
			}
			if(expFile != NULL)
			{
				free(expFile);
			}
			if(curMibData != NULL)
			{
				free(curMibData);
			}
			return NULL;
		}
		//printf("%d [sig=%c%c, ver=%d, len=%d]!\n",__LINE__,csHeader.signature[0], csHeader.signature[1], ver, csHeader.len);

		if(curMibData != NULL)
			free(curMibData);
		
#endif // #ifdef MIB_TLV		
	}
	else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
	// Read current s/w mib
	{
		if ( flash_read((char *)&csHeader, CURRENT_SETTING_OFFSET, sizeof(csHeader))==0 ) {
	//		printf("Read current setting header failed!\n");
			return NULL;
		}
		csHeader.len=HEADER_SWAP(csHeader.len);
	}

//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);

	if ( sscanf((char *)&csHeader.signature[TAG_LEN], "%02d", &ver) != 1)
		ver = -1;

	if ( memcmp(csHeader.signature, CURRENT_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
		(ver != CURRENT_SETTING_VER) || // version not equal to current
			(csHeader.len < (sizeof(APMIB_T)+1)) ) { // length is less than current
//		printf("Invalid current setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",
//			csHeader.signature[0], csHeader.signature[1], ver, csHeader.len);
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}

//	if (ver > CURRENT_SETTING_VER)
//		printf("Current setting version is greater than current [f:%d, c:%d]!\n", ver, CURRENT_SETTING_VER);

#if CONFIG_APMIB_SHARED_MEMORY == 1
	buff = apmib_shm_calloc(1, csHeader.len, CSCONF_SHM_KEY, &created);
#else
	buff = calloc(1, csHeader.len);
#endif	
	if ( buff == 0 ) {
//		printf("Allocate buffer failed!\n");
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
    if (created) {
        //printf("No need to read current setting!\n");
#ifdef COMPRESS_MIB_SETTING
	if(compFile != NULL)
		free(compFile);
	if(expFile != NULL)
		free(expFile);
#endif        
        return buff;
    }
#endif

#ifdef COMPRESS_MIB_SETTING

	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
		memcpy(buff, expFile+sizeof(PARAM_HEADER_T), csHeader.len);
		free(expFile);
		free(compFile);
	}
	else // not compress mib data, copy mibdata from flash
#endif
	if ( flash_read(buff, CURRENT_SETTING_OFFSET+sizeof(csHeader), csHeader.len)==0 ) {
//		printf("Read current setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, CSCONF_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}

	if ( !CHECKSUM_OK((unsigned char *)buff, csHeader.len) ) {
//		printf("Invalid checksum of current setting!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, CSCONF_SHM_KEY);
#else	
		free(buff);
#endif
		return NULL;
	}
	return buff;
}

#ifdef BACKUP_CURRENT_SETTING_SUPPORT
char * apmib_backupcsconf(void)
{
	PARAM_HEADER_T backupcsHeader;

	int ver=0;
	char * buff=NULL;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	int created;
#endif

#ifdef COMPRESS_MIB_SETTING
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader={0};


	flash_read_withtype((char *)&compHeader, BACKUP_CURRENT_SETTING_OFFSET,sizeof(COMPRESS_MIB_HEADER_T),BACKUP_CURRENT_SETTING);
	//printf(" BACKUP_CURRENT_SETTING_OFFSET= %x , %s %u \n", BACKUP_CURRENT_SETTING_OFFSET,__FILE__,__LINE__);
	compLen = DWORD_SWAP(compHeader.compLen);
	if(memcmp(compHeader.signature, COMP_BC_SIGNATURE, COMP_SIGNATURE_LEN) == 0) //check whether compress mib data
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);
		if((compLen > 0) && (compLen <= BACKUP_CURRENT_SETTING_SECTOR_LEN))
		{
			compFile = calloc(1, compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile == NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
				return NULL;
			}
			expFile = calloc(1, zipRate*compLen);
			if(expFile == NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:expFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
				free(compFile);
				return NULL;
			}

			flash_read_withtype((char *)compFile, BACKUP_CURRENT_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T),BACKUP_CURRENT_SETTING);
			//decompress compressed data to expFile, the return value is the length of decompressed data
			expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);
			free(compFile);

			//copy the mib header from expFile
			memcpy((char *)&backupcsHeader, expFile, sizeof(backupcsHeader));
			backupcsHeader.len = HEADER_SWAP(backupcsHeader.len);

			//check version and tag
			if(sscanf((char *)&backupcsHeader.signature[TAG_LEN], "%02d", &ver) != 1)
			{
				ver = -1;
			}

			if((memcmp(backupcsHeader.signature, CURRENT_SETTING_HEADER_TAG, TAG_LEN) != 0) || // invalid signature
			   (ver != CURRENT_SETTING_VER)  //version not equal to CURRENT_SETTING_VER
			  ) 
			{

				printf("Invalid backup current setting signature or version number! [File:%s Line:%d]\n",__FILE__,__LINE__);
				printf("backupcsHeader.signature  :%c%c\n",backupcsHeader.signature[0], backupcsHeader.signature[1]);
				printf("CURRENT_SETTING_HEADER_TAG:%s\n",CURRENT_SETTING_HEADER_TAG);
				
				printf("version number     :%d\n", ver);
				printf("CURRENT_SETTING_VER:%d\n", CURRENT_SETTING_VER);

			
				//printf("Invalid backup current setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",
				   	//   backupcsHeader.signature[0], backupcsHeader.signature[1], ver, backupcsHeader.len);
				if(expFile != NULL)
				{
					free(expFile);
				}
				return NULL;
			}	
		} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return NULL;
		}
#ifdef MIB_TLV
		mib_table_entry_T * pmib_tl = NULL;
		pmib_tl = mib_get_table(BACKUP_CURRENT_SETTING);

		unsigned int is_checksum_ok = 0;
		unsigned int tlv_content_len = backupcsHeader.len - 1; //last byte is checksum

		unsigned char * curMibData;
		curMibData = malloc(sizeof(APMIB_T) + 1); //1: checksum

		if(curMibData != NULL)
		{
			memset(curMibData, 0x00, sizeof(APMIB_T)+1);
		}
		else
		{
			printf("malloc curMibData error! File:%s  Line:%d\n",__FILE__,__LINE__);
			if(expFile != NULL)
			{
				free(expFile);
			}
			return NULL;
		}

		if(expFile != NULL)
		{
			is_checksum_ok = CHECKSUM_OK(expFile+sizeof(PARAM_HEADER_T), backupcsHeader.len);
		}
		//According to pmib_tl, get value from expFile to curMibData(it's length is sizeof(APMIB_T)+1).
 		if(is_checksum_ok == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(PARAM_HEADER_T), (void*)curMibData, tlv_content_len) == 1)
		{
			sprintf((char *)&backupcsHeader.signature[TAG_LEN], "%02d", CURRENT_SETTING_VER);
			backupcsHeader.len = sizeof(APMIB_T) + 1; //change backupcsHeader.len from 'tlv+checksum' to 'APMIB_T+1'
			curMibData[backupcsHeader.len-1] = CHECKSUM(curMibData, backupcsHeader.len-1);//write checksum to the last byte of curMibData

			free(expFile);
			expFile = malloc(sizeof(PARAM_HEADER_T)+backupcsHeader.len);//malloc expFile: sizeof(APMIB_T) + 1+sizeof(PARAM_HEADER_T)
			if(expFile != NULL)
			{
				memcpy(expFile, &backupcsHeader, sizeof(PARAM_HEADER_T));
				memcpy(expFile+sizeof(PARAM_HEADER_T), curMibData, backupcsHeader.len);
			}
			else
			{
				printf("malloc expFile error! File:%s  Line:%d\n",__FILE__,__LINE__);
				if(curMibData != NULL)
				{
					free(curMibData);
				}
				return NULL;
			}
		}
		else
		{
			printf("mib_tlv_init fail!! File:%s  Line:%d\n",__FILE__,__LINE__);
			if(expFile != NULL)
			{
				free(expFile);
			}
			if(curMibData != NULL)
			{
				free(curMibData);
			}
			return NULL;
		}
		if(curMibData != NULL)
		{
			free(curMibData);
		}
#endif
	}
	else //not compress mib-data, get mib header from flash
#endif //COMPRESS_MIB_SETTING,Read current s/w mib
	{
		if(flash_read_withtype((char *)&backupcsHeader, BACKUP_CURRENT_SETTING_OFFSET, sizeof(backupcsHeader),BACKUP_CURRENT_SETTING) == 0) 
		{
			printf("read backup current setting header failed! File:%s  Line:%d\n",__FILE__,__LINE__);
			return NULL;
		}
		backupcsHeader.len=HEADER_SWAP(backupcsHeader.len);
	}

	if(sscanf((char *)&backupcsHeader.signature[TAG_LEN], "%02d", &ver) != 1)
	{
		ver = -1;
	}

	if((memcmp(backupcsHeader.signature, CURRENT_SETTING_HEADER_TAG, TAG_LEN) != 0) || //invalid signatur
	   (ver != CURRENT_SETTING_VER) ||      //version not equal to current
	   (backupcsHeader.len < (sizeof(APMIB_T)+1)) //length is less than current
	  ) 
	{ 
		printf("Invalid backup current setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",
		backupcsHeader.signature[0], backupcsHeader.signature[1], ver, backupcsHeader.len);
#ifdef COMPRESS_MIB_SETTING
		if(expFile != NULL)
		{
			free(expFile);
		}
#endif
		return NULL;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
	//created==1:has been created and assigned by expFile; created==0: has not been created and assigned by expFile
	buff = apmib_shm_calloc(1, backupcsHeader.len, BACKUP_CSCONF_SHM_KEY, &created);
#else
	buff = calloc(1, backupcsHeader.len);
#endif

	if(buff == NULL)
	{
		printf("allocate buffer failed! File:%s  Line:%d\n",__FILE__,__LINE__);
#ifdef COMPRESS_MIB_SETTING
		if(expFile != NULL)
		{
			free(expFile);
		}
#endif
		return NULL;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
    if(created == 1)  //No need to read current setting!
    {
#ifdef COMPRESS_MIB_SETTING
		if(expFile != NULL)
		{
			free(expFile);
		}
#endif        
        return buff;
    }
#endif

#ifdef COMPRESS_MIB_SETTING
	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
		memcpy(buff, expFile+sizeof(PARAM_HEADER_T), backupcsHeader.len);
		free(expFile);
	}
	else //not compress mib data, copy mibdata from flash
#endif
	if(flash_read_withtype(buff, BACKUP_CURRENT_SETTING_OFFSET+sizeof(backupcsHeader), backupcsHeader.len,BACKUP_CURRENT_SETTING) == 0)
	{
		printf("Read current setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, BACKUP_CSCONF_SHM_KEY);
#else
		free(buff);
#endif		
		return NULL;
	}

	if(CHECKSUM_OK((unsigned char *)buff, backupcsHeader.len) != 1)
	{
		printf("Invalid checksum of current setting!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(buff, BACKUP_CSCONF_SHM_KEY);
#else	
		free(buff);
#endif
		return NULL;
	}
	return buff;
}
#endif

int apmib_init_HW(void)
{
	char *buff;

	if ((buff=apmib_hwconf()) == NULL)
		return 0;
	pHwSetting = (HW_SETTING_Tp)buff;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
	if((buff=apmib_bluetoothHwconf())==NULL)
		return 0;
	pBlueToothHwSetting=(BLUETOOTH_HW_SETTING_Tp)buff;
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
	if((buff=apmib_customerHwconf())==NULL)
		return 0;
	pCustomerHwSetting=(CUSTOMER_HW_SETTING_Tp)buff;
#endif
#ifdef RF_DPK_SETTING_SUPPORT
	if((buff=apmib_rfDPKconf())==NULL)
		return 0;
	pRfDpkSetting=(RF_DPK_SETTING_Tp)buff;
#endif
	return 1;
}

#ifdef AP_CONTROLER_SUPPORT
int apmib_virtual_flash_read(char *buf, int offset, int len)
{
	if(virtual_flash==NULL && apmib_virtual_flash_malloc()<0)
	{
		fprintf(stderr,"\r\n virtual_flash is null__[%s-%u]",__FILE__,__LINE__);
		return 0;
	}
	if(offset<HW_SETTING_OFFSET||offset>=WEB_PAGE_OFFSET || offset+len>WEB_PAGE_OFFSET)
	{
		fprintf(stderr,"\r\n apmib_virtual_flash_read fail! oversize __[%s-%u]",__FILE__,__LINE__);
		return 0;
	}
	//apmib_sem_lock();
	memcpy(buf,virtual_flash+(offset-HW_SETTING_OFFSET),len);
	//fprintf(stderr,"\r\n virtual_flash=%p offset=0x%x len=%d__[%s-%u]",virtual_flash,offset,len,__FILE__,__LINE__);
	//apmib_sem_unlock();
	return 1;
}
int apmib_virtual_flash_write(char *buf, int offset, int len)
{
	if(virtual_flash==NULL && apmib_virtual_flash_malloc()<0)
	{
		fprintf(stderr,"\r\n virtual_flash is null__[%s-%u]",__FILE__,__LINE__);
		return 0;
	}
	if(offset<HW_SETTING_OFFSET||offset>=WEB_PAGE_OFFSET || offset+len>WEB_PAGE_OFFSET)
	{
		fprintf(stderr,"\r\n apmib_virtual_flash_write fail! oversize __[%s-%u]",__FILE__,__LINE__);
		return 0;
	}
//	fprintf(stderr,"\r\n virtual_flash=%p offset=0x%x len=%d__[%s-%u]",virtual_flash,offset,len,__FILE__,__LINE__);
	//apmib_sem_lock();
	memcpy(virtual_flash+(offset-HW_SETTING_OFFSET),buf,len);
	//apmib_sem_unlock();
	return 1;
}

int apmib_virtual_flash_malloc()
{
	int create=0;
	//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
	
	if(virtual_flash==NULL)	
	{
		//apmib_sem_lock();
		virtual_flash=apmib_shm_calloc(1,WEB_PAGE_OFFSET-HW_SETTING_OFFSET,CONF_SHM_VFLASH_KEY,&create);		
		//apmib_sem_unlock();
		//fprintf(stderr,"\r\n virtual_flash=%p __[%s-%u]",virtual_flash,__FILE__,__LINE__);
		if(virtual_flash==NULL)
		{
			fprintf(stderr,"\r\n apmib_virtual_flash_malloc fail! apmib_shm_calloc __[%s-%u]",__FILE__,__LINE__);
			return -1;
		}
	}
	
	
	//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
	return 0;
}
#endif
////////////////////////////////////////////////////////////////////////////////
int apmib_init(void)
{
#ifndef MIB_TLV
	int i, j, k;
#endif
	char *buff;

#if CONFIG_APMIB_SHARED_MEMORY == 1	
    apmib_sem_lock();
#endif

	if ( pMib != NULL )	// has been initialized
#if CONFIG_APMIB_SHARED_MEMORY == 1
		goto linkchain;
#else
		return 1;
#endif

	if ((buff=apmib_hwconf()) == NULL) {
#if CONFIG_APMIB_SHARED_MEMORY == 1	
        apmib_sem_unlock();
#endif
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
		return 0;
	}
	pHwSetting = (HW_SETTING_Tp)buff;

	if ((buff=apmib_dsconf()) == NULL) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pHwSetting);
#endif
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
		return 0;
	}
	pMibDef = (APMIB_Tp)buff;

	if ((buff=apmib_csconf()) == NULL) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pHwSetting);
		free(pMibDef);
#endif
		return 0;
	}
	pMib = (APMIB_Tp)buff;

	
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		if ((buff=apmib_customerHwconf()) == NULL) {
#if CONFIG_APMIB_SHARED_MEMORY == 1	
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_sem_unlock();
#else
		free(pHwSetting);
		free(pMibDef);
		free(pMib);
#endif

			return 0;
		}
		pCustomerHwSetting = (CUSTOMER_HW_SETTING_Tp)buff;
#endif

#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		if ((buff=apmib_bluetoothHwconf()) == NULL) {
#if CONFIG_APMIB_SHARED_MEMORY == 1	
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
#ifdef CUSTOMER_HW_SETTING_SUPPORT
			apmib_shm_free(pCustomerHwSetting,CUSTOMER_HW_SHM_KEY);
#endif
			apmib_sem_unlock();
#else
		free(pHwSetting);
		free(pMibDef);
		free(pMib);
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		free(pCustomerHwSetting);
#endif
#endif

			return 0;
		}
		pBlueToothHwSetting = (BLUETOOTH_HW_SETTING_Tp)buff;

#endif

#ifdef RF_DPK_SETTING_SUPPORT
	if ((buff=apmib_rfDPKconf()) == NULL) {
#if CONFIG_APMIB_SHARED_MEMORY == 1	
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
	#ifdef CUSTOMER_HW_SETTING_SUPPORT
		apmib_shm_free(pCustomerHwSetting, CUSTOMER_HW_SHM_KEY);
	#endif
    #ifdef BLUETOOTH_HW_SETTING_SUPPOR
        apmib_shm_free(pBlueToothHwSetting,BLUETOOTH_HW_SHM_KEY);
    #endif
		apmib_sem_unlock();
#else /* !CONFIG_APMIB_SHARED_MEMORY */
	free(pHwSetting);
	free(pMibDef);
	free(pMib);
#ifdef CUSTOMER_HW_SETTING_SUPPORT
    free(pCustomerHwSetting);
#endif
#ifdef BLUETOOTH_HW_SETTING_SUPPOR
    free(pBlueToothHwSetting);
#endif
#endif /* CONFIG_APMIB_SHARED_MEMORY */

		return 0;
	}
	pRfDpkSetting = (RF_DPK_SETTING_Tp)buff;
#endif /* RF_DPK_SETTING_SUPPORT */


#if CONFIG_APMIB_SHARED_MEMORY == 1
linkchain:
#endif
#ifndef MIB_TLV
	for (j=0; j<NUM_WLAN_INTERFACE; j++)
		for (k=0; k<(NUM_VWLAN_INTERFACE+1); k++) // wlan[j][0] is for root
	{
		// initialize MAC access control list
		if ( !init_linkchain(&wlanMacChain[j][k], sizeof(MACFILTER_T), MAX_WLAN_AC_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif			
			return 0;
		}
		
		for (i=0; i<pMib->wlan[j][k].acNum; i++) {
			if ( !add_linkchain(&wlanMacChain[j][k], (char *)&pMib->wlan[j][k].acAddrArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
				apmib_shm_free(pMib, CSCONF_SHM_KEY);
				apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
				apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
				apmib_sem_unlock();
#else
				free(pMib);
				free(pMibDef);
				free(pHwSetting);
#endif
				return 0;
			}
		}
		wlanMacChain[j][k].compareLen = sizeof(MACFILTER_T) - COMMENT_LEN;

		// initialize WDS list
		if ( !init_linkchain(&wdsChain[j][k], sizeof(WDS_T), MAX_WDS_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
		for (i=0; i<pMib->wlan[j][k].wdsNum; i++) {
			if ( !add_linkchain(&wdsChain[j][k], (char *)&pMib->wlan[j][k].wdsArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
				apmib_shm_free(pMib, CSCONF_SHM_KEY);
				apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
				apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
				apmib_sem_unlock();
#else
				free(pMib);
				free(pMibDef);
				free(pHwSetting);
#endif
				return 0;
			}
		}
		wdsChain[j][k].compareLen = sizeof(WDS_T) - COMMENT_LEN;
#if defined(WIFI_SIMPLE_CONFIG) && defined(CONFIG_APP_TR069)
		// initialize wsc ER table
		if ( !init_linkchain(&wscERChain[j][k], sizeof(WSC_ER_T), MAX_WSC_ER_NUM)) {
			printf("[%s %d]init_linkchain error !\n", __FUNCTION__,__LINE__);
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
		for (i=0; i<pMib->wlan[j][k].wscERNum; i++) {
			if ( !add_linkchain(&wscERChain[j][k], (char *)&pMib->wlan[j][k].wscERArray[i]) ) {
				printf("[%s %d]add_linkchain error !\n", __FUNCTION__,__LINE__);
#if CONFIG_APMIB_SHARED_MEMORY == 1
				apmib_shm_free(pMib, CSCONF_SHM_KEY);
				apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
				apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
				apmib_sem_unlock();
#else
				free(pMib);
				free(pMibDef);
				free(pHwSetting);
#endif
				return 0;
			}
		}
		wscERChain[j][k].compareLen = sizeof(WSC_ER_T); // comment ?
#endif		
		// initialize schedule table
		if ( !init_linkchain(&scheduleRuleChain[j][k], sizeof(SCHEDULE_T), MAX_SCHEDULE_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
		for (i=0; i<pMib->wlan[j][k].scheduleRuleNum; i++) {
			if ( !add_linkchain(&scheduleRuleChain[j][k], (char *)&pMib->wlan[j][k].scheduleRuleArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
				apmib_shm_free(pMib, CSCONF_SHM_KEY);
				apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
				apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
#else
				free(pMib);
				free(pMibDef);
				free(pHwSetting);
#endif
				return 0;
			}
		}
			scheduleRuleChain[j][k].compareLen = sizeof(SCHEDULE_T);
			
		
	}

#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)	// below code copy above ACL code
	// initialize MAC access control list
	if ( !init_linkchain(&meshAclChain, sizeof(MACFILTER_T), MAX_MESH_ACL_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif			
		return 0;
	}

	for (i=0; i<pMib->meshAclNum; i++) {
		if ( !add_linkchain(&meshAclChain, (char *)&pMib->meshAclAddrArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	meshAclChain.compareLen = sizeof(MACFILTER_T) - COMMENT_LEN;
#endif	// CONFIG_RTK_MESH && _MESH_ACL_ENABLE_

	




#ifdef HOME_GATEWAY
	// initialize port forwarding table
	if ( !init_linkchain(&portFwChain, sizeof(PORTFW_T), MAX_FILTER_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->portFwNum; i++) {
		if ( !add_linkchain(&portFwChain, (char *)&pMib->portFwArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	portFwChain.compareLen = sizeof(PORTFW_T) - COMMENT_LEN;

	// initialize ip-filter table
	if ( !init_linkchain(&ipFilterChain, sizeof(IPFILTER_T), MAX_FILTER_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->ipFilterNum; i++) {
		if ( !add_linkchain(&ipFilterChain, (char *)&pMib->ipFilterArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	ipFilterChain.compareLen = sizeof(IPFILTER_T) - COMMENT_LEN;

	// initialize port-filter table
	if ( !init_linkchain(&portFilterChain, sizeof(PORTFILTER_T), MAX_FILTER_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->portFilterNum; i++) {
		if ( !add_linkchain(&portFilterChain, (char *)&pMib->portFilterArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	portFilterChain.compareLen = sizeof(PORTFILTER_T) - COMMENT_LEN;

	// initialize mac-filter table
	if ( !init_linkchain(&macFilterChain, sizeof(MACFILTER_T), MAX_FILTER_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->macFilterNum; i++) {
		if ( !add_linkchain(&macFilterChain, (char *)&pMib->macFilterArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	macFilterChain.compareLen = sizeof(MACFILTER_T) - COMMENT_LEN;

	// initialize url-filter table
	if ( !init_linkchain(&urlFilterChain, sizeof(URLFILTER_T), MAX_URLFILTER_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->urlFilterNum; i++) {
		if ( !add_linkchain(&urlFilterChain, (char *)&pMib->urlFilterArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	urlFilterChain.compareLen = sizeof(URLFILTER_T);// - COMMENT_LEN;

	// initialize trigger-port table
	if ( !init_linkchain(&triggerPortChain, sizeof(TRIGGERPORT_T), MAX_FILTER_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->triggerPortNum; i++) {
		if ( !add_linkchain(&triggerPortChain, (char *)&pMib->triggerPortArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	triggerPortChain.compareLen = 5;	// length of trigger port range + proto type
#if defined(GW_QOS_ENGINE) && !defined(MULTI_WAN_QOS)
	// initialize QoS rules table
	if ( !init_linkchain(&qosChain, sizeof(QOS_T), MAX_QOS_RULE_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->qosRuleNum; i++) {
		if ( !add_linkchain(&qosChain, (char *)&pMib->qosRuleArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	qosChain.compareLen =  sizeof(QOS_T);
#endif

#if defined(QOS_BY_BANDWIDTH) && !defined(MULTI_WAN_QOS)
	// initialize QoS rules table
	if ( !init_linkchain(&qosChain, sizeof(IPQOS_T), MAX_QOS_RULE_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->qosRuleNum; i++) {
		if ( !add_linkchain(&qosChain, (char *)&pMib->qosRuleArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	qosChain.compareLen =  sizeof(IPQOS_T);
#endif

#ifdef CONFIG_RTL_AIRTIME
	// initialize airtime table
	if ( !init_linkchain(&airtimeChain, sizeof(AIRTIME_T), MAX_AIRTIME_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif			
		return 0;
	}
	
	for (i=0; i<pMib->airTimeNum; i++) {
		if ( !add_linkchain(&airtimeChain, (char *)&pMib->airTimeArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	airtimeChain.compareLen = sizeof(AIRTIME_T) - COMMENT_LEN;
	
	// initialize airtime2 table
	if ( !init_linkchain(&airtimeChain2, sizeof(AIRTIME_T), MAX_AIRTIME_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif			
		return 0;
	}

	for (i=0; i<pMib->airTimeNum2; i++) {
		if ( !add_linkchain(&airtimeChain2, (char *)&pMib->airTimeArray2[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	airtimeChain2.compareLen = sizeof(AIRTIME_T) - COMMENT_LEN;
#endif /* CONFIG_RTL_AIRTIME */

#ifdef ROUTE_SUPPORT
	// initialize static route table
	if ( !init_linkchain(&staticRouteChain, sizeof(STATICROUTE_T), MAX_ROUTE_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->staticRouteNum; i++) {
		if ( !add_linkchain(&staticRouteChain, (char *)&pMib->staticRouteArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	staticRouteChain.compareLen = sizeof(STATICROUTE_T) -4 ; // not contain gateway
#endif //ROUTE
#ifdef CHECK_IPV6_DHCP_OPTION
		// initialize dhcp6 option16 table
	if ( !init_linkchain(&dhcp6Option16Chain, sizeof(DHCP6_OPTION16_T), MAX_DHCP6_OPTION16_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->dhcp6Option16InstanceNum; i++) {
		if ( !add_linkchain(&dhcp6Option16Chain, (char *)&pMib->dhcp6Option16Array[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	dhcp6Option16Chain.compareLen = sizeof(DHCP6_OPTION16_T) -4 ;
	
		// initialize dhcp6 option17 table
		if ( !init_linkchain(&dhcp6Option17Chain, sizeof(DHCP6_OPTION17_T), MAX_DHCP6_OPTION17_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
		for (i=0; i<pMib->dhcp6Option17InstanceNum; i++) {
			if ( !add_linkchain(&dhcp6Option17Chain, (char *)&pMib->dhcp6Option17Array[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
				apmib_shm_free(pMib, CSCONF_SHM_KEY);
				apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
				apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
				apmib_sem_unlock();
#else
				free(pMib);
				free(pMibDef);
				free(pHwSetting);
#endif
				return 0;
			}
		}
		dhcp6Option17Chain.compareLen = sizeof(DHCP6_OPTION17_T) -4 ;
#endif
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	if ( !init_linkchain(&capPortalAllowChain, sizeof(CAP_PORTAL_T), MAX_CAP_PORTAL_ALLOW_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->capPortalCurNum; i++) {
		if ( !add_linkchain(&capPortalAllowChain, (char *)&pMib->capPortalAllowArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	capPortalAllowChain.compareLen = sizeof(CAP_PORTAL_T) -4 ; // not contain gateway
#endif //_SUPPORT_CAPTIVEPORTAL_PROFILE_

#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	// initialize dhcpServerOptionChain table
	if ( !init_linkchain(&dhcpServerOptionChain, sizeof(MIB_CE_DHCP_OPTION_T), MAX_DHCP_SERVER_OPTION_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->dhcpServerOptionNum; i++) {
		if ( !add_linkchain(&dhcpServerOptionChain, (char *)&pMib->dhcpServerOptionArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	dhcpServerOptionChain.compareLen = sizeof(MIB_CE_DHCP_OPTION_T);


	// initialize dhcpClientOptionChain table
	if ( !init_linkchain(&dhcpClientOptionChain, sizeof(MIB_CE_DHCP_OPTION_T), MAX_DHCP_CLIENT_OPTION_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->dhcpClientOptionNum; i++) {
		if ( !add_linkchain(&dhcpClientOptionChain, (char *)&pMib->dhcpClientOptionArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
		return 0;
		}
	}
	dhcpClientOptionChain.compareLen = sizeof(MIB_CE_DHCP_OPTION_T);


	// initialize dhcpsServingPoolChain table
	if ( !init_linkchain(&dhcpsServingPoolChain, sizeof(DHCPS_SERVING_POOL_T), MAX_DHCPS_SERVING_POOL_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->dhcpsServingPoolNum; i++) {
		if ( !add_linkchain(&dhcpsServingPoolChain, (char *)&pMib->dhcpsServingPoolArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
		return 0;
		}
	}
	dhcpsServingPoolChain.compareLen = sizeof(DHCPS_SERVING_POOL_T);
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */

#ifdef VPN_SUPPORT
	// initialize port forwarding table
	if ( !init_linkchain(&ipsecTunnelChain, sizeof(IPSECTUNNEL_T), MAX_TUNNEL_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->ipsecTunnelNum; i++) {
		if ( !add_linkchain(&ipsecTunnelChain, (char *)&pMib->ipsecTunnelArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	ipsecTunnelChain.compareLen = 1 ;  // only tunnel id
#endif
#endif // HOME_GATEWAY
#ifdef TLS_CLIENT
	if ( !init_linkchain(&certRootChain, sizeof(CERTROOT_T), MAX_CERTROOT_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->certRootNum; i++) {
		if ( !add_linkchain(&certRootChain, (char *)&pMib->certRootArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	certRootChain.compareLen = 21 ;  // only comment
	if ( !init_linkchain(&certUserChain, sizeof(CERTUSER_T), MAX_CERTUSER_NUM)) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_shm_free(pMib, CSCONF_SHM_KEY);
		apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
		apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
		apmib_sem_unlock();
#else
		free(pMib);
		free(pMibDef);
		free(pHwSetting);
#endif
		return 0;
	}
	for (i=0; i<pMib->certUserNum; i++) {
		if ( !add_linkchain(&certUserChain, (char *)&pMib->certUserArray[i]) ) {
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_shm_free(pMib, CSCONF_SHM_KEY);
			apmib_shm_free(pMibDef, DSCONF_SHM_KEY);
			apmib_shm_free(pHwSetting, HWCONF_SHM_KEY);
			apmib_sem_unlock();
#else
			free(pMib);
			free(pMibDef);
			free(pHwSetting);
#endif
			return 0;
		}
	}
	certUserChain.compareLen = 21 ;  // only comment	
#endif
	init_linkchain(&dhcpRsvdIpChain, sizeof(DHCPRSVDIP_T), MAX_DHCP_RSVD_IP_NUM);
	for (i=0; i<pMib->dhcpRsvdIpNum; i++)
		add_linkchain(&dhcpRsvdIpChain, (char *)&pMib->dhcpRsvdIpArray[i]);	
	dhcpRsvdIpChain.compareLen = 4;

#if defined(VLAN_CONFIG_SUPPORTED)
	init_linkchain(&vlanConfigChain, sizeof(VLAN_CONFIG_T), MAX_IFACE_VLAN_CONFIG);
	for (i=0; i<pMib->VlanConfigNum; i++)
		add_linkchain(&vlanConfigChain, (char *)&pMib->VlanConfigArray[i]);	
	vlanConfigChain.compareLen = sizeof(VLAN_CONFIG_T);

#endif

#ifdef RTK_CAPWAP
	init_linkchain(&capwapWtpConfigChain, sizeof(CAPWAP_WTP_CONFIG_T), MAX_CAPWAP_WTP_NUM);
	for (i=0; i<pMib->wtpConfigNum; i++)
		add_linkchain(&capwapWtpConfigChain, (char *)&pMib->wtpConfigArray[i]);	
	capwapWtpConfigChain.compareLen = sizeof(CAPWAP_WTP_CONFIG_T);

#endif


#endif /*no def MIB_TLV*/

#if CONFIG_APMIB_SHARED_MEMORY == 1
    apmib_sem_unlock();
#endif

	return 1;
}


///////////////////////////////////////////////////////////////////////////////
#if CONFIG_APMIB_SHARED_MEMORY == 1
char *apmib_load_hwconf(void)
{
	int ver;
	char *buff;
	int created;

#ifdef COMPRESS_MIB_SETTING
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;
	int compress_hw_setting=1;
#endif

#ifdef COMPRESS_MIB_SETTING
//	COMP_TRACE(stderr,"\r\n  __[%s-%u]", __FILE__,__LINE__);

	flash_read((char *)&compHeader, HW_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T));
	if(memcmp(compHeader.signature, COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);
		if ( (compLen > 0) && (compLen <= HW_SETTING_SECTOR_LEN) ) {
			compFile=malloc(compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}
			expFile=malloc(zipRate*compLen);
			if(expFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
				free(compFile);
				return 0;
			}

			flash_read((char *)compFile, HW_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T));

			expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);
			// copy the mib header from expFile
			memcpy((char *)&hsHeader, expFile, sizeof(hsHeader));
			hsHeader.len=WORD_SWAP(hsHeader.len);
			COMP_TRACE(stderr,"\r\n hsHeader.len = %u __[%s-%u]",hsHeader.len, __FILE__,__LINE__);
		} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}

#ifdef MIB_TLV
		mib_table_entry_T *pmib_tl = NULL;
		unsigned char *hwMibData;
		unsigned int tlv_content_len = hsHeader.len - 1; // last one is checksum

		hwMibData = malloc(sizeof(HW_SETTING_T)+1); // 1: checksum

		if(hwMibData != NULL)
			memset(hwMibData, 0x00, sizeof(HW_SETTING_T)+1);
	
		pmib_tl = mib_get_table(HW_SETTING);
		unsigned int tlv_checksum = 0;

		if(expFile != NULL)
			tlv_checksum = CHECKSUM_OK(expFile+sizeof(hsHeader), hsHeader.len);

//mib_display_tlv_content(HW_SETTING, expFile+sizeof(PARAM_HEADER_T), hsHeader.len);

		if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(hsHeader), (void*)hwMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
		{
			sprintf((char *)&hsHeader.signature[TAG_LEN], "%02d", HW_SETTING_VER);
			hsHeader.len = sizeof(HW_SETTING_T)+1;
			COMP_TRACE(stderr,"\r\n hsHeader.len = %u __[%s-%u]",hsHeader.len, __FILE__,__LINE__);
			hwMibData[hsHeader.len-1]  = CHECKSUM(hwMibData, hsHeader.len-1);

			if(expFile!= NULL)
				free(expFile);

			expFile = malloc(sizeof(hsHeader)+hsHeader.len);
			memcpy(expFile, &hsHeader, sizeof(hsHeader));
			memcpy(expFile+sizeof(hsHeader), hwMibData, hsHeader.len);

//mib_display_data_content(HW_SETTING, expFile+sizeof(PARAM_HEADER_T), hsHeader.len);

			
		}	

		if(hwMibData != NULL)
			free(hwMibData);
		
#endif // #ifdef MIB_TLV

		
	}
	else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
	// Read hw setting
	{
		if ( flash_read((char *)&hsHeader, HW_SETTING_OFFSET, sizeof(hsHeader))==0 ) {
	//		printf("Read hw setting header failed!\n");
			return NULL;
		}
		hsHeader.len=WORD_SWAP(hsHeader.len);
		compress_hw_setting=0;
	}

	if ( sscanf((char *)&hsHeader.signature[TAG_LEN], "%02d", &ver) != 1)
		ver = -1;

	if ( memcmp(hsHeader.signature, HW_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
		(ver != HW_SETTING_VER) || // version not equal to current
		(hsHeader.len < (sizeof(HW_SETTING_T)+1)) ) { // length is less than current
//		printf("Invalid hw setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n", hsHeader.signature[0],
//			hsHeader.signature[1], ver, hsHeader.len);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
//	if (ver > HW_SETTING_VER)
//		printf("HW setting version is greater than current [f:%d, c:%d]!\n", ver, HW_SETTING_VER);

	buff = apmib_shm_calloc(1, hsHeader.len, HWCONF_SHM_KEY, &created);
	if ( buff == 0 ) {
//		printf("Allocate buffer failed!\n");
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
#ifdef COMPRESS_MIB_SETTING
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);	
		memcpy(buff, expFile+sizeof(hsHeader), hsHeader.len);
		free(expFile);
		free(compFile);
	}
	else // not compress mib data, copy mibdata from flash
#endif
	if ( flash_read(buff, HW_SETTING_OFFSET+sizeof(hsHeader), hsHeader.len)==0 ) {
//		printf("Read hw setting failed!\n");
		apmib_shm_free(buff, HWCONF_SHM_KEY);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
	if ( !CHECKSUM_OK((unsigned char *)buff, hsHeader.len) ) {
//		printf("Invalid checksum of hw setting!\n");
		apmib_shm_free(buff, HWCONF_SHM_KEY);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
	if(!compress_hw_setting){
#ifdef MIB_TLV
		int index=0;
		unsigned short *wVal=NULL;
		unsigned int *dwVal=NULL;
		mib_table_entry_T * pmib_tl = mib_get_table(HW_SETTING);//hwmib_root_table
		pmib_tl=pmib_tl->next_mib_table;//hwmib_table
		while(pmib_tl[index].id){
			switch(pmib_tl[index].type)
			{
			case WORD_T:
				wVal=buff+pmib_tl[index].offset;
				*wVal=WORD_SWAP(*wVal);
				break;
			case DWORD_T:
				dwVal=buff+pmib_tl[index].offset;
				*dwVal=DWORD_SWAP(*dwVal);
				break;
			default:
				break;
			}
			index++;
		}
#endif
	}
	return buff;
}
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
////////////////////////////////////////////////////////////////////////////////
char *apmib_load_bluetooth_hwconf(void)
{
	int ver;
	char *buff;
	int created;

#ifdef COMPRESS_MIB_SETTING
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;
#endif

#ifdef COMPRESS_MIB_SETTING
//	COMP_TRACE(stderr,"\r\n  __[%s-%u]", __FILE__,__LINE__);

	flash_read((char *)&compHeader, BLUETOOTH_HW_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T));
	if(memcmp(compHeader.signature, COMP_BT_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);
		if ( (compLen > 0) && (compLen <= BLUETOOTH_HW_SETTING_SECTOR_LEN) ) {
			compFile=malloc(compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}
			expFile=malloc(zipRate*compLen);
			if(expFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
				free(compFile);
				return 0;
			}

			flash_read((char *)compFile, BLUETOOTH_HW_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T));

			expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);
			// copy the mib header from expFile
			memcpy((char *)&blueToothHwHeader, expFile, sizeof(blueToothHwHeader));
			blueToothHwHeader.len=HEADER_SWAP(blueToothHwHeader.len);
//			COMP_TRACE(stderr,"\r\n bluetooth hsHeader.len = %u __[%s-%u]",blueToothHwHeader.len, __FILE__,__LINE__);
		} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}

#ifdef MIB_TLV
		mib_table_entry_T *pmib_tl = NULL;
		unsigned char *bluetoothHwMibData;
		unsigned int tlv_content_len = blueToothHwHeader.len - 1; // last one is checksum

		bluetoothHwMibData = malloc(sizeof(BLUETOOTH_HW_SETTING_T)+1); // 1: checksum

		if(bluetoothHwMibData != NULL)
			memset(bluetoothHwMibData, 0x00, sizeof(BLUETOOTH_HW_SETTING_T)+1);
	
		pmib_tl = mib_get_table(BLUETOOTH_HW_SETTING);
		unsigned int tlv_checksum = 0;

		if(expFile != NULL)
			tlv_checksum = CHECKSUM_OK(expFile+sizeof(blueToothHwHeader), blueToothHwHeader.len);

//mib_display_tlv_content(HW_SETTING, expFile+sizeof(PARAM_HEADER_T), hsHeader.len);

		if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(blueToothHwHeader), (void*)bluetoothHwMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
		{
			sprintf((char *)&blueToothHwHeader.signature[TAG_LEN], "%02d", BLUETOOTH_HW_SETTING_VER);
			blueToothHwHeader.len = sizeof(BLUETOOTH_HW_SETTING_T)+1;
		//	COMP_TRACE(stderr,"\r\n bluetooth hsHeader.len = %u __[%s-%u]",blueToothHwHeader.len, __FILE__,__LINE__);
			bluetoothHwMibData[blueToothHwHeader.len-1]  = CHECKSUM(bluetoothHwMibData, blueToothHwHeader.len-1);

			if(expFile!= NULL)
				free(expFile);

			expFile = malloc(sizeof(blueToothHwHeader)+blueToothHwHeader.len);
			memcpy(expFile, &blueToothHwHeader, sizeof(blueToothHwHeader));
			memcpy(expFile+sizeof(blueToothHwHeader), bluetoothHwMibData, blueToothHwHeader.len);

//mib_display_data_content(HW_SETTING, expFile+sizeof(PARAM_HEADER_T), hsHeader.len);

			
		}	

		if(bluetoothHwMibData != NULL)
			free(bluetoothHwMibData);
		
#endif // #ifdef MIB_TLV

		
	}
	else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
	// Read hw setting
	{
		if ( flash_read((char *)&blueToothHwHeader, BLUETOOTH_HW_SETTING_OFFSET, sizeof(blueToothHwHeader))==0 ) {
	//		printf("Read hw setting header failed!\n");
			return NULL;
		}
		blueToothHwHeader.len=WORD_SWAP(blueToothHwHeader.len);
	}

	if ( sscanf((char *)&blueToothHwHeader.signature[TAG_LEN], "%02d", &ver) != 1)
		ver = -1;

	if ( memcmp(blueToothHwHeader.signature, BLUETOOTH_HW_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
		(ver != BLUETOOTH_HW_SETTING_VER) || // version not equal to current
		(blueToothHwHeader.len < (sizeof(BLUETOOTH_HW_SETTING_T)+1)) ) { // length is less than current
//		printf("Invalid hw setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n", hsHeader.signature[0],
//			hsHeader.signature[1], ver, hsHeader.len);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
//	if (ver > HW_SETTING_VER)
//		printf("HW setting version is greater than current [f:%d, c:%d]!\n", ver, HW_SETTING_VER);

	buff = apmib_shm_calloc(1, blueToothHwHeader.len, BLUETOOTH_HW_SHM_KEY, &created);
	if ( buff == 0 ) {
//		printf("Allocate buffer failed!\n");
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
#ifdef COMPRESS_MIB_SETTING
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);	
		memcpy(buff, expFile+sizeof(blueToothHwHeader), blueToothHwHeader.len);
		free(expFile);
		free(compFile);
	}
	else // not compress mib data, copy mibdata from flash
#endif
	if ( flash_read(buff, BLUETOOTH_HW_SETTING_OFFSET+sizeof(blueToothHwHeader), blueToothHwHeader.len)==0 ) {
//		printf("Read hw setting failed!\n");
		apmib_shm_free(buff, BLUETOOTH_HW_SHM_KEY);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
	if ( !CHECKSUM_OK((unsigned char *)buff, blueToothHwHeader.len) ) {
//		printf("Invalid checksum of hw setting!\n");
		apmib_shm_free(buff, BLUETOOTH_HW_SHM_KEY);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
	return buff;
}


#endif

#ifdef CUSTOMER_HW_SETTING_SUPPORT
////////////////////////////////////////////////////////////////////////////////
char *apmib_load_customer_hwconf(void)
{
	int ver;
	char *buff;
	int created;

#ifdef COMPRESS_MIB_SETTING
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;
#endif

#ifdef COMPRESS_MIB_SETTING
//	COMP_TRACE(stderr,"\r\n  __[%s-%u]", __FILE__,__LINE__);

	flash_read((char *)&compHeader, CUSTOMER_HW_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T));
	if(memcmp(compHeader.signature, COMP_US_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);
		if ( (compLen > 0) && (compLen <= CUSTOMER_HW_SETTING_SECTOR_LEN) ) {
			compFile=malloc(compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}
			expFile=malloc(zipRate*compLen);
			if(expFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
				free(compFile);
				return 0;
			}

			flash_read((char *)compFile, CUSTOMER_HW_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T));

			expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);
			// copy the mib header from expFile
			memcpy((char *)&customerHwHeader, expFile, sizeof(customerHwHeader));
			customerHwHeader.len=HEADER_SWAP(customerHwHeader.len);
//			COMP_TRACE(stderr,"\r\n customer hsHeader.len = %u __[%s-%u]",customerHwHeader.len, __FILE__,__LINE__);
		} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}

#ifdef MIB_TLV
		mib_table_entry_T *pmib_tl = NULL;
		unsigned char *customerHwMibData;
		unsigned int tlv_content_len = customerHwHeader.len - 1; // last one is checksum

		customerHwMibData = malloc(sizeof(CUSTOMER_HW_SETTING_T)+1); // 1: checksum

		if(customerHwMibData != NULL)
			memset(customerHwMibData, 0x00, sizeof(CUSTOMER_HW_SETTING_T)+1);
	
		pmib_tl = mib_get_table(CUSTOMER_HW_SETTING);
		unsigned int tlv_checksum = 0;

		if(expFile != NULL)
			tlv_checksum = CHECKSUM_OK(expFile+sizeof(customerHwHeader), customerHwHeader.len);

//mib_display_tlv_content(HW_SETTING, expFile+sizeof(PARAM_HEADER_T), hsHeader.len);

		if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(customerHwHeader), (void*)customerHwMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
		{
			sprintf((char *)&customerHwHeader.signature[TAG_LEN], "%02d", CUSTOMER_HW_SETTING_VER);
			customerHwHeader.len = sizeof(CUSTOMER_HW_SETTING_T)+1;
		//	COMP_TRACE(stderr,"\r\n customer hsHeader.len = %u __[%s-%u]",customerHwHeader.len, __FILE__,__LINE__);
			customerHwMibData[customerHwHeader.len-1]  = CHECKSUM(customerHwMibData, customerHwHeader.len-1);

			if(expFile!= NULL)
				free(expFile);

			expFile = malloc(sizeof(customerHwHeader)+customerHwHeader.len);
			memcpy(expFile, &customerHwHeader, sizeof(customerHwHeader));
			memcpy(expFile+sizeof(customerHwHeader), customerHwMibData, customerHwHeader.len);

//mib_display_data_content(HW_SETTING, expFile+sizeof(PARAM_HEADER_T), hsHeader.len);

			
		}	

		if(customerHwMibData != NULL)
			free(customerHwMibData);
		
#endif // #ifdef MIB_TLV

		
	}
	else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
	// Read hw setting
	{
		if ( flash_read((char *)&customerHwHeader, CUSTOMER_HW_SETTING_OFFSET, sizeof(customerHwHeader))==0 ) {
	//		printf("Read hw setting header failed!\n");
			return NULL;
		}
		customerHwHeader.len=HEADER_SWAP(customerHwHeader.len);
	}

	if ( sscanf((char *)&customerHwHeader.signature[TAG_LEN], "%02d", &ver) != 1)
		ver = -1;

	if ( memcmp(customerHwHeader.signature, CUSTOMER_HW_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
		(ver != CUSTOMER_HW_SETTING_VER) || // version not equal to current
		(customerHwHeader.len < (sizeof(CUSTOMER_HW_SETTING_T)+1)) ) { // length is less than current
//		printf("Invalid hw setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n", hsHeader.signature[0],
//			hsHeader.signature[1], ver, hsHeader.len);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
//	if (ver > HW_SETTING_VER)
//		printf("HW setting version is greater than current [f:%d, c:%d]!\n", ver, HW_SETTING_VER);

	buff = apmib_shm_calloc(1, customerHwHeader.len, CUSTOMER_HW_SHM_KEY, &created);
	if ( buff == 0 ) {
//		printf("Allocate buffer failed!\n");
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
#ifdef COMPRESS_MIB_SETTING
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);	
		memcpy(buff, expFile+sizeof(customerHwHeader), customerHwHeader.len);
		free(expFile);
		free(compFile);
	}
	else // not compress mib data, copy mibdata from flash
#endif
	if ( flash_read(buff, CUSTOMER_HW_SETTING_OFFSET+sizeof(customerHwHeader), customerHwHeader.len)==0 ) {
//		printf("Read hw setting failed!\n");
		apmib_shm_free(buff, CUSTOMER_HW_SHM_KEY);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
	if ( !CHECKSUM_OK((unsigned char *)buff, customerHwHeader.len) ) {
//		printf("Invalid checksum of hw setting!\n");
		apmib_shm_free(buff, CUSTOMER_HW_SHM_KEY);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
	return buff;
}


#endif


#ifdef RF_DPK_SETTING_SUPPORT
////////////////////////////////////////////////////////////////////////////////
char *apmib_load_rf_dpk_conf(void)
{
	int ver;
	char *buff;
	int created;

#ifdef COMPRESS_MIB_SETTING
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;
	int compress_hw_setting=1;
#endif

#ifdef COMPRESS_MIB_SETTING
//	COMP_TRACE(stderr,"\r\n  __[%s-%u]", __FILE__,__LINE__);

	flash_read((char *)&compHeader, RF_DPK_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T));
	if(memcmp(compHeader.signature, COMP_RS_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);
        printf("==> %s:%d, compLen=%d\n", __func__, __LINE__, compLen);//bruce
        
		if ( (compLen > 0) && (compLen <= RF_DPK_SETTING_SECTOR_LEN) ) {
			compFile=malloc(compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}
			expFile=malloc(zipRate*compLen);
			if(expFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
				free(compFile);
				return 0;
			}

			flash_read((char *)compFile, RF_DPK_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T));

			expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);
			// copy the mib header from expFile
			memcpy((char *)&rfDpkHeader, expFile, sizeof(rfDpkHeader));
			rfDpkHeader.len=WORD_SWAP(rfDpkHeader.len);
			COMP_TRACE(stderr,"\r\n rfDpkHeader.len = %u __[%s-%u]",rfDpkHeader.len, __FILE__,__LINE__);
		} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}

#ifdef MIB_TLV
		mib_table_entry_T *pmib_tl = NULL;
		unsigned char *rfHwMibData;
		unsigned int tlv_content_len = rfDpkHeader.len - 1; // last one is checksum

		rfHwMibData = malloc(sizeof(RF_DPK_SETTING_T)+1); // 1: checksum

		if(rfHwMibData != NULL)
			memset(rfHwMibData, 0x00, sizeof(RF_DPK_SETTING_T)+1);
	
		pmib_tl = mib_get_table(RF_DPK_SETTING);
		unsigned int tlv_checksum = 0;

		if(expFile != NULL)
			tlv_checksum = CHECKSUM_OK(expFile+sizeof(rfDpkHeader), rfDpkHeader.len);

//mib_display_tlv_content(RF_DPK_SETTING, expFile+sizeof(PARAM_HEADER_T), rfDpkHeader.len);

		if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(rfDpkHeader), (void*)rfHwMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to rfHwMibData. parse total len is  tlv_content_len*/
		{
			sprintf((char *)&rfDpkHeader.signature[TAG_LEN], "%02d", RF_DPK_SETTING_VER);
			rfDpkHeader.len = sizeof(RF_DPK_SETTING_T)+1;
			//COMP_TRACE(stderr,"\r\n rfDpkHeader.len = %u __[%s-%u]",rfDpkHeader.len, __FILE__,__LINE__);
			rfHwMibData[rfDpkHeader.len-1]  = CHECKSUM(rfHwMibData, rfDpkHeader.len-1);

			if(expFile!= NULL)
				free(expFile);

			expFile = malloc(sizeof(rfDpkHeader)+rfDpkHeader.len);
			memcpy(expFile, &rfDpkHeader, sizeof(rfDpkHeader));
			memcpy(expFile+sizeof(rfDpkHeader), rfHwMibData, rfDpkHeader.len);

//mib_display_data_content(RF_DPK_SETTING, expFile+sizeof(PARAM_HEADER_T), rfDpkHeader.len);

			
		}	

		if(rfHwMibData != NULL)
			free(rfHwMibData);
		
#endif // #ifdef MIB_TLV

		
	}
	else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
	// Read RF DPKsetting
	{
		if ( flash_read((char *)&rfDpkHeader, RF_DPK_SETTING_OFFSET, sizeof(rfDpkHeader))==0 ) {
	//		printf("Read RF DPK setting header failed!\n");
			return NULL;
		}
		rfDpkHeader.len=WORD_SWAP(rfDpkHeader.len);
		compress_hw_setting=0;
	}

	if ( sscanf((char *)&rfDpkHeader.signature[TAG_LEN], "%02d", &ver) != 1)
		ver = -1;

	if ( memcmp(rfDpkHeader.signature, RF_DPK_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
		(ver != RF_DPK_SETTING_VER) || // version not equal to current
		(rfDpkHeader.len < (sizeof(RF_DPK_SETTING_T)+1)) ) { // length is less than current
//		printf("Invalid RF DPK setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n", rfDpkHeader.signature[0],
//			rfDpkHeader.signature[1], ver, rfDpkHeader.len);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
//	if (ver > RF_DPK_SETTING_VER)
//		printf("RF DPK setting version is greater than current [f:%d, c:%d]!\n", ver, RF_DPK_SETTING_VER);

	buff = apmib_shm_calloc(1, rfDpkHeader.len, RF_DPK_SHM_KEY, &created);
	if ( buff == 0 ) {
//		printf("Allocate buffer failed!\n");
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
#ifdef COMPRESS_MIB_SETTING
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);	
		memcpy(buff, expFile+sizeof(rfDpkHeader), rfDpkHeader.len);
		free(expFile);
		free(compFile);
	}
	else // not compress mib data, copy mibdata from flash
#endif
	if ( flash_read(buff, RF_DPK_SETTING_OFFSET+sizeof(rfDpkHeader), rfDpkHeader.len)==0 ) {
//		printf("Read RF DPK setting failed!\n");
		apmib_shm_free(buff, RF_DPK_SHM_KEY);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
	if ( !CHECKSUM_OK((unsigned char *)buff, rfDpkHeader.len) ) {
//		printf("Invalid checksum of RF DPK setting!\n");
		apmib_shm_free(buff, RF_DPK_SHM_KEY);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
	if(!compress_hw_setting){
#ifdef MIB_TLV
		int index=0;
		unsigned short *wVal=NULL;
		unsigned int *dwVal=NULL;
		mib_table_entry_T * pmib_tl = mib_get_table(RF_DPK_SETTING);//rf_dpk_mib_root_table
		pmib_tl=pmib_tl->next_mib_table;//rf_dpk_mib_table
		while(pmib_tl[index].id){
			switch(pmib_tl[index].type)
			{
			case WORD_T:
				wVal=buff+pmib_tl[index].offset;
				*wVal=WORD_SWAP(*wVal);
				break;
			case DWORD_T:
				dwVal=buff+pmib_tl[index].offset;
				*dwVal=DWORD_SWAP(*dwVal);
				break;
			default:
				break;
			}
			index++;
		}
#endif
	}
	return buff;
}
#endif /* RF_DPK_SETTING_SUPPORT */



////////////////////////////////////////////////////////////////////////////////
char *apmib_load_dsconf(void)
{
#ifndef RTL_DEF_SETTING_IN_FW
	int ver;
	char *buff;
	int created;

#ifdef COMPRESS_MIB_SETTING
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;
#endif

#ifdef COMPRESS_MIB_SETTING

	flash_read((char *)&compHeader, DEFAULT_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T));

	if(memcmp(compHeader.signature, COMP_DS_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);
		if ( (compLen > 0) && (compLen <= DEFAULT_SETTING_SECTOR_LEN) ) {
			compFile=malloc(compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}
			expFile=malloc(zipRate*compLen);
			if(expFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
				free(compFile);
				return 0;
			}

			flash_read((char *)compFile, DEFAULT_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T));

			expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);

			// copy the mib header from expFile
			memcpy((char *)&dsHeader, expFile, sizeof(dsHeader));
			dsHeader.len=HEADER_SWAP(dsHeader.len);
		} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}

#ifdef MIB_TLV
		mib_table_entry_T *pmib_tl = NULL;
		unsigned char *defMibData;
		unsigned int tlv_content_len = dsHeader.len - 1; // last one is checksum

		defMibData = malloc(sizeof(APMIB_T)+1); // 1: checksum

		if(defMibData != NULL)
			memset(defMibData, 0x00, sizeof(APMIB_T)+1);
	
		pmib_tl = mib_get_table(DEFAULT_SETTING);
		unsigned int tlv_checksum = 0;

		if(expFile != NULL)
			tlv_checksum = CHECKSUM_OK(expFile+sizeof(PARAM_HEADER_T), dsHeader.len);

//mib_display_tlv_content(DEFAULT_SETTING, expFile+sizeof(PARAM_HEADER_T), dsHeader.len);
		
		if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(PARAM_HEADER_T), (void*)defMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
		{
			sprintf((char *)&dsHeader.signature[TAG_LEN], "%02d", DEFAULT_SETTING_VER);
			dsHeader.len = sizeof(APMIB_T)+1;
			defMibData[dsHeader.len-1]  = CHECKSUM(defMibData, sizeof(APMIB_T));

			if(expFile!= NULL)
				free(expFile);

			expFile = malloc(sizeof(PARAM_HEADER_T)+dsHeader.len);
			memcpy(expFile, &dsHeader, sizeof(PARAM_HEADER_T));
			memcpy(expFile+sizeof(PARAM_HEADER_T), defMibData, dsHeader.len);
			
//mib_display_data_content(DEFAULT_SETTING, expFile+sizeof(PARAM_HEADER_T), dsHeader.len);
		}	
		else
		{
			COMP_TRACE(stderr,"\r\n ERR!Invalid checksum[%u] or mib_tlv_init() fail! __[%s-%u]",tlv_checksum,__FILE__,__LINE__);
		}
		
		if(defMibData != NULL)
			free(defMibData);
		
#endif // #ifdef MIB_TLV		

		
	}
	else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
	// Read default s/w mib
	{
		if ( flash_read((char *)&dsHeader, DEFAULT_SETTING_OFFSET, sizeof(dsHeader))==0 ) {
	//		printf("Read default setting header failed!\n");
			return NULL;
		}		
		dsHeader.len=HEADER_SWAP(dsHeader.len);
	}

	if ( sscanf((char *)&dsHeader.signature[TAG_LEN], "%02d", &ver) != 1)
		ver = -1;

	if ( memcmp(dsHeader.signature, DEFAULT_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
		(ver != DEFAULT_SETTING_VER) || // version not equal to current
		(dsHeader.len < (sizeof(APMIB_T)+1)) ) { // length is less than current
//		printf("Invalid default setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",
//			dsHeader.signature[0], dsHeader.signature[1], ver, dsHeader.len);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}
//	if (ver > DEFAULT_SETTING_VER)
//		printf("Default setting version is greater than current [f:%d, c:%d]!\n", ver, DEFAULT_SETTING_VER);

	buff = apmib_shm_calloc(1, dsHeader.len, DSCONF_SHM_KEY, &created);
	if ( buff == 0 ) {
//		printf("Allocate buffer failed!\n");
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}

#ifdef COMPRESS_MIB_SETTING

	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);		
		memcpy(buff, expFile+sizeof(PARAM_HEADER_T), dsHeader.len);
		free(expFile);
		free(compFile);
	}
	else // not compress mib data, copy mibdata from flash
#endif
	if ( flash_read(buff, DEFAULT_SETTING_OFFSET+sizeof(dsHeader), dsHeader.len)==0 ) {
//		printf("Read default setting failed!\n");
		apmib_shm_free(buff, DSCONF_SHM_KEY);
		return NULL;
	}

	if ( !CHECKSUM_OK((unsigned char *)buff, dsHeader.len) ) {
//		printf("Invalid checksum of current setting!\n");
		apmib_shm_free(buff, DSCONF_SHM_KEY);
		return NULL;
	}
	return buff;
#endif
}

////////////////////////////////////////////////////////////////////////////////
char *apmib_load_csconf(void)
{
	int ver;
	char *buff;
	int created;

#ifdef COMPRESS_MIB_SETTING
	int zipRate=0;
	unsigned char *compFile=NULL, *expFile=NULL;
	unsigned int expandLen=0, compLen=0;
	COMPRESS_MIB_HEADER_T compHeader;
#endif

#ifdef COMPRESS_MIB_SETTING

//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);
	flash_read((char *)&compHeader, CURRENT_SETTING_OFFSET, sizeof(COMPRESS_MIB_HEADER_T));
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);	
	if(memcmp(compHeader.signature, COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN) == 0 ) //check whether compress mib data
	{
		zipRate = WORD_SWAP(compHeader.compRate);
		compLen = DWORD_SWAP(compHeader.compLen);
		if ( (compLen > 0) && (compLen <= CURRENT_SETTING_SECTOR_LEN) ) {
			compFile=calloc(1,compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);
				return 0;
			}
			expFile=calloc(1,zipRate*compLen);
			if(expFile==NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! __[%s-%u]",__FILE__,__LINE__);			
				free(compFile);
				return 0;
			}

			flash_read((char *)compFile, CURRENT_SETTING_OFFSET, compLen+sizeof(COMPRESS_MIB_HEADER_T));

			expandLen = Decode(compFile+sizeof(COMPRESS_MIB_HEADER_T), compLen, expFile);
			//printf("expandLen read len: %d\n", expandLen);

			// copy the mib header from expFile
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);				
			memcpy((char *)&csHeader, expFile, sizeof(csHeader));
			csHeader.len=HEADER_SWAP(csHeader.len);

            #define MAX_CONFIG_LEN 1024*1024
            #define MIN_CONFIG_LEN 8*1024
            if((csHeader.len > MAX_CONFIG_LEN)||(csHeader.len < MIN_CONFIG_LEN))
            {
                printf("INVALID CONFIG FILE\n");
                return NULL;
            }
		} 
		else 
		{
			COMP_TRACE(stderr,"\r\n ERR:compLen = %u __[%s-%u]",compLen, __FILE__,__LINE__);
			return 0;
		}

#ifdef MIB_TLV
		mib_table_entry_T *pmib_tl = NULL;
		unsigned char *curMibData;
		unsigned int tlv_content_len = csHeader.len - 1; // last one is checksum

		curMibData = malloc(sizeof(APMIB_T)+1); // 1: checksum

		if(curMibData != NULL)
			memset(curMibData, 0x00, sizeof(APMIB_T)+1);
	
		pmib_tl = mib_get_table(CURRENT_SETTING);
		unsigned int tlv_checksum = 0;

		if(expFile != NULL)
			tlv_checksum = CHECKSUM_OK(expFile+sizeof(PARAM_HEADER_T), csHeader.len);

//mib_display_tlv_content(CURRENT_SETTING, expFile+sizeof(PARAM_HEADER_T), csHeader.len);

		if(tlv_checksum == 1 && mib_tlv_init(pmib_tl, expFile+sizeof(PARAM_HEADER_T), (void*)curMibData, tlv_content_len) == 1) /* According to pmib_tl, get value from expFile to hwMibData. parse total len is  tlv_content_len*/
		{
			sprintf((char *)&csHeader.signature[TAG_LEN], "%02d", CURRENT_SETTING_VER);
			csHeader.len = sizeof(APMIB_T)+1;
			curMibData[csHeader.len-1]  = CHECKSUM(curMibData, csHeader.len-1);

			if(expFile!= NULL)
				free(expFile);

			expFile = malloc(sizeof(PARAM_HEADER_T)+csHeader.len);
			memcpy(expFile, &csHeader, sizeof(PARAM_HEADER_T));
			memcpy(expFile+sizeof(PARAM_HEADER_T), curMibData, csHeader.len);
			
//mib_display_data_content(CURRENT_SETTING, expFile+sizeof(PARAM_HEADER_T), csHeader.len);
		}	

		if(curMibData != NULL)
			free(curMibData);
		
#endif // #ifdef MIB_TLV	

	}
	else // not compress mib-data, get mib header from flash
#endif //#ifdef COMPRESS_MIB_SETTING
	// Read current s/w mib
	{
		if ( flash_read((char *)&csHeader, CURRENT_SETTING_OFFSET, sizeof(csHeader))==0 ) {
	//		printf("Read current setting header failed!\n");
			return NULL;
		}
		csHeader.len=HEADER_SWAP(csHeader.len);
	}

	if ( sscanf((char *)&csHeader.signature[TAG_LEN], "%02d", &ver) != 1)
		ver = -1;

	if ( memcmp(csHeader.signature, CURRENT_SETTING_HEADER_TAG, TAG_LEN) || // invalid signatur
		(ver != CURRENT_SETTING_VER) || // version not equal to current
			(csHeader.len < (sizeof(APMIB_T)+1)) ) { // length is less than current
//		printf("Invalid current setting signature or version number [sig=%c%c, ver=%d, len=%d]!\n",
//			csHeader.signature[0], csHeader.signature[1], ver, csHeader.len);
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}

//	if (ver > CURRENT_SETTING_VER)
//		printf("Current setting version is greater than current [f:%d, c:%d]!\n", ver, CURRENT_SETTING_VER);

	buff = apmib_shm_calloc(1, csHeader.len, CSCONF_SHM_KEY, &created);
	if ( buff == 0 ) {
//		printf("Allocate buffer failed!\n");
#ifdef COMPRESS_MIB_SETTING
		if(compFile != NULL)
			free(compFile);
		if(expFile != NULL)
			free(expFile);
#endif
		return NULL;
	}

#ifdef COMPRESS_MIB_SETTING

	if(expFile != NULL) //compress mib data, copy the mibdata body from expFile
	{
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);					
		memcpy(buff, expFile+sizeof(PARAM_HEADER_T), csHeader.len);
		free(expFile);
		free(compFile);
	}
	else // not compress mib data, copy mibdata from flash
#endif
	if ( flash_read(buff, CURRENT_SETTING_OFFSET+sizeof(csHeader), csHeader.len)==0 ) {
//		printf("Read current setting failed!\n");
		apmib_shm_free(buff, CSCONF_SHM_KEY);
		return NULL;
	}

	if ( !CHECKSUM_OK((unsigned char *)buff, csHeader.len) ) {
//		printf("Invalid checksum of current setting!\n");
		apmib_shm_free(buff, CSCONF_SHM_KEY);
		return NULL;
	}

	return buff;
}
#endif

int apmib_reinit(void)
{
#ifndef MIB_TLV
	int i, j;
#endif

	if (pMib == NULL)	// has not been initialized
		return 0;

#if CONFIG_APMIB_SHARED_MEMORY != 1
	free(pMib);
	free(pMibDef);
	free(pHwSetting);
#endif

#ifdef MIB_TLV
#else
	for (i=0; i<NUM_WLAN_INTERFACE; i++) 
		for (j=0; j<(NUM_VWLAN_INTERFACE+1); j++) 
	{
		free(wlanMacChain[i][j].buf);
		free(wdsChain[i][j].buf);
#if defined(WIFI_SIMPLE_CONFIG) && defined(CONFIG_APP_TR069)
		free(wscERChain[i][j].buf);
#endif
	}

#ifdef HOME_GATEWAY
	free(portFwChain.buf);
	free(ipFilterChain.buf);
	free(portFilterChain.buf);
	free(macFilterChain.buf);
	free(urlFilterChain.buf);
	free(triggerPortChain.buf);
#if (defined(GW_QOS_ENGINE) || defined(QOS_BY_BANDWIDTH)) && !defined(MULTI_WAN_QOS)
    	free(qosChain.buf);
#endif
#ifdef ROUTE_SUPPORT
	free(staticRouteChain.buf);
#endif //ROUTE
#ifdef CHECK_IPV6_DHCP_OPTION
	free(dhcp6Option16Chain.buf);
	free(dhcp6Option17Chain.buf);
#endif
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	free(capPortalAllowChain.buf);
#endif
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	free(dhcpServerOptionChain.buf);
	free(dhcpClientOptionChain.buf);
	free(dhcpsServingPoolChain.buf);
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */
#ifdef VPN_SUPPORT
	free(ipsecTunnelChain.buf);
#endif
#endif
#ifdef TLS_CLIENT
	free(certRootChain.buf);
	free(certUserChain.buf);
#endif

	free(dhcpRsvdIpChain.buf);
#if defined(VLAN_CONFIG_SUPPORTED)
	free(vlanConfigChain.buf);
#endif	

#ifdef RTK_CAPWAP
	free(capwapWtpConfigChain.buf);
#endif	

#ifdef CONFIG_RTL_AIRTIME
	free(airtimeChain.buf);
	free(airtimeChain2.buf);
#endif /* CONFIG_RTL_AIRTIME */

#endif

#if CONFIG_APMIB_SHARED_MEMORY != 1
	pMib=NULL;
	pMibDef=NULL;
	pHwSetting=NULL;
#endif


#if CONFIG_APMIB_SHARED_MEMORY == 1	
    apmib_sem_lock();
    apmib_load_hwconf();
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
	apmib_load_bluetooth_hwconf();
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
	apmib_load_customer_hwconf();
#endif
#ifdef RF_DPK_SETTING_SUPPORT
	apmib_load_rf_dpk_conf();
#endif /* RF_DPK_SETTING_SUPPORT */
    apmib_load_dsconf();
    apmib_load_csconf();
    apmib_sem_unlock();
#endif

	return apmib_init();
}

////////////////////////////////////////////////////////////////////////////////
static int search_tbl(int id, mib_table_entry_T *pTbl, int *idx)
{
	int i;
	for (i=0; pTbl[i].id; i++) {
		if ( pTbl[i].id == id ) {
			*idx = i;
			return id;
		}
	}
	return 0;
}


#if defined(CONFIG_APP_TR069) && defined(WLAN_SUPPORT)
int swapWLANIdxForCwmp(unsigned char wlanifNumA, unsigned char wlanifNumB)
{
	
	int ret=-1;
	unsigned int i,num;
	CWMP_WLANCONF_T *pwlanConf, wlanConf;
	CWMP_WLANCONF_T target[2];
	pwlanConf = &wlanConf;
	apmib_get(MIB_CWMP_WLANCONF_TBL_NUM, (void *)&num);

	for( i=1; i<=num;i++ )
	{
		*((char *)pwlanConf) = (char)i;
		if(!apmib_get(MIB_CWMP_WLANCONF_TBL, (void *)pwlanConf))
			continue;
			
		memcpy(&target[0], &wlanConf, sizeof(CWMP_WLANCONF_T));
		if( pwlanConf->RootIdx==0)
			pwlanConf->RootIdx=1;
		else if(pwlanConf->RootIdx==1)
			pwlanConf->RootIdx=0;
			
		if(pwlanConf->RfBandAvailable ==PHYBAND_5G)
			pwlanConf->RfBandAvailable =PHYBAND_2G;
		else if(pwlanConf->RfBandAvailable ==PHYBAND_2G)
			pwlanConf->RfBandAvailable =PHYBAND_5G;
			
		memcpy(&target[1], &wlanConf, sizeof(CWMP_WLANCONF_T));
		printf("%s:%d, do swap wlanCONF  mib\n", __FUNCTION__, __LINE__);
		apmib_set(MIB_CWMP_WLANCONF_MOD, (void *)&target);	
		
	}	
	return ret; 

}		
#endif
void swapWlanMibSetting(unsigned char wlanifNumA, unsigned char wlanifNumB)
{
	unsigned char *wlanMibBuf=NULL;
	unsigned int totalSize = sizeof(CONFIG_WLAN_SETTING_T)*(NUM_VWLAN_INTERFACE+1); // 4vap+1rpt+1root

#if CONFIG_APMIB_SHARED_MEMORY == 1
	apmib_sem_lock();
#endif

	wlanMibBuf = malloc(totalSize); 
#if 0	
	printf("\r\n wlanifNumA=[%u],__[%s-%u]\r\n",wlanifNumA,__FILE__,__LINE__);
	printf("\r\n wlanifNumB=[%u],__[%s-%u]\r\n",wlanifNumB,__FILE__,__LINE__);
	
	printf("\r\n pMib->wlan[wlanifNumA]=[0x%x],__[%s-%u]\r\n",pMib->wlan[wlanifNumA],__FILE__,__LINE__);
	printf("\r\n pMib->wlan[wlanifNumB]=[0x%x],__[%s-%u]\r\n",pMib->wlan[wlanifNumB],__FILE__,__LINE__);
	
	printf("\r\n pMib->wlan[0][0].wlanDisabled=[%u],__[%s-%u]\r\n",pMib->wlan[0][0].wlanDisabled,__FILE__,__LINE__);
	printf("\r\n pMib->wlan[0][0].phyBandSelect=[%u],__[%s-%u]\r\n",pMib->wlan[0][0].phyBandSelect,__FILE__,__LINE__);
	printf("\r\n pMib->wlan[0][0].channel=[%u],__[%s-%u]\r\n",pMib->wlan[0][0].channel,__FILE__,__LINE__);
	
	printf("\r\n pMib->wlan[1][0].wlanDisabled=[%u],__[%s-%u]\r\n",pMib->wlan[1][0].wlanDisabled,__FILE__,__LINE__);
	printf("\r\n pMib->wlan[1][0].phyBandSelect=[%u],__[%s-%u]\r\n",pMib->wlan[1][0].phyBandSelect,__FILE__,__LINE__);
	printf("\r\n pMib->wlan[1][0].channel=[%u],__[%s-%u]\r\n",pMib->wlan[1][0].channel,__FILE__,__LINE__);
#endif			
	if(wlanMibBuf != NULL)
	{
		memcpy(wlanMibBuf, pMib->wlan[wlanifNumA], totalSize);
		memcpy(pMib->wlan[wlanifNumA], pMib->wlan[wlanifNumB], totalSize);
		memcpy(pMib->wlan[wlanifNumB], wlanMibBuf, totalSize);

		free(wlanMibBuf);
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif


#if defined(CONFIG_APP_TR069) && defined(WLAN_SUPPORT)
	swapWLANIdxForCwmp(wlanifNumA, wlanifNumB);
#endif

	
#if 0	
	printf("\r\n pMib->wlan[0][0].wlanDisabled=[%u],__[%s-%u]\r\n",pMib->wlan[0][0].wlanDisabled,__FILE__,__LINE__);
	printf("\r\n pMib->wlan[0][0].phyBandSelect=[%u],__[%s-%u]\r\n",pMib->wlan[0][0].phyBandSelect,__FILE__,__LINE__);
	printf("\r\n pMib->wlan[0][0].channel=[%u],__[%s-%u]\r\n",pMib->wlan[0][0].channel,__FILE__,__LINE__);
	
	printf("\r\n pMib->wlan[1][0].wlanDisabled=[%u],__[%s-%u]\r\n",pMib->wlan[1][0].wlanDisabled,__FILE__,__LINE__);
	printf("\r\n pMib->wlan[1][0].phyBandSelect=[%u],__[%s-%u]\r\n",pMib->wlan[1][0].phyBandSelect,__FILE__,__LINE__);
	printf("\r\n pMib->wlan[1][0].channel=[%u],__[%s-%u]\r\n",pMib->wlan[1][0].channel,__FILE__,__LINE__);
#endif	
#ifdef UNIVERSAL_REPEATER
	int rptEnable1, rptEnable2;
	char rptSsid1[MAX_SSID_LEN], rptSsid2[MAX_SSID_LEN];
	
	memset(rptSsid1, 0x00, MAX_SSID_LEN);
	memset(rptSsid2, 0x00, MAX_SSID_LEN);
	
	apmib_get(MIB_REPEATER_ENABLED1, (void *)&rptEnable1);
	apmib_get(MIB_REPEATER_ENABLED2, (void *)&rptEnable2);
	apmib_get(MIB_REPEATER_SSID1, (void *)rptSsid1);
	apmib_get(MIB_REPEATER_SSID2, (void *)rptSsid2);

	apmib_set(MIB_REPEATER_ENABLED1, (void *)&rptEnable2);
	apmib_set(MIB_REPEATER_ENABLED2, (void *)&rptEnable1);
	apmib_set(MIB_REPEATER_SSID1, (void *)rptSsid2);
	apmib_set(MIB_REPEATER_SSID2, (void *)rptSsid1);
#endif
#if VLAN_CONFIG_SUPPORTED 
	unsigned char *vlanMibBuf=NULL;
	totalSize = sizeof(VLAN_CONFIG_T)*5; // 4vap+1root

#if CONFIG_APMIB_SHARED_MEMORY == 1
	apmib_sem_lock();
#endif

	vlanMibBuf = malloc(totalSize);
	if(vlanMibBuf != NULL)
	{
		memcpy(vlanMibBuf, pMib->VlanConfigArray+4, totalSize);
		memcpy(pMib->VlanConfigArray+4, pMib->VlanConfigArray+9, totalSize);
		memcpy(pMib->VlanConfigArray+9, vlanMibBuf, totalSize);

		free(vlanMibBuf);
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
	
#endif
}
int clone_wlaninfo_get(CONFIG_WLAN_SETTING_T *wlanptr, int rootwlan_idx,int vwlan_idx)
{
	
#if CONFIG_APMIB_SHARED_MEMORY == 1	
		apmib_sem_lock();
#endif
	//printf("clone_wlaninfo_set get wlan mib<%d, %d>\n",rootwlan_idx,vwlan_idx );
	memcpy(wlanptr, &pMib->wlan[rootwlan_idx][vwlan_idx], sizeof(CONFIG_WLAN_SETTING_T));

#if CONFIG_APMIB_SHARED_MEMORY == 1	
			apmib_sem_unlock();
#endif

	return 0;

}
int clone_wlaninfo_set(CONFIG_WLAN_SETTING_T *wlanptr, int rootwlan_idx,int vwlan_idx,int Newrootwlan_idx,int Newvwlan_idx, int ChangeRFBand )
{
	
#if CONFIG_APMIB_SHARED_MEMORY == 1	
		apmib_sem_lock();
#endif

	if(ChangeRFBand==1 && pMib->wlan[rootwlan_idx][vwlan_idx].wlanDisabled==0){
		//printf("set old wlan if to disabled if enabled in orig\n");
		pMib->wlan[rootwlan_idx][vwlan_idx].wlanDisabled=1;
	}
		//printf("clone_wlaninfo_set set wlan mib<%d, %d>\n",Newrootwlan_idx,Newvwlan_idx );
	memcpy(&pMib->wlan[Newrootwlan_idx][Newvwlan_idx], wlanptr, sizeof(CONFIG_WLAN_SETTING_T));

#if CONFIG_APMIB_SHARED_MEMORY == 1	
			apmib_sem_unlock();
#endif

	return 0;

}

////////////////////////////////////////////////////////////////////////////////
int apmib_get(int id, void *value)
{
	int i, index;
	void *pMibTbl;
	mib_table_entry_T *pTbl;
	unsigned char ch;
	unsigned short wd;
	unsigned long dwd;
#ifdef MIB_TLV
	//unsigned int offset;
	unsigned int num;
#endif
#ifdef MULTI_WAN_SUPPORT
	if(id==MIB_WANIFACE_CURRENT_IDX)
	{
		*(int*)value = wan_idx;
		return 1;
	}
		
#endif

#ifndef MIB_TLV
#if CONFIG_APMIB_SHARED_MEMORY == 1
	apmib_sem_lock();
#endif
#endif

	if ( search_tbl(id, mib_table, &i) ) {
		pMibTbl = (void *)pMib;
		pTbl = mib_table;
	}
	else if ( search_tbl(id, mib_wlan_table, &i) ) {
		pMibTbl = (void *)&pMib->wlan[wlan_idx][vwlan_idx];
		pTbl = mib_wlan_table;
	}
	else if ( search_tbl(id, hwmib_table, &i) ) {
		pMibTbl = (void *)pHwSetting;
		pTbl = hwmib_table;
	}
	else if ( search_tbl(id, hwmib_wlan_table, &i) ) {
		pMibTbl = (void *)&pHwSetting->wlan[wlan_idx];
		pTbl = hwmib_wlan_table;
	}
#ifdef MULTI_WAN_SUPPORT
	else if( search_tbl(id, mib_waniface_tbl, &i) ){
		if(wan_idx<1 || wan_idx>WANIFACE_NUM)
		{
			fprintf(stderr,"wan_idx must be 1-%d",WANIFACE_NUM);
			goto GET_NOT_FIND_MIB;
		}
		pMibTbl = (void *)&pMib->WANIfaceArray[wan_idx-1];
		pTbl = mib_waniface_tbl;
	}
#endif
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
	else if ( search_tbl(id, bluetooth_hwmib_table, &i) ) {
		pMibTbl = (void *)pBlueToothHwSetting;
		pTbl = bluetooth_hwmib_table;
	}

#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
	else if ( search_tbl(id, customer_hwmib_table, &i) ) {
		pMibTbl = (void *)pCustomerHwSetting;
		pTbl = customer_hwmib_table;
	}
#endif
#ifdef RF_DPK_SETTING_SUPPORT
	else if ( search_tbl(id, rf_dpk_mib_table, &i) ) {
		pMibTbl = (void *)pRfDpkSetting;
		pTbl = rf_dpk_mib_table;
	}
#endif /* RF_DPK_SETTING_SUPPORT */
	else {
GET_NOT_FIND_MIB:

#ifndef MIB_TLV
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
#endif
		return 0;
	}
	
#ifdef MIB_TLV
	if(pTbl[i].type > TABLE_LIST_T)
	{		
		apmib_get(((pTbl[i].id & MIB_ID_MASK)-1),&num);
		index = (int)( *((unsigned char *)value));
		//printf("get index %d\n",index);
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_lock();
#endif		
		get_tblentry((void*)pMibTbl,pTbl[i].offset,num,&pTbl[i],(void *)value,index);
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
		return 1;
	}
#endif

#ifdef MIB_TLV
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_lock();
#endif	
#endif

	switch (pTbl[i].type) {
	case BYTE_T:
//		*((int *)value) =(int)(*((unsigned char *)(((long)pMibTbl) + pTbl[i].offset)));
		memcpy((char *)&ch, ((char *)pMibTbl) + pTbl[i].offset, 1);
		*((int *)value) = (int)ch;
		break;

	case WORD_T:
//		*((int *)value) =(int)(*((unsigned short *)(((long)pMibTbl) + pTbl[i].offset)));
		memcpy((char *)&wd, ((char *)pMibTbl) + pTbl[i].offset, 2);
		*((int *)value) = (int)wd;
		break;

	case STRING_T:
		strcpy( (char *)value, (const char *)(((long)pMibTbl) + pTbl[i].offset) );
		break;

	case BYTE5_T:
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), 5);
		break;

	case BYTE6_T:
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), 6);
		break;

	case BYTE13_T:
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), 13);
		break;

	case DWORD_T:
		memcpy((char *)&dwd, ((char *)pMibTbl) + pTbl[i].offset, 4);
		*((unsigned int *)value) = (unsigned int)dwd;
		break;

	case BYTE_ARRAY_T:
#ifdef VOIP_SUPPORT
		if(id == MIB_VOIP_CFG){
			// rock: do nothing here, use flash voip get xxx to replace
		}
		else
#endif /*VOIP_SUPPORT*/
		if(id == MIB_L2TP_PAYLOAD)
		{
//			apmib_get(MIB_L2TP_PAYLOAD_LENGTH, (void *)&payload_len);
			memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), MAX_L2TP_BUFF_LEN);
		}
		else
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), pTbl[i].size);
		break;
	case DWORD_ARRAY_T:
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), pTbl[i].size);
		break;
	case IA_T:
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), 4);
		break;

#ifdef HOME_GATEWAY
#ifdef CONFIG_IPV6
	case RADVDPREFIX_T:
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), pTbl[i].size);
		break;
		
	case DNSV6_T:
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), pTbl[i].size);
		break;
	case DHCPV6S_T:
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), pTbl[i].size);
		break;
	case DHCPV6C_T:
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), pTbl[i].size);
		break;
	case ADDR6_T:
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), pTbl[i].size);
		break;
	case ADDRV6_T:
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), pTbl[i].size);
		break;
	case TUNNEL6_T:
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), pTbl[i].size);
		break;	
#endif
#endif

#ifdef VOIP_SUPPORT
	case VOIP_T:
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), pTbl[i].size);
		//printf("apimb: mib_get MIB_VOIP_CFG, do nothing here\n");
		break;
#endif
#ifdef RTK_CAPWAP
	case CAPWAP_ALL_WLANS_CONFIG_T:
		memcpy( (unsigned char *)value, (unsigned char *)(((long)pMibTbl) + pTbl[i].offset), pTbl[i].size);
		printf("all wlan config size = %d\n", pTbl[i].size);
		break;
#endif

#ifndef MIB_TLV
	case WLAC_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
		index = (int)( *((unsigned char *)value));
		return get_linkchain(&wlanMacChain[wlan_idx][vwlan_idx], (char *)value, index );

//#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_) // below code copy above ACL code
	case MESH_ACL_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
		index = (int)( *((unsigned char *)value));
		return get_linkchain(&meshAclChain, (char *)value, index );
//#endif	// CONFIG_RTK_MESH && _MESH_ACL_ENABLE_

	case WDS_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
		index = (int)( *((unsigned char *)value));
		return get_linkchain(&wdsChain[wlan_idx][vwlan_idx], (char *)value, index );
#if defined(WIFI_SIMPLE_CONFIG) && defined(CONFIG_APP_TR069)
	case WSC_ER_ARRAY_T:	
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
		index = (int)( *((unsigned char *)value));
		return get_linkchain(&wscERChain[wlan_idx][vwlan_idx], (char *)value, index );
#endif
	case SCHEDULE_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
    index = (int)( *((unsigned char *)value));
 		return get_linkchain(&scheduleRuleChain[wlan_idx][vwlan_idx], (char *)value, index ); 		


#ifdef HOME_GATEWAY
	case PORTFW_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
                index = (int)( *((unsigned char *)value));
 		return get_linkchain(&portFwChain, (char *)value, index );

	case IPFILTER_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
		index = (int)( *((unsigned char *)value));
 		return get_linkchain(&ipFilterChain, (char *)value, index );

	case PORTFILTER_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
                index = (int)( *((unsigned char *)value));
 		return get_linkchain(&portFilterChain, (char *)value, index );

	case MACFILTER_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
                index = (int)( *((unsigned char *)value));
 		return get_linkchain(&macFilterChain, (char *)value, index );

	case URLFILTER_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
                index = (int)( *((unsigned char *)value));
 		return get_linkchain(&urlFilterChain, (char *)value, index );

	case TRIGGERPORT_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
                index = (int)( *((unsigned char *)value));
 		return get_linkchain(&triggerPortChain, (char *)value, index );

#if (defined(GW_QOS_ENGINE) || defined(QOS_BY_BANDWIDTH)) && !defined (MULTI_WAN_QOS)
	case QOS_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
                index = (int)( *((unsigned char *)value));
 		return get_linkchain(&qosChain, (char *)value, index );
#endif
#ifdef ROUTE_SUPPORT
	case STATICROUTE_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
                index = (int)( *((unsigned char *)value));
 		return get_linkchain(&staticRouteChain, (char *)value, index );
#endif
#ifdef CHECK_IPV6_DHCP_OPTION
		case DHCP6_OPTION16_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_sem_unlock();
#endif	
				index = (int)( *((unsigned char *)value));
		return get_linkchain(&dhcp6Option16Chain, (char *)value, index );
			
		case DHCP6_OPTION17_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_sem_unlock();
#endif	
				index = (int)( *((unsigned char *)value));
		return get_linkchain(&dhcp6Option17Chain, (char *)value, index );

#endif
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	case CAP_PORTAL_ALLOW_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
                index = (int)( *((unsigned char *)value));
 		return get_linkchain(&capPortalAllowChain, (char *)value, index );
#endif

#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	case DHCP_SERVER_OPTION_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
                index = (int)( *((unsigned char *)value));
 		return get_linkchain(&dhcpServerOptionChain, (char *)value, index );

	case DHCP_CLIENT_OPTION_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
                index = (int)( *((unsigned char *)value));
 		return get_linkchain(&dhcpClientOptionChain, (char *)value, index );

	case DHCPS_SERVING_POOL_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
                index = (int)( *((unsigned char *)value));
 		return get_linkchain(&dhcpsServingPoolChain, (char *)value, index );
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */

#ifdef VPN_SUPPORT
	case IPSECTUNNEL_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
                index = (int)( *((unsigned char *)value));
 		return get_linkchain(&ipsecTunnelChain, (char *)value, index );
#endif

#endif /*HOME_GATEWAY*/
#ifdef TLS_CLIENT
	case CERTROOT_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
                index = (int)( *((unsigned char *)value));
 		return get_linkchain(&certRootChain, (char *)value, index );
	case CERTUSER_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
                index = (int)( *((unsigned char *)value));
 		return get_linkchain(&certUserChain, (char *)value, index ); 		
#endif
	case DHCPRSVDIP_ARRY_T:	
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
		index = (int)( *((unsigned char *)value));
 		return get_linkchain(&dhcpRsvdIpChain, (char *)value, index );

#if defined(VLAN_CONFIG_SUPPORTED)
	case VLANCONFIG_ARRAY_T:	
#if CONFIG_APMIB_SHARED_MEMORY == 1
	apmib_sem_unlock();
#endif
		index = (int)( *((unsigned char *)value));
 		return get_linkchain(&vlanConfigChain, (char *)value, index ); 		
#endif 	

#ifdef RTK_CAPWAP
	case CAPWAP_WTP_CONFIG_ARRAY_T:	
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
		index = (int)( *((unsigned char *)value));
 		return get_linkchain(&capwapWtpConfigChain, (char *)value, index ); 		
#endif

#ifdef CONFIG_RTL_AIRTIME
	case AIRTIME_ARRAY_T:
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif	
		index = (int)( *((unsigned char *)value));
		return get_linkchain(wlan_idx==0?(&airtimeChain):(&airtimeChain2), (char *)value, index );
#endif /* CONFIG_RTL_AIRTIME */


#endif /*no def MIB_TLV*/
	default:
		break;
	}
#if CONFIG_APMIB_SHARED_MEMORY == 1
	apmib_sem_unlock();
#endif
	return 1;
}

#ifdef RTK_REINIT_SUPPORT

int apmib_rtkReinit_get(int id, void* value,int wlanIdx,int vwlanIdx,int wanIdx)
{
	int retVal=0;
	apmib_save_Idx();
	wlan_idx=wlanIdx;
	vwlan_idx=vwlanIdx;
#ifdef MULTI_WAN_SUPPORT	
	wan_idx=wanIdx;
#endif
	retVal=apmib_get(id,value);
	apmib_recov_Idx();
	return retVal;
}
void apmib_rtkReinit_setIdx(int wlanIdx,int vwlanIdx,int wanIdx)
{
	if(wlanIdx>=0)
		wlan_idx=wlanIdx;
	if(vwlanIdx>=0)
		vwlan_idx=vwlanIdx;
}
void dumpMibChangeListInfo(MibChangeNode_tp node)
{
	int i=0;
	if(!node)
		return;
	fprintf(stderr,"\nid=%d length=%d mibChangeNode_wlan_idx=%d mibChangeNode_vwlan_idx=%d mibChangeNode_wan_idx=%d change_func_id=%d\n",
		node->id,node->length,node->mibChangeNode_wlan_idx,node->mibChangeNode_vwlan_idx,node->mibChangeNode_wan_idx,node->change_func_id);
	fprintf(stderr,"\t  value=");
	for(i=0;i<node->length;i++)
		fprintf(stderr,"%02x ",node->value[i]);
	fprintf(stderr,"\n");
}

int send_reinit_event_mibchange(MibChangeNode_tp pMibChangeNode)
{
	//char buff[512]={0};
	char *tmpBuf=NULL;
	char *buff=NULL;
	int i=0;
	int buf_len=strlen("id=%x,name=%s,length=%x,value=")+pMibChangeNode->length*3+strlen(",mibChangeNode_wlan_idx=%x,mibChangeNode_vwlan_idx=%x,mibChangeNode_wan_idx=%x,change_func_id=%x")+sizeof(int)*2*6+64;
	int tmpBuf_len=buf_len+strlen("reinitCli -e %d -l %d -d %s ")+20;
	if(pMibChangeNode->change_func_id==0)
		return 0;
	buff=(char*)malloc(buf_len);
	tmpBuf=(char*)malloc(tmpBuf_len);
	if(!buff||!tmpBuf)
	{
		fprintf(stderr,"malloc %d fail %s:%d\n",buf_len,__FUNCTION__,__LINE__);
		return -1;
	}
	bzero(buff,buf_len);
	bzero(tmpBuf,tmpBuf_len);
	sprintf(buff,"id=%x,name=%s,length=%x,value=",pMibChangeNode->id,pMibChangeNode->mib_name,pMibChangeNode->length);	
	//if(fwrite(buff,strlen(buff),1,fp)!=1) goto REINIT_ARGS_WRITE_FAIL;

	//bzero(buff,sizeof(buff));
	for(i=0;i<pMibChangeNode->length;i++)
	{
		sprintf(tmpBuf,"%02x-",pMibChangeNode->value[i]);
		strcat(buff,tmpBuf);
	}

	//if(fwrite(buff,strlen(buff),1,fp)!=1) goto REINIT_ARGS_WRITE_FAIL;

	sprintf(tmpBuf,",mibChangeNode_wlan_idx=%x,mibChangeNode_vwlan_idx=%x,mibChangeNode_wan_idx=%x,change_func_id=%x",pMibChangeNode->mibChangeNode_wlan_idx,pMibChangeNode->mibChangeNode_vwlan_idx,pMibChangeNode->mibChangeNode_wan_idx,pMibChangeNode->change_func_id);	
	strcat(buff,tmpBuf);
	//fprintf(stderr,"%s:%d %s changes\n",__FUNCTION__,__LINE__,pMibChangeNode->mib_name);

	sprintf(tmpBuf,"reinitCli -e %d -l %d -d %s ",REINIT_EVENT_MIBCHANGE,buf_len,buff);

	system(tmpBuf);
	free(buff);
	free(tmpBuf);
	return 0;

}
#endif
////////////////////////////////////////////////////////////////////////////////
int apmib_getDef(int id, void *value)
{
	int ret;
	APMIB_Tp saveMib=pMib;

	pMib = pMibDef;
	ret = apmib_get(id, value);
	pMib = saveMib;
	return ret;
}


////////////////////////////////////////////////////////////////////////////////
int apmib_set(int id, void *value)
{
	int i, ret=1;
	void *pMibTbl;
	mib_table_entry_T *pTbl;
	unsigned char ch;
	unsigned short wd;
	unsigned long dwd;
	unsigned char* tmp;
	int max_chan_num=MAX_2G_CHANNEL_NUM_MIB;
#ifdef RTK_REINIT_SUPPORT
	int no_reinit_flag=0;
#endif

#ifdef MIB_TLV
	//unsigned int offset=0;
	unsigned int mib_num_id=0;
	unsigned int num;
	unsigned int id_orig;
	#if defined(MIB_MOD_TBL_ENTRY)
	unsigned int mod_tbl=0;
	#endif
	


#ifdef MULTI_WAN_SUPPORT
	if(id==MIB_WANIFACE_CURRENT_IDX)
	{
		wan_idx = *(int*)value;
		if(wan_idx<WAN1 || wan_idx > WANIFACE_NUM)
		{
			fprintf(stderr,"wrong wan_index %d!!\n",wan_idx);
			return 0;
		}
		return 1;
	}
		
#endif
#endif

#ifdef RTK_REINIT_SUPPORT
	if(id&MIB_NO_REINIT_FLAG){
		no_reinit_flag=1;
		id-=MIB_NO_REINIT_FLAG;
	}
	
	MibChangeNode_t changeNode={0};
	if(!no_reinit_flag)
	{
		changeNode.id=id;
		changeNode.mibChangeNode_wlan_idx=wlan_idx;
		changeNode.mibChangeNode_vwlan_idx=vwlan_idx;
#ifdef MULTI_WAN_SUPPORT
		changeNode.mibChangeNode_wan_idx= wan_idx;
#endif
	}
#endif

#ifdef MIB_TLV
#else
#if CONFIG_APMIB_SHARED_MEMORY == 1
	apmib_sem_lock();
#endif
#endif

#ifdef MIB_TLV
	id_orig = id;
	if( id_orig & MIB_ADD_TBL_ENTRY)
	{
		id=((id_orig & MIB_ID_MASK)-1 )|(MIB_TABLE_LIST);
		mib_num_id=(id_orig & MIB_ID_MASK)-2;		

	}
	else if(id_orig & MIB_DEL_TBL_ENTRY)
	{
	#if defined(MIB_MOD_TBL_ENTRY)
		if (id_orig & MIB_MOD_TBL_ENTRY) {
			id_orig &= ~MIB_MOD_TBL_ENTRY;
			id = id_orig;
			mod_tbl = 1;
		}
	#endif

		id=((id_orig & MIB_ID_MASK)-2 )|(MIB_TABLE_LIST);
		mib_num_id=(id_orig & MIB_ID_MASK)-3;	
	}
	else if(id_orig & MIB_DELALL_TBL_ENTRY)
	{
		id=((id_orig & MIB_ID_MASK)-3 )|(MIB_TABLE_LIST);
		mib_num_id=(id_orig & MIB_ID_MASK)-4;	
	}
#else

	if (id == MIB_WLAN_AC_ADDR_ADD) {
		ret = add_linkchain(&wlanMacChain[wlan_idx][vwlan_idx], (char *)value);
		if ( ret )
			pMib->wlan[wlan_idx][vwlan_idx].acNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_WLAN_AC_ADDR_DEL) {
		ret = delete_linkchain(&wlanMacChain[wlan_idx][vwlan_idx], (char *)value);
		if ( ret )
			pMib->wlan[wlan_idx][vwlan_idx].acNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_WLAN_AC_ADDR_DELALL) {
		delete_all_linkchain(&wlanMacChain[wlan_idx][vwlan_idx]);
		pMib->wlan[wlan_idx][vwlan_idx].acNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}

#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_) // below code copy above ACL code
	if (id == MIB_MESH_ACL_ADDR_ADD) {
		ret = add_linkchain(&meshAclChain, (char *)value);
		if ( ret )
			pMib->meshAclNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_MESH_ACL_ADDR_DEL) {
		ret = delete_linkchain(&meshAclChain, (char *)value);
		if ( ret )
			pMib->meshAclNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_MESH_ACL_ADDR_DELALL) {
		delete_all_linkchain(&meshAclChain);
		pMib->meshAclNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
#endif	// CONFIG_RTK_MESH && _MESH_ACL_ENABLE_

	if (id == MIB_WLAN_WDS_ADD) {
		ret = add_linkchain(&wdsChain[wlan_idx][vwlan_idx], (char *)value);
		if ( ret )
			pMib->wlan[wlan_idx][vwlan_idx].wdsNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_WLAN_WDS_DEL) {
		ret = delete_linkchain(&wdsChain[wlan_idx][vwlan_idx], (char *)value);
		if ( ret )
			pMib->wlan[wlan_idx][vwlan_idx].wdsNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_WLAN_WDS_DELALL) {
		delete_all_linkchain(&wdsChain[wlan_idx][vwlan_idx]);
		pMib->wlan[wlan_idx][vwlan_idx].wdsNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
#if defined(WIFI_SIMPLE_CONFIG) && defined(CONFIG_APP_TR069)
// wsc ER Mib
	if (id == MIB_WLAN_WSC_ER_ADD) {
		ret = add_linkchain(&wscERChain[wlan_idx][vwlan_idx], (char *)value);
		if ( ret )
			pMib->wlan[wlan_idx][vwlan_idx].wscERNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_WLAN_WSC_ER_DEL) {
		ret = delete_linkchain(&wscERChain[wlan_idx][vwlan_idx], (char *)value);
		if ( ret )
			pMib->wlan[wlan_idx][vwlan_idx].wscERNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_WLAN_WSC_ER_DELALL) {
		delete_all_linkchain(&wscERChain[wlan_idx][vwlan_idx]);
		pMib->wlan[wlan_idx][vwlan_idx].wscERNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
#endif
//Schedule Mib
	if (id == MIB_WLAN_SCHEDULE_ADD) {
		ret = add_linkchain(&scheduleRuleChain[wlan_idx][vwlan_idx], (char *)value);
		if ( ret )
			pMib->wlan[wlan_idx][vwlan_idx].scheduleRuleNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	//### Edit by sen_liu 2011.5.13 I believe it is a clerical error
	//if (id == MIB_SCHEDULE_DEL) {
	if (id == MIB_WLAN_SCHEDULE_DEL) {
	//### end
		ret = delete_linkchain(&scheduleRuleChain[wlan_idx][vwlan_idx], (char *)value);
		if ( ret )
			pMib->wlan[wlan_idx][vwlan_idx].scheduleRuleNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_WLAN_SCHEDULE_DELALL) {
		delete_all_linkchain(&scheduleRuleChain[wlan_idx][vwlan_idx]);
		pMib->wlan[wlan_idx][vwlan_idx].scheduleRuleNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}

#if defined(VLAN_CONFIG_SUPPORTED)
	if (id == MIB_VLANCONFIG_ADD || id == MIB_VLANCONFIG_DEL) {
		int entryNum=0,i;
		VLAN_CONFIG_T entry;
		VLAN_CONFIG_Tp entry_new;
		entry_new = (VLAN_CONFIG_Tp)value;
		apmib_get(MIB_VLANCONFIG_TBL_NUM, (void *)&entryNum);
		for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
			apmib_get(MIB_VLANCONFIG_TBL, (void *)&entry);
			if(!strcmp(entry.netIface, entry_new->netIface)){
				update_linkchain(VLANCONFIG_ARRAY_T, &entry, entry_new, sizeof(VLAN_CONFIG_T));
				break;
			}
		}
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}

#if defined(VLAN_CONFIG_SUPPORTED)
	if (id == MIB_VLANCONFIG_DELALL) {
		int entryNum=0,i;
		VLAN_CONFIG_T entry;
		VLAN_CONFIG_T entry_new;
		apmib_get(MIB_VLANCONFIG_TBL_NUM, (void *)&entryNum);

		for (i=1; i<=entryNum; i++) {
		*((char *)&entry) = (char)i;
			apmib_get(MIB_VLANCONFIG_TBL, (void *)&entry);
			memcpy(&entry_new, &entry, sizeof(VLAN_CONFIG_T));
			entry_new.enabled=0;
			update_linkchain(VLANCONFIG_ARRAY_T, &entry, &entry_new, sizeof(VLAN_CONFIG_T));
		}
#endif		
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}

#endif


#ifdef RTK_CAPWAP
	if (id == MIB_CAPWAP_WTP_CONFIG_ADD) {
		ret = add_linkchain(&capwapWtpConfigChain, (char *)value);
		if ( ret ) {
			pMib->wtpConfigNum++;
		}
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_CAPWAP_WTP_CONFIG_DEL) {
		ret = delete_linkchain(&capwapWtpConfigChain, (char *)value);
		if ( ret )
			pMib->wtpConfigNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_CAPWAP_WTP_CONFIG_DELALL) {
		delete_all_linkchain(&capwapWtpConfigChain);
		pMib->wtpConfigNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
#endif	// RTK_CAPWAP

#ifdef HOME_GATEWAY
	if (id == MIB_PORTFW_ADD) {
		ret = add_linkchain(&portFwChain, (char *)value);
		if ( ret )
			pMib->portFwNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_PORTFW_DEL) {
		ret = delete_linkchain(&portFwChain, (char *)value);
		if ( ret )
			pMib->portFwNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_PORTFW_DELALL) {
		delete_all_linkchain(&portFwChain);
		pMib->portFwNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}

	if (id == MIB_IPFILTER_ADD) {
		ret = add_linkchain(&ipFilterChain, (char *)value);
		if ( ret )
			pMib->ipFilterNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_IPFILTER_DEL) {
		ret = delete_linkchain(&ipFilterChain, (char *)value);
		if ( ret )
			pMib->ipFilterNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_IPFILTER_DELALL) {
		delete_all_linkchain(&ipFilterChain);
		pMib->ipFilterNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}

	if (id == MIB_PORTFILTER_ADD) {
		ret = add_linkchain(&portFilterChain, (char *)value);
		if ( ret )
			pMib->portFilterNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_PORTFILTER_DEL) {
		ret = delete_linkchain(&portFilterChain, (char *)value);
		if ( ret )
			pMib->portFilterNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_PORTFILTER_DELALL) {
		delete_all_linkchain(&portFilterChain);
		pMib->portFilterNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}

	if (id == MIB_MACFILTER_ADD) {
		ret = add_linkchain(&macFilterChain, (char *)value);
		if ( ret )
			pMib->macFilterNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_MACFILTER_DEL) {
		ret = delete_linkchain(&macFilterChain, (char *)value);
		if ( ret )
			pMib->macFilterNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_MACFILTER_DELALL) {
		delete_all_linkchain(&macFilterChain);
		pMib->macFilterNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}

	if (id == MIB_URLFILTER_ADD) {
		ret = add_linkchain(&urlFilterChain, (char *)value);
		if ( ret )
			pMib->urlFilterNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_URLFILTER_DEL) {
		ret = delete_linkchain(&urlFilterChain, (char *)value);
		if ( ret )
			pMib->urlFilterNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_URLFILTER_DELALL) {
		delete_all_linkchain(&urlFilterChain);
		pMib->urlFilterNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}

	if (id == MIB_TRIGGERPORT_ADD) {
		ret = add_linkchain(&triggerPortChain, (char *)value);
		if ( ret )
			pMib->triggerPortNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_TRIGGERPORT_DEL) {
		ret = delete_linkchain(&triggerPortChain, (char *)value);
		if ( ret )
			pMib->triggerPortNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_TRIGGERPORT_DELALL) {
		delete_all_linkchain(&triggerPortChain);
		pMib->triggerPortNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}

#ifdef CONFIG_RTL_AIRTIME//apmib_set
	if (id == MIB_AIRTIME_ADD) {
		ret = add_linkchain(&airtimeChain, (char *)value);
		if ( ret )
			pMib->airTimeNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_AIRTIME_DEL) {
		ret = delete_linkchain(&airtimeChain, (char *)value);
		if ( ret )
			pMib->airTimeNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_AIRTIME_DELALL) {
		delete_all_linkchain(&airtimeChain);
		pMib->airTimeNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
	
	if (id == MIB_AIRTIME2_ADD) {
		ret = add_linkchain(&airtimeChain2, (char *)value);
		if ( ret )
			pMib->airTimeNum2++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_AIRTIME2_DEL) {
		ret = delete_linkchain(&airtimeChain2, (char *)value);
		if ( ret )
			pMib->airTimeNum2--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_AIRTIME2_DELALL) {
		delete_all_linkchain(&airtimeChain2);
		pMib->airTimeNum2 = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
#endif /* CONFIG_RTL_AIRTIME */

#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	if (id == MIB_DHCP_SERVER_OPTION_ADD) {
		ret = add_linkchain(&dhcpServerOptionChain, (char *)value);
		if ( ret )
			pMib->dhcpServerOptionNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_DHCP_SERVER_OPTION_DEL) {
		ret = delete_linkchain(&dhcpServerOptionChain, (char *)value);
		if ( ret )
			pMib->dhcpServerOptionNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_DHCP_SERVER_OPTION_DELALL) {
		delete_all_linkchain(&dhcpServerOptionChain);
		pMib->dhcpServerOptionNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}


	if (id == MIB_DHCP_CLIENT_OPTION_ADD) {
		ret = add_linkchain(&dhcpClientOptionChain, (char *)value);
		if ( ret )
			pMib->dhcpClientOptionNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_DHCP_CLIENT_OPTION_DEL) {
		ret = delete_linkchain(&dhcpClientOptionChain, (char *)value);
		if ( ret )
			pMib->dhcpClientOptionNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_DHCP_CLIENT_OPTION_DELALL) {
		delete_all_linkchain(&dhcpClientOptionChain);
		pMib->dhcpClientOptionNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}

	if (id == MIB_DHCPS_SERVING_POOL_ADD) {
		ret = add_linkchain(&dhcpsServingPoolChain, (char *)value);
		if ( ret )
			pMib->dhcpsServingPoolNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_DHCPS_SERVING_POOL_DEL) {
		ret = delete_linkchain(&dhcpsServingPoolChain, (char *)value);
		if ( ret )
			pMib->dhcpsServingPoolNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_DHCPS_SERVING_POOL_DELALL) {
		delete_all_linkchain(&dhcpsServingPoolChain);
		pMib->dhcpsServingPoolNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}	
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */


#if defined(CONFIG_CMCC_QOS)	// cmcc qos
	// class tbl
	if (id == MIB_QOS_CLASS_ADD) {
		ret = add_linkchain(&qosclassChain, (char *)value);
		if (ret)
			pMib->QosClassNum++;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_CLASS_DEL) {
		ret = delete_linkchain(&qosclassChain, (char *)value);
		if ( ret )
			pMib->QosClassNum--;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_CLASS_DELALL) {
		delete_all_linkchain(&qosclassChain);
		pMib->QosClassNum = 0;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return 1;
	}

	// type_tbl
	if (id == MIB_QOS_TYPE_ADD) {
		ret = add_linkchain(&qostypeChain, (char *)value);
		if (ret)
			pMib->QosTypeNum++;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_TYPE_DEL) {
		ret = delete_linkchain(&qostypeChain, (char *)value);
		if ( ret )
			pMib->QosTypeNum--;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_TYPE_DELALL) {
		delete_all_linkchain(&qostypeChain);
		pMib->QosTypeNum = 0;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return 1;
	}

	// rule tbl
	if (id == MIB_QOS_RULE_ADD) {
		ret = add_linkchain(&qosruleChain, (char *)value);
		if (ret)
			pMib->QosRuleNum++;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_RULE_DEL) {
		ret = delete_linkchain(&qosruleChain, (char *)value);
		if ( ret )
			pMib->QosRuleNum--;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_RULE_DELALL) {
		delete_all_linkchain(&qosruleChain);
		pMib->QosRuleNum = 0;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return 1;
	}

	//wan_port tbl
#if defined(CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT)
	if(id == MIB_QOS_WANPORT_ADD) {
		ret = add_linkchain(&qoswanPortChain,(char *)value)
		if(ret)
			pMib->QosWanPortNum++;
		#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_WANPORT_DEL) {
		ret = delete_linkchain(&qoswanPortChain, (char *)value);
		if ( ret )
			pMib->QosWanPortNum--;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_WANPORT_DELALL) {
		delete_all_linkchain(&qoswanPortChain);
		pMib->QosWanPortNum = 0;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return 1;
	}
#endif
	// queue tbl
	if (id == MIB_QOS_QUEUE_ADD) {
		ret = add_linkchain(&qosqueueChain, (char *)value);
		if (ret)
			pMib->QosQueueNum++;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_QUEUE_DEL) {
		ret = delete_linkchain(&qosqueueChain, (char *)value);
		if ( ret )
			pMib->QosQueueNum--;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_QUEUE_DELALL) {
		delete_all_linkchain(&qosqueueChain);
		pMib->QosQueueNum = 0;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return 1;
	}

	// app tbl
	if (id == MIB_QOS_APP_ADD) {
		ret = add_linkchain(&qosappChain, (char *)value);
		if (ret)
			pMib->QosAppNum++;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_APP_DEL) {
		ret = delete_linkchain(&qosappChain, (char *)value);
		if ( ret )
			pMib->QosAppNum--;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_APP_DELALL) {
		delete_all_linkchain(&qosappChain);
		pMib->QosAppNum = 0;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return 1;
	}
	
	// car tbl
	if (id == MIB_QOS_CAR_ADD) {
		ret = add_linkchain(&qoscarChain, (char *)value);
		if (ret)
			pMib->QosCarNum++;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_CAR_DEL) {
		ret = delete_linkchain(&qoscarChain, (char *)value);
		if ( ret )
			pMib->QosCarNum--;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_CAR_DELALL) {
		delete_all_linkchain(&qoscarChain);
		pMib->QosCarNum = 0;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return 1;
	}
	// car iface tbl
	if (id == MIB_QOS_CAR_IFACE_ADD) {
		ret = add_linkchain(&qoscarIfaceChain, (char *)value);
		if (ret)
			pMib->QosCarIfaceNum++;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_CAR_IFACE_DEL) {
		ret = delete_linkchain(&qoscarIfaceChain, (char *)value);
		if ( ret )
			pMib->QosCarIfaceNum--;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_CAR_IFACE_DELALL) {
		delete_all_linkchain(&qoscarIfaceChain);
		pMib->QosCarIfaceNum = 0;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
	#endif
		return 1;
	}

	// car vlan tbl
	if (id == MIB_QOS_CAR_VLAN_ADD) {
		ret = add_linkchain(&qoscarVlanChain, (char *)value);
		if (ret)
			pMib->QosCarVlanNum++;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_CAR_VLAN_DEL) {
		ret = delete_linkchain(&qoscarVlanChain, (char *)value);
		if ( ret )
			pMib->QosCarVlanNum--;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_CAR_VLAN_DELALL) {
		delete_all_linkchain(&qoscarVlanChain);
		pMib->QosCarVlanNum = 0;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
	#endif
		return 1;
	}

	// car ip tbl
	if (id == MIB_QOS_CAR_IP_ADD) {
		ret = add_linkchain(&qoscarIpChain, (char *)value);
		if (ret)
			pMib->QosCarIpNum++;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_CAR_IP_DEL) {
		ret = delete_linkchain(&qoscarIpChain, (char *)value);
		if ( ret )
			pMib->QosCarIpNum--;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
	#endif
		return ret;
	}
	if (id == MIB_QOS_CAR_IP_DELALL) {
		delete_all_linkchain(&qoscarIpChain);
		pMib->QosCarIpNum = 0;
	#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
	#endif
		return 1;
	}


#elif defined(QOS_OF_TR069) || defined(MULTI_WAN_QOS)	// 098 qos
    if (id == MIB_QOS_QUEUE_ADD) {
		ret = add_linkchain(&qosqueueChain, (char *)value);
		if ( ret )
			pMib->QosQueueNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_QOS_QUEUE_DEL) {
		ret = delete_linkchain(&qosqueueChain, (char *)value);
		if ( ret )
			pMib->QosQueueNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_QOS_QUEUE_DELALL) {
		delete_all_linkchain(&qosqueueChain);
		pMib->QosQueueNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
	
	    if (id == MIB_QOS_CLASS_ADD) {
		ret = add_linkchain(&qosclassChain, (char *)value);
		if ( ret )
			pMib->QosClassNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_QOS_CLASS_DEL) {
		ret = delete_linkchain(&qosclassChain, (char *)value);
		if ( ret )
			pMib->QosClassNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_QOS_CLASS_DELALL) {
		delete_all_linkchain(&qosclassChain);
		pMib->QosClassNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
	/*app table*/
	{
	if (id == MIB_TR098_QOS_APP_ADD)
	{
		ret = add_linkchain(&Tr098QOSAppChain, (char *)value);
		if ( ret )
			pMib->tr098_appconf_num++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_TR098_QOS_APP_DEL) {
		ret = delete_linkchain(&Tr098QOSAppChain, (char *)value);
		if ( ret )
			pMib->tr098_appconf_num--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_TR098_QOS_APP_DELALL) {
		delete_all_linkchain(&Tr098QOSAppChain);
		pMib->tr098_appconf_num = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
	}
	/*flow table*/
	{
	if (id == MIB_TR098_QOS_FLOW_ADD)
	{
		ret = add_linkchain(&Tr098QOSFlowChain, (char *)value);
		if ( ret )
			pMib->tr098_flowconf_num++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_TR098_QOS_FLOW_DEL) {
		ret = delete_linkchain(&Tr098QOSFlowChain, (char *)value);
		if ( ret )
			pMib->tr098_flowconf_num--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_TR098_QOS_FLOW_DELALL) {
		delete_all_linkchain(&Tr098QOSFlowChain);
		pMib->tr098_flowconf_num = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
	}
	//mark_qos
	   if (id == MIB_QOS_POLICER_ADD) {
		ret = add_linkchain(&qospolicerChain, (char *)value);
		if ( ret )
			pMib->QosPolicerNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_QOS_POLICER_DEL) {
		ret = delete_linkchain(&qospolicerChain, (char *)value);
		if ( ret )
			pMib->QosPolicerNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_QOS_POLICER_DELALL) {
		delete_all_linkchain(&qospolicerChain);
		pMib->QosPolicerNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}

	 if (id == MIB_QOS_QUEUESTATS_ADD) {
		ret = add_linkchain(&qosqueuestatsChain, (char *)value);
		if ( ret )
			pMib->QosQueueStatsNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_QOS_QUEUESTATS_DEL) {
		ret = delete_linkchain(&qosqueuestatsChain, (char *)value);
		if ( ret )
			pMib->QosQueueNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_QOS_QUEUESTATS_DELALL) {
		delete_all_linkchain(&qosqueuestatsChain);
		pMib->QosQueueStatsNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}

#elif defined(GW_QOS_ENGINE) || defined(QOS_BY_BANDWIDTH)	// retailor qos
	if (id == MIB_QOS_ADD) {
		ret = add_linkchain(&qosChain, (char *)value);
		if ( ret )
			pMib->qosRuleNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_QOS_DEL) {
		ret = delete_linkchain(&qosChain, (char *)value);
		if ( ret )
			pMib->qosRuleNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_QOS_DELALL) {
		delete_all_linkchain(&qosChain);
		pMib->qosRuleNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
#endif


#ifdef ROUTE_SUPPORT
	if (id == MIB_STATICROUTE_ADD) {
		ret = add_linkchain(&staticRouteChain, (char *)value);
		if ( ret )
			pMib->staticRouteNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_STATICROUTE_DEL) {
		ret = delete_linkchain(&staticRouteChain, (char *)value);
		if ( ret )
			pMib->staticRouteNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_STATICROUTE_DELALL) {
		delete_all_linkchain(&staticRouteChain);
		pMib->staticRouteNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
#endif //ROUTE
#ifdef CHECK_IPV6_DHCP_OPTION
	if (id == MIB_DHCP6_OPTION16_ADD) {
		ret = add_linkchain(&dhcp6Option16Chain, (char *)value);
		if ( ret )
			pMib->dhcp6Option16InstanceNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_DHCP6_OPTION16_DEL) {
		ret = delete_linkchain(&dhcp6Option16Chain, (char *)value);
		if ( ret )
			pMib->dhcp6Option16InstanceNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_DHCP6_OPTION16_DELALL) {
		delete_all_linkchain(&dhcp6Option16Chain);
		pMib->dhcp6Option16InstanceNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
#endif

	if (id == MIB_DHCP6_OPTION17_ADD) {
		ret = add_linkchain(&dhcp6Option17Chain, (char *)value);
		if ( ret )
			pMib->dhcp6Option17InstanceNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_DHCP6_OPTION17_DEL) {
		ret = delete_linkchain(&dhcp6Option17Chain, (char *)value);
		if ( ret )
			pMib->dhcp6Option17InstanceNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_DHCP6_OPTION17_DELALL) {
		delete_all_linkchain(&dhcp6Option17Chain);
		pMib->dhcp6Option17InstanceNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
#endif
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
	if (id == MIB_CAP_PORTAL_ALLOW_TBL_ADD) {
		ret = add_linkchain(&capPortalAllowChain, (char *)value);
		if ( ret )
			pMib->capPortalCurNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_CAP_PORTAL_ALLOW_TBL_DEL) {
		ret = delete_linkchain(&capPortalAllowChain, (char *)value);
		if ( ret )
			pMib->capPortalCurNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_CAP_PORTAL_ALLOW_TBL_DELALL) {
		delete_all_linkchain(&capPortalAllowChain);
		pMib->capPortalCurNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
		return 1;
	}
#endif //_SUPPORT_CAPTIVEPORTAL_PROFILE_

	if (id == MIB_DHCPRSVDIP_DEL) {
		ret = delete_linkchain(&dhcpRsvdIpChain, (char *)value);
		if ( ret )
			pMib->dhcpRsvdIpNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_DHCPRSVDIP_DELALL) {
		delete_all_linkchain(&dhcpRsvdIpChain);
		pMib->dhcpRsvdIpNum = 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
	if (id == MIB_DHCPRSVDIP_ADD) {
		ret = add_linkchain(&dhcpRsvdIpChain, (char *)value);
		if ( ret )
			pMib->dhcpRsvdIpNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}

#ifdef HOME_GATEWAY
#ifdef VPN_SUPPORT
	if (id == MIB_IPSECTUNNEL_ADD) {
		ret = add_linkchain(&ipsecTunnelChain, (char *)value);
		if ( ret )
			pMib->ipsecTunnelNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_IPSECTUNNEL_DEL) {
		ret = delete_linkchain(&ipsecTunnelChain, (char *)value);
		if ( ret )
			pMib->ipsecTunnelNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_IPSECTUNNEL_DELALL) {
		delete_all_linkchain(&ipsecTunnelChain);
		pMib->ipsecTunnelNum= 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
#endif
#endif
#ifdef TLS_CLIENT
	if (id == MIB_CERTROOT_ADD) {
		ret = add_linkchain(&certRootChain, (char *)value);
		if ( ret )
			pMib->certRootNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_CERTROOT_DEL) {
		ret = delete_linkchain(&certRootChain, (char *)value);
		if ( ret )
			pMib->certRootNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_CERTROOT_DELALL) {
		delete_all_linkchain(&certRootChain);
		pMib->certRootNum= 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}
	if (id == MIB_CERTUSER_ADD) {
		ret = add_linkchain(&certUserChain, (char *)value);
		if ( ret )
			pMib->certUserNum++;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_CERTUSER_DEL) {
		ret = delete_linkchain(&certUserChain, (char *)value);
		if ( ret )
			pMib->certUserNum--;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return ret;
	}
	if (id == MIB_CERTUSER_DELALL) {
		delete_all_linkchain(&certUserChain);
		pMib->certUserNum= 0;
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    apmib_sem_unlock();
#endif
		return 1;
	}	
#endif
#endif /*MIB_TLV*/

	if ( search_tbl(id, mib_table, &i) ) {
		pMibTbl = (void *)pMib;
		pTbl = mib_table;
	}
	else if ( search_tbl(id, mib_wlan_table, &i) ) {
		pMibTbl = (void *)&pMib->wlan[wlan_idx][vwlan_idx];
		pTbl = mib_wlan_table;
	}
	else if ( search_tbl(id, hwmib_table, &i) ) {
		pMibTbl = (void *)pHwSetting;
		pTbl = hwmib_table;
	}
	else if ( search_tbl(id, hwmib_wlan_table, &i) ) {
		pMibTbl = (void *)&pHwSetting->wlan[wlan_idx];
		pTbl = hwmib_wlan_table;
	}
#ifdef MULTI_WAN_SUPPORT
	else if( search_tbl(id, mib_waniface_tbl, &i) ){
		if(wan_idx<1 || wan_idx>WANIFACE_NUM)
		{
			fprintf(stderr,"wan_idx is %d, must be 1-%d\n",wan_idx,WANIFACE_NUM);
			goto SET_NOT_FIND_MIB;
		}
		pMibTbl = (void *)&pMib->WANIfaceArray[wan_idx-1];
		pTbl = mib_waniface_tbl;
	}
#endif
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
	else if ( search_tbl(id, bluetooth_hwmib_table, &i) ) {
		pMibTbl = (void *)pBlueToothHwSetting;
		pTbl = bluetooth_hwmib_table;
	}
#endif

#ifdef CUSTOMER_HW_SETTING_SUPPORT
	else if ( search_tbl(id, customer_hwmib_table, &i) ) {
		pMibTbl = (void *)pCustomerHwSetting;
		pTbl = customer_hwmib_table;
	}
#endif
#ifdef RF_DPK_SETTING_SUPPORT
	else if ( search_tbl(id, rf_dpk_mib_table, &i) ) {
		pMibTbl = (void *)pRfDpkSetting;
		pTbl = rf_dpk_mib_table;
	}
#endif /* RF_DPK_SETTING_SUPPORT */
	else {
SET_NOT_FIND_MIB:	

		printf("id not found\n");
		return 0;	
	}

#ifdef MIB_TLV
#ifdef RTK_REINIT_SUPPORT

	if(!no_reinit_flag)
	{
		if(pTbl[i].type < TABLE_LIST_T)
		{//for no table item, check whether need to change
			char* orig_value=NULL;
			int valueSize=pTbl[i].total_size;
			if(pTbl[i].type==BYTE_T||pTbl[i].type==WORD_T)
			{
				valueSize=sizeof(int);
			}
			orig_value=(char*)malloc(valueSize);
			bzero(orig_value,valueSize);
			apmib_get(pTbl[i].id,orig_value);
			//printf("id=%d orig_value=%u%u%u%u new_value=%u%u%u%u size=%u\n",pTbl[i].id,orig_value[0],orig_value[1],orig_value[2],orig_value[3],((char*)value)[0],((char*)value)[1],((char*)value)[2],((char*)value)[3],pTbl[i].total_size);
			if(memcmp(orig_value,value,valueSize)==0||
				pTbl[i].type==STRING_T && strcmp(orig_value,(char*)value)==0)
			{
				//printf("id=%d orig_value=%u%u%u%u new_value=%u%u%u%u size=%u\n",pTbl[i].id,orig_value[0],orig_value[1],orig_value[2],orig_value[3],((char*)value)[0],((char*)value)[1],((char*)value)[2],((char*)value)[3],pTbl[i].total_size);
				free(orig_value);
				return 1;
			}
			if(pTbl[i].type==STRING_T)
			{//string end with '\0', needn't copy all
				changeNode.length=strnlen((char*)orig_value,pTbl[i].total_size)+1;
			}
			free(orig_value);
		}
		else{
			if(mod_tbl==1){//MIB_WANIFACE_MOD
			//	printf("----------------%s:%d\n",__FUNCTION__,__LINE__);
				if(memcmp((char*)value,((char*)value)+pTbl[i].unit_size,pTbl[i].unit_size)==0)//nothing changed
					return 1;
				//printf("-------------------%s:%d\n",__FUNCTION__,__LINE__);
			}
		}

	//for table operation, set id to table id
//	printf("%s:%d id=%x\n",__FUNCTION__,__LINE__,changeNode.id);
		if(changeNode.id & MIB_ADD_TBL_ENTRY) 
		{
			changeNode.id-=(MIB_ADD_TBL_ENTRY+1);
			changeNode.id|=MIB_TABLE_LIST;
		}else
		if((changeNode.id & MIB_DEL_TBL_ENTRY) && (changeNode.id & MIB_MOD_TBL_ENTRY)) 
		{
			changeNode.id-=(MIB_DEL_TBL_ENTRY+MIB_MOD_TBL_ENTRY+2);
			changeNode.id|=MIB_TABLE_LIST;
		}else	
		if(changeNode.id & MIB_DEL_TBL_ENTRY) 
		{
			changeNode.id-=MIB_DEL_TBL_ENTRY+2;
			changeNode.id|=MIB_TABLE_LIST;
		}else
		if(changeNode.id & MIB_DELALL_TBL_ENTRY) 
		{
			changeNode.id-=MIB_DELALL_TBL_ENTRY+3;
			changeNode.id|=MIB_TABLE_LIST;
		}
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_lock();
#endif
//		printf("%s:%d id=%x funcid=%d %d\n",__FUNCTION__,__LINE__,changeNode.id,pTbl[i].reinit_func_id,pTbl[i].decision_func_id);

		if(changeNode.id & MIB_TABLE_LIST)//table value not save, and all will treat change
			changeNode.length=0;
		else
			changeNode.length=pTbl[i].total_size;
		if(pTbl!=mib_wlan_table)
		{//only wlan mib need wlan idx
			changeNode.mibChangeNode_vwlan_idx=changeNode.mibChangeNode_wlan_idx=0;
		}
		
		changeNode.value=(char*)malloc(changeNode.length);
		memcpy(changeNode.mib_name,pTbl[i].name,sizeof(pTbl[i].name));
		
		memcpy(changeNode.value,(unsigned char *)(((long)pMibTbl) + pTbl[i].offset),changeNode.length);
		if(pTbl[i].decision_func_id!=0 && pTbl[i].decision_func_id!=DECISION_NULL_FUNC_ID)
			changeNode.change_func_id= pTbl[i].decision_func_id;
		else if(pTbl[i].reinit_func_id!=0 && pTbl[i].reinit_func_id!=REINIT_NULL_FUNC_ID)
			changeNode.change_func_id= pTbl[i].reinit_func_id;
		else 
			changeNode.change_func_id=0;
		//dumpMibChangeListInfo(&changeNode);
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_sem_unlock();
#endif

		if(send_reinit_event_mibchange(&changeNode)<0)
		{
			printf("send_reinit_event_mibchange fail!\n");
		}
		
		free(changeNode.value);
	}

#endif

	if(pTbl[i].type > TABLE_LIST_T)
	{		
		apmib_get(mib_num_id,&num);
		
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_lock();
#endif
		if(id_orig & MIB_ADD_TBL_ENTRY)
		{
			ret= add_tblentry((void *)pMibTbl,pTbl[i].offset,num,&pTbl[i],(void *)value);
			if(ret)
				num++;
		}
		else if(id_orig & MIB_DEL_TBL_ENTRY)
		{
		#if defined(MIB_MOD_TBL_ENTRY)
			if (mod_tbl == 1) {
				ret= mod_tblentry((void *)pMibTbl,pTbl[i].offset,num,&pTbl[i],(void *)value);
			}
			else 
		#endif
			{
			ret= delete_tblentry((void *)pMibTbl,pTbl[i].offset,num,&pTbl[i],(void *)value);
			if(ret)
				num--;
			}
		}
		else if(id_orig & MIB_DELALL_TBL_ENTRY)
		{
			ret= delete_all_tblentry((void *)pMibTbl,pTbl[i].offset,num,&pTbl[i]);
			if(ret)
				num=0;
		}
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
		if(ret)
			apmib_set(mib_num_id,&num);		
		//printf("num %d ret %d\n",num,ret);
		return ret;
	}
#endif

#ifdef MIB_TLV
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_sem_lock();
#endif
#endif

	switch (pTbl[i].type) {
	case BYTE_T:
//		*((unsigned char *)(((long)pMibTbl) + pTbl[i].offset)) = (unsigned char)(*((int *)value));
		ch = (unsigned char)(*((int *)value));
		memcpy( ((char *)pMibTbl) + pTbl[i].offset, &ch, 1);
		break;

	case WORD_T:
//		*((unsigned short *)(((long)pMibTbl) + pTbl[i].offset)) = (unsigned short)(*((int *)value));
		wd = (unsigned short)(*((int *)value));
		memcpy( ((char *)pMibTbl) + pTbl[i].offset, &wd, 2);
		break;

	case STRING_T:
		if ( strlen(value)+1 > pTbl[i].size )
		{
#if CONFIG_APMIB_SHARED_MEMORY == 1
	    		apmib_sem_unlock();
#endif		
			return 0;
		}
		if (value==NULL || strlen(value)==0)
			*((char *)(((long)pMibTbl) + pTbl[i].offset)) = '\0';
		else
			strcpy((char *)(((long)pMibTbl) + pTbl[i].offset), (char *)value);
		break;

	case BYTE5_T:
		memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value, 5);
		break;

	case BYTE6_T:
		memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value, 6);
		break;

	case BYTE13_T:
		memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value, 13);
		break;

	case DWORD_T:
		dwd = (unsigned long)(*((unsigned int *)value));
		memcpy( ((char *)pMibTbl) + pTbl[i].offset, &dwd, 4);
		break;
	case BYTE_ARRAY_T:
		tmp = (unsigned char*) value;
#ifdef VPN_SUPPORT
		if(id == MIB_IPSEC_RSA_FILE){
                        memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value, MAX_RSA_FILE_LEN);
		}
		else
#endif

		if(id == MIB_L2TP_PAYLOAD){
//			  apmib_get(MIB_L2TP_PAYLOAD_LENGTH, (void *)&payload_len);
			  memcpy((unsigned char *)((( long)pMibTbl) + pTbl[i].offset), (unsigned char *)value, MAX_L2TP_BUFF_LEN);
		}
		else
			
#ifdef VOIP_SUPPORT
#ifdef VOIP_SUPPORT_TLV_CFG
	// do nothing
#else
		if(id == MIB_VOIP_CFG){
			printf("apimb: mib_set MIB_VOIP_CFG\n");

			memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value, pTbl[i].size);

		}
		else
#endif
#endif /*VOIP_SUPPORT*/
#ifdef CONFIG_8021Q_VLAN_SUPPORTED
		if(id == MIB_VLAN_PVID_ARRAY)
		{
			memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value, pTbl[i].size);
		}else
#endif		
		{
#if defined(CONFIG_RTL_8196B)
			max_chan_num = (id == MIB_HW_TX_POWER_CCK)? MAX_CCK_CHAN_NUM: MAX_OFDM_CHAN_NUM ;
#elif defined(CONFIG_RTL_8198C)||defined(CONFIG_RTL_8196C) || defined(CONFIG_RTL_8198) || defined(CONFIG_RTL_819XD) || defined(CONFIG_RTL_8196E) || defined(CONFIG_RTL_8197F) || defined(CONFIG_RTL_8197G)
                        if((id >= MIB_HW_TX_POWER_CCK_A &&  id <=MIB_HW_TX_POWER_DIFF_OFDM) || (id >= MIB_HW_TX_POWER_TSSI_CCK_A &&  id <=MIB_HW_TX_POWER_TSSI_HT40_1S_D))
				max_chan_num = MAX_2G_CHANNEL_NUM_MIB;
			else if((id >= MIB_HW_TX_POWER_5G_HT40_1S_A &&  id <=MIB_HW_TX_POWER_TSSI_5G_HT40_1S_D)	
#if defined(CONFIG_WLAN_HAL_8814AE)
					||(id >= MIB_HW_TX_POWER_5G_HT40_1S_C &&  id <=MIB_HW_TX_POWER_5G_HT40_1S_D)
#endif
			)
				max_chan_num = MAX_5G_CHANNEL_NUM_MIB;         
#endif

#if defined(CONFIG_RTL_8812_SUPPORT) || defined(CONFIG_WLAN_HAL_8822BE) || defined(CONFIG_WLAN_HAL_8822CE) || defined(CONFIG_WLAN_HAL_8812FE)  || defined(CONFIG_WLAN_HAL_8814BE)
			if(((id >= MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_A) && (id <= MIB_HW_TX_POWER_DIFF_OFDM4T_CCK4T_A)) 
				|| ((id >= MIB_HW_TX_POWER_DIFF_20BW1S_OFDM1T_B) && (id <= MIB_HW_TX_POWER_DIFF_OFDM4T_CCK4T_B)) )
				max_chan_num = MAX_2G_CHANNEL_NUM_MIB;

			if(((id >= MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_A) && (id <= MIB_HW_TX_POWER_DIFF_5G_80BW4S_160BW4S_A)) 
				|| ((id >= MIB_HW_TX_POWER_DIFF_5G_20BW1S_OFDM1T_B) && (id <= MIB_HW_TX_POWER_DIFF_5G_80BW4S_160BW4S_B)) )
				max_chan_num = MAX_5G_DIFF_NUM;
#endif

    #ifdef RF_DPK_SETTING_SUPPORT
		if ((id >= MIB_RF_DPK_PWSF_2G_A) && (id <= MIB_RF_DPK_PWSF_2G_B)) {
            memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)(value+1), PWSF_2G_LEN);
        } else
    #endif
			if(tmp[0]==2){
				if(tmp[3] == 0xff){ // set one channel value
					memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset + (long)tmp[1] -1), (unsigned char *)(tmp+2), 1);
				}
			}else{
					memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)(value+1), max_chan_num);
				}		
		}
		break;
	case DWORD_ARRAY_T:
		memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value, pTbl[i].size);
		break;
	case IA_T:
		memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value,  4);
		break;
#ifdef HOME_GATEWAY
#ifdef CONFIG_IPV6
	case RADVDPREFIX_T:
		memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value,  sizeof(radvdCfgParam_t));
		break;

	case DNSV6_T:
		memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value,  sizeof(dnsv6CfgParam_t));
		break;
	case DHCPV6S_T:
		memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value,  sizeof(dhcp6sCfgParam_t));
		break;
	case DHCPV6C_T:
		memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value,  sizeof(dhcp6cCfgParam_t));
		break;
	case ADDR6_T:
		memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value,  sizeof(addrIPv6CfgParam_t));
		break;
	case ADDRV6_T:
		memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value,  sizeof(addr6CfgParam_t));
		break;
	case TUNNEL6_T:
		memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value,  sizeof(tunnelCfgParam_t));
		break;		
#endif
#endif

#ifdef VOIP_SUPPORT
	case VOIP_T:
		//tr-104 use 
		printf("apimb: mib_set MIB_VOIP_CFG\n");
		memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value, pTbl[i].size);
		break;
#endif
	
#ifdef RTK_CAPWAP
	case CAPWAP_ALL_WLANS_CONFIG_T:
//printf("babylon test mib_set MIB_CAPWAP_ALL_WLANS_CFG: pTbl[i].size=%d\n", pTbl[i].size);
		memcpy((unsigned char *)(((long)pMibTbl) + pTbl[i].offset), (unsigned char *)value, pTbl[i].size);
		break;
#endif

	case WLAC_ARRAY_T:
	
//#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)
	case MESH_ACL_ARRAY_T:
//#endif

	case WDS_ARRAY_T:
#if defined(WIFI_SIMPLE_CONFIG) && defined(CONFIG_APP_TR069)
	case WSC_ER_ARRAY_T:
#endif	
	case SCHEDULE_ARRAY_T:
#ifdef HOME_GATEWAY
	case PORTFW_ARRAY_T:
	case IPFILTER_ARRAY_T:
	case PORTFILTER_ARRAY_T:
	case MACFILTER_ARRAY_T:
	case URLFILTER_ARRAY_T:
	case TRIGGERPORT_ARRAY_T:


#if defined(CONFIG_CMCC_QOS)
#if defined(CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT)
	case QOS_WANPORT_ARRAY_T:
#endif
	case QOS_CLASS_ARRAY_T:
	case QOS_TYPE_ARRAY_T:
	case QOS_RULE_ARRAY_T:
	case QOS_QUEUE_ARRAY_T:
	case QOS_APP_ARRAY_T:
	case QOS_CAR_ARRAY_T:
	case QOS_CAR_IFACE_ARRAY_T:
	case QOS_CAR_VLAN_ARRAY_T:
	case QOS_CAR_IP_ARRAY_T:
#if defined(CONFIG_MAC_SHAPING_QOS)
	case QOS_CAR_MAC_ARRAY_T:
#endif
		
#elif defined(QOS_OF_TR069) || defined(MULTI_WAN_QOS)	// added by shirley 2014/5/20
	case QOS_CLASS_ARRAY_T:
    case QOS_QUEUE_ARRAY_T:
	case TR098_APPCONF_ARRAY_T:
    case TR098_FLOWCONF_ARRAY_T:
//mark_qos		
	case QOS_POLICER_ARRAY_T: 
    case QOS_QUEUESTATS_ARRAY_T:
#elif defined(GW_QOS_ENGINE) || defined(QOS_BY_BANDWIDTH)
	case QOS_ARRAY_T:
	
#endif
#ifdef ROUTE_SUPPORT
	case STATICROUTE_ARRAY_T:
#endif
#ifdef CHECK_IPV6_DHCP_OPTION
	case DHCP6_OPTION16_ARRAY_T:
	case DHCP6_OPTION17_ARRAY_T:
#endif
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	case DHCP_SERVER_OPTION_ARRAY_T:
	case DHCP_CLIENT_OPTION_ARRAY_T:
	case DHCPS_SERVING_POOL_ARRAY_T:
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */
#endif

#ifdef HOME_GATEWAY
#ifdef VPN_SUPPORT
	case IPSECTUNNEL_ARRAY_T:
#endif
#endif
#ifdef TLS_CLIENT
	case CERTROOT_ARRAY_T:
	case CERTUSER_ARRAY_T:
#endif
#ifdef TR181_V6_SUPPORT
#ifdef CONFIG_IPV6
	case DHCPV6C_SENDOPT_ARRAY_T:
#endif
	case DNS_CLIENT_SERVER_ARRAY_T:
#endif
	case DHCPRSVDIP_ARRY_T:	
#if defined(VLAN_CONFIG_SUPPORTED)	
	case VLANCONFIG_ARRAY_T:		
#endif
#ifdef WLAN_PROFILE
	case PROFILE_ARRAY_T:		
#endif
#ifdef RTK_CAPWAP
	case CAPWAP_WTP_CONFIG_ARRAY_T:
#endif
#ifdef CONFIG_RTL_AIRTIME
	case AIRTIME_ARRAY_T:
#endif
#ifdef CONFIG_APP_MMELINKSERVER
	case MAPPING_ARRAY_T:
#endif
#if defined(CONFIG_APP_AWIFI_SUPPORT)
	case AWIFI_TRUSTED_MAC_ARRAY_T:
	case AWIFI_TRUSTED_URL_ARRAY_T:
#endif		
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
		return 0;
	default:
		break;
	}
	
#if CONFIG_APMIB_SHARED_MEMORY == 1
    apmib_sem_unlock();
#endif

	return 1;
}


////////////////////////////////////////////////////////////////////////////////
int apmib_setDef(int id, void *value)
{
	int ret;
	APMIB_Tp saveMib=pMib;

	pMib = pMibDef;
	ret = apmib_set(id, value);
	pMib = saveMib;
	return ret;
}




#if defined(BACKUP_CURRENT_SETTING_SUPPORT) || defined(BACKUP_DEFAULT_SETTING_SUPPORT) || defined(BACKUP_HARDWARE_SETTING_SUPPORT)
char * getCompSignature(CONFIG_DATA_T settingType)
{
	switch(settingType)
	{
#ifdef BACKUP_CURRENT_SETTING_SUPPORT
		case CURRENT_SETTING:
			return COMP_CS_SIGNATURE;
		case BACKUP_CURRENT_SETTING:
			return COMP_BC_SIGNATURE;
#endif
#ifdef BACKUP_DEFAULT_SETTING_SUPPORT
		case DEFAULT_SETTING:
			return COMP_DS_SIGNATURE;
		case BACKUP_DEFAULT_SETTING:
			return COMP_BD_SIGNATURE;
			
#endif 

#ifdef	CUSTOMER_HW_SETTING_SUPPORT
		case CUSTOMER_HW_SETTING:
			return COMP_US_SIGNATURE;
#endif 

#ifdef	BLUETOOTH_HW_SETTING_SUPPORT
		case BLUETOOTH_HW_SETTING:
			return COMP_BT_SIGNATURE;
#endif 

#ifdef BACKUP_HARDWARE_SETTING_SUPPORT
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		case BACKUP_CUSTOMER_HW_SETTING:
			return COMP_BU_SIGNATURE;
#endif 
#endif 

#ifdef BACKUP_HARDWARE_SETTING_SUPPORT
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
			case BACKUP_BLUETOOTH_HW_SETTING:
				return COMP_BB_SIGNATURE;
#endif 
#endif 


	}
	return NULL;
}
#endif 
#if defined(BACKUP_CURRENT_SETTING_SUPPORT) || defined(BACKUP_DEFAULT_SETTING_SUPPORT) || defined(BACKUP_HARDWARE_SETTING_SUPPORT)
char * getSignature(CONFIG_DATA_T settingType)
{
	switch(settingType)
	{
#ifdef BACKUP_HARDWARE_SETTING_SUPPORT
	case HW_SETTING:
			return HW_SETTING_HEADER_TAG;
	case BACKUP_HW_SETTING:
		return BACKUP_HW_SETTING_HEADER_TAG;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
	case BLUETOOTH_HW_SETTING:
			return BLUETOOTH_HW_SETTING_HEADER_TAG;
	case BACKUP_BLUETOOTH_HW_SETTING:
		return BACKUP_BLUETOOTH_HW_SETTING_HEADER_TAG;
#endif 
#endif
	}
	return NULL;
}

int mib_get_flash_offset(CONFIG_DATA_T type);
unsigned int mib_get_real_len(CONFIG_DATA_T type);

//this function copy source setting to destination setting after modify compress header signature only
int resetDstSetFromSrcSet(CONFIG_DATA_T dstSetting,CONFIG_DATA_T srcSetting)
{
	unsigned char *compFile = NULL;
	COMPRESS_MIB_HEADER_T compHeader;
	unsigned int compLen;

	int srcOffset;
	char * srcSignature;
	unsigned int real_size;
	srcOffset = mib_get_flash_offset(srcSetting);
	srcSignature = getCompSignature(srcSetting);
	real_size = mib_get_real_len(srcSetting);

	int dstOffset;
	char * dstSignature;
	dstOffset = mib_get_flash_offset(dstSetting);
	dstSignature = getCompSignature(dstSetting);
	
	if(srcSignature == NULL)
	{
		printf("get Compress Signature error! [File:%s Line:%d]\n",__FILE__,__LINE__);
		return -1;
	}
	
	flash_read_withtype((char *)&compHeader, srcOffset, sizeof(COMPRESS_MIB_HEADER_T),srcSetting);
	if(memcmp(compHeader.signature, srcSignature, COMP_SIGNATURE_LEN) == 0)//if compHeader.signature is equal to srcSignature, compHeader.signature is correct
	{
		compLen = DWORD_SWAP(compHeader.compLen);
		if((compLen > 0) && (compLen <= real_size))
		{
			compFile = calloc(1, compLen+sizeof(COMPRESS_MIB_HEADER_T));
			if(compFile == NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR:compFile malloc fail! [%s-%u]",__FILE__,__LINE__);
				return -1;
			}
			flash_read_withtype((char *)compFile, srcOffset, compLen+sizeof(COMPRESS_MIB_HEADER_T),srcSetting);



			sprintf((char *)compHeader.signature, "%s", dstSignature);//modify compHeader.signature to dstSignature
			memcpy(compFile, &compHeader, sizeof(COMPRESS_MIB_HEADER_T)); 
			flash_write_withtype((char *)compFile, dstOffset, compLen+sizeof(COMPRESS_MIB_HEADER_T),dstSetting);

			free(compFile);
		}
		else
		{
			printf("compLen is error! [File:%s Line:%d]\n",__FILE__,__LINE__);
			return -1;
		}
	}
	else
	{
		printf("compHeader.signature is error! [File:%s Line:%d]\n",__FILE__,__LINE__);
		return -1;
	}
	return 0;
}

#ifdef BACKUP_HARDWARE_SETTING_SUPPORT
int resetHWDstSetFromSrcSet(CONFIG_DATA_T dstSetting,CONFIG_DATA_T srcSetting)
{
	unsigned int  compLen;
	int ver;
	int srcOffset;
	char * srcSignature;
	unsigned int real_size;
	srcOffset = mib_get_flash_offset(srcSetting);
	srcSignature = getSignature(srcSetting);
	real_size = mib_get_real_len(srcSetting);
	
	int dstOffset;
	char * dstSignature;
	dstOffset = mib_get_flash_offset(dstSetting);
	dstSignature = getSignature(dstSetting);
	char hw_setting_start[6];
	char *buff;
	int i;
		
	
	if ( flash_read_withtype ((char *)&hsHeader, srcOffset, sizeof(hsHeader),srcSetting)==0 ) {
		printf("Read %s header failed! [File:%s Line:%d]\n",srcSetting,__FILE__,__LINE__);
		return NULL;
	}
	else{
		compLen=WORD_SWAP(hsHeader.len);

		if((compLen > 0) && (compLen <= real_size))
		{
			buff = calloc(1,( compLen+sizeof(hsHeader)));	
			if ( buff ){
				memcpy((char *)hsHeader.signature, dstSignature,TAG_LEN);//modify compHeader.signature to dstSignature
				memcpy(buff, &hsHeader, sizeof(hsHeader));
				flash_read_withtype((char *)buff+sizeof(hsHeader), srcOffset+sizeof(hsHeader), compLen,srcSetting);
				flash_write_withtype((char *)buff, dstOffset, compLen+sizeof(hsHeader),dstSetting);
				free(buff);
			}else
				printf("callo buff fial! compLen+sizeof(hsHeader)=%u [File:%s Line:%d]\n",(compLen+sizeof(hsHeader)),__FILE__,__LINE__);
		}
		else{
			printf("compLen is error! [File:%s Line:%d]\n",__FILE__,__LINE__);
			return -1;
		}
	}
}
#endif 

#endif

#ifdef BACKUP_HARDWARE_SETTING_SUPPORT
int resetbackupHWSetFromHWSet(void) //reset backup_hw_setting from hw_setting.
{
	return resetHWDstSetFromSrcSet(BACKUP_HW_SETTING, HW_SETTING);
}
int resetHWSetFrombackupHWSet(void) //reset hw_setting from backup_hw_setting.
{
	return resetHWDstSetFromSrcSet(HW_SETTING, BACKUP_HW_SETTING);
}

#ifdef BLUETOOTH_HW_SETTING_SUPPORT
int resetbackupBTHWSetFromBTHWSet(void) //reset backup_bluetooth_hw setting from bluetooth_hw setting.
{
	return resetHWDstSetFromSrcSet(BACKUP_BLUETOOTH_HW_SETTING, BLUETOOTH_HW_SETTING);
}
int resetBTHWSetFrombackupBTHWSet(void) //reset bluetooth_hw setting from backup_bluetooth_hw setting.
{
	return resetHWDstSetFromSrcSet(BLUETOOTH_HW_SETTING, BACKUP_BLUETOOTH_HW_SETTING);
}
#endif 
//resetDstSetFromSrcSet is for compress mib 
#ifdef CUSTOMER_HW_SETTING_SUPPORT
int resetbackupCUHWSetFromCUHWSet(void) //reset backup_customer_hw_setting from customer_hw_setting.
{
	return resetDstSetFromSrcSet(BACKUP_CUSTOMER_HW_SETTING, CUSTOMER_HW_SETTING);
}
int resetCUHWSetFrombackupCUHWSet(void) //reset customer_hw_setting from backup_customer_hw_setting.
{
	return resetDstSetFromSrcSet(CUSTOMER_HW_SETTING, BACKUP_CUSTOMER_HW_SETTING);
}
#endif 

#endif 

#ifdef BACKUP_CURRENT_SETTING_SUPPORT
int resetBackupCurSetFromCurSet(void) //reset backup current setting from current setting.
{
	return resetDstSetFromSrcSet(BACKUP_CURRENT_SETTING, CURRENT_SETTING);
}
int resetCurSetFromBackupCurSet(void) //reset current setting from backup current setting.
{
	return resetDstSetFromSrcSet(CURRENT_SETTING, BACKUP_CURRENT_SETTING);
}
#endif

#ifdef BACKUP_DEFAULT_SETTING_SUPPORT
int resetBackupDefSetFromDefSet(void)//reset backup default setting from default setting.
{
	return resetDstSetFromSrcSet(BACKUP_DEFAULT_SETTING, DEFAULT_SETTING);
}
int resetDefSetFromBackupDefSet(void) //reset default setting from backup default setting.
{
	return resetDstSetFromSrcSet(DEFAULT_SETTING, BACKUP_DEFAULT_SETTING);
}
#endif

/* Update current used MIB into flash in current setting area */
int apmib_update(CONFIG_DATA_T type)
{
	int i, len;
#ifndef MIB_TLV
	int j, k;
#endif
	unsigned char checksum;
	unsigned char *data;
#ifdef _LITTLE_ENDIAN_
	unsigned char *pdata;
#endif
#ifdef MIB_TLV
	unsigned char *pfile = NULL;
	unsigned char *mib_tlv_data = NULL;
	unsigned int tlv_content_len = 0;
	unsigned int mib_tlv_max_len = 0;
#endif

	int write_hw_tlv = 0;

#if CONFIG_APMIB_SHARED_MEMORY == 1
	apmib_sem_lock();
#endif
	if (type & HW_SETTING) {
//	always Write HW setting uncompressed		
//		if(compress_hw_setting == 0)
//		write_hw_tlv = 0;
#ifdef _LITTLE_ENDIAN_
		data=malloc(hsHeader.len);
		if(data == NULL)
		{
			printf("malloc failed\n");
			return -1;
		}
		pdata=data;
		memcpy(data,pHwSetting, hsHeader.len);		
		swap_mib_value((HW_SETTING_Tp)data,HW_SETTING);
#else
		data = (unsigned char *)pHwSetting;
#endif
		checksum = CHECKSUM(data, hsHeader.len-1);
		data[hsHeader.len-1] = checksum;
		len=hsHeader.len;

#ifdef COMPRESS_MIB_SETTING
	if(write_hw_tlv)
	{
#ifdef MIB_TLV
		mib_tlv_max_len = mib_get_setting_len(type)*4;
//fprintf(stderr,"\r\n mib_tlv_max_len = %p, __[%s-%u]",mib_tlv_max_len,__FILE__,__LINE__);

		pfile = malloc(mib_tlv_max_len);
		tlv_content_len = 0;
		
//mib_display_data_content(HW_SETTING, data, hsHeader.len);		

		if(pfile != NULL && mib_tlv_save(type, (void*)data, pfile, &tlv_content_len) == 1)
		{

			mib_tlv_data = malloc(tlv_content_len+1); // 1:checksum
			if(mib_tlv_data != NULL)
			{
				memcpy(mib_tlv_data, pfile, tlv_content_len);
			}
					
			free(pfile);
			pfile = NULL;

		}
		
		if(mib_tlv_data != NULL)
		{
			hsHeader.len = tlv_content_len+1;
			data = mib_tlv_data;
			data[tlv_content_len] = CHECKSUM(data, tlv_content_len);
//mib_display_tlv_content(HW_SETTING, data, hsHeader.len);
		}

#endif // #ifdef MIB_TLV		
		if( mib_compress_write(type, data) == 1)
		{

		}
#ifdef MIB_TLV
		/*restore len to MIB structure size since len changed to TLV_data length*/
		hsHeader.len = len;
#endif
	}
		else
#endif //#ifedf COMPRESS_MIB_SETTING	

		/*overwrite header*/
		if(compress_hw_setting && (write_hw_tlv == 0))
			flash_write((char *)&hsHeader,HW_SETTING_OFFSET,sizeof(hsHeader));
		
		if ( flash_write((char *)data, HW_SETTING_OFFSET+sizeof(hsHeader), hsHeader.len)==0 ) {
			printf("write hs MIB failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_sem_unlock();
#endif
#ifdef MIB_TLV
	if(write_hw_tlv)	{

			if(mib_tlv_data)
				free(mib_tlv_data);
			
			if(pfile)
				free(pfile);
	}
#endif
#ifdef _LITTLE_ENDIAN_
			free(pdata);
#endif
			return 0;
		}
		
#ifdef _LITTLE_ENDIAN_
		free(pdata);
#endif
	}
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		if(type & BLUETOOTH_HW_SETTING)
		{
		
#ifdef _LITTLE_ENDIAN_
				data=malloc(blueToothHwHeader.len);
				if(data == NULL)
				{
					printf("malloc failed\n");
					return -1;
				}
				pdata=data;
				memcpy(data,pBlueToothHwSetting, blueToothHwHeader.len);		
				swap_mib_value((BLUETOOTH_HW_SETTING_Tp)data,BLUETOOTH_HW_SETTING);
#else
				data = (unsigned char *)pBlueToothHwSetting;
#endif
				checksum = CHECKSUM(data, blueToothHwHeader.len-1);
				//printf("%s:%d checksum=%d\n",__FUNCTION__,__LINE__,checksum);
				*(data + blueToothHwHeader.len - 1) = checksum;
				i = BLUETOOTH_HW_SETTING_OFFSET + sizeof(blueToothHwHeader);
				len = blueToothHwHeader.len;
				

			if ( flash_write((char *)data, i, len)==0 ) {
				printf("Write flash bluetooth hw-setting failed!\n");
	
#ifdef _LITTLE_ENDIAN_
				free(pdata);
#endif
	
				return 0;
			}
#ifdef _LITTLE_ENDIAN_
			free(pdata);
#endif
			
		}
#endif

#ifdef CUSTOMER_HW_SETTING_SUPPORT
	if(type & CUSTOMER_HW_SETTING)
	{
	
#ifdef _LITTLE_ENDIAN_
			data=malloc(customerHwHeader.len);
			if(data == NULL)
			{
				printf("malloc failed\n");
				return -1;
			}
			pdata=data;
			memcpy(data,pCustomerHwSetting, customerHwHeader.len);		
			swap_mib_value((CUSTOMER_HW_SETTING_Tp)data,CUSTOMER_HW_SETTING);
#else
			data = (unsigned char *)pCustomerHwSetting;
#endif
			checksum = CHECKSUM(data, customerHwHeader.len-1);
			//printf("%s:%d checksum=%d\n",__FUNCTION__,__LINE__,checksum);
			*(data + customerHwHeader.len - 1) = checksum;
			i = CUSTOMER_HW_SETTING_OFFSET + sizeof(customerHwHeader);
			len = customerHwHeader.len;
			
#ifdef COMPRESS_MIB_SETTING
#ifdef MIB_TLV
		mib_tlv_max_len = mib_get_setting_len(type)*4;


		pfile = malloc(mib_tlv_max_len);
		tlv_content_len = 0;

//mib_display_data_content(type, data, len);

		if(pfile != NULL && mib_tlv_save(type, (void*)data, pfile, &tlv_content_len) == 1)
		{

			mib_tlv_data = malloc(tlv_content_len+1); // 1:checksum
			if(mib_tlv_data != NULL)
			{
				memcpy(mib_tlv_data, pfile, tlv_content_len);
			}
					
			free(pfile);
			pfile = NULL;

		}
		
		if(mib_tlv_data != NULL)
		{
			customerHwHeader.len = tlv_content_len+1;
			data = mib_tlv_data;
			data[tlv_content_len] = CHECKSUM(data, tlv_content_len);
		//	printf("CHECKSUM %s:%d tlv_content_len=%d data=%d\n",__FUNCTION__,__LINE__,tlv_content_len,data[tlv_content_len]);
//mib_display_tlv_content(type, data, tlv_content_len+1);

		}

#endif // #ifdef MIB_TLV
		if( mib_compress_write(type, data) == 1)
		{

		}	
		else
#endif //#ifedf COMPRESS_MIB_SETTING	
		if ( flash_write((char *)data, i, len)==0 ) {
			printf("Write flash customer hw-setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_sem_unlock();
#endif

#ifdef MIB_TLV
			if(mib_tlv_data)
				free(mib_tlv_data);
			
			if(pfile)
				free(pfile);
#endif
#ifdef _LITTLE_ENDIAN_
			free(pdata);
#endif

			return 0;
		}
#ifdef _LITTLE_ENDIAN_
		free(pdata);
#endif
		
	}
#endif

#ifdef RF_DPK_SETTING_SUPPORT
	if (type & RF_DPK_SETTING) {
        int write_header = 0;
        
//	always Write RF DPK setting uncompressed		
//		if(compress_hw_setting == 0)
//		write_hw_tlv = 0;
#ifdef _LITTLE_ENDIAN_
		data=malloc(rfDpkHeader.len);
		if(data == NULL)
		{
			printf("malloc failed\n");
			return -1;
		}
		pdata=data;
		memcpy(data,pRfDpkSetting, rfDpkHeader.len);		
		swap_mib_value((RF_DPK_SETTING_Tp)data,RF_DPK_SETTING);
#else
		data = (unsigned char *)pRfDpkSetting;
#endif
		checksum = CHECKSUM(data, rfDpkHeader.len-1);
		data[rfDpkHeader.len-1] = checksum;
		len=rfDpkHeader.len;

#ifdef COMPRESS_MIB_SETTING
	if(write_hw_tlv)
	{
#ifdef MIB_TLV
		mib_tlv_max_len = mib_get_setting_len(type)*4;
//fprintf(stderr,"\r\n mib_tlv_max_len = %p, __[%s-%u]",mib_tlv_max_len,__FILE__,__LINE__);

		pfile = malloc(mib_tlv_max_len);
		tlv_content_len = 0;
		
//mib_display_data_content(RF_DPK_SETTING, data, rfDpkHeader.len);		

		if(pfile != NULL && mib_tlv_save(type, (void*)data, pfile, &tlv_content_len) == 1)
		{

			mib_tlv_data = malloc(tlv_content_len+1); // 1:checksum
			if(mib_tlv_data != NULL)
			{
				memcpy(mib_tlv_data, pfile, tlv_content_len);
			}
					
			free(pfile);
			pfile = NULL;

		}
		
		if(mib_tlv_data != NULL)
		{
			rfDpkHeader.len = tlv_content_len+1;
			data = mib_tlv_data;
			data[tlv_content_len] = CHECKSUM(data, tlv_content_len);
//mib_display_tlv_content(RF_DPK_SETTING, data, rfDpkHeader.len);
		}

#endif // #ifdef MIB_TLV		
		if( mib_compress_write(type, data) == 1)
		{

		}
#ifdef MIB_TLV
		/*restore len to MIB structure size since len changed to TLV_data length*/
		rfDpkHeader.len = len;
#endif
	}
		else
#endif //#ifedf COMPRESS_MIB_SETTING	

        /* check signature */
        if ( memcmp(rfDpkHeader.signature, RF_DPK_SETTING_HEADER_TAG, TAG_LEN)) { /* invalid signature */
            printf("wirte DPK signature [sig=%s], current is [sig=%c%c]!\n", RF_DPK_SETTING_HEADER_TAG, rfDpkHeader.signature[0],rfDpkHeader.signature[1]);
            write_header = 1;
            sprintf((char *)rfDpkHeader.signature, "%s%02d", RF_DPK_SETTING_HEADER_TAG, RF_DPK_SETTING_VER);
            rfDpkHeader.len=WORD_SWAP(len);
        }

		/*overwrite header*/
		if (write_header || (compress_hw_setting && (write_hw_tlv == 0))) {  
			flash_write((char *)&rfDpkHeader,RF_DPK_SETTING_OFFSET,sizeof(rfDpkHeader));

            if (write_header) {
                rfDpkHeader.len=len;
            }
        }
		
		if ( flash_write((char *)data, RF_DPK_SETTING_OFFSET+sizeof(rfDpkHeader), rfDpkHeader.len)==0 ) {
			printf("write RF DPK setting MIB failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_sem_unlock();
#endif
#ifdef MIB_TLV
			if(write_hw_tlv)	{
                if(mib_tlv_data)
                    free(mib_tlv_data);

                if(pfile)
                    free(pfile);
			}
#endif
#ifdef _LITTLE_ENDIAN_
			free(pdata);
#endif
			return 0;
		}
        
#ifdef _LITTLE_ENDIAN_
		free(pdata);
#endif
	}
#endif /* RF_DPK_SETTING_SUPPORT */

	if(type & CURRENT_SETTING) type=CURRENT_SETTING;
	else
	if(type & DEFAULT_SETTING) type=DEFAULT_SETTING;	

	if ((type & CURRENT_SETTING) || (type & DEFAULT_SETTING)) {
		
#ifndef MIB_TLV
		for (j=0; j<NUM_WLAN_INTERFACE; j++) 
			for (k=0; k<(NUM_VWLAN_INTERFACE+1); k++)
		{
			memset( pMib->wlan[j][k].acAddrArray, '\0', MAX_WLAN_AC_NUM*sizeof(MACFILTER_T) );
			for (i=0; i<pMib->wlan[j][k].acNum; i++) {
				get_linkchain(&wlanMacChain[j][k], (void *)&pMib->wlan[j][k].acAddrArray[i], i+1);
			}
			memset( pMib->wlan[j][k].wdsArray, '\0', MAX_WDS_NUM*sizeof(WDS_T) );
			for (i=0; i<pMib->wlan[j][k].wdsNum; i++) {
				get_linkchain(&wdsChain[j][k], (void *)&pMib->wlan[j][k].wdsArray[i], i+1);
			}
#if defined(WIFI_SIMPLE_CONFIG) && defined(CONFIG_APP_TR069)
			memset( pMib->wlan[j][k].wscERArray, '\0', MAX_WSC_ER_NUM*sizeof(WSC_ER_T) );
			for (i=0; i<pMib->wlan[j][k].wscERNum; i++) {
				get_linkchain(&wscERChain[j][k], (void *)&pMib->wlan[j][k].wscERArray[i], i+1);
			}
#endif
			memset( pMib->wlan[j][k].scheduleRuleArray, '\0', MAX_SCHEDULE_NUM*sizeof(SCHEDULE_T) );
			for (i=0; i<pMib->wlan[j][k].scheduleRuleNum; i++) {
				get_linkchain(&scheduleRuleChain[j][k], (void *)&pMib->wlan[j][k].scheduleRuleArray[i], i+1);
			}
		}

#if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)	// below code copy above ACL code
		memset( pMib->meshAclAddrArray, '\0', MAX_MESH_ACL_NUM*sizeof(MACFILTER_T) );
		for (i=0; i<pMib->meshAclNum; i++) {
			get_linkchain(&meshAclChain, (void *)&pMib->meshAclAddrArray[i], i+1);
		}
#endif

		
#ifdef HOME_GATEWAY
		memset( pMib->portFwArray, '\0', MAX_FILTER_NUM*sizeof(PORTFW_T) );
		for (i=0; i<pMib->portFwNum; i++) {
			get_linkchain(&portFwChain, (void *)&pMib->portFwArray[i], i+1);
		}

		memset( pMib->ipFilterArray, '\0', MAX_FILTER_NUM*sizeof(IPFILTER_T) );
		for (i=0; i<pMib->ipFilterNum; i++) {
			get_linkchain(&ipFilterChain, (void *)&pMib->ipFilterArray[i], i+1);
		}
		memset( pMib->portFilterArray, '\0', MAX_FILTER_NUM*sizeof(PORTFILTER_T) );
		for (i=0; i<pMib->portFilterNum; i++) {
			get_linkchain(&portFilterChain, (void *)&pMib->portFilterArray[i], i+1);
		}
		memset( pMib->macFilterArray, '\0', MAX_FILTER_NUM*sizeof(MACFILTER_T) );
		for (i=0; i<pMib->macFilterNum; i++) {
			get_linkchain(&macFilterChain, (void *)&pMib->macFilterArray[i], i+1);
		}
		memset( pMib->urlFilterArray, '\0', MAX_URLFILTER_NUM*sizeof(URLFILTER_T) );
		for (i=0; i<pMib->urlFilterNum; i++) {
			get_linkchain(&urlFilterChain, (void *)&pMib->urlFilterArray[i], i+1);
		}
		memset( pMib->triggerPortArray, '\0', MAX_FILTER_NUM*sizeof(TRIGGERPORT_T) );
		for (i=0; i<pMib->triggerPortNum; i++) {
			get_linkchain(&triggerPortChain, (void *)&pMib->triggerPortArray[i], i+1);
		}
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
	memset( pMib->dhcpServerOptionArray, '\0', MAX_DHCP_SERVER_OPTION_NUM*sizeof(MIB_CE_DHCP_OPTION_T) );
	for (i=0; i<pMib->dhcpServerOptionNum; i++) {
		get_linkchain(&dhcpServerOptionChain, (void *)&pMib->dhcpServerOptionArray[i], i+1);
	}

	memset( pMib->dhcpClientOptionArray, '\0', MAX_DHCP_CLIENT_OPTION_NUM*sizeof(MIB_CE_DHCP_OPTION_T) );
	for (i=0; i<pMib->dhcpClientOptionNum; i++) {
		get_linkchain(&dhcpClientOptionChain, (void *)&pMib->dhcpClientOptionArray[i], i+1);
	}

	memset( pMib->dhcpsServingPoolArray, '\0', MAX_DHCPS_SERVING_POOL_NUM*sizeof(DHCPS_SERVING_POOL_T) );
	for (i=0; i<pMib->dhcpsServingPoolNum; i++) {
		get_linkchain(&dhcpsServingPoolChain, (void *)&pMib->dhcpsServingPoolArray[i], i+1);
	}
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */

#if defined(CONFIG_CMCC_QOS)
	// TODO
#elif defined(QOS_OF_TR069) || defined(MULTI_WAN_QOS)	//added by shirley 2014/5/20
        memset( pMib->QosQueueArray, '\0', MAX_QOS_QUEUE_NUM*sizeof(QOSQUEUE_T) );
		for (i=0; i<pMib->QosQueueNum; i++) {
			get_linkchain(&qosqueueChain, (void *)&pMib->QosQueueArray[i], i+1);
		}
		
	    memset( pMib->QosClassArray, '\0', MAX_QOS_CLASS_NUM*sizeof(QOSCLASS_T) );
		for (i=0; i<pMib->QosClassNum; i++) {
			get_linkchain(&qosclassChain, (void *)&pMib->QosClassArray[i], i+1);
		}

		memset( pMib->tr098_appconf_array, '\0', MAX_TR098_APP_TBL_NUM*sizeof(TR098_APPCONF_T) );
				for (i=0; i<pMib->tr098_appconf_num; i++) {
					get_linkchain(&Tr098QOSAppChain, (void *)&pMib->tr098_appconf_array[i], i+1);
		}
		
	    memset( pMib->tr098_flowconf_array, '\0', MAX_TR098_FLOW_TBL_NUM*sizeof(TR098_FLOWCONF_T) );
		for (i=0; i<pMib->tr098_flowconf_num; i++) {
			get_linkchain(&Tr098QOSFlowChain, (void *)&pMib->tr098_flowconf_array[i], i+1);
		}
	
		//mark_qos
	    memset( pMib->QosPolicerArray, '\0', MAX_QOS_POLICER_NUM*sizeof(QOSPOLICER_T) );
		for (i=0; i<pMib->QosPolicerNum; i++) {
			get_linkchain(&qospolicerChain, (void *)&pMib->QosPolicerArray[i], i+1);
		}
		 memset( pMib->QosQueueStatsArray, '\0', MAX_QOS_QUEUESTATS_NUM*sizeof(QOSQUEUESTATS_T) );
		for (i=0; i<pMib->QosQueueStatsNum; i++) {
			get_linkchain(&qosqueuestatsChain, (void *)&pMib->QosQueueStatsArray[i], i+1);
		}		

#elif defined(QOS_BY_BANDWIDTH)
		memset( pMib->qosRuleArray, '\0', MAX_QOS_RULE_NUM*sizeof(IPQOS_T) );
		for (i=0; i<pMib->qosRuleNum; i++) {
			get_linkchain(&qosChain, (void *)&pMib->qosRuleArray[i], i+1);
		}

#elif defined(GW_QOS_ENGINE)
		memset( pMib->qosRuleArray, '\0', MAX_QOS_RULE_NUM*sizeof(QOS_T) );
		for (i=0; i<pMib->qosRuleNum; i++) {
			get_linkchain(&qosChain, (void *)&pMib->qosRuleArray[i], i+1);
		}
		
#endif


#ifdef ROUTE_SUPPORT
		memset( pMib->staticRouteArray, '\0', MAX_ROUTE_NUM*sizeof(STATICROUTE_T) );
		for (i=0; i<pMib->staticRouteNum; i++) {
			get_linkchain(&staticRouteChain, (void *)&pMib->staticRouteArray[i], i+1);
		}
#endif //ROUTE

#endif
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
		memset( pMib->capPortalAllowArray, '\0', MAX_ROUTE_NUM*sizeof(STATICROUTE_T) );
		for (i=0; i<pMib->capPortalCurNum; i++) {
			get_linkchain(&capPortalAllowChain, (void *)&pMib->capPortalAllowArray[i], i+1);
		}
#endif //ROUTE

#ifdef HOME_GATEWAY
#ifdef VPN_SUPPORT
		memset( pMib->ipsecTunnelArray, '\0', MAX_TUNNEL_NUM*sizeof(IPSECTUNNEL_T) );
		for (i=0; i<pMib->ipsecTunnelNum; i++) {
			get_linkchain(&ipsecTunnelChain, (void *)&pMib->ipsecTunnelArray[i], i+1);
		}
#endif
#endif
#ifdef TLS_CLIENT
		memset( pMib->certRootArray, '\0', MAX_CERTROOT_NUM*sizeof(CERTROOT_T) );
		for (i=0; i<pMib->certRootNum; i++) {
			get_linkchain(&certRootChain, (void *)&pMib->certRootArray[i], i+1);
		}
		memset( pMib->certUserArray, '\0', MAX_CERTUSER_NUM*sizeof(CERTUSER_T) );
		for (i=0; i<pMib->certUserNum; i++) {
			get_linkchain(&certUserChain, (void *)&pMib->certUserArray[i], i+1);
		}		
#endif
		memset(pMib->dhcpRsvdIpArray, '\0', MAX_DHCP_RSVD_IP_NUM*sizeof(DHCPRSVDIP_T));
		for (i=0; i<pMib->dhcpRsvdIpNum; i++) {
			get_linkchain(&dhcpRsvdIpChain, (void *)&pMib->dhcpRsvdIpArray[i], i+1);
		}
#if defined(VLAN_CONFIG_SUPPORTED)		
		memset(pMib->VlanConfigArray, '\0', MAX_IFACE_VLAN_CONFIG*sizeof(VLAN_CONFIG_T));
		for (i=0; i<pMib->VlanConfigNum; i++) {
			get_linkchain(&vlanConfigChain, (void *)&pMib->VlanConfigArray[i], i+1);
		}
#endif	

#ifdef RTK_CAPWAP
		memset( pMib->wtpConfigArray, '\0', MAX_CAPWAP_WTP_NUM*sizeof(CAPWAP_WTP_CONFIG_T) );
		for (i=0; i<pMib->numWtpConfig; i++) {
			get_linkchain(&capwapWtpConfigChain, (void *)&pMib->wtpConfigArray[i], i+1);
		}
#endif

#ifdef CONFIG_RTL_AIRTIME
		memset( pMib->airTimeArray, '\0', MAX_AIRTIME_NUM*sizeof(AIRTIME_T) );
		for (i=0; i<pMib->airTimeNum; i++) {
			get_linkchain(&airtimeChain, (void *)&pMib->airTimeArray[i], i+1);
		}
		
		memset( pMib->airTimeArray2, '\0', MAX_AIRTIME_NUM*sizeof(AIRTIME_T) );
		for (i=0; i<pMib->airTimeNum2; i++) {
			get_linkchain(&airtimeChain2, (void *)&pMib->airTimeArray2[i], i+1);
		}
#endif /* CONFIG_RTL_AIRTIME */

#endif /*not def MI B_TLV*/

		if (type & CURRENT_SETTING) {
#ifdef _LITTLE_ENDIAN_
			data=malloc(csHeader.len);
			if(data == NULL)
			{
				printf("malloc failed\n");
				return -1;
			}			
			pdata=data;
			memcpy(data,pMib, csHeader.len);		
			swap_mib_value((APMIB_Tp)data, CURRENT_SETTING);

#else
			data = (unsigned char *)pMib;
#endif
			checksum = CHECKSUM(data, csHeader.len-1);
			*(data + csHeader.len - 1) = checksum;//recaculate checksum and assign it to data
			i = CURRENT_SETTING_OFFSET + sizeof(csHeader);//when not define COMPRESS_MIB_SETTING, i points to the content after csHeader.
			len = csHeader.len;
		}
		else {
#ifdef _LITTLE_ENDIAN_
			data=malloc(dsHeader.len);
			if(data == NULL)
			{
				printf("malloc failed\n");
				return -1;
			}	
			pdata=data;
			memcpy(data,pMibDef, dsHeader.len);		
			swap_mib_value((APMIB_Tp)data, DEFAULT_SETTING);

#else			
			data = (unsigned char *)pMibDef;
#endif			
			checksum = CHECKSUM(data, dsHeader.len-1);
			*(data + dsHeader.len - 1) = checksum;
			i = DEFAULT_SETTING_OFFSET + sizeof(dsHeader);
			len = dsHeader.len;
		}


#ifdef COMPRESS_MIB_SETTING
#ifdef MIB_TLV
		mib_tlv_max_len = mib_get_setting_len(type)*4;


		pfile = malloc(mib_tlv_max_len);
		tlv_content_len = 0;

//mib_display_data_content(type, data, len);

		if(pfile != NULL && mib_tlv_save(type, (void*)data, pfile, &tlv_content_len) == 1)
		{

			mib_tlv_data = malloc(tlv_content_len+1); // 1:checksum
			if(mib_tlv_data != NULL)
			{
				memcpy(mib_tlv_data, pfile, tlv_content_len);
			}
					
			free(pfile);
			pfile = NULL;

		}
		
		if(mib_tlv_data != NULL)
		{
			if (type & CURRENT_SETTING)
				csHeader.len = tlv_content_len+1;
			else
				dsHeader.len = tlv_content_len+1;
			data = mib_tlv_data;
			data[tlv_content_len] = CHECKSUM(data, tlv_content_len);
			
//mib_display_tlv_content(type, data, tlv_content_len+1);

		}

#endif // #ifdef MIB_TLV
		if( mib_compress_write(type, data) == 1)
		{

		}	
		else
#endif //#ifedf COMPRESS_MIB_SETTING	
		if ( flash_write((char *)data, i, len)==0 ) {
			printf("Write flash current-setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_sem_unlock();
#endif

#ifdef MIB_TLV
			if(mib_tlv_data)
				free(mib_tlv_data);
			
			if(pfile)
				free(pfile);
#endif

#ifdef _LITTLE_ENDIAN_
			free(pdata);
#endif

			return 0;
		}
#ifdef MIB_TLV
		/*restore len to APMIB_T structure size*/
		if (type & CURRENT_SETTING)
			csHeader.len = len;
		else
			dsHeader.len = len;
#endif
		
#ifdef _LITTLE_ENDIAN_
		free(pdata);
#endif
	}
#if CONFIG_APMIB_SHARED_MEMORY == 1
	apmib_sem_unlock();
#endif

#ifdef MIB_TLV
	if(mib_tlv_data)
		free(mib_tlv_data);
	if(pfile)
		free(pfile);
#endif	

#ifdef BACKUP_CURRENT_SETTING_SUPPORT
	if(type & CURRENT_SETTING)
	{
		if(resetBackupCurSetFromCurSet() == -1)
		{
			printf("resetBackupCurSetFromCurSet is error! [File:%s Line:%d]\n",__FILE__,__LINE__);
			return -1;
		}
	}
#endif

#ifdef BACKUP_DEFAULT_SETTING_SUPPORT
	if(type & DEFAULT_SETTING)
	{
		if(resetBackupDefSetFromDefSet() == -1)
		{
			printf("resetBackupDefSetFromDefSet is error! [File:%s Line:%d]\n",__FILE__,__LINE__);
			return -1;
		}
	}
#endif

#ifdef BACKUP_HARDWARE_SETTING_SUPPORT
if(type & HW_SETTING)
{
	if( resetbackupHWSetFromHWSet() == -1)
	{
		printf("reset Backup hw set From hw set is error! [File:%s Line:%d]\n",__FILE__,__LINE__);
		return -1;
	}
}


#ifdef CUSTOMER_HW_SETTING_SUPPORT
	if(type & CUSTOMER_HW_SETTING)
	{
		if( resetbackupCUHWSetFromCUHWSet() == -1)
		{
			printf("reset Backup Customer hw set From Customer hw set is error! [File:%s Line:%d]\n",__FILE__,__LINE__);
			return -1;
		}
	}

#endif
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
	if(type & BLUETOOTH_HW_SETTING)
	{
		if( resetbackupBTHWSetFromBTHWSet() == -1)
		{
			printf("reset Backup BLUETOOTH HW set From BLUETOOTH HW set is error! [File:%s Line:%d]\n",__FILE__,__LINE__);
			return -1;
		}
	}

#endif
#endif 

	return 1;
}


////////////////////////////////////////////////////////////////////////////////
/* Update default setting MIB into current setting area
 */
int apmib_updateDef(void)
{
	unsigned char *data, checksum;
#ifdef _LITTLE_ENDIAN_
	unsigned char *pdata;
#endif
	PARAM_HEADER_T header;
	int i;
#ifdef MIB_TLV
	unsigned char *pfile = NULL;
	unsigned char *mib_tlv_data = NULL;
	unsigned int tlv_content_len = 0;
	unsigned int mib_tlv_max_len = 0;
#endif

#if CONFIG_APMIB_SHARED_MEMORY == 1
	apmib_sem_lock();
#endif	
	memcpy(header.signature, CURRENT_SETTING_HEADER_TAG, TAG_LEN);
	memcpy(&header.signature[TAG_LEN], &dsHeader.signature[TAG_LEN], SIGNATURE_LEN-TAG_LEN);

	header.len = dsHeader.len;
#ifdef _LITTLE_ENDIAN_
	data=malloc(dsHeader.len);
	if(data == NULL)
	{
		printf("malloc failed\n");
		return -1;
	}
	pdata=data;
	memcpy(data,pMibDef,dsHeader.len);		
	swap_mib_value((APMIB_Tp)data,DEFAULT_SETTING);
#else
	data = (unsigned char *)pMibDef;
#endif
	checksum = CHECKSUM(data, header.len-1);
	*(data + header.len - 1) = checksum;

	i = CURRENT_SETTING_OFFSET;
#ifdef COMPRESS_MIB_SETTING
#ifdef MIB_TLV
	mib_tlv_max_len = mib_get_setting_len(DEFAULT_SETTING)*4;

	pfile = malloc(mib_tlv_max_len);
	tlv_content_len = 0;

//mib_display_data_content(DEFAULT_SETTING, data, sizeof(APMIB_T));	
	if(pfile != NULL && mib_tlv_save(DEFAULT_SETTING, (void*)data, pfile, &tlv_content_len) == 1)
	{

		mib_tlv_data = malloc(tlv_content_len+1); // 1:checksum
		if(mib_tlv_data != NULL)
		{
			memcpy(mib_tlv_data, pfile, tlv_content_len);
		}
				
		free(pfile);
	}
	
	if(mib_tlv_data != NULL)
	{
		header.len = tlv_content_len+1;
		
		data = mib_tlv_data;
		data[tlv_content_len] = CHECKSUM(data, tlv_content_len);

//mib_display_tlv_content(CURRENT_SETTING, data, tlv_content_len+1);			

	}

#endif // #ifdef MIB_TLV

	if(mib_updateDef_compress_write(CURRENT_SETTING, (char *)data, &header) == 1)
	{
		COMP_TRACE(stderr,"\r\n mib_updateDef_compress_write CURRENT_SETTING DONE, __[%s-%u]", __FILE__,__LINE__);			
	}
	else
	{
#endif

	if ( flash_write((char *)&header, i, sizeof(header))==0 ) {
		printf("Write flash current-setting header failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif

#ifdef _LITTLE_ENDIAN_
		free(pdata);
#endif
		return 0;
	}
	i += sizeof(header);

	if ( flash_write((char *)data, i, header.len)==0 ) {
		printf("Write flash current-setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
#ifdef _LITTLE_ENDIAN_
		free(pdata);
#endif

		return 0;
	}
#ifdef COMPRESS_MIB_SETTING
	}
	if(mib_tlv_data) {
		free(mib_tlv_data);
		mib_tlv_data=NULL;
	}		
#endif

#if CONFIG_APMIB_SHARED_MEMORY == 1
	apmib_sem_unlock();
#endif	
#ifdef _LITTLE_ENDIAN_
	free(pdata);
#endif

	return 1;
}


////////////////////////////////////////////////////////////////////////////////
/* Update MIB into flash current setting area
 */
#ifdef COMPRESS_MIB_SETTING
int apmib_updateFlash(CONFIG_DATA_T type, char *data, int len, int force, int ver)
{
	int offset;
	/*since COMPRESS MIB no way to keep old or upgrade mib. so only replaced*/
	if ( type == HW_SETTING ) {
		offset = HW_SETTING_OFFSET;
	}
	else if ( type == DEFAULT_SETTING ) {
		offset = DEFAULT_SETTING_OFFSET;
	}
	else  {
		offset = CURRENT_SETTING_OFFSET;
	}
	/*no care force and ver.*/
	if(0==flash_write(data,offset,len)){
		printf("flash write apmib failed");
		return 0;
	}
	return 1;
}

#else
int apmib_updateFlash(CONFIG_DATA_T type, char *data, int len, int force, int ver)
{
	unsigned char checksum, checksum1, *ptr=NULL;
	int i, offset=0, curLen, curVer;
	unsigned char *pMibData, *pHdr, tmpBuf[20];

#if CONFIG_APMIB_SHARED_MEMORY == 1
	apmib_sem_lock();
#endif

	if ( type == HW_SETTING ) {
		curLen = hsHeader.len - 1;
		pMibData = (unsigned char *)pHwSetting;
		pHdr = (unsigned char *)&hsHeader;
		i = HW_SETTING_OFFSET;
	}
	else if ( type == DEFAULT_SETTING ) {
		curLen = dsHeader.len - 1;
		pMibData = (unsigned char *)pMibDef;
		pHdr = (unsigned char *)&dsHeader;
		i = DEFAULT_SETTING_OFFSET;
	}
	else  {
		curLen = csHeader.len - 1;
		pMibData = (unsigned char *)pMib;
		pHdr = (unsigned char *)&csHeader;
		i = CURRENT_SETTING_OFFSET;
	}

	if (force==2) { // replace by input mib
#ifdef HEADER_LEN_INT
		if(type == HW_SETTING)
			((HW_PARAM_HEADER_Tp)pHdr)->len = len + 1;
		else
#endif
			((PARAM_HEADER_Tp)pHdr)->len = len + 1;
		sprintf(tmpBuf, "%02d", ver);
		memcpy(&pHdr[TAG_LEN], tmpBuf, SIGNATURE_LEN-TAG_LEN);
		checksum = CHECKSUM(data, len);
		pMibData = data;
		curLen = len;
	}
	else if (force==1) { // update mib but keep not used mib
#ifdef HEADER_LEN_INT
		if(type == HW_SETTING)
			sscanf(&((HW_PARAM_HEADER_Tp)pHdr)->signature[TAG_LEN], "%02d", &curVer);
		else
#endif
			sscanf(&((PARAM_HEADER_Tp)pHdr)->signature[TAG_LEN], "%02d", &curVer);
		if ( curVer < ver ) {
			sprintf(tmpBuf, "%02d", ver);
#ifdef HEADER_LEN_INT
			if(type == HW_SETTING)
				memcpy(&((HW_PARAM_HEADER_Tp)pHdr)->signature[TAG_LEN],
					tmpBuf, SIGNATURE_LEN-TAG_LEN);
			else
#endif
				memcpy(&((PARAM_HEADER_Tp)pHdr)->signature[TAG_LEN],
					tmpBuf, SIGNATURE_LEN-TAG_LEN);
		}
		checksum = CHECKSUM(data, len);
		if (curLen > len) {
#ifdef HEADER_LEN_INT
			if(type == HW_SETTING)
				((HW_PARAM_HEADER_Tp)pHdr)->len = curLen + 1;
			else
#endif
				((PARAM_HEADER_Tp)pHdr)->len = curLen + 1;
			ptr = pMibData + len;
			offset = curLen - len;
			checksum1 = CHECKSUM(ptr, offset);
			checksum +=  checksum1;
		}
		else
		{
#ifdef HEADER_LEN_INT
			if(type == HW_SETTING)
				((HW_PARAM_HEADER_Tp)pHdr)->len = len + 1;
			else
#endif
				((PARAM_HEADER_Tp)pHdr)->len = len + 1;
		}

		curLen = len;
		pMibData = data;
	}
	else { // keep old mib, only update new added portion
#ifdef HEADER_LEN_INT
		if(type == HW_SETTING)
			sscanf(&((HW_PARAM_HEADER_Tp)pHdr)->signature[TAG_LEN], "%02d", &curVer);
		else
#endif
			sscanf(&((PARAM_HEADER_Tp)pHdr)->signature[TAG_LEN], "%02d", &curVer);
		if ( curVer < ver ) {
			sprintf(tmpBuf, "%02d", ver);
#ifdef HEADER_LEN_INT
			if(type == HW_SETTING)
				memcpy(&((HW_PARAM_HEADER_Tp)pHdr)->signature[TAG_LEN],
					tmpBuf, SIGNATURE_LEN-TAG_LEN);
			else
#endif
				memcpy(&((PARAM_HEADER_Tp)pHdr)->signature[TAG_LEN],
					tmpBuf, SIGNATURE_LEN-TAG_LEN);
		}
		if ( len > curLen ) {
#ifdef HEADER_LEN_INT
			if(type == HW_SETTING)
				((HW_PARAM_HEADER_Tp)pHdr)->len = len + 1;
			else
#endif
				((PARAM_HEADER_Tp)pHdr)->len = len + 1;
			offset = len - curLen;
			checksum = CHECKSUM(pMibData, curLen);
			ptr = data + curLen;
			checksum1 = CHECKSUM(ptr, offset);
			checksum +=  checksum1;
		}
		else
			checksum = CHECKSUM(pMibData, curLen);
	}
#ifdef HEADER_LEN_INT
	if(type == HW_SETTING)
	{
		if ( flash_write((char *)pHdr, i, sizeof(HW_PARAM_HEADER_T))==0 ) {
			printf("Write flash current-setting header failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_sem_unlock();
#endif
			return 0;
		}
		i += sizeof(HW_PARAM_HEADER_T);
	}
	else
#endif
	{
		if ( flash_write((char *)pHdr, i, sizeof(PARAM_HEADER_T))==0 ) {
			printf("Write flash current-setting header failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_sem_unlock();
#endif
			return 0;
		}
		i += sizeof(PARAM_HEADER_T);
	}
	if ( flash_write(pMibData, i, curLen)==0 ) {
		printf("Write flash current-setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
		return 0;
	}
	i += curLen;

	if (offset > 0) {
		if ( flash_write((char *)ptr, i, offset)==0 ) {
			printf("Write flash current-setting failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
			apmib_sem_unlock();
#endif
			return 0;
		}
		i += offset;
	}

	if ( flash_write((char *)&checksum, i, sizeof(checksum))==0 ) {
		printf("Write flash current-setting checksum failed!\n");
#if CONFIG_APMIB_SHARED_MEMORY == 1
		apmib_sem_unlock();
#endif
		return 0;
	}

#if CONFIG_APMIB_SHARED_MEMORY == 1
	apmib_sem_unlock();
#endif	
	return 1;
}
#endif
#ifdef AP_CONTROLER_SUPPORT
static int flash_read(char *buf, int offset, int len)
{
	return apmib_virtual_flash_read(buf,offset,len);
}
static int flash_write(char *buf, int offset, int len)
{
	return apmib_virtual_flash_write(buf,offset,len);
}


#else
////////////////////////////////////////////////////////////////////////////////////////////////////
#if (defined BACKUP_CURRENT_SETTING_SUPPORT)||(defined BACKUP_DEFAULT_SETTING_SUPPORT)||(defined BACKUP_HARDWARE_SETTING_SUPPORT)
static int flash_read_withtype(char *buf, int offset, int len,CONFIG_DATA_T srcSetting)
{
	int fh=-1;
	int ok=1;

	if ((srcSetting == CURRENT_SETTING)||
		(srcSetting == DEFAULT_SETTING)||
		(srcSetting == HW_SETTING)||
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		(srcSetting == BLUETOOTH_HW_SETTING)||
#endif 
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		(srcSetting == CUSTOMER_HW_SETTING)||
#endif 
		0)
	{
#ifdef CONFIG_MTD_NAND
	fh = open(FLASH_DEVICE_SETTING, O_RDONLY | O_CREAT);
#else
	fh = open(FLASH_DEVICE_NAME, O_RDONLY);
#endif
	}
	else if(
#ifdef BACKUP_CURRENT_SETTING_SUPPORT
		(srcSetting == BACKUP_CURRENT_SETTING) ||
#endif
#ifdef BACKUP_DEFAULT_SETTING_SUPPORT
		(srcSetting == BACKUP_DEFAULT_SETTING) ||
#endif
#ifdef BACKUP_HARDWARE_SETTING_SUPPORT

		(srcSetting == BACKUP_HW_SETTING)||
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		(srcSetting == BACKUP_BLUETOOTH_HW_SETTING)||
#endif 
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		(srcSetting == BACKUP_CUSTOMER_HW_SETTING)||
#endif
#endif 
	0)
	{
#ifdef CONFIG_MTD_NAND 
		fh = open(FLASH_DEVICE_SETTING_BACKUP, O_RDONLY | O_CREAT);
#else 
		fh = open(FLASH_DEVICE_NAME, O_RDONLY);
#endif
	}
	if(fh == -1)
		return 0;
	lseek(fh, offset, SEEK_SET);
	if(read(fh, buf, len)!= len)
		ok = 0;
	close(fh);
	return ok;
}
#endif

#if (defined BACKUP_CURRENT_SETTING_SUPPORT)||(defined BACKUP_DEFAULT_SETTING_SUPPORT)||(defined BACKUP_HARDWARE_SETTING_SUPPORT)
static int flash_write_withtype(char *buf, int offset, int len,CONFIG_DATA_T srcSetting)
{
	int fh;
	int ok=1;
	if ((srcSetting == CURRENT_SETTING)||
		(srcSetting == DEFAULT_SETTING)||
		(srcSetting == HW_SETTING)||
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		(srcSetting == BLUETOOTH_HW_SETTING)||
#endif 
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		(srcSetting == CUSTOMER_HW_SETTING)||
#endif 
	0)
	{
#ifdef CONFIG_MTD_NAND
		fh = open(FLASH_DEVICE_SETTING, O_RDWR | O_CREAT);
#else
		fh = open(FLASH_DEVICE_NAME, O_RDWR);
#endif
	}
	else if(
#ifdef BACKUP_CURRENT_SETTING_SUPPORT
		(srcSetting == BACKUP_CURRENT_SETTING) ||
#endif
#ifdef BACKUP_DEFAULT_SETTING_SUPPORT
		(srcSetting == BACKUP_DEFAULT_SETTING) ||
#endif
#ifdef BACKUP_HARDWARE_SETTING_SUPPORT

		(srcSetting == BACKUP_HW_SETTING)||
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		(srcSetting == BACKUP_BLUETOOTH_HW_SETTING)||
#endif 
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		(srcSetting == BACKUP_CUSTOMER_HW_SETTING)||
#endif

#endif 
	0)
	{
#ifdef CONFIG_MTD_NAND
		fh = open(FLASH_DEVICE_SETTING_BACKUP, O_RDWR | O_CREAT);
#else 
		fh = open(FLASH_DEVICE_NAME, O_RDWR);
#endif
	}			
	if ( fh == -1 )
		return 0;
	lseek(fh, offset, SEEK_SET);
	if ( write(fh, buf, len) != len)
		ok = 0;
	close(fh);
	sync();
	return ok;
}

#endif
/////////////////////////////////////////////////////////////////////////////////
static int flash_read(char *buf, int offset, int len)
{
	int fh;
	int ok=1;
#ifdef CONFIG_MTD_NAND
	fh = open(FLASH_DEVICE_SETTING, O_RDONLY | O_CREAT);
#else
	fh = open(FLASH_DEVICE_NAME, O_RDONLY);
#endif
	if ( fh == -1 )
		return 0;

	lseek(fh, offset, SEEK_SET);

	if ( read(fh, buf, len) != len)
		ok = 0;

	close(fh);

	return ok;
}


////////////////////////////////////////////////////////////////////////////////
static int flash_write(char *buf, int offset, int len)
{
	int fh;
	int ok=1;

#ifdef CONFIG_MTD_NAND
	fh = open(FLASH_DEVICE_SETTING, O_RDWR | O_CREAT);
#else
	fh = open(FLASH_DEVICE_NAME, O_RDWR);
#endif

	if ( fh == -1 )
		return 0;

	lseek(fh, offset, SEEK_SET);

	if ( write(fh, buf, len) != len)
		ok = 0;

	close(fh);
	sync();

	return ok;
}
#endif
#ifdef MIB_TLV
int add_tblentry(void *pmib, unsigned offset, int num,const mib_table_entry_T *mib_tbl,void *val)
{
	//printf("num %d count %d \n",num,(mib_tbl->total_size/mib_tbl->unit_size));
	if( num >= (mib_tbl->total_size/mib_tbl->unit_size)){
	#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
		RTL_LOG_DBG_A("APMIB[%s] num[%d] total[%d] - add_tblentry FAILED", mib_tbl->name, num, mib_tbl->total_size/mib_tbl->unit_size);
	#endif	
		return 0;
	}
	#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
	else{
        RTL_LOG_DBG_A("APMIB[%s] num[%d] total[%d] - add_tblentry SUCCESS", mib_tbl->name, num, mib_tbl->total_size/mib_tbl->unit_size);
	    memcpy(pmib+offset+num*mib_tbl->unit_size,val,mib_tbl->unit_size);
	    return 1;
	}
	#else
	memcpy(pmib+offset+num*mib_tbl->unit_size,val,mib_tbl->unit_size);
	return 1;
	#endif
}

int delete_tblentry(void *pmib, unsigned offset, int num,const mib_table_entry_T *mib_tbl,void *val)
{
	int i = 0;
	int total_size = mib_tbl->total_size;
	int unit_size = mib_tbl->unit_size;
	if(num > total_size/unit_size)
		num=total_size/unit_size;
	for(i=0;i<num;i++)
	{
		if(0==memcmp(pmib+offset+i*unit_size,val,unit_size))
			break;
	}
	/*not found*/
	if(i == num){
	#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
		RTL_LOG_DBG_A("APMIB[%s] num[%d] total[%d] - delete_tblentry FAILED - not found", mib_tbl->name, num, mib_tbl->total_size/mib_tbl->unit_size);
	#endif	
		return 0;
	}
		
	
	if(i==(num-1))
	{
	#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
		RTL_LOG_DBG_A("APMIB[%s] num[%d] total[%d] - delete_tblentry SUCCESS - last", mib_tbl->name, num, mib_tbl->total_size/mib_tbl->unit_size);
	#endif	
		memset(pmib+offset+i*unit_size,0x0,unit_size);	
	}
	else
	{
	#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
		RTL_LOG_DBG_A("APMIB[%s] num[%d] total[%d] - delete_tblentry SUCCESS - nested idx[%d]", mib_tbl->name, num, mib_tbl->total_size/mib_tbl->unit_size, i);
	#endif		
		for(;i<num;i++)
		{
			memcpy(pmib+offset+i*unit_size,pmib+offset+(i+1)*unit_size,unit_size);
		}
	}
	return 1;
}

#if defined(MIB_MOD_TBL_ENTRY) //brucehou
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
int mod_tblentry(void *pmib, unsigned offset, int num,const mib_table_entry_T *mib_tbl,void *val)
{
	int i = 0;
	int total_size = mib_tbl->total_size;
	int unit_size = mib_tbl->unit_size;
	if(num > total_size/unit_size)
		num=total_size/unit_size;
	for(i=0;i<num;i++)
	{
		if(0==memcmp(pmib+offset+i*unit_size,val,unit_size))
			break;
	}

	/*not found*/
	if(i == num){
	#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
		RTL_LOG_DBG_A("APMIB[%s] num[%d] total[%d] - mod_tblentry FAILED - not found", mib_tbl->name, num, mib_tbl->total_size/mib_tbl->unit_size);
	#endif	
		return 0;
	}
	#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
	RTL_LOG_DBG_A("APMIB[%s] num[%d] total[%d] - mod_tblentry SUCCESS - found", mib_tbl->name, num, mib_tbl->total_size/mib_tbl->unit_size);	
	#endif
	memcpy(pmib+offset+i*unit_size,val+unit_size,unit_size);
	return 1;
}
#endif /* #if defined(MIB_MOD_TBL_ENTRY) */

int delete_all_tblentry(void *pmib, unsigned int offset, int num,const mib_table_entry_T *mib_tbl)
{
#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
	RTL_LOG_DBG_A("APMIB[%s] num[%d] total[%d] - delete_all_tblentry SUCCESS", mib_tbl->name, num, mib_tbl->total_size/mib_tbl->unit_size);
#endif	
	memset(pmib+offset,0x0,mib_tbl->total_size);
	return 1;
}
int update_tblentry(void *pmib,unsigned int offset,int num,const mib_table_entry_T *mib_tbl,void *old, void *newone)
{
	int i=0;
	int total_size = mib_tbl->total_size;
	int unit_size = mib_tbl->unit_size;
	if(num > total_size/unit_size)
		return 0;
	for(i=0;i<num;i++)
	{
		if(0==memcmp(pmib+offset+i*unit_size,old,unit_size))
		{
			break;
		}
	}
	if(i == num){
	#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
		RTL_LOG_DBG_A("APMIB[%s] num[%d] total[%d] - update_tblentry FAILED - not found", mib_tbl->name, num, mib_tbl->total_size);
	#endif
		return 0;
	}

	/*found*/
	#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
	RTL_LOG_DBG_A("APMIB[%s] num[%d] total[%d] - update_tblentry SUCCESS - found", mib_tbl->name, num, mib_tbl->total_size);
	#endif
	memcpy(pmib+offset+i*unit_size,newone,unit_size);
	return 1;

}

int get_tblentry(void *pmib,unsigned int offset,int num,const mib_table_entry_T *mib_tbl,void *val, int index)

{
	if(index > num){
	#ifdef RTL_FLASH_TOOL_SET_MULTI_MIB
		RTL_LOG_DBG_A("APMIB[%s] num[%d] total[%d] - get_tblentry FAILED - index too high[%d]", mib_tbl->name, num, mib_tbl->total_size, index);
	#endif
		return 0;
	}
		
	memcpy(val,pmib+offset+(index-1)*mib_tbl->unit_size,mib_tbl->unit_size);
	return 1;
}


#else
///////////////////////////////////////////////////////////////////////////////
static int init_linkchain(LINKCHAIN_Tp pLinkChain, int size, int num)
{
	FILTER_Tp entry;
	int offset=sizeof(FILTER_Tp)*2;
	char *pBuf;
	int i;

	pLinkChain->realsize = size;

	if (size%4)
		size = (size/4+1)*4;

	pBuf = calloc(num, size+offset);
	if ( pBuf == NULL )
		return 0;

	pLinkChain->buf = pBuf;
	pLinkChain->pUsedList = NULL;
	pLinkChain->pFreeList = NULL;
	entry = (FILTER_Tp)pBuf;

	ADD_LINK_LIST(pLinkChain->pFreeList, entry);
	for (i=1; i<num; i++) {
		entry = (FILTER_Tp)&pBuf[i*(size+offset)];
		ADD_LINK_LIST(pLinkChain->pFreeList, entry);
	}

	pLinkChain->size = size;
	pLinkChain->num = num;
	pLinkChain->usedNum = 0;
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
static int add_linkchain(LINKCHAIN_Tp pLinkChain, char *val)
{
	FILTER_Tp entry;

	// get a free entry
	entry = pLinkChain->pFreeList;
	if (entry == NULL)
		return 0;

	if (entry->next==pLinkChain->pFreeList)
		pLinkChain->pFreeList = NULL;
	else
		pLinkChain->pFreeList = entry->next;

	REMOVE_LINK_LIST(entry);

	// copy content
	memcpy(entry->val, val, pLinkChain->realsize);

	// add to used list
	if (pLinkChain->pUsedList == NULL) {
		ADD_LINK_LIST(pLinkChain->pUsedList, entry);
	}
	else {
		ADD_LINK_LIST(pLinkChain->pUsedList->prev, entry);
	}
	pLinkChain->usedNum++;
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
static int delete_linkchain(LINKCHAIN_Tp pLinkChain, char *val)
{
	FILTER_Tp curEntry=pLinkChain->pUsedList;

	while (curEntry != NULL) {
		if ( !memcmp(curEntry->val,(unsigned char *)val,pLinkChain->compareLen) ) {
				if (curEntry == pLinkChain->pUsedList) {
					if ( pLinkChain->pUsedList->next != pLinkChain->pUsedList )
						pLinkChain->pUsedList = pLinkChain->pUsedList->next;
					else
						pLinkChain->pUsedList = NULL;
				}
				REMOVE_LINK_LIST(curEntry);
				ADD_LINK_LIST(pLinkChain->pFreeList, curEntry);
				pLinkChain->usedNum--;
				return 1;
		}
		if ( curEntry->next == pLinkChain->pUsedList )
		{
			return 0;
		}
		curEntry = curEntry->next;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
static void delete_all_linkchain(LINKCHAIN_Tp pLinkChain)
{
	FILTER_Tp curEntry;

	if (pLinkChain->pUsedList==NULL)
		return;

	// search for matched mac address
	while (pLinkChain->pUsedList) {
		curEntry = pLinkChain->pUsedList;
		if (pLinkChain->pUsedList->next != pLinkChain->pUsedList)
			pLinkChain->pUsedList = pLinkChain->pUsedList->next;
		else
			pLinkChain->pUsedList = NULL;

		REMOVE_LINK_LIST(curEntry);
		ADD_LINK_LIST(pLinkChain->pFreeList, curEntry);
		pLinkChain->usedNum--;
	}
}

///////////////////////////////////////////////////////////////////////////////
static int get_linkchain(LINKCHAIN_Tp pLinkChain, char *val, int index)
{
	FILTER_Tp curEntry=pLinkChain->pUsedList;

	if ( curEntry == NULL || index > pLinkChain->usedNum)
 		return 0;

	while (--index > 0)
        	curEntry = curEntry->next;
	
	memcpy( (unsigned char *)val, curEntry->val, pLinkChain->realsize);

	return 1;
}

int update_linkchain(int fmt, void *Entry_old, void *Entry_new, int type_size)
{
	LINKCHAIN_Tp pLinkChain=NULL;
	FILTER_Tp curEntry;
	void *entry;
	int i; 
	int entry_cmp;
#ifdef HOME_GATEWAY
	 	if(fmt==PORTFW_ARRAY_T){
			pLinkChain = &portFwChain;	 
		}else if(fmt == IPFILTER_ARRAY_T){
			pLinkChain = &ipFilterChain;
		}else if(fmt == PORTFILTER_ARRAY_T){
			pLinkChain = &portFilterChain;
		}else if(fmt == MACFILTER_ARRAY_T){
			pLinkChain = &macFilterChain;
		}else if(fmt == URLFILTER_ARRAY_T){
			pLinkChain = &urlFilterChain;
		}else	if(fmt==TRIGGERPORT_ARRAY_T){
				pLinkChain = &triggerPortChain;
		}else if(fmt==DHCPRSVDIP_ARRY_T){
			pLinkChain = &dhcpRsvdIpChain;			
		}
#ifdef CONFIG_RTL_AIRTIME
		else if(fmt == AIRTIME_ARRAY_T){
			pLinkChain = (wlan_idx==0)?(&airtimeChain):(&airtimeChain2);
		}
#endif /* CONFIG_RTL_AIRTIME */
#if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_)
		else if(fmt==DHCP_SERVER_OPTION_ARRAY_T){
			pLinkChain = &dhcpServerOptionChain;	 
		}
		else if(fmt==DHCP_CLIENT_OPTION_ARRAY_T){
			pLinkChain = &dhcpClientOptionChain;	 
		}
		else if(fmt==DHCPS_SERVING_POOL_ARRAY_T){
			pLinkChain = &dhcpsServingPoolChain;	 
		}
#endif /* #if defined(_PRMT_X_TELEFONICA_ES_DHCPOPTION_) */
#ifdef ROUTE_SUPPORT				
		if(fmt==STATICROUTE_ARRAY_T){
		 	pLinkChain = &staticRouteChain;
		}
#endif			
#ifdef CHECK_IPV6_DHCP_OPTION				
		if(fmt==DHCP6_OPTION16_ARRAY_T){
		 	pLinkChain = &dhcp6Option16Chain;
		}
		if(fmt==DHCP6_OPTION17_ARRAY_T){
		 	pLinkChain = &dhcp6Option17Chain;
		}		
#endif
#else
		 if(fmt==DHCPRSVDIP_ARRY_T){
			pLinkChain = &dhcpRsvdIpChain;	
		}		
#endif			
#ifdef _SUPPORT_CAPTIVEPORTAL_PROFILE_
		if(fmt==CAP_PORTAL_ALLOW_ARRAY_T){
		 	pLinkChain = &capPortalAllowChain;
		}
#endif

#if defined(VLAN_CONFIG_SUPPORTED)
		 if(fmt==VLANCONFIG_ARRAY_T){
		 	pLinkChain = &vlanConfigChain;	
		}
#endif	
	curEntry = pLinkChain->pUsedList;
	for(i=0;i<pLinkChain->usedNum;i++){
		entry = curEntry->val;
		entry_cmp=memcmp(entry, Entry_old, type_size );
		if(entry_cmp ==0){
		//	fprintf(stderr,"find the entry to update!\n");
			memcpy(entry, Entry_new, type_size);
			break;
		}
		curEntry = curEntry->next;
	}
	return 1;
}
#endif

#ifdef COMPRESS_MIB_SETTING

#define N		 4096	/* size of ring buffer */
#define F		   18	/* upper limit for match_length */
#define THRESHOLD	2   /* encode string into position and length if match_length is greater than this */
static unsigned char *text_buf;	/* ring buffer of size N, with extra F-1 bytes to facilitate string comparison */
#define LZSS_TYPE	unsigned short
#define NIL			N	/* index for root of binary search trees */
struct lzss_buffer {
	unsigned char	text_buf[N + F - 1];
	LZSS_TYPE	lson[N + 1];
	LZSS_TYPE	rson[N + 257];
	LZSS_TYPE	dad[N + 1];
};
static LZSS_TYPE		match_position, match_length;  /* of longest match.  These are set by the InsertNode() procedure. */
static LZSS_TYPE		*lson, *rson, *dad;  /* left & right children & parents -- These constitute binary search trees. */

void InsertNode(LZSS_TYPE r)
	/* Inserts string of length F, text_buf[r..r+F-1], into one of the
	   trees (text_buf[r]'th tree) and returns the longest-match position
	   and length via the global variables match_position and match_length.
	   If match_length = F, then removes the old node in favor of the new
	   one, because the old one will be deleted sooner.
	   Note r plays double role, as tree node and position in buffer. */
{
	LZSS_TYPE  i, p, cmp;
	unsigned char  *key;

	cmp = 1;
	key = &text_buf[r];
	p = N + 1 + key[0];
	rson[r] = lson[r] = NIL;
	match_length = 0;
	while(1) {
		if (cmp >= 0) {
			if (rson[p] != NIL)
				p = rson[p];
			else {
				rson[p] = r;
				dad[r] = p;
				return;
			}
		} else {
			if (lson[p] != NIL)
				p = lson[p];
			else {
				lson[p] = r;
				dad[r] = p;
				return;
			}
		}
		for (i = 1; i < F; i++)
			if ((cmp = key[i] - text_buf[p + i]) != 0)
				break;
		if (i > match_length) {
			match_position = p;
			if ((match_length = i) >= F)
				break;
		}
	}
	dad[r] = dad[p];
	lson[r] = lson[p];
	rson[r] = rson[p];
	dad[lson[p]] = r;
	dad[rson[p]] = r;
	if (rson[dad[p]] == p)
		rson[dad[p]] = r;
	else
		lson[dad[p]] = r;
	dad[p] = NIL;  /* remove p */
}

void InitTree(void)  /* initialize trees */
{
	int  i;

	/* For i = 0 to N - 1, rson[i] and lson[i] will be the right and
	   left children of node i.  These nodes need not be initialized.
	   Also, dad[i] is the parent of node i.  These are initialized to
	   NIL (= N), which stands for 'not used.'
	   For i = 0 to 255, rson[N + i + 1] is the root of the tree
	   for strings that begin with character i.  These are initialized
	   to NIL.  Note there are 256 trees. */

	for (i = N + 1; i <= N + 256; i++)
		rson[i] = NIL;
	for (i = 0; i < N; i++)
		dad[i] = NIL;
}

void DeleteNode(LZSS_TYPE p)  /* deletes node p from tree */
{
	LZSS_TYPE  q;
	
	if (dad[p] == NIL)
		return;  /* not in tree */
	if (rson[p] == NIL)
		q = lson[p];
	else if (lson[p] == NIL)
		q = rson[p];
	else {
		q = lson[p];
		if (rson[q] != NIL) {
			do {
				q = rson[q];
			} while (rson[q] != NIL);
			rson[dad[q]] = lson[q];
			dad[lson[q]] = dad[q];
			lson[q] = lson[p];
			dad[lson[p]] = q;
		}
		rson[q] = rson[p];
		dad[rson[p]] = q;
	}
	dad[q] = dad[p];
	if (rson[dad[p]] == p)
		rson[dad[p]] = q;
	else
		lson[dad[p]] = q;
	dad[p] = NIL;
}
int Encode(unsigned char *ucInput, unsigned int inLen, unsigned char *ucOutput)
{
	
#if defined(CHEAT_COMPRESS_MIB_SETTING)
	memcpy(ucOutput, ucInput, inLen);
	return inLen;
#else
	
	LZSS_TYPE  i, len, r, s, last_match_length, code_buf_ptr;
	unsigned char c;
	unsigned char  code_buf[17], mask;
	unsigned int ulPos=0;
	int enIdx=0;

	struct lzss_buffer *lzssbuf;

	if (0 != (lzssbuf = malloc(sizeof(struct lzss_buffer)))) {
		memset(lzssbuf, 0, sizeof(struct lzss_buffer));
		text_buf = lzssbuf->text_buf;
		rson = lzssbuf->rson;
		lson = lzssbuf->lson;
		dad = lzssbuf->dad;
	} else {
		return 0;
	}
	
	InitTree();  /* initialize trees */
	code_buf[0] = 0;  /* code_buf[1..16] saves eight units of code, and
		code_buf[0] works as eight flags, "1" representing that the unit
		is an unencoded letter (1 byte), "0" a position-and-length pair
		(2 bytes).  Thus, eight units require at most 16 bytes of code. */
	code_buf_ptr = mask = 1;
	s = 0;
	r = N - F;
	for (i = s; i < r; i++)
		text_buf[i] = ' ';  /* Clear the buffer with
		any character that will appear often. */

	for (len = 0; (len < F) && ulPos < inLen; len++)
		text_buf[r + len] = ucInput[ulPos++];  /* Read F bytes into the last F bytes of the buffer */
	
	//if ((textsize = len) == 0) return;  /* text of size zero */
	if (len == 0) {
		enIdx = 0;
		goto finished;
	}
	
	for (i = 1; i <= F; i++)
		InsertNode(r - i);  /* Insert the F strings,
		each of which begins with one or more 'space' characters.  Note
		the order in which these strings are inserted.  This way,
		degenerate trees will be less likely to occur. */
	InsertNode(r);  /* Finally, insert the whole string just read.  The
		global variables match_length and match_position are set. */
	do {
		if (match_length > len) match_length = len;  /* match_length
			may be spuriously long near the end of text. */
		if (match_length <= THRESHOLD) {
			match_length = 1;  /* Not long enough match.  Send one byte. */
			code_buf[0] |= mask;  /* 'send one byte' flag */
			code_buf[code_buf_ptr++] = text_buf[r];  /* Send uncoded. */
		} else {
			code_buf[code_buf_ptr++] = (unsigned char) match_position;
			code_buf[code_buf_ptr++] = (unsigned char)
				(((match_position >> 4) & 0xf0)
			  | (match_length - (THRESHOLD + 1)));  /* Send position and
					length pair. Note match_length > THRESHOLD. */
		}
		if ((mask <<= 1) == 0) {  /* Shift mask left one bit. */
			for (i = 0; i < code_buf_ptr; i++)  /* Send at most 8 units of */
				ucOutput[enIdx++]=code_buf[i];
			//codesize += code_buf_ptr;
			code_buf[0] = 0;  code_buf_ptr = mask = 1;
		}
		last_match_length = match_length;

		for (i = 0; i< last_match_length && 
			ulPos < inLen; i++){
			c = ucInput[ulPos++];
			DeleteNode(s);		/* Delete old strings and */
			text_buf[s] = c;	/* read new bytes */
			if (s < F - 1)
				text_buf[s + N] = c;  /* If the position is near the end of buffer, extend the buffer to make string comparison easier. */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
				/* Since this is a ring buffer, increment the position
				   modulo N. */
			InsertNode(r);	/* Register the string in text_buf[r..r+F-1] */
		}
		
		while (i++ < last_match_length) {	/* After the end of text, */
			DeleteNode(s);					/* no need to read, but */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
			if (--len) InsertNode(r);		/* buffer may not be empty. */
		}
	} while (len > 0);	/* until length of string to be processed is zero */
	if (code_buf_ptr > 1) {		/* Send remaining code. */
		for (i = 0; i < code_buf_ptr; i++) 
			ucOutput[enIdx++]=code_buf[i];
		//codesize += code_buf_ptr;
	}
finished:
	free(lzssbuf);
	return enIdx;
	
#endif //#if defined(CHEAT_COMPRESS_MIB_SETTING)
}

int Decode(unsigned char *ucInput, unsigned int inLen, unsigned char *ucOutput)	/* Just the reverse of Encode(). */
{
	
#if defined(CHEAT_COMPRESS_MIB_SETTING)
	memcpy(ucOutput, ucInput, inLen);
	return inLen;
#else
	int  i, j, k, r, c;
	unsigned int  flags;
	unsigned int ulPos=0;
	unsigned int ulExpLen=0;

	if ((text_buf = malloc( N + F - 1 )) == 0) {
		//fprintf(stderr, "fail to get mem %s:%d\n", __FUNCTION__, __LINE__);
		return 0;
	}
	
	for (i = 0; i < N - F; i++)
		text_buf[i] = ' ';
	r = N - F;
	flags = 0;
	while(1) {
		if (((flags >>= 1) & 256) == 0) {
			c = ucInput[ulPos++];
			if (ulPos>inLen)
				break;
			flags = c | 0xff00;		/* uses higher byte cleverly */
		}							/* to count eight */
		if (flags & 1) {
			c = ucInput[ulPos++];
			if ( ulPos > inLen )
				break;
			ucOutput[ulExpLen++] = c;
			text_buf[r++] = c;
			r &= (N - 1);
		} else {
			i = ucInput[ulPos++];
			if ( ulPos > inLen ) break;
			j = ucInput[ulPos++];
			if ( ulPos > inLen ) break;
			
			i |= ((j & 0xf0) << 4);
			j = (j & 0x0f) + THRESHOLD;
			for (k = 0; k <= j; k++) {
				c = text_buf[(i + k) & (N - 1)];
				ucOutput[ulExpLen++] = c;
				text_buf[r++] = c;
				r &= (N - 1);
			}
		}
	}

	free(text_buf);
	return ulExpLen;
#endif //#if defined(CHEAT_COMPRESS_MIB_SETTING)	
}

int mib_get_flash_offset(CONFIG_DATA_T type)
{
	switch(type)
	{
		case HW_SETTING:
			return HW_SETTING_OFFSET;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		case BLUETOOTH_HW_SETTING:
			return BLUETOOTH_HW_SETTING_OFFSET;
#endif

#ifdef CUSTOMER_HW_SETTING_SUPPORT
		case CUSTOMER_HW_SETTING:
			return CUSTOMER_HW_SETTING_OFFSET;
#endif
#ifdef BACKUP_CURRENT_SETTING_SUPPORT
		case BACKUP_CURRENT_SETTING:
			return BACKUP_CURRENT_SETTING_OFFSET;
#endif
#ifdef RF_DPK_SETTING_SUPPORT
		case RF_DPK_SETTING:
			return RF_DPK_SETTING_OFFSET;
#endif /* RF_DPK_SETTING_SUPPORT */
		case DEFAULT_SETTING:
			return DEFAULT_SETTING_OFFSET;
#ifdef BACKUP_DEFAULT_SETTING_SUPPORT
		case BACKUP_DEFAULT_SETTING:
			return BACKUP_DEFAULT_SETTING_OFFSET;
#endif
		case CURRENT_SETTING:
			return CURRENT_SETTING_OFFSET;
			
#ifdef BACKUP_HARDWARE_SETTING_SUPPORT
		case BACKUP_HW_SETTING:
			return BACKUP_HARDWARE_SETTING_OFFSET;
			
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		case BACKUP_BLUETOOTH_HW_SETTING:
			return BACKUP_BLUETOOTH_HW_SETTING_OFFSET;
#endif 
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		case BACKUP_CUSTOMER_HW_SETTING:
			return BACKUP_CUSTOMER_HW_SETTING_OFFSET;
#endif 

#endif 

		default:
			return -1;
	}
	
}

unsigned int mib_get_real_len(CONFIG_DATA_T type)
{
	switch(type)
	{
		case HW_SETTING:
			return HW_SETTING_SECTOR_LEN;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		case BLUETOOTH_HW_SETTING:
			return BLUETOOTH_HW_SETTING_SECTOR_LEN;
#endif

#ifdef CUSTOMER_HW_SETTING_SUPPORT
		case CUSTOMER_HW_SETTING:
			return CUSTOMER_HW_SETTING_SECTOR_LEN;
#endif

#ifdef BACKUP_CURRENT_SETTING_SUPPORT
		case BACKUP_CURRENT_SETTING:
			return BACKUP_CURRENT_SETTING_SECTOR_LEN;
#endif
#ifdef RF_DPK_SETTING_SUPPORT
		case RF_DPK_SETTING:
			return RF_DPK_SETTING_SECTOR_LEN;
#endif /* RF_DPK_SETTING_SUPPORT */
		case DEFAULT_SETTING:
			return DEFAULT_SETTING_SECTOR_LEN;
#ifdef BACKUP_DEFAULT_SETTING_SUPPORT
		case BACKUP_DEFAULT_SETTING:
			return BACKUP_DEFAULT_SETTING_SECTOR_LEN;
#endif
		case CURRENT_SETTING:
			return CURRENT_SETTING_SECTOR_LEN;
#ifdef BACKUP_HARDWARE_SETTING_SUPPORT
		case BACKUP_HW_SETTING:
			return HW_SETTING_SECTOR_LEN;
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		case BACKUP_BLUETOOTH_HW_SETTING:
			return BLUETOOTH_HW_SETTING_SECTOR_LEN;
#endif 
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		case BACKUP_CUSTOMER_HW_SETTING:
			return CUSTOMER_HW_SETTING_SECTOR_LEN;
#endif
#endif 

		default:			
			return 0;
	}
	
}

unsigned int mib_get_setting_len(CONFIG_DATA_T type)
{
	switch(type)
	{
		case HW_SETTING:
			return sizeof(HW_SETTING_T);
			
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		case BLUETOOTH_HW_SETTING:
			return sizeof(BLUETOOTH_HW_SETTING_T);
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		case CUSTOMER_HW_SETTING:
			return sizeof(CUSTOMER_HW_SETTING_T);
#endif
#ifdef RF_DPK_SETTING_SUPPORT
		case RF_DPK_SETTING:
			return sizeof(RF_DPK_SETTING_T);
#endif /* RF_DPK_SETTING_SUPPORT */
		case DEFAULT_SETTING:
		case CURRENT_SETTING:
#ifdef BACKUP_CURRENT_SETTING_SUPPORT
		case BACKUP_CURRENT_SETTING:
#endif
			return sizeof(APMIB_T);
		default:			
			return 0;
	}
	
}
PARAM_HEADER_T* mib_get_header(CONFIG_DATA_T type)
{
	
	switch(type)
	{
#ifndef HEADER_LEN_INT
		case HW_SETTING:
			return &hsHeader;
#endif
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		case BLUETOOTH_HW_SETTING:
			return &blueToothHwHeader;
#endif

#ifdef CUSTOMER_HW_SETTING_SUPPORT
		case CUSTOMER_HW_SETTING:
			return &customerHwHeader;
#endif
#ifdef RF_DPK_SETTING_SUPPORT
		case RF_DPK_SETTING:
			return &rfDpkHeader;
#endif /* RF_DPK_SETTING_SUPPORT */
		case DEFAULT_SETTING:
			return &dsHeader;
#ifdef BACKUP_CURRENT_SETTING_SUPPORT
		case BACKUP_CURRENT_SETTING:
#endif
		case CURRENT_SETTING:
			return &csHeader;
		default :
			return NULL;
		
	}

}
unsigned int mib_compress_write(CONFIG_DATA_T type, unsigned char *data)
{
	unsigned char* pContent = NULL;

	COMPRESS_MIB_HEADER_T compHeader;
	unsigned char *expPtr, *compPtr;
	unsigned int expLen = 0;
	unsigned int compLen;
	unsigned int real_size = 0;
#ifdef HEADER_LEN_INT
	HW_PARAM_HEADER_T *phwHeader;
#endif
	PARAM_HEADER_T *pheader;
	int dst;




	dst = mib_get_flash_offset(type);
	//real_size = mib_get_flash_offset(type);
	real_size = mib_get_real_len(type);
#ifdef HEADER_LEN_INT
	if(type==HW_SETTING)
	{
		phwHeader=&hsHeader;
		expLen = phwHeader->len + sizeof(HW_PARAM_HEADER_T);
	}
	else
#endif
	{
		pheader = mib_get_header(type);
		expLen = pheader->len + sizeof(PARAM_HEADER_T);
	}
	if( (compPtr = malloc(real_size)) == NULL)
	{
		printf("\r\n ERR!! malloc  size %u failed! __[%s-%u]\n",real_size,__FILE__,__LINE__);
	}
	if( (expPtr = malloc(expLen)) == NULL)
	{
		printf("\r\n ERR!! malloc  size %u failed! __[%s-%u]\n",expLen,__FILE__,__LINE__);
		if(compPtr != NULL)
			free(compPtr);
	}

	if(compPtr != NULL && expPtr!= NULL)
	{
#ifdef HEADER_LEN_INT
		if(type==HW_SETTING)
		{
			pContent = &expPtr[sizeof(HW_PARAM_HEADER_T)];	// point to start of MIB data 

			memcpy(pContent, data, WORD_SWAP(phwHeader->len));
			phwHeader->len=WORD_SWAP(phwHeader->len);
			memcpy(expPtr, phwHeader, sizeof(HW_PARAM_HEADER_T));
			phwHeader->len=WORD_SWAP(phwHeader->len);
		}
		else
#endif
		{
			pContent = &expPtr[sizeof(PARAM_HEADER_T)];	// point to start of MIB data 

			memcpy(pContent, data, pheader->len);
			pheader->len=HEADER_SWAP(pheader->len);
			memcpy(expPtr, pheader, sizeof(PARAM_HEADER_T));	
			pheader->len=HEADER_SWAP(pheader->len);
		}
		compLen = Encode(expPtr, expLen, compPtr+sizeof(COMPRESS_MIB_HEADER_T));
		
		if(type == HW_SETTING)
			sprintf((char *)compHeader.signature,"%s",COMP_HS_SIGNATURE);
		
#ifdef BLUETOOTH_HW_SETTING_SUPPORT
		else if(type == BLUETOOTH_HW_SETTING)
			sprintf((char *)compHeader.signature,"%s",COMP_BT_SIGNATURE);
#endif
#ifdef CUSTOMER_HW_SETTING_SUPPORT
		else if(type == CUSTOMER_HW_SETTING)
			sprintf((char *)compHeader.signature,"%s",COMP_US_SIGNATURE);
#endif
#ifdef RF_DPK_SETTING_SUPPORT
		else if(type == RF_DPK_SETTING)
			sprintf((char *)compHeader.signature,"%s",COMP_RS_SIGNATURE);
#endif /* RF_DPK_SETTING_SUPPORT */
#ifdef BACKUP_CURRENT_SETTING_SUPPORT
		else if(type == BACKUP_CURRENT_SETTING)
			sprintf((char *)compHeader.signature,"%s",COMP_BC_SIGNATURE);
#endif

		else if(type == DEFAULT_SETTING)
			sprintf((char *)compHeader.signature,"%s",COMP_DS_SIGNATURE);
		else
			sprintf((char *)compHeader.signature,"%s",COMP_CS_SIGNATURE);

		compHeader.compRate = WORD_SWAP((expLen/compLen)+1);
		compHeader.compLen =DWORD_SWAP(compLen);
		memcpy(compPtr, &compHeader, sizeof(COMPRESS_MIB_HEADER_T));
		if ( flash_write((char *)compPtr, dst, compLen+sizeof(COMPRESS_MIB_HEADER_T))==0 ) {
			COMP_TRACE(stderr,"Write flash compress setting[%u] failed![%s-%u]\n",type,__FILE__,__LINE__);
			
			if(expPtr)
				free(expPtr);
			if(compPtr)
				free(compPtr);
			return 0;
		}
		else
		{
		//	COMP_TRACE(stderr,"\r\n Compress [%u] to [%u]. Compress rate=%u, __[%s-%u]",expLen,compLen,compHeader.compRate ,__FILE__,__LINE__);

		//	COMP_TRACE(stderr,"\r\n Flash write to 0x%x len=%d\n",dst,compLen+sizeof(COMPRESS_MIB_HEADER_T));

		//	COMP_TRACE(stderr,"\r\n");
			if(expPtr)
				free(expPtr);
			if(compPtr)
				free(compPtr);
			return 1;
		}
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);			
	}

	if(expPtr)
		free(expPtr);
	if(compPtr)
		free(compPtr);
	return 0;
}

int mib_updateDef_compress_write(CONFIG_DATA_T type, char *data, PARAM_HEADER_T *pheader)
{
	unsigned char* pContent = NULL;

	COMPRESS_MIB_HEADER_T compHeader;
	unsigned char *expPtr, *compPtr;
	unsigned int expLen = 0;
	unsigned int compLen;
	unsigned int real_size = 0;
	//int zipRate=0;
	char *pcomp_sig;
	int dst = mib_get_flash_offset(type);

	if(dst == 0)
	{
		printf("\r\n ERR!! no flash offset! __[%s-%u]\n",__FILE__,__LINE__);
		return 0;
	}
	
	switch(type)
	{
		case HW_SETTING:
			pcomp_sig = COMP_HS_SIGNATURE;
			break;
		case DEFAULT_SETTING:
			pcomp_sig = COMP_DS_SIGNATURE;
			break;
		case CURRENT_SETTING:
			pcomp_sig = COMP_CS_SIGNATURE;
			break;
		default:
			printf("\r\n ERR!! no type match __[%s-%u]\n",__FILE__,__LINE__);
			return 0;

	}
#ifdef HEADER_LEN_INT
		if(type==HW_SETTING)
		{
			expLen = ((HW_PARAM_HEADER_Tp)pheader)->len+sizeof(HW_PARAM_HEADER_T);
		}
		else
#endif
		expLen = pheader->len+sizeof(PARAM_HEADER_T);
	if(expLen == 0)
	{
		printf("\r\n ERR!! no expLen! __[%s-%u]\n",__FILE__,__LINE__);
		return 0;
	}
	real_size = mib_get_real_len(type);
	if(real_size == 0)
	{
		printf("\r\n ERR!! no expLen! __[%s-%u]\n",__FILE__,__LINE__);
		return 0;
	}
	
	if( (compPtr = malloc(real_size)) == NULL)
	{
		printf("\r\n ERR!! malloc  size %u failed! __[%s-%u]\n",real_size,__FILE__,__LINE__);
	}

	if( (expPtr = malloc(expLen)) == NULL)
	{
		printf("\r\n ERR!! malloc  size %u failed! __[%s-%u]\n",expLen,__FILE__,__LINE__);
		if(compPtr != NULL)
			free(compPtr);
	}
	
	if(compPtr != NULL && expPtr!= NULL)
	{
		//int status;
#ifdef HEADER_LEN_INT
		if(type==HW_SETTING)
		{
			pContent = &expPtr[sizeof(HW_PARAM_HEADER_T)];	// point to start of MIB data 
			memcpy(pContent, data, ((HW_PARAM_HEADER_Tp)pheader)->len);
			memcpy(expPtr, pheader, sizeof(HW_PARAM_HEADER_T));
		}
		else
#endif
		{
			pContent = &expPtr[sizeof(PARAM_HEADER_T)];	// point to start of MIB data 
			memcpy(pContent, data, pheader->len);
			pheader->len=HEADER_SWAP(pheader->len);
			memcpy(expPtr, pheader, sizeof(PARAM_HEADER_T));
			pheader->len=HEADER_SWAP(pheader->len);
		}
		compLen = Encode(expPtr, expLen, compPtr+sizeof(COMPRESS_MIB_HEADER_T));
		sprintf((char *)compHeader.signature,"%s",pcomp_sig);
		compHeader.compRate = WORD_SWAP((expLen/compLen)+1);
		compHeader.compLen = DWORD_SWAP(compLen);
		memcpy(compPtr, &compHeader, sizeof(COMPRESS_MIB_HEADER_T));

		if ( flash_write((char *)compPtr, dst, compLen+sizeof(COMPRESS_MIB_HEADER_T))==0 )
//		if ( write(fh, (const void *)compPtr, compLen+sizeof(COMPRESS_MIB_HEADER_T))!=compLen+sizeof(COMPRESS_MIB_HEADER_T) ) 
		{
			printf("Write flash compress [%u] setting failed![%s-%u]\n",type,__FILE__,__LINE__);			
			return 0;
		}
		
		if(compPtr != NULL)
			free(compPtr);
		if(expPtr != NULL)
			free(expPtr);

		return 1;
			
	}
	else
	{
		return 0;
	}

	return 0;
}
#endif //#ifdef COMPRESS_MIB_SETTING

int save_cs_to_file(void)
{
	char *buf, *ptr=NULL;
	PARAM_HEADER_Tp pHeader;
	//unsigned char checksum;
	int len, fh;
	char tmpBuf[100];
#ifdef COMPRESS_MIB_SETTING
#ifdef MIB_TLV
	int tlv_content_len,compLen;
	unsigned char *pCompptr;
	COMPRESS_MIB_HEADER_Tp pcompHeader;
	char *pbuf;
#endif
#endif


#ifdef MIB_TLV
	len=mib_get_setting_len(CURRENT_SETTING)*4;
#else
	len = csHeader.len;
#endif

	len += sizeof(PARAM_HEADER_T);
	buf = malloc(len);
	if ( buf == NULL ) {
		strcpy(tmpBuf, "Allocate buffer failed!");
		return 0;
	}
//	fprintf(stderr,"%s %d\n",__FUNCTION__,__LINE__);
#ifdef __mips__
	fh = open("/web/config.dat", O_RDWR|O_CREAT|O_TRUNC);
#else
	fh = open("/web/config.dat", O_RDWR|O_CREAT|O_TRUNC);
#endif
	if (fh == -1) {
		printf("Create config file error!\n");
		free(buf);
		fprintf(stderr,"%s %d\n",__FUNCTION__,__LINE__);
		return 0;
	}

	pHeader = (PARAM_HEADER_Tp)buf;	
#ifdef MIB_TLV
	pbuf=malloc(csHeader.len);
	if(pbuf) {
		memcpy(pbuf, pMib, csHeader.len-1);
	} else {
		printf("%s %d malloc failed\n",__FUNCTION__,__LINE__);
		return 0;
	}
#else
	len = pHeader->len = csHeader.len;
	memcpy(&buf[sizeof(PARAM_HEADER_T)], pMib, len-1);
#endif

#ifdef _LITTLE_ENDIAN_
#ifdef VOIP_SUPPORT
	// rock: need swap here 
	// 1. write to share space (ex: save setting to config file)
	// 2. read from share space (ex: import config file) 
	pHeader->len  = WORD_SWAP(pHeader->len);
#else
	//pHeader->len  = WORD_SWAP(pHeader->len);
#endif

#ifdef MIB_TLV
	swap_mib_word_value(pbuf);
#else
	swap_mib_word_value((APMIB_Tp)&buf[sizeof(PARAM_HEADER_T)]);
#endif
#endif
	memcpy(pHeader->signature, csHeader.signature, SIGNATURE_LEN);
	ptr = (char *)&buf[sizeof(PARAM_HEADER_T)];
	
#ifdef COMPRESS_MIB_SETTING
#ifdef MIB_TLV
	//fprintf(stderr,"%s %d\n",__FUNCTION__,__LINE__);

	tlv_content_len=0;
	if(mib_tlv_save(CURRENT_SETTING, (void*)pbuf, (unsigned char *)ptr, (unsigned int *)&tlv_content_len) == 1){
		if(tlv_content_len >= (mib_get_setting_len(CURRENT_SETTING)*4)){
			printf("TLV Data len is too long");
			close(fh);
			free(buf);
			free(pbuf);
			//fprintf(stderr,"%s %d tlv_content_len 0x%x len 0x%x\n",__FUNCTION__,__LINE__,tlv_content_len,len);
			return 0;
		}
		ptr[tlv_content_len] = CHECKSUM((unsigned char *)ptr, tlv_content_len);	
		pHeader->len=tlv_content_len+1; /*add checksum*/		
		pHeader->len=HEADER_SWAP(pHeader->len); /*endian*/
	}

	/*compress*/
	pCompptr = malloc((WEB_PAGE_OFFSET-CURRENT_SETTING_OFFSET)+sizeof(COMPRESS_MIB_HEADER_T));
	if(NULL == pCompptr){
			printf("malloc for Compress buffer failed!! \n");
			close(fh);
			free(buf);
			free(pbuf);
			//fprintf(stderr,"%s %d\n",__FUNCTION__,__LINE__);
			return 0;
	}
	compLen = Encode((unsigned char *)buf, HEADER_SWAP(pHeader->len)+sizeof(PARAM_HEADER_T), pCompptr+sizeof(COMPRESS_MIB_HEADER_T));
	pcompHeader=(COMPRESS_MIB_HEADER_Tp)pCompptr;
	memcpy(pcompHeader->signature,COMP_CS_SIGNATURE,COMP_SIGNATURE_LEN);
	pcompHeader->compRate = WORD_SWAP((HEADER_SWAP(pHeader->len)/compLen)+1);
	pcompHeader->compLen =DWORD_SWAP(compLen);
#endif	
#endif

#ifdef MIB_TLV
	//fprintf(stderr,"%s %d compLen %d\n",__FUNCTION__,__LINE__,compLen);

	if ( write(fh, pCompptr, compLen+sizeof(COMPRESS_MIB_HEADER_T)) != compLen+sizeof(COMPRESS_MIB_HEADER_T)) {
		printf("Write config file error!\n");
		close(fh);
		free(pCompptr);
		free(buf);
		free(pbuf);
		fprintf(stderr,"%s %d\n",__FUNCTION__,__LINE__);
		return 0;
	}
	//fprintf(stderr,"%s %d compLen %d\n",__FUNCTION__,__LINE__,compLen);
#else
	checksum = CHECKSUM(ptr, len-1);
	buf[sizeof(PARAM_HEADER_T)+len-1] = checksum;

	ptr = &buf[sizeof(PARAM_HEADER_T)];
	ENCODE_DATA(ptr, len);


	if ( write(fh, buf, len+sizeof(PARAM_HEADER_T)) != len+sizeof(PARAM_HEADER_T)) {
		printf("Write config file error!\n");
		close(fh);
		free(buf);
		return 0;
	}
#endif

	
	//fprintf(stderr,"%s %d compLen %d\n",__FUNCTION__,__LINE__,compLen);
close(fh);
sync();

#ifdef MIB_TLV	
	if(pCompptr) {
		free(pCompptr);
		pCompptr=NULL;
	}	
	free(pbuf);
#endif	
	free(buf);
#ifdef TXT_CONFIG_SUPPORT
#ifdef CONFIG_RTL_WEBPAGES_IN_ROOTFS
	system("cvcfg-gw -in /var/config.dat -ot /var/config.txt");
	sync();
	system("cp /var/config.dat /var/config.dat_bak");

	sleep(3);
	system("cp /var/config.txt /var/config.dat");

#else
	system("cvcfg-gw -in /web/config.dat -ot /web/config.txt");
	sync();
	system("cp /web/config.dat /web/config.dat_bak");

	sleep(3);
	system("cp /web/config.txt /web/config.dat");
#endif
#endif
	return 1;
}
//#endif // WEBS


#ifdef MIB_TLV

/*get mib value directly from tlv Block. Note: only can get simple mib*/
int tlv_simple_mib_get(unsigned int mib_id,unsigned char *from_data,unsigned int total_len, void *value)
{
	unsigned short tlv_tag;		
	unsigned short tlv_len;
	unsigned char *idx;
	int i=0,ret=-1;

	idx=from_data;	
	while(i<total_len)	
	{		
		memcpy(&tlv_tag, idx+i, sizeof(tlv_tag));		
		i+=sizeof(tlv_tag);					

		memcpy(&tlv_len, idx+i, sizeof(tlv_len));
		i+=sizeof(tlv_len);

		if(mib_id == tlv_tag) {
			/*found*/
			memcpy(value,idx+i,tlv_len);
			ret=0;
			break;
		}
		/*skip to next header*/
		i += tlv_len;
	}
	return ret;
}
int mib_search_by_id(const mib_table_entry_T *mib_tbl, unsigned short mib_id, unsigned char *pmib_num, const mib_table_entry_T **ppmib, unsigned int *offset)
{
	int i=0;
	const mib_table_entry_T *mib;
	unsigned short mib_num=0;
	
	memcpy(&mib_num, pmib_num, 1);
	
//printf("\r\n search mib_id=%u, offset=%u, mib_num=%u",mib_id,*offset, mib_num);
	
	
	for (i=0; mib_tbl[i].id; i++)
	{
//printf("\r\n mib_tbl[%u].mib_name=%s",i, mib_tbl[i].mib_name);
		mib = &mib_tbl[i];
		
		if(mib_id == mib_tbl[i].id
#ifdef TLV_LENGTH_INT
		|| (mib_id|MIBTBL_WLAN_TLV_INT)==mib_tbl[i].id
#endif
		)
		{
			*offset += mib->offset + mib->unit_size*mib_num;
			*ppmib = mib;
//printf("\r\n !! FIND at %s TBL !!",mib_tbl[i].mib_name);
			return 1;
		}
		else
		{
			if(mib_tbl[i].type >= TABLE_LIST_T)
			{
				
				if((mib->total_size%mib->unit_size) == 0 && mib_num < (mib->total_size /mib->unit_size))
				{					
					*offset += mib->offset + mib->unit_size*mib_num;
//printf("\r\n >> Entry %s TBL >>",mib->mib_name);
					if(mib_search_by_id(mib->next_mib_table, mib_id, pmib_num+1, ppmib, offset) == 1)
					{
						return 1;
					}
					else
					{
//printf("\r\n << Leave %s TBL <<",mib->mib_name);
						*offset -= mib->offset + mib->unit_size*mib_num;
					}
				}
				
			}
		}
	}
	return 0;
}

static int mib_init_value(unsigned char *ptlv_data_value, unsigned short tlv_len, const mib_table_entry_T *mib_tbl, void *data)
{
	unsigned int vInt;
	unsigned short vShort;
	unsigned char *pChar;
	
	
	
#if 0
int j=0;
fprintf(stderr,"\r\n mib_tbl->type = %u",mib_tbl->type);
fprintf(stderr,"\r\n %s = ",mib_tbl->name);
for(j=0; j<tlv_len; j++)
	fprintf(stderr,"0x%x_", *(ptlv_data_value+j));
fprintf(stderr,"\r\n");
#endif
			
	switch (mib_tbl->type)
	{
		case BYTE_T:
		case BYTE_ARRAY_T:
		case DWORD_ARRAY_T:
			pChar = (unsigned char *) data;
			memcpy(data, ptlv_data_value, tlv_len);			
			break;
			
		case WORD_T:
			pChar = (unsigned char *) data;
			memcpy(&vShort, ptlv_data_value, sizeof(vShort));			
			vShort = WORD_SWAP(vShort);
			memcpy(data, &vShort, sizeof(vShort));
			break;
			
		case DWORD_T:
			pChar = (unsigned char *) data;
			memcpy(&vInt, ptlv_data_value, sizeof(vInt));
			vInt = DWORD_SWAP(vInt);
			memcpy(data, &vInt, sizeof(vInt));
			break;
			
		case STRING_T:
			pChar = (unsigned char *) data;
			strncpy((char *)pChar, (char *)ptlv_data_value, mib_tbl->total_size);
			break;
			
		case IA_T:
			pChar = (unsigned char *) data;
			memcpy(data, ptlv_data_value, mib_tbl->total_size);
			
			break;
			
		case BYTE5_T:
		case BYTE6_T:
		case BYTE13_T:
			pChar = (unsigned char *) data;
			memcpy(data, ptlv_data_value, mib_tbl->total_size); // avoid alignment issue
			
			break;
			
#ifdef HOME_GATEWAY
#ifdef CONFIG_IPV6
		case RADVDPREFIX_T:
			{
				radvdCfgParam_Tp radvdCfg=(radvdCfgParam_Tp)data;
				memcpy(data, ptlv_data_value, mib_tbl->total_size);
				radvdCfg->interface.MaxRtrAdvInterval=DWORD_SWAP(radvdCfg->interface.MaxRtrAdvInterval);
				radvdCfg->interface.MinRtrAdvInterval=DWORD_SWAP(radvdCfg->interface.MinRtrAdvInterval);
				radvdCfg->interface.MinDelayBetweenRAs=DWORD_SWAP(radvdCfg->interface.MinDelayBetweenRAs);
				radvdCfg->interface.AdvLinkMTU=DWORD_SWAP(radvdCfg->interface.AdvLinkMTU);
				radvdCfg->interface.AdvReachableTime=DWORD_SWAP(radvdCfg->interface.AdvReachableTime);
				radvdCfg->interface.AdvRetransTimer=DWORD_SWAP(radvdCfg->interface.AdvRetransTimer);
				radvdCfg->interface.AdvDefaultLifetime=WORD_SWAP(radvdCfg->interface.AdvDefaultLifetime);
				break;
			}
		case DNSV6_T:
			{
				int i=0,j=0;
				dnsv6CfgParam_Tp dnsv6Cfg=(dnsv6CfgParam_Tp)data;
				memcpy(data, ptlv_data_value, mib_tbl->total_size);
				for(i=0;i<RR_MAX_NUM;i++){
					for(j=0;j<8;j++){
						dnsv6Cfg->rr[i].address[j]=WORD_SWAP(dnsv6Cfg->rr[i].address[j]);
					}
				}
				break;
			}
		case DHCPV6S_T:
			{				
				memcpy(data, ptlv_data_value, mib_tbl->total_size);
				break;
			}
		case DHCPV6C_T:
			{	
				dhcp6cCfgParam_Tp dhcp6cCfg=(dhcp6cCfgParam_Tp)data;
				memcpy(data, ptlv_data_value, mib_tbl->total_size);
				dhcp6cCfg->dhcp6pd.sla_id=DWORD_SWAP(dhcp6cCfg->dhcp6pd.sla_id);
				dhcp6cCfg->dhcp6pd.sla_len=DWORD_SWAP(dhcp6cCfg->dhcp6pd.sla_len);
				break;
			}
		case ADDR6_T:
			{	
				int i=0,j=0;
				daddrIPv6CfgParam_Tp daddrV6Cfg=(daddrIPv6CfgParam_Tp)data;		
				memcpy(data, ptlv_data_value, mib_tbl->total_size);
				daddrV6Cfg->enabled=DWORD_SWAP(daddrV6Cfg->enabled);
				for(i=0;i<2;i++)
					daddrV6Cfg->prefix_len[i]=DWORD_SWAP(daddrV6Cfg->prefix_len[i]);
				for(i=0;i<2;i++)
					for(j=0;j<8;j++)
						daddrV6Cfg->addrIPv6[i][j]=WORD_SWAP(daddrV6Cfg->addrIPv6[i][j]);
				break;
			}
		case ADDRV6_T:
			{	
				int i=0;
				addr6CfgParam_Tp addr6Cfg=(addr6CfgParam_Tp)data;
				memcpy(data, ptlv_data_value, mib_tbl->total_size);
				addr6Cfg->prefix_len=DWORD_SWAP(addr6Cfg->prefix_len);
				for(i=0;i<8;i++)
					addr6Cfg->addrIPv6[i]=WORD_SWAP(addr6Cfg->addrIPv6[i]);
				break;
			}
		case TUNNEL6_T:
			memcpy(data, ptlv_data_value, mib_tbl->total_size);
			break;		
#endif
#endif
#ifdef VOIP_SUPPORT
#ifdef VOIP_SUPPORT_TLV_CFG
		case VOIP_T:
			break;
#else
		case VOIP_T:
			pChar = (unsigned char *) data;
			memcpy(data, ptlv_data_value, tlv_len);
			break;
#endif
#endif
#ifdef RTK_CAPWAP
		case CAPWAP_ALL_WLANS_CONFIG_T:
//                        printf("CAPWAP_WLAN_CONFIGS_T: tlv_len=%u, mib_tbl->total_size=%u\n", tlv_len, mib_tbl->total_size);
			pChar = (unsigned char *) data;
			memcpy(data, ptlv_data_value, mib_tbl->total_size);
			break;
#endif
		default :
			fprintf(stderr,"\r\n ERR!no mib_name[%s] type[%u]. __[%s-%u]",mib_tbl->name, mib_tbl->type,__FILE__,__LINE__);			
			return 0;
		
		
	}
		
	return 1;
}

unsigned short find_same_tag_times(unsigned char *pdata_array, unsigned int data_size)
{
	unsigned short first_tlv_tag;
	unsigned short tlv_tag;	
	unsigned short tlv_len;
	unsigned short times=0;
	unsigned char *idx = pdata_array;
	int i=0;
	
	memcpy(&first_tlv_tag, idx, sizeof(first_tlv_tag));
	
	while(i<data_size)
	{
		memcpy(&tlv_tag, idx+i, sizeof(tlv_tag));
		i+=sizeof(tlv_tag);			
		
		if(tlv_tag == first_tlv_tag)
			times++;
		
		memcpy(&tlv_len, idx+i, sizeof(tlv_len));
		i+=sizeof(tlv_len);
		tlv_len = WORD_SWAP(tlv_len);
		i+=tlv_len;
		
	}
	
	return times;
}

/* TLV to --> Data structure */
unsigned int mib_tlv_init_from(const mib_table_entry_T *mib_root_tbl, unsigned char *pdata_array, void *pfile, unsigned int data_size, unsigned int *pmib_root_offset)
{	
	unsigned char *idx;		
	int i;
	//int j;
	unsigned short tlv_tag;		
	unsigned short tlv_len;	
#ifdef TLV_LENGTH_INT
	unsigned int tlv_int_len=0;
	char tlv_len_is_int_flag=0;
#endif
	unsigned short tlv_num;	
	unsigned char tlv_data_value[1000];	
	unsigned char *ptlv_data_value=tlv_data_value;		
	//unsigned int offset=0;		
	unsigned char mib_num[10];	
	unsigned char *pmib_num = mib_num;	
	memset(mib_num, 0x00, sizeof(mib_num));		

#if 0
int kk;
idx=pdata_array;	
fprintf(stderr,"\r\n data_size=%u \r\n",data_size);	
for(kk=0; kk<data_size; kk++)	
{		
fprintf(stderr,"0x%02x_", *(idx+kk));		
if((kk+1)%10==0) fprintf(stderr,"\r\n");
}	
fprintf(stderr,"\r\n");
#endif		

	i=0;	
	idx=pdata_array;	
	while(i<data_size)	
	{		
		memcpy(&tlv_tag, idx+i, sizeof(tlv_tag));			
		tlv_tag = WORD_SWAP(tlv_tag);
		i+=sizeof(tlv_tag);					
#ifdef TLV_LENGTH_INT

		if((tlv_tag&MIBTBL_WLAN_TLV_INT)==MIBTBL_WLAN_TLV_INT)
		{
			memcpy(&tlv_int_len, idx+i, sizeof(tlv_int_len));
			tlv_int_len=DWORD_SWAP(tlv_int_len);
			i+=sizeof(tlv_int_len);
			tlv_len_is_int_flag=1;
		}else
#endif
		{
			memcpy(&tlv_len, idx+i, sizeof(tlv_len));		
			tlv_len = WORD_SWAP(tlv_len);
			i+=sizeof(tlv_len);
#ifdef TLV_LENGTH_INT
			tlv_len_is_int_flag=0;
#endif  
		}

#ifdef VOIP_SUPPORT
#ifdef VOIP_SUPPORT_TLV_CFG
		if( tlv_tag == MIB_VOIP_CFG ) 
		{
			unsigned int mib_offset=0;
			const mib_table_entry_T *pmib_tbl;

			if( mib_search_by_id(mib_root_tbl, tlv_tag, pmib_num, &pmib_tbl, &mib_offset) != 1) {
				printf("\r\n%s(%d) tag:0x%04x, len:0x%04x\n",__FUNCTION__,__LINE__,tlv_tag,tlv_len);
				
                i+=tlv_len;
                continue;
			}

			voip_mib_cfg_init_from_tlv(	mibtbl_voip, pdata_array+i, 
										pfile, tlv_len, &mib_offset );
			i+=tlv_len;				

			/* voip mib has done, move forward */
			continue;
		}
#endif
#endif

		if((tlv_tag & MIB_TABLE_LIST) == 0) // NO member		
		{
			const mib_table_entry_T *mib_tbl;
			unsigned int mib_offset=0;

			if((ptlv_data_value=malloc(tlv_len)) == NULL)
			{
				COMP_TRACE(stderr,"\r\n ERR! malloc fail. tlv_tag=%p, tlv_len=%u __[%s-%u]",tlv_tag, tlv_len, __FILE__,__LINE__);
				return 0;
			}
			memcpy(ptlv_data_value, idx+i, tlv_len);
			i+=tlv_len;

#if 0
fprintf(stderr,"\r\n NO member >>");
fprintf(stderr,"\r\n tlv_tag=0x%x", tlv_tag);
fprintf(stderr,"\r\n tlv_len=0x%x", tlv_len);
fprintf(stderr,"\r\n tlv_value=");
for(j=0; j<tlv_len; j++)
	fprintf(stderr,"0x%x_", *(ptlv_data_value+j));
fprintf(stderr,"\r\n");
#endif
			if( mib_search_by_id(mib_root_tbl, tlv_tag, pmib_num, &mib_tbl, &mib_offset) != 1)			
			{
				//fprintf(stderr,"\r\n Can't find mib_id=%u",tlv_tag);
			}
			else
			{
//fprintf(stderr,"\r\n %u+%u",mib_offset,*pmib_root_offset );
				mib_offset += *pmib_root_offset;
				if(mib_tbl != NULL)
				{
				//fprintf(stderr,"\r\n");
#if 0 // TLV_DEBUG
int k;
unsigned char *pChar = ptlv_data_value;
fprintf(stderr,"\r\n >>mib_tbl->name=%s",mib_tbl->name);	
fprintf(stderr,"\r\n >>>tlv_tag=0x%x, tlv_len=0x%x, tlv_value = ",tlv_tag,tlv_len);
for(k=0; k< tlv_len; k++)
{	
fprintf(stderr,"0x%02x_", *(pChar+k));	
if( (k+1)%10 == 0) fprintf(stderr,"\r\n");
}
fprintf(stderr,"\r\n");
#endif
					if(mib_init_value(ptlv_data_value, tlv_len, mib_tbl, (void *)((int) pfile + mib_offset)) != 1)
					{
						fprintf(stderr,"\r\n Assign mib_name[%s] fail!", mib_tbl->name);
						fprintf(stderr,"\r\n mibtbl->id (%08x) unitsize (%d) totoal size (%d) mibtbl->nextbl %p",mib_tbl->id,mib_tbl->unit_size,mib_tbl->total_size,mib_tbl->next_mib_table);
					}
					else
					{
                        /* Done */
						if(ptlv_data_value != NULL) {
							free(ptlv_data_value);
                            ptlv_data_value = NULL;
                        } 
						//mib_display_value(mib_tbl, (void *)((int) pmib_data + mib_offset));
					}
				}
			}
		}
		else // have member		
		{			
			int j=0;
			const mib_table_entry_T *pmib_tbl;
			unsigned int mib_offset=0;

			if( mib_search_by_id(mib_root_tbl, tlv_tag, pmib_num, &pmib_tbl, &mib_offset) != 1)			
			{
				//fprintf(stderr,"\r\n Can't find mib_id=%u",tlv_tag);		
#ifdef TLV_LENGTH_INT
				if(tlv_len_is_int_flag)
					i+=tlv_int_len;
				else
#endif					
				i+=tlv_len;	
			}
			else
			{
#ifdef TLV_LENGTH_INT
				if(tlv_len_is_int_flag)
				{
					if((ptlv_data_value=malloc(tlv_int_len)) == NULL)
					{
						COMP_TRACE(stderr,"\r\n ERR! malloc fail. tlv_tag=%p, tlv_len=%u __[%s-%u]",tlv_tag, tlv_int_len, __FILE__,__LINE__);
						return 0;
					}
					
					memcpy(ptlv_data_value, idx+i, tlv_int_len);
					tlv_num = find_same_tag_times(ptlv_data_value, tlv_int_len);
					if(tlv_num != 0)
						tlv_int_len = tlv_int_len/tlv_num;
					//if(tlv_tag==MIB_WLAN_ROOT|| tlv_tag==(101|MIB_TABLE_LIST))
					//			fprintf(stderr,"\r\n tlv_num=%u tlv_len=%d__[%s-%u]", tlv_num, tlv_len, __FILE__,__LINE__);

					while(j<tlv_num)
					{
						memcpy(ptlv_data_value, idx+i+(tlv_int_len*j), tlv_int_len);
						if( j < (pmib_tbl->total_size / pmib_tbl->unit_size))
						{
							unsigned int mib_tlb_offset=0;
							mib_tlb_offset = mib_offset + *pmib_root_offset+j*(pmib_tbl->unit_size);
							mib_tlv_init_from(pmib_tbl->next_mib_table, ptlv_data_value, pfile, tlv_int_len, &mib_tlb_offset);
						}
						j++;
					}

	                if(ptlv_data_value != NULL) {
	                    free(ptlv_data_value);
	                    ptlv_data_value = NULL;
	                } 
					i+=tlv_int_len*tlv_num;		
				}else

#endif
				{
						if((ptlv_data_value=malloc(tlv_len)) == NULL)
						{
							COMP_TRACE(stderr,"\r\n ERR! malloc fail. tlv_tag=%p, tlv_len=%u __[%s-%u]",tlv_tag, tlv_len, __FILE__,__LINE__);
							return 0;
						}
						
						memcpy(ptlv_data_value, idx+i, tlv_len);
						tlv_num = find_same_tag_times(ptlv_data_value, tlv_len);
		//if(tlv_tag==MIB_WLAN_ROOT|| tlv_tag==(101|MIB_TABLE_LIST))
		//	fprintf(stderr,"\r\n tlv_num=%u tlv_len=%d__[%s-%u]", tlv_num, tlv_len, __FILE__,__LINE__);
						if(tlv_num != 0)
							tlv_len = tlv_len/tlv_num;

						while(j<tlv_num)
						{
		//fprintf(stderr,"\r\n TREE_NODE %s[%u] ENTRY",pmib_tbl->name,j);
							memcpy(ptlv_data_value, idx+i+(tlv_len*j), tlv_len);
							//printf("\r\n %u<%u/%u",j,pmib_tbl->total_size, pmib_tbl->unit_size );
							if( j < (pmib_tbl->total_size / pmib_tbl->unit_size))
							{
								unsigned int mib_tlb_offset=0;
		//fprintf(stderr,"\r\n  __[%s-%u]",  __FILE__,__LINE__);
								//printf("\r\n %u+%u+%u*%u",mib_offset,*pmib_root_offset, j,pmib_tbl->unit_size );
								mib_tlb_offset = mib_offset + *pmib_root_offset+j*(pmib_tbl->unit_size);
								//printf("\r\n TREE_NODE name =%s[%u] mib_tbl_offset is %u",pmib_tbl->mib_name,  j, mib_tlb_offset);
								//printf("\r\n tlv_len=%u __[%s-%u]", tlv_len,  __FILE__,__LINE__);
								mib_tlv_init_from(pmib_tbl->next_mib_table, ptlv_data_value, pfile, tlv_len, &mib_tlb_offset);
							}
		//fprintf(stderr,"\r\n TREE_NODE %s[%u] LEAVE",pmib_tbl->name,j);					
							j++;
						}

		                if(ptlv_data_value != NULL) {
		                    free(ptlv_data_value);
		                    ptlv_data_value = NULL;
		                } 
						i+=tlv_len*tlv_num;	
				}
			}
		}
	}

	if(ptlv_data_value != NULL) {
		free(ptlv_data_value);
		ptlv_data_value = NULL;
	} 

	return 1;
}

unsigned int mib_tlv_init(const mib_table_entry_T *mib_tbl, unsigned char *from_data, void *pfile, unsigned int tlv_content_len)
{
	unsigned int mib_offset = 0;

	if(mib_tbl == NULL || from_data == NULL || pfile == NULL || tlv_content_len == 0)
 		return 0;
		
	if(mib_tlv_init_from(mib_tbl, from_data, pfile, tlv_content_len, &mib_offset) == 1) {		
 		return 1;
	}	
	else {		
 		return 0;
	}	

}

unsigned int mib_tlv_save(CONFIG_DATA_T type, void *mib_data, unsigned char *mib_tlvfile, unsigned int *tlv_content_len)
{
	mib_table_entry_T *pmib_tl = NULL;

//fprintf(stderr,"\r\n tlv_content_len = %p, __[%s-%u]",tlv_content_len,__FILE__,__LINE__);				


	if(mib_tlvfile == NULL)
	{
		return 0;
	}
//fprintf(stderr,"\r\n mib_tlvfile = 0x%x, __[%s-%u]",mib_tlvfile,__FILE__,__LINE__);		
	
	pmib_tl = mib_get_table(type);

	if(pmib_tl==0)
	{
		return 0;
	}
//fprintf(stderr,"\r\n mib_data = %p, __[%s-%u]",mib_data,__FILE__,__LINE__);					
//fprintf(stderr,"\r\n mib_tlvfile = %p, __[%s-%u]",mib_tlvfile,__FILE__,__LINE__);	
//fprintf(stderr,"\r\n pmib_tl->name=%s, __[%s-%u]",pmib_tl->name,__FILE__,__LINE__);	
//fprintf(stderr,"\r\n tlv_content_len = %p, __[%s-%u]",tlv_content_len,__FILE__,__LINE__);		
	mib_write_to_raw(pmib_tl, (void *)((int) mib_data), mib_tlvfile, tlv_content_len);

	return 1;

}

void mib_display_data_content(CONFIG_DATA_T type, unsigned char * pdata, unsigned int mib_data_len)
{
	int kk;
	fprintf(stderr,"\r\n type=%u, mibdata_len = %u",type, mib_data_len);
	fprintf(stderr,"\r\n pdata=");
	for(kk=0; kk< mib_data_len; kk++)
	{
		fprintf(stderr,"0x%02x_", *(pdata+kk));
		if( (kk+1)%10 == 0) fprintf(stderr,"\r\n");
	}
	fprintf(stderr,"0x%02x_",CHECKSUM(pdata, mib_data_len));
}

void mib_display_tlv_content(CONFIG_DATA_T type, unsigned char * ptlv, unsigned int mib_tlv_len)
{
	int kk;
	fprintf(stderr,"\r\n type=%u, tlv_content_len = %u",type, mib_tlv_len);			
	fprintf(stderr,"\r\n tlv_content=");
	for(kk=0; kk< mib_tlv_len; kk++)
	{
		fprintf(stderr,"0x%02x_", *(ptlv+kk));
		if( (kk+1)%10 == 0) fprintf(stderr,"\r\n");
	}
}

int mib_write_to_raw(const mib_table_entry_T *mib_tbl, void *data, unsigned char *pfile, unsigned int *idx)
{	
	unsigned short tlv_tag=0;	
#ifdef TLV_LENGTH_INT
	unsigned int tlv_int_len=0;
#endif
	unsigned short tlv_len=0;	
	unsigned short tlv_num=0;		
	int i, j;
	//int k;

#if 0
fprintf(stderr,"\r\n > mib_tbl->name=%s, __[%s-%u]",mib_tbl->name,__FILE__,__LINE__);	
#endif

//fprintf(stderr,"\r\n idx = %p, __[%s-%u]",idx,__FILE__,__LINE__);		
//fprintf(stderr,"\r\n data = %p, __[%s-%u]",data,__FILE__,__LINE__);					
//fprintf(stderr,"\r\n pfile = %p, __[%s-%u]",pfile,__FILE__,__LINE__);	

	/* If pointing to other table */
	if(mib_tbl->type >= TABLE_LIST_T)	
	{

		const mib_table_entry_T *mib = mib_tbl->next_mib_table;		
		unsigned int offset=0;				

		/* traverse whole table */
		for(i=0 ; mib[i].id ; i++)		
		{			
			const mib_table_entry_T *pmib = &mib[i];			
#if 0
fprintf(stderr,"\r\n Turn mib[i].mib_name=%s __[%s-%u]",mib[i].name,__FILE__,__LINE__);
#endif
			if(mib[i].type < TABLE_LIST_T)			
			{				
				mib_write_to_raw(pmib, (void *)((int) data + offset), pfile, idx);				
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);				
//printf("\r\n offset + %u __[%s-%u]",pmib->unit_size,__FILE__,__LINE__);				
				//offset += pmib->size;
				offset += pmib->total_size;
			}
			else
			{				
				unsigned int ori_idx = 0;				
				unsigned short *ptlv_len = NULL;								
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);				
				if((pmib->total_size%pmib->unit_size) == 0)					
					tlv_num = pmib->total_size/pmib->unit_size;
				
				tlv_tag = (pmib->id);
				tlv_tag = WORD_SWAP(tlv_tag); // for endian handle
				/* tag */
				memcpy(pfile+*idx, &tlv_tag, 2);
				*idx+=2;	
				
				tlv_len = WORD_SWAP(tlv_len);				
//fprintf(stderr,"\r\n __[%s-%u]",__FILE__,__LINE__);				
#ifdef TLV_LENGTH_INT
                                tlv_tag = (pmib->id);
				if((tlv_tag&MIBTBL_WLAN_TLV_INT)==MIBTBL_WLAN_TLV_INT)
				{
					unsigned int *ptlv_int_len = NULL;
					memcpy(pfile+*idx, &tlv_int_len, 4);
					ptlv_int_len=(unsigned int *)(pfile+*idx);
					*idx+=4;
					tlv_num = pmib->total_size/pmib->unit_size;				
					ori_idx = *idx;
					for(j=0 ; j<tlv_num ; j++)				
					{
					
						mib_write_to_raw(pmib, (void *)((int) data + offset), pfile, idx);
						offset += pmib->unit_size;					
					}											
					/* Now, we know the Table length, fill it! */
					tlv_int_len = (*idx-ori_idx);	
					tlv_int_len = DWORD_SWAP(tlv_int_len); // for endian			
					memcpy(ptlv_int_len, &tlv_int_len, 4);
				}else
					
#endif
				{
					/* Set length to zero. It will be updated later */
					memcpy(pfile+*idx, &tlv_len, 2);				

					/* Remember pointer for saving Table length */
					/* We don't know exact length now. Length will be updated later. */
					ptlv_len = (unsigned short *)(pfile+*idx);				
					*idx+=2;								
					tlv_num = pmib->total_size/pmib->unit_size;				
					ori_idx = *idx;
	//printf("\r\n -- ptlv_len[0x%x] *idx[%u] ori_idx[%u] tlv_num[%u]<< ",ptlv_len, *idx,ori_idx,tlv_num);
					//if(tlv_tag==MIB_WLAN_ROOT || tlv_tag==MIB_MACFILTER_TBL)
					//	fprintf(stderr,"%s:%d ori_idx=%d tlv_num=%d\n",__FUNCTION__,__LINE__,ori_idx,tlv_num);
						

					for(j=0 ; j<tlv_num ; j++)				
					{
					
						mib_write_to_raw(pmib, (void *)((int) data + offset), pfile, idx);
						offset += pmib->unit_size;					
					}		
					/* Now, we know the Table length, fill it! */
					tlv_len = (*idx-ori_idx);	
					tlv_len = WORD_SWAP(tlv_len); // for endian
					memcpy(ptlv_len, &tlv_len, 2);
				}
			}								
		} /* for(i) */		
	}
	else
	{	
		/* Most item run into here, including VoIP configuration, to save TLV */

		unsigned char *pChar = (unsigned char *) data;		
		//unsigned short mib_value;

#ifdef VOIP_SUPPORT_TLV_CFG
		if(mib_tbl->type == VOIP_T) 
		{			
			unsigned short *ptlv_len;
			unsigned short tlv_tag;
			unsigned short tlv_len;
			unsigned int tlv_offset_org;

			//printf("%s(%d)VoIP save TLV\n",__FUNCTION__,__LINE__);
			//printf("%s(%d)id=0x%04x, name=%s, u_sz=%d, t_sz=%d\n",__FUNCTION__,__LINE__,
			//			mib_tbl->id,mib_tbl->name,mib_tbl->unit_size,mib_tbl->total_size);

			/* Tag */
			tlv_tag = mib_tbl->id;
			tlv_tag = WORD_SWAP(tlv_tag);
			memcpy((unsigned char*)((int)pfile + *idx),&tlv_tag,sizeof(tlv_tag));
			*idx+=2;

			/* addr of tlv_len */
			ptlv_len = (unsigned short *)(pfile+*idx);				

			/* we don't know exact len now */
			tlv_len = 0;
			tlv_len = WORD_SWAP(tlv_len);
			memcpy(ptlv_len,&tlv_len,sizeof(tlv_len));
			*idx+=2;
			tlv_offset_org = *idx;

			/* call voip TLV process */
			voip_mib_cfg_write_to_tlv(&mibtbl_voip_root, (void *)data, pfile, idx);				

			/* fill correct tlv_len */
			tlv_len = *idx - tlv_offset_org;
			tlv_len = WORD_SWAP(tlv_len);
			memcpy(ptlv_len,&tlv_len,sizeof(tlv_len));

			return 1;
		}
#endif
		/* Saving Tag */
		tlv_tag = (mib_tbl->id);
		tlv_tag = WORD_SWAP(tlv_tag); // for endian
		memcpy(pfile+*idx, &tlv_tag, 2);
		*idx+=2;				

		/* Saving Length */
        tlv_len = (mib_tbl->total_size);		
		tlv_len = WORD_SWAP(tlv_len); // for endian
		memcpy(pfile+*idx, &tlv_len, 2);

		*idx+=2;

		/* Saving Value */			
		memcpy(pfile+*idx, pChar, WORD_SWAP(tlv_len)); // for endian

		*idx+=WORD_SWAP(tlv_len);
		
#if 0 // TLV_DEBUG
fprintf(stderr,"\r\n >>mib_tbl->name=%s",mib_tbl->name);	
fprintf(stderr,"\r\n >>>tlv_tag=0x%x, tlv_len=0x%x, tlv_value = ",tlv_tag,tlv_len);
for(k=0; k< tlv_len; k++)
{	
fprintf(stderr,"0x%02x_", *(pChar+k));	
if( (k+1)%10 == 0) fprintf(stderr,"\r\n");
}
fprintf(stderr,"\r\n");
#endif

#if 0
fprintf(stderr,"\r\n pFile=");
for(k=0; k< *idx; k++)
{	
fprintf(stderr,"0x%02x_", *(pfile+k));	
if( (k+1)%10 == 0) fprintf(stderr,"\r\n");
}
#endif			
	}	
	return 1;
}

#if 0 

// allow the same id in different mib table
static int _mibtbl_check(const mib_table_entry_T *mib_tbl)
{
	int i, j;

	for (i=0; mib_tbl[i].id; i++)
	{
		if (mib_tbl[i].type >= TABLE_LIST_T)
		{
			if (_mibtbl_check(mib_tbl[i].next_mib_table) != 0)
			{
				fprintf(stderr, "MIB (%s, %d, %d) Error: check mib_tbl failed\n",
					mib_tbl[i].name, mib_tbl[i].total_size, mib_tbl[i].unit_size);
				return -1;
			}
		}

		// check duplicate mib id
		for (j=i+1; mib_tbl[j].id; j++)
		{
			if (mib_tbl[i].id == mib_tbl[j].id)
			{
				fprintf(stderr, "MIB Error: %s detect duplicate id in %s\n",
					mib_tbl[i].name, mib_tbl[j].name);
				return -1;
			}
		}
	}

	return 0;
}

int mibtbl_check(void)
{
	mib_table_entry_T *pmib_tl_hw;
	mib_table_entry_T *pmib_tl;

	pmib_tl_hw = mib_get_table(HW_SETTING);
	pmib_tl = mib_get_table(CURRENT_SETTING);

	if ((_mibtbl_check(pmib_tl_hw) != 0) ||
		(_mibtbl_check(pmib_tl) != 0))
		return -1;

	return 0;
}

#else

// NOT allow the same id in different mib table
int mibtbl_check_add(mib_table_entry_T *root, mib_table_entry_T *mib_tbls[])
{
	int i, cnt;

	cnt = 0;
	for (i=0; root[i].id; i++)
	{
		if (root[i].type >= TABLE_LIST_T)
			cnt += mibtbl_check_add(root[i].next_mib_table, &mib_tbls[cnt]);
		else
			mib_tbls[cnt++] = &root[i];
	}

	return cnt;
}

static int
mibtbl_check_id(const void *p1, const void *p2)
{
	int res;
	mib_table_entry_T *mib1 = * (mib_table_entry_T * const *) p1;
	mib_table_entry_T *mib2 = * (mib_table_entry_T * const *) p2;

//	if (mib1->id == mib2->id)
	if (mib1->id == mib2->id && mib1->name != mib2->name)
		fprintf(stderr, "MIB Error: %s detect duplicate id in %s\n",
			mib1->name, mib2->name);

	return mib1->id - mib2->id;
}

#define MAX_MIBTBL_CHECK 1024
int mibtbl_check(void)
{
	int i;
	int cnt;
	static mib_table_entry_T *mib_tbls[MAX_MIBTBL_CHECK];
	mib_table_entry_T *pmib_tl_hw;
	mib_table_entry_T *pmib_tl;

	pmib_tl_hw = mib_get_table(HW_SETTING);
	pmib_tl = mib_get_table(CURRENT_SETTING);

	cnt = mibtbl_check_add(pmib_tl_hw, mib_tbls);
	cnt += mibtbl_check_add(pmib_tl, &mib_tbls[cnt]);

	if (cnt >= MAX_MIBTBL_CHECK)
	{
		fprintf(stderr, "MAX_MIBTBL_CHECK is smaller than MIB TBL\n");
		return -1;
	}

	qsort(mib_tbls, cnt, sizeof(mib_table_entry_T *), mibtbl_check_id);
	return 0;	
}

#endif

#define TZ_FILE "/var/TZ"
unsigned char *gettoken(const unsigned char *str,unsigned int index,unsigned char symbol)
{
	static char tmp[50];
	unsigned char tk[50]; //save symbol index
	char *ptmp;
	int i,j,cnt=1,start,end;
	//scan symbol and save index
	
	memset(tmp, 0x00, sizeof(tmp));
	
	for (i=0;i<strlen((char *)str);i++)
	{          
		if (str[i]==symbol)
		{
			tk[cnt]=i;
			cnt++;
		}
	}
	
	if (index>cnt-1)
	{
		return NULL;
	}
			
	tk[0]=0;
	tk[cnt]=strlen((char *)str);
	
	if (index==0)
		start=0;
	else
		start=tk[index]+1;

	end=tk[index+1];
	
	j=0;
	for(i=start;i<end;i++)
	{
		tmp[j]=str[i];
		j++;
	}
		
	return (unsigned char *)tmp;
}

void set_timeZone(void)
{
	unsigned int daylight_save = 1;
	char daylight_save_str[5];
	char time_zone[8];
	char command[100], str_datnight[100],str_cmd[128]={0};
	unsigned char *str_tz1;

	struct timezone tz;
	memset(&tz, 0, sizeof(tz));
	
	apmib_get( MIB_DAYLIGHT_SAVE,  (void *)&daylight_save);
	memset(daylight_save_str, 0x00, sizeof(daylight_save_str));
	sprintf(daylight_save_str,"%u",daylight_save);
	
	apmib_get( MIB_NTP_TIMEZONE,  (void *)&time_zone);

	if(daylight_save == 0)
		sprintf( str_datnight, "%s", "");
	else if(strcmp(time_zone,"9 1") == 0)
		sprintf( str_datnight, "%s", "PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"8 1") == 0)
		sprintf( str_datnight, "%s", "PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"7 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"6 1") == 0)
	        sprintf( str_datnight, "%s", "PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"6 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"5 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"5 3") == 0)
	        sprintf( str_datnight, "%s", "PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"4 3") == 0)
	        sprintf( str_datnight, "%s", "PDT,M10.2.0/00:00:00,M3.2.0/00:00:00");
	else if(strcmp(time_zone,"3 1") == 0)
	        sprintf( str_datnight, "%s", "PDT,M4.1.0/00:00:00,M10.5.0/00:00:00");
	else if(strcmp(time_zone,"3 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M2.2.0/00:00:00,M10.2.0/00:00:00");
	else if(strcmp(time_zone,"1 1") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/00:00:00,M10.5.0/01:00:00");
	else if(strcmp(time_zone,"0 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/01:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"-1") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/02:00:00,M10.5.0/03:00:00");
	else if(strcmp(time_zone,"-2 1") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/02:00:00,M10.5.0/03:00:00");
	else if(strcmp(time_zone,"-2 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/03:00:00,M10.5.0/04:00:00");
	else if(strcmp(time_zone,"-2 3") == 0)
	        sprintf( str_datnight, "%s", "PDT,M4.5.5/00:00:00,M9.5.5/00:00:00");
	else if(strcmp(time_zone,"-2 5") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/03:00:00,M10.5.5/04:00:00");
	else if(strcmp(time_zone,"-2 6") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.5/02:00:00,M10.1.0/02:00:00");
	else if(strcmp(time_zone,"-3 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/02:00:00,M10.5.0/03:00:00");
	else if(strcmp(time_zone,"-4 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/04:00:00,M10.5.0/05:00:00");
	else if(strcmp(time_zone,"-9 4") == 0)
	        sprintf( str_datnight, "%s", "PDT,M10.5.0/02:00:00,M4.1.0/03:00:00");
	else if(strcmp(time_zone,"-10 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M10.5.0/02:00:00,M4.1.0/03:00:00");
	else if(strcmp(time_zone,"-10 4") == 0)
	        sprintf( str_datnight, "%s", "PDT,M10.1.0/02:00:00,M4.1.0/03:00:00");
	else if(strcmp(time_zone,"-10 5") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/02:00:00,M10.5.0/03:00:00");
	else if(strcmp(time_zone,"-12 1") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.2.0/03:00:00,M10.1.0/02:00:00");
	else
	        sprintf( str_datnight, "%s", "");

	str_tz1 = gettoken((unsigned char*)time_zone, 0, ' ');
	
	if(strcmp(time_zone,"3 1") == 0 ||
		strcmp(time_zone,"-3 4") == 0 ||
	 	strcmp(time_zone,"-4 3") == 0 ||
	 	strcmp(time_zone,"-5 3") == 0 ||
	 	strcmp(time_zone,"-9 4") == 0 ||
	 	strcmp(time_zone,"-9 5") == 0
		)
	{
         sprintf( command, "GMT%s:30%s", str_tz1, str_datnight);
	}
	else
		sprintf( command, "GMT%s%s", str_tz1, str_datnight); 
	unlink(TZ_FILE);
	sprintf(str_cmd,"echo %s >%s",command,TZ_FILE);
	system(str_cmd);

	//set time zone to kernel (global variable sys_tz in linux-3.10/kernel/time.c)
	int tmp_minutes=atoi(str_tz1)*60;
	//printf("\n%s:%d tmp_minutes=%d\n",__FUNCTION__,__LINE__,tmp_minutes);
	
	tz.tz_minuteswest=tmp_minutes;
	
	settimeofday(NULL, &tz);
}

#endif
#ifdef CONFIG_IPV6
/*
*	input:name(interface name) addrIndex(count index)
*	output:v6 address,interface index,prefix len,
*			scope(IFA_LINK|IFA_HOST|IFA_SITE),
dad_status(IFA_F_NODAD|IFA_F_OPTIMISTIC|IFA_F_HOMEADDRESS|IFA_F_DEPRECATED|IFA_F_TENTATIVE|IFA_F_PERMANENT)
*	output can be NULL
*	return:
	-1:fail
	0:not find
	1:find
*/
int get_inet6_stats(char * name,int addrIndex,char* address,int *pindex,int *pplen,int *pscope,int *pdad_status)
{	
	FILE * fh=NULL;
	int plen, scope, dad_status, if_idx,addrIndex_count=0;
	char addr6p[8][5];
	char addressBuf[40]={0},nameBuf[16]={0};
	fh = fopen(_PATH_PROCNET_IFINET6, "r");
	if (!fh) {
		printf("can't open %s\n",_PATH_PROCNET_IFINET6);
		return -1;
	}
	
	while(fscanf
			   (fh, "%4s%4s%4s%4s%4s%4s%4s%4s %08x %02x %02x %02x %20s\n",
				addr6p[0], addr6p[1], addr6p[2], addr6p[3], addr6p[4],
				addr6p[5], addr6p[6], addr6p[7], &if_idx, &plen, &scope,
				&dad_status, nameBuf) != EOF
		)
		{
			//printf("nameBuf=%s\n",nameBuf);
			if (strcmp(nameBuf, name)==0) 
			{
				if(addrIndex_count==addrIndex)
				{
					sprintf(addressBuf, "%s:%s:%s:%s:%s:%s:%s:%s",
						addr6p[0], addr6p[1], addr6p[2], addr6p[3],
						addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
					//printf("%s %d %xd %d %d\n",addressBuf,if_idx,plen,scope,dad_status);
					if(address)
						strcpy(address,addressBuf);
					if(pindex)
						*pindex=if_idx;
					if(pplen)
						*pplen=plen;
					if(pscope)
						*pscope=scope;
					if(pdad_status)
						*pdad_status=dad_status;
					fclose(fh);
					return 1;
				}				
				addrIndex_count++;
			}			
		}	
	fclose(fh);
	return 0;
}
#endif

#ifdef CONFIG_IPV6

#define NS_INT16SZ   2
#define NS_INADDRSZ  4
#define NS_IN6ADDRSZ    16

//add string to IPv4 address exchange
int
inet_pton4(src, dst)
	const char *src;
	unsigned char *dst;
{
	static const char digits[] = "0123456789";
	int saw_digit, octets, ch;
	unsigned char tmp[NS_INADDRSZ], *tp;
	 
	saw_digit = 0;
	octets = 0;
	*(tp = tmp) = 0;
	while ((ch = *src++) != '\0') {
		const char *pch;

		if ((pch = strchr(digits, ch)) != NULL) {
		unsigned int new = *tp * 10 + (pch - digits);
 
		if (new > 255)
			return (0);
		*tp = new;
		if (! saw_digit) {
			if (++octets > 4)
				return (0);
			saw_digit = 1;
			}
		} else if (ch == '.' && saw_digit) {
			if (octets == 4)
				return (0);
			*++tp = 0;
			saw_digit = 0;
		} else
			return (0);
	}
	if (octets < 4)
		return (0);
	memcpy(dst, tmp, NS_INADDRSZ);
	return (1);
}

//add string to IPv6 address exchange
int
inet_pton6(src, dst)
	const char *src;
	unsigned char *dst;
{
	static const char xdigits_l[] = "0123456789abcdef",
		xdigits_u[] = "0123456789ABCDEF";
	unsigned char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
	const char *xdigits, *curtok;
	int ch, saw_xdigit;
	unsigned int val;
 
	memset((tp = tmp), '\0', NS_IN6ADDRSZ);
	endp = tp + NS_IN6ADDRSZ;
	colonp = NULL;
	/** Leading :: requires some special handling. */
	if (*src == ':')
		if (*++src != ':')
			return (0);
	curtok = src;
	saw_xdigit = 0;
	val = 0;
	while ((ch = *src++) != '\0') {
		const char *pch;
	 
		if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
			pch = strchr((xdigits = xdigits_u), ch);
		if (pch != NULL) {
			val <<= 4;
			val |= (pch - xdigits);
			if (val > 0xffff)
				return (0);
			saw_xdigit = 1;
			continue;
		}
		if (ch == ':') {
			curtok = src;
		if (!saw_xdigit) {
			if (colonp)
				return (0);
			colonp = tp;
				continue;
		}
		if (tp + NS_INT16SZ > endp)
			return (0);
		*tp++ = (unsigned char) (val >> 8) & 0xff;
		*tp++ = (unsigned char) val & 0xff;
		saw_xdigit = 0;
		val = 0;
		continue;
		}
		if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
			inet_pton4(curtok, tp) > 0) {
			tp += NS_INADDRSZ;
			saw_xdigit = 0;
			break;  /** '\0' was seen by inet_pton4(). */
		}
		return (0);
	}
	if (saw_xdigit) {
		if (tp + NS_INT16SZ > endp)
			return (0);
		*tp++ = (unsigned char) (val >> 8) & 0xff;
		*tp++ = (unsigned char) val & 0xff;
	}
	if (colonp != NULL) {
	/**
	  * Since some memmove()'s erroneously fail to handle
	  * overlapping regions, we'll do the shift by hand.
	  */
		const int n = tp - colonp;
		int i;
	 
		for (i = 1; i <= n; i++) {
			endp[- i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}
	if (tp != endp)
		return (0);
	memcpy(dst, tmp, NS_IN6ADDRSZ);
	return (1);
}

int get_wanif(char*wan_interface)
{
	int val=0;
	if(!wan_interface) return -1;
	if(apmib_get(MIB_IPV6_DHCPC_IFACE,(void*)wan_interface)==0)
	{
		printf("get MIB_IPV6_DHCPC_IFACE fail!\n");
		return -1;
	}
	if(wan_interface[0])
		return 0;
	if(!apmib_get(MIB_IPV6_LINK_TYPE,&val)){	
			fprintf(stderr, "get mib %d error!\n", MIB_IPV6_LINK_TYPE);
			return -1;			
	}
			
	if(val == IPV6_LINKTYPE_PPP)
		sprintf(wan_interface,"ppp0");
	else
		sprintf(wan_interface,"eth1");
	return 0;
}


#ifdef TR181_V6_SUPPORT

/***********************
return value:
-1:fail
1:int
2:string
3:duid_t
...
***********************/
int tr181_ipv6_get(char * name,void * value)
{
	int val=0;
	if(!name||!value)
	{
		printf("tr181_get invalid input!\n");
		return -1;
	}
#ifdef CONFIG_IPV6
	if(strcmp(name,"Device.DHCPv6.Client.Enable")==0)
	{
		if(apmib_get(MIB_IPV6_WAN_ENABLE,(void*)&val)==0)
		{
			printf("get MIB_IPV6_WAN_ENABLE fail!\n");
			return -1;
		}
		*((int*)value)=val;
		return 1;
	}
	if(strcmp(name,"Device.DHCPv6.Client.Interface")==0)
	{
		
		if(apmib_get(MIB_IPV6_DHCPC_IFACE,(void*)value)==0)
		{
			printf("get MIB_IPV6_DHCPC_IFACE fail!\n");
			return -1;
		}
		return 2;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.DUID")==0)
	{
		char wanIf[16]={0};
		struct sockaddr hwaddr={0};
		struct duid_t dhcp6c_duid={0};
		struct ifreq ifr={0};
    	int skfd=0;
		if(get_wanif(wanIf)<0)
			return -1;
		
    	skfd = socket(AF_INET, SOCK_DGRAM, 0);
   		strcpy(ifr.ifr_name, wanIf);
		if (ioctl(skfd, SIOCGIFHWADDR, &ifr) >= 0) {
			memcpy(&hwaddr, &ifr.ifr_hwaddr, sizeof(struct sockaddr));
		}else		
		{
				fprintf(stderr, "Read hwaddr Error\n");
				return -1;	
		}
		dhcp6c_duid.duid_type=3;
		dhcp6c_duid.hw_type=1;
		memcpy(dhcp6c_duid.mac,hwaddr.sa_data,6);
		*((struct duid_t*)value)=dhcp6c_duid;
	//	sprintf(DUID_buff,"%04x%04x%02x%02x%02x%02x%02x%02x",3,1,hwaddr.sa_data[0],hwaddr.sa_data.mac[1],hwaddr.sa_data.mac[2],hwaddr.sa_data.mac[3],hwaddr.sa_data.mac[4],hwaddr.sa_data.mac[5]);
		return 3;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.Status")==0)
	{
		if(apmib_get(MIB_IPV6_DHCP_MODE,(void*)&val)==0)
		{
			printf("get MIB_IPV6_DHCP_MODE fail!\n");
			return -1;
		}
		if(val==IPV6_DHCP_STATELESS)
		{//not enable
			strcpy((char*)value,"Disabled");			
			return 2;
		}
		if(val==IPV6_DHCP_STATEFUL)
		{//enable
			if(access("/var/dhcp6c_parse_fail",0)<0)
				strcpy((char*)value,"Enabled");
			else
				strcpy((char*)value,"Error_Misconfigured");
		}
		return 2;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.RequestAddresses")==0)
	{
		if(apmib_get(MIB_IPV6_DHCPC_REQUEST_ADDR,(void*)&val)==0)
		{
			printf("get MIB_IPV6_DHCPC_REQUEST_ADDR fail!\n");
			return -1;
		}
		*((int*)value)=val;
		return 1;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.RequestPrefixes")==0)
	{
		if(apmib_get(MIB_IPV6_DHCP_PD_ENABLE,(void*)&val)==0)
		{
			printf("get MIB_IPV6_DHCP_PD_ENABLE fail!\n");
			return -1;
		}
		*((int*)value)=val;
		return 1;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.RapidCommit")==0)
	{
		if(apmib_get(MIB_IPV6_DHCP_RAPID_COMMIT_ENABLE,(void*)&val)==0)
		{
			printf("get MIB_IPV6_DHCP_RAPID_COMMIT_ENABLE fail!\n");
			return -1;
		}
		*((int*)value)=val;
		return 1;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.Renew")==0)
	{
		*((int*)value)=0;
		return 1;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.SuggestedT1")==0)
	{
		if(apmib_get(MIB_IPV6_DHCPC_SUGGESTEDT1,(void*)&val)==0)
		{
			printf("get MIB_IPV6_DHCPC_SuggestedT1 fail!\n");
			return -1;
		}
		*((int*)value)=val;
		return 1;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.SuggestedT2")==0)
	{
		if(apmib_get(MIB_IPV6_DHCPC_SUGGESTEDT2,(void*)&val)==0)
		{
			printf("get MIB_IPV6_DHCPC_SuggestedT2 fail!\n");
			return -1;
		}
		*((int*)value)=val;
		return 1;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.SupportedOptions")==0)
	{
		char *buff=(char*)value;
		strcpy(buff,"1,2,3,5,6,8,14,21,22,23,27,28,29,30,31,33,34");
		//OPTION_CLIENTID (1)	 OPTION_SERVERID (2) 	OPTION_IA_NA (3)	 OPTION_IAADDR (5)
		// OPTION_ORO (6)	OPTION_ELAPSED_TIME (8)	OPTION_RAPID_COMMIT (14)	DH6OPT_SIP_SERVER_D 21
		//DH6OPT_SIP_SERVER_A 22 DH6OPT_DNS 23 DH6OPT_NIS_SERVERS 27 DH6OPT_NISP_SERVERS 28 
		//DH6OPT_NIS_DOMAIN_NAME 29 DH6OPT_NISP_DOMAIN_NAME 30 DH6OPT_NTP 31 DH6OPT_BCMCS_SERVER_D 33
		//DH6OPT_BCMCS_SERVER_A 34
		return 2;
	}
	
	else if(strcmp(name,"Device.DHCPv6.Client.SentOptionNumberOfEntries")==0)
	{
		*((int*)value)=IPV6_DHCPC_SENDOPT_NUM;
		return 1;
	}
	else if(strncmp(name,"Device.DHCPv6.Client.SentOption.{",strlen("Device.DHCPv6.Client.SentOption.{"))==0)
	{
		char index_buf[8]={0};
		int idx=0,count=0,maxNum=IPV6_DHCPC_SENDOPT_NUM;
		DHCPV6C_SENDOPT_T entry={0};
		char*p=name+strlen("Device.DHCPv6.Client.SentOption.{");
		
		//get index
		while((*p)>='0'&&(*p)<='9')
		{
			index_buf[count++]=*p;
			p++;
		}
		idx=atoi(index_buf)+1;
		if(idx>maxNum)
		{
			printf("invalid input idx %d must less than max:%d!\n",idx,IPV6_DHCPC_SENDOPT_NUM);
			return -1;
		}
		apmib_set(MIB_IPV6_DHCPC_SENDOPT_TBL_NUM,(void*)&maxNum);
		//get entry
		*((char*)&entry)=(char)idx;
		if(apmib_get(MIB_IPV6_DHCPC_SENDOPT_TBL,(void*)&entry)==0)
		{
			printf("get MIB_IPV6_DHCPC_SENDOPT_TBL fail!\n");
			return -1;
		}

		//get name		
		if((*p)!='}'&&(*(p+1)!='.'))
		{
			printf("invalid input %s!\n",name);
			return -1;
		}
		name=p+2;
		
		if(strcmp(name,"Enable")==0)
		{
			*((int*)value)=entry.enable;
			return 1;
		}
		else if(strcmp(name,"Tag")==0)
		{
			*((int*)value)=entry.tag;
			return 1;
		}
		else if(strcmp(name,"Value")==0)
		{
			strcpy((char*)value,entry.value);
			return 2;
		}else
		{
			printf("can't find handle of Device.DHCPv6.Client.SentOption.{%d}.%s\n",idx,name);
			return -1;
		}
	}
	else if(strcmp(name,"Device.DHCPv6.Client.ReceivedOptionNumberOfEntries")==0)
	{
		FILE*fp=fopen("/var/dhcp6c_rcvOpt","r");
		int count=0;
		char valueBuf[128]={0};
		if(fp==NULL)
		{
			*((int*)value)=0;
			return 1;
		}
		for(count=0;count<IPV6_DHCPC_RCVOPT_NUM+1;count++)
		{
			if(fgets(valueBuf,sizeof(valueBuf),fp)==NULL)
			{
				break;
			}
		}
		fclose(fp);
		if(count==IPV6_DHCPC_RCVOPT_NUM+1)
		{
			printf("over max!check /var/dhcp6c_rcvOpt\n");
			return -1;
		}
		*((int*)value)=count;
		return 1;
	}
	else if(strncmp(name,"Device.DHCPv6.Client.ReceivedOption.{",strlen("Device.DHCPv6.Client.ReceivedOption.{"))==0)
	{
		char index_buf[8]={0};
		int idx=0,count=0;
		int tag=0;
		char valueBuf[128]={0};
		char valueNameBuf[128]={0};
		char valueValBuf[128]={0};
		FILE*fp=NULL;
		char*p=name+strlen("Device.DHCPv6.Client.ReceivedOption.{");
		
		//get index
		while((*p)>='0'&&(*p)<='9')
		{
			index_buf[count++]=*p;
			p++;
		}
		idx=atoi(index_buf);
		if(idx>=IPV6_DHCPC_RCVOPT_NUM)
		{
			printf("invalid input idx %d must less than max:%d!\n",idx,IPV6_DHCPC_RCVOPT_NUM);
			return -1;
		}
		fp=fopen("/var/dhcp6c_rcvOpt","r");
		if(fp==NULL)
		{
			printf("can't get receive option!\n");
			return -1;
		}
		for(count=0;count<=idx;count++)
		{
			bzero(valueBuf,sizeof(valueBuf));
			if(fgets(valueBuf,sizeof(valueBuf),fp)==NULL)
			{
				printf("index max %d\n",count-1);
				fclose(fp);
				return -1;
			}
		}

		fclose(fp);
		//printf("%s\n",valueBuf);
		sscanf(valueBuf,"%s	%d	%s\n",valueNameBuf,&tag,valueValBuf);
		//printf("valueValBuf=%s\n",valueValBuf);

		//get name		
		if((*p)!='}'&&(*(p+1)!='.'))
		{
			printf("invalid input %s!\n",name);
			return -1;
		}
		name=p+2;
		
		if(strcmp(name,"Tag")==0)
		{
			*((int*)value)=tag;
			return 1;
		}
		else if(strcmp(name,"Value")==0)
		{
			strcpy((char*)value,valueValBuf);
			return 2;
		}else
		{
			printf("can't find handle of Device.DHCPv6.Client.ReceivedOption.{%d}.%s\n",idx,name);
			return -1;
		}		
	}
#endif
	if(strncmp(name,"Device.DNS.SupportedRecordTypes",strlen("Device.DNS.SupportedRecordTypes"))==0)
	{
		char *buff=(char*)value;
#ifdef CONFIG_IPV6
		strcpy(buff,"A,AAAA");
#else
		strcpy(buff,"A");
#endif
		/* 
		A ([RFC1035]) 
		AAAA ([RFC3596]) 
		SRV ([RFC2782]) 
		PTR ([RFC1035]) 
		*/
		return 2;
	}
	else if(strncmp(name,"Device.DNS.Client.Enable",strlen("Device.DNS.Client.Enable"))==0)
	{
		if(apmib_get(MIB_DNS_CLIENT_ENABLE,(void*)&val)==0)
		{
			printf("get MIB_DNS_CLIENT_ENABLE fail!\n");
			return -1;
		}
		*((int *)value) = val;
		return 1;
	}
	else if(strncmp(name,"Device.DNS.Client.Status",strlen("Device.DNS.Client.Status"))==0)
	{
		if(apmib_get(MIB_DNS_CLIENT_ENABLE,(void*)&val)==0)
		{
			printf("get MIB_DNS_CLIENT_ENABLE fail!\n");
			return -1;
		}
		if(val == 1)
			strcpy((char*)value,"Enabled");
		else
			strcpy((char*)value,"Disabled");
		return 2;
	}
	else if(strncmp(name,"Device.DNS.Client.ServerNumberOfEntries",strlen("Device.DNS.Client.ServerNumberOfEntries"))==0)
	{
		*((int *)value) = DNS_CLIENT_SERVER_NUM;
		return 1;
	}
	else if(strncmp(name,"Device.DNS.Client.Server.{",strlen("Device.DNS.Client.Server.{"))==0)
	{
		char index_buf[8]={0};
		int idx=0,count=0,maxNum=DNS_CLIENT_SERVER_NUM;
		DNS_CLIENT_SERVER_T entry={0};
		char*p=name+strlen("Device.DNS.Client.Server.{");
		
		//get index
		while((*p)>='0'&&(*p)<='9')
		{
			index_buf[count++]=*p;
			p++;
		}
		idx=atoi(index_buf)+1;
		if(idx>maxNum)
		{
			printf("invalid input idx %d must less than max:%d!\n",idx,DNS_CLIENT_SERVER_NUM);
			return -1;
		}
		apmib_set(MIB_DNS_CLIENT_SERVER_TBL_NUM,(void*)&maxNum);
		//get entry
		*((char*)&entry)=(char)idx;
		if(apmib_get(MIB_DNS_CLIENT_SERVER_TBL,(void*)&entry)==0)
		{
			printf("get MIB_DNS_CLIENT_SERVER_TBL fail!\n");
			return -1;
		}

		//get name		
		if((*p)!='}'&&(*(p+1)!='.'))
		{
			printf("invalid input %s!\n",name);
			return -1;
		}
		name=p+2;
		
		if(strcmp(name,"Enable")==0)
		{
			*((int*)value)=entry.enable;
			return 1;
		}
		else if(strcmp(name,"Status")==0)
		{
			if(entry.status == 1)
				strcpy((char*)value,"Enable");
			else
				strcpy((char*)value,"Disable");
			return 2;
		}
		/*
		else if(strcmp(name,"Alias")==0)
		{
			strcpy((char*)value,entry.alias);
			return 2;
		}
		*/
		else if(strcmp(name,"DNSServer")==0)
		{
			strcpy((char*)value,entry.ipAddr);
			return 2;
		}
		/*
		else if(strcmp(name,"Interface")==0)
		{
			strcpy((char*)value,entry.interface);
			return 2;
		}
		*/
		else if(strcmp(name,"Type")==0)
		{
			if(entry.type == 1)
				strcpy((char*)value,"DHCPv4");
			else if(entry.type == 2)
				strcpy((char*)value,"DHCPv6");
			else if(entry.type == 3)
				strcpy((char*)value,"RouterAdvertisement");
			else if(entry.type == 4)
				strcpy((char*)value,"IPCP");
			else if(entry.type == 5)
				strcpy((char*)value,"Static");
			else
				strcpy((char*)value,"Unknown");
			return 2;
		}
		else
		{
			printf("can't find handle of Device.DNS.Client.Server.{%d}.%s\n",idx,name);
			return -1;
		}
	}
	printf("can't find handle of %s\n",name);

	return -1;
}
/*********************************
return value:
-1:fail
1:OK
2:need to reinit
3.need to reboot
********************************/
int tr181_ipv6_set(char * name,void * value)
{
	int val=0;
	int origVal=0;
	if(!name||!value)
	{
		printf("tr181_get invalid input!\n");
		return -1;
	}
#ifdef CONFIG_IPV6
	if(strcmp(name,"Device.DHCPv6.Client.Enable")==0)
	{
		val=*((int*)value);
		if(apmib_get(MIB_IPV6_WAN_ENABLE,(void*)&origVal)==0)
		{
			printf("get MIB_IPV6_WAN_ENABLE fail!\n");
			return -1;
		}
		if(origVal==val)
			return 1;
		if(apmib_set(MIB_IPV6_WAN_ENABLE,(void*)&val)==0)
		{
			printf("set MIB_IPV6_WAN_ENABLE fail!\n");
			return -1;
		}
		return 2;
	}
	if(strcmp(name,"Device.DHCPv6.Client.Interface")==0)
	{
		char buff[64]={0};
		if(tr181_ipv6_get(name,(void*)buff)<0)
		{
			printf("can't set %s\n",name);
			return -1;
		}
		if(strcmp(buff,(char*)value)==0)
		{
			return 1;
		}
		if(apmib_set(MIB_IPV6_DHCPC_IFACE,(void*)value)==0)
		{
			printf("set MIB_IPV6_DHCPC_IFACE fail!\n");
			return -1;
		}
		return 2;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.RequestAddresses")==0)
	{
		val=*((int*)value);
		if(apmib_get(MIB_IPV6_DHCPC_REQUEST_ADDR,(void*)&origVal)==0)
		{
			printf("get MIB_IPV6_DHCPC_REQUEST_ADDR fail!\n");
			return -1;
		}
		if(val==origVal)return 1;
		if(apmib_set(MIB_IPV6_DHCPC_REQUEST_ADDR,(void*)&val)==0)
		{
			printf("set MIB_IPV6_DHCPC_REQUEST_ADDR fail!\n");
			return -1;
		}		
		return 2;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.RequestPrefixes")==0)
	{
		val=*((int*)value);
		if(apmib_get(MIB_IPV6_DHCP_PD_ENABLE,(void*)&origVal)==0)
		{
			printf("get MIB_IPV6_DHCP_PD_ENABLE fail!\n");
			return -1;
		}
		if(val==origVal)return 1;
		if(apmib_set(MIB_IPV6_DHCP_PD_ENABLE,(void*)&val)==0)
		{
			printf("set MIB_IPV6_DHCP_PD_ENABLE fail!\n");
			return -1;
		}		
		return 2;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.RapidCommit")==0)
	{
		val=*((int*)value);
		if(apmib_get(MIB_IPV6_DHCP_RAPID_COMMIT_ENABLE,(void*)&origVal)==0)
		{
			printf("get MIB_IPV6_DHCP_RAPID_COMMIT_ENABLE fail!\n");
			return -1;
		}
		if(val==origVal)return 1;
		if(apmib_set(MIB_IPV6_DHCP_RAPID_COMMIT_ENABLE,(void*)&val)==0)
		{
			printf("set MIB_IPV6_DHCP_RAPID_COMMIT_ENABLE fail!\n");
			return -1;
		}		
		return 2;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.Renew")==0)
	{
		val=*((int*)value);
		if(val)
		{
			FILE* fp=fopen("/var/run/dhcp6c.pid","r");
			int pid=0;
			if(fp==NULL)
			{
				printf("can't open pid file /var/run/dhcp6c.pid!\n");
				return -1;
			}
			fscanf(fp,"%d",&pid);
			fclose(fp);
			if(pid<0 || pid>100000)
			{
				printf("invalid pid in file /var/run/dhcp6c.pid!\n");
				return -1;
			}
			kill(pid,SIGHUP);
		}
		return 1;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.SuggestedT1")==0)
	{
		val=*((int*)value);
		if(apmib_get(MIB_IPV6_DHCPC_SUGGESTEDT1,(void*)&origVal)==0)
		{
			printf("get MIB_IPV6_DHCPC_REGGEST_T1 fail!\n");
			return -1;
		}
		if(val==origVal)return 1;
		if(apmib_set(MIB_IPV6_DHCPC_SUGGESTEDT1,(void*)&val)==0)
		{
			printf("set MIB_IPV6_DHCPC_REGGEST_T1 fail!\n");
			return -1;
		}
		return 2;
	}
	else if(strcmp(name,"Device.DHCPv6.Client.SuggestedT2")==0)
	{
		val=*((int*)value);
		if(apmib_get(MIB_IPV6_DHCPC_SUGGESTEDT2,(void*)&origVal)==0)
		{
			printf("get MIB_IPV6_DHCPC_REGGEST_T2 fail!\n");
			return -1;
		}
		if(val==origVal)return 1;
		if(apmib_set(MIB_IPV6_DHCPC_SUGGESTEDT2,(void*)&val)==0)
		{
			printf("set MIB_IPV6_DHCPC_REGGEST_T2 fail!\n");
			return -1;
		}
		return 2;
	}
	else if(strncmp(name,"Device.DHCPv6.Client.SentOption.{",strlen("Device.DHCPv6.Client.SentOption.{"))==0)
	{
		char index_buf[8]={0};
		int idx=0,count=0,maxNum=IPV6_DHCPC_SENDOPT_NUM;
		DHCPV6C_SENDOPT_T entry[2]={0};
		char*p=name+strlen("Device.DHCPv6.Client.SentOption.{");
		
		//get index
		while((*p)>='0'&&(*p)<='9')
		{
			index_buf[count++]=*p;
			p++;
		}
		idx=atoi(index_buf)+1;
		if(idx>maxNum)
		{
			printf("invalid input idx %d must less than max:%d!\n",idx,IPV6_DHCPC_SENDOPT_NUM);
			return -1;
		}
		apmib_set(MIB_IPV6_DHCPC_SENDOPT_TBL_NUM,(void*)&maxNum);
		//get entry
		*((char*)entry)=(char)idx;
		if(apmib_get(MIB_IPV6_DHCPC_SENDOPT_TBL,(void*)entry)==0)
		{
			printf("get MIB_IPV6_DHCPC_SENDOPT_TBL fail!\n");
			return -1;
		}
		entry[1]=entry[0];

		//get name		
		if((*p)!='}'&&(*(p+1)!='.'))
		{
			printf("invalid input %s!\n",name);
			return -1;
		}
		name=p+2;
		
		if(strcmp(name,"Enable")==0)
		{
			if(*((int*)value)==entry[1].enable)
				return 1;
			entry[1].enable=*((int*)value);			
		}
		else if(strcmp(name,"Tag")==0)
		{
			//*((int*)value)=entry.tag;
			//return 1;
			//check other tag
			int i=0,tagVal=0;
			DHCPV6C_SENDOPT_T entryTmp={0};
			tagVal=*((int*)value);
			if(entry[1].tag==tagVal)
				return 1;
			for(i=1;i<=maxNum;i++)
			{
				*((char *)&entryTmp) = (char)i;
				if ( !apmib_get(MIB_IPV6_DHCPC_SENDOPT_TBL, (void *)&entryTmp)){
					printf("set %s fail!\n",name);
					return -1;
				}
				if(entryTmp.tag==tagVal && tagVal!=0)
				{
					printf("set %s fail! Tag exist!\n",name);
					return -1;
				}
			}
			entry[1].tag=*((int*)value);
		}
		else if(strcmp(name,"Value")==0)
		{
			if(strcmp((char*)value,entry[1].value)==0)
				return 1;
			strcpy(entry[1].value,(char*)value);
		}else
		{
			printf("can't find handle of Device.DHCPv6.Client.SentOption.{%d}.%s\n",idx,name);
			return -1;
		}
		if(apmib_set(MIB_IPV6_DHCPC_SENDOPT_MOD,(void*)entry)==0)
		{
			printf("set %s fail!\n",name);
			return -1;
		}
		return 2;
	}
#endif
	if(strncmp(name,"Device.DNS.Client.Enable",strlen("Device.DNS.Client.Enable"))==0)
	{
		val=*((int*)value);
		if(apmib_get(MIB_DNS_CLIENT_ENABLE,(void*)&origVal)==0)
		{
			printf("get MIB_DNS_CLIENT_ENABLE fail!\n");
			return -1;
		}
		if(origVal==val)
			return 1;
		if(apmib_set(MIB_DNS_CLIENT_ENABLE,(void*)&val)==0)
		{
			printf("set MIB_IPV6_WAN_ENABLE fail!\n");
			return -1;
		}
		return 2;
	}
	else if(strncmp(name,"Device.DNS.Client.Server.{",strlen("Device.DNS.Client.Server.{"))==0)
	{
		char index_buf[8]={0};
		int idx=0,count=0,maxNum=DNS_CLIENT_SERVER_NUM;
		DNS_CLIENT_SERVER_T entry[2]={0};
		char*p=name+strlen("Device.DNS.Client.Server.{");
		
		//get index
		while((*p)>='0'&&(*p)<='9')
		{
			index_buf[count++]=*p;
			p++;
		}
		idx=atoi(index_buf)+1;
		if(idx>maxNum)
		{
			printf("invalid input idx %d must less than max:%d!\n",idx,DNS_CLIENT_SERVER_NUM);
			return -1;
		}
		apmib_set(MIB_DNS_CLIENT_SERVER_TBL_NUM,(void*)&maxNum);
		//get entry
		*((char*)entry)=(char)idx;
		if(apmib_get(MIB_DNS_CLIENT_SERVER_TBL,(void*)entry)==0)
		{
			printf("get MIB_DNS_CLIENT_SERVER_TBL fail!\n");
			return -1;
		}
//		entry[1]=entry[0];
		memcpy(&(entry[1]), &(entry[0]), sizeof(DNS_CLIENT_SERVER_T));

		//get name		
		if((*p)!='}' || (*(p+1)!='.'))
		{
			printf("invalid input %s!\n",name);
			return -1;
		}
		name=p+2;
		
		if(strcmp(name,"Enable")==0)
		{
			if(*((int*)value)==entry[1].enable)
				return 1;
			entry[1].enable=*((int*)value);
			entry[1].status = *((int*)value);
		}
		/*
		else if(strcmp(name,"Alias")==0)
		{
			if(strcmp((char*)value,entry[1].alias)==0)
				return 1;
			strcpy(entry[1].alias,(char*)value);
		}
		*/
		else if(strcmp(name,"DNSServer")==0)
		{
			if(strcmp((char*)value,entry[1].ipAddr)==0)
				return 1;
			if(entry[1].type == 5)
				strcpy(entry[1].ipAddr,(char*)value);
			else
			{
				printf("DNSServer is automatically configured as result of DHCP, IPCP, or RA received DNS server information.\n");
				return -1;
			}
			struct in_addr ia_val;
#ifdef CONFIG_IPV6
			addr6CfgParam_t addr6_dns;
			addr6_dns.prefix_len = 64;
			if ( inet_aton(value, &ia_val) == 0 &&
				inet_pton6(value, addr6_dns.addrIPv6) ==0)
#else
			if ( inet_aton(value, &ia_val) == 0)
#endif
			{
				printf("invalid internet address!\n");
				return;
			}
				
			if(idx == 4)
			{
				apmib_set(MIB_DNS1, (void *)&ia_val);
			}
			else if(idx == 5)
			{
				apmib_set(MIB_DNS2, (void *)&ia_val);
			}
			else if(idx == 6)
			{
				apmib_set(MIB_DNS3, (void *)&ia_val);
			}
#ifdef CONFIG_IPV6
			else if(idx == 9)
			{
				if(!apmib_set(MIB_IPV6_ADDR_DNS_PARAM,  (void *)&addr6_dns))
				{
					printf("set MIB_IPV6_ADDR_DNS_PARAM failed\n");
					return;
				}
			}
#endif
		}
		/*
		else if(strcmp(name,"Interface")==0)
		{
			if(strcmp((char*)value,entry[1].interface)==0)
				return 1;
			if(entry[1].type == 5)
				strcpy(entry[1].interface,(char*)value);
			else
			{
				printf("Interface is automatically configured as result of DHCP, IPCP, or RA received DNS server information.\n");
				return -1;
			}
		}
		*/
		else
		{
			printf("can't find handle of Device.DNS.Client.Server.{%d}.%s\n",idx,name);
			return -1;
		}
		if(apmib_set(MIB_DNS_CLIENT_SERVER_MOD,(void*)entry)==0)
		{
			printf("set %s fail!\n",name);
			return -1;
		} 
		return 2;
	}
	printf("can't find handle of %s\n",name);
}
#endif
#endif

#if 0
static char currentTimeBuf[64];
char* get_time()
{
#include <sys/time.h>
	struct timeval currentTime={0};

	if(gettimeofday(&currentTime,NULL)<0)
		return NULL;

	sprintf(currentTimeBuf,"%d %ld",currentTime.tv_sec,currentTime.tv_usec);
	return currentTimeBuf;
}
#endif
#ifdef MULTI_WAN_SUPPORT

#ifdef MAIN_WAN_MAPPING

int update_uninited_mappingWanIdx()
{//hope not run this function, just set wanidx to wanifidx
	int i=0;
	fprintf(stderr,"Mib data not init!\n");
	for(i=1;i<=WANIFACE_NUM;i++){
		//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);
		if(apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i)==0) {
			//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);
			return -1;
			}
		//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);
		if(apmib_set(MIB_WANIFACE_MAPPING_WANIFIDX|MIB_NO_REINIT_FLAG,(void*)&i)==0){
			//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);
			return -1;
			}
		//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);
	}
	//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);
	apmib_update(CURRENT_SETTING);
	return 0;
}
#endif
//convent wan index to wan interface index
int getWanIfIdx(int wanIdx)
{
#ifdef MAIN_WAN_MAPPING
	int mappingWanIfIdx=0,wanIdx_bak=0;
	if(wanIdx<1 || wanIdx>WANIFACE_NUM) return -1;
	apmib_get(MIB_WANIFACE_CURRENT_IDX,(void*)&wanIdx_bak);
	if(apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&wanIdx)==0) return -1;
	if(apmib_get(MIB_WANIFACE_MAPPING_WANIFIDX,(void*)&mappingWanIfIdx)==0) return -1;
	if(mappingWanIfIdx<1 || mappingWanIfIdx>WANIFACE_NUM){
		fprintf(stderr,"%s:%d wan%d mappingWanIfIdx=%d\n",__FUNCTION__,__LINE__,wanIdx,mappingWanIfIdx);
		if(update_uninited_mappingWanIdx()<0){
			//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		if(apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&wanIdx)==0) return -1;
		//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);
		if(apmib_get(MIB_WANIFACE_MAPPING_WANIFIDX,(void*)&mappingWanIfIdx)==0) return -1;		
	}
	//fprintf(stderr,"%s:%d wanIdx=%d mappingWanIfIdx=%d\n",__FUNCTION__,__LINE__,wanIdx,mappingWanIfIdx);
	if(wanIdx_bak!=0)
		apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&wanIdx_bak);
	return mappingWanIfIdx;
#else
	return wanIdx;
#endif
}
int getPPPIfName(int index,char* ifName,int len)
{
	int wanIfIdx=0;
	if(!ifName||len<6) return -1;
	if(index<1||index>WANIFACE_NUM){
		printf("Invalid wan index %d!\n",index);
		return -1;
	}

#ifdef CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT	
	return get_wan_ppp_ifname_from_idx(index, ifName);
#endif
	
	//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);
	wanIfIdx=getWanIfIdx(index);
	if(wanIfIdx==1) snprintf(ifName,len,"ppp1");
	else snprintf(ifName,len,"ppp%d",wanIfIdx+6);
	return 0;
}
int getEthWanIfName(int index,char* ifName,int len)
{
	int wanIfIdx=0;
	if(!ifName||len<6) return -1;
	if(index<1||index>WANIFACE_NUM){
		printf(stderr,"Invalid wan index %d!\n",index);
		return -1;
	}

#ifdef CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT	
	return get_wan_eth_ifname_from_idx(index, ifName);
#endif
	
	//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);
	wanIfIdx=getWanIfIdx(index);
	if(wanIfIdx==1) snprintf(ifName,len,"eth1");
	else snprintf(ifName,len,"eth%d",wanIfIdx+6);
	return 0;
}
int GET_REAL_IFNAME(char *ifName)	
{	
	int stridx;

	if(ifName == NULL)
		return 0;
	
	for(stridx=0; stridx<32; stridx++)	
		if(ifName[stridx] == '.')
		{
			ifName[stridx] = '\0';
			return 1;
		}

	return 0;
}
#ifdef MAIN_WAN_MAPPING
int getWanIdxViaIfIdx(int ifIdx)
{
	int i=0,tmpIfidx=0;
	for(i=1;i<=WANIFACE_NUM;i++){
		apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i);
		
		if(apmib_get(MIB_WANIFACE_MAPPING_WANIFIDX,(void*)&tmpIfidx)==0){
			return -1;
		}
		if(tmpIfidx==ifIdx) return i;
	}
	fprintf(stderr,"Mib data MIB_WANIFACE_MAPPING_WANIFIDX not init!\n");
	return -1;
}
#endif

int getWanIfIdexViaIfname(char* name)
{
	int idx = 0;
	char realName[32] = {0};
	

	if(! name) 
		return -1;

	memcpy(realName, name, 32);
	GET_REAL_IFNAME(realName);
	
	idx=atoi(realName+3);//ppp* eth*
	if(idx>1) idx-=6;
	if(idx<1||idx>WANIFACE_NUM)
		return -1;
	return idx;
}

#ifdef CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT

const char* LANIFNAME[] = {"eth0", "eth2", "eth3", "eth4", "eth5"};

struct port_root_iface_mapping port_iface_mapping[] = 
{
	{0, "null"},
	{1, "eth1"},
	{2, "eth8"},
	{3, "eth9"},
	{4, "eth10"},
	{5, "eth11"}
};

/*
	Check whether HW NAT is setted
*/
int check_hwNat_is_enabled(void)
{
	int i;
	int wan_enable=0;
	int hw_nat_enable=0;

	for(i=1;i<=WANIFACE_NUM;i++)
	{
		apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i);	
		
		if(apmib_get(MIB_WANIFACE_ENABLE, (void*)&wan_enable)==0)
			return -1;

		if(wan_enable==0)
			continue;

		if(apmib_get(MIB_WANIFACE_HW_NAT_ENABLED, (void*)&hw_nat_enable)==0)
			return -1;

		if(hw_nat_enable==1)
			return 1;		
	}
	return 0;
}

/*
	Get physical port ID according to logical port ID
*/
int transfer_logic_port_to_phy_port(int lport_id, int *pport_id)
{
	unsigned char logic_phy_port_mapping[MAX_PHY_PORT_NUM]={0};
	if(lport_id<1 || lport_id>MAX_PHY_PORT_NUM)
		return -1;

	if(apmib_get(MIB_WAN_LOGIC_PORT_MAPPING_TO_PHY_PORT, (void*)logic_phy_port_mapping)==0)
		return -1;

	*pport_id=logic_phy_port_mapping[lport_id-1];	

	return 0;
}

/*
	Get physical port ID according to LAN deivce name
	Lan device name is eth0/eth2/eth3/eth4/eth5
*/
int transfer_lan_dev_to_phy_port(char *ifname, int *pport_id)
{
	int i;
	unsigned char lan_dev_phy_port[MAX_PHY_PORT_NUM]={0};
	
	if(ifname==NULL || pport_id==NULL)
		return -1;

	for(i=0;i<MAX_PHY_PORT_NUM;i++)
	{
		if(strcmp(ifname, LANIFNAME[i])==0)
			break;
	}

	if(i>=MAX_PHY_PORT_NUM)
		return -1;
	
	if(apmib_get(MIB_LAN_DEV_MAPPING_TO_PHY_PORT, (void*)lan_dev_phy_port)==0)
		return -1;

	*pport_id=lan_dev_phy_port[i];

	return 0;
}

/*
	Check the physical port mapped by LAN device whether is setted by WAN 
*/
int check_lan_dev_exist(char *ifname)
{
	int lan_pport_id=0, wan_pport_id=0;
	int wan_enable=0, wan_lport_id=0;
	int i;
	
	if(ifname==NULL)
		return -1;
	
	if(transfer_lan_dev_to_phy_port(ifname, &lan_pport_id)<0)
		return -1;

	for(i=1;i<=WANIFACE_NUM;i++)
	{
		apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i);	
		
		if(apmib_get(MIB_WANIFACE_ENABLE, (void*)&wan_enable)==0)
			return -1;		

		if(wan_enable==0)
			continue;		

		if(apmib_get(MIB_WANIFACE_PORT_ID, (void*)&wan_lport_id)==0)
			return -1;		

		if(transfer_logic_port_to_phy_port(wan_lport_id, &wan_pport_id)<0)
			continue;		

		if(wan_pport_id==lan_pport_id)
			return 0;	
	}
	return 1;	
}

/*
	Check the VLAN ID whether is used by WAN which binding to the same logical port
*/
int check_port_vid_is_used(int port_id, int vlan_id)
{
	int i;
	int wan_enable=0;
	int portId=0;
	int vid=0;
	
	if(port_id<1 || port_id>MAX_PHY_PORT_NUM)
		return -1;

	for(i=1;i<=WANIFACE_NUM;i++)
	{
		apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i);
		
		if(apmib_get(MIB_WANIFACE_ENABLE, (void*)&wan_enable)==0)
			return -1;

		if(wan_enable==0)
			continue;

		if(apmib_get(MIB_WANIFACE_PORT_ID, (void*)&portId)==0)
			return -1;

		if(portId==port_id)
		{
			if(apmib_get(MIB_WANIFACE_VLANID, (void*)&vid)==0)
				return -1;	

			if(vlan_id==vid)
				return i;
		}
	}
	return 0;
}

/*
	Get WAN ethernet device name according to logical port ID and VLAN ID
*/
int generate_wan_eth_ifname(int port_id, int vlan_id, char *ifname)
{
	int i;
	int wan_enable=0;
	char tmp_ifname[16]={0};
	
	if(port_id<1 || port_id>MAX_PHY_PORT_NUM || ifname==NULL)
		return -1;

	strcpy(ifname, port_iface_mapping[port_id].root_ifname);

	if(vlan_id>0)
	{
		sprintf(ifname, "%s.%d", ifname, vlan_id);
	}
	
	for(i=1;i<=WANIFACE_NUM;i++)
	{
		apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i);
		
		if(apmib_get(MIB_WANIFACE_ENABLE, (void*)&wan_enable)==0)
			return -1;

		if(wan_enable==0)
			continue;
		
		if(apmib_get(MIB_WANIFACE_WAN_ETH_NAME,(void*)tmp_ifname)==0)
			return -1;

		if(strcmp(tmp_ifname, ifname)==0)
		{
			printf("\n%s:%d VID %d is already exit!!!\n",__FUNCTION__,__LINE__,vlan_id);
			return i;
		}		
	}

	return 0;
}

/*	
	Get WAN PPP device name according to WAN index
*/
int generate_wan_ppp_ifname(int wan_idx, char *ifname)
{
	if(wan_idx<1 || wan_idx>WANIFACE_NUM || ifname==NULL)
		return -1;

	sprintf(ifname, "ppp%d", wan_idx);	

	return 0;
}

/*	
	Get WAN index according to WAN ethernet/PPP device name
*/
int get_wan_idx_from_ifname(char *ifname)
{
	int i=0;
	int wan_enable=0;
	char eth_name[16]={0};
	char ppp_name[16]={0};
	for(i=1;i<=WANIFACE_NUM;i++)
	{
		apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i);

		if(apmib_get(MIB_WANIFACE_ENABLE, (void*)&wan_enable)==0)
			return -1;

		if(wan_enable==0)
			continue;
		
		if(apmib_get(MIB_WANIFACE_WAN_ETH_NAME,(void*)eth_name)==0)
			return -1;

		if(strcmp(ifname, eth_name)==0)
			return i;

		if(apmib_get(MIB_WANIFACE_WAN_PPP_NAME,(void*)ppp_name)==0)
			return -1;

		if(strcmp(ifname, ppp_name)==0)
			return i;		
	}
	return -1;
}

/*	
	Get WAN ethernet device name according to WAN index
*/
int get_wan_eth_ifname_from_idx(int wan_idx, char *ifname)
{
	int i=0;
	char eth_name[16]={0};
	char ppp_name[16]={0};

	if(wan_idx<1 || wan_idx>WANIFACE_NUM || ifname==NULL)
		return -1;
	
	apmib_set(MIB_WANIFACE_CURRENT_IDX, (void*)&wan_idx);
	
	if(apmib_get(MIB_WANIFACE_WAN_ETH_NAME,(void*)ifname)==0)
		return -1;

	return 0;	
}

/*	
	Get WAN PPP device name according to WAN index
*/
int get_wan_ppp_ifname_from_idx(int wan_idx, char *ifname)
{
	int i=0;
	char eth_name[16]={0};
	char ppp_name[16]={0};

	if(wan_idx<1 || wan_idx>WANIFACE_NUM || ifname==NULL)
		return -1;
	
	apmib_set(MIB_WANIFACE_CURRENT_IDX, (void*)&wan_idx);
	
	if(apmib_get(MIB_WANIFACE_WAN_PPP_NAME, (void*)ifname)==0)
		return -1;

	return 0;	
}

/*	
	Get the logical port ID binded by WAN according to WAN index
*/
int get_wan_logic_portid_from_idx(int wan_idx, int *lport_id)
{
	int i=0;
	int wan_enable=0;
	int wan_lport_id=0;

	if(wan_idx<1 || wan_idx>WANIFACE_NUM || lport_id==NULL)
		return -1;
	
	apmib_set(MIB_WANIFACE_CURRENT_IDX, (void*)&wan_idx);
	
	if(apmib_get(MIB_WANIFACE_ENABLE, (void*)&wan_enable)==0)
		return -1;

	if(wan_enable==0)
		return -1;
	
	if(apmib_get(MIB_WANIFACE_PORT_ID, (void*)&wan_lport_id)==0)
		return -1;

	*lport_id=wan_lport_id;

	return 0;	
}

/*	
	Get WAN index which is setted HW NAT
*/
int get_hw_nat_wan_idx(int *wan_idx)
{
	int i;
	int wan_enable=0;
	int hw_nat_enable=0;

	for(i=1;i<=WANIFACE_NUM;i++)
	{
		apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i);	
		
		if(apmib_get(MIB_WANIFACE_ENABLE, (void*)&wan_enable)==0)
			return -1;

		if(wan_enable==0)
			continue;

		if(apmib_get(MIB_WANIFACE_HW_NAT_ENABLED, (void*)&hw_nat_enable)==0)
			return -1;

		if(hw_nat_enable==1)
		{
			*wan_idx=i;	
			return 0;
		}
	}
	return -1;
}

/*	
	Check the physical port mapped by wan logical port whether used by LAN device which is binded to WAN 
*/
int check_wan_logic_port_available(int wan_lport_id)
{
	int i;	
	int lan_pport_id=0, wan_pport_id=0;
	WanIntfacesType wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM]={0};
	WanIntfacesType wan_idx;
	
	if(wan_lport_id<1 || wan_lport_id>MAX_PHY_PORT_NUM)
		return -1;
	
	if(transfer_logic_port_to_phy_port(wan_lport_id, &wan_pport_id)<0)
		return -1;
	
	if(apmib_get(MIB_WANIFACE_BINDING_LAN_PORTS,(void*)wanBindingLanPorts)==0)
		return -1;

	for(i=0;i<WAN_INTERFACE_LAN_PORT_NUM;i++)
	{
		wan_idx=wanBindingLanPorts[i];
		if(wan_idx<1 || wan_idx>WANIFACE_NUM)
			continue;

		if(transfer_lan_dev_to_phy_port(LANIFNAME[i], &lan_pport_id)<0)
			continue;

		if(wan_pport_id==lan_pport_id)
			return 0;		
	}	
	return 1;	
}


/*
	Update bridge ethernet device list
*/
int update_bridge_iface_list(void)
{
	int i;
	int retVal;
	char cmdBuf[64]={0};
	
	for(i=0;i<MAX_PHY_PORT_NUM;i++)
	{
		retVal=check_lan_dev_exist(LANIFNAME[i]);
		
		if(retVal==0)
		{
			snprintf(cmdBuf, sizeof(cmdBuf), "brctl delif br0 %s >/dev/null 2>&1",LANIFNAME[i]);
			system(cmdBuf);
		}
		else if(retVal==1)
		{
			snprintf(cmdBuf, sizeof(cmdBuf), "brctl addif br0 %s >/dev/null 2>&1",LANIFNAME[i]);
			system(cmdBuf);
		}
	}
	return 0;
}

/*
	Get LAN device index which mapping to the same physical port ID with the WAN logical port ID
*/
int get_lan_dev_idx_from_logic_port(int wan_lport_id)
{
	int i;
	int wan_pport_id;
	unsigned char lan_dev_phy_port[MAX_PHY_PORT_NUM]={0};
	
	if(wan_lport_id<1 || wan_lport_id>MAX_PHY_PORT_NUM)
		return -1;

	if(transfer_logic_port_to_phy_port(wan_lport_id, &wan_pport_id)<0)
		return -1;

	if(apmib_get(MIB_LAN_DEV_MAPPING_TO_PHY_PORT, (void*)lan_dev_phy_port)==0)
		return -1;

	for(i=0;i<MAX_PHY_PORT_NUM;i++)
	{
		if(lan_dev_phy_port[i]==wan_pport_id)
			return i+1;	
	}
	
	return -1;	
}

/*setup mac addr if mac[n-1]++ > 0xff and set mac[n-2]++, etc.*/
void setup_mac_addr(unsigned char *macAddr, int index)
{
	if((macAddr[5]+index) > 0xff)
	{
	  printf("\nmacAddr[5]+%d = %x > 0xff\n",index,macAddr[5]+index);
	  if((macAddr[4]+1) > 0xff)
	  {
	  	printf("macAddr[4]+1 = %x > 0xff\n",macAddr[4]+1);	  
	  	if((macAddr[3]+1) > 0xff)
		{
		  printf("something wrong for setting your macAddr[3] > 0xff!!\n");
		}
		macAddr[3]+=1;
	  }
	  macAddr[4]+=1;
    }

	macAddr[5]+=index;
}

/* Get first internet WAN index*/
int get_first_internet_wan_idx(int *wan_idx)
{
	int i;
	char file_name[32]={0};
	for(i=1;i<=WANIFACE_NUM;i++)
	{
		snprintf(file_name,sizeof(file_name), "/var/internet_wan_%d", i);
		if(isFileExist(file_name))
		{
			*wan_idx=i;
			return 0;
		}		
	}
	return -1;
}

/* Get WAN root interface name according WAN logic port ID*/
int get_wan_root_name_from_logic_port(int wan_lport_id, char *root_ifname)
{
	int i;	
	int wan_enable=0;
	int port_id=0;
	
	if(wan_lport_id<1 || wan_lport_id>MAX_PHY_PORT_NUM || root_ifname==NULL)
		return -1;


	for(i=1;i<=WANIFACE_NUM;i++)
	{
		apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i);	
		
		if(apmib_get(MIB_WANIFACE_ENABLE, (void*)&wan_enable)==0)
			return -1;

		if(wan_enable==0)
			continue;

		if(apmib_get(MIB_WANIFACE_PORT_ID, (void*)&port_id)==0)
			return -1;

		if(port_id==wan_lport_id)
		{
			strcpy(root_ifname, port_iface_mapping[port_id].root_ifname);
			return 0;
		}
	}	
	return -1;	
}

/* Get LAN physical port mask according LAN device name*/
int get_lan_port_mask_from_dev_name(char *ifname, int *port_mask)
{
	int lan_pport_id=-1;

	if(ifname==NULL || port_mask==NULL)
		return -1;
	
	if(transfer_lan_dev_to_phy_port(ifname, &lan_pport_id)==0)
	{
		*port_mask=(1<<lan_pport_id);
		return 0;
	}

	return -1;
}

/* Get WAN physical port mask according WAN index*/
int get_wan_port_mask_from_wan_idx(int wan_idx, int *port_mask)
{
	int wan_lport_id=-1;
	int wan_pport_id=-1;

	if(wan_idx<1 || wan_idx>WANIFACE_NUM  || port_mask==NULL)
		return -1;
	
	if(get_wan_logic_portid_from_idx(wan_idx, &wan_lport_id)==0)
	{
		if(transfer_logic_port_to_phy_port(wan_lport_id, &wan_pport_id)==0)
		{
			*port_mask=(1<<wan_pport_id);
			return 0;
		}
	}
	
	return -1;
}

int get_wan_root_name_from_wan_eth_name(char *wan_ifname, char *root_ifname)
{
	int i;
	char *pchar=NULL;
	char tmp_name[16]={0};
	
	if(wan_ifname==NULL || root_ifname==NULL)
		return -1;

	strcpy(tmp_name, wan_ifname);
	
	pchar=strchr(tmp_name, '.');
	
	if(pchar!=NULL)
		*pchar='\0';

	strcpy(root_ifname, tmp_name);
	
	return 0;
}

int check_exist_other_wan_use_root_iface(int wan_idx, char *root_ifname)
{
	int i=0;
	int wan_enable=0;
	char eth_name[16]={0};
	char eth_root_name[16]={0};

	if(wan_idx<1 || wan_idx>WANIFACE_NUM  || root_ifname==NULL)
		return -1;
	
	for(i=1;i<=WANIFACE_NUM;i++)
	{
		apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i);

		if(apmib_get(MIB_WANIFACE_ENABLE, (void*)&wan_enable)==0)
			return -1;

		if(wan_enable==0 || i==wan_idx)
			continue;

		if(apmib_get(MIB_WANIFACE_WAN_ETH_NAME,(void*)eth_name)==0)
			return -1;
		
		get_wan_root_name_from_wan_eth_name(eth_name, eth_root_name);

		if(strcmp(eth_root_name, root_ifname)==0)
			return i;		
	}
	return -1;
}

#endif

int getWanIdx(const char *name)
{
#ifdef CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT
	return get_wan_idx_from_ifname(name);
#endif

	int idx = 0;
	char realName[32] = {0};	

	if(! name) 
		return -1;

	memcpy(realName, name, 32);
	GET_REAL_IFNAME(realName);
	
	idx=atoi(realName+3);//ppp* eth*
	if(idx>1) idx-=6;
	if(idx<1||idx>WANIFACE_NUM)
		return -1;
#ifdef MAIN_WAN_MAPPING
	return getWanIdxViaIfIdx(idx);
#else
	return idx;
#endif
}


int getWanIfaceEntry(int index,WANIFACE_T* pEntry)
{	
	apmib_init();
	memset(pEntry, '\0', sizeof(*pEntry));	
	*((char *)pEntry) = (char)index;
	if(!apmib_get(MIB_WANIFACE_TBL,(void *)pEntry)){
		printf("get wanIface mib error\n");
		return 0;
	}
	else
		return 1;
}

int updateWanIfName(void)
{
	WANIFACE_T WanIfaceEntry;
	int idx;
	char cmdbuf[128], tmpbuf[16];
	int vlanId = 0;
	char wanIfName[IFNAMESIZE] ={0};
	char runCmdBuf[256]={0};


	memset((void *)cmdbuf, 0, sizeof(cmdbuf));
	
	for(idx=1; idx<=WANIFACE_NUM; idx++)
	{
		if(!getWanIfaceEntry(idx, &WanIfaceEntry))
		{
			printf("%s.%d.get waniface entry fail\n", __FUNCTION__, __LINE__);
			return -1;
		}
		if(getEthWanIfName(idx,wanIfName,sizeof(wanIfName))<0){
			printf("%s.%d.get getEthWanIfName fail idx=%d wanIfName=%s\n", __FUNCTION__, __LINE__,idx,wanIfName);
			return -1;
		}

		vlanId = WanIfaceEntry.vlan ? WanIfaceEntry.vlanid : -1;
		if(WanIfaceEntry.enable && vlanId > 0)
			sprintf(tmpbuf, "%s.%d:", wanIfName, vlanId);
		else
			sprintf(tmpbuf, "%s:", wanIfName);

		strcat(cmdbuf, tmpbuf);
	}

	snprintf(runCmdBuf,sizeof(runCmdBuf),"echo %s > %s",cmdbuf,MULTI_WAN_IFNAME_FILE);
	system(runCmdBuf);
	return 0;
}
int getWanIfName(int wan_idx, char *name,int len)
{
	FILE *fp = NULL;
	char buf[128];
	char *token = NULL, *saveStr = NULL;
	int idx = 0;

	if(! name) 
		goto err_return;
	
	if(wan_idx < 1 || wan_idx > WANIFACE_NUM) 
		goto err_return;

#ifdef CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT	
	return get_wan_eth_ifname_from_idx(wan_idx, name);
#endif	

	if(!isFileExist(MULTI_WAN_IFNAME_FILE))
	{
		updateWanIfName();

		if(!isFileExist(MULTI_WAN_IFNAME_FILE))
			goto err_return;
	}

	fp = fopen(MULTI_WAN_IFNAME_FILE, "r");
	if(!fp)
		goto err_return;

	memset(buf, 0, sizeof(buf));
	fgets(buf, sizeof(buf), fp);
	fclose(fp);
	token = strtok_r(buf, ":", &saveStr);
	do{
		if(!token)
			break;

		idx++;
		if(idx == wan_idx)
			break;
		
		token = strtok_r(NULL, ":", &saveStr);
	}
	while(token);

	if(token == NULL || idx != wan_idx || memcmp(token, "eth", 3) != 0)
		goto err_return;
	
	snprintf(name,len, "%s", token);

	return 0;

err_return:
	
	printf("[%s %d] getWanIfName failed!\n", __FUNCTION__, __LINE__);
	return -1;
}


#ifdef MAIN_WAN_MAPPING
void dump_mappingWanIdx_info()
{
#if 0
	int i=0;
	int wan_idx=0,mappingWanIfIdx=0;
	char ifname[IFNAMESIZE]={0};
	printf("current mapping:\n");
	if(apmib_get(MIB_WANIFACE_CURRENT_IDX,(void*)&wan_idx)==0) return -1;
	for(i=1;i<=WANIFACE_NUM;i++){
		if(apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i)==0) return -1;
		if(apmib_get(MIB_WANIFACE_MAPPING_WANIFIDX,(void*)&mappingWanIfIdx)==0) return -1;
		if(getWanIfName(i,ifname,sizeof(ifname))<0)return -1;
		printf("wan%d  wanif%d %s\n",i,mappingWanIfIdx,ifname);
	}
	if(apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&wan_idx)==0) return -1;
	//fprintf(stderr,"%s:%d\n",__FUNCTION__,__LINE__);
#endif
}

int get_main_wan_index(void)
{
	int i = 0, wanidx_bak = 0, theWanMappingWanIfIdex = 0;
	
	apmib_get(MIB_WANIFACE_CURRENT_IDX,(void*)&wanidx_bak);

	for(i = 1; i <= WANIFACE_NUM; i++){
		apmib_set(MIB_WANIFACE_CURRENT_IDX, (void*)&i);
		apmib_get(MIB_WANIFACE_MAPPING_WANIFIDX, (void*)&theWanMappingWanIfIdex);

		if(theWanMappingWanIfIdex == 1){	// main wan
			break;
		}
	}

	if(wanidx_bak!=0)
		apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&wanidx_bak);

	return (i <= WANIFACE_NUM? i: -1);
}
#endif
#endif

#if defined(CONFIG_CMCC_QOS)
/* Macro definition */
static int _is_hex(char c)
{
    return (((c >= '0') && (c <= '9')) ||
            ((c >= 'A') && (c <= 'F')) ||
            ((c >= 'a') && (c <= 'f')));
}

static int string_to_hex(char *string, unsigned char *key, int len)
{
	char tmpBuf[4];
	int idx, ii=0;
	for (idx=0; idx<len; idx+=2) {
		tmpBuf[0] = string[idx];
		tmpBuf[1] = string[idx+1];
		tmpBuf[2] = 0;
		if ( !_is_hex(tmpBuf[0]) || !_is_hex(tmpBuf[1]))
			return 0;

		key[ii++] = (unsigned char) strtol(tmpBuf, (char**)NULL, 16);
	}
	return 1;
}

int updateCarId(int carType, int direction)
{
	int val = 0, index = 0, number = 0;
	int mib_num = 0, mib_tbl = 0;

	switch(carType){
		case 1:			// iface
		{
			QOSCAR_IFACE_T car_iface;
			memset(&car_iface, 0, sizeof(QOSCAR_IFACE_T));

			if(direction == OUT_BOUND){
				mib_num = MIB_QOS_CAR_UP_IFACE_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_IFACE_TBL;
			}
			else if(direction == IN_BOUND){
				mib_num = MIB_QOS_CAR_DOWN_IFACE_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_IFACE_TBL;
			}
			else{
				printf("[%s:%d] direction%d invalid!\n", __FUNCTION__, __LINE__, direction);
				return 0;
			}
			
			if(!apmib_get(mib_num, (void *)&number)){
				printf("[%s:%d]get MIB_TBL_NUM %d fail!\n", __FUNCTION__, __LINE__, mib_num);
				return 0;
			}

			for (index = 1; index <= number; index++){
		        *((char *)&car_iface) = (char)index;
			
		        if (!apmib_get(mib_tbl, (void *)&car_iface)){
					printf("[%s:%d]get MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_tbl);
					return 0;
		        }

				val = car_iface.CarIfaceInstanceNum;
				break;
		    }
		}
			break;

		case 2:			// vlan
		{
			QOSCAR_VLAN_T car_vlan;
			memset(&car_vlan, 0, sizeof(QOSCAR_VLAN_T));

			if(direction == OUT_BOUND){
				mib_num = MIB_QOS_CAR_UP_VLAN_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_VLAN_TBL;
			}
			else if(direction == IN_BOUND){
				mib_num = MIB_QOS_CAR_DOWN_VLAN_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_VLAN_TBL;
			}
			else{
				printf("[%s:%d] direction%d invalid!\n", __FUNCTION__, __LINE__, direction);
				return 0;
			}

			if(!apmib_get(mib_num, (void *)&number)){
				printf("[%s:%d]get MIB_TBL_NUM %d fail!\n", __FUNCTION__, __LINE__, mib_num);
				return 0;
			}

			for (index = 1; index <= number; index++){
		        *((char *)&car_vlan) = (char)index;
			
		        if (!apmib_get(mib_tbl, (void *)&car_vlan)){
					printf("[%s:%d]get MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_tbl);
					return 0;
		        }

				val = car_vlan.CarVlanInstanceNum;
		        break;
		    }

		}
			break;
		
		case 3:			// ip range
		{
			QOSCAR_IP_T car_ip;
			memset(&car_ip, 0, sizeof(QOSCAR_IFACE_T));

			if(direction == OUT_BOUND){
				mib_num = MIB_QOS_CAR_UP_IP_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_IP_TBL;
			}
			else if(direction == IN_BOUND){
				mib_num = MIB_QOS_CAR_DOWN_IP_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_IP_TBL;
			}
			else{
				printf("[%s:%d] direction%d invalid!\n", __FUNCTION__, __LINE__, direction);
				return 0;
			}

			if(!apmib_get(mib_num, (void *)&number)){
				printf("[%s:%d]get MIB_BL_NUM %d fail!\n", __FUNCTION__, __LINE__, mib_num);
				return 0;
			}

			for (index = 1; index <= number; index++){
		        *((char *)&car_ip) = (char)index;
			
		        if (!apmib_get(mib_tbl, (void *)&car_ip)){
					printf("[%s:%d]get MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_tbl);
					return 0;
		        }

				val = car_ip.CarIpInstanceNum;
		        break;
		    }
		}
			break;

	#if defined(CONFIG_MAC_SHAPING_QOS)
		case 4:			// mac
		{
			QOSCAR_MAC_T car_mac;
			memset(&car_mac, 0, sizeof(QOSCAR_MAC_T));

			if(direction == OUT_BOUND){
				mib_num = MIB_QOS_CAR_UP_MAC_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_MAC_TBL;
			}
			else if(direction == IN_BOUND){
				mib_num = MIB_QOS_CAR_DOWN_MAC_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_MAC_TBL;
			}
			else{
				printf("[%s:%d] direction%d invalid!\n", __FUNCTION__, __LINE__, direction);
				return 0;
			}

			if(!apmib_get(mib_num, (void *)&number)){
				printf("[%s:%d]get MIB_TBL_NUM %d fail!\n", __FUNCTION__, __LINE__, mib_num);
				return 0;
			}

			for (index = 1; index <= number; index++){
		        *((char *)&car_mac) = (char)index;
			
		        if (!apmib_get(mib_tbl, (void *)&car_mac)){
					printf("[%s:%d]get MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_tbl);
					return 0;
		        }

				val = car_mac.CarMacInstanceNum;
		        break;
		    }

		}
			break;
	#endif
	
		default:
			break;
	}

	if(!apmib_set(MIB_CURRENT_CARID, (void *)&val)){
       	printf("set MIB_CURRENT_CARID error!\n");
		return 0;
    }
	
	return 1;
}

int updateCarTbl(int direction, int mode)
{
	int mib_num = 0, mib_tbl = 0;
	int index = 0, num = 0, len = 0;
	char str_tmp[512] = {0};
	QOSCAR_T qosCar, qosCarArray[2];
		
	memset(&qosCar, '\0', sizeof(QOSCAR_T));
	memset(&qosCarArray[0], 0, sizeof(QOSCAR_T));
	memset(&qosCarArray[1], 0, sizeof(QOSCAR_T));
		
	if(!apmib_get(MIB_QOS_CAR_TBL_NUM, (void *)&num) || num < 1){
		printf("[%s:%d]get MIB_QOS_CAR_TBL_NUM fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	for(index = 1; index <= num; index++){
		*((char *)&qosCar) = (char)index;
        if (!apmib_get(MIB_QOS_CAR_TBL, (void *)&qosCar)){
			printf("[%s:%d]get MIB_QOS_CAR_TBL fail!\n", __FUNCTION__, __LINE__);
			return 0;
        }

		memcpy(&qosCarArray[0], &qosCar, sizeof(QOSCAR_T));
		break;
	}

	if(direction == OUT_BOUND){
		switch(mode){
			case 1:
				mib_num = MIB_QOS_CAR_UP_IFACE_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_IFACE_TBL;
				snprintf(qosCar.IfaceLimit_UP, MAX_CAR_MODE_LEN, "%s", "");	// flush
				break;
			case 2:
				mib_num = MIB_QOS_CAR_UP_VLAN_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_VLAN_TBL;
				snprintf(qosCar.VlanTagLimit_UP, MAX_CAR_MODE_LEN, "%s", "");	// flush
				break;
			case 3:
				mib_num = MIB_QOS_CAR_UP_IP_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_IP_TBL;
				snprintf(qosCar.IPLimit_UP, MAX_CAR_MODE_LEN, "%s", "");	// flush
				break;
		#if defined(CONFIG_MAC_SHAPING_QOS)
			case 4:
				return 1;
		#endif
			default:
				printf("[%s:%d] mode%d invalid!\n", __FUNCTION__, __LINE__, mode);
				return 0;
		}
	}
	else if(direction == IN_BOUND){
		switch(mode){
			case 1:
				mib_num = MIB_QOS_CAR_DOWN_IFACE_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_IFACE_TBL;
				snprintf(qosCar.IfaceLimit_DOWN, MAX_CAR_MODE_LEN, "%s", "");	// flush
				break;
			case 2:
				mib_num = MIB_QOS_CAR_DOWN_VLAN_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_VLAN_TBL;
				snprintf(qosCar.VlanTagLimit_DOWN, MAX_CAR_MODE_LEN, "%s", "");	// flush
				break;
			case 3:
				mib_num = MIB_QOS_CAR_DOWN_IP_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_IP_TBL;
				snprintf(qosCar.IPLimit_DOWN, MAX_CAR_MODE_LEN, "%s", "");	// flush
				break;
		#if defined(CONFIG_MAC_SHAPING_QOS)
			case 4:
				return 1;
		#endif
			default:
				printf("[%s:%d] mode%d invalid!\n", __FUNCTION__, __LINE__, mode);
				return 0;
		}
	}
	else{
		printf("[%s:%d] direction%d invalid!\n", __FUNCTION__, __LINE__, direction);
		return 0;
	}

	if(!apmib_get(mib_num, (void *)&num)){
		printf("[%s:%d]get MIB%d fail!\n", __FUNCTION__, __LINE__, mib_num);
		return 0;
	}

	for(index = 1; index <= num; index++){
		switch(mode){
			case 1:
			{
				QOSCAR_IFACE_T qosCar1;
				memset(&qosCar1, '\0', sizeof(QOSCAR_IFACE_T));
				
				*((char *)&qosCar1) = (char)index;
		        if (!apmib_get(mib_tbl, (void *)&qosCar1)){
					printf("[%s:%d]get MIB_TBL%d item%dfail!\n", __FUNCTION__, __LINE__, mib_tbl, index);
					continue;
		        }

				if(len == 0){
					len += sprintf(str_tmp+len, "%s/%d", qosCar1.CarIfaceName, qosCar1.CarIfaceShaping);
				}
				else{
					len += sprintf(str_tmp+len, ",%s/%d", qosCar1.CarIfaceName, qosCar1.CarIfaceShaping);
				}
			}
				break;
			case 2:
			{
				QOSCAR_VLAN_T qosCar2;
				memset(&qosCar2, '\0', sizeof(QOSCAR_VLAN_T));
				
				*((char *)&qosCar2) = (char)index;
		        if (!apmib_get(mib_tbl, (void *)&qosCar2)){
					printf("[%s:%d]get MIB_TBL%d item%dfail!\n", __FUNCTION__, __LINE__, mib_tbl, index);
					continue;
		        }

				if(len == 0){
					len += sprintf(str_tmp+len, "%d/%d", qosCar2.CarVlanid, qosCar2.CarVlanShaping);
				}
				else{
					len += sprintf(str_tmp+len, ",%d/%d", qosCar2.CarVlanid, qosCar2.CarVlanShaping);
				}
			}
				break;
			case 3:
			{
				QOSCAR_IP_T qosCar3;
				char str_ip_s[20] = {0}, str_ip_e[20] = {0};

				memset(&qosCar3, '\0', sizeof(QOSCAR_IP_T));
				
				*((char *)&qosCar3) = (char)index;
		        if (!apmib_get(mib_tbl, (void *)&qosCar3)){
					printf("[%s:%d]get MIB_TBL%d item%dfail!\n", __FUNCTION__, __LINE__, mib_tbl, index);
					continue;
		        }

				snprintf(str_ip_s, sizeof(str_ip_s), "%s", inet_ntoa(*((struct in_addr *)qosCar3.CarIpStart)));
				snprintf(str_ip_e, sizeof(str_ip_e), "%s", inet_ntoa(*((struct in_addr *)qosCar3.CarIpEnd)));
				if(len == 0){
					len += sprintf(str_tmp+len, "%s-%s/%d", str_ip_s, str_ip_e, qosCar3.CarIpShaping);
				}
				else{
					len += sprintf(str_tmp+len, ",%s-%s/%d", str_ip_s, str_ip_e, qosCar3.CarIpShaping);
				}
			}
				break;
			default:
				printf("[%s:%d] mode%d invalid!\n", __FUNCTION__, __LINE__, mode);
				return 0;
		}
	}

	//printf("[%s:%d] str_tmp: %s!\n", __FUNCTION__, __LINE__, str_tmp);
	if(direction == OUT_BOUND){
		switch(mode){
			case 1:
				snprintf(qosCar.IfaceLimit_UP, MAX_CAR_MODE_LEN, "%s", str_tmp);	// flush
				break;
			case 2:
				snprintf(qosCar.VlanTagLimit_UP, MAX_CAR_MODE_LEN, "%s", str_tmp);	// flush
				break;
			case 3:
				snprintf(qosCar.IPLimit_UP, MAX_CAR_MODE_LEN, "%s", str_tmp);	// flush
				break;
			default:
				break;;
		}
	}
	else{
		switch(mode){
			case 1:
				snprintf(qosCar.IfaceLimit_DOWN, MAX_CAR_MODE_LEN, "%s", str_tmp);	// flush
				break;
			case 2:
				snprintf(qosCar.VlanTagLimit_DOWN, MAX_CAR_MODE_LEN, "%s", str_tmp);	// flush
				break;
			case 3:
				snprintf(qosCar.IPLimit_DOWN, MAX_CAR_MODE_LEN, "%s", str_tmp);	// flush
				break;
			default:
				break;
		}
	}

	memcpy(&qosCarArray[1], &qosCar, sizeof(QOSCAR_T));
	if(!apmib_set(MIB_QOS_CAR_MOD, (void *)&qosCarArray)){
		printf("[%s:%d] mod QOS_CAR_TBL error!\n", __FUNCTION__, __LINE__);
		return 0;
    }
	
	return 1;
}

int check_qos_mode(unsigned int *up_mode, unsigned int *down_mode)
{
	int carNum = 0, index = 0;
	QOSCAR_T qosCar;
	
	if(!apmib_get(MIB_QOS_CAR_TBL_NUM, (void *)&carNum)){
		printf("[%s:%d]get MIB_QOS_CAR_TBL_NUM error!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	for(index = 1; index <= carNum; index++){
		memset(&qosCar, '\0', sizeof(QOSCAR_T));
		*((char *)&qosCar) = (char)index;

		if((apmib_get(MIB_QOS_CAR_TBL, (void *)&qosCar)) == 0){
			printf("[%s:%d]get MIB_QOS_CAR_TBL error!\n", __FUNCTION__, __LINE__);
			return -1;
		}

		*up_mode = qosCar.SpeedLimitMode_UP;
		*down_mode = qosCar.SpeedLimitMode_DOWN;
		break;
	}

	return 0;
}

int addQosCar(int carType, int direction, char* proper1, char* proper2, unsigned int rate)
{
	int number = 0, index = 0, val = 0;
	int mib_num = 0, mib_tbl = 0, mib_add = 0;
	
#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
	int tmp_number = 0, tmp_mode = 0, tmp_mib = 0, up_mode = 0, down_mode = 0;

	if(check_qos_mode(&up_mode, &down_mode) == -1){
		printf("[%s:%d] check_qos_mode fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}
	
	if(direction == OUT_BOUND){
		tmp_mode = down_mode;

		switch(tmp_mode){
		case 1:
			tmp_mib = MIB_QOS_CAR_DOWN_IFACE_TBL_NUM;
			break;
		case 2:
			tmp_mib = MIB_QOS_CAR_DOWN_VLAN_TBL_NUM;
			break;
		case 3:
			tmp_mib = MIB_QOS_CAR_DOWN_IP_TBL_NUM;
			break;
		default:
			break;
		}
	}
	else if(direction == IN_BOUND){
		tmp_mode = up_mode;

		switch(tmp_mode){
		case 1:
			tmp_mib = MIB_QOS_CAR_UP_IFACE_TBL_NUM;
			break;
		case 2:
			tmp_mib = MIB_QOS_CAR_UP_VLAN_TBL_NUM;
			break;
		case 3:
			tmp_mib = MIB_QOS_CAR_UP_IP_TBL_NUM;
			break;
		default:
			break;
		}
	}

	if(tmp_mib){
		if(!apmib_get(tmp_mib, (void *)&tmp_number)){
			printf("[%s:%d]get MIB %d error!\n", __FUNCTION__, __LINE__, tmp_mib);
			return 0;
		}
	}

	//printf("[%s:%d] tmp_mode: %d, tmp_number: %d\n", __FUNCTION__, __LINE__, tmp_mode, tmp_number);
#endif

	switch(carType){
		case 1:			// iface
		{
			QOSCAR_IFACE_T car_iface, car_iface_new;
			memset(&car_iface, 0, sizeof(QOSCAR_IFACE_T));

			if(direction == OUT_BOUND){
				mib_num = MIB_QOS_CAR_UP_IFACE_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_IFACE_TBL;
				mib_add = MIB_QOS_CAR_UP_IFACE_ADD;
			}
			else if(direction == IN_BOUND){
				mib_num = MIB_QOS_CAR_DOWN_IFACE_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_IFACE_TBL;
				mib_add = MIB_QOS_CAR_DOWN_IFACE_ADD;
			}
			else{
				printf("[%s:%d] direction%d invalid!\n", __FUNCTION__, __LINE__, direction);
				return 0;
			}
			
			if(!apmib_get(mib_num, (void *)&number)){
				printf("[%s:%d]get MIB_TBL_NUM %d fail!\n", __FUNCTION__, __LINE__, mib_num);
				return 0;
			}

		#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)	
			/*	83xx p0 and p4 share meter 0-7. 
				Thus uplink+downlink support 8 rules at most.	
				83xx p2 and p6 share meter 16-23. And meter 16 and 17 are reserved for p6.
				Thus downlink support 6 rules at most.	*/
			if((direction == IN_BOUND && number >= 6) || (number + tmp_number >= MAX_QOS_QUEUE_NUM))
		#else
			if(number >= MAX_QOS_QUEUE_NUM)
		#endif
			{
				return -1;	// tbl full
			}

			for (index = 1; index <= number; index++){
		        *((char *)&car_iface) = (char)index;
			
		        if (!apmib_get(mib_tbl, (void *)&car_iface)){
					printf("[%s:%d]get MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_tbl);
					return 0;
		        }

		        if (car_iface.CarIfaceInstanceNum > val){
					val = car_iface.CarIfaceInstanceNum;
		        }
		    }

			memset(&car_iface_new, 0, sizeof(QOSCAR_IFACE_T));
			car_iface_new.CarIfaceInstanceNum = val+1;
			car_iface_new.CarIfaceCarId = 1;	// temporarily use 1st car tbl entry only
			snprintf(car_iface_new.CarIfaceName, MAX_IFACE_NAME_LEN, "%s", proper1);
			car_iface_new.CarIfaceShaping = rate;

			if(!apmib_set(mib_add, (void *)&car_iface_new)){
				printf("[%s:%d]add MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_add);
				return 0;
			}

			updateCarTbl(direction, 1);
		}
			break;
		
		case 2:			// vlan
		{
			QOSCAR_VLAN_T car_vlan, car_vlan_new;
			memset(&car_vlan, 0, sizeof(QOSCAR_VLAN_T));

			if(direction == OUT_BOUND){
				mib_num = MIB_QOS_CAR_UP_VLAN_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_VLAN_TBL;
				mib_add = MIB_QOS_CAR_UP_VLAN_ADD;
			}
			else if(direction == IN_BOUND){
				mib_num = MIB_QOS_CAR_DOWN_VLAN_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_VLAN_TBL;
				mib_add = MIB_QOS_CAR_DOWN_VLAN_ADD;
			}
			else{
				printf("[%s:%d] direction%d invalid!\n", __FUNCTION__, __LINE__, direction);
				return 0;
			}
			
			if(!apmib_get(mib_num, (void *)&number)){
				printf("[%s:%d]get MIB_TBL_NUM %d fail!\n", __FUNCTION__, __LINE__, mib_num);
				return 0;
			}

		#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)	
			/*	83xx p0 and p4 share meter 0-7. 
				Thus uplink+downlink support 8 rules at most.	
				83xx p2 and p6 share meter 16-23. And meter 16 and 17 are reserved for p6.
				Thus downlink support 6 rules at most.	*/
			if((direction == IN_BOUND && number >= 6) || (number + tmp_number >= MAX_QOS_QUEUE_NUM))
		#else
			if(number >= MAX_QOS_QUEUE_NUM)
		#endif
			{	
				return -1;	// tbl full
			}
			
			for (index = 1; index <= number; index++){
		        *((char *)&car_vlan) = (char)index;
			
		        if (!apmib_get(mib_tbl, (void *)&car_vlan)){
					printf("[%s:%d]get MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_tbl);
					return 0;
		        }

		        if (car_vlan.CarVlanInstanceNum > val){
					val = car_vlan.CarVlanInstanceNum;
		        }
		    }

			memset(&car_vlan_new, 0, sizeof(QOSCAR_VLAN_T));
			car_vlan_new.CarVlanInstanceNum = val+1;
			car_vlan_new.CarVlanCarId = 1;	// temporarily use 1st car tbl entry only

			if(proper1 != NULL)
				car_vlan_new.CarVlanid = atoi(proper1);
			car_vlan_new.CarVlanShaping = rate;
			
			if(!apmib_set(mib_add, (void *)&car_vlan_new)){
				printf("[%s:%d]add MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_add);
				return 0;
			}
			updateCarTbl(direction, 2);
		}
			break;
		
		case 3:			// ip range
		{
			QOSCAR_IP_T car_ip, car_ip_new;
			memset(&car_ip, 0, sizeof(QOSCAR_IFACE_T));

			if(direction == OUT_BOUND){
				mib_num = MIB_QOS_CAR_UP_IP_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_IP_TBL;
				mib_add = MIB_QOS_CAR_UP_IP_ADD;
			}
			else if(direction == IN_BOUND){
				mib_num = MIB_QOS_CAR_DOWN_IP_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_IP_TBL;
				mib_add = MIB_QOS_CAR_DOWN_IP_ADD;
			}
			else{
				printf("[%s:%d] direction%d invalid!\n", __FUNCTION__, __LINE__, direction);
				return 0;
			}
			
			if(!apmib_get(mib_num, (void *)&number)){
				printf("[%s:%d]get MIB_TBL_NUM %d fail!\n", __FUNCTION__, __LINE__, mib_num);
				return 0;
			}

		#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)	
			/*	83xx p0 and p4 share meter 0-7. 
				Thus uplink+downlink support 8 rules at most.	
				83xx p2 and p6 share meter 16-23. And meter 16 and 17 are reserved for p6.
				Thus downlink support 6 rules at most.	*/
			if((direction == IN_BOUND && number >= 6) || (number + tmp_number >= MAX_QOS_QUEUE_NUM))
		#else
			if(number >= MAX_QOS_QUEUE_NUM)
		#endif
			{	
				return -1;		// tbl full
			}

			for (index = 1; index <= number; index++){
		        *((char *)&car_ip) = (char)index;
			
		        if (!apmib_get(mib_tbl, (void *)&car_ip)){
					printf("[%s:%d]get MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_tbl);
					return 0;
		        }

		        if (car_ip.CarIpInstanceNum > val){
					val = car_ip.CarIpInstanceNum;
		        }
		    }

			memset(&car_ip_new, 0, sizeof(QOSCAR_IP_T));
			car_ip_new.CarIpInstanceNum = val+1;
			car_ip_new.CarIpCarId = 1;	// temporarily use 1st car tbl entry only

			if(proper1 != NULL && proper2 != NULL){
				inet_aton(proper1, (struct in_addr *)&car_ip_new.CarIpStart);
				inet_aton(proper2, (struct in_addr *)&car_ip_new.CarIpEnd);
			}
			car_ip_new.CarIpShaping = rate;
			
			if(!apmib_set(mib_add, (void *)&car_ip_new)){
				printf("[%s:%d]add MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_add);
				return 0;
			}
			updateCarTbl(direction, 3);
		}
		
			break;

	#if defined(CONFIG_MAC_SHAPING_QOS)
		case 4:			// mac
		{
			QOSCAR_MAC_T car_mac, car_mac_new;
			memset(&car_mac, 0, sizeof(QOSCAR_MAC_T));

			if(direction == OUT_BOUND){
				mib_num = MIB_QOS_CAR_UP_MAC_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_MAC_TBL;
				mib_add = MIB_QOS_CAR_UP_MAC_ADD;
			}
			else if(direction == IN_BOUND){
				mib_num = MIB_QOS_CAR_DOWN_MAC_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_MAC_TBL;
				mib_add = MIB_QOS_CAR_DOWN_MAC_ADD;
			}
			else{
				printf("[%s:%d] direction%d invalid!\n", __FUNCTION__, __LINE__, direction);
				return 0;
			}
			
			if(!apmib_get(mib_num, (void *)&number)){
				printf("[%s:%d]get MIB_TBL_NUM %d fail!\n", __FUNCTION__, __LINE__, mib_num);
				return 0;
			}

			for (index = 1; index <= number; index++){
		        *((char *)&car_mac) = (char)index;
			
		        if (!apmib_get(mib_tbl, (void *)&car_mac)){
					printf("[%s:%d]get MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_tbl);
					return 0;
		        }

		        if (car_mac.CarMacInstanceNum > val){
					val = car_mac.CarMacInstanceNum;
		        }
		    }

			memset(&car_mac_new, 0, sizeof(QOSCAR_MAC_T));
			car_mac_new.CarMacInstanceNum = val+1;
			car_mac_new.CarMacCarId = 1;	// temporarily use 1st car tbl entry only

			if(proper1 != NULL){
				unsigned char *buf;
				unsigned char str_mac[13] = {0};
				unsigned int i = 0;

				buf = proper1;
				
				if(strlen(buf) == 17){
					i = 0;
					while(*buf){
						if(*buf != ':'){
							str_mac[i++] = *buf;
						}
						++buf;
					}
					str_mac[i] = '\0';
					if(!string_to_hex(str_mac, car_mac_new.CarMacAddr, 12)){
						printf("[%s:%d] Mac Addr Invalid!\n", __FUNCTION__, __LINE__);
						return 0;
					}	
				}
			}

			car_mac_new.CarMacShaping = rate;
			
			if(!apmib_set(mib_add, (void *)&car_mac_new)){
				printf("[%s:%d]add MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_add);
				return 0;
			}
			updateCarTbl(direction, 4);
		}
		
			break;
	#endif
	
		default:
			break;
	}

	return 1;
}

int delQosCar(int carType, int direction, unsigned int id)
{
	int number = 0, index = 0;
	int mib_num = 0, mib_tbl = 0, mib_del = 0;
	
	if(id == 0)
		return 0;
	
	switch(carType){
		case 1: 		// iface
		{
			QOSCAR_IFACE_T car_iface;
			memset(&car_iface, 0, sizeof(QOSCAR_IFACE_T));
			
			if(direction == OUT_BOUND){
				mib_num = MIB_QOS_CAR_UP_IFACE_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_IFACE_TBL;
				mib_del = MIB_QOS_CAR_UP_IFACE_DEL;
			}
			else if(direction == IN_BOUND){
				mib_num = MIB_QOS_CAR_DOWN_IFACE_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_IFACE_TBL;
				mib_del = MIB_QOS_CAR_DOWN_IFACE_DEL;
			}
			else{
				printf("[%s:%d] direction%d invalid!\n", __FUNCTION__, __LINE__, direction);
				return 0;
			}
			
			if(!apmib_get(mib_num, (void *)&number)){
				printf("[%s:%d]get MIB_TBL_NUM %d fail!\n", __FUNCTION__, __LINE__, mib_num);
				return 0;
			}

			for (index = 1; index <= number; index++){
		        *((char *)&car_iface) = (char)index;
			
		        if (!apmib_get(mib_tbl, (void *)&car_iface)){
					printf("[%s:%d]get MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_tbl);
					return 0;
		        }

		        if (car_iface.CarIfaceInstanceNum == id){
					if (!apmib_set(mib_del, (void *)&car_iface)) {
						printf("[%s:%d]del MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_del);
						return 0;
					}
					break;
		        }
		    }
		}
			break;
		
		case 2: 		// vlan
		{
			QOSCAR_VLAN_T car_vlan;
			memset(&car_vlan, 0, sizeof(QOSCAR_VLAN_T));

			if(direction == OUT_BOUND){
				mib_num = MIB_QOS_CAR_UP_VLAN_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_VLAN_TBL;
				mib_del = MIB_QOS_CAR_UP_VLAN_DEL;
			}
			else if(direction == IN_BOUND){
				mib_num = MIB_QOS_CAR_DOWN_VLAN_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_VLAN_TBL;
				mib_del = MIB_QOS_CAR_DOWN_VLAN_DEL;
			}
			else{
				printf("[%s:%d] direction%d invalid!\n", __FUNCTION__, __LINE__, direction);
				return 0;
			}

			if(!apmib_get(mib_num, (void *)&number)){
				printf("[%s:%d]get MIB_TBL_NUM %d fail!\n", __FUNCTION__, __LINE__, mib_num);
				return 0;
			}

			
			for (index = 1; index <= number; index++){
				*((char *)&car_vlan) = (char)index;
			
				if (!apmib_get(mib_tbl, (void *)&car_vlan)){
					printf("[%s:%d]get MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_tbl);
					return 0;
				}

				if (car_vlan.CarVlanInstanceNum == id){
					if (!apmib_set(mib_del, (void *)&car_vlan)) {
						printf("[%s:%d]del MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_del);
						return 0;
					}
					break;
				}
			}
		}
			break;
		
		case 3: 		// ip range
		{
			QOSCAR_IP_T car_ip;
			memset(&car_ip, 0, sizeof(QOSCAR_IP_T));

			if(direction == OUT_BOUND){
				mib_num = MIB_QOS_CAR_UP_IP_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_IP_TBL;
				mib_del = MIB_QOS_CAR_UP_IP_DEL;
			}
			else if(direction == IN_BOUND){
				mib_num = MIB_QOS_CAR_DOWN_IP_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_IP_TBL;
				mib_del = MIB_QOS_CAR_DOWN_IP_DEL;
			}
			else{
				printf("[%s:%d] direction%d invalid!\n", __FUNCTION__, __LINE__, direction);
				return 0;
			}
			
			if(!apmib_get(mib_num, (void *)&number)){
				printf("[%s:%d]get MIB_TBL_NUM %d fail!\n", __FUNCTION__, __LINE__, mib_num);
				return 0;
			}

			for (index = 1; index <= number; index++){
		        *((char *)&car_ip) = (char)index;
			
		        if (!apmib_get(mib_tbl, (void *)&car_ip)){
					printf("[%s:%d]get MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_tbl);
					return 0;
		        }

		        if (car_ip.CarIpInstanceNum == id){
					if (!apmib_set(mib_del, (void *)&car_ip)) {
						printf("[%s:%d]del MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_del);
						return 0;
					}
					break;
		        }
		    }
		}
			break;
	
		#if defined(CONFIG_MAC_SHAPING_QOS)
		case 4: 		// mac
		{
			QOSCAR_MAC_T car_mac;
			memset(&car_mac, 0, sizeof(QOSCAR_MAC_T));

			if(direction == OUT_BOUND){
				mib_num = MIB_QOS_CAR_UP_MAC_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_UP_MAC_TBL;
				mib_del = MIB_QOS_CAR_UP_MAC_DEL;
			}
			else if(direction == IN_BOUND){
				mib_num = MIB_QOS_CAR_DOWN_MAC_TBL_NUM;
				mib_tbl = MIB_QOS_CAR_DOWN_MAC_TBL;
				mib_del = MIB_QOS_CAR_DOWN_MAC_DEL;
			}
			else{
				printf("[%s:%d] direction%d invalid!\n", __FUNCTION__, __LINE__, direction);
				return 0;
			}

			if(!apmib_get(mib_num, (void *)&number)){
				printf("[%s:%d]get MIB_TBL_NUM %d fail!\n", __FUNCTION__, __LINE__, mib_num);
				return 0;
			}

			
			for (index = 1; index <= number; index++){
				*((char *)&car_mac) = (char)index;
			
				if (!apmib_get(mib_tbl, (void *)&car_mac)){
					printf("[%s:%d]get MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_tbl);
					return 0;
				}

				if (car_mac.CarMacInstanceNum == id){
					if (!apmib_set(mib_del, (void *)&car_mac)) {
						printf("[%s:%d]del MIB_TBL %d fail!\n", __FUNCTION__, __LINE__, mib_del);
						return 0;
					}
					break;
				}
			}
		}
			break;
	#endif
		
		default:
			break;
	}

	updateCarId(carType, direction);
	updateCarTbl(direction, carType);
	return 1;
}

int updateClassId(void)
{
	int val = 0;

	if(!apmib_set(MIB_CURRENT_CLASSID, (void *)&val)){
       	printf("set MIB_CURRENT_CLASSID error!\n");
		return 0;
    }
	
	return 1;
}


int updateQosRule(int class_id)
{
	QOSRULE_T rule_entry;
	QOSCLASS_T class_entry;
	QOSTYPE_T type_entry;
	int number = 0, index = 1;
	int wan_port;
	char str_null[MAC_ADDR_LEN] = {0};

	if(!apmib_get(MIB_QOS_RULE_TBL_NUM, (void *)&number)){
		printf("[%s:%d]get MIB_QOS_RULE_TBL_NUM fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}
	
	for(index = 1; index <= number; index++){
		memset(&rule_entry, '\0', sizeof(QOSRULE_T));
		*((char *)&rule_entry) = (char)index;

		if((apmib_get(MIB_QOS_RULE_TBL, (void *)&rule_entry)) == 0){
			printf("[%s:%d]get MIB_QOS_RULE_TBL fail!\n", __FUNCTION__, __LINE__);
			return 0;
		}

		if(rule_entry.RuleInstanceNum == class_id){
			apmib_set(MIB_QOS_RULE_DEL, (void *)&rule_entry);
		}
	}

	if((apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&number)) == 0){
		printf("[%s:%d]get MIB_QOS_CLASS_TBL_NUM fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	for(index = 1; index <= number; index++){
		memset(&class_entry, '\0', sizeof(QOSCLASS_T));
		*((char *)&class_entry) = (char)index;

		if((apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry)) == 0){
			printf("[%s:%d]get MIB_QOS_CLASS_TBL fail!\n", __FUNCTION__, __LINE__);
			return 0;
		}

		if(class_entry.ClassInstanceNum == class_id){
			memset(&rule_entry, '\0', sizeof(QOSRULE_T));

#if defined(CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT)
			rule_entry.RuleWanPort = class_entry.ClassWanPort;
#endif
			rule_entry.RuleInstanceNum = class_entry.ClassInstanceNum;
			rule_entry.DSCPMark = class_entry.DSCPMark;
			rule_entry.Dot1pMark = class_entry.Dot1pMark;
			rule_entry.RuleQueue = class_entry.ClassQueue;

			// init
			rule_entry.SourcePort = -1;
			rule_entry.SourcePortRangeMax = -1;
			rule_entry.DestPort = -1;
			rule_entry.DestPortRangeMax = -1;
			rule_entry.DscpStart = -1;
			rule_entry.DscpEnd = -1;
			rule_entry.TosStart = -1;
			rule_entry.TosEnd = -1;
			rule_entry.Dot1pStart = -1;
			rule_entry.Dot1pEnd = -1;

			break;
		}
	}
	
//find class,update type to rule
	if(index <= number){
		if((apmib_get(MIB_QOS_TYPE_TBL_NUM, (void *)&number)) == 0){
			printf("[%s:%d]get MIB_QOS_TYPE_TBL_NUM fail!\n", __FUNCTION__, __LINE__);
			return 0;
		}

		for(index = 1; index <= number; index++){
			memset(&type_entry, '\0', sizeof(QOSTYPE_T));
			*((char *)&type_entry) = (char)index;

			if((apmib_get(MIB_QOS_TYPE_TBL, (void *)&type_entry)) == 0){
				printf("[%s:%d]get MIB_QOS_TYPE_TBL fail!\n", __FUNCTION__, __LINE__);
				return 0;
			}

			if(type_entry.TypeClassId != class_id){
				continue;
			}

			if(!strncmp(type_entry.TypeName, QOS_SMAC, strlen(QOS_SMAC)) || !strncmp(type_entry.TypeName, QOS_DMAC, strlen(QOS_DMAC))){
				unsigned char *buf1, *buf2;
				unsigned char str_mac_s[13] = {0}, str_mac_e[13] = {0};
				unsigned int i = 0;

				buf1 = type_entry.TypeMin;
				buf2 = type_entry.TypeMax;
				
				if(strlen(buf1) == 17){
					i = 0;
					while(*buf1){
						if(*buf1 != ':'){
							str_mac_s[i++] = *buf1;
						}
						++buf1;
					}
					str_mac_s[i] = '\0';

					if(!strncmp(type_entry.TypeName, QOS_SMAC, strlen(QOS_SMAC))){
						if(!string_to_hex(str_mac_s, rule_entry.SourceMacStart, 12)){
							printf("[%s:%d] Type %d Smac Min Input Fail!\n", __FUNCTION__, __LINE__, index);
							continue;
						}	
					}
					else{
						if(!string_to_hex(str_mac_s, rule_entry.DestMacStart, 12)){
							printf("[%s:%d] Type %d Dmac Min Input Fail!\n", __FUNCTION__, __LINE__, index);
							continue;
						}
					}
				}

				if(strlen(buf2) == 17){
					i = 0;
					while(*buf2){
						if(*buf2 != ':'){
							str_mac_e[i++] = *buf2;
						}
						++buf2;
					}
					str_mac_e[i] = '\0';

					if(!strncmp(type_entry.TypeName, QOS_SMAC, strlen(QOS_SMAC))){
						if(!string_to_hex(str_mac_e, rule_entry.SourceMacEnd, 12)){
							printf("[%s:%d] Type %d Smac Max Input Fail!\n", __FUNCTION__, __LINE__, index);
							continue;
						}	
					}
					else{
						if(!string_to_hex(str_mac_e, rule_entry.DestMacEnd, 12)){
							printf("[%s:%d] Type %d Dmac Max Input Fail!\n", __FUNCTION__, __LINE__, index);
							continue;
						}	
					}
				}
			}
			else if(!strncmp(type_entry.TypeName, QOS_SIP, strlen(QOS_SIP))){
				if(strlen(type_entry.TypeMin) && !inet_aton(type_entry.TypeMin, (struct in_addr *)&rule_entry.SourceIpStart)){
					printf("[%s:%d] Type %d Source Ip Min Input Fail!\n", __FUNCTION__, __LINE__, index);
					continue;
				}

				
				if(strlen(type_entry.TypeMax) && !inet_aton(type_entry.TypeMax, (struct in_addr *)&rule_entry.SourceIpEnd)){
					printf("[%s:%d] Type %d Source Ip Max Input Fail!\n", __FUNCTION__, __LINE__, index);
					continue;
				}
			}
			else if(!strncmp(type_entry.TypeName, QOS_DIP, strlen(QOS_DIP))){
				if(strlen(type_entry.TypeMin) && !inet_aton(type_entry.TypeMin, (struct in_addr *)&rule_entry.DestIpStart)){
					printf("[%s:%d] Type %d Dest Ip Min Input Fail!\n", __FUNCTION__, __LINE__, index);
					continue;
				}

				if(strlen(type_entry.TypeMax) && !inet_aton(type_entry.TypeMax, (struct in_addr *)&rule_entry.DestIpEnd)){
					printf("[%s:%d] Type %d Dest Ip Max Input Fail!\n", __FUNCTION__, __LINE__, index);
					continue;
				}
			}
			else if(!strncmp(type_entry.TypeName, QOS_SPORT, strlen(QOS_SPORT))){
				rule_entry.SourcePort = atoi(type_entry.TypeMin);
				rule_entry.SourcePortRangeMax = atoi(type_entry.TypeMax);
			}
			else if(!strncmp(type_entry.TypeName, QOS_DPORT, strlen(QOS_DPORT))){
				rule_entry.DestPort = atoi(type_entry.TypeMin);
				rule_entry.DestPortRangeMax = atoi(type_entry.TypeMax);
			}
			else if(!strncmp(type_entry.TypeName, QOS_DSCP, strlen(QOS_DSCP))){
				rule_entry.DscpStart = atoi(type_entry.TypeMin);
				rule_entry.DscpEnd = atoi(type_entry.TypeMax);
			}
			else if(!strncmp(type_entry.TypeName, QOS_TOS, strlen(QOS_TOS))){
				rule_entry.TosStart = atoi(type_entry.TypeMin);
				rule_entry.TosEnd = atoi(type_entry.TypeMax);
			}
			else if(!strncmp(type_entry.TypeName, QOS_DOT1P, strlen(QOS_DOT1P))){
				rule_entry.Dot1pStart = atoi(type_entry.TypeMin);
				rule_entry.Dot1pEnd = atoi(type_entry.TypeMax);
			}
		#if 0
			else if(!strncmp(type_entry.TypeName, QOS_WAN, strlen(QOS_WAN))){
				snprintf(rule_entry.WanInterfaceStart, MAX_IFACE_NAME_LEN, "%s", type_entry.TypeMin);
				snprintf(rule_entry.WanInterfaceEnd, MAX_IFACE_NAME_LEN, "%s", type_entry.TypeMax);
			}
			else if(!strncmp(type_entry.TypeName, QOS_LAN, strlen(QOS_LAN))){
				snprintf(rule_entry.LanInterfaceStart, MAX_IFACE_NAME_LEN, "%s", type_entry.TypeMin);
				snprintf(rule_entry.LanInterfaceEnd, MAX_IFACE_NAME_LEN, "%s", type_entry.TypeMax);
			}
		#endif

			if(strlen(type_entry.Protocol) != 0){
				snprintf(rule_entry.Protocol, MAX_PROTOCOL_LEN, "%s", type_entry.Protocol);
			}
		}

		if(!apmib_set(MIB_QOS_RULE_ADD, (void *)&rule_entry)){
			printf("[%s:%d]set MIB_QOS_RULE_ADD fail!\n", __FUNCTION__, __LINE__);
			return 0;
		}
		
		return 1;
	}

	return 0;
}

int addQosClass(void)
{
	QOSCLASS_T class_entry;
	int number = 0, index = 0, val = 0;
	int wan_port = 0;
	
	memset(&class_entry, 0, sizeof(QOSCLASS_T));
	if(!apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&number)){
		printf("[%s:%d]get MIB_QOS_CLASS_TBL_NUM fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	if(number == MAX_QOS_CLASS_NUM){
		return -1;
	}
	
    for (index = 1; index <= number; index++){
        *((char *)&class_entry) = (char)index;
	
        if (!apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry)){
			printf("[%s:%d]get MIB_QOS_CLASS_TBL fail!\n", __FUNCTION__, __LINE__);
			return 0;
        }

        if (class_entry.ClassInstanceNum > val){
			val = class_entry.ClassInstanceNum;
        }
    }

	memset(&class_entry, 0, sizeof(QOSCLASS_T));
	class_entry.ClassInstanceNum = val+1;
	
	if(!apmib_set(MIB_QOS_CLASS_ADD, (void *)&class_entry)){
		printf("[%s:%d]set MIB_QOS_CLASS_ADD fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	updateQosRule(val+1);
	return 1;
}

int delQosClass(unsigned int class_id)
{
	QOSCLASS_T class_entry;
	int number = 0, index = 0;
	int wan_port;
	
	if(class_id < 1)
		return 0;

	if(!apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&number)){
		printf("[%s:%d]get MIB_QOS_CLASS_TBL_NUM fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	for(index = 1; index <= number; index++){
		memset(&class_entry, '\0', sizeof(QOSCLASS_T));
		*((char *)&class_entry) = (char)index;

		if((apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry)) == 0){
			printf("[%s:%d]get MIB_QOS_CLASS_TBL fail!\n", __FUNCTION__, __LINE__);
			return 0;
		}

		if(class_entry.ClassInstanceNum == class_id){
			if (!apmib_set(MIB_QOS_CLASS_DEL, (void *)&class_entry)) {
				printf("[%s:%d]Delete MIB_QOS_CLASS_DEL error!", __FUNCTION__, __LINE__);
				return 0;
			}

			updateClassId();
			//printf("[%s:%d] class_id: %d\n", __FUNCTION__, __LINE__, class_id);
			updateQosRule(class_id);
			return 1;
		}
	}

	return 0;
}

int updateTypeId(unsigned int classId)
{
	int val = 0;

	if(!apmib_set(MIB_CURRENT_TYPEID, (void *)&val)){
       	printf("set MIB_CURRENT_TYPEID error!\n");
		return 0;
    }

	return 1;
}


int addQosTypeByClassId(unsigned int class_id)
{
	QOSTYPE_T type_entry;
	int number = 0, index = 0, val = 0, count = 0;
	int wan_port;
		
	memset(&type_entry, 0, sizeof(QOSTYPE_T));
	if(!apmib_get(MIB_QOS_TYPE_TBL_NUM, (void *)&number)){
		printf("[%s:%d]get MIB_QOS_TYPE_TBL_NUM fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}
	
	for (index = 1; index <= number; index++){
		*((char *)&type_entry) = (char)index;
	
		if (!apmib_get(MIB_QOS_TYPE_TBL, (void *)&type_entry)){
			printf("[%s:%d]get MIB_QOS_TYPE_TBL fail!\n", __FUNCTION__, __LINE__);
			return 0;
		}
		
		if(type_entry.TypeClassId != class_id)
			continue;

		if (type_entry.TypeInstanceNum > val){
			val = type_entry.TypeInstanceNum;
		}
		
		count++;
	}
	
	if(count == MAX_QOS_PER_CLASS_TYPE_NUM){
		return -1;
	}
		
	val += 1;
	if(!apmib_set(MIB_CURRENT_TYPEID, (void *)&val)){
		printf("[%s:%d]set MIB_CURRENT_TYPEID error!\n", __FUNCTION__, __LINE__);
		return 0;
    }
	
	memset(&type_entry, 0, sizeof(QOSTYPE_T));

	type_entry.TypeInstanceNum = val;
	type_entry.TypeClassId = class_id;

	if(!apmib_set(MIB_QOS_TYPE_ADD, (void *)&type_entry)){
		printf("[%s:%d]set MIB_QOS_TYPE_ADD fail!\n", __FUNCTION__, __LINE__);
		return 0;
	} 

	//printf("[%s:%d] class_id: %d\n", __FUNCTION__, __LINE__, class_id);
	updateQosRule(class_id);
	return 1;
}

int delQosTypeByClassId(unsigned int class_id)
{
	QOSTYPE_T type_entry;
	int number = 0, index, start = 1;
	int wan_port;
	
	if(class_id < 1)
		return 0;

	if(!apmib_get(MIB_QOS_TYPE_TBL_NUM, (void *)&number)){
		printf("[%s:%d]get MIB_QOS_TYPE_TBL_NUM fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	while(1){
		for(index = start; index <= number; index++){
			memset(&type_entry, '\0', sizeof(QOSTYPE_T));
			*((char *)&type_entry) = (char)index;

			if((apmib_get(MIB_QOS_TYPE_TBL, (void *)&type_entry)) == 0){
				printf("[%s:%d]get MIB_QOS_TYPE_TBL fail!\n", __FUNCTION__, __LINE__);
				return 0;
			}
			
			if(type_entry.TypeClassId != class_id)
				continue;
			
			if (!apmib_set(MIB_QOS_TYPE_DEL, (void *)&type_entry)) {
				printf("[%s:%d]Delete MIB_QOS_TYPE_DEL error!", __FUNCTION__, __LINE__);
				return 0;
			}

			start = index;
			if(!apmib_get(MIB_QOS_TYPE_TBL_NUM, (void *)&number)){
				printf("[%s:%d]get MIB_QOS_TYPE_TBL_NUM fail!\n", __FUNCTION__, __LINE__);
				return 0;
			}
			break;
		}

		if(index > number){
			break;
		}
	}

	//printf("[%s:%d] class_id: %d, number: %d\n", __FUNCTION__, __LINE__, class_id, number);
	updateQosRule(class_id);
	return 1;
}

int delQosType(unsigned int class_id, unsigned int type_id)
{
	QOSTYPE_T type_entry;
	int number = 0, index = 0;
	int wan_port;
	
	if(type_id < 1)
		return 0;

	if(!apmib_get(MIB_QOS_TYPE_TBL_NUM, (void *)&number)){
		printf("[%s:%d]get MIB_QOS_TYPE_TBL_NUM fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}
	
	for(index = 1; index <= number; index++){
		memset(&type_entry, '\0', sizeof(QOSTYPE_T));
		*((char *)&type_entry) = (char)index;

		if((apmib_get(MIB_QOS_TYPE_TBL, (void *)&type_entry)) == 0){
			printf("[%s:%d]get MIB_QOS_TYPE_TBL fail!\n", __FUNCTION__, __LINE__);
			return 0;
		}
		
		if(type_entry.TypeClassId == class_id && type_entry.TypeInstanceNum == type_id)
		{
			if (!apmib_set(MIB_QOS_TYPE_DEL, (void *)&type_entry)) {
				printf("[%s:%d]Delete MIB_QOS_TYPE_DEL error!", __FUNCTION__, __LINE__);
				return 0;
			}

			updateTypeId(class_id);
			//printf("[%s:%d] class_id: %d\n", __FUNCTION__, __LINE__, type_entry.TypeClassId);
			updateQosRule(class_id);
			return 1;
		}
	}

	return 0;
}

int protoCheck(unsigned int class_id, unsigned char* proto)
{
	QOSTYPE_T type_entry;
	unsigned int number = 0, index = 0, count = 0;
	int wan_port;
	
	if(proto == NULL || strlen(proto) == 0){
		printf("[%s:%d] Protocol Invalid!", __FUNCTION__, __LINE__);
		return 0;
	}

	if(!apmib_get(MIB_QOS_TYPE_TBL_NUM, (void *)&number)){
		printf("[%s:%d]get MIB_QOS_TYPE_TBL_NUM fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}
	
	for(index = 1; index <= number; index++){
		memset(&type_entry, '\0', sizeof(QOSTYPE_T));
		*((char *)&type_entry) = (char)index;

		if((apmib_get(MIB_QOS_TYPE_TBL, (void *)&type_entry)) == 0){
			printf("[%s:%d]get MIB_QOS_TYPE_TBL fail!\n", __FUNCTION__, __LINE__);
			return 0;
		}
		
		if(type_entry.TypeClassId != class_id)
			continue;

		if(strlen(type_entry.Protocol) && strncmp(type_entry.Protocol, proto, type_entry.Protocol)){
			count++;
		}
	}

	if(count >= 1){
		printf("[%s:%d] Multiple type entries must share the same protocol!\n", __FUNCTION__, __LINE__);
		return 0;
	}
	
	return 1;
}

int dscpCheck(unsigned int class_id, unsigned int type)
{
	QOSTYPE_T type_entry;
	unsigned int number = 0, index = 0, count = 0;
	unsigned int wan_port;

	if(type < 1 || type > 2){		// 1: dscp	2: tos
		return 0;
	}

	if(!apmib_get(MIB_QOS_TYPE_TBL_NUM, (void *)&number)){
		printf("[%s:%d]get MIB_QOS_TYPE_TBL_NUM fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}
	
	for(index = 1; index <= number; index++){
		memset(&type_entry, '\0', sizeof(QOSTYPE_T));
		*((char *)&type_entry) = (char)index;

		if((apmib_get(MIB_QOS_TYPE_TBL, (void *)&type_entry)) == 0){
			printf("[%s:%d]get MIB_QOS_TYPE_TBL fail!\n", __FUNCTION__, __LINE__);
			return 0;
		}
		
		if(type_entry.TypeClassId != class_id)
			continue;

		if((!strncmp(type_entry.TypeName, QOS_DSCP, strlen(QOS_DSCP)) && type==2) || 
		(!strncmp(type_entry.TypeName, QOS_TOS, strlen(QOS_TOS)) && type==1)){
			count++;
		}
	}

	if(count >= 1){
		printf("[%s:%d] A class entry can only have either dscp or tos type entry!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	return 1;
}

//check if different type has different type name
int typeValidCheck(unsigned int class_id, unsigned int type_id, unsigned char* type)
{
	QOSTYPE_T type_entry;
	unsigned int number = 0, index = 0;
	int wan_port;
	
	if(type == NULL || strlen(type) == 0){		
		return 0;
	}

	if(!apmib_get(MIB_QOS_TYPE_TBL_NUM, (void *)&number)){
		printf("[%s:%d]get MIB_QOS_TYPE_TBL_NUM fail!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	for(index = 1; index <= number; index++){
		memset(&type_entry, '\0', sizeof(QOSTYPE_T));
		*((char *)&type_entry) = (char)index;

		if((apmib_get(MIB_QOS_TYPE_TBL, (void *)&type_entry)) == 0){
			printf("[%s:%d]get MIB_QOS_TYPE_TBL fail!\n", __FUNCTION__, __LINE__);
			return 0;
		}
		
		if(type_entry.TypeClassId == class_id && type_entry.TypeInstanceNum != type_id)
		{
			if(!strncmp(type_entry.TypeName, type, strlen(type))){
				printf("[%s:%d] A class entry can only have one %s type entry!\n", __FUNCTION__, __LINE__, type_entry.TypeName);
				return 0;
			}
		}
	}
	
	return 1;
}

int qosModeValidity(unsigned char *mode)
{
	if(mode == NULL)
		return 0;
	
	if(strncmp(mode, "TR069", 5) && strncmp(mode, "VOIP", 4) && strncmp(mode, "IPTV", 4) 
	&& strncmp(mode, "INTERNET", 8))
		return 0;

	return 1;
}

#if defined(CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT)
int empty_qosApp(unsigned int wan_index)
{
	unsigned int num, index, start=1;
	QOSAPP_T qosApp;

	//printf("enter empty_qosApp!\n");
	if((apmib_get(MIB_QOS_APP_TBL_NUM,(void *)&num)) == 0){
		printf("get MIB_QOS_APP_TBL_NUM fail!\n");
		return 0;
	}

	while(1){
		for(index = start; index <= num; index++){		
			memset(&qosApp, '\0', sizeof(QOSAPP_T));
			*((char *)&qosApp) = (char)index;
		
			if((apmib_get(MIB_QOS_APP_TBL, (void *)&qosApp)) == 0){
				printf("get MIB_QOS_APP_TBL fail!\n");
			}

#if defined(CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT)
			if(qosApp.AppWanPort != wan_index)
				continue;
#endif

		
			if((apmib_set(MIB_QOS_APP_DEL, (void *)&qosApp)) == 0){
				printf("MIB_QOS_APP_DEL fail!\n");
				return 0;
			}

			start = index;
			
			if((apmib_get(MIB_QOS_APP_TBL_NUM,(void *)&num)) == 0){
				printf("get MIB_QOS_APP_TBL_NUM fail!\n");
				return 0;
			}
			break;	
		}

		if(index > num){
			break;
		}
	}

	return 1;
}
#endif

int parseQosMode(unsigned char *mode)
{
	unsigned int count = 0, index = 0;
	unsigned char buf[64] = {0}, modeArry[4][16] = {0};
	char *strptr, *cmdptr;
	QOSAPP_T qosApp;

	if(mode == NULL){
		printf("[%s:%d] Mode NULL!\n", __FUNCTION__, __LINE__);
		return 0;
	}
	snprintf(buf, sizeof(buf), "%s", mode);
	strptr = buf;
	
	// TR069, VOIP, IPTV, INTERNET
	for(count = 0; count < 4; count++){
		cmdptr = strsep(&strptr, ",");
		if(qosModeValidity(cmdptr) == 0){
			printf("[%s:%d] Mode Invalid!\n", __FUNCTION__, __LINE__);
			return 0;
		}

		snprintf(modeArry[count], 16, "%s", cmdptr);
		if(strptr == NULL){	// the end
			count++;
			break;	
		}
	}

	snprintf(buf, sizeof(buf), "%s", mode);
	
#if defined(CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT)
	unsigned int wan_index;

	if((apmib_get(MIB_QOS_WAN_INDEX, (void *)&wan_index)) == 0){
		printf("get MIB_QOS_WAN_INDEX fail!\n");
		return 0;
	}

	empty_qosApp(wan_index);

	//printf("[%s:%d] wan_port %d! count : %d\n", __FUNCTION__, __LINE__,wan_port,count);

	for(index = 0; index <count ; index++){	
		memset(&qosApp, '\0', sizeof(QOSAPP_T));

		qosApp.AppWanPort = wan_index;
		qosApp.AppInstanceNum == index + 1;
		qosApp.AppQueue = index + 1;
		snprintf(qosApp.AppName, sizeof(qosApp.AppName), "%s", modeArry[index]);

		apmib_set(MIB_QOS_APP_DEL, (void *)&qosApp);

		printf("[%s:%d] qosApp.AppInstanceNum %d!\n", __FUNCTION__, __LINE__,qosApp.AppInstanceNum);
		
		if(apmib_set(MIB_QOS_APP_ADD, (void *)&qosApp) == 0) {
			printf("[%s:%d] MIB_QOS_APP_ADD Error!\n", __FUNCTION__, __LINE__);
			return 0;
		}	
	}
#else
	if (!apmib_set(MIB_QOS_MODE, (void *)buf)) {
		printf("[%s:%d] set MIB_QOS_MODE Error!\n", __FUNCTION__, __LINE__);
		return 0;
	}	
	
	memset(&qosApp, '\0', sizeof(QOSAPP_T));
	if (!apmib_set(MIB_QOS_APP_DELALL, (void *)&qosApp)) {
		printf("[%s:%d] MIB_QOS_APP_DELALL Error!\n", __FUNCTION__, __LINE__);
		return 0;
	}	
	
	// update QOS_APP_TBL
	for(index = 0; index < count ; index++){
		memset(&qosApp, '\0', sizeof(QOSAPP_T));

		qosApp.AppInstanceNum = index + 1;
		qosApp.AppQueue = index + 1;	// decreasing priority
		snprintf(qosApp.AppName, sizeof(qosApp.AppName), "%s", modeArry[index]);

		apmib_set(MIB_QOS_APP_DEL, (void *)&qosApp);
		if(apmib_set(MIB_QOS_APP_ADD, (void *)&qosApp) == 0) {
			printf("[%s:%d] MIB_QOS_APP_ADD Error!\n", __FUNCTION__, __LINE__);
			return 0;
		}
	}
	
#endif
	
	return 1;
}

int updateQosMode(void)
{
	unsigned char modeArry[MAX_QOS_QUEUE_NUM+1][16] = {0}, buf[64] = {0};
	unsigned int index = 1, num = 0, len = 0;
	QOSAPP_T qosApp;

	if(apmib_get(MIB_QOS_APP_TBL_NUM, (void *)&num) == 0) {
		printf("[%s:%d] get MIB_QOS_APP_TBL_NUM Error!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	for(index = 1; index <= num; index++){
		memset(&qosApp, '\0', sizeof(QOSAPP_T));
		*((char *)&qosApp) = (char)index;
					
		if (!apmib_get(MIB_QOS_APP_TBL, (void *)&qosApp))
			continue;

		snprintf(modeArry[qosApp.AppQueue], 16, "%s", qosApp.AppName);
	}

	for(index = 1; index <= MAX_QOS_QUEUE_NUM; index++){
		if(strlen(modeArry[index])){
			if(len == 0){
				len += sprintf(buf+len, "%s", modeArry[index]);
			}
			else{
				len += sprintf(buf+len, ",%s", modeArry[index]);
			}
		}
	}
#if defined(CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT)
#else
	if (!apmib_set(MIB_QOS_MODE, (void *)buf)) {
		printf("[%s:%d] set MIB_QOS_MODE Error!\n", __FUNCTION__, __LINE__);
		return 0;
	}
#endif

	return 1;
}

#elif (defined(MULTI_WAN_QOS) || defined(QOS_OF_TR069))
/* when set class table , need to maintian classificastion order */
int add_classifyMIB(QOSCLASS_Tp entry)
{
	int entryNum=0, index = 0, orderTop = 0;
	QOSCLASS_T class_entry, class_array[2];
	
	if(entry != NULL){
		apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum);
	
		if(entry->ClassificationOrder){
			for(index = 1; index <= entryNum; index++){
				memset(&class_entry, '\0', sizeof(class_entry));
				*((char *)&class_entry) = (char)index;
					
				apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry);
					
				if(class_entry.ClassificationOrder == entry->ClassificationOrder)
					break;
			}
			
			if(index <= entryNum){
				/* if entry->ClassificationOrder matches that of an existing entry, 
				   the Order value for the existing entry and all lower Order entries is incremented (lowered in precedence) 
				   to ensure uniqueness of this value.  */
				for (index=1; index<=entryNum; index++){
					memset(&class_entry, '\0', sizeof(class_entry));
					*((char *)&class_entry) = (char)index;
				
					apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry);
					if(class_entry.ClassificationOrder >= entry->ClassificationOrder){
						memset(&class_array[0], 0, sizeof(QOSCLASS_T));
						memset(&class_array[1], 0, sizeof(QOSCLASS_T));
						memcpy(&class_array[0], &class_entry, sizeof(QOSCLASS_T));
					
						class_entry.ClassificationOrder +=1;
						memcpy(&class_array[1], &class_entry, sizeof(QOSCLASS_T));
					
						if (apmib_set(MIB_QOS_CLASS_MOD, (void *)&class_array) == 0) {
							printf("MOD QOS_CLASS error!");
							goto setErr;
						}	
					}
				}
			}
		}
		else{
			for(index = 1; index <= entryNum; index++){
				memset(&class_entry, '\0', sizeof(class_entry));
				*((char *)&class_entry) = (char)index;
				
				apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry);
				
				if(class_entry.ClassificationOrder > orderTop)
					orderTop = class_entry.ClassificationOrder;
			}
		
			entry->ClassificationOrder = orderTop + 1;
		}
	
		if (apmib_set(MIB_QOS_CLASS_ADD, (void *)entry) == 0) {
			printf("Add table entry error!");
			goto setErr;
		}
		return 1;
	}
	
setErr:
	return 0;
}

/* A deletion causes Order values to be compacted. */
int del_classifyMIB(QOSCLASS_Tp entry)
{
	int entryNum=0, index = 0, orderTop = 0;
	QOSCLASS_T class_entry, class_array[2];
	
	if(entry != NULL){
		apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum);
	
		if(entry->ClassificationOrder){
			for(index = 1; index <= entryNum; index++){
				memset(&class_entry, '\0', sizeof(class_entry));
				*((char *)&class_entry) = (char)index;
					
				apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry);
					
				if(class_entry.ClassificationOrder > orderTop)
					orderTop = class_entry.ClassificationOrder;
			}
			
			if(entry->ClassificationOrder < orderTop){
				for (index=1; index<=entryNum; index++) {
					memset(&class_entry, '\0', sizeof(class_entry));
					*((char *)&class_entry) = (char)index;
					apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry);

					if(entry->ClassInstanceNum == class_entry.ClassInstanceNum)
						continue;
				
					if(class_entry.ClassificationOrder > entry->ClassificationOrder){
						memset(&class_array[0], 0, sizeof(QOSCLASS_T));
						memset(&class_array[1], 0, sizeof(QOSCLASS_T));
						memcpy(&class_array[0], &class_entry, sizeof(QOSCLASS_T));
				
						class_entry.ClassificationOrder -=1;
						memcpy(&class_array[1], &class_entry, sizeof(QOSCLASS_T));
					
						if ( apmib_set(MIB_QOS_CLASS_MOD, (void *)&class_array) == 0){
							printf("MOD QOS_CLASS error!");
							goto setErr;
						}
					}	
				}
			}
		}

		if (apmib_set(MIB_QOS_CLASS_DEL, (void *)entry) == 0){
			printf("del table entry error!");
			goto setErr;
		}
	
		return 1;
	}
	
setErr:
	return 0;
}

/*  When ClassificationOrder is changed, incrementing occurs before compaction. */
int mod_classifyMIB(QOSCLASS_Tp entry,int order)
{
	int entryNum=0, index = 0;
	QOSCLASS_T class_entry, class_array[2];

	if(entry != NULL){
		apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum);

		for (index=1; index<=entryNum; index++){
			memset(&class_entry, '\0', sizeof(class_entry));
			*((char *)&class_entry) = (char)index;
			apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry);

			if(class_entry.ClassificationOrder == order){
				break;	
			}
		}

		/* modified ClassificationOrder doesn't match any existing entry */
		if(index == (entryNum + 1)){
			goto compact;
		}

		// incrementing 
		for (index=1; index<=entryNum; index++){
			memset(&class_entry, '\0', sizeof(class_entry));
			*((char *)&class_entry) = (char)index;
			apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry);

			if(class_entry.ClassInstanceNum != entry->ClassInstanceNum){
				if(class_entry.ClassificationOrder >= order){
					memset(&class_array[0], 0, sizeof(QOSCLASS_T));
					memset(&class_array[1], 0, sizeof(QOSCLASS_T));
					memcpy(&class_array[0], &class_entry, sizeof(QOSCLASS_T));
	
					class_entry.ClassificationOrder += 1;
					memcpy(&class_array[1], &class_entry, sizeof(QOSCLASS_T));

					if ( apmib_set(MIB_QOS_CLASS_MOD, (void *)&class_array) == 0) {
						printf("MOD QOS_CLASS error!");
						goto setErr;
					}
				}
			}
		}

compact:
		for (index=1; index<=entryNum; index++){
			memset(&class_entry, '\0', sizeof(class_entry));
			*((char *)&class_entry) = (char)index;
			apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry);

			if(class_entry.ClassInstanceNum != entry->ClassInstanceNum){
				if(class_entry.ClassificationOrder > entry->ClassificationOrder){
					/* If small -> big, compact between (small, big), if big -> small, compact (big, infinite)*/
					if(entry->ClassificationOrder >= order || class_entry.ClassificationOrder < order){
						memset(&class_array[0], 0, sizeof(QOSCLASS_T));
						memset(&class_array[1], 0, sizeof(QOSCLASS_T));
						memcpy(&class_array[0], &class_entry, sizeof(QOSCLASS_T));
					
						class_entry.ClassificationOrder -= 1;
						memcpy(&class_array[1], &class_entry, sizeof(QOSCLASS_T));

						if(apmib_set(MIB_QOS_CLASS_MOD, (void *)&class_array) == 0) {
							printf("MOD QOS_CLASS error!");
							goto setErr;
						}
					}
				}
			}
		}

		memcpy(&class_array[0], entry, sizeof(QOSCLASS_T));
		entry->ClassificationOrder = order;
		memcpy(&class_array[1], entry, sizeof(QOSCLASS_T));
	
		if (apmib_set(MIB_QOS_CLASS_MOD, (void *)&class_array) == 0) {
			printf("MOD QOS_CLASS error!");
			goto setErr;
		}
	
		return 1;
	}
	
setErr:
	return 0;
}
#endif
int isFileExist(char *file_name)
{
	struct stat status;

	if ( stat(file_name, &status) < 0)
		return 0;

	return 1;
}

#ifdef CUSTOMIZE_SSID
static void convert_bin_to_str(unsigned char *bin, int len, char *out)
{
	int i;
	char tmpbuf[10];

	out[0] = '\0';

	for (i=0; i<len; i++) {
		sprintf(tmpbuf, "%02x", bin[i]);
		strcat(out, tmpbuf);
	}
}

#ifdef CUSTOMIZE_SSID_AP_RPT
int cmcc_is_unwanted_char(unsigned char ch)
{
	int ret = 0;
	if(ch=='0' || ch=='o' || ch=='O' || ch=='B' || ch=='8' || ch=='1' || ch=='I' || ch=='l')
	{
		//printf("unwant %c\n", ch);
		ret = 1;	
	}
	return ret;
}

void get_random_ssid_str(unsigned char s[],int number)
{
	unsigned char str[64] = "00123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	int cnt = 0;;
	char ss[2];
	unsigned char ch;
	int idx = 0;
	
	for(;;){
		idx = rand()%62+1;		
		ch = str[idx];			
		if(cmcc_is_unwanted_char(ch))
			continue;			
		cnt++;
		sprintf(ss, "%c", ch);		
		strcat(s,ss);	
		if(cnt==number)
			break;
	}	
	return;
}

int is_ssid_same(unsigned char *ssid)
{
	int ret = 0;
	int i = 0, j=0;
	unsigned char ssidtmp[MAX_SSID_LEN] = {0};

	for(i=0; i<NUM_WLAN_INTERFACE; ++i){
		wlan_idx = i;
		for(j=0; j<(NUM_VWLAN+1); ++j){
			vwlan_idx = j;
			apmib_get(MIB_WLAN_SSID, (void *)ssidtmp);
			//printf("[%s %d]wlan_idx:%d, vwlan_idx:%d, ssidtmp:%s, target ssid:%s\n", __FUNCTION__,__LINE__,
			//	wlan_idx, vwlan_idx, ssidtmp, ssid);
			if(strcmp(ssidtmp, ssid)==0){
				ret = 1;
				break;
			}				
		}
		if(ret)
			break;
	}	
	return ret;
}

int set_ssid_customize_ap_repeater(int update)
{
	unsigned char ssid[MAX_SSID_LEN] = {0};  
	unsigned char ssidtmp[CUSTOMIZE_SSID_RANDOM_CHAR_NUM] = {0};
	unsigned int phyBandSelect = 0, is_guest = 0, pos=0;
	int i, j, len = 0;
	unsigned int cnt = 0;
	int wlanidx_bak = 0;

	apmib_save_wlanIdx();
	
	srand((unsigned int)time((time_t *)NULL)); // important		
	for(j=0; j<NUM_WLAN_INTERFACE;++j)
	{
		cnt = 0;
		wlan_idx = j;	
		apmib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyBandSelect);
		
		if( phyBandSelect==PHYBAND_5G){
			len = strlen(CUSTOMIZE_SSID_PREFIX_AP_5G);
			memcpy(ssid, CUSTOMIZE_SSID_PREFIX_AP_5G, len);			
		}else if( phyBandSelect==PHYBAND_2G){
			len = strlen(CUSTOMIZE_SSID_PREFIX_AP_2G);
			memcpy(ssid, CUSTOMIZE_SSID_PREFIX_AP_2G, len);			
		}
		
		for(;;)
		{
			memset(ssidtmp, 0, CUSTOMIZE_SSID_RANDOM_CHAR_NUM);
			get_random_ssid_str(ssidtmp, CUSTOMIZE_SSID_RANDOM_CHAR_NUM);
			pos = len;			
			memcpy(ssid+pos, ssidtmp, CUSTOMIZE_SSID_RANDOM_CHAR_NUM);	
			pos += CUSTOMIZE_SSID_RANDOM_CHAR_NUM;			
			ssid[pos] = '\0';			

		#if 1
			/* if same ssid, then re-generate ssid */	
			wlanidx_bak = wlan_idx;			
			
			if(is_ssid_same(ssid)){
				wlan_idx = wlanidx_bak;
				continue;
			}

			wlan_idx = wlanidx_bak;
			vwlan_idx = cnt;			
			apmib_set(MIB_WLAN_SSID, (void *)&ssid);
			//printf("wlan_idx:%d, vwlan_idx:%d, ssid:%s\n", wlan_idx, vwlan_idx, ssid);
		#endif
		
			cnt++;
			if(cnt == (NUM_VWLAN+1)) // root+vap
				break;
		}				
	}

	apmib_recov_wlanIdx();

	if(update)
		apmib_update(CURRENT_SETTING);
	return 0;
}
#endif

#ifdef CUSTOMIZE_SSID_GW
int set_ssid_customize_gw(int update)
{
	unsigned char ssid[MAX_SSID_LEN] = {0};   
	unsigned char mac[MAC_ADDR_LEN] = {0};
	unsigned char macstr[MAC_ADDR_LEN*2+1] = {0}, macstrtmp[MAC_ADDR_LEN+1] = {0};
	int i = 0, j=0, len=0;	
	unsigned int phyBandSelect = 0, pos=0;
	
#ifdef CUSTOMIZE_SSID_GUEST
	unsigned int is_guest = 0;
#endif	

	apmib_save_wlanIdx();
	
	for(j=0; j<NUM_WLAN_INTERFACE;++j)
	{
		wlan_idx = j;	
		apmib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyBandSelect);
		if( phyBandSelect==PHYBAND_5G ){
			len = strlen(CUSTOMIZE_SSID_PREFIX_GW_5G);
			memcpy(ssid, CUSTOMIZE_SSID_PREFIX_GW_5G, len);			
		}else if( phyBandSelect==PHYBAND_2G){
			len = strlen(CUSTOMIZE_SSID_PREFIX_GW_2G);
			memcpy(ssid, CUSTOMIZE_SSID_PREFIX_GW_2G, len);			
		}	
		
		for(i=0; i<NUM_VWLAN_INTERFACE;++i)
		{
			vwlan_idx = i;			
			switch (vwlan_idx)
			{
				case 0:
					apmib_get(MIB_HW_WLAN_ADDR, (void *)mac);
					break;
				case 1:
					apmib_get(MIB_HW_WLAN_ADDR1, (void *)mac);
					break;
				case 2:
					apmib_get(MIB_HW_WLAN_ADDR2, (void *)mac);
					break;
				case 3:
					apmib_get(MIB_HW_WLAN_ADDR3, (void *)mac);
					break;
				case 4:
					apmib_get(MIB_HW_WLAN_ADDR4, (void *)mac);
					break;
				default:
					printf("Fail to get MAC address of VAP%d!\n", vwlan_idx-1);				
					return 0;
			}
			convert_bin_to_str(mac, MAC_ADDR_LEN, macstr);
			pos = len;			
			//memcpy(ssid+pos, macstr+6, 6);	
			//pos += 6;	
			memcpy(ssid+pos, macstr+8, 4);	
			pos += 4;	
			
#ifdef CUSTOMIZE_SSID_GUEST			
			apmib_get(MIB_WLAN_ACCESS, (void *)&is_guest);
			if(is_guest){									
				memcpy(ssid+pos, CUSTOMIZE_SSID_GUEST_SUFFIX, strlen(CUSTOMIZE_SSID_GUEST_SUFFIX));		
				pos += strlen(CUSTOMIZE_SSID_GUEST_SUFFIX);				
			}	
#endif			
			ssid[pos] = '\0';			
			
			//printf("[%s %d]ssid:%s\n", __FUNCTION__,__LINE__,ssid);
			apmib_set(MIB_WLAN_SSID, (void *)&ssid);
		}		
	}

	apmib_recov_wlanIdx();

	if(update)
		apmib_update(CURRENT_SETTING);
	return 0;	
}
#endif
#if 0
int isFileExist(char *file_name)
{
	struct stat status;

	if ( stat(file_name, &status) < 0)
		return 0;

	return 1;
}
#endif
int set_ssid_customize(int update)
{
	printf("SSID Customized !\n");			

#ifdef HOME_GATEWAY
#ifdef CUSTOMIZE_SSID_GW
	set_ssid_customize_gw(update);
#endif
#else
#ifdef CUSTOMIZE_SSID_AP_RPT
	set_ssid_customize_ap_repeater(update);
#endif
#endif

	return 0;
}
#endif

#if defined(CONFIG_MULTIPLE_WAN_VLAN_BINDING)
int check_vlan_pair(unsigned char *binding_vlan, unsigned char *err_buff, int buff_size)
{
	unsigned char *tokptr1=NULL, *strptr1 = NULL, *tokptr2=NULL, *strptr2 = NULL;
	int lan_vlan = 0, wan_vlan = 0, count = 0, i = 0;
	int entry_num = 0, wan_index = 0, flag = 0;
	VLANBINDING_T entry,  tmp_entry;
	WANIFACE_T	WanIfaceEntry;
	
	if (!binding_vlan || !err_buff || !buff_size)
		return -1;
		
	strptr1 = binding_vlan;
	//printf("%s %d strptr1=%s \n", __func__, __LINE__, strptr1);
	for (tokptr1 = strsep(&strptr1,VLANBINDING_PAIR_DELIMITER); tokptr1 != NULL; 
		 tokptr1 = strsep(&strptr1,VLANBINDING_PAIR_DELIMITER)){

		 if (!strstr(tokptr1, VLANBINDING_PAIR_MEMBER_DELIMITER)){
			 snprintf(err_buff, buff_size, "format error! configure as follows: \"m1/n1,m2/n2...\", among them, \"m\" means lan side vlan, \"n\" means wan side vlan.");
			return -1;
		}
		 
		lan_vlan = wan_vlan = -1;
		sscanf(tokptr1, "%d/%d", &lan_vlan, &wan_vlan);
		//printf("%s %d lan_vlan=%d wan_vlan=%d tokptr1=%s  strlen(tokptr1)=%d strptr1=%s\n", __func__, __LINE__, lan_vlan,wan_vlan, tokptr1,  strlen(tokptr1),strptr1);
		if (lan_vlan <= 0 || lan_vlan > 4095 || wan_vlan <= 0 || wan_vlan > 4095){
			snprintf(err_buff, buff_size, "VID invalid!, should between 1-4095!");
			return -1;
		}
		
		flag = 0;
		for(wan_index = 1; wan_index <= WANIFACE_NUM; ++wan_index)
		{
			memset(&WanIfaceEntry,0,sizeof(WANIFACE_T));
			getWanIfaceEntry(wan_index,&WanIfaceEntry);
			if(WanIfaceEntry.enable)
			{	
				#if 0
				if (WanIfaceEntry.AddressType == BRIDGE){
					if (WanIfaceEntry.vlan && (wan_vlan == WanIfaceEntry.vlanid)
						|| !WanIfaceEntry.vlan && (wan_vlan == RTL_WANVLANID)){
							if (lan_vlan != wan_vlan){
								snprintf(err_buff, buff_size, "binding lan vlan group to bridge wan, lan/wan vlan should be same, untag wan please use vid 8!");
								return -1;
							}
						}
				}
				#endif
				
				if (WanIfaceEntry.vlan && (wan_vlan == WanIfaceEntry.vlanid)
						|| !WanIfaceEntry.vlan && (wan_vlan == RTL_WANVLANID)){
					/*if ((WanIfaceEntry.AddressType == BRIDGE) && (lan_vlan != wan_vlan)){
						snprintf(err_buff, buff_size, "binding lan vlan group to bridge wan, lan/wan vlan should be same, untag wan please use vid 8!");
						return -1;
					}*/
					
					flag = 1;
				}
				

			}
			
		}
		if (!flag){

			snprintf(err_buff, buff_size, "binding lan vlan group to non-existence wan side vlan, please configure wan first, untag wan please use vid 8!");
			return -1;
		}
		
		count++;
		
	}
		
	if (count > WANIFACE_NUM){
		snprintf(err_buff, buff_size, "the count of vlan pair more should less than or equal to %d!", WANIFACE_NUM);
		return -1;
	}
	
	if (!count){
		snprintf(err_buff, buff_size, "format error! configure as follows: \"m1/n1,m2/n2,...\", among them, \"m\" means lan side vlan, \"n\" means wan side vlan.");
		return -1;
	}	

	return 0;
}
#endif

