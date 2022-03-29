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

#ifndef XMRIG_STRATUM_H
#define XMRIG_STRATUM_H


#include <stdbool.h>
#include <inttypes.h>
#include <curl/curl.h>


/**
 * 128tx exploit.
 *
 * Max blob size is 84 (75 fixed + 9 variable), aligned to 96.
 * https://github.com/xmrig/xmrig/issues/1 Thanks fireice-uk.
 */
struct work {
    uint32_t blob[21] __attribute__((aligned(16)));
    size_t blob_size  __attribute__((aligned(16)));
    uint32_t target   __attribute__((aligned(16)));
    uint32_t hash[8]  __attribute__((aligned(16)));
    char job_id[64]   __attribute__((aligned(16)));
    uint64_t height;
};


struct stratum_ctx {
    char *url;

    CURL *curl;
    char *curl_url;
    char curl_err_str[CURL_ERROR_SIZE];
    curl_socket_t sock;
    size_t sockbuf_size;
    char *sockbuf;
    pthread_mutex_t sock_lock;
    bool ready;

    char id[64];

    struct work work;
    struct work g_work;
    time_t g_work_time;
    pthread_mutex_t work_lock;
};


bool stratum_send_line(struct stratum_ctx *sctx, char *s);
bool stratum_socket_full(struct stratum_ctx *sctx, int timeout);
char *stratum_recv_line(struct stratum_ctx *sctx);
bool stratum_connect(struct stratum_ctx *sctx, const char *url);
void stratum_disconnect(struct stratum_ctx *sctx);
bool stratum_authorize(struct stratum_ctx *sctx, const char *user, const char *pass);
bool stratum_handle_method(struct stratum_ctx *sctx, const char *s);
bool stratum_handle_response(char *buf);
bool stratum_keepalived(struct stratum_ctx *sctx);

#endif /* XMRIG_STRATUM_H */
