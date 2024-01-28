<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<HTML>
<HEAD>
<TITLE>静态路由</TITLE>
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

function postGW( enable, destNet, subMask, nextHop, metric, interface, select )
{
	document.route.enable.checked = enable;
	document.route.destNet.value=destNet;
	document.route.subMask.value=subMask;
	document.route.nextHop.value=nextHop;
	document.route.metric.value=metric;
	document.route.interface.value=interface;	
	document.route.select_id.value=select;	
}

function postGWv6( enable, destNet, prefixLen, nextHop, interface, select )
{
	document.routev6.enable.checked = enable;
	document.routev6.destNet.value=destNet;
	document.routev6.prefixLen.value=prefixLen;
	document.routev6.nextHop.value=nextHop;
	document.routev6.interface.value=interface;	
	document.routev6.select_id.value=select;	
}
	
</SCRIPT>
</head>

<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000">
<blockquote>
<DIV align="left" style="padding-left:20px; padding-top:5px">

<form action=/boaform/formRouting method=POST name="route">
<b>IPv4 静态路由</b>
<table border=1 width="600" cellspacing=4 cellpadding=0>
  <tr><td align=center width="8%">目的地址</td>
<td align=center width="8%">网关</td>
<td align=center width="8%">子网掩码</td>
<td align=center width="8%">接口</td>
<td align=center width="5%">删除</td>
</tr>

</table>
  <br>
  <tr>
   <td>
		<input type="button" class="btnaddup" value="添加" name="gotoaddRoute" onclick="window.location.href='/routing_v4_cmcc.asp';">&nbsp;&nbsp;
		<input type="submit" class="BtnDel" value="删除" name="delRoute" onClick="return deleteClick()">
		<input type="hidden" value="/routing_cmcc.asp" name="submit-url">
		&nbsp;&nbsp;
   </td>
  </tr>
  <tr>
  <br><br><br>
  <hr class="sep" size=1 noshade align=top></tr> 
</form>
<br><br><br><br><br><br><br><br><br><br><br><br>
<form action=/boaform/formIPv6Routing method=POST name="routev6">
<b>IPv6 静态路由</b>
<table border=1 width="600" cellspacing=4 cellpadding=0>
  <tr><td align=center>目的地址</td>
<td align=center>网关</td>
<td align=center>接口</td>
<td align=center>使能</td>
<td align=center>删除</td>
</tr>

</table>
  <br>
  <tr>
   <td>
		<input type="button" class="btnaddup" value="添加" name="gotoaddV6Route" onclick="window.location.href='/routing_v6_cmcc.asp';">&nbsp;&nbsp;
		<input type="submit" class="BtnDel" value="删除" name="delV6Route" onClick="return deleteClick()">
		<input type="hidden" value="/routing_cmcc.asp" name="submit-url">
		&nbsp;&nbsp;
   </td>
  </tr>
  <br>
</form>
</DIV>
</blockquote>
</body>

</html>
