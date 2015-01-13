//  Copyright (c) 2015 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_UTIL_BACK_INSERT_ITERATOR_JAN_08_2015_0155PM)
#define HPX_UTIL_BACK_INSERT_ITERATOR_JAN_08_2015_0155PM

#include <hpx/hpx_fwd.hpp>
#include <hpx/traits/segmented_iterator_traits.hpp>
#include <hpx/util/move.hpp>

#include <cstddef>

#include <boost/shared_ptr.hpp>

namespace hpx { namespace util
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Container>
    class local_back_insert_iterator
    {
    public:
        typedef std::output_iterator_tag iterator_category;
        typedef void value_type;
        typedef void difference_type;
        typedef void pointer;
        typedef void reference;

        typedef Container container_type;
        typedef typename Container::value_type value_type;

        typedef typename Container::local_raw_iterator
            local_raw_iterator;

        typedef typename Container::partition_client_type partition_client;
        typedef typename Container::partition_server_type partition_server;
        typedef boost::shared_ptr<partition_server> partition_server_ptr;

    public:
        explicit local_back_insert_iterator(partition_client partition,
                partition_server_ptr const& data)
          : partition_(partition),
            data_(data)
        {}

        local_back_insert_iterator& operator=(value_type const& val)
        {
            if (data_)
                data_->push_back(val);
            else
                partition_.push_back(val);

            return *this;
        }

        local_back_insert_iterator& operator=(value_type&& val)
        {
            if (data_)
                data_->push_back(std::move(val));
            else
                partition_.push_back(std::move(val));

            return *this;
        }

        local_back_insert_iterator& operator*()
        {
            return *this;
        }

        local_back_insert_iterator& operator++()
        {
            return *this;
        }

        local_back_insert_iterator operator++(int)
        {
            return *this;
        }

        local_raw_iterator base_iterator()
        {
            HPX_ASSERT(data_);
            return data_->end();
        }
        local_raw_const_iterator base_iterator() const
        {
            HPX_ASSERT(data_);
            return data_->cend();
        }

        partition_client& get_partition() { return partition_; }
        partition_client get_partition() const { return partition_; }

        partition_server_ptr& get_data() { return data_; }
        partition_server_ptr const& get_data() const { return data_; }

    private:
        friend class boost::serialization::access;

        template <typename Archive>
        void load(Archive& ar, unsigned version)
        {
            ar & partition_;
            if (partition_)
                data_ = partition_.get_ptr();
        }
        template <typename Archive>
        void save(Archive& ar, unsigned version) const
        {
            ar & partition_;
        }

        BOOST_SERIALIZATION_SPLIT_MEMBER()

    protected:
        // refer to a partition of the vector
        partition_client partition_;

        // caching address of component
        partition_server_ptr data_;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container>
    class local_segment_back_insert_iterator
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    ///
    /// Conceptually this iterator is a ReadableIterator (its operator* can't
    /// be used for assignment), even while the iterator category is
    /// output_iterator_tag.
    ///
    template <typename Container>
    class segment_back_insert_iterator
    {
        typedef typename Container::value_type value_type;
        typedef typename Container::segment_iterator base_iterator_type;

        typedef std::output_iterator_tag iterator_category;
        typedef Container container_type;
        typedef void difference_type;
        typedef void pointer;
        typedef void reference;

        explicit segment_back_insert_iterator(base_iterator_type const& it,
                Container* data = 0)
          : it_(it), data_(data)
        {}

        segment_back_insert_iterator& operator*()
        {
            return *this;
        }

        segment_back_insert_iterator& operator++()
        {
            ++it_;
            return *this;
        }

        segment_back_insert_iterator operator++(int)
        {
            segment_back_insert_iterator curr(it_, data_);
            ++it_;
            return curr;
        }

        Container* get_data() { return data_; }
        Container const* get_data() const { return data_; }

    protected:
        base_iterator_type it_;
        Container *data_;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container>
    class back_insert_iterator
    {
    public:
        typedef Container container_type;

        typedef std::output_iterator_tag iterator_category;
        typedef void difference_type;
        typedef void pointer;
        typedef void reference;
        typedef typename Container::value_type value_type;

        typedef segment_back_insert_iterator<Container> segment_iterator;
        typedef local_segment_back_insert_iterator<Container> local_segment_iterator;
        typedef local_back_insert_iterator<Container> local_iterator;
        typedef typename Container::local_raw_iterator local_raw_iterator;

        explicit back_insert_iterator(Container& cont)
          : container(std::addressof(cont))
        {}

        back_insert_iterator& operator=(value_type const& val)
        {
            container->push_back(val);
            return *this;
        }

        back_insert_iterator& operator=(value_type&& val)
        {
            container->push_back(std::forward<value_type>(val));
            return *this;
        }

        back_insert_iterator& operator*()
        {
            return *this;
        }

        back_insert_iterator& operator++()
        {
            return *this;
        }

        back_insert_iterator operator++(int)
        {
            return *this;
        }

        Container* get_data() { return container; }
        Container const* get_data() const { return container; }

    protected:
        Container *container;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Container>
    inline back_insert_iterator<Container>
    back_inserter(Container& cont)
    {
        return back_insert_iterator<Container>(cont);
    }
}}

namespace hpx { namespace traits
{
    template <typename Cont>
    struct segmented_iterator_traits<
            back_insert_iterator<Cont>,
            typename boost::enable_if<
                typename segmented_iterator_traits<
                    typename Cont::iterator
                >::is_segmented_iterator
            >::type
        >
    {
        typedef boost::mpl::true_ is_segmented_iterator;

        typedef back_insert_iterator<Cont> iterator;
        typedef typename iterator::segment_iterator segment_iterator;
        typedef typename iterator::local_segment_iterator local_segment_iterator;
        typedef typename iterator::local_iterator local_iterator;

        typedef typename local_iterator::local_raw_iterator local_raw_iterator;

//             typename Container::iterator end = cont.end();
//             typename Container::segment_iterator seg_end = cont.segment_end();

        //  Conceptually this function is supposed to denote which segment
        //  the iterator is currently pointing to (i.e. just global iterator).
        static segment_iterator segment(iterator iter)
        {
            typename iterator::container_type* cont = iter.get_data();
            return segment_iterator(
                cont->get_segment_iterator(cont->size()), cont);
        }

        //  This function should specify which is the current segment and
        //  the exact position to which local iterator is pointing.
        static local_iterator local(iterator iter)
        {
            typename iterator::container_type& cont = iter.get_data();
            return cont.get_local_iterator(cont.size());
        }

        //  Build a full iterator from the segment and local iterators
        static iterator compose(segment_iterator seg_iter, local_iterator)
        {
            vector<T>* data = seg_iter.get_data();
            return iterator(data, data->get_global_index(
                seg_iter, seg_iter.base()->size_));
        }

        //  This function should specify the local iterator which is at the
        //  beginning of the partition.
        static local_iterator begin(segment_iterator seg_iter)
        {
            std::size_t offset = 0;
            if (seg_iter.is_at_end())
            {
                // return iterator to the end of last segment
                --seg_iter;
                offset = seg_iter.base()->size_;
            }

            return local_iterator(seg_iter.base()->partition_, offset,
                seg_iter.base()->local_data_);
        }

        //  This function should specify the local iterator which is at the
        //  end of the partition.
        static local_iterator end(segment_iterator seg_iter)
        {
            if (seg_iter.is_at_end())
                --seg_iter;     // return iterator to the end of last segment

            return local_iterator(seg_iter.base()->partition_,
                seg_iter.base()->size_, seg_iter.base()->local_data_);
        }

        //  This function should specify the local iterator which is at the
        //  beginning of the partition data.
        static local_raw_iterator begin(local_segment_iterator const& seg_iter)
        {
            return seg_iter->begin();
        }

        //  This function should specify the local iterator which is at the
        //  end of the partition data.
        static local_raw_iterator end(local_segment_iterator const& seg_iter)
        {
            return seg_iter->end();
        }

        // Extract the base id for the segment referenced by the given segment
        // iterator.
        static id_type get_id(segment_iterator const& iter)
        {
            return iter->get_id();
        }
    };
}}

#endif

