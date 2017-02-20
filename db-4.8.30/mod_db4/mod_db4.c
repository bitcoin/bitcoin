/*-
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * http://www.apache.org/licenses/LICENSE-2.0.txt
 * 
 * authors: George Schlossnagle <george@omniti.com>
 */

extern "C"
{
#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "scoreboard.h"
#include "util_script.h"

#include "sem_utils.h"
}
#include "mod_db4_export.h"
#include "utils.h"

extern scoreboard *ap_scoreboard_image;

/*
 * Declare ourselves so the configuration routines can find and know us.
 * We'll fill it in at the end of the module.
 */

extern module MODULE_VAR_EXPORT db4_module;

void kill_all_children()
{
    int i, ret = 1;
    ap_sync_scoreboard_image();
    for(;ret != 0;) {
        ret = 0;
        for (i = 0; i < HARD_SERVER_LIMIT; ++i) {
            ret += kill(ap_scoreboard_image->parent[i].pid, SIGTERM);
        }    
    }
}

int moderator_main(void * ptr, child_info *ci)
{
    for(;;) {
        env_wait_for_child_crash();
        kill_all_children();
        env_global_rw_lock();
        global_ref_count_clean();
        env_ok_to_proceed();
        env_global_unlock();
    } 
}

static void sig_unrecoverable(int sig)
{
    env_child_crash();
    /* reinstall default apache handler */
    signal(sig, SIG_DFL);
    kill(getpid(), sig);
}

static void db4_init(server_rec *s, pool *p)
{
    int mpid;
    env_locks_init();
    mpid=ap_spawn_child(p, moderator_main, NULL, kill_always, NULL, NULL, NULL);
}

/*
 * Worker process init
 */

static void db4_child_init(server_rec *s, pool *p)
{
    /* install our private signal handlers */
    signal(SIGSEGV, sig_unrecoverable);
    signal(SIGBUS,  sig_unrecoverable);
    signal(SIGABRT, sig_unrecoverable);
    signal(SIGILL,  sig_unrecoverable);
    env_rsrc_list_init();
}

/*
 * Worker process exit
 */
static void db4_child_exit(server_rec *s, pool *p)
{
    mod_db4_child_clean_process_shutdown();
}

static const command_rec db4_cmds[] =
{
    {NULL}
};

module MODULE_VAR_EXPORT db4_module =
{
    STANDARD_MODULE_STUFF,
    db4_init,               /* module initializer */
    NULL,  /* per-directory config creator */
    NULL,   /* dir config merger */
    NULL,       /* server config creator */
    NULL,        /* server config merger */
    db4_cmds,               /* command table */
    NULL,           /* [9] list of handlers */
    NULL,  /* [2] filename-to-URI translation */
    NULL,      /* [5] check/validate user_id */
    NULL,       /* [6] check user_id is valid *here* */
    NULL,     /* [4] check access by host address */
    NULL,       /* [7] MIME type checker/setter */
    NULL,        /* [8] fixups */
    NULL,             /* [10] logger */
#if MODULE_MAGIC_NUMBER >= 19970103
    NULL,      /* [3] header parser */
#endif
#if MODULE_MAGIC_NUMBER >= 19970719
    db4_child_init,         /* process initializer */
#endif
#if MODULE_MAGIC_NUMBER >= 19970728
    db4_child_exit,         /* process exit/cleanup */
#endif
#if MODULE_MAGIC_NUMBER >= 19970902
    NULL   /* [1] post read_request handling */
#endif
};
/* vim: set ts=4 sts=4 bs=2 ai expandtab : */
