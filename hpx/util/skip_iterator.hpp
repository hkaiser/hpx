//  Copyright (c) 2007-2014 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_UTIL_SKIP_ITERATOR_NOV_07_2014_1125AM)
#define HPX_UTIL_SKIP_ITERATOR_NOV_07_2014_1125AM

#include <hpx/hpx_fwd.hpp>
#include <hpx/traits/segemented_iterator_traits.hpp>

#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/serialization/serialization.hpp>

#include <iterator>
#include <algorithm>

namespace hpx { namespace util
{
    template <typename BaseIter>
    struct skip_iterator
      : boost::iterator_adaptor<
            skip_iterator<BaseIter>,
            BaseIter,
            boost::use_default,
            typename std::iterator_traits<BaseIter>::iterator_category
        >
    {
        typedef boost::iterator_adaptor<
                skip_iterator<BaseIter>,
                BaseIter,
                boost::use_default,
                typename std::iterator_traits<BaseIter>::iterator_category
            > base_type;

        skip_iterator() {}
        skip_iterator(BaseIter base, typename base_type::difference_type skip,
                bool perform_skipping = true)
          : base_type(base),
            skip_(skip), perform_skipping_(perform_skipping)
        {}

        typename base_type::difference_type get_skip() const { return skip_; }

    private:
        friend class boost::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & skip_ & perform_skipping_ & base_reference();
        }

    private:
        friend class boost::iterator_core_access;

        void increment()
        {
            std::advance(this->base_reference(), skip_);
        }

        void decrement()
        {
            std::advance(this->base_reference(), -skip_);
        }

        void advance(typename base_type::difference_type n)
        {
            std::advance(this->base_reference(), skip_ * n);
        }

        typename base_type::difference_type skip_;
        bool perform_skipping_;
    };
}}

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace traits
{
    // A skip_iterator represents a segmented iterator if it wraps a segmented
    // iterator.
    template <typename BaseIter>
    struct segmented_iterator_traits<
        util::skip_iterator<BaseIter>,
        typename boost::disable_if<
            typename segmented_iterator_traits<BaseIter>::is_segmented_iterator
        >::type>
    {
        typedef boost::mpl::false_ is_segmented_iterator;

        typedef util::skip_iterator<BaseIter> segment_iterator;
        typedef util::skip_iterator<
                typename segmented_iterator_traits<BaseIter>::local_iterator
            > local_iterator;

        //  This function should specify the local iterator which is at the
        //  beginning of the partition.
        static local_iterator begin(segment_iterator const& iter)
        {
            return local_iterator(
                segmented_iterator_traits<BaseIter>::begin(iter.base()),
                iter.get_skip());
        }

        //  This function should specify the local iterator which is at the
        //  end of the partition.
        static local_iterator end(segment_iterator const& iter)
        {
            return local_iterator(
                segmented_iterator_traits<BaseIter>::end(iter.base()),
                iter.get_skip());
        }
    };

    template <typename BaseIter>
    struct segmented_iterator_traits<
        util::skip_iterator<BaseIter>,
        typename boost::enable_if<
            typename segmented_iterator_traits<BaseIter>::is_segmented_iterator
        >::type>
    {
        typedef boost::mpl::true_ is_segmented_iterator;

        typedef util::skip_iterator<BaseIter> iterator;
        typedef util::skip_iterator<
                typename segmented_iterator_traits<BaseIter>::segment_iterator
            > segment_iterator;
        typedef util::skip_iterator<
                typename segmented_iterator_traits<BaseIter>::local_iterator
            > local_iterator;

        //  Conceptually this function is supposed to denote which segment
        //  the iterator is currently pointing to (i.e. just global iterator).
        static segment_iterator segment(iterator iter)
        {
            return segment_iterator(
                segmented_iterator_traits<BaseIter>::segment(iter.base()),
                iter.get_skip(), false);
        }

        //  This function should specify which is the current segment and
        //  the exact position to which local iterator is pointing.
        static local_iterator local(iterator iter)
        {
            return local_iterator(
                segmented_iterator_traits<BaseIter>::local(iter.base()),
                iter.get_skip());
        }

        //  This function should specify the local iterator which is at the
        //  beginning of the partition.
        static local_iterator begin(segment_iterator const& iter)
        {
            return local_iterator(
                segmented_iterator_traits<BaseIter>::begin(iter.base()),
                iter.get_skip());
        }

        //  This function should specify the local iterator which is at the
        //  end of the partition.
        static local_iterator end(segment_iterator const& iter)
        {
            return local_iterator(
                segmented_iterator_traits<BaseIter>::end(iter.base()),
                iter.get_skip());
        }

        // Extract the base id for the segment referenced by the given segment
        // iterator.
        static id_type get_id(segment_iterator const& iter)
        {
            return segmented_iterator_traits<BaseIter>::get_id(iter.base());
        }
    };
}}

#endif
