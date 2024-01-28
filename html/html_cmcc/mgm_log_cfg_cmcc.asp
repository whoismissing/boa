<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<html>
<head>
<title>写入等级设置</title>
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
<script type="text/javascript" src="share_cmcc.js"></script>
<script language="javascript" type="text/javascript">

/********************************************************************
**          on document update
********************************************************************/
function on_updatectrl() 
{
	with (document.forms[0]) {
		if (syslogEnable[0].checked) {
			recordLevel.disabled = true;
		} else {
			recordLevel.disabled = false;
		}
	}
}

/********************************************************************
**          on document submit
*******************************************************************/
function on_submit()
{
	with (document.forms[0]) {
		submit();
	}
}
</script>
</head>

<!-------------------------------------------------------------------------------------->
<!--主页代码-->
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000">
	<blockquote>
		<div align="left" style="padding-left:20px; padding-top:5px">
			<form id="form" action="/boaform/admin/formSysLogConfig" method="post">
				<hr align="left" class="sep" size="1" width="90%">
				<table border="0" cellpadding="0" cellspacing="0">
				   <tr>
					  <td width="80">日志:</td>
					  <td><input name="syslogEnable" type="radio" value="0" onClick="on_updatectrl();" >禁用</td>
					  <td><input name="syslogEnable" type="radio" value="1" onClick="on_updatectrl();"checked>启用</td>
				   </tr>
				   <tr><td colspan="2">&nbsp;</td>
				   </tr>
				   <tr>
					  <td>日志等级:</td>
					  <td colspan="2">
				  		<select name="recordLevel" size="1" style="width:120px ">
						<option value="0">紧急</option><option value="1">警报</option><option value="2">重要</option><option selected value="3">错误</option><option value="4">警告</option><option value="5">注意</option><option value="6">通知</option><option value="7">调试</option>
						</select>
					 </td>
				   </tr>
				</table>
				</div>
				<hr align="left" class="sep" size="1" width="90%">
				<button class="btnsaveup" onClick="on_submit()">确定</button>
				<button class="BtnCnl" onClick="location.reload(true)">取消</button>
				<input type="hidden" name="submit-url" value="/mgm_log_cfg_cmcc.asp">
<script>
	on_updatectrl();
</script>			</form>
		</div>
	</blockquote>
</body>

</html>
