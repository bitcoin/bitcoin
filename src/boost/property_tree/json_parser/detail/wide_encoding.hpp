#ifndef BOOST_PROPERTY_TREE_DETAIL_JSON_PARSER_WIDE_ENCODING_HPP
#define BOOST_PROPERTY_TREE_DETAIL_JSON_PARSER_WIDE_ENCODING_HPP

#include <boost/assert.hpp>
#include <boost/range/iterator_range_core.hpp>

#include <utility>

namespace boost { namespace property_tree {
    namespace json_parser { namespace detail
{

    struct external_wide_encoding
    {
        typedef wchar_t external_char;

        bool is_nl(wchar_t c) const { return c == L'\n'; }
        bool is_ws(wchar_t c) const {
            return c == L' ' || c == L'\t' || c == L'\n' || c == L'\r';
        }

        bool is_minus(wchar_t c) const { return c == L'-'; }
        bool is_plusminus(wchar_t c) const { return c == L'+' || c == L'-'; }
        bool is_dot(wchar_t c) const { return c == L'.'; }
        bool is_eE(wchar_t c) const { return c == L'e' || c == L'E'; }
        bool is_0(wchar_t c) const { return c == L'0'; }
        bool is_digit(wchar_t c) const { return c >= L'0' && c <= L'9'; }
        bool is_digit0(wchar_t c) const { return c >= L'1' && c <= L'9'; }

        bool is_quote(wchar_t c) const { return c == L'"'; }
        bool is_backslash(wchar_t c) const { return c == L'\\'; }
        bool is_slash(wchar_t c) const { return c == L'/'; }

        bool is_comma(wchar_t c) const { return c == L','; }
        bool is_open_bracket(wchar_t c) const { return c == L'['; }
        bool is_close_bracket(wchar_t c) const { return c == L']'; }
        bool is_colon(wchar_t c) const { return c == L':'; }
        bool is_open_brace(wchar_t c) const { return c == L'{'; }
        bool is_close_brace(wchar_t c) const { return c == L'}'; }

        bool is_a(wchar_t c) const { return c == L'a'; }
        bool is_b(wchar_t c) const { return c == L'b'; }
        bool is_e(wchar_t c) const { return c == L'e'; }
        bool is_f(wchar_t c) const { return c == L'f'; }
        bool is_l(wchar_t c) const { return c == L'l'; }
        bool is_n(wchar_t c) const { return c == L'n'; }
        bool is_r(wchar_t c) const { return c == L'r'; }
        bool is_s(wchar_t c) const { return c == L's'; }
        bool is_t(wchar_t c) const { return c == L't'; }
        bool is_u(wchar_t c) const { return c == L'u'; }

        int decode_hexdigit(wchar_t c) {
            if (c >= L'0' && c <= L'9') return c - L'0';
            if (c >= L'A' && c <= L'F') return c - L'A' + 10;
            if (c >= L'a' && c <= L'f') return c - L'a' + 10;
            return -1;
        }
    };

    template <bool B> struct is_utf16 {};

    class wide_wide_encoding : public external_wide_encoding
    {
        typedef is_utf16<sizeof(wchar_t) == 2> test_utf16;
    public:
        typedef wchar_t internal_char;

        template <typename Iterator>
        boost::iterator_range<Iterator>
        to_internal(Iterator first, Iterator last) const {
            return boost::make_iterator_range(first, last);
        }

        wchar_t to_internal_trivial(wchar_t c) const {
            BOOST_ASSERT(!is_surrogate_high(c) && !is_surrogate_low(c));
            return c;
        }

        template <typename Iterator, typename Sentinel,
                  typename EncodingErrorFn>
        void skip_codepoint(Iterator& cur, Sentinel end,
                            EncodingErrorFn error_fn) const {
            transcode_codepoint(cur, end, DoNothing(), error_fn);
        }

        template <typename Iterator, typename Sentinel, typename TranscodedFn,
                  typename EncodingErrorFn>
        void transcode_codepoint(Iterator& cur, Sentinel end,
                TranscodedFn transcoded_fn, EncodingErrorFn error_fn) const {
            return transcode_codepoint(cur, end, transcoded_fn, error_fn,
                                       test_utf16());
        }

        template <typename TranscodedFn>
        void feed_codepoint(unsigned codepoint,
                            TranscodedFn transcoded_fn) const {
            feed_codepoint(codepoint, transcoded_fn, test_utf16());
        }

        template <typename Iterator, typename Sentinel>
        void skip_introduction(Iterator& cur, Sentinel end) const {
            // Endianness is already decoded at this level.
            if (cur != end && *cur == 0xfeff) {
                ++cur;
            }
        }

    private:
        struct DoNothing {
            void operator ()(wchar_t) const {}
        };

        template <typename Iterator, typename Sentinel, typename TranscodedFn,
                  typename EncodingErrorFn>
        void transcode_codepoint(Iterator& cur, Sentinel,
                                 TranscodedFn transcoded_fn,
                                 EncodingErrorFn error_fn,
                                 is_utf16<false>) const {
            wchar_t c = *cur;
            if (c < 0x20) {
                error_fn();
            }
            transcoded_fn(c);
            ++cur;
        }
        template <typename Iterator, typename Sentinel, typename TranscodedFn,
                  typename EncodingErrorFn>
        void transcode_codepoint(Iterator& cur, Sentinel end,
                                 TranscodedFn transcoded_fn,
                                 EncodingErrorFn error_fn,
                                 is_utf16<true>) const {
            wchar_t c = *cur;
            if (c < 0x20) {
                error_fn();
            }
            if (is_surrogate_low(c)) {
                error_fn();
            }
            transcoded_fn(c);
            ++cur;
            if (is_surrogate_high(c)) {
                if (cur == end) {
                    error_fn();
                }
                c = *cur;
                if (!is_surrogate_low(c)) {
                    error_fn();
                }
                transcoded_fn(c);
                ++cur;
            }
        }

        template <typename TranscodedFn>
        void feed_codepoint(unsigned codepoint, TranscodedFn transcoded_fn,
                            is_utf16<false>) const {
            transcoded_fn(static_cast<wchar_t>(codepoint));
        }
        template <typename TranscodedFn>
        void feed_codepoint(unsigned codepoint, TranscodedFn transcoded_fn,
                            is_utf16<true>) const {
            if (codepoint < 0x10000) {
                transcoded_fn(static_cast<wchar_t>(codepoint));
            } else {
                codepoint -= 0x10000;
                transcoded_fn(static_cast<wchar_t>((codepoint >> 10) | 0xd800));
                transcoded_fn(static_cast<wchar_t>(
                    (codepoint & 0x3ff) | 0xdc00));
            }
        }

        static bool is_surrogate_high(unsigned codepoint) {
            return (codepoint & 0xfc00) == 0xd800;
        }
        static bool is_surrogate_low(unsigned codepoint) {
            return (codepoint & 0xfc00) == 0xdc00;
        }
    };

}}}}

#endif
