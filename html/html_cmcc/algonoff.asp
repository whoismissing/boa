<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<HTML>
<HEAD>
<TITLE>ALG On-Off</TITLE>
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

function checkChange(cb)
{
	if(cb.checked==true){
		cb.value = 1;
	}
	else{
		cb.value = 0;
	}
}

function apply_click()
{
	document.getElementById("formALGOnOff").submit();
	 return true;
}
function LoadSetting()
{
	var l2tp_algonoff=<%getIndex("l2tp_algonoff");%>;
	var ipsec_algonoff=<%getIndex("ipsec_algonoff");%>;
	var h323_algonoff=<%getIndex("h323_algonoff");%>;
	var rtsp_algonoff=<%getIndex("rtsp_algonoff");%>;
	var sip_algonoff=<%getIndex("sip_algonoff");%>;
	var ftp_algonoff=<%getIndex("ftp_algonoff");%>;
	var pptp_algonoff=<%getIndex("pptp_algonoff");%>;
	var tftp_algonoff=<%getIndex("tftp_algonoff");%>;
	if(l2tp_algonoff!=2)
	{
		document.write("<tr>");
	
   		document.write("<td ><input type=\"checkbox\" id=\"l2tp_algonoff\" name=l2tp_algonoff  onChange=\"checkChange(this)\">启用L2TP ALG</td><td ></td>");
		if(l2tp_algonoff==1)	
		{
			document.getElementById("l2tp_algonoff").checked= true;
		}
		else{
			document.getElementById("l2tp_algonoff").checked= false;
		}
		document.write("</tr>");
	}
	if(ipsec_algonoff!=2)
	{
		document.write("<tr>");
		document.write("<td ><input type=\"checkbox\"  id=\"ipsec_algonoff\" name=ipsec_algonoff  onChange=\"checkChange(this)\">启用IPSec ALG</td><td ></td>");
		if(ipsec_algonoff==1)	
		{
			document.getElementById("ipsec_algonoff").checked= true;
		}
		else
		{
			document.getElementById("ipsec_algonoff").checked= false;
		}
		document.write("</tr>");
	}
	if(h323_algonoff!=2)
	{
		document.write("<tr>");
		document.write("<td ><input type=\"checkbox\"  id=\"h323_algonoff\" name=h323_algonoff  onChange=\"checkChange(this)\">启用H323 ALG</td><td ></td>");
		if(h323_algonoff==1)	
		{
			document.getElementById("h323_algonoff").checked= true;
		}
		else
		{
			document.getElementById("h323_algonoff").checked= false;
		
		}
		document.write("</tr>");
	}
	if(rtsp_algonoff!=2)
	{
		document.write("<tr>");
		document.write("<td ><input type=\"checkbox\"  id=\"rtsp_algonoff\" name=rtsp_algonoff  onChange=\"checkChange(this)\" >启用RTSP ALG</td><td ></td>");
		if(rtsp_algonoff==1)	
		{
			document.getElementById("rtsp_algonoff").checked= true;	
		}
		else
		{
			document.getElementById("rtsp_algonoff").checked= false;
		}
		document.write("</tr>");
	}
	if(sip_algonoff!=2)
	{
		document.write("<tr>");
		document.write("<td ><input type=\"checkbox\"  id=\"sip_algonoff\" name=sip_algonoff  onChange=\"checkChange(this)\" >启用SIP ALG</td><td ></td>");
		if(sip_algonoff==1)	
		{
			document.getElementById("sip_algonoff").checked= true;	
			
		}
		else
		{
			document.getElementById("sip_algonoff").checked= false;	
		
		}
		document.write("</tr>");
	}
	if(ftp_algonoff!=2)
	{
		document.write("<tr>");
		document.write("<td ><input type=\"checkbox\"  id=\"ftp_algonoff\" name=ftp_algonoff  onChange=\"checkChange(this)\"> 启用FTP ALG</td><td ></td>");
		if(ftp_algonoff==1)	
		{
			document.getElementById("ftp_algonoff").checked= true;	
		}
		else
		{
			document.getElementById("ftp_algonoff").checked= false;
		}
		document.write("</tr>");
	}
	if(pptp_algonoff!=2)
	{
		document.write("<tr>");
		document.write("<td ><input type=\"checkbox\"  id=\"pptp_algonoff\" name=pptp_algonoff  onChange=\"checkChange(this)\" >启用PPTP ALG</td><td ></td>");
		if(pptp_algonoff==1)	
		{
			document.getElementById("pptp_algonoff").checked= true;
		}
		else
		{
			document.getElementById("pptp_algonoff").checked= false;
		}
		document.write("</tr>");
	}
	if(tftp_algonoff!=2)
	{
		document.write("<tr>");
		document.write("<td ><input type=\"checkbox\"  id=\"tftp_algonoff\" name=tftp_algonoff  onChange=\"checkChange(this)\" >启用TFTP ALG</td><td ></td>");
		if(tftp_algonoff==1)	
		{
			document.getElementById("tftp_algonoff").checked= true;
		}
		else
		{
			document.getElementById("tftp_algonoff").checked= false;
		}
		document.write("</tr>");
	}

}

</SCRIPT>
</head>

<body  topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000">
<blockquote>
<DIV align="left" style="padding-left:20px; padding-top:5px">

<table  width=500>
<tr><td colspan=4></td></tr>
<form id="formALGOnOff" action=/boafrm/formALGOnOff method=POST name="algof">
<table>
<script>

LoadSetting();



</script>
	<td></td>
	<td></td>
<tr>
  <td> <input type="hidden" value="/algonoff.asp" name="submit-url"></td>
  <td ><input  class="btnsaveup" type=submit value="保存/应用" name="apply" onClick="return apply_click()"></td>
  <td></td>
</tr>
</table>
</form>
</table>
</DIV>
</blockquote>
</body>
</html>
