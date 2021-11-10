#ifndef BOOST_PYTHON_SLICE_JDB20040105_HPP
#define BOOST_PYTHON_SLICE_JDB20040105_HPP

// Copyright (c) 2004 Jonathan Brandmeyer
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/detail/prefix.hpp>
#include <boost/config.hpp>
#include <boost/python/object.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/converter/pytype_object_mgr_traits.hpp>

#include <boost/iterator/iterator_traits.hpp>

#include <iterator>
#include <algorithm>

namespace boost { namespace python {

namespace detail
{
  class BOOST_PYTHON_DECL slice_base : public object
  {
   public:
      // Get the Python objects associated with the slice.  In principle, these 
      // may be any arbitrary Python type, but in practice they are usually 
      // integers.  If one or more parameter is ommited in the Python expression 
      // that created this slice, than that parameter is None here, and compares 
      // equal to a default-constructed boost::python::object.
      // If a user-defined type wishes to support slicing, then support for the 
      // special meaning associated with negative indices is up to the user.
      object start() const;
      object stop() const;
      object step() const;
        
   protected:
      explicit slice_base(PyObject*, PyObject*, PyObject*);

      BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(slice_base, object)
  };
}

class slice : public detail::slice_base
{
    typedef detail::slice_base base;
 public:
    // Equivalent to slice(::)
    slice() : base(0,0,0) {}

    // Each argument must be slice_nil, or implicitly convertable to object.
    // They should normally be integers.
    template<typename Integer1, typename Integer2>
    slice( Integer1 start, Integer2 stop)
        : base( object(start).ptr(), object(stop).ptr(), 0 )
    {}
    
    template<typename Integer1, typename Integer2, typename Integer3>
    slice( Integer1 start, Integer2 stop, Integer3 stride)
        : base( object(start).ptr(), object(stop).ptr(), object(stride).ptr() )
    {}
        
    // The following algorithm is intended to automate the process of 
    // determining a slice range when you want to fully support negative
    // indices and non-singular step sizes.  Its functionallity is simmilar to 
    // PySlice_GetIndicesEx() in the Python/C API, but tailored for C++ users.
    // This template returns a slice::range struct that, when used in the 
    // following iterative loop, will traverse a slice of the function's
    // arguments.
    // while (start != end) { 
    //     do_foo(...); 
    //     std::advance( start, step); 
    // }
    // do_foo(...); // repeat exactly once more.
    
    // Arguments: a [begin, end) pair of STL-conforming random-access iterators.
        
    // Return: slice::range, where start and stop define a _closed_ interval
    // that covers at most [begin, end-1] of the provided arguments, and a step 
    // that is non-zero.
    
    // Throws: error_already_set() if any of the indices are neither None nor 
    //   integers, or the slice has a step value of zero.
    // std::invalid_argument if the resulting range would be empty.  Normally, 
    //   you should catch this exception and return an empty sequence of the
    //   appropriate type.
    
    // Performance: constant time for random-access iterators.
    
    // Rationale: 
    //   closed-interval: If an open interval were used, then for a non-singular
    //     value for step, the required state for the end iterator could be 
    //     beyond the one-past-the-end postion of the specified range.  While 
    //     probably harmless, the behavior of STL-conforming iterators is 
    //     undefined in this case.
    //   exceptions on zero-length range: It is impossible to define a closed 
    //     interval over an empty range, so some other form of error checking 
    //     would have to be used by the user to prevent undefined behavior.  In
    //     the case where the user fails to catch the exception, it will simply
    //     be translated to Python by the default exception handling mechanisms.

    template<typename RandomAccessIterator>
    struct range
    {
        RandomAccessIterator start;
        RandomAccessIterator stop;
        typename iterator_difference<RandomAccessIterator>::type step;
    };
    
    template<typename RandomAccessIterator>
    slice::range<RandomAccessIterator>
    get_indices( const RandomAccessIterator& begin, 
        const RandomAccessIterator& end) const
    {
        // This is based loosely on PySlice_GetIndicesEx(), but it has been 
        // carefully crafted to ensure that these iterators never fall out of
        // the range of the container.
        slice::range<RandomAccessIterator> ret;
        
        typedef typename iterator_difference<RandomAccessIterator>::type difference_type;
        difference_type max_dist = std::distance(begin, end);

        object slice_start = this->start();
        object slice_stop = this->stop();
        object slice_step = this->step();
        
        // Extract the step.
        if (slice_step == object()) {
            ret.step = 1;
        }
        else {
            ret.step = extract<long>( slice_step);
            if (ret.step == 0) {
                PyErr_SetString( PyExc_IndexError, "step size cannot be zero.");
                throw_error_already_set();
            }
        }
        
        // Setup the start iterator.
        if (slice_start == object()) {
            if (ret.step < 0) {
                ret.start = end;
                --ret.start;
            }
            else
                ret.start = begin;
        }
        else {
            difference_type i = extract<long>( slice_start);
            if (i >= max_dist && ret.step > 0)
                    throw std::invalid_argument( "Zero-length slice");
            if (i >= 0) {
                ret.start = begin;
                BOOST_USING_STD_MIN();
                std::advance( ret.start, min BOOST_PREVENT_MACRO_SUBSTITUTION(i, max_dist-1));
            }
            else {
                if (i < -max_dist && ret.step < 0)
                    throw std::invalid_argument( "Zero-length slice");
                ret.start = end;
                // Advance start (towards begin) not farther than begin.
                std::advance( ret.start, (-i < max_dist) ? i : -max_dist );
            }
        }
        
        // Set up the stop iterator.  This one is a little trickier since slices
        // define a [) range, and we are returning a [] range.
        if (slice_stop == object()) {
            if (ret.step < 0) {
                ret.stop = begin;
            }
            else {
                ret.stop = end;
                std::advance( ret.stop, -1);
            }
        }
        else {
            difference_type i = extract<long>(slice_stop);
            // First, branch on which direction we are going with this.
            if (ret.step < 0) {
                if (i+1 >= max_dist || i == -1)
                    throw std::invalid_argument( "Zero-length slice");
                
                if (i >= 0) {
                    ret.stop = begin;
                    std::advance( ret.stop, i+1);
                }
                else { // i is negative, but more negative than -1.
                    ret.stop = end;
                    std::advance( ret.stop, (-i < max_dist) ? i : -max_dist);
                }
            }
            else { // stepping forward
                if (i == 0 || -i >= max_dist)
                    throw std::invalid_argument( "Zero-length slice");
                
                if (i > 0) {
                    ret.stop = begin;
                    std::advance( ret.stop, (std::min)( i-1, max_dist-1));
                }
                else { // i is negative, but not more negative than -max_dist
                    ret.stop = end;
                    std::advance( ret.stop, i-1);
                }
            }
        }
        
        // Now the fun part, handling the possibilites surrounding step.
        // At this point, step has been initialized, ret.stop, and ret.step
        // represent the widest possible range that could be traveled
        // (inclusive), and final_dist is the maximum distance covered by the
        // slice.
        typename iterator_difference<RandomAccessIterator>::type final_dist = 
            std::distance( ret.start, ret.stop);
        
        // First case, if both ret.start and ret.stop are equal, then step
        // is irrelevant and we can return here.
        if (final_dist == 0)
            return ret;
        
        // Second, if there is a sign mismatch, than the resulting range and 
        // step size conflict: std::advance( ret.start, ret.step) goes away from
        // ret.stop.
        if ((final_dist > 0) != (ret.step > 0))
            throw std::invalid_argument( "Zero-length slice.");
        
        // Finally, if the last step puts us past the end, we move ret.stop
        // towards ret.start in the amount of the remainder.
        // I don't remember all of the oolies surrounding negative modulii,
        // so I am handling each of these cases separately.
        if (final_dist < 0) {
            difference_type remainder = -final_dist % -ret.step;
            std::advance( ret.stop, remainder);
        }
        else {
            difference_type remainder = final_dist % ret.step;
            std::advance( ret.stop, -remainder);
        }
        
        return ret;
    }

    // Incorrect spelling. DO NOT USE. Only here for backward compatibility.
    // Corrected 2011-06-14.
    template<typename RandomAccessIterator>
    slice::range<RandomAccessIterator>
    get_indicies( const RandomAccessIterator& begin, 
        const RandomAccessIterator& end) const
    {
        return get_indices(begin, end);
    }
        
 public:
    // This declaration, in conjunction with the specialization of 
    // object_manager_traits<> below, allows C++ functions accepting slice 
    // arguments to be called from from Python.  These constructors should never
    // be used in client code.
    BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(slice, detail::slice_base)
};


namespace converter {

template<>
struct object_manager_traits<slice>
    : pytype_object_manager_traits<&PySlice_Type, slice>
{
};
    
} // !namesapce converter

} } // !namespace ::boost::python


#endif // !defined BOOST_PYTHON_SLICE_JDB20040105_HPP
