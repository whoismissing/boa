<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<HTML>
<HEAD>
<TITLE>IGMP Snooping 配置</TITLE>
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
/*
var cgi = new Object();

*/

/********************************************************************
**          on document load
********************************************************************/
function on_init()
{
	sji_docinit(document, cgi);
}
function LoadSetting()
{
	
	var igmp_snoop_enabled=<%getIndex("igmp_snoop_enabled");%>;
	var igmp_proxy_enabled=<%getIndex("igmp_proxy_enabled");%>;
	var mld_snoop_enabled=<%getIndex("mld_snoop_enabled");%>;
	var mld_proxy_enabled=<%getIndex("mld_proxy_enabled");%>;
	if(igmp_snoop_enabled!=2)
		if(igmp_snoop_enabled==1)	
		{
			document.getElementById("chkIgmpSnp").checked= true;
			
		}
		else
		{
			document.getElementById("chkIgmpSnp").checked= false;
		
		}
	if(igmp_proxy_enabled!=2)
		if(igmp_proxy_enabled==1)	
		{
			document.getElementById("chkIgmpProxy").checked= true;
		}
		else
		{
			document.getElementById("chkIgmpProxy").checked= false;
		}
	if(mld_snoop_enabled!=2)
		if(mld_snoop_enabled==1)	
		{
			document.getElementById("chkMldSnp").checked= true;
			
		}
		else
		{
			document.getElementById("chkMldSnp").checked= false;
		
		}
	if(mld_proxy_enabled!=2)
		if(mld_proxy_enabled==1)	
		{
			document.getElementById("chkMldProxy").checked= true;
		}
		else
		{
			document.getElementById("chkMldProxy").checked= false;
		}  
}
function on_apply(form)
{
	var opmode;
	opmode = <% getIndex("opMode"); %> ;
	var igmp_snoop_enabled=<%getIndex("igmp_snoop_enabled");%>;
	var igmp_proxy_enabled=<%getIndex("igmp_proxy_enabled");%>;
	var mld_snoop_enabled=<%getIndex("mld_snoop_enabled");%>;
	var mld_proxy_enabled=<%getIndex("mld_proxy_enabled");%>;
	if(igmp_snoop_enabled!=2)
		if(document.getElementById("chkIgmpSnp").checked==true)
			document.getElementById("snp").value=1;
			
		else
			document.getElementById("snp").value=0;
	if(mld_snoop_enabled!=2)
		if(document.getElementById("chkMldSnp").checked==true)
			document.getElementById("mldsnp").value=1;
		else
			document.getElementById("mldsnp").value=0;
	if(igmp_proxy_enabled!=2)
		if(document.getElementById("chkIgmpProxy").checked==true)
		{
			if(opmode == 1)
			{
				alert('bridge mode can not set igmp/mld proxy!');
				return false;
			}
			document.getElementById("proxy").value=1;
		}
		else
			document.getElementById("proxy").value=0;
	if(mld_proxy_enabled!=2)
		if(document.getElementById("chkMldProxy").checked==true)
		{
			if(opmode == 1)
			{
				alert('bridge mode can not set igmp/mld proxy!');
				return false;
			}
			document.getElementById("mldproxy").value=1;
		}
		else
			document.getElementById("mldproxy").value=0;
		document.getElementById("formSnooping").submit();
	
	return true;;
}
</SCRIPT>
</HEAD>
<!-------------------------------------------------------------------------------------->
<!--主页代码-->
<body  topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000">
	<blockquote>
		<DIV align="left" style="padding-left:20px; padding-top:5px">
			<form id="formSnooping" action=/boafrm/formIgmpMldSnoopingProxy method=POST name="igmpMldSnoop">
			<table border=0 width="500" cellspacing=4 cellpadding=0>
			<script type="text/javascript">
				if(<%getIndex("igmp_snoop_enabled");%> !=2)
				{
					document.write("<tr><td width=150>启用IGMP Snooping: </td><td> <input type='checkbox' name=chkIgmpSnp id='chkIgmpSnp'></td></tr>");
				}
				if(<%getIndex("igmp_snoop_enabled");%> !=2)
				{
					document.write("<tr><td width=150>启用MLD Snooping:</td><td><input type='checkbox' name=chkMldSnp id='chkMldSnp'></td></tr>");
				}
				</script>
					
				<input type="hidden" id="snp" name="snp">
				<input type="hidden" id="mldsnp" name="mldsnp">
				<input type="hidden" name="submit-url" value="/snooping_proxy_cmcc.asp">
				<script type="text/javascript">
				if(<%getIndex("igmp_snoop_enabled");%> !=2)
				{
					document.write("<tr><td width=150>启用IGMP Proxy:</td><td><input type='checkbox' name=chkIgmpProxy id='chkIgmpProxy'></td></tr>");
				}
				if(<%getIndex("igmp_snoop_enabled");%> !=2)
				{
					document.write("<tr><td width=150>启用MLD Proxy:</td><td><input type='checkbox' name=chkMldProxy id='chkMldProxy'></td></tr>");
				}
				</script>
					</table>
				<br>
				<center><button class="btnsaveup" name="apply_proxy" onclick="return on_apply(1)">应用</button> </center>		
				<input type="hidden" id="proxy" name="proxy">
				<input type="hidden" id="mldproxy" name="mldproxy">
				<input type="hidden" name="submit-url" value="/snooping_proxy_cmcc.asp">
			<script>
				LoadSetting();
	
			</script>			
			</form>
		</DIV>

	</blockquote>
</body>

</html>
