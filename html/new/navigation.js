/*redirect form*/function get_form(page, wlan_id){        return 'boafrm/formWlanRedirect?redirect-url='+page+'&wlan_id='+wlan_id ;} /*add_menuItem(URL,title)*/function add_menuItem(frameset,url,name){  var str;  document.write("<td class=\"topnavoff\">");  str="<a href=\""+url+"\" target=\""+frameset+"\" id=\""+name+"\" rel=\""+name+"\" onclick=\"return on_click_menu(this);\">"+name+"</a>";  //alert(str);  document.write(str);  document.write("</td>");}function add_topMenuItem(url,menu_name){  add_menuItem("sub_menu",url,menu_name);}function add_subMenuItem(url,menu_name){	var str;  document.write("<tr height=\"35\">");    document.write("<td class=\"topnavoff\">");  str="<a href=\""+url+"\" target=\"content\" id=\""+menu_name+"\" rel=\""+menu_name+"\" onclick=\"return on_click_menu(this);\">"+menu_name+"</a>";  //alert(str);  document.write(str);  document.write("</td>");  //add_menuItem("content",url,menu_name);  document.write("</tr>");}function index_of(array,value){	var i;	for(i = 0; i < array.length; i++){		if(array[i] == value)			return i;			}	return -1;}function show_subnav_element(url,menu_name,isReadable){	if(isReadable){		add_subMenuItem(url,menu_name);	}}function show_topmenu_wlan(is8021xClient, isMeshDefined, is80211rSupport, isAirtimeDefined){		var subhtml_array = [<%getIndex("wlbasic.htm");%>,						<%getIndex("wladvanced.htm");%>,						<%getIndex("wlsecurity.htm");%>,						<%getIndex("wlactrl.htm");%>,						<%getIndex("wlwds.htm");%>,						<%getIndex("wlsurvey.htm");%>,						<%getIndex("wlwps.htm");%>,						<%getIndex("wlsch.htm");%>];		var first_show_index = index_of(subhtml_array,1); 		if(first_show_index == -1){		if(isMeshDefined){			if(<%getIndex("wlmesh.htm")%>)				return 1;		}		if(is8021xClient){			if(<%getIndex("rsCertInstall.htm")%>)				return 1;		}		if(isAirtimeDefined){			if(<%getIndex("airtime.htm")%>)				return 1;		}		return 0;	}else		return 1;		}function show_topmenu_tcpip(showLanDev){	var subhtml_array = [<%getIndex("tcpiplan.htm");%>,						<%getIndex("tcpipwan.htm");%>];		var first_show_index = index_of(subhtml_array,1); 	if(first_show_index == -1){		if(showLanDev){			if(<%getIndex("clients.htm");%>)				return 1;		}		return 0;	}else		return 1;	}function show_topmenu_ipv6(is_mape){	var subhtml_array = [<%getIndex("ipv6_wan.htm");%>,						<%getIndex("dhcp6s.htm");%>,						<%getIndex("radvd.htm");%>,						<%getIndex("tunnel6.htm");%>];		var first_show_index = index_of(subhtml_array,1); 	if(first_show_index == -1){		if(is_mape){			if(<%getIndex("map_e.htm");%>)				return 1;		}		return 0;	}else		return 1;	}function show_topmenu_firewall(multiple_wan_enable, ipv6_enable,route_setup_enable,quagga_enable,pimd_enable,mrouted_enable){	var subhtml_array1 = [<%getIndex("macfilter.htm");%>,						<%getIndex("portfw.htm");%>,						<%getIndex("urlfilter.htm");%>,						<%getIndex("dmz.htm");%>,						<%getIndex("vlan.htm");%>];	var first_show_index1 = index_of(subhtml_array1,1);	if(first_show_index1 == -1){		if(ipv6_enable){			if(multiple_wan_enable){				var subhtml_array2 = [<%getIndex("portfilter6.htm");%>,									  <%getIndex("ip6filter.htm");%>];							}			else{				var subhtml_array2 = [<%getIndex("portfilter6.htm");%>,									  <%getIndex("ip6filter.htm");%>,									  <%getIndex("ip6_qos.htm");%>];			}			var first_show_index2 = index_of(subhtml_array2,1);			if(first_show_index2 != -1)				return 1;		}else{			if(multiple_wan_enable){				var subhtml_array2 = [<%getIndex("portfilter.htm");%>,									  <%getIndex("ipfilter.htm");%>];			}			else{				var subhtml_array2 = [<%getIndex("portfilter.htm");%>,									<%getIndex("ipfilter.htm");%>,									<%getIndex("ip_qos.htm");%>];			}			var first_show_index2 = index_of(subhtml_array2,1);			if(first_show_index2 != -1)				return 1;		}		if(quagga_enable)			{			<%getIndex("quaggaroute.htm");%>			<%getIndex("bgproute.htm");%>			<%getIndex("isisroute.htm");%>			}		if(pimd_enable)			{			<%getIndex("pimd.htm");%>			}		if(mrouted_enable)			{			<%getIndex("mrouted.htm");%>			}		if(route_setup_enable){			if(<%getIndex("route.htm");%>)				return 1;		}		return 0;	}	else		return 1;}function show_topmenu_voip(){	var subhtml_array = [<%getIndex("voip_general.asp");%>,						<%getIndex("voip_tone.asp");%>,						<%getIndex("voip_ring.asp");%>,						<%getIndex("voip_other.asp");%>,						//<%getIndex("voip_config.asp");%>,						<%getIndex("voip_network.asp");%>];	var first_show_index = index_of(subhtml_array,1);	if(first_show_index == -1)		return 0;	else		return 1;}function show_topmenu_mng(isDisplayCPU, isEnableBT,isDisplayTR069){	var subhtml_array = [<%getIndex("status.htm");%>,						<%getIndex("stats.htm");%>,						<%getIndex("ddns.htm");%>,						<%getIndex("ntp.htm");%>,						<%getIndex("dos.htm");%>,						<%getIndex("syslog.htm");%>,						<%getIndex("upload.htm");%>,						<%getIndex("saveconf.htm");%>];	var first_show_index = index_of(subhtml_array,1);	if(first_show_index == -1){		if(isDisplayCPU){			if(<%getIndex("cpuShow.htm");%>)				return 1;		}							if(isDisplayTR069){			if(<%getIndex("tr069config.htm");%>)				return 1;		}		if(isEnableBT){			if(<%getIndex("transmission.htm");%>)				return 1;		}					if(userDegreeFlag == 1){			if(<%getIndex("super_password.htm");%>)				return 1;		}		else{			if(<%getIndex("password.htm");%>)				return 1;		}		return 0;	}	else		return 1;		}function show_topmenu_disk(){	var subhtml_array = [<%getIndex("diskinfo.htm");%>,						<%getIndex("accountmng.htm");%>,						<%getIndex("disksharefolder.htm");%>,						<%getIndex("diskpartition.htm");%>,						<%getIndex("diskformat.htm");%>,];			var subnav_array = ["Disk Information",						"Account Management",						"Share Folder",						"Disk Partition",						"Disk Format"];	var first_show_index = index_of(subhtml_array,1);	if(first_show_index != -1){		return 0;	}else		return 1;}function init_submenu(submenuId){	if(document.getElementById(submenuId))	{		if(document.getElementById(submenuId).click)		{			document.getElementById(submenuId).click();		}		else		{			var evt  = document.createEvent('MouseEvents');			evt.initEvent('click',true,true);			document.getElementById(submenuId).dispatchEvent(evt);		}	}}function init_submenu_wlan(is8021xClient, isMeshDefined, is80211rSupport, isAirtimeDefined){	var subhtml_array1 = [<%getIndex("wlbasic.htm");%>,						<%getIndex("wladvanced.htm");%>,						<%getIndex("wlsecurity.htm");%>,						<%getIndex("wlactrl.htm");%>,						<%getIndex("wlwds.htm");%>,						<%getIndex("wlp2p.htm")%>];			var subnav_array1 = ["Basic Setting",						"Advanced",						"Security",						"Access Control",						"WDS Setting",						"Wi-Fi Direct"];	var first_show_index1 = index_of(subhtml_array1,1);	if(first_show_index1== -1){		if(isMeshDefined){			if(<%getIndex("wlmesh.htm");%>){				init_submenu("Mesh Setting");				return;			}		}					var subhtml_array2 = [<%getIndex("wlsurvey.htm");%>,							<%getIndex("wlwps.htm");%>,							<%getIndex("wlsch.htm");%>];					var subnav_array2 = ["Site Survey","WPS","Schedule"];		var first_show_index2 = index_of(subhtml_array2,1);		if(first_show_index2 == -1){				if(is8021xClient){					if(<%getIndex("rsCertInstall.htm");%>){						init_submenu("802.1x Cert Install");						return;					}				}				if(isAirtimeDefined){					if(<%getIndex("airtime.htm");%>){						init_submenu("Airtime Fairness");						return;					}				}						}else			init_submenu(subnav_array2[first_show_index2]);	}	else		init_submenu(subnav_array1[first_show_index1]);}function init_submenu_tcpip(showLanDev){	if(<%getIndex("tcpiplan.htm")%> == 0){		if(showLanDev){			if(<%getIndex("clients.htm");%>){				init_submenu("Lan Clients");				return;			}		}		if(<%getIndex("tcpipwan.htm");%>){			init_submenu("Wan Setting");			return;		}	}else		init_submenu("Lan Setting");}function init_submenu_ipv6(is_mape){	var subhtml_array = [<%getIndex("ipv6_wan.htm");%>,						<%getIndex("dhcp6s.htm");%>,						<%getIndex("radvd.htm");%>,						<%getIndex("tunnel6.htm");%>];			var subnav_array = ["IPv6 Wan Setting",						"IPv6 Lan Setting",						"Radvd",						"Tunnel (6 over 4)"];	var first_show_index = index_of(subhtml_array,1);	if(first_show_index == -1){		if(is_mape){			if(<%getIndex("map_e.htm");%>){				init_submenu("MAP-E");				return;			}		}	}	else		init_submenu(subnav_array[first_show_index]);  }function init_submenu_voip(){	var subhtml_array = [<%getIndex("voip_general.asp");%>,						<%getIndex("voip_tone.asp");%>,						//<%getIndex("voip_ring.asp");%>,						<%getIndex("voip_other.asp");%>,						//<%getIndex("voip_config.asp");%>,						<%getIndex("voip_network.asp");%>];			var subnav_array = ["General",						"Tone",						//"Ring",						"Other",						//"Config",						"Network"];	var first_show_index = index_of(subhtml_array,1);	if(first_show_index != -1){		init_submenu(subnav_array[first_show_index]);		return;	}		}function init_submenu_firewall(multiple_wan_enable, ipv6_enable,route_setup_enable,quagga_enable,pimd_enable,mrouted_enable){		if(ipv6_enable)	{		var subhtml_array = [<%getIndex("portfilter6.htm");%>,							<%getIndex("ip6filter.htm");%>,							<%getIndex("macfilter.htm");%>,							<%getIndex("portfw.htm");%>,							<%getIndex("urlfilter.htm");%>,							<%getIndex("dmz.htm");%>,							<%getIndex("vlan.htm");%>];				var subnav_array = ["Port Filtering",							"IP Filtering",							"MAC Filtering",							"Port Forwarding",							"URL Filtering",							"DMZ",							"vlan"];		var first_show_index = index_of(subhtml_array,1);		if(first_show_index != -1){			init_submenu(subnav_array[first_show_index]);			return;		}	}	else{		var subhtml_array = [<%getIndex("portfilter.htm");%>,							<%getIndex("ipfilter.htm");%>,							<%getIndex("macfilter.htm");%>,							<%getIndex("portfw.htm");%>,							<%getIndex("urlfilter.htm");%>,							<%getIndex("dmz.htm");%>,							<%getIndex("vlan.htm");%>];				var subnav_array = ["Port Filtering",							"IP Filtering",							"MAC Filtering",							"Port Forwarding",							"URL Filtering",							"DMZ",							"vlan"];		var first_show_index = index_of(subhtml_array,1);		if(first_show_index != -1){			init_submenu(subnav_array[first_show_index]);			return;		}	}	 if(route_setup_enable){		if(<%getIndex("route.htm");%>){			init_submenu("Route Setup");			return;		}	 }if(quagga_enable){		if(<%getIndex("quaggaroute.htm");%>){			init_submenu("Ospf Setup");			//return;		}		if(<%getIndex("bgproute.htm");%>){			init_submenu("Bgp Setup");			//return;		}		if(<%getIndex("isisroute.htm");%>){			init_submenu("IsIs Setup");			//return;		}	 }if(pimd_enable){		if(<%getIndex("pimd.htm");%>){			init_submenu("Pimd Setup");			//return;		}	 }if(mrouted_enable){		if(<%getIndex("mrouted.htm");%>){			init_submenu("Mrouted Setup");			//return;		}	 }	if(multiple_wan_enable == 0){		 if(ipv6_enable){			if(<%getIndex("ip6_qos.htm");%>){				init_submenu("Qos");				return;			}		 }else{			if(<%getIndex("ip_qos.htm");%>){				init_submenu("Qos");				return;			}		 }	}}function init_submenu_mng(isDisplayCPU, isEnableBT,isDisplayTR069){	var subtml_array1 = [<%getIndex("status.htm");%>,						<%getIndex("stats.htm");%>,						<%getIndex("ddns.htm");%>];	var subnav_array1 = ["Status","Statistics","DDNS"];	var first_show_index1 = index_of(subtml_array1,1); 	if(first_show_index1 == -1){		if(isDisplayCPU){			if(<%getIndex("cpuShow.htm");%>){				init_submenu("CPU Utilizaiton");				return;			}						}		var subtml_array2 = [<%getIndex("ntp.htm");%>,							<%getIndex("dos.htm");%>];		var subnav_array2 = ["Time Zone Setting","Deny Of Serivce"];		var first_show_index2 = index_of(subtml_array2,1); 		if(first_show_index2 == -1){			if(isDisplayTR069){				if(<%getIndex("tr069config.htm");%>){					init_submenu("TR-069 Config");					return;				}								}			var subtml_array3 = [<%getIndex("syslog.htm");%>,								<%getIndex("upload.htm");%>,								 <%getIndex("saveconf.htm");%>];			var subnav_array3 = ["Log","Upgrade Firmware","Save/Reload Setting"];			var first_show_index3 = index_of(subtml_array3,1); 			if(first_show_index3 == -1){				if(userDegreeFlag == 1){					if(<%getIndex("super_password.htm");%>){						init_submenu("Password");						return;					}									}else{					if(<%getIndex("password.htm");%>){						init_submenu("Password");						return;					}				}				if(isEnableBT){					if(<%getIndex("transmission.htm");%>){						init_submenu("Transmission BT");						return;					}				}			}			else				init_submenu(subnav_array3[first_show_index3]);					}		else			init_submenu(subnav_array2[first_show_index2]);								}	else		init_submenu(subnav_array1[first_show_index1]);	 		}function init_submenu_disk(){	var subhtml_array = [<%getIndex("diskinfo.htm");%>,						<%getIndex("accountmng.htm");%>,						<%getIndex("disksharefolder.htm");%>,						<%getIndex("diskpartition.htm");%>,						<%getIndex("diskformat.htm");%>,];			var subnav_array = ["Disk Information",						"Account Management",						"Share Folder",						"Disk Partition",						"Disk Format"];	var first_show_index = index_of(subhtml_array,1);	if(first_show_index != -1){		init_submenu(subnav_array[first_show_index]);		return;	}  }/*draw_topnav*/function draw_topnav(multiple_wan_enable, wlan_num,ipv6,isDisplayVOIP,userDegreeFlag,is8021xClient, isMeshDefined, is80211rSupport, isAirtimeDefined,                           showLanDev,is_mape,route_setup_enable,quagga_enable,pimd_enable,mrouted_enable,                           isDisplayCPU,isEnableBT,isDisplayTR069, disk_onoff, multi_ap_onoff){	if(multiple_wan_enable == 0)  		add_topMenuItem("sub_menu_setup.htm","Setup"); 	  if(userDegreeFlag != 0){    if(<%getIndex("sub_menu_wlan_support");%> != 0)    {		if(<%getIndex("sub_menu_wlan.htm");%>){			if(show_topmenu_wlan(is8021xClient, isMeshDefined, is80211rSupport, isAirtimeDefined)){				add_topMenuItem(get_form("sub_menu_wlan.htm",0),"Wlan1");	 			if(wlan_num==2)	  				add_topMenuItem(get_form("sub_menu_wlan.htm",1),"Wlan2");			}			}    }	if(<%getIndex("sub_menu_tcpip.htm");%>){		if(show_topmenu_tcpip(showLanDev)){			add_topMenuItem("sub_menu_tcpip.htm","TCP/IP");		}	}	if(ipv6){		if(<% getIndex("multiWanSupport"); %>+0 == 0)		{			if(<%getIndex("sub_menu_ipv6.htm");%>)				if(show_topmenu_ipv6(is_mape))					add_topMenuItem("sub_menu_ipv6.htm","ipv6");		}	}	if (<%getInfo("show_firewall_all")%>) {		if(<%getIndex("sub_menu_firewall.htm");%>){			if(show_topmenu_firewall(multiple_wan_enable, ipv6,route_setup_enable,quagga_enable,pimd_enable,mrouted_enable))				add_topMenuItem("sub_menu_firewall.htm","Firewall");		}	}	if (<%getInfo("show_multiwan_qos")%>) {		if(<%getIndex("sub_menu_qos.htm");%>){				add_topMenuItem("sub_menu_qos.htm","Qos");		}	}			if(isDisplayVOIP){		if(<%getIndex("sub_menu_voip.htm");%>)			if(show_topmenu_voip())				add_topMenuItem("sub_menu_voip.htm","VoIP");	}			if(<%getIndex("sub_menu_mng.htm");%>){		if(show_topmenu_mng(isDisplayCPU,isEnableBT,isDisplayTR069))			add_topMenuItem("sub_menu_mng.htm","Management");	}		if (disk_onoff){		if(<%getIndex("sub_menu_disk_mng.htm");%>){			if(show_topmenu_disk())				add_topMenuItem("sub_menu_disk_mng.htm","Storage");		}	 }		  }  else{     if(<%getIndex("sub_menu_wlan_support");%> != 0)		add_topMenuItem(get_form("sub_menu_wlan.htm",0),"Wlan1"); 	 if(wlan_num==2)  		add_topMenuItem(get_form("sub_menu_wlan.htm",1),"Wlan2");	 if(multi_ap_onoff)		add_topMenuItem("sub_menu_multi_ap.htm","EasyMesh");  	 add_topMenuItem("sub_menu_tcpip.htm","TCP/IP"); 	 if(ipv6)	 {		 if(<% getIndex("multiWanSupport"); %>+0 == 0)  		{			add_topMenuItem("sub_menu_ipv6.htm","ipv6");		} 	 }	 if (<%getInfo("show_firewall_all")%>) {  	 	add_topMenuItem("sub_menu_firewall.htm","Firewall");	 }	 if (<%getInfo("show_multiwan_qos")%>) {	 	add_topMenuItem("sub_menu_qos.htm","Qos");	 }  	 if( isDisplayVOIP)  		add_topMenuItem("sub_menu_voip.htm","VoIP");  	 add_topMenuItem("sub_menu_mng.htm","Management");	 if (disk_onoff)  		add_topMenuItem("sub_menu_disk_mng.htm","Storage");  }  if(multiple_wan_enable == 0)  	init_submenu("Setup");  else  	init_submenu("Wlan1");  	}function draw_subnav_head(){	document.write("<table id=\"topnav_container\" border=\"0\" cellspacing=\"1\" summary=\"\">");	document.write("<tbody>");	document.write("<tr height=\"34\">");	document.write("<td class=\"topHeaderMenu\">");	document.write("<div id=\"topHeaderId\"></div>");	document.write("</td></tr>");}function draw_subnav_tail(){	document.write("<tr><td class=\"subMenuDummy\"></td></tr>");	document.write("</tbody></table>");}function draw_subnav_setup(multiple_wan_enable,OneWanSptUnderMtl,userDegreeFlag){    add_subMenuItem("wizard.htm","Wizard");  if (<%getInfo("show_opmode_menu")%>==1) {	  if(!multiple_wan_enable){		  if(userDegreeFlag != 0){			show_subnav_element("opmode.htm","Operation Mode",<%getIndex("opmode.htm")%>); 		  }else		  	add_subMenuItem("opmode.htm","Operation Mode");   	  }  } else if(<%getInfo("show_opmode_menu")%>==2) { // ulinker	  if(!multiple_wan_enable){	  			  if(userDegreeFlag != 0){		  				show_subnav_element("ulinker_opmode.htm","ULinker Operation Mode",<%getIndex("ulinker_opmode.htm")%>); 		  			  }else		  	add_subMenuItem("ulinker_opmode.htm","ULinker Operation Mode"); 	  	}   }   init_submenu("Wizard");}function draw_subnav_wlan(is8021xClient, isWapiSupport, isWapiLocalAsSupport, isMeshDefined, is80211rSupport, isAirtimeDefined,userDegreeFlag){	if(userDegreeFlag != 0){		show_subnav_element("wlbasic.htm","Basic Setting",<%getIndex("wlbasic.htm")%>); 		show_subnav_element("wladvanced.htm","Advanced",<%getIndex("wladvanced.htm")%>);  		show_subnav_element("wlsecurity.htm","Security",<%getIndex("wlsecurity.htm")%>);  		show_subnav_element("wlactrl.htm","Access Control",<%getIndex("wlactrl.htm")%>);  		show_subnav_element("wlwds.htm","WDS Setting",<%getIndex("wlwds.htm")%>);		if(isMeshDefined==1)	  		show_subnav_element("wlmesh.htm","Mesh Setting",<%getIndex("wlmesh.htm")%>);    		show_subnav_element("wlsurvey.htm","Site Survey",<%getIndex("wlsurvey.htm")%>);  		show_subnav_element("wlwps.htm","WPS",<%getIndex("wlwps.htm")%>);  		show_subnav_element("wlsch.htm","Schedule",<%getIndex("wlsch.htm")%>);		if(is8021xClient==1)     		show_subnav_element("rsCertInstall.htm","802.1x Cert Install",<%getIndex("rsCertInstall.htm")%>); 		if(isAirtimeDefined==1)   			show_subnav_element("airtime.htm","Airtime Fairness",<%getIndex("airtime.htm")%>);		if(<% getIndex("isP2PSupport")%>)			show_subnav_element("wlp2p.htm", "Wi-Fi Direct", <%getIndex("wlp2p.htm")%>);						init_submenu_wlan(is8021xClient,isMeshDefined,is80211rSupport,isAirtimeDefined);	}else{		add_subMenuItem("wlbasic.htm","Basic Setting");  		add_subMenuItem("wladvanced.htm","Advanced");  		add_subMenuItem("wlsecurity.htm","Security");  		add_subMenuItem("wlactrl.htm","Access Control");  		add_subMenuItem("wlwds.htm","WDS Setting"); 		if(isMeshDefined==1)	  		add_subMenuItem("wlmesh.htm","Mesh Setting");    		add_subMenuItem("wlsurvey.htm","Site Survey");  		add_subMenuItem("wlwps.htm","WPS");  		add_subMenuItem("wlsch.htm","Schedule");  		if(is8021xClient==1)     		add_subMenuItem("rsCertInstall.htm","802.1x Cert Install");		if(isWapiSupport==1)		{     			add_subMenuItem("wlwapiinstallcert.htm","Certification Install");			add_subMenuItem("wlwapiRekey.htm","Key Update");		}		if(isWapiLocalAsSupport==1)     		add_subMenuItem("wlwapiCertManagement.htm","Certification Manage");		 		 if(isAirtimeDefined==1)   		 	add_subMenuItem("airtime.htm","Airtime Fairness");		if(<% getIndex("isP2PSupport")%>)			add_subMenuItem("wlp2p.htm", "Wi-Fi Direct");				 init_submenu("Basic Setting");	}}  function draw_subnav_tcpip(multiple_wan_enable,showLanDev,ipv6_enable,userDegreeFlag){	if(userDegreeFlag != 0){		show_subnav_element("tcpiplan.htm","Lan Setting",<%getIndex("tcpiplan.htm")%>);		if(showLanDev==1)			show_subnav_element("clients.htm","Lan Clients",<%getIndex("clients.htm")%>);		if (<%getInfo("show_tcpip_wan")%>) {			show_subnav_element("tcpipwan.htm","Wan Setting",<%getIndex("tcpipwan.htm")%>);		}/*		if(ipv6_enable)		{			if(multiple_wan_enable)			{		  		add_subMenuItem("dhcp6s.htm","IPv6 Lan Setting");		  		add_subMenuItem("radvd.htm","Radvd");				if(<% getIPv6Info("is_radvd_wan"); %> + 0)					add_subMenuItem("radvd_wan.htm","Radvd wan");		  		add_subMenuItem("tunnel6.htm","Tunnel (6 over 4)");		  		if( <% getIPv6Info("is_mape"); %> + 0)		  			add_subMenuItem("map_e.htm","MAP-E");			}		}*/		init_submenu_tcpip(showLanDev);	}else{		add_subMenuItem("tcpiplan.htm","Lan Setting");  		if(showLanDev==1)  			add_subMenuItem("clients.htm","Lan Clients"); 		if (<%getInfo("show_tcpip_wan")%>) {			if(multiple_wan_enable)			  	add_subMenuItem("tcpip_multiwanlist.htm","WAN SETTING");			 else		  		add_subMenuItem("tcpipwan.htm","Wan Setting");		}/*		if(ipv6_enable)		{			if(multiple_wan_enable)			{		  		add_subMenuItem("dhcp6s.htm","IPv6 Lan Setting");		  		add_subMenuItem("radvd.htm","Radvd");				if(<% getIPv6Info("is_radvd_wan"); %> + 0)					add_subMenuItem("radvd_wan.htm","Radvd wan");		  		add_subMenuItem("tunnel6.htm","Tunnel (6 over 4)");		  		if( <% getIPv6Info("is_mape"); %> + 0)		  			add_subMenuItem("map_e.htm","MAP-E");			}		}*/		init_submenu("Lan Setting");		}  }function draw_subnav_multi_ap(userDegreeFlag){	if(userDegreeFlag != 0){		// show_subnav_element("tcpiplan.htm","Lan Setting",<%getIndex("tcpiplan.htm")%>);		// if(showLanDev==1)		// 	show_subnav_element("clients.htm","Lan Clients",<%getIndex("clients.htm")%>);		// if (<%getInfo("show_tcpip_wan")%>) {		// 	show_subnav_element("tcpipwan.htm","Wan Setting",<%getIndex("tcpipwan.htm")%>);		// }		// init_submenu_tcpip(showLanDev);	}else{		add_subMenuItem("multi_ap_setting_general.htm","General");		if ( 1 == <% getIndex("multi_ap_controller"); %>) {			add_subMenuItem("multi_ap_setting_topology.htm","Topology");		}		init_submenu("General");	}}function draw_subnav_ipv6(is_mape,is_radvd_wan,userDegreeFlag){	if(userDegreeFlag != 0){		show_subnav_element("ipv6_wan.htm","IPv6 Wan Setting",<%getIndex("ipv6_wan.htm")%>);  		show_subnav_element("dhcp6s.htm","IPv6 Lan Setting",<%getIndex("dhcp6s.htm")%>);  		show_subnav_element("radvd.htm","Radvd",<%getIndex("radvd.htm")%>);		if(is_radvd_wan)			show_subnav_element("radvd_wan.htm","Wan Radvd",<%getIndex("radvd_wan.htm")%>);  		show_subnav_element("tunnel6.htm","Tunnel (6 over 4)",<%getIndex("tunnel6.htm")%>);		if(is_mape)  			show_subnav_element("map_e.htm","MAP-E",<%getIndex("map_e.htm")%>);				init_submenu_ipv6(is_mape);	}else{		add_subMenuItem("ipv6_wan.htm","IPv6 Wan Setting");  		add_subMenuItem("dhcp6s.htm","IPv6 Lan Setting");  		add_subMenuItem("radvd.htm","Radvd");		if(is_radvd_wan)			add_subMenuItem("radvd_wan.htm","Radvd wan");  		add_subMenuItem("tunnel6.htm","Tunnel (6 over 4)");  		if(is_mape)  			add_subMenuItem("map_e.htm","MAP-E");				init_submenu("IPv6 Wan Setting");	}}function draw_subnav_firewall(multiple_wan_enable, ipv6_enable, route_setup_enable,quagga_enable,pimd_enable,mrouted_enable,userDegreeFlag){	if (<%getInfo("show_firewall_all")%>) {		if(userDegreeFlag != 0){			if(ipv6_enable)			{				show_subnav_element("portfilter6.htm","Port Filtering",<%getIndex("portfilter6.htm");%>);				show_subnav_element("ip6filter.htm","IP Filtering",<%getIndex("ip6filter.htm");%>);			}			else			{		 	 	show_subnav_element("portfilter.htm","Port Filtering",<%getIndex("portfilter.htm");%>);		  		show_subnav_element("ipfilter.htm","IP Filtering",<%getIndex("ipfilter.htm");%>);			}		 	show_subnav_element("macfilter.htm","MAC Filtering",<%getIndex("macfilter.htm");%>);		  	show_subnav_element("portfw.htm","Port Forwarding",<%getIndex("portfw.htm");%>);		  	show_subnav_element("urlfilter.htm","URL Filtering",<%getIndex("urlfilter.htm");%>);		  	show_subnav_element("dmz.htm","DMZ",<%getIndex("dmz.htm");%>);			//if(<%getIndex("rtk_vlan_support");%>)				//show_subnav_element("vlan.htm","vlan",<%getIndex("vlan.htm");%>);			if(<%getIndex("linux_vlan_support");%>)				show_subnav_element("8021q_vlan.htm","802.1Q VLAN",<%getIndex("8021q_vlan.htm");%>);			if(route_setup_enable)		  		show_subnav_element("route.htm","Route Setup",<%getIndex("route.htm");%>);			if(quagga_enable)				{		  		show_subnav_element("quaggaroute.htm","Ospf Setup",<%getIndex("quaggaroute.htm");%>);				show_subnav_element("bgproute.htm","Bgp Setup",<%getIndex("bgproute.htm");%>);				show_subnav_element("isisroute.htm","IsIs Setup",<%getIndex("isisroute.htm");%>);				}			if(pimd_enable)				{		  		show_subnav_element("pimd.htm","Pimd Setup",<%getIndex("pimd.htm");%>);				}			if(mrouted_enable)				{		  		show_subnav_element("mrouted.htm","Mrouted Setup",<%getIndex("mrouted.htm");%>);				}			if(multiple_wan_enable == 0){			  	if(ipv6_enable)			  		show_subnav_element("ip6_qos.htm","Qos",<%getIndex("ip6_qos.htm");%>);			 	 else			  		show_subnav_element("ip_qos.htm","Qos",<%getIndex("ip_qos.htm");%>);			}						if(<%getIndex("vlan_binding_support");%>)				add_subMenuItem("vlan_binding.htm","VLAN BINDING");			init_submenu_firewall(multiple_wan_enable, ipv6_enable,route_setup_enable,quagga_enable,pimd_enable,mrouted_enable);		}		else{			if(ipv6_enable)			{				add_subMenuItem("portfilter6.htm","Port Filtering");				add_subMenuItem("ip6filter.htm","IP Filtering");			}			else			{		 	 	add_subMenuItem("portfilter.htm","Port Filtering");		 	 	add_subMenuItem("ipfilter.htm","IP Filtering");			}		 		add_subMenuItem("macfilter.htm","MAC Filtering");		  		add_subMenuItem("portfw.htm","Port Forwarding");		  		add_subMenuItem("urlfilter.htm","URL Filtering");		  		add_subMenuItem("dmz.htm","DMZ");		  					//if(<%getIndex("rtk_vlan_support");%>)				//add_subMenuItem("vlan.htm","vlan");			if(<%getIndex("linux_vlan_support");%>)				add_subMenuItem("8021q_vlan.htm","802.1Q VLAN");		  		 	if(route_setup_enable)		  		add_subMenuItem("route.htm","Route Setup");			if(quagga_enable)				{		  		add_subMenuItem("quaggaroute.htm","Ospf Setup");				add_subMenuItem("bgproute.htm","Bgp Setup");				add_subMenuItem("isisroute.htm","IsIs Setup");				}			if(pimd_enable)		  		add_subMenuItem("pimd.htm","Pimd Setup");			if(mrouted_enable)		  		add_subMenuItem("mrouted.htm","Mrouted Setup");			if(multiple_wan_enable == 0){			  	if(ipv6_enable)			  		add_subMenuItem("ip6_qos.htm","Qos");			 	else			  		add_subMenuItem("ip_qos.htm","Qos");			}						if(<%getIndex("snmp_support")%>)				add_subMenuItem("snmp.htm","SNMP");							if(<%getIndex("vlan_binding_support");%>)				add_subMenuItem("vlan_binding.htm","VLAN BINDING");		  	init_submenu("Port Filtering");		}	}}function draw_subnav_qos(){	if (<%getInfo("show_multiwan_qos")%>) {		  add_subMenuItem("traffic_shaping.htm","Traffic Shaping");		  add_subMenuItem("qos_policy.htm","QoS Policy");		  add_subMenuItem("qos_classification.htm","Classificaion");		  init_submenu("Traffic Shaping");	}}function draw_subnav_voip(userDegreeFlag){	/*if(userDegreeFlag != 0){		show_subnav_element("voip_general.asp","General",<%getIndex("voip_general.asp");%>); 		show_subnav_element("voip_tone.asp","Tone",<%getIndex("voip_tone.asp");%>);  		show_subnav_element("voip_ring.asp","Ring",<%getIndex("voip_ring.asp");%>); 		show_subnav_element("voip_other.asp","Other",<%getIndex("voip_other.asp");%>);  		show_subnav_element("voip_config.asp","Config",<%getIndex("voip_config.asp");%>);  		show_subnav_element("voip_network.asp","Network",<%getIndex("voip_network.asp");%>);		init_submenu_voip();	}else	*/	{		<% getInfo("voip_menu_new"); %> 		add_subMenuItem("voip_tone.asp","Tone");  		//add_subMenuItem("voip_ring.asp","Ring"); 		add_subMenuItem("voip_other.asp","Other");  		//add_subMenuItem("voip_config.asp","Config");  		add_subMenuItem("voip_network.asp","Network");		init_submenu(<%getInfo("voip_submenu_1");%>);	}}function draw_subnav_mng(isDisplayCPU, isEnableBT,isDisplayTR069,isDisplayOpenvpn,userDegreeFlag,supportSuperUser,supportDiagnostic,isSupportWeave,isSupportZigBee){	if(userDegreeFlag != 0){		show_subnav_element("status.htm","Status",<%getIndex("status.htm");%>);		show_subnav_element("stats.htm","Statistics",<%getIndex("stats.htm");%>);		if (<%getInfo("show_mgt_ddns")%>) {			show_subnav_element("ddns.htm","DDNS",<%getIndex("ddns.htm");%>);		}		if(isDisplayCPU == 1)  			show_subnav_element("cpuShow.htm","CPU Utilizaiton",<%getIndex("cpuShow.htm");%>);		if (<%getInfo("show_mgt_ntp")%>) {			show_subnav_element("ntp.htm","Time Zone Setting",<%getIndex("ntp.htm");%>);		}		if (<%getInfo("show_mgt_dos")%>) {			show_subnav_element("dos.htm","Deny Of Serivce",<%getIndex("dos.htm");%>);    		}    	if(isDisplayTR069 == 1)  			show_subnav_element("tr069config.htm","TR-069 Config",<%getIndex("tr069config.htm");%>);     		show_subnav_element("syslog.htm","Log",<%getIndex("syslog.htm");%>);    	show_subnav_element("upload.htm","Upgrade Firmware",<%getIndex("upload.htm");%>);    	show_subnav_element("saveconf.htm","Save/Reload Setting",<%getIndex("saveconf.htm");%>);  		if(userDegreeFlag == 1) 	 		show_subnav_element("super_password.htm","Password",<%getIndex("super_password.htm");%>);  		else 			show_subnav_element("password.htm","Password",<%getIndex("password.htm");%>); 		if(isEnableBT == 1)  			show_subnav_element("transmission.htm","Transmission BT",<%getIndex("transmission.htm");%>);		if(<%getInfo("show_elink")%>)		show_subnav_element("elink.asp","elink配置",<%getIndex("elink.asp");%>);		init_submenu_mng(isDisplayCPU, isEnableBT,isDisplayTR069);	}	else{		add_subMenuItem("status.htm","Status");  		add_subMenuItem("stats.htm","Statistics");		if (<%getInfo("show_mgt_ddns")%>) {  			add_subMenuItem("ddns.htm","DDNS");		}  		if(isDisplayCPU == 1)  			add_subMenuItem("cpuShow.htm","CPU Utilizaiton");		if (<%getInfo("show_mgt_ntp")%>) {  			add_subMenuItem("ntp.htm","Time Zone Setting");		}		if (<%getInfo("show_mgt_dos")%>) {  			add_subMenuItem("dos.htm","Deny Of Service");		}  		if(isDisplayTR069 == 1)  			add_subMenuItem("tr069config.htm","TR-069 Config");  		add_subMenuItem("syslog.htm","Log");  		add_subMenuItem("upload.htm","Upgrade Firmware");  		add_subMenuItem("saveconf.htm","Save/Reload Setting");		if(supportSuperUser)			add_subMenuItem("super_password.htm","Password");		else  			add_subMenuItem("password.htm","Password");		  		if(isEnableBT == 1)  			add_subMenuItem("transmission.htm","Transmission BT");		if(isSupportWeave==1)  			add_subMenuItem("weave.htm","Weave setting");		if(isSupportZigBee==1)  			add_subMenuItem("zigbee.htm","ZigBee Management");		if(isDisplayOpenvpn == 1)  			add_subMenuItem("openvpn.htm","Openvpn Setting");  		if(supportDiagnostic == 1)  			add_subMenuItem("diagnostic.htm","System Diagnostic"); 		init_submenu("Status");	}} function draw_subnav_disk(userDegreeFlag){	if(userDegreeFlag != 0){		show_subnav_element("diskinfo.htm","Disk Information",<%getIndex("diskinfo.htm");%>);		show_subnav_element("accountmng.htm","Account Management",<%getIndex("accountmng.htm");%>);		show_subnav_element("disksharefolder.htm","Share Folder",<%getIndex("disksharefolder.htm");%>);		show_subnav_element("diskpartition.htm","Disk Partition",<%getIndex("diskpartition.htm");%>);		show_subnav_element("diskformat.htm","Disk Format",<%getIndex("diskformat.htm");%>);		init_submenu_disk();	}else{		add_subMenuItem("diskinfo.htm","Disk Information");  		add_subMenuItem("accountmng.htm","Account Management");  		add_subMenuItem("disksharefolder.htm","Share Folder"); 		add_subMenuItem("diskpartition.htm","Disk Partition");  		add_subMenuItem("diskformat.htm","Disk Format");		init_submenu("Disk Information");	} }function has_class(element, class_name){        if (!element.className) {                element.className = "";                return false;        }        var regex = new RegExp("(^|\\s)\\s*" + class_name + "\\s*(\\s|$)");        return regex.test(element.className);}/*add_class()*/function add_class(element, class_name){        if (has_class(element, class_name)) {                return;        }        element.className += (element.className == "" ? "" : " ") + class_name;}/*remove_class()*/function remove_class(element, class_name){        if (!element.className) {                element.className = "";                return;        }        /*         * This regex is similar to \bclassName\b, except that \b does not         * treat certain legal CSS characters as "word characters": notably,         * the . and - characters.         */        var regex = new RegExp("(^|\\s)\\s*" + class_name + "\\s*(\\s|$)");        element.className = element.className.replace(regex, "$1$2");}/*on_click_menu(this)*/function on_click_menu(element){  var items = document.getElementsByTagName("a");  for (var i = 0; i < items.length; i++) {        var item = items[i];        remove_class(item.parentNode, "topnavon");        add_class(item.parentNode, "topnavoff");  }  remove_class(element.parentNode, "topnavoff");  add_class(element.parentNode, "topnavon"); }