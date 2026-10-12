//
// Copyright 2026 Black River Systems Company, Inc.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// BRSC: per-capture switch of an RX stream between the controlling UHD session
// and a fixed remote UDP destination. Not part of upstream UHD.

#pragma once

#include <uhd/config.hpp>
#include <uhd/stream.hpp>
#include <cstddef>

//! Lets applications detect at compile time that this API is available
#define UHD_BRSC_RX_DESTINATION 1

namespace uhd { namespace rfnoc { namespace brsc {

/*! Where the data of an RX streamer channel goes
 *
 * The remote destination is configured once, when the streamer is created,
 * with these stream args:
 * - `alt_dest_addr`, `alt_dest_port`: remote IPv4 address and UDP port. The
 *   presence of `alt_dest_addr` enables the feature.
 * - `alt_dest_mac_addr`: remote MAC address (AA:BB:CC:DD:EE:FF). Optional if
 *   the device can ARP.
 * - `alt_adapter`: transport adapter to send remote data out of (e.g. `sfp1`).
 *   Must not be the adapter the streamer itself is connected through.
 * - `alt_stream_mode`: `full_packet` (default) or `raw_payload`.
 *
 * Flow control is disabled for such streamers (`enable_fc` defaults to 0, and
 * `enable_fc=1` is rejected), because the device can't wait for credits from
 * a receiver that doesn't send any. Local captures therefore also run without
 * flow control. `dest_addr` (upstream remote streaming) can't be combined with
 * `alt_dest_addr`.
 *
 * A new streamer starts out LOCAL. UNKNOWN is only reported after a failed
 * set_rx_destination(), see there.
 *
 * The streamer holds no reference to the device's routing objects, so it must
 * not outlive its multi_usrp / rfnoc_graph: set_rx_destination() after the
 * device is gone is undefined behaviour.
 */
enum class rx_destination { LOCAL, REMOTE, UNKNOWN };

//! Channel index meaning "every channel of the streamer"
constexpr size_t ALL_CHANS = size_t(~0);

/*! Route the data of one or all channels of \p streamer to \p dest
 *
 * This reprograms one crossbar routing entry on the device (one management
 * transaction per channel). It does nothing for channels that are already
 * routed to \p dest, and writes again for channels whose destination is
 * UNKNOWN.
 *
 * If it throws, the affected channel's destination is UNKNOWN until the next
 * successful call: a timeout only means the reply didn't arrive, the write may
 * still have landed. Recreating the streamer also restores LOCAL. A switch to
 * LOCAL always re-arms the receive sequence check, even when it fails.
 *
 * The device's control traffic shares the transport with this transaction and
 * is held up while it waits for its reply: normally well under a millisecond,
 * but up to the 100 ms timeout (twice, with the one retry) if a reply is lost.
 *
 * Preconditions, which are not checked:
 * - Call from the thread that issues stream commands and calls recv() on this
 *   streamer, between captures, with the affected channels idle.
 * - Switch to REMOTE before issuing the timed stream command of a remote
 *   capture.
 * - Switch back to LOCAL only after the remote capture is over (start time +
 *   nsamps / rate + ~1 ms margin), and after draining its async errors with
 *   recv(timeout ~0). Packets still in flight would otherwise reach this
 *   session and be returned by the next recv().
 *
 * \throws uhd::value_error if \p dest is UNKNOWN
 * \throws uhd::type_error if \p streamer was not created by an RFNoC device
 * \throws uhd::index_error if \p chan is out of range
 * \throws uhd::not_implemented_error if the streamer was created without
 *         `alt_dest_addr`
 * \throws uhd::io_error if the device did not acknowledge the route change
 */
UHD_API void set_rx_destination(
    const uhd::rx_streamer::sptr& streamer, rx_destination dest, size_t chan = ALL_CHANS);

//! Return where the data of channel \p chan of \p streamer currently goes
UHD_API rx_destination get_rx_destination(
    const uhd::rx_streamer::sptr& streamer, size_t chan = 0);

//! Return true if channel \p chan of \p streamer was created with `alt_dest_addr`
UHD_API bool has_switchable_rx_destination(
    const uhd::rx_streamer::sptr& streamer, size_t chan = 0);

}}} // namespace uhd::rfnoc::brsc
