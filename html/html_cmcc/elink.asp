<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html">
<% getIndex("no-cache"); %>
<% getInfo("include_css"); %>
<title>elink Configuration</title>
<script type="text/javascript" src="share.js">
</script>
<% language=javascript %>
<SCRIPT language=Javascript src="<% getInfo("lang");%>"></SCRIPT>
<SCRIPT>
var elink_enable_mib=<% getIndex("elink_enable"); %>
var elink_sync_mib=<% getIndex("elink_sync"); %>
var elink_sdk_support=<% getIndex("elink_cloud_sdk"); %>
function LoadSetting()
{
	if(elink_enable_mib)
	{
		document.elink.elink_enable[0].checked=true;
		document.elink.elink_enable[1].checked=false;
	}
	else
	{
		document.elink.elink_enable[0].checked=false;
		document.elink.elink_enable[1].checked=true;
		document.getElementById("sync_configure").style.display="none";
	}
	if(elink_sync_mib)
	{
		document.elink.elink_sync[0].checked=true;
		document.elink.elink_sync[1].checked=false;
	}
	else
	{
		document.elink.elink_sync[0].checked=false;
		document.elink.elink_sync[1].checked=true;
	}

	if(elink_sdk_support)
		document.getElementById("elinksdk").style.display="";
	else
		document.getElementById("elinksdk").style.display="none";

}
function elink_sync_func(value)
{
	if(value ==0) //disable sync config
	{
		document.elink.elink_sync[0].checked=false;
	}
	else //enable sync config
	{
		document.elink.elink_sync[1].checked=false;
	}

}
function elink_configure(value)
{
	if(value == 0) //disable elink
	{
		document.elink.elink_enable[0].checked=false;
		document.getElementById("sync_configure").style.display="none";
	}
	else	//enable elink
	{
		document.elink.elink_enable[0].checked=true;
		document.getElementById("sync_configure").style.display="";
	}

}
function saveChanges()
{
	return true;
}
function getFileType(str)
{
	if(str.value == "")
		return;
	var name = str.split(".");
	var type = name[name.length-1];
	return type;
}
function sendClicked(F)
{
  if(document.upload.binary.value == ""){
      	document.upload.binary.focus();
  	alert(upload_send_file_empty);
  	return false;
  }
  var fileType = getFileType(document.upload.binary.value);
  if(fileType != "tar"){      	
  	document.upload.binary.focus();
  	alert(upload_invalid_format);
  	return false;
  }
  F.submit();
}
</SCRIPT>
</head>

<body onload="LoadSetting()">
<blockquote>
<h2><script>dw(elink_header)</script></h2>

<form action=/boafrm/formElink method=POST name="elink">
      <input type="hidden" value="/elink.asp" name="submit-url">
<table border=0 width="500" cellspacing=4 cellpadding=0>
  <tr><font size=2>
    <script>dw(elink_explain)</script>
  </tr>
<tr><hr size=1 noshade align=top></tr>
  <tr>
      <td width="30%"><font size=2><b>elink:</b></td>
      <td width="70%"><font size=2>
      <input type="radio" name="elink_enable" value="1" onClick="elink_configure(1)"><script>dw(elink_enable)</script>&nbsp;&nbsp;
      <input type="radio" name="elink_enable" value="0" onClick="elink_configure(0)"><script>dw(elink_disable)</script>
      </td>
  </tr>
  <tr id="sync_configure">
		<td width="30%"><font size=2><b><script>dw(elink_sync_config)</script></b></td>
		<td width="70%"><font size=2>
		<input type="radio" name="elink_sync" id="sync_yes"value="1" onClick="elink_sync_func(1)"><script>dw(elink_sync_config_yes)</script>&nbsp;&nbsp;
		<input type="radio" name="elink_sync" id="sync_no" value="0" onClick="elink_sync_func(0)"><script>dw(elink_sync_config_no)</script>
		</td>
	</tr>

</table>
<% getInfo("apply_prompt_comment_start");%>
      <input type="submit" value="Apply Changes" name="save" onClick="return saveChanges()">&nbsp;&nbsp;
      <input type="reset" value="Undo" name="reset" onClick="resetClick()">
<% getInfo("apply_prompt_comment_end");%>
<% getInfo("apply_direct_comment_start");%>
     <!-- 
     <input type="submit" value="Save" name="save" onClick="return saveChanges()">&nbsp;&nbsp;
      <script>document.elink.save.value = elink_save;</script>
      -->
      <input type="submit" value="Save & Apply" name="save_apply" onClick="return saveChanges()">&nbsp;&nbsp;
      <script>document.elink.save_apply.value = elink_save_apply;</script>
      <input type="reset" value="Undo" name="reset" onClick="resetClick()">
      <script>document.elink.reset.value = elink_reset;</script>
<% getInfo("apply_direct_comment_end");%>
<tr><hr size=1 noshade align=top></tr>
</form>

<form id="elinksdk" method="post" action="boafrm/formElinkSDKUpload" enctype="multipart/form-data" name="upload">
<h2><script>dw(elinksdk_header)</script></h2>
<tr><hr size=1 noshade align=top></tr>
<table border="0" cellspacing="4" width="500">
	<tr>
    	<td width="50%"><font size=2><b><script>dw(elinksdk_version)</script></b>&nbsp;&nbsp;&nbsp;&nbsp;</td>
        <td width="50%"><font size=2><% getInfo("elinksdkVersion"); %></td>
	</tr>
  	<tr>
      	<td width="20%"><font size=2><b><script>dw(upload_file_select)</script></b>&nbsp;&nbsp;&nbsp;&nbsp;</td>
      	<td width="80%"><font size=2><input type="file" name="binary" size=20></td>
  	</tr>
</table>
<p> <input onclick=sendClicked(this.form) type=button value="Upload" name="send">&nbsp;&nbsp;  
    <script>document.upload.send.value = upload_send;</script>
<!--
	<input type="hidden" value="0x10000" name="writeAddrWebPages">
	<input type="hidden" value="0x20000" name="writeAddrCode">
-->
	<input type="hidden" value="/elink.asp" name="submit-url">

</p>
<tr><hr size=1 noshade align=top></tr>
</form>

</blockquote>
</body>

</html>

