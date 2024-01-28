#ifndef __RTL_H__
#define __RTL_H__

#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>


///////////////////////////////////////////////////////////
// fwd reference
///////////////////////////////////////////////////////////
static int rtl_fs__file_exist(char *file_name);
static int rtl_fs__file_open_fp(char * f_name, char * flags, FILE ** fp);
static int rtl_fs__file_write_raw(char * f_name, char * flags, char * buf, size_t buf_len, int fd);
static int rtl_fs__file_write_from_buf(char * f_name, char * flags, char * buf, size_t buf_len);
static int rtl_fs__file_fp2fd(FILE *fp, char * f_name, int *fd);
static int rtl_fs__file_dup2(int old, char * f_name, int new);
static int rtl_fs__file_symlink(char * f_name, char * f_link_name);
static int rtl_fs__file_unlink(char * f_name);
static int rtl_fs__file_unlink_S_(char * f_name);
static int rtl_fs__dir_create(char *dir_name);
static int rtl_fs__dir_create_S_(char *dir_name);
static int rtl_fs__exec(char **argv);
///////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////

#define RTL_COMMIT_VERSION 1
// cmd line
#define RTL_CMD__LINE_ELEM_MIN        3
#define RTL_CMD__LINE_ELEM_MAX        50
#define RTL_CMD__LINE_ELEM_LEN_MAX    200
#define RTL_CMD__LINE_VALUE_PARAM_MAX 30 // MIB_IPSECTUNNEL_ADD has 27+
#define RTL_HISTORY_SAFE_MODE_LINE_MAX      10
#define RTL_HISTORY_SAFE_SCRIPT_LAST_MAX    RTL_HISTORY_SAFE_MODE_LINE_MAX
#define RTL_HISTORY_SAFE_SCRIPT_PATH_LENGTH 500

// file system
#define RTL_DIR                     "/tmp/rtl/"
#define RTL_LAST_PREFIX             "safe_script_last"
#define RTL_LAST_PREFIX_SN          RTL_LAST_PREFIX "_sn(%u)_"
#define SAFE_SCRIPT_LAST_PREFIX     RTL_DIR RTL_LAST_PREFIX_SN
#define SAFE_SCRIPT_LAST            SAFE_SCRIPT_LAST_PREFIX RTL_TIME_FMT_FILE
#define SAFE_SCRIPT_LAST_SYMLINK    RTL_DIR "LAST"
#define SAFE_SCRIPT_REF_FILE        RTL_DIR "REF"
#define SAFE_MODE_HISTORY           RTL_DIR"history"
#define SAFE_MODE_HISTORY_OLD       RTL_DIR"history_old"
#define SAFE_COMMON_REDIR_TO_STDOUT RTL_DIR"redirON"

// buffer limit for output redirection
// if reached dump content and continue with buffering
#define RTL_OUTPUT_BUFFER 50000

//////////////////////////////////////////////////
// use pure macros for STRINGIFY FUNCTIONALITY !!!
// errors - KEEP IN SUNC WITH RTL_RC_TAB       !!!
//////////////////////////////////////////////////
//typedef enum rtl_err_codes {

/* POSITIVE - BASH LIKE */
#define RTL_RC__BASH_GENERAL                        1
#define RTL_RC__BASH_MISUSE                         2
#define RTL_RC__BASH_COMMAND_CANT_EXEC              126
#define RTL_RC__BASH_COMMAND_NOT_FOUND              127
#define RTL_RC__BASH_EXIT_INVALID                   128

#define RTL_RC__BOUNDARY                            246  // not used - used as reference for negative values
#define RTL_RC__RESTART_FORCE_ERROR                 251
#define RTL_RC__RESTART_ALL_ERROR                   252
#define RTL_RC__RESTART_WLAN_ERROR                  253
#define RTL_RC__RESTART_FW_ERROR                    254

#define RTL_RC__BASH_EXIT_OUT_OF_RANGE              255

#define RTL_RC__OK                                  0

/* POSITIVE - RTL RC LIKE */
#define RTL_RC__PARAMS_NB_ERROR                     (RTL_RC__BOUNDARY                          - 256)
#define RTL_RC__PARAMS_TRUNCATE_ERROR               (RTL_RC__PARAMS_NB_ERROR                   - 1)
#define RTL_RC__PARAMS_EMPTY_ERROR                  (RTL_RC__PARAMS_TRUNCATE_ERROR             - 1)
#define RTL_RC__PARAMS_KEY_EMPTY_ERROR              (RTL_RC__PARAMS_EMPTY_ERROR                - 1)
#define RTL_RC__PARAMS_VALUE_EMPTY_ERROR            (RTL_RC__PARAMS_KEY_EMPTY_ERROR            - 1)
#define RTL_RC__PARAMS_VALUE_TOOLONG_ERROR          (RTL_RC__PARAMS_VALUE_EMPTY_ERROR          - 1)
#define RTL_RC__PARAMS_VALUE_SPLIT_ERROR            (RTL_RC__PARAMS_VALUE_TOOLONG_ERROR        - 1)
#define RTL_RC__MIB_INIT_ERROR                      (RTL_RC__PARAMS_VALUE_SPLIT_ERROR          - 1)
#define RTL_RC__MIB_LOOKUP_ERROR                    (RTL_RC__MIB_INIT_ERROR                    - 1)
#define RTL_RC__MIB_SET_ERROR                       (RTL_RC__MIB_LOOKUP_ERROR                  - 1)
#define RTL_RC__FLS_UPD_CS_ERROR                    (RTL_RC__MIB_SET_ERROR                     - 1)
#define RTL_RC__FLS_UPD_DS_ERROR                    (RTL_RC__FLS_UPD_CS_ERROR                  - 1)
#define RTL_RC__FLS_UPD_DS_ERROR_CS_FATAL           (RTL_RC__FLS_UPD_DS_ERROR                  - 1)
#define RTL_RC__FLS_UPD_HS_ERROR                    (RTL_RC__FLS_UPD_DS_ERROR_CS_FATAL         - 1)
#define RTL_RC__FLS_UPD_HS_ERROR_CS_FATAL           (RTL_RC__FLS_UPD_HS_ERROR                  - 1)
#define RTL_RC__FLS_UPD_HS_ERROR_DS_FATAL           (RTL_RC__FLS_UPD_HS_ERROR_CS_FATAL         - 1)
#define RTL_RC__FLS_UPD_HS_ERROR_CS_DS_FATAL        (RTL_RC__FLS_UPD_HS_ERROR_DS_FATAL         - 1)
#define RTL_RC__MIB_LOCK_ERROR                      (RTL_RC__FLS_UPD_HS_ERROR_CS_DS_FATAL      - 1)
#define RTL_RC__MIB_UNLOCK_ERROR                    (RTL_RC__MIB_LOCK_ERROR                    - 1)
#define RTL_RC__PARAMS_VALUE_TBL_ROW_OP_ERROR       (RTL_RC__MIB_UNLOCK_ERROR                  - 1)
#define RTL_RC__PARAMS_VALUE_TBL_MANDATORY_ERROR    (RTL_RC__PARAMS_VALUE_TBL_ROW_OP_ERROR     - 1)
#define RTL_RC__ALLOC_ERROR                         (RTL_RC__PARAMS_VALUE_TBL_MANDATORY_ERROR  - 1)
#define RTL_RC__FD_OPEN_ERROR                       (RTL_RC__ALLOC_ERROR                       - 1)
#define RTL_RC__FD_WRITE_ERROR                      (RTL_RC__FD_OPEN_ERROR                     - 1)
#define RTL_RC__SINGLE_SCRIPT_POPEN_ERROR           (RTL_RC__FD_WRITE_ERROR                    - 1)
#define RTL_RC__SINGLE_SCRIPT_CHILD_SIGNAL_ERROR    (RTL_RC__SINGLE_SCRIPT_POPEN_ERROR         - 1)
#define RTL_RC__SINGLE_SCRIPT_CHILD_STATUS_ERROR    (RTL_RC__SINGLE_SCRIPT_CHILD_SIGNAL_ERROR  - 1)
#define RTL_RC__SINGLE_SCRIPT_FORK_ERROR            (RTL_RC__SINGLE_SCRIPT_CHILD_STATUS_ERROR  - 1)
#define RTL_RC__SINGLE_SCRIPT_PIPE_ERROR            (RTL_RC__SINGLE_SCRIPT_FORK_ERROR          - 1)
#define RTL_RC__DIR_MK_ERROR                        (RTL_RC__SINGLE_SCRIPT_PIPE_ERROR          - 1)
#define RTL_RC__DIR_OPEN_ERROR                      (RTL_RC__DIR_MK_ERROR                      - 1)
#define RTL_RC__FD_UNLINK_ERROR                     (RTL_RC__DIR_OPEN_ERROR                    - 1)
#define RTL_RC__FD_FILENO_ERROR                     (RTL_RC__FD_UNLINK_ERROR                   - 1)
#define RTL_RC__FD_DUP2_ERROR                       (RTL_RC__FD_FILENO_ERROR                   - 1)
#define RTL_RC__FD_SYMLINK_ERROR                    (RTL_RC__FD_DUP2_ERROR                     - 1)
#define RTL_RC__FD_READ_ERROR                       (RTL_RC__FD_SYMLINK_ERROR                  - 1)
#define RTL_RC__NO_REF_FILE_ERROR                   (RTL_RC__FD_READ_ERROR                     - 1)
#define RTL_RC__FD_MV_ERROR                         (RTL_RC__NO_REF_FILE_ERROR                 - 1)
#define RTL_RC__LIST_STA_INFO_ERROR                 (RTL_RC__FD_MV_ERROR                       - 1)

//} rtl_err_codes;


// utils
#define RTL_RC_ELEM(s)                      {s, #s},
#define RTL_RC_ELEM_LAST                    {0, NULL}
#define RTL_ARRAY_ELEMS(a)                  (sizeof(a)/sizeof(a[0]))

#define _RTL_STR(s) #s
#define RTL_STR(s) _RTL_STR(s)

// check helpers
#define RTL_CHECK_CALLOC_SS(p , len)        { (p) = (char *) calloc(1, (len)); if(!(p)) { RTL_LOG_RC_SS(RTL_RC__ALLOC_ERROR,"alloc failed - not enough memmory") } }
#define RTL_CHECK_CALLOC_SM(p , len)        { (p) = (char *) calloc(1, (len)); if(!(p)) { RTL_LOG_RC_SM(RTL_RC__ALLOC_ERROR,"alloc failed - not enough memmory") } }
#define RTL_CHECK_CALLOC_SYS(p , len)       { (p) = (char *) calloc(1, (len)); if(!(p)) { RTL_SYSLOG_RC(RTL_RC__ALLOC_ERROR,"alloc failed - not enough memmory") } }
#define RTL_CHECK_CALLOC_LIST(p , len)      { (p) = (char *) calloc(1, (len)); if(!(p)) { RTL_LOG_RC_LIST(RTL_RC__ALLOC_ERROR,"alloc failed - not enough memmory") } }
#define RTL_FREE(p)                         {  free((p));  (p) = NULL; }
#define RTL_CHECK_RC(rc, CMD)               { (rc) = (CMD);  if(RTL_RC__OK != (rc)) { return rc;      } }
#define RTL_CHECK_RC_CLEANUP(rc, CMD)       { (rc) = (CMD);  if(RTL_RC__OK != (rc)) { goto   cleanup; } }

// clbk macros
#define RTL_CLBK_RC_CNT 0
#define RTL_CLBK_RC_STP 1
#define RTL_CLBK_RC_ERR 2

///////////////////////////////////////////////////////////
// data definitions
///////////////////////////////////////////////////////////

typedef struct rtl_kv_ref {
    char *  k;
    char *  v;

} rtl_kv_ref_t;

typedef struct rtl_sm_err_tab {
    int     err_code;
    char *  err_str;

} rtl_sm_err_tab_t;

typedef struct rtl_param_tbl_check
{
    char  *mibname;
    void (*helper)(void);
    int    idx_not_null;
} rtl_param_tbl_check_t;


#define RTL_LAST_INIT {0,0, {0}, 0}

typedef struct rtl_last_script_nb {
    unsigned int  latest;
    unsigned int  oldest;
    char          oldest_name[RTL_HISTORY_SAFE_SCRIPT_PATH_LENGTH];
    unsigned int  total;
} rtl_last_script_nb_t;

///////////////////////////////////////////////////////////
// globals
///////////////////////////////////////////////////////////

const rtl_sm_err_tab_t RTL_RC_TAB[] = {  // KEEP IN SUNC WITK rtl_err_codes
    // positive
    RTL_RC_ELEM(RTL_RC__BASH_GENERAL)
	RTL_RC_ELEM(RTL_RC__BASH_MISUSE)
    RTL_RC_ELEM(RTL_RC__BASH_COMMAND_CANT_EXEC)
    RTL_RC_ELEM(RTL_RC__BASH_COMMAND_NOT_FOUND)
    RTL_RC_ELEM(RTL_RC__BASH_EXIT_INVALID)
    RTL_RC_ELEM(RTL_RC__RESTART_FORCE_ERROR)
    RTL_RC_ELEM(RTL_RC__RESTART_ALL_ERROR)
    RTL_RC_ELEM(RTL_RC__RESTART_WLAN_ERROR)
    RTL_RC_ELEM(RTL_RC__RESTART_FW_ERROR)
    RTL_RC_ELEM(RTL_RC__BASH_EXIT_OUT_OF_RANGE)

    RTL_RC_ELEM(RTL_RC__OK)
    // negative
    RTL_RC_ELEM(RTL_RC__PARAMS_NB_ERROR)
    RTL_RC_ELEM(RTL_RC__PARAMS_TRUNCATE_ERROR)
    RTL_RC_ELEM(RTL_RC__PARAMS_EMPTY_ERROR)
    RTL_RC_ELEM(RTL_RC__PARAMS_KEY_EMPTY_ERROR)
    RTL_RC_ELEM(RTL_RC__PARAMS_VALUE_EMPTY_ERROR)
    RTL_RC_ELEM(RTL_RC__PARAMS_VALUE_TOOLONG_ERROR)
    RTL_RC_ELEM(RTL_RC__PARAMS_VALUE_SPLIT_ERROR)
    RTL_RC_ELEM(RTL_RC__MIB_INIT_ERROR)
    RTL_RC_ELEM(RTL_RC__MIB_LOOKUP_ERROR)
    RTL_RC_ELEM(RTL_RC__MIB_SET_ERROR)
    RTL_RC_ELEM(RTL_RC__FLS_UPD_CS_ERROR)
    RTL_RC_ELEM(RTL_RC__FLS_UPD_DS_ERROR)
    RTL_RC_ELEM(RTL_RC__FLS_UPD_DS_ERROR_CS_FATAL)
    RTL_RC_ELEM(RTL_RC__FLS_UPD_HS_ERROR)
    RTL_RC_ELEM(RTL_RC__FLS_UPD_HS_ERROR_CS_FATAL)
    RTL_RC_ELEM(RTL_RC__FLS_UPD_HS_ERROR_DS_FATAL)
    RTL_RC_ELEM(RTL_RC__FLS_UPD_HS_ERROR_CS_DS_FATAL)
    RTL_RC_ELEM(RTL_RC__MIB_LOCK_ERROR)
    RTL_RC_ELEM(RTL_RC__MIB_UNLOCK_ERROR)
    RTL_RC_ELEM(RTL_RC__PARAMS_VALUE_TBL_ROW_OP_ERROR)
    RTL_RC_ELEM(RTL_RC__PARAMS_VALUE_TBL_MANDATORY_ERROR)
    RTL_RC_ELEM(RTL_RC__ALLOC_ERROR)
    RTL_RC_ELEM(RTL_RC__FD_OPEN_ERROR)
    RTL_RC_ELEM(RTL_RC__FD_WRITE_ERROR)
    RTL_RC_ELEM(RTL_RC__SINGLE_SCRIPT_POPEN_ERROR)
    RTL_RC_ELEM(RTL_RC__SINGLE_SCRIPT_CHILD_SIGNAL_ERROR)
    RTL_RC_ELEM(RTL_RC__SINGLE_SCRIPT_CHILD_STATUS_ERROR)
    RTL_RC_ELEM(RTL_RC__SINGLE_SCRIPT_FORK_ERROR)
    RTL_RC_ELEM(RTL_RC__SINGLE_SCRIPT_PIPE_ERROR)
    RTL_RC_ELEM(RTL_RC__DIR_MK_ERROR)
    RTL_RC_ELEM(RTL_RC__DIR_OPEN_ERROR)
    RTL_RC_ELEM(RTL_RC__FD_UNLINK_ERROR)
    RTL_RC_ELEM(RTL_RC__FD_FILENO_ERROR)
    RTL_RC_ELEM(RTL_RC__FD_DUP2_ERROR)
    RTL_RC_ELEM(RTL_RC__FD_SYMLINK_ERROR)
    RTL_RC_ELEM(RTL_RC__FD_READ_ERROR)
    RTL_RC_ELEM(RTL_RC__NO_REF_FILE_ERROR)
    RTL_RC_ELEM(RTL_RC__FD_MV_ERROR)
    RTL_RC_ELEM(RTL_RC__LIST_STA_INFO_ERROR)
    // keep last
    RTL_RC_ELEM_LAST
};

#define RTL_RC_TAB_MAX_IDX (RTL_ARRAY_ELEMS(RTL_RC_TAB)-1)


APMIB_T         Mib_RTL         = {{0}};
APMIB_T         MibDef_RTL      = {{0}};
HW_SETTING_T    HwSetting_RTL   = {0};


const rtl_param_tbl_check_t RTL_PARAM_TBL_CHECK [] = {
    {"MACAC_ADDR"       , showSetACHelp, 1},
    {"WDS"    , showSetWdsHelp, 1},

    #ifdef TLS_CLIENT
    {"CERTROOT_TBL", showSetCertRootHelp, -1},
    {"CERTUSER_TBL", showSetCertUserHelp, -1},
    #endif

    #ifdef HOME_GATEWAY
    {"PORTFW_TBL" , showSetPortFwHelp , -1 },  // TODO MODIFY
    {"IPFILTER_TBL", showSetIpFilterHelp, -1 },
    {"MACFILTER_TBL", showSetMacFilterHelp, -1},
    {"PORTFILTER_TBL", showSetPortFilterHelp, -1},
    {"TRIGGERPORT_TBL", showSetTriggerPortHelp, -1},
    #ifdef VPN_SUPPORT
    {"IPSECTUNNEL_TBL",   showSetIpsecTunnelHelp, -1},
    #endif
    #ifdef ROUTE_SUPPORT  // ??? TODO under HOME GATEWAY ???
    {"STATICROUTE_TBL", showSetStaticRouteHelp, -1},    // TODO MODIFY
    #endif
    {"URLFILTER_TBL", showSetUrlFilterHelp, -1},
    #endif

    // MIB_DHCPRSVDIP_TBL TODO
    // MIB_QOS_RULE_TBL + MODIFY TODO
    // MIB_IPV6_DHCPC_SENDOPT_TBL + ONLY MODIFY !!! TODO
    // MIB_DNS_CLIENT_SERVER_TBL + ONLY MODIFY !!! TODO
    //  MIB_WLAN_SCHEDULE_TBL + MODIFY TODO

    #if defined(CONFIG_RTK_MESH) && defined(_MESH_ACL_ENABLE_)
    {"MESH_ACL_ADDR"    , showSetACHelp, 1},
    #endif

    #if defined(VLAN_CONFIG_SUPPORTED)
    {"VLANCONFIG_TBL"    , showSetVlanConfigHelp, -1},   // TODO MODIFY
    #endif

    // MIB_CWMP_WLANCONF_TBL + MODIFY TODO
    // MIB_PROFILE_TBL1 + MODIFY TODO
    // MIB_PROFILE_TBL2 + MODIFY TODO
    // MIB_APP_LOGIN_MAC_TIME_TBL  + MODIFY TODO

    #ifdef CONFIG_RTL_MAC_BASED_HTTP_REDIRECT
    {"MAC_REDIRECT_TBL" , NULL , -1 },
    #endif

    // MIB_NASFILTER_TBL TODO
    
    {NULL, NULL, -1},
 };


///////////////////////////////////////////////////////////
// function - use static to copy into modules if necessary
///////////////////////////////////////////////////////////

static char * rtl_err_str(int err_code)
{
    int i;
    for(i=0; i < RTL_RC_TAB_MAX_IDX; i++)
    {
        if(err_code == RTL_RC_TAB[i].err_code)
            return RTL_RC_TAB[i].err_str;
    }

    return "UNKNOWN";

}

char  p_history_ref_str[RTL_HISTORY_SAFE_SCRIPT_PATH_LENGTH] = {0};
char *p_in_params_str = NULL;
FILE *fp_sm           = NULL;

#define _RTL_RC_SUMARY(CODE) (((CODE) == RTL_RC__OK) ? "SUCCESS" : "FAILED")

#define _RTL_LOG_RC(prefix, CODE, fmt, ...)                                                                                     \
do {                                                                                                                            \
    RTL_TIME_CODE                                                                                                               \
    fprintf(stdout, RTL_TIME_FORMAT "%05d|%s|| %s[%d - %s] REF[%s] CMD[%s] " fmt "\n",                                          \
                    RTL_TIME_PARAMS, __LINE__, RTL_LOG_CALLER[prefix], _RTL_RC_SUMARY(CODE), CODE, rtl_err_str((CODE)),         \
                    (p_history_ref_str[0] ? p_history_ref_str   : "ref not found"),                                                      \
                    (p_in_params_str  ? p_in_params_str : "in params not build"),                                                \
                    ##__VA_ARGS__);                                                                                             \
    /*fflush(stdout);*/                                                                                                         \
    if((RTL_LOG_CALLER__SM == (prefix)) && fp_sm )                                                                              \
    {                                                                                                                           \
        fprintf(fp_sm, RTL_TIME_FORMAT "%05d|%s|| %s[%d - %s] REF[%s] CMD[%s] " fmt "\n",                                       \
                        RTL_TIME_PARAMS, __LINE__, RTL_LOG_CALLER[prefix], _RTL_RC_SUMARY(CODE), CODE, rtl_err_str((CODE)),     \
                        (p_history_ref_str[0] ? p_history_ref_str   : "ref not found"),                                                  \
                        (p_in_params_str  ? p_in_params_str : "in params not build"),                                            \
                        ##__VA_ARGS__);                                                                                         \
    }                                                                                                                           \
    return (CODE);                                                                                                              \
} while(0);

#define RTL_LOG_RC_SM(CODE, fmt, ...)   _RTL_LOG_RC(RTL_LOG_CALLER__SM , (CODE),  fmt, ##__VA_ARGS__)
#define RTL_LOG_RC_SS(CODE, fmt, ...)   _RTL_LOG_RC(RTL_LOG_CALLER__SS , (CODE),  fmt, ##__VA_ARGS__)
#define RTL_SYSLOG_RC(CODE, fmt, ...)   _RTL_LOG_RC(RTL_LOG_CALLER__SYS, (CODE),  fmt, ##__VA_ARGS__)
#define RTL_LOG_RC_LIST(CODE, fmt, ...) _RTL_LOG_RC(RTL_LOG_CALLER__LIST, (CODE),  fmt, ##__VA_ARGS__)

// definition in apmib.h
#define RTL_LOG_DBG_SM(fmt, ...)        _RTL_LOG(RTL_LOG_CALLER__SM, fmt , ##__VA_ARGS__);
#define RTL_LOG_DBG_SS(fmt, ...)        _RTL_LOG(RTL_LOG_CALLER__SS, fmt , ##__VA_ARGS__);
#define RTL_SYSLOG_DBG(fmt, ...)        _RTL_LOG(RTL_LOG_CALLER__SYS, fmt , ##__VA_ARGS__);
#define RTL_LOG_LIST(fmt, ...)          _RTL_LOG(RTL_LOG_CALLER__LIST, fmt , ##__VA_ARGS__);
static void rtl_err_str_dump(void)
{
    int i;
    for(i=0; i < RTL_RC_TAB_MAX_IDX; i++)
    {
        RTL_LOG_DBG_SM(" RTL_RC_TAB[%d][%d <-> %s]", i, RTL_RC_TAB[i].err_code, RTL_RC_TAB[i].err_str)
	}
}
///////////////////////////////////////////////////////////////
static void rtl_safe_mode_backup(void)
{
    RTL_LOG_DBG_SM("backup: Mib[sizeof(%u<->%u)] MibDef[sizeof(%u<->%u)] HwSetting[sizeof(%u<->%u)]",
                sizeof(Mib_RTL), sizeof(*pMib),
                sizeof(MibDef_RTL), sizeof(*pMibDef),
                sizeof(HwSetting_RTL), sizeof(*pHwSetting));

    memcpy(&Mib_RTL      , pMib, sizeof(Mib_RTL));
    memcpy(&MibDef_RTL   , pMibDef , sizeof(MibDef_RTL));
    memcpy(&HwSetting_RTL, pHwSetting, sizeof(HwSetting_RTL));
}
		
static void rtl_safe_mode_restore(void)
{
	RTL_LOG_DBG_SM("restore: Mib[sizeof(%u<->%u)] MibDef[sizeof(%u<->%u)] HwSetting[sizeof(%u<->%u)]",
                sizeof(Mib_RTL), sizeof(*pMib),
                sizeof(MibDef_RTL), sizeof(*pMibDef),
                sizeof(HwSetting_RTL), sizeof(*pHwSetting));

    if(memcmp(pMib, &Mib_RTL, sizeof(Mib_RTL)))  // TODO just for DBG
    {
        RTL_LOG_DBG_SM("restore: ds revert");       // TODO just for DBG
        memcpy(pMib, &Mib_RTL, sizeof(Mib_RTL));
    }

    if(memcmp(pMibDef, &MibDef_RTL,sizeof(MibDef_RTL)))  // TODO just for DBG
    {
        RTL_LOG_DBG_SM("restore: cs revert");       // TODO just for DBG
        memcpy(pMibDef, &MibDef_RTL,sizeof(MibDef_RTL));
    }
	
	if(memcmp(pHwSetting, &HwSetting_RTL, sizeof(HwSetting_RTL)))  // TODO just for DBG
	  {
	    RTL_LOG_DBG_SM("restore: hs revert");       // TODO just for DBG
	    memcpy(pHwSetting, &HwSetting_RTL, sizeof(HwSetting_RTL));
	}
}
				
///////////////////////////////////////////////////////////////
// POKUSS
///////////////////////////////////////////////////////////////
static int rtl_eval_key_val(const char *p, int p_ord, int p_len, char *p_cp, rtl_kv_ref_t *kv_ref)
{
    char          *p_copy = p_cp;

    RTL_LOG_DBG_SM("KV eval[%03d]:  \'%s\'", p_ord, p)

    if(p_len < 1)
    {
        RTL_LOG_RC_SM(RTL_RC__PARAMS_EMPTY_ERROR, "parameter[%d] specified as \"\" ...", p_ord)
    }

    kv_ref->k = p_copy;
    while(*p_copy != '\0')
    {
        if(*p_copy == '=')
        {
            // check if not end or immediate start
            if((*(p_copy+1) == '\0') || p_copy == p_cp)
            {
                RTL_LOG_RC_SM(RTL_RC__PARAMS_KEY_EMPTY_ERROR, "missing \'MIB=VALUE\' in parameter[%d] \'%s\' ", p_ord, p)
            }

            // terminate key
            *p_copy = '\0';

            kv_ref->v = p_copy+1;

            break;
       }
        p_copy++;
    }

    if(!kv_ref->v)
    {
        RTL_LOG_RC_SM(RTL_RC__PARAMS_VALUE_EMPTY_ERROR, "missing \'=value\' in parameter[%d] \'%s\' ", p_ord, p)
    }

//    RTL_LOG_DBG_SM("k[%s] v[%s] in parameter \'%s\'", kv_ref->k, kv_ref->v, p);

    return RTL_RC__OK;

}

static int rtl_eval_tbl(const char * k, int k_ord, int valNum, char **value)
{
    int i = 0;

    RTL_LOG_DBG_SM("T  eval[%03d]: \'%s\'", k_ord, k)

    while(RTL_PARAM_TBL_CHECK[i].mibname)
    {
        RTL_LOG_DBG_SM("T  eval[%03d]: \'%s\' VERSUS \'%s\'", k_ord, k, RTL_PARAM_TBL_CHECK[i].mibname)

        if(strstr(k, RTL_PARAM_TBL_CHECK[i].mibname))
        {
            // common
            if (!valNum || (strcmp(value[0], "add") && strcmp(value[0], "del") && strcmp(value[0], "delall")))
            {
                if(RTL_PARAM_TBL_CHECK[i].helper)
                    RTL_PARAM_TBL_CHECK[i].helper();

                RTL_LOG_RC_SM(RTL_RC__PARAMS_VALUE_TBL_ROW_OP_ERROR, "T  eval[%03d]: \'%s\' missing row operator", k_ord, k)
            }
            // specific
            if(-1 != RTL_PARAM_TBL_CHECK[i].idx_not_null)
            {
                if ( (!strcmp(value[0], "del") && !value[1]) || (!strcmp(value[0], "add") && !value[1]) )
               {
                    if(RTL_PARAM_TBL_CHECK[i].helper)
                        RTL_PARAM_TBL_CHECK[i].helper();

                    RTL_LOG_RC_SM(RTL_RC__PARAMS_VALUE_TBL_MANDATORY_ERROR, "T  eval[%03d]: \'%s\' missing next operand", k_ord, k)
                }
            }
        }

        i++;
    }

    // all OK or not defined ...
    return RTL_RC__OK;

}


static int rtl_eval_val(const char *v, int v_ord, int v_len, char valueRef[RTL_CMD__LINE_VALUE_PARAM_MAX][RTL_CMD__LINE_ELEM_LEN_MAX], int *valueNb)
{
    char    *p;
    char     tmp[RTL_CMD__LINE_ELEM_LEN_MAX];

    RTL_LOG_DBG_SM("V  eval[%03d]: \'%s\'", v_ord, v)

   if(v_len < 1 || v_len > sizeof(tmp))
    {
        RTL_LOG_RC_SM(v_len < 1 ? RTL_RC__PARAMS_VALUE_EMPTY_ERROR : RTL_RC__PARAMS_VALUE_TOOLONG_ERROR,
                   "value[%d] specified as %s ...", v_ord,  ((v_len < 1) ? "\"\"" : "too long"));
    }

    strcpy(tmp, v);

    *valueNb = 0;

   char sep[2] = "^";

    p = strtok(tmp, sep);
    while(p)
    {
        if(*valueNb >= RTL_CMD__LINE_VALUE_PARAM_MAX)
        {
            RTL_LOG_RC_SM(RTL_RC__PARAMS_VALUE_SPLIT_ERROR, "value expect maximum %d splited values  ...", *valueNb)
        }

        strcpy(&valueRef[*valueNb][0], p);

        *valueNb = *valueNb+1;

        p = strtok(NULL, sep);
    }
	
    return RTL_RC__OK;

}

static int rtl_safe_mode(char paramArray[RTL_CMD__LINE_ELEM_MAX][RTL_CMD__LINE_ELEM_LEN_MAX], int paramNum) // safe-mode
{
    int  i,j;
    int  rc = 0;
    int  idx, hws=0, dss=0, css=0;
    int  css_restore_fail = 0, dss_restore_fail = 0;


    mib_table_entry_T *pTbl=NULL;

    rtl_kv_ref_t  paramCopyRef[paramNum];
    char          paramCopy[paramNum][RTL_CMD__LINE_ELEM_LEN_MAX];
    int           valueNb;
    char          valueStore[RTL_CMD__LINE_VALUE_PARAM_MAX][RTL_CMD__LINE_ELEM_LEN_MAX];
    char         *valueRef[RTL_CMD__LINE_VALUE_PARAM_MAX];


    memset(paramCopyRef, 0, paramNum * sizeof(paramCopyRef[0]));
    memcpy(paramCopy, paramArray, paramNum*RTL_CMD__LINE_ELEM_LEN_MAX);


    RTL_LOG_DBG_SM(" ______________SAFE__MODE(%dmib params)______________", paramNum);

    //  ***  PARSE KEY<-> VALUE***
    for(i=0;i<paramNum;i++)
    {
        rc = rtl_eval_key_val(&paramArray[i][0], i+3 /*startFrom1 + flash + safe-mode*/, strlen(&paramArray[i][0]), &paramCopy[i][0], &paramCopyRef[i]);
        if(rc != RTL_RC__OK)
        {
            return rc;
        }
    }

    if ( !apmib_init())
    {
        RTL_LOG_RC_SM(RTL_RC__MIB_INIT_ERROR, "Initialize AP MIB failed! ")
    }

    rtl_safe_mode_backup();

    for(i=0;i<paramNum;i++)
    {
       RTL_LOG_DBG_SM("")
        RTL_LOG_DBG_SM("KV     [%03d]: k[%s] v[%s]", i+3 /*startFrom1 + flash + safe-mode*/, paramCopyRef[i].k, paramCopyRef[i].v)

        wlan_idx    = 0;
        vwlan_idx   = 0;
        config_area = 0;

        //  *** LOOKUP ***
        idx = searchMIB(paramCopyRef[i].k);
        if(idx == -1)
        {
            rtl_safe_mode_restore();
           RTL_LOG_RC_SM(RTL_RC__MIB_LOOKUP_ERROR, "K  mib [%03d]: k[%s] v[%s] lookup failed! ...", i+3 /*startFrom1 + flash + safe-mode*/, paramCopyRef[i].k, paramCopyRef[i].v)
        }

//        if (config_area == HW_MIB_AREA)
//            pTbl = hwmib_table;
//        else if (config_area == HW_MIB_WLAN_AREA)
//            pTbl = hwmib_wlan_table;
//        else if (config_area ==DEF_MIB_AREA  || config_area == MIB_AREA)
//            pTbl = mib_table;
//        else
//            pTbl = mib_wlan_table;
        // ---------------------------------------------------------------
        // ---------------------------------------------------------------

        if (config_area == HW_MIB_AREA || config_area == HW_MIB_WLAN_AREA)
        {
            hws = 1;

            if (config_area == HW_MIB_AREA)
                pTbl = hwmib_table;
            else
                pTbl = hwmib_wlan_table;
        }
        else if (config_area == DEF_MIB_AREA || config_area == DEF_MIB_WLAN_AREA)
        {
            dss = 1;

            if (config_area == DEF_MIB_AREA)
                pTbl = mib_table;
            else
                pTbl = mib_wlan_table;
        }
       else
        {
            css = 1;

           if (config_area == MIB_AREA)
                pTbl = mib_table;
            else
                pTbl = mib_wlan_table;
        }

        RTL_LOG_DBG_SM("K  mib [%03d]: k[%s] lookup OK -> wlan_idx[%d] vwlan_idx[%d] config_area[%d -- hs(1) hsw(2) ds(3) dsw(4) cs(5) csw(6)]",
                    i+3 /*startFrom1 + flash + safe-mode*/, paramCopyRef[i].k, wlan_idx, vwlan_idx, config_area)

//        if(strstr(token, "WLAN0"))
//            wlan_idx = 0;
//        else if(strstr(token, "WLAN1"))
//            wlan_idx = 1;
//        else+//            wlan_idx = 0;
//
//        if(strstr(token, "VAP"))
//        {
//            vwlan_idx = atoi(&token[9])+1;
//        }
//        else if(strstr(token, "VXD"))
//        {
//            vwlan_idx = 5;
//        }
//        else
//            vwlan_idx = 0;

        // *** PARSE VALUE ***
        valueNb = 0;
        memset(valueStore, 0, sizeof(valueStore));
        memset(valueRef  , 0, sizeof(valueStore));
        rc = rtl_eval_val(paramCopyRef[i].v, i+3 /*startFrom1 + flash + safe-mode*/, strlen(paramCopyRef[i].v), valueStore, &valueNb);
        if(rc != RTL_RC__OK)
        {
            rtl_safe_mode_restore();
            return rc;
        }

        for(j=0; j<valueNb; j++)
        {
            valueRef[j] = &valueStore[j][0];
            RTL_LOG_DBG_SM("V  args[%03d]: [%d] [%s]", i+3 /*startFrom1 + flash + safe-mode*/, j, valueRef[j])
        }

        // *** TABLE ***
        if(0 && pTbl[idx].type >= TABLE_LIST_T)
        {
            rc = rtl_eval_tbl(paramCopyRef[i].k, i+3 /*startFrom1 + flash + safe-mode*/, valueNb, valueRef);
            if(rc != RTL_RC__OK)
            {
                rtl_safe_mode_restore();
                return rc;
           }
        }

        // *** SET MIB ***
        rc = setMIB(paramCopyRef[i].k, pTbl[idx].id, pTbl[idx].type, pTbl[idx].size, valueNb, valueRef, 0 /*flash_flag*/);
        if(rc != RTL_RC__OK)
        {
            rtl_safe_mode_restore();
            return rc;
        }
    }

    if(css)
    {
        if(!apmib_update(CURRENT_SETTING))
        {
            rtl_safe_mode_restore();
            RTL_LOG_RC_SM(RTL_RC__FLS_UPD_CS_ERROR, "flash update failed!")
        }
    }
    if(dss)
    {
        if(!apmib_update(DEFAULT_SETTING))
        {
            rtl_safe_mode_restore();
            if((css) &&!apmib_update(CURRENT_SETTING))
            {
                   // what to do ???
                    css_restore_fail = 1;
            }
            RTL_LOG_RC_SM((css_restore_fail == 1) ? RTL_RC__FLS_UPD_DS_ERROR_CS_FATAL : RTL_RC__FLS_UPD_DS_ERROR, "flash update failed! ")
        }
    }
    if(hws)
    {
        if(!apmib_update(HW_SETTING))
        {
            rtl_safe_mode_restore();

            if((css) && !apmib_update(CURRENT_SETTING))
            {
            // what to do ???
                css_restore_fail = 1;
            }

            if((dss) && !apmib_update(DEFAULT_SETTING))
            {
                // what to do ???
                dss_restore_fail = 1;
            }

            if(css_restore_fail == 1 && dss_restore_fail == 1)
                RTL_LOG_RC_SM(RTL_RC__FLS_UPD_HS_ERROR_CS_DS_FATAL, "flash update failed!")
            else if(css_restore_fail == 1)
                RTL_LOG_RC_SM(RTL_RC__FLS_UPD_HS_ERROR_CS_FATAL, "flash update failed!")
            else if(dss_restore_fail == 1)
                RTL_LOG_RC_SM(RTL_RC__FLS_UPD_HS_ERROR_DS_FATAL, "flash update failed!")
           else
                RTL_LOG_RC_SM(RTL_RC__FLS_UPD_HS_ERROR, "flash update failed!")
        }
    }

    RTL_LOG_DBG_SM("")
    RTL_LOG_RC_SM(RTL_RC__OK, "finish ...")
}


char *script_0K ="#!/bin/sh      \n"
                 "echo SCRIPT_0K \n"
                 "echo \"SHELL VARIABLE==>> $SHELL\"\n"
                 "echosa $MISULAS\n"
                 "               \n";

char *script_N0K ="#!/bin/sh      \n"
                "echo SCRIPT_N0K \n"
                "echo \"SHELL VARIABLE==>> $SHELL\"\n"
				"                \n";


char *script_1_NOK  = "#!/bin/sh                        \n"
                      "SM='WLAN5_SSID=TESTMISIK__1111'  \n"
                      "echo $SM                         \n"
                      "echo \"SHELL VARIABLE==>> $SHELL\"\n"
                      "                                 \n";

char *script_1_OK  = "#!/bin/sh                         \n"
                     "SM='WLAN0_SSID=TESTMISIK__1111'   \n"
                     "echo $SM                          \n"
                     "echo \"SHELL VARIABLE==>> $SHELL\"\n"
                     "                                  \n";

char *script_2_OK  = "#!/bin/sh                                          \n"
                     "SM=\"$SM 'VLANCONFIG_TBL=add 1 miso 1 2 3 4 5'\"   \n"
                     "echo $SM                                           \n"
                     "echo \"SHELL VARIABLE==>> $SHELL\"                 \n"
                     "                                                   \n";

#define RTL_SCRIPT_INT_DELIMITER "\necho ---------------------------------------------------------------------------\n" \
                                "echo ---------------------------------------------------------------------------\n" \
                                 "echo ---------------------------------------------------------------------------\n" \

char *script_start =  "set -e                                                                     \n";

char *script_end =  "RTL_CONSOLE=\'shell safe-mode   [-----] >>\'                               \n"
                    "echo \"$RTL_CONSOLE VARIABLE SM     ==>> \'$SM\'\"                         \n"
                    "echo \"$RTL_CONSOLE VARIABLE SM_WL  ==>> \'$SM_WL\'\"                      \n"
                    "echo \"$RTL_CONSOLE VARIABLE SM_FW  ==>> \'$SM_FW\'\"                      \n"
                    "echo \"$RTL_CONSOLE VARIABLE SM_FW  ==>> \'$SM_FW\'\"                      \n"
                    "                                                                           \n"
                    "                                                                           \n"
                    "EXEC_CMD()                                                                 \n"
                    "{                                                                          \n"
                    "    CMD=$1                                                                 \n"
                    "    echo \"$RTL_CONSOLE exec PROCESSING '$CMD' ...\"                       \n"
                    "    eval $CMD                                                              \n"
                    "    rtl_return=$?                                                          \n"
                    "                                                                           \n"
                    "    if [ $rtl_return -ne 0 ]                                               \n"
                    "    then                                                                   \n"
                    "       echo \"$RTL_CONSOLE exec FAILED[shell($rtl_return) rtl($2)] for CMD=\'$CMD\'\" \n"
                    "                                                                           \n"

					"       if [ \"$2\" == \"\" ]                                               \n"

					"       then                                                                \n"
                    "           exit $rtl_return                                                \n"
                    "       else                                                                \n"
                    "           exit $2                                                         \n"
                    "       fi                                                                  \n"
                    "                                                                           \n"
                    "    else                                                                   \n"
                    "       echo \"$RTL_CONSOLE exec SUCCESS[$rtl_return] for CMD=\'$CMD\'\"    \n"
                    "    fi                                                                     \n"
                    "}                                                                          \n"
                    "                                                                           \n"
                    "                                                                           \n"
                    "if [ \"$SM\" != \"\" ]                                                     \n"
                    "then                                                                       \n"
                    "                                                                           \n"
                    "    echo \"$RTL_CONSOLE SAFE-MODE processing ...\"                         \n"
                    "                                                                           \n"
#ifdef TMP_TEST
                    "    EXEC_CMD \"/tmp/flash safe-mode $SM\" \"\"                            \n" // TEST PURPOSE
#else
                    "    EXEC_CMD \"flash safe-mode $SM\" \"\"                                 \n"
#endif
                    "                                                                           \n"
//                   		 "    if [ \"$SM_SKIP_RESTART\" != \"\" ]                                    \n"
//                    		 "    then                                                                   \n"
//                   		 "                                                                           \n"
//                    		 "           echo \"$RTL_CONSOLE *************************************************************************************\"  \n"
//                    		 "           echo \"$RTL_CONSOLE SM_SKIP_RESTART !!! *** WARNING *** !!! safe mode without reboot was invoked by script ... \"  \n"
//                   		 "           echo \"$RTL_CONSOLE *************************************************************************************\"  \n"
//                    		 "                                                                           \n"
//                    		 "    elif [ \"$SM_ALL\" != \"\" ]                                           \n"
//                   		 "    then                                                                   \n"
//                    		"                                                                           \n"
//                    		"           echo \"$RTL_CONSOLE SM_ALL so exec 'sysconf init gw all'... \"  \n"
//                    		"           EXEC_CMD \"sysconf init gw all\" " RTL_STR(RTL_RC__RESTART_ALL_ERROR) "\n"
//                    		"                                                                           \n"
//                   		 "    
//					"	else                                                                   \n"
//                    		"                                                                           \n"
//                    		"           FORCE_ALL=\"1\"                                                 \n"
//                    		"                                                                           \n"
//                   		 "           if [ \"$SM_WL\" != \"\" ]                                       \n"
//                    		"           then                                                            \n"
//                   		 "                                                                           \n"
//                    		"               FORCE_ALL=\"0\"                                             \n"
//                   		 "               echo \"$RTL_CONSOLE SM_WL was set - exec 'sysconf wlanapp init'...\"\n"
//                   		 "               EXEC_CMD \"sysconf wlanapp init\" " RTL_STR(RTL_RC__RESTART_WLAN_ERROR) "\n"
//                   		 "                                                                           \n"
//                   		 "           fi                                                              \n"
//                   		 "                                                                           \n"
//                   		 "           if [ \"$SM_FW\" != \"\" ]                                       \n"
//                    		"           then                                                            \n"
//                    		"                                                                           \n"
//                    		"               FORCE_ALL=\"0\"                                             \n"
//                    		"               echo \"$RTL_CONSOLE SM_FW was set - exec 'sysconf firewall init'...\"\n"
//                    		"               EXEC_CMD \"sysconf firewall init\" " RTL_STR(RTL_RC__RESTART_FW_ERROR) "\n"
//                    		"                                                                           \n"
//                   		 "           fi                                                              \n"
//                   		 "                                                                           \n"
//                   		 "           if [ \"$FORCE_ALL\" == \"1\" ]                                  \n"
//                   		 "           then                                                            \n"
//                    		"                                                                           \n"
//                   		 "               echo \"$RTL_CONSOLE FORCE_ALL so exec 'sysconf init gw all'... \"  \n"
//                   		 "               EXEC_CMD \"sysconf init gw all\" " RTL_STR(RTL_RC__RESTART_FORCE_ERROR) "\n"
//                   		 "                                                                           \n"
//                   		 "           fi                                                              \n"
//                   		 "                                                                           \n"
//                   		 "    fi                                                                     \n"
                    "                                                                           \n"
                    "else                                                                       \n"
                    "                                                                           \n"
                    "    echo \"$RTL_CONSOLE SAFE-MODE skipped ...\"                            \n"
                    "                                                                           \n"
                    "fi                                                                         \n"
                    "                                                                           \n"
                    "echo \"$RTL_CONSOLE ******************** ALL LINES PROCESSED **************************** \"\n";

static void rtl_print_with_borders(const char *b_name, const char *print_string, int seq_nb)
{
    RTL_LOG_DBG_SS(" ************************************************************");
    RTL_LOG_DBG_SS(" **** [sn %d] %s - start", seq_nb, b_name);
    RTL_LOG_DBG_SS(" ************************************************************");
    RTL_LOG_DBG_SS("\n%s", print_string);
    RTL_LOG_DBG_SS(" ************************************************************");
    RTL_LOG_DBG_SS(" **** [sn %d] %s - end", seq_nb, b_name);
    RTL_LOG_DBG_SS(" ************************************************************");
}


static int rtl_fs__bash_rc_correction(int bash_rc)
{

    bash_rc  = WEXITSTATUS(bash_rc);

    if((RTL_RC__OK                     == bash_rc) ||
       (RTL_RC__BASH_GENERAL            == bash_rc) ||
       (RTL_RC__BASH_MISUSE             == bash_rc) ||
       (RTL_RC__BASH_COMMAND_CANT_EXEC  == bash_rc) ||
       (RTL_RC__BASH_COMMAND_NOT_FOUND  == bash_rc) ||
       (RTL_RC__BASH_EXIT_INVALID       == bash_rc) ||
       (RTL_RC__BASH_EXIT_OUT_OF_RANGE  == bash_rc) ||
       // RTL_RC__BOUNDARY section
       (RTL_RC__RESTART_ALL_ERROR       == bash_rc) ||
       (RTL_RC__RESTART_WLAN_ERROR      == bash_rc) ||
       (RTL_RC__RESTART_FW_ERROR        == bash_rc))
    {
        // BASH specific codes
        return bash_rc;
    }
    else
    {
        // RF specific codes
        return (bash_rc - 256);
    }
}

static int rtl_fs__exec_common(int fd , char *script_buf, pid_t pid)
{
    errno = 0;
    if(!(fd < 0))
    {
        char    buf[RTL_OUTPUT_BUFFER];
        int     i = 0, rc, rc_shell, dump_seq_nb = 0;
        int     exit_pid, exit_done = 0;
        ssize_t r;
        int     nb_wr; // TODO remove

        #define CHECK_SIZE_AND_DUMP  if(!(i < (sizeof(buf) -1 )))  \
        {                                                          \
			buf[i] = 0;                                            \
            rtl_print_with_borders("OUTPUT", buf, dump_seq_nb++);  \
                                                                   \
			/* reset */                                            \
            i = 0;                                                 \
            memset(buf,0,sizeof(buf));                             \
        }                                                          \



        fcntl(fd, F_SETFL, O_NONBLOCK);

        memset(buf,0,sizeof(buf));

        while (1)
        {
            errno = 0;
            exit_pid = waitpid(pid, &rc, 0);
            if(exit_pid == pid)
            {
                exit_done = 1;
            }

            // TODO remove - start
            nb_wr = snprintf(&buf[i], RTL_OUTPUT_BUFFER - i, "\n>>>DBG>>>-------waitpid - exit_pid[%d]----------\n", exit_pid);
            i += nb_wr;
            CHECK_SIZE_AND_DUMP
            // TODO remove - end

            errno = 0;
            r = read(fd, &buf[i], RTL_OUTPUT_BUFFER - i);
            if (r == -1 && errno == EAGAIN)
			{
                // TODO remove - start
                nb_wr = snprintf(&buf[i], RTL_OUTPUT_BUFFER - i, "\n>>>DBG>>>-------AGAIN exit_done[%d]----------\n", exit_done);
                i += nb_wr;
                CHECK_SIZE_AND_DUMP
                // TODO remove - end

                errno = 0;

                if(exit_done)
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
            else if (r > 0)
            {
                // USEFULL FOR DEBUG - only if read size is 1
//                 	if(buf[i] == '\n')
//                    	 	printf("\n---***---\n");
//                  	else
//                    		printf("%c", buf[i]);

                i += r;

                CHECK_SIZE_AND_DUMP

            }
            else
            {
                // TODO remove - start
                nb_wr = snprintf(&buf[i], RTL_OUTPUT_BUFFER - i, "\n>>>DBG>>>------- BREAK r[%d]----------\n", r);
                i += nb_wr;
                CHECK_SIZE_AND_DUMP
                // TODO remove - end

                break;
            }

        }

        buf[i] = 0;

        rtl_print_with_borders("OUTPUT", buf, dump_seq_nb);

        if(WIFEXITED(rc))
        {
            rc_shell = rtl_fs__bash_rc_correction(rc);
            RTL_LOG_RC_SS( rc_shell, "**** CHILD RESULT(rc[%d] WEXITSTATUS[%d] errno[%d])", rc, WEXITSTATUS(rc), errno)
        }
        else if (WIFSIGNALED(rc))
        {
            RTL_LOG_RC_SS(RTL_RC__SINGLE_SCRIPT_CHILD_SIGNAL_ERROR, "**** CHILD RESULT(rc[%d] WTERMSIG[%d] errno[%d])", rc , WTERMSIG(rc), errno)
        }
        else
        {
            RTL_LOG_RC_SS(RTL_RC__SINGLE_SCRIPT_CHILD_SIGNAL_ERROR, "**** CHILD RESULT(rc[%d] ERRROR ??? errno[%d])", rc , errno)
        }
    }
    else
    {
        RTL_LOG_RC_SS(RTL_RC__SINGLE_SCRIPT_POPEN_ERROR, "popen failed errno[%d - %s]", errno, strerror(errno))
    }
}

static int rtl_fs__exec_pipe(char *script_buf)
{
    int ofd[2];
    pid_t pid;
    int   rc;

    rtl_print_with_borders("FINAL SCRIPT", script_buf, 0);

    if(pipe(ofd))
    {
        RTL_LOG_RC_SS(RTL_RC__SINGLE_SCRIPT_PIPE_ERROR,"child creation failed for %s", script_buf)
    }

    switch (pid = fork())
    {
       case -1:    /* error */
       {
           RTL_LOG_RC_SS(RTL_RC__SINGLE_SCRIPT_FORK_ERROR,"child creation failed for %s", script_buf)
       }
       case 0:    /* child */
       {
           close(ofd[0]);
           RTL_CHECK_RC(rc, rtl_fs__file_dup2(ofd[1], "pipe write STDOUT", STDOUT_FILENO))
           RTL_CHECK_RC(rc, rtl_fs__file_dup2(ofd[1], "pipe write STDERR", STDERR_FILENO))
           close(ofd[1]);

           char *argv[]={ "/bin/ash", "-c", script_buf, 0};

           RTL_CHECK_RC(rc, rtl_fs__exec(argv))
       }
       default:    /* parent */
       {
           close(ofd[1]);

           rc = rtl_fs__exec_common(ofd[0], script_buf, pid);

           close(ofd[0]);

           return rc;
       }
   }
}

static int rtl_fs__file_exist(char *file_name)
{
    struct stat status;

    if ( stat(file_name, &status) < 0)
    {
        return 0;
    }

    return 1;
}

static int rtl_fs__file_open_fp(char * f_name, char * flags, FILE ** fp)
{
    errno = 0;
    *fp = fopen(f_name, flags);
    if(*fp == NULL)
    {
       RTL_SYSLOG_RC(RTL_RC__FD_OPEN_ERROR, "file '%s' (flags '%s') open failed - errno[%d - %s]",
                       f_name, flags, errno, strerror(errno))
    }

    return RTL_RC__OK;
}

static int rtl_fs__file_write_raw(char * f_name, char * flags, char * buf, size_t buf_len, int fd)
{
    errno = 0;
    if(buf_len != write(fd, buf, buf_len))
    {
        RTL_SYSLOG_RC(RTL_RC__FD_WRITE_ERROR, "file '%s' (flags '%s') write failed - errno[%d - %s]",
                        f_name, flags, errno, strerror(errno))
    }

    return RTL_RC__OK;

}

static int rtl_fs__file_write_from_buf(char * f_name, char * flags, char * buf, size_t buf_len)
{
    int rc;
    int fd;
    FILE *fp = NULL;

    RTL_CHECK_RC(rc, rtl_fs__file_open_fp(f_name, flags, &fp))

    RTL_CHECK_RC_CLEANUP(rc, rtl_fs__file_fp2fd(fp, f_name, &fd))

    RTL_CHECK_RC_CLEANUP(rc, rtl_fs__file_write_raw(f_name, flags, buf, buf_len, fd))

    // rc = RTL_RC__OK; last call must be 0  + clenup fd

cleanup:
    if(fp)
    {
        fclose(fp);
    }

    return rc;
}


static int rtl_fs__file_read_raw(char * f_name, int fd, char ** buf, size_t buf_len)
{
    size_t      fsize;

    RTL_CHECK_CALLOC_SYS(*buf , buf_len)

    errno = 0;
    fsize = read(fd, *buf, buf_len);
    if(buf_len != fsize)
    {
        RTL_SYSLOG_RC(RTL_RC__FD_READ_ERROR, "file '%s' read failed - errno[%d - %s]",
                        f_name, errno, strerror(errno))
    }

    return RTL_RC__OK;
}

static int rtl_fs__file_read_to_buf(char * f_name, char ** buf, size_t *buf_len)
{
    int         rc;
    int         fd;
    FILE       *fp = NULL;

    *buf     = NULL;
    *buf_len = 0;

    RTL_CHECK_RC(rc, rtl_fs__file_open_fp(f_name, "r", &fp))

    RTL_CHECK_RC_CLEANUP(rc, rtl_fs__file_fp2fd(fp, f_name, &fd))

    fseek(fp , 0 , SEEK_END);
    *buf_len = ftell(fp);
    rewind(fp);

    RTL_CHECK_RC_CLEANUP(rc, rtl_fs__file_read_raw(f_name, fd, buf, *buf_len))

    fclose(fp);

    return RTL_RC__OK;

cleanup:
    if(fp)
    {
        fclose(fp);
     }

    RTL_FREE(*buf)

    return rc;
}

static int rtl_fs__file_fp2fd(FILE *fp, char * f_name, int *fd)
{
    errno = 0;
    if((*fd = fileno(fp)) < 0)
    {

        RTL_SYSLOG_RC(RTL_RC__FD_FILENO_ERROR, "file '%s' fileno failed - errno[%d - %s]", f_name, errno, strerror(errno))
    }
    return RTL_RC__OK;
}

static int rtl_fs__file_dup2(int old, char * f_name, int new)
{
    errno = 0;

    if(dup2(old, new) == -1)
    {
        RTL_SYSLOG_RC(RTL_RC__FD_DUP2_ERROR, "file '%s' dup2 failed (old[%d] new[%d]) - errno[%d - %s]", f_name, old, new, errno, strerror(errno))
    }
    return RTL_RC__OK;
}

static int rtl_fs__file_symlink(char * f_name, char * f_link_name)
{
    errno = 0;
    if(symlink(f_name, f_link_name) == -1)
    {
       RTL_SYSLOG_RC(RTL_RC__FD_SYMLINK_ERROR, "file '%s' -> symlink '%s' failed - errno[%d - %s]",
                       f_name, f_link_name, errno, strerror(errno))
    }

    return RTL_RC__OK;
}

static int rtl_fs__file_unlink(char * f_name)
{
    errno = 0;
    if(unlink(f_name) == -1)
    {
       RTL_SYSLOG_RC(RTL_RC__FD_UNLINK_ERROR, "file '%s' unlink failed - errno[%d - %s]",
                       f_name, errno, strerror(errno))
    }

    return RTL_RC__OK;
}

static int rtl_fs__file_unlink_S_(char * f_name)
{
    if(rtl_fs__file_exist(f_name))
    {
        return rtl_fs__file_unlink(f_name);
    }

    return RTL_RC__OK;
}

static int rtl_fs__file_mv(char * f_name_old, char * f_name_new)
{
    errno = 0;
    if(rename(f_name_old, f_name_new) == -1)
    {
       RTL_SYSLOG_RC(RTL_RC__FD_MV_ERROR, "f_name_old '%s' mv failed f_name_new '%s' failed - errno[%d - %s]",
               f_name_old, f_name_new, errno, strerror(errno))
    }

    return RTL_RC__OK;
}


static int rtl_fs__exec(char **argv)
{
    errno = 0;
    if(execv(argv[0], argv) == -1)
    {
       RTL_SYSLOG_RC(RTL_RC__FD_OPEN_ERROR, "exec '%s' failed - errno[%d - %s]", argv[0], errno, strerror(errno))
    }

    // newer reached
    return RTL_RC__OK;
}

static int rtl_fs__dir_create(char *dir_name)
{
    int rc;
    rc = mkdir(dir_name, (S_IRWXU | S_IRWXG | S_IRWXO));
    if(rc)
       RTL_SYSLOG_RC(RTL_RC__DIR_MK_ERROR, "mkdir %s failed - errno[%d - %s]", dir_name, errno, strerror(errno))

    return RTL_RC__OK;
}

static int rtl_fs__dir_create_S_(char *dir_name)
{
    if(!rtl_fs__file_exist(dir_name))
    {
        return rtl_fs__dir_create(dir_name);
    }

    return RTL_RC__OK;
}

static int rtl_fs__dir_CLBK_handle_latest(char *dir_name, char *d_name, void *ret_val)
{
    rtl_last_script_nb_t *val = (rtl_last_script_nb_t *) ret_val;
    unsigned int current;

    #define  RTL_STORE_OLDEST  snprintf(val->oldest_name, sizeof(val->oldest_name), "%s%s", dir_name, d_name);

#if 0
    RTL_SYSLOG_DBG("opendir scan '%s' ", d_name)
#endif

    if(!strncmp(RTL_LAST_PREFIX, d_name, sizeof(RTL_LAST_PREFIX)-1))
    {
        sscanf(d_name, RTL_LAST_PREFIX_SN, &current);

        if(!val->oldest && !val->latest)
        {
            val->oldest = current;
            val->latest = current;

            RTL_STORE_OLDEST
        }
        else
        {
            if(val->oldest  > current)
            {
                val->oldest = current;
                RTL_STORE_OLDEST
            }

            if(val->latest  < current)
                val->latest = current;
        }

        val->total++;
    }

    #undef RTL_STORE_OLDEST

    return RTL_CLBK_RC_CNT;
}

#if 0
static int rtl_fs__dir_CLBK_dbg_list(char *dir_name, char *d_name, void *ret_val)
{
    RTL_SYSLOG_DBG("opendir scan '%s' ", d_name)
    return RTL_CLBK_RC_CNT;
}
#endif

static int rtl_fs__dir_scan(char *dir_name, void* ret_val, int (f)(char *, char *, void *))
{
    DIR           *dir;
    struct dirent *next;
    int            rc;

    RTL_CHECK_RC(rc, rtl_fs__dir_create_S_(dir_name))

    dir = opendir(dir_name);
    if(!dir)
    {
        RTL_SYSLOG_RC(RTL_RC__DIR_MK_ERROR, "opendir %s failed - errno[%d - %s]", dir_name, errno, strerror(errno))
    }

    while ((next = readdir(dir)) != NULL)
    {
        if(!(RTL_CLBK_RC_CNT == f(dir_name, next->d_name, ret_val)))
        {
            break;
        }
    }

    closedir(dir);
	
    return RTL_RC__OK;
}

static int rtl_safe_common_final(void)
{
    int      rc;

    // script history - reference to latest
    RTL_CHECK_RC(rc, rtl_fs__file_unlink_S_(SAFE_SCRIPT_REF_FILE))

    return RTL_RC__OK;
}

static int rtl_safe_redir_to_stdout(void)
{
    int rc;

    rc = rtl_fs__file_exist(SAFE_COMMON_REDIR_TO_STDOUT);

    RTL_SYSLOG_DBG("redir to stdout %s (%s %s)",
                    rc ? "Yes": "No",
                    SAFE_COMMON_REDIR_TO_STDOUT,
                    rc ? "found": "NOT found")

    return rc;
}

static int rtl_safe_script_init(void)
{
    int      rc;
    int      fd;
    FILE    *fp = NULL;
    char     f_name[RTL_HISTORY_SAFE_SCRIPT_PATH_LENGTH] = {0};

    rtl_last_script_nb_t ret_val = RTL_LAST_INIT;
    unsigned int    new = 1;

    if(rtl_safe_redir_to_stdout())
    {
        return RTL_RC__OK;
    }

#if 0
    // script history - TODO debug -> dump content of directory
    RTL_SYSLOG_DBG("---------------------------------------------------------")
    RTL_CHECK_RC_CLEANUP(rc, rtl_fs__dir_scan(RTL_DIR, (void *)&ret_val, rtl_fs__dir_CLBK_dbg_list))
    RTL_SYSLOG_DBG("---------------------------------------------------------")
#endif

    // solve oldest/latest/new
    RTL_CHECK_RC(rc, rtl_fs__dir_scan(RTL_DIR, (void *)&ret_val, rtl_fs__dir_CLBK_handle_latest))
    new += ret_val.latest;

#if 0
    RTL_SYSLOG_DBG("opendir scan counter(latest[%u] oldest[%u][%s] total[%u] -> NEW[%u])", ret_val.latest, ret_val.oldest, ret_val.oldest_name, ret_val.total, new)
#endif

    // script history - create latest
    RTL_TIME_CODE
    snprintf(f_name, sizeof(f_name), SAFE_SCRIPT_LAST, new, RTL_TIME_PARAMS);

    RTL_CHECK_RC(rc, rtl_fs__file_open_fp(f_name, "w", &fp))

    RTL_CHECK_RC_CLEANUP(rc, rtl_fs__file_fp2fd(fp, f_name, &fd))

    // script history - redirect + keep reference string to latest for safe-mode
    RTL_CHECK_RC_CLEANUP(rc, rtl_fs__file_dup2(fd, f_name, STDOUT_FILENO))

    snprintf(p_history_ref_str, sizeof(p_history_ref_str), "%s", f_name);


    // script history - link to latest
    RTL_CHECK_RC_CLEANUP(rc, rtl_fs__file_unlink_S_(SAFE_SCRIPT_LAST_SYMLINK))

    RTL_CHECK_RC_CLEANUP(rc, rtl_fs__file_symlink(f_name, SAFE_SCRIPT_LAST_SYMLINK))

    // script history - remove oldest
    if((ret_val.total >= RTL_HISTORY_SAFE_SCRIPT_LAST_MAX) &&
       (strlen(ret_val.oldest_name) > 0))
    {
        RTL_CHECK_RC_CLEANUP(rc, rtl_fs__file_unlink(ret_val.oldest_name))
    }

    // script history - reference to latest
    RTL_CHECK_RC_CLEANUP(rc, rtl_fs__file_unlink_S_(SAFE_SCRIPT_REF_FILE))

    RTL_CHECK_RC_CLEANUP(rc, rtl_fs__file_write_from_buf(SAFE_SCRIPT_REF_FILE, "w", f_name, strlen(f_name)))

#if 0
    // script history - TODO debug -> dump content of directory
    RTL_SYSLOG_DBG("---------------------------------------------------------")
    RTL_CHECK_RC_CLEANUP(rc, rtl_fs__dir_scan(RTL_DIR, (void *)&ret_val, rtl_fs__dir_CLBK_dbg_list))
    RTL_SYSLOG_DBG("---------------------------------------------------------")
#endif

    return RTL_RC__OK;

cleanup:
    if(fp)
    {
        fclose(fp);
    }

    return rc;
}

static int rtl_safe_mode_init(void)
{
    int              rc, i;
    unsigned int     nl_cnt = 0;
    char            *buf    = NULL;
    size_t           buf_len;

    if(rtl_safe_redir_to_stdout())
    {
        return RTL_RC__OK;
    }

    RTL_CHECK_RC(rc, rtl_fs__dir_create_S_(RTL_DIR))


    // safe-mode - retrieval of latest reference string
    // !!! for standalone functionality - DO NOT be strict on existence of SAFE_SCRIPT_REF_FILE
    if(rtl_fs__file_exist(SAFE_SCRIPT_REF_FILE))
    {
        RTL_CHECK_RC_CLEANUP(rc, rtl_fs__file_read_to_buf(SAFE_SCRIPT_REF_FILE, &buf, &buf_len))
        snprintf(p_history_ref_str, sizeof(p_history_ref_str), "%.*s", buf_len, buf);
        RTL_FREE(buf)
        RTL_SYSLOG_DBG("safe-mode history - p_history_ref_str[%s]", p_history_ref_str)
    }

    // safe-mode - check line limits
    if(rtl_fs__file_exist(SAFE_MODE_HISTORY))
    {
        RTL_CHECK_RC_CLEANUP(rc, rtl_fs__file_read_to_buf(SAFE_MODE_HISTORY, &buf, &buf_len))

        for(i = 0 ; i < buf_len; i++)
        {
            if('\n' == buf[i])
            {
                nl_cnt++;
            }

        }

        RTL_FREE(buf)
        RTL_SYSLOG_DBG("safe-mode history - nl_cnt[%d]", nl_cnt)

        if(RTL_HISTORY_SAFE_MODE_LINE_MAX <= nl_cnt)
        {
            RTL_CHECK_RC_CLEANUP(rc, rtl_fs__file_mv(SAFE_MODE_HISTORY, SAFE_MODE_HISTORY_OLD))
        }
    }

    RTL_CHECK_RC(rc, rtl_fs__file_open_fp(SAFE_MODE_HISTORY, "a", &fp_sm))

    return  RTL_RC__OK;

cleanup:
    if(fp_sm)
    {
        fclose(fp_sm);
    }

    return rc;
}


static int rtl_safe_script(int s_nb, char **s_argv)
{
    int i=0 , rc;
	size_t f_len = 0, s_len = 0;
    char *script_buf = NULL;

    f_len += (sizeof(RTL_SCRIPT_INT_DELIMITER) - 1);

    f_len = f_len + strlen(script_start) ;  // counted length + nbScriptForNEWLINE

    // calculate final length
    for(i=0; i < s_nb; i++)
    {
        s_len  = strlen(s_argv[i]);
        f_len += s_len;
        f_len += (sizeof(RTL_SCRIPT_INT_DELIMITER) - 1);

        RTL_LOG_DBG_SS(" **** script[%03d] s_len(%u) f_len(%u) ****", i, s_len, f_len)
    }

    f_len += (sizeof(RTL_SCRIPT_INT_DELIMITER) - 1);

    f_len = f_len + strlen(script_end) + 1 ;  // counted length + nbScriptForNEWLINE + '\0'

    RTL_CHECK_CALLOC_SS(script_buf, f_len)

    strcat(script_buf, RTL_SCRIPT_INT_DELIMITER);
    strcat(script_buf, script_start);

    for(i=0; i < s_nb; i++)
    {
        strcat(script_buf, RTL_SCRIPT_INT_DELIMITER);
        strcat(script_buf, s_argv[i]);
    }

    strcat(script_buf, RTL_SCRIPT_INT_DELIMITER);
    strcat(script_buf, script_end);

    RTL_CHECK_RC_CLEANUP(rc, rtl_fs__exec_pipe(script_buf))

cleanup:
    RTL_FREE(script_buf);

    return rc;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//changes in following table should be synced to MCS_DATA_RATEStr[] in 8190n_proc.c
WLAN_RATE_T rate_11n_table_20M_LONG[]={
    {MCS0,  "6.5"},
    {MCS1,  "13"},
    {MCS2,  "19.5"},
    {MCS3,  "26"},
    {MCS4,  "39"},
    {MCS5,  "52"},
    {MCS6,  "58.5"},
    {MCS7,  "65"},
    {MCS8,  "13"},
    {MCS9,  "26"},
    {MCS10,     "39"},
    {MCS11,     "52"},
    {MCS12,     "78"},
    {MCS13,     "104"},
    {MCS14,     "117"},
    {MCS15,     "130"},
    {0}
};
WLAN_RATE_T rate_11n_table_20M_SHORT[]={
    {MCS0,  "7.2"},
    {MCS1,  "14.4"},
    {MCS2,  "21.7"},
    {MCS3,  "28.9"},
    {MCS4,  "43.3"},
    {MCS5,  "57.8"},
    {MCS6,  "65"},
    {MCS7,  "72.2"},
    {MCS8,  "14.4"},
    {MCS9,  "28.9"},
    {MCS10,     "43.3"},
    {MCS11,     "57.8"},
    {MCS12,     "86.7"},
    {MCS13,     "115.6"},
    {MCS14,     "130"},
    {MCS15,     "144.5"},
    {0}
};
WLAN_RATE_T rate_11n_table_40M_LONG[]={
    {MCS0,  "13.5"},
    {MCS1,  "27"},
    {MCS2,  "40.5"},
    {MCS3,  "54"},
    {MCS4,  "81"},
    {MCS5,  "108"},
    {MCS6,  "121.5"},
    {MCS7,  "135"},
    {MCS8,  "27"},
    {MCS9,  "54"},
    {MCS10,     "81"},
    {MCS11,     "108"},
    {MCS12,     "162"},
    {MCS13,     "216"},
    {MCS14,     "243"},
    {MCS15,     "270"},
    {0}
};
WLAN_RATE_T rate_11n_table_40M_SHORT[]={
	{MCS0,  "15"},
    {MCS1,  "30"},
    {MCS2,  "45"},
    {MCS3,  "60"},
    {MCS4,  "90"},
    {MCS5,  "120"},
    {MCS6,  "135"},
    {MCS7,  "150"},
    {MCS8,  "30"},
    {MCS9,  "60"},
    {MCS10,     "90"},
    {MCS11,     "120"},
    {MCS12,     "180"},
    {MCS13,     "240"},
    {MCS14,     "270"},
    {MCS15,     "300"},
    {0}
};

WLAN_RATE_T tx_fixed_rate[]={
    {1, "1"},
    {(1<<1),    "2"},
    {(1<<2),    "5.5"},
    {(1<<3),    "11"},
    {(1<<4),    "6"},
    {(1<<5),    "9"},
    {(1<<6),    "12"},
    {(1<<7),    "18"},
    {(1<<8),    "24"},
    {(1<<9),    "36"},
    {(1<<10),   "48"},
    {(1<<11),   "54"},
    {(1<<12),   "MCS0"},
    {(1<<13),   "MCS1"},
    {(1<<14),   "MCS2"},
    {(1<<15),   "MCS3"},
    {(1<<16),   "MCS4"},
    {(1<<17),   "MCS5"},
    {(1<<18),   "MCS6"},
    {(1<<19),   "MCS7"},
    {(1<<20),   "MCS8"},
    {(1<<21),   "MCS9"},
    {(1<<22),   "MCS10"},
    {(1<<23),   "MCS11"},
    {(1<<24),   "MCS12"},
    {(1<<25),   "MCS13"},
    {(1<<26),   "MCS14"},
    {(1<<27),   "MCS15"},
    {((1<<31)+0),   "NSS1-MCS0"},
    {((1<<31)+1),   "NSS1-MCS1"},
    {((1<<31)+2),   "NSS1-MCS2"},
    {((1<<31)+3),   "NSS1-MCS3"},
    {((1<<31)+4),   "NSS1-MCS4"},
    {((1<<31)+5),   "NSS1-MCS5"},
    {((1<<31)+6),   "NSS1-MCS6"},
    {((1<<31)+7),   "NSS1-MCS7"},
    {((1<<31)+8),   "NSS1-MCS8"},
    {((1<<31)+9),   "NSS1-MCS9"},
    {((1<<31)+10),  "NSS2-MCS0"},
    {((1<<31)+11),  "NSS2-MCS1"},
    {((1<<31)+12),  "NSS2-MCS2"},
    {((1<<31)+13),  "NSS2-MCS3"},
    {((1<<31)+14),  "NSS2-MCS4"},
    {((1<<31)+15),  "NSS2-MCS5"},
    {((1<<31)+16),  "NSS2-MCS6"},
    {((1<<31)+17),  "NSS2-MCS7"},
    {((1<<31)+18),  "NSS2-MCS8"},
    {((1<<31)+19),  "NSS2-MCS9"},
    {0}
};
//changes in following table should be synced to VHT_MCS_DATA_RATE[] in 8812_vht_gen.c
const unsigned short VHT_MCS_DATA_RATE[3][2][20] =
    {   {   {13, 26, 39, 52, 78, 104, 117, 130, 156, 156,
             26, 52, 78, 104, 156, 208, 234, 260, 312, 312},            // Long GI, 20MHz
            {14, 29, 43, 58, 87, 116, 130, 144, 173, 173,
            29, 58, 87, 116, 173, 231, 260, 289, 347, 347}  },      // Short GI, 20MHz
        {   {27, 54, 81, 108, 162, 216, 243, 270, 324, 360,
            54, 108, 162, 216, 324, 432, 486, 540, 648, 720},       // Long GI, 40MHz
            {30, 60, 90, 120, 180, 240, 270, 300,360, 400,
            60, 120, 180, 240, 360, 480, 540, 600, 720, 800}},      // Short GI, 40MHz
        {   {59, 117,  176, 234, 351, 468, 527, 585, 702, 780,
            117, 234, 351, 468, 702, 936, 1053, 1170, 1404, 1560},  // Long GI, 80MHz
            {65, 130, 195, 260, 390, 520, 585, 650, 780, 867,
            130, 260, 390, 520, 780, 1040, 1170, 1300, 1560,1733}   }   // Short GI, 80MHz
    };




static inline int
iw_get_ext(int                  skfd,           /* Socket to the kernel */
           char *               ifname,         /* Device name */
           int                  request,        /* WE ID */
           struct iwreq *       pwrq)           /* Fixed part of the request */
{
  /* Set device name */
  strncpy(pwrq->ifr_name, ifname, IFNAMSIZ);
  /* Do the request */
  return(ioctl(skfd, request, pwrq));
}

int getWlStaInfo( char *interface,  WLAN_STA_INFO_Tp pInfo )
{
#ifndef NO_ACTION
    int skfd=0;
    struct iwreq wrq;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(skfd==-1)
        return -1;
    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0){
      /* If no wireless name : no wireless extensions */
      close( skfd );
        return -1;
    }
    wrq.u.data.pointer = (caddr_t)pInfo;
    wrq.u.data.length = sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1);
    memset(pInfo, 0, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));

    if (iw_get_ext(skfd, interface, SIOCGIWRTLSTAINFO, &wrq) < 0){
        close( skfd );
        return -1;
    }
    close( skfd );
#else
    return -1;
#endif
    return 0;
}

void rtl_list__set_11ac_txrate(WLAN_STA_INFO_Tp pInfo,char* txrate)
{
    int channelWidth=0;//20M 0,40M 1,80M 2
    int shortGi=0;
    int rate_idx=pInfo->txOperaRates-0x90;
    if(!txrate)return;
/*
    TX_USE_40M_MODE     = BIT(0),
    TX_USE_SHORT_GI     = BIT(1),
    TX_USE_80M_MODE     = BIT(2)
*/
    if(pInfo->ht_info & 0x4)
        channelWidth=2;
    else if(pInfo->ht_info & 0x1)
        channelWidth=1;
    else
        channelWidth=0;
    if(pInfo->ht_info & 0x2)
        shortGi=1;

    sprintf(txrate, "%d", VHT_MCS_DATA_RATE[channelWidth][shortGi][rate_idx] >> 1);
}

int rtl_list__wirelessClientList(/*request *wp, */ int argc, char **argv)
{
    int i, found=0;
    WLAN_STA_INFO_Tp pInfo;
    char *buff;
    char mode_buf[20];
    char txrate[20];
    int rateid=0;
    char WLAN_IF[20];

    snprintf(WLAN_IF, sizeof(WLAN_IF), "%s", argv[0]);

    RTL_LOG_LIST("client specified interface '%s' argc '%d' ", WLAN_IF, argc)


//    buff = calloc(1, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));
//    if ( buff == 0 )
//    {
//
//        printf("Allocate buffer failed!\n");
//        return 0;
//    }
    RTL_CHECK_CALLOC_LIST(buff, (sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1)))

    #ifdef MBSSID
//    char Root_WLAN_IF[20];
    if (argc == 2)
    {
        int virtual_index;
        char virtual_name[20];
//        strcpy(Root_WLAN_IF, WLAN_IF);
        virtual_index = atoi(argv[argc-1]) - 1;

//        #ifdef CONFIG_RTL8196B_GW_8M
//        if (virtual_index > 0)
//            return 0;
//        #endif

        sprintf(virtual_name, "-va%d", virtual_index);
        strcat(WLAN_IF, virtual_name);
    }
    #endif

    if ( getWlStaInfo(WLAN_IF,  (WLAN_STA_INFO_Tp)buff ) < 0 )
    {
//        printf("Read wlan sta info failed!\n");
//
//        #ifdef MBSSID
//        if (argc == 2)
//            strcpy(WLAN_IF, Root_WLAN_IF);
//        #endif
//
//        return 0;

        free(buff);
        RTL_LOG_RC_LIST(RTL_RC__LIST_STA_INFO_ERROR, "getWlStaInfo failed for '%s'", WLAN_IF)
    }

//    #ifdef MBSSID
//    if (argc == 2)
//        strcpy(WLAN_IF, Root_WLAN_IF);
//    #endif

    for (i=1; i<=MAX_STA_NUM; i++) {
        pInfo = (WLAN_STA_INFO_Tp)&buff[i*sizeof(WLAN_STA_INFO_T)];

//        if (pInfo->aid && (pInfo->flag & STA_INFO_FLAG_ASOC)) {
        if (pInfo->aid && (pInfo->flag)) {

        if(pInfo->network& BAND_5G_11AC)
            sprintf(mode_buf, "%s", (" 11ac"));
        else if(pInfo->network & BAND_11N)
            sprintf(mode_buf, "%s", (" 11n"));
        else if (pInfo->network & BAND_11G)
            sprintf(mode_buf,"%s",  (" 11g"));
        else if (pInfo->network & BAND_11B)
            sprintf(mode_buf, "%s", (" 11b"));
        else if (pInfo->network& BAND_11A)
            sprintf(mode_buf, "%s", (" 11a"));
        else
            sprintf(mode_buf, "%s", (" ---"));


        if(pInfo->txOperaRates >= 0x90) {
            //sprintf(txrate, "%d", pInfo->acTxOperaRate);
            rtl_list__set_11ac_txrate(pInfo, txrate);
        } else if((pInfo->txOperaRates & 0x80) != 0x80){
            if(pInfo->txOperaRates%2){
                sprintf(txrate, "%d%s",pInfo->txOperaRates/2, ".5");
            }else{
                sprintf(txrate, "%d",pInfo->txOperaRates/2);
            }
        }else{
            if((pInfo->ht_info & 0x1)==0){ //20M
                if((pInfo->ht_info & 0x2)==0){//long
                    for(rateid=0; rateid<16;rateid++){
                        if(rate_11n_table_20M_LONG[rateid].id == pInfo->txOperaRates){
                            sprintf(txrate, "%s", rate_11n_table_20M_LONG[rateid].rate);
                            break;
                        }
                    }
                }else if((pInfo->ht_info & 0x2)==0x2){//short
                    for(rateid=0; rateid<16;rateid++){
                        if(rate_11n_table_20M_SHORT[rateid].id == pInfo->txOperaRates){
                            sprintf(txrate, "%s", rate_11n_table_20M_SHORT[rateid].rate);
                            break;
                        }
                    }
                }
            }else if((pInfo->ht_info & 0x1)==0x1){//40M
                if((pInfo->ht_info & 0x2)==0){//long

                    for(rateid=0; rateid<16;rateid++){
                        if(rate_11n_table_40M_LONG[rateid].id == pInfo->txOperaRates){
                            sprintf(txrate, "%s", rate_11n_table_40M_LONG[rateid].rate);
                            break;
                        }
                    }
                }else if((pInfo->ht_info & 0x2)==0x2){//short
                    for(rateid=0; rateid<16;rateid++){
                        if(rate_11n_table_40M_SHORT[rateid].id == pInfo->txOperaRates){
                            sprintf(txrate, "%s", rate_11n_table_40M_SHORT[rateid].rate);
                            break;
                        }
                    }
                }
}

        }
//            nBytesSent += req_format_write(wp,
//            ("<tr bgcolor=#b7b7b7><td><font size=2>%02x:%02x:%02x:%02x:%02x:%02x</td>"
//            "<td><font size=2>%s</td>"
//            "<td><font size=2>%d</td>"
//                "<td><font size=2>%d</td>"
//            "<td><font size=2>%s</td>"
//            "<td><font size=2>%s</td>"
//            "<td><font size=2>%d</td>"
//            "</tr>"),
//            pInfo->addr[0],pInfo->addr[1],pInfo->addr[2],pInfo->addr[3],pInfo->addr[4],pInfo->addr[5],
//            mode_buf,
//            pInfo->tx_packets, pInfo->rx_packets,
//            txrate,
//            ( (pInfo->flag & STA_INFO_FLAG_ASLEEP) ? "yes" : "no"),
//            pInfo->expired_time/100
//            );

            printf("MAC[%02x:%02x:%02x:%02x:%02x:%02x] "
            "MODE[%s] "
            "TXpacket[%lu] "
            "RXpacket[%lu] "
            "TXrate[%s] "
            "POWERsave[%s] "
            "EXPIRED[%lu] "
            "\n",
            pInfo->addr[0],pInfo->addr[1],pInfo->addr[2],pInfo->addr[3],pInfo->addr[4],pInfo->addr[5],
            mode_buf,
            pInfo->tx_packets, pInfo->rx_packets,
            txrate,
            ( (pInfo->flag & STA_INFO_FLAG_ASLEEP) ? "yes" : "no"),
            pInfo->expired_time/100);

            found++;
        }
    }

//    if (found == 0)
//    {
//        nBytesSent += req_format_write(wp,
//            ("<tr bgcolor=#b7b7b7><td><font size=2>None</td>"
//            "<td><font size=2>---</td>"
//                "<td><font size=2>---</td>"
//            "<td><font size=2>---</td>"
//            "<td><font size=2>---</td>"
//            "<td><font size=2>---</td>"
//            "<td><font size=2>---</td>"
//            "</tr>"));
//    }

    free(buff);

    RTL_LOG_RC_LIST(RTL_RC__OK, "TOTAL nb found %d for interface '%s'", found , WLAN_IF)
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#endif // __RTL_H__

