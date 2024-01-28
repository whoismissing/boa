/* 
 */
 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "sysconf.h"
#include "sys_utility.h"
#include "apmib.h"
#include "mibtbl.h"

#define NTPTMP_FILE "/tmp/ntp_tmp"
#define TZ_FILE "/etc/TZ"

static int isDaemon=0;

int daemon_already_running()
{
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <errno.h>
	#define PID_FILE "/var/run/ntp_inet.pid"
		int pidFd;
		char pidBuf[16]={0};
		char *pid_file = PID_FILE;
		struct flock lock;

		pidFd=open(pid_file,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
		if(pidFd<0)
		{
			fprintf(stderr, "could not create pid file: %s \n", pid_file);
			exit(1);
		}
		
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_len = 0;
		lock.l_whence = SEEK_SET;
		if(fcntl(pidFd,F_SETLK,&lock)<0)
		{
			if(errno==EACCES || errno==EAGAIN)
			{
				close(pidFd);				
				return 1;
			}
			fprintf(stderr, "can't lock %s:%s \n", pid_file,strerror(errno));
			exit(1);
		}
		
		ftruncate(pidFd,0);

		sprintf(pidBuf, "%ld\n", (long)getpid());
		write(pidFd,pidBuf,strlen(pidBuf)+1);
		close(pidFd);
		return 0;
}


int main(int argc, char *argv[])
{
	int i,id=0,ntp_conf_num=0;
	unsigned char ntp_server[NTP_SERVER_NUM][40]={0};
	//unsigned char command[100];
	unsigned short fail_wait_time = 60;
	unsigned int succ_wait_time = 86400;
	//unsigned char daylight_save_str[5];
	
	unsigned int ntp_ip_version = 0; /*default set to IPv4 for ntpclient: 0-IPv4, 1-IPv6, 2-IPv4&IPv6*/
	#ifdef CONFIG_IPV6
	ntp_ip_version = 2; /* 2: IPv4&IPv6*/
	#endif
	
	for(i=1; i<argc; i++)
	{
		if(argv[i][0]!='-')
		{
			sprintf((char *)ntp_server[id], "%s", argv[i]);
			ntp_conf_num++;
			id++;
		}
		else 
			switch(argv[i][1])
			{
				case 'x':
					isDaemon = 1;
					break;
				case 'u':
					succ_wait_time = atoi(argv[++i]);
					
					if (succ_wait_time == 0) {
						succ_wait_time = 86400;
						fprintf(stderr, "[Warn]set succ_wait_time %d s,change to 86400 s!\n", succ_wait_time);
					}
					break;
				default:
					fprintf(stderr, "%s: Unknown option\n", argv[i]);
			}
	}

	//sprintf((char *)command, "%s", argv[3]);
	//sprintf((char *)daylight_save_str, "%s", argv[4]);
	
	if(isDaemon==1){
		if (daemon(0, 1) == -1) {
			perror("ntp_inet fork error");
			return 0;
		}
	}
	
	if(daemon_already_running())
	{
		fprintf(stderr, "ntp_inet daemon_already_running!\n");
		exit(1);
	}
	id = 0;
	for (;;) {
		int ret=1;
		
		unsigned char cmdBuffer[100];
		FILE *fp_timeStatus = NULL;

		if (id == ntp_conf_num) {
			id = 0;
			sleep(fail_wait_time);
		}

		RunSystemCmd(NULL_FILE, "rm", "/tmp/ntp_tmp", NULL_STR);
		RunSystemCmd(NULL_FILE, "rm", "/tmp/timeStatus", NULL_STR);
		fp_timeStatus=fopen("/tmp/timeStatus","w+");
		if(fp_timeStatus == NULL)//fix converity error: NULL_RETURNS
		{
			fprintf(stderr, "error to open file:/tmp/timeStatus.FILE:%s;LINE:%d\n",__FILE__,__LINE__);
			exit(1);
		}
		//RunSystemCmd(NULL_FILE, "rm", "/var/TZ", NULL_STR);

		//ret = RunSystemCmd(NTPTMP_FILE, "ntpclient", "-s", "-h", ntp_server, "-i", "5", ">", NULL_STR);
		sprintf((char *)cmdBuffer, "ntpclient -s -h %s -i 5 -P %d > %s", ntp_server[id], ntp_ip_version, NTPTMP_FILE);
		system((char *)cmdBuffer);
		
		if(isFileExist(NTPTMP_FILE))
		{
			FILE *fp=NULL;	
			unsigned char ntptmp_str[100];
			memset(ntptmp_str,0x00,sizeof(ntptmp_str));
			
			fp=fopen(NTPTMP_FILE, "r");
			if(fp!=NULL){
				fgets((char *)ntptmp_str,sizeof(ntptmp_str),fp);
				fclose(fp);

				if(strlen((char *)ntptmp_str) != 0)
				{
					
					// success

					//RunSystemCmd(TZ_FILE, "echo", command, NULL_STR);
					/*if(strcmp((char *)daylight_save_str, "1") == 0)
					{
						sprintf((char *)cmdBuffer,"date > %s",  NTPTMP_FILE);
						system((char *)cmdBuffer);
						//RunSystemCmd(NTPTMP_FILE, "date", ">", NULL_STR);
					}*/
					fputs("2",fp_timeStatus);  //eTStatusSynchronized
					fclose(fp_timeStatus);
					RunSystemCmd(NULL_FILE, "echo", "ntp client success", NULL_STR);
					if(succ_wait_time != 0)
						sleep(succ_wait_time);
					
				}
				else
				{
					//RunSystemCmd(NULL_FILE, "echo", "ntp client fail", NULL_STR);
					fputs("1",fp_timeStatus); //eTStatusUnsynchronized
					fclose(fp_timeStatus);
					id++;
					sleep(5);				
				}									
			}
			else
			{
				fputs("3",fp_timeStatus); //eTStatusErrorFailed
				fclose(fp_timeStatus);
				RunSystemCmd(NULL_FILE, "echo", "Can't connect ntp server!!", NULL_STR);
				id++;
				sleep(5);
				//sleep(fail_wait_time);
			}
		}
		else
		{
			//RunSystemCmd(NULL_FILE, "echo", "Can't create ntp tmp file!!", NULL_STR);
			fputs("3",fp_timeStatus);
			fclose(fp_timeStatus);
			id++;
			sleep(5);
			//sleep(fail_wait_time);
		}
		
	}
	
	
	return 0;
}



