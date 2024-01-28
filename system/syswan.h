#ifndef INCLUDE_SYSWAN_H
#define INCLUDE_SYSWAN_H

void start_dns_relay(void);

void start_upnp_igd();
#if defined(CONFIG_APP_MINIIGD)
void start_miniigd(int wantype, int sys_opmode, int wisp_id, char *interface);
#elif defined(CONFIG_APP_MINIUPNPD)
void start_miniupnpd(int wantype, int sys_opmode, int wisp_id, char *lan_interface);
#endif

void start_ddns(void);
void start_ntp(void);
void start_igmpproxy();
void 	start_routing();
int start_wan(int wan_mode, int sys_op, char *wan_iface, char *lan_iface, int wisp_id, int act_source);
void set_staticIP(int sys_op, char *wan_iface, char *lan_iface, int wisp_id, int act_source);
void set_dhcp_client(int sys_op, char *wan_iface, char *lan_iface, int wisp_id, int act_source);
void set_pppoe(int sys_op, char *wan_iface, char *lan_iface, int wisp_id, int act_source);
void set_pptp(int sys_op, char *wan_iface, char *lan_iface, int wisp_id, int act_source);
void set_l2tp(int sys_op, char *wan_iface, char *lan_iface, int wisp_id, int act_source);
void start_wan_dhcp_client(char *iface);
#endif



