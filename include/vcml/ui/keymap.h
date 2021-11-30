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

#ifndef VCML_UI_KEYMAP_H
#define VCML_UI_KEYMAP_H

#include "vcml/common/types.h"
#include "vcml/common/report.h"
#include "vcml/logging/logger.h"

#if defined(__linux__)
    #include <linux/input.h>
#else
    #include "input-event-codes.h"
#endif

namespace vcml { namespace ui {

    enum keysym : u32 {
        KEYSYM_NONE = 0,

        KEYSYM_1 = '1',
        KEYSYM_2 = '2',
        KEYSYM_3 = '3',
        KEYSYM_4 = '4',
        KEYSYM_5 = '5',
        KEYSYM_6 = '6',
        KEYSYM_7 = '7',
        KEYSYM_8 = '8',
        KEYSYM_9 = '9',
        KEYSYM_0 = '0',

        KEYSYM_A = 'A',
        KEYSYM_B = 'B',
        KEYSYM_C = 'C',
        KEYSYM_D = 'D',
        KEYSYM_E = 'E',
        KEYSYM_F = 'F',
        KEYSYM_G = 'G',
        KEYSYM_H = 'H',
        KEYSYM_I = 'I',
        KEYSYM_J = 'J',
        KEYSYM_K = 'K',
        KEYSYM_L = 'L',
        KEYSYM_M = 'M',
        KEYSYM_N = 'N',
        KEYSYM_O = 'O',
        KEYSYM_P = 'P',
        KEYSYM_Q = 'Q',
        KEYSYM_R = 'R',
        KEYSYM_S = 'S',
        KEYSYM_T = 'T',
        KEYSYM_U = 'U',
        KEYSYM_V = 'V',
        KEYSYM_W = 'W',
        KEYSYM_X = 'X',
        KEYSYM_Y = 'Y',
        KEYSYM_Z = 'Z',
        KEYSYM_a = 'a',
        KEYSYM_b = 'b',
        KEYSYM_c = 'c',
        KEYSYM_d = 'd',
        KEYSYM_e = 'e',
        KEYSYM_f = 'f',
        KEYSYM_g = 'g',
        KEYSYM_h = 'h',
        KEYSYM_i = 'i',
        KEYSYM_j = 'j',
        KEYSYM_k = 'k',
        KEYSYM_l = 'l',
        KEYSYM_m = 'm',
        KEYSYM_n = 'n',
        KEYSYM_o = 'o',
        KEYSYM_p = 'p',
        KEYSYM_q = 'q',
        KEYSYM_r = 'r',
        KEYSYM_s = 's',
        KEYSYM_t = 't',
        KEYSYM_u = 'u',
        KEYSYM_v = 'v',
        KEYSYM_w = 'w',
        KEYSYM_x = 'x',
        KEYSYM_y = 'y',
        KEYSYM_z = 'z',

        KEYSYM_EXCLAIM = '!',
        KEYSYM_DBLQUOTE = '"',
        KEYSYM_HASH = '#',
        KEYSYM_DOLLAR = '$',
        KEYSYM_PERCENT = '%',
        KEYSYM_AMPERSAND = '&',
        KEYSYM_QUOTE = '\'',
        KEYSYM_LEFTPAR = '(',
        KEYSYM_RIGHTPAR = ')',
        KEYSYM_ASTERISK = '*',
        KEYSYM_PLUS = '+',
        KEYSYM_COMMA = ',',
        KEYSYM_MINUS = '-',
        KEYSYM_DOT = '.',
        KEYSYM_SLASH = '/',
        KEYSYM_COLON = ':',
        KEYSYM_SEMICOLON = ';',
        KEYSYM_LESS = '<',
        KEYSYM_EQUAL = '=',
        KEYSYM_GREATER = '>',
        KEYSYM_QUESTION = '?',
        KEYSYM_AT = '@',
        KEYSYM_LEFTBRACKET = '[',
        KEYSYM_BACKSLASH = '\\',
        KEYSYM_RIGHTBRACKET = ']',
        KEYSYM_CARET = '^',
        KEYSYM_UNDERSCORE = '_',
        KEYSYM_BACKQUOTE = '`',
        KEYSYM_LEFTBRACE = '{',
        KEYSYM_PIPE = '|',
        KEYSYM_RIGHTBRACE = '}',
        KEYSYM_TILDE = '~',

        KEYSYM_SPECIAL = 1u << 31,

        KEYSYM_ESC,
        KEYSYM_ENTER,
        KEYSYM_BACKSPACE,
        KEYSYM_SPACE,
        KEYSYM_TAB,
        KEYSYM_LEFTSHIFT,
        KEYSYM_RIGHTSHIFT,
        KEYSYM_LEFTCTRL,
        KEYSYM_RIGHTCTRL,
        KEYSYM_LEFTALT,
        KEYSYM_RIGHTALT,
        KEYSYM_LEFTMETA,
        KEYSYM_RIGHTMETA,
        KEYSYM_MENU,
        KEYSYM_CAPSLOCK,

        KEYSYM_F1,
        KEYSYM_F2,
        KEYSYM_F3,
        KEYSYM_F4,
        KEYSYM_F5,
        KEYSYM_F6,
        KEYSYM_F7,
        KEYSYM_F8,
        KEYSYM_F9,
        KEYSYM_F10,
        KEYSYM_F11,
        KEYSYM_F12,

        KEYSYM_PRINT,
        KEYSYM_SCROLLOCK,
        KEYSYM_PAUSE,

        KEYSYM_INSERT,
        KEYSYM_DELETE,
        KEYSYM_HOME,
        KEYSYM_END,
        KEYSYM_PAGEUP,
        KEYSYM_PAGEDOWN,

        KEYSYM_LEFT,
        KEYSYM_RIGHT,
        KEYSYM_UP,
        KEYSYM_DOWN,

        KEYSYM_NUMLOCK,
        KEYSYM_KP0,
        KEYSYM_KP1,
        KEYSYM_KP2,
        KEYSYM_KP3,
        KEYSYM_KP4,
        KEYSYM_KP5,
        KEYSYM_KP6,
        KEYSYM_KP7,
        KEYSYM_KP8,
        KEYSYM_KP9,
        KEYSYM_KPENTER,
        KEYSYM_KPPLUS,
        KEYSYM_KPMINUS,
        KEYSYM_KPMUL,
        KEYSYM_KPDIV,
        KEYSYM_KPDOT,
        KEYSYM_KPUP,
        KEYSYM_KPDOWN,
        KEYSYM_KPLEFT,
        KEYSYM_KPRIGHT,
        KEYSYM_KPHOME,
        KEYSYM_KPEND,
        KEYSYM_KPPAGEUP,
        KEYSYM_KPPAGEDOWN,
        KEYSYM_KPINSERT,
        KEYSYM_KPDELETE,
    };

    struct syminfo {
        u32  keysym; // vcml key symbol (check above)
        u32  code;   // linux key code (depends on keyboard layout)
        bool shift;  // additionally needs shift to produce key symbol
        bool l_alt;  // additionally needs alt to produce key symbol
        bool r_alt;  // additionally needs altgr to produce key symbol

        bool is_special() const { return keysym > KEYSYM_SPECIAL; }
    };

    class keymap
    {
    private:
        static unordered_map<string, keymap> maps;

        keymap(const vector<syminfo>& layout);

    public:
        const vector<syminfo>& layout;

        keymap() = delete;
        keymap(const keymap&) = default;
        keymap(keymap&&) = default;

        const syminfo* lookup_symbol(u32 symbol) const;

        static const keymap& lookup(const string& name);
        static void register_keymap(const string& name,
                                    const vector<syminfo>& layout);
    };

}}

#endif
