//===- FuzzedDataProvider.h - Utility header for fuzz targets ---*- C++ -* ===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// A single header library providing an utility class to break up an array of
// bytes. Whenever run on the same input, provides the same output, as long as
// its methods are called in the same order, with the same arguments.
//===----------------------------------------------------------------------===//

#ifndef LLVM_FUZZER_FUZZED_DATA_PROVIDER_H_
#define LLVM_FUZZER_FUZZED_DATA_PROVIDER_H_

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class FuzzedDataProvider {
public:
  // |data| is an array of length |size| that the FuzzedDataProvider wraps to
  // provide more granular access. |data| must outlive the FuzzedDataProvider.
  FuzzedDataProvider(const uint8_t *data, size_t size)
      : data_ptr_(data), remaining_bytes_(size) {}
  ~FuzzedDataProvider() = default;

  // Returns a std::vector containing |num_bytes| of input data. If fewer than
  // |num_bytes| of data remain, returns a shorter std::vector containing all
  // of the data that's left. Can be used with any byte sized type, such as
  // char, unsigned char, uint8_t, etc.
  template <typename T> std::vector<T> ConsumeBytes(size_t num_bytes) {
    num_bytes = std::min(num_bytes, remaining_bytes_);
    return ConsumeBytes<T>(num_bytes, num_bytes);
  }

  // Similar to |ConsumeBytes|, but also appends the terminator value at the end
  // of the resulting vector. Useful, when a mutable null-terminated C-string is
  // needed, for example. But that is a rare case. Better avoid it, if possible,
  // and prefer using |ConsumeBytes| or |ConsumeBytesAsString| methods.
  template <typename T>
  std::vector<T> ConsumeBytesWithTerminator(size_t num_bytes,
                                            T terminator = 0) {
    num_bytes = std::min(num_bytes, remaining_bytes_);
    std::vector<T> result = ConsumeBytes<T>(num_bytes + 1, num_bytes);
    result.back() = terminator;
    return result;
  }

  // Returns a std::string containing |num_bytes| of input data. Using this and
  // |.c_str()| on the resulting string is the best way to get an immutable
  // null-terminated C string. If fewer than |num_bytes| of data remain, returns
  // a shorter std::string containing all of the data that's left.
  std::string ConsumeBytesAsString(size_t num_bytes) {
    static_assert(sizeof(std::string::value_type) == sizeof(uint8_t),
                  "ConsumeBytesAsString cannot convert the data to a string.");

    num_bytes = std::min(num_bytes, remaining_bytes_);
    std::string result(
        reinterpret_cast<const std::string::value_type *>(data_ptr_),
        num_bytes);
    Advance(num_bytes);
    return result;
  }

  // Returns a number in the range [min, max] by consuming bytes from the
  // input data. The value might not be uniformly distributed in the given
  // range. If there's no input data left, always returns |min|. |min| must
  // be less than or equal to |max|.
  template <typename T> T ConsumeIntegralInRange(T min, T max) {
    static_assert(std::is_integral<T>::value, "An integral type is required.");
    static_assert(sizeof(T) <= sizeof(uint64_t), "Unsupported integral type.");

    if (min > max)
      abort();

    // Use the biggest type possible to hold the range and the result.
    uint64_t range = static_cast<uint64_t>(max) - min;
    uint64_t result = 0;
    size_t offset = 0;

    while (offset < sizeof(T) * CHAR_BIT && (range >> offset) > 0 &&
           remaining_bytes_ != 0) {
      // Pull bytes off the end of the seed data. Experimentally, this seems to
      // allow the fuzzer to more easily explore the input space. This makes
      // sense, since it works by modifying inputs that caused new code to run,
      // and this data is often used to encode length of data read by
      // |ConsumeBytes|. Separating out read lengths makes it easier modify the
      // contents of the data that is actually read.
      --remaining_bytes_;
      result = (result << CHAR_BIT) | data_ptr_[remaining_bytes_];
      offset += CHAR_BIT;
    }

    // Avoid division by 0, in case |range + 1| results in overflow.
    if (range != std::numeric_limits<decltype(range)>::max())
      result = result % (range + 1);

    return static_cast<T>(min + result);
  }

  // Returns a std::string of length from 0 to |max_length|. When it runs out of
  // input data, returns what remains of the input. Designed to be more stable
  // with respect to a fuzzer inserting characters than just picking a random
  // length and then consuming that many bytes with |ConsumeBytes|.
  std::string ConsumeRandomLengthString(size_t max_length) {
    // Reads bytes from the start of |data_ptr_|. Maps "\\" to "\", and maps "\"
    // followed by anything else to the end of the string. As a result of this
    // logic, a fuzzer can insert characters into the string, and the string
    // will be lengthened to include those new characters, resulting in a more
    // stable fuzzer than picking the length of a string independently from
    // picking its contents.
    std::string result;

    // Reserve the anticipated capaticity to prevent several reallocations.
    result.reserve(std::min(max_length, remaining_bytes_));
    for (size_t i = 0; i < max_length && remaining_bytes_ != 0; ++i) {
      char next = ConvertUnsignedToSigned<char>(data_ptr_[0]);
      Advance(1);
      if (next == '\\' && remaining_bytes_ != 0) {
        next = ConvertUnsignedToSigned<char>(data_ptr_[0]);
        Advance(1);
        if (next != '\\')
          break;
      }
      result += next;
    }

    result.shrink_to_fit();
    return result;
  }

  // Returns a std::vector containing all remaining bytes of the input data.
  template <typename T> std::vector<T> ConsumeRemainingBytes() {
    return ConsumeBytes<T>(remaining_bytes_);
  }

  // Prefer using |ConsumeRemainingBytes| unless you actually need a std::string
  // object.
  // Returns a std::vector containing all remaining bytes of the input data.
  std::string ConsumeRemainingBytesAsString() {
    return ConsumeBytesAsString(remaining_bytes_);
  }

  // Returns a number in the range [Type's min, Type's max]. The value might
  // not be uniformly distributed in the given range. If there's no input data
  // left, always returns |min|.
  template <typename T> T ConsumeIntegral() {
    return ConsumeIntegralInRange(std::numeric_limits<T>::min(),
                                  std::numeric_limits<T>::max());
  }

  // Reads one byte and returns a bool, or false when no data remains.
  bool ConsumeBool() { return 1 & ConsumeIntegral<uint8_t>(); }

  // Returns a copy of a value selected from a fixed-size |array|.
  template <typename T, size_t size>
  T PickValueInArray(const T (&array)[size]) {
    static_assert(size > 0, "The array must be non empty.");
    return array[ConsumeIntegralInRange<size_t>(0, size - 1)];
  }

  template <typename T>
  T PickValueInArray(std::initializer_list<const T> list) {
    // static_assert(list.size() > 0, "The array must be non empty.");
    return *(list.begin() + ConsumeIntegralInRange<size_t>(0, list.size() - 1));
  }

  // Return an enum value. The enum must start at 0 and be contiguous. It must
  // also contain |kMaxValue| aliased to its largest (inclusive) value. Such as:
  // enum class Foo { SomeValue, OtherValue, kMaxValue = OtherValue };
  template <typename T> T ConsumeEnum() {
    static_assert(std::is_enum<T>::value, "|T| must be an enum type.");
    return static_cast<T>(ConsumeIntegralInRange<uint32_t>(
        0, static_cast<uint32_t>(T::kMaxValue)));
  }

  // Reports the remaining bytes available for fuzzed input.
  size_t remaining_bytes() { return remaining_bytes_; }

private:
  FuzzedDataProvider(const FuzzedDataProvider &) = delete;
  FuzzedDataProvider &operator=(const FuzzedDataProvider &) = delete;

  void Advance(size_t num_bytes) {
    if (num_bytes > remaining_bytes_)
      abort();

    data_ptr_ += num_bytes;
    remaining_bytes_ -= num_bytes;
  }

  template <typename T>
  std::vector<T> ConsumeBytes(size_t size, size_t num_bytes_to_consume) {
    static_assert(sizeof(T) == sizeof(uint8_t), "Incompatible data type.");

    // The point of using the size-based constructor below is to increase the
    // odds of having a vector object with capacity being equal to the length.
    // That part is always implementation specific, but at least both libc++ and
    // libstdc++ allocate the requested number of bytes in that constructor,
    // which seems to be a natural choice for other implementations as well.
    // To increase the odds even more, we also call |shrink_to_fit| below.
    std::vector<T> result(size);
    std::memcpy(result.data(), data_ptr_, num_bytes_to_consume);
    Advance(num_bytes_to_consume);

    // Even though |shrink_to_fit| is also implementation specific, we expect it
    // to provide an additional assurance in case vector's constructor allocated
    // a buffer which is larger than the actual amount of data we put inside it.
    result.shrink_to_fit();
    return result;
  }

  template <typename TS, typename TU> TS ConvertUnsignedToSigned(TU value) {
    static_assert(sizeof(TS) == sizeof(TU), "Incompatible data types.");
    static_assert(!std::numeric_limits<TU>::is_signed,
                  "Source type must be unsigned.");

    // TODO(Dor1s): change to `if constexpr` once C++17 becomes mainstream.
    if (std::numeric_limits<TS>::is_modulo)
      return static_cast<TS>(value);

    // Avoid using implementation-defined unsigned to signer conversions.
    // To learn more, see https://stackoverflow.com/questions/13150449.
    if (value <= std::numeric_limits<TS>::max())
      return static_cast<TS>(value);
    else {
      constexpr auto TS_min = std::numeric_limits<TS>::min();
      return TS_min + static_cast<char>(value - TS_min);
    }
  }

  const uint8_t *data_ptr_;
  size_t remaining_bytes_;
};

#endif // LLVM_FUZZER_FUZZED_DATA_PROVIDER_H_
