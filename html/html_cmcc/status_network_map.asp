<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<HTML>
<HEAD>
<TITLE>Network connection status</TITLE>
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
<SCRIPT language=Javascript src="<% getInfo("lang");%>"></SCRIPT>
<style type="text/css">
.wizard_main{
 padding-bottom: 10px;
}
.wizard_main h2{
 text-align: left;
 color: #fff;
 font-size: 18px;
 padding: 0 60px;
 font-weight: normal;
}
.wizard_main h2 span{
 color: #ccc;
 font-size: 14px;
 padding-right: 10px;
}
.wizard_main h2 img{
 background: url(image/line.jpg) repeat-x;
 width: 360px;
 height: 1px;
 padding-left: 10px;
}
.wizard_main h2 i{
 background: #ccc;
 width: 50px;
 height:20px
}
.image_01{
 background:url(image/pc.jpg) 10px 0 no-repeat;
 width: 134px;
 height: 117px;
}
.image_02{
 background:url(image/connect_yes.jpg) 5px 15px no-repeat;
 width: 114px;
 height: 117px;
}
.image_03{
 background:url(image/route.jpg) no-repeat;
 width: 136px;
 height: 117px;
}
.image_04{
 background:url(image/connect_no.jpg) 5px 15px no-repeat;
 width: 114px;
 height: 117px;
}
.image_05{
 background:url(image/global_no.jpg) 10px 0 no-repeat;
 width: 136px;
 height: 117px;
}
.image_06{
 background:url(image/global_yes.jpg) 10px 0 no-repeat;
 width: 136px;
 height: 117px;
}
.setting_table_local{
 background: #d4d4d4;
 width: 720px;
 text-align: left;
}
.setting_client{
 padding-left: 15px;
}
</style>

<SCRIPT language="javascript" type="text/javascript">

var cgi = new Object();
/********************************************************************
**          on document load
********************************************************************/

var wanType = "没有 internet WAN";
var wanStatus = 0;
var internetip = "0.0.0.0";
var wirelessClientNum = 0;
var wiredClientNum = 0;
var clts = new Array();
var wlanlist = new Array();


/*
clts.push(new it_nr("0", new it("macAddr", "08:57:00:e7:60:31"), new it("ipAddr", "192.168.0.121")));
clts.push(new it_nr("1", new it("macAddr", "f0:1f:af:67:71:99"), new it("ipAddr", "192.168.0.33")));

wiredClientNum = 2;
wirelessClientNum = 0;

wanStatus = 1;
internetip = "192.168.3.100";
*/

 <% dhcpWiredClientList(); %>;
 <% dhcpWirelessClientList(); %>;

wiredClientNum = <%getIndex("wired_dhcp_client_num")%>;
wirelessClientNum = <%getIndex("wireless_dhcp_client_num")%>;
wanStatus = <%getIndex("PingStatus")%>;


function updateChanX()
{
	var clientNum = wirelessClientNum + wiredClientNum;
//	document.getElementById("wantype").innerText = wanType;	
//	document.getElementById("id_internetip").innerText = "Internet IP: " + internetip;
	document.getElementById("userNum").innerText = "  用户数: " + clientNum;
}

function postload()
{

	if(1 == wanStatus)
	{
		document.getElementById("id_wanclass").className = 'image_02';
		document.getElementById("id_wanclass_global").className = 'image_06';
		
	}else{
		document.getElementById("id_wanclass").className = 'image_04';
		document.getElementById("id_wanclass_global").className = 'image_05';		
	}

/*
	if(1 == wanStatus)
	{
		document.getElementById("id_wanclass").className = 'image_02';
		
	}

	if(internetip.indexOf("0.0.0.0") < 0 )
	{
		document.getElementById("id_wanclass").className = 'image_02';
		document.getElementById("id_wanclass_global").className = 'image_06';
	}
	else
	{
		document.getElementById("id_wanclass").className = 'image_04';
		document.getElementById("id_wanclass_global").className = 'image_05';
	}
*/
}

function updateChan()
{
	postload();
	updateChanX();
}


function on_init()
{
	updateChan();
	
	for (var i = 0; i < clts.length; i++) {
		var row = lstdev.insertRow(i + 1);

		row.nowrap = true;
		row.style.verticalAlign = "top";
		row.style.textAlign = "center";

		var cell = row.insertCell(0);
		cell.innerHTML = clts[i].ipAddr;
		cell = row.insertCell(1);
		cell.innerHTML = clts[i].macAddr;
	}

	for (var i = 0; i < wlanlist.length; i++) {
		var row = wlandev.insertRow(i + 1);

		row.nowrap = true;
		row.style.verticalAlign = "top";
		row.style.textAlign = "center";

		var cell = row.insertCell(0);
		cell.innerHTML = wlanlist[i].ipAddr;
		cell = row.insertCell(1);
		cell.innerHTML = wlanlist[i].macAddr;
	}
}

</script>

</head>
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onload="on_init()">
<div align="left" style="padding-left:20px; padding-top:5px">	
	<b>网络拓扑</b>	
	<br><br>
</div>
<div class="wizard_main" style="padding-left:20px;">
	<table class="setting_table_local">
		<tr>
            <td align="center" width="90" class="setting_client" height="30">客户端</td>
			<td align="center" width="122"> </td>
            <td align="center" width="136">路由器</td>
			<td align="center" width="98"> </td>
            <td align="center" width="136">Internet</td>
		</tr>
	</table>
	<table class="setting_table_local">
		<tr>
			<td class="image_01"></td>
			<td class="image_02"></td>
			<td class="image_03"></td>
			<td id="id_wanclass" class="image_04"></td>
			<td id="id_wanclass_global" class="image_05"></td>
		</tr>
	</table>
	<table class="setting_table_local">
		<tr>
            <td class="setting_client" id="userNum" width="124" height="30" align="center"></td>
            <td width="144"></td>
            <td id="id_internetip" width="280"></td>
			<td align="center" id="wantype"></td>
			
		</tr>
	</table>
</div>
<br>
<div id="clientlist">
	<div align="left" style="padding-left:20px;">
		<div align="left"><b>有线连接：</b></div>
		<table id="lstdev" class="flat" border="1" cellpadding="1" cellspacing="1">
			<tr class="hdb" align="center" nowrap><font size=2>
				<td width="150">IP地址</td>
				<td width="150">MAC地址</td>
				</font>
			</tr>
		</table>
	</div>
	<br><br>
	<div align="left" style="padding-left:20px;">
		<div align="left"><b>无线连接：</b></div>
		<table id="wlandev" class="flat" border="1" cellpadding="1" cellspacing="1">
			<tr class="hdb" align="center" nowrap><font size=2>
				<td width="150">IP地址</td>
				<td width="150">MAC地址</td>
				</font>
			</tr>
		</table>
	</div>
</div>
</body>
</html>

