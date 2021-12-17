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

#include "vcml/common/systemc.h"
#include "vcml/common/thctl.h"

namespace vcml {

    sc_object* find_object(const string& name) {
        return sc_core::sc_find_object(name.c_str());
    }

    sc_attr_base* find_attribute(const string& name) {
        size_t pos = name.rfind(SC_HIERARCHY_CHAR);
        if (pos == string::npos)
            return nullptr;

        sc_object* parent = find_object(name.substr(0, pos));
        if (parent == nullptr)
            return nullptr;

        return parent->get_attribute(name);
    }

    const sc_time SC_MAX_TIME = time_from_value(~0ull);

    void hierarchy_push(sc_module* mod) {
        sc_simcontext* simc = sc_get_curr_simcontext();
        VCML_ERROR_ON(!simc, "no simulation context");
        simc->hierarchy_push(mod);
    }

    sc_module* hierarchy_pop() {
        sc_simcontext* simc = sc_get_curr_simcontext();
        VCML_ERROR_ON(!simc, "no simulation context");
        return simc->hierarchy_pop();
    }

    sc_module* hierarchy_top() {
        sc_simcontext* simc = sc_get_curr_simcontext();
        VCML_ERROR_ON(!simc, "no simulation context");
        return simc->hierarchy_curr();
    }

    const char* tlm_response_to_str(tlm_response_status status) {
        switch (status) {
        case TLM_OK_RESPONSE:
            return "TLM_OK_RESPONSE";

        case TLM_INCOMPLETE_RESPONSE:
            return "TLM_INCOMPLETE_RESPONSE";

        case TLM_GENERIC_ERROR_RESPONSE:
            return "TLM_GENERIC_ERROR_RESPONSE";

        case TLM_ADDRESS_ERROR_RESPONSE:
            return "TLM_ADDRESS_ERROR_RESPONSE";

        case TLM_COMMAND_ERROR_RESPONSE:
            return "TLM_COMMAND_ERROR_RESPONSE";

        case TLM_BURST_ERROR_RESPONSE:
            return "TLM_BURST_ERROR_RESPONSE";

        case TLM_BYTE_ENABLE_ERROR_RESPONSE:
            return "TLM_BYTE_ENABLE_ERROR_RESPONSE";

        default:
            return "TLM_UNKNOWN_RESPONSE";
        }
    }

    string tlm_transaction_to_str(const tlm_generic_payload& tx) {
        stringstream ss;

        // command
        switch (tx.get_command()) {
        case TLM_READ_COMMAND  : ss << "RD "; break;
        case TLM_WRITE_COMMAND : ss << "WR "; break;
        default: ss << "IG "; break;
        }

        // address
        ss << "0x";
        ss << std::hex;
        ss.width(tx.get_address() > ~0u ? 16 : 8);
        ss.fill('0');
        ss << tx.get_address();

        // data array
        unsigned int size = tx.get_data_length();
        unsigned char* c = tx.get_data_ptr();

        ss << " [";
        if (size == 0)
            ss << "<no data>";
        for (unsigned int i = 0; i < size; i++) {
            ss.width(2);
            ss.fill('0');
            ss << static_cast<unsigned int>(*c++);
            if (i != (size - 1))
                ss << " ";
        }
        ss << "]";

        // response status
        ss << " (" << tx.get_response_string() << ")";

        // ToDo: byte enable, streaming, etc.
        return ss.str();
    }

#if SYSTEMC_VERSION >= SYSTEMC_VERSION_2_3_1a
    static inline bool __kernel_has_phase_callbacks() {
        class callbacks_tester: public sc_object
        {
        public:
            sc_actions previous;
            callbacks_tester(): sc_object("$$$vcml_phase_callback_tester$$$"),
                previous(sc_core::sc_report_handler::set_actions(
                    sc_core::SC_ID_PHASE_CALLBACKS_UNSUPPORTED_,
                    sc_core::SC_THROW)) {
                register_simulation_phase_callback(sc_core::SC_END_OF_UPDATE);
            }

            ~callbacks_tester() {
                sc_core::sc_report_handler::set_actions(
                    sc_core::SC_ID_PHASE_CALLBACKS_UNSUPPORTED_, previous);
            }
        };

        try {
            callbacks_tester t;
            return true;
        } catch (sc_core::sc_report& r) {
            return false;
        }
    }
#endif

    bool kernel_has_phase_callbacks() {
#if SYSTEMC_VERSION < SYSTEMC_VERSION_2_3_1a
        return false;
#else
        static bool has_callbacks = __kernel_has_phase_callbacks();
        return has_callbacks;
#endif
    }

    // we just need this class to have something that is called every cycle...
    class helper_module: public sc_core::sc_trace_file, private sc_module
    {
    public:
    #define DECL_TRACE_METHOD_A(t) \
        virtual void trace(const t& object, const string& n) override {}

    #define DECL_TRACE_METHOD_B(t) \
        virtual void trace(const t& object, const string& n, int w) override {}

#if SYSTEMC_VERSION >= SYSTEMC_VERSION_2_3_2
        DECL_TRACE_METHOD_A(sc_event)
        DECL_TRACE_METHOD_A(sc_time)
#endif

        DECL_TRACE_METHOD_A(bool)
        DECL_TRACE_METHOD_A(sc_dt::sc_bit)
        DECL_TRACE_METHOD_A(sc_dt::sc_logic)

        DECL_TRACE_METHOD_B(unsigned char)
        DECL_TRACE_METHOD_B(unsigned short)
        DECL_TRACE_METHOD_B(unsigned int)
        DECL_TRACE_METHOD_B(unsigned long)
        DECL_TRACE_METHOD_B(char)
        DECL_TRACE_METHOD_B(short)
        DECL_TRACE_METHOD_B(int)
        DECL_TRACE_METHOD_B(long)
        DECL_TRACE_METHOD_B(sc_dt::int64)
        DECL_TRACE_METHOD_B(sc_dt::uint64)

        DECL_TRACE_METHOD_A(float)
        DECL_TRACE_METHOD_A(double)
        DECL_TRACE_METHOD_A(sc_dt::sc_int_base)
        DECL_TRACE_METHOD_A(sc_dt::sc_uint_base)
        DECL_TRACE_METHOD_A(sc_dt::sc_signed)
        DECL_TRACE_METHOD_A(sc_dt::sc_unsigned)

        DECL_TRACE_METHOD_A(sc_dt::sc_fxval)
        DECL_TRACE_METHOD_A(sc_dt::sc_fxval_fast)
        DECL_TRACE_METHOD_A(sc_dt::sc_fxnum)
        DECL_TRACE_METHOD_A(sc_dt::sc_fxnum_fast)

        DECL_TRACE_METHOD_A(sc_dt::sc_bv_base)
        DECL_TRACE_METHOD_A(sc_dt::sc_lv_base)

    #undef DECL_TRACE_METHOD_A
    #undef DECL_TRACE_METHOD_B

        virtual void trace(const unsigned int& object,
                           const std::string& name,
                           const char** enum_literals ) override {}
        virtual void write_comment(const std::string& comment) override {}
        virtual void set_time_unit(double v,
                                   sc_core::sc_time_unit tu) override {}

        const bool use_phase_callbacks;

        vector<function<void(void)>> end_of_elab;
        vector<function<void(void)>> start_of_sim;

        vector<function<void(void)>> deltas;
        vector<function<void(void)>> tsteps;

        struct timer_compare {
            inline bool
            operator() (const timer::event* a, const timer::event* b) const {
                return a->timeout > b->timeout;
            }
        };

        sc_event timeout_event;
        std::priority_queue<timer::event*, vector<timer::event*>,
                            timer_compare> timers;

        void update_timer() {
            if (timers.empty()) {
                timeout_event.cancel();
                return;
            }

            sc_time next_timeout = timers.top()->timeout;
            timeout_event.notify(next_timeout - sc_time_stamp());
        }

        void run_timer() {
            const sc_time now = sc_time_stamp();
            while (!timers.empty()) {
                timer::event* event = timers.top();
                if (event->timeout > now)
                    break;

                if (event->timeout < now)
                    VCML_ERROR("missed timer event");

                timers.pop();
                if (event->owner)
                    event->owner->trigger();
                delete event;
            }

            update_timer();
        }

        void add_timer(timer::event* ev) {
            thctl_guard guard;
            timers.push(ev);
            update_timer();
        }

        helper_module(const sc_module_name& nm):
            sc_core::sc_trace_file(),
            sc_core::sc_module(nm),
            use_phase_callbacks(kernel_has_phase_callbacks()),
            end_of_elab(), start_of_sim(), deltas(), tsteps(),
            timeout_event("timeout_ev"), timers() {
#if SYSTEMC_VERSION >= SYSTEMC_VERSION_2_3_1a
            if (use_phase_callbacks) {
                register_simulation_phase_callback(
                    sc_core::SC_END_OF_UPDATE | sc_core::SC_BEFORE_TIMESTEP);
            }
#endif
            if (!use_phase_callbacks)
                simcontext()->add_trace_file(this);

            SC_HAS_PROCESS(helper_module);
            SC_METHOD(run_timer);
            sensitive << timeout_event;
            dont_initialize();
        }

#if SYSTEMC_VERSION >= SYSTEMC_VERSION_2_3_1a
        virtual void simulation_phase_callback() override {
            switch (simcontext()->get_status()) {
            case sc_core::SC_END_OF_UPDATE:
                cycle(true);
                break;
            case sc_core::SC_BEFORE_TIMESTEP:
                cycle(false);
                break;

            default:
                break;
            }
        }
#endif

        virtual ~helper_module() {
#if SYSTEMC_VERSION >= SYSTEMC_VERSION_2_3_1a
            if (!use_phase_callbacks)
                sc_get_curr_simcontext()->remove_trace_file(this);
#endif
        }

        static helper_module& instance() {
            static helper_module helper("$$$vcml_helper_module$$$");
            return helper;
        }

    protected:
        virtual void cycle(bool delta_cycle) override {
            if (delta_cycle) {
                for (auto& func : deltas)
                    func();
            } else {
                for (auto& func : tsteps)
                    func();
            }
        }

        virtual void end_of_elaboration() override {
            for (auto& func : end_of_elab)
                func();
        }

        virtual void start_of_simulation() override {
            for (auto& func : start_of_sim)
                func();
        }
    };

    void on_end_of_elaboration(function<void(void)> callback) {
        helper_module& helper = helper_module::instance();
        helper.end_of_elab.push_back(callback);
    }

    void on_start_of_simulation(function<void(void)> callback) {
        helper_module& helper = helper_module::instance();
        helper.start_of_sim.push_back(callback);
    }

    void on_each_delta_cycle(function<void(void)> callback) {
        helper_module& helper = helper_module::instance();
        helper.deltas.push_back(callback);
    }

    void on_each_time_step(function<void(void)> callback) {
        helper_module& helper = helper_module::instance();
        helper.tsteps.push_back(callback);
    }

    timer::timer(function<void(timer&)> cb):
        m_triggers(0), m_timeout(), m_event(nullptr), m_cb(std::move(cb)) {
    }

    timer::~timer() {
        cancel();
    }

    void timer::trigger() {
        m_event = nullptr;
        m_triggers++;
        m_cb(*this);
    }

    void timer::cancel() {
        if (m_event) {
            m_event->owner = nullptr;
            m_event = nullptr;
        }
    }

    void timer::reset(const sc_time& delta) {
        cancel();

        m_event = new event;
        m_event->owner = this;
        m_event->timeout = m_timeout = sc_time_stamp() + delta;
        helper_module::instance().add_timer(m_event);
    }

    __thread struct async_worker* g_async = nullptr;

    struct async_worker
    {
        const size_t id;
        sc_process_b* const process;

        atomic<bool> alive;
        atomic<bool> working;
        function<void(void)> task;

        atomic<u64> progress;
        atomic<function<void(void)>*> request;

        mutex mtx;
        condition_variable_any notify;
        thread worker;

        async_worker(size_t _id, sc_process_b* _proc):
            id(_id),
            process(_proc),
            alive(true),
            working(false),
            task(),
            progress(0),
            request(nullptr),
            mtx(),
            notify(),
            worker(std::bind(&async_worker::work, this)) {
            VCML_ERROR_ON(!process, "invalid parent process");
            set_thread_name(worker, mkstr("vcml_async_%zu", id));
        }

        ~async_worker() {
            if (worker.joinable()) {
                alive = false;
                notify.notify_all();
                worker.join();
            }
        }

        void work() {
            g_async = this;

            mtx.lock();
            while (alive) {
                while (alive && !working)
                    notify.wait(mtx);

                if (!alive)
                    break;

                task();
                working = false;
            }

            g_async = nullptr;
        }

        static void yield() {
#if defined(__x86_64__)
            asm volatile ("pause" ::: "memory");
#elif defined(__aarch64__)
            asm volatile ("yield" ::: "memory");
#endif
        }

        void run_async(function<void(void)>& job) {
            mtx.lock();
            task = job;
            working = true;
            mtx.unlock();
            notify.notify_all();

            while (working) {
                u64 p = progress.exchange(0);
                sc_core::wait(time_from_value(p));

                if (request) {
                    (*request)();
                    request = nullptr;
                }
            }

            u64 p = progress.exchange(0);
            if (p > 0)
                sc_core::wait(time_from_value(p));
        }

        void run_sync(function<void(void)> job) {
            g_async->request = &job;
            while (g_async->request)
                yield();
        }

        static async_worker& lookup(sc_process_b* thread) {
            VCML_ERROR_ON(!thread, "invalid thread");

            typedef unordered_map<sc_process_b*, shared_ptr<async_worker>> map;
            static map workers;

            auto it = workers.find(thread);
            if (it != workers.end())
                return *it->second.get();

            size_t id = workers.size();
            auto worker = std::make_shared<async_worker>(id, thread);
            return *(workers[thread] = worker);
        }
    };

    void sc_async(function<void(void)> job) {
        auto thread = current_thread();
        VCML_ERROR_ON(!thread, "sc_async must be called from SC_THREAD");
        async_worker& worker = async_worker::lookup(thread);
        worker.run_async(job);
    }

    void sc_progress(const sc_time& delta) {
        VCML_ERROR_ON(!g_async, "no async thread to progress");
        g_async->progress += delta.value();
    }

    void sc_sync(function<void(void)> job) {
        if (thctl_is_sysc_thread()) {
           job();
        } else if (g_async != nullptr) {
            g_async->run_sync(job);
        } else {
            VCML_ERROR("not on systemc or async thread");
        }
    }

    bool sc_is_async() {
        return g_async != nullptr;
    }

    bool is_thread(sc_process_b* proc) {
        if (!thctl_is_sysc_thread())
            return false;
        if (proc == nullptr)
            proc = current_process();
        if (proc == nullptr)
            return false;
        return proc->proc_kind() == sc_core::SC_THREAD_PROC_;
    }

    bool is_method(sc_process_b* proc) {
        if (!thctl_is_sysc_thread())
            return false;
        if (proc == nullptr)
            proc = current_process();
        if (proc == nullptr)
            return false;
        return proc->proc_kind() == sc_core::SC_METHOD_PROC_;
    }

    sc_process_b* current_process() {
        if (g_async)
            return g_async->process;
        if (!thctl_is_sysc_thread())
            return nullptr;
        return sc_core::sc_get_current_process_b();
    }

    sc_process_b* current_thread() {
        sc_process_b* proc = current_process();
        if (proc == nullptr || proc->proc_kind() != sc_core::SC_THREAD_PROC_)
            return nullptr;
        return proc;
    }

    sc_process_b* current_method() {
        sc_process_b* proc = current_process();
        if (proc == nullptr || proc->proc_kind() != sc_core::SC_METHOD_PROC_)
            return nullptr;
        return proc;
    }

    bool sim_running() {
        sc_simcontext* simc = sc_get_curr_simcontext();
        switch (simc->get_status()) {
#if SYSTEMC_VERSION >= SYSTEMC_VERSION_2_3_1a
        case sc_core::SC_END_OF_UPDATE: return true;
        case sc_core::SC_BEFORE_TIMESTEP: return true;
#endif
        default:
            return simc->get_status() < sc_core::SC_STOPPED;
        }
    }

}

namespace sc_core {

    std::istream& operator >> (std::istream& is, sc_time& t) {
        std::string str; is >> str;
        str = vcml::to_lower(str);

        char* endptr = nullptr;
        sc_dt::uint64 value = strtoul(str.c_str(), &endptr, 0);
        double fval = value;

        if (strcmp(endptr, "ps") == 0)
            t = sc_time(fval, sc_core::SC_PS);
        else if (strcmp(endptr, "ns") == 0)
            t = sc_time(fval, sc_core::SC_NS);
        else if (strcmp(endptr, "us") == 0)
            t = sc_time(fval, sc_core::SC_US);
        else if (strcmp(endptr, "ms") == 0)
            t = sc_time(fval, sc_core::SC_MS);
        else if (strcmp(endptr, "s") == 0)
            t = sc_time(fval, sc_core::SC_SEC);
        else // raw value, not part of ieee1666!
            t = sc_time(value, true);

        return is;
    }

}

namespace tlm {

    std::ostream& operator << (std::ostream& os, tlm_response_status status) {
        os << ::vcml::tlm_response_to_str(status);
        return os;
    }

    std::ostream& operator << (std::ostream& os,
        const tlm_generic_payload& payload) {
        os << ::vcml::tlm_transaction_to_str(payload);
        return os;
    }

}
