#pragma once

#include "quick_allocator.hpp"

//
//  detail/shared_count.hpp
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov and Multi Media Ltd.
//  Copyright 2004-2005 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

namespace detail
{
	class sp_counted_base
	{
	private:
	
	    sp_counted_base( sp_counted_base const & );
	    sp_counted_base & operator= ( sp_counted_base const & );
	
	    long use_count_;        // #shared
	    long weak_count_;       // #weak + (#shared != 0)
	
	public:
	
	    sp_counted_base(): use_count_( 1 ), weak_count_( 1 )
	    {
	    }
	
	    virtual ~sp_counted_base() // nothrow
	    {
	    }
	
	    // dispose() is called when use_count_ drops to zero, to release
	    // the resources managed by *this.
	
	    virtual void dispose() = 0; // nothrow

	    // destroy() is called when weak_count_ drops to zero.

	    virtual void destroy() // nothrow
	    {
	        delete this;
	    }

	    void add_ref_copy()
	    {
	        ++use_count_;
	    }
	
	    bool add_ref_lock() // true on success
	    {
	        if( use_count_ == 0 ) return false;
	        ++use_count_;
	        return true;
	    }

	    void release() // nothrow
	    {
	        if( --use_count_ == 0 )
	        {
	            dispose();
	            weak_release();
	        }
	    }

	    void weak_add_ref() // nothrow
	    {
	        ++weak_count_;
	    }

	    void weak_release() // nothrow
	    {
	        if( --weak_count_ == 0 )
	        {
	            destroy();
	        }
	    }

	    long use_count() const // nothrow
	    {
	        return use_count_;
	    }
	};

	template <class X> class sp_counted_impl_p: public sp_counted_base
	{
	private:
	    X * px_;
	
	    sp_counted_impl_p(sp_counted_impl_p const &);
	    sp_counted_impl_p & operator= (sp_counted_impl_p const &);

	public:
	    explicit sp_counted_impl_p(X * px): px_(px)
	    {
	    }

	    virtual void dispose() // nothrow
	    {
	        checked_delete(px_);
	    }
    
    	void* operator new(size_t)
		{
			return quick_allocator<sizeof(sp_counted_impl_p)>::alloc();
		}

		void operator delete(void* p)
		{
        	quick_allocator<sizeof(sp_counted_impl_p)>::dealloc(p);
		}
	};

	class weak_count;

	class shared_count
	{
	private:
    	sp_counted_base * pi_;

    	friend class weak_count;

	public:
    	shared_count(): pi_(0) // nothrow
	    {
	    }

	    template <class Y> explicit shared_count( Y * p ): pi_( 0 )
	    {
	        pi_ = new sp_counted_impl_p<Y>( p );

	        if ( pi_ == 0 )
	        {
            	checked_delete( p );
            	throw std::bad_alloc();
        	}
    	}

    	~shared_count() // nothrow
	    {
	        if( pi_ != 0 ) pi_->release();
	    }

	    shared_count(shared_count const & r): pi_(r.pi_) // nothrow
	    {
	        if( pi_ != 0 ) pi_->add_ref_copy();
	    }

	    explicit shared_count(weak_count const & r); // throws bad_weak_ptr when r.use_count() == 0

	    shared_count & operator= (shared_count const & r) // nothrow
	    {
	        sp_counted_base * tmp = r.pi_;

	        if( tmp != pi_ )
	        {
	            if( tmp != 0 ) tmp->add_ref_copy();
	            if( pi_ != 0 ) pi_->release();
	            pi_ = tmp;
	        }
	
	        return *this;
	    }
	
	    void swap(shared_count & r) // nothrow
	    {
	        sp_counted_base * tmp = r.pi_;
	        r.pi_ = pi_;
	        pi_ = tmp;
    	}
	
	    long use_count() const // nothrow
	    {
	        return pi_ != 0? pi_->use_count(): 0;
	    }
	
	    bool unique() const // nothrow
	    {
	        return use_count() == 1;
	    }
	
	    friend inline bool operator==(shared_count const & a, shared_count const & b)
	    {
	        return a.pi_ == b.pi_;
	    }
	
	    friend inline bool operator<(shared_count const & a, shared_count const & b)
	    {
	        return a.pi_ < b.pi_;
	    }
	};

	class weak_count
	{
	private:
	    sp_counted_base * pi_;

	    friend class shared_count;

	public:
    	weak_count(): pi_(0) // nothrow
	    {
	    }

	    weak_count(shared_count const & r): pi_(r.pi_) // nothrow
	    {
	        if(pi_ != 0) pi_->weak_add_ref();
	    }

	    weak_count(weak_count const & r): pi_(r.pi_) // nothrow
	    {
	        if(pi_ != 0) pi_->weak_add_ref();
	    }

	    ~weak_count() // nothrow
	    {
	        if(pi_ != 0) pi_->weak_release();
	    }

	    weak_count & operator= (shared_count const & r) // nothrow
    	{
    	    sp_counted_base * tmp = r.pi_;
	        if(tmp != 0) tmp->weak_add_ref();
	        if(pi_ != 0) pi_->weak_release();
	        pi_ = tmp;

	        return *this;
	    }

	    weak_count & operator= (weak_count const & r) // nothrow
	    {
	        sp_counted_base * tmp = r.pi_;
	        if(tmp != 0) tmp->weak_add_ref();
	        if(pi_ != 0) pi_->weak_release();
	        pi_ = tmp;
	
	        return *this;
	    }

	    void swap(weak_count & r) // nothrow
	    {
	        sp_counted_base * tmp = r.pi_;
	        r.pi_ = pi_;
	        pi_ = tmp;
	    }
	
	    long use_count() const // nothrow
	    {
	        return pi_ != 0? pi_->use_count(): 0;
	    }
	
	    friend inline bool operator==(weak_count const & a, weak_count const & b)
	    {
	        return a.pi_ == b.pi_;
	    }
	
	    friend inline bool operator<(weak_count const & a, weak_count const & b)
	    {
	        return a.pi_ < b.pi_;
	    }
	};

	inline shared_count::shared_count( weak_count const & r ): pi_( r.pi_ )
	{
	    if( pi_ == 0 || !pi_->add_ref_lock() )
	    {
	        throw 1;
	    }
	}
	
} // namespace detail
