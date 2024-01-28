<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<html>
<head>
<title>家庭网关—时间管理服务器</title>
<meta http-equiv=pragma content=no-cache>
<meta http-equiv=cache-control content="no-cache, must-revalidate">
<meta http-equiv=content-type content="text/html; charset=utf-8">
<meta http-equiv=content-script-type content=text/javascript>
<!--系统公共css-->
<style type=text/css>
@import url(/style/default.css);
</style>
<!--系统公共脚本-->
<script language="javascript" src="common.js"></script>
<script language="javascript" type="text/javascript">

var ntpServers = new Array();

ntpServers.push("clock.fmt.he.net");
ntpServers.push("clock.nyc.he.net");
ntpServers.push("clock.sjc.he.net");
ntpServers.push("clock.via.net");
ntpServers.push("ntp1.tummy.com");
ntpServers.push("time.cachenetworks.com");
ntpServers.push("time.nist.gov");

function ntpChange(optionlist, textbox)
{
	if (document.forms[0].ntpEnabled.checked)
	{
		if( optionlist.value == "Other" )
		{
			textbox.disabled = false;
		}
		else
		{
			textbox.value = "";
			textbox.disabled = true;
		}	
	}	
}

function ntpEnblChange()
{
	if (document.forms[0].ntpEnabled.checked)
		status = 'visible';
	else
		status = 'visible';   //others alway visible

	if (document.getElementById)
		document.getElementById('ntpConfig').style.visibility = status;
	else if (!document.layers)
		document.all.ntpConfig.style.visibility = status;
		
	if (!document.forms[0].ntpEnabled.checked)	
		document.getElementById('ntpTZ').style.visibility = 'hidden';
	else
		document.getElementById('ntpTZ').style.visibility = status;
}

function writeNtpList(select, other, server, needed)
{
	var flag = 0;

	if (!needed)
		select.add(new Option("None"));

	for (var i = 0; i < ntpServers.length; i++) {
		if (server.value == ntpServers[i]) {
			select.add(new Option(ntpServers[i], ntpServers[i], true, true));
			flag = 1;
		} else {
			select.add(new Option(ntpServers[i]));
		}
	}

	if (flag || !needed && server.value.length == 0) {
		select.add(new Option("Other"));
		other.disabled = true;
		other.value = "";
	} else {
		select.add(new Option("Other", "Other", true, true));
		other.disabled = false;
		other.value = server.value;
	}
}

function sntp_init()
{
	var select1 = document.getElementById("ntpServerHost1");
	var select2 = document.getElementById("ntpServerHost2");
	if (document.forms[0].ntpEnabled.checked)
	{
		document.getElementById("ntpServerOther1").disabled = true;
		document.getElementById("ntpServerOther2").disabled = true;
	}
	else
	{
		document.getElementById("ntpServerOther1").disabled = false;
		document.getElementById("ntpServerOther2").disabled = false;	
		document.getElementById("ntpServerOther1").value = select1.value;	
		document.getElementById("ntpServerOther2").value = select2.value;		
	}

}

function if_typeChange()
{
	var if_type = document.forms[0].if_type.value;
	var if_wan = document.forms[0].if_wan;
	var keyword, i;

	switch (if_type) {
		case "0":
			keyword = "INTERNET";
			break;
		case "1":
			keyword = "VOICE";
			break;
		case "2":
			keyword = "TR069";
			break;
		case "3":
		default:
			keyword = "Other";
			break;
	}

	for (i = 0; i < if_wan.options.length; i++) {
		if (if_wan.options[i].text.search(keyword) == -1) {
			//if_wan.options[i].style.display = "none";		//this is not workable with IE
			if_wan.options[i].disabled = true;
		} else {
			//if_wan.options[i].style.display = "block";
			if_wan.options[i].disabled = false;
		}
	}

	/* deselect when the previous selected interface does not match the new selected if_type */
	if (if_wan.selectedIndex == -1 || (if_wan.selectedIndex >= 0 && if_wan.options[if_wan.selectedIndex].text.search(keyword) == -1))
	{
		if_wan.selectedIndex = -1;

		/* Find first available interface */
		for (i = 0; i < if_wan.options.length; i++)
		{
			if (if_wan.options[i].text.search(keyword) > 0)
				if_wan.selectedIndex = i;
		}
	}
}

/********************************************************************
**          on document submit
********************************************************************/
function on_submit()
{
	var select1 = document.getElementById("ntpServerHost1");
	var srv1 = select1.options[select1.selectedIndex].text;
	var select2 = document.getElementById("ntpServerHost2");
	var srv2 = select2.options[select2.selectedIndex].text;
	with (document.forms[0]) {
		if (ntpEnabled.checked) {
			if (srv1 == "Other") {
				if (ntpServerOther1.value.length == 0) {	// == Other
					alert
					    ('主SNTP服务器设置为其他，但其他栏为空。');
					return;
				} else {
					server1.value = document.timeZone.ntpServerOther1.value;
				}
			} else {
				server1.value = srv1;
			}

			if (srv2 == "Other") {
				if (ntpServerOther2.value.length == 0) {	// == Other
					alert
					    ('从SNTP服务器设置为其他，但其他栏为空。');
					return;
				} else {
					server2.value = ntpServerOther2.value;
				}
			} else {
				if (ntpServerHost2.selectedIndex > 0)
					server2.value = srv2;
				else
					server2.value = "";
			}

			if (interval.value == 0) {
				alert("同步间隔设置不可为 0。");
				return;
			}
		}
	}
}

</script>
</head>

<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onload="sntp_init()">
	<blockquote>
	<div align="left" style="padding-left:20px; padding-top:10px">
	  <form action=/boaform/admin/formTimezone method=POST name="timeZone" id="timeZone">
		SNTP开关<input type='checkbox' name='ntpEnabled' onClick='ntpEnblChange()'>
		<br>
		<div id='ntpConfig'>
		  <table border="0" cellpadding="0" cellspacing="0">
		  	<tr>
			  <td width='150'>&nbsp;系統日期时间:</td>
			  <td colspan=2>
			  	1970年1月1日11时34分
			  </td>
			</tr>
			<tr>
			  <td width='150'>&nbsp;绑定的WAN连接:</td>
			  <td colspan=2>
			  <select name='if_type' onClick='if_typeChange()'>
				<option value = "0" selected>INTERNET</option>
				<option value = "1">VOICE</option>
				<option value = "2">TR069</option>
				<option value = "3">Other</option>
			  </select>
			  </td>
			</tr>
			<tr>
			  <td width='150'>&nbsp;主SNTP服务器:</td>
			  <td>
			  <select id='ntpServerHost1' name='ntpServerHost1' size="1" onChange='ntpChange(ntpServerHost1, ntpServerOther1)'>
			  </select>
			  </td>
			  <td>&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;
			  <input maxlength=63 name='ntpServerOther1' id='ntpServerOther1'></td>
			</tr>
			<tr>
			  <td>&nbsp;从SNTP服务器:</td>
			  <td>
			  <select id='ntpServerHost2' name='ntpServerHost2' size="1" onChange='ntpChange(ntpServerHost2, ntpServerOther2)'>
			  </select>
			  </td>  
			  <td>&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;
			  <input maxlength=63 name='ntpServerOther2' id='ntpServerOther2'></td>
			</tr>
		  </table>
		  <table border="0" cellpadding="0" cellspacing="0">
			<tr>
			  <td width='150'>&nbsp;同步间隔:</td>
			  <td colspan=2><input type='text' name='interval'> s</td>
			</tr>
			<td style="display:none;" 'width='150' value="同步的WAN连接:"></td>
			  <select style="display:none;" name="if_wan">
			  <option value=130817>2_INTERNET_R_VID_2</option>

			  </select>
		  </table>
		  <br>
		  <br><br>
		</div>
		<div id='ntpTZ'>
			<tr>
			  <td width='150'>&nbsp;时间区域:</td>
			  <br><br><td colspan=2>&nbsp;
			  <select name='tmzone' size="1">
				  <option value="0">(GMT-12:00)国际日期变更线(西)</option><option value="1">(GMT-12:00)中途岛，萨摩亚</option><option value="2">(GMT-10:00)夏威夷</option><option value="3">(GMT-09:00)阿拉斯加</option><option value="4">(GMT-08:00)美国西部标准时间，提华纳</option><option value="5">(GMT-07:00)山地时区(加拿大)</option><option value="6">(GMT-07:00)奇瓦瓦，拉巴斯</option><option value="7">(GMT-07:00)亚利桑那州，马萨特兰</option><option value="8">(GMT-06:00)中央标准时间(用于美国和加拿大中部)</option><option value="9">(GMT-06:00)萨斯喀彻温省</option><option value="10">(GMT-06:00)瓜达拉哈拉，墨西哥城，蒙特雷</option><option value="11">(GMT-06:00)中美洲</option><option value="12">(GMT-05:00)东部时间</option><option value="13">(GMT-05:00)印第安那州</option><option value="14">(GMT-05:00)波哥大，利马，基多</option><option value="15">(GMT-04:00)大西洋时间</option><option value="16">(GMT-04:00)加拉加斯，拉巴斯</option><option value="17">(GMT+04:00)圣地亚哥</option><option value="18">(GMT-03:30)纽芬兰</option><option value="19">(GMT-03:00)巴西利亚</option><option value="20">(GMT-03:00)布宜诺斯艾利斯，乔治敦</option><option value="21">(GMT-03:00)格陵兰</option><option value="22">(GMT-02:00)中大西洋地区</option><option value="23">(GMT-01:00)亚述尔群岛</option><option value="24">(GMT-01:00)佛得角群岛</option><option value="25">(GMT+00:00)格林尼治标准时间：都柏林，爱丁堡，里斯本,伦敦</option><option value="26">(GMT+00:00)卡萨布兰卡，蒙罗维亚</option><option value="27">(GMT+01:00)贝尔格莱德，布拉迪斯拉发，卢布尔雅那，布拉格</option><option value="28">(GMT+01:00)萨拉热窝，斯科普里，华沙，萨格勒布</option><option value="29">(GMT+01:00)布鲁塞尔，哥本哈根，马德里，巴黎</option><option value="30">(GMT+01:00)阿姆斯特丹，柏林，伯尔尼, 罗马，斯德哥尔摩，维也纳</option><option value="31">(GMT+01:00)中西非</option><option value="32">(GMT+02:00)布加勒斯特</option><option value="33">(GMT+02:00)开罗</option><option value="34">(GMT+02:00)赫尔辛基，基辅，里加，索非亚，塔林, 维尔纽斯</option><option value="35">(GMT+02:00)雅典，伊斯坦布尔，明斯克</option><option value="36">(GMT+02:00)耶路撒冷</option><option value="37">(GMT+02:00)哈拉雷，比勒陀利亚</option><option value="38">(GMT+03:00)莫斯科，圣彼得堡，伏尔加格勒</option><option value="39">(GMT+03:00)科威特，利雅得</option><option value="40">(GMT+03:00)奈洛比</option><option value="41">(GMT+03:00)巴格达</option><option value="42">(GMT+03:30)德黑兰</option><option value="43">(GMT+04:00)阿布扎比，马斯喀特</option><option value="44">(GMT+04:00)巴库，第比利斯，耶烈万</option><option value="45">(GMT+04:30)喀布尔</option><option value="46">(GMT+05:00)叶卡特琳堡</option><option value="47">(GMT+05:00)伊斯兰堡，卡拉奇，塔什干</option><option value="48">(GMT+05:30)清奈，加尔各答，孟买，新德里</option><option value="49">(GMT+05:45)加德满都</option><option value="50">(GMT+06:00)阿斯坦纳，达卡</option><option value="51">(GMT+06:00)斯里兰卡</option><option value="52">(GMT+06:00)阿拉木图，新西伯利亚</option><option value="53">(GMT+06:30)仰光</option><option value="54">(GMT+07:00)曼谷，河内，雅加达</option><option value="55">(GMT+07:00)拉斯诺亚尔斯克</option><option value="56" selected>(GMT+08:00)北京，重庆，香港，乌鲁木齐</option><option value="57">(GMT+08:00)吉隆坡，新加坡</option><option value="58">(GMT+08:00)台北</option><option value="59">(GMT+08:00)珀斯</option><option value="60">(GMT+08:00)伊尔库次克，乌兰巴托</option><option value="61">(GMT+09:00)首尔</option><option value="62">(GMT+09:00)大阪，札幌，东京</option><option value="63">(GMT+09:00)雅库茨克</option><option value="64">(GMT+09:30)达尔文</option><option value="65">(GMT+09:30)阿德莱德</option><option value="66">(GMT+10:00)堪培拉，墨尔本，悉尼</option><option value="67">(GMT+10:00)布里斯班</option><option value="68">(GMT+10:00)霍巴特</option><option value="69">(GMT+10:00)符拉迪沃斯托克</option><option value="70">(GMT+10:00)关岛，莫尔兹比港</option><option value="71">(GMT+11:00)马加丹</option><option value="72">(GMT+12:00)斐济，堪察加半岛，马绍尔群岛</option><option value="73">(GMT+12:00)奥克兰，惠灵顿</option>
			  </select>
			  </td>
			</tr>
		</div>
		<br><br><br><br><br>
		<input type='hidden' name="server1" value="">
		<input type='hidden' name="server2" value="">
		<input type="hidden" name="submit-url" value="/net_sntp_cmcc.asp">
		<div><tr><center>			
		<input class="btnsaveup" type="submit" value="确定" onClick="on_submit();">&nbsp;&nbsp;
		<input type="button" class="btnsaveup2" value="取消" onclick="window.location.reload()">
		</center></tr></div>
		</form>	
	  <script language="javascript">
		  with (document.forms[0]) {
			  ntpEnabled.checked = true;
server1.value = "time.windows.com";
server2.value = "time.nist.gov";
if_type.selectedIndex = 0;
if_typeChange();
if_wan.value = 130816;
interval.value = 86400;

			  writeNtpList(ntpServerHost1, ntpServerOther1, server1, true);
			  writeNtpList(ntpServerHost2, ntpServerOther2, server2, false);
			  ntpEnblChange();
		  }
	  </script>
	  </div>
	</blockquote>
</body>

</html>
