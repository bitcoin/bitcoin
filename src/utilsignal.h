/*
 *  Signal.h
 *  A lightweight signals and slots implementation using fast delegates
 *
 *  Created by Patrick Hogan on 5/18/09.
 *
 */

#ifndef _Signal_H_
#define _Signal_H_

#include "utildelegate.h"
#include <set>

namespace Gallant {

template< class Param0 = void >
class Signal0
{
public:
	typedef Delegate0< void > _Delegate;

private:
	typedef std::set<_Delegate> DelegateList;
	typedef typename DelegateList::const_iterator DelegateIterator;
	DelegateList delegateList;

public:
	void Connect( _Delegate delegate )
	{
		delegateList.insert( delegate );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)() )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)() const )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	void Disconnect( _Delegate delegate )
	{
		delegateList.erase( delegate );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)() )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)() const )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	void Clear()
	{
		delegateList.clear();
	}

	void Emit() const
	{
		for (DelegateIterator i = delegateList.begin(); i != delegateList.end(); )
		{
			(*(i++))();
		}
	}

	void operator() () const
	{
		Emit();
	}

	bool Empty() const
	{
        return delegateList.empty();
    }
};


template< class Param1 >
class Signal1
{
public:
	typedef Delegate1< Param1 > _Delegate;

private:
	typedef std::set<_Delegate> DelegateList;
	typedef typename DelegateList::const_iterator DelegateIterator;
	DelegateList delegateList;

public:
	void Connect( _Delegate delegate )
	{
		delegateList.insert( delegate );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1 ) )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1 ) const )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	void Disconnect( _Delegate delegate )
	{
		delegateList.erase( delegate );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1 ) )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1 ) const )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	void Clear()
	{
		delegateList.clear();
	}

	void Emit( Param1 p1 ) const
	{
		for (DelegateIterator i = delegateList.begin(); i != delegateList.end(); )
		{
			(*(i++))( p1 );
		}
	}

	void operator() ( Param1 p1 ) const
	{
		Emit( p1 );
	}

	bool Empty() const
	{
        return delegateList.empty();
    }
};


template< class Param1, class Param2 >
class Signal2
{
public:
	typedef Delegate2< Param1, Param2 > _Delegate;

private:
	typedef std::set<_Delegate> DelegateList;
	typedef typename DelegateList::const_iterator DelegateIterator;
	DelegateList delegateList;

public:
	void Connect( _Delegate delegate )
	{
		delegateList.insert( delegate );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1, Param2 p2 ) )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1, Param2 p2 ) const )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	void Disconnect( _Delegate delegate )
	{
		delegateList.erase( delegate );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1, Param2 p2 ) )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1, Param2 p2 ) const )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	void Clear()
	{
		delegateList.clear();
	}

	void Emit( Param1 p1, Param2 p2 ) const
	{
		for (DelegateIterator i = delegateList.begin(); i != delegateList.end(); )
		{
			(*(i++))( p1, p2 );
		}
	}

	void operator() ( Param1 p1, Param2 p2 ) const
	{
		Emit( p1, p2 );
	}

	bool Empty() const
	{
        return delegateList.empty();
    }
};


template< class Param1, class Param2, class Param3 >
class Signal3
{
public:
	typedef Delegate3< Param1, Param2, Param3 > _Delegate;

private:
	typedef std::set<_Delegate> DelegateList;
	typedef typename DelegateList::const_iterator DelegateIterator;
	DelegateList delegateList;

public:
	void Connect( _Delegate delegate )
	{
		delegateList.insert( delegate );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3 ) )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3 ) const )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	void Disconnect( _Delegate delegate )
	{
		delegateList.erase( delegate );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3 ) )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3 ) const )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	void Clear()
	{
		delegateList.clear();
	}

	void Emit( Param1 p1, Param2 p2, Param3 p3 ) const
	{
		for (DelegateIterator i = delegateList.begin(); i != delegateList.end(); )
		{
			(*(i++))( p1, p2, p3 );
		}
	}

	void operator() ( Param1 p1, Param2 p2, Param3 p3 ) const
	{
		Emit( p1, p2, p3 );
	}

	bool Empty() const
	{
        return delegateList.empty();
    }
};


template< class Param1, class Param2, class Param3, class Param4 >
class Signal4
{
public:
	typedef Delegate4< Param1, Param2, Param3, Param4 > _Delegate;

private:
	typedef std::set<_Delegate> DelegateList;
	typedef typename DelegateList::const_iterator DelegateIterator;
	DelegateList delegateList;

public:
	void Connect( _Delegate delegate )
	{
		delegateList.insert( delegate );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4 ) )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4 ) const )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	void Disconnect( _Delegate delegate )
	{
		delegateList.erase( delegate );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4 ) )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4 ) const )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	void Clear()
	{
		delegateList.clear();
	}

	void Emit( Param1 p1, Param2 p2, Param3 p3, Param4 p4 ) const
	{
		for (DelegateIterator i = delegateList.begin(); i != delegateList.end(); )
		{
			(*(i++))( p1, p2, p3, p4 );
		}
	}

	void operator() ( Param1 p1, Param2 p2, Param3 p3, Param4 p4 ) const
	{
		Emit( p1, p2, p3, p4 );
	}

	bool Empty() const
	{
        return delegateList.empty();
    }
};


template< class Param1, class Param2, class Param3, class Param4, class Param5 >
class Signal5
{
public:
	typedef Delegate5< Param1, Param2, Param3, Param4, Param5 > _Delegate;

private:
	typedef std::set<_Delegate> DelegateList;
	typedef typename DelegateList::const_iterator DelegateIterator;
	DelegateList delegateList;

public:
	void Connect( _Delegate delegate )
	{
		delegateList.insert( delegate );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5 ) )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5 ) const )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	void Disconnect( _Delegate delegate )
	{
		delegateList.erase( delegate );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5 ) )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5 ) const )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	void Clear()
	{
		delegateList.clear();
	}

	void Emit( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5 ) const
	{
		for (DelegateIterator i = delegateList.begin(); i != delegateList.end(); )
		{
			(*(i++))( p1, p2, p3, p4, p5 );
		}
	}

	void operator() ( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5 ) const
	{
		Emit( p1, p2, p3, p4, p5 );
	}

	bool Empty() const
	{
        return delegateList.empty();
    }
};


template< class Param1, class Param2, class Param3, class Param4, class Param5, class Param6 >
class Signal6
{
public:
	typedef Delegate6< Param1, Param2, Param3, Param4, Param5, Param6 > _Delegate;

private:
	typedef std::set<_Delegate> DelegateList;
	typedef typename DelegateList::const_iterator DelegateIterator;
	DelegateList delegateList;

public:
	void Connect( _Delegate delegate )
	{
		delegateList.insert( delegate );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6 ) )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6 ) const )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	void Disconnect( _Delegate delegate )
	{
		delegateList.erase( delegate );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6 ) )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6 ) const )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	void Clear()
	{
		delegateList.clear();
	}

	void Emit( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6 ) const
	{
		for (DelegateIterator i = delegateList.begin(); i != delegateList.end(); )
		{
			(*(i++))( p1, p2, p3, p4, p5, p6 );
		}
	}

	void operator() ( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6 ) const
	{
		Emit( p1, p2, p3, p4, p5, p6 );
	}

	bool Empty() const
	{
        return delegateList.empty();
    }
};


template< class Param1, class Param2, class Param3, class Param4, class Param5, class Param6, class Param7 >
class Signal7
{
public:
	typedef Delegate7< Param1, Param2, Param3, Param4, Param5, Param6, Param7 > _Delegate;

private:
	typedef std::set<_Delegate> DelegateList;
	typedef typename DelegateList::const_iterator DelegateIterator;
	DelegateList delegateList;

public:
	void Connect( _Delegate delegate )
	{
		delegateList.insert( delegate );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7 ) )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7 ) const )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	void Disconnect( _Delegate delegate )
	{
		delegateList.erase( delegate );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7 ) )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7 ) const )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	void Clear()
	{
		delegateList.clear();
	}

	void Emit( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7 ) const
	{
		for (DelegateIterator i = delegateList.begin(); i != delegateList.end(); )
		{
			(*(i++))( p1, p2, p3, p4, p5, p6, p7 );
		}
	}

	void operator() ( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7 ) const
	{
		Emit( p1, p2, p3, p4, p5, p6, p7 );
	}

	bool Empty() const
	{
        return delegateList.empty();
    }
};


template< class Param1, class Param2, class Param3, class Param4, class Param5, class Param6, class Param7, class Param8 >
class Signal8
{
public:
	typedef Delegate8< Param1, Param2, Param3, Param4, Param5, Param6, Param7, Param8 > _Delegate;

private:
	typedef std::set<_Delegate> DelegateList;
	typedef typename DelegateList::const_iterator DelegateIterator;
	DelegateList delegateList;

public:
	void Connect( _Delegate delegate )
	{
		delegateList.insert( delegate );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7, Param8 p8 ) )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Connect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7, Param8 p8 ) const )
	{
		delegateList.insert( MakeDelegate( obj, func ) );
	}

	void Disconnect( _Delegate delegate )
	{
		delegateList.erase( delegate );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7, Param8 p8 ) )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	template< class X, class Y >
	void Disconnect( Y * obj, void (X::*func)( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7, Param8 p8 ) const )
	{
		delegateList.erase( MakeDelegate( obj, func ) );
	}

	void Clear()
	{
		delegateList.clear();
	}

	void Emit( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7, Param8 p8 ) const
	{
		for (DelegateIterator i = delegateList.begin(); i != delegateList.end(); )
		{
			(*(i++))( p1, p2, p3, p4, p5, p6, p7, p8 );
		}
	}

	void operator() ( Param1 p1, Param2 p2, Param3 p3, Param4 p4, Param5 p5, Param6 p6, Param7 p7, Param8 p8 ) const
	{
		Emit( p1, p2, p3, p4, p5, p6, p7, p8 );
	}

	bool Empty() const
	{
        return delegateList.empty();
    }
};


} // namespace

#endif //_SIGNAL_H_

