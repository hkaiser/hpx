//  (C) Copyright 2005 Matthias Troyer and Dave Abrahams
//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2015 Andreas Schaefer
//  Copyright (c) 2017 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_SERIALIZATION_ARRAY_HPP
#define HPX_SERIALIZATION_ARRAY_HPP

#include <hpx/config.hpp>
#include <hpx/runtime/serialization/serialize.hpp>
#include <hpx/traits/is_bitwise_serializable.hpp>
#include <hpx/util/assert.hpp>

#include <boost/array.hpp>

#ifdef HPX_HAVE_CXX11_STD_ARRAY
#include <array>
#endif
#include <cstddef>
#include <type_traits>

namespace hpx { namespace serialization
{
    template <class T>
    class array
    {
    public:
        typedef T value_type;

        array(value_type* t, std::size_t s):
            m_t(t),
            m_element_count(s)
        {}

        value_type* address() const
        {
            return m_t;
        }

        std::size_t count() const
        {
            return m_element_count;
        }

        template <class Archive>
        void serialize_optimized(Archive& ar, unsigned int v, std::false_type)
        {
            for (std::size_t i = 0; i != m_element_count; ++i)
                ar & m_t[i];
        }

        void serialize_optimized(output_archive& ar, unsigned int, std::true_type)
        {
            // try using chunking
            ar.save_binary_chunk(m_t, m_element_count * sizeof(T));
        }

        void serialize_optimized(input_archive& ar, unsigned int, std::true_type)
        {
            // try using chunking
            ar.load_binary_chunk(m_t, m_element_count * sizeof(T));
        }

        template <class Archive>
        void serialize(Archive& ar, unsigned int v)
        {
            typedef std::integral_constant<bool,
                hpx::traits::is_bitwise_serializable<
                    typename std::remove_const<T>::type
                >::value> use_optimized;

#ifdef BOOST_BIG_ENDIAN
            bool archive_endianess_differs = ar.endian_little();
#else
            bool archive_endianess_differs = ar.endian_big();
#endif

            if (ar.disable_array_optimization() || archive_endianess_differs)
                serialize_optimized(ar, v, std::false_type());
            else
                serialize_optimized(ar, v, use_optimized());
        }

    private:
        value_type* m_t;
        std::size_t m_element_count;
    };

    // make_array function
    template <class T> HPX_FORCEINLINE
    array<T> make_array(T* begin, std::size_t size)
    {
        return array<T>(begin, size);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <class T>
    class zero_copy_array
    {
    public:
        typedef T value_type;

        zero_copy_array(std::size_t s)
          : m_t(nullptr),
            m_element_count(s)
        {}

        value_type* address() const
        {
            return m_t;
        }

        std::size_t count() const
        {
            return m_element_count;
        }

        void serialize_optimized(input_archive& ar, unsigned int v, std::false_type)
        {
            for (std::size_t i = 0; i != m_element_count; ++i)
                ar & m_t[i];
        }

        void serialize_optimized(input_archive& ar, unsigned int v, std::true_type)
        {
            HPX_ASSERT(m_t == nullptr);
            std::size_t size = m_element_count * sizeof(T);

            if (ar.disable_data_chunking())
            {
                m_t = static_cast<T*>(ar.zero_copy_allocator()->allocate(size));
                if (nullptr == m_t) throw std::bad_alloc();

                ar.load_binary(m_t, size);
            }
            else
            {
                // try using zero-copy chunking
                void* t = nullptr;
                if (!ar.load_binary_chunk_direct(t, size))
                {
                    // fall back if failed
                    m_t = static_cast<T*>(ar.zero_copy_allocator()->
                        allocate(size));
                    if (nullptr == m_t) throw std::bad_alloc();

                    ar.load_binary_chunk(m_t, size);
                }
                else
                {
                    m_t = static_cast<T*>(t);
                }
            }
        }

        void serialize(output_archive& ar, unsigned int v)
        {
            static_assert(sizeof(output_archive) == 0,
                "this type can be only used for de-serialization");
        }

        void serialize(input_archive& ar, unsigned int v)
        {
            typedef std::integral_constant<bool,
                hpx::traits::is_bitwise_serializable<
                    typename std::remove_const<T>::type
                >::value> use_optimized;

#ifdef BOOST_BIG_ENDIAN
            bool archive_endianess_differs = ar.endian_little();
#else
            bool archive_endianess_differs = ar.endian_big();
#endif

            if (ar.disable_array_optimization() || archive_endianess_differs)
                serialize_optimized(ar, v, std::false_type());
            else
                serialize_optimized(ar, v, use_optimized());
        }

    private:
        value_type* m_t;
        std::size_t m_element_count;
    };

    // make_array function
    template <class T> HPX_FORCEINLINE
    zero_copy_array<T> make_zero_copy_array(std::size_t size)
    {
        return zero_copy_array<T>(size);
    }

    ///////////////////////////////////////////////////////////////////////////
    // implement serialization for boost::array
    template <class Archive, class T, std::size_t N>
    void serialize(Archive& ar, boost::array<T,N>& a, const unsigned int /* version */)
    {
        ar & hpx::serialization::make_array(a.begin(), a.size());
    }

#ifdef HPX_HAVE_CXX11_STD_ARRAY
  // implement serialization for std::array
    template <class Archive, class T, std::size_t N>
    void serialize(Archive& ar, std::array<T,N>& a, const unsigned int /* version */)
    {
        ar & hpx::serialization::make_array(a.data(), a.size());
    }
#endif

    // allow our array to be serialized as prvalue
    // compiler should support good ADL implementation
    // but it is required for all hpx serialization code anyways
    template <typename T> HPX_FORCEINLINE
    output_archive & operator<<(output_archive & ar, array<T> t)
    {
        ar.invoke(t);
        return ar;
    }

    template <typename T> HPX_FORCEINLINE
    input_archive & operator>>(input_archive & ar, array<T> t)
    {
        ar.invoke(t);
        return ar;
    }

    template <typename T> HPX_FORCEINLINE
    output_archive & operator&(output_archive & ar, array<T> t) //-V524
    {
        ar.invoke(t);
        return ar;
    }

    template <typename T> HPX_FORCEINLINE
    input_archive & operator&(input_archive & ar, array<T> t) //-V524
    {
        ar.invoke(t);
        return ar;
    }

    // serialize plain arrays:
    template <typename T, std::size_t N> HPX_FORCEINLINE
    output_archive & operator<<(output_archive & ar, T (&t)[N])
    {
        array<T> array = make_array(t, N);
        ar.invoke(array);
        return ar;
    }

    template <typename T, std::size_t N> HPX_FORCEINLINE
    input_archive & operator>>(input_archive & ar, T (&t)[N])
    {
        array<T> array = make_array(t, N);
        ar.invoke(array);
        return ar;
    }

    template <typename T, std::size_t N> HPX_FORCEINLINE
    output_archive & operator&(output_archive & ar, T (&t)[N]) //-V524
    {
        array<T> array = make_array(t, N);
        ar.invoke(array);
        return ar;
    }

    template <typename T, std::size_t N> HPX_FORCEINLINE
    input_archive & operator&(input_archive & ar, T (&t)[N]) //-V524
    {
        array<T> array = make_array(t, N);
        ar.invoke(array);
        return ar;
    }
}}

#endif // HPX_SERIALIZATION_ARRAY_HPP
