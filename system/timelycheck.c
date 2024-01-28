#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "apmib.h"
#include "mibtbl.h"

#ifdef CONFIG_APP_ADAPTER_API_ISP
#include "rtk_isp_common_utility.h"
#include "rtk_isp_lan_adapter.h"
#include "rtk_isp_ethernet_adapter.h"
#endif

#if defined(CONFIG_ELINKSDK_SUPPORT)
#include "libelinksdk.h"
#define RTL_LINK_PID_FILE "/var/run/rtl_link.pid"
#endif

#ifdef CONFIG_APP_ADAPTER_API_ISP
#define LAN_IFNAME "br0"
#define SEND_ARP_NUM 1
#define CHECK_INTERVAL 3
#define MAX_PORT_NUMBER 5

#define MAX_TERM_NUMBER 64

static RTK_ETH_PORT_INFO last_port_info[MAX_PORT_NUMBER];

static LAN_DEV_BASE_INFO last_dev_info[MAX_TERM_NUMBER];
static int last_wired_dev_num=0;
static unsigned long ntp_difference;
static int FileExist(char *file_name)
{
	struct stat status;

	if ( stat(file_name, &status) < 0)
		return 0;

	return 1;
}

void dumpDevInfo(int num, LAN_DEV_BASE_INFO *pDevInfo)
{
	int i;
	for(i=0;i<num;i++)
	{
		printf("\n###########################\n");
		printf("ip=%s\n",inet_ntoa(*((struct in_addr *)&pDevInfo[i].ip)));
		printf("mac=%02x:%02x:%02x:%02x:%02x:%02x\n",pDevInfo[i].mac[0], pDevInfo[i].mac[1], pDevInfo[i].mac[2],pDevInfo[i].mac[3],pDevInfo[i].mac[4],pDevInfo[i].mac[5]);
		printf("dev name=%s\n",pDevInfo[i].dev_name);
		printf("link_time=%u\n",pDevInfo[i].link_time);
		printf("\n###########################\n");
	}
}

int rtk_output_port_info_to_file(RTK_ETH_PORT_INFO *pEthPortInfo, int array_len)
{
	int i;
	int port_num;
	unsigned int up_speed=0;
	unsigned int down_speed=0;
	char line_buffer[64]={0};

	int fd=0; 
	
	if((fd = open(ETH_PORT_INFO, O_RDWR|O_CREAT|O_TRUNC))<0)
		return -1;
	
	rtk_set_lock(fd, F_WRLCK);
	
	for(i=0;i<array_len;i++)
	{
		port_num=pEthPortInfo[i].port_num;
		up_speed=pEthPortInfo[i].up_speed;
		down_speed=pEthPortInfo[i].down_speed;

		//port_num up_speed down_speed
		snprintf(line_buffer, sizeof(line_buffer), "port_num=%d,up_speed=%u,down_speed=%u\n",port_num,up_speed,down_speed);

		write(fd, line_buffer, strlen(line_buffer));		
	}	
	
	rtk_set_lock(fd, F_UNLCK);	
	
	close(fd);	
	return 0;	
}

int rtk_output_dev_to_file(LAN_DEV_BASE_INFO *pDevInfo, int array_len)
{
	int i;
	char line_buffer[512]={0};
	int fd=0; 

	
	if((fd = open(LAN_DEV_LINK_INFO, O_RDWR|O_CREAT|O_TRUNC))<0)
		return -1;
	
	rtk_set_lock(fd, F_WRLCK);
	
	for(i=0;i<array_len; i++)
	{		
		memset(line_buffer, 0, sizeof(line_buffer));		
		
		snprintf(line_buffer, sizeof(line_buffer), "ip=%s,mac=%02X:%02X:%02X:%02X:%02X:%02X,dev_name=%s,conn_type=%d,link_time=%u,up_speed=%u,down_speed=%u,conn_port=%d,expires=%u,is_alive=%d\n",
		inet_ntoa(*((struct in_addr *)&pDevInfo[i].ip)),pDevInfo[i].mac[0],pDevInfo[i].mac[1],pDevInfo[i].mac[2],pDevInfo[i].mac[3],pDevInfo[i].mac[4],pDevInfo[i].mac[5],
		pDevInfo[i].dev_name,pDevInfo[i].conn_type,pDevInfo[i].link_time,pDevInfo[i].up_speed,pDevInfo[i].down_speed,pDevInfo[i].conn_port,pDevInfo[i].expires,pDevInfo[i].is_alive);  
		
		write(fd, line_buffer, strlen(line_buffer));
	}	
	
	rtk_set_lock(fd, F_UNLCK);	
	
	close(fd);
	return 0;
}

int rtk_check_port_speed()
{
	int i;
	
	unsigned int tx_bytes=0;
	unsigned int rx_bytes=0;	
	RTK_ETH_PORT_INFO cur_port_info[MAX_PORT_NUMBER];

	memset(cur_port_info, 0, sizeof(cur_port_info));
	
	for(i=0;i<MAX_PORT_NUMBER;i++)
	{		
		cur_port_info[i].port_num=i;
		rtk_ethernet_get_port_statistics(i, &tx_bytes, &rx_bytes);
		cur_port_info[i].rx_bytes=rx_bytes;
		cur_port_info[i].tx_bytes=tx_bytes;

		cur_port_info[i].down_speed=(cur_port_info[i].rx_bytes-last_port_info[i].rx_bytes)/CHECK_INTERVAL;		
		cur_port_info[i].up_speed=(cur_port_info[i].tx_bytes-last_port_info[i].tx_bytes)/CHECK_INTERVAL;

		//save cur info
		last_port_info[i].port_num=cur_port_info[i].port_num;
		last_port_info[i].rx_bytes=cur_port_info[i].rx_bytes;
		last_port_info[i].tx_bytes=cur_port_info[i].tx_bytes;
		last_port_info[i].down_speed=cur_port_info[i].down_speed;
		last_port_info[i].up_speed=cur_port_info[i].up_speed;		
	}
	
	//save info to file 
	rtk_output_port_info_to_file(last_port_info, MAX_PORT_NUMBER);

	return 0;
}

int rtk_check_dev_is_alive()
{
	int i, j;
	struct in_addr lan_addr;
	struct sockaddr hwaddr;
	unsigned char lan_mac[6];
	
	LAN_DEV_BASE_INFO cur_dev_info[MAX_TERM_NUMBER];	
	int cur_wired_dev_num=0;

	int port_num=0;
	
	memset(cur_dev_info, 0, sizeof(cur_dev_info));
	
	getInAddr(LAN_IFNAME, IP_ADDR_T, (void *)&lan_addr);
	getInAddr(LAN_IFNAME, HW_ADDR_T, (void *)&hwaddr);	
	memcpy(lan_mac, hwaddr.sa_data, 6);
	
	rtk_lan_get_wired_client_list(cur_dev_info, MAX_TERM_NUMBER, &cur_wired_dev_num);

	//printf("\ncur_wired_dev_num=%d\n",cur_wired_dev_num);
	
	for(i=0; i<cur_wired_dev_num; i++)
	{			
		for(j=0;j<SEND_ARP_NUM;j++)
		{				
			if(rtk_send_arp(cur_dev_info[i].ip, lan_addr.s_addr, lan_mac, LAN_IFNAME)==0)
				break;
		}
		if(j<SEND_ARP_NUM)
		{
			cur_dev_info[i].link_time=CHECK_INTERVAL;
			cur_dev_info[i].is_alive=1;
		}
		else
			cur_dev_info[i].is_alive=0;
		
		port_num=cur_dev_info[i].conn_port;
		
		cur_dev_info[i].down_speed=last_port_info[port_num].down_speed;
		cur_dev_info[i].up_speed=last_port_info[port_num].up_speed;
	}	
	
	for(i=0;i<cur_wired_dev_num;i++)
	{
		for(j=0;j<last_wired_dev_num;j++)
		{
			if(cur_dev_info[i].is_alive==1 && memcmp(cur_dev_info[i].mac, last_dev_info[j].mac, 6)==0 && cur_dev_info[i].ip==last_dev_info[j].ip)
			{
				cur_dev_info[i].link_time+=last_dev_info[j].link_time;
				break;
			}
		}
		if(j==last_wired_dev_num)
			cur_dev_info[i].link_time=0;
	}	
	memcpy(last_dev_info, cur_dev_info, sizeof(LAN_DEV_BASE_INFO)*cur_wired_dev_num);		
	last_wired_dev_num=cur_wired_dev_num;	

	rtk_output_dev_to_file(cur_dev_info, cur_wired_dev_num);

#if 0
	LAN_DEV_BASE_INFO wifiClient[128];
	int wifi_client_num=0;
	rtk_lan_get_wifi_client_info(wifiClient, 128, &wifi_client_num);

	LAN_DEV_BASE_INFO WiredClient[64];
	int wired_client_num=0;
	rtk_lan_get_wired_client_info(WiredClient, 64, &wired_client_num);
#endif
	
	return 0;
}

#endif

#if defined(CONFIG_APP_BOA_AUTO_RECOVER)
#define CHECK_BOA_TIMEOUT 5
#endif

#if defined(CONFIG_ELINKSDK_SUPPORT)
#define CHECK_ELINKSDK_TIMEOUT 2
#endif


static unsigned int time_count;

#if defined(APP_WATCHDOG)
static int is_watchdog_alive(void)
{
	int is_alive = 0;
	int pid = -1;
	pid = find_pid_by_name("watchdog");
	if(pid > 0)
		is_alive = 1;
	return is_alive;
}

#endif

#if defined(CONFIG_APP_BOA_AUTO_RECOVER)
static int is_boa_alive(void)
{
	int pid = -1;
	pid = find_pid_by_name("boa");
	if(pid > 0)
		return 1;
	else
		return 0;
}
#endif

#if defined(CONFIG_ELINKSDK_SUPPORT)
static int is_elinksdk_alive(void)
{
	int status = 0;
	rtl_elinksdk_is_alive(&status);
	return status;
}

static int is_elink_alive(void){
    int pid = -1;
    pid = find_pid_by_name("elink");
    if(pid > 0)
        return 1;
    else
        return 0;
}
#endif

#ifdef CONFIG_APP_ADAPTER_API_ISP
static int is_dut_restart(void)
{
	int i=0;
	time_t timep;
	struct tm *currenttime = NULL;
	unsigned int mode = 0,enable = 0,wday = 0,hour = 0,minute = 0;

	if(!apmib_get(MIB_DUT_TIMEING_RESTART_ENABLE,(void*)&enable))
		return 1;

	if(!apmib_get(MIB_DUT_TIMEING_RESTART_MODE,(void*)&mode))
		return 1;

	if(!apmib_get(MIB_DUT_TIMEING_RESTART_WDAY,(void*)&wday))
		return 1;

	if(!apmib_get(MIB_DUT_TIMEING_RESTART_HOUR,(void*)&hour))
		return 1;

	if(!apmib_get(MIB_DUT_TIMEING_RESTART_MINUTE,(void*)&minute))
		return 1;

	time(&timep);
	currenttime = localtime(&timep);

	if(enable){
		if((wday &((1<<(4*(6-currenttime->tm_wday))))) != 0){
			if(currenttime->tm_hour == hour && currenttime->tm_min == minute)
				return 0;
		}
	}
	
	return 1;
}
#endif

void rtk_check_wan_ping_status()
{
	int access=0, ret=-1;

	apmib_set(MIB_SITE_ACCESS,(void*)&access);
	ret=system("ping www.baidu.com -c 1 ");
	if(ret == 0)
		access = 1;
	apmib_set(MIB_SITE_ACCESS,(void*)&access);
}

void timeout_handler() 
{
	char tmpBuf[128];
	time_count++;
	if(!(time_count%1))
	{
#if defined(APP_WATCHDOG)
		if(is_watchdog_alive() == 0)
		{
			//printf("watchdog is not alive\n");
			system("watchdog 1000&");
		}
#endif

	}	
#if defined(CONFIG_APP_BOA_AUTO_RECOVER)
	if(!(time_count%CHECK_BOA_TIMEOUT))
	{
		if(!is_boa_alive())
		{
			system("boa");
		}
	}
#endif
	if(!(time_count%60))
 	{
		
	}
#ifdef CONFIG_APP_ADAPTER_API_ISP
	if(!(time_count%30))
 	{
		if(!is_dut_restart()){
			system("reboot");
			while(1);
		}
	}
#endif
#ifdef CONFIG_APP_ADAPTER_API_ISP
	if(!(time_count%CHECK_INTERVAL))
 	{		
		rtk_check_port_speed();
		
		rtk_check_dev_is_alive();		

		rtk_lan_update_accumulated_client_info(&ntp_difference);
	}
#endif
#ifdef CONFIG_ELINKSDK_SUPPORT
	if(!(time_count%CHECK_ELINKSDK_TIMEOUT) && rtl_elinksdk_enabled_timelycheck())
 	{
		if(!is_elinksdk_alive())
		{
			rtl_elinksdk_start();
		}		
	}
	
	if(!(time_count%CHECK_ELINKSDK_TIMEOUT) && FileExist(RTL_LINK_PID_FILE)){
	    if(!is_elink_alive())
	    {
            system("elink -m1 -d2");
	    }
	}
#endif

 	//alarm(1);
}

static void handle_sigterm(int sig)
{
#ifdef CONFIG_APP_ADAPTER_API_ISP
	rtk_lan_fini_accumulated_client_info();
#endif

	unlink(TIMELYCHECK_RUNFILE);
	exit(0);
}

static void handle_sigusr2(int sig)
{
	printf("timelycheck got sigusr2\n");

#ifdef CONFIG_APP_ADAPTER_API_ISP
	if(isFileExist(NTP_TIME_DIFFERENCE_FILE))
	{
		FILE *fp = fopen(NTP_TIME_DIFFERENCE_FILE, "r");
		char buf[32]={0};

		if(fp!=NULL)
		{
			fread(buf, 1, sizeof(buf), fp);
			printf("%s: %s\n", NTP_TIME_DIFFERENCE_FILE, buf);

			ntp_difference = strtol(buf, NULL, 10);

			printf("ntp_difference:%d\n", ntp_difference);
			fclose(fp);
		}
	}
#endif

}

static int log_pid()
{
	FILE *fp = fopen(TIMELYCHECK_RUNFILE, "w+");
	char pid[8]={0};

	if(fp==NULL)
		return 0;

	snprintf(pid, sizeof(pid), "%d\n", getpid());
	fwrite(pid, 1, strlen(pid), fp);
	
	fclose(fp);

	return 1;
}

int main(int argc, char** argv)
{
	char	line[300];
	char action[16];
	int i;
	if ( !apmib_init()) {
		printf("Initialize AP MIB failed !\n");
		return -1;
	}	

	if(!log_pid()){
		printf("log timelycheck pid failed !\n");
		return -1;
	}	
	
#ifdef CONFIG_APP_ADAPTER_API_ISP
	rtk_lan_init_accumulated_client_info();
#endif

	//signal(SIGALRM,timeout_handler);
	signal(SIGUSR1, rtk_check_wan_ping_status);
	signal(SIGUSR2, handle_sigusr2);
	signal(SIGTERM, handle_sigterm);
	
	//alarm(1);
	while(1)
	{
		#ifdef SEND_GRATUITOUS_ARP
		checkWanStatus();
		#endif

		#ifdef CONFIG_IPV6
		checkWanLinkStatus();
		#endif
		timeout_handler();
		sleep(1);
	}
}

