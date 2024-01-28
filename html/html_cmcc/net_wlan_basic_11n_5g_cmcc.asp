<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<HTML>
<HEAD>
<TITLE>家庭网关-WLAN配置</TITLE>
<META http-equiv=pragma content=no-cache>
<META http-equiv=cache-control content="no-cache, must-revalidate">
<META http-equiv=content-type content="text/html; charset=utf-8">
<META http-equiv=content-script-type content=text/javascript>
<!--系统公共css-->
<STYLE type=text/css>
@import url(/style/default.css);
</STYLE>
<!--系统公共脚本-->
<SCRIPT language="javascript" src="common.js"></SCRIPT>
<script type="text/javascript" src="share_cmcc.js"></script>
<SCRIPT language="javascript" type="text/javascript">

var cgi_wlCurrentChannel = "11"; // 当前信道
//var cgi_wlBssid = "wlBssid"; // wlBssid值
var enbl = true; // 取wlEnbl值；true 表示启用无线, false 则不启用。
var ssid = "CMCC-one"; // SSID值
var txPower = "100"; // 发送功率
//var hiddenSSID = true; // 取消广播的开关, false 表示启用广播，true 表示不启用广播
var wlanMode = 1; // WLAN模式
var wlBandMode = 0;
var ssid_2g = "0";
var ssid_5g = "0";
var defaultBand = 75;
var _Band2G5GSupport = new Array();
var regDomain; //, defaultChan;
var _wlanEnabled = new Array();
var _band = new Array();
var _ssid = new Array();
var _bssid = new Array();
var _chan = new Array();
var _chanwid = new Array();
var _ctlband = new Array();
var _txRate = new Array();
var _txpower = new Array();
var _txpower_high = new Array();
var _hiddenSSID = new Array();
var _wlCurrentChannel = new Array();
var _auto = new Array();
var _rf_used = new Array();
var _shortGI0 = new Array();
var _defaultKeyidx = new Array(); 
var wlan_num = 2;
var wlan_support_8812e = 1;
var dfs_enable=1;
var wlan_sta_control = 0;
var wlan_module_enable = 1;
var wlan_rate_prior_enable = 0;
var wlan_txpower_high_enable = 0;
var _wlan_rate_prior = new Array();
var _wlan_11k = new Array();
var _wlan_11v = new Array();
var _wlan_beacon_interval = new Array();
var _wlan_dtim_period = new Array();
var channel_list_5g_dfs =[[],
					  [36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165],
					  [36,40,44,48,52,56,60,64,149,153,157,161],
					  [36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140],
					  [36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140],
					  [36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140],
					  [36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140],
					  [36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140],
					  [34,38,42,46],
					  [36,40,44,48],
					  [36,40,44,48,52,56,60,64],
					  [56,60,64,100,104,108,112,116,136,140,149,153,157,161,165],
					  [36,40,44,48,52,56,60,64,132,136,140,149,153,157,161,165],
					  [36,40,44,48,52,56,60,64,149,153,157,161,165],
    			      [36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165],
    			      [36,40,44,48,52,56,60,64,100,104,108,112,116,136,140,149,153,157,161,165],
    			      [36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,144,149,153,157,161,165,169,173,177]
					  ];


var channel_list_5g =[[],
					  [36,40,44,48,149,153,157,161,165],
					  [36,40,44,48,149,153,157,161],
					  [36,40,44,48],
					  [36,40,44,48],
					  [36,40,44,48],
					  [36,40,44,48],
					  [36,40,44,48],
					  [34,38,42,46],
					  [36,40,44,48],
					  [36,40,44,48],
					  [56,60,64,149,153,157,161,165],
					  [36,40,44,48,149,153,157,161,165],
					  [36,40,44,48,149,153,157,161,165],
    			      [36,40,44,48,149,153,157,161,165],
    			      [36,40,44,48,149,153,157,161,165],
    			      [36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,144,149,153,157,161,165,169,173,177]
					  ];
var user_mode = 1;

/*Auth*/
var ssidIdx = 1;//SSID索引号
var wpa="tkip+aes";//用于判断后表列回显
var mode = "open";//网络认证方式
var bcntpsubcriber = "1";//网络认证方式开关：0表示不可编辑，1表示可编辑
var wlssidlist="China_kkk";//列表显示
var wep = "enabled";//sep加密分别为：enabled，disabled
var wlCorerev = "3";//WPA加密:大于等于3时,WPA 加密列表值分别为："TKIP", "AES", "TKIP+AES"；否则WPA 加密列表值"TKIP"
var bit = "0";//密钥长度:WEP128=0;WEP64=1
var wpapskValue = "00005544";//WPA预共享密钥
var wepkValue = "11111";//WEP预共享密钥
var wpapskUpInter = "55";//WPA更新会话密钥间隔
var keyIdx = "1";//网络密钥索引号分别对应：1,2,3,4
var keys = new Array( "key1", "key2","key3", "key4" ); //网络密钥值列表

var defPskLen, defPskFormat;
var wps20;
var oldMethod;
var wlanMode;
var encrypt_value;
var cipher_value;
var wpa2cipher_value;
var _wlan_mode=new Array();
var _encrypt=new Array();
var _enable1X=new Array();
var _wpaAuth=new Array();
var _wpaPSKFormat=new Array();
var _wpaPSK=new Array();
var _rsPort=new Array();
var _rsIpAddr=new Array();
var _rsPassword=new Array();
var _uCipher=new Array();
var _wpa2uCipher=new Array();
var _wepAuth=new Array();
var _wepLen=new Array();
var _wepKeyFormat=new Array();
var _wlan_isNmode=new Array();

/*WPS*/
var _WPS_running = new Array(); //running 
var wscDisable=new Array();



// band=76
txrate=1
auto=1
chanwid=2
rf_num=2

regDomain=13
defaultChan=0

/********************************************************************
**          on document load
********************************************************************/
/*Auth*/
function get_by_id(id)
{
	with(document) {
		return getElementById(id);
	}
}

function getSelectVal(id)
{
	with(document) {
		return get_by_id(id).value;
	}
}

function show_wpa_settings()
{
	get_by_id("show_wpa_psk2").style.display = "";		
}

function show_authentication()
{	
	var security = get_by_id("security_method");
	var form1 = document.wlanSetup;

	if (wlanMode==1 && security.value == 6) {	/* client and WIFI_SEC_WPA2_MIXED */
		alert("Not allowed for the Client mode.");
		security.value = oldMethod;
		return false;
	}
	oldMethod = security.value;
	get_by_id("show_wep_auth").style.display = "none";
	get_by_id("setting_wep").style.display = "none";
	get_by_id("setting_wpa").style.display = "none";
	get_by_id("show_wpa_cipher").style.display = "none";
	get_by_id("show_wpa2_cipher").style.display = "none";

	if (security.value == 1) {	/* WIFI_SEC_WEP */
		get_by_id("show_wep_auth").style.display = "";
		if (wlanMode == 1)
			get_by_id("setting_wep").style.display = "";
		else {
			get_by_id("setting_wep").style.display = "";
		}
	}else if (security.value == 2 || security.value == 4 || security.value == 6){	/* WIFI_SEC_WPA/WIFI_SEC_WPA2/WIFI_SEC_WPA2_MIXED */
	
		get_by_id("setting_wpa").style.display = "";
		if (security.value == 2) {	/* WIFI_SEC_WPA */
			get_by_id("show_wpa_cipher").style.display = "";
			/*if(encrypt_value != security.value){
				form1.ciphersuite.value = 1;
			}
			else{*/
				if(cipher_value == 1)
					form1.ciphersuite.value = 1;
				if(cipher_value == 2)
					form1.ciphersuite.value = 2;
				if(cipher_value == 3)
					form1.ciphersuite.value = 3;
			//}
		}
		if(security.value == 4) {	/* WIFI_SEC_WPA2 */
			get_by_id("show_wpa2_cipher").style.display = "";
			/*if(encrypt_value != security.value){
				form1.wpa2ciphersuite.value = 2;
			}
			else{*/
				if(wpa2cipher_value == 1)
					form1.wpa2ciphersuite.value = 1;
				if(wpa2cipher_value == 2)
					form1.wpa2ciphersuite.value = 2;
				if(wpa2cipher_value == 3)
					form1.wpa2ciphersuite.value = 3;
			//}
		}
		if(security.value == 6){	/* WIFI_SEC_WPA2_MIXED */
			get_by_id("show_wpa_cipher").style.display = "";
			get_by_id("show_wpa2_cipher").style.display = "";
			/*if(encrypt_value != security.value){
				form1.ciphersuite.value = 1;
				form1.wpa2ciphersuite.value = 2;
			}
			else{*/
				if(cipher_value == 1)
					form1.ciphersuite.value = 1;
				if(cipher_value == 2)
					form1.ciphersuite.value = 2;
				if(cipher_value == 3)
					form1.ciphersuite.value = 3;
				if(wpa2cipher_value == 1)
					form1.wpa2ciphersuite.value = 1;
				if(wpa2cipher_value == 2)
					form1.wpa2ciphersuite.value = 2;
				if(wpa2cipher_value == 3)
					form1.wpa2ciphersuite.value = 3;
			//}
		}
		show_wpa_settings();
	}
}

function setDefaultKeyValue(form, wlan_id)
{
	if (form.elements["length"+wlan_id].selectedIndex == 0) {
		if (form.elements["format"+wlan_id].selectedIndex == 0) {
			form.elements["key"].maxLength = 5;
			form.elements["key_1"].maxLength = 5;
			form.elements["key_2"].maxLength = 5;
			form.elements["key_3"].maxLength = 5;
		}
		else {
			form.elements["key"].maxLength = 10;
			form.elements["key_1"].maxLength = 10;
			form.elements["key_2"].maxLength = 10;
			form.elements["key_3"].maxLength = 10;
		}
	}
	else {
		if (form.elements["format"+wlan_id].selectedIndex == 0) {
			form.elements["key"].maxLength = 13;		
			form.elements["key_1"].maxLength = 13;	
			form.elements["key_2"].maxLength = 13;	
			form.elements["key_3"].maxLength = 13;	
		}
		else {
			form.elements["key"].maxLength = 26;
			form.elements["key_1"].maxLength = 26;
			form.elements["key_2"].maxLength = 26;
			form.elements["key_3"].maxLength = 26;
		}
	}
}

function updateWepFormat(form, wlan_id)
{
	if (form.elements["length" + wlan_id].selectedIndex == 0) {
		form.elements["format" + wlan_id].options[0].text = 'ASCII (5 characters)';
		form.elements["format" + wlan_id].options[1].text = 'Hex (10 characters)';
	}
	else {
		form.elements["format" + wlan_id].options[0].text = 'ASCII (13 characters)';
		form.elements["format" + wlan_id].options[1].text = 'Hex (26 characters)';
	}
	setDefaultKeyValue(form, wlan_id);
}

function check_wepkey()
{
	form = document.wlanSetup;
	var keyLen;
	if (form.length0.selectedIndex == 0) {
  		if (form.format0.selectedIndex == 0)
			keyLen = 5;
		else
			keyLen = 10;
	}
	else {
		if (form.format0.selectedIndex == 0)
			keyLen = 13;
		else
			keyLen = 26;
	}
	if (form.key0.value.length != keyLen) {
		alert('不合法的key length值, 应为' + keyLen + '字元');
		form.key0.focus();
		return 0;
	}
	if (form.key1.value.length != keyLen) {
		alert('不合法的key length值, 应为' + keyLen + '字元');
		form.key1.focus();
		return 0;
	}
	if (form.key2.value.length != keyLen) {
		alert('不合法的key length值, 应为' + keyLen + '字元');
		form.key2.focus();
		return 0;
	}
	if (form.key3.value.length != keyLen) {
		alert('不合法的key length值, 应为' + keyLen + '字元');
		form.key3.focus();
		return 0;
	}
	if (form.key0.value == "*****" ||
		form.key0.value == "**********" ||
		form.key0.value == "*************" ||
		form.key0.value == "**************************" )
		return 1;

	if (form.format0.selectedIndex==0)
		return 1;

	for (var i=0; i<form.key0.value.length; i++) {
		if ( (form.key0.value.charAt(i) >= '0' && form.key0.value.charAt(i) <= '9') ||
			(form.key0.value.charAt(i) >= 'a' && form.key0.value.charAt(i) <= 'f') ||
			(form.key0.value.charAt(i) >= 'A' && form.key0.value.charAt(i) <= 'F') )
			continue;

		alert("不合法的key值, 应为十六进制 (0-9 or a-f).");
		form.key0.focus();
		return 0;
	}
	for (var i=0; i<form.key1.value.length; i++) {
		if ( (form.key1.value.charAt(i) >= '0' && form.key1.value.charAt(i) <= '9') ||
			(form.key1.value.charAt(i) >= 'a' && form.key1.value.charAt(i) <= 'f') ||
			(form.key1.value.charAt(i) >= 'A' && form.key1.value.charAt(i) <= 'F') )
			continue;

		alert("不合法的key值, 应为十六进制 (0-9 or a-f).");
		form.key1.focus();
		return 0;
	}
	for (var i=0; i<form.key2.value.length; i++) {
		if ( (form.key2.value.charAt(i) >= '0' && form.key2.value.charAt(i) <= '9') ||
			(form.key2.value.charAt(i) >= 'a' && form.key2.value.charAt(i) <= 'f') ||
			(form.key2.value.charAt(i) >= 'A' && form.key2.value.charAt(i) <= 'F') )
			continue;

		alert("不合法的key值, 应为十六进制 (0-9 or a-f).");
		form.key2.focus();
		return 0;
	}
	for (var i=0; i<form.key3.value.length; i++) {
		if ( (form.key3.value.charAt(i) >= '0' && form.key3.value.charAt(i) <= '9') ||
			(form.key3.value.charAt(i) >= 'a' && form.key3.value.charAt(i) <= 'f') ||
			(form.key3.value.charAt(i) >= 'A' && form.key3.value.charAt(i) <= 'F') )
			continue;

		alert("不合法的key值, 应为十六进制 (0-9 or a-f).");
		form.key3.focus();
		return 0;
	}
	return 1;
}

function postSecurity(encrypt, enable1X, wpaAuth, wpaPSKFormat, wpaPSK, rsPort, rsIpAddr, rsPassword, uCipher, wpa2uCipher, wepAuth, wepLen, wepKeyFormat) 
{	
	document.wlanSetup.security_method.value = encrypt;
	encrypt_value = encrypt;
	cipher_value = uCipher;
	wpa2cipher_value = wpa2uCipher;

//	if (encrypt == 2 || encrypt == 3)	/* ENCRYPT_WPA_TKIP or ENCRYPT_WPA_AES */
//		document.wlanSetup.security_method.value = 2;	/* WIFI_SEC_WPA */
//	else if (encrypt == 4 || encrypt == 5)	/* ENCRYPT_WPA2_TKIP or ENCRYPT_WPA2_AES */
//		document.wlanSetup.security_method.value = 3;	/* WIFI_SEC_WPA2 */
//	else if (encrypt == 6)	/* ENCRYPT_WPA2_MIXED */
//		document.wlanSetup.security_method.value = 4;	/* WIFI_SEC_WPA2_MIXED */
//	else if (encrypt == 7)	/* ENCRYPT_WAPI */
//		document.wlanSetup.security_method.value = 5;	/* WIFI_SEC_WAPI */
//	else	/* ENCRYPT_DISABLED or ENCRYPT_WEP */
//		document.wlanSetup.security_method.value = encrypt;	/* WIFI_SEC_NONE or WIFI_SEC_WEP */

	document.wlanSetup.pskFormat.value = wpaPSKFormat;
	document.wlanSetup.pskValue.value = wpaPSK;				

	if ( uCipher == 1 )
		document.wlanSetup.ciphersuite.value = 1;
	if ( uCipher == 2 )
		document.wlanSetup.ciphersuite.value = 2;
	if ( uCipher == 3 ) {
		document.wlanSetup.ciphersuite.value = 3;
	}

	if ( wpa2uCipher == 1 )
		document.wlanSetup.wpa2ciphersuite.value = 1;
	if ( wpa2uCipher == 2 )
		document.wlanSetup.wpa2ciphersuite.value = 2;
	if ( wpa2uCipher == 3 ) {
		document.wlanSetup.wpa2ciphersuite.value = 3;
	}	

	//document.wlanSetup.auth_type[wepAuth].checked = true;
	document.wlanSetup.auth_type.selectedIndex = wepAuth;
	
	if ( wepLen == 0 )
		document.wlanSetup.length0.value = 1;
	else
		document.wlanSetup.length0.value = wepLen;
	
	document.wlanSetup.format0.value = wepKeyFormat+1;			
	show_authentication();

	defPskLen = document.wlanSetup.pskValue.value.length;
	updateWepFormat(document.wlanSetup, 0);
}

function SSIDSelected(index)
{
	wlanMode = _wlan_mode[index];
	document.wlanSetup.isNmode.value = _wlan_isNmode[index];
	postSecurity(_encrypt[index], _enable1X[index],
		_wpaAuth[index], _wpaPSKFormat[index], _wpaPSK[index],
		_rsPort[index], _rsIpAddr[index], _rsPassword[index],
		_uCipher[index], _wpa2uCipher[index], _wepAuth[index],
		_wepLen[index], _wepKeyFormat[index]);
	document.wlanSetup.wpaSSID.length = 0;
//*
	if(ssidIdx==0)
		document.wlanSetup.wpaSSID.options[0] = new Option("SSID1", "0", false, false);
	else
		document.wlanSetup.wpaSSID.options[0] = new Option("SSID5", "0", false, false);
	document.wlanSetup.wpaSSID.length = 1;
}

/*End of Auth*/

function on_init()
{
	with (document.wlanSetup) {
	
		wlanEnabled.checked = wlan_module_enable;
		ssid.value = "0";
		txpower.selectedIndex = _txpower[0];
		hiddenSSID.checked = _hiddenSSID[0];
		elements.wlan_idx.value = 0;
		chanwid.selectedIndex = _chanwid[0];
		shortGI0.selectedIndex = _shortGI0[1];
		wlSecCbClick(wlanEnabled);
	}
	with (document.wlanSetup) {
		defaultKeyidx.selectedIndex = _defaultKeyidx[0];
		disableWPS.checked = !wscDisable[1];
	}
	
	if(document.wlanSetup.elements["disableWPS"].checked)
	{
		get_by_id("show_wps_button").style.display = "";
		get_by_id("disableWPS").value = "ON";
	}
	else
	{
		get_by_id("show_wps_button").style.display = "none";
		get_by_id("disableWPS").value = "OFF";
	}
	
	if(_WPS_running[1] == 1)
	{
		get_by_id("triggerPBC").value = "Stop WPS";
	}
	else
	{
		get_by_id("triggerPBC").value = "Start WPS";
	}
}

/********************************************************************
**          on document update
********************************************************************/
function wlSecCbClick(cb) 
{
	var status = cb.checked;
	with (document.wlanSetup) {
		wlSecAll.style.display = status ? "block" : "none";
	}	
}

function validateNum(str)
{
  for (var i=0; i<str.length; i++) {
        if ( !(str.charAt(i) >='0' && str.charAt(i) <= '9')) {
                alert("无效的值。它应该在十进制数字 (0-9)");
                return false;
        }
  }
  return true;
}

/********************************************************************
**          on document submit
********************************************************************/
function on_submit() 
{
	var str, band, basicRate = 0, operRate = 0, num;


		if (document.wlanSetup.wlanEnabled.checked) {
			str = document.wlanSetup.ssid.value;

			//if (str.length == 0) {
			//	alert("SSID不能为空.");
			//	return;
			//}

			if (str.length > 27) {
				alert('SSID "' + str + '" 不能大于32个字符。');
				return;
			}

			if (isIncludeInvalidChar(str)) {
				alert("SSID 含有非法字符，请重新输入!");
				return;
			}

			if ( validateNum(document.wlanSetup.beaconInterval.value) == 0 ) {
				document.wlanSetup.beaconInterval.focus();
				return;
			}

			num = parseInt(document.wlanSetup.beaconInterval.value);
			if (document.wlanSetup.beaconInterval.value=="" || num < 20 || num > 1024) {
				alert("Beacon信标帧发送间隔应介于20~1024之间");
				document.wlanSetup.beaconInterval.focus();
				return;
			}

			if ( validateNum(document.wlanSetup.dtimPeriod.value) == 0 ) {
				document.wlanSetup.dtimPeriod.focus();
				return;
			}

			num = parseInt(document.wlanSetup.dtimPeriod.value);
			if (document.wlanSetup.dtimPeriod.value=="" || num < 1 || num > 255) {
				alert("DTIM间隔隔应介于1~255之间");
				document.wlanSetup.dtimPeriod.focus();
				return;
			}


			band = parseInt(document.wlanSetup.band.value, 10) + 1;

			/* band:
			   bit 0 == 802.11b, bit 1 == 802.11g,
			   bit 2 == 802.11a, bit 3 == 802.11n
			*/

			/* basicRate, operRate:
			   bit 0 == 1M
			   bit 1 == 2M
			   bit 2 == 5M
			   bit 3 == 11M
			   bit 4 == 6M
			   bit 5 == 9M
			   bit 6 == 12M
			   bit 7 == 18M
			   bit 8 == 24M
			   bit 9 == 36M
			   bit 10 == 48M
			   bit 11 == 54M
			*/ 

			/* 802.11b */
			if (band & 1) {
				operRate |= 0xf;
				basicRate |= 0xf;
			}

			/* 802.11g, 802.11a */
			if ((band & 2) || (band & 4)) {
				operRate |= 0xff0;
				if  (!(band & 1))
					basicRate |= 0xff0;
			}

			/* 802.11n */
			if (band & 8) {
				if (!(band & 3))
					operRate |= 0xfff;

				if (band & 2)
					basicRate = 0x1f0;
				else
					basicRate = 0xf;
			}

			operRate |= basicRate;
			document.wlanSetup.basicrates.value = basicRate;
			document.wlanSetup.operrates.value = operRate;
		}

	form = document.wlanSetup;
	if (form.security_method.value == 0) {	/* WIFI_SEC_NONE */
		alert("警告: 未设定加密! 网路可能不安全!"); //Warning : security is not set!this may be dangerous!
	}
	
	if (form.security_method.value == 1) {	/* WIFI_SEC_WEP */
		if (check_wepkey() == false)
			return false;
		document.wlanSetup.disableWPS.checked = false;
		alert("通知: 当使用WEP时, WPS会被关闭!"); //Info : WPS will be disabled when using WEP!
	}
	else if (form.security_method.value == 2 || form.security_method.value == 4 || form.security_method.value == 6) {	/* WPA or WPA2 or WPA2 mixed */
		if (form.security_method.value == 2) {	/* WIFI_SEC_WPA */
 
			/*if (form.ciphersuite.value == 3) 
			{
				alert("不能同时选择TKIP and AES"); //Can't select TKIP and AES in the same time.
				return false;
			}*/
			document.wlanSetup.disableWPS.checked = false;
			alert("通知: 当使用WPA only时, WPS会被关闭!"); //Info : WPS will be disabled when using WPA only!
		}

    if (form.security_method.value == 4) {	/* WIFI_SEC_WPA2 */
 
			/*if (form.wpa2ciphersuite.value == 3) 
			{
				alert("不能同时选择TKIP and AES"); //Can't select TKIP and AES in the same time.
				return false;
			}*/
			
			if (form.wpa2ciphersuite.value == 1) 
			{
				if (wps20)
				{
					alert("通知: 当使用TKIP only时, WPS会被关闭!"); //Info : WPS will be disabled when using TKIP only!
					document.wlanSetup.disableWPS.checked = false;
				}
			}
    }
	if (form.security_method.value == 6) {	/* WIFI_SEC_WPA2_MIXED */
			//if (wlanMode == 1 && ((form.ciphersuite_t.checked == true && form.ciphersuite_a.checked == true)
			//	|| (form.wpa2ciphersuite_t.checked == true && form.wpa2ciphersuite_a.checked == true))) {
			/*if (wlanMode == 1 && ((form.ciphersuite.value == 3) || (form.wpa2ciphersuite.value == 3))) 
			{
				alert("在client模式, 不能同时选择TKIP and AES"); //In the Client mode, you can't select TKIP and AES in the same time.
				return false;				
			}*/
			
			if (wps20 && form.ciphersuite.value == 1 && form.wpa2ciphersuite.value == 1)			
			{
				alert("通知: 当皆使用TKIP only时, WPS会被关闭!"); //Info : WPS will be disabled when using TKIP only!
				document.wlanSetup.disableWPS.checked = false;
			}
		}

		var str = form.pskValue.value;
		if (form.security_method.value > 1) {
			if(str.length == 64){	
				for (var i=0; i<str.length; i++) {
					if ( (str.charAt(i) >= '0' && str.charAt(i) <= '9') ||
							(str.charAt(i) >= 'a' && str.charAt(i) <= 'f') ||
							(str.charAt(i) >= 'A' && str.charAt(i) <= 'F') )
							continue;
					alert("预共享密钥的值无效。它应该是在十六进制数字 (0-9 或 a 至 f)。");
					form.pskValue.focus();
					return false;
				}
				form.pskFormat.value = 1;
			}
			else{
				
				if (str.length < 8) {
					alert('預共享密钥至少8个字元'); //Pre-Shared Key value should be set at least 8 characters.
					form.pskValue.focus();
					return false;
				}
				if (str.length > 63) {
					alert('預共享密钥至多64个字元'); //Pre-Shared Key value should be less than 64 characters.
					form.pskValue.focus();
					return false;
				}
				if (checkString(form.pskValue.value) == 0) {
					alert('无效的預共享密钥'); //Invalid Pre-Shared Key !
					form.pskValue.focus();
					return false;
				}
				form.pskFormat.value = 0;
			}
		}
	}

	document.wlanSetup.submit();
}

function get_rate(curRate)
{
	var rate_mask = new Array(31,1,1,1,1,2,2,2,2,2,2,2,2,4,4,4,4,4,4,4,4,8,8,8,8,8,8,8,8);
	var rate_name = new Array("Auto","1M","2M","5.5M","11M","6M","9M","12M","18M","24M","36M","48M","54M", "MCS0", "MCS1",
		"MCS2", "MCS3", "MCS4", "MCS5", "MCS6", "MCS7", "MCS8", "MCS9", "MCS10", "MCS11", "MCS12", "MCS13", "MCS14", "MCS15");
	vht_rate_name=["NSS1-MCS0","NSS1-MCS1","NSS1-MCS2","NSS1-MCS3","NSS1-MCS4",
		"NSS1-MCS5","NSS1-MCS6","NSS1-MCS7","NSS1-MCS8","NSS1-MCS9",
		"NSS2-MCS0","NSS2-MCS1","NSS2-MCS2","NSS2-MCS3","NSS2-MCS4",
		"NSS2-MCS5","NSS2-MCS6","NSS2-MCS7","NSS2-MCS8","NSS2-MCS9"];
	var rate, mask,idx=0, i;
	var band = parseInt(document.wlanSetup.band.value, 10) + 1;
	var wlan_idx = document.wlanSetup.elements["wlan_idx"].value;
	var rf_num = _rf_used[wlan_idx];
	var auto = _auto[wlan_idx];
	
	// band:
	//   bit 0 == 802.11b, bit 1 == 802.11g,
	//  bit 2 == 802.11a, bit 3 == 802.11n
	//

	mask = 0;
	if (auto)
		curRate = 0;
	if (band & 1)
		mask |= 1;
	if ((band & 2) || (band & 4))
		mask |= 2;
	if (band & 8) {
		if (rf_num == 2)
			mask |= 12;	
		else
			mask |= 4;
	}
	if(band & 64){
		mask |= 16;
	}

	document.wlanSetup.txRate.length = 0;
	
	for (i = 0; i < rate_mask.length; i++) {
		if (rate_mask[i] & mask) {
			rate = (i == 0) ? 0 : 1 << (i - 1);
			document.wlanSetup.txRate.options[idx++] = new Option(rate_name[i], i, false, curRate == rate);
		}
	}
	if(band & 64){
		for (i=0; i<vht_rate_name.length; i++) {
			rate = (((1 << 31)>>>0) + i);
			document.wlanSetup.txRate.options[idx++] = new Option(vht_rate_name[i], (i+30), false, curRate == rate);
		}
	}
	
	if(_auto[1])  
	{
		document.wlanSetup.txRate.selectedIndex = 0;
	}
}

function showhidetr(element, show)
{
	var status;

	status = show ? "" : "none";

	if (document.getElementById) {
		// standard
		document.getElementById(element).style.display = status;
	} else if (document.all) {
		// old IE
		document.all[element].style.display = status;
	} else if (document.layers) {
		// Netscape 4
		document.layers[element].display = status;
	}
}

function updatePage(byHand)
{
	var band = parseInt(document.wlanSetup.band.value, 10) + 1;

	showhidetr("optionfor11n", (band >= 8) && document.wlanSetup.wlanEnabled.checked);
	showhidetr("ShortGI", (band >= 8) && document.wlanSetup.wlanEnabled.checked);
	showhidetr("SpeedID", (band != 4) && document.wlanSetup.wlanEnabled.checked);
		
	updateChannel(byHand);
}

function updateChannel(byHand)
{
	var idx_value= document.wlanSetup.band.selectedIndex;
	var band_value= parseInt(document.wlanSetup.band.value, 10) + 1; 

	/* Tsai: change channel by hand ? */
	if (byHand) {
		wlan_channel = parseInt(document.wlanSetup.chan.value, 10);
	} else {
		wlan_channel = _chan[document.wlanSetup.elements["wlan_idx"].value];
	}

	showcontrolsideband_updated(band_value, idx_value);
	showchannelbound_updated(band_value, byHand);

	updateChannelBound();

	/* Tsai: update wlan_channel because chan may be changed */
	wlan_channel = parseInt(document.wlanSetup.chan.value, 10);

	if (document.wlanSetup.chanwid.selectedIndex == 0 || wlan_channel == 0)
		document.wlanSetup.ctlband.disabled = true;
	else
		document.wlanSetup.ctlband.disabled = false;

	if(document.wlanSetup.chan.disabled == true)
		document.wlanSetup.ctlband.disabled = true;
}

function updateChannel2()
{
	var idx_value= document.wlanSetup.band.selectedIndex;
	var band_value= parseInt(document.wlanSetup.band.value, 10) + 1; 

	wlan_channel = parseInt(document.wlanSetup.chan.value, 10);

	showcontrolsideband_updated(band_value, idx_value);

	updateChannelBound();

	/* Tsai: update wlan_channel because chan may be changed */
	wlan_channel = parseInt(document.wlanSetup.chan.value, 10);

	if (document.wlanSetup.chanwid.selectedIndex == 0 || wlan_channel == 0)
		document.wlanSetup.ctlband.disabled = true;
	else
		document.wlanSetup.ctlband.disabled = false;

	if(document.wlanSetup.chan.disabled == true)
		document.wlanSetup.ctlband.disabled = true;
}

function updateChan_selectedIndex()
{
	var chan_number_idx=document.wlanSetup.chan.selectedIndex;
	var chan_number= document.wlanSetup.chan.options[chan_number_idx].value;
	var band_value= parseInt(document.wlanSetup.band.value, 10);

	wlan_channel = chan_number;
	if(chan_number == 0)
		disableTextField(document.wlanSetup.ctlband);	
	else{
		if(document.wlanSetup.chanwid.selectedIndex == "0")
 			disableTextField(document.wlanSetup.ctlband);	
		else if(document.wlanSetup.chanwid.selectedIndex == "2" && (band_value == 75 || band_value == 71|| band_value ==63))
 			disableTextField(document.wlanSetup.ctlband);
 		else
			enableTextField(document.wlanSetup.ctlband);		
	}

	if((wlan_support_8812e==1) && (chan_number > 14)) //8812
		disableTextField(document.wlanSetup.ctlband);	
}

function updateChannelBound()
{
	var band = parseInt(document.wlanSetup.band.value, 10) + 1;
	var bound = document.wlanSetup.chanwid.selectedIndex;	/* 0:20Mhz, 1:40Mhz */
	var adjust;
	var idx_value= document.wlanSetup.band.selectedIndex;

	/* band:
	   bit 0 == 802.11b, bit 1 == 802.11g,
	   bit 2 == 802.11a, bit 3 == 802.11n
	*/

	/* Tsai: 802.11n */
	adjust = (band & 8) ? bound : 0;

	if (band & 4 || (band == 8 && idx_value == 1) || (band  & 64)) {
		/* Tsai: 802.11a and 802.11n and 802.11ac */
		showChannel5G(adjust);
		document.wlanSetup.chan.disabled = false;
	} else {
		/* Tsai: 802.11b, 802.11g and 802.11n */
		adjust = (adjust != 0) ? 1 : 0;
		showChannel2G(band, adjust);
		document.wlanSetup.chan.disabled = false;
	}
}

function showChannel5G(bound_40)
{
	var chan, idx = 0;

	/* Tsai: remove all options under chan */
	document.wlanSetup.chan.length = 0;

	if (dfs_enable == 1)		
			document.wlanSetup.chan.options[idx++] = new Option("Auto", 0, false, 0 == wlan_channel);      //Use DFS but not show "Auto(DFS)"
	else
			document.wlanSetup.chan.options[idx++] = new Option("Auto", 0, false, 0 == wlan_channel);


	if(wlan_support_8812e ==1)
	{
		var bound = document.wlanSetup.chanwid.selectedIndex;
		var channel_list;
		var chan, chan_pair, i, ii, iii, iiii, found; 


		if(regDomain==13) 
		{
			channel_list = channel_list_5g_dfs[regDomain];
			
			if(bound <= 2){
				for(i=0; i<channel_list.length; i++){
					chan = channel_list[i];
					document.wlanSetup.chan.options[idx] = new Option(chan, chan, false, chan == wlan_channel);
					idx++;
				}
			}
			
		}
		else{

			channel_list = channel_list_5g_dfs[regDomain];
			
			for (i=0; i<channel_list.length; i++) {
				
					chan = channel_list[i];
								
					if(regDomain != 16 && regDomain != 11)
						if((dfs_enable == 0) && (chan >= 52) && (chan <= 144))
							continue;

               		if(regDomain == 11)
                		if((dfs_enable == 0) && (chan >= 100) && (chan <= 140))
                    		continue;

					if(regDomain != 16){
						if(bound==1){
							for(ii=0; ii<channel_list_5g_dfs[16].length; ii++){
								if(chan == channel_list_5g_dfs[16][ii])
									break;
							}
							if(ii%2==0)
								chan_pair = channel_list_5g_dfs[16][ii+1];
							else
								chan_pair = channel_list_5g_dfs[16][ii-1];
							
							found = 0;
							for(ii=0; ii<channel_list.length; ii++){
								if(chan_pair == channel_list[ii]){
									found = 1;
									break;
								}
							}
							if(found == 0)
								chan=0;
						}
						else if(bound==2){
							for(ii=0; ii<channel_list_5g_dfs[16].length; ii++){
								if(chan == channel_list_5g_dfs[16][ii])
									break;
							}
							for(iii=(ii-(ii%4)); iii<((ii-(ii%4)+3)) ; iii++)
							{
								found = 0;
								chan_pair = channel_list_5g_dfs[16][iii];
								for(iiii=0; iiii<channel_list.length; iiii++){
									if(chan_pair == channel_list[iiii]){
										found = 1;
										break;
									}
								}
								if(found==0){
									chan=0;
									break;
								}
							}

							
						}
					}
					
					if(chan!=0){
						document.wlanSetup.chan.options[idx] = new Option(chan, chan, false, chan == wlan_channel);
						idx++;
					}
			}
		}
	/*
		var bound = document.wlanSetup.chanwid.selectedIndex;
		var channel_list;
		if(dfs_enable == 1) // use dfs channel
			channel_list = channel_list_5g_dfs[regDomain];
		else
			channel_list = channel_list_5g[regDomain];

		for (chan = 0; chan < channel_list.length; idx ++, chan ++) {
			if(bound != 0 && channel_list[chan] == 165) // channel 165 only exist 20M channel bandwidth
				break;
			document.wlanSetup.chan.options[idx] = new Option(channel_list[chan], channel_list[chan], false, false);
			if (channel_list[chan] == defaultChan) {
				document.wlanSetup.chan.selectedIndex = idx;
				defChanIdx = idx;
			}
		}
	*/
	/*
		var bound = document.wlanSetup.chanwid.selectedIndex;
		var inc_scale;
		var chan_end;
		
		inc_scale = 4;
		chan_str = 36;

		if (dfs_enable == 1)
			chan_end = 64;
		else
			chan_end = 48;

		for (chan = chan_str; chan <= chan_end; idx ++, chan += inc_scale) {
			document.wlanSetup.chan.options[idx] = new Option(chan, chan, false, false);
			if (chan == defaultChan) {
				document.wlanSetup.chan.selectedIndex = idx;
				defChanIdx = idx;
			}
		}
		if (dfs_enable == 1)
		{
			chan_str = 149;
			chan_end = 161;
			var bound = document.wlanSetup.chanwid.selectedIndex;
			if(bound == 0) // 20MHz
				chan_end = 165;
		
			for (chan = chan_str; chan <= chan_end; idx ++, chan += inc_scale) {
				document.wlanSetup.chan.options[idx] = new Option(chan, chan, false, false);
				if (chan == defaultChan) {
					document.wlanSetup.chan.selectedIndex = idx;
					defChanIdx = idx;
				}
			}
		}
	*/
	}
	else
	if (regDomain == 6) {	// MKK
		for (chan = 34; chan <= 64; chan += 2) {
			if ((chan == 50) || (chan == 54) || (chan == 58) || (chan == 62)) {
				continue;
			}

			document.wlanSetup.chan.options[idx++] = new Option(chan, chan, false, chan == wlan_channel);
		}
	} else {
		var start, end;

		if (bound_40 == 1) {
			var sideBand = document.wlanSetup.ctlband.selectedIndex;

			if (sideBand == 0) {	// upper
				start = 40;
				end = 64;
			} else {		// lower
				start = 36;
				end = 60;
			}
		} else {
			start = 36;
			end = 64;
		}

		for (chan = start; chan <= end; chan += (4 * (bound_40 + 1))) {
			document.wlanSetup.chan.options[idx++] = new Option(chan, chan, false, chan == wlan_channel);
		}
	}
}

function showChannel2G(band, bound_40)
{
	var start = 0, end = 0, idx = 0;

	/* band:
	   bit 0 == 802.11b, bit 1 == 802.11g,
	   bit 2 == 802.11a, bit 3 == 802.11n
	*/

	if (regDomain == 1 || regDomain == 2) {
		start = 1;
		end = 11;
	} else if (regDomain == 3) {
		start = 1;
		end = 13;
	} else if (regDomain == 4) {
		start = 1;
		end = 13;
	} else if (regDomain == 5) {
		start = 10;
		end = 13;
	} else if (regDomain == 6) {
		start = 1;
		end = 14;
	} else if (regDomain == 7) {
		start = 3;
		end = 13;
	} else if (regDomain == 8 || regDomain == 9 || regDomain == 10) {
		start = 1;
		end = 14;
	} else if (regDomain == 11) {
		start = 1;
		end = 11;
	} else if (regDomain == 12) {
		start = 1;
		end = 13;
	} else if (regDomain == 13) {
		start = 1;
		end = 13;
	} else if (regDomain == 14) {
		start = 1;
		end = 14;
	} else if (regDomain == 15) {
		start = 1;
		end = 13;
	} else if (regDomain == 16) {
		start = 1;
		end = 14;
	} else {		//wrong regDomain ?
		start = 1;
		end = 11;
	}

	/* Tsai: 802.11n */
	if (band & 8) {
		if (bound_40 == 1) {
			var sideBand = document.wlanSetup.ctlband.selectedIndex;

			if (regDomain == 5) {
				if (sideBand == 0) {	//upper
					start = 13;
					end = 13;
				} else {		//lower
					start = 10;
					end = 10;
				}
			} else {
				if(sideBand == 0){  //upper
					start = start+4;
					if(end == 14)
						end = 13			
					
				}else{ //lower
					if(end == 14)
						end = 13;
					end = end-4;
				}
			}
		}
	}

	/* Tsai: remove all options under chan */
	document.wlanSetup.chan.length = 0;

	document.wlanSetup.chan.options[idx++] = new Option("Auto", 0, false, 0 == wlan_channel);

	for (chan = start; chan <= end; chan++) {
		document.wlanSetup.chan.options[idx++] = new Option(chan, chan, false, chan == wlan_channel);
	}
}

function updateBand(band)
{
	var idx=0;
	var select_idx = 1;   //5G
	var Band2G5GSupport = _Band2G5GSupport[select_idx];

	document.wlanSetup.band.length = 0;

	if(Band2G5GSupport == 2 || wlBandMode == 1) // 2:PHYBAND_5G 1:BANDMODESIGNLE
	{
		document.wlanSetup.band.options[idx++] = new Option("802.11a", "3", false, false);
        document.wlanSetup.band.options[idx++] = new Option("802.11n/a混合", "11", false, false);

		if(wlan_support_8812e==1){
			document.wlanSetup.band.options[idx++] = new Option("802.11ac/n/a混合", "75", false, false); //8812
		}
	}

	if(Band2G5GSupport == 1 || wlBandMode == 1) // 1:PHYBAND_2G 1:BANDMODESIGNLE
	{
		document.wlanSetup.band.options[idx++] = new Option("802.11b/g/n 混合", "10", false, false);
		document.wlanSetup.band.options[idx++] = new Option("802.11b/g 混合", "2", false, false);
		document.wlanSetup.band.options[idx++] = new Option("802.11n", "7", false, false);
		document.wlanSetup.band.options[idx++] = new Option("802.11g", "1", false, false);
		document.wlanSetup.band.options[idx++] = new Option("802.11b", "0", false, false);
	}

	for (idx = 0; idx < document.wlanSetup.band.length; idx++) {
		if (document.wlanSetup.band.options[idx].value == band) {
			/* Tsai: 802.11n */
			if (band == 7) {
				var c = document.wlanSetup.band.options[idx].text.charAt(0);

				if ((Band2G5GSupport == 2 && c == "5")		//Band2G5GSupport=2:PHYBAND_5G
					|| (Band2G5GSupport == 1 && c == "2")) {		//Band2G5GSupport=1:PHYBAND_2G
					document.wlanSetup.band.selectedIndex = idx;
					break;
				}
			} else {
				document.wlanSetup.band.selectedIndex = idx;
				break;
			}
		}
	}
}

function Set_SSIDbyBand()
{
	var c;

	with (document.wlanSetup) {
		if(typeof band.options[band.selectedIndex] != "undefined") //*
			c = band.options[band.selectedIndex].text.charAt(0);

		if (c == "5")
			ssid.value = ssid_5g;
		else if (c == "2")
			ssid.value = ssid_2g;
	}
}

function showcontrolsideband_updated(band, idx_value)
{
	var idx=0;
	var i;
  	var controlsideband_idx = document.wlanSetup.ctlband.selectedIndex;
	var controlsideband_str = document.wlanSetup.ctlband.options[controlsideband_idx].value;

	document.wlanSetup.ctlband.length = 0;

  	if(wlan_support_8812e ==1 && ((band==8 && idx_value==1) || band ==12 || band==64 || band==72 || band ==76))
	{
		document.wlanSetup.ctlband.options[idx++] = new Option("Auto", "0", false, false);
		document.wlanSetup.ctlband.options[idx++] = new Option("Auto", "1", false, false);
  	}
  	else
	{
		document.wlanSetup.ctlband.options[idx++] = new Option("Upper", "0", false, false);
		document.wlanSetup.ctlband.options[idx++] = new Option("Lower", "1", false, false);
  	}
  	document.wlanSetup.ctlband.length = idx;
 
	//document.wlanSetup.ctlband.selectedIndex = controlsideband_idx;

	for (i=0; i<idx; i++) {
		if(controlsideband_str == document.wlanSetup.ctlband.options[i].value)
			document.wlanSetup.ctlband.selectedIndex = i;
	}
}


function showchannelbound_updated(band, byHand)
{
	var idx=0;
	var i;
	var channelbound_idx;
//	var channelbound_str = document.wlanSetup.chanwid.options[channelbound_idx].value;
	var idx_value = document.wlanSetup.band.selectedIndex;

	if(byHand)
		channelbound_idx = document.wlanSetup.chanwid.selectedIndex;
	else
		channelbound_idx = _chanwid[document.wlanSetup.elements["wlan_idx"].value]; 

	document.wlanSetup.chanwid.length = 0;

	document.wlanSetup.chanwid.options[idx++] = new Option("20MHz", "0", false, false);
	document.wlanSetup.chanwid.options[idx++] = new Option("40MHz", "1", false, false);

	if (band & 4 || (band == 8 && idx_value == 1) || (band & 64)) {
		if(band & 64)
			document.wlanSetup.chanwid.options[idx++] = new Option("80MHz", "2", false, false);
	}
	else{
		document.wlanSetup.chanwid.options[idx++] = new Option("20/40MHz", "2", false, false);
	}
	
 	document.wlanSetup.chanwid.length = idx;
 
	if(channelbound_idx > (idx-1))
		document.wlanSetup.chanwid.selectedIndex = idx - 1;
	else
		document.wlanSetup.chanwid.selectedIndex = _chanwid[1];

	if( document.wlanSetup.band.selectedIndex==1 && _chanwid[1]==2 )
	{
		document.wlanSetup.chanwid.selectedIndex = 1;
	}
}


function wlRatePriorCbClick(cb) 
{
	
	var status = cb.checked;
	var idx = document.wlanSetup.elements["wlan_idx"].value;

	with (document.wlanSetup) {
		band.disabled = status? true : false;
		chanwid.disabled = status? true : false;
		if(status){
			if(_Band2G5GSupport[idx]==2)
				updateBand(63); //ac
			else
				updateBand(7);
			get_rate(_txRate[idx]);
			chanwid.selectedIndex = 2;
			updateChannel(true);
		}
		else{
			updateBand(_band[idx]);
			get_rate(_txRate[idx]);
			updateChannel(false);
		}
  	}
		
}


function showtxpower_updated(band, value, value_high)
{
	var idx=0;
	var idx_value = document.wlanSetup.band.selectedIndex;
	var i;

	document.wlanSetup.txpower.length = 0;
	if(wlan_txpower_high_enable){
		if (band & 4 || (band == 8 && idx_value == 1) || (band & 64)){
			document.wlanSetup.txpower.options[idx++] = new Option("100%", "0", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("70%", "1", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("50%", "2", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("35%", "3", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("15%", "4", false, false);
		}
		else{
			document.wlanSetup.txpower.options[idx++] = new Option("200%", "5", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("100%", "0", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("70%", "1", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("50%", "2", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("35%", "3", false, false);
			document.wlanSetup.txpower.options[idx++] = new Option("15%", "4", false, false);
			if(value == 0 && value_high == 1){
				value = 5;
			}
		}
	}
	else{
		document.wlanSetup.txpower.options[idx++] = new Option("100%", "0", false, false);
		document.wlanSetup.txpower.options[idx++] = new Option("80%", "1", false, false);
		document.wlanSetup.txpower.options[idx++] = new Option("60%", "2", false, false);
		document.wlanSetup.txpower.options[idx++] = new Option("40%", "3", false, false);
		document.wlanSetup.txpower.options[idx++] = new Option("20%", "4", false, false);
	}
	document.wlanSetup.txpower.length = idx;
	
	for (i=0; i<idx; i++) {
		if(value == document.wlanSetup.txpower.options[i].value)
			document.wlanSetup.txpower.selectedIndex = i;
	}
}

function wlEnableSSIDClick(cb) 
{
	
	var status = cb.checked;

	var band_value = parseInt(document.wlanSetup.band.value, 10) + 1;

	with (document.wlanSetup) {
		wlSecInfo.style.display = status ? "block" : "none";
		optionfor11n.style.display = (status && (band_value >= 8)) ? "block" : "none";
		ShortGI.style.display = (status && (band_value >= 8)) ? "" : "none";
		adminWlinfo.style.display = status ? "block" : "none";
		adminWlinfo_1.style.display = status ? "block" : "none";

		//if(wlanEnabled.checked==false)
		//	advanced.style.display = "none";
		//else
		//	advanced.style.display = status ? "block" : "none";

	}
}

function BandSelected(index)
{
	var chan_selIdx = 0;
	
	document.wlanSetup.band.value = _band[index];
	document.wlanSetup.elements["wlan_idx"].value = index;
	document.wlanSetup.wlSsidIdx.length = 0;
	if(index==0)
		document.wlanSetup.wlSsidIdx.options[0] = new Option("SSID1", "0", false, false);
	else
		document.wlanSetup.wlSsidIdx.options[0] = new Option("SSID5", "0", false, false);
	document.wlanSetup.wlSsidIdx.length = 1;
	if(wlan_sta_control==1 && index==1)
		document.wlanSetup.ssid.disabled = true;
	else
		document.wlanSetup.ssid.disabled = false;
	document.wlanSetup.ssid.value = _ssid[index];
	document.getElementById('cur_wlChannel').innerHTML = '当前信道: '+ _wlCurrentChannel[index];
	document.wlanSetup.hiddenSSID.checked = _hiddenSSID[index];
	document.wlanSetup.chanwid.selectedIndex = _chanwid[index];
	document.wlanSetup.ctlband.selectedIndex = _ctlband[index];
	document.wlanSetup.shortGI0.selectedIndex = _shortGI0[index];
	
	if(document.wlanSetup.elements.namedItem("dot11kEnabled")!= null)	
		document.wlanSetup.dot11kEnabled[_wlan_11k[index]==0?1:0].checked = true;
	if(document.wlanSetup.elements.namedItem("dot11vEnabled")!= null){
		document.wlanSetup.dot11vEnabled[_wlan_11v[index]==0?1:0].checked = true;
		document.getElementById("dot11v").style.display = _wlan_11k[index]? "" : "none";
	}
	document.wlanSetup.enableSSID.checked = _wlanEnabled[index]? true: false;
	
	txrate = _txRate[index];
	auto = _auto[index];
	updateBand(_band[index]);
	get_rate(txrate);
	updatePage(false);
	Set_SSIDbyBand();

	showtxpower_updated(_band[index], _txpower[index], _txpower_high[index]);

	/*var status = _wlanEnabled[index];*/
	var status = 1;  
	var band_value = parseInt(document.wlanSetup.band.value, 10) + 1;

	with (document.wlanSetup) {
		wlSecInfo.style.display = status ? "block" : "none";
		optionfor11n.style.display = (status && (band_value >= 8)) ? "block" : "none";
		ShortGI.style.display = (status && (band_value >= 8)) ? "" : "none";
		SpeedID.style.display = (status && (band_value != 4)) ? "" : "none";
		adminWlinfo.style.display = status ? "block" : "none";
		adminWlinfo_1.style.display = status ? "block" : "none";

		wlRatePrior.style.display = wlan_rate_prior_enable ? "block" : "none";
		wlanRatePrior.checked = _wlan_rate_prior[index];
		if(wlan_rate_prior_enable == 1 && _wlan_rate_prior[index]==1){
			band.disabled = true;
			if(_Band2G5GSupport[index]==2)
				updateBand(63); //ac
			else
				updateBand(7);
			get_rate(txrate);
			chanwid.disabled = true;
			chanwid.selectedIndex = 2;
			updateChannel(true);
		}
		else{
			band.disabled = false;
			chanwid.disabled = false;
		}
		wlBand.style.display = user_mode ? "" : "none";
		ShortGI.style.display = user_mode ? "" : "none";
		beaconInterval.value = _wlan_beacon_interval[index];
		dtimPeriod.value = _wlan_dtim_period[index];
		
		for (chan_selIdx=0; chan_selIdx<15; chan_selIdx++) {
			if(chan.options[chan_selIdx].value == _chan[index])
			{
				chan.selectedIndex = chan_selIdx;
				break;
			}
		}

  	}
}


</script>
</head>
   
<!-------------------------------------------------------------------------------------->
<!--主页代码-->

<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onload="BandSelected(1)">
	<blockquote>
	<DIV align="left" style="padding-left:20px; padding-top:10px"><table>
	<form action=/boaform/admin/formWlanSetup method="post" name="wlanSetup">
	<table border="0" cellpadding="4" cellspacing="0">
		<tr>
			<td>功能开关</td>
			<td valign="middle" align="center" width="30" height="30">
				<input type='checkbox' name='wlanEnabled' onClick='wlSecCbClick(this);' value="ON"></td>
		</tr>
	</table>
	<br>
	<div id='wlSecAll'>
	<div id='wlSecInfoDualband'>
	
	<script>//BandSelected(1);</script>
	</div>
	<div id='wlSecInfo'>
	<table border="0" cellpadding="4" cellspacing="0">
		<tr id="wlRatePrior" style="display:none">
			<td valign="middle" align="center" width="30" height="30">
				<input type='checkbox' name='wlanRatePrior' onClick='wlRatePriorCbClick(this);' value="ON"></td>
			<td>启用速率优先</td>
		</tr>
	</table>
	<table border="0" cellpadding="4" cellspacing="0" width="400">
		<tr id="wlBand" style="display: none"> 
			<td width="45%">模式选择:</td>
			<td colspan="2">
				<select style="width: 180px" name=band size="1" onChange="updatePage(true); Set_SSIDbyBand(); get_rate(txrate);">
					<option value=3>5 GHz (A)</option>
<option value=7>5 GHz (N)</option>
<option value=11>5 GHz (A+N)</option>
<option value=63>5 GHz (AC)</option>
<option value=71>5 GHz (N+AC)</option>
<option value=75>5 GHz (A+N+AC)</option>

				</select>
				<!--<SCRIPT>updateBand(defaultBand);</SCRIPT>-->
			</td>
		</tr>
	</table>
	</div>
	<div id="adminWlinfo_1"  style="display:none">
	<table border="0" cellpadding="4" cellspacing="0" width="400">
		<tr>
			<td width="45%">信道选择:</td>
			<td><select style="width: 180px" name='chan' onChange="updateChan_selectedIndex()">
			</select></td>
			<td id='cur_wlChannel' width="26%" style="display: none">当前信道:
				<script language="javascript">
					document.writeln(cgi_wlCurrentChannel);
				</script>
			</td>
		</tr>
		<tr> 
			<td>发射功率调节:</td>
			<td colspan="2"><select style="width: 180px" name="txpower" size="1">
				<option value="0">100%</option>
<option value="1">70%</option>
<option value="2">50%</option>
<option value="3">35%</option>
<option value="4">15%</option>
 
			</select></td> 
		</tr>
	</table>
	</div>
	<table border="0" cellpadding="4" cellspacing="0"  width="400">
		<tr>
			<td width="45%">SSID 索引:</td>
			<td colspan="2"><select style="width: 180px" name="wlSsidIdx">
				<option value="0">SSID1</option>
			</select></td>
		</tr>
		<tr>
			<td width="45%">SSID名称:</td>
			<td colspan="2">
			<input size="10" style="WIDTH: 180px" name='ssid' maxlength="27" value=""></td>
		</tr>
	</table>
	<div id="optionfor11n" style="display:none">
	<table border="0" cellpadding="4" cellspacing="0" width="420">
		<tr id="ChannelMode">
			<td width="43%">频宽模式选择:</td>
			<td colspan="2"><select style="width: 180px" size="1" name="chanwid" onChange="updateChannel2()">
				<option value="0">20MHZ</option>
<option value="1">40MHZ</option>
<option value="2">80MHZ</option>

				</select> MHZ
			</td>
		</tr>
		<tr>
			<td style="display:none;">控制边带:</td>
			<td colspan="2" style="display:none;"><select size="1" name="ctlband" onChange="updateChannelBound()">
				<option value="0">Upper</option>
<option value="1">Lower</option>
      		
				</select>
			</td>
		</tr> 
	</table>
	</div>
	<div id="adminWlinfo"  style="display:none">
	<table border="0" cellpadding="4" cellspacing="0" width="400">
		<tr id="SpeedID"> 
			<td width="45%">速率:</td>
			<td colspan="2"><select style="width: 180px" size="1" name="txRate">
				<!--<SCRIPT>
					get_rate(txrate);
				</SCRIPT> -->
			</select></td> 
		</tr>
		<tr id="ShortGI">
			<td width="45%">保护间隔:</td>
			<td colspan="2">
				<select style="width: 180px" size="1" name="shortGI0">
				<option value="0">Long</option>
<option value="1">Short</option>

				</select>
			</td>
		</tr>
		<tr>
			<td>Beacon信标帧发送间隔:</td>
			<td colspan="2">
			<input size="10" style="WIDTH: 180px" type='text' name="beaconInterval" size="10" maxlength="4">
			</td>
		</tr>
		<tr>
			<td>DTIM间隔:</td>
			<td colspan="2">
			<input size="10" style="WIDTH: 180px" type='text' name="dtimPeriod" size="5" maxlength="3">
			</td>
		</tr>		
		<tr>
			<td width="45%">SSID使能:</td>
			<td valign="middle" width="30" height="30">
				<input type='checkbox' name='enableSSID' value="ON"></td>
		</tr>
		<tr>
			<td>广播取消:</td>
			<td valign="middle" width="30" height="30">
				<input type='checkbox' name='hiddenSSID' value="ON"></td>
		</tr>
	</table>          
	</div> 	
			<input type="hidden" name="wlan_idx" value=1>
			<input type="hidden" name="basicrates" value=0>
			<input type="hidden" name="operrates" value=0>
		<script>
				_Band2G5GSupport[0]=1;
_Band2G5GSupport[2]=1;
_wlCurrentChannel[0]=9;
_wlCurrentChannel[2]=9;
_wlan_11k[0]=0;
_wlan_11k[2]=0;
_wlan_11v[0]=0;
_wlan_11v[2]=0;
_bssid[0]='00:e0:4c:86:70:01';
_bssid[2]='00:e0:4c:86:70:01';
_wlanEnabled[0]=true;
_wlanEnabled[2]=false;
_hiddenSSID[0]=false;
_ssid[0]='0';
_hiddenSSID[2]=false;
_ssid[2]='CMCC-AP2';
_txpower[0]=0;
_txpower_high[0]=0;
_txpower[2]=0;
_txpower_high[2]=0;
_chan[0]=0;
_chan[2]=0;
_band[0]=10;
_band[2]=10;
_chanwid[0]=0;
_chanwid[2]=0;
_ctlband[0]=0;
_ctlband[2]=0;
_txRate[0]=1;
_auto[0]=1;
_rf_used[0]=2;
_shortGI0[0]=0;
_txRate[2]=1;
_auto[2]=1;
_rf_used[2]=2;
_shortGI0[2]=0;
_defaultKeyidx[0]=0;
_defaultKeyidx[2]=0;
wscDisable[0]=0;
wscDisable[2]=1;
_WPS_running[0]=0;
_WPS_running[2]=0;
_wlan_beacon_interval[0]=100;
_wlan_beacon_interval[2]=100;
_wlan_dtim_period[0]=1;
_wlan_dtim_period[2]=1;
_Band2G5GSupport[1]=2;
_Band2G5GSupport[3]=2;
_wlCurrentChannel[1]=149;
_wlCurrentChannel[3]=149;
_wlan_11k[1]=0;
_wlan_11k[3]=0;
_wlan_11v[1]=0;
_wlan_11v[3]=0;
_bssid[1]='00:e0:4c:86:70:05';
_bssid[3]='00:e0:4c:86:70:05';
_wlanEnabled[1]=true;
_wlanEnabled[3]=false;
_hiddenSSID[1]=false;
_ssid[1]='0';
_hiddenSSID[3]=false;
_ssid[3]='CMCC-AP6';
_txpower[1]=0;
_txpower_high[1]=0;
_txpower[3]=0;
_txpower_high[3]=0;
_chan[1]=0;
_chan[3]=0;
_band[1]=75;
_band[3]=75;
_chanwid[1]=2;
_chanwid[3]=2;
_ctlband[1]=0;
_ctlband[3]=0;
_txRate[1]=1;
_auto[1]=1;
_rf_used[1]=2;
_shortGI0[1]=0;
_txRate[3]=1;
_auto[3]=1;
_rf_used[3]=2;
_shortGI0[3]=0;
_defaultKeyidx[1]=0;
_defaultKeyidx[3]=0;
wscDisable[1]=0;
wscDisable[3]=1;
_WPS_running[1]=0;
_WPS_running[3]=0;
_wlan_beacon_interval[1]=100;
_wlan_beacon_interval[3]=100;
_wlan_dtim_period[1]=1;
_wlan_dtim_period[3]=1;

		</script>	
	<table border="0" cellpadding="0" cellspacing="0" width="400">
		<input type=hidden name="isNmode" value=0>
		<tr>
			<td width="35%" style="display:none;"> SSID索引: </td>
			<td style="display:none;"><select name="wpaSSID">
				<option value="0">SSID1</option>
				<!--% SSID_select %-->
			</select></td>
		</tr>
		<tr>
			<td>
				<table width="400" border="0" cellpadding="4" cellspacing="0">	
						<td width="45%">安全设置:</td>
						<td colspan="2"><select style="width: 180px" size="1" id="security_method" name="security_method" onChange="show_authentication(), encrypChange()">
								<option value=0>None</option>
<option value=1>WEP</option>
<option value=2>WPA-PSK</option>
<option value=4>WPA2-PSK</option>
<option value=6>WPA-PSK/WPA2-PSK</option>
 
							</select>
						<!--	<select style="display:none" size="1" id="method" name="method" onChange="show_authentication()">
								<option value=0>None</option>
<option value=1>WEP</option>
<option value=3>WPA(AES)</option>
<option value=4>WPA2(AES)</option>
<option value=6>WPA2 Mixed</option>

							</select> -->
						</td>
				</table>
			</td>
		</tr>
		<tr id="show_wep_auth" style="display:none">
			<td>
				<table width="400" border="0" cellpadding="4" cellspacing="0">
					<tr>
						<td width="45%">WEP 认证:</td> 
						<td colspan="2">
							<select style="width: 180px" size="1" name="auth_type">
							<option value="open">open</option>
<option value="shared">shared</option>
<option value="both">open+share</option>

						</td>
					</tr>
				</table>
			</td>
		</tr>
		<tr id="setting_wep" style="display:none">
			<td colspan="2" width="100%">
				<table width="400"  border="0" cellspacing="0" cellpadding="4" >
					<input type="hidden" name="wepEnabled" value="ON" checked>
					<tr >
						<td width="45%">加密长度:</td>
						<td colspan="2"><select style="width: 180px" size="1" name="length0" id="length" onChange="updateWepFormat(document.wlanSetup, 0)">
							<option value=1> 64-bit</option>
							<option value=2>128-bit</option>
						</select></td>
					</tr>
					<tr>
						<td width="45%">输入密钥型态:</td>
						<td colspan="2"><select style="width: 180px" size="1" id="format" name="format0" onChange="setDefaultKeyValue(document.wlanSetup, 0)">
							<option value="1">ASCII</option>
							<option value="2">Hex</option>
						</select></td>
					</tr>
					<tr>
						<td width="45%">当前密钥索引:</td>
						<td colspan="2"><select style="width: 180px" size="1" id="defaultKeyidx" name="defaultKeyidx"> 
							<option value="1">1</option>
							<option value="2">2</option>
							<option value="3">3</option>
							<option value="4">4</option>
						</select></td>
					</tr>
					<tr>
						<td width="45%">密钥 1:</td>
						<td colspan="2">
						<input size="10" style="WIDTH: 180px" type="text" id="key" name="key0" maxlength="26" size="26" value=""></td>
					</tr>
					<tr>
						<td width="45%">密钥 2:</td>
						<td colspan="2">
						<input size="10" style="WIDTH: 180px" type="text" id="key_1" name="key1" maxlength="26" size="26" value=""></td>
					</tr>
					<tr>
						<td width="45%">密钥 3:</td>
						<td colspan="2">
						<input size="10" style="WIDTH: 180px" type="text" id="key_2" name="key2" maxlength="26" size="26" value=""></td>
					</tr>
					<tr>
						<td width="45%">密钥 4:</td>
						<td colspan="2">
						<input size="10" style="WIDTH: 180px" type="text" id="key_3" name="key3" maxlength="26" size="26" value=""></td>
					</tr>
					<script language=JavaScript type=text/javascript>
						form = document.wlanSetup;
						form.key0.value = "";
						form.key1.value = "";
						form.key2.value = "";
						form.key3.value = "";
					</script>
				</table>	
			</td>
		</tr>
		<tr id="setting_wpa" style="display:none">
			<td colspan="2" width="100%">
				<table width="400" border="0" cellpadding="4" cellspacing="0">				
					<input type="hidden" name="wpaAuth" value="psk" checked>
					<tr id="show_wpa_psk2" style="display:none">
						<td width="45%">WPA 预认证共享密钥:</td>
						<td colspan="2">
						<input size="10" style="WIDTH: 180px" type="password" name="pskValue" id="wpapsk" maxlength="27" value=""></td>
						<!--td width="50%"><A HREF="javascript:wpapsk_window()">点击这里显示</A></td-->
					</tr>
					<tr id="show_wpa_cipher" style="display:none">
						<td width="45%">加密/认证配置(WPA):</td>
						<td colspan="2"><select style="width: 180px" size="1" name="ciphersuite" id="ciphersuite">
							<option value=1>TKIP</option>
							<option value=2>AES</option>
							<option value=3>TKIP+AES</option>
						</select></td>
					</tr>				
					<tr id="show_wpa2_cipher" style="display:none">
						<td width="45%">加密/认证配置(WPA2):</td>
						<td colspan="2"><select style="width: 180px" size="1" name="wpa2ciphersuite" id="wpa2ciphersuite">
							<option value=1>TKIP</option>
							<option value=2>AES</option>
							<option value=3>TKIP+AES</option>
						</select></td>
					</tr>
				</table>
			</td>
		</tr>
		<tr id="setting_wps">
			<td colspan="2" width="100%">
				<table width="400" border="0" cellpadding="4" cellspacing="0">
					<tr>
						<td width="45%">WPS 使能:</td>
						<td valign="middle" width="30" height="30">
							<input type='checkbox' id='disableWPS' name='disableWPS' value="ON" onchange="WPSCheck(document.wlanSetup);"></td>
					</tr>
					<tr id="show_wps_button" style="display:none">
						<td width="45%">WPS 模式:</td>
						<td colspan="2">
						<input class="btnsaveup" type="button" id="triggerPBC" name="triggerPBC" value="Start WPS" onClick="trigger_WPS_button();">
					</tr>
				</table>
			</td>
		</tr>
	</table>
	<br><br>
	<input type='hidden' name="lst" >
	<input type="hidden" name="wlan_idx" value=1>
	<input type="hidden" name="pskFormat">
	<script>
		_wlan_mode[0]=0;
	_encrypt[0]=6;
	_enable1X[0]=0;
	_wpaAuth[0]=2;
	_wpaPSKFormat[0]=0;
	_wpaPSK[0]='0';
	_rsPort[0]=1812;
	_rsIpAddr[0]='0.0.0.0';
	_rsPassword[0]='';
	_uCipher[0]=1;
	_wpa2uCipher[0]=2;
	_wepAuth[0]=2;
	_wepLen[0]=0;
	_wepKeyFormat[0]=0;
		_wlan_isNmode[0]=1;
	_wlan_mode[1]=0;
	_encrypt[1]=0;
	_enable1X[1]=0;
	_wpaAuth[1]=2;
	_wpaPSKFormat[1]=0;
	_wpaPSK[1]='';
	_rsPort[1]=1812;
	_rsIpAddr[1]='0.0.0.0';
	_rsPassword[1]='';
	_uCipher[1]=1;
	_wpa2uCipher[1]=2;
	_wepAuth[1]=2;
	_wepLen[1]=0;
	_wepKeyFormat[1]=0;
		_wlan_isNmode[1]=1;
	_wlan_mode[2]=0;
	_encrypt[2]=0;
	_enable1X[2]=0;
	_wpaAuth[2]=2;
	_wpaPSKFormat[2]=0;
	_wpaPSK[2]='';
	_rsPort[2]=1812;
	_rsIpAddr[2]='0.0.0.0';
	_rsPassword[2]='';
	_uCipher[2]=1;
	_wpa2uCipher[2]=2;
	_wepAuth[2]=2;
	_wepLen[2]=0;
	_wepKeyFormat[2]=0;
		_wlan_isNmode[2]=1;
	_wlan_mode[3]=0;
	_encrypt[3]=0;
	_enable1X[3]=0;
	_wpaAuth[3]=2;
	_wpaPSKFormat[3]=0;
	_wpaPSK[3]='';
	_rsPort[3]=1812;
	_rsIpAddr[3]='0.0.0.0';
	_rsPassword[3]='';
	_uCipher[3]=1;
	_wpa2uCipher[3]=2;
	_wepAuth[3]=2;
	_wepLen[3]=0;
	_wepKeyFormat[3]=0;
		_wlan_isNmode[3]=1;
	_wlan_mode[4]=1;
	_encrypt[4]=6;
	_enable1X[4]=0;
	_wpaAuth[4]=2;
	_wpaPSKFormat[4]=0;
	_wpaPSK[4]='0';
	_rsPort[4]=1812;
	_rsIpAddr[4]='0.0.0.0';
	_rsPassword[4]='';
	_uCipher[4]=1;
	_wpa2uCipher[4]=2;
	_wepAuth[4]=2;
	_wepLen[4]=0;
	_wepKeyFormat[4]=0;
		_wlan_isNmode[4]=1;
	
		wps20 = 1;

		show_authentication();
		defPskLen = document.wlanSetup.pskValue.value.length;
		updateWepFormat(document.wlanSetup, 0);
		SSIDSelected(0);
	</script>
	<p>            
	<div id=wlUserTip>
		<font color=red>提示:</font> <br>
				&nbsp;&nbsp;&nbsp;&nbsp;安全方式设置为 None 时，传输数据不会被加密。 <br>
				&nbsp;&nbsp;&nbsp;&nbsp;其他用户可以随意接入您的无线网络。容易引起安全问题。 <br>
				&nbsp;&nbsp;&nbsp;&nbsp;若没有特殊需要，建议不使用。 
	</div>
			<p></p>
		<td width=10>&nbsp;</td>	
	</div>
	</DIV>
	<br><br>	
			<input type="hidden" value="/net_wlan_basic_11n_5g_cmcc.asp" name="submit-url">
			<input type="hidden" id="action" name="action" value="0">
			<input type="hidden" id="ssid_idx" name="ssid_idx" value="0">
			<input class="btnsaveup" type='button' onClick='on_submit()' value='确定'>
			<input type="button" class="btnsaveup2" value="取消" onclick="window.location.reload()">
	</form></table>
	
<script language=JavaScript type=text/javascript>
function encrypChange()
{ 
	with ( get_by_id('security_method'))
	{
		var securmode = getSelectVal('security_method');
		
		switch(securmode)
		{
			case '0':
			{
				get_by_id("wlUserTip").innerHTML = "<font color=red>提示:</font>"
				+ "<br>"
				+ "&nbsp;&nbsp;&nbsp;&nbsp;安全方式设置为 None 时，传输数据不会被加密。"
				+ "<br>"
				+ "&nbsp;&nbsp;&nbsp;&nbsp;其他用户可以随意接入您的无线网络。容易引起安全问题。"
				+ "<br>"
				+ "&nbsp;&nbsp;&nbsp;&nbsp;若没有特殊需要，建议不使用。";
				break;
			}
			case '1':
			{
				get_by_id("wlUserTip").innerHTML = "<font color=red>提示:</font>"
				+ "<br>"
				+ "&nbsp;&nbsp;&nbsp;&nbsp;安全方式设置为 WEP 时，传输数据经过加密。"
				+ "<br>"
				+ "&nbsp;&nbsp;&nbsp;&nbsp;密钥可由用户设置为64bit或128bit。";
				break;
			}
			case '2':
			{
				get_by_id("wlUserTip").innerHTML = "<font color=red>提示:</font>"
				+ "<br>"
				+ "&nbsp;&nbsp;&nbsp;&nbsp;安全方式设置为 WPA-PSK 时，传输数据经过加密。"
				+ "<br>"
				+ "&nbsp;&nbsp;&nbsp;&nbsp;密钥可由用户设置为8位到63位。建议使用。";
				break;
			}
			case '4':
			{
				get_by_id("wlUserTip").innerHTML = "<font color=red>提示:</font>"
				+ "<br>"
				+ "&nbsp;&nbsp;&nbsp;&nbsp;安全方式设置为 WPA2 时，传输数据经过加密。"
				+ "<br>"
				+ "&nbsp;&nbsp;&nbsp;&nbsp;密钥可由用户设置为8位到63位。需要操作系统支持。";
				break;
			}
			case '6':
			{
				get_by_id("wlUserTip").innerHTML = "<font color=red>提示:</font>"
				+ "<br>"
				+ "&nbsp;&nbsp;&nbsp;&nbsp;安全方式设置为 WPA/WPA2 时，传输数据经过加密。"
				+ "<br>"
				+ "&nbsp;&nbsp;&nbsp;&nbsp;密钥可由用户设置为8位到63位。需要操作系统支持。";
				break;
			}
			default:
				break;
		}
	}
}
	on_init();
	encrypChange();
</script>
</blockquote>
</body>

</html>

<script language=JavaScript type=text/javascript>
function WPSCheck(element) 
{

	if( _WPS_running[0] != 0 && !element.elements["disableWPS"].checked)	//WPS is running
	{
		alert("当前WPS处于工作状态，无法关闭WPS功能！请按STOP WPS按钮后，再尝试关闭WPS功能。");
		window.location.reload();
	}
	
	if(element.elements["disableWPS"].checked && getSelectVal('security_method') == 1)
	{
		alert("加密模式为WEP时，无法开启WPS功能！");
		element.disableWPS.checked = false;
		return;
	}

	if(element.elements["disableWPS"].checked)
	{
		get_by_id("show_wps_button").style.display = "";
		get_by_id("disableWPS").value = "ON";
	}
	else
	{
		get_by_id("show_wps_button").style.display = "none";
		get_by_id("disableWPS").value = "OFF";
	}
	
	return;
}


function trigger_WPS_button()
{
	if(_WPS_running[1] == 0)  //TO Start
	{
		alert("请在2分钟内启动WPS连接.");	
		get_by_id("action").value = 1;   // start PBC	
	}
	else //To Stop
	{
		get_by_id("action").value = 2;   // stop PBC
	}
	document.wlanSetup.submit();
}

</script>