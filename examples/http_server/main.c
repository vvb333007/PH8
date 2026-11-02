
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "ph7.h"
#include "httpd.h"


/* Compile errors and warnings of the engine end up here. */
static int engine_err(const void *data, unsigned int len, void *user)
{
    (void)user;
    fwrite(data, 1, len, stderr);
    return PH7_OK;
}

int main(int argc, char **argv)
{
    ph7 *engine = NULL;
    const char *root = argc > 1 ? argv[1] : "www";

    signal(SIGPIPE, SIG_IGN);       /* send() to a closed socket must not kill us */

    if (ph7_init(&engine) != PH7_OK) {
        fprintf(stderr, "ph7_init failed\n");
        return 1;
    }
    ph7_config(engine, PH7_CONFIG_ERR_OUTPUT, engine_err, NULL);


    ph8_httpd_cfg_t cfg = PH8_HTTPD_CFG_DEFAULT();

    cfg.port = 8080;
    cfg.handler = ph8_site_handler;
    cfg.worker_stack = 8 * 1024;   /* the interpreter recurses: 8 KiB is far too little; tune on target */
    cfg.engine = engine;
    cfg.docroot = root;

    if (ph8_httpd_start(&cfg) != ESP_OK) {
        return 1;
    }
    for (;;) {
        sleep(3600);
    }
}
