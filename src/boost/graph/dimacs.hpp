// Copyright 2005 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Alex Breuer
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_DIMACS_HPP
#define BOOST_GRAPH_DIMACS_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iterator>
#include <exception>
#include <vector>
#include <queue>
#include <boost/assert.hpp>

namespace boost
{
namespace graph
{

    class BOOST_SYMBOL_VISIBLE dimacs_exception : public std::exception
    {
    };

    class dimacs_basic_reader
    {
    public:
        typedef std::size_t vertices_size_type;
        typedef std::size_t edges_size_type;
        typedef double vertex_weight_type;
        typedef double edge_weight_type;
        typedef std::pair< vertices_size_type, vertices_size_type > edge_type;
        enum incr_mode
        {
            edge,
            edge_weight
        };

        dimacs_basic_reader(std::istream& in, bool want_weights = true)
        : inpt(in), seen_edges(0), want_weights(want_weights)
        {
            while (getline(inpt, buf) && !buf.empty() && buf[0] == 'c')
                ;

            if (buf[0] != 'p')
            {
                boost::throw_exception(dimacs_exception());
            }

            std::stringstream instr(buf);
            std::string junk;

            instr >> junk >> junk >> num_vertices >> num_edges;
            read_edge_weights.push(-1);
            incr(edge_weight);
        }

        // for a past the end iterator
        dimacs_basic_reader()
        : inpt(std::cin)
        , num_vertices(0)
        , num_edges(0)
        , seen_edges(0)
        , want_weights(false)
        {
        }

        edge_type edge_deref()
        {
            BOOST_ASSERT(!read_edges.empty());
            return read_edges.front();
        }

        inline edge_type* edge_ref()
        {
            BOOST_ASSERT(!read_edges.empty());
            return &read_edges.front();
        }

        inline edge_weight_type edge_weight_deref()
        {
            BOOST_ASSERT(!read_edge_weights.empty());
            return read_edge_weights.front();
        }

        inline dimacs_basic_reader incr(incr_mode mode)
        {
            if (mode == edge)
            {
                BOOST_ASSERT(!read_edges.empty());
                read_edges.pop();
            }
            else if (mode == edge_weight)
            {
                BOOST_ASSERT(!read_edge_weights.empty());
                read_edge_weights.pop();
            }

            if ((mode == edge && read_edges.empty())
                || (mode == edge_weight && read_edge_weights.empty()))
            {

                if (seen_edges > num_edges)
                {
                    boost::throw_exception(dimacs_exception());
                }

                while (getline(inpt, buf) && !buf.empty() && buf[0] == 'c')
                    ;

                if (!inpt.eof())
                {
                    int source, dest, weight;
                    read_edge_line((char*)buf.c_str(), source, dest, weight);

                    seen_edges++;
                    source--;
                    dest--;

                    read_edges.push(edge_type(source, dest));
                    if (want_weights)
                    {
                        read_edge_weights.push(weight);
                    }
                }
                BOOST_ASSERT(read_edges.size() < 100);
                BOOST_ASSERT(read_edge_weights.size() < 100);
            }

            // the 1000000 just happens to be about how many edges can be read
            // in 10s
            //     if( !(seen_edges % 1000000) && !process_id( pg ) && mode ==
            //     edge ) {
            //       std::cout << "read " << seen_edges << " edges" <<
            //       std::endl;
            //     }
            return *this;
        }

        inline bool done_edges()
        {
            return inpt.eof() && read_edges.size() == 0;
        }

        inline bool done_edge_weights()
        {
            return inpt.eof() && read_edge_weights.size() == 0;
        }

        inline vertices_size_type n_vertices() { return num_vertices; }

        inline vertices_size_type processed_edges()
        {
            return seen_edges - read_edges.size();
        }

        inline vertices_size_type processed_edge_weights()
        {
            return seen_edges - read_edge_weights.size();
        }

        inline vertices_size_type n_edges() { return num_edges; }

    protected:
        bool read_edge_line(char* linebuf, int& from, int& to, int& weight)
        {
            char *fs = NULL, *ts = NULL, *ws = NULL;
            char* tmp = linebuf + 2;

            fs = tmp;
            if ('e' == linebuf[0])
            {
                while (*tmp != '\n' && *tmp != '\0')
                {
                    if (*tmp == ' ')
                    {
                        *tmp = '\0';
                        ts = ++tmp;
                        break;
                    }
                    tmp++;
                }
                *tmp = '\0';
                if (NULL == fs || NULL == ts)
                    return false;
                from = atoi(fs);
                to = atoi(ts);
                weight = 0;
            }
            else if ('a' == linebuf[0])
            {
                while (*tmp != '\n' && *tmp != '\0')
                {
                    if (*tmp == ' ')
                    {
                        *tmp = '\0';
                        ts = ++tmp;
                        break;
                    }
                    tmp++;
                }
                while (*tmp != '\n' && *tmp != '\0')
                {
                    if (*tmp == ' ')
                    {
                        *tmp = '\0';
                        ws = ++tmp;
                        break;
                    }
                    tmp++;
                }
                while (*tmp != '\n' && *tmp != '\0')
                    tmp++;
                *tmp = '\0';
                if (fs == NULL || ts == NULL || ws == NULL)
                    return false;
                from = atoi(fs);
                to = atoi(ts);
                if (want_weights)
                    weight = atoi(ws);
                else
                    weight = 0;
            }
            else
            {
                return false;
            }

            return true;
        }

        std::queue< edge_type > read_edges;
        std::queue< edge_weight_type > read_edge_weights;

        std::istream& inpt;
        std::string buf;
        vertices_size_type num_vertices, num_edges, seen_edges;
        bool want_weights;
    };

    template < typename T > class dimacs_edge_iterator
    {
    public:
        typedef dimacs_basic_reader::edge_type edge_type;
        typedef dimacs_basic_reader::incr_mode incr_mode;

        typedef std::input_iterator_tag iterator_category;
        typedef edge_type value_type;
        typedef value_type reference;
        typedef edge_type* pointer;
        typedef std::ptrdiff_t difference_type;

        dimacs_edge_iterator(T& reader) : reader(reader) {}

        inline dimacs_edge_iterator& operator++()
        {
            reader.incr(dimacs_basic_reader::edge);
            return *this;
        }

        inline edge_type operator*() { return reader.edge_deref(); }

        inline edge_type* operator->() { return reader.edge_ref(); }

        // don't expect this to do the right thing if you're not comparing
        // against a general past-the-end-iterator made with the default
        // constructor for dimacs_basic_reader
        inline bool operator==(dimacs_edge_iterator arg)
        {
            if (reader.n_vertices() == 0)
            {
                return arg.reader.done_edges();
            }
            else if (arg.reader.n_vertices() == 0)
            {
                return reader.done_edges();
            }
            else
            {
                return false;
            }
            return false;
        }

        inline bool operator!=(dimacs_edge_iterator arg)
        {
            if (reader.n_vertices() == 0)
            {
                return !arg.reader.done_edges();
            }
            else if (arg.reader.n_vertices() == 0)
            {
                return !reader.done_edges();
            }
            else
            {
                return true;
            }
            return true;
        }

    private:
        T& reader;
    };

    template < typename T > class dimacs_edge_weight_iterator
    {
    public:
        typedef dimacs_basic_reader::edge_weight_type edge_weight_type;
        typedef dimacs_basic_reader::incr_mode incr_mode;

        dimacs_edge_weight_iterator(T& reader) : reader(reader) {}

        inline dimacs_edge_weight_iterator& operator++()
        {
            reader.incr(dimacs_basic_reader::edge_weight);
            return *this;
        }

        inline edge_weight_type operator*()
        {
            return reader.edge_weight_deref();
        }

        // don't expect this to do the right thing if you're not comparing
        // against a general past-the-end-iterator made with the default
        // constructor for dimacs_basic_reader
        inline bool operator==(dimacs_edge_weight_iterator arg)
        {
            if (reader.n_vertices() == 0)
            {
                return arg.reader.done_edge_weights();
            }
            else if (arg.reader.n_vertices() == 0)
            {
                return reader.done_edge_weights();
            }
            else
            {
                return false;
            }
            return false;
        }

        inline bool operator!=(dimacs_edge_weight_iterator arg)
        {
            if (reader.n_vertices() == 0)
            {
                return !arg.reader.done_edge_weights();
            }
            else if (arg.reader.n_vertices() == 0)
            {
                return !reader.done_edge_weights();
            }
            else
            {
                return true;
            }
            return true;
        }

    private:
        T& reader;
    };

}
} // end namespace boost::graph
#endif
