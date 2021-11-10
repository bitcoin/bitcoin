//----------------------------------------------------------------------------
/// @file time_measure.hpp
/// @brief This class is done in order to simplify the time measure in the
///        benchmaark programs
///
/// @author Copyright (c) 2010 2015 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanyingfile LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_PARALLEL_TOOLS_TIME_MEASURE_HPP
#define __BOOST_SORT_PARALLEL_TOOLS_TIME_MEASURE_HPP

#include <chrono>

namespace boost       
{
namespace sort        
{
namespace common      
{

namespace chrn = std::chrono;
//
//***************************************************************************
//                D E F I N I T I O N S
//***************************************************************************
typedef chrn::steady_clock::time_point           time_point;

time_point now ( );
double subtract_time  ( const time_point & t1, const time_point & t2 );
//
//---------------------------------------------------------------------------
//  function : now
/// @brief return the time system in a internal format ( steady_clock)
/// @return time in steady_clock format
//---------------------------------------------------------------------------
time_point now ( ) {   return chrn::steady_clock::now( ); };
//
//---------------------------------------------------------------------------
//  function : subtract_time
/// @brief return the time in double format
/// @param [in] t1 : first  time in time_point format
/// @param [in] t2 : second time in time_point format
/// @return time in seconds of the difference of t1 - t2
//---------------------------------------------------------------------------
double subtract_time  ( const time_point & t1, const time_point & t2 )
{   //------------------------ begin ---------------------------------
    chrn::duration<double> time_span =
                chrn::duration_cast < chrn::duration < double > > ( t1 - t2 );
    return  time_span.count( );
};

//***************************************************************************
};//    End namespace benchmark
};//    End namespace sort
};//    End namespace boost
//***************************************************************************
#endif
