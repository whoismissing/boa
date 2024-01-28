
var language_sel='language:';
var language_sc='简体中文';
var language_en='English';
/***********	menu.htm	************/
var menu_site ='Site contents';
var menu_wizard ='Setup Wizard';
var menu_opmode ='Operating Mode';
var menu_wireless='Wireless';
var menu_basic='Basic Settings';
var menu_advance='Advanced Settings';
var menu_security='Security';
var menu_8021xCert='802.1x Cert Install';
var menu_accessControl='Access Control';
var menu_wds='WDS Settings';
var menu_mesh='Mesh Settings';
var menu_siteSurvey='Site Survey';
var menu_wps='WPS';
var menu_schedule='Schedule';
var menu_tcpip='TCP/IP Settings';
var menu_lan='LAN Interface';
var menu_wan='WAN Interface';
var menu_fireWall='Firewall';
var menu_portFilter='Port Filtering';
var menu_ipFilter='IP Filtering';
var menu_macFilter='MAC Filtering';
var menu_portFw='Port Forwarding';
var menu_urlFilter='URL Filtering';
var menu_dmz='DMZ';
var menu_vlan='VLAN';
var menu_qos='QOS';
var menu_routeSetup='Routing Setup';
var menu_management='Management';
var menu_status='Status';
var menu_statistics='Statistics';
var menu_ddns='DDNS';
var menu_timeZone='Time Zone Setting';
var menu_dos='Denial-of-Service';
var menu_log='Log';
var menu_updateFm='Upgrade Firmware';
var menu_setting='Save/Reload Settings';
var menu_psw='Password';
var menu_logout='Logout';
/***********	opmode.htm	************/
var opmode_header='Operating Mode';
var opmode_explain='You can setup different modes for the LAN and WLAN interfaces for NAT and bridging functions.';
var opmode_radio_gw=' Gateway: ';
var opmode_radio_gw_explain='In this mode, the device connects to the internet via an ADSL/Cable Modem. NAT is enabled and PCs on  LAN ports share the same IP Address to the ISP via the WAN port. The connection type can be setup on the WAN page using PPPOE, DHCP client, PPTP client, L2TP client, or static IP.';
var opmode_radio_br=' Bridge: ';
var opmode_radio_br_explain='In this mode, all ethernet ports and wireless interfaces are bridged together and the NAT function is disabled. All WAN related functions, including the firewall, are not supported.';
var opmode_radio_repeater=' Repeater: ';
var opmode_radio_repeater_explain='In this mode, all ethernet ports and wireless interface are bridged together and the wireless client will connect to remote access point.You can connect to the remote AP in Site-Survey page.';
var opmode_radio_wisp='Wireless ISP:';
var opmode_radio_wisp_explain='In this mode, all ethernet ports are bridged together and the wireless client will connect to the ISP access point. NAT is enabled and PCs on Ethernet ports share the same IP to the ISP via the wireless LAN. You can connect to the ISP’s AP on the Site-Survey page.  The connection type can be setup on the WAN page using PPPOE, DHCP client, PPTP client, L2TP client, or static IP.';
var opmode_radio_wisp_wanif='WAN Interface : ';
var opmode_apply='Apply Change';
var opmode_reset='Reset';
var opmode_save='Save';
var opmode_save_apply='Save & Apply';



/***********	wlan_schedule.htm	************/
var wlan_schedule_header = 'Wireless Schedule';
var wlan_schedule_explain = 'This page allows you setup the wireless schedule rule. Do not forget to configure the system time before enabling this feature.Supports two timing modes, default mode 0, when you enable the "Timer Mode Switch" switch, switch to Mode 1.';
var wlan_schedule_explain_mode0 = 'Mode 0: Set the time period from "From" to "To", wireless will be turned on during this time period.';
var wlan_schedule_explain_mode1 = 'Mode 1: Set the start time, select the "on" or "off" action you want to perform, and the wireless will be turned on or off from the start time.';
var wlan_schedule_enable = 'Enable Wireless Schedule';
var wlan_schedule_mode_enable = 'Timer Mode Switch';
var wlan_schedule_action_enable = 'Enable';
var wlan_schedule_action_disable = 'Disable';
var wlan_schedule_everyday = 'Everday';
var wlan_schedule_sun = 'Sun';
var wlan_schedule_mon = 'Mon';
var wlan_schedule_tue= 'Tue';
var wlan_schedule_wed = 'Wed';
var wlan_schedule_thu = 'Thu';
var wlan_schedule_fri = 'Fri';
var wlan_schedule_sat = 'Sat';
var wlan_schedule_24Hours = '24 Hours';
var wlan_schedule_from_action = 'Action';
var wlan_schedule_from = 'From';
var wlan_schedule_to = 'To';
var wlan_schedule_save = 'Apply Changes';
var wlan_schedule_reset = 'Reset';
var wlan_schedule_days = 'Days';
var wlan_schedule_time = 'Time';
var wlan_schedule_time_check = 'Please set the Days!';
var wlan_schedule_ena='Enable';
var wlan_schedule_day='Day';
var wlan_schedule_for_save='Save';
var wlan_schedule_save_apply = 'Save & Apply';
var wlan_schedule_for_hour='(hour)';
var wlan_schedule_for_min='(min)';
var wlan_schedule_for_action='(action)';
var wlan_schedule_rule='Schedule Rule';
var wlan_schedule_sbte='Error. The start time bigger than end time';
var wlan_schedult_sequale="Error. The start time can't be equal to end time"
/***********	wlwps.htm	************/
var wlwps_header = 'Wi-Fi Protected Setup';
var wlwps_header_explain = 'This page allows you to change the settings for WPS (Wi-Fi Protected Setup). Using this feature allows a wireless client to automically syncronize its settings and easily and securely connect to the Access Point.';
var wlwps_wps_disable = 'Disable WPS';
var wlwps_wps_save = 'Apply Changes';
var wlwps_wps_reset = 'Reset';
var wlwps_save = 'Save';
var wlwps_save_apply = 'Save & Apply';

var wlwps_status = 'WPS Status:';
var wlwps_status_conn = 'Configured';
var wlwps_status_unconn = 'UnConfigured';
var wlwps_status_reset = 'Reset to UnConfigured';
var wlwps_runon = 'WPS Run on:';
var wlwps_runon_root = 'Root AP';
var wlwps_runon_rpt = 'Repeater Client';
var wlwps_lockdown_state = 'Auto-lock-down state';
var wlwps_self_pinnum = 'Self-PIN Number:';
var wlwps_unlockautolockdown = 'Unlock';
var wlwps_lockdown_state_locked = 'Locked';
var wlwps_lockdown_state_unlocked = 'Unlocked';
var wlwps_pbc_title = 'Push Button Configuration:';
var wlwps_pbc_start_button = 'Start PBC';
var wlwps_stopwsc_title = 'STOP WSC';
var wlwps_stopwsc_button = 'Stop WSC';
var wlwps_pin_number_title = 'Client PIN Number:';
var wlwps_pin_start_button = 'Start PIN';
var wlwps_keyinfo = 'Current Key Info:';
var wlwps_authentication = 'Authentication';
var wlwps_authentication_open = 'Open';
var wlwps_authentication_wpa_psk = 'WPA PSK';
var wlwps_authentication_wep_share = 'WEP Shared';
var wlwps_authentication_wpa_enterprise = 'WPA Enterprise';
var wlwps_authentication_wpa2_enterprise = 'WPA2 Enterprise';
var wlwps_authentication_wpa2_psk = 'WPA2 PSK';
var wlwps_authentication_wpa2mixed_psk = 'WPA2-Mixed PSK';
var wlwps_encryption = 'Encryption';
var wlwps_encryption_none= 'None';
var wlwps_encryption_wep = 'WEP';
var wlwps_encryption_tkip = 'TKIP';
var wlwps_encryption_aes = 'AES';
var wlwps_encryption_tkip_aes = 'TKIP+AES';
var wlwps_key = 'Key';
var wlwps_pin_conn = 'PIN Configuration:';
var wlwps_assign_ssid = 'Assign SSID of Registrar:';
var wlsps_assign_mac = 'Assign Mac of Registrar:';
var wlwps_wpa_save_pininvalid = 'Invalid PIN length! The device PIN is usually four or eight digits long.';
var wlwps_wpa_save_pinnum = 'Invalid PIN! The device PIN must be numeric digits.';
var wlwps_wpa_save_pinchecksum = 'Invalid PIN! Checksum error.';
var wlwps_pinstart_pinlen = 'Invalid Enrollee PIN length! The device PIN is usually four or eight digits long.';
var wlwps_pinstart_pinnum = 'Invalid Enrollee PIN! Enrollee PIN must be numeric digits.';
var wlwps_pinstart_pinchecksum = 'Checksum failed!';
var wlwps_macaddr_nocomplete = 'Input MAC address is not complete. It should be 12 digits in hex.';

var warn_msg1='WPS was disabled automatically because wireless mode setting could not be supported. ' + 'You need go to the Wireless/Basic page to modify settings to enable WPS.';
var warn_msg2='WPS was disabled automatically because Radius Authentication could not be supported. ' + 'You need go to the Wireless/Security page to modify settings to enable WPS.';
var warn_msg3="PIN number was generated. Click the \'Apply Changes\' button to make the change effective.";
var wlwps_start_pbc_successfully='Start PBC successfully!<br><br>';
var wlwps_in='in';
var wlwps_client="client"
var wlwps_run_wps='You have to run Wi-Fi Protected Setup within 2 minutes.';
var wlwps_start_pin='Start PIN successfully!<br><br>';
var wlwps_apply_wps_pin_successfully='Applied WPS PIN successfully!<br><br>';
var wlwps_runwps_within_2minutes='You have to run Wi-Fi Protected Setup within 2 minutes.';
var wlwps_apply_wps_stop='Applied WPS STOP successfully!<br>';
var wlwps_apply_wps_unlock='Applied WPS unlock successfully!<br>';

/***********    wlmesh.htm      ************/
var wlmesh_onewlan_header = 'Wireless Mesh Network Setting</p>';
var wlmesh_morewlan_header = 'Wireless Mesh Network Setting -wlan';
var wlmesh_header_explain = 'Mesh network uses wireless media to communicate with other APs, like the Ethernet does. To do this, you must set these APs in the same channel with the same Mesh ID. The APs should be under AP+MESH/MESH mode.';
var wlmesh_enable = 'Enable Mesh';
var wlmesh_mesh_ID = 'Mesh ID';
var wlmesh_encryption = 'Encryption';
var wlmesh_pre_shared_key_format = 'Pre-Shared Key Format';
var wlmesh_passphrase = 'Passphrase';
var wlmesh_hex_64_characters = 'Hex (64 characters)';
var wlmesh_pre_shared_key = 'Pre Shared Key';
var wlmesh_save = 'Save';
var wlmesh_reset = 'Reset';
var wlmesh_save_apply = 'Save & Apply';
var wlmesh_showACL = 'Set Access Control';
var wlmesh_showInfo = 'Show Advanced Information';

/***********	wlsurvey.htm	************/
var wlsurvey_onewlan_header = 'Wireless Site Survey</p>';
var wlsurvey_morewlan_header = 'Wireless Site Survey -wlan';
var wlsurvey_header_explain = 'This page provides a tool to scan for wireless networks. If an Access Point or IBSS is found, you could choose to connect to it manually when client mode is enabled.';
var wlsurvey_site_survey = 'Site Survey';
var wlsurvey_chan_next = '<input type="button" value="  Next>>" id="next" onClick="saveClickSSID()">';
var wlsurvey_encryption = 'Encryption:';
var wlsurvey_encryption_no = 'None';
var wlsurvey_encryption_wep = 'WEP';
var wlsurvey_encryption_wpa = 'WPA';
var wlsurvey_encryption_wpa2= 'WPA2';
var wlsurvey_encryption_wpa_mixed= 'WPA-MIXED';

var wlsurvey_keytype = 'Key Type:';
var wlsurvey_keytype_open = 'Open';
var wlsurvey_keytype_shared = 'Shared';
var wlsurvey_keytype_both = 'Both';

var wlsurvey_keylen = 'Key Length:';
var wlsurvey_keylen_64 = '64-bit';
var wlsurvey_keylen_128 = '128-bit';

var wlsurvey_keyfmt = 'Key Format:';
var wlsurvey_keyfmt_ascii = 'ASCII';
var wlsurvey_keyfmt_hex = 'Hex';

var wlsurvey_keyset = 'Key Setting';

var wlsurvey_authmode = 'Authentication Mode:';
var wlsurvey_authmode_enter_radius = 'Enterprise (RADIUS)';
var wlsurvey_authmode_enter_server = 'Enterprise (AS Server)';
var wlsurvey_authmode_personal = 'Personal (Pre-Shared Key)';
var wlsurvey_ieee_80211w='IEEE 802.11w:'
var wlsurvey_80211w_none='None'
var wlsurvey_80211w_capable='Capable'
var wlsurvey_80211w_required='Required'
var wlsurvey_sha256='SHA256'
var wlsurvey_sha256_disable='disable'
var wlsurvey_sha256_enable='enable'
var wlsurvey_wpacip = 'WPA Cipher Suite:';
var wlsurvey_wpacip_tkip = 'TKIP';
var wlsurvey_wpacip_aes = 'AES';
var wlsurvey_wp2chip = 'WPA2 Cipher Suite:';
var wlsurvey_wp3chip = 'WPA3 Cipher Suite:';
var wlsurvey_wp2chip_tkip = 'TKIP';
var wlsurvey_wp3chip_tkip = 'TKIP';
var wlsurvey_wp2chip_aes = 'AES';
var wlsurvey_wp3chip_aes = 'AES';

var wlsurvey_preshared_keyfmt = 'Pre-Shared Key Format:';
var wlsurvey_preshared_keyfmt_passphrase = 'Passphrase';
var wlsurvey_preshared_keyfmt_hex = 'HEX (64 characters)';
var wlsurvey_preshared_key = 'Pre-Shared Key:';

var wlsurvey_eaptype = 'EAP Type:';
var wlsurvey_eaptype_md5 = 'MD5';
var wlsurvey_eaptype_tls = 'TLS';
var wlsurvey_eaptype_peap = 'PEAP';

var wlsurvey_intunn_type = 'Inside Tunnel Type:';
var wlsurvey_intunn_type_MSCHAPV2 = 'MSCHAPV2';

var wlsurvey_eap_userid = 'EAP User ID:';
var wlsurvey_radius_passwd = 'RADIUS User Password';
var wlsurvey_radius_name = 'RADIUS User Name:';
var wlsurvey_user_keypasswd = 'User Key Password (if any):';


var wlsurvey_use_as_server = 'Use Local AS Server:';
var wlsurvey_as_ser_ipaddr = 'AS Server IP Address:';
var wlsurvey_back_button = '<<Back  ';
var wlsurvey_conn_button = 'Connect';
var wlsurvey_wait_explain = 'Please wait...';
var wlsurvey_inside_nosupport = 'This inside type is not supported.';
var wlsurvey_eap_nosupport = 'This EAP type is not supported.';
var wlsurvey_wrong_method = 'Wrong method!';
var wlsurvey_nosupport_method = 'Error: not supported method id';
var wlsurvey_nosupport_wpa2 = 'Error: not supported WPA2 cipher suite ';
var wlsurvey_nosupport_wpasuit = 'Error: not supported WPA cipher suite ';
var wlsurvey_nosupport_encry = 'Error: not supported encryption';

var wlsurvey_tbl_ssid = 'SSID';
var wlsurvey_tbl_bssid = 'BSSID';
var wlsurvey_tbl_chan = 'Channel';
var wlsurvey_tbl_type = 'Type';
var wlsurvey_tbl_ency = 'Encrypt';
var wlsurvey_tbl_signal = 'Signal';
var wlsurvey_tbl_select = 'Select';
var wlsurvey_tbl_macaddr = 'MAC Adddress';
var wlsurvey_tbl_meshid = 'Mesh ID';
var wlsurvey_tbl_none = 'None';

var wlsurvey_read_site_error = 'Read site-survey status failed!';
var wlsurvey_get_modemib_error = 'Get MIB_WLAN_MODE MIB failed!';
var wlsurvey_get_bssinfo_error = 'Get bssinfo failed!';
var wlsurvey_auto_scan_running='Auto scan running!!please wait...'
var wlsurvey_site_survey_failed='Site-survey request failed!'
var wlsurvey_scan_request_timout='scan request timeout!'
var wlsurvey_scan_timeout='scan timeout!'
var wlsurvey_please_refresh_again='Please refresh again!'
var wlsurvey_connect_failed1='Connect failed 1!'
var wlsurvey_get_meshid_error='get MIB_WLAN_MESH_ID error!'
var wlsurvey_connect_failed2='Connect failed 2!!'
var wlsurvey_set_meshid_error='Set MeshID error!'
var wlsurvey_set_channel_error='Set Channel error!'
var wlsurvey_set_wpa_error='Set WPA error!'
var wlsurvey_connect_failed3='Connect failed 3!!'
var wlsurvey_set_networktype_failed='Set MIB_WLAN_NETWORK_TYPE failed!'
var wlsurvey_set_channelnum_error='Set channel number error!'
var wlsurvey_get_join_result_failed='Get Join result failed!'
var wlsurvey_network_key_mismatch='Network Security Key Mismatch!'
var wlsurvey_connect_timeout='connect timeout!'
var wlsurvey_connect_successfully='Connect successfully!'
var wlsurvey_connect_success_reboot='Connect successfully! Please wait while rebooting.'
var wlsurvey_mic_error='MIC error, please check the password!'
var wlsurvey_connect_addto_profile='Add to Wireless Profile'
var wlsurvey_connect_save='Save'
var wlsurvey_connect_save_apply='Save&Apply'
var wlsurvey_connect_ok='OK'
/***********	wlwds.htm	************/
var wlwds_onelan_header = 'WDS Settings</p>';
var wlwds_morelan_header = 'WDS Settings -wlan';
var wlwds_header_explain = 'Wireless Distribution System uses the wireless media to communicate with other APs, as Ethernet does. To do this, you must set these APs to the same channel and set the MAC address of other APs that you want to communicate with in the table, and then enable WDS.';
var wlwds_enable = 'Enable WDS';
var wlwds_mac_addr = 'MAC Address:';
var wlwds_data_rate = 'Data Rate:';
var wlwds_rate_auto = 'Auto';
var wlwds_comment = 'Comment:';
var wlwds_apply = 'Apply Changes';
var wlwds_reset = 'Reset';
var wlwds_save = 'Save';
var wlwds_save_apply = 'Save & Apply';

var wlwds_set_secu = 'Set Security';
var wlwd_show_stat = 'Show Statistics';
var wlwds_wdsap_list = 'Current WDS AP List:';
var wlwds_delete_select = 'Delete Selected';
var wlwds_delete_all = 'Delete All';
var wlwds_fmwlan_txrate = 'Tx Rate (Mbps)';
var wlwds_fmwlan_select = 'Select';
var wlwds_fmwlan_comment = 'Comment';
var wlwds_macaddr_nocomplete = 'Input MAC address is not complete. It should be 12 digits in hex.';
var wlwds_macaddr_invalid = 'Invalid MAC address. It should be in hex numbers (0-9 and a-f).';
var wlwds_delete_chick = 'Do you really want to delete the selected entry?';
var wlwds_delete_allchick = 'Do you really want to delete all entries?';
var wlwds_set_enable_flag="Set enabled flag error!"
var wlwds_invalid_mac_address="Error! Invalid MAC address."
var wlwds_comment_length_too_long='Error! Comment length too long.'
var wlwds_get_entry_number='Get entry number error!'
var wlwds_cannot_add_new_entry="Cannot add new entry because table is full!"
var wlwds_rule_exist="rule already exist!"
var wlwds_add_table_entry_error="Add table entry error!"
var wlwds_get_entry_number_error="Get entry number error!"
var wlwds_get_table_entry_error="Get table entry error!"
var wlwds_delete_table_entry_error="Delete table entry error!"
var wlwds_delete_all_table_error="Delete all table error!"

/***********	wlactrl.htm	************/
var wlactrl_header_for_double='Wireless Access Control -wlan'
var wlactrl_onelan_header = 'Wireless Access Control</p>';
var wlactrl_morelan_header = 'Wireless Access Control -wlan';
var wlactrl_header_explain = 'If you choose Allowed Listed, only those clients whose wireless MAC addresses are in the access control list will be able to connect to your Access Point. When Deny Listed is selected, these wireless clients on the list will not be able to connect to the Access Point.';
var wlactrl_accmode = 'Wireless Access Control Mode:';
var wlactrl_accmode_diable = 'Disable';
var wlactrl_accmode_allowlist = 'Allow Listed';
var wlactrl_accmode_denylist = 'Deny Listed';
var wlactrl_macaddr = 'MAC Address:';
var wlactrl_comment = 'Comment: ';
var wlactrl_select='Select'
var wlactrl_apply = 'Apply Changes';
var wlactrl_reset = 'Reset';
var wlactrl_save = 'Save';
var wlactrl_save_apply = 'Save & Apply';

var wlactrl_accctrl_list = 'Current Access Control List:';
var wlactrl_delete_select_btn = 'Delete Selected';
var wlactrl_delete_all_btn = 'Delete All';
var wlactrl_fmwlan_macaddr = 'MAC Address';
var wlactrl_fmwlan_select = 'Select';
var wlactrl_apply_explain = 'If the ACL allow list is turned on; WPS2.0 will be disabled';
var wlactrl_apply_mac_short = 'Input MAC address is not complete. It should be 12 digits in hex.';
var wlactrl_apply_mac_invalid = 'Invalid MAC address. It should be in hex numbers (0-9 and a-f).';
var wlactrl_delete_result = 'Deleting all entries will cause all clients to be unableto connect to the AP.  Are you sure?';
var wlactrl_delete_select = 'Do you really want to delete the selected entry?';
var wlactrl_delete_all = 'Do you really want to delete all entries?';
var wlactrl_set_enable_flag="Set enabled flag error!"
var wlactrl_invalid_mac_address="Error! Invalid MAC address."
var wlactrl_comment_length_too_long='Error! Comment length too long.'
var wlactrl_get_entry_number='Get entry number error!'
var wlactrl_cannot_add_new_entry="Cannot add new entry because table is full!"
var wlactrl_rule_exist="rule already exist!"
var wlactrl_add_table_entry_error="Add table entry error!"
var wlactrl_get_entry_number_error="Get entry number error!"
var wlactrl_get_table_entry_error="Get table entry error!"
var wlactrl_delete_table_entry_error="Delete table entry error!"
var wlactrl_delete_all_table_error="Delete all table error!"

/***********	firewall	************/
var firewall_proto = 'Protocol:';
var firewall_proto_both = 'Both';
var firewall_proto_tcp = 'TCP';
var firewall_proto_udp = 'UDP';
var firewall_add_rule = 'Add rule';
var firewall_apply = 'Apply Changes';
var firewall_reset = 'Reset';
var firewall_filtertbl = 'Current Filter Table:';
var firewall_delete_select = 'Delete Selected';
var firewall_delete_all = 'Delete All';

var firewall_delete_selectconfm = 'Do you really want to delete the selected entry?';
var firewall_delete_allconfm = 'Do you really want to delete all entries?';
var firewall_ipaddr_invalid = 'Invalid IP address';
var firewall_port_notdecimal = 'Invalid port number! It should be the decimal numbers (0-9).';
var firewall_port_toobig = 'Invalid port number! You should set a value between 1-65535.';
var firewall_port_rangeinvalid = 'Invalid port range! 1st port value should be less than the 2nd value.';



var firewall_local_ipaddr = 'Loal IP Address:';
var firewall_port_range = 'Port Range: ';
var firewall_comm = 'Comment:';
var firewall_ip_invalid_range = 'Invalid ip address range!\nEnding address should be greater than or equal to starting address.';

var firewall_tbl_proto = 'Protocol';
var firewall_tbl_comm = 'Comment';
var firewall_tbl_select = 'Select';
var firewall_tbl_localipaddr = 'Local IP Address';
var firewall_portrange = 'Port Range';

/***********	portfilter.htm	************/
var portfilter_header = 'Port Filtering';
var portfilter_header_explain = 'Entries in this table are used to restrict certain types of data packets from your local network passing to the Internet through the Gateway. Use of these filters can be helpful in securing or restricting your local network.';
var portfiletr_enable = 'Enable Port Filtering';
var portfilter_noport = 'You should set a port range for adding in an entry.';
var portfiletr_both = 'Both';
var portfiletr_port_range = 'Port Range: ';
var portfiletr_proto = 'Protocol:';
var portfiletr_comm = 'Comment:';
var portfiletr_filtertbl = 'Current Filter Table:';
var portfiletr_save = 'Save';
var portfiletr_apply = 'Save & Apply';
var portfiletr_reset = 'Reset';
var portfilter_delete = 'Delete Selected';
var portfilter_delete_all = 'Delete All';
var portfilter_port_invalid = 'Invalid port number! It should be the decimal number (0-9).';
var portfilter_port_invalid_num = 'Invalid port number! You should set a value between 1-65535.';
var portfilter_port_invalid_range = 'Invalid port range! 1st port value should be less than 2nd value.';
var portfilter_delete_selectconfm = 'Do you really want to delete the selected entry?';
var portfilter_delete_allconfm = 'Do you really want to delete the all entries?';
var portfiletr_tbl_range = 'Port Range';
var portfiletr_tbl_protocol = 'Protocol';
var portfiletr_tbl_comm = 'Comment';
var portfiletr_tbl_select = 'Select';



/***********	ipfilter.htm	************/
var ipfilter_header = 'IP Filtering';
var ipfilter_header_explain = 'Entries in this table are used to restrict certain types of data packets from your local network passing to the Internet through the Gateway. Use of such filters can be helpful in securing or restricting your local network.';
var ipfilter_enable = 'Enable IP Filtering';
var ipfilter_ipv4_enable = 'Enable IPv4';
var ipfilter_ipv6_enable = 'Enable IPv6';
var ipfilter_local_ipv4_addr = 'Local IPv4 Address: ';
var ipfilter_dst_ipv4_addr = 'Destination IPv4 Address: ';
var ipfilter_local_ipv6_addr = 'Local IPv6 Address: ';
var ipfilter_mode = 'IP Filtering Mode: ';
var ipfilter_mode_black = 'blacklist';
var ipfilter_mode_white = 'whitelist';
var ipfilter_allwan = 'All WAN';
var ipfilter_proto = 'Protocol:';
var ipfilter_comm = 'Comment:';
var ipfilter_save = 'Save';
var ipfilter_apply = 'Save & Apply';
var ipfilter_reset = 'Reset';
var ipfilter_filtertbl = 'Current Filter Table:';
var ipfilter_tbl_local_addr = 'Local IP Address';
var ipfilter_tbl_local_end_addr = 'Local End IP Address';
var ipfilter_tbl_dst_addr = 'Destination IP Address';
var ipfilter_tbl_dst_end_addr = 'Destination End IP Address';
var ipfilter_tbl_proto = 'Protocol';
var ipfilter_tbl_wan = 'Wan Name';
var ipfilter_tbl_comm = 'Comment';
var ipfilter_tbl_select = 'Select';
var ipfilter_delete = 'Delete Selected';
var ipfilter_delete_all = 'Delete All';
var ipfilter_delete_selectconfm = 'Do you really want to delete the selected entry?';
var ipfilter_delete_allconfm = 'Do you really want to delete the all entries?';
var ipfilter_change_to_blacklist = 'Change the filter mode to blacklist?';
var ipfilter_change_to_whitelist = 'Change the filter mode to whitelist?';
var ipfilter_ipaddr_invalid = 'Invalid IP address';
var ipfilter_ip6addr_invalid = 'IPv6 address invalid!';
var ipfilter_set_error1 = 'Please select the ip-forwarding wan.';
var ipfilter_set_mode_error = 'Set mode flag error!';
var ipfilter_no_ipaddr = 'Error! No ip address to set.';
var ipfilter_ipaddr_not_within_subnet = 'Invalid IP address! It should be set within the current subnet.';
var ipfilter_miss_ipaddr = 'Error! Missing address to set.';



/***********	Macfilter.htm	************/
var macfilter_header = 'MAC Filtering';
var macfilter_header_explain =  'Entries in this table are used to restrict certain types of data packets from your local network passing to the Internet through the Gateway. Use of such filters can be helpful in securing or restricting your local network.';
var macfilter_enable = 'Enable MAC Filtering';
var macfilter_macaddr = 'MAC Address: ';
var macfilter_smacaddr = 'SMAC Address: ';
var macfilter_dmacaddr = 'DMAC Address: ';
var macfilter_smac = 'SMAC Address';
var macfilter_dmac = 'DMAC Address';
var macfilter_comment = 'Comment';
var macfilter_select = 'Select';
var macfilter_comm = 'Comment: ';
var macfilter_macaddr_nocomplete = 'Input MAC address is not complete. It should be 12 digits in hex.';
var macfilter_macaddr_nohex = 'Invalid MAC address. It should be in hex numbers (0-9 or a-f).';
var macfilter_macaddr_nozero = 'Invalid MAC address. It should not be 00:00:00:00:00:00.';
var macfilter_macaddr_nobroadcast = 'Invalid MAC address. It should not be ff:ff:ff:ff:ff:ff.';
var macfilter_macaddr_nomulticast = 'Invalid MAC address. It should not be multicast mac address between 01:00:5e:00:00:00 and 01:00:5e:7f:ff:ff.';
var macfilter_filterlist_macaddr = 'MAC Address';
var macfilter_save = 'Save';
var macfilter_apply = 'Save & Apply';
var macfilter_reset = 'Reset';
var macfilter_current_table = 'Current Filter Table:';
var macfilter_delete = 'Delete Selected';
var macfilter_delete_all = 'Delete All';
var macfilter_delete_confirm = 'Do you really want to delete the selected entry?';
var macfilter_delete_all_confirm = 'Do you really want to delete the all entries?';
var macfilter_blacklist = 'blacklist';
var macfilter_whitelist = 'whitelist';

/***********	access_internet_user_limit.htm	************/
var userlimit_header = 'Access Internet User Limit Setting';
var userlimit_header_explain =  'Entries in this table are used to limit the number of users which can access the internet.';
var userlimit_enable = 'Enable Access Internet User Limit';
var userlimit_num = 'Input Limit User Number';
var userlimit_save = 'Save';
var userlimit_apply = 'Save & Apply';
var userlimit_reset = 'Reset';
var limit_user_num_wrong = 'Limit User Num should be 0-4';


/***********	Portfw.htm	************/
var portfw_header = 'Port Forwarding';
var portfw_header_explain = 'Entries in this table allow you to automatically redirect common network services to a specific machine behind the NAT firewall.  These settings are only necessary if you wish to host some sort of server such as a web server or mail server on the private local network behind your Gateway\'s NAT firewall.';//need xiugai
var portfw_enable = 'Enable Port Forwarding';
var portfw_ipaddr = 'IP Address:';
var portfw_apply_port_empty = 'Port range cannot be empty! You should set a value between 1-65535.';
var portfw_tbl = 'Current Port Forwarding Table:';
var portfw_delete_confirm = 'Do you really want to delete the selected entry?';
var portfw_delete_all_confirm = 'Do you really want to delete the all entries?';
var portfw_all_wan = 'All WAN:';
var portfw_local_ipaddr = 'Local IP Address:';
var portfw_local_portrange = 'Local Port Range:';
var portfw_proto = 'Protocol:';
var portfw_both = 'Both';
var portfw_remote_ipaddr = 'Remote IP Address:';
var portfw_remote_portrange = 'Remote Port Range:';
var portfw_comm = 'Comment: ';
var portfw_save = 'Save';
var portfw_apply = 'Save & Apply';
var portfw_reset = 'Reset';
var portfw_delete = 'Delete Selected';
var portfw_delete_all = 'Delete All';
var portfw_port_invalid1 = 'Invalid port number! It should be the decimal number (0-9).';
var portfw_port_invalid2 = 'Invalid port number! You should set a value between 1-65535.';
var portfw_port_invalid3 = 'Invalid port range! 1st port value should be less than 2nd value.';
var portfw_externel_port_empty = 'externel Port cannot be empty! You should set a value between 1-65535.';
var portfw_externel_port_invalid1 = 'Invalid external-port number! It should be the decimal number (0-9).';
var portfw_externel_port_invalid2 = 'Invalid external-port number! You should set a value between 1-65535.';
var portfw_externel_port_invalid3 = 'Invalid external-port range! 1st port value should be less than 2nd value.';
var portfw_tbl_local_ipaddr = 'Local IP Address';
var portfw_tbl_local_portrange = 'Local Port Range';
var portfw_tbl_proto = 'Protocol';
var portfw_tbl_remote_ipaddr = 'Remote IP Address';
var portfw_tbl_remote_portrange = 'Remote Port Range';
var portfw_tbl_wan = 'Wan Name';
var portfw_tbl_status = 'Status';
var portfw_tbl_remote_comm = 'Comment';
var portfw_tbl_select = 'Select';
var portfw_tbl_portrange = 'Port Range';
var portfw_ipaddr_invalid = 'Invalid IP address';


/***********	urlfilter.htm	************/
var urlfilter_header = 'URL Filtering';
var urlfilter_header_explain = 'The URL filter is used to restrict LAN users access to the internet. Block those URLs which contain keywords listed below.';
var urlfilter_enable = 'Enable URL Filtering';
var urlfilter_urladdr = 'URL Address: ';
var urlfilter_apply_error = 'Error Character: \";\"';
var urlfilter_filterlist_yrladdr = 'URL Address';
var urlfilter_blacklist = 'deny url address(black list)';
var urlfilter_whitelist = 'allow url address(white list)';
var urlfilter_save = 'Save';
var urlfilter_apply = 'Save & Apply';
var urlfilter_reset = 'Reset';
var urlfilter_current_table = 'Current Filter Table:';
var urlfilter_delete = 'Delete Selected';
var urlfilter_delete_all = 'Delete All';
var urlfilter_delete_confirm = 'Do you really want to delete the selected entry?';
var urlfilter_delete_all_confirm = 'Do you really want to delete the all entries?';
var urlfilter_select = 'Select';
var urlfilter_ipaddr = 'IP Address';
var urlfilter_macaddr = 'Mac Address';
var urlfilter_user_mode = 'User mode: ';
var urlfilter_user_mode_IP = 'Ip Address:  ';
var urlfilter_user_mode_Mac = 'Mac Address: ';
var urlfilter_user_mode_all = 'All users (default rule)';


/***********	vlan_binding.html	************/
var vlan_binding_header = 'VLAN Binding';
var vlan_binding_header_explain = 'Entries in this table are used to set lan vlans binding to wan vlans configure as follows: "m1/n1,m2/n2...", among them, "m" means lan side vlan, "n" means wan side vlan.';
var vlan_binding_id = 'VLAN Id';
var vlan_binding_lanid = 'only support one lan vid forward by hardward, do you really wan to change hw lan vid to';
var vlan_binding_delete_select = 'Do you really want to delete the selected entry?';
var vlan_binding_delete_all = 'Do you really want to delete the all entries?';
var vlan_binding_enable = 'Enable VLAN Binding';
var vlan_binding_type = 'Binding Type:';
var vlan_binding_port = 'Port Binding';
var vlan_binding_vlan = 'VLAN Binding';
var vlan_binding_bindvlan = 'Binding Vlan:';
var vlan_binding_hw_nat = 'Hw Nat Lan VID:';
var vlan_binding_cur_table = 'Current VLAN Binding Table:';
var vlan_binding_lan_dev = 'LAN Device';
var vlan_binding_type2 = 'Binding Type';
var vlan_binding_vlan2 = 'Binding Vlan';
var vlan_binding_select = 'Select';
var vlan_binding_apply = 'Apply Changes';
var vlan_binding_reset = 'Reset';
var vlan_binding_save = 'Save';
var vlan_binding_save_apply = 'Save & Apply';
var vlan_binding_delete_select = 'Delete Selected';
var vlan_binding_delete_all = 'Delete All';
var vlan_binding_lan_dev_list = 'Lan Device List:';

var vlan_binding_err0 = 'please set port binding in WAN page!';
var vlan_binding_err1 = 'VID invalid!, should between 1-4095!';
var vlan_binding_err2 = 'the same lan device cannot use two entry, if you want to update the settings, please delete this entry and add it again!';
var vlan_binding_err3 = 'the length of Binding Vlan is too long! max length:';
var vlan_binding_curlen = 'current length:';
var vlan_binding_err4 = 'Invalid hw nat lan vid, non-existence!';

var vlan_binding_err5 = 'format error! configure as follows: \"m1/n1,m2/n2...\", among them, \"m\" means lan side vlan, \"n\" means wan side vlan.';
var vlan_binding_err6 = 'binding lan vlan group to bridge wan, lan/wan vlan should be same, untag wan please use vid 8!';
var vlan_binding_err7 = 'binding lan vlan group to non-existence wan side vlan, please configure wan first, untag wan please use vid 8!';
var vlan_binding_err8= 'the count of vlan pair more should less than or equal to';


/***********	algonoff.asp	************/
var algonoff_enable_L2TP = 'Enable L2TP ALG';
var algonoff_enable_IPSec = 'Enable IPSec ALG';
var algonoff_enable_H323 = 'Enable H.323 ALG';
var algonoff_enable_RSTP = 'Enable RSTP ALG';
var algonoff_enable_SIP = 'Enable SIP ALG';
var algonoff_enable_FTP = 'Enable FTP ALG';
var algonoff_enable_PPTP = 'Enable PPTP ALG';
var algonoff_apply = 'Save & Apply';



/***********	dmz.htm	************/
var dmz_header = 'DMZ';
var dmz_header_explain = 'A Demilitarized Zone is used to provide Internet services without sacrificing unauthorized access to its local private network. Typically, the DMZ host contains devices accessible to Internet traffic, such as Web (HTTP ) servers, FTP servers, SMTP (e-mail) servers, and DNS servers.';
var dmz_enable = 'Enable DMZ';
var dmz_host_ipaddr = 'DMZ Host IP Address:';
var dmz_ip_invalid = 'Invalid IP address';
var dmz_checkip1 = 'IP address cannot be empty! It should be filled with 4 digit numbers as xxx.xxx.xxx.xxx.';
var dmz_checkip2 = ' value. It should be the decimal number (0-9).';
var dmz_checkip3 = 'range in 1st digit. It should be 1-223.';
var dmz_checkip4 = 'range in 1st digit. It should not be 127.';
var dmz_checkip5 = 'range in 2nd digit. It should be 0-255.';
var dmz_checkip6 = 'range in 3rd digit. It should be 0-255.';
var dmz_checkip7 = 'range in 4th digit. It should be 1-254.';
var dmz_save = 'Save';
var dmz_apply = 'Save & Apply';
var dmz_reset = 'Reset';

var dmz_ip_error = 'Invalid IP address! It should be set within the current subnet.';
var dmz_set_mib_error = 'Set DMZ MIB error!';

var dmz_ip_error2 = 'Invalid IP address! It should not equal to the Lan IP address.';



/***********	app_upnp_cmcc.htm	************/
var upnp_header = 'UPnP Setting';
//var upnp_header_explain = 'Support UPnP feature';
var app_upnp_cmcc_enable = 'Enable UPnP';
var app_upnp_cmcc_yes = 'Yes';
var app_upnp_cmcc_cancel = 'Cancel';
var app_upnp_cmcc_portmapping_wan = 'Wan List for Port-Mapping:';


/***********	snooping_proxy_cmcc.asp	************/
var cmcc_enable_snooping = 'Enable IGMP/MLD Snooping:';
var cmcc_enable_proxy = 'Enable IGMP/MLD Proxy:';
var snooping_proxy_cmcc_apply = 'Apply';

/***********	samba.asp	************/
var app_samba_enable = 'Enable Samba:';

/***********	minidlna.asp	************/
var app_minidlna_enable = 'Enable DLNA:';

/***********	logout.htm	************/
var logout_header = 'Logout';
var logout_header_explain = 'This page is used to  logout.';
var logout_confm = 'Do you want to logout ?';
var logout_apply = 'Apply Changes';

/***********	password.htm	************/
var password_header = 'Password Setup';
var password_header_explain = ' This page is used to setup an account to access the web server of the Access Point.';
var password_user_name = 'User Name:';
var password_user_passwd = 'New Password:';
var password_user_passwd_confm = 'Confirm Password:';
var password_apply = 'Apply Changes';
var password_user_empty = 'User account is empty. Please input the user name. ';
var password_passwd_unmatched = 'Password does not match. Please re-type the password in the \'new\' and \'confirmed\' boxes.';
var password_passwd_empty = 'Password cannot be empty. Please try again.';
var password_user_invalid = 'Cannot accept space character in user name. Please try again.';
var password_passwd_invalid = 'Cannot accept space character in password. Please try again.';
var password_reset = '  Reset  ';
var password_save = 'Save';
var password_save_apply = 'Save & Apply';
var password_err_username = 'ERROR: userName cannot be empty.';
var password_err_password = 'ERROR: Password cannot be empty.';
var password_change_setting = 'Change setting successfully!';
var password_not_turnoff_router = 'Do not turn off or reboot the Router during this time.';
var password_wait = 'Please wait ';
var password_seconds = 'seconds ...';

/***********	super_password.htm	************/
var super_password_header_explain = 'This page is used to setup an super user and common user account to access the web server of the Access Point.';

var super_password_super_user = 'Super User';
var super_password_super_user_empty = 'Super User account cannot be empty.Please try again.';
var super_password_super_passwd_empty = 'Password of Super User cannot be empty.Please try again.';
var super_password_super_user_invalid = 'Cannot accept space character in Super User name. Please try again.';
var super_password_super_passwd_invalid = 'Cannot accept space character in Super User password. Please try again.';
var super_password_super_passwd_unmatch = 'Password of Super User does not match. Please re-type the password.';
var super_password_err_super_username = 'ERROR: Super User Name cannot be empty.';
var super_password_err_super_password = 'ERROR: Super User Password cannot be empty.';

var super_password_common_user = 'Common User';
var super_password_user_empty = 'Common User account cannot be empty.Please try again.';
var super_password_passwd_empty = 'Password of Common User cannot be empty.Please try again.';
var super_password_user_invalid = 'Cannot accept space character in Common User name. Please try again.';
var super_password_passwd_invalid = 'Cannot accept space character in Common User password. Please try again.';
var super_password_passwd_unmatch = 'Password of Common User does not match. Please re-type the password.';
var super_password_err_username = 'ERROR: Common User Name cannot be empty.';
var super_password_err_password = 'ERROR: Common User Password cannot be empty.';


var super_password_users_invalid = 'The Super User and Common User accound cannot be the same. Please re-set the account.';
var super_password_set_mib_fail= 'Set MIB fail';

/***********	saveconf.htm	************/
var saveconf_header = 'Save/Reload Settings';
var saveconf_header_explain = ' This page allows you to save current settings to a file or reload the settings from a file that was saved previously. You can also reset the current configuration to factory defaults.';
var saveconf_save_to_file = 'Save Settings to File:';
var saveconf_save = 'Save...';
var saveconf_load_from_file = 'Load Settings from File:';
var saveconf_load = 'Upload';
var saveconf_reset_to_default = 'Reset Settings to Default:';
var saveconf_reset = 'Reset';
var saveconf_load_from_file_empty = 'Please select a file!';
var saveconf_reset_to_default_confm = 'Do you really want to reset the current settings to default settings?';
var saveconf_select_file = 'please select a file!';
var saveconf_reload_setting = 'Reload setting successfully!';
var saveconf_router_booting = 'The Router is booting.';
var saveconf_aprouter_booting = 'The AP is booting.';
var saveconf_not_turnoff_reboot = 'Do not turn off or reboot the Device during this time.';
var saveconf_invaild_format = 'Invalid file format!';
var saveconf_invaild_config = 'Invalid configuration file!';
var saveconf_update_sucessful = 'Update successfully!';
var saveconf_updating = 'Update in progressing.';
var saveconf_not_turnoff_ap = 'Do not turn off or reboot the AP during this time.';
var saveconf_buffer_failed = 'Allocate buffer failed!';
var saveconf_current_setting_failed = 'Write default to current setting failed!\n';
var saveconf_wait = 'Please wait ';
var saveconf_seconds = 'seconds ...';

/***********	upload.htm	************/
var upload_header = 'Upgrade Firmware';
var upload_header_explain = ' This page allows you to upgrade the Access Point firmware to the latest version. Please note, do not power off the device during the upload as it may crash the system.';
var upload_explain = ' This page allows you upgrade the  firmware from usb storage device(ex. /tmp/usb/sda1/fw.bin). Please note, do not power off the device during the upload because it may crash the system.';
var upload_version = 'Firmware Version:';
var upload_file_select = 'Select File:';
var upload_send = 'Upload';
var upload_reset = 'Reset';
var upload_up_failed = 'Update firmware failed!';
var upload_send_file_empty = 'File name cannot be empty!';
var upload_usb_update_not_enable = 'usb update image function is not enabled!';
var upload_cannot_upgrade = 'Can\'t upgrade firmware by web UI! ACS has set .ManagementServer.UpgradesManaged to true';
var upload_invalid_format = 'Invalid file format !';
var upload_dual_firmware = 'Enable Dual Firmware';
var upload_wait = 'Please wait...';

/***********	syslog.htm	************/
var syslog_header = 'System Log';
var syslog_header_explain = 'This page can be used to set a remote log server and view the system log.';
var syslog_enable = 'Enable Log';
var syslog_sys_enable = 'System All';
var syslog_wireless_enable = 'Wireless';
var syslog_dos_enable = 'DoS';
var syslog_11s_enable = '11s';
var syslog_rtlog_enable = 'Enable Remote Log';
var syslog_local_ipaddr = 'Log Server IP Address:';
var syslog_apply = 'Apply Changes';
var syslog_refresh = 'Refresh';
var syslog_clear = 'Clear';
var syslog_invalid_server_address = 'Invalid Server address!';
var syslog_display_header = 'Display System Log';
var syslog_display_header_explain = 'Display system log according to log level.';
var syslog_emerg = 'Emergency';
var syslog_alert = 'Alert';
var syslog_crit = 'Critical';
var syslog_err = 'Error';
var syslog_warn = 'Warn';
var syslog_notice = 'Notice';
var syslog_info = 'Information';
var syslog_debug = 'Debug';
var syslog_display_level = 'Display level:';
var syslog_savelog = 'Save log';
var syslog_clrlog = 'Clear log';


/***********	dos.htm	************/
var dos_header = 'Denial of Service';
var dos_header_explain = 'A "denial-of-service" (DoS) attack is characterized by an explicit attempt by hackers to prevent legitimate users of a service from using that service.';
var dos_enable = 'Enable DoS Prevention';
var dos_packet_sec = ' Packets/Second';
var dos_sysflood_syn = 'Whole System Flood: SYN';
var dos_sysflood_fin = 'Whole System Flood: FIN';
var dos_sysflood_udp = 'Whole System Flood: UDP';
var dos_sysflood_icmp = 'Whole System Flood: ICMP';
var dos_ipflood_syn = 'Per-Source IP Flood: SYN';
var dos_ipflood_fin = 'Per-Source IP Flood: FIN';
var dos_ipflood_udp = 'Per-Source IP Flood: UDP';
var dos_ipflood_icmp = 'Per-Source IP Flood: ICMP';
var dos_portscan = 'TCP/UDP PortScan';
var dos_portscan_low = 'Low';
var dos_portscan_high = 'High';
var dos_portscan_sensitivity = 'Sensitivity';
var dos_icmp_smurf = 'ICMP Smurf';
var dos_ip_land = 'IP Land';
var dos_ip_spoof = 'IP Spoof';
var dos_ip_teardrop = 'IP TearDrop';
var dos_pingofdeath = 'PingOfDeath';
var dos_tcp_scan = 'TCP Scan';
var dos_tcp_synwithdata = 'TCP SynWithData';
var dos_udp_bomb = 'UDP Bomb';
var dos_udp_echochargen = 'UDP EchoChargen';
var dos_select_all = ' Select ALL ';
var dos_clear_all = 'Clear ALL';
var dos_enable_srcipblocking = 'Enable Source IP Blocking';
var dos_block_time = 'Block time (sec)';
var dos_apply = 'Save & Apply';
var dos_enable_ipv6 = 'Enable Ipv6 DoS Prevention';
var dos_ipv6_smurf= 'Ipv6 Smurf';
var dos_ipv6_icmp_big= 'Ipv6 Icmp Too Big';
var dos_ipv6_Flood= 'Ipv6 Ns Flood';
var dos_Winnuke= 'Winnuke';
var dos_icmp_redirection= 'ICMP Redirection';
var dos_save = 'Save';



/***********	ntp.htm	************/
var ntp_header = 'Time Zone Setting';
var ntp_header_explain = 'You can maintain the system time by synchronizing with a public time server over the Internet.';
var ntp_curr_time = 'Current Time:';
var ntp_year = 'Yr';
var ntp_month = 'Mon';
var ntp_day = 'Day';
var ntp_hour = 'Hr';
var ntp_minute = 'Mn';
var ntp_second = 'Sec';
var ntp_copy_comptime = 'Copy Computer Time';
var ntp_time_zone = 'Time Zone Select:';
var ntp_enable_clientup = 'Enable NTP client Update';
var ntp_adjust_daylight = 'Automatically Adjust for Daylight Saving';
//var ntp_server = ' SNTP server:';
var ntp_server = 'NTP server: ';
var ntp_server_north_america1 = '198.123.30.132    - North America';
var ntp_server_north_america2 = '209.249.181.22   - North America';
//var ntp_server_Europe1 = '85.12.35.12  - Europe';
//var ntp_server_Europe2 = '217.144.143.91   - Europe';
var ntp_server_Europe1 = '131.188.3.220  - Europe';
var ntp_server_Europe2 = '130.149.17.8   - Europe';
var ntp_server_Australia = '223.27.18.137  - Australia';
//var ntp_server_asia1 = '133.100.11.8 - Asia Pacific';
var ntp_server_asia1 = '203.117.180.36 - Asia Pacific';
var ntp_server_asia2 = '210.72.145.44 - Asia Pacific';
var ntp_manu_ipset = '(Manual IP Setting)';
var ntp_save = 'Save';
var ntp_save_apply = 'Save & Apply';

var ntp_apply = 'Apply Change';
var ntp_reset = 'Reset';
var ntp_refresh = 'Refresh';
var ntp_month_invalid = 'Invalid month Number. It should be in  number (1-9).';
var ntp_time_invalid = 'Invalid Time value!';
var ntp_ip_invalid = 'Invalid IP address';
var ntp_servip_invalid = 'Invalid NTP Server IP address! It can not be empty.';
var ntp_field_check = 'Field can\'t be empty\n';
var ntp_invalid = 'Invalid';
var ntp_num_check = ' Number. It should be in  numbers (0-9).';

/***********	ddns.htm	************/
var ddns_header = 'Dynamic DNS Setting';
var ddns_header_explain = 'Dynamic DNS is a service that provides you with a valid, unchanging, internet domain name (an URL) to go with a (possibly changing) IP-address.';
var ddns_enable = 'Enable DDNS ';
var ddns_serv_provider = ' Service Provider:';
var ddns_dyndns = 'DynDNS';
var ddns_orayddns = 'OrayDDNS';
var ddns_tzo = 'TZO';
var ddns_domain_name = 'Domain Name:';
var ddns_user_name = ' User Name/Email:';
var ddns_passwd = ' Password/Key:';
var ddns_note = 'Note:';
var ddns_oray_header = 'For Oray DDNS, you can create your Oray account';
var ddns_here = 'here';
var ddns_dyn_header = 'For DynDNS, you can create your DynDNS account';
var ddns_tzo_header1 = 'For TZO, you can have a 30 days free trial';
var ddns_tzo_header2 = 'or manage your TZO account in';
var ddns_tzo_header3 = 'control panel';
var ddns_apply = 'Apply Change';
var ddns_reset = 'Reset';
var ddns_domain_name_empty = 'Domain Name can\'t be empty';
var ddns_user_name_empty = 'User Name/Email can\'t be empty';
var ddns_passwd_empty = 'Password/Key can\'t be empty';
var ddns_save = 'Save';
var ddns_save_apply = 'Save & Apply';


/***********	ip_qos.htm	************/
var ip_qos_header = 'QoS';
var ip_qos_header_explain = ' Entries in this table improve your online gaming experience by ensuring that your game traffic is prioritized over other network traffic, such as FTP or Web.';
var ip_qos_enable = 'Enable QoS';
var ip_qos_bandwidth = 'Bandwidth Shaping';
var ip_qos_schedule_sp = 'SP';
var ip_qos_schedule = 'WFQ';
var ip_qos_auto_upspeed = 'Automatic Uplink Speed';
var ip_qos_manu_upspeed = 'Uplink Speed (Kbps):';
var ip_qos_auto_downspeed = 'Automatic Downlink Speed';
var ip_qos_manu_downspeed = 'Downlink Speed (Kbps):';
var ip_qos_rule_set = 'QoS Rule Setting:';
var ip_qos_addrtype = 'Address Type:';
var ip_qos_addrtype_ip = 'IP';
var ip_qos_addrtype_mac = 'MAC';
var ip_qos_local_ipaddr = 'Local IP Address:';
var ip_qos_proto = 'Protocol:';
var ip_qos_proto_udp = 'udp';
var ip_qos_proto_tcp = 'tcp';
var ip_qos_proto_both = 'both';
var ip_qos_local_port = 'Local Port:(1~65535)';
var ip_qos_macaddr = 'MAC Address:';
var ip_qos_mode = 'Mode:';
var ip_qos_weight = 'Weight:(1~20)';
var ip_qos_priority = 'Priority:(1~8)';
var ip_qos_upband = 'Uplink Bandwidth (Kbps):';
var ip_qos_downband = 'Downlink Bandwidth (Kbps):';
var ip_qos_apply = 'Apply Changes';
var ip_qos_reset = 'Reset';
var ip_qos_curr_qos = 'Current QoS Rules Table:';
var ip_qos_delete_select_btn = 'Delete Selected';
var ip_qos_delete_all_btn = 'Delete All';

var ip_qos_upspeed_empty = 'Manual Uplink Speed cannot be empty or less then 100 when Automatic Uplink Speed is disabled.';
var ip_qos_downspeed_empty = 'Manual Downlink Speed cannot be empty or less then 100 when Automatic Downlink Speed is disabled.';
var ip_qos_ip_invalid = 'Invalid IP address';
var ip_qos_startip_invalid = 'Invalid start IP address! It should be set within the current subnet.';
var ip_qos_portrange_invalid = 'Invalid port range! It should be 1~65535.';
var ip_qos_macaddr_notcomplete = 'Input MAC address is not complete. It should be 12 digits in hex.';
var ip_qos_macaddr_invalid = 'Invalid MAC address. It should be in hex number (0-9 or a-f),and can not be all zero.';
var ip_qos_band_empty = 'Uplink Bandwidth or Downlink Bandwidth cannot be 0 or empty.';
var ip_qos_band_invalid = 'Invalid input! It should be the decimal number (0-9).';
var ip_qos_band_notint = 'Invalid input! It should be integer numbers';
var ip_qos_weight_empty = 'Weight cannot be empty.';
var ip_qos_weight_invalid = 'Invalid weight range! It should be int number (1~20).';
var ip_qos_priority_empty = 'Priority cannot be empty.';	/*by chloe...*/
var ip_qos_priority_invalid = 'Invalid priority range! It should be int number (1~8).';
var ip_qos_delete_select = 'Do you really want to delete the selected entry?';
var ip_qos_delete_all = 'Do you really want to delete the all entries?';

var ip_qos_tbl_localaddr = 'Local IP Address';
var ip_qos_tbl_macaddr = 'MAC Address';
var ip_qos_tbl_mode = 'Mode';
var ip_qos_tbl_valid = 'Valid';
var ip_qos_tbl_upband = 'Uplink Bandwidth';
var ip_qos_tbl_downband = 'Downlink Bandwidth';
var ip_qos_tbl_select = 'Select';
var ip_qos_restrict_maxband = "Restricted maximum bandwidth";
var ip_qos_quarant_minband = "Guaranteed minimum bandwidth";

/***********    qos_multiPhyWan_uplink.htm ************/
var uplink_multiPhyWan_qos_header = 'QoS MultiPhyWan Uplink Configuration';
var uplink_multiPhyWan_qos_header_explain = 'This page is used to configure uplink qos for each phy wan port.';

/*********** 	qos_multiPhyWan_rule.thm ***********/
var uplink_multiPhyWan_qos_rule_header = 'QoS MultiPhyWan Uplink Rule';
var uplink_multiPhyWan_qos_rule_header_explain = 'This page is used to configure uplink qos rules for each phy wan port.';

/***********	qos_shaping.htm	************/
var multiPhyWan_shaping_header = 'QoS MultiPhyWan Shaping Configuration';
var multiPhyWan_shaping_header_explain = 'This page is used to configure QoS shaping rules for each phy wan port based on LAN1, SSID, local ip address. etc.';


/***********	qos_uplink.htm	************/
var uplink_qos_header = 'QoS Uplink Configuration';
var uplink_qos_header_explain = 'This page is used to configure uplink qos.';

/***********	qos_rule.htm	************/
var uplink_qos_rule_header = 'QoS Uplink Rule';
var uplink_qos_rule_header_explain = 'This page is used to configure uplink qos rules.';


/***********	qos_shaping.htm	************/
var shaping_header = 'QoS Shaping Configuration';
var shaping_header_explain = 'This page is used to configure QoS shaping rules based on LAN1, SSID, local ip address. etc.';

/***********	wlbasic.htm	************/
var wlbasic_header_for_double="Wireless Basic Settings -wlan"
var wlbasic_tx_restrict="TX restrict:"
var wlbasic_rx_restrict="RX restrict:"
var wlbasic_multiple_ap="MultipleAP"
var wlbasic_show='Show'
var wlbasic_client_with_radius='You cannot set client mode with Enterprise (RADIUS) !<br><br>Please change the encryption method in security page first.'
var wlbasic_header='Wireless Basic Setting';
var wlbasic_explain = 'This page is used to configure the parameters for wireless LAN clients which may connect to your Access Point. Here you may change wireless encryption settings as well as wireless network parameters.';
var wlbasic_network_type = 'Network Type:';
var wlbasic_ssid = 'SSID:';
var wlbasic_disabled = 'Disable Wireless LAN Interface';
var wlbasic_country = 'Country:';
var wlbasic_band= 'Band:';
var wlbasic_infrastructure = "Infrastructure";
var wlbasic_adhoc = "Adhoc";
var wlbasic_addprofile = 'Add to Profile';
var wlbasic_channelwidth = 'Channel Width:';
var wlbasic_ctlsideband = 'Control Sideband:';
var wlbasic_ctlsideautomode = 'Auto';
var wlbasic_ctlsidelowermode = 'Lower';
var wlbasic_ctlsideuppermode = 'Upper';
var wlbasic_chnnelnum = 'Channel Number:';
var wlbasic_channelnum_auto='Auto'
var wlbasic_broadcastssid= 'Broadcast SSID:';
var wlbasic_brossid_enabled = 'Enabled';
var wlbasic_brossid_disabled = 'Disabled';
var wlbasic_wmm ='WMM:';
var wlbasic_wmm_disabled = 	'Disabled';
var wlbasic_wmm_enabled = 	'Enabled';
var wlbasic_data_rate = 'Data Rate:';
var wlbasic_ssid_priority = 'SSID Priority'
var wlbasic_data_rate_auto = "Auto";
var wlbasic_associated_clients = 'Associated Clients:';
var wlbasic_show_associated_clients = 'Show Active Clients';
var wlbasic_enable_mac_clone = 'Enable Mac Clone (Single Ethernet Client)';
var wlbasic_enable_repeater_mode = 'Enable Universal Repeater Mode (Acting as AP and client simultaneouly)';
var wlbasic_extended_ssid = 'SSID of Extended Interface:';
var wlbasic_ssid_note = 'Note: If you want to change the setting for Mode and SSID, you must go to the EasyConfig page to disable EasyConfig first.';
var wlbasic_enable_wl_profile = 'Enable Wireless Profile';
var wlbasic_wl_profile_list = 'Wireless Profile List:';
var wlbasic_apply = 'Apply Changes';
var wlbasic_reset = 'Reset';

var wlbasic_save = 'Save';
var wlbasic_save_apply = 'Save & Apply';

var wlbasic_delete_select = 'Delete Selected';
var wlbasic_delete_all = 'DeleteAll';
var wlbasic_enable_wire = 'Do you also want to enable wireless profile?';
var wlbasic_asloenable_wire = 'Do you also want to enable wireless profile?';
var wlbasic_no_restrict='(0:no restrict)'
var wlbasic_mode = 'Mode:';
var wlbasic_ap='AP'
var wlbasic_client = 'Client';
var wlbasic_wds='WDS';
var wlbasic_ap_wds='AP+WDS';
var wlbasic_ap_mesh='AP+MESH';
var wlbasic_mesh='MESH';
var wlbasic_wifi_config_ssid='SSID';
var wlbasic_wifi_config_encrypt='Encrypt';
var wlbasic_wifi_config_select='Select';
var wlbasic_channelnum_auto_for_5g='Auto(DFS)';
var wlbasic_set_disable_flag="Set disabled flag error!"
var wlbasic_set_wlan_reg="Set wlan regdomain error!"
var wlbasic_set_wlan_country="Set wlan countrystr error!"
var wlbasic_set_mib_wlan_tx="Set MIB_WLAN_TX_RESTRICT error!"
var wlbasic_set_mib_wlan_rx="Set MIB_WLAN_RX_RESTRICT error!"
var wlbasic_invalid_mode="Invalid mode value!"
var wlbasic_cannot_set_client="You cannot set client mode with 802.1x enabled!<br><br>Please change the encryption method in security page first."
var wlbasic_set_macclone="Set wlan Mac clone error!"
var wlbasic_set_mib_wlan_mode_error="Set MIB_WLAN_MODE error!"
var wlbasic_set_ssid_error="Set SSID error!"
var wlbasic_invalid_channel="Invalid channel number!"
var wlbasic_set_channel_number="Set channel number error!"
var wlbasic_invalid_network="Invalid network type value!"
var wlbasic_set_mib_wlan_network="Set MIB_WLAN_NETWORK_TYPE failed!"
var wlbasic_invalid_band="Invalid band value!"
var wlbasic_set_band_error="Set band error!"
var wlbasic_set_rate_adaptive="Set rate adaptive failed!"
var wlbasic_set_fixed_rate="Set fix rate failed!"
var wlbasic_set_tx_basic="Set Tx basic rate failed!"
var wlbasic_set_tx_operation="Set Tx operation rate failed!"
var wlbasic_invalid_channel_bond="Error! Invalid Channel Bonding."
var wlbasic_set_hidden_ssid="Set hidden ssid failed!"
var wlbasic_invalid_wmm="Error! Invalid WMM value."
var wlbasic_set_mib_wlan_wmm="Set MIB_WLAN_WMM_ENABLED failed!"
var wlbasic_set_mib_wlan_channel="Set MIB_WLAN_CHANNEL_BONDING failed!"
var wlbasic_invalid_control_sideband="Error! Invalid Control SideBand."
var wlbasic_set_mib_wlan_control_sideband="Set MIB_WLAN_CONTROL_SIDEBAND failed!"
var wlbasic_set_tx_basic_rate="Set Tx basic rate failed!"
var wlbasic_set_tx_operation="Set Tx operation rate failed!"
var wlbasic_ssid_of_extend_length="input 'SSID of Extended Interface' length is"
var wlbasic_oversize="oversize! Max should be"
var wlbasic_set_ssid_toolong="Error !Set SSID too long. "
var wlbasic_but_your_length="but you ssid length is"
/***********	wlstatbl.htm	************/
var wlstatbl_tbl_for_double='Active Wireless Client Table -wlan'
var wlstatbl_tbl_name = 'Active Wireless Client Table';
var wlstatbl_explain = ' This table shows the MAC address, transmission, reception packet counters and encrypted status for each associated wireless client.';
var wlstatbl_mac = 'MAC Address';
var wlstatbl_mode = 'Mode';
var wlstatbl_tx = 'Tx Packet';
var wlstatbl_rx = 'Rx Packe';
var wlstatbl_tx_rate ='Tx Rate (Mbps)';
var wlstatbl_ps = 'Power Saving';
var wlstatbl_expired_time = 'Expired Time (s)';
var wlstatbl_refresh = 'Refresh';
var wlstatbl_close = 'Close';
var wlstatbl_none='None'
var wlstatbl_yes='yes'
var wlstatbl_no='no'

/***********	wladvanced.htm	************/
var wladv_name_for_double='Wireless Advanced Settings -wlan'
var wladv_8021x_hs2='Set Security Type to 802.1x before turn on HS2 Daemon'
var wladv_vallid_num_alert = 'Invalid value. It should be in decimal numbers (0-9).';
var wladv_fragment_thre_alert = 'Invalid value of Fragment Threshold. Input value should be between 256-2346 in decimal.';
var wladv_fragment_aggr_alert = 'When choose band 11N or 11AC with Aggregation enabled, Frame Fragment is not allowed. Please change band type or disable Aggregation!';
var wladv_rts_thre_alert = 'Invalid value of RTS Threshold. Input value should be between 0-2347 in decimal.';
var wladv_beacon_alert = 'Invalid value of Beacon Interval. Input value should be between 20-1024 in decimal.';
var wladv_header = 'Wireless Advanced Settings';
var wladv_explain = 'These settings are only for more technically advanced users who have a sufficient knowledge about wireless LAN. These settings should not be changed unless you know what effect the changes will have on your Access Point.';
var wladv_frg_thre = 'Fragment Threshold:';
var wladv_rts_thre = 'RTS Threshold:';
var wladv_beacon_interval = 'Beacon Interval:';
var wladv_preamble_type = 'Preamble Type:';
var wladv_preamble_long = 'Long Preamble';
var wladv_preamble_short = 'Short Preamble';
var wladv_iapp = 'IAPP:';
var wladv_iapp_enabled = 'Enabled';
var wladv_iapp_disabled = 'Disabled';
var wladv_protection = 'Protection:';
var wladv_protection_enabled = 'Enabled';
var wladv_protection_disabled = 'Disabled';
var wladv_aggregation = 'Aggregation:';
var wladv_aggregation_enabled = 'Enabled';
var wladv_aggregation_disabled = 'Disabled';
var wladv_short_gi = 'Short GI:';
var wladv_short_gi_enabled = 'Enabled';
var wladv_short_gi_disabled = 'Disabled';
var wladv_wlan_partition = 'WLAN Partition:';
var wladv_wlan_partition_enabled = 'Enabled';
var wladv_wlan_partition_disabled = 'Disabled';
var wladv_stbc = 'STBC:';
var wladv_stbc_enabled = 'Enabled';
var wladv_stbc_disabled = 'Disabled';
var wladv_ldpc="LDPC:"
var wladv_ldpc_enabled = 'Enabled';
var wladv_ldpc_disabled = 'Disabled';
var wladv_coexist = '20/40MHz Coexist:';
var wladv_coexist_enabled = 'Enabled';
var wladv_coexist_disabled = 'Disabled';
var wladv_tx_beamform = 'TX Beamforming:';
var wladv_tx_beamform_enabled = 'Enabled';
var wladv_tx_beamform_disabled = 'Disabled';

var wladv_m2u = 'Mutilcast to Unicast:';
var wladv_m2u_enabled = 'Enabled';
var wladv_m2u_disabled = 'Disabled';
var wladv_tdls_prohibited = 'TDLS Prohibited:';
var wladv_tdls_prohibited_enabled = 'Enabled';
var wladv_tdls_prohibited_disabled = 'Disabled';
var wladv_tdls_channel_switch = 'TDLS Channel Switch Prohibited:';
var wladv_tdls_channel_switch_enabled = 'Enabled';
var wladv_tdls_channel_switch_disabled = 'Disabled';

var wladv_rf_power = 'RF Output Power:';
var wladv_apply = 'Apply Changes';
var wladv_reset = ' Reset ';

var wladv_save = 'Save';
var wladv_save_apply = 'Save & Apply';
var wladv_multicast_rate='Mutilcast Rate:'
var wladv_multicast_auto='Auto'
var wladv_80211k_support='802.11k Support:'
var wladv_crossband_support='Crossband Support:'
var wladv_enable_sta_control='Enable STA Control'
var wladv_prefer5g='Prefer 5GHz'
var wladv_prefer2g='Prefer 2GHz'
var wladv_fast_bss_tran='Fast BSS Transition Support:'
var wladv_support_overds='Support Over-the-DS:'
var wladv_80211v_bss_tran='802.11v BSS Transition Support:'
var wladv_smartroaming_support='Smart Roaming Support:'
var wladv_enable='Enabled'
var wladv_disable='Disabled'

/***********	wlsecutity.htm wlsecutity_all.htm	************/
var wlsec_header_for_double="Wireless Security Setup -wlan"

var wlsec_validate_note = "Note: if you choose [Enterprise (AS Server)] and apply changes here, \nthis wlan interface and its virtual interfaces will use the current AS setting.\nDo you want to continue?";
var wlsec_header = 'Wireless Security Setup';
var wlsec_explain = ' This page allows you setup wireless security. Using WEP or WPA Encryption Keys will help prevent unauthorized access to your wireless network.';
var wlsec_select_ssid = 'Select SSID:';
var wlsec_psk= 'PSK';
var wlsec_pre_shared = 'Pre-Shared Key';
var wlsec_tkip = 'TKIP';
var wlsec_aes = 'AES';
var wlsec_apply = 'Apply Changes';
var wlsec_reset = 'Reset';

var wlsec_save = 'Save';
var wlsec_apply = 'Save & Apply';


var wlsec_inside_type_alert = "This inside type is not supported.";
var wlsec_eap_alert = 'This EAP type is not supported.';
var wlsec_wapi_remote_ca_install_alert = "Please make sure that wapi certs from the remote AS have already been installed on webpage [WAPI] -> [Certificate Install].";
var wlsec_wapi_local_ca_install_alert = "Please make sure that wapi certs from the local AS have already been installed at webpage [WAPI] -> [Certificate Install].";
var wlsec_wapi_wrong_select_alert = "Wrong wapi cert selected index.";
var wlsec_wapi_key_length_alert = "the wapi key should be at least 8 characters and no more than 32 characters";
var wlsec_wapi_key_hex_alert = "The Hex Mode WAPI key length should be 64 Hex number";
var wlsec_wapi_invalid_key_alert = "Invalid key value. It should be in hex numbers (0-9 or a-f).";
var wlsec_wep_confirm = "if WEP is turned on, WPS2.0 will be disabled";
var wlsec_wpa_confirm = "under WPA only or TKIP only, WPS2 daemon will be disabled";
var wlsec_wpa2_empty_alert = "WPA2 Cipher Suite Cannot be empty.";
var wlsec_wpa_empty_alert = "WPA Cipher Suite Cannot be empty.";
var wlsec_tkip_confirm = "if TKIP only; WPS2.0 will be disabled";
var wlsec_encryption =  'Encryption:';
var wlsec_disabled = 'Disabled';
var wlsec_wpa_mix = 'WPA-Mixed';
var wlsec_802_1x = '802.1x Authentication:';
var wlsec_auth = 'Authentication:';
var wlsec_auth_open_sys = 'Open System';
var wlsec_auth_shared_key = 'Shared Key';
var wlsec_auth_auto = 'Auto';
var wlsec_key_length = 'Key Length:';
var wlsec_key_hex = 'HEX';
var wlsec_key_ascii = 'ASCII';
var wlsec_encryption_key = 'Encryption Key:';
var wlsec_auth_mode = 'Authentication Mode:';
var wlsec_auth_enterprise_mode = 'Enterprise (RADIUS)';
var wlsec_auth_enterprise_ap_mode = 'Enterprise (AS Server)';
var wlsec_auth_personal_mode = 'Personal (Pre-Shared Key)';
var wlsec_wpa_suite = 'WPA Cipher Suite:';
var wlsec_wpa2_suite = 'WPA2 Cipher Suite:';
var wlsec_wep_key_format = 'Key Format:';
var wlsec_pre_key_format = 'Pre-Shared Key Format:';
var wlsec_pre_key = 'Pre-Shared Key:';
var wlsec_passpharse = 'Passphrase';
var wlsec_key_hex64 = 'HEX (64 characters)';
var wlsec_key_64bit = '64 Bits';
var wlsec_key_128bit = '128 Bits';
var wlsec_radius_server_ip = "RADIUS&nbsp;Server&nbsp;IP&nbsp;Address:";
var wlsec_radius_server_port = 'RADIUS&nbsp;Server&nbsp;Port:';
var wlsec_radius_server_password = 'RADIUS&nbsp;Server&nbsp;Password:';
var wlsec_eap_type = 'EAP Type:';
var wlsec_inside_tunnel_type = 'Inside Tunnel Type:';
var wlsec_eap_user_id = 'EAP User ID:';
var wlsec_radius_user = 'RADIUS User Name:';
var wlsec_radius_user_password = 'RADIUS User Password:';
var wlsec_user_key_password = 'User Key Password (if any):';
var wlsec_use_local_as = 'Use Local AS Server:';
var wlsec_as_ip = 'AS&nbsp;Server&nbsp;IP&nbsp;Address:';
var wlsec_select_wapi_ca = 'Select WAPI certificate:';
var wlsec_use_ca_from_as = 'Use Cert from Remote AS';
var wlsec_adhoc_wep = 'Adhoc mode don\'t support WEP encryption with N band or AC band!';

/***********	tcpipwan.htm  tcpiplan.htm************/
var tcpip_reservedip_alert='Invalid IP address value!\nIt should not be reserved IP address(127.x.x.x).';
var tcpip_check_ip_msg = 'Invalid IP address';
var tcpip_check_server_ip_msg = 'Invalid server IP address';
var tcpip_check_dns_ip_msg1 = 'Invalid DNS1 address';
var tcpip_check_dns_ip_msg2 = 'Invalid DNS2 address';
var tcpip_check_dns_ip_msg3 = 'Invalid DNS3 address';
var tcpip_check_size_msg = "Invalid MTU size! You should set a value between";
var tcpip_check_dns_doamin = "Domain name should not be empty!";
var tcpip_domain_name_invalid = "Domain name Invalid!";
var tcpip_check_user_name_msg = "user name cannot be empty!";
var tcpip_check_password_msg = "password cannot be empty";
var tcpip_check_invalid_time_msg = "Invalid idle time value! You should set a value between";
var tcpip_pppoecontype_alert = "wrong pppoeConnType";
var tcpip_pptpontype_alert = "wrong pptpConnType";
var tcpip_l2tpcontype_alert = "wrong l2tpConnType";
var tcpip_pppcontype_alert = "wrong pppConnType";
var tcpip_browser_alert = 'Error! Your browser must have CSS support!';
var tcpip_wan_header = 'WAN Interface Setup';
var tcpip_wan_explain = 'This page is used to configure the parameters for Internet network which connects to the WAN port of your Access Point. Here you may change the access method to static IP, DHCP, PPPoE, PPTP or L2TP by click the item value of WAN Access type.';
var tcpip_wan_auto_dns = 'Attain DNS Automatically';
var tcpip_wan_manually_dns =  'Set DNS Manually';
var tcpip_wan_conn_time = '&nbsp;(1-1000 minutes)';
var tcpip_wan_max_mtu_size = 'bytes';
var tcpip_wan_conn = 'Connect';
var tcpip_wan_disconn = 'Disconnect';
var tcpip_wan_continuous = 'Continuous';
var tcpip_wan_on_demand = 'Connect on Demand';
var tcpip_wan_manual = 'Manual';
var tcpip_wan_access_type = 'WAN Access Type:';
var tcpip_wan_type_static_ip = 'Static IP';
var tcpip_wan_type_client = "DHCP Client";
var tcpip_wan_type_pppoe_henan = "PPPoE+(henan CNCMAX broadband)";
var tcpip_wan_type_pppoe_nanchang = "Dynamic PPPoE(nanchang ChinaNetClient)";
var tcpip_wan_type_pppoe_other1 = "PPPoE_other1(hunan and hubei ChinaNetClient)";
var tcpip_wan_type_pppoe_other2 = "PPPoE_other2(hunan and hubei ChinaNetClient)";
var tcpip_wan_type_dhcp_plus = "DHCP+(henan region)";
var tcpip_wan_ip = "IP Address:";
var tcpip_wan_mask = 'Subnet Mask:';
var tcpip_wan_default_gateway = 'Default Gateway:';
var tcpip_wan_mtu_size = 'MTU Size:';
var tcpip_wan_host = 'Host Name:';
var tcpip_wan_user = 'User Name:';
var tcpip_wan_password = 'Password:';
var tcpip_wan_server_ac = 'Service Name(AC):';
var tcpip_wan_conn_type = 'Connection Type:';
var tcpip_wan_idle_time = 'Idle Time:';
var tcpip_wan_server_ip = 'Server IP Address:';
var tcpip_wan_clone_mac = 'Clone MAC Address:';
var tcpip_lan_clone_mac_error = 'Error! Invalid MAC address.';
var tcpip_lan_host_error = 'Error! Comment is too long. ';
var tcpip_wan_clone_mac_error = 'Error! Invalid MAC address.';
var tcpip_entry_table_full ='Cannot add new entry because table is full!';
var tcpip_entry_ip_error = 'Cannot add new entry because the ip is not the same subnet as LAN network!';
var tcpip_ip_setted = 'This IP address has been setted!';
var tcpip_mac_setted = 'This MAC address has been setted!';
var tcpip_port_setted = 'This Port number has been setted!';
var tcpip_ip_error = 'Invalid ip address! It must be in the lan dhcp server ip range, and is not same with router\'s ip!';
var tcpip_ip_mac_error ='The IP and MAC address must be not null!';
var tcpip_wan_clonemac = 'Clone MAC';
var tcpip_wan_restore_defmac = 'Restore Default MAC';

var tcpip_wan_enable_tr069_dhcp = '&nbsp;&nbsp;Enable DHCP For TR069';
var tcpip_wan_enable_upnp = '&nbsp;&nbsp;Enable uPNP';
var tcpip_wan_enable_igmp_proxy = '&nbsp;&nbsp;Enable IGMP Proxy';
var tcpip_wan_enable_ping = '&nbsp;&nbsp;Enable Ping Access on WAN';
var tcpip_wan_enable_webserver = '&nbsp;&nbsp;Enable Web Server Access on WAN';
var tcpip_wan_web_port = 'Web Accessed port:';

var tcpip_wan_enable_ipsec = '&nbsp;&nbsp;Enable IPsec pass through on VPN connection';
var tcpip_wan_enable_pptp = '&nbsp;&nbsp;Enable PPTP pass through on VPN connection';
var tcpip_wan_enable_l2tp = '&nbsp;&nbsp;Enable L2TP pass through on VPN connection';
var tcpip_wan_enable_ipv6 = '&nbsp;&nbsp;Enable IPv6 pass through on VPN connection';
var tcpip_wan_enable_8021x ='&nbsp;&nbsp;Enable ethernet 802.1x on WAN';
var tcpip_wan_enable_netsniper = '&nbsp;&nbsp;Enable bypass netsniper';
var tcpip_wan_eap_type = '&nbsp;&nbsp;&nbsp;EAP Type:';
var tcpip_wan_inside_tunnel_type = '&nbsp;&nbsp;&nbsp;Inside Tunnel Type:';
var tcpip_wan_eap_ttls_type ='&nbsp;&nbsp;&nbsp;EAP TTLS Phase2 Type:';

var tcpip_wan_eap_userid = 'EAP User ID:';
var tcpip_wan_radius_name = 'RADIUS User Name:';
var tcpip_wan_radius_passwd = 'RADIUS User Password';
var tcpip_wan_user_keypasswd = 'User Key Password (if any):';


var tcpip_wan_dynamic_ip = 'Dynamic IP (DHCP)';
var tcpip_wan_static_ip = 'Static IP';
var tcpip_wan_attain_by_DN = 'Attain Server By Domain Name';
var tcpip_wan_attain_by_Ip = 'Attain Server By Ip Address';
var tcpip_wan_port_empty ='Port cannot be empty! You should set a value between 1-65535.';
var tcpip_wan_port_invalid = 'Invalid port number! It should be the decimal number (0-9).';
var tcpip_wan_port_invalid_num = 'Invalid port number! You should set a value between 1-65535.';
var tcpip_wan_pptp_alert1 = 'PPTP user name cannot be empty!';
var tcpip_wan_pptp_alert2 = 'PPTP password cannot be empty!';
var tcpip_wan_l2tp_alert1 ='L2TP user name cannot be empty!';
var tcpip_wan_l2tp_alert2 ='L2TP password cannot be empty!';
var tcpip_wan_ppp_alert1 = 'PPP user name cannot be empty!';
var tcpip_wan_ppp_alert2 = 'PPP password cannot be empty!';
var tcpip_wan_gwip_alert = 'Invalid pptp default gateway IP address!\nIt should be located in the same subnet of local IP address.';
var tcpip_wan_inside_nosupport = 'This inside type is not supported.';
var tcpip_wan_phase2_nosupport = 'This phase2 type is not supported.';
var tcpip_wan_eap_nosupport = 'This EAP type is not supported.';

var tcpip_wan_con_su = 'Connected to server successfully.\n';
var tcpip_wan_con_fail = 'Connect to server failed!\n';
var tcpip_wan_ppp_discon = 'PPPoE disconnected.\n';
var tcpip_wan_pptp_discon = 'PPTP disconnected.\n';
var tcpip_wan_l2tp_discon = 'L2TP disconnected.\n';
var tcpip_wan_usb3g_discon = 'USB3G disconnected.\n';
var tcpip_wan_unkonwn = 'Unknown\n';
var tcpip_wan_invalid_ip = 'Invalid IP-address value!';
var tcpip_wan_invalid_subnet = 'Invalid subnet-mask value!';
var tcpip_wan_invalid_gw = 'Invalid default-gateway value!';
var tcpip_wan_invalid_host = 'Invalid Host Name! Please enter characters in A(a)~Z(z) or 0-9 without spacing.';
var tcpip_wan_igmpproxy = 'Nat is disabled, igmpproxy can not be enabled';
var tcpip_wan_invalid_port = 'Error! Invalid value of port.';
var tcpip_wan_set_fail = 'set wan interface item value fail!';

var tcpip_lan_invalid_dhcp_value = 'Invalid DHCP value!';
var tcpip_lan_invalid_ip_value = 'Invalid IP-address value!';
var tcpip_lan_invalid_subnet_value = 'Invalid subnet-mask value!';
var tcpip_lan_invalid_defgw_value = 'Invalid default-gateway value!';
var tcpip_lan_invalid_dhcpstart = 'Invalid DHCP client start address!';
var tcpip_lan_invalid_dhcpend = 'Invalid DHCP client end address!';
var tcpip_lan_invalid_dhcprange = 'Invalid DHCP client range!';
var tcpip_lan_invalid_dns_value = 'Invalid DNS address value!';
var tcpip_lan_invalid_domain = 'Invalid Domain Name! Please enter characters in A(a)~Z(z) or 0-9 without spacing.';
var tcpip_check_dhcp = 'Please check your \"Static DHCP\" setting. The static IP address must be in the range of dhcpd ip pool, and is not same with router\'s ip!';
var tcpip_dhcp_error = 'DHCP Client Range can\'t cover Static DHCP List!!!';
var tcpip_change = '<h4>Change setting successfully!<BR><BR>Do not turn off or reboot the Device during this time.</h4>';
var tcpip_reboot = 'reboot check fail!\n';
var tcpip_wan_invalid_dns_mode = 'Invalid DNS mode value!';
var tcpip_wan_invalid_ppp_type = 'Invalid PPP type value!';
var tcpip_wan_invalid_pptp_type = 'Invalid PPTP wan IP type!';
var tcpip_wan_invalid_defgw = 'Invalid pptp default gateway value!';
var tcpip_wan_invalid_pptp_server = 'Invalid pptp server ip value!';
var tcpip_wan_invalid_pptp_type ='Invalid PPTP type value!';
var tcpip_wan_invalid_l2tp_type = 'Invalid L2TP wan IP type!';
var tcpip_wan_invalid_l2tp_defgw ='Invalid l2tp default gateway value!';
var tcpip_wan_invalid_l2tp_server = 'Invalid l2tp server ip value!';
var tcpip_wan_invalid_l2tp = 'Invalid L2TP type value!';
var tcpip_wan_invalid_usb = 'Invalid USB3G type value!';
var tcpip_wan_invalid_ds = 'Invalid ds-lite mode value!';
var tcpip_wan_invalid_aftr ='Invalid AFTR address value!';
var tcpip_wan_invalid_ip_mode = 'Invalid IP mode value!';
var tcpip_wan_invalid_8021x = 'Invalid 802.1x EAP type value!';
var tcpip_wan_8021x = 'No 802.1x EAP type!';
var tcpip_wan_8021x_user = 'No 802.1x EAP User ID!';
var tcpip_wan_8021x_user2 = 'No 802.1x RADIUS User Name!';
var tcpip_wan_8021x_user3 = 'No 802.1x RADIUS User Password!';
var tcpip_wan_8021x_tunnel = 'Invalid 802.1x inside tunnel type value!';
var tcpip_wan_8021x_tunnel2 = 'No 802.1x inside tunnel type!';
var tcpip_wan_8021x_tunnel3 = '802.1x inside tunnel type not support!';
var tcpip_wan_8021x_phase = 'Invalid 802.1x phase2 type value!';
var tcpip_wan_8021x_phase2 ='No 802.1x phase 2 type!';
var tcpip_wan_eap_long = 'EAP user ID too long!';
var tcpip_wan_ttls_phase2 = '802.1x ttls phase2 type not support!';
var tcpip_wan_eap_nsupport = '802.1x EAP type not support!';
var tcpip_wan_upnp = 'Upnp Only can run on one WAN interface\n';

var tcpip_lan_save = 'Save';

var tcpip_apply = 'Save & Apply';
var tcpip_reset = 'Reset';


var tcpip_lan_wrong_dhcp_field = "Wrong dhcp field!";
var tcpip_lan_start_ip = 'Invalid DHCP client start address!';
var tcpip_lan_end_ip = 'Invalid DHCP client End address!';
var tcpip_lan_ip_alert = '\nIt should be located in the same subnet of the current IP address.';
var tcpip_lan_invalid_rang = 'Invalid DHCP client address range!\nEnding address should be greater than starting address.\n And limit ip pool size to 512';
var tcpip_lan_invalid_gw_ip = 'Invalid gateway address!'; 
var tcpip_lan_macaddr_nocomplete = 'Input MAC address is not complete. It should be 12 digits in hex.';
var tcpip_lan_macaddr_invlaid_value1 ='Invalid MAC address. It should not be ff:ff:ff:ff:ff:ff.';
var tcpip_lan_macaddr_invlaid_value2 ='Invalid MAC address. It should not be multicast mac address.';
var tcpip_lan_macaddr_invlaid_value3 = 'Invalid MAC address. It should not be 00:00:00:00:00:00.';
var tcpip_lan_macaddr_invlaid_value4 = 'Invalid MAC address. It should not be multicast mac address between 01:00:5e:00:00:00 and 01:00:5e:7f:ff:ff.';

var tcpip_lan_macaddr_invalid = 'Invalid MAC address. It should be in hex numbers (0-9 or a-f).';
var tcpip_lan_invalid_dhcp_lease_time = 'dhcp lease time invalid!';
var tcpip_lan_invalid_domain_name = 'Invalid Domain Name! Length of Domain Name shoule not more than 63';
var tcpip_lan_invalid_rang_value = "Invalid value. It should be in range (1 ~ 10080).";
var tcpip_lan_invalid_dhcp_type = "Load invalid dhcp type!";
var tcpip_lan_header = 'LAN Interface Setup';
var tcpip_lan_explain = ' This page is used to configure the parameters for the local area network that connects to the LAN port of your Access Point. Here you may change the settings for IP addresss, subnet mask, DHCP, etc..';
var tcpip_lan_ip = "IP Address:";
var tcpip_lan_mask = 'Subnet Mask:';
var tcpip_lan_default_gateway = 'Default Gateway:';
var tcpip_lan_dhcp = 'DHCP:';
var tcpip_lan_dhcp_disabled = 'Disabled';
var tcpip_lan_dhcp_client = 'Client';
var tcpip_lan_dhcp_server = 'Server';
var tcpip_lan_dhcp_relay = 'Relay';
var tcpip_lan_dhcp_auto = 'Auto';
var relay_dhcp_server_ip = 'DHCP Server IP:';
var relay_access_server_wan_idx = 'WAN Index:';
var tcpip_lan_dhcp_rang = 'DHCP Client Range:';
var tcpip_lan_dhcp_time = 'DHCP Lease Time:';
var tcpip_minutes = 'minutes';
var tcpip_lan_staicdhcp = 'Static DHCP:';
var tcpip_staticdhcp = "Set Static DHCP";
var tcpip_domain = "Domain Name:";
var tcpip_netbios = "NetBIOS Name:";
var tcpip_802_1d = "802.1d Spanning Tree:";
var tcpip_802_1d_enable = 'Enabled';
var tcpip_802_1d_disabled = 'Disabled';
var tcpip_show_client = 'Show Client';
var tcpip_lan_invalid_lan_ipv6 = 'LAN address invalid!';
var tcpip_lan_invalid_ula_ipv6 = 'ULA address invalid!';
var tcpip_lan_invalid_dns = 'dns address invalid!';
var tcpip_lan_invalid_start_ip = 'start address invalid!';
var tcpip_lan_invalid_end_ip = 'end address invalid!';
var tcpip_lan_invalid_first_prefix = 'First prefix address invalid!';
var tcpip_lan_invalid_second_prefix = 'Second prefix address invalid!';
var tcpip_lan_wac_if = 'WAC Interface:';

var tcpip_lan_ipv6_config = 'Configuring IPv6 LAN setting'; 
var tcpip_lan_ipv6_auto_config = 'Config Ipv6 LAN Automatically';
var tcpip_lan_ipv6_manu_config = 'Config Ipv6 LAN Manually';
var tcpip_lan_prefix = 'Prefix Length';
var tcpip_lan_ula_config = '<b>Configuring ULA setting</b> <BR>  <BR>';
var tcpip_lan_enable_ula = 'Enable ULA';
var tcpip_lan_manu_ula = 'Set ULA Manually';
var tcpip_lan_auto_ula = 'Set ULA Automatically';
var tcpip_lan_dhcpv6_config = 'Configuring DHCPv6 Server'; 
var tcpip_lan_enable = 'Enable:';
var tcpip_lan_dns = 'DNS Addr:';
var tcpip_lan_if = 'Interface Name:';
var tcpip_lan_pool = 'Addrs Pool:';
var tcpip_lan_addr_prefix = 'Addrs Prefix:';
var tcpip_lan_manu = 'Manually';
var tcpip_lan_pd = 'Prefix Delegation';
var tcpip_lan_ula_prefix = 'ULA Prefix';
var tcpip_lan_from = 'From:';
var tcpip_lan_to = 'To:';
var tcpip_lan_router_ad = 'Configuring Router Advertisement';
var tcpip_lan_router_ad_name = 'radvdinterfacename:';
var tcpip_lan_max_router_ad_interval = 'MaxRtrAdvInterval:';
var tcpip_lan_min_router_ad_interval = 'MinRtrAdvInterval:';
var tcpip_lan_min_delay = 'MinDelayBetweenRAs:';
var tcpip_lan_ad_flag = 'AdvManagedFlag:';
var tcpip_lan_ad_other_flag = 'AdvOtherConfigFlag:';
var tcpip_lan_ad_mtu = 'AdvLinkMTU:';
var tcpip_lan_ad_time = 'AdvReachableTime:';
var tcpip_lan_ad_timer = 'AdvRetransTimer:';
var tcpip_lan_ad_cur_hop = 'AdvCurHopLimit:';
var tcpip_lan_ad_def_lifetime = 'AdvDefaultLifetime:';
var tcpip_lan_ad_def_prefer = 'AdvDefaultPreference:';
var tcpip_lan_ad_src_lladdr = 'AdvSourceLLAddress:';
var tcpip_lan_unicast = 'UnicastOnly:';
var tcpip_lan_config_prefix1 = 'Configure Prefix1';
var tcpip_lan_prefix_mode = 'prefix mode:';
var tcpip_lan_pre = 'Prefix:';
var tcpip_lan_ad_linkflag = 'AdvOnLinkFlag:';
var tcpip_lan_auto_flag = 'AdvAutonomousFlag:';
var tcpip_lan_valid_lifetime = 'AdvValidLifetime:';
var tcpip_lan_prefer_lifetime = 'AdvPreferredLifetime:';
var tcpip_lan_router_addr = 'AdvRouterAddr:';
var tcpip_lan_config_prefix2 = 'Configure Prefix2';


var tcpip_l2tp_server_domain_name="Invalid Domain Name! Please enter characters in A(a)~Z(z) or 0-9 without spacing.";
var tcpip_l2tp_server_domain_name2='Invalid Domain Name! Domain name should start with a letter, end with a letter or digit.';

/***********	dhcptbl.htm ************/
var dhcp_header = 'Active DHCP Client Table';
var dhcp_explain = ' This table shows the assigned IP address, MAC address and time expired for each DHCP leased client.';
var dhcp_devname = 'Device Name';
var dhcp_ip = 'IP Address';
var dhcp_mac = 'MAC Address';
var dhcp_time = 'Time Expired(s)';
var dhcp_refresh = 'Refresh';
var dhcp_close = 'Close';
var dhcp_none = 'None';
var dhcp_always = 'Always';


/***********	util_gw.htm ************/
var util_gw_wps_warn1 = 'The SSID has been configured by WPS. Any change of the setting ';
var util_gw_wps_warn2 = 'AP Mode has been configured by WPS. Any change of the setting ';
var util_gw_wps_warn3 = 'The security setting has been configured by WPS. Any change of the setting ' ;
var util_gw_wps_warn4 = 'The WPA Enterprise Authentication cannot be supported by WPS. ';
var util_gw_wps_warn5 = 'The 802.1x Authentication cannot be supported by WPS. ';
var util_gw_wps_warn6 = 'WDS mode cannot be supported by WPS. ';
var util_gw_wps_warn7 = 'Adhoc Client mode cannot be supported by WPS. ';
var util_gw_wps_cause_disconn = 'May cause stations to be disconnected. ';
var util_gw_wps_want_to = 'Are you sure you want to continue with the new setting?';
var util_gw_wps_cause_disabled = 'Using this configuration will cause WPS to be disabled. ';
var util_gw_wps_ecrypt_11n = 'Invalid Encryption Mode! WPA or WPA2, Cipher suite AES should be used for 802.11n band.';
var util_gw_wps_ecrypt_basic = 'The Encryption Mode is not suitable for the 802.11n band, please modify wlan encrypt settings or it will not function properly.';
var util_gw_wps_ecrypt_confirm = 'Are you sure you want to continue with this encryption mode for the 11n band? This may cause some  performance loss!';
var util_gw_ssid_hidden_alert = "if hiddenSSID is turned on; WPS2.0 will be disabled";
var util_gw_ssid_empty = 'SSID cannot be empty';
var util_gw_preshared_key_length_alert =  'Pre-Shared Key value should be 64 characters.';
var util_gw_preshared_key_alert = "Invalid Pre-Shared Key value. It should be in hex number (0-9 or a-f).";
var util_gw_preshared_key_min_alert = 'Pre-Shared Key value should be set at least 8 characters.';
var util_gw_preshared_key_max_alert = 'Pre-Shared Key value should be less than 64 characters.';
var util_gw_decimal_rang = 'It should be a decimal number between 1-65535.';
var util_gw_invalid_radius_port = 'Invalid port number of RADIUS Server! ';
var util_gw_empty_radius_port = "RADIUS Server port number cannot be empty! ";
var util_gw_invalid_radius_ip = 'Invalid RADIUS Server IP address';
var util_gw_mask_empty = 'Subnet mask cannot be empty! ';
var util_gw_ip_format = 'It should be filled with four 3-digit numbers, i.e., xxx.xxx.xxx.xxx.';
var util_gw_mask_error = 'Subnet mask cannot be 0.0.0.0!';
var util_gw_mask_rang = '\nIt should be the number of 0, 128, 192, 224, 240, 248, 252 or 254';
var util_gw_mask_rang1 = '\nIt should be the number of 128, 192, 224, 240, 248, 252 or 254';
var util_gw_mask_invalid1 = 'Invalid subnet mask in 1st digit.';
var util_gw_mask_invalid2 = 'Invalid subnet mask in 2nd digit.';
var util_gw_mask_invalid3 = 'Invalid subnet mask in 3rd digit.';
var util_gw_mask_invalid4 = 'Invalid subnet mask in 4th digit.';
var util_gw_mask_invalid = 'Invalid subnet mask value. ';
var util_gw_decimal_value_rang = "It should be the decimal number (0-9).";
var util_gw_invalid_degw_ip = 'Invalid default gateway address';
var util_gw_invalid_gw_ip = 'Invalid gateway address!';
var util_gw_locat_subnet = '\nIt should be located in the same subnet of current IP address.';
var util_gw_mac_complete = 'Input MAC address is not complete. ';
var util_gw_mac_empty = 'Input MAC address is empty! ';
var util_gw_mac_zero = 'Input MAC address can not be all zero! ';
var util_gw_mac_ff = 'Input MAC address can not be all FF! ';
var util_gw_12hex = 'It should be 12 digits in hex.';
var util_gw_invalid_mac = 'Invalid MAC address. ';
var util_gw_ff = 'It should not be ff:ff:ff:ff:ff:ff.';
var util_gw_multicast = 'It should not be multicast mac address';
var util_gw_hex_rang = 'It should be in hex number (0-9 or a-f).';
var util_gw_ip_empty = 'IP address cannot be empty! ';
var util_gw_invalid_value = ' value. ';
var util_gw_should_be = 'It should be ';
var util_gw_check_ppp_rang1 = ' range in 1st digit. ';
var util_gw_check_ppp_rang2 = ' range in 2nd digit. ';
var util_gw_check_ppp_rang3 = ' range in 3rd digit. ';
var util_gw_check_ppp_rang4 = ' range in 4th digit. ';
var util_gw_check_ppp_rang5 = ' range in 5th digit. ';
var util_gw_invalid_key_length = 'Invalid length of Key ';
var util_gw_char = ' characters.';
var util_gw_invalid_wep_key_value = 'Invalid length of WEP Key value. ';
var util_gw_invalid_key_value = 'Invalid key value. ';
var util_gw_invalid_ip = 'Invalid IP address';
var util_gw_check_server_ip_msg = 'Invalid server IP address';
var util_gw_wan_gwip_alert = 'Invalid pptp default gateway IP address!\nIt should be located in the same subnet of local IP address.';
var util_gw_wan_gwip_alert2 ='Invalid l2tp default gateway IP address!\nIt should be located in the same subnet of local IP address.';
var util_gw_check_dns_ip_msg1 = 'Invalid DNS1 address';
var util_gw_check_dns_ip_msg2 = 'Invalid DNS2 address';
var util_gw_check_dns_ip_msg3 = 'Invalid DNS3 address';
var util_gw_check_user_name_msg = "user name cannot be empty!";
var util_gw_check_password_msg = "password cannot be empty";
var util_gw_ipaddr_empty = 'IP address cannot be empty! It should be filled with four 3-digit numbers, i.e., xxx.xxx.xxx.xxx.';
var util_gw_ipaddr_nodecimal = ' value. It should be decimal numbers (0-9).';
var util_gw_ipaddr_1strange1 =  ' range in 1st digit. It should be 1-223.';
var util_gw_ipaddr_1strange2 = ' range in 1st digit. It should not be 127.';
var util_gw_ipaddr_1strange3 = ' range in 1nd digit. It should be 0-255.';
var util_gw_ipaddr_1strange = ' range in 1st digit. It should be 1-223 except 127.';
var util_gw_ipaddr_1range=' range in 1st digit. It should be 1-223.';
var util_gw_ipaddr_1range_for127=' range in 1st digit. It should not be 127.';
var util_gw_ipaddr_2ndrange = ' range in 2nd digit. It should be 0-255.';
var util_gw_ipaddr_3rdrange = ' range in 3rd digit. It should be 0-255.';
var util_gw_ipaddr_4thrange = ' range in 4th digit. It should be 1-254.';
var util_gw_ipaddr_5thrange = ' range in 5th digit. It should be 1-32.';
var util_gw_tx_restrict='tx_restrict cannot be empty!';
var util_gw_tx_restrict_limit='tx_restrict value between 0 to 1000!';
var util_gw_rx_restrict='rx_restrict cannot be empty!';
var util_gw_rx_restrict_limit='rx_restrict value between 0 to 1000!';

var util_gw_ipv6_invalid = ' ipv6 address invalid!';
var util_gw_ipv6_prefix = ' ipv6 prefix must between 0-128!';
var util_gw_mustbetween = ' must between ';
var util_gw_field = 'The field also Should be set';


var util_gw_array0 = "(GMT-12:00)Eniwetok, Kwajalein";
var util_gw_array1 = "(GMT-11:00)Midway Island, Samoa";
var util_gw_array2 = "(GMT-10:00)Hawaii";
var util_gw_array3 = "(GMT-09:00)Alaska";
var util_gw_array4 = "(GMT-08:00)Pacific Time (US & Canada); Tijuana";
var util_gw_array5 = "(GMT-07:00)Arizona";
var util_gw_array6 = "(GMT-07:00)Mountain Time (US & Canada)";
var util_gw_array7 = "(GMT-06:00)Central Time (US & Canada)";
var util_gw_array8 = "(GMT-06:00)Mexico City, Tegucigalpa";
var util_gw_array9 = "(GMT-06:00)Saskatchewan";
var util_gw_array10 = "(GMT-05:00)Bogota, Lima, Quito";
var util_gw_array11 = "(GMT-05:00)Eastern Time (US & Canada)";
var util_gw_array12 = "(GMT-05:00)Indiana (East)";
var util_gw_array13 = "(GMT-04:00)Atlantic Time (Canada)";
var util_gw_array14 = "(GMT-04:00)Caracas, La Paz";
var util_gw_array15 = "(GMT-04:00)Santiago";
var util_gw_array16 = "(GMT-03:30)Newfoundland";
var util_gw_array17 = "(GMT-03:00)Brasilia";
var util_gw_array18 = "(GMT-03:00)Buenos Aires, Georgetown";
var util_gw_array19 = "(GMT-02:00)Mid-Atlantic";
var util_gw_array20 = "(GMT-01:00)Azores, Cape Verde Is.";
var util_gw_array21 = "(GMT)Casablanca, Monrovia";
var util_gw_array22 = "(GMT)Greenwich Mean Time: Dublin, Edinburgh, Lisbon, London";
var util_gw_array23 = "(GMT+01:00)Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna";
var util_gw_array24 = "(GMT+01:00)Belgrade, Bratislava, Budapest, Ljubljana, Prague";
var util_gw_array25 = "(GMT+01:00)Barcelona, Madrid";
var util_gw_array26 = "(GMT+01:00)Brussels, Copenhagen, Madrid, Paris, Vilnius";
var util_gw_array27 = "(GMT+01:00)Paris";
var util_gw_array28 = "(GMT+01:00)Sarajevo, Skopje, Sofija, Warsaw, Zagreb";
var util_gw_array29 = "(GMT+02:00)Athens, Istanbul, Minsk";
var util_gw_array30 = "(GMT+02:00)Bucharest";
var util_gw_array31 = "(GMT+02:00)Cairo";
var util_gw_array32 = "(GMT+02:00)Harare, Pretoria";
var util_gw_array33 = "(GMT+02:00)Helsinki, Riga, Tallinn";
var util_gw_array34 = "(GMT+02:00)Jerusalem";
var util_gw_array35 = "(GMT+03:00)Baghdad, Kuwait, Riyadh";
var util_gw_array36 = "(GMT+03:00)Moscow, St. Petersburg, Volgograd";
var util_gw_array37 = "(GMT+03:00)Mairobi";
var util_gw_array38 = "(GMT+03:30)Tehran";
var util_gw_array39 = "(GMT+04:00)Abu Dhabi, Muscat";
var util_gw_array40 = "(GMT+04:00)Baku, Tbilisi";
var util_gw_array41 = "(GMT+04:30)Kabul";
var util_gw_array42 = "(GMT+05:00)Ekaterinburg";
var util_gw_array43 = "(GMT+05:00)Islamabad, Karachi, Tashkent";
var util_gw_array44 = "(GMT+05:30)Bombay, Calcutta, Madras, New Delhi";
var util_gw_array45 = "(GMT+06:00)Astana, Almaty, Dhaka";
var util_gw_array46 = "(GMT+06:00)Colombo";
var util_gw_array47 = "(GMT+07:00)Bangkok, Hanoi, Jakarta";
var util_gw_array48 = "(GMT+08:00)Beijing, Chongqing, Hong Kong, Urumqi";
var util_gw_array49 = "(GMT+08:00)Perth";
var util_gw_array50 = "(GMT+08:00)Singapore";
var util_gw_array51 = "(GMT+08:00)Taipei";
var util_gw_array52 = "(GMT+09:00)Osaka, Sapporo, Tokyo";
var util_gw_array53 = "(GMT+09:00)Seoul";
var util_gw_array54 = "(GMT+09:00)Yakutsk";
var util_gw_array55 = "(GMT+09:30)Adelaide";
var util_gw_array56 = "(GMT+09:30)Darwin";
var util_gw_array57 = "(GMT+10:00)Brisbane";
var util_gw_array58 = "(GMT+10:00)Canberra, Melbourne, Sydney";
var util_gw_array59 = "(GMT+10:00)Guam, Port Moresby";
var util_gw_array60 = "(GMT+10:00)Hobart";
var util_gw_array61 = "(GMT+10:00)Vladivostok";
var util_gw_array62 = "(GMT+11:00)Magadan, Solomon Is., New Caledonia";
var util_gw_array63 = "(GMT+12:00)Auckland, Wllington";
var util_gw_array64 = "(GMT+12:00)Fiji, Kamchatka, Marshall Is.";

var util_gw_chanauto = 'Auto';
var uyi_gw_chan_dfsauto = 'Auto(DFS)';
var util_gw_bcast_mcast_mac = 'It should not be a broadcast or multicast mac address!';
var util_invalid_mac_addr_multicast='Invalid MAC address. It should not be multicast mac address between 01:00:5e:00:00:00 and 01:00:5e:7f:ff:ff.';

var util_save_wan1 = 'WAN IP address and LAN IP address cannot in the same net section.';
var util_save_wan2 = 'wan static ip address can not equal to gateway ip address.';
var util_save_wan3 = 'Invalid DNS address';
var util_save_mtu = 'MTU Size';
var util_save_wan4 = 'Invalid Host Name! Length of Domain Name shoule not more than 63';
var util_save_wan5 = 'Invalid Host Name! Please enter characters in A(a)~Z(z) or 0-9 or '-' without spacing.';
var util_save_wan6 = 'Invalid Host Name! Host Name should begin with letter,end with letter or digits.';
var util_save_wan7 = 'PPP user name cannot be empty!';
var util_save_wan8 = 'PPP password cannot be empty!';
var util_save_wan9 = 'You cannot input nonnumeric character!';
var util_save_wan10 = 'Invalid idle time value! You should set a value between 1-1000.';
var util_save_wan11 = 'Invalid ip input';
var util_save_wan12 = '2 PPP user name cannot be empty!';
var util_save_wan13 = '2 PPP password cannot be empty!';
var util_save_wan14 = '3 PPP user name cannot be empty!';
var util_save_wan15 = '3 PPP password cannot be empty!';
var util_save_wan16 = '4 PPP user name cannot be empty!';
var util_save_wan17 = '4 PPP password cannot be empty!';
var util_save_wan18 = 'domain name can not be empty';
var util_save_wan19 = 'APN name cannot be empty!';
var util_save_wan20 = 'Dial number cannot be empty!';
var util_save_wan21 = 'DNS1 address cannot be empty!';
var util_save_wan22 = 'DNS1 address cannot be 0.0.0.0!';
/***********	status.htm ************/
var status_ip = 'IP Address';
var status_mac = 'MAC Address';
var status_subnet_mask = 'Subnet Mask';
var status_default_gw = 'Default Gataway';
var status_attain_ip = 'Attain IP Protocol';
var status_ipv6_global_ip = 'Global Address';
var status_ipv6_ll = 'LL Address';
var status_ipv6_link = 'Link Type';
var status_ipv6_conn = 'Connection Type';
var status_header = 'Access Point Status';
var status_explain = ' This page shows the current status and some basic settings of the device.';
var status_wan_config = "WAN Configuration";
var status_ipv6_lan = 'LAN IPv6 Configuration';
var status_ipv6_wan = 'WAN IPv6 Configuration';
var status_sys = 'System';
var status_uptime = 'Uptime';
var status_fw_ver = 'Firmware Version';
var status_build_time = 'Build Time';
var status_wl = 'Wireless ';
var status_config = 'Configuration';
var status_client_mode_inf = 'Infrastructure Client';
var status_client_mode_adhoc = 'Ad-hoc Client';
var status_ap = 'AP';
var status_wds = 'WDS';
var status_ap_wds = 'AP+WDS';
var status_mesh = 'MESH';
var status_ap_mesh = 'AP+MESH';
var status_mpp = 'MPP';
var status_ap_mpp = 'AP+MPP';
var status_map = 'MAP';
var status_mp = 'MP';
var status_band = 'Band';
var status_ssid = 'SSID';
var status_channel_num = 'Channel Number';
var status_encrypt = 'Encryption';
var status_bssid = 'BSSID';
var status_assoc_cli = 'Associated Clients';
var status_state = 'State';
var status_vir_ap  = 'Virtual AP ';
var status_repater_config = " Repeater Interface Configuration";
var status_tcpip_config = 'TCP/IP Configuration';
var status_dhcp_server = 'DHCP Server';
var status_disabled = 'Disabled';
var status_enabled = 'Enabled';
var status_auto = 'Auto';
var status_unknown = 'Unknown';
var status_dhcp_get_ip = 'Getting IP from DHCP server...';
var status_conn = 'Connected';
var status_disconn = 'Disconnected';
var status_fixed = 'Fixed IP ';
var status_start = 'Started';
var status_idle = 'Idle';
var status_wait_key = 'Waiting for keys';
var status_scan = 'Scanning';
var status_mode = 'Mode';
var status_access_point = 'Access Point Status';
var status_page_information = 'This page shows the current status and some basic settings of the device.';
var status_version = 'Firmware Version:';
var status_wireless_repater_config = 'Wireless Repeater Interface Configuration';
var status_wireless_config = 'Wireless Configuration';
var status_wan1_config = 'WAN1 Configuration';
var status_wan2_config = 'WAN2 Configuration';
var status_wan3_config = 'WAN3 Configuration';
var status_wan4_config = 'WAN4 Configuration';
var status_wan1_ipv6_config = 'WAN1 IPv6 Configuration';
var status_wan2_ipv6_config = 'WAN2 IPv6 Configuration';
var status_wan3_ipv6_config = 'WAN3 IPv6 Configuration';
var status_wan4_ipv6_config = 'WAN4 IPv6 Configuration';
var status_dns_server = 'DNS server';


var status_current_setting_noeffect = 'Below status shows current settings, but does not take effect.';

/***********	stats.htm ************/
var stats_header = 'Statistics';
var stats_explain = 'This page shows the packet counters for transmission and reception pertaining to wireless and Ethernet networks.';
var stats_lan = 'LAN';
var stats_send = 'Sent Packets';
var stats_recv = 'Received Packets';
var stats_repeater = 'Repeater';
var stats_eth = 'Ethernet';
var stats_wan = 'WAN';
var stats_refresh = 'Refresh';
var stats_wireless_text = 'Wireless';
var stats_ethernet = 'Ethernet';
var stats_virtual_ap = 'Virtual AP ';
var stats_wireless_repeater = 'Wireless Repeater LAN';


/***********	wlwdstbl.htm ************/
var wlwdstbl_header = 'WDS AP Table';
var wlwdstbl_header_for_double='WDS AP Table -wlan'
var wlwdstbl_wlan = "wlan";
var wlwdstbl_explain = 'This table shows the MAC address, transmission, reception packet counters, and state information for each configured WDS AP.';
var wlwdstbl_mac = 'MAC Address';
var wlwdstbl_tx_pkt = 'Tx Packets';
var wlwdstbl_tx_err = 'Tx Errors';
var wlwdstbl_tx_rate = 'Tx Rate (Mbps)';
var wlwdstbl_rx_pkt = 'Rx Packets';
var wlwdstbl_refresh = 'Refresh';
var wlwdstbl_close = 'Close';
/***********	wlwdsenp.htm ************/
var wlwdsenp_hex = 'HEX';
var wlwdsenp_char = 'characters';
var wlwdsenp_header = 'WDS Security Setup';
var wlwdsenp_header_for_double='WDS Security Setup -wlan'
var wlwdsenp_wlan = 'wlan';
var wlwdsenp_explain = 'This page allows you to setup the wireless security for WDS. When enabled, you must make sure each WDS device has adopted the same encryption algorithm and Key.';
var wlwdsenp_wep_key_format = 'WEP Key Format:';
var wlwdsenp_wep_key_ascii_5characters='ASCII (5 characters)'
var wlwdsenp_wep_key_hex_10characters='Hex (10 characters)'
var wlwdsenp_encrypt = 'Encryption:';
var wlwdsenp_wep_key = 'WEP Key:';
var wlwdsenp_prekey_format = 'Pre-Shared Key Format:';
var wlwdsenp_prekey = 'Pre-Shared Key:';
var wlwdsenp_none = 'None';
var wlwdsenp_pass = 'Passphrase';
var wlwdsenp_hex_64characters='Hex (64 characters)'
var wlwdsenp_bits = 'bits';
var wlwdsenp_apply = "Apply Changes";
var wlwdsenp_reset = 'Reset';

var wlwdsenp_save='Save';
var wlwdsenp_save_apply='Save & Apply';


/***********	tcpip_staticdhcp.htm ************/
var tcpip_dhcp_alert1 = 'Invalid IP address value!';
var tcpip_dhcp_alert2 = 'Input MAC address or Port number is invalid. The MAC address should be 12 digits in hex and The Port number should be 1~5.';
var tcpip_dhcp_alert3 = 'Invalid IP address value! It is the same with lan ip!';
var tcpip_dhcp_alert4 = 'Input MAC address is not complete. It should be 12 digits in hex.';
var tcpip_dhcp_alert5 = 'mac address invalid!';

var tcpip_dhcp_del_select = 'Do you really want to delete the selected entry?';
var tcpip_dhcp_del_all = 'Do you really want to delete all entries?';
var tcpip_dhcp_header = 'Static DHCP Setup';
var tcpip_dhcp_explain = 'This page allows you reserve IP addresses and assign the same IP address to a network device with a specified MAC address each time it requests an IP address. This is similar to having a static IP address except that the device must still request an IP address from the DHCP server. ';
var tcpip_dhcp_st_enabled = 'Enable Static DHCP';
var tcpip_dhcp_ipaddr = 'IP Address:';
var tcpip_dhcp_ip = 'IP Address';

var tcpip_dhcp_mac_port = '<td align="left" width="20%" class=""><font size=2><b>MAC Address/Port Number:</td>';
var tcpip_dhcp_mac_port2 = 'MAC Address/Port Number:';
var tcpip_dhcp_macport ='MAC Address/Port Number';


var tcpip_dhcp_mac_addr = '<td align="left" width="20%" class=""><font size=2><b>MAC Address:</td>';
var tcpip_dhcp_mac_addr2 = 'MAC Address';

var tcpip_dhcp_comment = 'Comment';
var tcpip_dhcp_list = 'Static DHCP List:';
var tcpip_dhcp_apply = "Apply Changes";
var tcpip_dhcp_reset = 'Reset';
var tcpip_dhcp_save = 'Save';
var tcpip_dhcp_save_apply = 'Save & Apply';

var tcpip_dhcp_delsel = 'Delete Selected';
var tcpip_dhcp_delall = 'Delete All';
var tcpip_dhcp_select = 'Select';

/***********	aWiFi.htm	************/
var app_aWiFi_enable = "Enable aWiFi";
var app_aWiFi_internet_wan = "Internet Wan for aWiFi";

/***********	aWiFi_whiteList.htm	************/
var aWiFi_mac_header = 'Trusted clients MAC for aWiFi';
var aWiFi_mac_header_explain =  'Entries in this table are used to allow certain wifi clients from your local network passing to the Internet through the aWiFi.';
var aWiFiMac_macaddr = 'MAC Address: ';

var aWiFiMac_macaddr_nocomplete = 'Input MAC address is not complete. It should be 12 digits in hex.';
var aWiFiMac_macaddr_nohex = 'Invalid MAC address. It should be in hex numbers (0-9 or a-f).';
var aWiFiMac_macaddr_nozero = 'Invalid MAC address. It should not be 00:00:00:00:00:00.';
var aWiFiMac_macaddr_nobroadcast = 'Invalid MAC address. It should not be ff:ff:ff:ff:ff:ff.';
var aWiFiMac_macaddr_nomulticast = 'Invalid MAC address. It should not be multicast mac address between 01:00:5e:00:00:00 and 01:00:5e:7f:ff:ff.';

var aWiFiMac_current_table = 'Current Trusted Clients MAC Table:';

var aWiFi_url_header = 'Trusted URL for aWiFi';
var aWiFi_url_header_explain =  'Entries in this table are used to allow wifi clients from your local network passing to certain URL through the aWiFi.';

var aWiFiUrl_url = 'Trusted Url';
var aWiFiUrl_apply_error = 'Error Character: \";\"';
var aWiFiUrl_current_table = 'Current Trusted Url Table:';

var aWiFiWhiteList_save = 'Save';
var aWiFiWhiteList_apply = 'Save & Apply';
var aWiFiWhiteList_reset = 'Reset';


var aWiFiWhiteList_comment = 'Comment';
var aWiFiWhiteList_select = 'Select';


var aWiFiWhiteList_delete = 'Delete Selected';
var aWiFiWhiteList_delete_all = 'Delete All';
var aWiFiWhiteList_delete_confirm = 'Do you really want to delete the selected entry?';
var aWiFiWhiteList_delete_all_confirm = 'Do you really want to delete the all entries?';


/***********	wizard.htm ************/
var wizard_header = 'Setup Wizard';
var wizard_header_explain = 'The setup wizard will guide  you to configure the Access Point for the first time. Follow the setup wizard step by step.';
var wizard_welcome = 'Welcome to the Setup Wizard.';
var wizard_content_explain = 'The Wizard will guide you through the following steps. Begin by clicking on  Next.';
var wizard_content1 = 'Setup Operating Mode';
var wizard_content2 = 'Choose your Time Zone';
var wizard_content3 = 'Setup LAN Interface';
var wizard_content4 = 'Setup WAN Interface';
var wizard_content5 = 'Select Wireless Band';
var wizard_content6 = 'Wireless Basic Setting';
var wizard_content7 = 'Wireless Security Setting';
var wizard_next_btn = '  Next>>';
var wizard_back_btn = '<<Back  ';
var wizard_cancel_btn = '  Cancel  ';
var wizard_finish_btn = 'Finished';

var wizard_opmode_invalid = 'Invalid opmode value ';
var wizard_chanset_wrong = 'Wrong field input!';
var wizard_wantypeselect = 'Error! Your browser must have CSS support!';
var wizard_weplen_error = 'Invalid mib value length0';

var wizard_content5_explain = 'You can select the Wireless Band';
var wizard_wire_band = 'Wireless Band:';

var wizard_basic_header_explain = ' This page is used to configure the parameters for wireless LAN clients that may connect to your Access Point.';
var wizard_wlan1_div0_mode = 'Mode:';
var wizard_chan_auto = 'Auto';
var wizard_client = 'Client';

var wizard_wpa_tkip = 'WPA (TKIP)';
var wizard_wpa_aes = 'WPA (AES)';
var wizard_wpa2_aes = 'WPA2(AES)';
var wizard_wpa2_mixed = 'WPA2 Mixed';
var wizard_use_cert_from_remote_as0 = 'Use Cert from Remote AS0';
var wizard_use_cert_from_remote_as1 = 'Use Cert from Remote AS1';

var wizard_5G_basic = 'Wireless 5GHz Basic Settings';
var wizard_5G_sec = 'Wireless 5G Security Setup';
var wizard_2G_basic = 'Wireless 2.4GHz Basic Settings';
var wizard_2G_sec = 'Wireless 2.4G Security Setup';

/***********	route.htm ************/
var route_header = 'Routing Setup';
var route_header_explain = 'This page is used to setup dynamic routing protocol or edit static route entry.';
var route_enable_static ='Enable Static Route';
var route_enable = 'Enabled';
var route_disable = 'Disabled';
var route_apply = 'Apply Changes';
var route_reset = 'Reset';
var route_save = 'Save';
var route_save_apply = 'Save & Apply';


var route_dynamic = 'Enable Dynamic Route';
var route_nat = 'NAT:';
var route_rip = 'RIP:';
var route_rip1 = 'RIPv1';
var route_rip2 = 'RIPv2';
var route_rip6 = 'RIPng:';
var route_static = 'Enable Static Route';
var route_ipaddr = 'IP Address:';
var route_mask = 'Subnet Mask:';
var route_gateway = 'Gateway:';
var route_metric = 'Metric:';
var route_interface = 'Interface:';
var route_lan = 'LAN';
var route_wan = 'WAN';
var route_showtbl = 'Show Route Table';

var route_static_tbl = 'Static Route Table:';
var route_static_tbl6 ='Static Route6 Table:';
var route_tbl_destip = 'Destination IP Address';
var route_tbl_destipv6ip ='Destination IPv6 Address';
var route_tbl_mask = 'Netmask';
var route_tbl_gateway = 'Gateway';
var route_tbl_metric = 'Metric';
var route_tbl_inter = 'Interface';
var route_tbl_status = 'Status';
var route_tbl_select = 'Select';
var route_tbl_enable = 'Enabled';
var route_tbl_disable = 'Disabled';

var route_deletechick_warn = 'Do you really want to delete the selected entry?';
var route_deleteall_warn = 'Do you really want to delete all entries?';
var route_deletechick = 'Delete Selected';
var route_deleteall = 'Delete All';

var route_addchick0 = 'Invalid IP address ';
var route_addchick1 = 'Invalid IP address value! ';
var route_addchick2 = 'Invalid Gateway address! ';
var route_addchick3 = 'Invalid metric value! The range of Metric is 1 ~ 15';
var route_checkip1 = 'IP address cannot be empty! It should be filled with four 4-digit numbers, i.e., xxx.xxx.xxx.xxx.';
var route_checkip2 = ' value. It should be decimal numbers (0-9).';
var route_checkip3 = ' range in 1st digit. It should be 1-223.';
var route_checkip4 = ' range in 2nd digit. It should be 0-255.';
var route_checkip5 = ' range in 3rd digit. It should be 0-255.';
var route_checkip6 = ' range in 4th digit. It should be 0-255.';
var route_validnum = 'Invalid value. It should be in decimal number (0-9).';
var route_setrip = 'You can not disable NAT in PPP wan type!';

var route_duplicate = 'Duplicate with entry';

var route_ipv6_enable = 'Enable Static Route6';
var route_ipv6_addr = 'IPv6 Address:';
var route_ipv6_pre ='Prefix Length';
var route_ipv6_gw ='IPv6 Gateway:';

var route_invalid_dp = 'Invalid destpfx!';
var route_invalid_ng = 'Invalid nextgw!';

var route_invalid_netmask = 'Invalid Netmask:';
var route_invalid_netmask_nomatch = 'Netmask does not match route address!';
var route_duplicate = 'Duplicate with entry';
var route_error = 'Set Route error\n';
var route_change ='Change setting successfully!<br><br>Do not turn off or reboot the Router during this time.';

/***********	routetbl.htm ************/
var routetbl_header = 'Routing  Table ';
var routetbl_header_explain = ' This table shows all the RIP(v1/v2) routing entries:';
var routetbl_refresh = 'Refresh';
var routetbl_close = ' Close ';
var routetbl_dst = 'Destination';
var routetbl_gw = 'Gateway';
var routetbl_mask = 'Genmask';
var routetbl_flag = 'Flags';
var routetbl_iface = 'Interface';
var routetbl_type = 'Type';
var routetbl_metric = 'Metric';
var routetbl_under_explain = 'This table shows all the RIPng routing entries:';
var routetbl_nhop = 'Next Hop';
var routetbl_ref = 'Ref';
var routetbl_use = 'Use';
var routetbl_if = 'Iface';
var routetbl_static =  'Static';
var routetbl_dynamic = 'Dynamic';


/***********	tr069config.htm************/
var tr069_header = 'TR-069 Configuration';
var tr069_header_explain = 'This page is used to configure the TR-069 CPE. Here you may change the setting for the ACS\'s parameters.';
var tr069_enable = 'Enabled';
var tr069_disable = 'Disabled';
var tr069_url = 'URL';
var tr069_username = 'User Name:';
var tr069_password = 'Password:';
var tr069_periodic_inform_enable = 'Periodic Inform Enable:';
var tr069_periodic_inform_interval = 'Periodic Inform Interval:';
var tr069_connection_request = 'Connection Request';
var tr069_filetrans_info = 'File Transfer Interface';
var tr069_filetrans_intf_specify_enable = 'Specify Interface:';
var tr069_filetrans_dns_intf = 'Transfer URL Parsing:'
var tr069_filetrans_intf = 'File Transfer:';
var tr069_path = 'Path:';
var tr069_port = 'Port:';
var tr069_STUN_connection ='STUN Connection';
var tr069_STUN_server_url = 'STUN Server URL:';
var tr069_STUN_server_port = 'STUN Server Port:';
var tr069_save = 'Save';
var tr069_save_apply = 'Save & Apply';
var tr060_undo = 'Undo';
var tr069_select_file = 'please select a file!';
var tr069_ACS_url_not_empty = 'ACS URL cannot be empty!';
var tr069_invalid_username = 'Invalid User Name !';
var tr069_invalid_password = 'Invalid Password !';
var tr069_periodic_interval_time ='Please input periodic interval time.';
var tr069_interval_decimal_number ='Interval should be the decimal number (0-9).';
var tr069_invalid_path ='Invalid Path !';
var tr069_input_port_number = 'Please input the port number for connection request.';
var tr069_invalid_port_number = 'Invalid port number of connection request. It should be 0-65535.(0 means default STUN port 3478)';
var tr069_STUN_server_url_not_empty = 'STUN Server URL cannot be empty!';
var tr069_STUN_input_port_number ='Please input the port number for STUN Server.';
var tr069_STUN_invalid_port_number ='Invalid port number of STUN Server. It should be 0-65535.';

/**********fmtr069.c************/
var tr069_certificat = 'Certificate';
var tr069_CA_certificat = 'CA Certificat:';
var tr069_upload = 'Upload';
var tr069_ACS_certificat = 'ACS Certificates CPE:';
var tr069_certificat_management = 'Certificat Management:';
var tr069_authentication = 'Authentication';

/**********fmget.c************/
var fmget_ip_dhcp = 'Getting IP from DHCP server...';
var fmget_ip = 'Fixed IP';
var fmget_dhcp = 'DHCP';
var fmget_ip_connect = 'Fixed IP Connected';
var fmget_ip_disconnect = 'Fixed IP Disconnected';
var fmget_pppoe_connet = 'PPPoE Connected';
var fmget_pppoe_disconnect = 'PPPoE Disconnected';
var fmget_pptp_connect = 'PPTP Connected';
var fmget_pptp_disconnect = 'PPTP Disconnected';
var fmget_l2tp_connect = 'L2TP Connected';
var fmget_l2tp_disconnect = 'L2TP Disconnected';

/*********fmfwall.c*************/
var fmfwall_not_set_ip_addr = 'Error! No ip address to set.';
var fmfwall_invalid_ip_addr = 'Invalid IP address! It should be set within the current subnet.';
var fmfwall_enable = 'Enable';
var fmfwall_disable = 'Disable';
var fmfwall_guarantee_min_bandwidth = 'Guaranteed minimum bandwidth';
var fmfwall_restrict_max_bandwidth = 'Restricted maximum bandwidth';
var fmfwall_set_mode = 'Set mode flag error!';
var fmfwall_ip_filter_type = 'Error! IP filter type error.';
var fmfwall_miss_addr = 'Error! Missing address to set.';
var fmfwall_invalid_protocol_type = 'Error! Invalid protocol type.';
var fmfwall_protocol_not_empty = 'Error! Protocol type cannot be empty.';
var fmfwall_rule_exist = 'rule already exist!';
var fmfwall_ip_forwarding_wan = 'Please select the ip-forwarding wan.';

/**********cluster_mgmt.asp************/
var cluster_header = 'Cluster Management';
var cluster_header_explain = 'Cluster Management is a service, that provides your WLAN configurations to other LAN/WLAN AP. If enabled, all LAN/WLAN AP that supports cluster management will share the same WLAN configuration as this one.';
var cluster_management = 'Enable Cluster Management';
var cluster_periodic_enable = 'Send configuration every';
var cluster_periodic_interval = 'minute';
var cluster_save_apply = 'Save & Apply';
var cluster_reset = 'Reset';
var cluster_management_not_support ='Cluster Management is not supported in current firmware!';
var cluster_management_interval_invalid ='Configuration send interval must be an integer between 1~10000!';

/***********	vlan.htm ************/
var vlan_header = 'VLAN Settings';
var vlan_header_explain = 'Entries in below table are used to configure vlan settings. VLANs are created to provide the segmentation services traditionally provided by routers. VLANs address issues such as scalability, security, and network management.';
var vlan_apply = 'Apply Changes';
var vlan_reset = 'Reset';
var vlan_save_apply = 'Save & Apply';
var vlan_save = 'Save';

var vlan_enable = 'Enable VLAN';
var vlan_enable2 = 'Enable';
var vlan_ethe_wire = 'Ethernet/Wireless';
var vlan_vid ='VID';
var vlan_pri ='Priority';
var vlan_id = 'VLAN ID:';
var vlan_forwardrule = 'Forward Rule:';
var vlan_forwardrulenat = 'NAT';
var vlan_forwardrulebridge = 'Bridge';
var vlan_tagtbl = 'Tag Table';
var vlan_tagtbl_interface = 'interface name';
var vlan_tagtbl_taged = 'tagged';
var vlan_tagtbl_untaged = 'unTagged';
var vlan_tagtbl_notin = 'not in this vlan';

var vlan_settbl = 'Current VLAN setting table';
var vlan_settbl_id = 'VlanID';
var vlan_settbl_taged = 'tagged interface';
var vlan_settbl_untaged = 'untagged interface';
var vlan_settbl_forwardrule = 'forward rule';
var vlan_settbl_modify = 'modify';
var vlan_settbl_select = 'select';
var vlan_deletechick = 'Delete Selected';
var vlan_deleteall = 'Delete All';

var vlan_nettbl = 'Current net interface setting table';
var vlan_nettbl_name = 'interface name';
var vlan_nettbl_pvid = 'Pvid';
var vlan_nettbl_defprio = 'default Priority';
var vlan_nettbl_defcfi = 'default Cfi';

var vlan_checkadd1 = 'invalid vlan id,must be between 1-4094';
var vlan_checkadd2 = 'At least one interface should bind to this vlan!';
var vlan_deletesel = 'Do you really want to delete the selected entry?';
var vlan_deleteall_conf = 'Do you really want to delete all entries?';

/***********	wlmultipleap.htm ************/
var wlmultipleap_header='Multiple APs'
var wlmultipleap_explain="This page shows and updates the wireless setting for multiple APs."
var wlmultiple_no="No."
var wlmultiple_enable="Enable"
var wlmultipleap_band="Band"
var wlmultiple_SSID="SSID"
var wlmultiple_data_rate="Data Rate"
var wlmultiple_ssid_priority = 'SSID Priority';
var wlmultiple_brocadcast_ssid="Broadcast SSID"
var wlmultiple_wmm="WMM"
var wlmultiple_access="Access"
var wlmultiple_tx_restrict="Tx Restrict(Mbps)"
var wlmultiple_rx_restrict="Rx Restrict(Mbps)"
var wlmultiple_active_client="Active Client List"
var wlmultiple_wlan_mode="WLAN mode"

var wlmultipleap_reset = ' Reset ';
var wlmultiple_save = 'Save';
var wlmultiple_save_apply = 'Save & Apply';

/***********wlsecurity_all.htm ************/
var wlsecurity_ascii_5characters='ASCII (5 characters)'
var wlsecurity_hex_10characters='Hex (10 characters)'
var wlsecurity_ascii_13characters='ASCII (13 characters)'
var wlsecurity_hex_26characters='Hex (26 characters)'
var wlsecurity_encryption='Encryption:'
var wlsecurity_disable='Disable'
var wlsecurity_8021_auth='802.1x Authentication:'
var wlsecurity_authentication='Authentication:'
var wlsecurity_open_system='Open System'
var wlsecurity_shared_key='Shared Key'
var wlsecurity_auto='Auto'
var wlsecurity_key_length='Key Length:'
var wlsecurity_64_bit='64-bit'
var wlsecurit_128_bit='128-bit'
var wlsecurity_key_format='Key Format:'
var wlsecurity_encryption_key='Encryption Key:'
var wlsecurity_authentication_mode='Authentication Mode:'
var wlsecurity_enterprise_radius='Enterprise (RADIUS)'
var wlsecurity_psk='Personal (Pre-Shared Key)'
var wlsecurity_wpa_cipher_suite='WPA Cipher Suite:'
var wlsecurity_tkip='TKIP'
var wlsecurity_aes='AES'
var wlsecurity_wpa2_cipher_suite='WPA2 Cipher Suite:'
var wlsecurity_wpa3_cipher_suite='WPA3 Cipher Suite:'

var wlsecurity_management_frame_protection='Management Frame Protection:'
var wlsecurity_none='none'
var wlsecurity_capable='capable'
var wlsecurity_required='required'

var wlsecurity_sha256='SHA256:'
var wlsecurity_sha256_disable='disable'
var wlsecurity_sha256_enable='enable'

var wlsecurity_psk_format='Pre-Shared Key Format:'
var wlsecurity_passphrase='Passphrase'
var wlsecurity_hex_64_characters='HEX (64 characters)'
var wlsecurity_pre_shared_key='Pre-Shared Key:'
var wlsecurity_radius_server_ip_address='RADIUS Server IP Address:'
var wlsecurity_radius_server_port='RADIUS Server Port:'
var wlsecurity_radius_server_password='RADIUS Server Password:'
var wlsecurity_encryption_is_none="encryption is none now"
var wlsecurity_wep_on_wps20_disable="if WEP is turn on,WPS2.0 will be disabled"
/***********	pocket_sitesurvey.htm ************/
var pocket_site_survey_running="Site-survey is running,please try later."
/***********	elink.htm ************/
var elink_header='elink'
var elinksdk_header='elink cloud client Upgrade'
var elinksdk_version = 'Elink cloud client firmware Version:';
var elink_explain='Enable elink function will make ap automatic networking with the intelligent gateway。'
var elink_disable='disable'
var elink_enable='enable'
var elink_sync_config="sync configuration"
var elink_sync_config_yes='yes'
var elink_sync_config_no='no'
var elink_reset = ' Reset ';
var elink_save = 'Save';
var elink_save_apply = 'Save & Apply';

/***********fmwlan.c*********/
var wl_Del_tab_ent="Delete table entry error!"
var wl_Add_tab_ent="Add table entry error!"
var wl_set_MIB_WLAN_SCHEDULE_ENABLED_err="set MIB_WLAN_SCHEDULE_ENABLED error!"
var wl_set_MIB_WLAN_SCHEDULE_DELALL_err="MIB_WLAN_SCHEDULE_DELALL error!"
var wl_set_MIB_WLAN_SCHEDULE_ADD_error="MIB_WLAN_SCHEDULE_ADD error!"
var wl_Get_tab_ent="Get table entry error!"
var wl_Can_add_new="Cannot add new entry because table is full!"
var wl_set_mesh_enable="Set mesh enable error!"
var wl_set_mesh_cross="Set mesh crossband enable error!"
var wl_set_mesh_root="Set mesh Root enable error!"
var wl_clear_MIB_WLAN_RS_USER_CERT_PASSWD_err="[1] Clear MIB_WLAN_RS_USER_CERT_PASSWD error!"
var wl_You_can_set="You cannot set client mode with 802.1x enabled!<br><br>Please change the encryption method in security page first."
var wl_Set_wla_Mac="Set wlan Mac clone error!"
var wl_Set_MIB_WLAN_MODE_err="Set MIB_WLAN_MODE error!"
var wl_Add_Pro_dup="Add Profile duplicately!"
var wl_Set_SSI_err="Set SSID error!"
var wl_Del_all_tab="Delete all table error!"
var wl_Set_ena_fla="Set enabled flag error!"
var wl_Set_mod_err_unknown="Set mode error! unknown mode!!"
var wl_Set_mod_err="Set mode error!"
var wl_Set_SSI_Bas="Set SSID Base Mode error!"
var wl_Set_SSI_Bas_empty="Set SSID Base Mode error!Empty allocation!!"
var wl_Set_SSI_Bas_alloc="Set SSID Base Mode error!allocation must be 0~100!!"
var wl_Set_SSI_Bas_total="Set SSID Base Mode error!Totoal allocation must be less than 100!!"
var wl_Err_Ple_fil_applocation="Error! Please fill allocation!"
var wl_Err_Ple_fil="Error! Please fill ipaddr or macaddr!"
var wl_Inv_IP_add="Invalid IP address!It should be set within the current subnet."
var wl_ip_add_exi="ip address exist!"
var wl_Err_Inv_MAC="Error! Invalid MAC address."
var wl_mac_add_exi="mac address exist!"
var wl_Tot_All_mus="Total Allocation must be less than 100!"
var wl_Err_Com_len="Error! Comment length too long."
var wl_Get_ent_num="Get entry number error!"
var wl_Can_add_new_station="Cannot add new Staion because table is full!"
var wl_Err_no_enc="Error! no encryption method."
var wl_Inv_enc_met="Invalid encryption method!"
var wl_Set_MIB_WLAN_MESH_ENCRYPT_mib="Set MIB_WLAN_MESH_ENCRYPT mib error!"
var wl_Set_MIB_WLAN_MESH_AUTH_TYPE_fai="Set MIB_WLAN_MESH_AUTH_TYPE failed!"
var wl_Set_MIB_WLAN_MESH_WPA2_UNICIPHER_fai="Set MIB_WLAN_MESH_WPA2_UNICIPHER failed!"
var wl_Err_no_psk="Error! no psk format."
var wl_Err_inv_psk_format="Error! invalid psk format."
var wl_Set_MIB_WLAN_MESH_PSK_FORMAT_fai="Set MIB_WLAN_MESH_PSK_FORMAT failed!"
var wl_Err_inv_psk_value="Error! invalid psk value."
var wl_Set_MIB_WLAN_MESH_WPA_PSK_err="Set MIB_WLAN_MESH_WPA_PSK error!"
var wl_Err_No_mac="Error! No mac address to set."
var wl_Err_Inv_Mes="Error! Invalid Mesh ID."
var wl_Set_MIB_WLAN_MESH_ID_err="Set MIB_WLAN_MESH_ID error!"
var wl_Set_MIB_WLAN_WPA2_UNICIPHER_fai="Set MIB_WLAN_WPA2_UNICIPHER failed!"
var wl_Set_MIB_WLAN_WPA_CIPHER_SUITE_fai="Set MIB_WLAN_WPA_CIPHER_SUITE failed!"
var wl_Key_len_mus="Key length must exist!"
var wl_Inv_key_len="Invalid key length value!"
var wl_Set_WEP_MIB="Set WEP MIB error!"
var wl_Set_MIB_WLAN_ENCRYPT_mib="Set MIB_WLAN_ENCRYPT mib error!"
var wl_Key_typ_mus="Key type must exist!"
var wl_Inv_key_typ="Invalid key type value!"
var wl_Set_WEP_key="Set WEP key type error!"
var wl_Inv_wep_val="Invalid wep-key value!"
var wl_Set_wep_err="Set wep-key error!"
var wl_Inv_def_tx="Invalid default tx key id!"
var wl_Set_def_tx="Set default tx key id error!"
var wl_Inv_key_1="Invalid key 1 length!"
var wl_Inv_wep1_val="Invalid wep-key1 value!"
var wl_Set_wep1_err="Set wep-key1 error!"
var wl_Inv_key_2="Invalid key 2 length!"
var wl_Inv_wep2_val="Invalid wep-key2 value!"
var wl_Set_wep2_err="Set wep-key2 error!"
var wl_Inv_key_3="Invalid key 3 length!"
var wl_Inv_wep3_val="Invalid wep-key3 value!"
var wl_Set_wep3_err="Set wep-key3 error!"
var wl_Inv_wep4_val="Invalid wep-key4 value!"
var wl_Set_wep4_err="Set wep-key4 error!"
var wl_Err_802_aut_wds="Error! 802.1x authentication cannot be used when device is set to wds or mesh mode."
var wl_Err_802_aut="Error! 802.1x authentication cannot be used when device is set to client mode."
var wl_Set_hs2_ena="Set hs2 enable flag error!"
var wl_Set_1x_ena="Set 1x enable flag error!"
var wl_Get_MIB_WLAN_WEP_MIB="Get MIB_WLAN_WEP MIB error!"
var wl_Set_MIB_WLAN_MAC_AUTH_ENABLED_MIB="Set MIB_WLAN_MAC_AUTH_ENABLED MIB error!"
var wl_Set_MIB_WLAN_IEEE80211W_fai="Set MIB_WLAN_IEEE80211W failed!"
var wl_Set_MIB_WLAN_SHA256_ENABLE_fai="Set MIB_WLAN_SHA256_ENABLE failed!"
var wl_Err_WAP_AS="Error! WAPI AS cannot be used when device is set to client mode."
var wl_Err_Inv_wap="Error! Invalid wapi authentication value."
var wl_Set_MIB_WLAN_WAPI_AUTH_fai="Set MIB_WLAN_WAPI_AUTH failed!"
var wl_Set_MIB_WLAN_PSK_FORMAT_fai="Set MIB_WLAN_PSK_FORMAT failed!"
var wl_Err_inv_psk_len="Error! invalid psk len."
var wl_Err_Set_wap="Error! Set wapi key len fault"
var wl_Set_MIB_WLAN_WPA_PSK_err="Set MIB_WLAN_WPA_PSK error!"
var wl_No_WAP_AS="No WAPI AS address!"
var wl_Inv_AS_IP="Invalid AS IP-address value!"
var wl_No_WAP_cer="No WAPI cert selected!"
var wl_Set_RS_IP="Set RS IP-address error!"
var wl_Set_WAP_cer="Set WAPI cert sel error!"
var wl_Set_MIB_WLAN_ENABLE_SUPP_NONWPA_mib="Set MIB_WLAN_ENABLE_SUPP_NONWPA mib error!"
var wl_Set_MIB_WLAN_SUPP_NONWPA_mib="Set MIB_WLAN_SUPP_NONWPA mib error!"
var wl_Err_WPA_can_wds="Error! WPA-RADIUS cannot be used when device is set to wds or mesh mode."
var wl_Err_WPA_can="Error! WPA-RADIUS cannot be used when device is set to client mode."
var wl_Err_Inv_wpa="Error! Invalid wpa authentication value."
var wl_Set_MIB_WLAN_WPA_AUTH_fai="Set MIB_WLAN_WPA_AUTH failed!"
var wl_Inv_val_of_cipher="Invalid value of cipher suite!"
var wl_Err_Can_set="Error! Can't set cipher to TKIP + AES when device is set to client mode."
var wl_Inv_val_of_80211w="Invalid value of IEEE80211w!"
var wl_Inv_val_ofsha="Invalid value of wpa2EnableSHA256!"
var wl_Inv_val_of_wpa2="Invalid value of wpa2 cipher suite!"
var wl_Set_MIB_WLAN_WPA2_CIPHER_SUITE_fai="Set MIB_WLAN_WPA2_CIPHER_SUITE failed!"
var wl_Err_Inv_val_day="Error! Invalid value of rekey day."
var wl_Err_Inv_val_hr="Error! Invalid value of rekey hr."
var wl_Err_Inv_val_min="Error! Invalid value of rekey min."
var wl_Err_Inv_val_sec="Error! Invalid value of rekey sec."
var wl_Set_MIB_WLAN_WPA_GROUP_REKEY_TIME_err="Set MIB_WLAN_WPA_GROUP_REKEY_TIME error!"
var wl_Set_MIB_WLAN_RS_BAND_SEL_err="Set MIB_WLAN_RS_BAND_SEL error!"
var wl_Inv_802_EAP="Invalid 802.1x EAP type value!"
var wl_Set_MIB_WLAN_EAP_TYPE_err="Set MIB_WLAN_EAP_TYPE error!"
var wl_No_802_EAP_type="No 802.1x EAP type!"
var wl_EAP_use_ID="EAP user ID too long!"
var wl_Set_MIB_WLAN_EAP_USER_ID_err="Set MIB_WLAN_EAP_USER_ID error!"
var wl_No_802_EAP="No 802.1x EAP User ID!"
var wl_RAD_use_nam="RADIUS user name too long!"
var wl_Set_MIB_WLAN_RS_USER_NAME_err="Set MIB_WLAN_RS_USER_NAME error!"
var wl_No_802_RAD_name="No 802.1x RADIUS User Name!"
var wl_RAD_use_pas="RADIUS user password too long!"
var wl_Set_MIB_WLAN_RS_USER_PASSWD_err="Set MIB_WLAN_RS_USER_PASSWD error!"
var wl_No_802_RAD_password="No 802.1x RADIUS User Password!"
var wl_RAD_use_cer="RADIUS user cert password too long!"
var wl_Set_MIB_WLAN_RS_USER_CERT_PASSWD_err="Set MIB_WLAN_RS_USER_CERT_PASSWD error!"
var wl_Cle_MIB_WLAN_RS_USER_CERT_PASSWD_err="Clear MIB_WLAN_RS_USER_CERT_PASSWD error!"
var wl_No_802_RAD_user="No 802.1x RADIUS user cert password!"
var wl_No_802_RAD5u="No 802.1x RADIUS 5g user cert!nPlease upload it."
var wl_No_802_RAD5r="No 802.1x RADIUS 5g root cert!nPlease upload it."
var wl_No_802_RAD2u="No 802.1x RADIUS 2g user cert!nPlease upload it."
var wl_No_802_RAD2r="No 802.1x RADIUS 2g root cert!nPlease upload it."
var wl_Inv_802_ins="Invalid 802.1x inside tunnel type value!"
var wl_Set_MIB_WLAN_EAP_INSIDE_TYPE_err="Set MIB_WLAN_EAP_INSIDE_TYPE error!"
var wl_No_802_ins="No 802.1x inside tunnel type!"
var wl_No_802_RAD_cert_pass="No 802.1x RADIUS user cert password!"
var wl_802_ins_tun="802.1x inside tunnel type not support!"
var wl_802_EAP_typ="802.1x EAP type not support!"
var wl_No_RS_por="No RS port number!"
var wl_Err_Inv_val_port="Error! Invalid value of RS port number."
var wl_Set_RS_por="Set RS port error!"
var wl_No_RS_IP="No RS IP address!"
var wl_Inv_RS_IP="Invalid RS IP-address value!"
var wl_RS_pas_len="RS password length too long!"
var wl_Set_RS_pas="Set RS password error!"
var wl_Inv_RS_ret="Invalid RS retry value!"
var wl_Set_MIB_WLAN_RS_MAXRETRY_err="Set MIB_WLAN_RS_MAXRETRY error!"
var wl_Inv_RS_tim="Invalid RS time value!"
var wl_Set_MIB_WLAN_RS_INTERVAL_TIME_err="Set MIB_WLAN_RS_INTERVAL_TIME error!"
var wl_Set_MIB_WLAN_ACCOUNT_RS_ENABLED_mib="Set MIB_WLAN_ACCOUNT_RS_ENABLED mib error!"
var wl_No_acc_RS="No account RS port number!"
var wl_Set_acc_RS="Set account RS port error!"
var wl_No_acc_RS_ip="No account RS IP address!"
var wl_Inv_acc_RS="Invalid account RS IP-address value!"
var wl_Set_acc_RS_ip="Set account RS IP-address error!"
var wl_Acc_RS_pas="Account RS password length too long!"
var wl_Set_acc_RS_pass="Set account RS password error!"
var wl_Inv_acc_RS_retry="Invalid account RS retry value!"
var wl_Set_MIB_WLAN_ACCOUNT_RS_MAXRETRY_err="Set MIB_WLAN_ACCOUNT_RS_MAXRETRY error!"
var wl_Inv_acc_RS_time="Invalid account RS time value!"
var wl_Set_MIB_WLAN_ACCOUNT_RS_INTERVAL_TIME_err="Set MIB_WLAN_ACCOUNT_RS_INTERVAL_TIME error!"
var wl_Set_MIB_WLAN_ACCOUNT_RS_UPDATE_ENABLED_mib="Set MIB_WLAN_ACCOUNT_RS_UPDATE_ENABLED mib error!"
var wl_Err_Inv_val_update="Error! Invalid value of update time"
var wl_Set_MIB_WLAN_ACCOUNT_RS_UPDATE_DELAY_mib="Set MIB_WLAN_ACCOUNT_RS_UPDATE_DELAY mib error!"
var wl_Err_Inv_wep="Error! Invalid wepkeylen value."
var wl_Set_MIB_WLAN_WEP_fai="Set MIB_WLAN_WEP failed!"
var wl_ssi_is_inv="ssid_idx is invalid!!"
var wl_Inv_wpa_val="Invalid wpaAuth value!"
var wl_Mod_tab_ent="Modify table entry error!"
var wl_Err_Inv_aut="Error! Invalid authentication value."
var wl_Set_aut_fai="Set authentication failed!"
var wl_Err_Inv_val_frag="Error! Invalid value of fragment threshold."
var wl_Set_fra_thr="Set fragment threshold failed!"
var wl_Err_Inv_val_rts="Error! Invalid value of RTS threshold."
var wl_Set_RTS_thr="Set RTS threshold failed!"
var wl_Err_Inv_val_beacon="Error! Invalid value of Beacon Interval."
var wl_Set_Bea_int="Set Beacon interval failed!"
var wl_Err_Inv_val_ack="Error! Invalid value of Ack timeout."
var wl_Set_Ack_tim="Set Ack timeout failed!"
var wl_Set_rat_ada="Set rate adaptive failed!"
var wl_Set_fix_rat="Set fix rate failed!"
var wl_Set_Tx_bas="Set Tx basic rate failed!"
var wl_Set_Tx_ope="Set Tx operation rate failed!"
var wl_Err_Inv_Pre="Error! Invalid Preamble value."
var wl_Set_Pre_fai="Set Preamble failed!"
var wl_Err_Inv_hid="Error! Invalid hiddenSSID value."
var wl_Set_hid_ssi="Set hidden ssid failed!"
var wl_Err_Inv_val_dtim="Error! Invalid value of DTIM period."
var wl_Set_DTI_per="Set DTIM period failed!"
var wl_Err_Inv_IAP="Error! Invalid IAPP value."
var wl_Set_MIB_WLAN_IAPP_DISABLED_fai="Set MIB_WLAN_IAPP_DISABLED failed!"
var wl_Err_Inv_HS2="Error! Invalid HS2 value."
var wl_Set_MIB_WLAN_HS2_ENABLED_fai="Set MIB_WLAN_HS2_ENABLED failed!"
var wl_Err_Inv_11g="Error! Invalid 11g Protection value."
var wl_Set_MIB_WLAN_PROTECTION_DISABLED_fai="Set MIB_WLAN_PROTECTION_DISABLED failed!"
var wl_Err_Inv_WMM="Error! Invalid WMM value."
var wl_Set_MIB_WLAN_WMM_ENABLED_fai="Set MIB_WLAN_WMM_ENABLED failed!"
var wl_Err_Inv_tur="Error! Invalid turbo mode value."
var wl_Set_MIB_WLAN_TURBO_MODE_fai="Set MIB_WLAN_TURBO_MODE failed!"
var wl_Err_Inv_RF="Error! Invalid RF output power value."
var wl_Set_MIB_WLAN_RFPOWER_SCALE_fai="Set MIB_WLAN_RFPOWER_SCALE failed!"
var wl_Err_Inv_Cha="Error! Invalid Channel Bonding."
var wl_Set_MIB_WLAN_CHANNEL_BONDING_fai="Set MIB_WLAN_CHANNEL_BONDING failed!"
var wl_Err_Inv_Con="Error! Invalid Control SideBand."
var wl_Set_MIB_WLAN_CONTROL_SIDEBAND_fai="Set MIB_WLAN_CONTROL_SIDEBAND failed!"
var wl_Set_MIB_WLAN_AGGREGATION_fai="Set MIB_WLAN_AGGREGATION failed!"
var wl_Err_Inv_sho="Error! Invalid short GI."
var wl_Set_MIB_WLAN_SHORT_GI_fai="Set MIB_WLAN_SHORT_GI failed!"
var wl_Sit_req_fai="Site-survey request failed!"
var wl_Che_enc_err="Check encryption error!"
var wl_Enc_typ_mis="Encryption type mismatch!"
var wl_Joi_req_fai="Join request failed!"
var wl_conreq_tim_="connect-request timeout!"
var wl_Get_Joi_res="Get Join result failed!"
var wl_con_tim_="connect timeout!"
var wl_enc_val_not="encrypt value not validt!"
var wl_Inv_wep_key="Invalid wep key format value!"
var wl_Inv_wep_key_len="Invalid wep key length!"
var wl_Err_no_act="Error, no action is defined!"
var wl_Err_inv_act="Error, invalid action request!"
var wl_Inv_mod_val="Invalid mode value!"
var wl_The_que_sel="The question selection or answer of Q&A mode is not matched with installed value!"
var wl_Can_not_set_key="Can not set MCAST key policy!"
var wl_Can_not_set_tim="Can not set MCAST TIME!"
var wl_Can_not_set_pac="Can not set MCAST Packet!"
var wl_Can_not_set_pol="Can not set ucast key policy!"
var wl_Can_not_set_utime="Can not set ucast time!"
var wl_Can_not_set_upacket="Can not set ucast Packet!"
var wl_Set_Sea_MIB="Set Search MIB Index Error!"
var wl_Not_a_Cer="Not a Cert File!"
var wl_Can_not_ope="Can not open tmp cert!"
var wl_Unk_Cer_Fil="Unknown Cert File!"
var wl_Cer_Ins_Suc="Cerification Install Success!"
var wl_Set_Sea_Ind="Set Search Index Error!"
var wl_Set_Sea_Inf="Set Search Info Error!"
var set_MIB_WLAN_RS_BAND_SEL_err="Set MIB_WLAN_RS_BAND_SEL error!"
var wl_Wro_rsB_="Wrong rsBandSel !"
var wl_Del_all_802_5="Delete all 802.1x cerificates of 5GHz success!"
var wl_Del_all_802_2="Delete all 802.1x cerificates of 2.4GHz success!"
var wl_No_802_cer="No 802.1x cert inclued in upload file!"
var wl_No_802_pri="No 802.1x private key inclued in upload file!"
var wl_802_use_cer="802.1x user cerificate and user key upload success!"
var wl_No_802_cert_inclued="[2] No 802.1x cert inclued in upload file!"
var wl_not_ssid_type="Error! Not support this SSID TYPE."
var wl_802_roo_cer="802.1x root cerificate upload success!"
var wl_err_han_="error handlen"
var wl_Set_CA_sel="Set CA select error!"
var wl_Set_Use_sel="Set User select error!"
var wl_Err__Upl="Error ! Upload file length is 0 !"
var wl_Aut_sca_run="Auto scan running!!please wait..."
var wl_sca_req_tim="scan request timeout!"
var wl_sca_tim_="scan timeout!"
var wl_no_nas="Error! No NAS identifier to set."
var wl_err_no_r0r1_key="Error! No R0KH/R1KH key to set."
var wl_err_no_op_class="Error! No Op Class to set."
var wl_no_channel="Error! No Channel to set."
var wl_get_mbssid_tbl_error="Error! Get MIB_MBSSIB_TBL error."
var wl_entry_exists="Entry already exists!"
var wl_err_table_full="Error! Table Full."
var wl_err_add_chain= "Error! Add chain record."
var wl_del_chain_record="Delete chain record error!"
var wl_no_item_selected="There is no item selected to delete!"
var wl_disabled="Disabled"
var wl_enabled="Enabled"
var wl_error_handle="error handle"
var wl_open_file_error="open file error"

/***********	CONFIG_CUTE_MAHJONG ************/
var opmode_radio_upgradekit='Upgrade Kit:';
var opmode_radio_upgradekit_explain='In this mode all ethernet ports and wireless interfaces are bridged together and the NAT function is disabled.';
var opmode_radio_gw_explain_cmj='In this mode, the device connects to internet via an ADSL/Cable Modem. NAT is enabled and PCs on LAN ports share the same IP to the ISP via the WAN port. The connection type is DHCP client.';
var opmode_radio_wisp_explain_cmj='In this mode, all ethernet ports are bridged together and the wireless client will connect to the ISP’s Access Point. NAT is enabled and PCs on Ethernet ports share the same IP to the ISP via the wireless LAN. You can connect to the ISP’s AP on the Site-Survey page. The connection type is DHCP client.';
var opmode_radio_auto_explain_cmj='In this mode, the WAN detection function will automatically boot in Gateway or WISP mode.';

var wizard_cmj_content1 = 'Set Operating Mode';
var wizard_cmj_content2 = 'Set Wireless Network Name';
var wizard_cmj_content2_explain = 'Enter the Wireless Network Name of the AP.';
var wizard_cmj_content2_explain2 = 'Wireless Network Name (SSID):';
var wizard_cmj_content3 = 'Select the Wireless Security Mode';

var wizard_sitesurvey_content = 'Wireless Site Survey';
var wizard_sitesurvey_content_explain = 'This page provides a tool to scan for wireless networks. If an Access Point or IBSS is found, you can choose to connect to it manually when client mode is enabled.';

var switch_ui_mode2adv  = 'switch to advanced mode';
var switch_ui_mode2easy = 'switch to easy mode';


/***********	OK_MSG ************/
var okmsg_explain = "<body><blockquote><h4>Changed setting successfully!</h4>Your changes have been saved. The router must be rebooted for the changes to take effect.<br> You can reboot now, or you can continue to make other changes and reboot later.\n";
var okmsg_reboot_now = 'Reboot Now';
var okmsg_reboot_later = 'Reboot Later';

var okmsg_btn = '  OK  ';

var okmsg_fw_saveconf = 'Reloaded settings successfully!<br><br>The Router is booting.<br>Do not turn off or reboot the Device during this time.<br>';
var okmsg_fw_opmode = 'Set Operating Mode successfully!';
var okmsg_fw_passwd = 'Changed setting successfully!<br><br>Do not turn off or reboot the Router during this time.';

/*********** common msg ************/
var ip_should_in_current_subnet = 'Invalid IP address! It should be set within the current lan subnet.';
var ip_should_be_different_from_lanip = 'Invalid IP address! It should be different from lan ip address.';

//l2tp
var l2tp_wan_header = 'L2TP Interface Setup';
var l2tp_configuration ='Configurations';
var l2tp_action = 'Action';
var l2tp_this_pate_is_used_to_configure_the_parameters_for_l2tp_mode_vpn = 'This page is used to configure the parameters for L2TP mode VPN.';
var l2tp_persistent = 'Persistent';
var l2tp_dial_on_demand = 'Dial-on-Demand';
var l2tp_server = 'Server';
var l2tp_encryption = 'Encryption';
var l2tp_authentication = 'Authentication';
var l2tp_auto = 'Auto';
var l2tp_idle_time_min = 'Idle Time (min)';
var l2tp_none = 'NONE';
var l2tp_name = 'Name';
var l2tp_user = 'User Name';
var l2tp_password = 'Password';
var l2tp_interface = 'Interface';
var l2tp_default_gateway = 'Default Gateway';
var	l2tp_delete_selected = 'Delete Selected';
var l2tp_select = 'Select';
var	l2tp_connection_type = 'Connection Type';
var l2tp_table = 'Table';
var l2tp_manual = 'Manual';
var l2tp_idle_time_sec = 'Idle Time (sec)';
var l2tp_apply_changes = 'Apply Changes';
var l2tp_enabled = 'Enabled';
var l2tp_disabled = 'Disabled';
function dw(str)
{
	document.write(str);
}

