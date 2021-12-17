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

#ifndef VCML_OPENCORES_OCFBC_H
#define VCML_OPENCORES_OCFBC_H

#include "vcml/common/types.h"
#include "vcml/common/report.h"
#include "vcml/common/systemc.h"

#include "vcml/protocols/tlm.h"
#include "vcml/protocols/irq.h"

#include "vcml/properties/property.h"
#include "vcml/ui/console.h"

#include "vcml/ports.h"
#include "vcml/peripheral.h"

namespace vcml { namespace opencores {

    class ocfbc: public peripheral
    {
    private:
        enum palette_info {
            PALETTE_ADDR = 0x800,
            PALETTE_SIZE = 0x200 // 2x 256x4bytes
        };

        ui::console m_console;

        const range m_palette_addr;
        u32  m_palette[PALETTE_SIZE];

        u8*  m_fb;

        u32  m_resx;
        u32  m_resy;
        u32  m_bpp;
        bool m_pc;

        u32 read_STAT();

        u32 write_STAT(u32 val);
        u32 write_CTRL(u32 val);
        u32 write_HTIM(u32 val);
        u32 write_VTIM(u32 val);

        virtual tlm_response_status read(const range& addr, void* data,
                                         const tlm_sbi& info) override;
        virtual tlm_response_status write(const range& addr, const void* data,
                                          const tlm_sbi& info) override;

        sc_event m_enable;

        void create();
        void render();
        void update();

        bool cmd_info(const vector<string>& args, ostream& os);

        // disabled
        ocfbc();
        ocfbc(const ocfbc&);

    public:
        enum control_bits {
            CTLR_VEN   = 1 << 0,  /* video enable */
            CTLR_VIE   = 1 << 1,  /* vsync interrupt enable */
            CTLR_HIE   = 1 << 2,  /* hsync interrupt enable */
            CTLR_VBSIE = 1 << 3,  /* video bank switch interrupt enable */
            CTLR_CBSIE = 1 << 4,  /* CLUT bank switch interrupt enable */
            CTLR_VBSWE = 1 << 5,  /* video bank switching enable */
            CTLR_CBSWE = 1 << 6,  /* CLUT bank switching enable */
            CTLR_VBL1  = 0 << 7,  /* video memory burst length: 1 cycle */
            CTLR_VBL2  = 1 << 7,  /* video memory burst length: 2 cycles */
            CTLR_VBL4  = 2 << 7,  /* video memory burst length: 4 cycles */
            CTLR_VBL8  = 3 << 7,  /* video memory burst length: 8 cycles */
            CTLR_BPP8  = 0 << 9,  /* 8 bits per pixel */
            CTLR_BPP16 = 1 << 9,  /* 16 bits per pixel */
            CTLR_BPP24 = 2 << 9,  /* 24 bits per pixel */
            CTLR_BPP32 = 3 << 9,  /* 32 bits per pixel */
            CTLR_PC    = 1 << 11, /* 8 bit pseudo color */
        };

        enum status_bits {
            STAT_SINT   = 1 << 0,  /* system error interrupt pending */
            STAT_LUINT  = 1 << 1,  /* line FIFO underrun interrupt pending */
            STAT_VINT   = 1 << 4,  /* vertical interrupt pending */
            STAT_HINT   = 1 << 5,  /* horizontal interrupt pending */
            STAT_VBSINT = 1 << 6,  /* video bank switch interrupt pending */
            STAT_CBSINT = 1 << 7,  /* CLUT bank switch interrupt pending */
            STAT_AVMP   = 1 << 16, /* active video memory page */
            STAT_ACMP   = 1 << 17, /* active CLUT memory page */
            STAT_HC0A   = 1 << 20, /* hardware cursor 0 available */
            STAT_HC1A   = 1 << 24, /* hardware cursor 1 available */
        };

        reg<u32> CTLR;
        reg<u32> STAT;
        reg<u32> HTIM;
        reg<u32> VTIM;
        reg<u32> HVLEN;
        reg<u32> VBARA;
        reg<u32> VBARB;

        irq_initiator_socket IRQ;
        tlm_target_socket IN;
        tlm_initiator_socket OUT;

        property<clock_t> clock;

        ocfbc(const sc_module_name& name);
        virtual ~ocfbc();
        VCML_KIND(ocfbc);

    protected:
        virtual void end_of_simulation() override;
    };

}}

#endif
