//
// Copyright 2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_LIBUHD_TRANSPORT_VRT_PACKET_HANDLER_HPP
#define INCLUDED_LIBUHD_TRANSPORT_VRT_PACKET_HANDLER_HPP

#include <uhd/config.hpp>
#include <boost/asio.hpp>

namespace uhd{ namespace transport{

    // Jumbo frames are limited to 9000;
    static const size_t MAX_ETHERNET_MTU = 9000;

    typedef boost::shared_ptr<boost::asio::ip::udp::socket> socket_sptr;

    /*!
     * Wait for the socket to become ready for a receive operation.
     * \param sock_fd the open socket file descriptor
     * \param timeout the timeout duration in seconds
     * \return true when the socket is ready for receive
     */
    UHD_INLINE bool wait_for_recv_ready(int sock_fd, double timeout){
#ifdef UHD_PLATFORM_WIN32 // select is more portable than poll unfortunately
        //setup timeval for timeout
        timeval tv;
        //If the tv_usec > 1 second on some platforms, select will
        //error EINVAL: An invalid timeout interval was specified.
        tv.tv_sec = int(timeout);
        tv.tv_usec = int(timeout*1000000)%1000000;

        //setup rset for timeout
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(sock_fd, &rset);

        //http://www.gnu.org/s/hello/manual/libc/Interrupted-Primitives.html
        //This macro is provided with gcc to properly deal with EINTR.
        //If not provided, define an empty macro, assume that is OK
        #ifndef TEMP_FAILURE_RETRY
            #define TEMP_FAILURE_RETRY(x) (x)
        #endif

        //call select with timeout on receive socket
        return TEMP_FAILURE_RETRY(::select(sock_fd+1, &rset, NULL, NULL, &tv)) > 0;
#elif UHD_NO_BRSC
        //calculate the total timeout in milliseconds (from seconds)
        int total_timeout = int(timeout*1000);

        pollfd pfd_read;
        pfd_read.fd = sock_fd;
        pfd_read.events = POLLIN;

        //call poll with timeout on receive socket
        return ::poll(&pfd_read, 1, total_timeout) > 0;
#else
    // ---- POSIX (Linux, macOS, BSD): use poll() ----

    // Clamp and convert to milliseconds for poll()
    int timeout_ms;
    if (timeout <= 0.0) {
        timeout_ms = 0;
    } else if (timeout > 2147483.0) {
        timeout_ms = 2147483647;  // INT32_MAX, effectively infinite
    } else {
        timeout_ms = static_cast<int>(timeout * 1000.0 + 0.5); // round
        if (timeout_ms == 0) timeout_ms = 1; // ensure at least 1ms if timeout > 0
    }

    pollfd pfd_read;
    pfd_read.fd      = sock_fd;
    pfd_read.events  = POLLIN;
    pfd_read.revents = 0;

    // Retry on EINTR
    while (true) {
        const int ret = ::poll(&pfd_read, 1, timeout_ms);

        if (ret > 0) {
            // Check revents for actual readability vs. error conditions
            if (pfd_read.revents & POLLNVAL) {
                return false;
                // throw uhd::io_error("poll() returned POLLNVAL: invalid socket fd");
            }
            if (pfd_read.revents & (POLLERR | POLLHUP)) {
                // Socket error or peer hangup — caller's recv() will get
                // the actual error, so still return true to let it proceed.
                // Alternatively, you could throw here for stricter handling.
                // UHD_LOG_WARNING("UDP",
                //     "poll() indicated socket error/hangup (revents=0x"
                //     << std::hex << pfd_read.revents << ")");
                return true;
            }
            if (pfd_read.revents & POLLIN) {
                return true;     // data is ready
            }
            // Unexpected revents combination — treat as not ready
            return false;
        }

        if (ret == 0) {
            return false;        // timeout
        }

        // ret < 0: error
        if (errno == EINTR) {
            // Interrupted by signal. Ideally we'd subtract elapsed time
            // from timeout_ms, but for short timeouts (typical in UHD)
            // retrying with the original value is acceptable.
            continue;
        }
        // throw uhd::io_error(str(
        //     boost::format("poll() failed: %s") % std::strerror(errno)));
        return false;
    }
#endif
    }

}} //namespace uhd::transport

#endif /* INCLUDED_LIBUHD_TRANSPORT_VRT_PACKET_HANDLER_HPP */
