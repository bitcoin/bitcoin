#ifndef CRYPTOPP_INTEGER_H
#define CRYPTOPP_INTEGER_H

/** \file */

#include "cryptlib.h"
#include "secblock.h"
#include "stdcpp.h"

#include <iosfwd>

NAMESPACE_BEGIN(CryptoPP)

//! \struct InitializeInteger
//! Performs static intialization of the Integer class
struct InitializeInteger
{
	InitializeInteger();
};

typedef SecBlock<word, AllocatorWithCleanup<word, CRYPTOPP_BOOL_X86> > IntegerSecBlock;

//! \brief Multiple precision integer with arithmetic operations
//! \details The Integer class can represent positive and negative integers
//!   with absolute value less than (256**sizeof(word))<sup>(256**sizeof(int))</sup>.
//! \details Internally, the library uses a sign magnitude representation, and the class
//!   has two data members. The first is a IntegerSecBlock (a SecBlock<word>) and it i
//!   used to hold the representation. The second is a Sign, and its is used to track
//!   the sign of the Integer.
//! \nosubgrouping
class CRYPTOPP_DLL Integer : private InitializeInteger, public ASN1Object
{
public:
	//! \name ENUMS, EXCEPTIONS, and TYPEDEFS
	//@{
		//! \brief Exception thrown when division by 0 is encountered
		class DivideByZero : public Exception
		{
		public:
			DivideByZero() : Exception(OTHER_ERROR, "Integer: division by zero") {}
		};

		//! \brief Exception thrown when a random number cannot be found that
		//!   satisfies the condition
		class RandomNumberNotFound : public Exception
		{
		public:
			RandomNumberNotFound() : Exception(OTHER_ERROR, "Integer: no integer satisfies the given parameters") {}
		};

		//! \enum Sign
		//! \brief Used internally to represent the integer
		//! \details Sign is used internally to represent the integer. It is also used in a few API functions.
		//! \sa Signedness
		enum Sign {
			//! \brief the value is positive or 0
			POSITIVE=0,
			//! \brief the value is negative
			NEGATIVE=1};

		//! \enum Signedness
		//! \brief Used when importing and exporting integers
		//! \details Signedness is usually used in API functions.
		//! \sa Sign
		enum Signedness {
			//! \brief an unsigned value
			UNSIGNED,
			//! \brief a signed value
			SIGNED};

		//! \enum RandomNumberType
		//! \brief Properties of a random integer
		enum RandomNumberType {
			//! \brief a number with no special properties
			ANY,
			//! \brief a number which is probabilistically prime
			PRIME};
	//@}

	//! \name CREATORS
	//@{
		//! \brief Creates the zero integer
		Integer();

		//! copy constructor
		Integer(const Integer& t);

		//! \brief Convert from signed long
		Integer(signed long value);

		//! \brief Convert from lword
		//! \param sign enumeration indicating Sign
		//! \param value the long word
		Integer(Sign sign, lword value);

		//! \brief Convert from two words
		//! \param sign enumeration indicating Sign
		//! \param highWord the high word
		//! \param lowWord the low word
		Integer(Sign sign, word highWord, word lowWord);

		//! \brief Convert from a C-string
		//! \param str C-string value
		//! \details \p str can be in base 2, 8, 10, or 16. Base is determined by a case
		//!   insensitive suffix of 'h', 'o', or 'b'.  No suffix means base 10.
		explicit Integer(const char *str);
		
		//! \brief Convert from a wide C-string
		//! \param str wide C-string value
		//! \details \p str can be in base 2, 8, 10, or 16. Base is determined by a case
		//!   insensitive suffix of 'h', 'o', or 'b'.  No suffix means base 10.
		explicit Integer(const wchar_t *str);

		//! \brief Convert from a big-endian byte array
		//! \param encodedInteger big-endian byte array
		//! \param byteCount length of the byte array
		//! \param sign enumeration indicating Signedness
		Integer(const byte *encodedInteger, size_t byteCount, Signedness sign=UNSIGNED);

		//! \brief Convert from a big-endian array
		//! \param bt BufferedTransformation object with big-endian byte array
		//! \param byteCount length of the byte array
		//! \param sign enumeration indicating Signedness
		Integer(BufferedTransformation &bt, size_t byteCount, Signedness sign=UNSIGNED);

		//! \brief Convert from a BER encoded byte array
		//! \param bt BufferedTransformation object with BER encoded byte array
		explicit Integer(BufferedTransformation &bt);

		//! \brief Create a random integer
		//! \param rng RandomNumberGenerator used to generate material
		//! \param bitCount the number of bits in the resulting integer
		//! \details The random integer created is uniformly distributed over <tt>[0, 2<sup>bitCount</sup>]</tt>.
		Integer(RandomNumberGenerator &rng, size_t bitCount);

		//! \brief Integer representing 0
		//! \returns an Integer representing 0
		//! \details Zero() avoids calling constructors for frequently used integers
		static const Integer & CRYPTOPP_API Zero();
		//! \brief Integer representing 1
		//! \returns an Integer representing 1
		//! \details One() avoids calling constructors for frequently used integers
		static const Integer & CRYPTOPP_API One();
		//! \brief Integer representing 2
		//! \returns an Integer representing 2
		//! \details Two() avoids calling constructors for frequently used integers
		static const Integer & CRYPTOPP_API Two();

		//! \brief Create a random integer of special form
		//! \param rng RandomNumberGenerator used to generate material
		//! \param min the minimum value
		//! \param max the maximum value
		//! \param rnType RandomNumberType to specify the type
		//! \param equiv the equivalence class based on the parameter \p mod
		//! \param mod the modulus used to reduce the equivalence class
		//! \throw RandomNumberNotFound if the set is empty.
		//! \details Ideally, the random integer created should be uniformly distributed
		//!   over <tt>{x | min \<= x \<= max</tt> and \p x is of rnType and <tt>x \% mod == equiv}</tt>.
		//!   However the actual distribution may not be uniform because sequential
		//!   search is used to find an appropriate number from a random starting
		//!   point.
		//! \details May return (with very small probability) a pseudoprime when a prime
		//!   is requested and <tt>max \> lastSmallPrime*lastSmallPrime</tt>. \p lastSmallPrime
		//!   is declared in nbtheory.h.
		Integer(RandomNumberGenerator &rng, const Integer &min, const Integer &max, RandomNumberType rnType=ANY, const Integer &equiv=Zero(), const Integer &mod=One());

		//! \brief Exponentiates to a power of 2
		//! \returns the Integer 2<sup>e</sup>
		//! \sa a_times_b_mod_c() and a_exp_b_mod_c()
		static Integer CRYPTOPP_API Power2(size_t e);
	//@}

	//! \name ENCODE/DECODE
	//@{
		//! \brief The minimum number of bytes to encode this integer
		//! \param sign enumeration indicating Signedness
		//! \note The MinEncodedSize() of 0 is 1.
		size_t MinEncodedSize(Signedness sign=UNSIGNED) const;

		//! \brief Encode in big-endian format
		//! \param output big-endian byte array
		//! \param outputLen length of the byte array
		//! \param sign enumeration indicating Signedness
		//! \details Unsigned means encode absolute value, signed means encode two's complement if negative.
		//! \details outputLen can be used to ensure an Integer is encoded to an exact size (rather than a
		//!   minimum size). An exact size is useful, for example, when encoding to a field element size.
		void Encode(byte *output, size_t outputLen, Signedness sign=UNSIGNED) const;

		//! \brief Encode in big-endian format
		//! \param bt BufferedTransformation object
		//! \param outputLen length of the encoding
		//! \param sign enumeration indicating Signedness
		//! \details Unsigned means encode absolute value, signed means encode two's complement if negative.
		//! \details outputLen can be used to ensure an Integer is encoded to an exact size (rather than a
		//!   minimum size). An exact size is useful, for example, when encoding to a field element size.
		void Encode(BufferedTransformation &bt, size_t outputLen, Signedness sign=UNSIGNED) const;

		//! \brief Encode in DER format
		//! \param bt BufferedTransformation object
		//! \details Encodes the Integer using Distinguished Encoding Rules
		//!   The result is placed into a BufferedTransformation object
		void DEREncode(BufferedTransformation &bt) const;

		//! encode absolute value as big-endian octet string
		//! \param bt BufferedTransformation object
		//! \param length the number of mytes to decode
		void DEREncodeAsOctetString(BufferedTransformation &bt, size_t length) const;

		//! \brief Encode absolute value in OpenPGP format
		//! \param output big-endian byte array
		//! \param bufferSize length of the byte array
		//! \returns length of the output
		//! \details OpenPGPEncode places result into a BufferedTransformation object and returns the
		//!   number of bytes used for the encoding
		size_t OpenPGPEncode(byte *output, size_t bufferSize) const;
	
		//! \brief Encode absolute value in OpenPGP format	
		//! \param bt BufferedTransformation object
		//! \returns length of the output
		//! \details OpenPGPEncode places result into a BufferedTransformation object and returns the
		//!   number of bytes used for the encoding
		size_t OpenPGPEncode(BufferedTransformation &bt) const;

		//! \brief Decode from big-endian byte array
		//! \param input big-endian byte array
		//! \param inputLen length of the byte array
		//! \param sign enumeration indicating Signedness
		void Decode(const byte *input, size_t inputLen, Signedness sign=UNSIGNED);
		
		//! \brief Decode nonnegative value from big-endian byte array
		//! \param bt BufferedTransformation object
		//! \param inputLen length of the byte array
		//! \param sign enumeration indicating Signedness
		//! \note <tt>bt.MaxRetrievable() \>= inputLen</tt>.
		void Decode(BufferedTransformation &bt, size_t inputLen, Signedness sign=UNSIGNED);

		//! \brief Decode from BER format
		//! \param input big-endian byte array
		//! \param inputLen length of the byte array
		void BERDecode(const byte *input, size_t inputLen);

		//! \brief Decode from BER format
		//! \param bt BufferedTransformation object
		void BERDecode(BufferedTransformation &bt);

		//! \brief Decode nonnegative value from big-endian octet string
		//! \param bt BufferedTransformation object
		//! \param length length of the byte array
		void BERDecodeAsOctetString(BufferedTransformation &bt, size_t length);

		//! \brief Exception thrown when an error is encountered decoding an OpenPGP integer
		class OpenPGPDecodeErr : public Exception
		{
		public: 
			OpenPGPDecodeErr() : Exception(INVALID_DATA_FORMAT, "OpenPGP decode error") {}
		};

		//! \brief Decode from OpenPGP format
		//! \param input big-endian byte array
		//! \param inputLen length of the byte array
		void OpenPGPDecode(const byte *input, size_t inputLen);
		//! \brief Decode from OpenPGP format
		//! \param bt BufferedTransformation object
		void OpenPGPDecode(BufferedTransformation &bt);
	//@}

	//! \name ACCESSORS
	//@{
		//! return true if *this can be represented as a signed long
		bool IsConvertableToLong() const;
		//! return equivalent signed long if possible, otherwise undefined
		signed long ConvertToLong() const;

		//! number of significant bits = floor(log2(abs(*this))) + 1
		unsigned int BitCount() const;
		//! number of significant bytes = ceiling(BitCount()/8)
		unsigned int ByteCount() const;
		//! number of significant words = ceiling(ByteCount()/sizeof(word))
		unsigned int WordCount() const;

		//! return the i-th bit, i=0 being the least significant bit
		bool GetBit(size_t i) const;
		//! return the i-th byte
		byte GetByte(size_t i) const;
		//! return n lowest bits of *this >> i
		lword GetBits(size_t i, size_t n) const;

		//!
		bool IsZero() const {return !*this;}
		//!
		bool NotZero() const {return !IsZero();}
		//!
		bool IsNegative() const {return sign == NEGATIVE;}
		//!
		bool NotNegative() const {return !IsNegative();}
		//!
		bool IsPositive() const {return NotNegative() && NotZero();}
		//!
		bool NotPositive() const {return !IsPositive();}
		//!
		bool IsEven() const {return GetBit(0) == 0;}
		//!
		bool IsOdd() const	{return GetBit(0) == 1;}
	//@}

	//! \name MANIPULATORS
	//@{
		//!
		Integer&  operator=(const Integer& t);

		//!
		Integer&  operator+=(const Integer& t);
		//!
		Integer&  operator-=(const Integer& t);
		//!
		//! \sa a_times_b_mod_c() and a_exp_b_mod_c()
		Integer&  operator*=(const Integer& t)	{return *this = Times(t);}
		//!
		Integer&  operator/=(const Integer& t)	{return *this = DividedBy(t);}
		//!
		//! \sa a_times_b_mod_c() and a_exp_b_mod_c()
		Integer&  operator%=(const Integer& t)	{return *this = Modulo(t);}
		//!
		Integer&  operator/=(word t)  {return *this = DividedBy(t);}
		//!
		//! \sa a_times_b_mod_c() and a_exp_b_mod_c()
		Integer&  operator%=(word t)  {return *this = Integer(POSITIVE, 0, Modulo(t));}

		//!
		Integer&  operator<<=(size_t);
		//!
		Integer&  operator>>=(size_t);

		//! \brief Set this Integer to random integer
		//! \param rng RandomNumberGenerator used to generate material
		//! \param bitCount the number of bits in the resulting integer
		//! \details The random integer created is uniformly distributed over <tt>[0, 2<sup>bitCount</sup>]</tt>.
		void Randomize(RandomNumberGenerator &rng, size_t bitCount);

		//! \brief Set this Integer to random integer
		//! \param rng RandomNumberGenerator used to generate material
		//! \param min the minimum value
		//! \param max the maximum value
		//! \details The random integer created is uniformly distributed over <tt>[min, max]</tt>.
		void Randomize(RandomNumberGenerator &rng, const Integer &min, const Integer &max);

		//! \brief Set this Integer to random integer of special form
		//! \param rng RandomNumberGenerator used to generate material
		//! \param min the minimum value
		//! \param max the maximum value
		//! \param rnType RandomNumberType to specify the type
		//! \param equiv the equivalence class based on the parameter \p mod
		//! \param mod the modulus used to reduce the equivalence class
		//! \throw RandomNumberNotFound if the set is empty.
		//! \details Ideally, the random integer created should be uniformly distributed
		//!   over <tt>{x | min \<= x \<= max</tt> and \p x is of rnType and <tt>x \% mod == equiv}</tt>.
		//!   However the actual distribution may not be uniform because sequential
		//!   search is used to find an appropriate number from a random starting
		//!   point.
		//! \details May return (with very small probability) a pseudoprime when a prime
		//!   is requested and <tt>max \> lastSmallPrime*lastSmallPrime</tt>. \p lastSmallPrime
		//!   is declared in nbtheory.h.
		bool Randomize(RandomNumberGenerator &rng, const Integer &min, const Integer &max, RandomNumberType rnType, const Integer &equiv=Zero(), const Integer &mod=One());

		bool GenerateRandomNoThrow(RandomNumberGenerator &rng, const NameValuePairs &params = g_nullNameValuePairs);
		void GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &params = g_nullNameValuePairs)
		{
			if (!GenerateRandomNoThrow(rng, params))
				throw RandomNumberNotFound();
		}

		//! \brief Set the n-th bit to value
		//! \details 0-based numbering.
		void SetBit(size_t n, bool value=1);

		//! \brief Set the n-th byte to value
		//! \details 0-based numbering.
		void SetByte(size_t n, byte value);

		//! \brief Reverse the Sign of the Integer
		void Negate();
		
		//! \brief Sets the Integer to positive
		void SetPositive() {sign = POSITIVE;}
		
		//! \brief Sets the Integer to negative
		void SetNegative() {if (!!(*this)) sign = NEGATIVE;}

		//! \brief Swaps this Integer with another Integer
		void swap(Integer &a);
	//@}

	//! \name UNARY OPERATORS
	//@{
		//!
		bool		operator!() const;
		//!
		Integer 	operator+() const {return *this;}
		//!
		Integer 	operator-() const;
		//!
		Integer&	operator++();
		//!
		Integer&	operator--();
		//!
		Integer 	operator++(int) {Integer temp = *this; ++*this; return temp;}
		//!
		Integer 	operator--(int) {Integer temp = *this; --*this; return temp;}
	//@}

	//! \name BINARY OPERATORS
	//@{
		//! \brief Perform signed comparison
		//! \param a the Integer to comapre
		//!   \retval -1 if <tt>*this < a</tt>
		//!   \retval  0 if <tt>*this = a</tt>
		//!   \retval  1 if <tt>*this > a</tt>
		int Compare(const Integer& a) const;

		//!
		Integer Plus(const Integer &b) const;
		//!
		Integer Minus(const Integer &b) const;
		//!
		//! \sa a_times_b_mod_c() and a_exp_b_mod_c()
		Integer Times(const Integer &b) const;
		//!
		Integer DividedBy(const Integer &b) const;
		//!
		//! \sa a_times_b_mod_c() and a_exp_b_mod_c()
		Integer Modulo(const Integer &b) const;
		//!
		Integer DividedBy(word b) const;
		//!
		//! \sa a_times_b_mod_c() and a_exp_b_mod_c()
		word Modulo(word b) const;

		//!
		Integer operator>>(size_t n) const	{return Integer(*this)>>=n;}
		//!
		Integer operator<<(size_t n) const	{return Integer(*this)<<=n;}
	//@}

	//! \name OTHER ARITHMETIC FUNCTIONS
	//@{
		//!
		Integer AbsoluteValue() const;
		//!
		Integer Doubled() const {return Plus(*this);}
		//!
		//! \sa a_times_b_mod_c() and a_exp_b_mod_c()
		Integer Squared() const {return Times(*this);}
		//! extract square root, if negative return 0, else return floor of square root
		Integer SquareRoot() const;
		//! return whether this integer is a perfect square
		bool IsSquare() const;

		//! is 1 or -1
		bool IsUnit() const;
		//! return inverse if 1 or -1, otherwise return 0
		Integer MultiplicativeInverse() const;

		//! calculate r and q such that (a == d*q + r) && (0 <= r < abs(d))
		static void CRYPTOPP_API Divide(Integer &r, Integer &q, const Integer &a, const Integer &d);
		//! use a faster division algorithm when divisor is short
		static void CRYPTOPP_API Divide(word &r, Integer &q, const Integer &a, word d);

		//! returns same result as Divide(r, q, a, Power2(n)), but faster
		static void CRYPTOPP_API DivideByPowerOf2(Integer &r, Integer &q, const Integer &a, unsigned int n);

		//! greatest common divisor
		static Integer CRYPTOPP_API Gcd(const Integer &a, const Integer &n);
		//! calculate multiplicative inverse of *this mod n
		//! \sa a_times_b_mod_c() and a_exp_b_mod_c()
		Integer InverseMod(const Integer &n) const;
		//!
		//! \sa a_times_b_mod_c() and a_exp_b_mod_c()
		word InverseMod(word n) const;
	//@}

	//! \name INPUT/OUTPUT
	//@{
		//! \brief Extraction operator
		//! \param in a reference to a std::istream
		//! \param a a reference to an Integer
		//! \returns a reference to a std::istream reference
		friend CRYPTOPP_DLL std::istream& CRYPTOPP_API operator>>(std::istream& in, Integer &a);
		//!
		//! \brief Insertion operator
		//! \param out a reference to a std::ostream
		//! \param a a constant reference to an Integer
		//! \returns a reference to a std::ostream reference
		//! \details The output integer responds to std::hex, std::oct, std::hex, std::upper and
		//!   std::lower. The output includes the suffix \a \b h (for hex), \a \b . (\a \b dot, for dec)
		//!   and \a \b o (for octal). There is currently no way to supress the suffix.
		//! \details If you want to print an Integer without the suffix or using an arbitrary base, then
		//!   use IntToString<Integer>().
		//! \sa IntToString<Integer>
		friend CRYPTOPP_DLL std::ostream& CRYPTOPP_API operator<<(std::ostream& out, const Integer &a);
	//@}
		
#ifndef CRYPTOPP_DOXYGEN_PROCESSING
	//! modular multiplication
	CRYPTOPP_DLL friend Integer CRYPTOPP_API a_times_b_mod_c(const Integer &x, const Integer& y, const Integer& m);
	//! modular exponentiation
	CRYPTOPP_DLL friend Integer CRYPTOPP_API a_exp_b_mod_c(const Integer &x, const Integer& e, const Integer& m);
#endif

private:
	
	Integer(word value, size_t length);
	int PositiveCompare(const Integer &t) const;
	
	IntegerSecBlock reg;
	Sign sign;
	
#ifndef CRYPTOPP_DOXYGEN_PROCESSING
	friend class ModularArithmetic;
	friend class MontgomeryRepresentation;
	friend class HalfMontgomeryRepresentation;

	friend void PositiveAdd(Integer &sum, const Integer &a, const Integer &b);
	friend void PositiveSubtract(Integer &diff, const Integer &a, const Integer &b);
	friend void PositiveMultiply(Integer &product, const Integer &a, const Integer &b);
	friend void PositiveDivide(Integer &remainder, Integer &quotient, const Integer &dividend, const Integer &divisor);
#endif
};

//!
inline bool operator==(const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)==0;}
//!
inline bool operator!=(const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)!=0;}
//!
inline bool operator> (const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)> 0;}
//!
inline bool operator>=(const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)>=0;}
//!
inline bool operator< (const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)< 0;}
//!
inline bool operator<=(const CryptoPP::Integer& a, const CryptoPP::Integer& b) {return a.Compare(b)<=0;}
//!
inline CryptoPP::Integer operator+(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.Plus(b);}
//!
inline CryptoPP::Integer operator-(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.Minus(b);}
//!
//! \sa a_times_b_mod_c() and a_exp_b_mod_c()
inline CryptoPP::Integer operator*(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.Times(b);}
//!
inline CryptoPP::Integer operator/(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.DividedBy(b);}
//!
//! \sa a_times_b_mod_c() and a_exp_b_mod_c()
inline CryptoPP::Integer operator%(const CryptoPP::Integer &a, const CryptoPP::Integer &b) {return a.Modulo(b);}
//!
inline CryptoPP::Integer operator/(const CryptoPP::Integer &a, CryptoPP::word b) {return a.DividedBy(b);}
//!
//! \sa a_times_b_mod_c() and a_exp_b_mod_c()
inline CryptoPP::word    operator%(const CryptoPP::Integer &a, CryptoPP::word b) {return a.Modulo(b);}

NAMESPACE_END

#ifndef __BORLANDC__
NAMESPACE_BEGIN(std)
inline void swap(CryptoPP::Integer &a, CryptoPP::Integer &b)
{
	a.swap(b);
}
NAMESPACE_END
#endif

#endif
