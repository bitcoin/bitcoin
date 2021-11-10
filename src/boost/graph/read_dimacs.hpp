//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Jeremy G. Siek, Andrew Lumsdaine, Lie-Quan Lee
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

/*
  Reads maximal flow problem in extended DIMACS format.
  This works, but could use some polishing.
*/

/* ----------------------------------------------------------------- */

#ifndef BOOST_GRAPH_READ_DIMACS_HPP
#define BOOST_GRAPH_READ_DIMACS_HPP

#include <vector>
#include <iostream>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <boost/graph/graph_traits.hpp>

namespace boost
{

namespace detail
{

    template < class Graph, class CapacityMap, class ReverseEdgeMap >
    int read_dimacs_max_flow_internal(Graph& g, CapacityMap capacity,
        ReverseEdgeMap reverse_edge,
        typename graph_traits< Graph >::vertex_descriptor& src,
        typename graph_traits< Graph >::vertex_descriptor& sink,
        std::istream& in, bool require_source_and_sink,
        const std::string& problem_type)
    {
        //  const int MAXLINE = 100;      /* max line length in the input file
        //  */
        const int ARC_FIELDS = 3; /* no of fields in arc line  */
        const int NODE_FIELDS = 2; /* no of fields in node line  */
        const int P_FIELDS = 3; /* no of fields in problem line */

        typedef
            typename graph_traits< Graph >::vertex_descriptor vertex_descriptor;
        typedef typename graph_traits< Graph >::edge_descriptor edge_descriptor;

        std::vector< vertex_descriptor > verts;

        long m, n, /*  number of edges and nodes */
            i, head, tail, cap;

        long no_lines = 0, /* no of current input line */
            no_plines = 0, /* no of problem-lines */
            no_nslines = 0, /* no of node-source-lines */
            no_nklines = 0, /* no of node-source-lines */
            no_alines = 0; /* no of arc-lines */

        std::string in_line; /* for reading input line */
        char pr_type[4]; /* for reading type of the problem */
        char nd; /* source (s) or sink (t) */

        int k, /* temporary */
            err_no; /* no of detected error */

        /* -------------- error numbers & error messages ---------------- */
        const int EN1 = 0;
        const int EN2 = 1;
        const int EN3 = 2;
        const int EN4 = 3;
        //  const int EN6   = 4;
        //  const int EN10  = 5;
        //  const int EN7   = 6;
        const int EN8 = 7;
        const int EN9 = 8;
        const int EN11 = 9;
        const int EN12 = 10;
        //  const int EN13 = 11;
        const int EN14 = 12;
        const int EN16 = 13;
        const int EN15 = 14;
        const int EN17 = 15;
        const int EN18 = 16;
        const int EN21 = 17;
        const int EN19 = 18;
        const int EN20 = 19;
        const int EN22 = 20;

        static const char* err_message[] = {
            /* 0*/ "more than one problem line.",
            /* 1*/ "wrong number of parameters in the problem line.",
            /* 2*/ "it is not a Max Flow problem line.",
            /* 3*/ "bad value of a parameter in the problem line.",
            /* 4*/ "can't obtain enough memory to solve this problem.",
            /* 5*/ "more than one line with the problem name.",
            /* 6*/ "can't read problem name.",
            /* 7*/ "problem description must be before node description.",
            /* 8*/ "this parser doesn't support multiply sources and sinks.",
            /* 9*/ "wrong number of parameters in the node line.",
            /*10*/ "wrong value of parameters in the node line.",
            /*11*/ " ",
            /*12*/
            "source and sink descriptions must be before arc descriptions.",
            /*13*/ "too many arcs in the input.",
            /*14*/ "wrong number of parameters in the arc line.",
            /*15*/ "wrong value of parameters in the arc line.",
            /*16*/ "unknown line type in the input.",
            /*17*/ "reading error.",
            /*18*/ "not enough arcs in the input.",
            /*19*/ "source or sink doesn't have incident arcs.",
            /*20*/ "can't read anything from the input file."
        };
        /* --------------------------------------------------------------- */

        /* The main loop:
           -  reads the line of the input,
           -  analyses its type,
           -  checks correctness of parameters,
           -  puts data to the arrays,
           -  does service functions
        */

        while (std::getline(in, in_line))
        {
            ++no_lines;

            switch (in_line[0])
            {
            case 'c': /* skip lines with comments */
            case '\n': /* skip empty lines   */
            case '\0': /* skip empty lines at the end of file */
                break;

            case 'p': /* problem description      */
                if (no_plines > 0)
                /* more than one problem line */
                {
                    err_no = EN1;
                    goto error;
                }

                no_plines = 1;

                if (
                    /* reading problem line: type of problem, no of nodes, no of
                       arcs */
                    std::sscanf(
                        in_line.c_str(), "%*c %3s %ld %ld", pr_type, &n, &m)
                    != P_FIELDS)
                /*wrong number of parameters in the problem line*/
                {
                    err_no = EN2;
                    goto error;
                }

                if (pr_type != problem_type)
                /*wrong problem type*/
                {
                    err_no = EN3;
                    goto error;
                }

                if (n <= 0 || m <= 0)
                /*wrong value of no of arcs or nodes*/
                {
                    err_no = EN4;
                    goto error;
                }

                {
                    for (long vi = 0; vi < n; ++vi)
                        verts.push_back(add_vertex(g));
                }
                break;

            case 'n': /* source(s) description */
                if (no_plines == 0)
                /* there was not problem line above */
                {
                    err_no = EN8;
                    goto error;
                }

                /* reading source  or sink */
                k = std::sscanf(in_line.c_str(), "%*c %ld %c", &i, &nd);
                --i; // index from 0
                if (k < NODE_FIELDS)
                /* node line is incorrect */
                {
                    err_no = EN11;
                    goto error;
                }

                if (i < 0 || i > n)
                /* wrong value of node */
                {
                    err_no = EN12;
                    goto error;
                }

                switch (nd)
                {
                case 's': /* source line */

                    if (no_nslines != 0)
                    /* more than one source line */
                    {
                        err_no = EN9;
                        goto error;
                    }

                    no_nslines = 1;
                    src = verts[i];
                    break;

                case 't': /* sink line */

                    if (no_nklines != 0)
                    /* more than one sink line */
                    {
                        err_no = EN9;
                        goto error;
                    }

                    no_nklines = 1;
                    sink = verts[i];
                    break;

                default:
                    /* wrong type of node-line */
                    err_no = EN12;
                    goto error;
                }
                break;

            case 'a': /* arc description */
                if (require_source_and_sink
                    && (no_nslines == 0 || no_nklines == 0))
                /* there was not source and sink description above */
                {
                    err_no = EN14;
                    goto error;
                }

                if (no_alines >= m)
                /*too many arcs on input*/
                {
                    err_no = EN16;
                    goto error;
                }

                if (
                    /* reading an arc description */
                    std::sscanf(
                        in_line.c_str(), "%*c %ld %ld %ld", &tail, &head, &cap)
                    != ARC_FIELDS)
                /* arc description is not correct */
                {
                    err_no = EN15;
                    goto error;
                }

                --tail; // index from 0, not 1
                --head;
                if (tail < 0 || tail > n || head < 0 || head > n)
                /* wrong value of nodes */
                {
                    err_no = EN17;
                    goto error;
                }

                {
                    edge_descriptor e1, e2;
                    bool in1, in2;
                    boost::tie(e1, in1) = add_edge(verts[tail], verts[head], g);
                    boost::tie(e2, in2) = add_edge(verts[head], verts[tail], g);
                    if (!in1 || !in2)
                    {
                        std::cerr << "unable to add edge (" << head << ","
                                  << tail << ")" << std::endl;
                        return -1;
                    }
                    capacity[e1] = cap;
                    capacity[e2] = 0;
                    reverse_edge[e1] = e2;
                    reverse_edge[e2] = e1;
                }
                ++no_alines;
                break;

            default:
                /* unknown type of line */
                err_no = EN18;
                goto error;

            } /* end of switch */
        } /* end of input loop */

        /* ----- all is red  or  error while reading ----- */

        if (in.eof() == 0) /* reading error */
        {
            err_no = EN21;
            goto error;
        }

        if (no_lines == 0) /* empty input */
        {
            err_no = EN22;
            goto error;
        }

        if (no_alines < m) /* not enough arcs */
        {
            err_no = EN19;
            goto error;
        }

        if (require_source_and_sink
            && (out_degree(src, g) == 0 || out_degree(sink, g) == 0))
        /* no arc goes out of the source */
        {
            err_no = EN20;
            goto error;
        }

        /* Thanks God! all is done */
        return (0);

        /* ---------------------------------- */
    error: /* error found reading input */

        std::printf(
            "\nline %ld of input - %s\n", no_lines, err_message[err_no]);

        return -1;
    }
    /* --------------------   end of parser  -------------------*/

} // namespace detail

template < class Graph, class CapacityMap, class ReverseEdgeMap >
int read_dimacs_max_flow(Graph& g, CapacityMap capacity,
    ReverseEdgeMap reverse_edge,
    typename graph_traits< Graph >::vertex_descriptor& src,
    typename graph_traits< Graph >::vertex_descriptor& sink,
    std::istream& in = std::cin)
{
    return detail::read_dimacs_max_flow_internal(
        g, capacity, reverse_edge, src, sink, in, true, "max");
}

template < class Graph, class CapacityMap, class ReverseEdgeMap >
int read_dimacs_min_cut(Graph& g, CapacityMap capacity,
    ReverseEdgeMap reverse_edge, std::istream& in = std::cin)
{
    typename graph_traits< Graph >::vertex_descriptor dummy_src,
        dummy_sink; // Not filled in
    return detail::read_dimacs_max_flow_internal(
        g, capacity, reverse_edge, dummy_src, dummy_sink, in, false, "cut");
}

} // namespace boost

#endif // BOOST_GRAPH_READ_DIMACS_HPP
