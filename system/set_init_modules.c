#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "apmib.h"
#include "mibtbl.h"
#include "upmib.h"
#include "sysconf.h"
#include "sys_utility.h"
#include "syswan.h"
#ifdef 	CONFIG_RTL_ULINKER
#include <signal.h>
#endif

#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
extern void Init_Domain_Query_settings(int operation_mode, int wlan_mode, int lan_dhcp_mode, char *lan_mac);
extern void domain_query_start_dnrd(int wlan_mode, int start_dnrd);
#endif

int gateway=0;
int enable_wan=0;
int enable_br=0;
char br_interface[16]={0};
char br_lan1_interface[16]={0};
char br_lan2_interface[16]={0};
char wan_interface[16]={0};
char vlan_interface[32]={0};
static char wlan_interface[16]={0};
char wlan_valid_interface[512]={0};
char wlan_virtual_interface[80]={0};
static char wlan_vxd_interface[32]={0};
int num_wlan_interface=0;
int num_wlan_virtual_interface=0;
int num_wlan_vxd_interface=0;
int v_wlan_app_enabled=0;

int wlan_support=0;
int reinit_flag=0;

#if defined(CONFIG_RTK_MESH)
char wlan_mesh_interface[22]={0};
int num_wlan_mesh_interface=0;
#endif

#ifdef CONFIG_APP_TR069
char acsURLStr[CWMP_ACS_URL_LEN+1];
#endif //#ifdef CONFIG_APP_TR069

#ifdef CONFIG_SMART_REPEATER
int getWispRptIfaceName(char*pIface,int wlanId)
{
	int rptEnabled=0,wlanMode=0,opMode=0;
	char wlan_wanIfName[16]={0};
	if(wlanId == 0)
		apmib_get(MIB_REPEATER_ENABLED1, (void *)&rptEnabled);
	else if(1 == wlanId)
		apmib_get(MIB_REPEATER_ENABLED2, (void *)&rptEnabled);
	else return -1;
	apmib_get(MIB_OP_MODE,(void *)&opMode);
	if(opMode!=WISP_MODE)
		return -1;
	apmib_save_wlanIdx();

	sprintf(wlan_wanIfName,"wlan%d",wlanId);
	SetWlan_idx(wlan_wanIfName);
	//for wisp rpt mode,only care root ap
	apmib_get(MIB_WLAN_MODE, (void *)&wlanMode);
	if((AP_MODE==wlanMode || AP_WDS_MODE==wlanMode || AP_MESH_MODE==wlanMode || MESH_MODE==wlanMode ) && rptEnabled)
	{//root AP mode and rpt enabled, use -vxd as wanIf
		if(!strstr(pIface,"-vxd"))
		{
			strcat(pIface,"-vxd");
		}
	}else
	{
		char * ptmp = strstr(pIface,"-vxd");
		if(ptmp)
			memset(ptmp,0,sizeof(char)*strlen("-vxd"));
	}
	apmib_recov_wlanIdx();
	return 0;
}
#endif
#if defined(CONFIG_RTL_HIGH_PERFORMANCE_FILESYSTEM)
void load_rtl_fs_module(void)
{
	RunSystemCmd(NULL_FILE, "insmod", "/lib/modules/jnl.ko", NULL_STR);
	RunSystemCmd(NULL_FILE, "insmod", "/lib/modules/ufsd.ko", NULL_STR);
}
#endif
#if defined(CONFIG_APP_USBMOUNT)
#define	PARTITION_FILE "/proc/partitions"

int get_blockDevPartition(char *str, char *partition)
{

	unsigned char tk[50];
	unsigned int i,j;
	unsigned int curCnt,preCnt;
	
	if(str==NULL)
	{
		return -1;
	}
	
	memset(tk,0, sizeof(tk));

	/*	partition table format:
		major minor  #blocks  name
	*/
	
	preCnt=0;
	curCnt=0;
	for (i=0;i<strlen(str);i++)
	{          
		if( (str[i]!=' ') && (str[i]!='\n') && (str[i]!='\r'))
		{
			if(preCnt==curCnt)
			{
				tk[curCnt]=i;
				curCnt++;
			}
		}
		else if((str[i]==' ') || (str[i]=='\n') ||(str[i]=='\r') )
		{
			preCnt=curCnt;
		}
	}
	
	/*to check device major number is 8*/
	
	if(!isdigit(str[tk[0]]))
	{
		return -1;
	}

	if(tk[1]==0)
	{
		return -1;
	}

	if(tk[1]<=tk[0])
	{
		return -1;
	}

	if((str[tk[0]]!='8') ||(str[tk[0]+1]!=' '))
	{
		return -1;
	}
	
	if(tk[3]==0)
	{
		return -1;
	}

	/*to get partition name*/
	j=0;
	for(i=tk[3]; i<strlen(str); i++)
	{
		
		if((str[i]==' ') || (str[i]=='\n') ||(str[i]=='\n'))
		{
			partition[j]='\0';
			return 0;
		}
		else
		{
			partition[j]=str[i];
			j++;
		}
			
	}
	return 0;
}
int Check_shouldMount(char *partition_name)
{
	DIR *dir=NULL;
	struct dirent *next;
	int found=0;
	dir = opendir("/tmp/usb");
	if (!dir) {
		printf("Cannot open %s", "/tmp/usb");
		return -1;
	}
	while ((next = readdir(dir)) != NULL) {
		//printf("Check_shouldMount:next->d_reclen=%d, next->d_name=%s\n",next->d_reclen, next->d_name);
			/* Must skip ".." */
			if (strcmp(next->d_name, "..") == 0)
				continue;
			if (strcmp(next->d_name, ".") == 0)
				continue;
			if (strcmp(next->d_name, "mnt_map") == 0)
				continue;
			if(!strcmp(next->d_name, partition_name)){
				found=1;
				break;
			}
	}
	closedir(dir);
	return found;
}

void autoMountOnBootUp(void)
{
	FILE *fp;
	
	int line=0;
	char buf[512];
	char partition[32];
	char usbMntCmd[64];
	int ret=-1;
	if(isFileExist(PARTITION_FILE)){
		fp= fopen(PARTITION_FILE, "r");
		if (!fp) {
	        	printf("can not  open /proc/partitions\n");
			return; 
	   	}

		while (fgets(buf, sizeof(buf), fp)) 
		{
			ret=get_blockDevPartition(buf, &partition);
			if(ret==0)
			{
				if(Check_shouldMount(partition)==0){
				sprintf(usbMntCmd, "DEVPATH=/sys/block/sda/%s ACTION=add usbmount block", partition);
				RunSystemCmd(NULL_FILE,  "echo", usbMntCmd, NULL_STR);
				system(usbMntCmd);
			}
			}
			
		}
	
		fclose(fp);
	}
	

}

void start_mount()
{
#if defined(HTTP_FILE_SERVER_SUPPORTED) || (defined(RTL_USB_IP_HOST_SPEEDUP) && !defined(CONFIG_RTL_8198C))
	RunSystemCmd("/proc/sys/vm/min_free_kbytes", "echo", "2048", NULL_STR);
	RunSystemCmd("/proc/sys/net/core/rmem_max", "echo", "1048576", NULL_STR);
	RunSystemCmd("/proc/sys/net/core/wmem_max", "echo", "1048576", NULL_STR);
	RunSystemCmd("/proc/sys/net/ipv4/tcp_rmem", "echo", "4096 108544 4194304", NULL_STR);
	RunSystemCmd("/proc/sys/net/ipv4/tcp_wmem", "echo", "4096 108544 4194304", NULL_STR);
	RunSystemCmd("/proc/sys/net/ipv4/tcp_moderate_rcvbuf", "echo", "0", NULL_STR);
#else
	/*config linux parameter for improving samba performance*/
	RunSystemCmd("/proc/sys/vm/min_free_kbytes", "echo", "1024", NULL_STR);
	
	RunSystemCmd("/proc/sys/net/core/netdev_max_backlog", "echo", "8192", NULL_STR);
	RunSystemCmd("/proc/sys/net/core/optmem_max", "echo", "131072", NULL_STR);
	RunSystemCmd("/proc/sys/net/core/rmem_default", "echo", "524288", NULL_STR);
	RunSystemCmd("/proc/sys/net/core/rmem_max", "echo", "524288", NULL_STR);
	RunSystemCmd("/proc/sys/net/core/wmem_default", "echo", "524288", NULL_STR);
	RunSystemCmd("/proc/sys/net/core/wmem_max", "echo", "524288", NULL_STR);
	RunSystemCmd("/proc/sys/net/ipv4/tcp_rmem", "echo", "131072 262144 393216", NULL_STR);
	RunSystemCmd("/proc/sys/net/ipv4/tcp_wmem", "echo", "131072 262144 393216", NULL_STR);
	RunSystemCmd("/proc/sys/net/ipv4/tcp_mem", "echo", "768 1024 1380", NULL_STR);
#endif
	/*config hot plug and auto-mount*/
	RunSystemCmd("/proc/sys/kernel/hotplug", "echo", "/usr/hotplug", NULL_STR);
	RunSystemCmd(NULL_FILE, "mkdir", "-p /tmp/usb/", NULL_STR);

	/*force kernel to write data to disk, don't cache in memory for a long time*/
	RunSystemCmd("/proc/sys/vm/vfs_cache_pressure", "echo", "10000", NULL_STR);
	RunSystemCmd("/proc/sys/vm/dirty_background_ratio", "echo", "5", NULL_STR);
	RunSystemCmd("/proc/sys/vm/dirty_writeback_centisecs", "echo", "100", NULL_STR);
	/*automatically mount partions listed in /proc/partitions*/
	autoMountOnBootUp();
}

#if defined(CONFIG_SYSFS)
static void rtl_mount_sysfs(void)
{
	system("mount -t sysfs sysfs /sys");
}
#endif

#if defined(CONFIG_APP_VSFTPD)
void start_vsftpd()
{
	RunSystemCmd(NULL_FILE, "echo","start vsftpd", NULL_STR);
	RunSystemCmd(NULL_FILE,  "cp", "/etc/vsftpd.conf", "/var/config/vsftpd.conf",  NULL_STR);
	system("vsftpd /var/config/vsftpd.conf &");
}
#endif

#endif

#if defined(CONFIG_RPS)
static void rtl_configRps(void)
{
	system("echo 2 > /sys/class/net/eth0/queues/rx-0/rps_cpus");
	system("echo 2 > /sys/class/net/eth1/queues/rx-0/rps_cpus");
	system("echo 2 > /sys/class/net/eth2/queues/rx-0/rps_cpus");
	system("echo 2 > /sys/class/net/eth3/queues/rx-0/rps_cpus");
	system("echo 2 > /sys/class/net/eth4/queues/rx-0/rps_cpus");
	system("echo 2 > /sys/class/net/wlan0/queues/rx-0/rps_cpus");
	system("echo 2 > /sys/class/net/wlan1/queues/rx-0/rps_cpus");
	system("echo 4096 > /sys/class/net/eth0/queues/rx-0/rps_flow_cnt");
	system("echo 4096 > /sys/class/net/eth1/queues/rx-0/rps_flow_cnt");
	system("echo 4096 > /sys/class/net/eth2/queues/rx-0/rps_flow_cnt");
	system("echo 4096 > /sys/class/net/eth3/queues/rx-0/rps_flow_cnt");
	system("echo 4096 > /sys/class/net/eth4/queues/rx-0/rps_flow_cnt");
	system("echo 4096 > /proc/sys/net/core/rps_sock_flow_entries");
}
#endif
#if defined(CONFIG_APP_USBMOUNT)
static void rtl_config_read_ahead(void)
{
	FILE *fp;
	
	char buf[512];
	char partition[32];
	char usbMntCmd[64];
	int ret=-1;
	fp= fopen(PARTITION_FILE, "r");
	if (!fp) {
			printf("can not  open /proc/partitions\n");
		return; 
	}

	while (fgets(buf, sizeof(buf), fp)) 
	{
		ret=get_blockDevPartition(buf, &partition);
		if(ret==0 && Check_shouldMount(partition)==0 && strlen(partition) == 3)
		{
			sprintf(usbMntCmd, "echo 4096 > /sys/block/%s/queue/read_ahead_kb", partition);
			system(usbMntCmd);
		}
	}

	fclose(fp);
}
#endif

#if defined(CONFIG_APP_SAMBA)
void cp_samba_conf()
{
	//RunSystemCmd(NULL_FILE,  "echo", "start samba", NULL_STR);
	RunSystemCmd(NULL_FILE,  "mkdir", "/var/samba", NULL_STR);
	RunSystemCmd(NULL_FILE,  "cp", "/etc/samba/smb.conf", "/var/samba/smb.conf",  NULL_STR);
	RunSystemCmd(NULL_FILE,  "cp", "/etc/samba/smbpasswd", "/var/samba/smbpasswd",	NULL_STR);
	RunSystemCmd(NULL_FILE,  "cp", "/etc/samba/upcase.dat", "/var/lib/",  NULL_STR);
	RunSystemCmd(NULL_FILE,  "cp", "/etc/samba/lowcase.dat", "/var/lib/",  NULL_STR);
	RunSystemCmd(NULL_FILE,  "cp", "/etc/samba/valid.dat", "/var/lib/",  NULL_STR);
//	RunSystemCmd("/var/group",	"echo", " ",  NULL_STR);
//	RunSystemCmd(NULL_FILE,  "cp", "/etc/group", "/var/group",	NULL_STR);
}

#ifdef SAMBA_WEB_SUPPORT
void add_samba_group_user(){
	int i, number;
	char cmdBuffer[100];
	STORAGE_USER_T	user_info;
	STORAGE_GROUP_T group_info;
#if 0
	apmib_get(MIB_STORAGE_GROUP_TBL_NUM,(void*)&number);
	for(i = 0;i < number;i++)
	{
		memset(&group_info,'\0',sizeof(STORAGE_GROUP_T));
		*((char*)&group_info) = (char)(i+1);
		apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&group_info);

		memset(cmdBuffer,'\0',100);
		snprintf(cmdBuffer,100,"addgroup %s",group_info.storage_group_name);
		system(cmdBuffer);
	}
#endif
	apmib_get(MIB_STORAGE_USER_TBL_NUM,(void*)&number);
	for(i = 0;i < number;i++)
	{
		memset(&user_info,'\0',sizeof(STORAGE_USER_T));
		*((char*)&user_info) = (char)(i+1);
		apmib_get(MIB_STORAGE_USER_TBL,(void*)&user_info);

		memset(cmdBuffer,'\0',100);
#if 0
		if(!strcmp(user_info.storage_user_group,"--")){
			snprintf(cmdBuffer,100,"adduser %s",user_info.storage_user_name);
			system(cmdBuffer);
		}else{
			snprintf(cmdBuffer,100,"adduser -G %s %s",user_info.storage_user_group,user_info.storage_user_name);
			system(cmdBuffer);
		}
		
#else
		snprintf(cmdBuffer,100,"adduser %s",user_info.storage_user_name);
		system(cmdBuffer);
#endif
		
		memset(cmdBuffer,'\0',100);
#ifndef CONFIG_APP_SAMBA_3_6_24
		snprintf(cmdBuffer,100,"smbpasswd %s %s",user_info.storage_user_name,user_info.storage_user_password);
#else
		snprintf(cmdBuffer,100,"echo -e \"%s\\n%s\" | smbpasswd -a %s",user_info.storage_user_password,user_info.storage_user_password,user_info.storage_user_name);
#endif
		system(cmdBuffer);
	}
}

#endif

void start_samba()
{
	int samba_enabled, anon_access = 0;
	char cmdBuffer[100];
	
	system("killall usbStorageAppController 2>/dev/null");
	if(!apmib_get(MIB_SAMBA_ENABLED, (void*)&samba_enabled) || !samba_enabled)
		return;
	
	cp_samba_conf();

#ifdef SAMBA_WEB_SUPPORT	
	add_samba_group_user();
	apmib_get(MIB_STORAGE_ANON_ENABLE,(void*)&anon_access);
#endif
	
	snprintf(cmdBuffer,100,"usbStorageAppController -a %d &",anon_access);
	system(cmdBuffer);
	return;
}

#endif

#ifdef CONFIG_APP_MINIDLNA
#define MINIDLNA_CONF_FILE "/var/minidlna.conf"

void setMinidlnaSharedFolder(){
	int i, num;
	char tmp[10] = {0};
	char buf[100] = {0};
	MINIDLNA_SHAREFOLDER_T entry;
	if(!apmib_get(MIB_MINIDLNA_SHAREDFOLDER_TBL_NUM, (void *)&num))
		return;
	
	if(num){
		for(i = 1; i <= num; i++){
			*((char *)&entry) = (char)i;
		if(!apmib_get(MIB_MINIDLNA_SHAREDFOLDER_TBL, (void *)&entry))
			return;
		if(strlen(entry.minidlna_sharefolder_name) == 0)
			return;
		
		bzero(buf, sizeof(buf));
		bzero(tmp, sizeof(tmp));
		snprintf(buf, sizeof(buf), "%s", "media_dir=");
		if(strcmp("audio", entry.minidlna_sharedfolder_type) == 0) 
			snprintf(tmp, sizeof(tmp), "%s", "A,");
		else if(strcmp("video", entry.minidlna_sharedfolder_type) == 0) 
			snprintf(tmp, sizeof(tmp), "%s", "V,");
		else if(strcmp("images", entry.minidlna_sharedfolder_type) == 0) 
			snprintf(tmp, sizeof(tmp), "%s", "P,");
		if(strlen(tmp))
			strcat(buf,tmp);
		
		strcat(buf, entry.minidlna_sharefolder_name);

		//TODO
		//write buf to the /var/minidlna.conf	at the original media_dir=/tmp/usb/sda6/Media location
		}
	}
	
}


void clean_lan_minidlna()
{	
	RunSystemCmd(NULL_FILE, "killall", "-9", "minidlna",NULL_STR);	
}

void start_lan_minidlna()
{
	int enabled;

	clean_lan_minidlna();	
	if(!apmib_get(MIB_MINIDLNA_ENABLED, (void *)&enabled) || !enabled)
		return;
	
	system("cp /etc/minidlna.conf /var/minidlna.conf");

	//TODO : write share folder to /var/minidlna.conf	
	setMinidlnaSharedFolder();
	
	system("minidlna -R -f /var/minidlna.conf");	
}
#endif

#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
void set_Domain_Query_settings()
{
	int opmode=0,wlan_mode_root=0,lan_dhcp_mode=0;
	char lan_mac[16]={0};
	apmib_get(MIB_OP_MODE,(void *)&opmode);
	apmib_get(MIB_DHCP,(void *)&lan_dhcp_mode);
	
	if(SetWlan_idx("wlan0")){
			apmib_get( MIB_WLAN_MODE, (void *)&wlan_mode_root); 
	}
	get_lan_mac(lan_mac);
	Init_Domain_Query_settings(opmode, wlan_mode_root, lan_dhcp_mode, lan_mac);
}
#endif
int _is_hex(char c)
{
    return (((c >= '0') && (c <= '9')) ||
            ((c >= 'A') && (c <= 'F')) ||
            ((c >= 'a') && (c <= 'f')));
}

int _string_to_hex(char *string, unsigned char *key, int len)
{
	char tmpBuf[4];
	int idx, ii=0;
	for (idx=0; idx<len; idx+=2) {
		tmpBuf[0] = string[idx];
		tmpBuf[1] = string[idx+1];
		tmpBuf[2] = 0;
		if ( !_is_hex(tmpBuf[0]) || !_is_hex(tmpBuf[1]))
		{
			return 0;
		}

		key[ii++] = (unsigned char) strtol(tmpBuf, (char**)NULL, 16);
	}
	return 1;
}

char * strToValue(char *str,TYPE_T type)
{
	switch(type)
	{
		case BYTE_T:
		case WORD_T:
		case DWORD_T:
		{
			int i=atoi(str);
			*((int*)str)=i;
			return str;
		}
		case STRING_T:
			return str;
		case IA_T:
		{
			struct in_addr addr;
			inet_aton(str,&addr);
			*((struct in_addr*)str)=addr;
			return str;
		}
		case DHCPRSVDIP_ARRY_T:
		{
			DHCPRSVDIP_T staticIPEntry={0};
			char ipAddr[32]={0};
			char macAddr[32]={0};
#ifdef SUPPORT_DHCP_PORT_IP_BIND
			char portId[8]={0};
			sscanf(str,"%s\t%s\t%s\t%s",ipAddr,portId,macAddr,staticIPEntry.hostName);
			staticIPEntry.portId=atoi(portId);
#else
			sscanf(str,"%s\t%s\t%s",ipAddr,macAddr,staticIPEntry.hostName);
#endif
			inet_aton(ipAddr,(struct in_addr *)staticIPEntry.ipAddr);
			_string_to_hex(macAddr,staticIPEntry.macAddr,12);
			*((DHCPRSVDIP_T*)str)=staticIPEntry;
			return str;
		}
		default:
			return str;
	}
}

int up_mib_value()
{
        int old_ver=0;
        int new_ver=0;
        int i=0;
 
        apmib_get(MIB_MIB_VER, (void *)&old_ver);
#ifdef RTL_DEF_SETTING_IN_FW
		apmib_getDef(MIB_MIB_VER,(void *)&new_ver);
#else
        new_ver = atoi(update_mib[0].value);
#endif

        if(old_ver == new_ver)
        {
                return -1;
        }
        else
                printf("MIB Version update!\n");
		
		
 
#if defined(UPGRADE_BOOT_FROM_ROOT)
	 upgrade_boot();
#endif
#ifdef RTL_DEF_SETTING_IN_FW
		if(new_mib[0].id==0&&update_mib[1].id==0)
		{
			system("flash reset");
			return 0;
		}
#endif		
        i=0;
        while(new_mib[i].id != 0)
        {
        		apmib_set(new_mib[i].id,strToValue(new_mib[i].value,new_mib[i].type)); 
                //RunSystemCmd(NULL_FILE, "flash", "set", new_mib[i].name, new_mib[i].value, NULL_STR);
                i++;
        }
 
        i=0;
        while(update_mib[i].id != 0)
        {
        	apmib_set(update_mib[i].id,strToValue(update_mib[i].value,update_mib[i].type));        	
              //  RunSystemCmd(NULL_FILE, "flash", "set", update_mib[i].name, update_mib[i].value, NULL_STR);
                i++;
        }
 		apmib_update(CURRENT_SETTING);
        return 0;
 
}
void Init_Internet(int argc, char** argv)
{
#ifdef   HOME_GATEWAY	
	int wisp_id=0, wan_mode=0, opmode=0;
	char br_interface[16]={0};
	char wan_interface[16]={0};
	char tmp_args[16]={0};
	
	
	if(argc < 4)
		return;
		
	apmib_get(MIB_OP_MODE,(void *)&opmode);
	if(opmode == BRIDGE_MODE)
		return;
	
	apmib_get(MIB_WISP_WAN_ID,(void *)&wisp_id);
	
	sprintf(tmp_args, "%s",argv[1]);
	if(!strcmp(tmp_args, "pppoe"))
		wan_mode=3;
	else if(!strcmp(tmp_args, "pptp"))
		wan_mode=4;	
	else if(!strcmp(tmp_args, "l2tp"))
		wan_mode=6;
	else{
		printf("Un-support wan type for init\n");
		return;
	}
	sprintf(br_interface, "%s", "br0");
	sprintf(wan_interface, "%s",argv[3]);
	start_wan(wan_mode, opmode, wan_interface, br_interface, wisp_id, 0);		
#endif	
}
void Init_QoS(int argc, char** argv)
{
#ifdef   HOME_GATEWAY	
	int wisp_id=0, wan_mode=0, opmode=0, qosFlag=0;;

	apmib_get(MIB_OP_MODE,(void *)&opmode);
	if(opmode == 1)
		return;
		
	apmib_get(MIB_WISP_WAN_ID,(void *)&wisp_id);
	apmib_get(MIB_WAN_DHCP,(void *)&wan_mode);		

#if defined(CONFIG_RTL_HARDWARE_IPV6_SUPPORT)
	//modifyHwIpv6Status();
	update_hw_ipv6_status();
#endif

#if defined(QOS_OF_TR069)	
	apmib_get(MIB_QOS_ENABLE,(void *)&qosFlag); 
		if(qosFlag) 	// enable tr098 qos will make webpage qos invalid
			return;
#endif	
	set_QoS(opmode, wan_mode, wisp_id);
#endif	
	
}


#ifdef CONFIG_APP_RTK_BLUETOOTH_FM
void init_module_rtk_bt_fm()
{
	system("echo /bin/mdev > /proc/sys/kernel/hotplug");
    system("mount -t sysfs sysfs /sys");
    system("echo 1 > /sys/class/firmware/timeout");
}
#endif

void init_module_set_time()
{	

	RunSystemCmd(NULL_FILE, "flash", "settime", NULL_STR);

}
#ifdef TR181_V6_SUPPORT
int init_module_tr181()
{
	int i=0,intValue=1;
	char tmpBuff[512]={0};
	for(i=0; i<DNS_CLIENT_SERVER_NUM; i++)
	{
		sprintf(tmpBuff, "Device.DNS.Client.Server.{%d}.Enable", i);
		if(tr181_ipv6_set(tmpBuff ,(void *) &intValue) == -1)
		{
			printf("%s(%d): set %s fail!\n",__FUNCTION__,__LINE__, tmpBuff);
			return -1;
		}
	}
	return 0;
}
#endif
/**********************
* init once when start up *
***********************/
void init_modules_startup_init()
{
	
#ifdef CONFIG_APP_RTK_BLUETOOTH_FM
	init_module_rtk_bt_fm();
#endif

	up_mib_value();
#ifdef DEFSETTING_AUTO_UPDATE
		defsetting_auto_update();
#endif
#ifdef TR181_V6_SUPPORT
	init_module_tr181();
#endif
}
#ifdef CONFIG_RTL_DNS_TRAP
int init_module_dns_trap()
{
	unsigned char domain_name[MAX_NAME_LEN];
	apmib_get(MIB_DOMAIN_NAME, (void *)domain_name);
	strtolower(domain_name,strlen(domain_name));
	strcat(domain_name,".com");
	create_hosts_file(domain_name);
	RunSystemCmd(DNS_PROC_FILE, "echo", domain_name, NULL_STR);
}
#endif
#if defined(CONFIG_BAND_2G_ON_WLAN0) || defined(CONFIG_BAND_5G_ON_WLAN0)
void init_module_wlanBandMibSwitch()
{
	int wlanBand2G5GSelect;
	int phyBandSelect;
	int old_wlan_idx;
	apmib_get(MIB_WLAN_BAND2G5G_SELECT,(void *)&wlanBand2G5GSelect);
	if(wlanBand2G5GSelect == BANDMODEBOTH)//dual band
	{
		old_wlan_idx = wlan_idx;
		wlan_idx = 0;
		apmib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyBandSelect);	
#if defined(CONFIG_BAND_2G_ON_WLAN0)
		if(phyBandSelect != PHYBAND_2G)
			swapWlanMibSetting(0,1);
#else
#ifndef CONFIG_APP_APPLE_MFI_WAC
        if(phyBandSelect != PHYBAND_5G)
			swapWlanMibSetting(0,1);
#endif
#endif
		wlan_idx = old_wlan_idx;
	}
}
#endif


void init_module_reinit_run()
{
	int wan_dhcp_mode=0;
	char tmp_args[16]={0};
#if defined(CONFIG_RTL_819XD)	
	//The powerful CPU won't take 7s from interface down to up, so sleep 1s to force LAN/WLAN PC to renew IP.
	//The timeout is 25s in WEB UI, it can redirect to new IP after count down timeout, so sleep 1s is not side effect.
	sleep(1);		
#endif
#ifdef 	CONFIG_RTL_ULINKER
	{
		int pid_boa = find_pid_by_name("boa");
		if(pid_boa>0){
			kill(pid_boa,SIGUSR2);
		}
	}
#endif

}
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
void init_module_8021x()
{
		if((isFileExist(RS_USER_CERT_5G)==0) && (isFileExist(RS_ROOT_CERT_5G)==0) &&  (isFileExist(RS_USER_CERT_2G)==0) && (isFileExist(RS_ROOT_CERT_2G)==0)){
			RunSystemCmd(NULL_FILE, "rsCert","-rd", NULL_STR);
		}
}
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT
void init_modue_wapi()
{	
#ifdef CONFIG_RTL_WAPI_LOCAL_AS_SUPPORT
		if(isFileExist(CA_CERT_FILE)==0){
			RunSystemCmd(NULL_FILE, "loadWapiFiles", NULL_STR);
		}
#else
		if((isFileExist(CA4AP_CERT)==0 && isFileExist(AP_CERT)==0 )
			&& (isFileExist(CA4AP_CERT_AS0)==0 && isFileExist(AP_CERT_AS0)==0 )
			&& (isFileExist(CA4AP_CERT_AS1)==0 && isFileExist(AP_CERT_AS1)==0 )){
			RunSystemCmd(NULL_FILE, "loadWapiFiles", NULL_STR);
		}
#endif	
}
#endif

void generate_ifname(int argc, char** argv,int opmode)
{
	char tmp_args[16];
	int wisp_wan_id=0,lan_dhcp_mode=0,wan_dhcp_mode=0,i=0;
	int intValue=0;
	apmib_get(MIB_WISP_WAN_ID,(void *)&wisp_wan_id);
	apmib_get(MIB_DHCP,(void *)&lan_dhcp_mode);
#ifdef   HOME_GATEWAY	
	apmib_get(MIB_WAN_DHCP,(void *)&wan_dhcp_mode);
#endif	
	sprintf(tmp_args,"%s", argv[2]);  
	if(strcmp(tmp_args, "ap") == 0){
        sprintf(br_interface, "%s", "br0");
		sprintf(br_lan1_interface, "%s" , "eth0");
#if !defined(CONFIG_RTL_AP_PACKAGE) 
		if(opmode==BRIDGE_MODE)
			sprintf(br_lan2_interface, "%s", "eth1");
#endif
		gateway=0;
	}	
	if(!strcmp(tmp_args, "gw")){		
        gateway=1;
		if(opmode==WISP_MODE)
		{
			sprintf(wan_interface, "wlan%d", wisp_wan_id);
#if defined(CONFIG_SMART_REPEATER)	
			getWispRptIfaceName(wan_interface,wisp_wan_id);

			//strcat(wan_interface,"-vxd");
#endif			
		}
		else
		{
#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
			if(opmode == GATEWAY_MODE)
				sprintf(wan_interface, "%s", "eth1");
			
#else
			sprintf(wan_interface, "%s", "eth1");

	#if defined(CONFIG_4G_LTE_SUPPORT)
	if (lte_wan()) {
			sprintf(wan_interface, "%s", "usb0");
	}
	#endif /* #if defined(CONFIG_4G_LTE_SUPPORT) */
#endif
		}
			
//printf("\r\n wan_interface=[%s],__[%s-%u]\r\n",wan_interface,__FILE__,__LINE__);
			
		sprintf(br_interface, "%s", "br0");
#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
		//if(opmode != 0)
#ifdef RTK_USB3G
 		if((wan_dhcp_mode == USB3G) || (opmode != 0))
			sprintf(br_lan1_interface, "%s" , "eth0");
		else
#endif
		if(opmode != GATEWAY_MODE)
			sprintf(br_lan1_interface, "%s" , "eth0");
			
#else
		sprintf(br_lan1_interface, "%s" , "eth0");
#endif

		if(opmode ==BRIDGE_MODE || opmode == WISP_MODE) {
#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
#else
			sprintf(br_lan2_interface, "%s", "eth1");
#endif
		}
		/**/
		//RunSystemCmd("/proc/wan_port", "echo", "4", NULL_STR);	
	}
	/*
	if(isFileExist(ETH_VLAN_SWITCH)){//vlan init file is exist
		sprintf(vlan_interface, "%s %s %s", "eth2", "eth3", "eth4");
	}
	*/
	
	RunSystemCmd(PROC_BR_IGMPPROXY, "echo", "0", NULL_STR);
	
#if defined(VLAN_CONFIG_SUPPORTED)
	apmib_get(MIB_VLANCONFIG_ENABLED, (void *)&intValue);
	if(intValue !=0) {
#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
#elif defined(CONFIG_RTK_BRIDGE_VLAN_SUPPORT)
		sprintf(vlan_interface, "%s %s %s %s", "eth2", "eth3", "eth4", "eth7");
#else
		sprintf(vlan_interface, "%s %s %s", "eth2", "eth3", "eth4");
#endif
	}
	else		
		memset(vlan_interface, 0x00, sizeof(vlan_interface));

#endif //#if defined(VLAN_CONFIG_SUPPORTED)

	memset(wlan_interface, 0x00, sizeof(wlan_interface));
	memset(wlan_virtual_interface, 0x00, sizeof(wlan_virtual_interface));
	memset(wlan_vxd_interface, 0x00, sizeof(wlan_vxd_interface));
	memset(wlan_valid_interface, 0x00, sizeof(wlan_valid_interface));
	
	for(i=0;i<NUM_WLAN_INTERFACE;i++){
		if(wlan_interface[0]==0x00)
			sprintf(wlan_interface, "wlan%d", i);
		else{
			sprintf(tmp_args, " wlan%d", i);
			strcat(wlan_interface, tmp_args); 
		}
	}
	/* collect all wlan interface for clean_process() */
	num_wlan_interface=NUM_WLAN_INTERFACE;
	num_wlan_virtual_interface=if_readlist_proc(wlan_virtual_interface, "va", 0);
	num_wlan_vxd_interface=if_readlist_proc(wlan_vxd_interface, "vxd", 0);
#if defined(CONFIG_RTK_MESH)
	num_wlan_mesh_interface=if_readlist_proc(wlan_mesh_interface, "msh", 0);
#endif
	wlan_support = if_readlist_proc(wlan_valid_interface, "wlan", 0);
	if(wlan_support==0)
		memset(wlan_interface, 0x00, sizeof(wlan_interface));
		
//printf("\r\n wlan_vxd_interface=[%s],__[%s-%u]\r\n",wlan_vxd_interface,__FILE__,__LINE__);
		
/* set interface name  end*/			
#if defined(CONFIG_POCKET_AP_SUPPORT) && defined(HOME_GATEWAY)
	sprintf(tmp_args,"%s", argv[3]);
	if(strcmp(tmp_args, "wlan_app") != 0)
	{
		//clean_process(opmode,wan_dhcp_mode,gateway, enable_wan, br_interface, wlan_interface, wan_interface);
		init_module_nat_init();
		/*init wlan interface*/
		if (wlan_support != 0)
		{
			memset(wlan_interface, 0x00, sizeof(wlan_interface));
			for(i=0;i<NUM_WLAN_INTERFACE;i++)
			{
				int wlan_disable = 1;			
				unsigned char wlan_name[10];
				memset(wlan_name,0x00,sizeof(wlan_name));
				sprintf(wlan_name, "wlan%d",i);
				if(SetWlan_idx(wlan_name))
				{			
					apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disable);	  

				
					if(wlan_disable == 1)
					{
						RunSystemCmd(NULL_FILE, "iwpriv", wlan_name, "radio_off", NULL_STR);					
					}
					else
					{
						if(wlan_interface[0]==0x00)
							sprintf(wlan_interface, "%s", wlan_name);
						else
						{
							sprintf(tmp_args, " %s", wlan_name);
							strcat(wlan_interface, tmp_args); 
						}							
					}
				}
			}				
		}
	}
#endif	//#if defined(CONFIG_POCKET_AP_SUPPORT) && defined(HOME_GATEWAY)

//enable vwlan interface name
	{
		int repeater_enabled1=0,repeater_enabled2=0,wlan_mode_root=0,wlan_mode_root1=0;
		int wlan_root_disabled=0,wlan_root1_disabled=0;
		apmib_get(MIB_REPEATER_ENABLED1,(void *)&repeater_enabled1);
		apmib_get(MIB_REPEATER_ENABLED2,(void *)&repeater_enabled2);

#if defined(CONFIG_RTL_92D_SUPPORT)
		if(SetWlan_idx("wlan1")){
				apmib_get( MIB_WLAN_MODE, (void *)&wlan_mode_root1); 
				apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&wlan_root1_disabled);	  
		}
		if(v_wlan_app_enabled == 0 
			&& (wlan_mode_root1 == AP_MODE || wlan_mode_root1 == AP_WDS_MODE || repeater_enabled2 == 1)
		)
		{
			v_wlan_app_enabled=1; // have virtual or repeat
		}
		
#endif
		if(SetWlan_idx("wlan0")){
			apmib_get( MIB_WLAN_MODE, (void *)&wlan_mode_root); 
			apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&wlan_root_disabled);	  
		}

		if(v_wlan_app_enabled == 0 
			&& (wlan_mode_root == AP_MODE || wlan_mode_root == AP_WDS_MODE || repeater_enabled1 == 1)
		)
		{
			v_wlan_app_enabled=1; // have virtual or repeat
		}
		
		memset(wlan_vxd_interface, 0x00, sizeof(wlan_vxd_interface));
		if(repeater_enabled1 == 1 && !wlan_root_disabled){
#if defined(CONFIG_SMART_REPEATER)
			if(strcmp(wan_interface,"wlan0-vxd"))
#endif
			{
				if(strlen(wlan_vxd_interface) != 0)
					strcat(wlan_vxd_interface," ");	
				strcat(wlan_vxd_interface,"wlan0-vxd");
			}
		}
#if defined(CONFIG_RTL_92D_SUPPORT)
		if(repeater_enabled2 == 1 && !wlan_root1_disabled)
		{
#if defined(CONFIG_SMART_REPEATER)
	        if(strcmp(wan_interface,"wlan1-vxd"))
#endif
			{

				if(strlen(wlan_vxd_interface) != 0)
					strcat(wlan_vxd_interface," ");	
				strcat(wlan_vxd_interface,"wlan1-vxd");
			}	
		}
#endif

	}
}
void init_module_wlan_app(int action)
{	
	char tmpBuff[128]={0};
	int lan_dhcp_mode=0;
	apmib_get(MIB_DHCP,(void *)&lan_dhcp_mode);

	if(lan_dhcp_mode!=DHCP_LAN_SERVER && lan_dhcp_mode==DHCP_LAN_NONE
#ifdef CONFIG_DOMAIN_NAME_QUERY_SUPPORT		
				&& lan_dhcp_mode!=DHCP_AUTO
#endif
		)
		return;
#if defined(CONFIG_RTL_ULINKER_WLAN_DELAY_INIT)
	int ulinker_auto = 0;
	apmib_get(MIB_ULINKER_AUTO,  (void *)&ulinker_auto);
	if (ulinker_auto == 1) {
		if (isFileExist("/var/ulinker_init2")==0) {
			BDBG_FPRINT(stderr, "[%s:%d] break!\n", __FUNCTION__, __LINE__);
			return 0;
		}
		else {
			BDBG_FPRINT(stderr, "[%s:%d] exec\n", __FUNCTION__, __LINE__);
		}
	}
#endif

#if defined(CONFIG_RTL_ULINKER)
	/* notice ulinker_process to reset domain name query */
	system("echo 1 > /var/ulinker_reset_domain");
#endif

	memset(tmpBuff, 0x00, sizeof(tmpBuff));
	if(action==1){
		if(wlan_interface[0] && wlan_virtual_interface[0] && wlan_vxd_interface[0] && br_interface[0])
		sprintf(tmpBuff, "%s %s %s %s", wlan_interface, wlan_virtual_interface, wlan_vxd_interface, br_interface); 			
		else if(wlan_interface[0] && wlan_virtual_interface[0] && !wlan_vxd_interface[0] && br_interface[0])
			sprintf(tmpBuff, "%s %s %s", wlan_interface, wlan_virtual_interface, br_interface); 
		else if(wlan_interface[0] && !wlan_virtual_interface[0] && wlan_vxd_interface[0] && br_interface[0])
			sprintf(tmpBuff, "%s %s %s", wlan_interface, wlan_vxd_interface, br_interface); 	
		else if(wlan_interface[0] && !wlan_virtual_interface[0] && !wlan_vxd_interface[0] && br_interface[0])
			sprintf(tmpBuff, "%s %s", wlan_interface, br_interface); 						
	}else {
		//V_WLAN_APP_ENABLE=0 or para2=wlan_app
		if(wlan_interface[0] && br_interface[0])
		sprintf(tmpBuff, "%s %s", wlan_interface, br_interface); 	
	}

	RunSystemCmd(PROC_GPIO, "echo", "I", NULL_STR);

//printf("\r\n tmpBuff=[%s],__[%s-%u]\r\n",tmpBuff,__FILE__,__LINE__);

#if defined(CONFIG_APP_SIMPLE_CONFIG)
		system("killall simple_config >/dev/null 2>&1");
#endif
#ifdef SHRINK_INIT_TIME
		cleanWlan_Applications(wlan_interface);
#endif

	if(tmpBuff[0])
		setWlan_Applications("start", tmpBuff);

	#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT) || defined(CONFIG_APP_APPLE_MFI_WAC)
		system("rm -f  /var/system/start_init 2> /dev/null");
	#endif

}

#ifdef CONFIG_APP_APPLE_MFI_WAC
void init_mfi_wac()
{
	char wac_nameBuf[64]={0};
	char hostname_cmd[80]={0};

	apmib_get(MIB_MFI_WAC_DEVICE_NAME,	(void *)wac_nameBuf);
	if(wac_nameBuf[0]){
		sprintf(hostname_cmd,"hostname \'%s\'",wac_nameBuf);
		system(hostname_cmd);
	}
}
#endif
void set_wanif_down()
{
	int wisp_wan_id=0;
	apmib_get(MIB_WISP_WAN_ID,(void *)&wisp_wan_id);
	RunSystemCmd(NULL_FILE, "ifconfig", "eth1", "down", NULL_STR);
	RunSystemCmd(NULL_FILE, "ifconfig", "peth0", "down", NULL_STR);
#if defined(CONFIG_SMART_REPEATER)
	getWispRptIfaceName(wan_interface,wisp_wan_id);
	if(strstr(wan_interface,"-vxd"))
			RunSystemCmd(NULL_FILE, "ifconfig", wan_interface, "0.0.0.0", NULL_STR);
	RunSystemCmd(NULL_FILE, "ifconfig", wan_interface, "down", NULL_STR);
#endif

}

int init_module_common(int argc, char** argv,int reinit)
{	
	int opmode=0;
#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT) || defined(CONFIG_APP_APPLE_MFI_WAC)
		system("echo 1 > /var/system/start_init");
#endif	

//wan pptp/l2tp
#if defined(CONFIG_DYNAMIC_WAN_IP)
	//Added for pptp/l2tp use dynamic wan ip
	if(isFileExist(TEMP_WAN_CHECK))
		unlink(TEMP_WAN_CHECK);
	if(isFileExist(TEMP_WAN_DHCP_INFO))
		unlink(TEMP_WAN_DHCP_INFO);
	if(isFileExist(MANUAL_CONNECT_NOW))
		unlink(MANUAL_CONNECT_NOW);
#endif


	apmib_get(MIB_OP_MODE,(void *)&opmode);
	if(opmode==GATEWAY_MODE)
		RunSystemCmd("/var/sys_op", "echo", "0", NULL_STR);
	else if(opmode==BRIDGE_MODE)
		RunSystemCmd("/var/sys_op", "echo", "1", NULL_STR);
	else if(opmode==WISP_MODE)
		RunSystemCmd("/var/sys_op", "echo", "2", NULL_STR);

	generate_ifname(argc,argv,opmode);
	
	if(strcmp(argv[3], "all")==0 
		||strcmp(argv[3], "wan") ==0
		||strcmp(argv[3], "bridge")==0)
	{
		enable_wan=1;
		enable_br=1;
	}
	if(gateway!=1)//ap mode, must enable bridge
		enable_br=1;

#if defined(CONFIG_BAND_2G_ON_WLAN0) || defined(CONFIG_BAND_5G_ON_WLAN0)
		init_module_wlanBandMibSwitch();
#endif

	if(!(strcmp(argv[2], "ap")==0 && strcmp(argv[3], "wlan_app")==0)){	  
	   RunSystemCmd(NULL_FILE, "ifconfig", "br0", "down", NULL_STR);
	   RunSystemCmd(NULL_FILE, "ifconfig", "br0", "up", NULL_STR);
   }
	if(strcmp(argv[3], "wlan_app")==0){
		init_module_wlan_app(0);
		return 0;
	}

#ifdef CONFIG_APP_APPLE_MFI_WAC
	init_mfi_wac();
#endif

	if(!reinit || (reinit_flag & REINIT_FLAG_WAN)){
#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
		set_wanif_down();
#else
		if(reinit==1){		
#ifndef CONFIG_RTL_AP_PACKAGE
			set_wanif_down();
#endif
		}
#endif
	}
	return 1;
}


void init_module_nat_init()
{
	int opmode=0,wan_dhcp_mode=0;
	apmib_get(MIB_OP_MODE,(void *)&opmode);
#ifdef   HOME_GATEWAY
	apmib_get(MIB_WAN_DHCP,(void *)&wan_dhcp_mode);
#ifdef CONFIG_POCKET_AP_SUPPORT
#else
		if(isFileExist(HW_NAT_FILE)){/*hw nat supported*/
			/*cleanup hardware tables*/
			if(opmode==GATEWAY_MODE)
				RunSystemCmd(HW_NAT_FILE, "echo", "1", NULL_STR);	/*gateway mode*/
			else if(opmode==BRIDGE_MODE)
				RunSystemCmd(HW_NAT_FILE, "echo", "2", NULL_STR);	/*bridge mode*/
			else if(opmode==WISP_MODE)
				RunSystemCmd(HW_NAT_FILE, "echo", "3", NULL_STR);	/*wisp mode*/
			else if(opmode==3)
				RunSystemCmd(HW_NAT_FILE, "echo", "4", NULL_STR);	/*bridge mode with multiple vlan*/
			else
				RunSystemCmd(HW_NAT_FILE, "echo", "5", NULL_STR); /*wisp mode with multiple vlan*/
			
		}else{/*software nat supported*/ 
			if(isFileExist(SOFTWARE_NAT_FILE)){
			if(opmode==GATEWAY_MODE)
			{
#ifdef RTK_USB3G
				if(wan_dhcp_mode == USB3G)
			RunSystemCmd(SOFTWARE_NAT_FILE, "echo", "1", NULL_STR);
				else
#endif
					RunSystemCmd(SOFTWARE_NAT_FILE, "echo", "0", NULL_STR);
			}
			else if(opmode==BRIDGE_MODE)
				RunSystemCmd(SOFTWARE_NAT_FILE, "echo", "1", NULL_STR);
			else if(opmode==WISP_MODE)
				RunSystemCmd(SOFTWARE_NAT_FILE, "echo", "2", NULL_STR);
			else if(opmode==3)
				RunSystemCmd(SOFTWARE_NAT_FILE, "echo", "3", NULL_STR);
			else if(opmode==4)
				RunSystemCmd(SOFTWARE_NAT_FILE, "echo", "4", NULL_STR);
		}
			
		}
#endif	//CONFIG_POCKET_AP_SUPPORT
#endif	
}



void get_lan_mac(char * lan_mac)
{
	char tmpBuff[8]={0};
	apmib_get(MIB_ELAN_MAC_ADDR,  (void *)tmpBuff);
	if(!memcmp(tmpBuff, "\x00\x00\x00\x00\x00\x00", 6))
		apmib_get(MIB_HW_NIC0_ADDR,  (void *)tmpBuff);
	sprintf(lan_mac, "%02x%02x%02x%02x%02x%02x", (unsigned char)tmpBuff[0], (unsigned char)tmpBuff[1], 
		(unsigned char)tmpBuff[2], (unsigned char)tmpBuff[3], (unsigned char)tmpBuff[4], (unsigned char)tmpBuff[5]);
}

void init_lan_mac()
{
	char tmpBuff[64]={0},cmdBuffer[64]={0};
	int opmode=0;
	apmib_get(MIB_OP_MODE,(void *)&opmode);
	get_lan_mac(cmdBuffer);
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
	RunSystemCmd(NULL_FILE, "ifconfig", "eth0", "hw", "ether", cmdBuffer, NULL_STR);/*set eth0 mac address*/
	RunSystemCmd(NULL_FILE, "ifconfig", "eth2", "hw", "ether", cmdBuffer, NULL_STR);/*set eth0 mac address*/
	RunSystemCmd(NULL_FILE, "ifconfig", "eth3", "hw", "ether", cmdBuffer, NULL_STR);/*set eth0 mac address*/
	RunSystemCmd(NULL_FILE, "ifconfig", "eth4", "hw", "ether", cmdBuffer, NULL_STR);/*set eth0 mac address*/
#else
	if(br_lan1_interface[0])
		RunSystemCmd(NULL_FILE, "ifconfig", br_lan1_interface, "hw", "ether", cmdBuffer, NULL_STR);/*set eth0 mac address*/
#endif
	
	if(opmode == BRIDGE_MODE || opmode == WISP_MODE){
		apmib_get(MIB_ELAN_MAC_ADDR,  (void *)tmpBuff); // clone lan mac
		if(!memcmp(tmpBuff, "\x00\x00\x00\x00\x00\x00", 6))
		{
#if defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL_8197D_AP)
			apmib_get(MIB_HW_NIC0_ADDR,  (void *)tmpBuff);
#else
			apmib_get(MIB_HW_NIC1_ADDR,  (void *)tmpBuff);
#endif			
		}
		sprintf(cmdBuffer, "%02x%02x%02x%02x%02x%02x", (unsigned char)tmpBuff[0], (unsigned char)tmpBuff[1], 
			(unsigned char)tmpBuff[2], (unsigned char)tmpBuff[3], (unsigned char)tmpBuff[4], (unsigned char)tmpBuff[5]);	
		if(br_lan2_interface[0])	
			RunSystemCmd(NULL_FILE, "ifconfig", br_lan2_interface, "hw", "ether", cmdBuffer, NULL_STR);/*set eth1 mac address when bridge mode*/
	} 
}

#ifdef   HOME_GATEWAY	
void init_wan_mac()
{
	char tmpBuff[64]={0},cmdBuffer[64]={0};
	int opmode=0,old_wlan_idx=0,index=0;
	apmib_get(MIB_OP_MODE,(void *)&opmode);
	if(gateway == 1 && opmode != BRIDGE_MODE){
		apmib_get(MIB_WAN_MAC_ADDR,  (void *)tmpBuff); // clone wan mac
		if(!memcmp(tmpBuff, "\x00\x00\x00\x00\x00\x00", 6)){
			if(opmode == WISP_MODE)
			{
				apmib_get(MIB_WISP_WAN_ID, (void *)&index);
				old_wlan_idx=wlan_idx;
				wlan_idx = index;
				apmib_get(MIB_HW_WLAN_ADDR,  (void *)tmpBuff);
				wlan_idx=old_wlan_idx;
			}
			else
				apmib_get(MIB_HW_NIC1_ADDR,  (void *)tmpBuff);
		}
		sprintf(cmdBuffer, "%02x%02x%02x%02x%02x%02x", (unsigned char)tmpBuff[0], (unsigned char)tmpBuff[1], 
			(unsigned char)tmpBuff[2], (unsigned char)tmpBuff[3], (unsigned char)tmpBuff[4], (unsigned char)tmpBuff[5]);
		if(wan_interface[0])		
			RunSystemCmd(NULL_FILE, "ifconfig", wan_interface, "hw", "ether", cmdBuffer, NULL_STR);/*set wan mac address when it not bridge mode*/
#if defined(CONFIG_SMART_REPEATER)
			if(opmode == WISP_MODE && index == 0)
				RunSystemCmd(NULL_FILE, "ifconfig", "wlan0", "hw", "ether", cmdBuffer, NULL_STR);/*set wan mac address when it not bridge mode*/
			else if(opmode == WISP_MODE && index == 1)
				RunSystemCmd(NULL_FILE, "ifconfig", "wlan1", "hw", "ether", cmdBuffer, NULL_STR);/*set wan mac address when it not bridge mode*/
#endif
	}
}
#endif

void init_module_wlan(int reinit)
{
	int ulinker_auto=0,i=0,cmdRet=0,br_wlan_block=0,intValue=0;
	char tmpBuff[128]={0},tmp_args[128]={0};
	char *token=NULL,*savestr1=NULL;
	RunSystemCmd(NULL_FILE, "ifconfig", "pwlan0", "down", NULL_STR);
	
#if defined(CONFIG_RTL_ULINKER_WLAN_DELAY_INIT)
	apmib_get(MIB_ULINKER_AUTO,  (void *)&ulinker_auto);
	if (wlan_support != 0 && ulinker_auto == 0)
#else
	if (wlan_support != 0)
#endif
	{
		memset(wlan_interface, 0x00, sizeof(wlan_interface));
		for(i=0;i<NUM_WLAN_INTERFACE;i++)
		{
			int wlan_disable = 1;
			int wlan_blockrelay=0;
			unsigned char wlan_name[10];
			int mc2u_disabled = 0;
			memset(wlan_name,0x00,sizeof(wlan_name));
			sprintf(wlan_name, "wlan%d",i);
			apmib_save_wlanIdx();
			if(SetWlan_idx(wlan_name))
			{			
				apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disable);	  
#ifdef SHRINK_INIT_TIME
				if(wlan_disable == 1 && reinit == 1)
#else
				if(wlan_disable == 1)
#endif
				{
					RunSystemCmd(NULL_FILE, "ifconfig", wlan_name, "down", NULL_STR);
					RunSystemCmd(NULL_FILE, "iwpriv", wlan_name, "radio_off", NULL_STR);					
				}
				else
				{
	        if(wlan_interface[0]==0x00)
	            sprintf(wlan_interface, "%s", wlan_name);
	      	else
	        {
						sprintf(tmp_args, " %s", wlan_name);
		        strcat(wlan_interface, tmp_args); 
	        }	
						
					//printf("%s:%d wlan_name=%s\n",__FUNCTION__,__LINE__,wlan_name);
#if defined(CONFIG_SMART_REPEATER)
#ifdef SHRINK_INIT_TIME
					if(reinit == 1)
					{
#endif
					char vxd_interface[16]={0};
					sprintf(vxd_interface,"%s-vxd",wlan_name);
					//RunSystemCmd(NULL_FILE, "ifconfig", vxd_interface, "0.0.0.0", NULL_STR);
					RunSystemCmd(NULL_FILE, "ifconfig", vxd_interface, "down", NULL_STR);
#ifdef SHRINK_INIT_TIME
					}
#endif
					//printf("%s:%d vxd_interface=%s\n",__FUNCTION__,__LINE__,vxd_interface);
#endif
#ifdef SHRINK_INIT_TIME
					if(reinit == 1)
#endif
						RunSystemCmd(NULL_FILE, "ifconfig", wlan_name, "down", NULL_STR);
					cmdRet=RunSystemCmd(NULL_FILE, "flash", "set_mib", wlan_name, NULL_STR);
			
					if(cmdRet != 0)
					{
						printf("[%s %d]init %s failed!\n", __FUNCTION__, __LINE__ ,wlan_name);
						continue;
					}
					apmib_get(MIB_WLAN_MC2U_DISABLED, (void *)&mc2u_disabled); 
					if(mc2u_disabled)
					{
						//iwpriv wlan0 set_mib mc2u_disable=1
						RunSystemCmd(NULL_FILE, "iwpriv", wlan_name,"set_mib","mc2u_disable=1", NULL_STR);
					}
					else
					{
						//iwpriv wlan0 set_mib mc2u_disable=0
						RunSystemCmd(NULL_FILE, "iwpriv", wlan_name,"set_mib","mc2u_disable=0", NULL_STR);
					}
				
				}
				
				apmib_get( MIB_WLAN_BLOCK_RELAY,(void *)&wlan_blockrelay);
				/*if all wlan interface block then enable br_wlan_block*/
				if(wlan_blockrelay)
				{
					br_wlan_block++;
				}

			}
			apmib_recov_wlanIdx();
		}
		
		if(br_wlan_block == NUM_WLAN_INTERFACE)
		{
			RunSystemCmd("/proc/br_wlanblock", "echo","1",NULL_STR);
		}
		else
		{
			RunSystemCmd("/proc/br_wlanblock", "echo","0",NULL_STR);
		}
	}


	if(wlan_interface[0]){				
		if(wlan_vxd_interface[0]) {
			char		*strptr, *tokptr;
			memcpy(tmpBuff,wlan_vxd_interface,sizeof(wlan_vxd_interface));
			strptr=tmpBuff;
			token = strsep(&strptr," ");
			while(token!=NULL)
			{
#ifdef SHRINK_INIT_TIME
				if(reinit == 1)
#endif
					RunSystemCmd(NULL_FILE, "ifconfig", token, "down", NULL_STR);
				RunSystemCmd(NULL_FILE, "flash", "set_mib", token, NULL_STR);/*set vxd wlan iface*/
				token = strsep(&strptr," ");
			}
		}
		if(wlan_virtual_interface[0]){
			token=NULL;
			savestr1=NULL;
			sprintf(tmpBuff, "%s", wlan_virtual_interface);
			token = strtok_r(tmpBuff," ", &savestr1);
			do{
				if (token == NULL){
					break;
				}else{
#ifdef SHRINK_INIT_TIME
					if(reinit == 1)
					{
#endif
					RunSystemCmd(NULL_FILE, "ifconfig", token, "down", NULL_STR);
					RunSystemCmd(NULL_FILE, "flash", "set_mib", token, NULL_STR);/*set virtual wlan iface*/
#ifdef SHRINK_INIT_TIME
					}
					else
					{
					
						apmib_save_wlanIdx();
						SetWlan_idx(token);
						
						apmib_get(MIB_WLAN_WLAN_DISABLED, &intValue);
						if(intValue == 0)
						{
							RunSystemCmd(NULL_FILE, "flash", "set_mib", token, NULL_STR);/*set virtual wlan iface*/
						}
						apmib_recov_wlanIdx();
						
					}
#endif
				}
				token = strtok_r(NULL, " ", &savestr1);
			}while(token !=NULL);
		}
#if defined(CONFIG_SMART_REPEATER)
		if(strstr(wan_interface,"-vxd")){
#ifdef SHRINK_INIT_TIME
			if(reinit == 1)
#endif
				RunSystemCmd(NULL_FILE, "ifconfig", wan_interface, "down", NULL_STR);
			RunSystemCmd(NULL_FILE, "flash", "set_mib", wan_interface, NULL_STR);
		}
#endif
	}	
	else
		RunSystemCmd("/proc/gpio", "echo", "0", NULL_STR);
}
void gen_br_interfaces(unsigned char *brif)
{

	int opmode=-1;
	unsigned char tmpBuff[256]={0};
  apmib_get(MIB_OP_MODE,(void *)&opmode);
	
	if(vlan_interface[0]){
		strcat(tmpBuff, vlan_interface);
		strcat(tmpBuff, " ");
	}
	if(wlan_interface[0]){
		strcat(tmpBuff, wlan_interface);
		strcat(tmpBuff, " ");
	}			
	if(wlan_virtual_interface[0]){
		strcat(tmpBuff, wlan_virtual_interface);
		strcat(tmpBuff, " ");
	}
#if defined(CONFIG_RTL_8198_AP_ROOT) || defined(CONFIG_RTL_8197D_AP)
	if(br_lan2_interface[0]){
		strcat(tmpBuff, br_lan2_interface);
		strcat(tmpBuff, " ");
	}
#endif
	if(wlan_vxd_interface[0]){
		strcat(tmpBuff, wlan_vxd_interface);
		strcat(tmpBuff, " ");
	}
#if defined(CONFIG_RTK_MESH)
	if(wlan_mesh_interface[0]){
		strcat(tmpBuff, wlan_mesh_interface);
		strcat(tmpBuff, " ");
	}
#endif

	strcat(tmpBuff, br_interface);
	strcat(tmpBuff, " ");
#if defined(CONFIG_RTL_MULTI_LAN_DEV)
	if(opmode == GATEWAY_MODE)
	{
		strcat(tmpBuff, "eth0 eth2 eth3 eth4");
	}
	else
	{
		strcat(tmpBuff, "eth0 eth1 eth2 eth3 eth4");
	}	
#else
	
	strcat(tmpBuff, br_lan1_interface);

#if defined(CONFIG_RTL_IVL_SUPPORT)
	/*add eth1 to br0 when in bridge&wisp mode*/
	if(opmode == BRIDGE_MODE || opmode == WISP_MODE) 
	{
		strcat(tmpBuff, " ");
		strcat(tmpBuff, br_lan2_interface);
	}
#endif
	#if !defined(CONFIG_RTL_MULTI_LAN_DEV) && !defined(CONFIG_RTL_IVL_SUPPORT)
	/* rtk vlan enable and in bridge&wisp mode and disable ivl and disable multi lan, add eth1 to br0 */
	if (vlan_interface[0])
	{
		if(opmode == BRIDGE_MODE || opmode == WISP_MODE) 
		{
			strcat(tmpBuff, " ");
			strcat(tmpBuff, br_lan2_interface);
		}
	}
	#endif
#endif

#if defined(CONFIG_RTL_ULINKER)
	strcat(tmpBuff, " ");
	strcat(tmpBuff, "usb0");
#endif
	memcpy(brif, tmpBuff, sizeof(tmpBuff));
	return;

}
void clean_log()
{
	RunSystemCmd(NULL_FILE, "killall", "-9", "syslogd", NULL_STR);
	RunSystemCmd(NULL_FILE, "killall", "-9", "klogd", NULL_STR);
}
void start_log()
{
	int intValue=0,  intValue1=0;
	char tmpBuffer[32];
	char syslog_para[32];
	char localServer[32];
	
	apmib_get(MIB_SCRLOG_ENABLED, (void*)&intValue);
	if(intValue !=0 && intValue !=2 && intValue !=4 && intValue !=6 && intValue !=8 &&
		intValue !=10 && intValue !=12 && intValue !=14) {
			apmib_get(MIB_REMOTELOG_ENABLED, (void*)&intValue1);
			if(intValue1 != 0){
				apmib_get(MIB_REMOTELOG_SERVER,  (void *)tmpBuffer);
				if (memcmp(tmpBuffer, "\x0\x0\x0\x0", 4)){
#ifdef RINGLOG /* ring log */
					sprintf(localServer, "%s", inet_ntoa(*((struct in_addr *)tmpBuffer)));
					system("rm /var/log/log_split >/dev/null 2>&1");
					sprintf(tmpBuffer, "echo %d > /var/log/log_split", LOG_SPLIT);
					system(tmpBuffer);
					sprintf(tmpBuffer, "%d", MAX_LOG_SIZE);
					sprintf(syslog_para, "%d", LOG_SPLIT);
					fprintf(stderr, "syslog will use %dKB for log(%d rotate, 1 original, %dKB for each)\n",
						MAX_LOG_SIZE * (LOG_SPLIT+1), LOG_SPLIT, MAX_LOG_SIZE);
					RunSystemCmd(NULL_FILE, "syslogd", "-L","-R", localServer, "-s", tmpBuffer, "-b", syslog_para, NULL_STR);
#else
					sprintf(syslog_para, "%s", inet_ntoa(*((struct in_addr *)tmpBuffer)));
					RunSystemCmd(NULL_FILE, "syslogd", "-L", "-R", syslog_para, NULL_STR);
#endif					
				}
			}else{
			
//### add by sen_liu 2011.4.21 sync the system log update (enlarge from 1 pcs to 8 pcs) to	SDKv2.5 from kernel 2.4
#ifdef RINGLOG /* ring log */
				system("rm /var/log/log_split >/dev/null 2>&1");
				sprintf(tmpBuffer, "echo %d > /var/log/log_split", LOG_SPLIT);
				system(tmpBuffer);
				sprintf(tmpBuffer, "%d", MAX_LOG_SIZE);
				sprintf(syslog_para, "%d", LOG_SPLIT);
				fprintf(stderr, "syslog will use %dKB for log(%d rotate, 1 original, %dKB for each)\n",
					MAX_LOG_SIZE * (LOG_SPLIT+1), LOG_SPLIT, MAX_LOG_SIZE);
				RunSystemCmd(NULL_FILE, "syslogd", "-L", "-s", tmpBuffer, "-b", syslog_para, NULL_STR);
#else
//### end
				RunSystemCmd(NULL_FILE, "syslogd", "-L", NULL_STR);
#endif
			}
			RunSystemCmd(NULL_FILE, "klogd", NULL_STR);
		} 
}
void start_module_upnpd(int isgateway, int sys_op)
{
#ifdef   HOME_GATEWAY	
	int intValue=0,  intValue1=0;
	if(SetWlan_idx("wlan0")){
		apmib_get(MIB_WLAN_WSC_DISABLE, (void *)&intValue);
	}
	if(isgateway==1 && sys_op !=1)
		apmib_get(MIB_UPNP_ENABLED, (void *)&intValue1);
	else 
		intValue1=0;
	if(intValue==0 && intValue1==0){//wps enabled and upnp igd is disabled
		RunSystemCmd(NULL_FILE, "mini_upnpd", "-wsc", "/tmp/wscd_config", "-daemon", NULL_STR);
		//system("mini_upnpd -wsc /tmp/wscd_config &");
	}else if(intValue==1 && intValue1==1){//wps is disabled, and upnp igd is enabled
		RunSystemCmd(NULL_FILE, "mini_upnpd", "-igd", "/tmp/igd_config", "-daemon", NULL_STR);
		//system("mini_upnpd -igd /tmp/igd_config &");
	}else if(intValue==0 && intValue1==1){//both wps and upnp igd are enabled
		RunSystemCmd(NULL_FILE, "mini_upnpd", "-wsc", "/tmp/wscd_config", "-igd", "/tmp/igd_config","-daemon",  NULL_STR);
		//system("mini_upnpd -wsc /tmp/wscd_config -igd /tmp/igd_config &");
	}else if(intValue==1 && intValue1==0){//both wps and upnp igd are disabled
		/*do nothing*/
	}
#endif		
}

/*method to start reload is co-operate to parse rule of reload.c*/
void start_wlan_module_by_schedule(int index)
{
	int intValue=0,  intValue1=0, i=0, entryNum=0, bak_idx=0, bak_vidx=0;
	char tmp1[64]={0};
	SCHEDULE_T wlan_sched;
	int newfile=1;

	bak_idx=wlan_idx;
	wlan_idx=index;
	bak_idx=vwlan_idx;
	vwlan_idx=0;
	apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&intValue);
#if defined(NEW_SCHEDULE_SUPPORT)
	#define WLAN_SCHEDULE_FILE "/var/wlsch.conf"
	sprintf(tmp1,WLAN_SCHEDULE_FILE"%d",index);
	unlink(tmp1);
#endif	

	if(intValue==0){
		apmib_get(MIB_WLAN_SCHEDULE_ENABLED, (void *)&intValue1);
		apmib_get(MIB_WLAN_SCHEDULE_TBL_NUM, (void *)&entryNum);

		if(intValue1==1 && entryNum > 0){
			
			for (i=1; i<=entryNum; i++) {
				*((char *)&wlan_sched) = (char)i;
				apmib_get(MIB_WLAN_SCHEDULE_TBL, (void *)&wlan_sched);
#if defined(NEW_SCHEDULE_SUPPORT)

				char line_buffer[100]={0};
				if(wlan_sched.eco == 1 && !(wlan_sched.fTime == 0 && wlan_sched.tTime == 0))
				{
					sprintf(line_buffer,"%d,%d,%d,%d\n",i,wlan_sched.day,wlan_sched.fTime, wlan_sched.tTime);
					sprintf(tmp1,WLAN_SCHEDULE_FILE"%d",index);
					write_line_to_file(tmp1, (newfile==1?1:2), line_buffer);
					newfile = 2;
				}
#endif				
			}
			if(index == (NUM_WLAN_INTERFACE-1)){
#if defined(NEW_SCHEDULE_SUPPORT)
				sprintf(tmp1, "reload -k %s &", WLAN_SCHEDULE_FILE);
#else			
				sprintf(tmp1, "reload -e %d,%d,%d,%d,%s &", wlan_sched.eco, wlan_sched.fTime, wlan_sched.tTime, wlan_sched.day, wlan_sched.text);
#endif			
				system(tmp1);
			}
		}
		else{ /* do not care schedule*/

			if(index == (NUM_WLAN_INTERFACE-1)){
#if defined(NEW_SCHEDULE_SUPPORT)
				sprintf(tmp1, "reload -k %s &", WLAN_SCHEDULE_FILE);
				system(tmp1);
#else
				system("reload &");
#endif
			}
		}
	}
	else{
		/*wlan is disabled, we do not care wlan schedule*/
		if(index ==(NUM_WLAN_INTERFACE-1)){
#if defined(NEW_SCHEDULE_SUPPORT)
			sprintf(tmp1, "reload -k %s &", WLAN_SCHEDULE_FILE);
			system(tmp1);
#else
			system("reload &");
#endif
		}
	}

	vwlan_idx=bak_vidx;
	wlan_idx=bak_idx;
}

void set_lan_dhcp()
{
	int lan_dhcp_mode=0,intValue1=0,intValue=0,i=0;
	char Ip[32]={0}, Mask[32]={0}, Gateway[32]={0};
	char tmpBuff[32]={0};
	char tmp_args[16]={0};
	apmib_get(MIB_DHCP,(void *)&lan_dhcp_mode);
	if(lan_dhcp_mode==DHCP_LAN_NONE){		/*DHCP disabled*/
		apmib_get( MIB_IP_ADDR,  (void *)tmpBuff);
		sprintf(Ip, "%s", inet_ntoa(*((struct in_addr *)tmpBuff)));
		apmib_get( MIB_SUBNET_MASK,  (void *)tmpBuff);
		sprintf(Mask, "%s", inet_ntoa(*((struct in_addr *)tmpBuff)));
		apmib_get(MIB_DEFAULT_GATEWAY,  (void *)tmpBuff);
		
		if (!memcmp(tmpBuff, "\x0\x0\x0\x0", 4))
			memset(Gateway, 0x00, sizeof(Gateway));
		else
			sprintf(Gateway, "%s", inet_ntoa(*((struct in_addr *)tmpBuff)));
			
		RunSystemCmd(NULL_FILE, "ifconfig", br_interface, Ip, "netmask", Mask, NULL_STR);
		
		if(Gateway[0]){
			RunSystemCmd(NULL_FILE, "route", "del", "default", br_interface, NULL_STR);
			RunSystemCmd(NULL_FILE, "route", "add", "-net", "default", "gw", Gateway, "dev", br_interface, NULL_STR);
		}	
	}else
		if(lan_dhcp_mode==DHCP_LAN_SERVER //dhcp disabled or server mode or auto
#ifdef CONFIG_DOMAIN_NAME_QUERY_SUPPORT		
		|| lan_dhcp_mode==DHCP_AUTO
#endif
		)			
		{		/*DHCP server enabled*/
			intValue1=0;
			for(i=0;i<NUM_WLAN_INTERFACE;i++){
				sprintf(tmp_args, " wlan%d", i);
				if(SetWlan_idx(tmp_args)){
					apmib_get(MIB_WLAN_WDS_ENABLED, (void *)&intValue);
					//printf("wds enabled=%d\n",intValue);
					if(intValue!=0)
						intValue1=intValue1+5;
					else
						intValue1=intValue1+1;
				}	
			}
#ifndef SHRINK_INIT_TIME
			sleep(intValue1);/*wait wlan wds init */		
#endif
			if(gateway==1)
				set_lan_dhcpd(br_interface, 2);
			else
				set_lan_dhcpd(br_interface, 1);
				
		}
}
void init_module_lanapp_misc(int reinit)
{
	int lan_dhcp_mode=0,opmode=0,intValue=0;
	apmib_get(MIB_DHCP,(void *)&lan_dhcp_mode);
	apmib_get(MIB_OP_MODE,(void*)&opmode);
#if defined(CONFIG_APP_SIMPLE_CONFIG)
	if(gateway==1 && lan_dhcp_mode==DHCP_LAN_NONE)
		system("echo 1 > /var/sc_ip_status");
#endif
#if defined(CONFIG_APP_APPLE_MFI_WAC)		
		// IOT dhcp client device need to give up br0 ip here , since br0 ip will be decide in IOT networking framework (dhcpc+avahi)
		// it is implement in wfaudio package 
		if(gateway!=1)
			if(lan_dhcp_mode==DHCP_LAN_NONE || lan_dhcp_mode==DHCP_LAN_CLIENT)
			{
				//printf("give up br0 ip !!!\n");
				system("ifconfig br0 0.0.0.0");		
			}		
#endif	

}

void clean_lan_dhcpd()
{
	int pid=0;
	char strPID[16]={0};
	if(isFileExist(DHCPD_PID_FILE)){
		pid=getPid_fromFile(DHCPD_PID_FILE);
		if(pid != 0){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-16", strPID, NULL_STR);/*inform dhcp server write lease table to file*/
			sleep(1);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
		}
		unlink(DHCPD_PID_FILE);
	}
}
void init_module_lan_dhcp(int reinit)
{
	clean_lan_dhcpd();
	/* init lan dhcp setting*/
	if(enable_br==1)
		set_lan_dhcp();		

	init_module_lanapp_misc(reinit);	
}


void init_module_lan(int reinit)
{
	char tmpBuff[256]={0};
	int lan_dhcp_mode=0,opmode=0;
	apmib_get(MIB_OP_MODE,(void *)&opmode);
	apmib_get(MIB_DHCP,(void *)&lan_dhcp_mode);
	init_lan_mac();
	if(enable_br==1){
		gen_br_interfaces(tmpBuff);			
		//fprintf(stderr,"\r\n br_interface=[%s],__[%s-%u]\r\n",tmpBuff,__FILE__,__LINE__);	
		setbridge(tmpBuff);
		#if defined(CONFIG_IPV6) && !defined(CONFIG_APP_APPLE_MFI_WAC)
		setbridgeipv6();
		#endif
	}
	
}
/* 
* dhcpc daemon pid file name must be /etc/udhcpc/udhcpc-*.pid
*/
int killDhcpcDaemons()
{
	DIR *dir;
	struct dirent *next;
	char dhcpPidPath[256]={"/etc/udhcpc/"};

	dir = opendir("/etc/udhcpc");
	if (!dir) 
	{
		printf("Cannot open /etc/udhcpc");
		return -1;
	}
	
	while ((next = readdir(dir)) != NULL)
	{
		if(strncmp("udhcpc-",next->d_name,strlen("udhcpc-"))==0
		&& strncmp(".pid",next->d_name+strlen(next->d_name)-strlen(".pid"),strlen(".pid"))==0)
		{//begin with "udhcpc-" end with ".pid" 
			strcpy(dhcpPidPath,"/etc/udhcpc/");
			strcat(dhcpPidPath,next->d_name);
			killDaemonByPidFile(dhcpPidPath);
		}
	}
	closedir(dir);
	return 0;
}
#ifdef CONFIG_IPV6
void clean_wanipv6()
{
	int pid=0;
	char strPID[16]={0};
	killDaemonByPidFile(DHCP6S_PID_FILE);
	RunSystemCmd(NULL_FILE, "rm", "-f", DHCP6S_CONF_FILE, NULL_STR);
	
	if(isFileExist(DHCP6C_PID_FILE)) {
		pid=getPid_fromFile(DHCP6C_PID_FILE);
		if(pid){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-16", strPID, NULL_STR);/*inform dhcp server write lease table to file*/
			sleep(1);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);						
		}
		unlink(DHCP6C_PID_FILE);
	}
	RunSystemCmd(NULL_FILE, "rm", "-f", DHCP6C_CONF_FILE, NULL_STR);
	
	killDaemonByPidFile(DNSV6_PID_FILE);
	RunSystemCmd(NULL_FILE, "rm", "-f", DNSV6_CONF_FILE, NULL_STR);
	
	killDaemonByPidFile(RADVD_PID_FILE);
	RunSystemCmd(NULL_FILE, "rm", "-f", RADVD_CONF_FILE, NULL_STR);
	RunSystemCmd(NULL_FILE, "rm", "-f", DNSV6_ADDR_FILE, NULL_STR);
	
	killDaemonByPidFile(ECMH_PID_FILE);
	killDaemonByPidFile(MLDPROXY_PID_FILE);

	RunSystemCmd(NULL_FILE, "rm", "-f", DHCP6PD_CONF_FILE, NULL_STR);
#ifdef CONFIG_MAP_E_SUPPORT
		if(isFileExist(MAP_E_FILE))
		{
			RunSystemCmd(NULL_FILE, "ivictl", "-q", NULL_STR);
			RunSystemCmd(NULL_FILE, "rmmod", "/lib/modules/ivi.ko", NULL_STR);
			RunSystemCmd(NULL_FILE, "rm", "-f", MAP_E_FILE, NULL_STR);
		}
#endif

}
#endif
void clean_module_wan()
{
	char tmpBuff[128]={0},strPID[32]={0};
	int pid=0,sys_opmode=0;
	apmib_get(MIB_OP_MODE,(void*)&sys_opmode);

#ifdef CONFIG_APP_MINIIGD	
		RunSystemCmd(NULL_FILE, "killall", "-15", "miniigd", NULL_STR);
#elif defined(CONFIG_APP_MINIUPNPD)
		RunSystemCmd(NULL_FILE, "killall", "-15", "miniupnpd", NULL_STR); //miniupnpd
#endif
		if(isFileExist(IGD_PID_FILE)){
			unlink(IGD_PID_FILE);
		}

		RunSystemCmd(NULL_FILE, "killall", "-15", "routed", NULL_STR);
		if(isFileExist(RIP_PID_FILE)){
			unlink(RIP_PID_FILE);
		}
#ifdef RIP6_SUPPORT
		RunSystemCmd(NULL_FILE, "killall", "-15", "bird6", NULL_STR);
#endif
		RunSystemCmd(NULL_FILE, "rm", "-f", "/etc/ppp/first", NULL_STR);
		RunSystemCmd(NULL_FILE, "rm", "-f", "/etc/ppp/firstpptp", NULL_STR);
		RunSystemCmd(NULL_FILE, "rm", "-f", "/etc/ppp/firstl2tp", NULL_STR);
		RunSystemCmd(NULL_FILE, "rm", "-f", "/etc/ppp/firstdemand", NULL_STR);

		wan_disconnect("all");
		RunSystemCmd(NULL_FILE, "killall", "-9", "ntp_inet", NULL_STR);	

	killDaemonByPidFile(L2TPD_PID_FILE);
	/* kill dhcp client */
	killDhcpcDaemons();
	sprintf(tmpBuff, "/etc/udhcpc/udhcpc-%s.pid", "br0");
	if(isFileExist(tmpBuff)){
		unlink(tmpBuff);
	}
	killDaemonByPidFile(DNRD_PID_FILE);

	killDaemonByPidFile(IGMPPROXY_PID_FILE);
	if(isFileExist(IGMPPROXY_PID_FILE)){
			RunSystemCmd(PROC_BR_MCASTFASTFWD, "echo", "1,1", NULL_STR);
	}
	
#if defined(CONFIG_IPV6) && !defined(CONFIG_APP_APPLE_MFI_WAC)
	clean_wanipv6();
#endif
#if defined(CONFIG_POCKET_ROUTER_SUPPORT)
	/* Reset Firewall Rules*/
	system("iptables -F");
	system("iptables -F -t nat");
	system("iptables -F -t mangle");
	sprintf(tmpBuff, "iptables -A INPUT -i %s -j ACCEPT", br_interface);
	system(tmpBuff);
	if(sys_opmode==1){
		system("iptables -P INPUT ACCEPT");
		system("iptables -P FORWARD ACCEPT");
	}
#endif	
#ifdef CONFIG_APP_OPENVPN
	if(find_pid_by_name("openvpn")>0)
		system("killall openvpn > /dev/null 2>&1");
#endif

}
void init_module_wan(int reinit)
{
	int opmode=0,intValue=0,wan_dhcp_mode=0,wisp_wan_id=0,pid=0;
	char strPID[32]={0};
#ifdef CONFIG_DOMAIN_NAME_QUERY_SUPPORT
	int wlan_mode_root=0;
#endif
	apmib_get(MIB_OP_MODE,(void *)&opmode);
	apmib_get(MIB_WAN_DHCP,(void *)&wan_dhcp_mode);
	apmib_get(MIB_WISP_WAN_ID,(void *)&wisp_wan_id);

	if(reinit)
		clean_module_wan();

		init_wan_mac();
		
		sprintf(strPID, "%d", wan_dhcp_mode);
		RunSystemCmd("/var/system/last_wan", "echo", strPID, NULL_STR);
#ifdef SHRINK_INIT_TIME
	if( opmode!=BRIDGE_MODE || reinit ==1 ){
#endif
		RunSystemCmd(NULL_FILE, "iptables", "-F", NULL_STR);
		RunSystemCmd(NULL_FILE, "iptables", "-F", "-t", "nat",  NULL_STR);
		RunSystemCmd(NULL_FILE, "iptables", "-A", "INPUT", "-j", "ACCEPT", NULL_STR);
		
#ifdef CONFIG_DOMAIN_NAME_QUERY_SUPPORT		
	if(SetWlan_idx("wlan0")){
		apmib_get( MIB_WLAN_MODE, (void *)&wlan_mode_root); 
	}
		/* start dnrd for check dns query with hostname */
		domain_query_start_dnrd(wlan_mode_root, 1);
#endif		

		
		RunSystemCmd(NULL_FILE, "rm", "-f", "/var/eth1_ip", NULL_STR);
		RunSystemCmd(NULL_FILE, "rm", "-f", "/var/ntp_run", NULL_STR);
		//RunSystemCmd("/proc/pptp_src_ip", "echo", "0 0", NULL_STR);
		if(wan_interface[0])
		{
#ifdef SHRINK_INIT_TIME
				if( reinit==1 )
#endif
					RunSystemCmd(NULL_FILE, "ifconfig", wan_interface, "down", NULL_STR);
			RunSystemCmd(NULL_FILE, "ifconfig", wan_interface, "up", NULL_STR);
			}
#ifdef SHRINK_INIT_TIME
		}
#endif
	if(enable_wan==1 && (opmode == GATEWAY_MODE || opmode==WISP_MODE)){/*for init internet wan setting*/ 
			if(opmode==WISP_MODE)
			{
//printf("\r\n wan_interface=[%s],__[%s-%u]\r\n",wan_interface,__FILE__,__LINE__);				
				if(SetWlan_idx(wan_interface)){
					apmib_get(MIB_WLAN_ENCRYPT, (void *)&intValue);
					if(intValue != 0){
#if 0 //why wlan0 is wisp_wan interface and encrypt is not none, than wlan1's keep_rsnie will also be set as 1?????
						for(i=0;i<NUM_WLAN_INTERFACE;i++)
						{
							sprintf(tmp_args, " wlan%d", i);
							RunSystemCmd(NULL_FILE, "iwpriv", tmp_args, "set_mib", "keep_rsnie=1", NULL_STR);
						}
#else
						RunSystemCmd(NULL_FILE, "iwpriv", wan_interface, "set_mib", "keep_rsnie=1", NULL_STR);
#endif						
					}
				}
			}
			RunSystemCmd(PROC_FASTNAT_FILE, "echo", "1", NULL_STR);
			
			#if defined(CONFIG_RTL_FAST_IPV6)
			RunSystemCmd(PROC_FASTIPV6_FILE, "echo", "1", NULL_STR);
			#endif
			
			if(wan_dhcp_mode==PPTP){
				RunSystemCmd(PROC_FASTPPTP_FILE, "echo", "1", NULL_STR);
				apmib_get(MIB_PPTP_CONNECTION_TYPE, (void *)&intValue);
				if(intValue==1){
					RunSystemCmd(PROC_PPTP_CONN_FILE, "echo", "3", NULL_STR);
				}else{
					RunSystemCmd(PROC_PPTP_CONN_FILE, "echo", "0", NULL_STR);
				}
			}else{
				RunSystemCmd(PROC_FASTPPTP_FILE, "echo", "0", NULL_STR);
			}
				
			if(wan_dhcp_mode==L2TP){
				RunSystemCmd(PROC_FASTL2TP_FILE, "echo", "1", NULL_STR);
			}else{
				RunSystemCmd(PROC_FASTL2TP_FILE, "echo", "0", NULL_STR);
			}

			if(wan_dhcp_mode == PPPOE) {
				RunSystemCmd(PROC_FASTPPPOE_FILE, "echo", "1", NULL_STR);
			} else {
				RunSystemCmd(PROC_FASTPPPOE_FILE, "echo", "0", NULL_STR);
			}
			if((wan_dhcp_mode !=DHCP_SERVER && wan_dhcp_mode < 7) || (wan_dhcp_mode == USB3G) || (wan_dhcp_mode == AFTR) || (wan_dhcp_mode == DHCP_NONE)){ /* */
				start_wan(wan_dhcp_mode, opmode, wan_interface, br_interface, wisp_wan_id, 1);
			}else
				printf("Invalid wan type:wan_dhcp_mode=%d\n", wan_dhcp_mode);
		}
		else if(enable_wan==1 && opmode == BRIDGE_MODE ){
			/*Bridge mode, eth1 mtu should be sync with eth0 mtu (assume that eth0 mtu is 1500 here!!!)
			Otherwise, ping large pkt failed when GW mode changed to Bridge mode
			if eth1 mtu not equal with eth0 mtu.*/
			if(wan_interface[0])
				RunSystemCmd(NULL_FILE, "ifconfig", wan_interface, "mtu", "1500", NULL_STR);

			#if defined(CONFIG_RTL_FAST_IPV6)
			RunSystemCmd(PROC_FASTIPV6_FILE, "echo", "0", NULL_STR);
			#endif
		}

		//set kthreadd high priority for performance
		RunSystemCmd(NULL_FILE, "renice -20 2", NULL_STR);
		//set ksoftirqd high priority for performance
		RunSystemCmd(NULL_FILE, "renice -20 3", NULL_STR);
		//set boa high priority
		if(isFileExist(WEBS_PID_FILE)){
			pid=getPid_fromFile(WEBS_PID_FILE);
			if(pid != 0){
				sprintf(strPID, "%d", pid);
				RunSystemCmd(NULL_FILE, "renice", "-20", strPID, NULL_STR);
			}
		}
	
#if defined(CONFIG_IPV6) && !defined(CONFIG_APP_APPLE_MFI_WAC)
		system("ip tunnel del tun 2> /dev/null");
		
		set_wanipv6();
#endif
}

void init_passthrough()
{
	int opmode=0,intValue=0,wispWanId=0,repeater_enable1=0,repeater_enable2=0;
	apmib_get(MIB_OP_MODE,(void*)&opmode);
#ifdef HOME_GATEWAY // To enable/disable ipv6 passthru no matter wan is connected or not
	if(opmode == GATEWAY_MODE)	// Gateway mode
		apmib_get(MIB_CUSTOM_PASSTHRU_ENABLED, (void *)&intValue);
	else
		intValue=0;

	RunSystemCmd("/proc/custom_Passthru", "echo", (intValue & 0x1)?"1":"0", NULL_STR);
	if (intValue == 0)
	{
		RunSystemCmd(NULL_FILE, "brctl", "delif", "br0", "peth0", NULL_STR);
		RunSystemCmd(NULL_FILE, "ifconfig", "peth0", "down", NULL_STR);
	}
	else
	{
		RunSystemCmd(NULL_FILE, "brctl", "addif", "br0", "peth0", NULL_STR);
		RunSystemCmd(NULL_FILE, "ifconfig", "peth0", "up", NULL_STR);
	}
#if defined(CONFIG_RTL_92D_SUPPORT)
	if(opmode == WISP_MODE)
	{
		apmib_get(MIB_WISP_WAN_ID, (void *)&wispWanId);
		apmib_get(MIB_CUSTOM_PASSTHRU_ENABLED, (void *)&intValue);		
		apmib_get(MIB_REPEATER_ENABLED1,(void *)&repeater_enable1);
		apmib_get(MIB_REPEATER_ENABLED2,(void *)&repeater_enable2);
		if(intValue != 0)
		{
			char tmpStr[16];
			/*should also config wisp wlan index for dual band wireless interface*/
			intValue=((wispWanId&0xF)<<4)|intValue;
			if(repeater_enable1==1 || repeater_enable2==1)
				intValue = intValue | 0x8;
			memset(tmpStr,0,sizeof(tmpStr));
			sprintf(tmpStr,"%d",intValue);
				
			RunSystemCmd("/proc/custom_Passthru_wlan", "echo", tmpStr, NULL_STR);
			//RunSystemCmd("/proc/custom_Passthru_wlan", "echo", (intValue & 0x1)?"1":"0", NULL_STR);
		}
		else
		{
			RunSystemCmd("/proc/custom_Passthru_wlan", "echo", (intValue & 0x1)?"1":"0", NULL_STR);
		}
	}
#else
	if(opmode == WISP_MODE) //WISP mode, wireless enabled  and wireless client mode enabled
	{		
		apmib_get(MIB_REPEATER_ENABLED1,(void *)&repeater_enable1);

		apmib_get(MIB_CUSTOM_PASSTHRU_ENABLED, (void *)&intValue);
		if(intValue !=0)
		{
			char tmpStr[16];
			if(repeater_enable1==1)
				intValue |= 0x8;
			memset(tmpStr,0,sizeof(tmpStr));
			sprintf(tmpStr,"%d",intValue);
			RunSystemCmd("/proc/custom_Passthru_wlan", "echo", tmpStr, NULL_STR);
		}
		else
		{
			RunSystemCmd("/proc/custom_Passthru_wlan", "echo", (intValue & 0x1)?"1":"0", NULL_STR);
		}
	}
	else
	{
	intValue=0;
	RunSystemCmd("/proc/custom_Passthru_wlan", "echo", (intValue & 0x1)?"1":"0", NULL_STR);
	}
#endif
	
	if (intValue == 0)
	{
		RunSystemCmd(NULL_FILE, "brctl", "delif", "br0", "pwlan0", NULL_STR);
		RunSystemCmd(NULL_FILE, "ifconfig", "pwlan0", "down", NULL_STR);
	}
	else
	{
		RunSystemCmd(NULL_FILE, "brctl", "addif", "br0", "pwlan0", NULL_STR);
		RunSystemCmd(NULL_FILE, "ifconfig", "pwlan0", "up", NULL_STR);
	}
#endif
}

void init_igmp()
{
	int intValue=0,opmode=0;
	apmib_get(MIB_OP_MODE,(void*)&opmode);
	/*enable igmp snooping*/
	/*igmp snooping is independent with igmp proxy*/
#if defined (CONFIG_IGMPV3_SUPPORT)
	RunSystemCmd(PROC_BR_IGMPVERSION, "echo", "3", NULL_STR);
#else
	RunSystemCmd(PROC_BR_IGMPVERSION, "echo", "2", NULL_STR);
#endif
	RunSystemCmd(PROC_BR_IGMPSNOOP, "echo", "1", NULL_STR);
	if(opmode == BRIDGE_MODE)
		RunSystemCmd(PROC_BR_IGMPQUERY, "echo", "1", NULL_STR);
	
#if defined (CONFIG_RTL_MLD_SNOOPING)	
	RunSystemCmd(PROC_BR_MLDSNOOP, "echo", "1", NULL_STR);
	if(opmode == BRIDGE_MODE)
		//default disable mldquery. mld query packets may leads ping and throughput down periodically???
		RunSystemCmd(PROC_BR_MLDQUERY, "echo", "1", NULL_STR);
#endif

	intValue =0;
	apmib_get(MIB_IGMP_FAST_LEAVE_DISABLED, (void *)&intValue);
	if(intValue)
	{
		RunSystemCmd(PROC_BR_IGMPSNOOP, "echo", "fastleave","0","2", NULL_STR);
	}
	else
	{	
		RunSystemCmd(PROC_BR_IGMPSNOOP, "echo", "fastleave","1","0", NULL_STR);
	}
	
#ifdef SUPPORT_ZIONCOM_RUSSIA
	RunSystemCmd("/proc/sys/net/ipv4/conf/eth1/force_igmp_version", "echo", "2", NULL_STR);
#endif
}

#ifdef RTK_USB3G
#if defined(CONFIG_4G_LTE_SUPPORT)
int lte_wan(void)
{
	int lte = -1, wan_dhcp = -1;
	apmib_get( MIB_LTE4G,    (void *)&lte);
	apmib_get( MIB_WAN_DHCP, (void *)&wan_dhcp);

	if (wan_dhcp == DHCP_CLIENT && lte == 1) {
		return 1;
	}

	return 0;
}
#endif /* #if defined(CONFIG_4G_LTE_SUPPORT) */
void start_usb3g()
{
	int intValue=0,opmode=0;
	apmib_get(MIB_OP_MODE,(void*)&opmode);
	 apmib_get(MIB_WAN_DHCP, (void*)&intValue);
	if (opmode == 0 && (intValue == USB3G
#ifdef CONFIG_4G_LTE_SUPPORT 
	|| lte_wan()
#endif
	)) {
        system("echo \"/sbin/hotplug\" > /proc/sys/kernel/hotplug");
        system("mount -t sysfs none /sys           >/dev/null 2>&1");
    #ifdef KERNEL_3_10
        //system("mount -t devtmpfs none /dev           >/dev/null 2>&1");
        system("mount -t debugfs none /sys/kernel/debug >/dev/null 2>&1");
    #else
        system("mount -t usbfs none /proc/bus/usb  >/dev/null 2>&1");
        system("mount -t tmpfs none /dev           >/dev/null 2>&1");
    #endif
        system("mdev -s                            >/dev/null 2>&1");
        system("echo \"remove\" > /var/usb3g.stat");
        system("mnet -d &");
    }
}
#endif
#ifdef CONFIG_APP_APPLE_MFI_WAC
void clean_apple_mfi_wac()
{
	system("killall wfaudio");
	system("killall WACServer");
#if defined(CONFIG_APPLE_HOMEKIT)
	system("killall hapserver");
#endif
}
void start_apple_mfi_wac()
{
	int lan_dhcp_mode=0;
	apmib_get(MIB_DHCP,(void*)&lan_dhcp_mode);
	if(isFileExist("/var/system/mdnsd_started")==0){
		if(lan_dhcp_mode==DHCP_LAN_NONE || lan_dhcp_mode==DHCP_LAN_CLIENT)
		{
			system("ifconfig br0 0.0.0.0");		
		}		
		system("mdnsd");
		system("echo 1 > /var/system/mdnsd_started");
	}
	system ("wfaudio &"); //some brach start in rcs
}
#endif
#ifdef CONFIG_CPU_UTILIZATION
void clean_cpu_utilization()
{
	if(isFileExist(CPU_PID_FILE)){
		system("killall cpu >/dev/null 2>&1");
		system("rm -f  "CPU_PID_FILE" 2> /dev/null");		
		system("rm -f  "CPU_LOAD_FILE" 2> /dev/null");
	}
}
void start_cpu_utilization()
{
	int enable_cpu=0;
	apmib_get(MIB_ENABLE_CPU_UTILIZATION, (void *)&enable_cpu);
	if(enable_cpu)
	{
		//RunSystemCmd(NULL_FILE, "killall", "cpu", NULL_STR);
		if(isFileExist(CPU_PID_FILE)==0)
		{
			RunSystemCmd(NULL_FILE, "cpu", NULL_STR);		
		}
	}
	else
	{
		if(isFileExist(CPU_PID_FILE)!=0)
		{
			RunSystemCmd(NULL_FILE, "killall", "cpu", NULL_STR);
//			RunSystemCmd(NULL_FILE, "rm", "-f", "/web/*.cpudat", NULL_STR);
			system("rm -f  "CPU_PID_FILE" 2> /dev/null");
		}
	}
}
#endif
#if defined(CONFIG_APP_WEAVE)
void start_weave()
{
	int weave_registered = 0;
 	//printf("start dbus-daemon\n");
    system("dbus-daemon --system");
   // printf("start avahi-daemon\n");
    system("avahi-daemon &");
    sleep(3);
    
    apmib_get(MIB_WEAVE_REGISTERED,(void *)&weave_registered);
    if(weave_registered){
        unsigned char weave_config[MAX_WEAVE_CONFIG_LENGTH]={0};
        FILE * fp=NULL;
        printf("start weave-daemon\n");
        system("mkdir /var/lib/weave 2>/dev/null");
        fp=fopen(WEAVE_FILE_PATH,"w+");
        if(!fp)
        {
            printf("open %s fail!\n",WEAVE_FILE_PATH);
            return;
        }
        apmib_get(MIB_WEAVE_CONFIG,weave_config);

        fwrite(weave_config,strlen(weave_config),1,fp);
        fclose(fp);
        sync();
    }
    system("weave_daemon &");
}
#endif
#if defined(CONFIG_APP_TR069) && defined(CONFIG_RTL_HTTP_REDIRECT_TR098)
void tr069_httpRedirect_init()
{
	int cap_enable = 0,capEntryNum=0,i=0;
	CAP_PORTAL_T capEntry={0};
	char cap_url[CWMP_CAP_PORTAL_URL_LEN];
	char cmdBuffer[256]={0};
	apmib_get(MIB_CAPTIVEPORTAL_ENABLE,(void *)&cap_enable);
	sprintf(cmdBuffer,"echo %d > /proc/http_redirect/enable",cap_enable);
	system(cmdBuffer);
	apmib_get(MIB_CAPTIVEPORTAL_URL,(void *)cap_url);
	sprintf(cmdBuffer,"echo %s > /proc/http_redirect/CaptivePortalURL",cap_url);
	system(cmdBuffer);
	apmib_get(MIB_CAP_PORTAL_ALLOW_TBL_NUM, (void *)&capEntryNum);
	system("echo flush > /proc/http_redirect/AllowedList");
	for (i=1; i<=capEntryNum; i++) 
	{
		*((char *)&capEntry) = (char)i;
		if(!apmib_get(MIB_CAP_PORTAL_ALLOW_TBL, (void *)&capEntry))
		{
			printf("get mib MIB_CAP_PORTAL_ALLOW_TBL fail!\n");
			return 0;
		}
		sprintf(cmdBuffer,"echo add %s > /proc/http_redirect/AllowedList",capEntry.ipAddr);
		system(cmdBuffer);
	}	
}
#endif

void init_module_startup_app()
{
	
}
void init_module_common_app()
{
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
	init_module_8021x();
#endif

#ifdef CONFIG_RTL_WAPI_SUPPORT
	init_modue_wapi();
#endif
}
void init_module_app(int reinit)
{
	int opmode=0,intValue=0,lan_dhcp_mode=0,pid=0,index=0;
	apmib_get(MIB_OP_MODE,(void*)&opmode);
	apmib_get(MIB_DHCP,(void*)&lan_dhcp_mode);

	if(!reinit)
		init_module_startup_app();//run once, not killed deamon
		
	init_module_common_app(); //always need run

#ifdef CONFIG_RTL_DNS_TRAP
	init_module_dns_trap();
#endif

#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
	set_Domain_Query_settings();
#endif
	clean_log();
	start_log();
#ifndef STAND_ALONE_MINIUPNP
	RunSystemCmd(NULL_FILE, "killall", "-9", "mini_upnpd", NULL_STR);
	start_module_upnpd(gateway, opmode);
#endif

	killDaemonByPidFile(LLTD_PID_FILE);
	
	if(gateway==1 && opmode != BRIDGE_MODE){
		if(isFileExist(LLTD_PROCESS_FILE)){
			RunSystemCmd(NULL_FILE, "lld2d", br_interface, NULL_STR);
		}
	}
	
	if(isFileExist(SNMPD_PROCESS_FILE)){
		RunSystemCmd(NULL_FILE, "snmpd.sh", "restart", NULL_STR);
		//RunSystemCmd(NULL_FILE, "snmpd", "-c", SNMPD_CONF_FILE, "-p", SNMPD_PID_FILE,  NULL_STR);
	}
	
	if(isFileExist(NMSD_PROCESS_FILE)){
		RunSystemCmd(NULL_FILE, "nmsd", NULL_STR);
	}

	RunSystemCmd(NULL_FILE, "killall", "-9", "reload", NULL_STR);
	for(index=0; index<NUM_WLAN_INTERFACE; index++)
		start_wlan_module_by_schedule(index);

	init_passthrough();

	init_igmp();
	
#ifdef CONFIG_RTL_HIGH_PERFORMANCE_FILESYSTEM
	load_rtl_fs_module();
#endif
	
#if defined(CONFIG_APP_USBMOUNT)
	start_mount();

#if defined (CONFIG_APP_VSFTPD)
	start_vsftpd();
#endif	

#if defined (CONFIG_APP_SAMBA)
	start_samba();
#endif

#ifdef CONFIG_APP_MINIDLNA
	start_lan_minidlna();
#endif

#endif


#if defined(CONFIG_APP_DLNA_DMS)
	//printf("%s, %d\n", __FUNCTION__, __LINE__);
	system("ushare -f /etc/ushare.conf &");
#endif

#ifdef RTK_USB3G
  	start_usb3g();
#endif /* #ifdef RTK_USB3G */

#if defined(HTTP_FILE_SERVER_SUPPORTED)
	RunSystemCmd("/proc/http_file/getLanIp", "echo", "1", NULL_STR);
#endif

#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT) || defined(CONFIG_APP_APPLE_MFI_WAC)
	system("rm -f  /var/system/start_init 2> /dev/null");
#endif
	
#ifdef CONFIG_APP_APPLE_MFI_WAC
	clean_apple_mfi_wac();
	start_apple_mfi_wac();
#endif

#if defined(CONFIG_APP_FWD)
	RunSystemCmd(NULL_FILE, "killall", "-9", "fwd", NULL_STR); //LZQ
#ifdef SHRINK_INIT_TIME
		if(reinit == 0)
#endif
			system("fwd &");
#endif

	//reply only if the target IP address is local address configured on the incoming interface
	RunSystemCmd("/proc/sys/net/ipv4/conf/eth1/arp_ignore", "echo", "1", NULL_STR);
	/*increase routing cache rebuild count from 4 to 2048*/
	RunSystemCmd(RT_CACHE_REBUILD_COUNT, "echo", "2048", NULL_STR);

	
	RunSystemCmd(NULL_FILE, "killall", "-9", "timelycheck", NULL_STR); //LZQ
#ifdef SHRINK_INIT_TIME
	if(opmode != BRIDGE_MODE)
#endif
		system("timelycheck &");

#if defined(CONFIG_AUTO_DHCP_CHECK)
	RunSystemCmd(NULL_FILE, "killall", "-9", "Auto_DHCP_Check", NULL_STR);
	if(opmode==BRIDGE_MODE && lan_dhcp_mode == DHCP_SERVER)
	{
		system("Auto_DHCP_Check &");
	}
#endif	
	
#ifdef CONFIG_CPU_UTILIZATION
	clean_cpu_utilization();
	start_cpu_utilization();	
#endif


#if defined(CONFIG_APP_TR069)
		clean_tr069();
	if (find_pid_by_name("cwmpClient") == 0)
		start_tr069();
#endif


#if defined(CONFIG_RTL_ETH_802DOT1X_SUPPORT)
	init_EthDot1x(wan_dhcp_mode, wan_dhcp_mode, wan_interface, br_interface);
#endif

#ifdef RTK_CAPWAP
	capwap_app();
#endif

#if defined(CONFIG_APP_RTK_INBAND_CTL)
	RunSystemCmd(NULL_FILE, "killall", "-9", "hcd", NULL_STR);

	RunSystemCmd(PROC_INBAND_CTL_ACL, "echo", "0x8899", NULL_STR);
	system("hcd -daemon &");
#endif

#if defined(CONFIG_SYSFS)
	rtl_mount_sysfs();
#endif

#if defined(CONFIG_RPS)
	rtl_configRps();
#endif
#if defined(CONFIG_APP_ZIGBEE)
	RunSystemCmd(NULL_FILE, "killall", "-9", "zigbeed", NULL_STR);
        //printf("start zigbeed\n");
    system("zigbeed &");
        //system("zigbeed permit_join");
#endif
#if defined(CONFIG_APP_WEAVE)
	start_weave();
       
#endif  
#if defined(CONFIG_APP_USBMOUNT)
	rtl_config_read_ahead();
#endif
	
#ifdef CONFIG_RTK_EVENTD_SUPPORT
	if(isFileExist("/var/run/rtk_eventd.pid"))
		system("killall rtk_eventd > /dev/null 2>&1");		
	system("rtk_eventd");
#endif

#if defined(CONFIG_APP_TR069) && defined(CONFIG_RTL_HTTP_REDIRECT_TR098)
	tr069_httpRedirect_init();	
#endif
}

void init_module_wlan_setWlanIfUp()
{
	int opmode=0,intVal=0,wlan_mode=0,wlan_disabled=0,wisp_wan_id=0;
#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
	int vlan_wan_bridge_enable=0,vlan_wan_bridge_port=0;
#endif
	int wlan0_mesh_enabled=0, wlan1_mesh_enabled=0,iswlan_va=0;
	char *token=NULL,*savestr1=NULL;
	unsigned char wlanIfs[256]={0};
	char iface_name[16]={0};
	char wlan_wan_iface[16]={0};
  	apmib_get(MIB_OP_MODE,(void *)&opmode);
	apmib_get(MIB_WISP_WAN_ID,(void *)&wisp_wan_id);
#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT	
	apmib_get( MIB_VLAN_WAN_BRIDGE_PORT, (void *)&vlan_wan_bridge_port);
	apmib_get( MIB_VLAN_WAN_BRIDGE_ENABLE, (void *)&vlan_wan_bridge_enable);
#endif

	if(wlan_interface[0]){
		strcat(wlanIfs, wlan_interface);
		strcat(wlanIfs, " ");
	}			
	if(wlan_virtual_interface[0]){
		strcat(wlanIfs, wlan_virtual_interface);
		strcat(wlanIfs, " ");
	}

	if(wlan_vxd_interface[0]){
		strcat(wlanIfs, wlan_vxd_interface);
		strcat(wlanIfs, " ");
	}
#if defined(CONFIG_RTK_MESH)
	if(wlan_mesh_interface[0]){
		strcat(wlanIfs, wlan_mesh_interface);
		strcat(wlanIfs, " ");
	}
#endif
	if(isFileExist(MESH_PATHSEL)){
        SetWlan_idx("wlan0");
        apmib_get(MIB_WLAN_MODE, (void *)&wlan_mode);
        apmib_get(MIB_WLAN_MESH_ENABLE,(void *)&wlan0_mesh_enabled);    
        if(wlan_mode != AP_MESH_MODE && wlan_mode != MESH_MODE) {
            wlan0_mesh_enabled = 0;
        }
    
#if defined(FOR_DUAL_BAND)
        SetWlan_idx("wlan1");
        apmib_get(MIB_WLAN_MODE, (void *)&wlan_mode);
        apmib_get(MIB_WLAN_MESH_ENABLE,(void *)&wlan1_mesh_enabled);
        if(wlan_mode != AP_MESH_MODE && wlan_mode != MESH_MODE){
            wlan1_mesh_enabled = 0;
        }
#endif
        
	}


	
//printf("\r\n tmpBuff=[%s],__[%s-%u]\r\n",tmpBuff,__FILE__,__LINE__);
	
	token = strtok_r(wlanIfs," ", &savestr1);
	do{
		if(token == NULL){/*check if the first arg is NULL*/
			break;
		}else
		{
	    sprintf(iface_name,"%s", token);
	    
//printf("\r\n iface_name=[%s],__[%s-%u]\r\n",iface_name,__FILE__,__LINE__);

	    
			if(strncmp(iface_name, "wlan", 4)==0){//wlan iface
				if (strlen(iface_name) >= 9 && iface_name[5] == '-' &&
						iface_name[6] == 'v' && iface_name[7] == 'a')
				{
					char wlanRootName[16];
					memset(wlanRootName, 0x00, sizeof(wlanRootName));
					strncpy(wlanRootName,iface_name, 5);

					if(SetWlan_idx(wlanRootName)){
						apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&intVal);//get root if enable/disable
						apmib_get( MIB_WLAN_MODE, (void *)&wlan_mode); //get root if mode
						if(intVal==0)
						{
							if(wlan_mode != AP_MODE && wlan_mode != AP_WDS_MODE)
								wlan_disabled=1;//root if is disabled
							else
							{
								if(SetWlan_idx( iface_name))
								{
									apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&intVal);//get va if enable/disable
									if(intVal==0)
#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
									{
										wlan_disabled=0;
										if(opmode == GATEWAY_MODE && vlan_wan_bridge_enable)
										{
											wlan_disabled=0;
									
											if(vwlan_idx)//vap
											{
                      							if(vlan_wan_bridge_port&(1<<(6+vwlan_idx)))
													wlan_disabled=2;
											}
										}
									}
#else
										wlan_disabled=0;
#endif
									else
										wlan_disabled=1;
								}else
										wlan_disabled=1;
							}
						}else
							wlan_disabled=1;
					}else
						wlan_disabled=1;//root if is disabled
				} 
                else if(strstr(iface_name, "msh")) { /*search for wlan-msh, wlan0-msh0 or wlan1-msh0*/
                    //for mesh interfaces
                    if(wlan0_mesh_enabled || wlan1_mesh_enabled)                    
                    {
                        wlan_disabled=0;
                    }
                    else {
                        wlan_disabled=1;
                    }

				} else {
					if(SetWlan_idx( iface_name)){
						apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&intVal);
						wlan_disabled=intVal;
					}else
						wlan_disabled=1;
				}
				if(wlan_disabled==0){ //wlan if is enabled					
					sprintf(wlan_wan_iface,"wlan%d", wisp_wan_id);
#if defined(CONFIG_SMART_REPEATER)
					getWispRptIfaceName(wlan_wan_iface,wisp_wan_id);
					//strcat(wlan_wan_iface,"-vxd");
#endif					
					if(strlen(iface_name) >= 9 && iface_name[5] == '-' && iface_name[6] == 'v' && iface_name[7] == 'a')
						iswlan_va=1;

					if((iswlan_va==1) || (opmode != WISP_MODE) || (strcmp(wlan_wan_iface, iface_name) != 0) )
					{
						RunSystemCmd(NULL_FILE, "ifconfig", iface_name, "0.0.0.0", NULL_STR);
					}
                    else{
						RunSystemCmd(NULL_FILE, "ifconfig", iface_name, "up", NULL_STR);
					}
					if(SetWlan_idx( iface_name)){
						int wlan_wds_enabled=0,wlan_wds_num=0,j=0;
						char tmp_iface[16]={0};
						apmib_get( MIB_WLAN_WDS_ENABLED, (void *)&wlan_wds_enabled);
						apmib_get( MIB_WLAN_WDS_NUM, (void *)&wlan_wds_num);
						apmib_get( MIB_WLAN_MODE, (void *)&wlan_mode); //get wlan if mode
						if(wlan_wds_enabled !=0 && wlan_wds_num !=0 && (wlan_mode==WDS_MODE || wlan_mode==AP_WDS_MODE)){//add wds inface to br0
							for(j=0;j<wlan_wds_num;j++){
								sprintf(tmp_iface, "%s-wds%d", iface_name, j);
								RunSystemCmd(NULL_FILE, "ifconfig", tmp_iface, "0.0.0.0", NULL_STR);								
							}
						}
					}

					
				}
				//printf("[%s] [%s] [%s]\n",iface_name,bridge_iface,bridge_iface2);
			}
		}
		token = strtok_r(NULL, " ", &savestr1);
	}while(token !=NULL);

	
#if defined(CONFIG_SMART_REPEATER)
			if(strstr(wan_interface,"-vxd")){
				RunSystemCmd(NULL_FILE, "ifconfig", wan_interface, "up", NULL_STR);
			}
#endif
}

int setinit(int argc, char** argv)
{
	int reinit=0;
	if(isFileExist(REINIT_FILE))
			reinit=1;

	apmib_get(MIB_MODULES_REINIT_FLAG,(void*)&reinit_flag);
	printf("reinit_flag = 0x%x\n",reinit_flag);
	if(reinit_flag&REINIT_FLAG_SYSTEM)
		printf("reinit REINIT_FLAG_SYSTEM\n");
	if(reinit_flag&REINIT_FLAG_NAT)
		printf("reinit REINIT_FLAG_NAT\n");
	if(reinit_flag&REINIT_FLAG_WLAN)
		printf("reinit REINIT_FLAG_WLAN\n");
	if(reinit_flag&REINIT_FLAG_LAN)
		printf("reinit REINIT_FLAG_LAN\n");
	if(reinit_flag&REINIT_FLAG_LAN_DHCP)
		printf("reinit REINIT_FLAG_LAN_DHCP\n");
	if(reinit_flag&REINIT_FLAG_WLAN_APP)
		printf("reinit REINIT_FLAG_WLAN_APP\n");
	if(reinit_flag&REINIT_FLAG_WAN)
		printf("reinit REINIT_FLAG_WAN\n");
	if(reinit_flag&REINIT_FLAG_APP)
		printf("reinit REINIT_FLAG_APP\n");

	if(reinit && reinit_flag==0)
		return 0;
	
	if(!reinit)
		init_modules_startup_init();//start up init, run once
	else
		init_module_reinit_run();//only reinit run
   if(init_module_common(argc,argv,reinit)==0)//common init
		return 0;

	if(!reinit || (reinit_flag & REINIT_FLAG_LAN)){
		  //it will take about 7s for LAN PC to send DHCP request packet, down eth0 early
		  RunSystemCmd(NULL_FILE, "ifconfig", "eth0", "down", NULL_STR);
	}

	if(!reinit || (reinit_flag & REINIT_FLAG_SYSTEM))
		init_module_set_time();
	if(!reinit || (reinit_flag & REINIT_FLAG_NAT))
		init_module_nat_init();

	if(!reinit || (reinit_flag & REINIT_FLAG_WLAN))
		init_module_wlan(reinit);
	if(!reinit || (reinit_flag & REINIT_FLAG_LAN))
		init_module_lan(reinit);
	if((reinit_flag & REINIT_FLAG_WLAN)&&(!(reinit_flag&REINIT_FLAG_LAN)))
		init_module_wlan_setWlanIfUp();
	if(!reinit || (reinit_flag & REINIT_FLAG_LAN_DHCP))
		init_module_lan_dhcp(reinit);
	if(!reinit || (reinit_flag & REINIT_FLAG_WLAN_APP))
		init_module_wlan_app(v_wlan_app_enabled);
	
#ifdef   HOME_GATEWAY
	if(!reinit || (reinit_flag & REINIT_FLAG_WAN))
		init_module_wan(reinit);
#endif

	if(!reinit || (reinit_flag & REINIT_FLAG_APP))
		init_module_app(reinit);

	if(isFileExist(REINIT_FILE)==0)
			RunSystemCmd(REINIT_FILE, "echo", "1", NULL_STR);
	reinit_flag=0;
	apmib_set(MIB_MODULES_REINIT_FLAG,(void*)&reinit_flag);
	apmib_update(CURRENT_SETTING);
	return 1;
}
