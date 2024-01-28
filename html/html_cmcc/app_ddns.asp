<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<HTML>
<HEAD>
<TITLE>动态DNS</TITLE>
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
<script language="javascript" src="jquery-3.2.1.min.js"></script>
<SCRIPT language="javascript" type="text/javascript">

/********************************************************************
**          on document load
********************************************************************/

function btnApply()
{
	if(!sji_checkhostname(form.hostname.value, 1, 32))
	{
		alert("请输入合法的主机名！");
		return false;
	}

	if(!sji_checkusername(form.orayusername.value, 0, 32))
	{
		alert("请输入合法的用户名！");
		return false;
	}
	if(!sji_checkpswnor(form.oraypassword.value, 0, 32))
	{
		alert("请输入合法的密码！");
		return false;
	}

	form.submit();
}

function checkChange(cb)
{
	if(cb.checked==true){
		cb.value = 1;
	}
	else{
		cb.value = 0;
	}
}

function refreshStatus(){
	$.ajax({
		type: 'POST',
		url: '/app_ddns.asp',
		success: function(data) {
			$("#div_ddns_status").html($(data).find("#div_ddns_status").html()); 
		}
	});
	
}

</SCRIPT>
</HEAD>

<!-------------------------------------------------------------------------------------->
<!--主页代码-->
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000">
	<blockquote>
		<DIV align="left" style="padding-left:20px; padding-top:5px">
			<form id="form" action=/boaform/admin/formDDNS method=POST>
				<b></b><br><br><br>
				<table border="0" cellpadding="0" cellspacing="0">
				<tr>
						<td width="180">服务商:</td>
						<td>花生壳</td>
					</tr>
				<tr>
					<td width="180">域名:</td>
					<td><input type="text" name="hostname" style="width:200px " value=hphserver.oray.net disabled></td>
				</tr>
				<tr>
					<td width="180">用户名:</td>
					<td><input type="text" name="orayusername" size="20" maxlen="64" style="width:200px " value=></td>
				</tr>
				<tr>
					<td>密码:</td>
					<td><input type="password" name="oraypassword" style="width:200px " value=></td>
				</tr>
				<tr>
					<td>使能：</td>
					<td><input type="checkbox" name="ddnsEnable" onChange="checkChange(this)" value='1' checked></td>
				</tr>
				</table>
				<br>
				<br>
				<input type="button" class="btnsaveup" value="保存" onClick="btnApply()">
				<input type="hidden" id="action" name="action" value="ad">
				<input type="hidden" value="/app_ddns.asp" name="submit-url">
				<br>
				<br>
				<br>
				<br>
				<DIV id="div_ddns_status" align="left" style="padding-left:20px; padding-top:5px">
				<table id="ddns_status_tbl" width="100%" cellspacing="0" cellpadding="0" border="1">
					<tr id="ddnsStatus_tr">
						<td width="200">注册状态</td>
						<td >去使能</td>
					</tr>
					<tr id="ddnsDomain_tr">
						<td width="200">注册域名</td>
						<td></td>
					</tr>
				</table>
				</div>
			</form>
		</div>
	</blockquote>
<script>
	setInterval("refreshStatus()",3000);
</script>
</body>

</html>
