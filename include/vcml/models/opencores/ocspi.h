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

#ifndef VCML_OPENCORES_OCSPI_H
#define VCML_OPENCORES_OCSPI_H

#include "vcml/common/types.h"
#include "vcml/common/report.h"
#include "vcml/common/systemc.h"

#include "vcml/properties/property.h"

#include "vcml/protocols/tlm.h"
#include "vcml/protocols/irq.h"
#include "vcml/protocols/spi.h"

#include "vcml/ports.h"
#include "vcml/peripheral.h"

namespace vcml { namespace opencores {

    class ocspi: public peripheral
    {
    private:
        bool m_txe_irq;
        bool m_txr_irq;

        u8 write_TXDATA(u8 val);
        u8 write_STATUS(u8 val);
        u32 write_CONTROL(u32 val);
        u32 write_BAUDDIV(u32 val);

        // disabled
        ocspi();
        ocspi(const ocspi&);
    public:
        enum status_bis {
            STATUS_TXE = 1 << 0, /* transfer ended */
            STATUS_TXR = 1 << 1, /* transfer ready */
        };

        reg<u8> RXDATA;
        reg<u8> TXDATA;
        reg<u8> STATUS;
        reg<u32> CONTROL;
        reg<u32> BAUDDIV;

        irq_initiator_socket IRQ;
        tlm_target_socket IN;
        spi_initiator_socket SPI_OUT;

        property<clock_t> clock;

        ocspi(const sc_module_name& name);
        virtual ~ocspi();
        VCML_KIND(ocspi);
        virtual void reset() override;
    };

}}

#endif
