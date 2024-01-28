/*
 *      Web server handler routines for vlan binding
 * 
 *      $Id: fmvlanbinding.c,v 1.20 2017/10/18 lynn $
 *
 */

/*-- System inlcude files --*/
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>

#include "boa.h"
#include "globals.h"
#include "apform.h"
#include "apmib.h"
#include "utility.h"
#include "asp_page.h"
#include "common_utility.h"


#define APMIB_GET(A, B, C)	 \
	{if(!apmib_get(A, B)) { strcpy(errBuf, (C)); goto setErr; }}
#define APMIB_SET(A, B, C)	  \
	{if(!apmib_set(A, B)) { strcpy(errBuf, (C)); goto setErr; }}

int showLanDeviceList(request *wp, int argc, char **argv)
{
	int i = 0, j = 0, nBytesSent = 0;
	int wlan_disable = 1;
	int ret = 0;
	unsigned char buff1[128] = {'\0'}, buff2[16] = {'\0'};	
	unsigned char wlan_name[32];
	
    								   
    nBytesSent += req_format_write(wp,"<font size=2><b>Lan Device List:&nbsp;</b> <select name=\"landevice\">");
	//entry.intfIdx same with array lanPortBindIfnameTbl's index 
	for(i = 0; i < (WAN_INTERFACE_LAN_PORT_NUM+WAN_INTERFACE_WLAN_PORT_NUM); i++){
		if (i < WAN_INTERFACE_LAN_PORT_NUM){
			snprintf(buff1, sizeof(buff1), "LAN%d", i+1);
			snprintf(buff2, sizeof(buff2), "%d", i);
			if (i == 0)
				nBytesSent += req_format_write(wp,"<option selected value=\"%s\">%s</option>", buff2, buff1);
			else
				nBytesSent += req_format_write(wp,"<option value=\"%s\">%s</option>", buff2, buff1);
		}
		else{			
			memset(wlan_name,0x00,sizeof(wlan_name));
			getLanPortIfName(wlan_name, i);	
			ret = SetWlan_idx(wlan_name);
			if (ret){
				apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disable);
				if (!wlan_disable){
					snprintf(buff1, sizeof(buff1), "%d", i);
					nBytesSent += req_format_write(wp,"<option value=\"%s\">%s</option>", buff1, wlan_name);
				}
			}
		}
		
	}

	#if 0
	//ethernet lan devices
	for(i = 0; i < WAN_INTERFACE_LAN_PORT_NUM; i++){
		snprintf(buff1, sizeof(buff1), "LAN%d", i+1);
		snprintf(buff2, sizeof(buff2), "%d", i);
		if (i == 0)
			nBytesSent += req_format_write(wp,"<option selected value=\"%s\">%s</option>", buff2, buff1);
		else
			nBytesSent += req_format_write(wp,"<option value=\"%s\">%s</option>", buff2, buff1);
	}

	//wlan devices
	for(i = 0;i < NUM_WLAN_INTERFACE; i++)
	{
		memset(wlan_name,0x00,sizeof(wlan_name));
		snprintf(wlan_name,sizeof(wlan_name), "wlan%d",i);

		//root interface
		ret = SetWlan_idx(wlan_name);
		if (ret){
			apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disable);
			if (!wlan_disable){
				//vap + vxd
				snprintf(buff1, sizeof(buff1), "%d", WAN_INTERFACE_LAN_PORT_NUM + (i*WAN_INTERFACE_EACH_WLAN_PORT_NUM));
				nBytesSent += req_format_write(wp,"<option value=\"%s\">%s</option>", buff1, wlan_name);
			}
		}

		//vaps
		for (j = 0; j < NUM_VWLAN; j++){
			memset(wlan_name,0x00,sizeof(wlan_name));
			snprintf(wlan_name,sizeof(wlan_name), "wlan%d-va%d",i, j);
			
			ret = SetWlan_idx(wlan_name);
			if (ret){
				apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disable);
				if (!wlan_disable){
					snprintf(buff1, sizeof(buff1), "%d", WAN_INTERFACE_LAN_PORT_NUM + (i*WAN_INTERFACE_EACH_WLAN_PORT_NUM) + j +1);
					nBytesSent += req_format_write(wp,"<option value=\"%s\">%s</option>", buff1, wlan_name);
				}
			}
			
		}

		//vxd
		memset(wlan_name,0x00,sizeof(wlan_name));
		snprintf(wlan_name,sizeof(wlan_name), "wlan%d-vxd",i);
		ret = SetWlan_idx(wlan_name);
		if (ret){
			apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&wlan_disable);
			if (!wlan_disable){
				snprintf(buff1, sizeof(buff1), "%d", WAN_INTERFACE_LAN_PORT_NUM + (i*WAN_INTERFACE_EACH_WLAN_PORT_NUM) + NUM_VWLAN +1);
				nBytesSent += req_format_write(wp,"<option value=\"%s\">%s</option>", buff1, wlan_name);
			}
		}
		
	}
	#endif
	
    nBytesSent += req_format_write(wp,"</select></font>&nbsp;&nbsp");								   

	return nBytesSent;
}


int vlanBindingList(request *wp, int argc, char **argv)
{
	int nBytesSent = 0, entryNum = 0, i = 0;
	VLANBINDING_T entry;
	char ifName[IFACE_NAME_MAX]={0};
	char bindtype[IFACE_NAME_MAX] = {0};

	if ( !apmib_get(MIB_VLANBINDING_TBL_NUM, (void *)&entryNum)) {
  		fprintf(stderr, "Get table num entry error!\n");
		return -1;
	}

	nBytesSent += req_format_write(wp, ("<tr class=\"tbl_head\">"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b>LAN Device</b></font></td>\n"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b>Binding Type</b></font></td>\n"
      	"<td align=center width=\"40%%\" ><font size=\"2\"><b>Binding Vlan</b></font></td>\n"
      	"<td align=center width=\"20%%\" ><font size=\"2\"><b>Select</b></font></td></tr>\n"));

	for (i = 1; i <= entryNum; i++) {
		memset((void *)&entry, 0x00, sizeof(entry));
		*((char *)&entry) = (char)i;
		if ( !apmib_get(MIB_VLANBINDING_TBL, (void *)&entry)){
			fprintf(stderr, "Get table entry error!\n");
			return -1;
		}
		
		bzero(ifName,sizeof(ifName));
		if (entry.intfIdx < WAN_INTERFACE_LAN_PORT_NUM){
			snprintf(ifName, sizeof(ifName), "LAN%d", entry.intfIdx+1);
		}
		else{
			getLanPortIfName(ifName, entry.intfIdx);	
		}

		if (entry.bindingType == LANWAN_BINDING_VLANBASE)
			snprintf(bindtype, sizeof(bindtype), "VLAN");
		else
			snprintf(bindtype, sizeof(bindtype), "PORT");
		
		//printf("%s %d i=%d, ifName=%s, entry.bindingType=%u, entry.bindingVlan=%s entry.intfIdx=0x%x\n",__func__, __LINE__, i, ifName, entry.bindingType, entry.bindingVlan, entry.intfIdx);
		
		nBytesSent += req_format_write(wp, ("<tr class=\"tbl_body\">"
			"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"20%%\" ><font size=\"2\">%s</td>\n"
      		"<td align=center width=\"40%%\" ><font size=\"2\">%s</td>\n"
       		"<td align=center width=\"20%%\" ><input type=\"checkbox\" name=\"select%d\" value=\"ON\"></td></tr>\n"),
			ifName, bindtype, entry.bindingVlan,i);
	}
	
	return nBytesSent;
}

/* return value ==0, check_vid exist
 *  return value == -1, check_vid not exist
*/
int checkVlanBindingStr(unsigned char *vlan_str, int check_vid)
{
	unsigned char tmp_buff[256];
	unsigned char *tokptr1=NULL, *strptr1 = NULL, *tokptr2=NULL, *strptr2 = NULL;
	int lan_vlan = -1, wan_vlan = -1;
	
	if (!vlan_str)
		return 
		
	memset(tmp_buff, '\0', sizeof(tmp_buff));
	strcpy(tmp_buff, vlan_str);
	strptr1 = tmp_buff;
	//printf("%s %d strptr1=%s \n", __func__, __LINE__, strptr1);
	for (tokptr1 = strsep(&strptr1,VLANBINDING_PAIR_DELIMITER); tokptr1 != NULL; 
		  tokptr1 = strsep(&strptr1,VLANBINDING_PAIR_DELIMITER)){

		if (!strstr(tokptr1, VLANBINDING_PAIR_MEMBER_DELIMITER))
		 continue;
		
		lan_vlan = wan_vlan = -1;
		sscanf(tokptr1, "%d/%d", &lan_vlan, &wan_vlan);
		//printf("%s %d lan_vlan=%d, wan_vlan=%d\n", __func__, __LINE__, lan_vlan, wan_vlan);
		if (lan_vlan == check_vid){
			return 0;
		}
	}

	return -1;	
}

void formVlanBinding(request *wp, char *path, char *query)
{
	char *strTemp, *strVal, *strBindingVlan, *strLanif, *strBingType, *strHwVid;	
	unsigned char *tokptr1=NULL, *strptr1 = NULL, *tokptr2=NULL, *strptr2 = NULL;
	char *submitUrl;
	char errBuf[128], tmpBuf[128];
	int entryNum, enabled, lanindex, bindingtype, i = 0, max_len, hwvid = 0, ret = 0;
	int lan_vlan = -1, wan_vlan = -1, flag = 0;
	VLANBINDING_T entry,  tmp_entry;

	strTemp = req_get_cstream_var(wp, ("addVlanBinding"), "");
	#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strTemp[0]==0){
		strTemp = req_get_cstream_var(wp, ("addVlanBindingFlag"), "");
	}
	#endif	

	// add
	if ( strTemp[0]) {
		strVal = req_get_cstream_var(wp, ("enabled"), "");
		if ( !strcmp(strVal, "ON"))
			enabled = 1;
		else
			enabled = 0;
		APMIB_SET(MIB_VLANBINDING_ENABLED, (void *)&enabled, "Set enable error!");
		
		if (enabled){

			//lan device -->store position
			strLanif = req_get_cstream_var(wp, ("landevice"), "");
			lanindex = strtol(strLanif, NULL, 10);

			//binding type
			strBingType = req_get_cstream_var(wp, ("bindingtype"), "");
			bindingtype = strtol(strBingType, NULL, 10);
			//check binding type
			if (LANWAN_BINDING_PORTBASE == bindingtype){
				snprintf(errBuf, sizeof(errBuf), "please set port binding in WAN page!");
				goto setErr;
			}
			
			//binding vlan
			strBindingVlan = req_get_cstream_var(wp, ("bindingvlan"), "");

			//hw nat lan vid
			strHwVid = req_get_cstream_var(wp, ("hwnatlanvid"), "");
			if (strHwVid[0]){
				hwvid = strtol(strHwVid, NULL, 0);
				if (hwvid< 0 || hwvid > 4095){
					snprintf(errBuf, sizeof(errBuf), "VID invalid!, should between 0-4095!");
					goto setErr;
				}
			}
			
			if (!strBindingVlan[0] && !strHwVid[0]){
				//just enabled
				goto setOk;
			}
			
			apmib_get(MIB_VLANBINDING_TBL_NUM, (void *)&entryNum);
			for (i = 1; i <= entryNum; i++) {
				*((char *)&tmp_entry) = (char)i;
				APMIB_GET(MIB_VLANBINDING_TBL, (void *)&tmp_entry, "get tbl error!");

				//check lan interface occupied entry?
				if (strBindingVlan[0] && (lanindex == tmp_entry.intfIdx)){
					snprintf(errBuf, sizeof(errBuf), "the same lan device cannot use two entry, if you want to update the settings, please delete this enry and add it again!");
					goto setErr;
				}
				
				//printf("%s %d \n", __func__, __LINE__);
				//hwnat lan vid need exist, check in existed vlan binding entry?
				if (strHwVid[0]){
					ret = checkVlanBindingStr(tmp_entry.bindingVlan, hwvid);
					if (!ret){
						flag = 1;
						break;
					}
				}
			}

			//check strBindingVlan....
			if (strBindingVlan[0]){
				memset(tmpBuf, '\0', sizeof(tmpBuf));
				strcpy(tmpBuf, strBindingVlan);
				ret = check_vlan_pair(tmpBuf, errBuf, sizeof(errBuf));
				if (ret){
					goto setErr;
				}
				//check binding vlan length
				max_len = (WANIFACE_NUM*VLANBINDING_PAIR_LENGTH);
				if (strlen(strBindingVlan) > (max_len - 1)){
					snprintf(errBuf, sizeof(errBuf), "the length of Binding Vlan is too long! max length:%d, current length:%d", max_len, strlen(strBindingVlan));
					goto setErr;
				}
			}

			//hwnat lan vid, existed in current entry?
			if (strHwVid[0]&& hwvid>0){
				
				if (!flag && strBindingVlan[0]){
					ret = checkVlanBindingStr(strBindingVlan, hwvid);
					if (!ret){
						flag = 1;
					}
				}
				
				if (!flag){
					snprintf(errBuf, sizeof(errBuf), "Invalid hw nat lan vid, non-existence!");
					goto setErr;
				}
			}
			
			if (strHwVid[0])
				APMIB_SET(MIB_VLANBINDING_HWNAT_LANVID, (void *)&hwvid, "set hw lan vid error!");

			if (strBindingVlan[0]){
				memset((void *)&entry, 0x00, sizeof(entry));
				entry.intfIdx = lanindex;
				entry.bindingType = (unsigned char)bindingtype;
				strcpy((char *)&entry.bindingVlan, strBindingVlan);

				apmib_set(MIB_VLANBINDING_DEL, (void *)&entry);
				APMIB_SET(MIB_VLANBINDING_ADD, (void *)&entry, "set tbl error!");
			}
			
			//printf("%s %d lanindex=%d, bindingtype=%d, strBindingVlan=%s \n", __func__, __LINE__, lanindex, bindingtype, strBindingVlan);
		}
		else{
			
		}
		

	}

	//delete selected
	strTemp = req_get_cstream_var(wp, ("deleteSelVlanBinding"), "");
	if (strTemp[0]){
		APMIB_GET(MIB_VLANBINDING_TBL_NUM, (void *)&entryNum, "get tbl num error!");
		for (i = entryNum; i > 0; i--) {
			snprintf(tmpBuf, sizeof(tmpBuf), "select%d", i);
			strVal = req_get_cstream_var(wp, tmpBuf, "");
			if ( !strcmp(strVal, "ON") ) {
				*((char *)&tmp_entry) = (char)i;
				APMIB_GET(MIB_VLANBINDING_TBL, (void *)&tmp_entry, "get tbl error!");
				APMIB_SET(MIB_VLANBINDING_DEL, (void *)&tmp_entry, "delete selected tbl error!");
			}
		}		
	}

	//delete all
	strTemp = req_get_cstream_var(wp, ("deleteAllVlanBinding"), "");
	if (strTemp[0]){
		APMIB_SET(MIB_VLANBINDING_DELALL, (void *)&tmp_entry, "delete all tbl error!");
	}


setOk:
	apmib_update_web(CURRENT_SETTING);
	
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	 // hidden page
	//printf("%s:%d get submit=%s \n",__FUNCTION__,__LINE__,submitUrl);
	if (submitUrl[0])
		OK_MSG(submitUrl);

	return;

setErr:
	ERR_MSG(errBuf);
	return;
	
}


