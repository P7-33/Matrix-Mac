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

#include "options.h"
#include "applog.h"
#include "version.h"
#include "persistent_memory.h"
#include "cpu.h"


static void print_memory() {
    const char *t1 = (persistent_memory_flags & MEMORY_HUGEPAGES_AVAILABLE) ? OPT_COLOR(CL_LGR, "available") : OPT_COLOR(CL_LRD, "unavailable");
    const char *t2 = (persistent_memory_flags & MEMORY_HUGEPAGES_ENABLED)   ? OPT_COLOR(CL_LGR, "enabled")   : OPT_COLOR(CL_LRD, "disabled");

    if (opt_colors) {
        applog_notime(LOG_INFO, CL_LGR " * " CL_WHT "HUGE PAGES:   %s, %s", t1, t2);
    }
    else {
        applog_notime(LOG_INFO, " * HUGE PAGES:   %s, %s", t1, t2);
    }
}


static void print_cpu() {
    const char *t1 = (cpu_info.flags & CPU_FLAG_X86_64) ? OPT_COLOR(CL_LGR, "x86_64") : OPT_COLOR(CL_LRD, "-x86_64");
    const char *t2 = (cpu_info.flags & CPU_FLAG_AES)    ? OPT_COLOR(CL_LGR, "AES-NI") : OPT_COLOR(CL_LRD, "-AES-NI");

    if (opt_colors) {
        applog_notime(LOG_INFO, CL_LGR " * " CL_WHT "CPU:          %s (%d)", cpu_info.brand, cpu_info.sockets);
    }
    else {
        applog_notime(LOG_INFO, " * CPU:          %s (%d)", cpu_info.brand, cpu_info.sockets);
    }

 #   ifndef XMRIG_NO_LIBCPUID
    if (opt_colors) {
        applog_notime(LOG_INFO, CL_LGR " * " CL_WHT "CPU L2/L3:    %.1f MB/%.1f MB", cpu_info.l2_cache / 1024.0, cpu_info.l3_cache / 1024.0);
    }
    else {
        applog_notime(LOG_INFO, " * CPU L2/L3:    %.1f MB/%.1f MB", cpu_info.l2_cache / 1024.0, cpu_info.l3_cache / 1024.0);
    }
 #  endif

    if (opt_colors) {
        applog_notime(LOG_INFO, CL_LGR " * " CL_WHT "CPU FEATURES: %s %s", t1, t2);
    }
    else {
        applog_notime(LOG_INFO, " * CPU FEATURES: %s %s", t1, t2);
    }
}


static void print_threads() {
    const char *extra = "";
    if (opt_nicehash) {
        extra = ", nicehash";
    }

    if (opt_colors) {
        applog_notime(LOG_INFO, CL_LGR " * " CL_WHT "THREADS:      " CL_WHT "%d" CL_WHT ", av=%d, %s/%s, donate=%d%%%s", opt_n_threads, opt_av, get_current_algo_name(), get_current_variant_name(), opt_donate_level, extra);
    }
    else {
        applog_notime(LOG_INFO, " * THREADS:      %d, av=%d, %s/%s, donate=%d%%%s", opt_n_threads, opt_av, get_current_algo_name(), get_current_variant_name(), opt_donate_level, extra);
    }
}


static void print_stratum() {
    if (opt_colors) {
        applog_notime(LOG_INFO, CL_LGR " * " CL_WHT "STRATUM URL:  " CL_LCY "%s", opt_url);

        if (opt_backup_url) {
            applog_notime(LOG_INFO, CL_LGR " * " CL_WHT "FAILOVER URL: " CL_LCY "%s", opt_backup_url);
        }
        else {
            applog_notime(LOG_INFO, CL_LGR " * " CL_WHT "FAILOVER URL: " CL_LRD "none");
        }
    }
    else {
        applog_notime(LOG_INFO, " * STRATUM URL:  %s", opt_url);
        applog_notime(LOG_INFO, " * FAILOVER URL: %s", opt_backup_url ? opt_backup_url : "none");
    }
}


void print_summary() {
    if (opt_colors) {
        applog_notime(LOG_INFO, CL_LGR " * " CL_WHT APP_NAME " " APP_VERSION "  " CL_LCY APP_SITE);
    }
    else {
        applog_notime(LOG_INFO, " * " APP_NAME " " APP_VERSION "  " APP_SITE);
    }

    print_memory();
    print_cpu();
    print_threads();
    print_stratum();
}



