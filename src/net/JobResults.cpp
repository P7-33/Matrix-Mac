/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2020 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2020 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
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


#include "net/JobResults.h"
#include "base/io/log/Log.h"
#include "base/tools/Handle.h"
#include "base/tools/Object.h"
#include "net/interfaces/IJobResultListener.h"
#include "net/JobResult.h"


#ifdef XMRIG_ALGO_RANDOMX
#   include "crypto/randomx/randomx.h"
#   include "crypto/rx/Rx.h"
#   include "crypto/rx/RxVm.h"
#endif


#if defined(XMRIG_FEATURE_OPENCL) || defined(XMRIG_FEATURE_CUDA)
#   include "base/tools/Baton.h"
#   include "crypto/cn/CnCtx.h"
#   include "crypto/cn/CnHash.h"
#   include "crypto/cn/CryptoNight.h"
#   include "crypto/common/VirtualMemory.h"
#endif


#include <cassert>
#include <list>
#include <mutex>
#include <uv.h>


namespace xmrig {


#if defined(XMRIG_FEATURE_OPENCL) || defined(XMRIG_FEATURE_CUDA)
class JobBundle
{
public:
    inline JobBundle(const Job &job, uint32_t *results, size_t count) :
        job(job),
        nonces(count)
    {
        memcpy(nonces.data(), results, sizeof(uint32_t) * count);
    }

    Job job;
    std::vector<uint32_t> nonces;
};


class JobBaton : public Baton<uv_work_t>
{
public:
    inline JobBaton(std::list<JobBundle> &&bundles, IJobResultListener *listener, bool hwAES) :
        hwAES(hwAES),
        listener(listener),
        bundles(std::move(bundles))
    {}

    const bool hwAES;
    IJobResultListener *listener;
    std::list<JobBundle> bundles;
    std::vector<JobResult> results;
    uint32_t errors = 0;
};


static inline void checkHash(const JobBundle &bundle, std::vector<JobResult> &results, uint32_t nonce, uint8_t hash[32], uint32_t &errors)
{
    if (*reinterpret_cast<uint64_t*>(hash + 24) < bundle.job.target()) {
        results.emplace_back(bundle.job, nonce, hash);
    }
    else {
        LOG_ERR("COMPUTE ERROR"); // TODO Extend information.
        errors++;
    }
}


static void getResults(JobBundle &bundle, std::vector<JobResult> &results, uint32_t &errors, bool hwAES)
{
    const auto &algorithm = bundle.job.algorithm();
    auto memory           = new VirtualMemory(algorithm.l3(), false, false, false);
    uint8_t hash[32]{ 0 };

    if (algorithm.family() == Algorithm::RANDOM_X) {
#       ifdef XMRIG_ALGO_RANDOMX
        RxDataset *dataset = Rx::dataset(bundle.job, 0);
        if (dataset == nullptr) {
            errors += bundle.nonces.size();

            return;
        }

        auto vm = RxVm::create(dataset, memory->scratchpad(), !hwAES, Assembly::NONE, 0);

        for (uint32_t nonce : bundle.nonces) {
            *bundle.job.nonce() = nonce;

            randomx_calculate_hash(vm, bundle.job.blob(), bundle.job.size(), hash);

            checkHash(bundle, results, nonce, hash, errors);
        }

        RxVm::destroy(vm);
#       endif
    }
    else if (algorithm.family() == Algorithm::ARGON2) {
        errors += bundle.nonces.size(); // TODO ARGON2
    }
    else {
        cryptonight_ctx *ctx[1];
        CnCtx::create(ctx, memory->scratchpad(), memory->size(), 1);

        for (uint32_t nonce : bundle.nonces) {
            *bundle.job.nonce() = nonce;

            CnHash::fn(algorithm, hwAES ? CnHash::AV_SINGLE : CnHash::AV_SINGLE_SOFT, Assembly::NONE)(bundle.job.blob(), bundle.job.size(), hash, ctx, bundle.job.height());

            checkHash(bundle, results, nonce, hash, errors);
        }
    }

    delete memory;
}
#endif


class JobResultsPrivate
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(JobResultsPrivate)

    inline JobResultsPrivate(IJobResultListener *listener, bool hwAES) :
        m_hwAES(hwAES),
        m_listener(listener)
    {
        m_async = new uv_async_t;
        m_async->data = this;

        uv_async_init(uv_default_loop(), m_async, JobResultsPrivate::onResult);
    }


    inline ~JobResultsPrivate()
    {
        Handle::close(m_async);
    }


    inline void submit(const JobResult &result)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_results.push_back(result);

        uv_async_send(m_async);
    }


#   if defined(XMRIG_FEATURE_OPENCL) || defined(XMRIG_FEATURE_CUDA)
    inline void submit(const Job &job, uint32_t *results, size_t count)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_bundles.emplace_back(job, results, count);

        uv_async_send(m_async);
    }
#   endif


private:
    static void onResult(uv_async_t *handle) { static_cast<JobResultsPrivate*>(handle->data)->submit(); }


#   if defined(XMRIG_FEATURE_OPENCL) || defined(XMRIG_FEATURE_CUDA)
    inline void submit()
    {
        std::list<JobBundle> bundles;
        std::list<JobResult> results;

        m_mutex.lock();
        m_bundles.swap(bundles);
        m_results.swap(results);
        m_mutex.unlock();

        for (const auto &result : results) {
            m_listener->onJobResult(result);
        }

        if (bundles.empty()) {
            return;
        }

        auto baton = new JobBaton(std::move(bundles), m_listener, m_hwAES);

        uv_queue_work(uv_default_loop(), &baton->req,
            [](uv_work_t *req) {
                auto baton = static_cast<JobBaton*>(req->data);

                for (JobBundle &bundle : baton->bundles) {
                    getResults(bundle, baton->results, baton->errors, baton->hwAES);
                }
            },
            [](uv_work_t *req, int) {
                auto baton = static_cast<JobBaton*>(req->data);

                for (const auto &result : baton->results) {
                    baton->listener->onJobResult(result);
                }

                delete baton;
            }
        );
    }
#   else
    inline void submit()
    {
        std::list<JobResult> results;

        m_mutex.lock();
        m_results.swap(results);
        m_mutex.unlock();

        for (const auto &result : results) {
            m_listener->onJobResult(result);
        }
    }
#   endif


private:
    const bool m_hwAES;
    IJobResultListener *m_listener;
    std::list<JobResult> m_results;
    std::mutex m_mutex;
    uv_async_t *m_async;

#   if defined(XMRIG_FEATURE_OPENCL) || defined(XMRIG_FEATURE_CUDA)
    std::list<JobBundle> m_bundles;
#   endif
};


static JobResultsPrivate *handler = nullptr;


} // namespace xmrig


void xmrig::JobResults::done(const Job &job)
{
    submit(JobResult(job));
}


void xmrig::JobResults::setListener(IJobResultListener *listener, bool hwAES)
{
    assert(handler == nullptr);

    handler = new JobResultsPrivate(listener, hwAES);
}


void xmrig::JobResults::stop()
{
    assert(handler != nullptr);

    delete handler;

    handler = nullptr;
}


void xmrig::JobResults::submit(const Job &job, uint32_t nonce, const uint8_t *result)
{
    submit(JobResult(job, nonce, result));
}


void xmrig::JobResults::submit(const JobResult &result)
{
    assert(handler != nullptr);

    if (handler) {
        handler->submit(result);
    }
}


#if defined(XMRIG_FEATURE_OPENCL) || defined(XMRIG_FEATURE_CUDA)
void xmrig::JobResults::submit(const Job &job, uint32_t *results, size_t count)
{
    if (handler) {
        handler->submit(job, results, count);
    }
}
#endif
