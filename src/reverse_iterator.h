// Taken from https://gist.github.com/arvidsson/7231973

#ifndef BITCOIN_REVERSE_ITERATOR_H
#define BITCOIN_REVERSE_ITERATOR_H

/**
 * Template used for reverse iteration in C++11 range-based for loops.
 * 
 *   std::vector<int> v = {1, 2, 3, 4, 5};
 *   for (auto x : reverse_iterate(v))
 *       std::cout << x << " ";
 */

template <typename T>
class reverse_range
{
    T &m_x;
    
public:
    reverse_range(T &x) : m_x(x) {}
    
    auto begin() const -> decltype(this->m_x.rbegin())
    {
        return m_x.rbegin();
    }
    
    auto end() const -> decltype(this->m_x.rend())
    {
        return m_x.rend();
    }
};
 
template <typename T>
reverse_range<T> reverse_iterate(T &x)
{
    return reverse_range<T>(x);
}

#endif // BITCOIN_REVERSE_ITERATOR_H
