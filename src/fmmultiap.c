/*This file handles MultiAP webpage form request
 *
 */
#include <arpa/inet.h>
#include <dirent.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*-- Local inlcude files --*/
#include "boa.h"
#include "asp_page.h"
#include "apmib.h"
#include "globals.h"
#include "utility.h"
#include "apform.h"

void _set_up_backhaul_credentials()
{
	unsigned int seed       = 0;
	int          randomData = open("/dev/urandom", O_RDONLY);
	int          mibVal     = 1;
	if (randomData < 0) {
		// something went wrong, use fallback
		seed = time(NULL) + rand();
	} else {
		char    myRandomData[50];
		ssize_t result = read(randomData, myRandomData, sizeof myRandomData);
		if (result < 0) {
			// something went wrong, use fallback
			seed = time(NULL) + rand();
		}
		int i = 0;
		for (i = 0; i < 50; i++) {
			seed += (unsigned char)myRandomData[i];
			if (i % 5 == 0) {
				seed = seed * 10;
			}
		}
	}
	srand(seed);
	char SSIDDic[62]       = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
	char NetworkKeyDic[83] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890~!@#0^&*()_+{}[]:;..?";

	char backhaulSSID[21], backhaulNetworkKey[31];
	strcpy(backhaulSSID, "EasyMeshBH-");
	backhaulSSID[20]       = '\0';
	backhaulNetworkKey[30] = '\0';

	// randomly generate SSID post-fix
	int i;
	for (i = 11; i < 20; i++) {
		backhaulSSID[i] = SSIDDic[rand() % 62];
	}
	// randomly generate network key
	for (i = 0; i < 30; i++) {
		backhaulNetworkKey[i] = NetworkKeyDic[rand() % 83];
	}

	// set into mib
	if (!apmib_set(MIB_WLAN_SSID, (void *)backhaulSSID)) {
		printf("[Error] : Failed to set AP mib MIB_WLAN_SSID\n");
		return 0;
	}

	if (!apmib_set(MIB_WLAN_WPA_PSK, (void *)backhaulNetworkKey)) {
		printf("[Error] : Failed to set AP mib MIB_WLAN_WPA_PSK\n");
		return 0;
	}

	if (!apmib_set(MIB_WLAN_WSC_PSK, (void *)backhaulNetworkKey)) {
		printf("[Error] : Failed to set AP mib MIB_WLAN_WPA_PSK\n");
		return 0;
	}

	mibVal = WSC_AUTH_WPA2PSK;
	apmib_set(MIB_WLAN_WSC_AUTH, (void *)&mibVal);
	mibVal = WSC_ENCRYPT_AES;
	apmib_set(MIB_WLAN_WSC_ENC, (void *)&mibVal);
	mibVal = 1;
	apmib_set(MIB_WLAN_WSC_CONFIGURED, (void *)&mibVal);
	mibVal = WPA_CIPHER_TKIP;
	apmib_set(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&mibVal);
	mibVal = WPA_CIPHER_AES;
	apmib_set(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&mibVal);

	mibVal = 1;
	if (!apmib_set(MIB_WLAN_HIDDEN_SSID, (void *)&mibVal)) {
		printf("[Error] : Failed to set AP mib MIB_WLAN_HIDDEN_SSID\n");
		return 0;
	}
}

void formMultiAP(request *wp, char *path, char *query)
{
	char *submitUrl, *strVal;

	//Check if it is push button press, trigger push button then return.
	strVal    = req_get_cstream_var(wp, ("start_wsc"), "");
	if (strVal[0]) {
		submitUrl = req_get_cstream_var(wp, ("submit-url"), "");
		system("echo 1 > /tmp/virtual_push_button");
		send_redirect_perm(wp, submitUrl);
		return;
	}

	int i, j;
	// Enable dot11kv if not already enabled
	int mibVal = 1;
	strVal     = req_get_cstream_var(wp, ("needEnable11kv"), "");
	if (!strcmp(strVal, "1")) {
		for (i = 0; i < 2; i++) {
			wlan_idx = i;
			for (j = 0; j < 6; j++) {
				vwlan_idx = j;
				apmib_set(MIB_WLAN_DOT11K_ENABLE, (void *)&mibVal);
				apmib_set(MIB_WLAN_DOT11V_ENABLE, (void *)&mibVal);
			}
		}
	}

	char *device_name = req_get_cstream_var(wp, ("device_name_text"), "");
	apmib_set(MIB_MAP_DEVICE_NAME, (void *)device_name);

	// mibVal = 1;
	// apmib_set(MIB_STP_ENABLED, (void *)&mibVal);

	char *role_prev = req_get_cstream_var(wp, ("role_prev"), "");

	char *controller_backhaul_link = req_get_cstream_var(wp, "controller_backhaul_radio", "");
	char *agent_backhaul_link = req_get_cstream_var(wp, "agent_backhaul_radio", "");

	// printf("Controller: %s Agent: %s\n", controller_backhaul_link, agent_backhaul_link);

	// Read role info from form and set to mib accordingly
	strVal = req_get_cstream_var(wp, ("role"), "");
	mibVal = 0;
	if (!strcmp(strVal, "controller")) {
		// Set to controller
		mibVal = 1;
		apmib_set(MIB_MAP_CONTROLLER, (void *)&mibVal);

		apmib_get(MIB_OP_MODE, (void *)&mibVal);

		if(WISP_MODE != mibVal) {
			// Disable repeater
			mibVal = 0;
			apmib_set(MIB_REPEATER_ENABLED1, (void *)&mibVal);
			apmib_set(MIB_REPEATER_ENABLED2, (void *)&mibVal);
			// Disable vxd
			mibVal    = 1;
			wlan_idx  = 0;
			vwlan_idx = 5;
			apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mibVal);
			wlan_idx  = 1;
			vwlan_idx = 5;
			apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mibVal);
		}
		// if different from prev role, reset this mib to 0
		if (strcmp(strVal, role_prev)) {
			mibVal = 0;
			apmib_set(MIB_MAP_CONFIGURED_BAND, (void *)&mibVal);
		}

		// enable va0
#ifdef BACKHAUL_LINK_SELECTION
		if(1 != (controller_backhaul_link[0] - '0'))
#endif
		{
			mibVal    = 0;
			wlan_idx  = 0;
			vwlan_idx = 1;
			apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mibVal);
			mibVal = ENCRYPT_WPA2;
			apmib_set(MIB_WLAN_ENCRYPT, (void *)&mibVal);
			mibVal = WPA_AUTH_PSK;
			apmib_set(MIB_WLAN_WPA_AUTH, (void *)&mibVal);
		}
#ifdef BACKHAUL_LINK_SELECTION
		else
#endif
		{
			mibVal    = 0;
			wlan_idx  = 1;
			vwlan_idx = 1;
			apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mibVal);
			mibVal = ENCRYPT_WPA2;
			apmib_set(MIB_WLAN_ENCRYPT, (void *)&mibVal);
			mibVal = WPA_AUTH_PSK;
			apmib_set(MIB_WLAN_WPA_AUTH, (void *)&mibVal);
		}

		int val;
		for (i = 0; i < 2; i++) {
			wlan_idx  = i;
			for (j = 0; j < 5; j++) {
				vwlan_idx = j;
				if (!apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&val))
					return -1;
				mibVal = 0x20; // fronthaul value
				if (val == 0) // only set to fronthaul if this interface is enabled
					apmib_set(MIB_WLAN_MAP_BSS_TYPE, (void *)&mibVal);
			}

			vwlan_idx = 5;
			mibVal = 0;
			apmib_set(MIB_WLAN_MAP_BSS_TYPE, (void *)&mibVal);
		}

#ifdef BACKHAUL_LINK_SELECTION
		if(1 != (controller_backhaul_link[0] - '0'))
#endif
		{
			wlan_idx  = 0;
			vwlan_idx = 0;
			mibVal = 1;
			apmib_set(MIB_WLAN_STACTRL_ENABLE, (void *)&mibVal);
			apmib_set(MIB_WLAN_STACTRL_PREFER, (void *)&mibVal);
		}
#ifdef BACKHAUL_LINK_SELECTION
		else
#endif
		{
			wlan_idx  = 1;
			vwlan_idx = 0;
			mibVal = 1;
			apmib_set(MIB_WLAN_STACTRL_ENABLE, (void *)&mibVal);
			mibVal = 0;
			apmib_set(MIB_WLAN_STACTRL_PREFER, (void *)&mibVal);
		}

		for (i = 0; i < 2; i++) {
			wlan_idx  = i;
			vwlan_idx = 1;

#ifdef BACKHAUL_LINK_SELECTION
			if(i == (controller_backhaul_link[0] - '0')) {
#endif
				char buffer[64] = {0}, buffer2[64] = {0};
				apmib_get(MIB_WLAN_SSID, (void *)buffer);
				apmib_get(MIB_WLAN_WPA_PSK, (void *)buffer2);
				if (strcmp(strVal, role_prev) || NULL == strstr(buffer, "EasyMeshBH") || 0 == strlen(buffer2)) {
					_set_up_backhaul_credentials();
				}
				mibVal = 0x40;
#ifdef BACKHAUL_LINK_SELECTION
			} else {
				mibVal = 1;
				apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mibVal);
				mibVal = 0;
			}
#endif

			apmib_set(MIB_WLAN_MAP_BSS_TYPE, (void *)&mibVal);
		}
	} else if (!strcmp(strVal, "agent")) {
		mibVal = DHCP_CLIENT;
		apmib_set(MIB_DHCP, (void *)&mibVal);

		mibVal = 480;
		apmib_set(MIB_DHCP_LEASE_TIME, (void *)&mibVal);
		// Set to agent
		mibVal = 2;
		apmib_set(MIB_MAP_CONTROLLER, (void *)&mibVal);

		// wlan_idx  = 0;
		// vwlan_idx = 0;
		// mibVal = 1;
		// apmib_set(MIB_WLAN_STACTRL_ENABLE, (void *)&mibVal);
		// apmib_set(MIB_WLAN_STACTRL_PREFER, (void *)&mibVal);

		// wlan_idx  = 1;
		// vwlan_idx = 0;
		// mibVal = 1;
		// apmib_set(MIB_WLAN_STACTRL_ENABLE, (void *)&mibVal);
		// mibVal = 0;
		// apmib_set(MIB_WLAN_STACTRL_PREFER, (void *)&mibVal);

		// Enable vxd on 5g, set mode and enable wsc on vxd

#ifdef BACKHAUL_LINK_SELECTION
		for (i = 0; i < 2; i++) {
			wlan_idx  = i;
			vwlan_idx = 5;
			int bss_type = 0;
			if (i == (agent_backhaul_link[0] - '0')) {
				mibVal    = 0;
				bss_type  = 0x80;
			} else {
				mibVal    = 1;
				bss_type  = 0;
			}
			apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mibVal);
			apmib_set(MIB_WLAN_WSC_DISABLE, (void *)&mibVal);
			mibVal = 1;
			apmib_set(MIB_WLAN_MODE, (void *)&mibVal);
			apmib_set(MIB_WLAN_MAP_BSS_TYPE, (void *)&bss_type);
		}
#else
#ifdef CONFIG_BAND_2G_ON_WLAN0
		wlan_idx  = 1;
#else
		wlan_idx  = 0;
#endif
		vwlan_idx = 5;
		int bss_type = 0;
		mibVal    = 0;
		bss_type  = 0x80;
		apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mibVal);
		apmib_set(MIB_WLAN_WSC_DISABLE, (void *)&mibVal);
		mibVal = 1;
		apmib_set(MIB_WLAN_MODE, (void *)&mibVal);
		apmib_set(MIB_WLAN_MAP_BSS_TYPE, (void *)&bss_type);
#endif

//////////////////add for multiap////////////////////////
		mibVal = 4;
		apmib_set(MIB_WLAN_ENCRYPT, (void *)&mibVal);
		mibVal = 2;
		apmib_set(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&mibVal);
		mibVal = 32;
		apmib_set(MIB_WLAN_WSC_AUTH, (void *)&mibVal);
		mibVal = 8;
		apmib_set(MIB_WLAN_WSC_ENC, (void *)&mibVal);
//////////////////add for multiap////////////////////////


#ifdef BACKHAUL_LINK_SELECTION
		// Enable repeater
		if (1 == (agent_backhaul_link[0] - '0')) {
			mibVal = 0;
			apmib_set(MIB_REPEATER_ENABLED1, (void *)&mibVal);
			mibVal = 1;
			apmib_set(MIB_REPEATER_ENABLED2, (void *)&mibVal);
		} else {
			mibVal = 1;
			apmib_set(MIB_REPEATER_ENABLED1, (void *)&mibVal);
			mibVal = 0;
			apmib_set(MIB_REPEATER_ENABLED2, (void *)&mibVal);
		}
#else
	#ifdef CONFIG_BAND_2G_ON_WLAN0
		mibVal = 0;
		apmib_set(MIB_REPEATER_ENABLED1, (void *)&mibVal);
		mibVal = 1;
		apmib_set(MIB_REPEATER_ENABLED2, (void *)&mibVal);
	#else
		mibVal = 1;
		apmib_set(MIB_REPEATER_ENABLED1, (void *)&mibVal);
		mibVal = 0;
		apmib_set(MIB_REPEATER_ENABLED2, (void *)&mibVal);
	#endif
#endif

		// if different from prev role, reset this mib to 0
		if (strcmp(strVal, role_prev)) {
			mibVal = 0;
			apmib_set(MIB_MAP_CONFIGURED_BAND, (void *)&mibVal);
		}
	} else if (!strcmp(strVal, "disabled")) {
		mibVal = 0;
		apmib_set(MIB_MAP_CONTROLLER, (void *)&mibVal);

		// Disable repeater
		mibVal = 0;
		apmib_set(MIB_REPEATER_ENABLED1, (void *)&mibVal);
		apmib_set(MIB_REPEATER_ENABLED2, (void *)&mibVal);
		// Disable vxd
		mibVal    = 1;
		wlan_idx  = 0;
		vwlan_idx = 5;
		apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mibVal);
		wlan_idx  = 1;
		vwlan_idx = 5;
		apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mibVal);
		// reset configured band to 0
		mibVal = 0;
		apmib_set(MIB_MAP_CONFIGURED_BAND, (void *)&mibVal);
		// Set all interface bss type to 0 for MAP disabled status
		mibVal = 0x00;
		for (i = 0; i < 2; i++) {
			for (j = 0; j < 6; j++) {
				wlan_idx  = i;
				vwlan_idx = j;
				apmib_set(MIB_WLAN_MAP_BSS_TYPE, (void *)&mibVal);
			}
		}
	}

	// update flash
	apmib_update_web(CURRENT_SETTING);

//////////////////add for multiap////////////////////////
#ifdef MODULES_REINIT
	set_reinit_flag(REINIT_FLAG_ALL);
#endif

#ifndef NO_ACTION
	run_init_script("all");
#endif
/////////////////add for multiap/////////////////////////

	submitUrl = req_get_cstream_var(wp, ("submit-url"), "");
	strVal    = req_get_cstream_var(wp, ("save_apply"), "");
	// sysconf init   if save_apply
	if (strVal[0]) {
		OK_MSG(submitUrl);
	} else {
		send_redirect_perm(wp, submitUrl);
	}

	return;
}

int showBackhaulSelection(request * wp, int argc, char **argv)
{
	req_format_write(wp, "<tr id=\"controller_backhaul\"> <td width=\"30%%\"><b>Backhaul BSS:</b></td> <td width=\"70%%\"> \
	<input type=\"radio\" id=\"controller_backhaul_wlan0\" name=\"controller_backhaul_radio\" value=\"0\" onclick=\"isBackhaulOnChange()\">wlan0&nbsp;&nbsp; \
	<input type=\"radio\" id=\"controller_backhaul_wlan1\" name=\"controller_backhaul_radio\" value=\"1\" onclick=\"isBackhaulOnChange()\">wlan1&nbsp;&nbsp;</tr>");
	req_format_write(wp, "<tr id=\"agent_backhaul\"> <td width=\"30%%\"><b>Backhaul STA:</b></td> <td width=\"70%%\"> \
	<input type=\"radio\" id=\"agent_backhaul_wlan0\" name=\"agent_backhaul_radio\" value=\"0\" onclick=\"isBackhaulOnChange()\">wlan0&nbsp;&nbsp; \
	<input type=\"radio\" id=\"agent_backhaul_wlan1\" name=\"agent_backhaul_radio\" value=\"1\" onclick=\"isBackhaulOnChange()\">wlan1&nbsp;&nbsp;</tr>");
}
