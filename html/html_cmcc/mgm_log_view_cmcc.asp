<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<html>
<head>
<title>系统访问日志</title>
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
<script type="text/javascript" src="util_gw.js"> </script>
<% getInfo("include_css"); %>
<SCRIPT language=Javascript src="<% getInfo("lang");%>"></SCRIPT>
<script language="javascript" type="text/javascript">

var dis_level = <% getInfo("syslog_display_level");%>;
syslog = 1;

/*
cgi.mf = "ASB";
cgi.pc = "V1.0";
cgi.sn = "03100200000100100000007404010203";
cgi.ip = "192.168.2.1";
cgi.hv = "V1.0";
cgi.sv = "RG100A_V1.0";
*/
/*
rcs.push(new Array("1900-01-07 01:26:37", "Informational", "kernel: Freeing unused kernel memory: 144k freed"));
rcs.push(new Array("1900-01-07 01:26:37", "Warning", "kernel: EHCI: port status=0x00001000"));
*/

/********************************************************************
**          on document load
********************************************************************/
function getElementById(sId)
{
	if (document.getElementById)
	{
		return document.getElementById(sId);
	}
	else if (document.all)
	{
		// old IE
		return document.all(sId);
	}
	else if (document.layers)
	{
		// Netscape 4
		return document.layers[sId];
	}
	else
	{
		return null;
	}
}


function getElementByName(sId)
{    // standard
	if (document.getElementsByName)
	{
		var element = document.getElementsByName(sId);

		if (element.length == 0)
		{
			return null;
		}
		else if (element.length == 1)
		{
			return  element[0];
		}
		return element;
	}
}


function getElement(sId)
{
	 var ele = getElementByName(sId);
	 if (ele == null)
	 {
			return getElementById(sId);
	 }
	 return ele;
}


function getElById(sId)
{
	return getElement(sId);
}

function setSelectContent(sId, sValue)
{
		var item;
		if (null == (item = getElById(sId)))
		{
				alert(sId + " is not existed" );
				return false;
		}

		for (var i = 0; i < item.options.length; i++)
		{
		if (item.options[i].value == sValue)
				{

				item.selectedIndex = i;
				return true;
		}
	}
	alert("the option which value is " + sValue + " is not existed in " + sId);
	return false;
}

function on_init()
{
	with (document.forms[0]) {
	/*
		logDisplay.innerHTML = "";
		if(!syslog){
			dispLevel.disabled = true;
			return;
		}
		for (var i = 0; i < rcs.length; i++) 
		logDisplay.innerHTML += rcs[i][0] + "&nbsp;" + rcs[i][2] + "\n";
	*/
		setSelectContent('dispLevel',dis_level);
	}
}
function on_submit(act)
{
	with (document.forms[0]) {
		action.value = act;
		submit();
	}
}
</script>
</head>

<!-------------------------------------------------------------------------------------->
<!--主页代码-->
<h2><script>dw(syslog_display_header)</script></h2>
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onLoad="on_init();">
<script>dw(syslog_display_header_explain)</script>
	<div align="left" style="padding-left:20px; padding-top:10px">
			<hr align="left" class="sep" size="1" width="90%">

			<form id="form" action="/boafrm/formSetSyslogDisplayLevel" method="post">
			<table cellspacing="0" cellpadding="2" border="0">
			<tbody>
				<tr>
					<td width="130"><script>dw(syslog_display_level)</script></td>
					<td>
				  		<select name="dispLevel" size="1" style="width:120px " onchange="on_submit('dispLevel')">
						<option selected value="0"><script>dw(syslog_emerg)</script></option>
						<option value="1"><script>dw(syslog_alert)</script></option>
						<option value="2"><script>dw(syslog_crit)</script></option>
						<option value="3"><script>dw(syslog_err)</script></option>
						<option value="4"><script>dw(syslog_warn)</script></option>
						<option value="5"><script>dw(syslog_notice)</script></option>
						<option value="6"><script>dw(syslog_info)</script></option>
						<option value="7"><script>dw(syslog_debug)</script></option>
						</select>
					</td>
				</tr>
			<tr><td colspan="2">&nbsp;</td>
			</tr>
			</table>
			<input type="hidden" name="submit-url" value="/mgm_log_view_cmcc.asp">			
			</form>
			<table cellspacing="0" cellpadding="2" border="0">
			<tbody>
			<tr>
				<td colspan="2"><textarea id="logDisplay" name="logDisplay" style="WIDTH:453px;HEIGHT:521px;font:inherit;" wrap="hard" edit="OFF" readonly="readonly" wrap="OFF"><% FilteredSysLogList(); %></textarea>
				</td>
		  	</tr>
			</table>
			<form id="form" action="/boafrm/formSaveClrSysLog" method="post">
				<button class="btnsaveup" name="saverec" value="1" onClick="on_submit('saveLog');"><script>dw(syslog_savelog)</script></button>
				<button class="BtnCnl" name="clrrec" value="1" onClick="on_submit('clr');"><script>dw(syslog_clrlog)</script></button>
				<input type="hidden" id="action" name="action" value="none">
				<input type="hidden" name="submit-url" value="/mgm_log_view_cmcc.asp">	
			</form>
	</div>
</body>

</html>
