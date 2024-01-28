/* 
 */
 
#include <stdio.h>
#include <unistd.h>
#include "sysconf.h"
#include "sys_utility.h"
    
#define FIRSTDDNS "/var/firstddns"

static int isDaemon=0;



int main(int argc, char *argv[])
{
	int i=1;
	unsigned char command[100];
	//unsigned short fail_wait_time = 300;
	//unsigned int succ_wait_time = 86400;
	unsigned char ddns_type[10];
	unsigned char ddns_domanin_name[51];
	unsigned char ddns_user_name[51];
	unsigned char ddns_password[51];
	unsigned char ddns_port[16];

	if (argc < 5 || argc > 7) {
		fprintf(stderr, "Usage: ddns_inet [-x] TYPE USER_NAME PASSWORD DOMAIN_NAME [PORT]\n");
		return -1;
	}
	
	if(argv[i][0] == '-' && argv[i][1] == 'x') {
		isDaemon = 1;
		i++;
	}
	sprintf((char *)ddns_type, "%s", argv[i++]);
	sprintf((char *)ddns_user_name, "%s", argv[i++]);
	sprintf((char *)ddns_password, "%s", argv[i++]);
	sprintf((char *)ddns_domanin_name, "%s", argv[i++]);

	if(argc > i)
		sprintf((char *)ddns_port, "%s", argv[i]);
	else
		memset(ddns_port, 0 ,sizeof(ddns_port));
	
	sprintf((char *)command, "%s:%s", ddns_user_name, ddns_password);
	
	if(isDaemon==1){
		if (daemon(0, 1) == -1) {
			perror("ntp_inet fork error");
			return 0;
		}
	}
	
	RunSystemCmd(FIRSTDDNS, "echo", "pass", NULL_STR);
	for (;;) {
		//unsigned char cmdBuffer[100];
		int ret;
		
		if(strncmp(ddns_type,"oray",strlen("oray")) == 0)
			ret = RunSystemCmd(NULL_FILE, "updatedd", ddns_type, ddns_user_name, 
			ddns_password, NULL_STR);
		else
			ret = RunSystemCmd(NULL_FILE, "updatedd", ddns_type, command, ddns_domanin_name, ddns_port, NULL_STR);

		if(ret == 0) // success
		{
			RunSystemCmd(NULL_FILE, "echo", "DDNS update successfully", NULL_STR);
			sleep(86430);
		}
		else // fail
		{
			sleep(300);
		}
		
	}
	
	
	return 0;
}



