#ifndef BITCOIN_SRC_VBK_TYPED_WRAPPER_HPP
#define BITCOIN_SRC_VBK_TYPED_WRAPPER_HPP

namespace VeriBlock {


/// Typed Wrapper used to create strongly typed wrappers around primitive types (mostly ints).
/// @tparam T wrapped type
/// @tparam Tag compile time tag to differentiate different types
/// @example
/// using BlockHeight = TypedWrapper<uint64_t, struct BlockHeightTag>;
/// BlockHeight h(5);
/// int a = h; // compile error - different types
template <typename T, typename Tag>
struct TypedWrapper {
    explicit TypedWrapper(T t) : value_(t)
    {
    }

    const T& unwrap() const
    {
        return this->value_;
    }

    T unwrap()
    {
        return this->value_;
    }

    explicit operator T()
    {
        return value_;
    }

    bool operator==(const TypedWrapper<T, Tag>& other) const
    {
        return value_ == other.value_;
    }

    bool operator!=(const TypedWrapper<T, Tag>& other) const
    {
        return !(this->operator==(other));
    }

private:
    T value_;
};

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_TYPED_WRAPPER_HPP
