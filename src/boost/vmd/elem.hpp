
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_ELEM_HPP)
#define BOOST_VMD_ELEM_HPP

#include <boost/vmd/detail/setup.hpp>

#if BOOST_PP_VARIADICS

#include <boost/vmd/detail/modifiers.hpp>
#include <boost/vmd/detail/sequence_elem.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/** \def BOOST_VMD_ELEM(elem,...)

    \brief Accesses an element of a sequence.
  
    elem      = A sequence element number. From 0 to sequence size - 1. <br/>
    ...       = Variadic parameters.
    
    The first variadic parameter is required and is the sequence to access.
    Further variadic parameters are all optional.
    
    With no further variadic parameters the macro returns the particular element
    in the sequence. If the element number is outside the bounds of the sequence
    macro access fails and the macro turns emptiness.
    
    Optional parameters determine what it means that an element is successfully
    accessed as well as what data is returned by the macro.
    
    Filters: specifying a VMD type tells the macro to return the element only
             if it is of the VMD type specified, else macro access fails. If more than
             one VMD type is specified as an optional parameter the last one
             specified is the filter.
             
    Matching Identifiers: If the filter is specified as the identifier type, BOOST_VMD_TYPE_IDENTIFIER,
             optional parameters which are identifiers specify that the element accessed
             must match one of the identifiers else access fails. The identifiers may be specified multiple
             times as single optional parameters or once as a tuple of identifier
             parameters. If the identifiers are specified as single optional parameters
             they cannot be any of the specific BOOST_VMD_ optional parameters in order to be
             recognized as matching identifiers. Normally this should never be the case.
             The only situation where this could occur is if the VMD types, which are filters,
             are used as matching identifiers; in this case the matching identifiers need
             to be passed as a tuple of identifier parameters so they are not treated
             as filters.
             
    Filters and matching identifiers change what it means that an element is successfully
    accessed. They do not change what data is returned by the macro. The remaining optional
    parameters do not change what it means that an element is successfully accessed but they
    do change what data is returned by the macro.
             
  @code
  
    Splitting: Splitting allows the macro to return the rest of the sequence
             after the element accessed.
    
             If BOOST_VMD_RETURN_AFTER is specified the return is a tuple
             with the element accessed as the first tuple parameter and the rest of
             the sequence as the second tuple parameter. If element access fails 
             both tuple parameters are empty. 
             
             If BOOST_VMD_RETURN_ONLY_AFTER
             is specified the return is the rest of the sequence after the element accessed
             found. If the element access fails the return is emptiness. 
             
             If BOOST_VMD_RETURN_NO_AFTER, the default, is specified no splitting
             occurs. 
             
             If more than one of the splitting identifiers are specified
             the last one specified determines the splitting.
             
     Return Type: The element accessed can be changed to return both the type
             of the element as well as the element data with optional return type 
             parameters. When a type is returned, the element accessed which is returned becomes a
             two-element tuple where the type of the element accessed is the first tuple element and the element
             data itself is the second tuple element. If the macro fails to access the
             element the element access returned is emptiness and not a tuple. 
             
             If BOOST_VMD_RETURN_NO_TYPE, the default, is specified no type is returned 
             as part of the element accessed. 
             
             If BOOST_VMD_RETURN_TYPE is specified the specific type of the element
             is returned in the tuple. 
             
             If BOOST_VMD_RETURN_TYPE_ARRAY is specified
             an array type is returned if the element is an array, else a tuple
             type is returned if the element is a tuple, else the actual type
             is returned for non-tuple data. 
             
             If BOOST_VMD_RETURN_TYPE_LIST is specified
             a list type is returned if the element is a list, else a tuple
             type is returned if the element is a tuple, else the actual type
             is returned for non-tuple data. 
             
             If BOOST_VMD_RETURN_TYPE_TUPLE is specified
             a tuple type is returned for all tuple-like data, else the actual type
             is returned for non-tuple data. 
             
             If more than one return type optional
             parameter is specified the last one specified determines the return type.
             
             If a filter is specified optional return type parameters are ignored and
             the default BOOST_VMD_RETURN_NO_TYPE is in effect.
             
     Index:  If the filter is specified as the identifier type, BOOST_VMD_TYPE_IDENTIFIER,
             and matching identifiers are specified, an index parameter specifies that the
             numeric index, starting with 0, of the matching identifier found, be returned
             as part of the result. 
             
             If BOOST_VMD_RETURN_INDEX is specified an index is returned
             as part of the result. 
             
             If BOOST_VMD_RETURN_NO_INDEX, the default, is specified
             no index is returned as part of the result. 
             
             If both are specified the last one specified determines the index parameter. 
             
             When an index is returned as part of the result, the result is a tuple where the 
             element accessed is the first tuple parameter and the index is the last tuple parameter. 
             If element access fails the index is empty. If there is no BOOST_VMD_TYPE_IDENTIFIER 
             filter or if there are no matching identifiers the BOOST_VMD_RETURN_INDEX is ignored 
             and no index is returned as part of the result.
    
  @endcode
  
    returns   = With no optional parameters the element accessed is returned, or emptiness if
                element is outside the bounds of the sequence. Filters and matching identifiers
                can change the meaning of whether the element accessed is returned or failure
                occurs, but whenever failure occurs emptiness is returned as the element access part
                of that failure, else the element accessed is returned. Return type optional parameters,
                when filters are not used, return the element accessed as a two-element tuple
                where the first tuple element is the type and the second tuple element is the
                data; if the element is not accessed then emptiness is returned as the element access
                and not a tuple. Splitting with BOOST_VMD_RETURN_AFTER returns a tuple where the element accessed
                is the first tuple element and the rest of the sequence is the second tuple element.
                Splitting with BOOST_VMD_RETURN_ONLY_AFTER returns the rest of the sequence after
                the element accessed or emptiness if the element can not be accessed. Indexing
                returns the index as part of the output only if filtering with 
                BOOST_VMD_TYPE_IDENTIFIER is specified and matching identifiers are specified.
                When the index is returned with BOOST_VMD_RETURN_AFTER it is the third element
                of the tuple returned, else it is the second element of a tuple where the element
                accessed is the first element of the tuple.
    
*/

#define BOOST_VMD_ELEM(elem,...) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM(BOOST_VMD_ALLOW_ALL,elem,__VA_ARGS__) \
/**/

/** \def BOOST_VMD_ELEM_D(d,elem,...)

    \brief Accesses an element of a sequence. Re-entrant version.
  
    d         = The next available BOOST_PP_WHILE iteration. <br/>
    elem      = A sequence element number. From 0 to sequence size - 1. <br/>
    ...       = Variadic parameters.
    
    The first variadic parameter is required and is the sequence to access.
    Further variadic parameters are all optional.
    
    With no further variadic parameters the macro returns the particular element
    in the sequence. If the element number is outside the bounds of the sequence
    macro access fails and the macro turns emptiness.
    
    Optional parameters determine what it means that an element is successfully
    accessed as well as what data is returned by the macro.
    
    Filters: specifying a VMD type tells the macro to return the element only
             if it is of the VMD type specified, else macro access fails. If more than
             one VMD type is specified as an optional parameter the last one
             specified is the filter.
             
    Matching Identifiers: If the filter is specified as the identifier type, BOOST_VMD_TYPE_IDENTIFIER,
             optional parameters which are identifiers specify that the element accessed
             must match one of the identifiers else access fails. The identifiers may be specified multiple
             times as single optional parameters or once as a tuple of identifier
             parameters. If the identifiers are specified as single optional parameters
             they cannot be any of the specific BOOST_VMD_ optional parameters in order to be
             recognized as matching identifiers. Normally this should never be the case.
             The only situation where this could occur is if the VMD types, which are filters,
             are used as matching identifiers; in this case the matching identifiers need
             to be passed as a tuple of identifier parameters so they are not treated
             as filters.
             
    Filters and matching identifiers change what it means that an element is successfully
    accessed. They do not change what data is returned by the macro. The remaining optional
    parameters do not change what it means that an element is successfully accessed but they
    do change what data is returned by the macro.
             
  @code
  
    Splitting: Splitting allows the macro to return the rest of the sequence
             after the element accessed.
    
             If BOOST_VMD_RETURN_AFTER is specified the return is a tuple
             with the element accessed as the first tuple parameter and the rest of
             the sequence as the second tuple parameter. If element access fails 
             both tuple parameters are empty. 
             
             If BOOST_VMD_RETURN_ONLY_AFTER
             is specified the return is the rest of the sequence after the element accessed
             found. If the element access fails the return is emptiness. 
             
             If BOOST_VMD_RETURN_NO_AFTER, the default, is specified no splitting
             occurs. 
             
             If more than one of the splitting identifiers are specified
             the last one specified determines the splitting.
             
     Return Type: The element accessed can be changed to return both the type
             of the element as well as the element data with optional return type 
             parameters. When a type is returned, the element accessed which is returned becomes a
             two-element tuple where the type of the element accessed is the first tuple element and the element
             data itself is the second tuple element. If the macro fails to access the
             element the element access returned is emptiness and not a tuple. 
             
             If BOOST_VMD_RETURN_NO_TYPE, the default, is specified no type is returned 
             as part of the element accessed. 
             
             If BOOST_VMD_RETURN_TYPE is specified the specific type of the element
             is returned in the tuple. 
             
             If BOOST_VMD_RETURN_TYPE_ARRAY is specified
             an array type is returned if the element is an array, else a tuple
             type is returned if the element is a tuple, else the actual type
             is returned for non-tuple data. 
             
             If BOOST_VMD_RETURN_TYPE_LIST is specified
             a list type is returned if the element is a list, else a tuple
             type is returned if the element is a tuple, else the actual type
             is returned for non-tuple data. 
             
             If BOOST_VMD_RETURN_TYPE_TUPLE is specified
             a tuple type is returned for all tuple-like data, else the actual type
             is returned for non-tuple data. If more than one return type optional
             parameter is specified the last one specified determines the return type.
             
             If a filter is specified optional return type parameters are ignored and
             the default BOOST_VMD_RETURN_NO_TYPE is in effect.
             
     Index:  If the filter is specified as the identifier type, BOOST_VMD_TYPE_IDENTIFIER,
             and matching identifiers are specified, an index parameter specifies that the
             numeric index, starting with 0, of the matching identifier found, be returned
             as part of the result. 
             
             If BOOST_VMD_RETURN_INDEX is specified an index is returned
             as part of the result. 
             
             If BOOST_VMD_RETURN_NO_INDEX, the default, is specified
             no index is returned as part of the result. 
             
             If both are specified the last one specified determines the index parameter. 
             
             When an index is returned as part of the result, the result is a tuple where the 
             element accessed is the first tuple parameter and the index is the last tuple parameter. 
             If element access fails the index is empty. If there is no BOOST_VMD_TYPE_IDENTIFIER 
             filter or if there are no matching identifiers the BOOST_VMD_RETURN_INDEX is ignored 
             and no index is returned as part of the result.
    
  @endcode
  
    returns   = With no optional parameters the element accessed is returned, or emptiness if
                element is outside the bounds of the sequence. Filters and matching identifiers
                can change the meaning of whether the element accessed is returned or failure
                occurs, but whenever failure occurs emptiness is returned as the element access part
                of that failure, else the element accessed is returned. Return type optional parameters,
                when filters are not used, return the element accessed as a two-element tuple
                where the first tuple element is the type and the second tuple element is the
                data; if the element is not accessed then emptiness is returned as the element access
                and not a tuple. Splitting with BOOST_VMD_RETURN_AFTER returns a tuple where the element accessed
                is the first tuple element and the rest of the sequence is the second tuple element.
                Splitting with BOOST_VMD_RETURN_ONLY_AFTER returns the rest of the sequence after
                the element accessed or emptiness if the element can not be accessed. Indexing
                returns the index as part of the output only if filtering with 
                BOOST_VMD_TYPE_IDENTIFIER is specified and matching identifiers are specified.
                When the index is returned with BOOST_VMD_RETURN_AFTER it is the third element
                of the tuple returned, else it is the second element of a tuple where the element
                accessed is the first element of the tuple.
    
*/

#define BOOST_VMD_ELEM_D(d,elem,...) \
    BOOST_VMD_DETAIL_SEQUENCE_ELEM_D(d,BOOST_VMD_ALLOW_ALL,elem,__VA_ARGS__) \
/**/

#endif /* BOOST_PP_VARIADICS */
#endif /* BOOST_VMD_ELEM_HPP */
