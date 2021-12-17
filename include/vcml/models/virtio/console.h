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

#ifndef VCML_VIRTIO_CONSOLE_H
#define VCML_VIRTIO_CONSOLE_H

#include "vcml/common/types.h"
#include "vcml/common/report.h"
#include "vcml/common/systemc.h"
#include "vcml/common/range.h"

#include "vcml/serial/port.h"
#include "vcml/protocols/virtio.h"

#include "vcml/module.h"

namespace vcml { namespace virtio {

    class console : public module,
                    public virtio_device,
                    public serial::port
    {
    private:
        enum virtqueues : int {
            VIRTQUEUE_DATA_RX = 0,
            VIRTQUEUE_DATA_TX = 1,
            VIRTQUEUE_CTRL_RX = 2,
            VIRTQUEUE_CTRL_TX = 3,
        };

        enum features : u64 {
            VIRTIO_CONSOLE_F_SIZE        = 1ull << 0,
            VIRTIO_CONSOLE_F_MULTIPORT   = 1ull << 1,
            VIRTIO_CONSOLE_F_EMERG_WRITE = 1ull << 2,
        };

        struct console_config {
            u16 cols;
            u16 rows;
            u32 max_nr_ports;
            u32 emerg_write;
        } m_config;

        queue<vq_message> m_fifo;

        size_t rx_data(u8* data, size_t len);
        size_t tx_data(const u8* data, size_t len);

        void poll();

        virtual void identify(virtio_device_desc& desc) override;
        virtual bool notify(u32 vqid) override;

        virtual void read_features(u64& features) override;
        virtual bool write_features(u64 features) override;

        virtual bool read_config(const range& addr, void* ptr) override;
        virtual bool write_config(const range& addr, const void* ptr) override;

    public:
        property<u16> cols;
        property<u16> rows;

        property<u64> pollrate;

        virtio_target_socket VIRTIO_IN;

        console(const sc_module_name& nm);
        virtual ~console();
        VCML_KIND(virtio::console);

        virtual void reset();
    };

}}

#endif
