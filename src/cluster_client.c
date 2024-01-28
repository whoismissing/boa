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
#include <sys/time.h>

#include "cluster_common.h"
#include "apmib.h"

#define CLU_CLIENT_INTERVAL 5
#define CLU_CLIENT_CONF_SEND_REPEAT_TIMES 1

static int cluster_net_sock_client;
static struct sockaddr_ll clu_ll_addr;
static unsigned short cur_seq;
static unsigned short next_seq;
static unsigned char should_send_conf = 1;
static unsigned char broadcast_mac[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

extern APMIB_Tp pMib, pMibDef;

static void log_pid()
{
	FILE *f;
	pid_t pid;
	pid = getpid();

	if((f = fopen(CLU_CLIENT_RUNFILE, "w")) == NULL)
		return;
	fprintf(f, "%d\n", pid);
	fclose(f);
}

void handle_alarm(int sig)
{
    if(0==should_send_conf)
    {
        should_send_conf = 1;
        debug_print("send cluster conf periodic.\n");
    }
}

static int cluster_client_init(void)
{
    struct ifreq ifstruct;
	int i, lan_ifindex;
	
	cluster_net_sock_client = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	{
        sprintf(ifstruct.ifr_ifrn.ifrn_name, CLU_CLIENT_INTERFACE);
        if(ioctl(cluster_net_sock_client, SIOCGIFINDEX, &ifstruct)==-1)
		{
			printf("%s Get %s ifindex failed\n", __func__, ifstruct.ifr_ifrn.ifrn_name);
			return -1;
		}
		lan_ifindex = ifstruct.ifr_ifindex;
	}
    
    /* bind socket address */
    memset(&clu_ll_addr, 0, sizeof(struct sockaddr_ll));
    clu_ll_addr.sll_family = AF_PACKET; 
    clu_ll_addr.sll_halen = ETH_ALEN;
    if(ioctl(cluster_net_sock_client, SIOCGIFHWADDR, &ifstruct)==-1)
    {
        printf("%s Get %s macaddress failed\n", __func__, ifstruct.ifr_ifrn.ifrn_name);
        return -1;
    }
    for(i=0; i<ETH_ALEN; i++)
    {
        clu_ll_addr.sll_addr[i] = ifstruct.ifr_hwaddr.sa_data[i];
        debug_print("[%d]:0x%02x\n", i, clu_ll_addr.sll_addr[i]);
    }
    clu_ll_addr.sll_ifindex = lan_ifindex;
    clu_ll_addr.sll_protocol = htons(CLUSTER_MANAGE_PROTOCOL);
    if((bind(cluster_net_sock_client, (struct sockaddr *) &clu_ll_addr, 
                    sizeof(clu_ll_addr)))== -1)
    {
        perror("Error binding socket to interface\n");                                               
        return -1;
    }

    debug_print("init cluster client succeed\n");
	return 0;
}

/* Signal handler */
static void signal_usr1_handler(int sig)
{
	//when receive SIGUSR1, means configration changed. We need to update sequence for next configration pacekts flood.
	if(SIGUSR1 == sig)
	{
        if(next_seq == cur_seq)
        {
            next_seq++;
            should_send_conf = 1;
            debug_print("cur_seq=%d, next_seq=%d\n", cur_seq, next_seq);
        }
	}
}

static void signal_term_handler(int sig)
{
    printf("cluster_client exit.\n");
    unlink(CLU_CLIENT_RUNFILE);
    exit(0);
}

static void cluster_construct_wlan_profile(unsigned int wIdx, unsigned int vwIdx, unsigned char *buf)
{
	if(buf==NULL)
		return ;
	
	CLU_PAYLOAD_Tp p = (CLU_PAYLOAD_Tp)buf;
	unsigned char acnum = 0, i = 0;
	MACFILTER_T acrule = {0};
	wlan_idx = wIdx;
	vwlan_idx = vwIdx;
	
	// ssid
	apmib_get(MIB_WLAN_SSID, (void *)p->ssid);
	
	// channel
	apmib_get(MIB_WLAN_CHANNEL, (void *)&p->channel);
	
	// bandwidth
	apmib_get(MIB_WLAN_CHANNEL_BONDING, (void *)&p->channelbonding);
	apmib_get(MIB_WLAN_COEXIST_ENABLED, (void *)&p->CoexistEnabled);
	
	// encrypt
	apmib_get(MIB_WLAN_AUTH_TYPE, (void *)&p->authType);
	apmib_get(MIB_WLAN_ENABLE_1X, (void *)&p->enable1X);
	apmib_get(MIB_WLAN_WEP, (void *)&p->wep);
	apmib_get(MIB_WLAN_ENCRYPT, (void *)&p->encrypt);
	apmib_get(MIB_WLAN_WEP_KEY_TYPE, (void *)&p->wepKeyType);
	apmib_get(MIB_WLAN_WEP64_KEY1, (void *)p->wep64Key1);
	apmib_get(MIB_WLAN_WEP64_KEY2, (void *)p->wep64Key2);
	apmib_get(MIB_WLAN_WEP64_KEY3, (void *)p->wep64Key3);
	apmib_get(MIB_WLAN_WEP64_KEY4, (void *)p->wep64Key4);
	apmib_get(MIB_WLAN_WEP128_KEY1, (void *)p->wep128Key1);
	apmib_get(MIB_WLAN_WEP128_KEY2, (void *)p->wep128Key2);
	apmib_get(MIB_WLAN_WEP128_KEY3, (void *)p->wep128Key3);
	apmib_get(MIB_WLAN_WEP128_KEY4, (void *)p->wep128Key4);
	apmib_get(MIB_WLAN_MAC_AUTH_ENABLED, (void *)&p->macAuthEnabled);
	apmib_get(MIB_WLAN_WPA_AUTH, (void *)&p->wpaAuth);
	apmib_get(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&p->wpaCipher);
	apmib_get(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&p->wpa2Cipher);
	apmib_get(MIB_WLAN_WPA2_PRE_AUTH, (void *)&p->wpa2PreAuth);
	apmib_get(MIB_WLAN_PSK_FORMAT, (void *)&p->wpaPSKFormat);
	apmib_get(MIB_WLAN_WPA_PSK, (void *)p->wpaPSK);
	
	// mac control
	apmib_get(MIB_WLAN_MACAC_ENABLED, (void *)&p->acEnabled);
	apmib_get(MIB_WLAN_MACAC_NUM, (void *)&acnum);
	p->acNum = acnum;
	for(i=1; i<=acnum; i++)
	{
		memset(&acrule, 0x0, sizeof(acrule));
		*((char *)&acrule) = (char)i;
		apmib_get(MIB_WLAN_MACAC_ADDR, (void *)&acrule);
		memcpy((unsigned char *)&p->acAddrArray[i], (unsigned char *)&acrule, sizeof(MACFILTER_T));
		//debug_print("[%d][%d]:[%d]: offset:%d\n", wlan_idx, vwlan_idx, i, (unsigned char *)&p->acAddrArray[i]-(unsigned char *)p);
	}
	
	return ;
}

static void configrations_send(void)
{
    unsigned short index = 0, total_pkt = 0;
    unsigned short len = 0;     // frame real total length
    unsigned short checksum = 0;
    unsigned char ether_frame[CLUSTER_PKT_MAX_LEN], *payload = NULL;
    struct ethhdr *machdr;
    CLU_HDR_Tp cluhdr;
    int ret = 0, payload_sent = 0;
    int i,j;

    machdr = (struct ethhdr *)ether_frame;
    memcpy(machdr->h_dest, broadcast_mac, ETH_ALEN);
    memcpy(machdr->h_source, clu_ll_addr.sll_addr, ETH_ALEN);
    machdr->h_proto = htons(CLUSTER_MANAGE_PROTOCOL);

    cluhdr = (CLU_HDR_Tp)(ether_frame + ETH_HLEN);
    cluhdr->sequence = htons(cur_seq);
    debug_print("ETH_ALEN:%d, ETH_HLEN:%d\n", ETH_ALEN, ETH_HLEN);
    debug_print("build frame, seq: 0x%x, cluhdr->seq:0x%x\n", cur_seq, cluhdr->sequence);
    cluhdr->checksum = 0;
    cluhdr->totlen = 0;

    len = ETH_HLEN + sizeof(CLU_HDR_T);
    memset(ether_frame+len, 0x0, CLUSTER_PKT_MAX_LEN-len);
    
    payload=(unsigned char *)malloc(sizeof(CLU_PAYLOAD_T));
    if(payload==NULL)
    {
        debug_print("malloc %d bytes failed\n", sizeof(CLU_PAYLOAD_T));
        return ;
    }else
		memset(payload, 0x0, sizeof(CLU_PAYLOAD_T));

    //len = CLUSTER_PKT_MAX_LEN;
    debug_print("NUM_WLAN_INTERFACE:%d, NUM_VWLAN:%d\n", NUM_WLAN_INTERFACE, NUM_VWLAN);
    for(i=0; i<NUM_WLAN_INTERFACE; i++)
    {
        for(j=0; j<(NUM_VWLAN+2); j++)
        {
            debug_print("wlan_idx:%d, vwlan_idx:%d\n", i, j);
            index = 0;
            payload_sent = 0;
            memset(ether_frame+len, 0x0, CLUSTER_PKT_MAX_LEN-len);  // clear payload area

            // construct payload
            cluster_construct_wlan_profile(i, j, payload);
            cluhdr->wlan_idx = htons(i);
            cluhdr->vwlan_idx = htons(j);

            // fragmentation and send
            while(payload_sent!=sizeof(CLU_PAYLOAD_T))
            {
                len = ETH_HLEN + sizeof(CLU_HDR_T);
                memset(ether_frame+len, 0x0, CLUSTER_PKT_MAX_LEN-len);  // clear payload area

                if( (sizeof(CLU_PAYLOAD_T)-payload_sent)>(CLUSTER_PKT_MAX_LEN-len) )
                {
                    memcpy(ether_frame+len, payload+payload_sent, CLUSTER_PKT_MAX_LEN-len);
                    payload_sent += (CLUSTER_PKT_MAX_LEN-len);

                    len = CLUSTER_PKT_MAX_LEN;
                    cluhdr->more_fragment = htons(1);
                    cluhdr->frag_id = htons(index);
                    cluhdr->totlen = htons(len-ETH_HLEN);

                    checksum = get_checksum(ether_frame+ETH_HLEN, len-ETH_HLEN);
                    cluhdr->checksum = htons(checksum);
                    sendto(cluster_net_sock_client, ether_frame, len, 
                            0, (struct sockaddr *)&clu_ll_addr, sizeof(struct sockaddr_ll));
                    index++;
					total_pkt++;
                }else{
                    memcpy(ether_frame+len, payload+payload_sent, sizeof(CLU_PAYLOAD_T)-payload_sent);

                    len += (sizeof(CLU_PAYLOAD_T)-payload_sent);
                    cluhdr->more_fragment = htons(0);
                    cluhdr->frag_id = htons(index);
                    cluhdr->totlen = htons(len-ETH_HLEN);
                    checksum = get_checksum(ether_frame+ETH_HLEN, len-ETH_HLEN);
                    cluhdr->checksum = htons(checksum);
                    sendto(cluster_net_sock_client, ether_frame, len, 
                            0, (struct sockaddr *)&clu_ll_addr, sizeof(struct sockaddr_ll));

                    payload_sent = sizeof(CLU_PAYLOAD_T);
                    index++;
					total_pkt++;
                }

                usleep(200*1000);
            }
        }
    }

    if(payload)
        free(payload);
    debug_print("total packets = %d, send finish\n", total_pkt+1);
}

int main(int argc, char**argv)
{
    struct sigaction act;
    struct itimerval value;
	unsigned int peri_enable=0, peri_interval=0, send_count=0;
    cur_seq = next_seq = 0;

    log_pid();
    apmib_init();

    // set to periodic
    apmib_get(MIB_CLUSTER_MANAGE_PERIODIC_ENABLE, (void *)&peri_enable);
	apmib_get(MIB_CLUSTER_MANAGE_PERIODIC_INTERVAL, (void *)&peri_interval);
	if( peri_enable==1 && peri_interval>0 )
	{
		debug_print("register cluster client alarm, timer interval %d minutes\n", peri_interval);
	    value.it_interval.tv_sec = (peri_interval*60);
	    value.it_interval.tv_usec = 0;
	    value.it_value.tv_sec = (peri_interval*60);
	    value.it_value.tv_usec = 0;
	    act.sa_handler = handle_alarm;
	    sigemptyset(&act.sa_mask);
	    act.sa_flags = 0;
	    sigaction(SIGALRM, &act, 0);
		if (setitimer(ITIMER_REAL, &value, 0))
	    {
	        printf( "cluster_client: failed to create timer\n");
	        return -1;
	    }
	}
	
    signal(SIGUSR1, signal_usr1_handler);    //need known when configration changed
    signal(SIGTERM, signal_term_handler);

    debug_print("init cluster client\n");
    cluster_client_init();
    debug_print("init cluster client done\n");

    while(1)
    {
        if(should_send_conf)
        {
            should_send_conf = 0;
            cur_seq = next_seq;
            //sync sequence num first
            debug_print("cur_seq: %d\n", cur_seq);
            for(send_count=0; send_count<CLU_CLIENT_CONF_SEND_REPEAT_TIMES; send_count++)
            {
            	debug_print("send %d time\n", send_count+1);
                configrations_send();
                sleep(CLU_CLIENT_INTERVAL);
            }
        }else{
        	get_debug_level();
            sleep(CLU_CLIENT_INTERVAL);
        }	
    }

    close(cluster_net_sock_client);
    return 0;
}
