<!-- add by liuxiao 2008-01-21 -->
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
<title>中国电信</title>
<meta http-equiv=pragma content=no-cache>
<meta http-equiv=cache-control content="no-cache, must-revalidate">
<meta http-equiv=content-type content="text/html; charset=utf-8">
<meta http-equiv="refresh" content="5">
<meta http-equiv=content-script-type content=text/javascript>
<style type=text/css>
@import url(/style/default.css);
</style>
<script language="javascript" src="common.js"></script>
<SCRIPT language="javascript" type="text/javascript">

var rcs = new Array();
rcs.push(new it_nr("0", new it("ifname", "无线"), new it("rx_packets", 1110), new it("rx_bytes", 128888), new it("rx_errors", 0), new it("rx_dropped", 0), new it("tx_packets", 0), new it("tx_bytes", 0), new it("tx_errors", 0), new it("tx_dropped", 734)));

var wlDefChannel=new Array();
var wlan_root_interface_up = new Array();
wlan_root_interface_up[0]=1;

var Band2G5GSupport=new Array();
var wlan_parm = new Array();
wlDefChannel[0]=149;
Band2G5GSupport[0]=2;
wlan_parm.push(new it_nr("0", new it("ssid", "0"), new it("ssid_idx", 5), new it("hiddenssid", 0), new it("band", 76), new it("disabled", 0), new it("bssid", "00:e0:4c:86:70:05"), new it("encrypt_state", 1), new it("auth_mode", "WPA-PSK/WPA2-PSK"), new it("encrypt_mode", "TKIP+AES")));


/********************************************************************
**          on document load
********************************************************************/
function on_init()
{
	if (lsteth.rows) {
		while (lsteth.rows.length > 2)
			lsteth.deleteRow(2);
	}

	for (var i = 0; i < rcs.length; i++) {
		var row = lsteth.insertRow(i + 2);

		row.nowrap = true;
		//row.style.verticalAlign = "top";
		row.style.textAlign = "center";

		var cell = row.insertCell(0);
		cell.innerHTML = rcs[i].ifname;
		cell = row.insertCell(1);
		cell.innerHTML = rcs[i].rx_bytes;
		cell = row.insertCell(2);
		cell.innerHTML = rcs[i].rx_packets;
		cell = row.insertCell(3);
		cell.innerHTML = rcs[i].rx_errors;
		cell = row.insertCell(4);
		cell.innerHTML = rcs[i].rx_dropped;
		cell = row.insertCell(5);
		cell.innerHTML = rcs[i].tx_bytes;
		cell = row.insertCell(6);
		cell.innerHTML = rcs[i].tx_packets;
		cell = row.insertCell(7);
		cell.innerHTML = rcs[i].tx_errors;
		cell = row.insertCell(8);
		cell.innerHTML = rcs[i].tx_dropped;
	}
	
	if (wl_settings.rows) {
		while (wl_settings.rows.length > 1)
			wl_settings.deleteRow(1);
	}
	
	var len = 0;
	for (var i = 0; i < wlan_parm.length; i++) {

			var row = wl_settings.insertRow(len + 1);

			row.nowrap = true;
			//row.style.verticalAlign = "top";
			row.style.textAlign = "center";
			
			var cell = row.insertCell(0);
			cell.innerHTML = "SSID-" + wlan_parm[i].ssid_idx;
			cell = row.insertCell(1);
			cell.innerHTML = wlan_parm[i].ssid;
			cell = row.insertCell(2);
			cell.innerHTML = (wlan_parm[i].encrypt_state) ? "已配置": "未配置";
			cell = row.insertCell(3);
			cell.innerHTML = wlan_parm[i].auth_mode;
			cell = row.insertCell(4);
			cell.innerHTML = wlan_parm[i].encrypt_mode;
			len ++;
	}
}

var wlan_num = 2;

var wlan_module_enable = 1;

   
</script>
</head>
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onLoad="on_init();">
	<blockquote>
		<div align="left" style="padding-left:20px;"><br>
			<div align="left" style="width:768px;"><br>
			<table class="flat" border="1" cellpadding="0" cellspacing="1" width="90%">
<script>


document.write('\
<tr>\
<td width="20%" class="hdb">无线网络开关状态</td>\
<td class="hdt" width="80%">');
if(!wlan_module_enable)
	document.write('停用');
else
	document.write('启用');
document.write(	'</td></tr>');

if(wlan_module_enable){
	if(wlan_root_interface_up[0]){
		document.write('<tr><td class="hdb">信道</td><td class="hdt">'+wlDefChannel[0]+'</td></tr>');
	}
}
</script>
			</table>
			</div>
		</div>
		<br><br>
		<div align="left" style="padding-left:20px;">
			<br>
			<table id="lsteth" class="flat" border="1" cellpadding="1" cellspacing="1" width="100%">
			   <tr class="hdb" align="center" nowrap>
					<td>接口</td>
					<td colspan="4">接收</td>
					<td colspan="4">发送</td>
				</tr>
			   <tr class="hdb" align="center" nowrap>
					<td>&nbsp;</td>
					<td>字节</td>
					<td>包</td>
					<td>错误</td>
					<td>丢弃</td>
					<td>字节</td>
					<td>包</td>
					<td>错误</td>
					<td>丢弃</td>
				</tr>
			</table>
		</div>
		<br><br>
		<div align="left" style="padding-left:20px;">
			<br>
			<table id="wl_settings" class="flat" border="1" cellpadding="1" cellspacing="1" width="100%">
			<tr class="hdb" align="center" nowrap>
			<td width="20%">SSID索引</td>
			<td width="20%">SSID名称</td>
			<td width="20%">安全配置</td>
			<td width="20%">认证方式</td>
			<td width="20%">加密</td>
			</tr>
			</table>
		</div>
	</blockquote>
</body>
<!-- add end by liuxiao 2008-01-21 -->

</html>
