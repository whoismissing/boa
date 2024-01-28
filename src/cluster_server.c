#include <sys/types.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <linux/if_ether.h>
#include <net/ethernet.h>
#include <linux/if.h>
#include <netinet/ip.h>  
#include <linux/sockios.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <signal.h> 
#include <sys/param.h> 
#include <sys/stat.h> 
#include <sys/un.h> 
#include <signal.h>
#include <sys/time.h>


#include "cluster_common.h"
#include "apmib.h"
#include "utility.h"

#define IS_MASKED(mask, idx) 			((mask[idx/8]&(0x1 << (idx&0x7)))!=0x0)
#define MASK_IDX(mask, idx) 			(mask[idx/8] |= (0x1 << (idx&0x7)))
/* 
 * 1. max age starting from every valid cluster packet
 * 		updated at receving an incoming valid cluster packet.
 * 		reset receive buffer when max age reached.
 * 2. unit: second
 */
#define CLUSTER_SERVER_RECV_MAX_AGE		(60)
#define CLUSTER_SERVER_RECV_PKT_TIMEOUT	(5)


unsigned char local_interface_mac_address[ETH_ALEN];
extern int wlan_idx;	// interface index
extern APMIB_Tp pMib, pMibDef;

struct wps_config_info_struct {
	int wlan_mode;
	int auth;
	int shared_type;
	int wep_enc;
	int wpa_enc;
	int wpa2_enc;
	unsigned char ssid[MAX_SSID_LEN];
	int KeyId;
	unsigned char wep64Key1[WEP64_KEY_LEN];
	unsigned char wep64Key2[WEP64_KEY_LEN];
	unsigned char wep64Key3[WEP64_KEY_LEN];
	unsigned char wep64Key4[WEP64_KEY_LEN];
	unsigned char wep128Key1[WEP128_KEY_LEN];
	unsigned char wep128Key2[WEP128_KEY_LEN];
	unsigned char wep128Key3[WEP128_KEY_LEN];
	unsigned char wep128Key4[WEP128_KEY_LEN];
	unsigned char wpaPSK[MAX_PSK_LEN+1];
};

typedef struct _cluster_receive_info
{
    unsigned char  ready;   //all fragment received or not
    unsigned short frag_id; //last received fragment id
    unsigned short offset;  //offset in buffer
    unsigned char *buf;     //payload buffer
}CLU_RCV_INFO_T, *CLU_RCV_INFO_Tp;

static CLU_RCV_INFO_T cluster_receive_info_list[NUM_WLAN_INTERFACE][NUM_VWLAN+2];
static int cluster_net_sock_server;
static unsigned short cur_seq;
static int totalcnt;     	//implies we received the last packet for conf
enum{
	CLU_SRV_NOT_INIT=0,
	CLU_SRV_INIT,
	CLU_SRV_RECEIVING,
	CLU_SRV_UPDATING
}CLU_SRV_STATS;
static unsigned int age;
static int clu_srv_state=CLU_SRV_NOT_INIT;

static void log_pid()
{
	FILE *f;
	pid_t pid;
	pid = getpid();

	if((f = fopen(CLU_SERVER_RUNFILE, "w")) == NULL)
		return;
	fprintf(f, "%d\n", pid);
	fclose(f);
}

static void clr_pid()
{
    unlink(CLU_SERVER_RUNFILE);
}

static int cluster_server_init(void)
{
    struct ifreq ifstruct;
    int i;
	struct timeval recv_timeout={CLUSTER_SERVER_RECV_PKT_TIMEOUT,0};
	
CreateSocket:
	cluster_net_sock_server = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(cluster_net_sock_server==-1){
		debug_print("Creat server socket error.\n");
		sleep(2);
		goto CreateSocket;
	}

	// set a receive timeout
	if(setsockopt(cluster_net_sock_server, SOL_SOCKET, SO_RCVTIMEO, (void *)&recv_timeout, sizeof(struct timeval)))
	{
		debug_print("setsockopt error.\n");
	}

    sprintf(ifstruct.ifr_ifrn.ifrn_name, CLU_CLIENT_INTERFACE);
    if(ioctl(cluster_net_sock_server, SIOCGIFHWADDR, &ifstruct)==-1)
    {
        printf("%s Get %s macaddress failed\n", __func__, ifstruct.ifr_ifrn.ifrn_name);
        return -1;
    }
    for(i=0;i<ETH_ALEN;i++)
    {
        local_interface_mac_address[i] = ifstruct.ifr_hwaddr.sa_data[i];
        debug_print("local interface mac: [%d]0x%02x\n", i, local_interface_mac_address[i]);
    }

    cur_seq = 0;
    totalcnt = 0;
	return 0;
}

static int cluster_reinit(void)
{
    cur_seq = 0;
    totalcnt = 0;
    return;
}

static int cluster_receive_info_init()
{
    int i,j;
    CLU_RCV_INFO_Tp pclu_rcv_info;
    
    for(i=0; i<NUM_WLAN_INTERFACE; i++)
        for(j=0; j<(NUM_VWLAN+2); j++)
        {
            pclu_rcv_info = &(cluster_receive_info_list[i][j]);
            pclu_rcv_info->ready = 0;
            pclu_rcv_info->frag_id = 0;
            pclu_rcv_info->offset = 0;
            pclu_rcv_info->buf = (unsigned char *)malloc(sizeof(CLU_PAYLOAD_T));
            if(pclu_rcv_info->buf==NULL)
            {
                debug_print("malloc failed\n");
                return -1;
            }else
                memset(pclu_rcv_info->buf, 0x0, sizeof(CLU_PAYLOAD_T));
        }
}

static int cluster_receive_info_reset()
{
    int i,j;
    CLU_RCV_INFO_Tp pclu_rcv_info;
    
    for(i=0; i<NUM_WLAN_INTERFACE; i++)
        for(j=0; j<(NUM_VWLAN+2); j++)
        {
            pclu_rcv_info = &(cluster_receive_info_list[i][j]);
            pclu_rcv_info->ready = 0;
            pclu_rcv_info->frag_id = 0;
            pclu_rcv_info->offset = 0;
            if(pclu_rcv_info->buf!=NULL)
                memset(pclu_rcv_info->buf, 0x0, sizeof(CLU_PAYLOAD_T));
        }
}

static int cluster_receive_info_fini()
{
    int i,j;
    CLU_RCV_INFO_Tp pclu_rcv_info;
    
    for(i=0; i<NUM_WLAN_INTERFACE; i++)
        for(j=0; j<(NUM_VWLAN+2); j++)
        {
            pclu_rcv_info = &(cluster_receive_info_list[i][j]);
            if(pclu_rcv_info->buf!=NULL)
            {
                free(pclu_rcv_info->buf);
                pclu_rcv_info->buf = NULL;
            }
        }
}

/* Signal handler */
static void sig_usr1_handler(int sig)
{
	clu_srv_state = CLU_SRV_INIT;
	debug_print("clu_srv_state change to CLU_SRV_INIT.\n");
}

static void sig_term_handler(int sig)
{
    printf("cluster_server exit.\n");
    clr_pid();
    exit(0);
}

static void sig_alarm_handler(int sig)
{
	debug_print("age %d\n", age);
	if(clu_srv_state==CLU_SRV_RECEIVING)
		age++;
	
	get_debug_level();
}

static void cluster_update_wps_configured(int reset_flag)
{
#ifdef WIFI_SIMPLE_CONFIG
	int is_configured, encrypt1, encrypt2, auth, disabled, iVal, format, shared_type;
	char ssid1[100];
	int dual_wps_icons = 0;
	unsigned char tmpbuf[MAX_MSG_BUFFER_SIZE]={0};	
	struct wps_config_info_struct wps_config_info;
	
	{
		memset(&wps_config_info, 0, sizeof(struct wps_config_info_struct));
		apmib_get(MIB_WLAN_ENCRYPT, (void *)&wps_config_info.auth);
		apmib_get(MIB_WLAN_WEP, (void *)&wps_config_info.wep_enc);
		apmib_get(MIB_WLAN_SSID, (void *)wps_config_info.ssid);	
		apmib_get(MIB_WLAN_MODE, (void *)&wps_config_info.wlan_mode);
		apmib_get(MIB_WLAN_AUTH_TYPE, (void *)&wps_config_info.shared_type);
		apmib_get(MIB_WLAN_WEP_DEFAULT_KEY, (void *)&wps_config_info.KeyId);
		apmib_get(MIB_WLAN_WEP64_KEY1, (void *)wps_config_info.wep64Key1);
		apmib_get(MIB_WLAN_WEP64_KEY2, (void *)wps_config_info.wep64Key2);
		apmib_get(MIB_WLAN_WEP64_KEY3, (void *)wps_config_info.wep64Key3);
		apmib_get(MIB_WLAN_WEP64_KEY4, (void *)wps_config_info.wep64Key4);
		apmib_get(MIB_WLAN_WEP128_KEY1, (void *)wps_config_info.wep128Key1);
		apmib_get(MIB_WLAN_WEP128_KEY2, (void *)wps_config_info.wep128Key2);
		apmib_get(MIB_WLAN_WEP128_KEY3, (void *)wps_config_info.wep128Key3);
		apmib_get(MIB_WLAN_WEP128_KEY4, (void *)wps_config_info.wep128Key4);
		apmib_get(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&wps_config_info.wpa_enc);
		apmib_get(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&wps_config_info.wpa2_enc);
		apmib_get(MIB_WLAN_WPA_PSK, (void *)wps_config_info.wpaPSK);
	}

	debug_print("\n");
	apmib_get(MIB_DUAL_WPS_ICONS,&dual_wps_icons);
	{
		apmib_get(MIB_WLAN_SSID, (void *)ssid1);
		apmib_get(MIB_WLAN_MODE, (void *)&iVal);
		if (strcmp(ssid1, (char *)wps_config_info.ssid) || (iVal != wps_config_info.wlan_mode)) {
			apmib_set(MIB_WLAN_WSC_SSID, (void *)ssid1);
		}
	}
	{
		apmib_get(MIB_WLAN_AUTH_TYPE, (void *)&shared_type);
		apmib_get(MIB_WLAN_ENCRYPT, (void *)&encrypt1);
		if (encrypt1 == ENCRYPT_WEP && 
			shared_type != wps_config_info.shared_type) {
			if (shared_type == AUTH_OPEN || shared_type == AUTH_BOTH) {
				if (wps_config_info.shared_type == AUTH_SHARED) {
					auth = WSC_AUTH_OPEN;
					apmib_set(MIB_WLAN_WSC_AUTH, (void *)&auth);
				}
			}
			else {
				if (wps_config_info.shared_type == AUTH_OPEN ||
					wps_config_info.shared_type == AUTH_BOTH) {
					auth = WSC_AUTH_SHARED;
					apmib_set(MIB_WLAN_WSC_AUTH, (void *)&auth);
				}
			}
		}
	}

	apmib_get(MIB_WLAN_ENCRYPT, (void *)&encrypt1);
	if (encrypt1 == ENCRYPT_DISABLED) {
		auth = WSC_AUTH_OPEN;
		encrypt2 = WSC_ENCRYPT_NONE;
	}
	else if (encrypt1 == ENCRYPT_WEP) {
		apmib_get(MIB_WLAN_AUTH_TYPE, (void *)&shared_type);
		if (shared_type == AUTH_OPEN || shared_type == AUTH_BOTH)
			auth = WSC_AUTH_OPEN;
		else
			auth = WSC_AUTH_SHARED;
		encrypt2 = WSC_ENCRYPT_WEP;		
	}
	else if (encrypt1 == ENCRYPT_WPA) {
		auth = WSC_AUTH_WPAPSK;
		apmib_get(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&encrypt1);
		if (encrypt1 == WPA_CIPHER_TKIP)
			encrypt2 = WSC_ENCRYPT_TKIP;		
		else if (encrypt1 == WPA_CIPHER_AES)
			encrypt2 = WSC_ENCRYPT_AES;		
		else 
			encrypt2 = WSC_ENCRYPT_TKIPAES;				
	}
	else if (encrypt1 == ENCRYPT_WPA2) {
		auth = WSC_AUTH_WPA2PSK;
		apmib_get(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&encrypt1);
		if (encrypt1 == WPA_CIPHER_TKIP)
			encrypt2 = WSC_ENCRYPT_TKIP;		
		else if (encrypt1 == WPA_CIPHER_AES)
			encrypt2 = WSC_ENCRYPT_AES;		
		else 
			encrypt2 = WSC_ENCRYPT_TKIPAES;				
	}
	else {
		auth = WSC_AUTH_WPA2PSKMIXED;
		encrypt2 = WSC_ENCRYPT_TKIPAES;			

// When mixed mode, if no WPA2-AES, try to use WPA-AES or WPA2-TKIP
		apmib_get(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&encrypt1);
		apmib_get(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&iVal);
		if (!(iVal &	WPA_CIPHER_AES)) {
			if (encrypt1 &	WPA_CIPHER_AES) {			
				//auth = WSC_AUTH_WPAPSK;
				encrypt2 = WSC_ENCRYPT_AES;	
				//printf("%s %d : %d\n",__FUNCTION__,__LINE__ ,encrypt2);
			}
			else{
				encrypt2 = WSC_ENCRYPT_TKIP;	
				//printf("%s %d : %d\n",__FUNCTION__,__LINE__ ,encrypt2);
			}
		}
//-------------------------------------------- david+2008-01-03
		if(encrypt1==WPA_CIPHER_AES && iVal ==WPA_CIPHER_AES){
			encrypt2 = WSC_ENCRYPT_AES;	
			printf("%s %d\n",__FUNCTION__,__LINE__);			
		}
		//printf("%s %d :auth=0x%02X\n",__FUNCTION__,__LINE__ ,auth);		
		// for correct wsc_auth wsc_encry value when security is mixed mode
	}
	apmib_set(MIB_WLAN_WSC_AUTH, (void *)&auth);
	apmib_set(MIB_WLAN_WSC_ENC, (void *)&encrypt2);

	apmib_get(MIB_WLAN_ENCRYPT, (void *)&encrypt1);
	if (encrypt1 == ENCRYPT_WPA || encrypt1 == ENCRYPT_WPA2 || encrypt1 == ENCRYPT_WPA2_MIXED) {
		apmib_get(MIB_WLAN_WPA_AUTH, (void *)&format);
		if (format & 2) { // PSK
			apmib_get(MIB_WLAN_WPA_PSK, (void *)tmpbuf);
			apmib_set(MIB_WLAN_WSC_PSK, (void *)tmpbuf);					
		}		
	}
	if (reset_flag) {
		reset_flag = 0;
		apmib_set(MIB_WLAN_WSC_CONFIGBYEXTREG, (void *)&reset_flag);		
	}
	
configuration_changed:	
	reset_flag = 0;
	apmib_set(MIB_WLAN_WSC_CONFIGBYEXTREG, (void *)&reset_flag);
	apmib_get(MIB_WLAN_WSC_DISABLE, (void *)&disabled);	
	apmib_get(MIB_WLAN_WSC_CONFIGURED, (void *)&is_configured);
	//if (!is_configured && !disabled) { //We do not care wsc is enable for disable--20081223
	if (!is_configured) {
		is_configured = 1;
		apmib_set(MIB_WLAN_WSC_CONFIGURED, (void *)&is_configured);
#if defined(CONFIG_RTL_92D_SUPPORT)
		if(!dual_wps_icons)
		{
			if(wlan_idx==0){
				wlan_idx = 1;
				apmib_set(MIB_WLAN_WSC_CONFIGURED, (void *)&is_configured);
				wlan_idx = 0;			
			}else if(wlan_idx == 1){
				wlan_idx = 0;
				apmib_set(MIB_WLAN_WSC_CONFIGURED, (void *)&is_configured);
				wlan_idx = 1;			
			}
		}
#endif
	}
	
#endif
	return ;
}

static void cluster_save_wlan_profile()
{
    int i,j;
	unsigned char k=0;
	MACFILTER_T acrule = {0};
	CLU_PAYLOAD_T t = {0};
	CLU_PAYLOAD_Tp p = &t;

    for(i=0;i<NUM_WLAN_INTERFACE;i++)
        for(j=0;j<(NUM_VWLAN+2);j++)
        {
            if(cluster_receive_info_list[i][j].buf!=NULL && cluster_receive_info_list[i][j].ready==1)
            {
                debug_print("update pMib->wlan[%d][%d]\n", i, j);
				memset(p, 0x0, sizeof(CLU_PAYLOAD_T));
				p = (CLU_PAYLOAD_Tp)cluster_receive_info_list[i][j].buf;
				
				wlan_idx = i;
				vwlan_idx = j;
				// ssid
                debug_print("update ssid, channel, bw\n");
				apmib_set(MIB_WLAN_SSID, (void *)p->ssid);
				
				// channel
				apmib_set(MIB_WLAN_CHANNEL, (void *)&p->channel);
				
				// bandwidth
				apmib_set(MIB_WLAN_CHANNEL_BONDING, (void *)&p->channelbonding);
				apmib_set(MIB_WLAN_COEXIST_ENABLED, (void *)&p->CoexistEnabled);
				
				// encrypt
                debug_print("update encryption\n");
				apmib_set(MIB_WLAN_AUTH_TYPE, (void *)&p->authType);
				apmib_set(MIB_WLAN_ENABLE_1X, (void *)&p->enable1X);
				apmib_set(MIB_WLAN_WEP, (void *)&p->wep);
				apmib_set(MIB_WLAN_ENCRYPT, (void *)&p->encrypt);
				apmib_set(MIB_WLAN_WEP_KEY_TYPE, (void *)&p->wepKeyType);
				apmib_set(MIB_WLAN_WEP64_KEY1, (void *)p->wep64Key1);
				apmib_set(MIB_WLAN_WEP64_KEY2, (void *)p->wep64Key2);
				apmib_set(MIB_WLAN_WEP64_KEY3, (void *)p->wep64Key3);
				apmib_set(MIB_WLAN_WEP64_KEY4, (void *)p->wep64Key4);
				apmib_set(MIB_WLAN_WEP128_KEY1, (void *)p->wep128Key1);
				apmib_set(MIB_WLAN_WEP128_KEY2, (void *)p->wep128Key2);
				apmib_set(MIB_WLAN_WEP128_KEY3, (void *)p->wep128Key3);
				apmib_set(MIB_WLAN_WEP128_KEY4, (void *)p->wep128Key4);
				apmib_set(MIB_WLAN_MAC_AUTH_ENABLED, (void *)&p->macAuthEnabled);
				apmib_set(MIB_WLAN_WPA_AUTH, (void *)&p->wpaAuth);
				apmib_set(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&p->wpaCipher);
				apmib_set(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&p->wpa2Cipher);
				apmib_set(MIB_WLAN_WPA2_PRE_AUTH, (void *)&p->wpa2PreAuth);
				apmib_set(MIB_WLAN_PSK_FORMAT, (void *)&p->wpaPSKFormat);
				apmib_set(MIB_WLAN_WPA_PSK, (void *)p->wpaPSK);
				
				// mac control
                debug_print("update ac rule, total %d\n", p->acNum);
				apmib_set(MIB_WLAN_MACAC_ENABLED, (void *)&p->acEnabled);
				apmib_set(MIB_WLAN_MACAC_NUM, (void *)&p->acNum);
				apmib_set(MIB_WLAN_AC_ADDR_DELALL, (void *)&acrule);//clear all entry!
				for(k=1; k<=p->acNum; k++)
				{
					memset(&acrule, 0x0, sizeof(acrule));
					memcpy((unsigned char *)&acrule, (unsigned char *)&p->acAddrArray[k], sizeof(MACFILTER_T));
					*((char *)&acrule) = (char)k;
					apmib_set(MIB_WLAN_AC_ADDR_ADD, (void *)&acrule);
				}

				// wsc
				cluster_update_wps_configured(1);
            }
        }
		
	printf("cluster_server: update apmib\n");	
	apmib_update(CURRENT_SETTING);
	sleep(3);

    return;
}

static void cluster_apply_wlan_profile()
{
#ifdef RTK_REINIT_SUPPORT
	unsigned char cmd_buf[32]={0};
	sprintf(cmd_buf, "reinitCli -e %d",REINIT_EVENT_APPLYCAHNGES);
	debug_print("%s\n", cmd_buf);
	system(cmd_buf);
#else
	system("init.sh gw all");
#endif
}

static void dump_recv_buffer(unsigned char *buf, unsigned int len)
{
	int i=0;
	for(; buf&&i<len; i++)
	{
		if(i!=0 && i%8==0)
			printf("\n");
		printf("%02x ", buf[i]);
	}
	printf("\n");
}

int main(int argc, char**argv)
{
    int rcvlen = 0;
    int clilen=0;
    int i,j;
    struct sockaddr_ll cliaddr;
    unsigned char recvbuffer[CLUSTER_PKT_MAX_LEN];
    CLU_HDR_Tp cluhdr;
    CLU_RCV_INFO_Tp pclu_rcv_info = NULL;
    unsigned short checksum = 0, last_checksum = 0;
    unsigned short rcv_info_buf_cell_size = CLUSTER_PKT_MAX_LEN - ETH_HLEN - sizeof(CLU_HDR_T);
    struct sigaction act;
    struct itimerval value;
	
    log_pid();
    apmib_init();
	
	// setup SIGALARM signal handler.
    act.sa_handler = sig_alarm_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0; 
    sigaction(SIGALRM, &act, 0);

    // set alarm periodic 1 sec.
    value.it_interval.tv_sec = 1; 
    value.it_interval.tv_usec = 0; 
    value.it_value.tv_sec = 1; 
    value.it_value.tv_usec = 0; 
    if (setitimer(ITIMER_REAL, &value, 0))
    {
    	debug_print("[%s:%d] setitimer failed! Exit cluster server!\n", __FUNCTION__, __LINE__);
        return -1;
    }
	
    signal(SIGUSR1, sig_usr1_handler);    //need known when configration changed
    signal(SIGTERM, sig_term_handler);    //need known when configration changed

    cluster_server_init();
    cluster_receive_info_init();

	clu_srv_state = CLU_SRV_INIT;
	debug_print("clu_srv_state change to CLU_SRV_INIT.\n");
    debug_print("sizeof(CLU_PAYLOAD_T):%d\n", sizeof(CLU_PAYLOAD_T));
    debug_print("sizeof(CLU_HDR_T):%d\n", sizeof(CLU_HDR_T));
    debug_print("rcv_info_buf_cell_size:%d\n", rcv_info_buf_cell_size);
    debug_print("cluster_receive_info_list:%p\n", cluster_receive_info_list);

    while(1)
    {
    	if( clu_srv_state==CLU_SRV_RECEIVING && age>=CLUSTER_SERVER_RECV_MAX_AGE)
		{
			/* timeout for current receving, not enough configuration packets are fully received. */
			// reset everything!
			debug_print("reset everything due to receive profile timeout!\n");
			checksum = 0;
			last_checksum = 0;
			cluster_receive_info_reset();
			age = 0;
			clu_srv_state = CLU_SRV_INIT;
			debug_print("clu_srv_state change to CLU_SRV_INIT.\n");
		}else if( clu_srv_state==CLU_SRV_UPDATING){
			continue;
		}
		
        memset(recvbuffer, 0x0, sizeof(recvbuffer));
        rcvlen = recvfrom(cluster_net_sock_server,recvbuffer,sizeof(recvbuffer),0,(struct sockaddr*)&cliaddr,&clilen);

        if(rcvlen>0)
        {
            unsigned short cal_checksum = 0;
            unsigned short totlen = 0, more_fragment = 0, frag_id = 0, sequence = 0;
            unsigned short wlan_idx = 0, vwlan_idx = 0;
            int all_received = 0;

            // filter non-broadcast
            if(!(recvbuffer[0]==0xFF && recvbuffer[1]==0xFF && recvbuffer[2]==0xFF && recvbuffer[3]==0xFF && recvbuffer[4]==0xFF && recvbuffer[5]==0xFF ))
                continue;

            // filter self sent packet
            if( recvbuffer[6]==local_interface_mac_address[0] 
              &&recvbuffer[7]==local_interface_mac_address[1] 
              &&recvbuffer[8]==local_interface_mac_address[2] 
              &&recvbuffer[9]==local_interface_mac_address[3] 
              &&recvbuffer[10]==local_interface_mac_address[4] 
              &&recvbuffer[11]==local_interface_mac_address[5] 
              )
                continue;

            // filter checksum
            //dump_recv_buffer(recvbuffer, sizeof(CLU_HDR_T)+ETH_HLEN);
            cluhdr = (CLU_HDR_Tp)(recvbuffer + ETH_HLEN);
            checksum = ntohs(cluhdr->checksum);
            sequence = ntohs(cluhdr->sequence);
            cal_checksum = get_checksum(recvbuffer+ETH_HLEN, rcvlen-ETH_HLEN);
            if(last_checksum != cal_checksum)
            {
                debug_print("Invalid packet, checksum incorrect/old configuration. packet: %x, calculated: %x, last checksum: %x, sequence: %x, cur_seq: %x\n", checksum, cal_checksum, last_checksum, sequence, cur_seq);
                continue;
            }

            // filter sequence
            //if( cur_seq!=0&&sequence<=cur_seq )
			if( cur_seq==0 || (clu_srv_state==CLU_SRV_INIT&&sequence>cur_seq) 
				|| (clu_srv_state==CLU_SRV_RECEIVING&&sequence==cur_seq)
			)
            {
                cur_seq = sequence;
                debug_print("Packet checksum: %x, local calculated: %x, last checksum: %x, will assign checksum --> last_checksum\n", checksum, cal_checksum, last_checksum);
                last_checksum = checksum;
            }else
            {
                debug_print("Invalid packet, old configuration(cur_seq:%d, sequence:%d).\n", cur_seq, sequence);
                continue;
            }

            // defragment
            //debug_print("Receive conf sync packet: %d bytes.\n", rcvlen);
            totlen = ntohs(cluhdr->totlen);
            wlan_idx = ntohs(cluhdr->wlan_idx);
            vwlan_idx = ntohs(cluhdr->vwlan_idx);
            more_fragment = ntohs(cluhdr->more_fragment);
            frag_id = ntohs(cluhdr->frag_id);
            debug_print("totlen:%x, wlan_idx:%d, vwlan_idx:%d, frag_id:%d, more_frag:%d\n", totlen, wlan_idx, vwlan_idx, frag_id, more_fragment);
            if(wlan_idx < NUM_WLAN_INTERFACE && vwlan_idx < (NUM_VWLAN+2) )
                pclu_rcv_info = &(cluster_receive_info_list[wlan_idx][vwlan_idx]);
            else
                continue;

            if(pclu_rcv_info->buf!=NULL && pclu_rcv_info->ready==0)
            {
                unsigned char *pos = pclu_rcv_info->buf + frag_id*rcv_info_buf_cell_size;
                // prevent duplicated fragment
                if(*pos==0x0)
                {
                    //debug_print("Assign to receive buffer at offset %x.\n", frag_id*rcv_info_buf_cell_size);
                    // update age and state
                    clu_srv_state = CLU_SRV_RECEIVING;
					debug_print("clu_srv_state change to CLU_SRV_RECEIVING.\n");
                    age = 0;
					// safety check
					if((frag_id*rcv_info_buf_cell_size+totlen-sizeof(CLU_HDR_T))>sizeof(CLU_PAYLOAD_T))
					{
						debug_print("received payload overflows buffer!!!frag_id:%d, rcv_info_buf_cell_size:%d, totlen:%d\n", frag_id, rcv_info_buf_cell_size, totlen);
					}
                    memcpy(pclu_rcv_info->buf + frag_id*rcv_info_buf_cell_size, recvbuffer+ETH_HLEN+sizeof(CLU_HDR_T), totlen-sizeof(CLU_HDR_T));
                    pclu_rcv_info->offset += (totlen-sizeof(CLU_HDR_T));
                    //debug_print("%p's buffer received total %x.\n", pclu_rcv_info, pclu_rcv_info->offset);
                    if(pclu_rcv_info->offset == sizeof(CLU_PAYLOAD_T))
                    {
                        pclu_rcv_info->ready = 1;
                        //debug_print("%p is ready.\n", pclu_rcv_info);
                    }
                }
            }else
                continue;

            if(wlan_idx==(NUM_WLAN_INTERFACE-1) && vwlan_idx==(NUM_VWLAN+2-1))
            {
                for(i=0; i<NUM_WLAN_INTERFACE; i++)
                    for(j=0; j<(NUM_VWLAN+2); j++)
                    {
                        debug_print("rcv_info[%d][%d] ready: %d\n", i,j, cluster_receive_info_list[i][j].ready);
                        all_received += cluster_receive_info_list[i][j].ready;
                    }

                if(all_received==(NUM_WLAN_INTERFACE*(NUM_VWLAN+2)))
                {
                	clu_srv_state = CLU_SRV_UPDATING;
					debug_print("clu_srv_state change to CLU_SRV_UPDATING.\n");
                    cluster_save_wlan_profile();
                    cluster_receive_info_reset();
                    last_checksum = 0;
					//cur_seq++;		// after this update, only receive packet with sequence larger than cur_seq
					cluster_apply_wlan_profile();
                }
            }
        }
        
        debug_print("\n");
    }

    close(cluster_net_sock_server);
    cluster_receive_info_fini();
	return 0;
}

