<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html">
<% getIndex("no-cache"); %>
<title>LUNA_L2TP接口设置 </title>
<SCRIPT language=Javascript src="<% getInfo("lang");%>"></SCRIPT>
<% getInfo("include_css"); %>
<style>
.on {display:on}
.off {display:none}
</style>
<% language=javascript %>
<script type="text/javascript" src="util_gw.js"> </script>
<SCRIPT>
function checkTextStr(str)
{
	for (var i=0; i<str.length; i++) 
	{
		if ( str.charAt(i) == '%' || str.charAt(i) =='&' ||str.charAt(i) =='\\' || str.charAt(i) =='?' || str.charAt(i)=='"') 
			return false;			
	}
	return true;
}

function connTypeChange()
{		
	document.l2tp.defaultgw.checked = false;
	document.l2tp.defaultgw.disabled = false;	
	if(document.l2tp.pppconntype.selectedIndex==1)
	{
		document.l2tp.idletime.disabled=false;		
		document.l2tp.defaultgw.checked = true;
		document.l2tp.idletime.disabled=false;		
	}else{
		document.l2tp.idletime.disabled=true;		
	}
}

function l2tpSelection()
{
	if (document.l2tp.l2tpen[0].checked) {
		document.l2tp.username.disabled = true;
		document.l2tp.password.disabled = true;
		document.l2tp.pppconntype.disabled = true;
		document.l2tp.idletime.disabled = true;
		document.l2tp.MtuSize.disabled = true;
		document.l2tp.defaultgw.disabled = true;
		//document.l2tp.GetServMode.disabled = true;
		//document.l2tp.ServerDomainName.disabled = true;
		//document.l2tp.ServerIpAddr.disabled = true;
	}
	else {
		document.l2tp.username.disabled = false;
		document.l2tp.password.disabled = false;
		document.l2tp.pppconntype.disabled = false;
		document.l2tp.idletime.disabled = false;
		document.l2tp.MtuSize.disabled = false;
		document.l2tp.defaultgw.disabled = false;
		//document.l2tp.GetServMode.disabled = false;
		//document.l2tp.ServerDomainName.disabled = false;
		//document.l2tp.ServerIpAddr.disabled = false;
	}
	connTypeChange()
}

function onClickL2TPEnable()
{
	l2tpSelection();
	
	if (document.l2tp.l2tpen[0].checked)
		document.l2tp.lst.value = "disable";
	else
		document.l2tp.lst.value = "enable";
	//document.l2tp.submit();
}

function addL2TPItf(obj)
{
	if(document.l2tp.l2tpen[0].checked)
		return false;
	
	if (document.l2tp.ServerIpAddr.value=="") {		
		alert('<% multilang(LANG_PLEASE_ENTER_L2TP_SERVER_ADDRESS); %>');
		document.l2tp.ServerIpAddr.focus();
		return false;
	}
	
	if(!checkTextStr(document.l2tp.ServerIpAddr.value))
	{		
		alert('<% multilang(LANG_INVALID_VALUE_IN_SERVER_ADDRESS); %>');
		document.l2tp.ServerIpAddr.focus();
		return false;		
	}

	//document.l2tp.auth.selectedIndex!=3
	if (1)
	{
		if (document.l2tp.username.value=="")
		{			
			alert('<% multilang(LANG_PLEASE_ENTER_L2TP_CLIENT_USERNAME); %>');
			document.l2tp.username.focus();
			return false;
		}
		if(!checkTextStr(document.l2tp.username.value))
		{			
			alert('<% multilang(LANG_INVALID_VALUE_IN_USERNAME); %>');
			document.l2tp.username.focus();
			return false;		
		}
		if (document.l2tp.password.value=="") {			
			alert('<% multilang(LANG_PLEASE_ENTER_L2TP_CLIENT_PASSWORD); %>');
			document.l2tp.password.focus();
			return false;
		}
		if(!checkTextStr(document.l2tp.password.value))
		{			
			alert('<% multilang(LANG_INVALID_VALUE_IN_PASSWORD); %>');
			document.l2tp.password.focus();
			return false;		
		}
	}

	if (document.l2tp.pppconntype.selectedIndex==1)
	{
		if (document.l2tp.idletime.value=="")
		{			
			alert('<% multilang(LANG_PLEASE_ENTER_L2TP_TUNNEL_IDLE_TIME); %>');
			document.l2tp.idletime.focus();
			return false;
		}
		
		if (document.l2tp.defaultgw.checked==false)
		{			
			alert('<% multilang(LANG_PLEASE_SELECT_DEFAULT_GATEWAY_FOR_DIAL_ON_DEMAND); %>');
			document.l2tp.defaultgw.focus();
			return false;
		}
	}

	if (document.l2tp.MtuSize.value=="")
	{		
		alert('<% multilang(LANG_PLEASE_ENTER_L2TP_TUNNEL_MTU); %>');
		document.l2tp.MtuSize.focus();
		return false;
	}
	
	return true;
}

//l2tp new added
function l2tpGetServByDomainNameClicked()
{
	disableTextField(document.l2tp.ServerIpAddr);
	enableTextField(document.l2tp.ServerDomainName);	
}
function l2tpGetServByIpClicked()
{
	disableTextField(document.l2tp.ServerDomainName);
	enableTextField(document.l2tp.ServerIpAddr);

}

function on_load()
{
	document.l2tp.l2tpen[0].checked=true;
	document.l2tp.l2tpen[1].checked=false;
	
	if(<% getIndex("l2tpGetServIpByDomainName"); %>){
		document.l2tp.GetServMode[0].checked = true;
		document.l2tp.GetServMode[1].checked = false;
		disableTextField(document.l2tp.ServerIpAddr);
		enableTextField(document.l2tp.ServerDomainName);	
	}else{
		document.l2tp.GetServMode[0].checked = false;
		document.l2tp.GetServMode[1].checked = true;
		disableTextField(document.l2tp.ServerDomainName);
		enableTextField(document.l2tp.ServerIpAddr);
	}

	var connectType=<% getIndex("l2tpConnectType"); %>;
	document.l2tp.pppconntype.value=connectType;
	if(connectType==1)
	{
		document.l2tp.idletime.disabled=false;		
	}else{
		document.l2tp.idletime.disabled=true;		
	}

	l2tpSelection();
}
</SCRIPT>
</head>

<body onload="on_load();">
<blockquote>

<h2>L2TP VPN <script>dw(l2tp_configuration)</script></h2>

<form action=/boafrm/formL2tpCfg method=POST name="l2tp">

<table border=0 width="550" cellspacing=0 cellpadding=0>
  <tr><font size=2>
	<script>dw(l2tp_this_pate_is_used_to_configure_the_parameters_for_l2tp_mode_vpn)</script>
  </tr>
  <tr><hr size=1 noshade align=top></hr></tr>
</table>
<table border="0" width=480>
	<tr>
		<td><font size=2><b>L2TP VPN:</b>
	  	<input type="radio" value="0" name="l2tpen" onClick="onClickL2TPEnable()"><script>dw(l2tp_disabled)</script>&nbsp;&nbsp;
	 	<input type="radio" value="1" name="l2tpen" onClick="onClickL2TPEnable()"><script>dw(l2tp_enabled)</script>
	 	</font></td>
	</tr>
</table>
<input type="hidden" id="lst" name="lst" value="">
<br />
<table border="0" width=480>
	<tr>
      	<td width="100%" colspan="2"><font size=2>
		<b><input type="radio" value="GetServByDomainName" name="GetServMode" onClick="l2tpGetServByDomainNameClicked()"><script>dw(tcpip_wan_attain_by_DN)</script></b>
      	</td>
    </tr>
    <tr>
    	<td width="100%" colspan="2"><font size=2>
		<b><input type="radio" value="l2tpGetServByIp" name="GetServMode" onClick="l2tpGetServByIpClicked()"><script>dw(tcpip_wan_attain_by_Ip)</script></b>
      	</td>
    </tr>
    <tr>
      <td width="30%"><font size=2><b><script>dw(tcpip_domain)</script></b></td>
      <td width="70%"><font size=2><input type="text" name="ServerDomainName" size="18" maxlength="30" value="<% getInfo("l2tpServerDomain"); %>"></td>
    </tr>
   
    <tr>
      <td width="30%"><font size=2><b><script>dw(tcpip_wan_server_ip)</script></b></td>
      <td width="70%"><font size=2><input type="text" name="ServerIpAddr" size="18" maxlength="30" value="<% getInfo("l2tpServerIp"); %>"></td>
    </tr>
<!-- added end-->

	<tr>
	  <td width="30%"><font size=2><b><script>dw(l2tp_user)</script>:</b></td>
	  <td width="70%"><font size=2><input type="text" name="username" size="15" maxlength="35" value="<% getInfo("l2tpUserName"); %>"></td>
	</tr>
	<tr>
	  <td width="30%"><font size=2><b><script>dw(l2tp_password)</script>:</b></td>
	  <td width="70%"><font size=2><input type="text" name="password" size="15" maxlength="35" value="<% getInfo("l2tpPassword"); %>"></td>
	</tr>

	<tr>
		<td width="30%"><font size=2><b>PPP <script>dw(l2tp_connection_type)</script>:<b></td>
		<td width="70%"><font size=2><select name="pppconntype" onChange="connTypeChange()" value="<% getIndex("l2tpConnectType"); %>">
				<option value="0"><script>dw(l2tp_persistent)</script></option>
				<option value="1"><script>dw(l2tp_dial_on_demand)</script></option>
				<option value="2"><script>dw(l2tp_manual)</script></option>
			</select>
		</td>
	</tr>
	<tr>
	
		<td width="30%"><font size=2><b><script>dw(l2tp_idle_time_sec)</script>:</b></td>
		<td width="70%"><font size=2><input type="text" name="idletime" size="32" maxlength="256" value="<% getInfo("wan-l2tp-idle");%>"></td>
	</tr>
	<tr>
		<td width="30%"><font size=2><b>MTU:</b></td>
		<td width="70%"><font size=2><input type="text" name="MtuSize" size="32" maxlength="256" value="<% getInfo("l2tpMtuSize"); %>"></td>
	</tr>
	<tr>
		<td width="30%"><font size=2><b><script>dw(l2tp_default_gateway)</script>:</b></td>
		<td width="70%"><font size=2><input type="checkbox" name="defaultgw"></td>
	</tr>

</table>

<br />

<br />
<br />
<div class="btn_ctl">
<p><input type="submit" value="Save" name="save" onClick="return addL2TPItf(this)">&nbsp;&nbsp;
    <script>document.l2tp.save.value = tcpip_lan_save;</script>
	<input type="submit" value="Save & Apply" name="save_apply" onClick="return addL2TPItf(this)">&nbsp;&nbsp;
	<script>document.l2tp.save_apply.value = tcpip_apply;</script>
	<input type="reset" value="Reset" name="reset" onClick="resetClicked()">
	<script>document.l2tp.reset.value = tcpip_reset;</script>
</p>
</div>

<br />
<div class="btn_ctl">
	<input type="hidden" value="/l2tp.asp" name="submit-url">
</div>
<script>
	//l2tpSelection();
</script>
</form>
</blockquote>
</body>
</html>
