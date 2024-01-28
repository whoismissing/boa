/*
 *      Web server handler routines for UPnP 
 *
 *
 *
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "boa.h"
#include "asp_page.h"
#include "apmib.h"
#include "apform.h"
#include "utility.h"

extern int getWanIfaceEntry(int index, WANIFACE_T* pEntry);

void formUpnp(request *wp, char *path, char *query)
{
	char *submitUrl;
	char tmpBuf[100];

	int enabled=0;
	char *tmpStr ;
	int i, wan_idx_bak, gw_wan_idx = 0, upnp_wan_idx;
	
	WANIFACE_Tp pEntry;
	WANIFACE_T entry;
	
	memset(&entry, '\0', sizeof(entry));
	pEntry = &entry;

	tmpStr = req_get_cstream_var(wp, "upnpEnabled", "");  
	//printf("[%s:%d] upnp_enabled = %s\n",__FUNCTION__,__LINE__, tmpStr);
	
	if(!strcmp(tmpStr, "ON"))
		enabled = 1 ;
	else 
		enabled = 0 ;

	//printf("[%s:%d] upnp_enabled = %d\n",__FUNCTION__,__LINE__, enabled);

	//tmpStr = req_get_cstream_var(wp, "wanIf", "");	
	//printf("[%s:%d] wanIf = %s\n",__FUNCTION__,__LINE__, tmpStr);
	
	if(enabled == 0){
		for(i = 1; i <= WANIFACE_NUM; i++){
			if(!apmib_set(MIB_WANIFACE_CURRENT_IDX,(void*)&i))
			{
				strcpy(tmpBuf,"set MIB_WANIFACE_CURRENT_IDX value fail!");
				goto setErr_upnp;
			}

			if(!apmib_set(MIB_WANIFACE_ENABLEUPNP,(void*)&enabled))
			{
				strcpy(tmpBuf,"set MIB_WANIFACE_ENABLEUPNP value fail!");
				goto setErr_upnp;
			}		

		}
		goto setOK_upnp;
		
	}

	tmpStr = req_get_cstream_var(wp, "wanIf", "");	
	if(!strcmp(tmpStr, "")){
		strcpy(tmpBuf,"Please set at least one wan of gateway mode, and then select one for UPnP Port-Mapping!");
		goto setErr_upnp;
	}		
	else
		upnp_wan_idx = atoi(tmpStr);
	
	//printf("[%s:%d] upnp_wan_idx = %d\n",__FUNCTION__,__LINE__, upnp_wan_idx);
	
	for(i = 1; i <= WANIFACE_NUM; i++){
		if (!apmib_set(MIB_WANIFACE_CURRENT_IDX, (void *)&i))
		{
			strcpy(tmpBuf,"set MIB_WANIFACE_CURRENT_IDX value fail!");
			goto setErr_upnp;
		}

		if (i == upnp_wan_idx)
		{
			enabled = 1;
			if(!apmib_set(MIB_WANIFACE_ENABLEUPNP,(void*)&enabled))
			{
				strcpy(tmpBuf,"set MIB_WANIFACE_ENABLEUPNP value fail!");
				goto setErr_upnp;
			}	
		}else{
			enabled = 0;
			if(!apmib_set(MIB_WANIFACE_ENABLEUPNP,(void*)&enabled))
			{
				strcpy(tmpBuf,"set MIB_WANIFACE_ENABLEUPNP value fail!");
				goto setErr_upnp;
			}	
		}

	}
	
setOK_upnp:
	apmib_update_web(CURRENT_SETTING);

#ifdef MODULES_REINIT
	set_reinit_flag(REINIT_FLAG_APP);
#endif

#ifndef NO_ACTION
	run_init_script("all");
#endif
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	 // hidden page
	OK_MSG(submitUrl);
	return;

setErr_upnp:
	ERR_MSG(tmpBuf);
	return;
}



void wanListForPortMapping(request *wp, int argc, char **argv){
	int nBytesSent=0, entryNum, i;
	char tmpBuf[100];
	WANIFACE_Tp pEntry;
	WANIFACE_T entry;

	memset(&entry, '\0', sizeof(entry));
	pEntry = &entry;

			  	
	for (i = 1; i <= WANIFACE_NUM; i++){
		if (!getWanIfaceEntry(i, pEntry)){
			strcpy(tmpBuf,"getWanIfaceEntry value fail!");
			goto getErr_upnp;
		}

		if(pEntry->enable && (pEntry->cmode != IP_BRIDGE) && (pEntry->applicationtype == APPTYPE_INTERNET 
			|| pEntry->applicationtype == APPTYPE_TR069_INTERNET)){
			nBytesSent += req_format_write(wp, ("<tr>"
						"<td width=\"30%%\">"
						"<input type=\"radio\" value=\"%d\" id=\"wanIf%d\"name=\"wanIf\" > "
						"<font size=2> %s </font>"
						"</td></tr>"),
						i, i, pEntry->WanName);

		}
	}
	return;
	
getErr_upnp:
	ERR_MSG(tmpBuf);
	
}
