#include <errno.h>
#include <getopt.h>

#include <aerospike/aerospike.h>
#include <citrusleaf/cf_log.h>

#include "../main/aerospike/_log.h"
#include "test.h"
#include "aerospike_test.h"
#include "util/info_util.h"

/******************************************************************************
 * MACROS
 *****************************************************************************/

#define TIMEOUT 1000
#define SCRIPT_LEN_MAX 1048576

/******************************************************************************
 * VARIABLES
 *****************************************************************************/

aerospike * as = NULL;
int g_argc = 0;
char ** g_argv = NULL;
char g_host[MAX_HOST_SIZE];
int g_port = 3000;
static char g_user[AS_USER_SIZE];
static char g_password[AS_PASSWORD_HASH_SIZE];

/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static void citrusleaf_log_callback(cf_log_level level, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
	switch(level) {
		case CF_ERROR: 
			atf_logv(stderr, "ERROR", ATF_LOG_PREFIX, NULL, 0, fmt, ap);
			break;
		case CF_WARN: 
			atf_logv(stderr, "WARN", ATF_LOG_PREFIX, NULL, 0, fmt, ap);
			break;
		case CF_INFO: 
			atf_logv(stderr, "INFO", ATF_LOG_PREFIX, NULL, 0, fmt, ap);
			break;
		case CF_DEBUG: 
			atf_logv(stderr, "DEBUG", ATF_LOG_PREFIX, NULL, 0, fmt, ap);
			break;
		default:
			break;
	}
    va_end(ap);
}

static const char* short_options = "h:p:U:P::";

static struct option long_options[] = {
	{"hosts",        1, 0, 'h'},
	{"port",         1, 0, 'p'},
	{"user",         1, 0, 'U'},
	{"password",     2, 0, 'P'},
	{0,              0, 0, 0}
};

static bool parse_opts(int argc, char* argv[])
{
	int option_index = 0;
	int c;

	strcpy(g_host, "127.0.0.1");

	while ((c = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (c) {
		case 'h':
			if (strlen(optarg) >= sizeof(g_host)) {
				error("ERROR: host exceeds max length");
				return false;
			}
			strcpy(g_host, optarg);
			error("host:           %s", g_host);
			break;

		case 'p':
			g_port = atoi(optarg);
			break;

		case 'U':
			if (strlen(optarg) >= sizeof(g_user)) {
				error("ERROR: user exceeds max length");
				return false;
			}
			strcpy(g_user, optarg);
			error("user:           %s", g_user);
			break;

		case 'P':
			as_password_prompt_hash(optarg, g_password);
			break;
				
		default:
	        error("unrecognized options");
			return false;
		}
	}

	return true;
}

static bool before(atf_plan * plan) {


    if ( as ) {
        error("aerospike was already initialized");
        return false;
    }

    if (! parse_opts(g_argc, g_argv)) {
        error("failed to parse options");
    	return false;
    }
	
	as_config config;
	as_config_init(&config);
	as_config_add_host(&config, g_host, g_port);
	as_config_set_user(&config, g_user, g_password);
	config.lua.cache_enabled = false;
	strcpy(config.lua.system_path, "modules/lua-core/src");
	strcpy(config.lua.user_path, "src/test/lua");
    as_policies_init(&config.policies);

	as_error err;
	as_error_reset(&err);

	as = aerospike_new(&config);

	cf_set_log_level(CF_INFO);
	cf_set_log_callback(citrusleaf_log_callback);
	as_log_set_level(&as->log, AS_LOG_LEVEL_INFO);
	
	if ( aerospike_connect(as, &err) == AEROSPIKE_OK ) {
		info("connected to %s:%d", g_host, g_port);
    	return true;
	}
	else {
		error("%s @ %s[%s:%d]", err.message, err.func, err.file, err.line);
		return false;
	}
}

static bool after(atf_plan * plan) {

    if ( ! as ) {
        error("aerospike was not initialized");
        return false;
    }

	as_error err;
	as_error_reset(&err);
	
	if ( aerospike_close(as, &err) == AEROSPIKE_OK ) {
		info("disconnected from %s:%d", g_host, g_port);
    	return true;
	}
	else {
		error("%s @ %s[%s:%d]", g_host, g_port, err.message, err.func, err.file, err.line);
		return false;
	}
	
	aerospike_destroy(as);
	
	as = NULL;

    return true;
}

/******************************************************************************
 * TEST PLAN
 *****************************************************************************/

PLAN( aerospike_test ) {

    plan_before( before );
    plan_after( after );

    // aerospike_key module
    plan_add( key_basics );
    plan_add( key_apply );
    plan_add( key_apply2 );
    
    // aerospike_info module
    plan_add( info_basics );

    // aerospike_info module
    plan_add( udf_basics );
    plan_add( udf_types );
    plan_add( udf_record );

    //aerospike_sindex module
    plan_add( index_basics );

    // aerospike_query module
    plan_add( query_foreach );

    // aerospike_scan module
    plan_add( scan_basics );

    // aerospike_scan module
    plan_add( batch_get );

    // as_policy module
    plan_add( policy_read );
    plan_add( policy_scan );

    // as_ldt module
    plan_add( ldt_lmap );

}

