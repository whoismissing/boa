#ifndef _MULTI_USER_H
#define _MULTI_USER_H

#include <stdio.h>

#define MULTI_USER_NUMS 2

typedef enum {
	NO_LOGIN = 0,
	SUPER_USER,
	COMMON_USER	
}LOGIN_USER;
	

typedef enum{
	AUTH_NO_READ = 0,
	AUTH_READ = 1,
	AUTH_WRITE = 3
}AUTH_MODE;

#ifdef CONFIG_TAIGUO_CTCOM_OP_MODE_CHECK
#define AUTH_BITS 2

#define GW_MODE_INDEX 0
#define BR_MODE_INDEX 1

#define GW_MODE_AUTH(auth_mode) ( auth_mode << (AUTH_BITS*GW_MODE_INDEX) )
#define BR_MODE_AUTH(auth_mode) ( auth_mode << (AUTH_BITS*BR_MODE_INDEX) )
#endif


typedef struct auth_html_s 
{
	char *name;
	char auth_mode[MULTI_USER_NUMS];
	
} auth_html_t;


#ifdef CONFIG_TAIGUO_CTCOM_OP_MODE_CHECK
int getHtmlAuth(char *html_name, int login_user_level, int op_mode);
#else
int getHtmlAuth(char *html_name, int login_user_level);
#endif


#endif


