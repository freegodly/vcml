/******************************************************************************
 *                                                                            *
 * Copyright 2018 Jan Henrik Weinstock                                        *
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

#ifndef VCML_LOGGER_H
#define VCML_LOGGER_H

#include "vcml/common/types.h"
#include "vcml/common/strings.h"
#include "vcml/common/utils.h"
#include "vcml/common/report.h"
#include "vcml/common/systemc.h"

namespace vcml {

    enum log_level {
        LOG_ERROR = 0,
        LOG_WARN,
        LOG_INFO,
        LOG_DEBUG,
        LOG_TRACE,
        NUM_LOG_LEVELS
    };

    enum trace_direction : int {
        TRACE_FW = 1,
        TRACE_FW_NOINDENT = 2,
        TRACE_BW = -1,
        TRACE_BW_NOINDENT = -2,
    };

    VCML_TYPEINFO(log_level);

    ostream& operator << (ostream& os, const log_level& lvl);
    istream& operator >> (istream& is, log_level& lvl);

    struct logmsg {
        log_level level;
        sc_time   time;
        sc_time   time_offset;
        u64       cycle;
        string    sender;

        struct {
            const char* file;
            int         line;

        } source;

        vector<string> lines;

        logmsg(log_level _level, const string& _sender);
    };

    template <typename PAYLOAD>
    struct trace_msg : public logmsg {
        trace_direction direction;
        const PAYLOAD& payload;

        trace_msg(const string& _sender, trace_direction _direction,
                  const PAYLOAD& _payload):
            logmsg(LOG_TRACE, _sender),
            direction(_direction),
            payload(_payload) {
        }
    };

    ostream& operator << (ostream& os, const logmsg& msg);

    typedef function<bool(const logmsg& msg)> log_filter;

    class logger
    {
    private:
        log_level m_min;
        log_level m_max;

        vector<log_filter> m_filters;

        void register_logger();
        void unregister_logger();

        bool check_filters(const logmsg& msg) const;

        // disabled
        logger(const logger&);
        logger& operator = (const logger&);

        static size_t trace_curr_indent;
        static vector<logger*> loggers[NUM_LOG_LEVELS];

    public:
        inline void set_level(log_level max);
        void set_level(log_level min, log_level max);

        void filter(log_filter filter);
        void filter_time(const sc_time& t0, const sc_time& t1);
        void filter_cycle(u64 start, u64 end);
        void filter_source(const string& file, int line = -1);

        logger();
        logger(log_level max);
        logger(log_level min, log_level max);
        virtual ~logger();

        virtual void write_log(const logmsg& msg) = 0;

        template <typename PAYLOAD>
        void write_trace(const trace_msg<PAYLOAD>& msg);

        static bool would_log(log_level lvl);

        static void publish(log_level level,
                            const string& sender,
                            const string& message,
                            const char* file = nullptr,
                            int line = -1);

        static void log(const report& rep);

        template <typename SENDER, typename PAYLOAD>
        static void trace(trace_direction direction, const SENDER& sender,
                          const PAYLOAD& tx, const sc_time& dt = SC_ZERO_TIME);

        template <typename SENDER, typename PAYLOAD>
        static void trace_fw(const SENDER& sender, const PAYLOAD& tx,
                             const sc_time& dt = SC_ZERO_TIME);

        template <typename SENDER, typename PAYLOAD>
        static void trace_bw(const SENDER& sender, const PAYLOAD& tx,
                             const sc_time& dt = SC_ZERO_TIME);

        static bool print_time_stamp;
        static bool print_delta_cycle;
        static bool print_sender;
        static bool print_source;
        static bool print_backtrace;

        static size_t trace_name_length;
        static size_t trace_indent_incr;

        static void print_prefix(ostream& os, const logmsg& msg);
        static void print_logmsg(ostream& os, const logmsg& msg);

        static const char* prefix[NUM_LOG_LEVELS];
        static const char* desc[NUM_LOG_LEVELS];
    };

    inline void logger::set_level(log_level max) {
        set_level(LOG_ERROR, max);
    }

    inline bool logger::would_log(log_level lvl) {
        return !loggers[lvl].empty();
    }

    template <typename PAYLOAD>
    inline void logger::write_trace(const trace_msg<PAYLOAD>& msg) {
        write_log(msg);
    }

    inline void logger::filter(log_filter filter) {
        m_filters.push_back(filter);
    }

    inline void logger::filter_time(const sc_time& t0, const sc_time& t1) {
        filter([t0, t1](const logmsg& msg) -> bool {
            return msg.time >= t0 && msg.time < t1;
        });
    }

    inline void logger::filter_cycle(u64 start, u64 end) {
        filter([start, end](const logmsg& msg) -> bool {
            return msg.cycle >= start && msg.cycle < end;
        });
    }

    inline void logger::filter_source(const string& file, int line) {
        filter([file, line](const logmsg& msg) -> bool {
            if (!ends_with(msg.source.file, file))
                return false;
            if (line != -1 && msg.source.line != line)
                return false;
            return true;
        });
    }

    template <typename SENDER, typename PAYLOAD>
    inline void logger::trace(trace_direction direction, const SENDER& sender,
                              const PAYLOAD& tx, const sc_time& dt) {
        if (!would_log(LOG_TRACE))
            return;

        trace_msg<PAYLOAD> msg(sender.name(), direction, tx);
        msg.time_offset = dt;

        stringstream ss;
        if (direction == TRACE_FW)
            trace_curr_indent += trace_indent_incr;
        if (direction >= TRACE_FW)
            ss << string(trace_curr_indent, ' ') << ">> ";
        if (direction <= TRACE_BW)
            ss << string(trace_curr_indent, ' ') << "<< ";
        if (direction == TRACE_BW) {
            if (trace_curr_indent >= trace_indent_incr)
                trace_curr_indent -= trace_indent_incr;
            else
                trace_curr_indent = 0;
        }

        vector<string> lines = split(to_string(tx), '\n');
        for (auto line : lines)
            msg.lines.push_back(ss.str() + line);

        for (auto logger : loggers[LOG_TRACE])
            logger->write_trace(msg);
    }

    template <typename SENDER, typename PAYLOAD>
    inline void logger::trace_fw(const SENDER& sender, const PAYLOAD& tx,
                                 const sc_time& dt) {
        trace(TRACE_FW, sender, tx, dt);
    }

    template <typename SENDER, typename PAYLOAD>
    inline void logger::trace_bw(const SENDER& sender, const PAYLOAD& tx,
                                 const sc_time& dt) {
        trace(TRACE_BW, sender, tx, dt);
    }

    // VCML_OMIT_LOGGING_SOURCE: define this to remove the log_XXX macros and
    // replace them instead with first-grade functions. However, this will also
    // remove log message source information.
#ifndef VCML_OMIT_LOGGING_SOURCE
    static void log_tagged(log_level level, const char* file, int line,
                           const char* format, ...) VCML_DECL_PRINTF(4, 5);

    static inline void log_tagged(log_level lvl, const char* file, int line,
                                  const char* fmt, ...) {
        if (logger::would_log(lvl)) {
            va_list args; va_start(args, fmt);
            logger::publish(lvl, call_origin(), vmkstr(fmt, args), file, line);
            va_end(args);
        }
    }

#define log_error(...) \
    log_tagged(::vcml::LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...) \
    log_tagged(::vcml::LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...) \
    log_tagged(::vcml::LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) \
    log_tagged(::vcml::LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

#else

#define VCML_GEN_LOGFN(name, level)                                           \
    static void name(const char* format, ...) VCML_DECL_PRINTF(1, 2);         \
    static inline void name(const char* fmt, ...) {                           \
        if (logger::would_log(level)) {                                       \
            va_list args; va_start(args, fmt);                                \
            logger::publish(level, call_origin(), vmkstr(fmt, args));         \
            va_end(args);                                                     \
        }                                                                     \
    }

    VCML_GEN_LOGFN(log_error, ::vcml::LOG_ERROR)
    VCML_GEN_LOGFN(log_warn,  ::vcml::LOG_WARN)
    VCML_GEN_LOGFN(log_info,  ::vcml::LOG_INFO)
    VCML_GEN_LOGFN(log_debug, ::vcml::LOG_DEBUG)
#undef VCML_GEN_LOGFN
#endif

}

#endif
