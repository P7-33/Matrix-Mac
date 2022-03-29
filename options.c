/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2019 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2019 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include <curl/curl.h>
#include <getopt.h>

#include "version.h"
#include "utils/applog.h"
#include "options.h"
#include "cpu.h"
#include "donate.h"
#include "algo/cryptonight/cryptonight.h"


int64_t opt_affinity      = -1L;
int     opt_n_threads     = 0;
int     opt_retries       = 5;
int     opt_retry_pause   = 5;
int     opt_donate_level  = DONATE_LEVEL;
int     opt_max_cpu_usage = 75;
bool    opt_colors        = true;
bool    opt_keepalive     = false;
bool    opt_background    = false;
bool    opt_double_hash   = false;
bool    opt_safe          = false;
bool    opt_nicehash      = false;
char    *opt_url          = NULL;
char    *opt_backup_url   = NULL;
char    *opt_userpass     = NULL;
char    *opt_user         = NULL;
char    *opt_pass         = NULL;

enum Algo opt_algo         = ALGO_CRYPTONIGHT;
enum Variant opt_variant   = VARIANT_AUTO;
enum AlgoVariant opt_av    = AV_AUTO;
enum Assembly opt_assembly = ASM_AUTO;


struct AlgoData
{
    const char *name;
    const char *shortName;
    enum Algo algo;
    enum Variant variant;
};


static struct AlgoData const algorithms[] = {
    { "cryptonight",           "cn",           ALGO_CRYPTONIGHT,       VARIANT_AUTO },
    { "cryptonight/0",         "cn/0",         ALGO_CRYPTONIGHT,       VARIANT_0    },
    { "cryptonight/1",         "cn/1",         ALGO_CRYPTONIGHT,       VARIANT_1    },
    { "cryptonight/2",         "cn/2",         ALGO_CRYPTONIGHT,       VARIANT_2    },
    { "cryptonight/4",         "cn/4",         ALGO_CRYPTONIGHT,       VARIANT_4    },
    { "cryptonight/r",         "cn/r",         ALGO_CRYPTONIGHT,       VARIANT_4    },

#   ifndef XMRIG_NO_AEON
    { "cryptonight-lite",      "cn-lite",      ALGO_CRYPTONIGHT_LITE,  VARIANT_AUTO },
    { "cryptonight-light",     "cn-light",     ALGO_CRYPTONIGHT_LITE,  VARIANT_AUTO },
    { "cryptonight-lite/0",    "cn-lite/0",    ALGO_CRYPTONIGHT_LITE,  VARIANT_0    },
    { "cryptonight-lite/1",    "cn-lite/1",    ALGO_CRYPTONIGHT_LITE,  VARIANT_1    },
#   endif
};


static char const usage[] = "\
Usage: " APP_ID " [OPTIONS]\n\
Options:\n\
  -a, --algo=ALGO       cryptonight (default) or cryptonight-lite\n\
      --variant=N       cryptonight variant: 0-4\n\
  -o, --url=URL         URL of mining server\n\
  -b, --backup-url=URL  URL of backup mining server\n\
  -O, --userpass=U:P    username:password pair for mining server\n\
  -u, --user=USERNAME   username for mining server\n\
  -p, --pass=PASSWORD   password for mining server\n\
  -t, --threads=N       number of miner threads\n\
  -v, --av=N            algorithm variation, 0 auto select\n\
  -k, --keepalive       send keepalived for prevent timeout (need pool support)\n\
  -r, --retries=N       number of times to retry before switch to backup server (default: 5)\n\
  -R, --retry-pause=N   time to pause between retries (default: 5)\n\
      --cpu-affinity    set process affinity to CPU core(s), mask 0x3 for cores 0 and 1\n\
      --no-color        disable colored output\n\
      --donate-level=N  donate level, default 5%% (5 minutes in 100 minutes)\n\
  -B, --background      run the miner in the background\n\
  -c, --config=FILE     load a JSON-format configuration file\n\
      --max-cpu-usage=N maximum CPU usage for automatic threads mode (default 75)\n\
      --safe            safe adjust threads and av settings for current CPU\n\
      --nicehash        enable nicehash support\n\
  -h, --help            display this help and exit\n\
  -V, --version         output version information and exit\n\
";


static char const short_options[] = "a:c:khBp:Px:r:R:s:t:T:o:u:O:v:Vb:";


static struct option const options[] = {
    { "algo",          1, NULL, 'a'  },
    { "av",            1, NULL, 'v'  },
    { "background",    0, NULL, 'B'  },
    { "backup-url",    1, NULL, 'b'  },
    { "config",        1, NULL, 'c'  },
    { "cpu-affinity",  1, NULL, 1020 },
    { "donate-level",  1, NULL, 1003 },
    { "help",          0, NULL, 'h'  },
    { "keepalive",     0, NULL, 'k'  },
    { "max-cpu-usage", 1, NULL, 1004 },
    { "nicehash",      0, NULL, 1006 },
    { "no-color",      0, NULL, 1002 },
    { "pass",          1, NULL, 'p'  },
    { "retries",       1, NULL, 'r'  },
    { "retry-pause",   1, NULL, 'R'  },
    { "safe",          0, NULL, 1005 },
    { "threads",       1, NULL, 't'  },
    { "url",           1, NULL, 'o'  },
    { "user",          1, NULL, 'u'  },
    { "userpass",      1, NULL, 'O'  },
    { "version",       0, NULL, 'V'  },
    { "variant",       1, NULL, 1021 },
    { "asm",           1, NULL, 1022 },
    { NULL,            0, NULL, 0    }
};


static const char *algo_names[] = {
    "cryptonight",
#   ifndef XMRIG_NO_AEON
    "cryptonight-lite"
#   endif
};


static const char *variant_names[] = {
    "auto",
    "0",
    "1",
    "2",
    "4"
};


static const char *asm_names[] = {
    "none",
    "auto",
    "intel",
    "ryzen",
    "bulldozer"
};


#ifndef XMRIG_NO_AEON
static int get_cryptonight_lite_variant(int variant) {
    if (variant <= AV_AUTO || variant >= AV_MAX) {
        return (cpu_info.flags & CPU_FLAG_AES) ? AV_DOUBLE : AV_DOUBLE_SOFT;
    }

    if (opt_safe && !(cpu_info.flags & CPU_FLAG_AES) && variant <= AV_DOUBLE) {
        return variant + 2;
    }

    return variant;
}
#endif


static int get_algo_variant(int algo, int variant) {
#   ifndef XMRIG_NO_AEON
    if (algo == ALGO_CRYPTONIGHT_LITE) {
        return get_cryptonight_lite_variant(variant);
    }
#   endif

    if (variant <= AV_AUTO || variant >= AV_MAX) {
        return (cpu_info.flags & CPU_FLAG_AES) ? AV_SINGLE : AV_SINGLE_SOFT;
    }

    if (opt_safe && !(cpu_info.flags & CPU_FLAG_AES) && variant <= AV_DOUBLE) {
        return variant + 2;
    }

    return variant;
}


static void parse_config(json_t *config, char *ref);
static char *parse_url(const char *arg);


static void parse_arg(int key, char *arg) {
    char *p;
    int v;
    uint64_t ul;

    switch (key)
    {
    case 'a': /* --algo */
        for (size_t i = 0; i < ARRAY_SIZE(algorithms); i++) {
            if ((strcasecmp(arg, algorithms[i].name) == 0) || (strcasecmp(arg, algorithms[i].shortName) == 0)) {
                opt_algo    = algorithms[i].algo;
                opt_variant = algorithms[i].variant;
                break;
            }
        }
        break;

    case 1022: /* --asm */
        for (size_t i = 0; i < ARRAY_SIZE(asm_names); i++) {
            if (strcasecmp(arg, asm_names[i]) == 0) {
                opt_assembly = i;
            }
        }
        break;

    case 'O': /* --userpass */
        p = strchr(arg, ':');
        if (!p) {
            show_usage_and_exit(1);
        }

        free(opt_userpass);
        opt_userpass = strdup(arg);
        free(opt_user);
        opt_user = calloc(p - arg + 1, 1);
        strncpy(opt_user, arg, p - arg);
        free(opt_pass);
        opt_pass = strdup(p + 1);
        break;

    case 'o': /* --url */
        p = parse_url(arg);
        if (p) {
            free(opt_url);
            opt_url = p;
        }
        break;

    case 'b': /* --backup-url */
        p = parse_url(arg);
        if (p) {
            free(opt_backup_url);
            opt_backup_url = p;
        }
        break;

    case 'u': /* --user */
        free(opt_user);
        opt_user = strdup(arg);
        break;

    case 'p': /* --pass */
        free(opt_pass);
        opt_pass = strdup(arg);
        break;

    case 'r': /* --retries */
        v = atoi(arg);
        if (v < 1 || v > 1000) {
            show_usage_and_exit(1);
        }

        opt_retries = v;
        break;

    case 'R': /* --retry-pause */
        v = atoi(arg);
        if (v < 1 || v > 3600) {
            show_usage_and_exit(1);
        }

        opt_retry_pause = v;
        break;

    case 't': /* --threads */
        v = atoi(arg);
        if (v < 1 || v > 1024) {
            show_usage_and_exit(1);
        }

        opt_n_threads = v;
        break;

    case 1004: /* --max-cpu-usage */
        v = atoi(arg);
        if (v < 1 || v > 100) {
            show_usage_and_exit(1);
        }

        opt_max_cpu_usage = v;
        break;

    case 1005: /* --safe */
        opt_safe = true;
        break;

    case 'k': /* --keepalive */
        opt_keepalive = true;
        break;

    case 'V': /* --version */
        show_version_and_exit();
        break;

    case 'h': /* --help */
        show_usage_and_exit(0);
        break;

    case 'c': { /* --config */
        json_error_t err;
        json_t *config = json_load_file(arg, 0, &err);

        if (!json_is_object(config)) {
            if (err.line < 0) {
                applog(LOG_ERR, "%s\n", err.text);
            }
            else {
                applog(LOG_ERR, "%s:%d: %s\n", arg, err.line, err.text);
            }
        } else {
            parse_config(config, arg);
            json_decref(config);
        }
        break;
    }

    case 'B': /* --background */
        opt_background = true;
        opt_colors = false;
        break;

    case 'v': /* --av */
        v = atoi(arg);
        if (v <= AV_AUTO || v >= AV_MAX) {
            show_usage_and_exit(1);
        }

        opt_av = v;
        break;

    case 1020: /* --cpu-affinity */
        p  = strstr(arg, "0x");
        ul = p ? strtoul(p, NULL, 16) : atol(arg);
        if (ul > (1UL << cpu_info.total_logical_cpus) -1) {
            ul = -1;
        }

        opt_affinity = ul;
        break;

    case 1002: /* --no-color */
        opt_colors = false;
        break;

    case 1003: /* --donate-level */
//        v = atoi(arg);
//        if (v < 1 || v > 99) {
//            show_usage_and_exit(1);
//        }

//        opt_donate_level = v;
        break;

    case 1021: /* --variant */
        v = atoi(arg);
        if (v == 4 || strcasecmp(arg, "r") == 0) {
            opt_variant = VARIANT_4;
        }
        else if (v > VARIANT_AUTO && v < VARIANT_MAX) {
            opt_variant = v;
        }

        break;

    case 1006: /* --nicehash */
        opt_nicehash = true;
        break;

    default:
        show_usage_and_exit(1);
    }
}


static void parse_config(json_t *config, char *ref)
{
    size_t i;
    char buf[16];
    json_t *val;

    applog(LOG_ERR, ref);

    for (i = 0; i < ARRAY_SIZE(options); i++) {
        if (!options[i].name) {
            break;
        }

        val = json_object_get(config, options[i].name);
        if (!val) {
            continue;
        }

        if (options[i].has_arg && json_is_string(val)) {
            char *s = strdup(json_string_value(val));
            if (!s) {
                break;
            }

            parse_arg(options[i].val, s);
            free(s);
        }
        else if (options[i].has_arg && json_is_integer(val)) {
            sprintf(buf, "%d", (int) json_integer_value(val));
            parse_arg(options[i].val, buf);
        }
        else if (options[i].has_arg && json_is_real(val)) {
            sprintf(buf, "%f", json_real_value(val));
            parse_arg(options[i].val, buf);
        }
        else if (!options[i].has_arg) {
            if (json_is_true(val)) {
               parse_arg(options[i].val, "");
            }
        }
        else {
            applog(LOG_ERR, "JSON option %s invalid", options[i].name);
        }
    }
}


static char *parse_url(const char *arg)
{
    char *p = strstr(arg, "://");
    if (p) {
        if (strncasecmp(arg, "stratum+tcp://", 14)) {
            show_usage_and_exit(1);
        }

        return strdup(arg);
    }


    if (!strlen(arg) || *arg == '/') {
        show_usage_and_exit(1);
    }

    char *dest = malloc(strlen(arg) + 16);
    sprintf(dest, "stratum+tcp://%s", arg);

    return dest;
}


/**
 * Parse application command line via getopt.
 */
void parse_cmdline(int argc, char *argv[]) {
    opt_user = strdup("x");
    opt_pass = strdup("x");

    int key;

    while (1) {
        key = getopt_long(argc, argv, short_options, options, NULL);
        if (key < 0) {
            break;
        }

        parse_arg(key, optarg);
    }

    if (optind < argc) {
        fprintf(stderr, "%s: unsupported non-option argument '%s'\n", argv[0], argv[optind]);
        show_usage_and_exit(1);
    }

    if (!opt_url) {
        applog_notime(LOG_ERR, "No pool URL supplied. Exiting.\n", argv[0]);
        proper_exit(1);
    }

    if (strstr(opt_url, ".nicehash.com:") != NULL) {
        opt_nicehash = true;
    }

    if (!opt_userpass) {
        opt_userpass = malloc(strlen(opt_user) + strlen(opt_pass) + 2);
        if (!opt_userpass) {
            proper_exit(1);
        }

        sprintf(opt_userpass, "%s:%s", opt_user, opt_pass);
    }

    opt_av = get_algo_variant(opt_algo, opt_av);

    if (!cryptonight_init(opt_av)) {
        applog(LOG_ERR, "Cryptonight hash self-test failed. This might be caused by bad compiler optimizations.");
        proper_exit(1);
    }

    if (!opt_n_threads) {
        opt_n_threads = get_optimal_threads_count(opt_algo, opt_double_hash, opt_max_cpu_usage);
    }

    if (opt_safe) {
        const int count = get_optimal_threads_count(opt_algo, opt_double_hash, opt_max_cpu_usage);
        if (opt_n_threads > count) {
            opt_n_threads = count;
        }
    }
}


void show_usage_and_exit(int status) {
    if (status) {
        fprintf(stderr, "Try \"" APP_ID "\" --help' for more information.\n");
    }
    else {
        printf(usage);
    }

    proper_exit(status);
}


void show_version_and_exit(void) {
    printf(APP_NAME " " APP_VERSION "\n built on " __DATE__

    #ifdef __GNUC__
    " with GCC");
    printf(" %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    #endif

    printf("\n features:"
    #ifdef __i386__
    " i386"
    #endif
    #ifdef __x86_64__
    " x86_64"
    #endif
    #ifdef __AES__
    " AES-NI"
    #endif
    "\n");

    printf("\n%s\n", curl_version());
    #ifdef JANSSON_VERSION
    printf("libjansson/%s\n", JANSSON_VERSION);
    #endif
    proper_exit(0);
}


const char *get_current_algo_name(void) {
    return algo_names[opt_algo];
}


const char *get_current_variant_name(void)
{
    return variant_names[opt_variant + 1];
}
