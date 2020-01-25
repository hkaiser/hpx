//  Copyright (c) 2015-2017 John Biddiscombe
//  Copyright (c) 2017      Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <plugins/parcelport/libfabric/header.hpp>
#include <plugins/parcelport/libfabric/libfabric_region_provider.hpp>
#include <plugins/parcelport/libfabric/parcelport_libfabric.hpp>
#include <plugins/parcelport/libfabric/sender.hpp>
#include <hpx/debugging/print.hpp>
//
#include <hpx/assertion.hpp>
#include <hpx/functional/unique_function.hpp>
#include <hpx/thread_support/atomic_count.hpp>
#include <hpx/timing/high_resolution_timer.hpp>
#include <hpx/util/yield_while.hpp>
//
#include <rdma/fi_endpoint.h>
//
#include <memory>
#include <cstddef>
#include <cstring>
#include <chrono>

namespace hpx {
namespace parcelset {
namespace policies {
namespace libfabric
{
    // --------------------------------------------------------------------
    // The main message send routine
    void sender::async_write_impl(unsigned int flags)
    {
        buffer_.data_point_.time_ = util::high_resolution_clock::now();
        HPX_ASSERT(message_region_ == nullptr);
        HPX_ASSERT(completion_count_ == 0);
        // increment counter of total messages sent
        ++sends_posted_;

        // reserve some space for zero copy information
        rma_regions_.reserve(buffer_.num_chunks_.first);

        // for each zerocopy chunk, we must create a memory region for the data
        // do this before creating the header as the chunk details will be copied
        // into the header space
        int rma_chunks = 0;
        int index = 0;
        for (auto &c : buffer_.chunks_)
        {
            // Debug only, dump out the chunk info
            send_deb.debug("write : chunk : size " , hpx::debug::hex<>(c.size_)
                    , "type "   , hpx::debug::dec<>((uint64_t)c.type_)
                    , "rma "    , hpx::debug::ptr(c.rma_)
                    , "cpos "   , hpx::debug::ptr(c.data_.cpos_)
                    , "index "  , hpx::debug::dec<>(c.data_.index_));
            if (c.type_ == serialization::chunk_type_pointer)
            {
                auto regtimer = send_deb.declare_variable<util::high_resolution_timer>();
                (void)regtimer; // silenced unused var when optimized out

                // create a new memory region from the user supplied pointer
                region_type *zero_copy_region =
                    new region_type(domain_, c.data_.cpos_, c.size_);

                rma_regions_.push_back(zero_copy_region);

                // set the region remote access key in the chunk space
                c.rma_  = zero_copy_region->get_remote_key();
//                send_deb.debug("Time to register memory (ns) "
//                    , hpx::debug::dec<>(regtimer.elapsed_nanoseconds()));
                send_deb.debug("Created zero-copy rdma Get region "
                    , hpx::debug::dec<>(index) , *zero_copy_region
                    , "for rma " , hpx::debug::ptr(c.rma_));

                send_deb.trace(
                    hpx::debug::mem_crc32(zero_copy_region->get_address(),
                        zero_copy_region->get_message_length(),
                        "zero_copy_region (pre-send) "));
            }
            else if (c.type_ == serialization::chunk_type_rma)
            {
                send_deb.debug("an RMA chunk was found");
                rma_chunks++;
            }
            ++index;
        }

        // create the header using placement new in the pinned memory block
        char *header_memory = (char*)(header_region_->get_address());

        send_deb.debug("Placement new for header");
        header_ = new(header_memory) header_type(buffer_, this);
        header_region_->set_message_length(header_->header_length());
        send_deb.debug("header " , *header_);

        // Get the block of pinned memory where the message was encoded
        // during serialization
        message_region_ = buffer_.data_.m_region_;
        message_region_->set_message_length(header_->message_size());

        HPX_ASSERT(header_->message_size() == buffer_.data_.size());
        send_deb.debug("Found region allocated during encode_parcel : address "
            , hpx::debug::ptr(buffer_.data_.m_array_)
            , *message_region_);

        // The number of completions we need before cleaning up:
        // 1 (header block send) + 1 (ack message if we have RMA chunks)
        completion_count_ = 1;
        region_list_[0] = {
            header_region_->get_address(), header_region_->get_message_length() };
        region_list_[1] = {
            message_region_->get_address(), message_region_->get_message_length() };

        desc_[0] = header_region_->get_local_key();
        desc_[1] = message_region_->get_local_key();
        if (rma_regions_.size()>0 || rma_chunks>0 || !header_->message_piggy_back())
        {
            completion_count_ = 2;
        }

        if (header_->chunk_data()) {
            send_deb.debug("Sender " , hpx::debug::ptr(this)
                , "Chunk info is piggybacked");
        }
        else {
            send_deb.trace("Setting up header-chunk rma data with "
                , "zero-copy chunks " , hpx::debug::dec<>(rma_regions_.size())
                , "rma chunks " , hpx::debug::dec<>(rma_chunks));
            auto &cb = header_->chunk_header_ptr()->chunk_rma;
            chunk_region_  = memory_pool_->allocate_region(cb.size_);
            cb.data_.pos_  = chunk_region_->get_address();
            cb.rma_        = chunk_region_->get_remote_key();
            std::memcpy(cb.data_.pos_, buffer_.chunks_.data(), cb.size_);
            send_deb.debug("Set up header-chunk rma data with "
                , "size "   , hpx::debug::dec<>(cb.size_)
                , "rma "    , hpx::debug::ptr(cb.rma_)
                , "addr "   , hpx::debug::ptr(cb.data_.cpos_));
        }

        if ((flags & header_type::bootstrap_flag) != 0)
        {
            header_->set_bootstrap_flag();
        }

        if (header_->message_piggy_back())
        {
            send_deb.debug("Sender " , hpx::debug::ptr(this)
                , "Main message is piggybacked");

            send_deb.trace(hpx::debug::mem_crc32(header_region_->get_address(),
                header_region_->get_message_length(),
                "Header region (send piggyback)"));

            send_deb.trace(hpx::debug::mem_crc32(message_region_->get_address(),
                message_region_->get_message_length(),
                "Message region (send piggyback)"));

            // send 2 regions as one message, goes into one receive
            bool ok = false;
            while (!ok) {
                HPX_ASSERT(
                    (this->region_list_[0].iov_len + this->region_list_[1].iov_len) <=
                        HPX_PARCELPORT_LIBFABRIC_MESSAGE_HEADER_SIZE);
                ssize_t ret = fi_sendv(this->endpoint_, this->region_list_,
                    this->desc_, 2, this->dst_addr_, this);

                if (ret == 0) {
                    ok = true;
                }
                else if (ret == -FI_EAGAIN) {
                    send_deb.error("Reposting fi_sendv...");
                    parcelport_->background_work(0, parcelport_background_mode_all);
                }
                else if (ret == -FI_ENOENT) {
                    if (hpx::threads::get_self_id()==hpx::threads::invalid_thread_id) {
                        // during bootstrap, this might happen on an OS thread
                        // so use std::this_thread::sleep to really stop activity
                        send_deb.error("No destination endpoint (bootstrap?), "
                                      , "retrying after 1s ...");
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                    else {
                        // if a node has failed, we can recover @TODO : put something here
                        send_deb.error("No destination endpoint, retrying after 1s ...");
                        std::terminate();
                    }
                }
                else if (ret)
                {
                    throw fabric_error(int(ret), "fi_sendv");
                }
            }
        }
        else
        {
            header_->set_message_rdma_info(
                message_region_->get_remote_key(), message_region_->get_address());

            send_deb.debug("Sender " , hpx::debug::ptr(this)
                , "message region NOT piggybacked "
                , hpx::debug::hex<>(buffer_.data_.size())
                , *message_region_);

            send_deb.trace(hpx::debug::mem_crc32(header_region_->get_address(),
                header_region_->get_message_length(),
                "Header region (pre-send)"));

            send_deb.trace(hpx::debug::mem_crc32(message_region_->get_address(),
                message_region_->get_message_length(),
                "Message region (send for rdma fetch)"));

            bool ok = false;
            while (!ok) {
                ssize_t ret = fi_send(this->endpoint_,
                    this->region_list_[0].iov_base,
                    this->region_list_[0].iov_len,
                    this->desc_[0], this->dst_addr_, this);

                if (ret == 0) {
                    ok = true;
                }
                else if (ret == -FI_EAGAIN)
                {
                    send_deb.error("reposting fi_send...\n");
                    parcelport_->background_work(0, parcelport_background_mode_all);
                }
                else if (ret)
                {
                    throw fabric_error(int(ret), "fi_send");
                }
            }
        }
        FUNC_END_DEBUG_MSG;
    }

    // --------------------------------------------------------------------
    void sender::handle_send_completion()
    {
        send_deb.debug("Sender " , hpx::debug::ptr(this)
            , "handle send_completion "
            , "RMA regions " , hpx::debug::dec<>(rma_regions_.size())
            , "completion count " , hpx::debug::dec<>(completion_count_));
        cleanup();
    }

    // --------------------------------------------------------------------
    void sender::handle_message_completion_ack()
    {
        send_deb.debug("Sender " , hpx::debug::ptr(this)
            , "handle_message_completion_ack ( "
            , "RMA regions " , hpx::debug::dec<>(rma_regions_.size())
            , "completion count " , hpx::debug::dec<>(completion_count_));
        ++acks_received_;
        cleanup();
    }

    // --------------------------------------------------------------------
    void sender::cleanup()
    {
        send_deb.debug("Sender " , hpx::debug::ptr(this)
            , "decrementing completion_count from " , hpx::debug::dec<>(completion_count_));

        // if we need to wait for more completion events, return without cleaning
        if (--completion_count_ > 0)
            return;

        // track deletions
        ++sends_deleted_;

        error_code ec;
        handler_(ec);
        handler_.reset();

        // cleanup header and message region
        memory_pool_->deallocate(message_region_);
        message_region_ = nullptr;
        header_         = nullptr;
        // cleanup chunk region
        if (chunk_region_) {
            memory_pool_->deallocate(chunk_region_);
            chunk_region_ = nullptr;
        }

        for (auto& region: rma_regions_) {
            memory_pool_->deallocate(region);
        }
        rma_regions_.clear();
        buffer_.data_point_.time_ =
            util::high_resolution_clock::now() - buffer_.data_point_.time_;
        parcelport_->add_sent_data(buffer_.data_point_);
        send_deb.debug("Sender " , hpx::debug::ptr(this)
            , "calling postprocess_handler");
        postprocess_handler_(this);
        send_deb.debug("Sender " , hpx::debug::ptr(this)
            , "completed cleanup/postprocess_handler");
    }

    // --------------------------------------------------------------------
    void sender::handle_error(struct fi_cq_err_entry err)
    {
        send_deb.error("resending message after error " , hpx::debug::ptr(this));

        if (header_->message_piggy_back())
        {
            // send 2 regions as one message, goes into one receive
            bool ok = false;
            while (!ok) {
                HPX_ASSERT(
                    (this->region_list_[0].iov_len + this->region_list_[1].iov_len) <=
                        HPX_PARCELPORT_LIBFABRIC_MESSAGE_HEADER_SIZE);
                ssize_t ret = fi_sendv(this->endpoint_, this->region_list_,
                    this->desc_, 2, this->dst_addr_, this);

                if (ret == 0) {
                    ok = true;
                }
                else if (ret == -FI_EAGAIN)
                {
                    send_deb.error("reposting fi_sendv...\n");
                    parcelport_->background_work(0, parcelport_background_mode_all);
                }
                else if (ret)
                {
                    throw fabric_error(int(ret), "fi_sendv");
                }
            }
        }
        else
        {
            header_->set_message_rdma_info(
                message_region_->get_remote_key(), message_region_->get_address());

            // send just the header region - a single message
            bool ok = false;
            while (!ok) {
                ssize_t ret = fi_send(this->endpoint_,
                    this->region_list_[0].iov_base,
                    this->region_list_[0].iov_len,
                    this->desc_[0], this->dst_addr_, this);

                if (ret == 0) {
                    ok = true;
                }
                else if (ret == -FI_EAGAIN)
                {
                    send_deb.error("reposting fi_send...\n");
                    parcelport_->background_work(0, parcelport_background_mode_all);
                }
                else if (ret)
                {
                    throw fabric_error(int(ret), "fi_sendv");
                }
            }
        }
    }

    // --------------------------------------------------------------------
    std::ostream & operator<<(std::ostream & os, const sender &s)
    {
        if (s.header_) {
            os << "sender " << hpx::debug::ptr(&s) << "header block " << *(s.header_);
        }
        else {
            os << "sender " << hpx::debug::ptr(&s) << "header block nullptr";
        }
        return os;
    }

}}}}
