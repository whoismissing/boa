#include "multi_user.h"

auth_html_t auth_html[]={
	/****html_page_name     **************super_user   *********************	common_user			************/
#ifdef CONFIG_TAIGUO_CTCOM_OP_MODE_CHECK
	{"super_password.htm", 					{ BR_MODE_AUTH(AUTH_WRITE)|GW_MODE_AUTH(AUTH_WRITE), BR_MODE_AUTH(AUTH_NO_READ)|GW_MODE_AUTH(AUTH_NO_READ) }	},
/*
	{"status.htm", 							{ BR_MODE_AUTH(AUTH_WRITE)|GW_MODE_AUTH(AUTH_WRITE), BR_MODE_AUTH(AUTH_READ)|GW_MODE_AUTH(AUTH_NO_READ) }	},
	{"tcpiplan.htm", 						{ BR_MODE_AUTH(AUTH_WRITE)|GW_MODE_AUTH(AUTH_WRITE), BR_MODE_AUTH(AUTH_READ)|GW_MODE_AUTH(AUTH_WRITE) }	},
	{"help_cmcc/help_status_device.html", 	{ BR_MODE_AUTH(AUTH_WRITE)|GW_MODE_AUTH(AUTH_WRITE), BR_MODE_AUTH(AUTH_NO_READ)|GW_MODE_AUTH(AUTH_READ) }	},
	{"help_cmcc/help_apply_upnp.html", 		{ BR_MODE_AUTH(AUTH_WRITE)|GW_MODE_AUTH(AUTH_WRITE), BR_MODE_AUTH(AUTH_READ)|GW_MODE_AUTH(AUTH_NO_READ) }	},
*/
#else
	{"super_password.htm",				 	{ AUTH_WRITE,  								AUTH_NO_READ }	},
#endif
	{NULL, 									{	NULL, 										NULL}	}
};

#ifdef CONFIG_TAIGUO_CTCOM_OP_MODE_CHECK
int getHtmlAuth(char *html_name, int login_user_level, int op_mode)
#else
int getHtmlAuth(char *html_name, int login_user_level)
#endif
{
	int i;
	auth_html_t *curHtml;
	char user_auth;

	if( !html_name || (login_user_level <= NO_LOGIN) || (login_user_level > MULTI_USER_NUMS) )
		return AUTH_WRITE;

	for( i = 0; auth_html[i].name; i++){
		curHtml = &auth_html[i];
		if( !strcmp(curHtml->name, html_name)){
			user_auth = curHtml->auth_mode[login_user_level-1];			
#ifdef CONFIG_TAIGUO_CTCOM_OP_MODE_CHECK
			if(op_mode == 0)//GATEWAY_MODE
				user_auth = (user_auth >> (AUTH_BITS*GW_MODE_INDEX)) & 0x03;
			else //BRIDGE_MODE
				user_auth = (user_auth >> (AUTH_BITS*BR_MODE_INDEX)) & 0x03;				
#endif
			//printf("[%s:%d] user_auth = %d\n",__FUNCTION__,__LINE__,user_auth);
			return user_auth;

		}

	}

	return AUTH_WRITE;
	
}


