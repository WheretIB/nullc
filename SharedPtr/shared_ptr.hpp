#pragma once

//
//  shared_ptr.hpp
//
//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001, 2002, 2003 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/shared_ptr.htm for documentation.
//

#include "checked_delete.hpp"
#include "shared_count.hpp"

template <typename T> class weak_ptr;

namespace detail
{
	struct static_cast_tag {};
	struct const_cast_tag {};
	struct dynamic_cast_tag {};
} // namespace detail


//
//  shared_ptr
//
//  An enhanced relative of scoped_ptr with reference counted copy semantics.
//  The object pointed to is deleted when the last shared_ptr pointing to it
//  is destroyed or reset.
//

template <typename T> class shared_ptr
{
public:
    typedef T element_type;
    typedef T value_type;
    typedef T* pointer;
    typedef T& reference;

    shared_ptr(): px(0), pn() // never throws in 1.30+
    {
    }

    template <typename U> explicit shared_ptr(U* p): px(p), pn(p) // U must be complete
    {
    }

    template <typename U> explicit shared_ptr(const weak_ptr<U>& r): pn(r.pn) // may throw
    {
        // it is now safe to copy r.px, as pn(r.pn) did not throw
        px = r.px;
    }

    template <typename U> shared_ptr(const shared_ptr<U>& r): px(r.px), pn(r.pn) // never throws
    {
    }

    template <typename U> shared_ptr(const shared_ptr<U>& r, detail::static_cast_tag): px(static_cast<element_type*>(r.px)), pn(r.pn)
    {
    }

    template <typename U> shared_ptr(const shared_ptr<U>& r, detail::const_cast_tag): px(const_cast<element_type*>(r.px)), pn(r.pn)
    {
    }

    template <typename U> shared_ptr(const shared_ptr<U>& r, detail::dynamic_cast_tag): px(dynamic_cast<element_type*>(r.px)), pn(r.pn)
    {
        if(px == 0) // need to allocate new counter -- the cast failed
        {
            pn = detail::shared_count();
        }
    }

    template <typename U> shared_ptr& operator=(const shared_ptr<U>& r) // never throws
    {
        px = r.px;
        pn = r.pn; // shared_count::op= doesn't throw
        return *this;
    }

	void reset() // never throws in 1.30+
    {
        shared_ptr().swap(*this);
    }

    template <typename U> void reset(U* p) // Y must be complete
    {
        //assert(p == 0 || p != px); // catch self-reset errors
		shared_ptr(p).swap(*this);
    }

	T& operator*() const // never throws
    {
        //assert(px != 0);
        return *px;
    }

    T* operator-> () const // never throws
    {
        //assert(px != 0);
        return px;
    }
    
    T* get() const // never throws
    {
        return px;
    }

    // implicit conversion to "bool"
    typedef T* shared_ptr::*unspecified_bool_type;

    operator unspecified_bool_type() const // never throws
    {
        return px == 0 ? 0 : &shared_ptr::px;
    }

    bool unique() const // never throws
    {
        return pn.unique();
    }

    long use_count() const // never throws
    {
        return pn.use_count();
    }

    void swap(shared_ptr& other) // never throws
    {
        std::swap(px, other.px);
        pn.swap(other.pn);
    }

    template <typename U> bool _internal_less(const shared_ptr<U>& rhs) const
    {
        return pn < rhs.pn;
    }

private:
    template <typename U> friend class shared_ptr;
    template <typename U> friend class weak_ptr;

    T* px;                     // contained pointer
    detail::shared_count pn;    // reference counter

};  // shared_ptr

template <typename T, typename U> inline bool operator==(const shared_ptr<T>& a, const shared_ptr<U>& b)
{
    return a.get() == b.get();
}

template <typename T, typename U> inline bool operator!=(const shared_ptr<T>& a, const shared_ptr<U>& b)
{
    return a.get() != b.get();
}

template <typename T, typename U> inline bool operator<(const shared_ptr<T>& a, const shared_ptr<U>& b)
{
    return a._internal_less(b);
}

template <typename T, typename U> shared_ptr<T> static_pointer_cast(const shared_ptr<U>& r)
{
    return shared_ptr<T>(r, detail::static_cast_tag());
}

template <typename T, typename U> shared_ptr<T> const_pointer_cast(const shared_ptr<U>& r)
{
    return shared_ptr<T>(r, detail::const_cast_tag());
}

template <typename T, typename U> shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U>& r)
{
    return shared_ptr<T>(r, detail::dynamic_cast_tag());
}
/*
// operator<<
template <typename E, typename T, typename U> std::basic_ostream<E, T>& operator<<(std::basic_ostream<E, T>& os, const shared_ptr<U>& p)
{
    return os << p.get();
}
*/