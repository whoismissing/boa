<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<html>
<head>
<title>恢复出厂设置</title>
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
function on_click_button(reset)
{
	document.forms[0].reset.value = reset;
	return true;
}
</script>
</head>

<!-------------------------------------------------------------------------------------->
<!--主页代码-->
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000">
	<blockquote>
		<div align="left" style="padding-left:20px; padding-top:5px">
			<form id="form" action="/boaform/admin/formReboot" method="post">
				<p>&nbsp;</p>
				<p align="center">
					<button class="btnsaveup" type="submit"  onClick="return on_click_button(0)">恢复默认配置</button>
				</p>
				<p>&nbsp;</p>
				<hr>
				<p>&nbsp;</p>
				<p align="center">
					<button class="btnsaveup" type="submit"  onClick="return on_click_button(1)">恢复出厂配置</button>
				</p>
				<p>&nbsp;</p>
				<input type="hidden" value="/mgm_dev_reset_cmcc.asp" name="submit-url">
				<input type="hidden" value="0" name="reset">
			</form>
		</div>
	</blockquote>
</body>

</html>
