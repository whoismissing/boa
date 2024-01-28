<html>
<! Copyright (c) Realtek Semiconductor Corp., 2003. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html">
<% getIndex("no-cache"); %>
<title>Cluster Management</title>
<% getInfo("include_css"); %>
<SCRIPT language=Javascript src="<% getInfo("lang");%>"></SCRIPT>
</head>
<script>
var has_cluster_mgmt = <% getIndex("has_cluster_mgmt"); %>;

function clu_mgmt_saveChanges()
{
	var cf = document.clustermgmt;
	if(has_cluster_mgmt==0)
	{
		alert(cluster_management_not_support);
		return false;
	}

	if(cf.clu_periodic_enable.checked)
	{
		if(!checkFieldDigitRange(cf.clu_periodic_interval, 1, 10000, cluster_management_interval_invalid))
		{
			return false;
		}
	}
	
	return true;
}

function init()
{
	//var cf = document.forms[0];
	var cf = document.clustermgmt;
	var EnableCLU = <%getIndex("enabled_cluster_mgmt");%>;
	var CLUPeriodicEnable = <%getIndex("cluster_periodic_enabled");%>;
	var CLUPeriodicInterval = <%getIndex("cluster_periodic_interval");%>;

	if(EnableCLU == 1) 
    	cf.enable_clu_mgmt.checked = true;
    else
    	cf.enable_clu_mgmt.checked = false;

	if(CLUPeriodicEnable == 1) 
    	cf.clu_periodic_enable.checked = true;
    else
    	cf.clu_periodic_enable.checked = false;

	cf.clu_periodic_interval.value = CLUPeriodicInterval;
}

function updateState_1()
{
	if (document.clustermgmt.enable_clu_mgmt.checked) {  	
		enableTextField(document.clustermgmt.clu_periodic_enable);	
		enableTextField(document.clustermgmt.clu_periodic_interval);	
  	}
  	else {
  		disableTextField(document.clustermgmt.clu_periodic_enable);	
		disableTextField(document.clustermgmt.clu_periodic_interval);		
  	}
		
}

function updateState_2()
{
	if (document.clustermgmt.clu_periodic_enable.checked) {  	
		enableTextField(document.clustermgmt.clu_periodic_interval);	
  	}
  	else {
		disableTextField(document.clustermgmt.clu_periodic_interval);		
  	}
		
}
</script>

<body onload="init();">
<blockquote>
<h2><script>dw(cluster_header)</script></h2>


<table border=0 width="500" cellspacing=0 cellpadding=0>
  <tr><font size=2>
   <script>dw(cluster_header_explain)</script>
  </tr>
  <tr><hr size=1 noshade align=top></tr>
</table>
<form action=/boafrm/formCluMgmt method=POST name="clustermgmt">
<table border="0" width=600>

	<tr><td colspan="2"><font size=2><b>
		<input type="checkbox" name="enable_clu_mgmt" value="ON" ONCLICK=updateState_1()>&nbsp;&nbsp;<script>dw(cluster_management)</script></b><br>
	    </td>
	</tr>

	<tr>
		<td colspan="2"><font size=2><b>
			<input type="checkbox" name="clu_periodic_enable" value="ON" ONCLICK=updateState_2()>&nbsp;&nbsp;<script>dw(cluster_periodic_enable)</script></b><br>
	    </td>
	    <td colspan="2"><font size=2><b>
			<input type="text" name="clu_periodic_interval" size="8" maxlength="8">&nbsp;&nbsp;<script>dw(cluster_periodic_interval)</script></b><br>
	    </td>
	</tr>
	
</table>
  <p>
  
  <input type="hidden" value="/cluster_mgmt.asp" name="submit-url">
&nbsp;&nbsp;
  <input type="submit" value="Save & Apply" name="save_apply" onClick="return clu_mgmt_saveChanges()">
&nbsp;&nbsp;
  <script>document.clustermgmt.save_apply.value = cluster_save_apply;</script>
  <input type="reset" value="Reset" name="set" onClick="return clu_mgmt_saveChanges()">
  <script>document.clustermgmt.set.value = cluster_reset;</script>
</form>
</blockquote>
</font>
</body>

</html>

