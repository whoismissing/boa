<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<HTML>
<HEAD>
<TITLE>UPnP</TITLE>
<META http-equiv=pragma content=no-cache>
<META http-equiv=cache-control content="no-cache, must-revalidate">
<META http-equiv=content-type content="text/html; charset=utf-8">
<META http-equiv=content-script-type content="text/javascript">
<!--系统公共css-->
<STYLE type=text/css>
@import url(/style/default.css);
</STYLE>
<!--系统公共脚本-->
<SCRIPT language="javascript" src="common.js"></SCRIPT>
<SCRIPT language=Javascript src="<% getInfo("lang");%>"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript">

/********************************************************************
**          on document load
********************************************************************/
var upnp_enabled = <%getIndex("internet_wan_upnp_enabled")%>;
function Load_Setting()
{
	if(upnp_enabled != 0){
		document.upnp.upnpEnabled.checked = true;
		disableWanIfButton(false);
		document.getElementById("wanIf"+upnp_enabled).checked = true;
	//	document.upnp.wanIf.value = upnp_enabled;
	}
	else{
		document.upnp.upnpEnabled.checked = false;
		disableWanIfButton(true);
	}
}
</SCRIPT>
</HEAD>

<!-------------------------------------------------------------------------------------->
<!--主页代码-->
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onLoad="Load_Setting();">
<blockquote>
<form action=/boafrm/formUpnp method=POST name="upnp">
	<br>
	<table border="0" width=600>
		<tr>
		<td colspan="2"><font size=2><b>
			<input type="checkbox" name="upnpEnabled" 
			value="ON" onChange="checkChange(this)">&nbsp;&nbsp;<script>dw(app_upnp_cmcc_enable)</script></b><br>
		 </td>
		</tr>
		
		<tr ><td height=10> </td> </tr>	
		<tr><td><b><script>dw(app_upnp_cmcc_portmapping_wan)</script></b></td></tr>		
		<% wanListForPortMapping(); %>
		
	</table>
	
	<div align="center">
	<input type="hidden" value="/app_upnp_cmcc.asp" name="submit-url">
	<center><button class="btnsaveup" value="Save & Apply" name="save_apply">应用</button> </center>	
	</div>
</form>
</blockquote>
</body>
</html>