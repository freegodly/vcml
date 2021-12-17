/******************************************************************************
 *                                                                            *
 * Copyright 2020 Jan Henrik Weinstock                                        *
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

#ifndef VCML_SYSTEMC_H
#define VCML_SYSTEMC_H

#ifndef SC_INCLUDE_DYNAMIC_PROCESSES
#define SC_INCLUDE_DYNAMIC_PROCESSES
#endif

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include "vcml/common/types.h"
#include "vcml/common/strings.h"
#include "vcml/common/bitops.h"
#include "vcml/common/report.h"
#include "vcml/common/utils.h"

#define SYSTEMC_VERSION_2_3_0a    20120701
#define SYSTEMC_VERSION_2_3_1a    20140417
#define SYSTEMC_VERSION_2_3_2     20171012
#define SYSTEMC_VERSION_2_3_3     20181013

#if SYSTEMC_VERSION < SYSTEMC_VERSION_2_3_1a
inline sc_core::sc_time operator % (const sc_core::sc_time& t1,
                                    const sc_core::sc_time& t2 ) {
    sc_core::sc_time tmp(t1.value() % t2.value(), false);
    return tmp;
}
#endif

#if SYSTEMC_VERSION < SYSTEMC_VERSION_2_3_2
#include <typeindex>
namespace sc_core {
    typedef std::type_index sc_type_index;
}
#endif

namespace vcml {

    using sc_core::sc_object;
    using sc_core::sc_attr_base;

    sc_object*    find_object(const string& name);
    sc_attr_base* find_attribute(const string& name);

    using sc_core::sc_gen_unique_name;
    using sc_core::SC_HIERARCHY_CHAR;

    using sc_core::sc_delta_count;

    using sc_core::sc_time;
    using sc_core::sc_time_unit;
    using sc_core::sc_time_stamp;

    extern const sc_core::sc_time SC_MAX_TIME;

    using sc_core::SC_ZERO_TIME;
    using sc_core::SC_SEC;
    using sc_core::SC_MS;
    using sc_core::SC_US;
    using sc_core::SC_NS;
    using sc_core::SC_PS;

    inline u64 time_to_ns(const sc_time& t) {
        return t.value() / sc_time(1.0, SC_NS).value();
    }

    inline u64 time_to_us(const sc_time& t) {
        return t.value() / sc_time(1.0, SC_US).value();
    }

    inline u64 time_to_ms(const sc_time& t) {
        return t.value() / sc_time(1.0, SC_MS).value();
    }

    inline u64 time_to_sec(const sc_time& t) {
        return t.value() / sc_time(1.0, SC_SEC).value();
    }

    inline u64 time_stamp_ns() {
        return time_to_ns(sc_time_stamp());
    }

    inline u64 time_stamp_us() {
        return time_to_us(sc_time_stamp());
    }

    inline u64 time_stamp_ms() {
        return time_to_ms(sc_time_stamp());
    }

    inline u64 time_stamp_sec() {
        return time_to_sec(sc_time_stamp());
    }

    inline sc_time time_from_value(u64 val) {
#if SYSTEMC_VERSION < SYSTEMC_VERSION_2_3_1a
        return sc_time((sc_dt::uint64)val, false);
#else
        return sc_time::from_value(val);
#endif
    }

    VCML_TYPEINFO(sc_time);

    using sc_core::sc_start;
    using sc_core::sc_stop;

    #if SYSTEMC_VERSION < SYSTEMC_VERSION_2_3_0a
    #   define sc_pause sc_stop
    #else
    using sc_core::sc_pause;
    #endif

    using sc_core::sc_simcontext;
    using sc_core::sc_get_curr_simcontext;

    using sc_core::sc_event;
    using sc_core::sc_process_b;

    using sc_core::sc_actions;
    using sc_core::sc_report;

    using sc_core::sc_port;
    using sc_core::sc_export;
    using sc_core::sc_signal;
    using sc_core::sc_out;
    using sc_core::sc_in;

    using sc_core::sc_module_name;
    using sc_core::sc_module;

    void hierarchy_push(sc_module* mod);
    sc_module* hierarchy_pop();
    sc_module* hierarchy_top();

    template <typename MODULE = sc_core::sc_object>
    inline MODULE* hierarchy_search(sc_object* start = nullptr) {
        if (start == nullptr)
            start = hierarchy_top();

        for (sc_object* obj = start; obj; obj = obj->get_parent_object()) {
            MODULE* module = dynamic_cast<MODULE*>(obj);
            if (module)
                return module;
        }

        return nullptr;
    }

    class hierarchy_guard
    {
    private:
        sc_module* m_owner;

    public:
        hierarchy_guard(sc_module* owner):
            m_owner(owner ? owner : hierarchy_top()) {
            hierarchy_push(m_owner);
        }

        hierarchy_guard(sc_object* obj):
            hierarchy_guard(hierarchy_search<sc_module>(obj)) {
        }

        ~hierarchy_guard() {
            sc_module* top = hierarchy_pop();
            VCML_ERROR_ON(top != m_owner, "SystemC hierarchy corrupted");
        }
    };

    using sc_core::sc_spawn;
    using sc_core::sc_spawn_options;
    using sc_core::sc_type_index;

    using tlm::tlm_global_quantum;
    using tlm::tlm_generic_payload;
    using tlm::tlm_dmi;
    using tlm::tlm_extension;
    using tlm::tlm_extension_base;

    using tlm::tlm_command;
    using tlm::TLM_READ_COMMAND;
    using tlm::TLM_WRITE_COMMAND;
    using tlm::TLM_IGNORE_COMMAND;

    using tlm::tlm_response_status;
    using tlm::TLM_OK_RESPONSE;
    using tlm::TLM_INCOMPLETE_RESPONSE;
    using tlm::TLM_GENERIC_ERROR_RESPONSE;
    using tlm::TLM_ADDRESS_ERROR_RESPONSE;
    using tlm::TLM_COMMAND_ERROR_RESPONSE;
    using tlm::TLM_BURST_ERROR_RESPONSE;
    using tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE;

    template <typename T>
    inline bool success(const T& t) {
        return true;
    }

    template <typename T>
    inline bool failed(const T& t) {
        return false;
    }

    inline bool success(tlm_response_status status) {
        return status > TLM_INCOMPLETE_RESPONSE;
    }

    inline bool failed(tlm_response_status status) {
        return status < TLM_INCOMPLETE_RESPONSE;
    }

    template <> inline bool success(const tlm_generic_payload& tx) {
        return success(tx.get_response_status());
    }

    template <> inline bool failed(const tlm_generic_payload& tx) {
        return failed(tx.get_response_status());
    }

    inline void tx_setup(tlm_generic_payload& tx, tlm_command cmd, u64 addr,
        void* data, unsigned int size) {
        tx.set_command(cmd);
        tx.set_address(addr);
        tx.set_data_ptr(reinterpret_cast<unsigned char*>(data));
        tx.set_data_length(size);
        tx.set_streaming_width(size);
        tx.set_byte_enable_ptr(nullptr);
        tx.set_byte_enable_length(0);
        tx.set_response_status(TLM_INCOMPLETE_RESPONSE);
        tx.set_dmi_allowed(false);
    }

    inline u64 tx_size(const tlm_generic_payload& tx) {
        u64 size = tx.get_streaming_width();
        return size > 0 ? size : tx.get_data_length();
    }

    inline u64 tx_width(const tlm_generic_payload& tx) {
        return ffs((u64)tx.get_address() + tx_size(tx));
    }

    const char* tlm_response_to_str(tlm_response_status status);
    string tlm_transaction_to_str(const tlm_generic_payload& tx);

    inline vcml_access tlm_command_to_access(tlm_command c) {
        switch (c) {
        case TLM_READ_COMMAND: return VCML_ACCESS_READ;
        case TLM_WRITE_COMMAND: return VCML_ACCESS_WRITE;
        case TLM_IGNORE_COMMAND: return VCML_ACCESS_NONE;
        default:
            VCML_ERROR("illegal TLM command %d", (int)c);
        }
    }

    inline tlm_command tlm_command_from_access(vcml_access acs) {
        switch (acs) {
        case VCML_ACCESS_NONE: return TLM_IGNORE_COMMAND;
        case VCML_ACCESS_READ: return TLM_READ_COMMAND;
        case VCML_ACCESS_WRITE: return TLM_WRITE_COMMAND;
        case VCML_ACCESS_READ_WRITE: return TLM_WRITE_COMMAND;
        default:
            VCML_ERROR("illegal access mode: %d", (int)acs);
        }
    }

    using tlm_utils::simple_initiator_socket;
    using tlm_utils::simple_initiator_socket_tagged;
    using tlm_utils::simple_target_socket;
    using tlm_utils::simple_target_socket_tagged;

#define VCML_KIND(name)                         \
    virtual const char* kind() const override { \
        return "vcml::" #name;                  \
    }

    bool kernel_has_phase_callbacks();

    void on_end_of_elaboration(function<void(void)> callback);
    void on_start_of_simulation(function<void(void)> callback);

    void on_each_delta_cycle(function<void(void)> callback);
    void on_each_time_step(function<void(void)> callback);

    class timer
    {
    public:
        struct event {
            timer* owner;
            sc_time timeout;
        };

        size_t count() const { return m_triggers; }
        const sc_time& timeout() const { return m_timeout; }

        timer(function<void(timer&)> cb);
        timer(const sc_time& delta, function<void(timer&)> cb):
            timer(cb) { reset(delta); }
        timer(double t, sc_time_unit tu, function<void(timer&)> cb):
            timer(cb) { reset(t, tu); }
        ~timer();

        void trigger();
        void cancel();

        void reset(double t, sc_time_unit tu) { reset(sc_time(t, tu)); }
        void reset(const sc_time& delta);

    private:
        size_t m_triggers;
        sc_time m_timeout;
        event* m_event;
        function<void(timer&)> m_cb;
    };

    void sc_async(function<void(void)> job);
    void sc_progress(const sc_time& delta);
    void sc_sync(function<void(void)> job);

    bool sc_is_async();

    bool is_thread(sc_process_b* proc = nullptr);
    bool is_method(sc_process_b* proc = nullptr);

    sc_process_b* current_process();
    sc_process_b* current_thread();
    sc_process_b* current_method();

    bool sim_running();

    template <typename SOCKET, const size_t LIMIT = SIZE_MAX>
    class socket_array
    {
    public:
        typedef unordered_map<size_t, SOCKET*> map_type;
        typedef unordered_map<const SOCKET*, size_t> revmap_type;
        typedef typename map_type::iterator iterator;
        typedef typename map_type::const_iterator const_iterator;

    private:
        string m_name;
        size_t m_next;
        address_space m_space;
        sc_module* m_parent;
        map_type m_sockets;
        revmap_type m_ids;

        SOCKET& lookup(size_t idx) {
            SOCKET*& socket = m_sockets[idx];
            if (socket)
                return *socket;

            VCML_ERROR_ON(idx >= LIMIT, "socket out of bounds: %zu", idx);
            hierarchy_guard guard(m_parent);
            string nm = mkstr("%s[%zu]", m_name.c_str(), idx);
            socket = new SOCKET(nm.c_str(), m_space);
            m_ids[socket] = idx;
            return *socket;
        }

    public:
        const char* name() const { return m_name.c_str(); }

        socket_array(const char* nm, address_space as = VCML_AS_DEFAULT):
            m_name(nm), m_next(0), m_space(as),
            m_parent(hierarchy_top()), m_sockets() {
            VCML_ERROR_ON(!m_parent, "port_array outside sc_module");
        }

        virtual ~socket_array() {
            for (auto socket : m_sockets)
                delete socket.second;
        }

        iterator begin() { return m_sockets.begin(); }
        iterator end()   { return m_sockets.end(); }

        const_iterator begin() const { return m_sockets.cbegin(); }
        const_iterator end()   const { return m_sockets.cend(); }

        SOCKET& operator[] (size_t idx) { return lookup(idx); }
        const SOCKET& operator[] (size_t idx) const {
            VCML_ERROR_ON(!exists(idx), "socket %zu not found", idx);
            return *m_sockets.at(idx);
        }

        const char* name() { return m_name.c_str(); }
        size_t count() const { return m_sockets.size(); }
        bool exists(size_t idx) const { return stl_contains(m_sockets, idx); }
        size_t next_index() const { return m_next; }
        SOCKET& next() { return operator [] (next_index()); }

        size_t index_of(const SOCKET& socket) const {
            auto it = m_ids.find(&socket);
            if (it == m_ids.end())
                VCML_ERROR("socket %s not part of %s", socket.name(), name());
            return it->second;
        }

        set<size_t> all_keys() const {
            set<size_t> keys;
            for (const auto& socket : m_sockets)
                keys.insert(socket.first);
            return keys;
        }
    };

}

namespace sc_core {
    std::istream& operator >> (std::istream& is, sc_core::sc_time& t);
}

namespace tlm {
    std::ostream& operator << (std::ostream& os, tlm_response_status sts);
    std::ostream& operator << (std::ostream& os, const tlm_generic_payload&);
}

#endif
