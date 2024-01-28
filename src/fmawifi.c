/*
 *      Web server handler routines for aWiFi 
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

#if defined(MULTI_WAN_SUPPORT)
extern int getWanIfaceEntry(int index, WANIFACE_T* pEntry);
#endif

#if !defined(MULTI_WAN_SUPPORT)
void wanListForAWiFi(request *wp, int argc, char **argv){
	return;
}

#else
void wanListForAWiFi(request *wp, int argc, char **argv){
	int nBytesSent=0, entryNum, i;
	char tmpBuf[100] = {0};
	WANIFACE_T *pEntry, entry={0};

	pEntry = &entry;
			  	
	for (i = 1; i <= WANIFACE_NUM; i++){
		if (!getWanIfaceEntry(i, pEntry)){
			strcpy(tmpBuf,"getWanIfaceEntry value fail!");
			goto getErr_aWiFi;
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
	
getErr_aWiFi:
	ERR_MSG(tmpBuf);	
}
#endif

int aWiFiTrustedMacList(request *wp, int argc, char **argv){
	int cnt, index, nBytesSent;
	char mac[20], comment[MAX_AWIFI_TRUSTED_MAC_COMMENT];
	AWIFI_TRUSTED_MAC_T macEntry = {0};

	nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
      	"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(aWiFiMac_macaddr)</script></b></font></td>\n"
      	"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(aWiFiWhiteList_comment)</script></b></font></td>\n"
      	"<td align=center width=\"15%%\" ><font size=\"2\"><b><script>dw(aWiFiWhiteList_select)</script></b></font></td></tr>\n"));	
	
	if(!apmib_get(MIB_AWIFI_TRUSTED_MAC_TBL_NUM, (void *)&cnt)){
		fprintf(stderr, "Get MIB_AWIFI_TRUSTED_MAC_TBL_NUM error!\n");
		return -1;
	}
		

	if(cnt > 0){
		for(index = 1; index <= cnt; index++){
			bzero(mac, sizeof(mac));
			bzero(comment, sizeof(comment));
			bzero(&macEntry, sizeof(macEntry));
			
			*((char *)&macEntry) = (char)index;
			if(!apmib_get(MIB_AWIFI_TRUSTED_MAC_TBL, (void *)&macEntry)){
				fprintf(stderr, "Get MIB_AWIFI_TRUSTED_MAC_TBL error!\n");
				return -1;
			}
			
			snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
			macEntry.mac[0], macEntry.mac[1], macEntry.mac[2],
			macEntry.mac[3], macEntry.mac[4], macEntry.mac[5]);
			snprintf(comment, sizeof(comment), "%s", macEntry.comment);

			nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
       			"<td align=center width=\"15%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				mac, comment, index);
		}
	}	

	return nBytesSent;
}

int aWiFiTrustedUrlList(request *wp, int argc, char **argv){
	int cnt, index, nBytesSent;
	char url[MAX_AWIFI_TRUSTED_URL_LEN], comment[MAX_AWIFI_TRUSTED_URL_COMMENT];
	AWIFI_TRUSTED_URL_T urlEntry = {0};

	nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
      	"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(aWiFiUrl_url)</script></b></font></td>\n"
      	"<td align=center width=\"30%%\" ><font size=\"2\"><b><script>dw(aWiFiWhiteList_comment)</script></b></font></td>\n"
      	"<td align=center width=\"15%%\" ><font size=\"2\"><b><script>dw(aWiFiWhiteList_select)</script></b></font></td></tr>\n"));

	if(!apmib_get(MIB_AWIFI_TRUSTED_URL_TBL_NUM, (void *)&cnt)){
		fprintf(stderr, "Get MIB_AWIFI_TRUSTED_URL_TBL_NUM error!\n");
		return -1;
	}


	if(cnt > 0){
		for(index = 1; index <= cnt; index++){
			bzero(url, sizeof(url));
			bzero(comment, sizeof(comment));
			bzero(&urlEntry, sizeof(urlEntry));
			
			*((char *)&urlEntry) = (char)index;
			if(!apmib_get(MIB_AWIFI_TRUSTED_URL_TBL, (void *)&urlEntry)){
				fprintf(stderr, "Get MIB_AWIFI_TRUSTED_URL_TBL error!\n");
				return -1;
			}
			
			snprintf(url, sizeof(url), "%s", urlEntry.url);
			snprintf(comment, sizeof(comment), "%s", urlEntry.comment);

			nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
      			"<td align=center width=\"30%%\" ><font size=\"2\">%s</td>\n"
       			"<td align=center width=\"15%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
				url, comment, index);
		}
	}		
	return nBytesSent;
}

void formAWiFi(request *wp, char *path, char *query)
{
	char *tmpStr, *submitUrl;
	char tmpBuf[100];
#ifdef MULTI_WAN_SUPPORT
	WanIntfacesType wanBindingLanPorts[WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM]={0};
#endif
	int i, enabled = 0, aWiFi_wan_idx = 1, wlanBindStatus = 0;

	tmpStr = req_get_cstream_var(wp, "aWiFiEnabled", "");  	
	if(!strcmp(tmpStr, "ON"))
		enabled = 1 ;
	else 
		enabled = 0 ;

	//printf("[%s:%d] aWiFi_enabled = %d\n",__FUNCTION__,__LINE__, enabled);

	if(!apmib_set(MIB_AWIFI_ENABLED,(void*)&enabled))
	{
		strcpy(tmpBuf,"set MIB_AWIFI_ENABLED value fail!");
		goto setErr_aWiFi;
	}	
	
	if(enabled == 0){	
		goto setOK_aWiFi;		
	}

	tmpStr = req_get_cstream_var(wp, "wanIf", "");	
	//printf("[%s:%d] wanIf = %s\n",__FUNCTION__,__LINE__, tmpStr);
#if defined(MULTI_WAN_SUPPORT)
	if(!strcmp(tmpStr, "")){
		strcpy(tmpBuf,"Please set at least one wan of gateway mode, and then select one for aWiFi!");
		goto setErr_aWiFi;
	}		
	else
		aWiFi_wan_idx = atoi(tmpStr);

	
	if(!apmib_get(MIB_WANIFACE_BINDING_LAN_PORTS, (void *)wanBindingLanPorts))
	{
		strcpy(tmpBuf, "Get MIB_WANIFACE_BINDING_LAN_PORTS fail!\n");
		return -1;
	}
	
	for(i = WAN_INTERFACE_LAN_PORT_NUM; i < WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM; i++){
		if(wanBindingLanPorts[i] == aWiFi_wan_idx){
			wlanBindStatus = 1;
			break;
		}	
	}

	if(!wlanBindStatus){
		WANIFACE_T entry={0};
		if (!getWanIfaceEntry(aWiFi_wan_idx, &entry)){
			strcpy(tmpBuf,"getWanIfaceEntry value fail!");
			goto setErr_aWiFi;
		}
			
		snprintf(tmpBuf, sizeof(tmpBuf), "Please set at least one wlan interface binding to WAN (%s) for running aWiFi!", entry.WanName);
		goto setErr_aWiFi;
	}
#endif

	
	//printf("[%s:%d] aWiFi_wan_idx = %d\n",__FUNCTION__,__LINE__, aWiFi_wan_idx);
	if(!apmib_set(MIB_AWIFI_WAN_IDX,(void*)&aWiFi_wan_idx))
	{
		strcpy(tmpBuf,"set MIB_AWIFI_WAN_IDX value fail!");
		goto setErr_aWiFi;
	}	
	
setOK_aWiFi:
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

setErr_aWiFi:
	ERR_MSG(tmpBuf);
	return;
}


void formAWiFiMacList(request *wp, char *path, char *query){
	char *macAddSave,*macAddApply, *strMac, *strComment;
	char *macDel, *macDelAll;
	char macVal[6]={0},selected_id[20]={0},cmdBuf[64] = {0}, errorBuf[100]={0};
	char *strVal, *submitUrl;
	int cnt, selectId, i;
	AWIFI_TRUSTED_MAC_T *macEntryP, macEntry;

	macEntryP = &macEntry;

	//save_apply
	macAddSave = req_get_cstream_var(wp, ("addAWiFiMac"), "");
	macAddApply = req_get_cstream_var(wp, ("save_apply"), "");

	macDel = req_get_cstream_var(wp, ("deleteSelAWiFiMac"), "");
	macDelAll = req_get_cstream_var(wp, ("deleteAllAWiFiMac"), "");

	if(!apmib_get(MIB_AWIFI_TRUSTED_MAC_TBL_NUM, (void *)&cnt)){
		strcpy(errorBuf, ("Error! Get MIB_AWIFI_TRUSTED_MAC_TBL_NUM error."));
		goto setErr_Mac;	
	}	

	if(macAddSave[0] || macAddApply[0]){
		strMac = req_get_cstream_var(wp, ("mac"), "");
		strComment = req_get_cstream_var(wp, ("comment"), "");
		if(!strMac[0] && !strComment[0])
			goto setMacList_OK;

		if (strlen(strMac) != 12 || !string_to_hex(strMac, macVal, 12)) {
			strcpy(errorBuf, ("Error! Invalid MAC address."));
			goto setErr_Mac;	
		}

		if(cnt == MAX_AWIFI_TRUSTED_MAC_NUM){
			strcpy(errorBuf, ("Error! AWIFI_TRUSTED_MAC_TBL is already full."));
			goto setErr_Mac;	
		}

		for(i = 1; i <= cnt; i++){
			*((char *)macEntryP) = (char)i;
			if(!apmib_get(MIB_AWIFI_TRUSTED_MAC_TBL, (void *)macEntryP)){
				strcpy(errorBuf, ("Error! Get MIB_AWIFI_TRUSTED_MAC_TBL fail."));
				goto setErr_Mac;	
			}
			if(memcmp(macVal, macEntryP->mac, 6) == 0)
			{
				strcpy(errorBuf, ("Error! MAC has already exsit."));
				goto setErr_Mac;	
			}
		}
		
		memcpy(macEntryP->mac, macVal, 6);
		snprintf(macEntryP->comment, MAX_AWIFI_TRUSTED_MAC_COMMENT, "%s", strComment);
		if(!apmib_set(MIB_AWIFI_TRUSTED_MAC_ADD, (void *)macEntryP)){
			strcpy(errorBuf, ("Error! Set AWIFI_TRUSTED_MAC_ADD fail."));
			goto setErr_Mac;
		}

		if(macAddSave[0]) /*no reinit*/
			goto macList_refresh;
		
		goto setMacList_OK;
	}else if(macDel[0]){
		if(cnt > 0){
			for(i = 1; i <= cnt; i++){
				bzero(selected_id, sizeof(selected_id));
				snprintf(selected_id, sizeof(selected_id), "select%d", i);
				strVal = req_get_cstream_var(wp, selected_id, "");
				if(strVal[0]){
					*((char *)macEntryP) = (char)i;
					if(!apmib_get(MIB_AWIFI_TRUSTED_MAC_TBL, (void *)macEntryP)){
						strcpy(errorBuf, ("Error! Get MIB_AWIFI_TRUSTED_MAC_TBL fail."));
						goto setErr_Mac;	
					}
					if(!apmib_set(MIB_AWIFI_TRUSTED_MAC_DEL, (void *)macEntryP)){
						strcpy(errorBuf, ("Error! Set AWIFI_TRUSTED_MAC_DEL fail."));
						goto setErr_Mac;
					}
				}
			}
			goto setMacList_OK;
		}else
			return;
	}else if(macDelAll[0]){
		if(cnt > 0){
			if(!apmib_set(MIB_AWIFI_TRUSTED_MAC_DELALL, (void *)macEntryP)){
				strcpy(errorBuf, ("Error! Set AWIFI_TRUSTED_MAC_DELALL fail."));
				goto setErr_Mac;
			}
			goto setMacList_OK;
		}else
			return;
	}
	
setMacList_OK:
	apmib_update_web(CURRENT_SETTING);
	
#ifdef MODULES_REINIT
	set_reinit_flag(REINIT_FLAG_APP);
#endif

#ifndef NO_ACTION
	run_init_script("all");
#endif

#ifdef RTK_REINIT_SUPPORT
	sprintf(cmdBuf,"reinitCli -e %d",REINIT_EVENT_APPLYCAHNGES);
	system(cmdBuf);
#endif

macList_refresh:
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	 // hidden page
//	printf("%s:%d get submit=%s \n",__FUNCTION__,__LINE__,submitUrl);

	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
	
	return;
setErr_Mac:
	ERR_MSG(errorBuf);
}

void formAWiFiUrlList(request *wp, char *path, char *query){
	char *urlAddSave,*urlAddApply, *strUrl, *strComment;
	char *urlDel, *urlDelAll;
	char selected_id[20]={0}, cmdBuf[64]={0}, errorBuf[100]={0};
	char *strVal, *submitUrl;
	int wan_index, cnt, selectId, i;
	AWIFI_TRUSTED_URL_T *urlEntryP, urlEntry;

	urlEntryP = &urlEntry;

	//save_apply
	urlAddSave = req_get_cstream_var(wp, ("addAWiFiUrl"), "");
	urlAddApply = req_get_cstream_var(wp, ("save_apply"), "");

	urlDel = req_get_cstream_var(wp, ("deleteSelAWiFiUrl"), "");
	urlDelAll = req_get_cstream_var(wp, ("deleteAllAWiFiUrl"), "");

	if(!apmib_get(MIB_AWIFI_TRUSTED_URL_TBL_NUM, (void *)&cnt)){
		strcpy(errorBuf, ("Error! Get MIB_AWIFI_TRUSTED_URL_TBL_NUM error."));
		goto setErr_Url;	
	}
	
	if(urlAddSave[0] || urlAddApply[0]){
		strUrl = req_get_cstream_var(wp, ("url"), "");
		strComment = req_get_cstream_var(wp, ("comment"), "");
		if(!strUrl[0] && !strComment[0])
			goto setUrlList_OK;


		if(cnt == MAX_AWIFI_TRUSTED_URL_NUM){
			strcpy(errorBuf, ("Error! AWIFI_TRUSTED_URL_TBL is already full."));
			goto setErr_Url;	
		}

		for(i = 1; i <= cnt; i++){
			*((char *)urlEntryP) = (char)i;
			if(!apmib_get(MIB_AWIFI_TRUSTED_URL_TBL, (void *)urlEntryP)){
				strcpy(errorBuf, ("Error! Get MIB_AWIFI_TRUSTED_URL_TBL fail."));
				goto setErr_Url;	
			}
			if(strncmp(strUrl, urlEntryP->url, MAX_AWIFI_TRUSTED_URL_LEN) == 0)
			{
				strcpy(errorBuf, ("Error! URL has already exsit."));
				goto setErr_Url;	
			}
		}

		snprintf(urlEntryP->url, MAX_AWIFI_TRUSTED_URL_LEN, "%s", strUrl);
		snprintf(urlEntryP->comment, MAX_AWIFI_TRUSTED_URL_COMMENT, "%s", strComment);
		if(!apmib_set(MIB_AWIFI_TRUSTED_URL_ADD, (void *)urlEntryP)){
			strcpy(errorBuf, ("Error! Set AWIFI_TRUSTED_URL_ADD fail."));
			goto setErr_Url;
		}
		if(urlAddSave[0]) /*no reinit, just save mib*/
			goto urlList_refresh;
		
		goto setUrlList_OK;
	}else if(urlDel[0]){
		if(cnt > 0){
			for(i = 1; i <= cnt; i++){
				bzero(selected_id, sizeof(selected_id));
				snprintf(selected_id, sizeof(selected_id), "select%d", i);
				strVal = req_get_cstream_var(wp, selected_id, "");
				if(strVal[0]){
					*((char *)urlEntryP) = (char)i;
					if(!apmib_get(MIB_AWIFI_TRUSTED_URL_TBL, (void *)urlEntryP)){
						strcpy(errorBuf, ("Error! Get MIB_AWIFI_TRUSTED_URL_TBL fail."));
						goto setErr_Url;	
					}
					if(!apmib_set(MIB_AWIFI_TRUSTED_URL_DEL, (void *)urlEntryP)){
						strcpy(errorBuf, ("Error! Set AWIFI_TRUSTED_URL_DEL fail."));
						goto setErr_Url;
					}
				}
			}
			goto setUrlList_OK;
		}else
			return;
	}else if(urlDelAll[0]){
		if(cnt > 0){
			if(!apmib_set(MIB_AWIFI_TRUSTED_URL_DELALL, (void *)urlEntryP)){
				strcpy(errorBuf, ("Error! Set AWIFI_TRUSTED_URL_DELALL fail."));
				goto setErr_Url;
			}
			goto setUrlList_OK;
		}else
			return;
	}
	
setUrlList_OK:
	apmib_update_web(CURRENT_SETTING);

#ifdef MODULES_REINIT
	set_reinit_flag(REINIT_FLAG_APP);
#endif
	
#ifndef NO_ACTION
	run_init_script("all");
#endif
	
#ifdef RTK_REINIT_SUPPORT
	sprintf(cmdBuf,"reinitCli -e %d",REINIT_EVENT_APPLYCAHNGES);
	system(cmdBuf);
#endif

urlList_refresh:
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	 // hidden page
//	printf("%s:%d get submit=%s \n",__FUNCTION__,__LINE__,submitUrl);

	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
	
	return;
setErr_Url:
	ERR_MSG(errorBuf);
}

#ifdef CONFIG_APP_AWIFI_AUDIT_SUPPORT
void formAWifiAudit(request *wp, char *path, char *query)
{
	char *tmpStr, *submitUrl;
	char tmpBuf[100];
	int i, val, val_new;

	if(!apmib_get(MIB_AWIFI_AUDIT_TYPE, (void *)&val)){
		strcpy(tmpBuf,"Get MIB_AWIFI_AUDIT_TYPE Fail!");
		goto setErr_aWiFi_audit;
	}

	
	tmpStr = req_get_cstream_var(wp, "audit_type", "");	
	if(!strcmp(tmpStr, "1") || !strcmp(tmpStr, "0"))
		val_new = atoi(tmpStr);
	else{
		strcpy(tmpBuf,"Invalid aWiFi audit type!");
		goto setErr_aWiFi_audit;
	}

	if(val == val_new)
		return;

	
	if(!apmib_set(MIB_AWIFI_AUDIT_TYPE,(void*)&val_new))
	{
		strcpy(tmpBuf,"Set MIB_AWIFI_AUDIT_TYPE value fail!");
		goto setErr_aWiFi_audit;
	}	
	

setOK_aWiFi:
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

setErr_aWiFi_audit:
	ERR_MSG(tmpBuf);
	return;
}

#endif
