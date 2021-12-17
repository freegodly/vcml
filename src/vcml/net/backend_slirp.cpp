/******************************************************************************
 *                                                                            *
 * Copyright 2021 Jan Henrik Weinstock                                        *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *     http://www.apache.org/licenses/LICENSE-2.0                             *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 *                                                                            *
 ******************************************************************************/

#include "vcml/net/backend_slirp.h"

#include <poll.h>

namespace vcml { namespace net {

    static struct in_addr ipaddr(const string& str) {
        struct in_addr addr;
        if (inet_pton(AF_INET, str.c_str(), &addr) != 1)
            VCML_ERROR("cannot parse ipv4 address: %s", str.c_str());
        return addr;
    }

    static struct in_addr ipaddr(const string& format, int val) {
        return ipaddr(mkstr(format.c_str(), val));
    }

    static struct in6_addr ipaddr6(const string& str) {
        struct in6_addr addr6;
        if (inet_pton(AF_INET6, str.c_str(), &addr6) != 1)
            VCML_ERROR("cannot parse ipv6 address: %s", str.c_str());
        return addr6;
    }

    static struct in6_addr ipaddr6(const string& format, int val) {
        return ipaddr6(mkstr(format.c_str(), val));
    }

    static int slirp_add_poll_fd(int fd, int events, void* opaque) {
        pollfd request;
        request.fd = fd;
        request.events = 0;
        request.revents = 0;

        if (events & SLIRP_POLL_IN)
            request.events |= POLLIN;
        if (events & SLIRP_POLL_OUT)
            request.events |= POLLOUT;
        if (events & SLIRP_POLL_PRI)
            request.events |= POLLPRI;
        if (events & SLIRP_POLL_ERR)
            request.events |= POLLERR;
        if (events & SLIRP_POLL_HUP)
            request.events |= POLLHUP;

        vector<pollfd>* requests = (vector<pollfd>*)opaque;
        requests->push_back(request);
        return requests->size() - 1;
    }

    static int slirp_get_events(int idx, void* opaque) {
        vector<pollfd>* requests = (vector<pollfd>*)opaque;
        int events = 0, revents = requests->at(idx).revents;
        if (revents & POLLIN)
            events |= SLIRP_POLL_IN;
        if (revents & POLLOUT)
            events |= SLIRP_POLL_OUT;
        if (revents & POLLPRI)
            events |= SLIRP_POLL_PRI;
        if (revents & POLLERR)
            events |= SLIRP_POLL_ERR;
        if (revents & POLLHUP)
            events |= SLIRP_POLL_HUP;
        return events;
    }

    static ssize_t slirp_send(const void* buf, size_t len, void* opaque) {
        slirp_network* network = (slirp_network*)opaque;
        network->send_packet((const u8*)buf, len);
        return (ssize_t)len;
    }

    static void slirp_error(const char *msg, void *opaque) {
        log_error("%s", msg);
    }

    static int64_t slirp_clock_ns(void* opaque) {
        return time_stamp_ns();
    }

    static void* slirp_timer_new(SlirpTimerCb cb, void* obj, void* opaque) {
        return new timer([cb, obj](timer& t) -> void { (*cb)(obj); });
    }

    static void slirp_timer_free(void* t, void* opaque) {
        if (t) delete (timer*)t;
    }

    static void slirp_timer_mod(void* t, int64_t expire_time, void* opaque) {
        ((timer*)t)->reset(expire_time, SC_MS);
    }

    static void slirp_register_poll_fd(int fd, void* opaque) {
        // nothing to do
    }

    static void slirp_unregister_poll_fd(int fd, void* opaque) {
        // nothing to do
    }

    static void slirp_notify(void* opaque) {
        // nothing to do
    }

    static const SlirpCb slirp_cbs = {
        /* send_packet        = */ slirp_send,
        /* guest_error        = */ slirp_error,
        /* clock_get_ns       = */ slirp_clock_ns,
        /* timer_new          = */ slirp_timer_new,
        /* timer_free         = */ slirp_timer_free,
        /* timer_mod          = */ slirp_timer_mod,
        /* register_poll_fd   = */ slirp_register_poll_fd,
        /* unregister_poll_fd = */ slirp_unregister_poll_fd,
        /* notify             = */ slirp_notify,
    };

    void slirp_network::slirp_thread() {
        while (m_running) {
            unsigned int timeout = 10; // ms
            vector<pollfd> fds;

            slirp_pollfds_fill(m_slirp, &timeout, &slirp_add_poll_fd, &fds);
            if (fds.empty()) {
                usleep(timeout * 1000);
                continue;
            }

            int ret = poll(fds.data(), fds.size(), timeout);
            if (ret != 0)
                slirp_pollfds_poll(m_slirp, ret < 0, &slirp_get_events, &fds);
        }
    }

    slirp_network::slirp_network(unsigned int id):
        m_config(),
        m_slirp(),
        m_clients(),
        m_running(true),
        m_thread() {
        m_config.version = 1;

        m_config.in_enabled  = true;
        m_config.vnetwork    = ipaddr("10.0.%u.0", id);
        m_config.vnetmask    = ipaddr("255.255.255.0");
        m_config.vhost       = ipaddr("10.0.%u.2", id);
        m_config.vdhcp_start = ipaddr("10.0.%u.15", id);
        m_config.vnameserver = ipaddr("10.0.%u.3", id);

        m_config.in6_enabled   = true;
        m_config.vprefix_addr6 = ipaddr6("%x::", 0xfec0 + id);
        m_config.vhost6        = ipaddr6("%x::2", 0xfec0 + id);
        m_config.vnameserver6  = ipaddr6("%x::3", 0xfec0 + id);
        m_config.vprefix_len   = 64;

        m_config.vhostname = nullptr;
        m_config.tftp_server_name = nullptr;
        m_config.tftp_path = nullptr;
        m_config.bootfile = nullptr;
        m_config.vdnssearch = nullptr;
        m_config.vdomainname = nullptr;

        m_config.if_mtu = 0; // IF_MTU_DEFAULT
        m_config.if_mru = 0; // IF_MRU_DEFAULT
        m_config.disable_host_loopback = false;
        m_config.enable_emu = false;
        m_config.restricted = false;

        m_slirp = slirp_new(&m_config, &slirp_cbs, this);
        VCML_REPORT_ON(!m_slirp, "failed to initialize SLIRP");

        if (m_config.in_enabled)
            log_debug("created slirp ipv4 network 10.0.%u.0/24", id);
        if (m_config.in6_enabled)
            log_debug("created slirp ipv6 network %04x::", 0xfec0 + id);

        m_thread = thread(std::bind(&slirp_network::slirp_thread, this));
        set_thread_name(m_thread, mkstr("slirp_thread_%u", id));
    }

    slirp_network::~slirp_network() {
        m_running = false;
        if (m_thread.joinable())
            m_thread.join();

        for (auto client : m_clients)
            client->disconnect();

        if (m_slirp)
            slirp_cleanup(m_slirp);
    }

    void slirp_network::send_packet(const u8* ptr, size_t len) {
        auto packet = std::make_shared<vector<u8>>(ptr, ptr + len);
        for (auto client : m_clients)
            client->queue_packet(packet);
    }

    void slirp_network::recv_packet(const u8* ptr, size_t len) {
        slirp_input(m_slirp, ptr, len);
    }

    void slirp_network::register_client(backend_slirp* client) {
        m_clients.insert(client);
    }

    void slirp_network::unregister_client(backend_slirp* client) {
        m_clients.erase(client);
    }

    backend_slirp::backend_slirp(const string& adapter,
        const shared_ptr<slirp_network>& network):
        backend(adapter),
        m_network(network) {
        VCML_ERROR_ON(!m_network, "no network");
        m_network->register_client(this);
    }

    backend_slirp::~backend_slirp() {
        if (m_network)
            m_network->unregister_client(this);
    }

    void backend_slirp::send_packet(const vector<u8>& packet) {
        if (m_network)
            m_network->recv_packet(packet.data(), packet.size());
    }

    backend* backend_slirp::create(const string& adapter, const string& type) {
        unsigned int netid = 0;
        if (sscanf(type.c_str(), "slirp:%u", &netid) != 1)
            netid = 0;

        static unordered_map<unsigned int, shared_ptr<slirp_network>> networks;
        auto& network = networks[netid];
        if (network == nullptr)
            network = std::make_shared<slirp_network>(netid);
        return new backend_slirp(adapter, network);
    }

}}
