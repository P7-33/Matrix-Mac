/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2019      Howard Chu  <https://github.com/hyc>
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

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

#include "net/Network.h"
#include "3rdparty/rapidjson/document.h"
#include "backend/common/Tags.h"
#include "base/io/log/Log.h"
#include "base/net/stratum/Client.h"
#include "base/net/stratum/NetworkState.h"
#include "base/net/stratum/SubmitResult.h"
#include "base/tools/Chrono.h"
#include "base/tools/Timer.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "core/Miner.h"
#include "net/JobResult.h"
#include "net/JobResults.h"
#include "net/strategies/DonateStrategy.h"


#ifdef XMRIG_FEATURE_API
#   include "base/api/Api.h"
#   include "base/api/interfaces/IApiRequest.h"
#endif


#include <algorithm>
#include <cinttypes>
#include <ctime>
#include <iterator>
#include <memory>


namespace xmrig {


static const char *tag = BLUE_BG_BOLD(WHITE_BOLD_S " net ");


} // namespace xmrig



const char *xmrig::net_tag()
{
    return tag;
}


xmrig::Network::Network(Controller *controller) :
    m_controller(controller)
{
    JobResults::setListener(this, controller->config()->cpu().isHwAES());
    controller->addListener(this);

#   ifdef XMRIG_FEATURE_API
    controller->api()->addListener(this);
#   endif

    m_state = new NetworkState(this);

    const Pools &pools = controller->config()->pools();
    m_strategy = pools.createStrategy(m_state);

    if (pools.donateLevel() > 0) {
        m_donate = new DonateStrategy(controller, this);
    }

    m_timer = new Timer(this, kTickInterval, kTickInterval);
}


xmrig::Network::~Network()
{
    JobResults::stop();

    delete m_timer;
    delete m_donate;
    delete m_strategy;
    delete m_state;
}


void xmrig::Network::connect()
{
    m_strategy->connect();
}


void xmrig::Network::onActive(IStrategy *strategy, IClient *client)
{
    if (m_donate && m_donate == strategy) {
        LOG_NOTICE("%s " WHITE_BOLD("dev donate started"), tag);
        return;
    }

    const char *tlsVersion = client->tlsVersion();
    LOG_INFO("%s " WHITE_BOLD("use %s ") CYAN_BOLD("%s:%d ") GREEN_BOLD("%s") " " BLACK_BOLD("%s"),
             tag, client->mode(), client->pool().host().data(), client->pool().port(), tlsVersion ? tlsVersion : "", client->ip().data());

    const char *fingerprint = client->tlsFingerprint();
    if (fingerprint != nullptr) {
        LOG_INFO("%s " BLACK_BOLD("fingerprint (SHA-256): \"%s\""), tag, fingerprint);
    }
}


void xmrig::Network::onConfigChanged(Config *config, Config *previousConfig)
{
    if (config->pools() == previousConfig->pools() || !config->pools().active()) {
        return;
    }

    m_strategy->stop();

    config->pools().print();

    delete m_strategy;
    m_strategy = config->pools().createStrategy(m_state);
    connect();
}


void xmrig::Network::onJob(IStrategy *strategy, IClient *client, const Job &job)
{
    if (m_donate && m_donate->isActive() && m_donate != strategy) {
        return;
    }

    setJob(client, job, m_donate == strategy);
}


void xmrig::Network::onJobResult(const JobResult &result)
{
    if (result.index == 1 && m_donate) {
        m_donate->submit(result);
        return;
    }

    m_strategy->submit(result);
}


void xmrig::Network::onLogin(IStrategy *, IClient *client, rapidjson::Document &doc, rapidjson::Value &params)
{
    using namespace rapidjson;
    auto &allocator = doc.GetAllocator();

    Algorithms algorithms     = m_controller->miner()->algorithms();
    const Algorithm algorithm = client->pool().algorithm();
    if (algorithm.isValid()) {
        const size_t index = static_cast<size_t>(std::distance(algorithms.begin(), std::find(algorithms.begin(), algorithms.end(), algorithm)));
        if (index > 0 && index < algorithms.size()) {
            std::swap(algorithms[0], algorithms[index]);
        }
    }

    Value algo(kArrayType);

    for (const auto &a : algorithms) {
        algo.PushBack(StringRef(a.shortName()), allocator);
    }

    params.AddMember("algo", algo, allocator);
}


void xmrig::Network::onPause(IStrategy *strategy)
{
    if (m_donate && m_donate == strategy) {
        LOG_NOTICE("%s " WHITE_BOLD("dev donate finished"), tag);
        m_strategy->resume();
    }

    if (!m_strategy->isActive()) {
        LOG_ERR("%s " RED("no active pools, stop mining"), tag);

        return m_controller->miner()->pause();
    }
}


void xmrig::Network::onResultAccepted(IStrategy *, IClient *, const SubmitResult &result, const char *error)
{
    if (error) {
        LOG_INFO("%s " RED_BOLD("rejected") " (%" PRId64 "/%" PRId64 ") diff " WHITE_BOLD("%" PRIu64) " " RED("\"%s\"") " " BLACK_BOLD("(%" PRIu64 " ms)"),
                 backend_tag(result.backend), m_state->accepted(), m_state->rejected(), result.diff, error, result.elapsed);
    }
    else {
        LOG_INFO("%s " GREEN_BOLD("accepted") " (%" PRId64 "/%" PRId64 ") diff " WHITE_BOLD("%" PRIu64) " " BLACK_BOLD("(%" PRIu64 " ms)"),
                 backend_tag(result.backend), m_state->accepted(), m_state->rejected(), result.diff, result.elapsed);
    }
}


void xmrig::Network::onVerifyAlgorithm(IStrategy *, const IClient *, const Algorithm &algorithm, bool *ok)
{
    if (!m_controller->miner()->isEnabled(algorithm)) {
        *ok = false;

        return;
    }
}


#ifdef XMRIG_FEATURE_API
void xmrig::Network::onRequest(IApiRequest &request)
{
    if (request.type() == IApiRequest::REQ_SUMMARY) {
        request.accept();

        getResults(request.reply(), request.doc(), request.version());
        getConnection(request.reply(), request.doc(), request.version());
    }
}
#endif


void xmrig::Network::setJob(IClient *client, const Job &job, bool donate)
{
    if (job.height()) {
        LOG_INFO("%s " MAGENTA_BOLD("new job") " from " WHITE_BOLD("%s:%d") " diff " WHITE_BOLD("%" PRIu64) " algo " WHITE_BOLD("%s") " height " WHITE_BOLD("%" PRIu64),
                 tag, client->pool().host().data(), client->pool().port(), job.diff(), job.algorithm().shortName(), job.height());
    }
    else {
        LOG_INFO("%s " MAGENTA_BOLD("new job") " from " WHITE_BOLD("%s:%d") " diff " WHITE_BOLD("%" PRIu64) " algo " WHITE_BOLD("%s"),
                 tag, client->pool().host().data(), client->pool().port(), job.diff(), job.algorithm().shortName());
    }

    if (!donate && m_donate) {
        m_donate->setAlgo(job.algorithm());
        m_donate->setProxy(client->pool().proxy());
    }

    m_controller->miner()->setJob(job, donate);
}


void xmrig::Network::tick()
{
    const uint64_t now = Chrono::steadyMSecs();

    m_strategy->tick(now);

    if (m_donate) {
        m_donate->tick(now);
    }
}


#ifdef XMRIG_FEATURE_API
void xmrig::Network::getConnection(rapidjson::Value &reply, rapidjson::Document &doc, int version) const
{
    using namespace rapidjson;
    auto &allocator = doc.GetAllocator();

    reply.AddMember("algo",         m_state->algorithm().toJSON(), allocator);
    reply.AddMember("connection",   m_state->getConnection(doc, version), allocator);
}


void xmrig::Network::getResults(rapidjson::Value &reply, rapidjson::Document &doc, int version) const
{
    using namespace rapidjson;
    auto &allocator = doc.GetAllocator();

    reply.AddMember("results", m_state->getResults(doc, version), allocator);
}
#endif
