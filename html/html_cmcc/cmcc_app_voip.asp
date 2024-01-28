<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<HTML>
<HEAD>
<TITLE>日常应用</TITLE>
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

/********************************************************************
**          on document load
********************************************************************/
function on_init()
{
}

/********************************************************************
**          on document submit
********************************************************************/
function on_submit() 
{
	form.submit();
}
function on_codec_submit() 
{
	form_codec.submit();
}
function on_account_submit() 
{
	form_account.submit();
}
function on_account_del()
{
	form_account_table.submit();
}
function on_account_add()
{
	if (form_account_table.account_number.value == "2")
	{
		alert("超过最大用户数");
	}else	
		document.all.form_account_add.style.display="inline";
}
</SCRIPT>
</HEAD>

<!-------------------------------------------------------------------------------------->
<!--主页代码-->

<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" >
	<blockquote>
		<DIV align="left" style="padding-left:20px; padding-top:5px">
			<form id="form" action=/boaform/admin/voip_cmcc_set method=POST>
			<input type=hidden name=max_voip_ports value="1">
	
			<table id="sip_config" name="sip_config"  border="0" cellpadding="10" cellspacing="10" width="100%">
				<tr>
					<td><b>接口基本参数</b></td>
				</tr>	
				<tr>
					<td width=200>Outbound服务器地址:</td>
					<td width=200><input name="proxy0_obAddr" size="16" maxlength="32" type="text" style="width:140px" value="">(IP或域名)</td>
				</tr>
				<tr>
					<td>Outbound服务器端口号:</td>
					<td><input name="proxy0_obPort" size="16" maxlength="32" style="width:140px" value="5060">(1-65535)</td>
				</tr>
				<tr>
					<td>备用Outbound服务器地址:</td>
					<td><input name="proxy1_obAddr" size="16" maxlength="32" type="text" style="width:140px" value="">(IP或域名)</td>
				</tr>
				<tr>
					<td>备用Outbound服务器端口号:</td>
					<td><input name="proxy1_obPort" size="16" maxlength="32" style="width:140px" value="5060">(1-65535)</td>

				</tr>				
				<tr>
					<td>主用服务器地址:</td>
					<td><input name="proxy0_addr" size="16" maxlength="39" type="text" style="width:140px" value="">(IP或域名)</td>
				</tr>
				<tr>
					<td>主用端口号:</td>
					<td><input name="proxy0_port" size="16" maxlength="32" style="width:140px" value="5060">(1-65535)</td>
				</tr>
				<tr>
					<td>备用服务器地址:</td>
					<td><input name="proxy1_addr" size="16" maxlength="32" type="text" style="width:140px" value="">(IP或域名)</td>
				</tr>
				<tr>
					<td>备用端口号:</td>
					<td><input name="proxy1_port" size="16" maxlength="32" style="width:140px" value="5060">(1-65535)</td>
				</tr>
				<tr>
					<td>Register服务器地址:</td>
					<td><input name="reg_proxy0_addr" size="16" maxlength="39" type="text" style="width:140px" value="">(IP或域名)</td>
				</tr>
				<tr>
					<td>Register服务器端口号:</td>
					<td><input name="reg_proxy0_port" size="16" maxlength="32" style="width:140px" value="5060">(1-65535)</td>
				</tr>
				<tr>
					<td>备用Register服务器地址:</td>
					<td><input name="reg_proxy1_addr" size="16" maxlength="32" type="text" style="width:140px" value="">(IP或域名)</td>
				</tr>
				<tr>
					<td>备用Register服务器端口号:</td>
					<td><input name="reg_proxy1_port" size="16" maxlength="32" style="width:140px" value="5060">(1-65535)</td>
				</tr>				
				<tr>
					<td>域名属性:</td>
					<td><input name="proxy0_domain_name" size="16" maxlength="39" type="text" style="width:140px" value="">(IP或域名)</td>
				</tr>
				<tr>
					<td>备用域名属性:</td>
					<td><input name="proxy1_domain_name" size="16" maxlength="32" type="text" style="width:140px" value="">(IP或域名)</td>
				</tr>
				<tr>
			    	<td >使能数图</td>
			    	<td><input type="checkbox"  name="digitmap_enable" size=20 ></td>
				</tr>
				<tr>
					<td>数图</td>
					<td><TEXTAREA COLS=50 ROWS=5 name="dialplan" maxlength=4096></TEXTAREA></td>
				</tr>
				<tr>
					<td>会话更新周期:</td>
					<td><input name="proxy_cmcc_sessionExpiry" size="16" maxlength="32" style="width:140px" value="30">(单位: 分钟)</td>
				</tr>
				<tr>
					<td>注册重试间隔时间:</td>
					<td><input name="RegisterRetryInterval" size="16" maxlength="32" type="text" style="width:140px" value="90">(单位: 秒)(1-65534)</td>
				</tr>
				<tr>
					<td>注册周期:</td><td><input name="proxy0_reg_expire" size="16" maxlength="32" style="width:140px" value="3600">(单位: 秒)(1-65534)</td>
				</tr>
			</table>
			<table   border="0" cellpadding="0" cellspacing="0" width="700">
				<tr>
					<td align="center">
						<button class="btnsaveup" name="btnDown" onclick="on_submit()">保存/应用</button>
						<input type="reset" class="BtnCnl" value="取消">
						<input type="hidden" name="submit-url" value="">
					</td>
				</tr>
			</table>	
			</form>
			<form id="form_codec" action=/boaform/admin/voip_cmcc_codec_set method=POST>
			<br><b>VoIP语音编码配置</b><br>
			<table id="codec_config" name="codec_config"  border="3" cellpadding="0" cellspacing="0" width="700">
				<tr>
					<td>编解码方式</td>
					<td>使能</td>
					<td>打包时长(ms)</td>
					<td>优先级(1-4)</td>
				</tr>
				<tr><td width=200px>G722</td>
<td><input type="checkbox"  name="codec_0_enable" size=20 checked>
<td><select name=frame_size0>
<option  value=0>10 ms</option><option selected value=1>20 ms</option><option  value=2>30 ms</option></select></td>
<td><select name=preced0>
<option value=1>1<option value=2>2<option value=3>3<option value=4>4<option value=5 selected>5</select></td></tr>
<tr><td width=200px>G711-alaw</td>
<td><input type="checkbox"  name="codec_1_enable" size=20 checked>
<td><select name=frame_size1>
<option  value=0>10 ms</option><option selected value=1>20 ms</option><option  value=2>30 ms</option></select></td>
<td><select name=preced1>
<option value=1>1<option value=2 selected>2<option value=3>3<option value=4>4<option value=5>5</select></td></tr>
<tr><td width=200px>G711-ulaw</td>
<td><input type="checkbox"  name="codec_2_enable" size=20 checked>
<td><select name=frame_size2>
<option  value=0>10 ms</option><option selected value=1>20 ms</option><option  value=2>30 ms</option></select></td>
<td><select name=preced2>
<option value=1 selected>1<option value=2>2<option value=3>3<option value=4>4<option value=5>5</select></td></tr>
<tr><td width=200px>G729</td>
<td><input type="checkbox"  name="codec_3_enable" size=20 checked>
<td><select name=frame_size3>
<option  value=0>10 ms</option><option selected value=1>20 ms</option><option  value=2>30 ms</option></select></td>
<td><select name=preced3>
<option value=1>1<option value=2>2<option value=3 selected>3<option value=4>4<option value=5>5</select></td></tr>
<tr><td width=200px>G723</td>
<td><input type="checkbox"  name="codec_4_enable" size=20 checked>
<td><select name=frame_size4>
<option selected value=0>10 ms</option><option  value=1>20 ms</option><option  value=2>30 ms</option></select></td>
<td><select name=preced4>
<option value=1>1<option value=2>2<option value=3>3<option value=4 selected>4<option value=5>5</select></td></tr>

			</table>
			<table   border="0" cellpadding="0" cellspacing="0" width="700">
				<tr>
					<td align="center">
						<button class="btnsaveup" name="btnDown" onclick="on_codec_submit()">保存/应用</button>
						<input type="reset" class="BtnCnl" value="取消">
						<input type="hidden" name="submit-url" value="">
					</td>
				</tr>
			</table>	
			</form>			
			<form id="form_account_table" action=/boaform/admin/voip_cmcc_account_del method=POST>
			<br>
			<b>用戶基本參數</b><br>
			<table   border="0" cellpadding="0" cellspacing="0" width="700">
				<tr>
					<td align="center">
						<input id="btn_account_add" type="button" class="btnaddup" name="btnDown" value="新建" onClick="on_account_add()">
						<input id="btn_account_del" type="submit" class="BtnDel" name="btnDown" value="刪除" onClick="on_account_del()">
						<input type="hidden" name="submit-url" value="">
					</td>
				</tr>
			</table>
			<table   border="1" cellpadding="0" cellspacing="0" width="700">
				<tr>
<td align="center" style="WIDTH: 8%">  </td>
<td align="center" style="WIDTH: 8%"> 编号 </td>
<td align="center" style="WIDTH: 14%"> 注册用户名 </td>
<td align="center" style="WIDTH: 30%"> 鉴权用户名 </td>
<td align="center" style="WIDTH: 10%"> 密码 </td>
</tr>
<input type="hidden" name="account_number" value=0>

			</table>
			</form>
			<div id=form_account_add style="display:none">
				<form id="form_account" action=/boaform/admin/voip_cmcc_account_set method=POST>
				<br>
				<b>新建用户</b><br>
					<table   border="0" cellpadding="0" cellspacing="0" width="700">
						<tr>
<td width=200px>用户号码:</td><td><input name=account_number size=16 maxlength=32 style=width:140px value=></td>
</tr>
<tr>
<td>用户账号:</td><td><input name=account_login_id size=16 maxlength=80 style=width:140px value=></td>
</tr>
<tr>
<td>用户密码:</td><td><input name=account_password size=16 maxlength=32 type=password style=width:140px value= ></td>
</tr>

						<tr>
							<td align="center">
								<button class="btnsaveup" name="btnDown" onclick="on_account_submit()">保存/应用</button>
								<input type="reset" class="BtnCnl" value="取消">
								<input type="hidden" name="submit-url" value="">
							</td>
						</tr>
					</table>
				</form>
			</div>
		</DIV>
	</blockquote>
</body>

</html>
