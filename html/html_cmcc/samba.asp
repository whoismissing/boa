<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<!--系统默认模板-->
<HTML>
<HEAD>
<TITLE>SAMBA设置</TITLE>
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
<SCRIPT language=Javascript src="<% getInfo("lang");%>"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript">
var cgi = new Object();
/********************************************************************
**          on document load
********************************************************************/
function init()
{
	var cf = document.samba;
	var EnableSamba = <%getIndex("sambaEnabled");%>;

    	if(EnableSamba == 1) 
        	cf.sambaEnabled.checked = true;
        else
        	cf.sambaEnabled.checked = false;
}

function checkChange(cb)
{
	if(cb.checked==true){
		cb.value = "ON";
	}
	else{
		cb.value = "";
	}
}

</script>

</SCRIPT>
</HEAD>

<!-------------------------------------------------------------------------------------->
<!--主页代码-->
<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onLoad="init();">
  <blockquote>
	<DIV align="left" style="padding-left:20px; padding-top:5px">
		<form action=/boafrm/formSamba method=POST name="samba">
		<br>
		<table border="0" width=600>
		<tr>
		<td colspan="2"><font size=2><b>
			<input type="checkbox" name="sambaEnabled" value="ON" onChange="checkChange(this)">&nbsp;&nbsp;<script>dw(app_samba_enable)</script></b><br>
		 </td>
		</tr>
		
		</table>
	
	<div align="center">
	<input type="hidden" value="/samba.asp" name="submit-url">
	<center><button class="btnsaveup" value="Save & Apply" name="save_apply">应用</button> </center>	
	</div>
</form>
	</DIV>
  </blockquote>
</body>

</html>
