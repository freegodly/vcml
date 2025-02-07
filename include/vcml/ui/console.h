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

#ifndef VCML_UI_CONSOLE_H
#define VCML_UI_CONSOLE_H

#include "vcml/common/types.h"
#include "vcml/common/strings.h"
#include "vcml/common/report.h"
#include "vcml/common/thctl.h"

#include "vcml/ui/fbmode.h"
#include "vcml/ui/keymap.h"
#include "vcml/ui/input.h"
#include "vcml/ui/display.h"

#include "vcml/logging/logger.h"
#include "vcml/properties/property.h"

namespace vcml { namespace ui {

    class console
    {
    private:
        unordered_set<keyboard*> m_keyboards;
        unordered_set<pointer*>  m_pointers;
        unordered_set<shared_ptr<display>> m_displays;

    public:
        property<string> displays;

        bool has_display() const { return !m_displays.empty(); }

        u32 resx() const;
        u32 resy() const;

        console();
        virtual ~console();

        void notify(keyboard& kbd);
        void notify(pointer& ptr);

        void setup(const fbmode& mode, u8* fbptr);
        void render();
        void shutdown();
    };

    inline u32 console::resx() const {
        return m_displays.empty() ? 0u : (*m_displays.begin())->resx();
    }

    inline u32 console::resy() const {
        return m_displays.empty() ? 0u : (*m_displays.begin())->resy();
    }

}}

#endif
