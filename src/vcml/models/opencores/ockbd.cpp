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

#include "vcml/models/opencores/ockbd.h"

#define MOD_RELEASE (1 << 7)

namespace vcml { namespace opencores {

    void ockbd::update() {
        ui::input_event event;
        while (m_keyboard.pop_event(event)) {
            VCML_ERROR_ON(!event.is_key(), "illegal event from keyboard");
            u8 scancode = (u8)(event.key.code & 0xff);
            bool down = (event.key.state != ui::VCML_KEY_UP);

            if (!down)
                scancode |= MOD_RELEASE;

            if (m_key_fifo.size() < fifosize)
                m_key_fifo.push(scancode);
            else
                log_debug("FIFO full, dropping key");
        }

        if (!IRQ && !m_key_fifo.empty())
            log_debug("setting IRQ");

        IRQ = !m_key_fifo.empty();

        sc_time quantum = tlm_global_quantum::instance().get();
        next_trigger(max(clock_cycle(), quantum));
    }

    u8 ockbd::read_KHR() {
        VCML_ERROR_ON(IRQ && m_key_fifo.empty(), "IRQ without data");

        if (m_key_fifo.empty()) {
            log_debug("read KHR without data and interrupt");
            return 0;
        }

        u8 key = m_key_fifo.front();
        m_key_fifo.pop();

        log_debug("cpu fetched key 0x%hhx from KHR, %zu keys remaining",
                  key, m_key_fifo.size());
        if (IRQ && m_key_fifo.empty())
            log_debug("clearing IRQ");

        IRQ = !m_key_fifo.empty();
        return key;
    }

    ockbd::ockbd(const sc_module_name& nm):
        peripheral(nm),
        m_key_fifo(),
        m_keyboard(name()),
        m_console(),
        KHR("KHR", 0x0, 0),
        IRQ("IRQ"),
        IN("IN"),
        keymap("keymap", "us"),
        fifosize("fifosize", 16) {
        m_keyboard.set_layout(keymap);

        KHR.allow_read_only();
        KHR.on_read(&ockbd::read_KHR);

        if (m_console.has_display()) {
            m_console.notify(m_keyboard);
            SC_HAS_PROCESS(ockbd);
            SC_METHOD(update);
        }
    }

    ockbd::~ockbd() {
        // nothing to do
    }

    void ockbd::end_of_simulation() {
        m_console.shutdown();
        peripheral::end_of_simulation();
    }

}}
