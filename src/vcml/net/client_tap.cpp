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

#include "vcml/net/client_tap.h"
#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace vcml { namespace net {

    client_tap::client_tap(const string& adapter, int devno):
        client(adapter) {
        m_fd = open("/dev/net/tun", O_RDWR);
        VCML_REPORT_ON(m_fd < 0, "error opening tundev: %s", strerror(errno));

        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
        snprintf(ifr.ifr_name, IFNAMSIZ, "tap%d", devno);

        int err = ioctl(m_fd, TUNSETIFF, (void*)&ifr);
        VCML_REPORT_ON(err < 0, "error creating tapdev: %s", strerror(errno));
        log_info("using tap device %s", ifr.ifr_name);

        m_type = mkstr("tap:%d", devno);
    }

    client_tap::~client_tap() {
        if (m_fd >= 0)
            close(m_fd);
    }

    bool client_tap::recv_packet(vector<u8>& packet) {
        if (m_fd < 0)
            return false;

        if (!fd_peek(m_fd))
            return false;

        ssize_t len;
        packet.resize(ETH_MAX_FRAME_SIZE);

        do {
            len = ::read(m_fd, packet.data(), packet.size());
        } while (len < 0 && errno == EINTR);

        if (len < 0) {
            log_error("error reading tap device: %s", strerror(errno));
            close(m_fd);
            m_fd = -1;
        }

        if (len <= 0)
            return false;

        packet.resize(len);
        return true;
    }

    void client_tap::send_packet(const vector<u8>& packet) {
        if (m_fd >= 0)
            fd_write(m_fd, packet.data(), packet.size());
    }

    client* client_tap::create(const string& adapter, const string& type) {
        int devno = 0;
        vector<string> args = split(type, ':');
        if (args.size() > 1)
            devno = from_string<int>(args[1]);
        return new client_tap(adapter, devno);
    }

}}

#else  // not linux not supported
namespace vcml { namespace net {

    client_tap::client_tap(const string& adapter, int devno):
        client(adapter) {
        log_error("tap devices are not supported on this platform");
    }

    client_tap::~client_tap() { 
    }

    bool client_tap::recv_packet(vector<u8>& packet) {
        log_error("not supported"); 
        return false;
    }

    void client_tap::send_packet(const vector<u8>& packet) {    
    }

    client* client_tap::create(const string& adapter, const string& type) {
        return new client_tap(adapter, 0);
    }

}}
#endif
