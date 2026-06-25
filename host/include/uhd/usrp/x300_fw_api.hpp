//
// Copyright Black River Systems Company, Inc. 2026
//

#ifndef INCLUDED_X300_FW_API_HPP
#define INCLUDED_X300_FW_API_HPP

#include <functional>
#include <string>
#include <uhd/config.hpp>

/*!
 * Register a callback invoked when the X300 firmware communication fails on
 * its final retry (before throwing uhd::io_error). The callback receives the
 * error message string and is called from within the poke32/peek32 retry loop.
 */
UHD_API void x300_set_fw_error_callback(const std::function<void(const std::string&)>& cb);

/*!
 * Set the number of retries before declaring an X300 firmware communication
 * failure. Default is 3. Increase during active TX to tolerate transient
 * timeouts caused by a busy radio FPGA.
 */
UHD_API void x300_set_fw_retry_count(size_t count);

#endif /* INCLUDED_X300_FW_API_HPP */
