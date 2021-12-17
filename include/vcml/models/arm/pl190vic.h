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

#ifndef VCML_ARM_PL190VIC_H
#define VCML_ARM_PL190VIC_H

#include "vcml/common/types.h"
#include "vcml/common/report.h"
#include "vcml/common/systemc.h"
#include "vcml/common/range.h"

#include "vcml/protocols/tlm.h"
#include "vcml/protocols/irq.h"

#include "vcml/ports.h"
#include "vcml/peripheral.h"

#define VCML_ARM_PL190VIC_NVEC (16)
#define VCML_ARM_PL190VIC_NIRQ (32)
#define VCML_ARM_PL190VIC_PID  (0x00041190) // Peripheral ID
#define VCML_ARM_PL190VIC_CID  (0xB105F00D) // PrimeCell ID

namespace vcml { namespace arm {

    class pl190vic: public peripheral, public irq_target
    {
    private:
        u32  m_ext_irq;
        u32  m_current_irq;
        bool m_vect_int;

        void update();

        u32 write_INTE(u32 val);
        u32 write_IECR(u32 val);
        u32 write_SINT(u32 val);
        u32 write_SICR(u32 val);
        u32 write_ADDR(u32 val);
        u32 write_VCTRL(u32 val, size_t idx);

    public:
        enum vctrl_bits {
            VCTRL_ENABLED  = 1 << 5,
            VCTRL_SOURCE_M = 0x1f,
            VCTRL_M        = 0x3f,
        };

        reg<u32> IRQS; // IRQ Status register
        reg<u32> FIQS; // FIQ Status register
        reg<u32> RISR; // Raw Interrupt Status register
        reg<u32> INTS; // Interrupt Select register
        reg<u32> INTE; // Interrupt Enable register
        reg<u32> IECR; // Interrupt Enable Clear register
        reg<u32> SINT; // Software Interrupt register
        reg<u32> SICR; // Software Interrupt Clear register
        reg<u32> PROT; // Protection register
        reg<u32> ADDR; // Vector Address register
        reg<u32> DEFA; // Default Vector Address register

        reg<u32, VCML_ARM_PL190VIC_NVEC> VADDR; // Vector Addresses
        reg<u32, VCML_ARM_PL190VIC_NVEC> VCTRL; // Vector Controls

        reg<u32, 4> PID; // Peripheral ID registers
        reg<u32, 4> CID; // Cell ID registers

        tlm_target_socket IN;

        irq_target_socket_array<VCML_ARM_PL190VIC_NIRQ> IRQ_IN;
        irq_initiator_socket_array<> IRQ_OUT;
        irq_initiator_socket_array<> FIQ_OUT;

        pl190vic(const sc_module_name& nm);
        virtual ~pl190vic();
        VCML_KIND(arm::pl190vic);

        virtual void reset() override;
        virtual void irq_transport(const irq_target_socket& socket,
                                   irq_payload& irq) override;
    };

}}

#endif
