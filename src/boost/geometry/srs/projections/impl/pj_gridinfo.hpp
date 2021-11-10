// Boost.Geometry
// This file is manually converted from PROJ4

// This file was modified by Oracle on 2018, 2019.
// Modifications copyright (c) 2018-2019, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file is converted from PROJ4, http://trac.osgeo.org/proj
// PROJ4 is originally written by Gerald Evenden (then of the USGS)
// PROJ4 is maintained by Frank Warmerdam
// This file was converted to Geometry Library by Adam Wulkiewicz

// Original copyright notice:
// Author:   Frank Warmerdam, warmerdam@pobox.com

// Copyright (c) 2000, Frank Warmerdam

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef BOOST_GEOMETRY_SRS_PROJECTIONS_IMPL_PJ_GRIDINFO_HPP
#define BOOST_GEOMETRY_SRS_PROJECTIONS_IMPL_PJ_GRIDINFO_HPP


#include <boost/algorithm/string.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/util/math.hpp>

#include <boost/cstdint.hpp>

#include <algorithm>
#include <string>
#include <vector>


namespace boost { namespace geometry { namespace projections
{

namespace detail
{

/************************************************************************/
/*                             swap_words()                             */
/*                                                                      */
/*      Convert the byte order of the given word(s) in place.           */
/************************************************************************/

inline bool is_lsb()
{
    static const int byte_order_test = 1;
    static bool result = (1 == ((const unsigned char *) (&byte_order_test))[0]);
    return result;
}

inline void swap_words( char *data, int word_size, int word_count )
{
    for (int word = 0; word < word_count; word++)
    {
        for (int i = 0; i < word_size/2; i++)
        {
            std::swap(data[i], data[word_size-i-1]);
        }

        data += word_size;
    }
}

inline bool cstr_equal(const char * s1, const char * s2, std::size_t n)
{
    return std::equal(s1, s1 + n, s2);
}

struct is_trimmable_char
{
    inline bool operator()(char ch)
    {
        return ch == '\n' || ch == ' ';
    }
};

// structs originally defined in projects.h

struct pj_ctable
{
    struct lp_t  { double lam, phi; };
    struct flp_t { float lam, phi; };
    struct ilp_t { boost::int32_t lam, phi; };

    std::string id;         // ascii info
    lp_t ll;                // lower left corner coordinates
    lp_t del;               // size of cells
    ilp_t lim;              // limits of conversion matrix
    std::vector<flp_t> cvs; // conversion matrix

    inline void swap(pj_ctable & r)
    {
        id.swap(r.id);
        std::swap(ll, r.ll);
        std::swap(del, r.del);
        std::swap(lim, r.lim);
        cvs.swap(r.cvs);
    }
};

struct pj_gi_load
{
    enum format_t { missing = 0, ntv1, ntv2, gtx, ctable, ctable2 };
    typedef boost::long_long_type offset_t;

    explicit pj_gi_load(std::string const& gname = "",
                        format_t f = missing,
                        offset_t off = 0,
                        bool swap = false)
        : gridname(gname)
        , format(f)
        , grid_offset(off)
        , must_swap(swap)
    {}

    std::string gridname; // identifying name of grid, eg "conus" or ntv2_0.gsb

    format_t format;      // format of this grid, ie "ctable", "ntv1", "ntv2" or "missing".

    offset_t grid_offset; // offset in file, for delayed loading
    bool must_swap;       // only for NTv2

    pj_ctable ct;

    inline void swap(pj_gi_load & r)
    {
        gridname.swap(r.gridname);
        std::swap(format, r.format);
        std::swap(grid_offset, r.grid_offset);
        std::swap(must_swap, r.must_swap);
        ct.swap(r.ct);
    }

};

struct pj_gi
    : pj_gi_load
{
    explicit pj_gi(std::string const& gname = "",
                   pj_gi_load::format_t f = missing,
                   pj_gi_load::offset_t off = 0,
                   bool swap = false)
        : pj_gi_load(gname, f, off, swap)
    {}

    std::vector<pj_gi> children;

    inline void swap(pj_gi & r)
    {
        pj_gi_load::swap(r);
        children.swap(r.children);
    }
};

typedef std::vector<pj_gi> pj_gridinfo;


/************************************************************************/
/*                   pj_gridinfo_load_ctable()                          */
/*                                                                      */
/*      Load the data portion of a ctable formatted grid.               */
/************************************************************************/

// Originally nad_ctable_load() defined in nad_init.c
template <typename IStream>
bool pj_gridinfo_load_ctable(IStream & is, pj_gi_load & gi)
{
    pj_ctable & ct = gi.ct;
    
    // Move the input stream by the size of the proj4 original CTABLE
    std::size_t header_size = 80
                            + 2 * sizeof(pj_ctable::lp_t)
                            + sizeof(pj_ctable::ilp_t)
                            + sizeof(pj_ctable::flp_t*);
    is.seekg(header_size);
    
    // read all the actual shift values
    std::size_t a_size = ct.lim.lam * ct.lim.phi;
    ct.cvs.resize(a_size);
    
    std::size_t ch_size = sizeof(pj_ctable::flp_t) * a_size;
    is.read(reinterpret_cast<char*>(&ct.cvs[0]), ch_size);

    if (is.fail() || std::size_t(is.gcount()) != ch_size)
    {
        ct.cvs.clear();
        //ctable loading failed on fread() - binary incompatible?
        return false;
    }

    return true;
} 

/************************************************************************/
/*                  pj_gridinfo_load_ctable2()                          */
/*                                                                      */
/*      Load the data portion of a ctable2 formatted grid.              */
/************************************************************************/

// Originally nad_ctable2_load() defined in nad_init.c
template <typename IStream>
bool pj_gridinfo_load_ctable2(IStream & is, pj_gi_load & gi)
{
    pj_ctable & ct = gi.ct;

    is.seekg(160);

    // read all the actual shift values
    std::size_t a_size = ct.lim.lam * ct.lim.phi;
    ct.cvs.resize(a_size);

    std::size_t ch_size = sizeof(pj_ctable::flp_t) * a_size;
    is.read(reinterpret_cast<char*>(&ct.cvs[0]), ch_size);

    if (is.fail() || std::size_t(is.gcount()) != ch_size)
    {
        //ctable2 loading failed on fread() - binary incompatible?
        ct.cvs.clear();
        return false;
    }

    if (! is_lsb())
    {
        swap_words(reinterpret_cast<char*>(&ct.cvs[0]), 4, (int)a_size * 2);
    }

    return true;
}

/************************************************************************/
/*                      pj_gridinfo_load_ntv1()                         */
/*                                                                      */
/*      NTv1 format.                                                    */
/*      We process one line at a time.  Note that the array storage     */
/*      direction (e-w) is different in the NTv1 file and what          */
/*      the CTABLE is supposed to have.  The phi/lam are also           */
/*      reversed, and we have to be aware of byte swapping.             */
/************************************************************************/

// originally in pj_gridinfo_load() function
template <typename IStream>
inline bool pj_gridinfo_load_ntv1(IStream & is, pj_gi_load & gi)
{
    static const double s2r = math::d2r<double>() / 3600.0;

    std::size_t const r_size = gi.ct.lim.lam * 2;
    std::size_t const ch_size = sizeof(double) * r_size;

    is.seekg(gi.grid_offset);

    std::vector<double> row_buf(r_size);
    gi.ct.cvs.resize(gi.ct.lim.lam * gi.ct.lim.phi);
    
    for (boost::int32_t row = 0; row < gi.ct.lim.phi; row++ )
    {
        is.read(reinterpret_cast<char*>(&row_buf[0]), ch_size);

        if (is.fail() || std::size_t(is.gcount()) != ch_size)
        {
            gi.ct.cvs.clear();
            return false;
        }

        if (is_lsb())
            swap_words(reinterpret_cast<char*>(&row_buf[0]), 8, (int)r_size);

        // convert seconds to radians
        for (boost::int32_t i = 0; i < gi.ct.lim.lam; i++ )
        {
            pj_ctable::flp_t & cvs = gi.ct.cvs[row * gi.ct.lim.lam + (gi.ct.lim.lam - i - 1)];

            cvs.phi = (float) (row_buf[i*2] * s2r);
            cvs.lam = (float) (row_buf[i*2+1] * s2r);
        }
    }

    return true;
}

/* -------------------------------------------------------------------- */
/*                         pj_gridinfo_load_ntv2()                      */
/*                                                                      */
/*      NTv2 format.                                                    */
/*      We process one line at a time.  Note that the array storage     */
/*      direction (e-w) is different in the NTv2 file and what          */
/*      the CTABLE is supposed to have.  The phi/lam are also           */
/*      reversed, and we have to be aware of byte swapping.             */
/* -------------------------------------------------------------------- */

// originally in pj_gridinfo_load() function
template <typename IStream>
inline bool pj_gridinfo_load_ntv2(IStream & is, pj_gi_load & gi)
{
    static const double s2r = math::d2r<double>() / 3600.0;

    std::size_t const r_size = gi.ct.lim.lam * 4;
    std::size_t const ch_size = sizeof(float) * r_size;

    is.seekg(gi.grid_offset);

    std::vector<float> row_buf(r_size);
    gi.ct.cvs.resize(gi.ct.lim.lam * gi.ct.lim.phi);

    for (boost::int32_t row = 0; row < gi.ct.lim.phi; row++ )
    {
        is.read(reinterpret_cast<char*>(&row_buf[0]), ch_size);

        if (is.fail() || std::size_t(is.gcount()) != ch_size)
        {
            gi.ct.cvs.clear();
            return false;
        }

        if (gi.must_swap)
        {
            swap_words(reinterpret_cast<char*>(&row_buf[0]), 4, (int)r_size);
        }

        // convert seconds to radians
        for (boost::int32_t i = 0; i < gi.ct.lim.lam; i++ )
        {
            pj_ctable::flp_t & cvs = gi.ct.cvs[row * gi.ct.lim.lam + (gi.ct.lim.lam - i - 1)];

            // skip accuracy values
            cvs.phi = (float) (row_buf[i*4] * s2r);
            cvs.lam = (float) (row_buf[i*4+1] * s2r);
        }
    }

    return true;
}

/************************************************************************/
/*                   pj_gridinfo_load_gtx()                             */
/*                                                                      */
/*      GTX format.                                                     */
/************************************************************************/

// originally in pj_gridinfo_load() function
template <typename IStream>
inline bool pj_gridinfo_load_gtx(IStream & is, pj_gi_load & gi)
{
    boost::int32_t words = gi.ct.lim.lam * gi.ct.lim.phi;
    std::size_t const ch_size = sizeof(float) * words;
    
    is.seekg(gi.grid_offset);

    // TODO: Consider changing this unintuitive code
    // NOTE: Vertical shift data (one float per point) is stored in a container
    // holding horizontal shift data (two floats per point).
    gi.ct.cvs.resize((words + 1) / 2);

    is.read(reinterpret_cast<char*>(&gi.ct.cvs[0]), ch_size);

    if (is.fail() || std::size_t(is.gcount()) != ch_size)
    {
        gi.ct.cvs.clear();
        return false;
    }

    if (is_lsb())
    {
        swap_words(reinterpret_cast<char*>(&gi.ct.cvs[0]), 4, words);
    }

    return true;
}

/************************************************************************/
/*                          pj_gridinfo_load()                          */
/*                                                                      */
/*      This function is intended to implement delayed loading of       */
/*      the data contents of a grid file.  The header and related       */
/*      stuff are loaded by pj_gridinfo_init().                         */
/************************************************************************/

template <typename IStream>
inline bool pj_gridinfo_load(IStream & is, pj_gi_load & gi)
{
    if (! gi.ct.cvs.empty())
    {
        return true;
    }

    if (! is.is_open())
    {
        return false;
    }

    // Original platform specific CTable format.
    if (gi.format == pj_gi::ctable)
    {
        return pj_gridinfo_load_ctable(is, gi);
    }
    // CTable2 format.
    else if (gi.format == pj_gi::ctable2)
    {
        return pj_gridinfo_load_ctable2(is, gi);
    }
    // NTv1 format.
    else if (gi.format == pj_gi::ntv1)
    {
        return pj_gridinfo_load_ntv1(is, gi);
    }
    // NTv2 format.
    else if (gi.format == pj_gi::ntv2)
    {
        return pj_gridinfo_load_ntv2(is, gi);
    }
    // GTX format.
    else if (gi.format == pj_gi::gtx)
    {
        return pj_gridinfo_load_gtx(is, gi);
    }
    else
    {
        return false;
    }
}

/************************************************************************/
/*                        pj_gridinfo_parent()                          */
/*                                                                      */
/*      Seek a parent grid file by name from a grid list                */
/************************************************************************/

template <typename It>
inline It pj_gridinfo_parent(It first, It last, std::string const& name)
{
    for ( ; first != last ; ++first)
    {
        if (first->ct.id == name)
            return first;

        It parent = pj_gridinfo_parent(first->children.begin(), first->children.end(), name);
        if( parent != first->children.end() )
            return parent;
    }

    return last;
}

/************************************************************************/
/*                       pj_gridinfo_init_ntv2()                        */
/*                                                                      */
/*      Load a ntv2 (.gsb) file.                                        */
/************************************************************************/

template <typename IStream>
inline bool pj_gridinfo_init_ntv2(std::string const& gridname,
                                  IStream & is,
                                  pj_gridinfo & gridinfo)
{
    BOOST_STATIC_ASSERT( sizeof(boost::int32_t) == 4 );
    BOOST_STATIC_ASSERT( sizeof(double) == 8 );

    static const double s2r = math::d2r<double>() / 3600.0;

    std::size_t gridinfo_orig_size = gridinfo.size();

    // Read the overview header.
    char header[11*16];

    is.read(header, sizeof(header));
    if( is.fail() )
    {
        return false;
    }

    bool must_swap = (header[8] == 11)
                   ? !is_lsb()
                   : is_lsb();

    // NOTE: This check is not implemented in proj4
    if (! cstr_equal(header + 56, "SECONDS", 7))
    {
        return false;
    }

    // Byte swap interesting fields if needed.
    if( must_swap )
    {
        swap_words( header+8, 4, 1 );
        swap_words( header+8+16, 4, 1 );
        swap_words( header+8+32, 4, 1 );
        swap_words( header+8+7*16, 8, 1 );
        swap_words( header+8+8*16, 8, 1 );
        swap_words( header+8+9*16, 8, 1 );
        swap_words( header+8+10*16, 8, 1 );
    }

    // Get the subfile count out ... all we really use for now.
    boost::int32_t num_subfiles;
    memcpy( &num_subfiles, header+8+32, 4 );

    // Step through the subfiles, creating a PJ_GRIDINFO for each.
    for( boost::int32_t subfile = 0; subfile < num_subfiles; subfile++ )
    {
        // Read header.
        is.read(header, sizeof(header));
        if( is.fail() )
        {
            return false;
        }

        if(! cstr_equal(header, "SUB_NAME", 8))
        {
            return false;
        }

        // Byte swap interesting fields if needed.
        if( must_swap )
        {
            swap_words( header+8+16*4, 8, 1 );
            swap_words( header+8+16*5, 8, 1 );
            swap_words( header+8+16*6, 8, 1 );
            swap_words( header+8+16*7, 8, 1 );
            swap_words( header+8+16*8, 8, 1 );
            swap_words( header+8+16*9, 8, 1 );
            swap_words( header+8+16*10, 4, 1 );
        }

        // Initialize a corresponding "ct" structure.
        pj_ctable ct;
        pj_ctable::lp_t ur;

        ct.id = std::string(header + 8, 8);

        ct.ll.lam = - *((double *) (header+7*16+8)); /* W_LONG */
        ct.ll.phi = *((double *) (header+4*16+8));   /* S_LAT */

        ur.lam = - *((double *) (header+6*16+8));     /* E_LONG */
        ur.phi = *((double *) (header+5*16+8));       /* N_LAT */

        ct.del.lam = *((double *) (header+9*16+8));
        ct.del.phi = *((double *) (header+8*16+8));

        ct.lim.lam = (boost::int32_t) (fabs(ur.lam-ct.ll.lam)/ct.del.lam + 0.5) + 1;
        ct.lim.phi = (boost::int32_t) (fabs(ur.phi-ct.ll.phi)/ct.del.phi + 0.5) + 1;

        ct.ll.lam *= s2r;
        ct.ll.phi *= s2r;
        ct.del.lam *= s2r;
        ct.del.phi *= s2r;

        boost::int32_t gs_count;
        memcpy( &gs_count, header + 8 + 16*10, 4 );
        if( gs_count != ct.lim.lam * ct.lim.phi )
        {
            return false;
        }

        //ct.cvs.clear();

        // Create a new gridinfo for this if we aren't processing the
        // 1st subfile, and initialize our grid info.

        // Attach to the correct list or sublist.

        // TODO is offset needed?
        pj_gi gi(gridname, pj_gi::ntv2, is.tellg(), must_swap);
        gi.ct = ct;

        if( subfile == 0 )
        {
            gridinfo.push_back(gi);
        }
        else if( cstr_equal(header+24, "NONE", 4) )
        {
            gridinfo.push_back(gi);
        }
        else
        {
            pj_gridinfo::iterator git = pj_gridinfo_parent(gridinfo.begin() + gridinfo_orig_size,
                                                           gridinfo.end(),
                                                           std::string((const char*)header+24, 8));

            if( git == gridinfo.end() )
            {
                gridinfo.push_back(gi);
            }
            else
            {
                git->children.push_back(gi);
            }
        }

        // Seek past the data.
        is.seekg(gs_count * 16, std::ios::cur);
    }

    return true;
}

/************************************************************************/
/*                       pj_gridinfo_init_ntv1()                        */
/*                                                                      */
/*      Load an NTv1 style Canadian grid shift file.                    */
/************************************************************************/

template <typename IStream>
inline bool pj_gridinfo_init_ntv1(std::string const& gridname,
                                  IStream & is,
                                  pj_gridinfo & gridinfo)
{
    BOOST_STATIC_ASSERT( sizeof(boost::int32_t) == 4 );
    BOOST_STATIC_ASSERT( sizeof(double) == 8 );

    static const double d2r = math::d2r<double>();

    // Read the header.
    char header[176];

    is.read(header, sizeof(header));
    if( is.fail() )
    {
        return false;
    }

    // Regularize fields of interest.
    if( is_lsb() )
    {
        swap_words( header+8, 4, 1 );
        swap_words( header+24, 8, 1 );
        swap_words( header+40, 8, 1 );
        swap_words( header+56, 8, 1 );
        swap_words( header+72, 8, 1 );
        swap_words( header+88, 8, 1 );
        swap_words( header+104, 8, 1 );
    }

    // NTv1 grid shift file has wrong record count, corrupt?
    if( *((boost::int32_t *) (header+8)) != 12 )
    {
        return false;
    }

    // NOTE: This check is not implemented in proj4
    if (! cstr_equal(header + 120, "SECONDS", 7))
    {
        return false;
    }

    // Fill in CTABLE structure.
    pj_ctable ct;
    pj_ctable::lp_t ur;

    ct.id = "NTv1 Grid Shift File";

    ct.ll.lam = - *((double *) (header+72));
    ct.ll.phi = *((double *) (header+24));
    ur.lam = - *((double *) (header+56));
    ur.phi = *((double *) (header+40));
    ct.del.lam = *((double *) (header+104));
    ct.del.phi = *((double *) (header+88));
    ct.lim.lam = (boost::int32_t) (fabs(ur.lam-ct.ll.lam)/ct.del.lam + 0.5) + 1;
    ct.lim.phi = (boost::int32_t) (fabs(ur.phi-ct.ll.phi)/ct.del.phi + 0.5) + 1;

    ct.ll.lam *= d2r;
    ct.ll.phi *= d2r;
    ct.del.lam *= d2r;
    ct.del.phi *= d2r;
    //ct.cvs.clear();

    // is offset needed?
    gridinfo.push_back(pj_gi(gridname, pj_gi::ntv1, is.tellg()));
    gridinfo.back().ct = ct;

    return true;
}

/************************************************************************/
/*                       pj_gridinfo_init_gtx()                         */
/*                                                                      */
/*      Load a NOAA .gtx vertical datum shift file.                     */
/************************************************************************/

template <typename IStream>
inline bool pj_gridinfo_init_gtx(std::string const& gridname,
                                 IStream & is,
                                 pj_gridinfo & gridinfo)
{
    BOOST_STATIC_ASSERT( sizeof(boost::int32_t) == 4 );
    BOOST_STATIC_ASSERT( sizeof(double) == 8 );

    static const double d2r = math::d2r<double>();

    // Read the header.
    char header[40];

    is.read(header, sizeof(header));
    if( is.fail() )
    {
        return false;
    }

    // Regularize fields of interest and extract.
    double         xorigin, yorigin, xstep, ystep;
    boost::int32_t rows, columns;

    if( is_lsb() )
    {
        swap_words( header+0, 8, 4 );
        swap_words( header+32, 4, 2 );
    }

    memcpy( &yorigin, header+0, 8 );
    memcpy( &xorigin, header+8, 8 );
    memcpy( &ystep, header+16, 8 );
    memcpy( &xstep, header+24, 8 );

    memcpy( &rows, header+32, 4 );
    memcpy( &columns, header+36, 4 );

    // gtx file header has invalid extents, corrupt?
    if( xorigin < -360 || xorigin > 360
        || yorigin < -90 || yorigin > 90 )
    {
        return false;
    }

    // Fill in CTABLE structure.
    pj_ctable ct;

    ct.id = "GTX Vertical Grid Shift File";

    ct.ll.lam = xorigin;
    ct.ll.phi = yorigin;
    ct.del.lam = xstep;
    ct.del.phi = ystep;
    ct.lim.lam = columns;
    ct.lim.phi = rows;

    // some GTX files come in 0-360 and we shift them back into the
    // expected -180 to 180 range if possible. This does not solve
    // problems with grids spanning the dateline.
    if( ct.ll.lam >= 180.0 )
        ct.ll.lam -= 360.0;

    if( ct.ll.lam >= 0.0 && ct.ll.lam + ct.del.lam * ct.lim.lam > 180.0 )
    {
        //"This GTX spans the dateline!  This will cause problems." );
    }

    ct.ll.lam *= d2r;
    ct.ll.phi *= d2r;
    ct.del.lam *= d2r;
    ct.del.phi *= d2r;
    //ct.cvs.clear();

    // is offset needed?
    gridinfo.push_back(pj_gi(gridname, pj_gi::gtx, 40));
    gridinfo.back().ct = ct;

    return true;
}

/************************************************************************/
/*                   pj_gridinfo_init_ctable2()                         */
/*                                                                      */
/*      Read the header portion of a "ctable2" format grid.             */
/************************************************************************/

// Originally nad_ctable2_init() defined in nad_init.c
template <typename IStream>
inline bool pj_gridinfo_init_ctable2(std::string const& gridname,
                                     IStream & is,
                                     pj_gridinfo & gridinfo)
{
    BOOST_STATIC_ASSERT( sizeof(boost::int32_t) == 4 );
    BOOST_STATIC_ASSERT( sizeof(double) == 8 );

    char header[160];

    is.read(header, sizeof(header));
    if( is.fail() )
    {
        return false;
    }

    if( !is_lsb() )
    {
        swap_words( header +  96, 8, 4 );
        swap_words( header + 128, 4, 2 );
    }

    // ctable2 - wrong header!
    if (! cstr_equal(header, "CTABLE V2", 9))
    {
        return false;
    }

    // read the table header
    pj_ctable ct;

    ct.id = std::string(header + 16, std::find(header + 16, header + 16 + 80, '\0'));
    //memcpy( &ct.ll.lam,  header +  96, 8 );
    //memcpy( &ct.ll.phi,  header + 104, 8 );
    //memcpy( &ct.del.lam, header + 112, 8 );
    //memcpy( &ct.del.phi, header + 120, 8 );
    //memcpy( &ct.lim.lam, header + 128, 4 );
    //memcpy( &ct.lim.phi, header + 132, 4 );
    memcpy( &ct.ll,  header +  96, 40 );

    // do some minimal validation to ensure the structure isn't corrupt
    if ( (ct.lim.lam < 1) || (ct.lim.lam > 100000) 
      || (ct.lim.phi < 1) || (ct.lim.phi > 100000) )
    {
        return false;
    }
    
    // trim white space and newlines off id
    boost::trim_right_if(ct.id, is_trimmable_char());

    //ct.cvs.clear();

    gridinfo.push_back(pj_gi(gridname, pj_gi::ctable2));
    gridinfo.back().ct = ct;

    return true;
}

/************************************************************************/
/*                    pj_gridinfo_init_ctable()                         */
/*                                                                      */
/*      Read the header portion of a "ctable" format grid.              */
/************************************************************************/

// Originally nad_ctable_init() defined in nad_init.c
template <typename IStream>
inline bool pj_gridinfo_init_ctable(std::string const& gridname,
                                    IStream & is,
                                    pj_gridinfo & gridinfo)
{
    BOOST_STATIC_ASSERT( sizeof(boost::int32_t) == 4 );
    BOOST_STATIC_ASSERT( sizeof(double) == 8 );

    // 80 + 2*8 + 2*8 + 2*4
    char header[120];

    // NOTE: in proj4 data is loaded directly into CTABLE

    is.read(header, sizeof(header));
    if( is.fail() )
    {
        return false;
    }

    // NOTE: in proj4 LSB is not checked here

    // read the table header
    pj_ctable ct;

    ct.id = std::string(header, std::find(header, header + 80, '\0'));
    memcpy( &ct.ll, header + 80, 40 );

    // do some minimal validation to ensure the structure isn't corrupt
    if ( (ct.lim.lam < 1) || (ct.lim.lam > 100000) 
      || (ct.lim.phi < 1) || (ct.lim.phi > 100000) )
    {
        return false;
    }
    
    // trim white space and newlines off id
    boost::trim_right_if(ct.id, is_trimmable_char());

    //ct.cvs.clear();

    gridinfo.push_back(pj_gi(gridname, pj_gi::ctable));
    gridinfo.back().ct = ct;

    return true;
}

/************************************************************************/
/*                          pj_gridinfo_init()                          */
/*                                                                      */
/*      Open and parse header details from a datum gridshift file       */
/*      returning a list of PJ_GRIDINFOs for the grids in that          */
/*      file.  This superceeds use of nad_init() for modern             */
/*      applications.                                                   */
/************************************************************************/

template <typename IStream>
inline bool pj_gridinfo_init(std::string const& gridname,
                             IStream & is,
                             pj_gridinfo & gridinfo)
{
    char header[160];

    // Check if the stream is opened.
    if (! is.is_open()) {
        return false;
    }

    // Load a header, to determine the file type.
    is.read(header, sizeof(header));

    if ( is.fail() ) {
        return false;
    }
    
    is.seekg(0);

    // Determine file type.
    if ( cstr_equal(header + 0, "HEADER", 6)
      && cstr_equal(header + 96, "W GRID", 6)
      && cstr_equal(header + 144, "TO      NAD83   ", 16) )
    {
        return pj_gridinfo_init_ntv1(gridname, is, gridinfo);
    }
    else if( cstr_equal(header + 0, "NUM_OREC", 8)
          && cstr_equal(header + 48, "GS_TYPE", 7) )
    {
        return pj_gridinfo_init_ntv2(gridname, is, gridinfo);
    }
    else if( boost::algorithm::ends_with(gridname, "gtx")
          || boost::algorithm::ends_with(gridname, "GTX") )
    {
        return pj_gridinfo_init_gtx(gridname, is, gridinfo);
    }
    else if( cstr_equal(header + 0, "CTABLE V2", 9) )
    {
        return pj_gridinfo_init_ctable2(gridname, is, gridinfo);
    }
    else
    {
        return pj_gridinfo_init_ctable(gridname, is, gridinfo);
    }
}


} // namespace detail

}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_SRS_PROJECTIONS_IMPL_PJ_GRIDINFO_HPP
