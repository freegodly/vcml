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

#include "vcml/models/opencores/ompic.h"

#define OMPIC_DATA(x) ((x) & 0xffff)
#define OMPIC_DEST(x) (((x) >> 16) & 0x3fff)

namespace vcml { namespace opencores {

    u32 ompic::read_STATUS(size_t core_idx) {
        VCML_ERROR_ON(core_idx >= m_num_cores, "core_id >= num_cores");
        u32 val = m_status[core_idx];
        if (IRQ[core_idx].read())
            val |= CTRL_IRQ_GEN;
        return val;
    }

    u32 ompic::read_CONTROL(size_t core_idx) {
        VCML_ERROR_ON(core_idx >= m_num_cores, "core_id >= num_cores");
        return m_control[core_idx];
    }

    u32 ompic::write_CONTROL(u32 val, size_t core_idx) {
        VCML_ERROR_ON(core_idx >= m_num_cores, "core_id >= num_cores");

        u32 self = static_cast<uint32_t>(core_idx);
        u32 dest = OMPIC_DEST(val);
        u32 data = OMPIC_DATA(val);

        if (dest >= m_num_cores) {
            log_warn("illegal interrupt request ignored");
            log_warn(" core: cpu%d", self);
            log_warn(" dest: cpu%d", dest);
            log_warn(" data: 0x%04x", data);
            return 0;
        }

        m_control[core_idx] = val;
        if (val & CTRL_IRQ_GEN) {
            m_status[dest] = self << 16 | data;
            log_debug("cpu%d triggers interrupt on cpu%d (data: 0x%04x)",
                      self, dest, data);
            if (IRQ[dest].read())
                log_debug("interrupt already pending for cpu%d", dest);
            IRQ[dest] = true;
        }

        if (val & CTRL_IRQ_ACK) {
            log_debug("cpu%d acknowledges interrupt", self);
            if (!IRQ[self].read())
                log_debug("no pending interrupt for cpu%d", self);
            IRQ[self] = false;
        }

        return val;
    }

    ompic::ompic(const sc_core::sc_module_name& nm, unsigned int num_cores):
        peripheral(nm),
        m_num_cores(num_cores),
        m_control(nullptr),
        m_status(nullptr),
        CONTROL(nullptr),
        STATUS(nullptr),
        IRQ("IRQ"),
        IN("IN") {
        VCML_ERROR_ON(num_cores == 0, "number of cores must not be zero");

        CONTROL = new reg<u32>*[m_num_cores];
        STATUS  = new reg<u32>*[m_num_cores];

        m_control = new uint32_t[m_num_cores]();
        m_status  = new uint32_t[m_num_cores]();

        stringstream ss;
        for (unsigned int core = 0; core < num_cores; core++) {
            ss.str("");
            ss << "CONTROL" << core;

            CONTROL[core] = new reg<u32>(ss.str().c_str(), core * 8);
            CONTROL[core]->allow_read_write();
            CONTROL[core]->on_read(&ompic::read_CONTROL);
            CONTROL[core]->on_write(&ompic::write_CONTROL);
            CONTROL[core]->tag = core;

            ss.str("");
            ss << "STATUS" << core;

            STATUS[core] = new reg<u32>(ss.str().c_str(), core * 8 + 4);
            STATUS[core]->allow_read_only();
            STATUS[core]->on_read(&ompic::read_STATUS);
            STATUS[core]->tag = core;
        }
    }

    ompic::~ompic() {
        for (unsigned int core = 0; core < m_num_cores; core++) {
            delete CONTROL [core];
            delete STATUS  [core];
        }

        delete [] CONTROL;
        delete [] STATUS;
        delete [] m_control;
        delete [] m_status;
    }

}}
