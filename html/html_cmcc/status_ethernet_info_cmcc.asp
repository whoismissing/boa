<!-- add by liuxiao 2008-01-21 -->
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
<title>家庭网关</title>
<meta http-equiv=pragma content=no-cache>
<meta http-equiv=cache-control content="no-cache, must-revalidate">
<meta http-equiv=content-type content="text/html; charset=utf-8">
<meta http-equiv="refresh" content="5">
<meta http-equiv=content-script-type content=text/javascript>
<style type=text/css>
@import url(/style/default.css);
</style>
<script language="javascript" src="common.js"></script>
<SCRIPT language="javascript" type="text/javascript">

var ethers = new Array();
var clts = new Array();
ethers.push(new it_nr("0", new it("ifname", "Ethernet"), new it("rx_packets", 5869), new it("rx_bytes", 721379), new it("rx_errors", 0), new it("rx_dropped", 0), new it("tx_packets", 6410), new it("tx_bytes", 4621688), new it("tx_errors", 0), new it("tx_dropped", 0)));

clts.push(new it_nr("0", new it("devname", "00:0E:0C:A1:D4:53"), new it("macAddr", "00:0e:0c:a1:d4:53"), new it("ipAddr", "192.168.1.209"), new it("liveTime", ""), new it("conType", "有线"), new it("ipAssign", "静态分配")));



/********************************************************************
**          on document load
********************************************************************/
function on_init()
{
	if (lsteth.rows) {
		while (lsteth.rows.length > 2)
			lsteth.deleteRow(2);
	}

	for (var i = 0; i < ethers.length; i++) {
		var row = lsteth.insertRow(i + 2);

		row.nowrap = true;
		row.style.verticalAlign = "top";
		row.style.textAlign = "center";

		var cell = row.insertCell(0);
		cell.innerHTML = ethers[i].ifname;
		cell = row.insertCell(1);
		cell.innerHTML = ethers[i].rx_bytes;
		cell = row.insertCell(2);
		cell.innerHTML = ethers[i].rx_packets;
		cell = row.insertCell(3);
		cell.innerHTML = ethers[i].rx_errors;
		cell = row.insertCell(4);
		cell.innerHTML = ethers[i].rx_dropped;
		cell = row.insertCell(5);
		cell.innerHTML = ethers[i].tx_bytes;
		cell = row.insertCell(6);
		cell.innerHTML = ethers[i].tx_packets;
		cell = row.insertCell(7);
		cell.innerHTML = ethers[i].tx_errors;
		cell = row.insertCell(8);
		cell.innerHTML = ethers[i].tx_dropped;
	}

	if (lstdev.rows) {
		while (lstdev.rows.length > 1)
			lstdev.deleteRow(1);
	}

	for (var i = 0; i < clts.length; i++) {
		var row = lstdev.insertRow(i + 1);

		row.nowrap = true;
		row.style.verticalAlign = "top";
		row.style.textAlign = "center";

		var cell = row.insertCell(0);
		cell.innerHTML = clts[i].ipAddr;
		cell = row.insertCell(1);
		cell.innerHTML = clts[i].macAddr;
		cell = row.insertCell(2);
		cell.innerHTML = clts[i].ipAssign;
		cell = row.insertCell(3);
		cell.innerHTML = clts[i].devname;
		cell = row.insertCell(4);
		cell.innerHTML = clts[i].conType;
	}
}
</SCRIPT>
</head>

<body topmargin="0" leftmargin="0" marginwidth="0" marginheight="0" alink="#000000" link="#000000" vlink="#000000" onLoad="on_init();">
	<blockquote>
		<div align="left" style="padding-left:20px;"><br>
			<table class="flat" border="1" cellpadding="1" cellspacing="1" width="100%">
				<tr>
					<td class="hdb" width="20%">IP地址</td>
					<td>192.168.1.1</td>
				</tr>
				<tr>
					<td class="hdb" width="20%">IPv6地址</td>
					<td>fe80::1</td>
				</tr>
				<tr>
					<td class="hdb">MAC地址</td>
					<td>00:e0:4c:86:70:01</td>
				</tr>
			</table>
		</div>
		<br>
		<div align="left" style="padding-left:20px;">
			<table class="flat" border="1" cellpadding="1" cellspacing="1" width="100%"><tr class="hdb" align="center" nowrap> <td></td><td>LAN-1</td><td>LAN-2</td><td>LAN-3</td><td>LAN-4</td></tr><tr align="center"><td>连接状态</td><td>未连接</td><td>连接上</td><td>未连接</td><td>未连接</td></tr><tr align="center"><td>工作模式</td><td>半双工</td><td>全双工</td><td>半双工</td><td>半双工</td></tr><tr align="center"><td>速率</td><td>10M</td><td>1000M</td><td>10M</td><td>10M</td></tr></tr></table>
		</div>
		<br>
		<div align="left" style="padding-left:20px;">
			<table id="lstdev" class="flat" border="1" cellpadding="1" cellspacing="1" width="100%">
				<tr class="hdb" align="center" nowrap>
					<td>IP地址</td>
					<td>MAC地址</td>
					<td>状态</td>
					<td>设备名称</td>
					<td>设备类型</td>
				</tr>
			</table>
		</div>
		<br>
		<div align="left" style="padding-left:20px;">
			<table id="lsteth" class="flat" border="1" cellpadding="1" cellspacing="1" width="100%">
				<tr class="hdb" align="center" nowrap>
					<td>接口</td>
					<td colspan="4">接收</td>
					<td colspan="4">发送</td>
				</tr>
				<tr class="hdb" align="center" nowrap>
					<td>&nbsp;</td>
					<td>字节</td>
					<td>包</td>
					<td>错误</td>
					<td>丢弃</td>
					<td>字节</td>
					<td>包</td>
					<td>错误</td>
					<td>丢弃</td>
				</tr>
			</table>
		</div>
		<br>
	</blockquote>
</body>
<!-- add end by liuxiao 2008-01-21 -->

</html>
