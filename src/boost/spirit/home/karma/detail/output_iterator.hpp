//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_OUTPUT_ITERATOR_MAY_26_2007_0506PM)
#define BOOST_SPIRIT_KARMA_OUTPUT_ITERATOR_MAY_26_2007_0506PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <iterator>
#include <vector>
#include <algorithm>

#include <boost/config.hpp>
#include <boost/noncopyable.hpp>
#include <boost/mpl/if.hpp>

#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/support/iterators/ostream_iterator.hpp>
#include <boost/spirit/home/support/unused.hpp>

#if defined(BOOST_MSVC) && defined(BOOST_SPIRIT_UNICODE)
#include <boost/spirit/home/support/char_encoding/unicode.hpp>
#endif

namespace boost { namespace spirit { namespace karma { namespace detail 
{
    ///////////////////////////////////////////////////////////////////////////
    //  This class is used to keep track of the current position in the output.
    ///////////////////////////////////////////////////////////////////////////
    class position_sink 
    {
    public:
        position_sink() : count(0), line(1), column(1) {}
        void tidy() { count = 0; line = 1; column = 1; }

        template <typename T>
        void output(T const& value) 
        {
            ++count; 
            if (value == '\n') {
                ++line;
                column = 1;
            }
            else {
                ++column;
            }
        }
        std::size_t get_count() const { return count; }
        std::size_t get_line() const { return line; }
        std::size_t get_column() const { return column; }

    private:
        std::size_t count;
        std::size_t line;
        std::size_t column;
    };

    ///////////////////////////////////////////////////////////////////////////
    struct position_policy
    {
        position_policy() {}
        position_policy(position_policy const& rhs) 
          : track_position_data(rhs.track_position_data) {}

        template <typename T>
        void output(T const& value) 
        {
            // track position in the output 
            track_position_data.output(value);
        }

        // return the current count in the output
        std::size_t get_out_count() const
        {
            return track_position_data.get_count();
        }

	// return the current line in the output
	std::size_t get_line() const
	{
	    return track_position_data.get_line();
	}

	// return the current column in the output
	std::size_t get_column() const
	{
	    return track_position_data.get_column();
	}

    private:
        position_sink track_position_data;            // for position tracking
    };

    struct no_position_policy
    {
        no_position_policy() {}
        no_position_policy(no_position_policy const&) {}

        template <typename T>
        void output(T const& /*value*/) {}
    };

    ///////////////////////////////////////////////////////////////////////////
    //  This class is used to count the number of characters streamed into the 
    //  output.
    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator>
    class counting_sink : boost::noncopyable
    {
    public:
        counting_sink(OutputIterator& sink_, std::size_t count_ = 0
              , bool enabled = true) 
          : count(count_), initial_count(count), prev_count(0), sink(sink_)
        {
            prev_count = sink.chain_counting(enabled ? this : NULL);
        }
        ~counting_sink() 
        {
            if (prev_count)           // propagate count 
                prev_count->update_count(count-initial_count);
            sink.chain_counting(prev_count);
        }

        void output() 
        {
            ++count; 
        }
        std::size_t get_count() const { return count; }

        // propagate count from embedded counters
        void update_count(std::size_t c)
        {
            count += c;
        }

    private:
        std::size_t count;
        std::size_t initial_count;
        counting_sink* prev_count;                // previous counter in chain
        OutputIterator& sink;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator>
    struct counting_policy
    {
    public:
        counting_policy() : count(NULL) {}
        counting_policy(counting_policy const& rhs) : count(rhs.count) {}

        // functions related to counting
        counting_sink<OutputIterator>* chain_counting(
            counting_sink<OutputIterator>* count_data)
        {
            counting_sink<OutputIterator>* prev_count = count;
            count = count_data;
            return prev_count;
        }

        template <typename T>
        void output(T const&) 
        {
            // count characters, if appropriate
            if (NULL != count)
                count->output();
        }

    private:
        counting_sink<OutputIterator>* count;      // for counting
    };

    struct no_counting_policy
    {
        no_counting_policy() {}
        no_counting_policy(no_counting_policy const&) {}

        template <typename T>
        void output(T const& /*value*/) {}
    };

    ///////////////////////////////////////////////////////////////////////////
    //  The following classes are used to intercept the output into a buffer
    //  allowing to do things like alignment, character escaping etc.
    ///////////////////////////////////////////////////////////////////////////
    class buffer_sink : boost::noncopyable
    {
       // wchar_t is only 16-bits on Windows. If BOOST_SPIRIT_UNICODE is
       // defined, the character type is 32-bits wide so we need to make
       // sure the buffer is at least that wide.
#if (defined(_MSC_VER) || defined(__SIZEOF_WCHAR_T__) && __SIZEOF_WCHAR_T__ == 2) && defined(BOOST_SPIRIT_UNICODE)
       typedef spirit::char_encoding::unicode::char_type buffer_char_type;
#else
       typedef wchar_t buffer_char_type;
#endif

    public:
        buffer_sink()
          : width(0) {}

        ~buffer_sink() 
        {
            tidy(); 
        }

        void enable(std::size_t width_) 
        {
            tidy();             // release existing buffer
            width = (width_ == std::size_t(-1)) ? 0 : width_;
            buffer.reserve(width); 
        }

        void tidy() 
        {
            buffer.clear(); 
            width = 0; 
        }

        template <typename T>
        void output(T const& value)
        {
            BOOST_STATIC_ASSERT(sizeof(T) <= sizeof(buffer_char_type));
            buffer.push_back(value);
        }

        template <typename OutputIterator_>
        bool copy(OutputIterator_& sink, std::size_t maxwidth) const 
        {
#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable: 4267)
#endif
            typename std::basic_string<buffer_char_type>::const_iterator end = 
                buffer.begin() + (std::min)(buffer.size(), maxwidth);

#if defined(BOOST_MSVC)
#pragma warning(disable: 4244) // conversion from 'x' to 'y', possible loss of data
#endif
            std::copy(buffer.begin(), end, sink);
#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif
            return true;
        }
        template <typename RestIterator>
        bool copy_rest(RestIterator& sink, std::size_t start_at) const 
        {
#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable: 4267)
#endif
            typename std::basic_string<buffer_char_type>::const_iterator begin = 
                buffer.begin() + (std::min)(buffer.size(), start_at);

#if defined(BOOST_MSVC)
#pragma warning(disable: 4244) // conversion from 'x' to 'y', possible loss of data
#endif
            std::copy(begin, buffer.end(), sink);
#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif
            return true;
        }

        std::size_t buffer_size() const 
        {
            return buffer.size();
        }

    private:
        std::size_t width;
        std::basic_string<buffer_char_type> buffer;
    };

    ///////////////////////////////////////////////////////////////////////////
    struct buffering_policy
    {
    public:
        buffering_policy() : buffer(NULL) {}
        buffering_policy(buffering_policy const& rhs) : buffer(rhs.buffer) {}

        // functions related to buffering
        buffer_sink* chain_buffering(buffer_sink* buffer_data)
        {
            buffer_sink* prev_buffer = buffer;
            buffer = buffer_data;
            return prev_buffer;
        }

        template <typename T>
        bool output(T const& value) 
        { 
            // buffer characters, if appropriate
            if (NULL != buffer) {
                buffer->output(value);
                return false;
            }
            return true;
        }

        bool has_buffer() const { return NULL != buffer; }

    private:
        buffer_sink* buffer;
    };

    struct no_buffering_policy
    {
        no_buffering_policy() {}
        no_buffering_policy(no_buffering_policy const&) {}

        template <typename T>
        bool output(T const& /*value*/) 
        {
            return true;
        }

        bool has_buffer() const { return false; }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  forward declaration only
    template <typename OutputIterator> 
    struct enable_buffering;

    template <typename OutputIterator, typename Properties
      , typename Derived = unused_type>
    class output_iterator;

    ///////////////////////////////////////////////////////////////////////////
    template <typename Buffering, typename Counting, typename Tracking>
    struct output_iterator_base : Buffering, Counting, Tracking
    {
        typedef Buffering buffering_policy;
        typedef Counting counting_policy;
        typedef Tracking tracking_policy;

        output_iterator_base() {}
        output_iterator_base(output_iterator_base const& rhs) 
          : buffering_policy(rhs), counting_policy(rhs), tracking_policy(rhs)
        {}

        template <typename T>
        bool output(T const& value) 
        { 
            this->counting_policy::output(value);
            this->tracking_policy::output(value);
            return this->buffering_policy::output(value);
        }
    };

    template <typename Buffering, typename Counting, typename Tracking>
    struct disabling_output_iterator : Buffering, Counting, Tracking
    {
        typedef Buffering buffering_policy;
        typedef Counting counting_policy;
        typedef Tracking tracking_policy;

        disabling_output_iterator() : do_output(true) {}
        disabling_output_iterator(disabling_output_iterator const& rhs) 
          : buffering_policy(rhs), counting_policy(rhs), tracking_policy(rhs)
          , do_output(rhs.do_output)
        {}

        template <typename T>
        bool output(T const& value) 
        { 
            if (!do_output) 
                return false;

            this->counting_policy::output(value);
            this->tracking_policy::output(value);
            return this->buffering_policy::output(value);
        }

        bool do_output;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator, typename Properties, typename Derived>
    struct make_output_iterator
    {
        // get the most derived type of this class
        typedef typename mpl::if_<
            traits::not_is_unused<Derived>, Derived
          , output_iterator<OutputIterator, Properties, Derived>
        >::type most_derived_type;

        static const generator_properties::enum_type properties = static_cast<generator_properties::enum_type>(Properties::value);

        typedef typename mpl::if_c<
            (properties & generator_properties::tracking) ? true : false
          , position_policy, no_position_policy
        >::type tracking_type;

        typedef typename mpl::if_c<
            (properties & generator_properties::buffering) ? true : false
          , buffering_policy, no_buffering_policy
        >::type buffering_type;

        typedef typename mpl::if_c<
            (properties & generator_properties::counting) ? true : false
          , counting_policy<most_derived_type>, no_counting_policy
        >::type counting_type;

        typedef typename mpl::if_c<
            (properties & generator_properties::disabling) ? true : false
          , disabling_output_iterator<buffering_type, counting_type, tracking_type>
          , output_iterator_base<buffering_type, counting_type, tracking_type>
        >::type type;
    };

    ///////////////////////////////////////////////////////////////////////////
    //  Karma uses an output iterator wrapper for all output operations. This
    //  is necessary to avoid the dreaded 'scanner business' problem, i.e. the
    //  dependency of rules and grammars on the used output iterator. 
    //
    //  By default the user supplied output iterator is wrapped inside an 
    //  instance of this internal output_iterator class. 
    //
    //  This output_iterator class normally just forwards to the embedded user
    //  supplied iterator. But it is possible to enable additional functionality
    //  on demand, such as counting, buffering, and position tracking.
    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator, typename Properties, typename Derived>
    class output_iterator 
      : public make_output_iterator<OutputIterator, Properties, Derived>::type
    {
    private:
        // base iterator type
        typedef typename make_output_iterator<
            OutputIterator, Properties, Derived>::type base_iterator;

    public:
        typedef std::output_iterator_tag iterator_category;
        typedef void value_type;
        typedef void difference_type;
        typedef void pointer;
        typedef void reference;

        explicit output_iterator(OutputIterator& sink_)
          : sink(&sink_)
        {}
        output_iterator(output_iterator const& rhs)
          : base_iterator(rhs), sink(rhs.sink)
        {}

        output_iterator& operator*() { return *this; }
        output_iterator& operator++() 
        { 
            if (!this->base_iterator::has_buffer())
                ++(*sink);           // increment only if not buffering
            return *this; 
        } 
        output_iterator operator++(int) 
        {
            if (!this->base_iterator::has_buffer()) {
                output_iterator t(*this);
                ++(*sink); 
                return t; 
            }
            return *this;
        }

#if defined(BOOST_MSVC)
// 'argument' : conversion from '...' to '...', possible loss of data
#pragma warning (push)
#pragma warning (disable: 4244)
#endif
        template <typename T>
        void operator=(T const& value) 
        { 
            if (this->base_iterator::output(value))
                *(*sink) = value; 
        }
#if defined(BOOST_MSVC)
#pragma warning (pop)
#endif

        // plain output iterators are considered to be good all the time
        bool good() const { return true; }

        // allow to access underlying output iterator
        OutputIterator& base() { return *sink; }

    protected:
        // this is the wrapped user supplied output iterator
        OutputIterator* sink;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Elem, typename Traits, typename Properties>
    class output_iterator<karma::ostream_iterator<T, Elem, Traits>, Properties>
      : public output_iterator<karma::ostream_iterator<T, Elem, Traits>, Properties
          , output_iterator<karma::ostream_iterator<T, Elem, Traits>, Properties> >
    {
    private:
        typedef output_iterator<karma::ostream_iterator<T, Elem, Traits>, Properties
          , output_iterator<karma::ostream_iterator<T, Elem, Traits>, Properties> 
        > base_type;
        typedef karma::ostream_iterator<T, Elem, Traits> base_iterator_type;
        typedef std::basic_ostream<Elem, Traits> ostream_type;

    public:
        output_iterator(base_iterator_type& sink)
          : base_type(sink) {}

        ostream_type& get_ostream() { return (*this->sink).get_ostream(); }
        ostream_type const& get_ostream() const { return (*this->sink).get_ostream(); }

        // expose good bit of underlying stream object
        bool good() const { return (*this->sink).get_ostream().good(); }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  Helper class for exception safe enabling of character counting in the
    //  output iterator
    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator>
    struct enable_counting
    {
        enable_counting(OutputIterator& sink_, std::size_t count = 0)
          : count_data(sink_, count) {}

        // get number of characters counted since last enable
        std::size_t count() const
        {
            return count_data.get_count();
        }

    private:
        counting_sink<OutputIterator> count_data;              // for counting
    };

    template <typename OutputIterator>
    struct disable_counting
    {
        disable_counting(OutputIterator& sink_)
          : count_data(sink_, 0, false) {}

    private:
        counting_sink<OutputIterator> count_data;
    };

    ///////////////////////////////////////////////////////////////////////////
    //  Helper class for exception safe enabling of character buffering in the
    //  output iterator
    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator>
    struct enable_buffering
    {
        enable_buffering(OutputIterator& sink_
              , std::size_t width = std::size_t(-1))
          : sink(sink_), prev_buffer(NULL), enabled(false)
        {
            buffer_data.enable(width);
            prev_buffer = sink.chain_buffering(&buffer_data);
            enabled = true;
        }
        ~enable_buffering()
        {
            disable();
        }

        // reset buffer chain to initial state
        void disable()
        {
            if (enabled) {
                BOOST_VERIFY(&buffer_data == sink.chain_buffering(prev_buffer));
                enabled = false;
            }
        }

        // copy to the underlying sink whatever is in the local buffer
        bool buffer_copy(std::size_t maxwidth = std::size_t(-1)
          , bool disable_ = true)
        {
            if (disable_)
                disable();
            return buffer_data.copy(sink, maxwidth) && sink.good();
        }

        // return number of characters stored in the buffer
        std::size_t buffer_size() const
        {
            return buffer_data.buffer_size();
        }

        // copy to the remaining characters to the specified sink
        template <typename RestIterator>
        bool buffer_copy_rest(RestIterator& sink, std::size_t start_at = 0) const
        {
            return buffer_data.copy_rest(sink, start_at);
        }

        // copy the contents to the given output iterator
        template <typename OutputIterator_>
        bool buffer_copy_to(OutputIterator_& sink
          , std::size_t maxwidth = std::size_t(-1)) const
        {
            return buffer_data.copy(sink, maxwidth);
        }

    private:
        OutputIterator& sink;
        buffer_sink buffer_data;    // for buffering
        buffer_sink* prev_buffer;   // previous buffer in chain
        bool enabled;
    };

    ///////////////////////////////////////////////////////////////////////////
    //  Helper class for exception safe disabling of output
    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator>
    struct disable_output
    {
        disable_output(OutputIterator& sink_)
          : sink(sink_), prev_do_output(sink.do_output)
        {
            sink.do_output = false;
        }
        ~disable_output()
        {
            sink.do_output = prev_do_output;
        }

        OutputIterator& sink;
        bool prev_do_output;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Sink>
    bool sink_is_good(Sink const&)
    {
        return true;      // the general case is always good
    }

    template <typename OutputIterator, typename Derived>
    bool sink_is_good(output_iterator<OutputIterator, Derived> const& sink)
    {
        return sink.good(); // our own output iterators are handled separately
    }

}}}}

#endif 

