#ifndef INCLUDED_X300_FW_ERROR_CB_HPP
#define INCLUDED_X300_FW_ERROR_CB_HPP

#include <uhd/config.hpp>
#include <functional>
#include <string>

UHD_API void x300_set_fw_error_callback(const std::function<void(const std::string&)>& cb);

#endif /* INCLUDED_X300_FW_ERROR_CB_HPP */
