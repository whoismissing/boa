/*
 *  Boa, an http server
 *  This file Copyright (C) 2002 Peter Korsgaard <jacmet@sunsite.dk>
 *  Some changes Copyright (C) 2003-2004 Jon Nelson <jnelson@boa.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if defined (CONFIG_RTL_IGMP_SNOOPING)||defined (CONFIG_ISP_IGMPPROXY_MULTIWAN)
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/reboot.h>
#include <unistd.h>
#include "boa.h"
#include "asp_page.h"
#include "apmib.h"
#include "apform.h"
#include "utility.h"
#endif

unsigned int rtk_multicast_get_stb_num(unsigned int *num)
{
	FILE *fp;
	char buf[100];
	unsigned char *saveStr;
	unsigned char *token;
	memset(buf,0,sizeof(buf));
	if ((fp = fopen("/proc/br_igmpDb", "r")) != NULL)
	{
			while(fgets(buf, sizeof(buf),fp)){
				saveStr = buf;
				token = strsep(&saveStr,":");
				if(token != NULL)
				{
					if(strcmp(token, "igmp_client_num")==0)
					{
						token = strsep(&saveStr,":");
						if(token != NULL)
						{
						
							fclose(fp);
							*num=atoi(token);
							return 0;
				
						}
							
					}
				}
				memset(buf,0,sizeof(buf));
			}
			fclose(fp);
			return -1;
	}
	return -1;


}

unsigned int rtk_multicast_set_snoopy_enable(bool enable)
{
	char buf[64];
	memset(buf,0,sizeof(buf));
	enable=!enable;
	apmib_set(MIB_IGMP_SNOOPING_DISABLE, (void *) &enable);
	sprintf(buf,"echo %d > /proc/br_igmpsnoop\n",!enable);
	system(buf);
	return 0;

}


unsigned int rtk_multicast_get_snoopy_enable(bool *enable)
{
	FILE *fp;
	char buf[100];
	unsigned char *saveStr;
	unsigned char *token;
	memset(buf,0,sizeof(buf));
	if ((fp = fopen("/proc/br_igmpsnoop", "r")) != NULL)
	{
			while(fgets(buf, sizeof(buf),fp)){
				saveStr = buf;
				token = strsep(&saveStr,":");
				if(token != NULL)
				{
					if(strcmp(token, "igmpsnoopenabled")==0)
					{
						token = strsep(&saveStr," ");
						if(token != NULL)
						{
							fclose(fp);
							if(atoi(token))
								*enable=1;
							else
								*enable=0;
							return 0;

						}
							
					}
				}
				memset(buf,0,sizeof(buf));
			}
			fclose(fp);
			return -1;
	}
	return -1;

}

unsigned int rtk_multicast_set_mld_snoopy_enable(bool enable)
{
	char buf[64];
	memset(buf,0,sizeof(buf));
	enable=!enable;
	apmib_set(MIB_MLD_SNOOPING_DISABLE, (void *) &enable);
	sprintf(buf,"echo %d > /proc/br_mldsnoop\n",!enable);
	system(buf);
	return 0;

}


unsigned int rtk_multicast_get_mld_snoopy_enable(bool *enable)
{
	FILE *fp;
	char buf[100];
	unsigned char *saveStr;
	unsigned char *token;
	memset(buf,0,sizeof(buf));
	if ((fp = fopen("/proc/br_mldsnoop", "r")) != NULL)
	{
			while(fgets(buf, sizeof(buf),fp)){
				saveStr = buf;
				token = strsep(&saveStr,":");
				if(token != NULL)
				{
					if(strcmp(token, "mldsnoopenabled")==0)
					{
						token = strsep(&saveStr," ");
						if(token != NULL)
						{
							fclose(fp);
							if(atoi(token))
								*enable=1;
							else
								*enable=0;
							return 0;

						}
							
					}
				}
				memset(buf,0,sizeof(buf));
			}
			fclose(fp);
			return -1;
	}
	return -1;

}




unsigned int rtk_multicast_get_igmp_enable(bool *enable)
{
	apmib_get(MIB_IGMP_ENABLE, (void *) enable);
	return 0;
}

unsigned int rtk_multicast_set_igmp_enable(bool enable)
{
	apmib_set(MIB_IGMP_ENABLE, (void *) &enable);
	return 0;
}

unsigned int rtk_multicast_set_proxy_enable(bool enable)
{
	enable=!enable;
	apmib_set(MIB_IGMP_PROXY_DISABLED, (void *) &enable);
	return 0;
}
unsigned int rtk_multicast_get_proxy_enable(bool *enable)
{
	apmib_get(MIB_IGMP_PROXY_DISABLED, (void *) enable);
	*enable=!(*enable);
	return 0;
}
#ifdef CONFIG_IPV6
unsigned int rtk_multicast_set_mld_proxy_enable(bool enable)
{
	enable=!enable;
	apmib_set(MIB_MLD_PROXY_DISABLED, (void *) &enable);
	return 0;
}
unsigned int rtk_multicast_get_mld_proxy_enable(bool *enable)
{
	apmib_get(MIB_MLD_PROXY_DISABLED, (void *) enable);
	*enable=!(*enable);
	return 0;
}
#endif
void formIgmpMldSnoopingProxy(request *wp, char *path, char *query)
{
	char	*str, *submitUrl;

	char tmpBuf[100];
	bool SnoopProxyEnable;
	
	str = req_get_cstream_var(wp, "snp", "");
	if(str[0])
	{
		if (!strcmp(str, "on")||atoi(str)==1)
			SnoopProxyEnable = 1;
		else
			SnoopProxyEnable = 0;
		rtk_multicast_set_snoopy_enable(SnoopProxyEnable);
	}
	str = req_get_cstream_var(wp, "mldsnp", "");
	if(str[0])
	{
		if (!strcmp(str, "on")||atoi(str)==1)
			SnoopProxyEnable = 1;
		else
			SnoopProxyEnable = 0;
		rtk_multicast_set_mld_snoopy_enable(SnoopProxyEnable);
	}
	#if defined (CONFIG_ISP_IGMPPROXY_MULTIWAN)|| defined (CONFIG_ISP_MLDPROXY_MULTIWAN)
	str = req_get_cstream_var(wp, "proxy", "");
	if(str[0])
	{
		if (!strcmp(str, "on")||atoi(str)==1)
			SnoopProxyEnable = 1;
		else
			SnoopProxyEnable = 0;
		rtk_multicast_set_proxy_enable(SnoopProxyEnable);
		#ifdef CONFIG_IPV6
		rtk_multicast_set_mld_proxy_enable(SnoopProxyEnable);
		#endif
			
	}
	str = req_get_cstream_var(wp, "mldproxy", "");
	if(str[0])
	{
		if (!strcmp(str, "on")||atoi(str)==1)
			SnoopProxyEnable = 1;
		else
			SnoopProxyEnable = 0;
		#ifdef CONFIG_IPV6
		rtk_multicast_set_mld_proxy_enable(SnoopProxyEnable);
		#endif
			
	}
	#endif
	apmib_update_web(CURRENT_SETTING);
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	//printf("%s:%d get submit=%s \n",__FUNCTION__,__LINE__,submitUrl);
	fprintf(stderr,"send apply change event!\n");
	snprintf(tmpBuf,sizeof(tmpBuf),"reinitCli -e %d &", REINIT_EVENT_APPLYCAHNGES);
	system(tmpBuf);  
	if (submitUrl[0])
		OK_MSG(submitUrl);

return;

setErr:
	ERR_MSG(tmpBuf);
}





