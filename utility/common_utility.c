#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>

#include <net/if.h>
#include <stddef.h>
#include <net/if_arp.h>
#include <linux/if_ether.h>
#include <dirent.h>
#ifdef CONFIG_FWD_UNIFIED_FRAMEWORK
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>	
#endif
#include "common_utility.h"
#include "../apmib/apmib.h"

int DoCmd(char *const argv[], char *file)
{    
	pid_t pid;
	int status;
	int fd;
	char _msg[30];
	switch (pid = fork()) 
	{
			case -1:	/* error */
				perror("fork");
				return errno;
			case 0:	/* child */
				
				signal(SIGINT, SIG_IGN);
				if(file){
					if((fd = open(file, O_RDWR | O_CREAT))==-1){ /*open the file */
						sprintf(_msg, "open %s", file); 
  						perror(_msg);
  						exit(errno);
					}
					dup2(fd,STDOUT_FILENO); /*copy the file descriptor fd into standard output*/
					dup2(fd,STDERR_FILENO); /* same, for the standard error */
					close(fd); /* close the file descriptor as we don't need it more  */
				}else{
				#ifndef SYS_DEBUG		
					close(2); //do not output error messages
				#endif	
				}
				setenv("PATH", "/sbin:/bin:/usr/sbin:/usr/bin", 1);
				execvp(argv[0], argv);
				perror(argv[0]);
				exit(errno);
			default:	/* parent */
				{
					
					waitpid(pid, &status, 0);
					#ifdef SYS_DEBUG	
					if(status != 0)
						printf("parent got child's status:%d, cmd=%s %s %s\n", status, argv[0], argv[1], argv[2]);
					#endif		
					if (WIFEXITED(status)){
					#ifdef SYS_DEBUG	
						printf("parent will return :%d\n", WEXITSTATUS(status));
					#endif		
						return WEXITSTATUS(status);
					}else{
						
						return status;
					}
				}
	}
}

int RunSystemCmd(char *filepath, ...)
{
	va_list argp;
	char *argv[24]={0};
	int status;
	char *para;
	int argno = 0;
	va_start(argp, filepath);
    #ifdef DISPLAY_CMD
	printf("\n"); 
    #endif
	while (1){ 
		para = va_arg( argp, char*);
		if ( strcmp(para, "") == 0 )
			break;
		argv[argno] = para;
        #ifdef DISPLAY_CMD
		printf(" %s ", para); 
        #endif
		argno++;
	} 
    #ifdef DISPLAY_CMD    
	printf("\n");     
    #endif
	argv[argno+1] = NULL;
	status = DoCmd(argv, filepath);
	va_end(argp);
	return status;
}

int isFileExist(char *file_name)
{
	struct stat status;

	if ( stat(file_name, &status) < 0)
		return 0;

	return 1;
}
int setPid_toFile(char* file_name)
{
	FILE *pidfp;
	int pid = getpid();
	if ((pidfp = fopen(file_name, "w")) != NULL) {
		fprintf(pidfp, "%d\n", pid);
		fclose(pidfp);
		return 1;
	}
	return -1;
}
int getPid_fromFile(char *file_name)
{
	FILE *fp;
	char *pidfile = file_name;
	int result = -1;
	
	fp= fopen(pidfile, "r");
	if (!fp) {
        	printf("can not open:%s\n", file_name);
		return -1;
   	}
	if(fscanf(fp,"%d",&result) == -1)//fix converity error:CHECKED_RETURN
	{
		printf("error to get result from %s. FILE:%s LINE:%d\n", file_name,__FILE__,__LINE__);
		fclose(fp);
		return -1;
	}
	fclose(fp);
	
	return result;
}
int killDaemonByPidFile(char *pidFile)
{
	char strPID[10];
	int pid=-1;
	
	if(!pidFile)
	{
		printf("%s : input file name is null!\n",__FUNCTION__);
		return -1;
	}
	if(isFileExist(pidFile))
	{
		pid=getPid_fromFile(pidFile);
		if(pid != 0){
			sprintf(strPID, "%d", pid);
			RunSystemCmd(NULL_FILE, "kill", "-9", strPID, NULL_STR);
		}
		unlink(pidFile);
		return 0;
	}else
	{
		//printf("%s : %s file is not exist!\n",__FUNCTION__,pidFile);
		return -1;
	}
	return 0;
}
pid_t find_pid_by_name( char* pidName)
{
	DIR *dir;
	struct dirent *next;
	pid_t pid;
	
	if ( strcmp(pidName, "init")==0)
		return 1;
	
	dir = opendir("/proc");
	if (!dir) {
		printf("Cannot open /proc");
		return 0;
	}

	while ((next = readdir(dir)) != NULL) {
		FILE *status;
		char filename[READ_BUF_SIZE];
		char buffer[READ_BUF_SIZE];
		char name[READ_BUF_SIZE];

		/* Must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, "..") == 0)
			continue;

		/* If it isn't a number, we don't want it */
		if (!isdigit(*next->d_name))
			continue;

		sprintf(filename, "/proc/%s/status", next->d_name);
		if (! (status = fopen(filename, "r")) ) {
			continue;
		}
		if (fgets(buffer, READ_BUF_SIZE-1, status) == NULL) {
			fclose(status);
			continue;
		}
		fclose(status);

		/* Buffer should contain a string like "Name:   binary_name" */
		sscanf(buffer, "%*s %s", name);
		if (strcmp(name, pidName) == 0) {
		//	pidList=xrealloc( pidList, sizeof(pid_t) * (i+2));
			pid=(pid_t)strtol(next->d_name, NULL, 0);
			closedir(dir);
			return pid;
		}
	}	
	closedir(dir);
	return 0;
}
int get_file_size(char* filename)
{
	struct stat statbuf={0};
	if(!filename) return -1;
	if(stat(filename,&statbuf)<0)
		return -1;
	return (int)statbuf.st_size;
}

int write_line_to_file(char *filename, int mode, char *line_data)
{
	unsigned char tmpbuf[512];
	int fh=0;
	char* oldData=NULL;
	int fsize;

	if(access(filename,0)!=0)//file not exist!add to new file
	{
		mode=ADD_NEW_LINE_TO_NEW_FILE;
	}
	
	if(mode == ADD_NEW_LINE_TO_NEW_FILE) {/* write line datato file */
		
		fh = open(filename, O_RDWR|O_CREAT|O_TRUNC);
		
	}else if(mode == ADD_NEW_LINE_TO_END){/*append line data to file*/
		fh = open(filename, O_RDWR|O_APPEND);	
	}else if(mode == ADD_NEW_LINE_TO_HEAD){ // write line data to the beginning of file
	//first, save old file data
		fsize=get_file_size(filename);		
		if(fsize<0) { 
			fprintf(stderr, "get_file_size fail!\n"); 
			return 0;
		}
		oldData=malloc(fsize);
		bzero(oldData,fsize);
		fh = open(filename, O_RDWR);
		if(fh<0){
			fprintf(stderr, "open %s error!\n", filename);
			return 0;
		}
		if(read(fh,oldData,fsize)<0){
			fprintf(stderr, "read %s error!\n", filename);
			return 0;
		}
		close(fh);
	//then, begin to write new line
		fh=open(filename, O_RDWR|O_CREAT|O_TRUNC);
	}else{
		return 0;
	}
	
	
	if (fh < 0) {
		fprintf(stderr, "Create %s error!\n", filename);
		return 0;
	}


	sprintf((char *)tmpbuf, "%s", line_data);
	write(fh, tmpbuf, strlen((char *)tmpbuf));

	if(mode==ADD_NEW_LINE_TO_HEAD){
	//last, write old data
		write(fh,oldData,fsize);
		free(oldData);
	}

	close(fh);
	return 1;
}

int get_dns_ip(char *wan_ifname, struct in_addr *dns_ip, int *dns_num)
{
	FILE *fp = NULL;
	char dns_info_file[64]={0};
	char buffer[64]={0};
	char dns_buffer[16]={0};
	int i=0;

#ifdef MULTI_WAN_SUPPORT
	int wan_idx=getWanIdx(wan_ifname);	
	if(wan_idx<1 || wan_idx>WANIFACE_NUM)
	{
		printf("\nwan index is valid!\n");
		return -1;
	}

	WANIFACE_T WanIfaceEntry;	
	if(!getWanIfaceEntry(wan_idx, &WanIfaceEntry))
	{
		printf("\nget wan interface entry fail!\n");
		return -1;
	}		
	snprintf(dns_info_file, sizeof(dns_info_file), "/var/resolv.conf_%d_%d",wan_idx,WanIfaceEntry.applicationtype);
#else	
	if(strncmp(wan_ifname, "eth8", 4)==0 || strncmp(wan_ifname, "ppp8", 4)==0) //for upvel or skyworth retalor two wan
		strcpy(dns_info_file, "/var/tr069_resolv.conf");
	else
		strcpy(dns_info_file, "/var/resolv.conf");
#endif

	if((fp = fopen(dns_info_file, "r"))==NULL)
	{	
		printf("\nopen file %s fail!\n",dns_info_file);
		return -1;
	}

	while(fgets(buffer, sizeof(buffer), fp) !=NULL && (i<MAX_DNS_NUM))
	{
		if(strstr(buffer, "nameserver"))
		{
			strcpy(dns_buffer, buffer+strlen("nameserver "));
			inet_aton(dns_buffer, &dns_ip[i++]);
		}	
	}	

	*dns_num=i;
	
	fclose(fp);
	return (i>0 ? 0 : -1);
}

static dnsheader_t *alloc_packet(void *packet, int len)
{
    dnsheader_t *x;

    x = malloc(sizeof(dnsheader_t));
    memset(x, 0, sizeof(dnsheader_t));

    x->packet = malloc(len + 2);
    x->len    = len;
    memcpy(x->packet, packet, len);

    return (x);
}

int free_packet(dnsheader_t *x)
{
    if(x->packet!=NULL)
	{
        free(x->packet);
        x->packet=NULL;
    }
    if(x!=NULL)
    {
	    free(x);
        x=NULL;
    }
	return (0);
}

static dnsheader_t *decode_header(unsigned char *packet, int len)
{
    unsigned short int *p;
    dnsheader_t *x;

    x = alloc_packet(packet, len);
    p = (unsigned short int *) x->packet;

    x->id      = ntohs(p[0]);
    x->u       = ntohs(p[1]);
    x->qdcount = ntohs(p[2]);
    x->ancount = ntohs(p[3]);
    x->nscount = ntohs(p[4]);
    x->arcount = ntohs(p[5]);

    x->here    = (char *) &x->packet[12];
    return (x);
}

static int get_objectname(unsigned char *msg, unsigned const char *limit, 
			  unsigned char **here, char *string, int strlen,
			  int k)
{
    unsigned int len;
    int	i;

    if ((*here>=limit) || (k>=strlen)) return(-1);
    while ((len = **here) != 0) {

	*here += 1;
	if ( *here >= limit ) return(-1);
	/* If the name is compressed (see 4.1.4 in rfc1035)  */
	if (len & 0xC0) {
	    unsigned offset;
	    unsigned char *p;

	    offset = ((len & ~0xc0) << 8) + **here;
	    if ((p = &msg[offset]) >= limit) return(-1);
	    if (p == *here-1) {
	      //log_debug("looping ptr");
	      return(-2);
	    }

	    if ((k = get_objectname(msg, limit, &p, string, RR_NAMESIZE, k))<0)
	      return(-1); /* if we cross the limit, bail out */
	    break;
	}
	else if (len < 64) {
	  /* check so we dont pass the limits */
	  if (((*here + len) > limit) || (len+k+2 > strlen)) return(-1);

	  for (i=0; i < len; i++) {
	    string[k++] = **here;
	    *here += 1;
	  }

	  string[k++] = '.';
	}
    }

    *here += 1;
    string[k] = 0;
    
    return (k);
}

static unsigned char *read_record(dnsheader_t *x, rr_t *y,
				  unsigned char *here, int question,
				  unsigned const char *limit)
{
    int	k, len;
    unsigned short int conv;
	unsigned short check_len=0;
    /*
     * Read the name of the resource ...
     */

    k = get_objectname(x->packet, limit, &here, y->name, sizeof(y->name), 0);
    if (k < 0) return(NULL);
    y->name[k] = 0;

    /* safe to read TYPE and CLASS? */
    if ((here+4) > limit) return (NULL);

    /*
     * ... the type of data ...
     */

    memcpy(&conv, here, sizeof(unsigned short int));
    y->type = ntohs(conv);
    here += 2;

    /*
     * ... and the class type.
     */

    memcpy(&conv, here, sizeof(unsigned short int));
    y->class = ntohs(conv);
    here += 2;

    /*
     * Question blocks stop here ...
     */

    if (question != 0) return (here);


    /*
     * ... while answer blocks carry a TTL and the actual data.
     */

    /* safe to read TTL and RDLENGTH? */
    if ((here+6) > limit) return (NULL);
    memcpy(&y->ttl, here, sizeof(unsigned long int));
    y->ttl = ntohl(y->ttl);
    here += 4;

    /*
     * Fetch the resource data.
     */

    //memcpy(&y->len, here, sizeof(unsigned short int));
    memcpy(&check_len, here, sizeof(unsigned short));
    //printf("the check len=%X\n", check_len);

   len = y->len = ntohs(check_len);
	
    //printf("A the len =%X\n", y->len);
    //len = y->len = ntohs(y->len);

	here += 2;

    /* safe to read RDATA? */
    if ((here + y->len) > limit) 
	{
		//printf("\nreturn NULL!!!!!!!!!\n");
		return (NULL);
    	}
    
    if (y->len > sizeof(y->data) - 4) {
	y->len = sizeof(y->data) - 4;
    }

    memcpy(y->data, here, y->len);
    here += len;
    y->data[y->len] = 0;

    return (here);
}

int rtl_gethostbyname(char *wan_ifname, char *acs_url, char *acs_ip)
{
	int sock, len = 0, i, j, count=0;
	socklen_t socklen = 0;
	unsigned char send_buf[DNS_BUF_SIZE] = {0};
	unsigned char recv_buf[DNS_BUF_SIZE] = {0};
	char *p = NULL;
	struct in_addr ipaddr[MAX_DNS_NUM];
	int dns_num=0;
	struct sockaddr_in rcv_servaddr, send_servaddr[MAX_DNS_NUM];
	struct timeval tv;
	//char wan_name[16]={0};

	rr_t	y;
	dnsheader_t *x=NULL;
	unsigned char *limit=NULL;
	int retval=-1;
	struct in_addr acs_ipaddr, reply_ip;

 
	int send_count=0;

	if(wan_ifname==NULL || acs_url==NULL || acs_ip==NULL)
	{
		printf("\nInvalid argument!\n");
		return retval;
	}	
	memset(&tv, 0, sizeof(tv));		
	if(inet_aton(acs_url, &acs_ipaddr)!=0)
	{
		strcpy(acs_ip, acs_url);
		//printf("\nIt's already IP %s\n", acs_ip);
		return 0;
	}
	memset(ipaddr, 0, sizeof(struct in_addr)*MAX_DNS_NUM);
	if(get_dns_ip(wan_ifname, ipaddr, &dns_num)<0)
	{
		//printf("\nget dns fail!\n");
		return retval;
	}

	//printf("\n########dns_num=%d\n",dns_num);
	//printf("\n########acs_url=%s\n",acs_url);	
	//printf("\n########wan_name=%s\n",wan_name);

	if((sock = socket(AF_INET, SOCK_DGRAM, 0))<0)
	{
		printf("\ncreate socket fail!\n");
		return retval;
	}
	
	setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, wan_ifname, strlen(wan_ifname)+1);	

	for (i = 0; i < dns_num; i++) 
	{
		bzero(&send_servaddr[i], sizeof(struct sockaddr_in));
		send_servaddr[i].sin_family = AF_INET;
		send_servaddr[i].sin_port   = htons(DNS_SRV_PORT);
		send_servaddr[i].sin_addr.s_addr=ipaddr[i].s_addr;

		//printf("\n########ipaddr[%d]=%08x\n",i,ipaddr[i]);
	}	

	DNS_HDR *dnshdr = (DNS_HDR *)send_buf;
	DNS_QER *dnsqer = (DNS_QER *)(send_buf + sizeof(DNS_HDR));
	memset(send_buf, 0, DNS_BUF_SIZE);
	dnshdr->id = (U16)1;
	dnshdr->tag = htons(0x0100);
	dnshdr->numq = htons(1);
	dnshdr->numa = 0;

	strncpy(send_buf + sizeof(DNS_HDR) + 1, acs_url, strlen(acs_url));
	p = send_buf + sizeof(DNS_HDR) + 1;
	i = 0;
	while (p < (send_buf + sizeof(DNS_HDR) + 1 + strlen(acs_url)))
	{
		if ( *p == '.')
		{
			*(p - i - 1) = i;
			i = 0;
		}
		else
		{
			i++;
		}
		p++;
	}
	*(p - i - 1) = i;
	
	dnsqer = (DNS_QER *)(send_buf + sizeof(DNS_HDR) + 2 + strlen(acs_url));
	dnsqer->classes = htons(1);
	dnsqer->type = htons(1);

	for(send_count=0; send_count<SEND_DNS_REQ_NUM;send_count++)
	for(i=0;i<dns_num;i++)
	{
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		
		socklen = sizeof(struct sockaddr_in);		
		len = sendto(sock, send_buf, sizeof(DNS_HDR) + sizeof(DNS_QER) + strlen(acs_url) + 2, 0, (struct sockaddr *)&send_servaddr[i], socklen);

		if (len < 0)
		{
			//printf("\nsend error\n");
			continue;
		}
		len = recvfrom(sock, recv_buf, DNS_BUF_SIZE, 0, (struct sockaddr *)&rcv_servaddr, &socklen);
		if (len < 0)
		{
			//printf("\nreceive timeout\n");
			continue;
		}		
	
       if(*((unsigned short*)(&recv_buf[6]))==0)
            continue;

		if ((x = decode_header(recv_buf, len)) == NULL ) 
			goto exit;

		if (x->ancount == 0)
		{
			free_packet(x);
			continue;
		}		
		
		limit = x->packet + len;

		if ((x->here = read_record(x, &y, x->here, 1, limit)) == NULL) 
			goto exit;
		
		//fprintf(stderr, "  name= %s, type= %d, class= %d\n", y.name, y.type, y.class);

		//fprintf(stderr, "\nans= %u\n", x->ancount);
		for (j = 0; j < x->ancount; j++) 
		{
			if ((x->here = read_record(x, &y, x->here, 0, limit)) == NULL) 
			{
				//printf("\nread_record fail!!!!!!\n");
				goto exit;
			}
			
			//fprintf(stderr, "\nname= %s, type= %d, class= %d, len=%d, ttl= %lu\n", y.name, y.type, y.class, y.len, y.ttl);

			if(y.type==0x0001 && y.class==0x0001 && y.len==4)
			{			
				memcpy(&reply_ip.s_addr, y.data, y.len);
				strcpy(acs_ip, inet_ntoa(reply_ip));
				retval=0;
				goto exit;
			}
		}	
	}	
	
exit:
	if(sock>0)
		close(sock);

	if(x!=NULL)
		free_packet(x);
	
	return retval;
}

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



#if defined(MULTI_WAN_SUPPORT)
//the table must fit the real phy ports on customer's board, currently support 4 eth lan and 5*2 wlan interface
struct lanPortBindIfname lanPortBindIfnameTbl[ETH_LAN_NUM+WLAN_NUM*NUM_WLAN_INTERFACE]=
{
		{ETH_LAN1_BIND_MASK,	ELAN1_DEV_NAME},
		{ETH_LAN2_BIND_MASK,	ELAN2_DEV_NAME},
		{ETH_LAN3_BIND_MASK,	ELAN3_DEV_NAME},
		{ETH_LAN4_BIND_MASK,	ELAN4_DEV_NAME},
#ifdef CONFIG_RTL_ISP_MULTI_PHY_WAN_SUPPORT
		{ETH_LAN5_BIND_MASK,	ELAN5_DEV_NAME},
#endif
		
		{WLAN1_BIND_MASK,		WLAN0_DEV_NAME},
		{WLAN1_VA1_BIND_MASK,	WLAN0_VA0_DEV_NAME},
		{WLAN1_VA2_BIND_MASK,	WLAN0_VA1_DEV_NAME},
		{WLAN1_VA3_BIND_MASK,	WLAN0_VA2_DEV_NAME},
		{WLAN1_VA4_BIND_MASK,	WLAN0_VA3_DEV_NAME},
//		{WLAN1_VA5_BIND_MASK,	WLAN0_VA4_DEV_NAME},
//		{WLAN1_VA6_BIND_MASK,	WLAN0_VA5_DEV_NAME},
//		{WLAN1_VA7_BIND_MASK,	WLAN0_VA6_DEV_NAME},
		{WLAN1_VXD_BIND_MASK,	WLAN0_VXD_DEV_NAME},
		
		{WLAN2_BIND_MASK,		WLAN1_DEV_NAME},
		{WLAN2_VA1_BIND_MASK,	WLAN1_VA0_DEV_NAME},
		{WLAN2_VA2_BIND_MASK,	WLAN1_VA1_DEV_NAME},
		{WLAN2_VA3_BIND_MASK,	WLAN1_VA2_DEV_NAME},
		{WLAN2_VA4_BIND_MASK,	WLAN1_VA3_DEV_NAME},
//		{WLAN2_VA5_BIND_MASK,	WLAN1_VA4_DEV_NAME},
//		{WLAN2_VA6_BIND_MASK,	WLAN1_VA5_DEV_NAME},
//		{WLAN2_VA7_BIND_MASK,	WLAN1_VA6_DEV_NAME},
		{WLAN2_VXD_BIND_MASK,	WLAN1_VXD_DEV_NAME}
		
//		{WLAN3_BIND_MASK,		WLAN2_DEV_NAME},
//		{WLAN3_VA1_BIND_MASK,	WLAN2_VA0_DEV_NAME},
//		{WLAN3_VA2_BIND_MASK,	WLAN2_VA1_DEV_NAME},
//		{WLAN3_VA3_BIND_MASK,	WLAN2_VA2_DEV_NAME},
//		{WLAN3_VA4_BIND_MASK,	WLAN2_VA3_DEV_NAME},
//		{WLAN3_VA5_BIND_MASK,	WLAN2_VA4_DEV_NAME},
//		{WLAN3_VA6_BIND_MASK,	WLAN2_VA5_DEV_NAME},
//		{WLAN3_VA7_BIND_MASK,	WLAN2_VA6_DEV_NAME},
//		{WLAN3_VXD_BIND_MASK,	WLAN2_VXD_DEV_NAME},
};


//wanBindingLanPorts index to ifname
int getLanPortIfName(char* name,int index)
{
	if(!name || index<0 || index>(WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM)) return -1;

	snprintf(name,IFACE_NAME_MAX,lanPortBindIfnameTbl[index].ifname);
#if 0
	if(index==0) 
		snprintf(name,IFACE_NAME_MAX,"eth0");
	else if(index>0 && index<WAN_INTERFACE_LAN_PORT_NUM)  
		snprintf(name,IFACE_NAME_MAX,"eth%d",index+1);
	else if(((index-WAN_INTERFACE_LAN_PORT_NUM)%WAN_INTERFACE_EACH_WLAN_PORT_NUM)==0)
		snprintf(name,IFACE_NAME_MAX,"wlan%d",(index-WAN_INTERFACE_LAN_PORT_NUM)/WAN_INTERFACE_EACH_WLAN_PORT_NUM);
	else
		snprintf(name,IFACE_NAME_MAX,"wlan%d-va%d",(index-WAN_INTERFACE_LAN_PORT_NUM)/WAN_INTERFACE_EACH_WLAN_PORT_NUM,((index-WAN_INTERFACE_LAN_PORT_NUM)%WAN_INTERFACE_EACH_WLAN_PORT_NUM));
#endif
	return 1;
}

//wanBindingLanPorts ifname to index
int getLanPortIndex(char *name)
{
	int i=0;

	for(i=0; i<sizeof(lanPortBindIfnameTbl)/sizeof(struct lanPortBindIfname); i++)
	{
		if(strcmp(name, lanPortBindIfnameTbl[i].ifname)==0)
			return i;
	}

	return -1;
}
#endif

/**************************************************************
 ********          Firmware/Configure Upgrade          ********
 **************************************************************/
#ifdef CONFIG_FWD_UNIFIED_FRAMEWORK
int isUpgrade_OK=0;
int isFWUPGRADE=0;
#ifdef BOA_WITH_SSL
int fwInProgress=0;
#endif
int isCFGUPGRADE=0;
int isREBOOTASP=0;
int Reboot_Wait=0;
int isCFG_ONLY=0;
int isCountDown=0;
int configlen = 0;

static int shm_id=0, shm_size;
char *shm_memory=NULL; //, *shm_name;
//validate callback
static int fw_validation(char *upload_data, int offset, unsigned char *buffer);
static int csds_validation(char *upload_data, int offset, unsigned char *buffer);
#ifdef CONFIG_FWD_UNIFIED_SUPPORT_PLUGIN
static int plugin_validation(char *upload_data, int offset, unsigned char *buffer);
#endif
//boot.bin callback
static int boot_getBurnAddr(char *pHeader, char *logbuf, int flag, int backup_bank);
static int boot_getBurnSize(char *pHeader, char *logbuf, int flag, int idx);
//linux.bin callback
static int linux_getBurnAddr(char *pHeader, char *logbuf, int flag, int backup_bank);
//root.bin callback
static int root_getBurnAddr(char *pHeader, char *logbuf, int flag, int backup_bank);
static int fw_getBurnSize(char *pHeader, char *logbuf, int flag, int idx);
//csds callback
static int ds_getBurnAddr(char *pHeader, char *logbuf, int flag, int backup_bank);
static int cs_getBurnAddr(char *pHeader, char *logbuf, int flag, int backup_bank);
static int cfg_getBurnSize(char *pHeader, char *logbuf, int flag, int idx);
#ifdef CONFIG_FWD_UNIFIED_SUPPORT_PLUGIN
//plugin callback
static int plugin_getBurnAddr(char *pHeader, char *logbuf, int flag, int backup_bank);
static int plugin_getBurnSize(char *pHeader, char *logbuf, int flag, int idx);
#endif

#define FWD_HDR_FLAG_SKIPHDR	(1<<16)
typedef struct _fwd_section_info{
	unsigned char *signature;
	unsigned char *comment;
	int sectype;
	int sig_len;
	int hdr_type;	//skip HDR length when burn to flash
	int maxsize;	//0 for not specified
	int reboot;
	int (*validate)(char *data, int offset, char *logbuf);
	int (*getBurnAddr)(char *pHeader, char *logbuf, int flag, int backup_bank);
	int (*getBurnSize)(char *pHeader, char *logbuf, int flag, int idx);
}FWD_SECTION_INFO_T, *FWD_SECTION_INFO_Tp;

/*
 * data section information table
 */
const FWD_SECTION_INFO_T FWD_SECTION_TBL[]=
{
	/* signature, 		comment, 		sectype,				sig_len, 		hdr_type, 									max/rebo 	validate,		getBurnAddr,		getBurnSize*/
	{BOOT_SIGNATURE, 	"bootcode", 	FWD_DATA_TYPE_BOOT, 	SIGNATURE_LEN, 	FWD_HDR_FLAG_SKIPHDR|sizeof(IMG_HEADER_T), 	0, 1, 		fw_validation, 	boot_getBurnAddr, 	boot_getBurnSize},
	{WEB_HEADER,		"webpage",		FWD_DATA_TYPE_WEB,		SIGNATURE_LEN,	sizeof(IMG_HEADER_T), 						0, 1, 		NULL, 			NULL, 				NULL},
	{FW_HEADER, 		"kernel", 		FWD_DATA_TYPE_LINUX, 	SIGNATURE_LEN, 	sizeof(IMG_HEADER_T), 						0, 1,		fw_validation,	linux_getBurnAddr,	fw_getBurnSize},
	{FW_HEADER_WITH_ROOT, "kernel(rootfs)", FWD_DATA_TYPE_LINUX,SIGNATURE_LEN,	sizeof(IMG_HEADER_T), 						0, 1, 		fw_validation,	linux_getBurnAddr,	fw_getBurnSize},
	{ROOT_HEADER,		"rootfs",		FWD_DATA_TYPE_ROOTFS,	SIGNATURE_LEN,	FWD_HDR_FLAG_SKIPHDR|sizeof(IMG_HEADER_T), 	0, 1, 		fw_validation,	root_getBurnAddr,	fw_getBurnSize},
	{COMP_DS_SIGNATURE, "default setting", FWD_DATA_TYPE_DS,	COMP_SIGNATURE_LEN, sizeof(COMPRESS_MIB_HEADER_T), 			0, 1, 		csds_validation,ds_getBurnAddr,		cfg_getBurnSize},
	{COMP_CS_SIGNATURE, "current setting", FWD_DATA_TYPE_CS,	COMP_SIGNATURE_LEN, sizeof(COMPRESS_MIB_HEADER_T), 			0, 1, 		csds_validation,cs_getBurnAddr,		cfg_getBurnSize},
#ifdef CONFIG_FWD_UNIFIED_SUPPORT_PLUGIN
	{PLUGIN_SIGNATURE,	"plugin",		FWD_DATA_TYPE_PLUGIN,	SIGNATURE_LEN,	FWD_HDR_FLAG_SKIPHDR|sizeof(PLUGIN_HEADER_T),0,0,		plugin_validation,plugin_getBurnAddr,plugin_getBurnSize},
#endif
};

#define FWD_SECTION_TBLSIZE	(sizeof(FWD_SECTION_TBL)/sizeof(FWD_SECTION_INFO_T))
#define FWD_STATFILE_PREFIX		"/var/fwd_status_"		//append with callee's PID

int find_head_offset(char *upload_data)
{
	int head_offset=0 ;
	char *pStart=NULL;
	int iestr_offset=0;
	char *dquote;
	char *dquote1;
	
	if (upload_data==NULL) {
		//fprintf(stderr, "upload data is NULL\n");
		return -1;
	}

	pStart = strstr(upload_data, WINIE6_STR);
	if (pStart == NULL) {
		pStart = strstr(upload_data, LINUXFX36_FWSTR);
		if (pStart == NULL) {
			pStart = strstr(upload_data, MACIE5_FWSTR);
			if (pStart == NULL) {
				pStart = strstr(upload_data, OPERA_FWSTR);
				if (pStart == NULL) {
					pStart = strstr(upload_data, "filename=");
					if (pStart == NULL) {
						pStart = strstr(upload_data, CWMP_DL_HTTP_PREFIX);
						if (pStart == NULL) {
							return 0;
						}else{
							// extra prefix string + 4bytes of file length for TR-069
							return (strlen(CWMP_DL_HTTP_PREFIX)+4);
						}
					}
					else {
						dquote =  strstr(pStart, "\"");
						if (dquote !=NULL) {
							dquote1 = strstr(dquote, LINE_FWSTR);
							if (dquote1!=NULL) {
								iestr_offset = 4;
								pStart = dquote1;
							}
							else {
								return -1;
							}
						}
						else {
							return -1;
						}
					}
				}
				else {
					iestr_offset = 16;
				}
			} 
			else {
				iestr_offset = 14;
			}
		}
		else {
			iestr_offset = 26;
		}
	}
	else {
		iestr_offset = 17;
	}
	//fprintf(stderr,"####%s:%d %d###\n",  __FILE__, __LINE__ , iestr_offset);
	head_offset = (int)(((unsigned long)pStart)-((unsigned long)upload_data)) + iestr_offset;
	return head_offset;
}

int fwChecksumOk(char *data, int len)
{
	unsigned short sum=0;
	int i;

	for (i=0; i<len; i+=2) {
#ifdef _LITTLE_ENDIAN_
		sum += WORD_SWAP( *((unsigned short *)&data[i]) );
#else
		sum += *((unsigned short *)&data[i]);
#endif

	}
	return( (sum==0) ? 1 : 0);
}

static void kill_processes(void)
{
	printf("upgrade: killing tasks...\n");
	
	kill(1, SIGTSTP);		/* Stop init from reforking tasks */
	kill(1, SIGSTOP);		
	kill(2, SIGSTOP);		
	kill(3, SIGSTOP);		
	kill(4, SIGSTOP);		
	kill(5, SIGSTOP);		
	kill(6, SIGSTOP);		
	kill(7, SIGSTOP);		
	//atexit(restartinit);		/* If exit prematurely, restart init */
	sync();

	signal(SIGTERM,SIG_IGN);	/* Don't kill ourselves... */
	setpgrp(); 			/* Don't let our parent kill us */
	sleep(1);
	signal(SIGHUP, SIG_IGN);	/* Don't die if our parent dies due to
					 * a closed controlling terminal */
	
}

/* 
 * FIXME:
 *   called in fmmgmt.c formUploadConfig(), which is better to refine following framework.
 */
int updateConfigIntoFlash(unsigned char *data, int total_len, int *pType, int *pStatus)
{
	int len=0, status=1, type=0, ver, force;
#ifdef HEADER_LEN_INT
	HW_PARAM_HEADER_Tp phwHeader;
	int isHdware=0;
#endif
	PARAM_HEADER_Tp pHeader;
#ifdef COMPRESS_MIB_SETTING
	COMPRESS_MIB_HEADER_Tp pCompHeader;
	unsigned char *expFile=NULL;
	unsigned int expandLen=0;
	int complen=0;
	int compLen_of_header=0;
	short compRate=0;
#endif
	char *ptr;
	unsigned char isValidfw = 0;
	#if 0
	if(0 == check_config_valid(data,total_len))
	{
		printf("%s %d\n",__FUNCTION__,__LINE__);
		pStatus=0;
		return 0;
	}
	#endif
	do {
		if (
#ifdef COMPRESS_MIB_SETTING
			memcmp(&data[complen], COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN) &&
			memcmp(&data[complen], COMP_DS_SIGNATURE, COMP_SIGNATURE_LEN) &&
			memcmp(&data[complen], COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN)
#else
			memcmp(&data[len], CURRENT_SETTING_HEADER_TAG, TAG_LEN) &&
			memcmp(&data[len], CURRENT_SETTING_HEADER_FORCE_TAG, TAG_LEN) &&
			memcmp(&data[len], CURRENT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) &&
			memcmp(&data[len], DEFAULT_SETTING_HEADER_TAG, TAG_LEN) &&
			memcmp(&data[len], DEFAULT_SETTING_HEADER_FORCE_TAG, TAG_LEN) &&
			memcmp(&data[len], DEFAULT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) &&
			memcmp(&data[len], HW_SETTING_HEADER_TAG, TAG_LEN) &&
			memcmp(&data[len], HW_SETTING_HEADER_FORCE_TAG, TAG_LEN) &&
			memcmp(&data[len], HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) 
#endif
		) {
			if (isValidfw == 1)
				break;
		}
#ifdef HEADER_LEN_INT
	if(
	#ifdef COMPRESS_MIB_SETTING
		memcmp(&data[complen], COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN)==0
	#else
		memcmp(&data[len], HW_SETTING_HEADER_TAG, TAG_LEN)==0 ||
		memcmp(&data[len], HW_SETTING_HEADER_FORCE_TAG, TAG_LEN)==0 ||
		memcmp(&data[len], HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN)
	#endif
	)
	{
		isHdware=1;
	}
#endif
#ifdef COMPRESS_MIB_SETTING
		pCompHeader =(COMPRESS_MIB_HEADER_Tp)&data[complen];
		compRate = WORD_SWAP(pCompHeader->compRate);
		compLen_of_header = DWORD_SWAP(pCompHeader->compLen);
		/*decompress and get the tag*/
#ifdef RTK_MIB_TAG_CHECK
		expFile=malloc(compLen_of_header*WORD_SWAP(pCompHeader->realcompRate));
#else
		expFile=malloc(compLen_of_header*compRate);
#endif
		if (NULL==expFile) {
			printf("malloc for expFile error!!\n");
			return 0;
		}
		expandLen = Decode(data+complen+sizeof(COMPRESS_MIB_HEADER_T), compLen_of_header, expFile);
#ifdef HEADER_LEN_INT
		if(isHdware)
			phwHeader = (HW_PARAM_HEADER_Tp)expFile;
		else
#endif
		pHeader = (PARAM_HEADER_Tp)expFile;
#else
#ifdef HEADER_LEN_INT
		if(isHdware)
			phwHeader = (HW_PARAM_HEADER_Tp)expFile;
		else
#endif
		pHeader = (PARAM_HEADER_Tp)&data[len];
#endif
		
#ifdef _LITTLE_ENDIAN_
#ifdef HEADER_LEN_INT
		if(isHdware)
			phwHeader->len = WORD_SWAP(phwHeader->len);
		else
#endif
		pHeader->len = HEADER_SWAP(pHeader->len);
#endif
#ifdef HEADER_LEN_INT
		if(isHdware)
			len += sizeof(HW_PARAM_HEADER_T);
		else
#endif
		len += sizeof(PARAM_HEADER_T);

        /*in case use wrong version config.dat*/
        #define MAX_CONFIG_LEN 1024*1024
        #define MIN_CONFIG_LEN 8*1024
#ifdef HEADER_LEN_INT
		if(isHdware)
        {
            if((phwHeader->len > MAX_CONFIG_LEN)||(phwHeader->len < MIN_CONFIG_LEN))
            {
                printf("INVALID config.data FILE\n");
                status = 0;
                break;
            }
        }else
#endif
        {
            if((pHeader->len > MAX_CONFIG_LEN)||(pHeader->len < MIN_CONFIG_LEN))
            {
                printf("INVALID config.data FILE\n");
                status = 0;
                break;
            }
        }

		if ( sscanf((char *)&pHeader->signature[TAG_LEN], "%02d", &ver) != 1)
			ver = -1;
			
		force = -1;
		if ( !memcmp(pHeader->signature, CURRENT_SETTING_HEADER_TAG, TAG_LEN) ) {
			isValidfw = 1;
			force = 1; // update
		}
		else if ( !memcmp(pHeader->signature, CURRENT_SETTING_HEADER_FORCE_TAG, TAG_LEN)) {
			isValidfw = 1;
			force = 2; // force
		}
		else if ( !memcmp(pHeader->signature, CURRENT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN)) {
			isValidfw = 1;
			force = 0; // upgrade
		}

		if ( force >= 0 ) {
#if 0
			if ( !force && (ver < CURRENT_SETTING_VER || // version is less than current
				(pHeader->len < (sizeof(APMIB_T)+1)) ) { // length is less than current
				status = 0;
				break;
			}
#endif

#ifdef COMPRESS_MIB_SETTING
#ifdef HEADER_LEN_INT
			if(isHdware)
				ptr = (char *)(expFile+sizeof(HW_PARAM_HEADER_T));
			else
#endif
			ptr = (char *)(expFile+sizeof(PARAM_HEADER_T));
#else
			ptr = &data[len];
#endif

#ifdef COMPRESS_MIB_SETTING
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				DECODE_DATA(ptr, phwHeader->len);
			else
#endif
			DECODE_DATA(ptr, pHeader->len);
#endif
#ifdef HEADER_LEN_INT
			if(isHdware)
			{
					if ( !CHECKSUM_OK((unsigned char *)ptr, phwHeader->len)) {
						status = 0;
						break;
					}
			}
			else
#endif
			if ( !CHECKSUM_OK((unsigned char *)ptr, pHeader->len)) {
				status = 0;
				break;
			}
//#ifdef _LITTLE_ENDIAN_
//			swap_mib_word_value((APMIB_Tp)ptr);
//#endif

// added by rock /////////////////////////////////////////
#ifdef VOIP_SUPPORT
		#ifndef VOIP_SUPPORT_TLV_CFG
			flash_voip_import_fix(&((APMIB_Tp)ptr)->voipCfgParam, &pMib->voipCfgParam);
#endif
#endif

#ifdef COMPRESS_MIB_SETTING
			//printf("[%s:%d] start ptr:%p\n", __FUNCTION__, __LINE__, data);
			apmib_updateFlash(CURRENT_SETTING, (char *)&data[complen], compLen_of_header+sizeof(COMPRESS_MIB_HEADER_T), force, ver);
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				apmib_updateFlash(CURRENT_SETTING, ptr, phwHeader->len-1, force, ver);
			else
#endif
			apmib_updateFlash(CURRENT_SETTING, ptr, pHeader->len-1, force, ver);
#endif

#ifdef COMPRESS_MIB_SETTING
			complen += compLen_of_header+sizeof(COMPRESS_MIB_HEADER_T);
			if (expFile) {
				free(expFile);
				expFile=NULL;
			}
#else
			
#ifdef HEADER_LEN_INT
			if(isHdware)
				len += phwHeader->len;
			else
#endif
			len += pHeader->len;
#endif
			type |= CURRENT_SETTING;
			continue;
		}


		if ( !memcmp(pHeader->signature, DEFAULT_SETTING_HEADER_TAG, TAG_LEN) ) {
			isValidfw = 1;
			force = 1;	// update
		}
		else if ( !memcmp(pHeader->signature, DEFAULT_SETTING_HEADER_FORCE_TAG, TAG_LEN) ) {
			isValidfw = 1;
			force = 2;	// force
		}
		else if ( !memcmp(pHeader->signature, DEFAULT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) ) {
			isValidfw = 1;
			force = 0;	// upgrade
		}

		if ( force >= 0 ) {
#if 0
			if ( (ver < DEFAULT_SETTING_VER) || // version is less than current
				(pHeader->len < (sizeof(APMIB_T)+1)) ) { // length is less than current
				status = 0;
				break;
			}
#endif

#ifdef COMPRESS_MIB_SETTING
#ifdef HEADER_LEN_INT
			if(isHdware)
				ptr = (char *)(expFile+sizeof(HW_PARAM_HEADER_T));
			else
#endif
			ptr = (char *)(expFile+sizeof(PARAM_HEADER_T));
#else
			ptr = &data[len];
#endif

#ifdef COMPRESS_MIB_SETTING
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				DECODE_DATA(ptr, phwHeader->len);
			else
#endif
			DECODE_DATA(ptr, pHeader->len);
#endif
#ifdef HEADER_LEN_INT
			if(isHdware)
			{
				if ( !CHECKSUM_OK((unsigned char *)ptr, phwHeader->len)) {
				status = 0;
				break;
				}
			}
			else
#endif
			if ( !CHECKSUM_OK((unsigned char *)ptr, pHeader->len)) {
				status = 0;
				break;
			}

//#ifdef _LITTLE_ENDIAN_
//			swap_mib_word_value((APMIB_Tp)ptr);
//#endif

// added by rock /////////////////////////////////////////
#ifdef VOIP_SUPPORT
		#ifndef VOIP_SUPPORT_TLV_CFG
			flash_voip_import_fix(&((APMIB_Tp)ptr)->voipCfgParam, &pMibDef->voipCfgParam);
#endif
#endif

#ifdef COMPRESS_MIB_SETTING
			apmib_updateFlash(DEFAULT_SETTING, (char *)&data[complen], compLen_of_header+sizeof(COMPRESS_MIB_HEADER_T), force, ver);
#else
		
#ifdef HEADER_LEN_INT
			if(isHdware)
				apmib_updateFlash(DEFAULT_SETTING, ptr, phwHeader->len-1, force, ver);
			else
#endif
			apmib_updateFlash(DEFAULT_SETTING, ptr, pHeader->len-1, force, ver);
#endif

#ifdef COMPRESS_MIB_SETTING
			complen += compLen_of_header+sizeof(COMPRESS_MIB_HEADER_T);
			if (expFile) {
				free(expFile);
				expFile=NULL;
			}	
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				len += phwHeader->len;
			else
#endif
			len += pHeader->len;
#endif
			type |= DEFAULT_SETTING;
			continue;
		}

		if ( !memcmp(pHeader->signature, HW_SETTING_HEADER_TAG, TAG_LEN) ) {
			isValidfw = 1;
			force = 1;	// update
		}
		else if ( !memcmp(pHeader->signature, HW_SETTING_HEADER_FORCE_TAG, TAG_LEN) ) {
			isValidfw = 1;
			force = 2;	// force
		}
		else if ( !memcmp(pHeader->signature, HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) ) {
			isValidfw = 1;
			force = 0;	// upgrade
		}

		if ( force >= 0 ) {
#if 0
			if ( (ver < HW_SETTING_VER) || // version is less than current
				(pHeader->len < (sizeof(HW_SETTING_T)+1)) ) { // length is less than current
				status = 0;
				break;
			}
#endif
#ifdef COMPRESS_MIB_SETTING
#ifdef HEADER_LEN_INT
			if(isHdware)
				ptr = (char *)(expFile+sizeof(HW_PARAM_HEADER_T));
			else
#endif
			ptr = (char *)(expFile+sizeof(PARAM_HEADER_T));
#else
			ptr = &data[len];
#endif
			

#ifdef COMPRESS_MIB_SETTING
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				DECODE_DATA(ptr, phwHeader->len);
			else
#endif
			DECODE_DATA(ptr, pHeader->len);
#endif
#ifdef HEADER_LEN_INT
			if(isHdware)
			{
				if ( !CHECKSUM_OK((unsigned char *)ptr, phwHeader->len)) {
				status = 0;
				break;
				}
			}
			else
#endif
			if ( !CHECKSUM_OK((unsigned char *)ptr, pHeader->len)) {
				status = 0;
				break;
			}
#ifdef COMPRESS_MIB_SETTING
			apmib_updateFlash(HW_SETTING, (char *)&data[complen], compLen_of_header + sizeof(COMPRESS_MIB_HEADER_T), force, ver);
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				apmib_updateFlash(HW_SETTING, ptr, phwHeader->len-1, force, ver);
			else
#endif
			apmib_updateFlash(HW_SETTING, ptr, pHeader->len-1, force, ver);
#endif

#ifdef COMPRESS_MIB_SETTING
			complen += compLen_of_header + sizeof(COMPRESS_MIB_HEADER_T);
			if (expFile) {
				free(expFile);
				expFile=NULL;
			}
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				len += phwHeader->len;
			else
#endif
			len += pHeader->len;
#endif

			type |= HW_SETTING;
			continue;
		}
	}
#ifdef COMPRESS_MIB_SETTING	
	while (complen < total_len);

	if (expFile) {
		free(expFile);
		expFile=NULL;
	}
#else
	while (len < total_len);
#endif

	*pType = type;
	*pStatus = status;
#ifdef COMPRESS_MIB_SETTING	
	return complen;
#else
	return len;
#endif
}


/*************************************************
 **************** Local Utilities ****************
 *************************************************/
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
#define SQSH_SIGNATURE		((char *)"sqsh")
#define SQSH_SIGNATURE_LE       ((char *)"hsqs")

#define IMAGE_ROOTFS 2
#define IMAGE_KERNEL 1
#define GET_BACKUP_BANK 2
#define GET_ACTIVE_BANK 1

#define GOOD_BANK_MARK_MASK 0x80000000  //goo abnk mark must set bit31 to 1

#define NO_IMAGE_BANK_MARK 0x80000000  
#define OLD_BURNADDR_BANK_MARK 0x80000001 
#define BASIC_BANK_MARK 0x80000002           
#define FORCEBOOT_BANK_MARK 0xFFFFFFF0  //means always boot/upgrade in this bank

static int get_actvie_bank()
{
	FILE *fp;
	char buffer[2];
	int bootbank;
	fp = fopen("/proc/bootbank", "r");
	
	if (!fp) {
		fprintf(stderr,"%s\n","Read /proc/bootbank failed!\n");
	}else
	{
			//fgets(bootbank, sizeof(bootbank), fp);
			fgets(buffer, sizeof(buffer), fp);
			fclose(fp);
	}
	bootbank = buffer[0] - 0x30;	
	if ( bootbank ==1 || bootbank ==2)
		return bootbank;
	else
		return 1;	
}

static void get_bank_info(int dual_enable,int *active,int *backup)
{
	int bootbank=0,backup_bank;
	
	bootbank = get_actvie_bank();	

	if(bootbank == 1 )
	{
		if( dual_enable ==0 )
			backup_bank =1;
		else
			backup_bank =2;
	}
	else if(bootbank == 2 )
	{
		if( dual_enable ==0 )
			backup_bank =2;
		else
			backup_bank =1;
	}
	else
	{
		bootbank =1 ;
		backup_bank =1 ;
	}	

	*active = bootbank;
	*backup = backup_bank;	

	//fprintf(stderr,"get_bank_info active_bank =%d , backup_bank=%d  \n",*active,*backup); //mark_debug	   
}

static int check_system_image(int fh, IMG_HEADER_Tp pHeader)
{
	// Read header, heck signature and checksum
	int i, ret=0;		
	char image_sig[4]={0};
	char image_sig_root[4]={0};
	
    /*check linux signature*/
	if ( read(fh, pHeader, sizeof(IMG_HEADER_T)) != sizeof(IMG_HEADER_T)) 
     		return 0;	
	
	memcpy(image_sig, FW_HEADER, SIGNATURE_LEN);
	memcpy(image_sig_root, FW_HEADER_WITH_ROOT, SIGNATURE_LEN);

	if (!memcmp(pHeader->signature, image_sig, SIGNATURE_LEN)){
		//printf("sys signature ");
		ret=1;
	}else if  (!memcmp(pHeader->signature, image_sig_root, SIGNATURE_LEN)){
		//printf("sys signature ");
		ret=2;
	}else{
		//printf("no sys signature ");
	}				
       //mark_dual , ignore checksum() now.(to do) 
	return (ret);
}

static int get_image_header(int fh, int dual_enable, int support_offset_change, int active_bank, IMG_HEADER_Tp header_p)
{
	int ret=0, offset=0;
	//check 	CODE_IMAGE_OFFSET2 , CODE_IMAGE_OFFSET3 ?
	//ignore check_image_header () for fast get header , assume image are same offset......	
	// support CONFIG_RTL_FLASH_MAPPING_ENABLE ? , scan header ...
	
#ifdef CONFIG_MTD_NAND
	if(support_offset_change)
		offset = CODE_IMAGE_OFFSET;
	else
		offset = CODE_IMAGE_OFFSET-NAND_BOOT_SETTING_SIZE;
#else
	offset = CODE_IMAGE_OFFSET;
#endif

	if(support_offset_change)
		if(active_bank==2){
			offset += CONFIG_RTL_FLASH_DUAL_IMAGE_OFFSET;
		}
	
	lseek(fh, offset, SEEK_SET);
	ret = check_system_image(fh, header_p);
	//printf("at 0x%x\n", offset);
	lseek(fh, offset, SEEK_SET);
	
	return ret;	
}

static unsigned long header_to_mark(int  flag, IMG_HEADER_Tp pHeader)
{
	unsigned long ret_mark=NO_IMAGE_BANK_MARK;
	//mark_dual ,  how to diff "no image" "image with no bank_mark(old)" , "boot with lowest priority"
	if(flag) //flag ==0 means ,header is illegal
	{
		if( (pHeader->burnAddr & GOOD_BANK_MARK_MASK) )
			ret_mark=pHeader->burnAddr;	
		else
			ret_mark = OLD_BURNADDR_BANK_MARK;
	}
	return ret_mark;
}

static unsigned long get_next_bankmark(char *kernel_dev, int dual_enable, int support_offset_change, int active_bank, unsigned char *msg)
{
    unsigned long bankmark=NO_IMAGE_BANK_MARK;
    int ret=0,fh;
    IMG_HEADER_T header; 	

	//printf("[%s:%d] current kernel:%s, dual_enable:%d\n", __FUNCTION__, __LINE__, kernel_dev, dual_enable);
	
	fh = open(kernel_dev, O_RDONLY);
	if ( fh == -1 ) {
      	sprintf(msg,"Open %s failed!\n", kernel_dev);
		return NO_IMAGE_BANK_MARK;
	}
	ret = get_image_header(fh, dual_enable, support_offset_change, active_bank, &header);	
	close(fh);

	//printf("get_next_bankmark from %s, ret=%d \n",kernel_dev,ret); //mark_debug
	bankmark= header_to_mark(ret, &header);	
	//get next boot mark

	if( bankmark < BASIC_BANK_MARK)
		return BASIC_BANK_MARK;
	else if( (bankmark ==  FORCEBOOT_BANK_MARK) || (dual_enable == 0)) //dual_enable = 0 ....
	{
		return FORCEBOOT_BANK_MARK;//it means dual bank disable
	}
	else
		return bankmark+1;
	
}

#endif

static int get_mtd_info(struct fwd_partition *target_map, struct fwd_partition *current_map, int support_offset_change, unsigned char *msg)
{
#ifdef MTD_NAME_MAPPING
	if(target_map==NULL)
		return FWD_FAILED;
	
	char bootMtd[FWD_MAX_TARGET_NAMESIZE], webpageMtd[FWD_MAX_TARGET_NAMESIZE],linuxMtd[FWD_MAX_TARGET_NAMESIZE],rootfsMtd[FWD_MAX_TARGET_NAMESIZE];
	unsigned int i = 0;
	for(i=0; i<FWD_DATA_TYPE_SIZE; i++)
		memset(target_map[i].name, 0x0, FWD_MAX_TARGET_NAMESIZE);
	
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	int active_bank,backup_bank;
	int dual_enable =0;
	char webpageMtd2[FWD_MAX_TARGET_NAMESIZE],linuxMtd2[FWD_MAX_TARGET_NAMESIZE],rootfsMtd2[FWD_MAX_TARGET_NAMESIZE];

	if(current_map==NULL)
		return FWD_FAILED;
	
	for(i=0; i<FWD_DATA_TYPE_SIZE; i++)
		memset(current_map[i].name, 0x0, FWD_MAX_TARGET_NAMESIZE);
#endif


	if(support_offset_change)
	{
		for(i=0; i<FWD_DATA_TYPE_SIZE; i++)
		{
#ifdef CONFIG_MTD_NAND
			if(i>=FWD_DATA_TYPE_HS)
			{
				//use settings file for cs/ds
				strcpy(target_map[i].name, FLASH_DEVICE_SETTING);
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
				//actually single config
				strcpy(current_map[i].name, FLASH_DEVICE_SETTING);
#endif
			}else
#endif /* CONFIG_MTD_NAND */
			{
				// support new firmware flash layout change, use 'wholeflash' partition
				if(rtl_name_to_mtdblock(FLASH_WHOLE_FLASH_NAME,webpageMtd) == 0){
					sprintf(msg, "cannot find wholeflash mtdblock!");
					return FWD_FAILED;
				}
				strcpy(target_map[i].name, webpageMtd);
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
				//for bookmark check
				strcpy(current_map[i].name, webpageMtd);
#endif
			}
		}
	}
	else{
		// burn according to current firmware flash layout
		if(rtl_name_to_mtdblock(FLASH_BOOT_NAME,bootMtd) == 0
			||rtl_name_to_mtdblock(FLASH_WEBPAGE_NAME,webpageMtd) == 0
			  || rtl_name_to_mtdblock(FLASH_LINUX_NAME,linuxMtd) == 0
				|| rtl_name_to_mtdblock(FLASH_ROOTFS_NAME,rootfsMtd) == 0)
		{
			sprintf(msg, "cannot find boot/webpage/linux/rootfs mtdblock!");
			return FWD_FAILED;
		}
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
		if(rtl_name_to_mtdblock(FLASH_WEBPAGE_NAME2,webpageMtd2) == 0
			|| rtl_name_to_mtdblock(FLASH_LINUX_NAME2,linuxMtd2) == 0
				|| rtl_name_to_mtdblock(FLASH_ROOTFS_NAME2,rootfsMtd2) == 0)
		{
			sprintf(msg,  "cannot find webpage2/linux2/rootfs2 mtdblock!");
			return FWD_FAILED;
		}
		
		apmib_get(MIB_DUALBANK_ENABLED,(void *)&dual_enable);   
		get_bank_info(dual_enable,&active_bank,&backup_bank); 

		//actually single bank
#ifdef CONFIG_MTD_NAND
		strcpy(target_map[FWD_DATA_TYPE_HS].name, FLASH_DEVICE_SETTING);
		strcpy(target_map[FWD_DATA_TYPE_DS].name, FLASH_DEVICE_SETTING);
		strcpy(target_map[FWD_DATA_TYPE_CS].name, FLASH_DEVICE_SETTING);
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
		strcpy(current_map[FWD_DATA_TYPE_HS].name, FLASH_DEVICE_SETTING);
		strcpy(current_map[FWD_DATA_TYPE_DS].name, FLASH_DEVICE_SETTING);
		strcpy(current_map[FWD_DATA_TYPE_CS].name, FLASH_DEVICE_SETTING);
#endif
#else
		strcpy(target_map[FWD_DATA_TYPE_HS].name, webpageMtd);
		strcpy(target_map[FWD_DATA_TYPE_DS].name, webpageMtd);
		strcpy(target_map[FWD_DATA_TYPE_CS].name, webpageMtd);
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
		strcpy(current_map[FWD_DATA_TYPE_HS].name, webpageMtd);
		strcpy(current_map[FWD_DATA_TYPE_DS].name, webpageMtd);
		strcpy(current_map[FWD_DATA_TYPE_CS].name, webpageMtd);
#endif
#endif
				
		//linux+webpage+rootfs mtd
		//follow backup_bank and active_bank value
		if(backup_bank==1)
		{
			strcpy(target_map[FWD_DATA_TYPE_BOOT].name, bootMtd);
			strcpy(target_map[FWD_DATA_TYPE_WEB].name, webpageMtd);
			strcpy(target_map[FWD_DATA_TYPE_LINUX].name, linuxMtd);
			strcpy(target_map[FWD_DATA_TYPE_ROOTFS].name, rootfsMtd);
		}else{
			strcpy(target_map[FWD_DATA_TYPE_BOOT].name, bootMtd);
			strcpy(target_map[FWD_DATA_TYPE_WEB].name, webpageMtd2);
			strcpy(target_map[FWD_DATA_TYPE_LINUX].name, linuxMtd2);
			strcpy(target_map[FWD_DATA_TYPE_ROOTFS].name, rootfsMtd2);
		}
		if(active_bank==1)
		{
			strcpy(current_map[FWD_DATA_TYPE_BOOT].name, bootMtd);
			strcpy(current_map[FWD_DATA_TYPE_WEB].name, webpageMtd);
			strcpy(current_map[FWD_DATA_TYPE_LINUX].name, linuxMtd);
			strcpy(current_map[FWD_DATA_TYPE_ROOTFS].name, rootfsMtd);
		}else{
			strcpy(current_map[FWD_DATA_TYPE_BOOT].name, bootMtd);
			strcpy(current_map[FWD_DATA_TYPE_WEB].name, webpageMtd2);
			strcpy(current_map[FWD_DATA_TYPE_LINUX].name, linuxMtd2);
			strcpy(current_map[FWD_DATA_TYPE_ROOTFS].name, rootfsMtd2); 
		}
#else
		{
			// no dual image
			strcpy(target_map[FWD_DATA_TYPE_BOOT].name, bootMtd);
			strcpy(target_map[FWD_DATA_TYPE_WEB].name, webpageMtd);
			strcpy(target_map[FWD_DATA_TYPE_LINUX].name, linuxMtd);
			strcpy(target_map[FWD_DATA_TYPE_ROOTFS].name, rootfsMtd);
#ifdef CONFIG_MTD_NAND
			strcpy(target_map[FWD_DATA_TYPE_HS].name, FLASH_DEVICE_SETTING);
			strcpy(target_map[FWD_DATA_TYPE_DS].name, FLASH_DEVICE_SETTING);
			strcpy(target_map[FWD_DATA_TYPE_CS].name, FLASH_DEVICE_SETTING);
#else
			strcpy(target_map[FWD_DATA_TYPE_HS].name, webpageMtd);
			strcpy(target_map[FWD_DATA_TYPE_DS].name, webpageMtd);
			strcpy(target_map[FWD_DATA_TYPE_CS].name, webpageMtd);
#endif	/* CONFIG_MTD_NAND */
		}
#endif	/* CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE */
	}
#else	/* MTD_NAME_MAPPING */
#error "CONFIG_MTD_NAME_MAPPING must be Y!"
#endif  /* MTD_NAME_MAPPING */

#ifdef CONFIG_FWD_UNIFIED_SUPPORT_PLUGIN
	{
		unsigned char plug_instpath[MAX_NAME_LEN+1] = {0};

		apmib_get(MIB_PLUGIN_INST_PATH, (void *)plug_instpath);
		if(strlen(plug_instpath)!=0 && strlen(plug_instpath)<FWD_MAX_TARGET_NAMESIZE)
			strcpy(target_map[FWD_DATA_TYPE_PLUGIN].name, plug_instpath);
		else
			//use default install path
			strcpy(target_map[FWD_DATA_TYPE_PLUGIN].name, PLUGIN_INST_DEF_PATH);
	}
#endif

	return FWD_SUCCEED;
}

static void dump_mtd_info(struct fwd_partition *target_map, struct fwd_partition *current_map)
{
#if 0
	//keep here for debug!
	int i;
	if(target_map!=NULL)
	{
		printf("------TARGET MAP------\n");
		for(i=0; i<FWD_DATA_TYPE_SIZE; i++)
			printf("[%d]: %s\n", i, target_map[i].name);
	}

	if(current_map!=NULL)
	{
		printf("------CURRENT MAP------\n");
		for(i=0; i<FWD_DATA_TYPE_SIZE; i++)
			printf("[%d]: %s\n", i, current_map[i].name);
	}
#endif
}

static int write_fwd_line_to_file(FILE *fp, unsigned char *line, unsigned int len, char *msg)
{
	int numWrite=0;
	if( (numWrite=fwrite(line, 1, len, fp))<=0 )
	{
		sprintf(msg, "write error line :%s!", line);
		return FWD_FAILED;
	}else{
		sync();
		//debug!
		//printf("(%d:%d)%s", len, numWrite, line);
		return FWD_SUCCEED;
	}
}

/*************************************************
 *************** callback funcs ******************
 *************************************************/
static int fw_validation(char *upload_data, int offset, unsigned char *buffer)
{
	IMG_HEADER_Tp pHeader = (IMG_HEADER_Tp) &upload_data[offset];
	int len = 0, burn_address = 0, ret = FWD_FAILED;
	
#ifdef _LITTLE_ENDIAN_
	len = DWORD_SWAP(pHeader->len);
	burn_address = DWORD_SWAP(pHeader->burnAddr);
#else
	len = pHeader->len;
	burn_address = pHeader->burnAddr;
#endif

	//size
	//printf("\tsize check:");
	if (len>CONFIG_FLASH_SIZE || burn_address+len>CONFIG_FLASH_SIZE) { //len check by sc_yang 
		sprintf(buffer, ("Image len exceed max size 0x%x ! len=0x%x, burnaddr=0x%x\n"), CONFIG_FLASH_SIZE, len, burn_address);
		//printf("NOK\n");
		goto FW_VALID_RET;
	}
	//printf("OK\n");

	//checksum
	//printf("\tchecksum:");
	if ( !fwChecksumOk(&upload_data[sizeof(IMG_HEADER_T)+offset], len)) {
		sprintf(buffer, ("Image checksum mismatched! len=0x%x, checksum=0x%x"), len,
			*((unsigned short *)&upload_data[len-2]) );
		//printf("NOK\n");
		goto FW_VALID_RET;
	}
	//printf("OK\n");
	ret = FWD_SUCCEED;
	
FW_VALID_RET:
	return ret;
}

static int csds_validation(char *upload_data, int offset, unsigned char *buffer)
{
#define MAX_CONFIG_LEN 1024*1024
#define MIN_CONFIG_LEN 8*1024
	PARAM_HEADER_Tp pHeader = NULL;
#ifdef COMPRESS_MIB_SETTING
	COMPRESS_MIB_HEADER_Tp pCompHeader = (COMPRESS_MIB_HEADER_Tp)&upload_data[offset];
	unsigned char *expPtr=NULL, *ptr=NULL;
	unsigned int expandLen=0, compLen=0, expAllocSize=0, ret=FWD_FAILED;
	unsigned short compRate=0;

	compRate = WORD_SWAP(pCompHeader->compRate);
	compLen = DWORD_SWAP(pCompHeader->compLen);
	
	/* decompress */
#ifdef RTK_MIB_TAG_CHECK
	expAllocSize = compLen*WORD_SWAP(pCompHeader->realcompRate);
#else
	expAllocSize = compLen*compRate;
#endif
	expPtr = (unsigned char *)malloc(expAllocSize);
	if (NULL==expPtr) {
		sprintf(buffer, "malloc %d bytes for expPtr error!\n", expAllocSize);
		goto CSDS_VALID_RET;
	}
	expandLen = Decode(upload_data+offset+sizeof(COMPRESS_MIB_HEADER_T), compLen, expPtr);
	if(0==expandLen)
	{
		sprintf(buffer, "decode compressed setting failed!\n");
		goto CSDS_VALID_RET;
	}
	pHeader = (PARAM_HEADER_Tp)expPtr;
#else /* not COMPRESS_MIB_SETTING */
	pHeader = (PARAM_HEADER_Tp)&upload_data[offset];
#endif

	//size
	//printf("\tsize check:");
	{
#ifdef _LITTLE_ENDIAN_
		pHeader->len = HEADER_SWAP(pHeader->len);
#endif
        if((pHeader->len > MAX_CONFIG_LEN)||(pHeader->len < MIN_CONFIG_LEN))
        {
            sprintf(buffer, "INVALID config.data, oversize (%d bytes)\n", pHeader->len);
			//printf("NOK\n");
            goto CSDS_VALID_RET;
        }else{
			//printf("OK\n");
        }
    }

	//signature
	//printf("\tsignature check:");
	{
		if (!memcmp(pHeader->signature, CURRENT_SETTING_HEADER_TAG, TAG_LEN)
			|| !memcmp(pHeader->signature, CURRENT_SETTING_HEADER_FORCE_TAG, TAG_LEN)
			|| !memcmp(pHeader->signature, CURRENT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN)
			|| !memcmp(pHeader->signature, DEFAULT_SETTING_HEADER_TAG, TAG_LEN)
			|| !memcmp(pHeader->signature, DEFAULT_SETTING_HEADER_FORCE_TAG, TAG_LEN)
			|| !memcmp(pHeader->signature, DEFAULT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN)
			) 
		{
			//printf("OK\n");
		}else{
			sprintf(buffer, "Invalid signature %02x%02x\n", pHeader->signature[0], pHeader->signature[1]);
			//printf("NOK\n");
			goto CSDS_VALID_RET;
		}
	}

	//checksum
	//printf("\tchecksum:");
	{
		//decode
#ifdef COMPRESS_MIB_SETTING
		ptr = (char *)(expPtr+sizeof(PARAM_HEADER_T));
#else
		ptr = &upload_data[offset];
		DECODE_DATA(ptr, pHeader->len);
#endif
		//do checksum
		if ( !CHECKSUM_OK((unsigned char *)ptr, pHeader->len)) {
            sprintf(buffer, "cs/ds data checksum error!\n");
			//printf("NOK\n");
			goto CSDS_VALID_RET;
		}else{
			//printf("OK\n");
		}
	}
	ret = FWD_SUCCEED;
	
CSDS_VALID_RET:
	return ret;
}

#ifdef CONFIG_FWD_UNIFIED_SUPPORT_PLUGIN
static int plugin_validation(char *upload_data, int offset, unsigned char *buffer)
{
	PLUGIN_HEADER_Tp pHeader = (IMG_HEADER_Tp) &upload_data[offset];
	int len = 0, ret = FWD_FAILED;
	unsigned short padding = 0;

	padding = WORD_SWAP(pHeader->padding);
	len = DWORD_SWAP(pHeader->len);
	
	//checksum
	printf("\tchecksum:");
	if (!fwChecksumOk(&upload_data[offset+sizeof(PLUGIN_HEADER_T)-sizeof(pHeader->cksum)], len+padding+sizeof(pHeader->cksum))) 
	{
		sprintf(buffer, ("Plugin Image checksum mismatched! len=0x%x, checksum=0x%x"), len,
			*((unsigned short *)&upload_data[len-2]) );
		printf("NOK\n");
		goto FW_VALID_RET;
	}
	printf("OK\n");
	ret = FWD_SUCCEED;
	
FW_VALID_RET:
	return ret;
}
#endif

//boot.bin callback
static int boot_getBurnAddr(char *pHeader, char *logbuf, int flag, int backup_bank)
{
	return 0;//forever flash 0 address.
}
static int boot_getBurnSize(char *pHeader, char *logbuf, int flag, int idx)
{	
	int burn_size = -1;
	IMG_HEADER_Tp hdr = (IMG_HEADER_Tp)pHeader;
	
#ifdef _LITTLE_ENDIAN_
	burn_size  = DWORD_SWAP(hdr->len);
#else
	burn_size = hdr->len;
#endif

	return burn_size;
}

//linux.bin callback
static int linux_getBurnAddr(char *pHeader, char *logbuf, int flag, int backup_bank)
{
	int burn_address = -1;
	IMG_HEADER_Tp hdr = (IMG_HEADER_Tp)pHeader;
	
#ifdef _LITTLE_ENDIAN_
	burn_address = DWORD_SWAP(hdr->burnAddr);
#else
	burn_address = hdr->burnAddr;
#endif

	if(!IS_BIT_SET(flag, FWD_SUPPORT_OFFSET_CHANGE))
	{
#ifdef CONFIG_MTD_NAND
		burn_address -= NAND_BOOT_SETTING_SIZE;
#endif
	}

	if(IS_BIT_SET(flag, FWD_SUPPORT_OFFSET_CHANGE))
	{
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
		if(backup_bank==2)
			burn_address += CONFIG_RTL_FLASH_DUAL_IMAGE_OFFSET;
#endif
	}
	return burn_address;
}

//root.bin callback
static int root_getBurnAddr(char *pHeader, char *logbuf, int flag, int backup_bank)
{
	int burn_address = -1;

	if(IS_BIT_SET(flag, FWD_SUPPORT_OFFSET_CHANGE))
	{
		IMG_HEADER_Tp hdr = (IMG_HEADER_Tp)pHeader;
#ifdef _LITTLE_ENDIAN_
		burn_address = DWORD_SWAP(hdr->burnAddr);
#else
		burn_address = hdr->burnAddr;
#endif

#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
		if(backup_bank==2)
			burn_address += CONFIG_RTL_FLASH_DUAL_IMAGE_OFFSET;
#endif
	}else{
		burn_address = 0;
	}

	return burn_address;
}

static int fw_getBurnSize(char *pHeader, char *logbuf, int flag, int idx)
{
	int burn_size = -1;
	IMG_HEADER_Tp hdr = (IMG_HEADER_Tp)pHeader;
	
#ifdef _LITTLE_ENDIAN_
	burn_size  = DWORD_SWAP(hdr->len);
#else
	burn_size = hdr->len;
#endif

	return burn_size;
}

//ds callback
static int ds_getBurnAddr(char *pHeader, char *logbuf, int flag, int backup_bank)
{
	return DEFAULT_SETTING_OFFSET;
}

//cs callback
static int cs_getBurnAddr(char *pHeader, char *logbuf, int flag, int backup_bank)
{
	return CURRENT_SETTING_OFFSET;
}

static int cfg_getBurnSize(char *pHeader, char *logbuf, int flag, int idx)
{
	int burn_size = -1;
	COMPRESS_MIB_HEADER_Tp hdr = (IMG_HEADER_Tp)pHeader;
	
#ifdef _LITTLE_ENDIAN_
	burn_size  = DWORD_SWAP(hdr->compLen);
#else
	burn_size = hdr->compLen;
#endif

	return burn_size;
}

#ifdef CONFIG_FWD_UNIFIED_SUPPORT_PLUGIN
static int plugin_getBurnAddr(char *pHeader, char *logbuf, int flag, int backup_bank)
{
	return 0;
}

static int plugin_getBurnSize(char *pHeader, char *logbuf, int flag, int idx)
{
	PLUGIN_HEADER_Tp pPlugHdr = (PLUGIN_HEADER_Tp)pHeader;
	int len = 0;

	//only return payload length, which is to be burn.
	len = DWORD_SWAP(pPlugHdr->len);
	return len;
}
#endif

/*************************************************
 ****************** local APIs *******************
 *************************************************/
static int add_fwd_conf_fileheader(FILE *fp, int flag, unsigned char *msg)
{
	extern int get_shm_id();
	int shm_id = get_shm_id();
	unsigned char line[FWD_MAX_LINE_SIZE]={0};

	//Raw Data Source
	{
		//from
		int from = IS_BIT_SET(flag, FWD_DATA_SOURCE);
		sprintf(line, "%s :%d\n", FWD_CONF_FROM, from);
		if(FWD_FAILED==write_fwd_line_to_file(fp, line, strlen(line), msg))
			return FWD_FAILED;

		if(from)
		{
			// FWD_DATA_IN_FILE, name in msg
			if(msg==NULL)
			{
				printf("filename buffer is NULL pointer!\n");
				return FWD_FAILED;
			}
			snprintf(line, FWD_MAX_LINE_SIZE, "%s :%s\n", FWD_CONF_FILENAME, msg);
			if(FWD_FAILED==write_fwd_line_to_file(fp, line, strlen(line)>FWD_MAX_LINE_SIZE?FWD_MAX_LINE_SIZE:strlen(line), msg))
				return FWD_FAILED;
		}else{
			// FWD_DATA_IN_SHM, shmid
			sprintf(line, "%s :%d\n", FWD_CONF_SHMID, shm_id);
			if(FWD_FAILED==write_fwd_line_to_file(fp, line, strlen(line), msg))
				return FWD_FAILED;
		}
	}

	//reboot_delay
	{
		int delay = IS_BIT_SET(flag, FWD_REBOOT_DELAY);
		sprintf(line, "%s :%d\n", FWD_CONF_REBOOT, delay);
		if(FWD_FAILED==write_fwd_line_to_file(fp, line, strlen(line), msg))
			return FWD_FAILED;
	}
	
	return FWD_SUCCEED;
}

static int add_fwd_conf_section(FILE *fp, int secIdx, int flag, void *upload_data, unsigned int loc_offset, char *msg)
{
	unsigned char line[FWD_MAX_LINE_SIZE]={0};
	char *pHeader = (char *)upload_data + loc_offset;
	int type = FWD_SECTION_TBL[secIdx].sectype;
	int support_offset_change = IS_BIT_SET(flag, FWD_SUPPORT_OFFSET_CHANGE);
	int active_bank=0, backup_bank=0, dual_enable=0;
	struct fwd_partition targetMap[FWD_DATA_TYPE_SIZE], currentMap[FWD_DATA_TYPE_SIZE];

	//get mtd map
	//FIXME:
	//		if JFFS2 is used, HS/DS/CS are in files, no need to set offset!
	memset(targetMap, 0x0, FWD_DATA_TYPE_SIZE*sizeof(struct fwd_partition));
	memset(currentMap, 0x0, FWD_DATA_TYPE_SIZE*sizeof(struct fwd_partition));
	if(FWD_FAILED == get_mtd_info(targetMap, currentMap, support_offset_change, msg))
	{
		sprintf(msg, "get mtd info failed!\n");
		return FWD_FAILED;
	}else{
		// debug!
		dump_mtd_info(targetMap, currentMap);
	}
	
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	apmib_get(MIB_DUALBANK_ENABLED,(void *)&dual_enable);	
	get_bank_info(dual_enable,&active_bank,&backup_bank);
#endif

	//type
	{
		sprintf(line, "%s :%d\n", FWD_CONF_TYPE, type);
		if(FWD_FAILED==write_fwd_line_to_file(fp, line, strlen(line), msg))
			return FWD_FAILED;
	}

	//target
	{	
#ifdef CONFIG_FWD_UNIFIED_SUPPORT_PLUGIN
		if(secIdx==FWD_DATA_TYPE_PLUGIN)
		{
			PLUGIN_HEADER_Tp pPlugHdr = (PLUGIN_HEADER_Tp)pHeader;
			//for plugin, targetMap.name is only install directory name, should append plugin name
			snprintf(line, FWD_MAX_LINE_SIZE-1, "%s :%s/%s\n", FWD_CONF_TARGET, targetMap[type].name, pPlugHdr->name);
		}else
#endif
		sprintf(line, "%s :%s\n", FWD_CONF_TARGET, targetMap[type].name);
		if(FWD_FAILED==write_fwd_line_to_file(fp, line, strlen(line), msg))
			return FWD_FAILED;
	}
	
	//burn address
	{
		// FIXME:
		//		if JFFS2 is used, HS/DS/CS are in files, no need to set offset!
		int burn_address = -1;
		
		if(FWD_SECTION_TBL[secIdx].getBurnAddr)
			burn_address = (*FWD_SECTION_TBL[secIdx].getBurnAddr)(pHeader, msg, flag, backup_bank);
		else{
			sprintf(msg, "getBurnAddr() is not defined in secIdx %d\n", secIdx);
			return FWD_FAILED;
		}
		
		if( burn_address<0 ){
			sprintf(msg, "getBurnAddr() return invalid\n");
			return FWD_FAILED;
		}else{
			sprintf(line, "%s :%d\n", FWD_CONF_BURN_ADDR, burn_address);
			if(FWD_FAILED==write_fwd_line_to_file(fp, line, strlen(line), msg))
				return FWD_FAILED;
		}
	}
	
	//burn size
	{
		int burn_size = -1;

		if(FWD_SECTION_TBL[secIdx].getBurnSize)
			burn_size = (*FWD_SECTION_TBL[secIdx].getBurnSize)(pHeader, msg, flag, secIdx);
		else{
			sprintf(msg, "getBurnSize() is not defined in secIdx %d\n", secIdx);
			return FWD_FAILED;
		}
		
		if( burn_size<0 ){
			sprintf(msg, "getBurnSize() return invalid\n");
			return FWD_FAILED;
		}else{
			if(FWD_SECTION_TBL[secIdx].hdr_type&FWD_HDR_FLAG_SKIPHDR)
			{
				//skip header, do nothing
			}else{
				//burn header, add length
				burn_size += (FWD_SECTION_TBL[secIdx].hdr_type);
			}
			
			sprintf(line, "%s :%d\n", FWD_CONF_BURN_SIZE, burn_size);
			if(FWD_FAILED==write_fwd_line_to_file(fp, line, strlen(line), msg))
				return FWD_FAILED;
		}
	}
	
	//data offset in raw data
	{
		if(FWD_SECTION_TBL[secIdx].hdr_type&FWD_HDR_FLAG_SKIPHDR)
		{
			//skip header, step forward
			loc_offset += (FWD_SECTION_TBL[secIdx].hdr_type&(~FWD_HDR_FLAG_SKIPHDR));
		}else{
			//burn header, do nothing
		}
		
		sprintf(line, "%s :%d\n", FWD_CONF_DATA_OFFSET, loc_offset);
		if(FWD_FAILED==write_fwd_line_to_file(fp, line, strlen(line), msg))
			return FWD_FAILED;
	}

#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	//set bookmark for new fw
	if(type==FWD_DATA_TYPE_LINUX)
	{
		IMG_HEADER_Tp hdr = (IMG_HEADER_Tp)pHeader;
		unsigned long ret = 0;

		ret = get_next_bankmark(currentMap[FWD_DATA_TYPE_LINUX].name, 
					dual_enable, support_offset_change, active_bank, msg);
		if(ret==NO_IMAGE_BANK_MARK)
			return FWD_FAILED;
		else{
			hdr->burnAddr = ret;
			//debug!
			//printf("get_next_bankmark returns:0x%x\n", hdr->burnAddr);
		}
	}
#endif

	return FWD_SUCCEED;
}

static int add_fwd_ready_file(unsigned char *msg)
{
	int pid = getpid();
	unsigned char line[FWD_MAX_LINE_SIZE] = {0};
	FILE *fp = fopen(FWD_READYFILE, "w+");

	if(fp==NULL)
		return FWD_FAILED;
	
	sprintf(line, "%s :%d\n", FWD_READY_PID, pid);
	if(FWD_FAILED==write_fwd_line_to_file(fp, line, strlen(line), msg)){
		fclose(fp);
		return FWD_FAILED;
	}else{
		fclose(fp);
		return FWD_SUCCEED;
	}
}


/*************************************************
 *************** EXTERNAL APIS *******************
 *************************************************/
int get_shm_id()
{
	return shm_id;
}

static int set_shm_id(int id)
{
	 shm_id=id;
	 return 0; //fix converity error: MISSING_RETURN
}

int clear_fwupload_shm(int shmid)
{
	//printf("[%s]shm_memory :%p\n", __FUNCTION__, shm_memory);
	if(shm_memory){
		if (shmdt(shm_memory) == -1) {
			fprintf(stderr, "shmdt failed\n");
		}
	}

	if (shm_id != 0) {
		if(shmctl(shm_id, IPC_RMID, 0) == -1)
			fprintf(stderr, "shmctl(IPC_RMID) failed\n");
	}
	shm_id = 0;
	shm_memory = NULL;
	return 0; //fix converity error: MISSING_RETURN
}

char * setup_fwupload_shm(unsigned char *pathname, unsigned int alloc_size)
{
	if( pathname==NULL || alloc_size==0 )
		return NULL;
	
	/* Generate a System V IPC key */ 
	key_t key;
	key = ftok(pathname, 0xE04);

	/* Allocate a shared memory segment */
	set_shm_id(shmget(key, alloc_size, IPC_CREAT | IPC_EXCL | 0666));
	if (get_shm_id() == -1) {
		printf("shm alloc size too big!(%d)\n", alloc_size);
		return NULL;
	}
	
	/* Attach the shared memory segment */
	shm_memory = (char *)shmat(get_shm_id(), NULL, 0);
	
	return shm_memory;
}

/*
 * parse upload information to FWD_CONFFILE
 * upload_data:
 *		if flag | FWD_DATA_IN_FILE, it's the filename of raw upgrade data.
 *		if flag | FWD_DATA_IN_BUFFER, it's the share memory start pointer that stores raw upgrade data.
 * upload_len:
 *		raw upgrade data total length
 * flag:
 *		BIT(0): FWD_DATA_SOURCE
 *				If set, FWD_DATA_IN_FILE, otherwise, FWD_DATA_IN_SHM.
 *		BIT(1): FWD_REBOOT_DELAY
 *				If set, FWD will not do watchdog reboot after successful burning.
 *				Otherwise, FWD will do watchdog reboot.
 *      BIT(2): If set, take whole flash as one mtdblock, which can support offset change in new firmware.
 *				Otherwise, burn each section to flash according to current flash layout.
 *		BIT(3): If set, FWD ready file will be generated when valid section exists, even if there's invalid section.
 *				Otherwise, rtl_upgrade_proceed() will return FWD_FAILED when invalid section encountered.
 * buffer:
 *		error message buffer for return. At lease FWD_MAX_LINE_SIZE bytes size.
 * Return value:
 *		FWD_SUCCEED: succeed
 *		FWD_FAILED : failed.
 */
int rtl_upgrade_proceed(char *upload_data, int upload_len, int flag, char *buffer)
{
	if(upload_data==NULL || buffer==NULL || upload_len==0)
	{
		//printf("%s invalid argument!", __FUNCTION__);
		return FWD_FAILED;
	}else{
#if 0
		//debug!
		printf("flag:%x, data:%p, len:%d\n", flag, upload_data, upload_len);
		int i=0;
		for(i=0; i<32; i++)
			if(i!=0&&i%8==0)
				printf("\n");
			else
				printf("%02x ", upload_data[i]);
#endif
	}
	
	int secinfo_idx=-1, head_offset=0, len=0;
	IMG_HEADER_Tp pHeader;
	COMPRESS_MIB_HEADER_Tp pConHdr;
	int ret=FWD_FAILED, i=0;
	unsigned char isValidfw = 0;
	FILE *fp = NULL;
	
	fp=fopen(FWD_CONFFILE, "w+");
	if(fp==NULL)
	{
		sprintf(buffer, "open %s error!", FWD_CONFFILE);
		goto ret_upload;
	}
	
	//strip raw data prefix
	head_offset = find_head_offset(upload_data);
	//printf("####%s:%d head_offset=%d upload_data=%p###\n",  __FILE__, __LINE__ , head_offset, upload_data);
	if (head_offset == -1) {
		sprintf(buffer, "Invalid file format! head_offset=-1\n");
		goto ret_upload;
	}
#ifdef DEFSETTING_AUTO_UPDATE
	system("flash save-defsetting");
#endif

	//write conf file header
	if(FWD_FAILED == add_fwd_conf_fileheader(fp, flag, buffer))
	{
		goto ret_upload;
	}
	
	//parse raw data
	while ( (head_offset+sizeof(IMG_HEADER_T)) < upload_len ) 
	{
		//check header type
		printf("parse at data offset 0x%x, secinfo_idx:", head_offset);
		for(i=0; i<FWD_SECTION_TBLSIZE; i++)
		{
			if(!memcmp(&upload_data[head_offset], FWD_SECTION_TBL[i].signature, FWD_SECTION_TBL[i].sig_len))
			{
				//found matching signature
				isValidfw = 1;
				secinfo_idx = i;
			}
		}
		printf("%d\n", secinfo_idx);
		if(secinfo_idx==-1)
		{
			//invalid section
			if(IS_BIT_SET(flag, FWD_FORCE_BURN) && isValidfw==1)
			{
				printf("upgrade data contains invalid content, ignore it!\n");
				break;
			}else{
				//detect invalid section, return failed.
				sprintf(buffer, ("Invalid file format!"));
				ret = FWD_FAILED;
				goto ret_upload;
			}
		}

		//data validation
		printf("data validation:");
		if(FWD_SECTION_TBL[secinfo_idx].validate)
		{
			ret = (*FWD_SECTION_TBL[secinfo_idx].validate)(upload_data, head_offset, buffer);
			if(ret==FWD_FAILED)
				goto ret_upload;
		}
		printf("OK\n");

		//add to fwd conf file
		printf("generate CONF file:");
		if(FWD_FAILED==add_fwd_conf_section(fp, secinfo_idx, flag, (void *)upload_data, head_offset, buffer))
		{
			printf("conf file NOT generated.\n");
			goto ret_upload;
		}else{
			printf("OK\n");
			//prepare for next round, step head_offset forward
			
			int step = -1;
			if(FWD_SECTION_TBL[secinfo_idx].getBurnSize)
				step = (*FWD_SECTION_TBL[secinfo_idx].getBurnSize)((char *)&upload_data[head_offset], buffer, flag, secinfo_idx);

			if( step<0 )
			{
				ret = FWD_FAILED;
				goto ret_upload;
			}else{
				step += (FWD_SECTION_TBL[secinfo_idx].hdr_type&(~FWD_HDR_FLAG_SKIPHDR));
			}

			head_offset += step;
		}

		secinfo_idx = -1;
	}
	
	printf("generate READY file:");
	if(FWD_FAILED==add_fwd_ready_file(buffer))
	{
		printf("ready file NOT generated.\n");
		goto ret_upload;
	}
	printf("OK.\n");
	ret = FWD_SUCCEED;
	
ret_upload: 
	fprintf(stderr, "%s\n", buffer);
	
	if(fp) fclose(fp);
	
	return ret;
}
	
int rtl_watchdog_reboot(unsigned char *msg)
{
	FILE *fp_watchdog = NULL;

	fp_watchdog = fopen("/proc/watchdog_reboot","w+"); //reboot
	if(fp_watchdog)
	{
		printf("%s!!! Reason:%s\n", __FUNCTION__, (msg==NULL)?"Unknown":msg);

		fflush(0);
		fflush(1);
		fflush(2);
		
		fputs("111", fp_watchdog);
		fclose(fp_watchdog);
	}

	for(;;);
}

/*
 * API rtl_api_upgrade should be non-reentrant. That is,
 * two processes can not burn firmware/configuration at the same time!
 * Use semaphore to sync.
 */
/*
 * Return value: return the fwd lockfile fd.
 */
int rtl_upgrade_start(int pid)
{
	int fd = open(FWD_LOCKFILE, O_CREAT|O_RDWR);
	if(-1==fd)
	{
		printf("[%s:%d] open lock file error!\n", __FUNCTION__, __LINE__);
		return FWD_FAILED;
	}
	
	flock(fd, LOCK_EX);
	
	//printf("rtl_upgrade_start by pid %d...\n", pid);
	return fd;
}

/*
 * pid: getpid()
 * fd : return value of rtl_upgrade_start()
 */
void rtl_upgrade_done(int pid, int fd)
{
	//printf("rtl_upgrade_done by pid %d!!!\n", pid);

	flock(fd, LOCK_UN);
	close(fd);
	fd = -1;
	
	return ;
}

/*
 * pid:    callee pid, getpid()
 * status: return FWD_STATUS in file FWD_STATFILE_PREFIX_pid 
 * msg:    return the detailed message in file FWD_STATFILE_PREFIX_pid. 
 *         Size at least FWD_MAX_LINE_SIZE bytes
 */
int rtl_upgrade_status(int pid, int *status, unsigned char *msg)
{
	if(status==NULL||pid<=0||msg==NULL)
	{
		return FWD_FAILED;
	}
	
	unsigned char filename[64]={0}, line[FWD_MAX_LINE_SIZE*2]={0};//at most to lines in status file
	int fd = -1, ret;

	sprintf(filename, "%s%d", FWD_STATFILE_PREFIX, pid);
	fd = open(filename, O_RDONLY);
	if(-1==fd)
	{
		sprintf(msg, "open %s failed\n", filename);
		return FWD_FAILED;
	}

	/* file lock */
	flock(fd, LOCK_EX);

	ret = read(fd, line, sizeof(line));
	if(ret<=0)
	{
		sprintf(msg, "read %s failed\n", filename);
		goto UPGD_STATUS_RET;
	}else{
		unsigned char *ptr = NULL;
		sscanf(line, "%*s :%d\n", status);
		// sscanf is not used for msg, since msg may white space split string.
		if((ptr=strstr(line, FWD_STAT_MSG))!=NULL)
		{
			strcpy(msg, ptr+strlen(FWD_STAT_MSG)+strlen(" :"));
		}
		// debug!
		//printf("[%s]: status:%d, msg:%s\n", __FUNCTION__, *status, msg);
	}

UPGD_STATUS_RET:
	/* file unlock */
	flock(fd, LOCK_UN);

	close(fd);
	return FWD_SUCCEED;
}

#endif //CONFIG_FWD_UNIFIED_FRAMEWORK

/**************************************************
 *************** wlan vdev support ****************
 **************************************************/
#ifdef WLAN_VDEV_SUPPORT
static int getWlanMib(int wlanRootIndex, int wlanValIndex, int id, void *value)
{
	int root_old, val_old;

	root_old = wlan_idx;
	val_old = vwlan_idx;

	wlan_idx = wlanRootIndex;
	vwlan_idx = wlanValIndex;
	
	apmib_get(id, (void *)value);
	
	wlan_idx = root_old;
	vwlan_idx = val_old;
	
	return 0;
}

static void get_wlan_idx_from_name(char *ifname, int *cur_rootIdx, int *cur_vwlanIdx)
{
	if(strstr(ifname, "va"))
	{
		sscanf(ifname, "wlan%d-va%d", cur_rootIdx, cur_vwlanIdx);
		*cur_vwlanIdx += 1;
	}else if(strstr(ifname, "vxd")){
		sscanf(ifname, "wlan%d-vxd", cur_rootIdx);
		*cur_vwlanIdx = 5;
	}else{
		sscanf(ifname, "wlan%d", cur_rootIdx);
		*cur_vwlanIdx = 0;
	}
}

static int wlan_vdev_get_type(int wan_idx)
{
	int vdev_type=0, v4_type=0, v6_type=0;
	WANIFACE_T wan_entity;
	
	*((char *)&wan_entity) = (char)wan_idx;
	apmib_get(MIB_WANIFACE_TBL, (void *)&wan_entity);

	/*WLAN_VDEV_IP_PROTO_BITMASK, BIT 3-2*/
#ifdef CONFIG_CMCC
	if(wan_entity.IpProtocol==IP_PROTOCOL_MODE_IPV4 || wan_entity.IpProtocol==IP_PROTOCOL_MODE_BOTH)
		vdev_type |= WLAN_VDEV_IPPROTO_V4;
#ifdef CONFIG_IPV6
	if(wan_entity.IpProtocol==IP_PROTOCOL_MODE_IPV6 || wan_entity.IpProtocol==IP_PROTOCOL_MODE_BOTH)
		vdev_type |= WLAN_VDEV_IPPROTO_V6;
#endif
#endif/*CONFIG_CMCC*/

	/*WLAN_VDEV_BR_PROTO_BITMASK, BIT 1-0*/
	v4_type = wan_entity.AddressType;
	if(v4_type == BRIDGE)
		vdev_type |= WLAN_VDEV_FOR_V4;
#ifdef CONFIG_IPV6
	v6_type = wan_entity.ipv6Enable?wan_entity.ipv6Origin:0;
	if(v6_type == IPV6_ORIGIN_BRIDGE)
		vdev_type |= WLAN_VDEV_FOR_V6;
#endif

#ifdef CONFIG_CMCC
	/*OPTION: IP version mismatch drop, BIT 4*/
	vdev_type |= WLAN_VDEV_OPT_IP_DROP;

	/*OPTION: DHCP trap to br0, BIT 5*/
	if(wan_entity.AddressType==BRIDGE && wan_entity.applicationtype==APPTYPE_INTERNET)
		//Mode: bridge + ApplicationType: Internet
		vdev_type |= WLAN_VDEV_OPT_DHCP_TRAP;
#endif
	
	return vdev_type;
}

/*
 * enable a specified wlan vdev, only work in force mode.
 * @dev_name: the real device name of expected vdev
 * @br_name: the bridge name which vdev is expected to add
 */
void wlan_vdev_enable(char *dev_name, char *br_name)
{
	char cmdBuf[128];
	int cur_rootIdx=0, cur_vwlanIdx=0, wlan_disabled=0, vdev_type=0, wan_idx=0;
	
	//only enable vdev when real device is enable
	get_wlan_idx_from_name(dev_name, &cur_rootIdx, &cur_vwlanIdx);
	getWlanMib(cur_rootIdx, cur_vwlanIdx, MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disabled);
	if(wlan_disabled)
		return ;
	else{
		//check whether the root wlan interface is disabled
		getWlanMib(cur_rootIdx, 0, MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disabled);
		if(wlan_disabled)
			return;
	}

	//get vdev_type
	if(sscanf(br_name, "br%d", &wan_idx)!=1)
		return ;
	vdev_type = wlan_vdev_get_type(wan_idx);
	
	snprintf(cmdBuf, sizeof(cmdBuf), "echo %d > /proc/%s/vdev", vdev_type, dev_name);
	printf("[%s:%d] %s\n", __FUNCTION__, __LINE__, cmdBuf);
	system(cmdBuf);

	snprintf(cmdBuf, sizeof(cmdBuf), "brctl addif %s v%s", br_name, dev_name);
	printf("[%s:%d] %s\n", __FUNCTION__, __LINE__, cmdBuf);
	system(cmdBuf);

	snprintf(cmdBuf, sizeof(cmdBuf), "ifconfig v%s up", dev_name);
	printf("[%s:%d] %s\n", __FUNCTION__, __LINE__, cmdBuf);
	system(cmdBuf);

	return ;
}

/*
 * disable a specified wlan vdev
 * @dev_name: the real device name of expected vdev
 * @br_name: the bridge name which vdev is expected to add
 * @force: force disable whether or not interface is disabled
 */
void wlan_vdev_disable(char *dev_name, char *br_name, int force)
{
	char cmdBuf[128];

	if(!force)
	{
		//if interface is disabled, return
		int cur_rootIdx=0, cur_vwlanIdx=0, wlan_disabled=0;
	
		get_wlan_idx_from_name(dev_name, &cur_rootIdx, &cur_vwlanIdx);
		getWlanMib(cur_rootIdx, cur_vwlanIdx, MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disabled);
		if(wlan_disabled)
			return ;
		else{
			//check whether the root wlan interface is disabled
			getWlanMib(cur_rootIdx, 0, MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disabled);
			if(wlan_disabled)
				return;
		}
	}
	
	snprintf(cmdBuf, sizeof(cmdBuf), "ifconfig v%s down", dev_name);
	printf("[%s:%d] %s\n", __FUNCTION__, __LINE__, cmdBuf);
	system(cmdBuf);

	snprintf(cmdBuf, sizeof(cmdBuf), "brctl delif %s v%s", br_name, dev_name);
	printf("[%s:%d] %s\n", __FUNCTION__, __LINE__, cmdBuf);
	system(cmdBuf);

	snprintf(cmdBuf, sizeof(cmdBuf), "echo 0 > /proc/%s/vdev", dev_name);
	printf("[%s:%d] %s\n", __FUNCTION__, __LINE__, cmdBuf);
	system(cmdBuf);

}

/*
 * enable all possible wlan vdev under wlan root index root_wlan_idx,
 * including root wlan itself.
 * @root_wlan_idx: the root_idx of wlan that is to operate
 * @action: 1-enable all, 0-disable all
 * @force: force action whether or not interface is disabled
 */
void wlan_vdev_action_all(int root_wlan_idx, int action, int force)
{
	int vwlan = 0, i, cur_wlan_idx=0, cur_vwlan_idx=0, wlan_disabled=0;
	int wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM]={0};
	char ifname[32] = {0}, brname[32] = {0}, portname[32] = {0};
	int wan_type=0;

	apmib_get(MIB_WANIFACE_BINDING_LAN_PORTS, (void *)wanBindingLanPorts);
	sprintf(ifname, "wlan%d", root_wlan_idx);

	//check wlan interfaces only
	for(i=WAN_INTERFACE_LAN_PORT_NUM; i<sizeof(wanBindingLanPorts)/sizeof(int); i++)
	{
		//skip unbind wlan port
		if(wanBindingLanPorts[i]==0)
			continue;

		if(getLanPortIfName(portname, i)!=1)
			continue;

		if(wanBindingLanPorts[i] > 0 && wanBindingLanPorts[i] < WANIFACE_NUM)
		{
			apmib_set(MIB_WANIFACE_CURRENT_IDX, (void*)&wanBindingLanPorts[i]);
			apmib_get(MIB_WANIFACE_ADDRESSTYPE,&wan_type);
		}

		if(wan_type != BRIDGE)
			continue;

		//skip wlan interface not under root_wlan_idx
		if(strncmp(portname, ifname, strlen(ifname))!=0)
			continue;

		//skip vxd interface
		if(strstr(portname, "vxd"))
			continue;

		if(!force)
		{
			//skip disabled interface if not force mode
			get_wlan_idx_from_name(portname, &cur_wlan_idx, &cur_vwlan_idx);
			getWlanMib(cur_wlan_idx, cur_vwlan_idx, MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disabled);
			if(wlan_disabled)
				continue;
			else{
				//check whether the root wlan interface is disabled
				getWlanMib(cur_wlan_idx, 0, MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disabled);
				if(wlan_disabled)
					return;
			}
		}
		
		//now portname is the expected wlan interface.
		//get brname and vdev_type
		sprintf(brname, "br%d", wanBindingLanPorts[i]);

		if(action)
			wlan_vdev_enable(portname, brname);
		else
			wlan_vdev_disable(portname, brname, force);
	}
}
#endif

#ifdef MULTI_WAN_SUPPORT
int notifyDhcpToSendRelesePkt(int wan_idx)
{
	FILE *fp=NULL;
	int dhcpc_pid, wan_addr_type;
	char cmd_buf[64]={0}, pid_file[64]={0};

	if(wan_idx<1 || wan_idx>WANIFACE_NUM)
	{
		printf("\n%s:%d Invalid wan_idx=%d\n",__FUNCTION__,__LINE__,wan_idx);
		return -1;
	}

	apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&wan_idx);
	apmib_get(MIB_WANIFACE_ADDRESSTYPE,(void*)&wan_addr_type);
	if(wan_addr_type!=DHCP_CLIENT)
		return -1;

	snprintf(pid_file, sizeof(pid_file), "/var/run/wan_%d.pid", wan_idx);	
	if((fp=fopen(pid_file, "r"))!=NULL)
	{
		fscanf(fp, "%d", &dhcpc_pid);
		if(dhcpc_pid > 0)
		{
			//printf("%s:%d##dhcpc_pid=%d\n",__FUNCTION__,__LINE__,dhcpc_pid);
			snprintf(cmd_buf, sizeof(cmd_buf), "kill -SIGUSR2 %d", dhcpc_pid);	
			system(cmd_buf);
			sleep(1);//wait dhcpc daemon to send release packet
		}
		fclose(fp);
		return 0;
	}
	return -1;	
}

int getWanIfNameByWanIdx(int wan_dix, char *wan_ifname)
{
	FILE *fp=NULL;
	char file_name[64]={0};
	if(wan_idx<1 || wan_idx>WANIFACE_NUM || wan_ifname==NULL)
	{
		printf("\n%s:%d Invalid argument!\n",__FUNCTION__,__LINE__);
		return -1;
	}
	snprintf(file_name, sizeof(file_name), "/var/wan_interface_%d", wan_dix);
	fp=fopen(file_name, "r");
	if(fp)
	{
		fscanf(fp, "%s",wan_ifname);				
		fclose(fp);
		return 0;
	}

	return -1;	
}

#endif

int sendSignalToDaemon(char *pid_file, int signal)
{	
	int pid;
	FILE *fp=NULL;	
	char line_buffer[64]={0};

	if(pid_file==NULL || signal<0)
		return -1;
	
	if ((fp = fopen(pid_file, "r")) != NULL) 
	{
		fgets(line_buffer, sizeof(line_buffer), fp);
		fclose(fp);
		if (sscanf(line_buffer, "%d", &pid) && pid>1) 
		{
			kill(pid, signal);
			return 0;			
		}		
	}
	return -1;
}

