<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<HTML>
<HEAD>
<TITLE>家庭网关</TITLE>
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
<SCRIPT language="javascript" type="text/javascript">
var timeout = 300000;	/* 5 min */
var timeoutTimer;

timeoutTimer = setTimeout(timeoutFunc, timeout);

function clearAuthData()
{
	
  if (document.all) 
  {
    // Internet Explorer: 'ClearAuthenticationCache' is only available in IE
    document.execCommand('ClearAuthenticationCache');

	} 
	else 
	{
    var xmlhttp;
    if (window.XMLHttpRequest) 
   	{
        xmlhttp = new XMLHttpRequest();
    } 
    else if (window.ActiveXObject) 
    {
        try {
            xmlhttp = new ActiveXObject('Msxml2.XMLHTTP');
        } catch (ex) {
            try {
                xmlhttp = new ActiveXObject('Microsoft.XMLHTTP');
            } catch (ex) {}
        }
    }
    if (xmlhttp.readyState < 4) 
    {
        xmlhttp.abort();
    }
    // Firefox/Mozilla: use anonymous "login" to trigger a "logout"
    xmlhttp.open('GET', '/?sling:authRequestLogin=1', false, 'anonymous', 'null');
    xmlhttp.send('');
}
		document.logout.submit();
		return true;

}
function timeoutFunc()
{
	clearAuthData();
	document.forms[0].submit();
}

function resetTimeoutFunc()
{
	clearTimeout(timeoutTimer);
	timeoutTimer = setTimeout(timeoutFunc, timeout);
}
/********************************************************************
**          menu class
********************************************************************/
function menu(name)
{
	this.name = name;
	this.names = new Array();
	this.objs = new Array();
	
	this.destroy = function(){delete map;map = null;}
	this.add = function(obj, name){var i = this.names.length; if(name){this.names[i] = name;}else{this.names[i] = obj.name;} this.objs[i] = obj;}
	
	return this;
}

var mnroot = new menu("root");
var mncata = null;
var mnpage = null;
var wlan_num =  <% getIndex("show_wlan_num"); %> ;
/*======================================================*/
/********************************状态 B*********************************/


/* 设备信息
mnpage = new menu("设备信息");
mnpage.add("status_network_map.asp","网络拓扑信息");
mnpage.add("status.htm","设备基本信息");
mnpage.add("stats.htm","统计信息");
mncata = new menu("状态");
mncata.add(mnpage);
*/

mncata = new menu("状态");
mnpage = new menu("设备信息");
if(<%getIndex("status_network_map.asp")%>)
	mnpage.add( "status_network_map.asp","网络拓扑信息");
if(<%getIndex("status.htm")%>)
	mnpage.add( "status.htm","设备基本信息");
if(<%getIndex("stats.htm")%>)
	mnpage.add( "stats.htm","统计信息");
mncata.add(mnpage);


/* 远程管理状态
mnpage = new menu("远程管理状态");
mnpage.add("status_tr069_info_admin_cmcc.asp","交互建立");
mnpage.add("status_tr069_config_admin_cmcc.asp","业务配置下发状态");
mncata.add(mnpage);
mnroot.add(mncata);
*/
mnpage = new menu("远程管理状态");
if(<%getIndex("status_tr069_info_admin_cmc.asp")%>)
	mnpage.add( "status_tr069_info_admin_cmcc.asp","交互建立");
if(<%getIndex("status_tr069_config_admin_cmcc.asp")%>)
	mnpage.add( "status_tr069_config_admin_cmcc.asp","业务配置下发状态");
mncata.add(mnpage);

mnroot.add(mncata);

/********************************状态 E*********************************/

/********************************网络 B*********************************/
mncata = new menu("网络");

/* 网络模式
mnpage = new menu("网络模式");
mnpage.add("opmode.htm","网络模式设置");
mncata.add(mnpage);
*/
mnpage = new menu("网络模式");
if(<%getIndex("opmode.htm")%>)
	mnpage.add( "opmode.htm","网络模式设置");
mncata.add(mnpage);

/* Internet设置
mnpage = new menu("Internet设置");
mnpage.add("net_fastconf.asp","快速配置向导");
mnpage.add("net_eth_links_cmcc.htm","WAN侧连接");
mnpage.add("tcpiplan.htm","LAN侧连接");
mncata.add(mnpage);
*/
mnpage = new menu("Internet设置");
if(<%getIndex("net_fastconf.asp")%>)
	mnpage.add( "net_fastconf.asp","快速配置向导");
if(<%getIndex("net_eth_links_cmcc.htm")%>)
	mnpage.add( "net_eth_links_cmcc.htm","WAN侧连接");
if(<%getIndex("tcpiplan.htm")%>)
	mnpage.add( "tcpiplan.htm","LAN侧连接");
	//mnpage.add( "l2tp.asp","VPN-l2tp");
mncata.add(mnpage);


/*mnpage = new menu("绑定设置");
mnpage.add("net_vlan_mapping.asp","绑定模式");
mncata.add(mnpage);
mnpage = new menu("LAN侧地址配置");
mnpage.add("net_dhcpd_cmcc.asp","IPv4配置");
mnpage.add("net_ipv6_cmcc.asp","IPv6 配置");
mncata.add(mnpage);*/

/* QoS
mnpage = new menu("QoS");
//mnpage.add("qos_uplink.htm","上行QOS");
mnpage.add("qos_shaping.htm","限速配置");
mncata.add(mnpage);
*/
mnpage = new menu("QoS");
if(<%getIndex("ispMultiPhyWan")%>){
	if(<%getIndex("qos_multiPhyWan_uplink.htm")%>)
	mnpage.add( "qos_multiPhyWan_uplink.htm","多物理wan上行QOS");
if(<%getIndex("qos_multiPhyWan_shaping.htm")%>)
	mnpage.add( "qos_multiPhyWan_shaping.htm","多物理wan限速配置");
} else {
if(<%getIndex("qos_uplink.htm")%>)
	mnpage.add( "qos_uplink.htm","上行QOS");
if(<%getIndex("qos_shaping.htm")%>)
	mnpage.add( "qos_shaping.htm","限速配置");
}
mncata.add(mnpage);

/* WLAN5G配置
if(wlan_num>1){
mnpage = new menu("WLAN5G配置");
mnpage.add("boafrm/formWlanRedirect?redirect-url=wlbasic.htm&wlan_id=0","WLAN基本配置");
mnpage.add("wladvanced.htm","高级配置");
mnpage.add("wlsecurity.htm","安全配置");
mnpage.add("wlactrl.htm","连接控制");
mnpage.add("wlwds.htm","WDS配置");
mnpage.add("wlsurvey.htm","网络扫描");
mnpage.add("wlwps.htm","WPS配置");
mnpage.add("wlsch.htm","WIFI连接规则");
mncata.add(mnpage);
}
*/
if(wlan_num>1){
	mnpage = new menu("WLAN5G配置");
	
	if(<%getIndex("wlbasic.htm")%>)
		mnpage.add( "boafrm/formWlanRedirect?redirect-url=wlbasic.htm&wlan_id=0","WLAN基本配置");
	if(<%getIndex("wladvanced.htm")%>)
		mnpage.add( "wladvanced.htm","高级配置");
	if(<%getIndex("wlsecurity.htm")%>)
		mnpage.add( "wlsecurity.htm","安全配置");
	if(<%getIndex("wlactrl.htm")%>)
		mnpage.add( "wlactrl.htm","连接控制");
	if(<%getIndex("wlwds.htm")%>)
		mnpage.add( "wlwds.htm","WDS配置");
	if(<%getIndex("wlsurvey.htm")%>)
		mnpage.add( "wlsurvey.htm","网络扫描");
	if(<%getIndex("wlwps.htm")%>)
		mnpage.add( "wlwps.htm","WPS配置");
	if(<%getIndex("isMeshDefined")%>)    /* getIndex("wlmesh.htm") -> getIndex("isMeshDefined")*/
		mnpage.add( "wlmesh.htm","MESH配置");
	if(<%getIndex("wlsch.htm")%>)
		mnpage.add( "wlsch.htm","WIFI连接规则");

	mncata.add(mnpage);
}

/* WLAN2.4G配置
if(wlan_num>0){
mnpage = new menu("WLAN2.4G配置");
mnpage.add("boafrm/formWlanRedirect?redirect-url=wlbasic.htm&wlan_id=1","WLAN基本配置");
mnpage.add("wladvanced.htm","高级配置");
mnpage.add("wlsecurity.htm","安全配置");
mnpage.add("wlactrl.htm","连接控制");
mnpage.add("wlwds.htm","WDS配置");
mnpage.add("wlsurvey.htm","网络扫描");
mnpage.add("wlwps.htm","WPS配置");
mnpage.add("wlsch.htm","WIFI连接规则");
mncata.add(mnpage);
}
*/
if(wlan_num>0){
	mnpage = new menu("WLAN2.4G配置");
	
	if(<%getIndex("wlbasic.htm")%>)
		mnpage.add( "boafrm/formWlanRedirect?redirect-url=wlbasic.htm&wlan_id=1","WLAN基本配置");
	if(<%getIndex("wladvanced.htm")%>)
		mnpage.add( "wladvanced.htm","高级配置");
	if(<%getIndex("wlsecurity.htm")%>)
		mnpage.add( "wlsecurity.htm","安全配置");
	if(<%getIndex("wlactrl.htm")%>)
		mnpage.add( "wlactrl.htm","连接控制");
	if(<%getIndex("wlwds.htm")%>)
		mnpage.add( "wlwds.htm","WDS配置");
	if(<%getIndex("wlsurvey.htm")%>)
		mnpage.add( "wlsurvey.htm","网络扫描");
	if(<%getIndex("wlwps.htm")%>)
		mnpage.add( "wlwps.htm","WPS配置");
	if(<%getIndex("isMeshDefined")%>)
		mnpage.add( "wlmesh.htm","MESH配置");
	if(<%getIndex("wlsch.htm")%>)
		mnpage.add( "wlsch.htm","WIFI连接规则");

	mncata.add(mnpage);
}

//mnpage = new menu("远程管理");
//mnpage.add("tr069config.htm","省级数字家庭管理平台服务器");
//mnpage.add("usereg_inside_loid_cmcc.asp","LOID认证");
//mnpage.add("usereg_inside_menu_cmcc.asp","PASSWORD认证");
//mncata.add(mnpage);

/* 时间管理
mnpage = new menu("时间管理");
mnpage.add("ntp.htm","时间管理");
mncata.add(mnpage);
*/
mnpage = new menu("时间管理");
if(<%getIndex("ntp.htm")%>)
	mnpage.add( "ntp.htm","时间管理");
mncata.add(mnpage);

/* 路由配置
mnpage = new menu("路由配置");
mnpage.add("route.htm","静态路由");
mncata.add(mnpage);
*/
mnpage = new menu("路由配置");
if(<%getIndex("route.htm")%>)
	mnpage.add( "route.htm","静态路由");
mncata.add(mnpage);
//====================Multi-AP============================

mnpage = new menu("EasyMesh");
if(<%getIndex("multi_ap_setting_general.htm");%>)
	mnpage.add( "multi_ap_setting_general.htm","General");
if ( 1 == <% getIndex("multi_ap_controller");%>)   //get from navigation.js
        mnpage.add( "multi_ap_setting_topology.htm","Topology");
	mncata.add(mnpage);

//====================Multi-AP============================
mnroot.add(mncata);
/********************************网络 E*********************************/

/********************************安全 B*********************************/
mncata = new menu("安  全");

/* 广域网访问设置
mnpage = new menu("广域网访问设置");
mnpage.add("urlfilter.htm","广域网访问设置");
mncata.add(mnpage);
*/
mnpage = new menu("广域网访问设置");
if(<%getIndex("urlfilter.htm")%>)
	mnpage.add( "urlfilter.htm","广域网访问设置");
mncata.add(mnpage);


/* 防火墙
mnpage = new menu("防火墙");
//mnpage.add("secu_firewall_level_cmcc.asp","安全级");
mnpage.add("dos.htm","攻击保护设置");
mncata.add(mnpage);
*/
mnpage = new menu("防火墙");
if(<%getIndex("secu_firewall_level_cmcc.asp")%>)
	mnpage.add( "secu_firewall_level_cmcc.asp","安全级");
mncata.add(mnpage);

/* MAC过滤
mnpage = new menu("MAC过滤");
mnpage.add("macfilter.htm","MAC过滤");
mncata.add(mnpage);
*/
mnpage = new menu("MAC过滤");
if(<%getIndex("macfilter.htm")%>)
	mnpage.add( "macfilter.htm","MAC过滤");
mncata.add(mnpage);

/* IP过滤
mnpage = new menu("IP过滤");
if(<%getIndex("ipv6");%>)
	mnpage.add("ip6filter.htm","IP过滤");
else
	mnpage.add("ipfilter.htm","IP过滤");
mncata.add(mnpage);
*/
mnpage = new menu("IP过滤");
//for getIndex to judge : strstr(name,".htm")
if(<%getIndex("ipv6");%>){
  if(<%getIndex("ip6filter.htm")%>)
    mnpage.add( "ip6filter.htm","IP过滤");
}
else{
  if(<%getIndex("ipfilter.htm")%>)
    mnpage.add( "ipfilter.htm","IP过滤");
}
mncata.add(mnpage);

/* 端口过滤
mnpage = new menu("端口过滤");
if(<%getIndex("ipv6");%>)
	mnpage.add("portfilter6.htm","端口过滤");
else
	mnpage.add("portfilter.htm","端口过滤");
mncata.add(mnpage);
*/
mnpage = new menu("端口过滤");
if(<%getIndex("ipv6");%>){
  if(<%getIndex("portfilter6.htm")%>)
    mnpage.add( "portfilter6.htm","端口过滤");
}
else{
  if(<%getIndex("portfilter.htm")%>)
    mnpage.add( "portfilter.htm","端口过滤");
}
mncata.add(mnpage);

if(<%getIndex("access_user_limit");%>)
{
	mnpage = new menu("公网接入用户限制");
	if(<%getIndex("access_internet_user_limit.htm")%>)
		mnpage.add("access_internet_user_limit.htm","公网接入用户限制");
	mncata.add(mnpage);
}


if(<%getIndex("vlan_binding_support");%>)
{
/* VLAN绑定
	mnpage = new menu("VLAN绑定");
	mnpage.add("vlan_binding.htm","VLAN绑定");
	mncata.add(mnpage);
*/
  mnpage = new menu("VLAN绑定");
  if(<%getIndex("vlan_binding.htm");%>)
  	mnpage.add("vlan_binding.htm","VLAN绑定");
  mncata.add(mnpage);
}

mnroot.add(mncata);

/********************************安全 E*********************************/

/********************************应用 B*********************************/
mncata = new menu("应  用");
/* DDNS配置
mnpage = new menu("DDNS配置");
mnpage.add("ddns.htm","DDNS配置");
mncata.add(mnpage);
*/
mnpage = new menu("DDNS配置");
if(<%getIndex("ddns.htm");%>)
	mnpage.add("ddns.htm","DDNS配置");
mncata.add(mnpage);

/* 高级NAT配置
mnpage = new menu("高级NAT配置");
mnpage.add("algonoff.asp","ALG配置");
mnpage.add("dmz.htm","DMZ配置");
mnpage.add("portfw.htm","虚拟主机配置");
mncata.add(mnpage);
*/
mnpage = new menu("高级NAT配置");
if(<%getIndex("algonoff.asp");%>)
  mnpage.add("algonoff.asp","ALG配置");
if(<%getIndex("dmz.htm");%>)
	mnpage.add("dmz.htm","DMZ配置");
if(<%getIndex("portfw.htm");%>)
  mnpage.add("portfw.htm","虚拟主机配置");
mncata.add(mnpage);


/* UPNP配置
mnpage = new menu("UPNP配置");
mnpage.add("app_upnp_cmcc.asp","UPNP配置");
mncata.add(mnpage);
*/
mnpage = new menu("UPNP配置");
if(<%getIndex("app_upnp_cmcc.asp");%>)
  mnpage.add("app_upnp_cmcc.asp","UPNP配置");
mncata.add(mnpage);
  
//mnpage = new menu("宽带电话设置");
//mnpage.add("cmcc_app_voip.asp","宽带电话设置");
//mncata.add(mnpage);

/* IGMP设置
mnpage = new menu("IGMP设置");
mnpage.add("snooping_proxy_cmcc.asp","IGMP设置");
mncata.add(mnpage);
*/
mnpage = new menu("IGMP设置");
if(<%getIndex("snooping_proxy_cmcc.asp");%>)
  mnpage.add("snooping_proxy_cmcc.asp","IGMP设置");
mncata.add(mnpage);

if(<%getIndex("aWiFi_support")%>){
	mnpage = new menu("aWiFi设置");
	if(<%getIndex("app_aWiFi.htm");%>)
  		mnpage.add("app_aWiFi.htm","aWiFi设置");
	if(<%getIndex("aWiFi_macList.htm");%>)
		mnpage.add("aWiFi_macList.htm","aWiFI白名单(MAC)");
	if(<%getIndex("aWiFi_urlList.htm");%>)
  		mnpage.add("aWiFi_urlList.htm","aWiFi白名单(域名)");
	if(<%getIndex("aWiFi_audit_support");%> && <%getIndex("aWiFi_audit.htm");%>)
  		mnpage.add("aWiFi_audit.htm","aWiFi安审设置");
	mncata.add(mnpage);
}

/*samba storage managment*/
if(<%getIndex("isMiniDLNASupport")%>){
	mnpage = new menu("DLNA配置");
	mnpage.add("minidlna.asp","dlna配置");
	mncata.add(mnpage);
}

if(<%getIndex("isSambaSupport")%>)
{
	mnpage = new menu("磁盘管理");
	mnpage.add("samba.asp","samba配置");
	if(<%getIndex("DiskWebSupportOnOff")%>){
		mnpage.add("diskinfo.htm","磁盘信息");//"Disk Information"
		mnpage.add("accountmng.htm","用户管理");//"Account Management"
		mnpage.add("disksharefolder.htm","共享目录");//"Share Folder"
		mnpage.add("diskpartition.htm","磁盘分区");//"Disk Partition"
		mnpage.add("diskformat.htm","磁盘格式化");	//	"Disk Format"			
	}
	mncata.add(mnpage);
}

mnroot.add(mncata);

/********************************应用 E*********************************/

/********************************管理 B*********************************/
mncata = new menu("管  理");

/* 用户管理
mnpage = new menu("用户管理");
mnpage.add("password.htm","用户管理");
mncata.add(mnpage);
*/
mnpage = new menu("用户管理");
if(<%getIndex("MULTI_USER_ENABLED")%>)
{
	if(<%getIndex("super_password.htm")%>){
	  mnpage.add("super_password.htm","用户管理");
	}else if(<%getIndex("password.htm");%>){
	  mnpage.add("password.htm","用户管理");
	}
}else{
	if(<%getIndex("password.htm");%>)
		mnpage.add("password.htm","用户管理");
}
mncata.add(mnpage);

/* 设备管理
mnpage = new menu("设备管理");
mnpage.add("cluster_mgmt.asp","集群管理");
mnpage.add("mgm_dev_reboot.asp","设备重启");
mnpage.add("saveconf.htm","恢复配置");
mnpage.add("upload.htm","固件更新");
mncata.add(mnpage);
*/
mnpage = new menu("设备管理");
if(<%getIndex("cluster_mgmt.asp");%>)
  mnpage.add("cluster_mgmt.asp","集群管理");
if(<%getIndex("elink.asp");%>)
  mnpage.add("elink.asp","elink配置");
if(<%getIndex("mgm_dev_reboot.asp");%>)
  mnpage.add("mgm_dev_reboot.asp","设备重启");
if(<%getIndex("saveconf.htm");%>)
  mnpage.add("saveconf.htm","恢复配置");
if(<%getIndex("upload.htm");%>)
  mnpage.add("upload.htm","固件更新");
mncata.add(mnpage);

/* 日志文件管理
*/
mnpage = new menu("日志文件管理");
mnpage.add("syslog.htm","日志管理");
mnpage.add("mgm_log_view_cmcc.asp","日志查看");
mncata.add(mnpage);
/*
mnpage = new menu("用户管理");
if(<%getIndex("password.htm");%>)
  mnpage.add("password.htm","用户管理");
mncata.add(mnpage);
*/
/* 远程管理
mnpage = new menu("远程管理");
mnpage.add("tr069config.htm","TR-069配置");
mncata.add(mnpage);
mnroot.add(mncata);
*/
mnpage = new menu("远程管理");
if(<%getIndex("tr069config.htm");%>)
  mnpage.add("tr069config.htm","TR-069配置");
mncata.add(mnpage);

mnroot.add(mncata);

/********************************管理 E*********************************/

/********************************诊断 B*********************************/
mncata = new menu("诊断");	

/* 网络诊断
mnpage = new menu("网络诊断");
mnpage.add("diag_ping_admin.asp","PING测试");
mnpage.add("diag_tracert_admin.asp","Tracert测试");
mnpage.add("diagnose_tr069_admin_cmcc.asp","Inform手动上报");
mncata.add(mnpage);
*/
mnpage = new menu("网络诊断");
if(<%getIndex("diag_ping_admin.asp");%>)
  mnpage.add("diag_ping_admin.asp","PING测试");
if(<%getIndex("iag_tracert_admin.asp");%>)
  mnpage.add("diag_tracert_admin.asp","Tracert测试");
if(<%getIndex("diagnose_tr069_admin_cmcc.asp");%>)
  mnpage.add("diagnose_tr069_admin_cmcc.asp","Inform手动上报");
mncata.add(mnpage);

/* VoIP诊断
mnpage = new menu("VoIP诊断");
mnpage.add("auto_call_voip.asp","VoIP诊断");
mncata.add(mnpage);
*/	
mnpage = new menu("VoIP诊断");
if(<%getIndex("auto_call_voip.asp");%>)
  mnpage.add("auto_call_voip.asp","VoIP诊断");
mncata.add(mnpage);

mnroot.add(mncata);

/********************************诊断 E*********************************/

/********************************帮助 B*********************************/
mncata = new menu("帮  助");	
/* 状态
mnpage = new menu("状态");
mnpage.add("/help_cmcc/help_status_device.html","设备信息");
mnpage.add("/help_cmcc/help_status_net.asp","网络侧信息");
mnpage.add("/help_cmcc/help_status_user.html","用户侧信息");
//mnpage.add("/help/help_status_voip.html","宽带语音信息");
mnpage.add("/help_cmcc/help_status_tr069.html","远程管理状态");
mncata.add(mnpage);
*/
mnpage = new menu("状态");
if(<%getIndex("help_cmcc/help_status_device.html");%>)
  mnpage.add("/help_cmcc/help_status_device.html","设备信息");
if(<%getIndex("help_cmcc/help_status_net.asp");%>)
  mnpage.add("/help_cmcc/help_status_net.asp","网络侧信息");
if(<%getIndex("help_cmcc/help_status_user.html");%>)
  mnpage.add("/help_cmcc/help_status_user.html","用户侧信息");
if(<%getIndex("help_cmcc/help_status_tr069.html");%>)
  mnpage.add("/help_cmcc/help_status_tr069.html","远程管理状态");
mncata.add(mnpage);

/* 网络
mnpage = new menu("网络");
mnpage.add("/help_cmcc/help_net_pon.html","宽带设置");
mnpage.add("/help_cmcc/help_net_vlan_binding.html","绑定配置");
mnpage.add("/help_cmcc/help_net_lan.html","LAN侧地址配置");
mnpage.add("/help_cmcc/help_net_wlan.asp","WLAN网络配置");
mnpage.add("/help_cmcc/help_net_remote.asp","远程管理");
mnpage.add("/help_cmcc/help_net_qos.html","QoS");
mnpage.add("/help_cmcc/help_net_time.html","时间管理");
mnpage.add("/help_cmcc/help_net_route.html","路由配置");
mncata.add(mnpage);
*/
mnpage = new menu("网络");
if(<%getIndex("help_cmcc/help_net_pon.html");%>)
  mnpage.add("/help_cmcc/help_net_pon.html","宽带设置");
if(<%getIndex("help_cmcc/help_net_vlan_binding.html");%>)
  mnpage.add("/help_cmcc/help_net_vlan_binding.html","绑定配置");
if(<%getIndex("help_cmcc/help_net_lan.html");%>)
  mnpage.add("/help_cmcc/help_net_lan.html","LAN侧地址配置");
if(<%getIndex("help_cmcc/help_net_wlan.asp");%>)
  mnpage.add("/help_cmcc/help_net_wlan.asp","WLAN网络配置");
if(<%getIndex("help_cmcc/help_net_remote.asp");%>)
  mnpage.add("/help_cmcc/help_net_remote.asp","远程管理");
if(<%getIndex("help_cmcc/help_net_qos.html");%>)
  mnpage.add("/help_cmcc/help_net_qos.html","QoS");
if(<%getIndex("help_cmcc/help_net_time.html");%>)
  mnpage.add("/help_cmcc/help_net_time.html","时间管理");
if(<%getIndex("help_cmcc/help_net_route.html");%>)
  mnpage.add("/help_cmcc/help_net_route.html","路由配置");
mncata.add(mnpage);

/* 安全
mnpage = new menu("安全");
mnpage.add("/help_cmcc/help_security_wanaccess.html","广域网访问设置");
mnpage.add("/help_cmcc/help_security_firewall.html","防火墙");
mnpage.add("/help_cmcc/help_security_macfilter.html","MAC过滤");
mnpage.add("/help_cmcc/help_security_portfilter.html","端口过滤");
mncata.add(mnpage);
*/
mnpage = new menu("安全");
if(<%getIndex("help_cmcc/help_security_wanaccess.html");%>)
  mnpage.add("/help_cmcc/help_security_wanaccess.html","广域网访问设置");
if(<%getIndex("help_cmcc/help_security_firewall.html");%>)
  mnpage.add("/help_cmcc/help_security_firewall.html","防火墙");
if(<%getIndex("help_cmcc/help_security_macfilter.html");%>)
  mnpage.add("/help_cmcc/help_security_macfilter.html","MAC过滤");
if(<%getIndex("help_cmcc/help_security_portfilter.html");%>)
  mnpage.add("/help_cmcc/help_security_portfilter.html","端口过滤");
mncata.add(mnpage);

/* 应用 
mnpage = new menu("应用");
mnpage.add("/help_cmcc/help_apply_ddns.html","DDNS配置");
mnpage.add("/help_cmcc/help_apply_nat.html","高级NAT配置");
mnpage.add("/help_cmcc/help_apply_upnp.html","UPNP配置");
//mnpage.add("/help/help_apply_voip.html","宽带电话设置");
mnpage.add("/help_cmcc/help_apply_igmp.html","IGMP设置");
mncata.add(mnpage);
*/
mnpage = new menu("应用");
if(<%getIndex("help_cmcc/help_apply_ddns.html");%>)
  mnpage.add("/help_cmcc/help_apply_ddns.html","DDNS配置");
if(<%getIndex("help_cmcc/help_apply_nat.html");%>)
  mnpage.add("/help_cmcc/help_apply_nat.html","高级NAT配置");
if(<%getIndex("help_cmcc/help_apply_upnp.html");%>)
  mnpage.add("/help_cmcc/help_apply_upnp.html","UPNP配置");
if(<%getIndex("help_cmcc/help_apply_igmp.html");%>)
  mnpage.add("/help_cmcc/help_apply_igmp.html","IGMP设置");
mncata.add(mnpage);

/* 管理
mnpage = new menu("管理");
mnpage.add("/help_cmcc/help_manage_user.html","用户管理");
mnpage.add("/help_cmcc/help_manage_device.asp","设备管理");
mnpage.add("/help_cmcc/help_manage_logfile.html","日志文件管理");
mnpage.add("/help_cmcc/help_manage_tr069.html","远程管理");
mncata.add(mnpage);
*/
mnpage = new menu("管理");
if(<%getIndex("help_cmcc/help_manage_user.html");%>)
  mnpage.add("/help_cmcc/help_manage_user.html","用户管理");
if(<%getIndex("help_cmcc/help_manage_device.asp");%>)
  mnpage.add("/help_cmcc/help_manage_device.asp","设备管理");
if(<%getIndex("help_cmcc/help_manage_logfile.html");%>)
  mnpage.add("/help_cmcc/help_manage_logfile.html","日志文件管理");
if(<%getIndex("help_cmcc/help_manage_tr069.html");%>)
  mnpage.add("/help_cmcc/help_manage_tr069.html","远程管理");
mncata.add(mnpage);

/* 诊断
mnpage = new menu("诊断");
mnpage.add("/help_cmcc/help_diag_net.html","网络诊断");
//mnpage.add("/help/help_diag_voip.html","VOIP诊断");
mncata.add(mnpage);
*/
mnpage = new menu("诊断");
if(<%getIndex("help_cmcc/help_diag_net.html");%>)
  mnpage.add("/help_cmcc/help_diag_net.html","网络诊断");
mncata.add(mnpage);  

mnroot.add(mncata);

/********************************帮助 E*********************************/

/*======================================================*/




/********************************************************************
**          on document load
********************************************************************/
var NavImage = new Array();
NavImage[0]=new Array("nav_condition_n.png");
NavImage[1]=new Array("nav_network_n.png");
NavImage[2]=new Array("nav_security_n.png");
NavImage[3]=new Array("nav_yingyong_n.png");
NavImage[4]=new Array("nav_administration_n.png");
NavImage[5]=new Array("nav_diagnosis_n.png");
NavImage[6]=new Array("nav_help_n.png");
var NavImageClick = new Array();
NavImageClick[0]=new Array("nav_condition_h.png");
NavImageClick[1]=new Array("nav_network_h.png");
NavImageClick[2]=new Array("nav_security_h.png");
NavImageClick[3]=new Array("nav_yingyong_h.png");
NavImageClick[4]=new Array("nav_administration_h.png");
NavImageClick[5]=new Array("nav_diagnosis_h.png");
NavImageClick[6]=new Array("nav_help_h.png");

function on_init()
{
	var fst = null;
	
	if(!topmenu) topmenu = document.getElementById("topmenu");
	if(!submenu) submenu = document.getElementById("submenu");
	
	if(topmenu.cells){while(topmenu.cells.length > 0) topmenu.deleteCell(0);}
	
	for(var i = 0; i < mnroot.names.length; i++)
	{
		var cell = topmenu.insertCell(i);
        var txt = "<a href=\"#\" onClick=\"on_catalog(" + i + ");\"><div class=\"menusize\"><img  id=\"catalogimg"+i+"\"src=\"image/"+NavImage[i]+"\"><br>"; 
		//txt += "<a href=\"#\" onClick=\"on_catolog(" + i + ");\">";
		txt += "<font style=\"font-size:14px;font-weight:bold;\" id=\"catalogfont"+i+"\">" + mnroot.names[i] + "</font></div></a>";
		//cell.bgColor = "#EF8218";
		cell.width = "95";
		cell.align = "center";
		//cell.style = "line-height: 25px;";
		cell.innerHTML = txt;
		cell.mnobj = mnroot.objs[i];
		if(fst == null)fst = i;
	}
	topmenu.sel = 0;
	//topmenu.cells[0].bgColor = "#427594";
	document.getElementById("catalogimg0").src="image/"+NavImageClick[0];
	//document.getElementById("catalogfont0").style="color:#fff45c;font-size:14px;font-weight:bold;";
		document.getElementById("catalogfont0").style.color="fff45c"
		document.getElementById("catalogfont0").style.fontSize="14px"
		document.getElementById("catalogfont0").style.fontWeight="bold"
	
	
	//menuname.innerHTML = mnroot.names[0];
	on_catalog(fst);
}

/********************************************************************
**          on_catalog changed
********************************************************************/
function on_catalog(index)
{
	var fst = null;
	
	if(!topmenu.cells || index >= topmenu.cells.length)return;
	
	if(topmenu.sel != index)
	{
		//topmenu.cells[topmenu.sel].bgColor = "#EF8218";
		document.getElementById("catalogimg"+topmenu.sel).src="image/"+NavImage[topmenu.sel];
		//document.getElementById("catalogfont"+topmenu.sel).style="font-size:14px;font-weight:bold;";
		document.getElementById("catalogfont"+topmenu.sel).style.fontSize="14px";  
		document.getElementById("catalogfont"+topmenu.sel).style.fontWeight="bold";  
		document.getElementById("catalogfont"+topmenu.sel).style.color="#ffffff";
		//topmenu.cells[index].bgColor = "#427594";
		document.getElementById("catalogimg"+index).src="image/"+NavImageClick[index];
	//	document.getElementById("catalogfont"+index).style="color:#fff45c;font-size:14px;font-weight:bold;";
		document.getElementById("catalogfont"+index).style.color="#fff45c";
		document.getElementById("catalogfont"+index).style.fontSize="14px";
		document.getElementById("catalogfont"+index).style.fontWeight="bold";
		//document.getElementById("menu"+index).color="#fff45c";
		topmenu.sel = index;
		//menuname.innerHTML = mnroot.names[index];
	}
	
	var mnobj = topmenu.cells[index].mnobj;
	
	if(submenu.cells){while(submenu.cells.length > 1) submenu.deleteCell(1);}

	for(var i = 0; i < mnobj.names.length; i++)
	{
		var cell = submenu.insertCell(i * 2 + 1);
		var index = i * 2 + 2;
		cell = submenu.insertCell(index);
		var txt ="<a href=\"#\" onClick=\"on_menu(" + index + ");\">";
        txt += "<span class=\"menu_space\" id=\"menufont"+index+"\">" + mnobj.names[i] + "</span></a>";
		//cell.width = "75px";
		cell.innerHTML = txt;
		cell.nowrap = true;
		cell.name = mnobj.names[i];
		cell.mnobj = mnobj.objs[i];
		if(fst == null)fst = index;
	}
	submenu.sel=fst;
	document.getElementById("menufont"+fst).style.color="#fff45c";
	document.getElementById("menufont"+fst).style.fontSize="12px";
	on_menu(fst);
}

/********************************************************************
**          on menu fire
********************************************************************/
function on_menu(index)
{
	if(!submenu.cells || index >= submenu.cells.length)return;
	
	if(submenu.sel != index)
	{
		//document.getElementById("menufont"+submenu.sel).style="color:#fff45c;font-size:12px;";
		//document.getElementById("menufont"+index).style="color:#fff45c;font-size:12px;";
		document.getElementById("menufont"+submenu.sel).style.color="#ffffff";
		document.getElementById("menufont"+submenu.sel).fontSize="12px";
		document.getElementById("menufont"+index).style.color="#fff45c";
		document.getElementById("menufont"+index).style.fontSize="12px";
		submenu.sel = index;
	}
	
	tbobj = submenu.cells[index];
	var mnobj = tbobj.mnobj;
	//var lstmenu = leftFrame.lstmenu;
	if(!lstmenu) lstmenu = leftFrame.document.getElementById("lstmenu");
	if(!lstmenu)return;
	if(lstmenu.rows){while(lstmenu.rows.length > 0) lstmenu.deleteRow(0);}
	
	for(var i = 0; i < mnobj.names.length; i++)
	{
		var row = lstmenu.insertRow(i);
		
		row.nowrap = true;
		row.vAlign = "top";
		
		var cell = row.insertCell(0);
		
		cell.width = "100%";
		cell.align = "center";
		if(i == 0){		
			cell.innerHTML = "<br><p>&nbsp;&nbsp;<a id=\"thirdmenufont"+i+"\" onClick=\"on_thirdmenu(" + i + ")\"; style=\"color:#fff45c\" href=\"" + mnobj.objs[i] + "\", target=\"contentIframe\">" + mnobj.names[i] + "</a></p>";
		}else{
			cell.innerHTML = "<br><p>&nbsp;&nbsp;<a id=\"thirdmenufont"+i+"\" onClick=\"on_thirdmenu(" + i + ")\"; href=\"" + mnobj.objs[i] + "\", target=\"contentIframe\">" + mnobj.names[i] + "</a></p>";
		}
		cell.nowrap = true;
		cell.name = mnobj.names[i];
		cell.mnobj = mnobj.objs[i];
	}
	contentIframe.location.href=mnobj.objs[0];
}

function on_thirdmenu(index){
	tbobj = submenu.cells[submenu.sel ];
	var mnobj = tbobj.mnobj;
	for(var i = 0; i < mnobj.names.length; i++){
		document.getElementById("thirdmenufont"+i).style.color="#ffffff";
	}
	//document.getElementById("thirdmenufont"+index).style="color:#fff45c";
	document.getElementById("thirdmenufont"+index).style.color="#fff45c";

}
function contenFramesize()
{
	getElById('contentIframe').style.height=600; 
	var mainbody = contentIframe.document.body.scrollHeight;
	var trmainbody = getElById('trmain').clientHeight;
	var mainbodyoffset = getElById('contentIframe').offsetHeight;
	var end = mainbody;
	if (end < (trmainbody-31))
		end = trmainbody-31;
	getElById('contentIframe').style.height=end;	//must be id
}

function getElById(idVal) {
	if (document.getElementById != null)
	return document.getElementById(idVal)
	if (document.all != null)
	return document.all[idVal]	
	alert("Problem getting element by id")
	return null;
}		

</SCRIPT>
<style>
.toplogo{
	background:#2bbdd4;
	width:180px;
	height:50px;
	padding-left: 80px;
}
.type_cmcc{
	background:#2bbdd4;
	width:605px;
	color:#fff;
	font-size: 15px;
}
.welcomeLink{
	background:#2bbdd4;
	color: #fff;
    font-size: 14px;
    font-weight: bold;
	display: inline-block;
	padding-right: 10px;
	padding-top: 15px;
}
.welcomeLink input{
	background:0 none;
	border: 0 none;
	color: #fff;
	font-weight: bold;
}
#topmenu span{
	color:#fff;
}
#topmenu  a{
	color:#fff;
	TEXT-DECORATION: none;
	font-family: "微软雅黑";
}
#lstmenu a{
	color: #fff;
	TEXT-DECORATION: none;
	font-weight: bold;
	font-family: "微软雅黑";
}
#submenu a{
	color:#fff;
	TEXT-DECORATION: none;
	font-family: "微软雅黑";
	cursor:pointer;
}
.menu_space {
    display: inline-block;
    padding: 0 8px;
}
#contentIframe {
    background: #f8f8f8;
}
.menusize {
    color: #ffffff;
    font-size: 14px;
    font-weight: bold;
    height: 70px;
    line-height: 25px;
    text-align: center;
    text-decoration: none;
    width: 70px;
	cursor:pointer;
}
p{
	text-align: center;
}
</style>
</HEAD>

<!-------------------------------------------------------------------------------------->
<!--主页代码-->
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onLoad="on_init();" align=center;>
<form action=/boafrm/formLogout method=POST name="top" target="_top">
	<table border="0" width="980" cellspacing="0" cellpadding="0" align="center" height="50">
		<tr bgcolor="#2bbdd4">
		<!--	<td class="toplogo"> 
				<img class="logoimg" src="image/mobile2.png">
			</td>
			<td class="type_cmcc">
			  型号：  CMCC
			</td>-->
			<td class="type_cmcc">
			  家庭网关
			</td>
			<td class="welcomeLink" align="right" valign="center">
			欢迎您！&nbsp;<input type="submit"      value="退出" onclick="return clearAuthData()">
			</td>
		</tr>
	</table>
	
	<table border="0" cellpadding="0" cellspacing="0" width="980" align="center"> 
		<tr nowrap bgcolor="#2bbdd4"> 
			<td width="150" rowspan="3" bgcolor="#2bbdd4" id="menuname">&nbsp;
			<td height="32"  width="830" style="font-size:9pt" align="right"></td>
		</tr>
		<tr> 
			<td height="70" bgcolor="#2bbdd4"> 
				<table border="0" cellpadding="0" cellspacing="0" width="665" height="70" bgcolor="#2bbdd4"> 	
					<tr id="topmenu" nowrap style="color:#fff;">
					  <td align="center" width="70">　</td>
					</tr>
				</table>
			</td>
		</tr>
	  <tr> 
		<td height="34" bgcolor="#303841"> 
		<table border="0" cellpadding="0" cellspacing="0" height="30">
			<tr id="submenu" style="font-size:9pt; color:#C0C0C0" nowrap> 
			  <td>　</td>
			</tr>
		</table>
		</td>
	  </tr>
	</table>


<div name="leftFrame" class="leftframe" style="margin:0px auto; width:980px; text-align: center;">


<table border="0" cellpadding="0" cellspacing="0">
  <tr valign="top" id='trmain'>
    <td bgcolor="#363e47" valign="top">
		<table border="0" cellpadding="0" cellspacing="0" width="150" id="lstmenu">
		<tr><td></td></tr> 
		</table>
	</td>
	<td width="830">
	<iframe id="contentIframe" name="contentIframe" align="middle" src="status_device_basic_info.asp" frameborder="0" width="830"  onload="contenFramesize();"></iframe>
	</td>
  </tr>
</table>
</div>

		
	<table cellSpacing=0 cellPadding=0 width=980 border=0 align="center">
		<tr>
			<td	bgcolor="#f8f8f8" width="150" height="50" align="right"></td>
			<td	bgcolor="#f8f8f8" width="680" height="50" align="center"><label size="12px" color="#333333"></label></td>
			<td	bgcolor="#f8f8f8" width="150" height="50" align="right"></td>
		</tr>
	</table>

	

</form>
</body>

</html>
