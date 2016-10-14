#include <cstddef>
#include <istream>
#include <stdexcept>
#include <typeinfo>

#ifndef _GLIBCXX_USE_NOEXCEPT
    #define _GLIBCXX_USE_NOEXCEPT throw()
#endif

namespace std {

const char* bad_exception::what() const throw()
{
  return "std::bad_exception";
}

const char* bad_cast::what() const throw()
{
  return "std::bad_cast";
}

const char* bad_alloc::what() const throw()
{
  return "std::bad_alloc";
}

namespace __detail
{
struct _List_node_base
{
  void _M_hook(std::__detail::_List_node_base* const __position) throw () __attribute__((used))
  {
    _M_next = __position;
    _M_prev = __position->_M_prev;
    __position->_M_prev->_M_next = this;
    __position->_M_prev = this;
  }
  void _M_unhook() __attribute__((used))
  {
    _List_node_base* const __next_node = _M_next;
    _List_node_base* const __prev_node = _M_prev;
    __prev_node->_M_next = __next_node;
    __next_node->_M_prev = __prev_node;
  }
  _List_node_base* _M_next;
  _List_node_base* _M_prev;
};
} // namespace detail

template ostream& ostream::_M_insert(bool);
template ostream& ostream::_M_insert(long);
template ostream& ostream::_M_insert(double);
template ostream& ostream::_M_insert(unsigned long);
template ostream& ostream::_M_insert(const void*);
template ostream& __ostream_insert(ostream&, const char*, streamsize);
template istream& istream::_M_extract(long&);
template istream& istream::_M_extract(unsigned short&);

out_of_range::~out_of_range() _GLIBCXX_USE_NOEXCEPT { }

length_error::~length_error() _GLIBCXX_USE_NOEXCEPT { }

// Used with permission.
// See: https://github.com/madlib/madlib/commit/c3db418c0d34d6813608f2137fef1012ce03043d

void
ctype<char>::_M_widen_init() const {
    char __tmp[sizeof(_M_widen)];
    for (unsigned __i = 0; __i < sizeof(_M_widen); ++__i)
        __tmp[__i] = __i;
    do_widen(__tmp, __tmp + sizeof(__tmp), _M_widen);

    _M_widen_ok = 1;
    // Set _M_widen_ok to 2 if memcpy can't be used.
    for (unsigned __i = 0; __i < sizeof(_M_widen); ++__i)
        if (__tmp[__i] != _M_widen[__i]) {
            _M_widen_ok = 2;
            break;
        }
}

void  __throw_out_of_range_fmt(const char*, ...) __attribute__((__noreturn__));
void  __throw_out_of_range_fmt(const char* err, ...)
{
    // Safe and over-simplified version. Ignore the format and print it as-is.
    __throw_out_of_range(err);
}

}// namespace std
