// cryptlib.h - written and placed in the public domain by Wei Dai

//! \file cryptlib.h
//! \brief Abstract base classes that provide a uniform interface to this library.

/*!	\mainpage Crypto++ Library 5.6.5 API Reference
<dl>
<dt>Abstract Base Classes<dd>
	cryptlib.h
<dt>Authenticated Encryption Modes<dd>
	CCM, EAX, \ref GCM "GCM (2K tables)", \ref GCM "GCM (64K tables)"
<dt>Block Ciphers<dd>
	\ref Rijndael "AES", Weak::ARC4, Blowfish, BTEA, Camellia, CAST128, CAST256, DES, \ref DES_EDE2 "2-key Triple-DES", \ref DES_EDE3 "3-key Triple-DES",
	\ref DES_XEX3 "DESX", GOST, IDEA, \ref LR "Luby-Rackoff", MARS, RC2, RC5, RC6, \ref SAFER_K "SAFER-K", \ref SAFER_SK "SAFER-SK", SEED, Serpent,
	\ref SHACAL2 "SHACAL-2", SHARK, SKIPJACK,
Square, TEA, \ref ThreeWay "3-Way", Twofish, XTEA
<dt>Stream Ciphers<dd>
	ChaCha8, ChaCha12, ChaCha20, \ref Panama "Panama-LE", \ref Panama "Panama-BE", Salsa20, \ref SEAL "SEAL-LE", \ref SEAL "SEAL-BE", WAKE, XSalsa20
<dt>Hash Functions<dd>
	BLAKE2s, BLAKE2b, \ref Keccak "Keccak (F1600)", SHA1, SHA224, SHA256, SHA384, SHA512, \ref SHA3 "SHA-3", Tiger, Whirlpool, RIPEMD160, RIPEMD320, RIPEMD128, RIPEMD256, Weak::MD2, Weak::MD4, Weak::MD5
<dt>Non-Cryptographic Checksums<dd>
	CRC32, Adler32
<dt>Message Authentication Codes<dd>
	VMAC, HMAC, CBC_MAC, CMAC, DMAC, TTMAC, \ref GCM "GCM (GMAC)", BLAKE2
<dt>Random Number Generators<dd>
	NullRNG(), LC_RNG, RandomPool, BlockingRng, NonblockingRng, AutoSeededRandomPool, AutoSeededX917RNG,
	\ref MersenneTwister "MersenneTwister (MT19937 and MT19937-AR)", RDRAND, RDSEED
<dt>Key Derivation and Password-based Cryptography<dd>
	HKDF, \ref PKCS12_PBKDF "PBKDF (PKCS #12)", \ref PKCS5_PBKDF1 "PBKDF-1 (PKCS #5)", \ref PKCS5_PBKDF2_HMAC "PBKDF-2/HMAC (PKCS #5)"
<dt>Public Key Cryptosystems<dd>
	DLIES, ECIES, LUCES, RSAES, RabinES, LUC_IES
<dt>Public Key Signature Schemes<dd>
	DSA2, GDSA, ECDSA, NR, ECNR, LUCSS, RSASS, RSASS_ISO, RabinSS, RWSS, ESIGN
<dt>Key Agreement<dd>
	DH, DH2, \ref MQV_Domain "MQV", \ref HMQV_Domain "HMQV", \ref FHMQV_Domain "FHMQV", ECDH, ECMQV, ECHMQV, ECFHMQV, XTR_DH
<dt>Algebraic Structures<dd>
	Integer, PolynomialMod2, PolynomialOver, RingOfPolynomialsOver,
	ModularArithmetic, MontgomeryRepresentation, GFP2_ONB, GF2NP, GF256, GF2_32, EC2N, ECP
<dt>Secret Sharing and Information Dispersal<dd>
	SecretSharing, SecretRecovery, InformationDispersal, InformationRecovery
<dt>Compression<dd>
	Deflator, Inflator, Gzip, Gunzip, ZlibCompressor, ZlibDecompressor
<dt>Input Source Classes<dd>
	StringSource, ArraySource, FileSource, SocketSource, WindowsPipeSource, RandomNumberSource
<dt>Output Sink Classes<dd>
	StringSinkTemplate, StringSink, ArraySink, FileSink, SocketSink, WindowsPipeSink, RandomNumberSink
<dt>Filter Wrappers<dd>
	StreamTransformationFilter, HashFilter, HashVerificationFilter, SignerFilter, SignatureVerificationFilter
<dt>Binary to Text Encoders and Decoders<dd>
	HexEncoder, HexDecoder, Base64Encoder, Base64Decoder, Base64URLEncoder, Base64URLDecoder, Base32Encoder, Base32Decoder
<dt>Wrappers for OS features<dd>
	Timer, Socket, WindowsHandle, ThreadLocalStorage, ThreadUserTimer
<dt>FIPS 140 validated cryptography<dd>
	fips140.h
</dl>

In the DLL version of Crypto++, only the following implementation class are available.
<dl>
<dt>Block Ciphers<dd>
	AES, \ref DES_EDE2 "2-key Triple-DES", \ref DES_EDE3 "3-key Triple-DES", SKIPJACK
<dt>Cipher Modes (replace template parameter BC with one of the block ciphers above)<dd>
	\ref ECB_Mode "ECB_Mode<BC>", \ref CTR_Mode "CTR_Mode<BC>", \ref CBC_Mode "CBC_Mode<BC>", \ref CFB_FIPS_Mode "CFB_FIPS_Mode<BC>", \ref OFB_Mode "OFB_Mode<BC>", \ref GCM "GCM<AES>"
<dt>Hash Functions<dd>
	SHA1, SHA224, SHA256, SHA384, SHA512
<dt>Public Key Signature Schemes (replace template parameter H with one of the hash functions above)<dd>
	RSASS\<PKCS1v15, H\>, RSASS\<PSS, H\>, RSASS_ISO\<H\>, RWSS\<P1363_EMSA2, H\>, DSA, ECDSA\<ECP, H\>, ECDSA\<EC2N, H\>
<dt>Message Authentication Codes (replace template parameter H with one of the hash functions above)<dd>
	HMAC\<H\>, CBC_MAC\<DES_EDE2\>, CBC_MAC\<DES_EDE3\>, GCM\<AES\>
<dt>Random Number Generators<dd>
	DefaultAutoSeededRNG (AutoSeededX917RNG\<AES\>)
<dt>Key Agreement<dd>
	DH, DH2
<dt>Public Key Cryptosystems<dd>
	RSAES\<OAEP\<SHA1\> \>
</dl>

<p>This reference manual is a work in progress. Some classes are lack detailed descriptions.
<p>Click <a href="CryptoPPRef.zip">here</a> to download a zip archive containing this manual.
<p>Thanks to Ryan Phillips for providing the Doxygen configuration file
and getting us started on the manual.
*/

#ifndef CRYPTOPP_CRYPTLIB_H
#define CRYPTOPP_CRYPTLIB_H

#include "config.h"
#include "stdcpp.h"
#include "trap.h"

#if defined(CRYPTOPP_BSD_AVAILABLE) || defined(CRYPTOPP_UNIX_AVAILABLE)
# include <signal.h>
#endif

#if CRYPTOPP_MSC_VERSION
# pragma warning(push)
# pragma warning(disable: 4127 4189 4702)
#endif

NAMESPACE_BEGIN(CryptoPP)

// forward declarations
class Integer;
class RandomNumberGenerator;
class BufferedTransformation;

//! \brief Specifies a direction for a cipher to operate
//! \sa BlockTransformation::IsForwardTransformation(), BlockTransformation::IsPermutation(), BlockTransformation::GetCipherDirection()
enum CipherDir {
	//! \brief the cipher is performing encryption
	ENCRYPTION,
	//! \brief the cipher is performing decryption
	DECRYPTION};

//! \brief Represents infinite time
const unsigned long INFINITE_TIME = ULONG_MAX;

// VC60 workaround: using enums as template parameters causes problems
//! \brief Converts a typename to an enumerated value
template <typename ENUM_TYPE, int VALUE>
struct EnumToType
{
	static ENUM_TYPE ToEnum() {return (ENUM_TYPE)VALUE;}
};

//! \brief Provides the byte ordering
//! \details Big-endian and little-endian modes are supported. Bi-endian and PDP-endian modes
//!   are not supported.
enum ByteOrder {
	//! \brief byte order is little-endian
	LITTLE_ENDIAN_ORDER = 0,
	//! \brief byte order is big-endian
	BIG_ENDIAN_ORDER = 1};

//! \brief Provides a constant for LittleEndian
typedef EnumToType<ByteOrder, LITTLE_ENDIAN_ORDER> LittleEndian;
//! \brief Provides a constant for BigEndian
typedef EnumToType<ByteOrder, BIG_ENDIAN_ORDER> BigEndian;

//! \class Exception
//! \brief Base class for all exceptions thrown by the library
//! \details All library exceptions directly or indirectly inherit from the Exception class.
//!   The Exception class itself inherits from std::exception. The library does not use
//!   std::runtime_error derived classes.
class CRYPTOPP_DLL Exception : public std::exception
{
public:
	//! \enum ErrorType
	//! \brief Error types or categories
	enum ErrorType {
		//! \brief A method was called which was not implemented
		NOT_IMPLEMENTED,
		//! \brief An invalid argument was detected
		INVALID_ARGUMENT,
		//! \brief BufferedTransformation received a Flush(true) signal but can't flush buffers
		CANNOT_FLUSH,
		//! \brief Data integerity check, such as CRC or MAC, failed
		DATA_INTEGRITY_CHECK_FAILED,
		//! \brief Input data was received that did not conform to expected format
		INVALID_DATA_FORMAT,
		//! \brief Error reading from input device or writing to output device
		IO_ERROR,
		//! \brief Some other error occurred not belonging to other categories
		OTHER_ERROR
	};

	//! \brief Construct a new  Exception
	explicit Exception(ErrorType errorType, const std::string &s) : m_errorType(errorType), m_what(s) {}
	virtual ~Exception() throw() {}

	//! \brief Retrieves a C-string describing the exception
	const char *what() const throw() {return (m_what.c_str());}
	//! \brief Retrieves a string describing the exception
	const std::string &GetWhat() const {return m_what;}
	//! \brief Sets the error string for the exception
	void SetWhat(const std::string &s) {m_what = s;}
	//! \brief Retrieves the error type for the exception
	ErrorType GetErrorType() const {return m_errorType;}
	//! \brief Sets the error type for the exceptions
	void SetErrorType(ErrorType errorType) {m_errorType = errorType;}

private:
	ErrorType m_errorType;
	std::string m_what;
};

//! \brief An invalid argument was detected
class CRYPTOPP_DLL InvalidArgument : public Exception
{
public:
	explicit InvalidArgument(const std::string &s) : Exception(INVALID_ARGUMENT, s) {}
};

//! \brief Input data was received that did not conform to expected format
class CRYPTOPP_DLL InvalidDataFormat : public Exception
{
public:
	explicit InvalidDataFormat(const std::string &s) : Exception(INVALID_DATA_FORMAT, s) {}
};

//! \brief A decryption filter encountered invalid ciphertext
class CRYPTOPP_DLL InvalidCiphertext : public InvalidDataFormat
{
public:
	explicit InvalidCiphertext(const std::string &s) : InvalidDataFormat(s) {}
};

//! \brief A method was called which was not implemented
class CRYPTOPP_DLL NotImplemented : public Exception
{
public:
	explicit NotImplemented(const std::string &s) : Exception(NOT_IMPLEMENTED, s) {}
};

//! \brief Flush(true) was called but it can't completely flush its buffers
class CRYPTOPP_DLL CannotFlush : public Exception
{
public:
	explicit CannotFlush(const std::string &s) : Exception(CANNOT_FLUSH, s) {}
};

//! \brief The operating system reported an error
class CRYPTOPP_DLL OS_Error : public Exception
{
public:
	OS_Error(ErrorType errorType, const std::string &s, const std::string& operation, int errorCode)
		: Exception(errorType, s), m_operation(operation), m_errorCode(errorCode) {}
	~OS_Error() throw() {}

	//! \brief Retrieve the operating system API that reported the error
	const std::string & GetOperation() const {return m_operation;}
	//! \brief Retrieve the error code returned by the operating system
	int GetErrorCode() const {return m_errorCode;}

protected:
	std::string m_operation;
	int m_errorCode;
};

//! \class DecodingResult
//! \brief Returns a decoding results
struct CRYPTOPP_DLL DecodingResult
{
	//! \brief Constructs a DecodingResult
	//! \details isValidCoding is initialized to  false and messageLength is initialized to 0.
	explicit DecodingResult() : isValidCoding(false), messageLength(0) {}
	//! \brief Constructs a DecodingResult
	//! \param len the message length
	//! \details isValidCoding is initialized to  true.
	explicit DecodingResult(size_t len) : isValidCoding(true), messageLength(len) {}

	//! \brief Compare two DecodingResult
	//! \param rhs the other DecodingResult
	//! \return true if both isValidCoding and messageLength are equal, false otherwise
	bool operator==(const DecodingResult &rhs) const {return isValidCoding == rhs.isValidCoding && messageLength == rhs.messageLength;}
	//! \brief Compare two DecodingResult
	//! \param rhs the other DecodingResult
	//! \return true if either isValidCoding or messageLength is \a not equal, false otherwise
	//! \details Returns <tt>!operator==(rhs)</tt>.
	bool operator!=(const DecodingResult &rhs) const {return !operator==(rhs);}

	//! \brief Flag to indicate the decoding is valid
	bool isValidCoding;
	//! \brief Recovered message length if isValidCoding is true, undefined otherwise
	size_t messageLength;

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY
	operator size_t() const {return isValidCoding ? messageLength : 0;}
#endif
};

//! \class NameValuePairs
//! \brief Interface for retrieving values given their names
//! \details This class is used to safely pass a variable number of arbitrarily typed arguments to functions
//!   and to read values from keys and crypto parameters.
//! \details To obtain an object that implements NameValuePairs for the purpose of parameter
//!   passing, use the MakeParameters() function.
//! \details To get a value from NameValuePairs, you need to know the name and the type of the value.
//!   Call GetValueNames() on a NameValuePairs object to obtain a list of value names that it supports.
//!   then look at the Name namespace documentation to see what the type of each value is, or
//!   alternatively, call GetIntValue() with the value name, and if the type is not int, a
//!   ValueTypeMismatch exception will be thrown and you can get the actual type from the exception object.
class CRYPTOPP_NO_VTABLE NameValuePairs
{
public:
	virtual ~NameValuePairs() {}

	//! \class ValueTypeMismatch
	//! \brief Thrown when an unexpected type is encountered
	//! \details Exception thrown when trying to retrieve a value using a different type than expected
	class CRYPTOPP_DLL ValueTypeMismatch : public InvalidArgument
	{
	public:
		//! \brief Construct a ValueTypeMismatch
		//! \param name the name of the value
		//! \param stored the \a actual type of the value stored
		//! \param retrieving the \a presumed type of the value retrieved
		ValueTypeMismatch(const std::string &name, const std::type_info &stored, const std::type_info &retrieving)
			: InvalidArgument("NameValuePairs: type mismatch for '" + name + "', stored '" + stored.name() + "', trying to retrieve '" + retrieving.name() + "'")
			, m_stored(stored), m_retrieving(retrieving) {}

		//! \brief Provides the stored type
		//! \return the C++ mangled name of the type
		const std::type_info & GetStoredTypeInfo() const {return m_stored;}

		//! \brief Provides the retrieveing type
		//! \return the C++ mangled name of the type
		const std::type_info & GetRetrievingTypeInfo() const {return m_retrieving;}

	private:
		const std::type_info &m_stored;
		const std::type_info &m_retrieving;
	};

	//! \brief Get a copy of this object or subobject
	//! \tparam T class or type
	//! \param object reference to a variable that receives the value
	template <class T>
	bool GetThisObject(T &object) const
	{
		return GetValue((std::string("ThisObject:")+typeid(T).name()).c_str(), object);
	}

	//! \brief Get a pointer to this object
	//! \tparam T class or type
	//! \param ptr reference to a pointer to a variable that receives the value
	template <class T>
	bool GetThisPointer(T *&ptr) const
	{
		return GetValue((std::string("ThisPointer:")+typeid(T).name()).c_str(), ptr);
	}

	//! \brief Get a named value
	//! \tparam T class or type
	//! \param name the name of the object or value to retrieve
	//! \param value reference to a variable that receives the value
	//! \returns true if the value was retrieved, false otherwise
	//! \sa GetValue(), GetValueWithDefault(), GetIntValue(), GetIntValueWithDefault(),
	//!   GetRequiredParameter() and GetRequiredIntParameter()
	template <class T>
	bool GetValue(const char *name, T &value) const
	{
		return GetVoidValue(name, typeid(T), &value);
	}

	//! \brief Get a named value
	//! \tparam T class or type
	//! \param name the name of the object or value to retrieve
	//! \param defaultValue the default value of the class or type if it does not exist
	//! \return the object or value
	//! \sa GetValue(), GetValueWithDefault(), GetIntValue(), GetIntValueWithDefault(),
	//!   GetRequiredParameter() and GetRequiredIntParameter()
	template <class T>
	T GetValueWithDefault(const char *name, T defaultValue) const
	{
		T value;
		bool result = GetValue(name, value);
		// No assert... this recovers from failure
		if (result) {return value;}
		return defaultValue;
	}

	//! \brief Get a list of value names that can be retrieved
	//! \return a list of names available to retrieve
	//! \details the items in the list are delimited with a colon.
	CRYPTOPP_DLL std::string GetValueNames() const
		{std::string result; GetValue("ValueNames", result); return result;}

	//! \brief Get a named value with type int
	//! \param name the name of the value to retrieve
	//! \param value the value retrieved upon success
	//! \return true if an int value was retrieved, false otherwise
	//! \details GetIntValue() is used to ensure we don't accidentally try to get an
	//!   unsigned int or some other type when we mean int (which is the most common case)
	//! \sa GetValue(), GetValueWithDefault(), GetIntValue(), GetIntValueWithDefault(),
	//!   GetRequiredParameter() and GetRequiredIntParameter()
	CRYPTOPP_DLL bool GetIntValue(const char *name, int &value) const
		{return GetValue(name, value);}

	//! \brief Get a named value with type int, with default
	//! \param name the name of the value to retrieve
	//! \param defaultValue the default value if the name does not exist
	//! \return the value retrieved on success or the default value
	//! \sa GetValue(), GetValueWithDefault(), GetIntValue(), GetIntValueWithDefault(),
	//!   GetRequiredParameter() and GetRequiredIntParameter()
	CRYPTOPP_DLL int GetIntValueWithDefault(const char *name, int defaultValue) const
		{return GetValueWithDefault(name, defaultValue);}

	//! \brief Ensures an expected name and type is present
	//! \param name the name of the value
	//! \param stored the type that was stored for the name
	//! \param retrieving the type that is being retrieved for the name
	//! \throws ValueTypeMismatch
	//! \details ThrowIfTypeMismatch() effectively performs a type safety check.
	//!    stored and  retrieving are C++ mangled names for the type.
	//! \sa GetValue(), GetValueWithDefault(), GetIntValue(), GetIntValueWithDefault(),
	//!   GetRequiredParameter() and GetRequiredIntParameter()
	CRYPTOPP_DLL static void CRYPTOPP_API ThrowIfTypeMismatch(const char *name, const std::type_info &stored, const std::type_info &retrieving)
		{if (stored != retrieving) throw ValueTypeMismatch(name, stored, retrieving);}

	//! \brief Retrieves a required name/value pair
	//! \tparam T class or type
	//! \param className the name of the class
	//! \param name the name of the value
	//! \param value reference to a variable to receive the value
	//! \throws InvalidArgument
	//! \details GetRequiredParameter() throws InvalidArgument if the name
	//!   is not present or not of the expected type T.
	//! \sa GetValue(), GetValueWithDefault(), GetIntValue(), GetIntValueWithDefault(),
	//!   GetRequiredParameter() and GetRequiredIntParameter()
	template <class T>
	void GetRequiredParameter(const char *className, const char *name, T &value) const
	{
		if (!GetValue(name, value))
			throw InvalidArgument(std::string(className) + ": missing required parameter '" + name + "'");
	}

	//! \brief Retrieves a required name/value pair
	//! \param className the name of the class
	//! \param name the name of the value
	//! \param value reference to a variable to receive the value
	//! \throws InvalidArgument
	//! \details GetRequiredParameter() throws InvalidArgument if the name
	//!   is not present or not of the expected type T.
	//! \sa GetValue(), GetValueWithDefault(), GetIntValue(), GetIntValueWithDefault(),
	//!   GetRequiredParameter() and GetRequiredIntParameter()
	CRYPTOPP_DLL void GetRequiredIntParameter(const char *className, const char *name, int &value) const
	{
		if (!GetIntValue(name, value))
			throw InvalidArgument(std::string(className) + ": missing required parameter '" + name + "'");
	}

	//! \brief Get a named value
	//! \param name the name of the object or value to retrieve
	//! \param valueType reference to a variable that receives the value
	//! \param pValue void pointer to a variable that receives the value
	//! \returns true if the value was retrieved, false otherwise
	//! \details GetVoidValue() retrives the value of  name if it exists.
	//! \note  GetVoidValue() is an internal function and should be implemented
	//!   by derived classes. Users should use one of the other functions instead.
	//! \sa GetValue(), GetValueWithDefault(), GetIntValue(), GetIntValueWithDefault(),
	//!   GetRequiredParameter() and GetRequiredIntParameter()
	CRYPTOPP_DLL virtual bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const =0;
};

#if CRYPTOPP_DOXYGEN_PROCESSING

//! \brief Namespace containing value name definitions.
//! \details Name is part of the CryptoPP namespace.
//! \details The semantics of value names, types are:
//! <pre>
//!     ThisObject:ClassName (ClassName, copy of this object or a subobject)
//!     ThisPointer:ClassName (const ClassName *, pointer to this object or a subobject)
//! </pre>
DOCUMENTED_NAMESPACE_BEGIN(Name)
// more names defined in argnames.h
DOCUMENTED_NAMESPACE_END

//! \brief Namespace containing weak and wounded algorithms.
//! \details Weak is part of the CryptoPP namespace. Schemes and algorithms are moved into Weak
//!   when their security level is reduced to an unacceptable level by contemporary standards.
//! \details To use an algorithm in the Weak namespace, you must <tt>\c \#define
//!   CRYPTOPP_ENABLE_NAMESPACE_WEAK 1</tt> before including a header for a weak or wounded
//!   algorithm. For example:
//!   <pre>
//!     \c \#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
//!     \c \#include <md5.h>
//!     ...
//!     CryptoPP::Weak::MD5 md5;
//!   </pre>

DOCUMENTED_NAMESPACE_BEGIN(Weak)
// weak and wounded algorithms
DOCUMENTED_NAMESPACE_END
#endif

//! \brief An empty set of name-value pairs
extern CRYPTOPP_DLL const NameValuePairs &g_nullNameValuePairs;

// ********************************************************

//! \class Clonable
//! \brief Interface for cloning objects
//! \note this is \a not implemented by most classes
//! \sa ClonableImpl, NotCopyable
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Clonable
{
public:
	virtual ~Clonable() {}

	//! \brief Copies  this object
	//! \return a copy of this object
	//! \throws NotImplemented
	//! \note this is \a not implemented by most classes
	//! \sa NotCopyable
	virtual Clonable* Clone() const {throw NotImplemented("Clone() is not implemented yet.");}	// TODO: make this =0
};

//! \class Algorithm
//! \brief Interface for all crypto algorithms
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Algorithm : public Clonable
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~Algorithm() {}
#endif

	//! \brief Interface for all crypto algorithms
	//! \param checkSelfTestStatus determines whether the object can proceed if the self
	//!   tests have not been run or failed.
	//! \details When FIPS 140-2 compliance is enabled and checkSelfTestStatus == true,
	//!   this constructor throws SelfTestFailure if the self test hasn't been run or fails.
	//! \details FIPS 140-2 compliance is disabled by default. It is only used by certain
	//!   versions of the library when the library is built as a DLL on Windows. Also see
	//!    CRYPTOPP_ENABLE_COMPLIANCE_WITH_FIPS_140_2 in config.h.
	Algorithm(bool checkSelfTestStatus = true);

	//! \brief Provides the name of this algorithm
	//! \return the standard algorithm name
	//! \details The standard algorithm name can be a name like \a AES or \a AES/GCM. Some algorithms
	//!   do not have standard names yet. For example, there is no standard algorithm name for
	//!   Shoup's  ECIES.
	//! \note  AlgorithmName is not universally implemented yet
	virtual std::string AlgorithmName() const {return "unknown";}
};

//! \class SimpleKeyingInterface
//! \brief Interface for algorithms that take byte strings as keys
//! \sa FixedKeyLength(), VariableKeyLength(), SameKeyLengthAs(), SimpleKeyingInterfaceImpl()
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE SimpleKeyingInterface
{
public:
	virtual ~SimpleKeyingInterface() {}

	//! \brief Returns smallest valid key length in bytes
	virtual size_t MinKeyLength() const =0;
	//! \brief Returns largest valid key length in bytes
	virtual size_t MaxKeyLength() const =0;
	//! \brief Returns default (recommended) key length in bytes
	virtual size_t DefaultKeyLength() const =0;

	//! \brief
	//! \param n the desired keylength
	//! \return the smallest valid key length in bytes that is greater than or equal to <tt>min(n, GetMaxKeyLength())</tt>
	virtual size_t GetValidKeyLength(size_t n) const =0;

	//! \brief Returns whether  keylength is a valid key length
	//! \param keylength the requested keylength
	//! \return true if keylength is valid, false otherwise
	//! \details Internally the function calls GetValidKeyLength()
	virtual bool IsValidKeyLength(size_t keylength) const
		{return keylength == GetValidKeyLength(keylength);}

	//! \brief Sets or reset the key of this object
	//! \param key the key to use when keying the object
	//! \param length the size of the key, in bytes
	//! \param params additional initialization parameters that cannot be passed
	//!   directly through the constructor
	virtual void SetKey(const byte *key, size_t length, const NameValuePairs &params = g_nullNameValuePairs);

	//! \brief Sets or reset the key of this object
	//! \param key the key to use when keying the object
	//! \param length the size of the key, in bytes
	//! \param rounds the number of rounds to apply the transformation function,
	//!    if applicable
	//! \details SetKeyWithRounds() calls SetKey() with a NameValuePairs
	//!   object that only specifies rounds. rounds is an integer parameter,
	//!   and <tt>-1</tt> means use the default number of rounds.
	void SetKeyWithRounds(const byte *key, size_t length, int rounds);

	//! \brief Sets or reset the key of this object
	//! \param key the key to use when keying the object
	//! \param length the size of the key, in bytes
	//! \param iv the intiialization vector to use when keying the object
	//! \param ivLength the size of the iv, in bytes
	//! \details SetKeyWithIV() calls SetKey() with a NameValuePairs
	//!   that only specifies IV. The IV is a byte buffer with size ivLength.
	//!   ivLength is an integer parameter, and <tt>-1</tt> means use IVSize().
	void SetKeyWithIV(const byte *key, size_t length, const byte *iv, size_t ivLength);

	//! \brief Sets or reset the key of this object
	//! \param key the key to use when keying the object
	//! \param length the size of the key, in bytes
	//! \param iv the intiialization vector to use when keying the object
	//! \details SetKeyWithIV() calls SetKey() with a NameValuePairs() object
	//!   that only specifies iv. iv is a byte buffer, and it must have
	//!   a size IVSize().
	void SetKeyWithIV(const byte *key, size_t length, const byte *iv)
		{SetKeyWithIV(key, length, iv, IVSize());}

	//! \brief Secure IVs requirements as enumerated values.
	//! \details Provides secure IV requirements as a monotomically increasing enumerated values. Requirements can be
	//!    compared using less than (&lt;) and greater than (&gt;). For example, <tt>UNIQUE_IV &lt; RANDOM_IV</tt>
	//!    and <tt>UNPREDICTABLE_RANDOM_IV &gt; RANDOM_IV</tt>.
	//! \sa IsResynchronizable(), CanUseRandomIVs(), CanUsePredictableIVs(), CanUseStructuredIVs()
	enum IV_Requirement {
		//! \brief The IV must be unique
		UNIQUE_IV = 0,
		//! \brief The IV must be random and possibly predictable
		RANDOM_IV,
		//! \brief The IV must be random and unpredictable
		UNPREDICTABLE_RANDOM_IV,
		//! \brief The IV is set by the object
		INTERNALLY_GENERATED_IV,
		//! \brief The object does not use an IV
		NOT_RESYNCHRONIZABLE
	};

	//! \brief Minimal requirement for secure IVs
	//! \return the secure IV requirement of the algorithm
	virtual IV_Requirement IVRequirement() const =0;

	//! \brief Determines if the object can be resynchronized
	//! \return true if the object can be resynchronized (i.e. supports initialization vectors), false otherwise
	//! \note If this function returns true, and no IV is passed to SetKey() and <tt>CanUseStructuredIVs()==true</tt>,
	//!   an IV of all 0's will be assumed.
	bool IsResynchronizable() const {return IVRequirement() < NOT_RESYNCHRONIZABLE;}

	//! \brief Determines if the object can use random IVs
	//! \return true if the object can use random IVs (in addition to ones returned by GetNextIV), false otherwise
	bool CanUseRandomIVs() const {return IVRequirement() <= UNPREDICTABLE_RANDOM_IV;}

	//! \brief Determines if the object can use random but possibly predictable IVs
	//! \return true if the object can use random but possibly predictable IVs (in addition to ones returned by
	//!    GetNextIV), false otherwise
	bool CanUsePredictableIVs() const {return IVRequirement() <= RANDOM_IV;}

	//! \brief Determines if the object can use structured IVs
	//! \returns true if the object can use structured IVs, false otherwise
	//! \details CanUseStructuredIVs() indicates whether the object can use structured IVs; for example a counter
	//!    (in addition to ones returned by GetNextIV).
	bool CanUseStructuredIVs() const {return IVRequirement() <= UNIQUE_IV;}

	//! \brief Returns length of the IV accepted by this object
	//! \return the size of an IV, in bytes
	//! \throws NotImplemented() if the object does not support resynchronization
	//! \details The default implementation throws NotImplemented
	virtual unsigned int IVSize() const
		{throw NotImplemented(GetAlgorithm().AlgorithmName() + ": this object doesn't support resynchronization");}

	//! \brief Provides the default size of an IV
	//! \return default length of IVs accepted by this object, in bytes
	unsigned int DefaultIVLength() const {return IVSize();}

	//! \brief Provides the minimum size of an IV
	//! \return minimal length of IVs accepted by this object, in bytes
	//! \throws NotImplemented() if the object does not support resynchronization
	virtual unsigned int MinIVLength() const {return IVSize();}

	//! \brief Provides the maximum size of an IV
	//! \return maximal length of IVs accepted by this object, in bytes
	//! \throws NotImplemented() if the object does not support resynchronization
	virtual unsigned int MaxIVLength() const {return IVSize();}

	//! \brief Resynchronize with an IV
	//! \param iv the initialization vector
	//! \param ivLength the size of the initialization vector, in bytes
	//! \details Resynchronize() resynchronizes with an IV provided by the caller. <tt>ivLength=-1</tt> means use IVSize().
	//! \throws NotImplemented() if the object does not support resynchronization
	virtual void Resynchronize(const byte *iv, int ivLength=-1) {
		CRYPTOPP_UNUSED(iv); CRYPTOPP_UNUSED(ivLength);
		throw NotImplemented(GetAlgorithm().AlgorithmName() + ": this object doesn't support resynchronization");
	}

	//! \brief Retrieves a secure IV for the next message
	//! \param rng a RandomNumberGenerator to produce keying material
	//! \param iv a block of bytes to receive the IV
	//! \details The IV must be at least IVSize() in length.
	//! \details This method should be called after you finish encrypting one message and are ready
	//!    to start the next one. After calling it, you must call SetKey() or Resynchronize().
	//!    before using this object again.
	//! \details Internally, the base class implementation calls RandomNumberGenerator's GenerateBlock()
	//! \note This method is not implemented on decryption objects.
	virtual void GetNextIV(RandomNumberGenerator &rng, byte *iv);

protected:
	//! \brief Returns the base class  Algorithm
	//! \return the base class  Algorithm
	virtual const Algorithm & GetAlgorithm() const =0;

	//! \brief Sets the key for this object without performing parameter validation
	//! \param key a byte buffer used to key the cipher
	//! \param length the length of the byte buffer
	//! \param params additional parameters passed as  NameValuePairs
	//! \details key must be at least DEFAULT_KEYLENGTH in length.
	virtual void UncheckedSetKey(const byte *key, unsigned int length, const NameValuePairs &params) =0;

	//! \brief Validates the key length
	//! \param length the size of the keying material, in bytes
	//! \throws InvalidKeyLength if the key length is invalid
	void ThrowIfInvalidKeyLength(size_t length);

	//! \brief Validates the object
	//! \throws InvalidArgument if the IV is present
	//! \details Internally, the default implementation calls  IsResynchronizable() and throws
	//!    InvalidArgument if the function returns  true.
	//! \note called when no IV is passed
	void ThrowIfResynchronizable();

	//! \brief Validates the IV
	//! \param iv the IV with a length of  IVSize, in bytes
	//! \throws InvalidArgument on failure
	//! \details Internally, the default implementation checks the  iv. If  iv is not  NULL,
	//!   then the function succeeds. If  iv is  NULL, then  IVRequirement is checked against
	//!    UNPREDICTABLE_RANDOM_IV. If  IVRequirement is  UNPREDICTABLE_RANDOM_IV, then
	//!   then the function succeeds. Otherwise, an exception is thrown.
	void ThrowIfInvalidIV(const byte *iv);

	//! \brief Validates the IV length
	//! \param length the size of an IV, in bytes
	//! \throws InvalidArgument if the number of  rounds are invalid
	size_t ThrowIfInvalidIVLength(int length);

	//! \brief Retrieves and validates the IV
	//! \param params  NameValuePairs with the IV supplied as a ConstByteArrayParameter
	//! \param size the length of the IV, in bytes
	//! \return a pointer to the first byte of the  IV
	//! \throws InvalidArgument if the number of  rounds are invalid
	const byte * GetIVAndThrowIfInvalid(const NameValuePairs &params, size_t &size);

	//! \brief Validates the key length
	//! \param length the size of the keying material, in bytes
	inline void AssertValidKeyLength(size_t length) const
		{CRYPTOPP_UNUSED(length); CRYPTOPP_ASSERT(IsValidKeyLength(length));}
};

//! \brief Interface for the data processing part of block ciphers
//! \details Classes derived from BlockTransformation are block ciphers
//!   in ECB mode (for example the DES::Encryption class), which are stateless.
//!   These classes should not be used directly, but only in combination with
//!   a mode class (see CipherModeDocumentation in modes.h).
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE BlockTransformation : public Algorithm
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~BlockTransformation() {}
#endif

	//! \brief Encrypt or decrypt a block
	//! \param inBlock the input message before processing
	//! \param outBlock the output message after processing
	//! \param xorBlock an optional XOR mask
	//! \details ProcessAndXorBlock encrypts or decrypts inBlock, xor with xorBlock, and write to outBlock.
	//! \details The size of the block is determined by the block cipher and its documentation. Use
	//!     BLOCKSIZE at compile time, or BlockSize() at runtime.
	//! \note The message can be transformed in-place, or the buffers must \a not overlap
	//! \sa FixedBlockSize, BlockCipherFinal from seckey.h and BlockSize()
	virtual void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const =0;

	//! \brief Encrypt or decrypt a block
	//! \param inBlock the input message before processing
	//! \param outBlock the output message after processing
	//! \details ProcessBlock encrypts or decrypts inBlock and write to outBlock.
	//! \details The size of the block is determined by the block cipher and its documentation.
	//!    Use  BLOCKSIZE at compile time, or BlockSize() at runtime.
	//! \sa FixedBlockSize, BlockCipherFinal from seckey.h and BlockSize()
	//! \note The message can be transformed in-place, or the buffers must \a not overlap
	void ProcessBlock(const byte *inBlock, byte *outBlock) const
		{ProcessAndXorBlock(inBlock, NULL, outBlock);}

	//! \brief Encrypt or decrypt a block in place
	//! \param inoutBlock the input message before processing
	//! \details ProcessBlock encrypts or decrypts inoutBlock in-place.
	//! \details The size of the block is determined by the block cipher and its documentation.
	//!    Use BLOCKSIZE at compile time, or BlockSize() at runtime.
	//! \sa FixedBlockSize, BlockCipherFinal from seckey.h and BlockSize()
	void ProcessBlock(byte *inoutBlock) const
		{ProcessAndXorBlock(inoutBlock, NULL, inoutBlock);}

	//! Provides the block size of the cipher
	//! \return the block size of the cipher, in bytes
	virtual unsigned int BlockSize() const =0;

	//! \brief Provides input and output data alignment for optimal performance.
	//! \return the input data alignment that provides optimal performance
	virtual unsigned int OptimalDataAlignment() const;

	//! returns true if this is a permutation (i.e. there is an inverse transformation)
	virtual bool IsPermutation() const {return true;}

	//! \brief Determines if the cipher is being operated in its forward direction
	//! \returns true if DIR is ENCRYPTION, false otherwise
	//! \sa IsForwardTransformation(), IsPermutation(), GetCipherDirection()
	virtual bool IsForwardTransformation() const =0;

	//! \brief Determines the number of blocks that can be processed in parallel
	//! \return the number of blocks that can be processed in parallel, for bit-slicing implementations
	//! \details Bit-slicing is often used to improve throughput and minimize timing attacks.
	virtual unsigned int OptimalNumberOfParallelBlocks() const {return 1;}

	//! \brief Bit flags that control AdvancedProcessBlocks() behavior
	enum FlagsForAdvancedProcessBlocks {
		//! \brief inBlock is a counter
		BT_InBlockIsCounter=1,
		//! \brief should not modify block pointers
		BT_DontIncrementInOutPointers=2,
		//! \brief
		BT_XorInput=4,
		//! \brief perform the transformation in reverse
		BT_ReverseDirection=8,
		//! \brief
		BT_AllowParallel=16};

	//! \brief Encrypt and xor multiple blocks using additional flags
	//! \param inBlocks the input message before processing
	//! \param xorBlocks an optional XOR mask
	//! \param outBlocks the output message after processing
	//! \param length the size of the blocks, in bytes
	//! \param flags additional flags to control processing
	//! \details Encrypt and xor multiple blocks according to FlagsForAdvancedProcessBlocks flags.
	//! \note If BT_InBlockIsCounter is set, then the last byte of inBlocks may be modified.
	virtual size_t AdvancedProcessBlocks(const byte *inBlocks, const byte *xorBlocks, byte *outBlocks, size_t length, word32 flags) const;

	//! \brief Provides the direction of the cipher
	//! \return ENCRYPTION if IsForwardTransformation() is true, DECRYPTION otherwise
	//! \sa IsForwardTransformation(), IsPermutation()
	inline CipherDir GetCipherDirection() const {return IsForwardTransformation() ? ENCRYPTION : DECRYPTION;}
};

//! \class StreamTransformation
//! \brief Interface for the data processing portion of stream ciphers
//! \sa StreamTransformationFilter()
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE StreamTransformation : public Algorithm
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~StreamTransformation() {}
#endif

	//! \brief Provides a reference to this object
	//! \return A reference to this object
	//! \details Useful for passing a temporary object to a function that takes a non-const reference
	StreamTransformation& Ref() {return *this;}

	//! \brief Provides the mandatory block size of the cipher
	//! \return The block size of the cipher if input must be processed in blocks, 1 otherwise
	virtual unsigned int MandatoryBlockSize() const {return 1;}

	//! \brief Provides the input block size most efficient for this cipher.
	//! \return The input block size that is most efficient for the cipher
	//! \details The base class implementation returns MandatoryBlockSize().
	//! \note Optimal input length is
	//!   <tt>n * OptimalBlockSize() - GetOptimalBlockSizeUsed()</tt> for any <tt>n \> 0</tt>.
	virtual unsigned int OptimalBlockSize() const {return MandatoryBlockSize();}

	//! \brief Provides the number of bytes used in the current block when processing at optimal block size.
	//! \return the number of bytes used in the current block when processing at the optimal block size
	virtual unsigned int GetOptimalBlockSizeUsed() const {return 0;}

	//! \brief Provides input and output data alignment for optimal performance.
	//! \return the input data alignment that provides optimal performance
	virtual unsigned int OptimalDataAlignment() const;

	//! \brief Encrypt or decrypt an array of bytes
	//! \param outString the output byte buffer
	//! \param inString the input byte buffer
	//! \param length the size of the input and output byte buffers, in bytes
	//! \details Either <tt>inString == outString</tt>, or they must not overlap.
	virtual void ProcessData(byte *outString, const byte *inString, size_t length) =0;

	//! \brief Encrypt or decrypt the last block of data
	//! \param outString the output byte buffer
	//! \param inString the input byte buffer
	//! \param length the size of the input and output byte buffers, in bytes
	//!  ProcessLastBlock is used when the last block of data is special.
	//!   Currently the only use of this function is CBC-CTS mode.
	virtual void ProcessLastBlock(byte *outString, const byte *inString, size_t length);

	//! \brief Provides the size of the last block
	//! \returns the minimum size of the last block
	//! \details MinLastBlockSize() returns the minimum size of the last block. 0 indicates the last
	//!   block is not special.
	virtual unsigned int MinLastBlockSize() const {return 0;}

	//! \brief Encrypt or decrypt a string of bytes
	//! \param inoutString the string to process
	//! \param length the size of the  inoutString, in bytes
	//! \details Internally, the base class implementation calls ProcessData().
	inline void ProcessString(byte *inoutString, size_t length)
		{ProcessData(inoutString, inoutString, length);}

	//! \brief Encrypt or decrypt a string of bytes
	//! \param outString the output string to process
	//! \param inString the input string to process
	//! \param length the size of the input and output strings, in bytes
	//! \details Internally, the base class implementation calls ProcessData().
	inline void ProcessString(byte *outString, const byte *inString, size_t length)
		{ProcessData(outString, inString, length);}

	//! \brief Encrypt or decrypt a byte
	//! \param input the input byte to process
	//! \details Internally, the base class implementation calls ProcessData() with a size of 1.
	inline byte ProcessByte(byte input)
		{ProcessData(&input, &input, 1); return input;}

	//! \brief Determines whether the cipher supports random access
	//! \returns true if the cipher supports random access, false otherwise
	virtual bool IsRandomAccess() const =0;

	//! \brief Seek to an absolute position
	//! \param pos position to seek
	//! \throws NotImplemented
	//! \details The base class implementation throws NotImplemented. The function
	//!   \ref CRYPTOPP_ASSERT "asserts" IsRandomAccess() in debug builds.
	virtual void Seek(lword pos)
	{
		CRYPTOPP_UNUSED(pos);
		CRYPTOPP_ASSERT(!IsRandomAccess());
		throw NotImplemented("StreamTransformation: this object doesn't support random access");
	}

	//! \brief Determines whether the cipher is self-inverting
	//! \returns true if the cipher is self-inverting, false otherwise
	//! \details IsSelfInverting determines whether this transformation is
	//!   self-inverting (e.g. xor with a keystream).
	virtual bool IsSelfInverting() const =0;

	//! \brief Determines if the cipher is being operated in its forward direction
	//! \returns true if DIR is ENCRYPTION, false otherwise
	//! \sa IsForwardTransformation(), IsPermutation(), GetCipherDirection()
	virtual bool IsForwardTransformation() const =0;
};

//! \class HashTransformation
//! \brief Interface for hash functions and data processing part of MACs
//! \details HashTransformation objects are stateful. They are created in an initial state,
//!   change state as Update() is called, and return to the initial
//!   state when Final() is called. This interface allows a large message to
//!   be hashed in pieces by calling Update() on each piece followed by
//!   calling Final().
//! \sa HashFilter(), HashVerificationFilter()
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE HashTransformation : public Algorithm
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~HashTransformation() {}
#endif

	//! \brief Provides a reference to this object
	//! \return A reference to this object
	//! \details Useful for passing a temporary object to a function that takes a non-const reference
	HashTransformation& Ref() {return *this;}

	//! \brief Updates a hash with additional input
	//! \param input the additional input as a buffer
	//! \param length the size of the buffer, in bytes
	virtual void Update(const byte *input, size_t length) =0;

	//! \brief Request space which can be written into by the caller
	//! \param size the requested size of the buffer
	//! \details The purpose of this method is to help avoid extra memory allocations.
	//! \details size is an \a IN and \a OUT parameter and used as a hint. When the call is made,
	//!   size is the requested size of the buffer. When the call returns, size is the size of
	//!   the array returned to the caller.
	//! \details The base class implementation sets  size to 0 and returns  NULL.
	//! \note Some objects, like ArraySink, cannot create a space because its fixed.
	virtual byte * CreateUpdateSpace(size_t &size) {size=0; return NULL;}

	//! \brief Computes the hash of the current message
	//! \param digest a pointer to the buffer to receive the hash
	//! \details Final() restarts the hash for a new message.
	//! \pre <tt>COUNTOF(digest) == DigestSize()</tt> or <tt>COUNTOF(digest) == HASH::DIGESTSIZE</tt> ensures
	//!   the output byte buffer is large enough for the digest.
	virtual void Final(byte *digest)
		{TruncatedFinal(digest, DigestSize());}

	//! \brief Restart the hash
	//! \details Discards the current state, and restart for a new message
	virtual void Restart()
		{TruncatedFinal(NULL, 0);}

	//! Provides the digest size of the hash
	//! \return the digest size of the hash.
	virtual unsigned int DigestSize() const =0;

	//! Provides the tag size of the hash
	//! \return the tag size of the hash.
	//! \details Same as DigestSize().
	unsigned int TagSize() const {return DigestSize();}

	//! \brief Provides the block size of the compression function
	//! \return the block size of the compression function, in bytes
	//! \details BlockSize() will return 0 if the hash is not block based. For example,
	//!   SHA3 is a recursive hash (not an iterative hash), and it does not have a block size.
	virtual unsigned int BlockSize() const {return 0;}

	//! \brief Provides the input block size most efficient for this hash.
	//! \return The input block size that is most efficient for the cipher
	//! \details The base class implementation returns MandatoryBlockSize().
	//! \details Optimal input length is
	//!   <tt>n * OptimalBlockSize() - GetOptimalBlockSizeUsed()</tt> for any <tt>n \> 0</tt>.
	virtual unsigned int OptimalBlockSize() const {return 1;}

	//! \brief Provides input and output data alignment for optimal performance
	//! \return the input data alignment that provides optimal performance
	virtual unsigned int OptimalDataAlignment() const;

	//! \brief Updates the hash with additional input and computes the hash of the current message
	//! \param digest a pointer to the buffer to receive the hash
	//! \param input the additional input as a buffer
	//! \param length the size of the buffer, in bytes
	//! \details Use this if your input is in one piece and you don't want to call Update()
	//!   and Final() separately
	//! \details CalculateDigest() restarts the hash for the next message.
	//! \pre <tt>COUNTOF(digest) == DigestSize()</tt> or <tt>COUNTOF(digest) == HASH::DIGESTSIZE</tt> ensures
	//!   the output byte buffer is large enough for the digest.
	virtual void CalculateDigest(byte *digest, const byte *input, size_t length)
		{Update(input, length); Final(digest);}

	//! \brief Verifies the hash of the current message
	//! \param digest a pointer to the buffer of an \a existing hash
	//! \return \p true if the existing hash matches the computed hash, \p false otherwise
	//! \throws ThrowIfInvalidTruncatedSize() if the existing hash's size exceeds DigestSize()
	//! \details Verify() performs a bitwise compare on the buffers using VerifyBufsEqual(), which is
	//!   a constant time comparison function. digestLength cannot exceed DigestSize().
	//! \details Verify() restarts the hash for the next message.
	//! \pre <tt>COUNTOF(digest) == DigestSize()</tt> or <tt>COUNTOF(digest) == HASH::DIGESTSIZE</tt> ensures
	//!   the output byte buffer is large enough for the digest.
	virtual bool Verify(const byte *digest)
		{return TruncatedVerify(digest, DigestSize());}

	//! \brief Updates the hash with additional input and verifies the hash of the current message
	//! \param digest a pointer to the buffer of an \a existing hash
	//! \param input the additional input as a buffer
	//! \param length the size of the buffer, in bytes
	//! \return \p true if the existing hash matches the computed hash, \p false otherwise
	//! \throws ThrowIfInvalidTruncatedSize() if the existing hash's size exceeds DigestSize()
	//! \details Use this if your input is in one piece and you don't want to call Update()
	//!   and Verify() separately
	//! \details VerifyDigest() performs a bitwise compare on the buffers using VerifyBufsEqual(),
	//!   which is a constant time comparison function. digestLength cannot exceed DigestSize().
	//! \details VerifyDigest() restarts the hash for the next message.
	//! \pre <tt>COUNTOF(digest) == DigestSize()</tt> or <tt>COUNTOF(digest) == HASH::DIGESTSIZE</tt> ensures
	//!   the output byte buffer is large enough for the digest.
	virtual bool VerifyDigest(const byte *digest, const byte *input, size_t length)
		{Update(input, length); return Verify(digest);}

	//! \brief Computes the hash of the current message
	//! \param digest a pointer to the buffer to receive the hash
	//! \param digestSize the size of the truncated digest, in bytes
	//! \details TruncatedFinal() call Final() and then copies digestSize bytes to digest.
	//!   The hash is restarted the hash for the next message.
	virtual void TruncatedFinal(byte *digest, size_t digestSize) =0;

	//! \brief Updates the hash with additional input and computes the hash of the current message
	//! \param digest a pointer to the buffer to receive the hash
	//! \param digestSize the length of the truncated hash, in bytes
	//! \param input the additional input as a buffer
	//! \param length the size of the buffer, in bytes
	//! \details Use this if your input is in one piece and you don't want to call Update()
	//!   and CalculateDigest() separately.
	//! \details CalculateTruncatedDigest() restarts the hash for the next message.
	//! \pre <tt>COUNTOF(digest) == DigestSize()</tt> or <tt>COUNTOF(digest) == HASH::DIGESTSIZE</tt> ensures
	//!   the output byte buffer is large enough for the digest.
	virtual void CalculateTruncatedDigest(byte *digest, size_t digestSize, const byte *input, size_t length)
		{Update(input, length); TruncatedFinal(digest, digestSize);}

	//! \brief Verifies the hash of the current message
	//! \param digest a pointer to the buffer of an \a existing hash
	//! \param digestLength the size of the truncated hash, in bytes
	//! \return \p true if the existing hash matches the computed hash, \p false otherwise
	//! \throws ThrowIfInvalidTruncatedSize() if digestLength exceeds DigestSize()
	//! \details TruncatedVerify() is a truncated version of Verify(). It can operate on a
	//!   buffer smaller than DigestSize(). However, digestLength cannot exceed DigestSize().
	//! \details Verify() performs a bitwise compare on the buffers using VerifyBufsEqual(), which is
	//!   a constant time comparison function. digestLength cannot exceed DigestSize().
	//! \details TruncatedVerify() restarts the hash for the next message.
	virtual bool TruncatedVerify(const byte *digest, size_t digestLength);

	//! \brief Updates the hash with additional input and verifies the hash of the current message
	//! \param digest a pointer to the buffer of an \a existing hash
	//! \param digestLength the size of the truncated hash, in bytes
	//! \param input the additional input as a buffer
	//! \param length the size of the buffer, in bytes
	//! \return \p true if the existing hash matches the computed hash, \p false otherwise
	//! \throws ThrowIfInvalidTruncatedSize() if digestLength exceeds DigestSize()
	//! \details Use this if your input is in one piece and you don't want to call Update()
	//!   and TruncatedVerify() separately.
	//! \details VerifyTruncatedDigest() is a truncated version of VerifyDigest(). It can operate
	//!   on a buffer smaller than DigestSize(). However, digestLength cannot exceed DigestSize().
	//! \details VerifyTruncatedDigest() restarts the hash for the next message.
	//! \pre <tt>COUNTOF(digest) == DigestSize()</tt> or <tt>COUNTOF(digest) == HASH::DIGESTSIZE</tt> ensures
	//!   the output byte buffer is large enough for the digest.
	virtual bool VerifyTruncatedDigest(const byte *digest, size_t digestLength, const byte *input, size_t length)
		{Update(input, length); return TruncatedVerify(digest, digestLength);}

protected:
	//! \brief Validates a truncated digest size
	//! \param size the requested digest size
	//! \throws InvalidArgument if the algorithm's digest size cannot be truncated to the requested size
	//! \details Throws an exception when the truncated digest size is greater than DigestSize()
	void ThrowIfInvalidTruncatedSize(size_t size) const;
};

typedef HashTransformation HashFunction;

//! \brief Interface for one direction (encryption or decryption) of a block cipher
//! \details These objects usually should not be used directly. See BlockTransformation for more details.
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE BlockCipher : public SimpleKeyingInterface, public BlockTransformation
{
protected:
	const Algorithm & GetAlgorithm() const {return *this;}
};

//! \brief Interface for one direction (encryption or decryption) of a stream cipher or cipher mode
//! \details These objects usually should not be used directly. See StreamTransformation for more details.
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE SymmetricCipher : public SimpleKeyingInterface, public StreamTransformation
{
protected:
	const Algorithm & GetAlgorithm() const {return *this;}
};

//! \brief Interface for message authentication codes
//! \details These objects usually should not be used directly. See HashTransformation for more details.
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE MessageAuthenticationCode : public SimpleKeyingInterface, public HashTransformation
{
protected:
	const Algorithm & GetAlgorithm() const {return *this;}
};

//! \brief Interface for one direction (encryption or decryption) of a stream cipher or block cipher mode with authentication
//! \details The StreamTransformation part of this interface is used to encrypt/decrypt the data, and the
//!   MessageAuthenticationCode part of this interface is used to input additional authenticated data (AAD,
//!   which is MAC'ed but not encrypted), and to generate/verify the MAC.
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE AuthenticatedSymmetricCipher : public MessageAuthenticationCode, public StreamTransformation
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~AuthenticatedSymmetricCipher() {}
#endif

	//! \brief Exception thrown when the object is in the wrong state for the operation
	//! \details this indicates that a member function was called in the wrong state, for example trying to encrypt
	//!   a message before having set the key or IV
	class BadState : public Exception
	{
	public:
		explicit BadState(const std::string &name, const char *message) : Exception(OTHER_ERROR, name + ": " + message) {}
		explicit BadState(const std::string &name, const char *function, const char *state) : Exception(OTHER_ERROR, name + ": " + function + " was called before " + state) {}
	};

	//! \brief Provides the maximum length of AAD that can be input
	//! \return the maximum length of AAD that can be input before the encrypted data
	virtual lword MaxHeaderLength() const =0;
	//! \brief Provides the maximum length of encrypted data
	//! \return the maximum length of encrypted data
	virtual lword MaxMessageLength() const =0;
	//! \brief Provides the the maximum length of AAD
	//! \return the maximum length of AAD that can be input after the encrypted data
	virtual lword MaxFooterLength() const {return 0;}
	//! \brief Determines if data lengths must be specified prior to inputting data
	//! \return true if the data lengths are required before inputting data, false otherwise
	//! \details if this function returns true, SpecifyDataLengths() must be called before attempting to input data.
	//!   This is the case for some schemes, such as CCM.
	//! \sa SpecifyDataLengths()
	virtual bool NeedsPrespecifiedDataLengths() const {return false;}
	//! \brief Prespecifies the data lengths
	//! \details this function only needs to be called if NeedsPrespecifiedDataLengths() returns true
	//! \sa NeedsPrespecifiedDataLengths()
	void SpecifyDataLengths(lword headerLength, lword messageLength, lword footerLength=0);
	//! \brief Encrypts and calculates a MAC in one call
	//! \return true if the authenticated encryption succeeded, false otherwise
	//! \details EncryptAndAuthenticate() encrypts and generates the MAC in one call. The function will truncate MAC if
	//!   <tt>macSize < TagSize()</tt>.
	virtual void EncryptAndAuthenticate(byte *ciphertext, byte *mac, size_t macSize, const byte *iv, int ivLength, const byte *header, size_t headerLength, const byte *message, size_t messageLength);
	//! \brief Decrypts and verifies a MAC in one call
	//! \return true if the MAC is valid and the decoding succeeded, false otherwise
	//! \details DecryptAndVerify() decrypts and verifies the MAC in one call. The function returns true iff MAC is valid.
	//!   DecryptAndVerify() will assume MAC is truncated if <tt>macLength < TagSize()</tt>.
	virtual bool DecryptAndVerify(byte *message, const byte *mac, size_t macLength, const byte *iv, int ivLength, const byte *header, size_t headerLength, const byte *ciphertext, size_t ciphertextLength);

	//! \brief Provides the name of this algorithm
	//! \return the standard algorithm name
	//! \details The standard algorithm name can be a name like \a AES or \a AES/GCM. Some algorithms
	//!   do not have standard names yet. For example, there is no standard algorithm name for
	//!   Shoup's  ECIES.
	virtual std::string AlgorithmName() const =0;

protected:
	const Algorithm & GetAlgorithm() const
		{return *static_cast<const MessageAuthenticationCode *>(this);}
	virtual void UncheckedSpecifyDataLengths(lword headerLength, lword messageLength, lword footerLength)
		{CRYPTOPP_UNUSED(headerLength); CRYPTOPP_UNUSED(messageLength); CRYPTOPP_UNUSED(footerLength);}
};

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY
typedef SymmetricCipher StreamCipher;
#endif

//! \class RandomNumberGenerator
//! \brief Interface for random number generators
//! \details The library provides a number of random number generators, from software based to hardware based generators.
//! \details All generated values are uniformly distributed over the range specified.
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE RandomNumberGenerator : public Algorithm
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~RandomNumberGenerator() {}
#endif

	//! \brief Update RNG state with additional unpredictable values
	//! \param input the entropy to add to the generator
	//! \param length the size of the input buffer
	//! \throws NotImplemented
	//! \details A generator may or may not accept additional entropy. Call CanIncorporateEntropy() to test for the
	//!   ability to use additional entropy.
	//! \details If a derived class does not override IncorporateEntropy(), then the base class throws
	//!   NotImplemented.
	virtual void IncorporateEntropy(const byte *input, size_t length)
	{
		CRYPTOPP_UNUSED(input); CRYPTOPP_UNUSED(length);
		throw NotImplemented("RandomNumberGenerator: IncorporateEntropy not implemented");
	}

	//! \brief Determines if a generator can accept additional entropy
	//! \return true if IncorporateEntropy() is implemented
	virtual bool CanIncorporateEntropy() const {return false;}

	//! \brief Generate new random byte and return it
	//! \return a random 8-bit byte
	//! \details Default implementation calls GenerateBlock() with one byte.
	//! \details All generated values are uniformly distributed over the range specified within the
	//!   the contraints of a particular generator.
	virtual byte GenerateByte();

	//! \brief Generate new random bit and return it
	//! \return a random bit
	//! \details The default implementation calls GenerateByte() and return its lowest bit.
	//! \details All generated values are uniformly distributed over the range specified within the
	//!   the contraints of a particular generator.
	virtual unsigned int GenerateBit();

	//! \brief Generate a random 32 bit word in the range min to max, inclusive
	//! \param min the lower bound of the range
	//! \param max the upper bound of the range
	//! \return a random 32-bit word
	//! \details The default implementation calls Crop() on the difference between max and
	//!    min, and then returns the result added to min.
	//! \details All generated values are uniformly distributed over the range specified within the
	//!   the contraints of a particular generator.
	virtual word32 GenerateWord32(word32 min=0, word32 max=0xffffffffUL);

	//! \brief Generate random array of bytes
	//! \param output the byte buffer
	//! \param size the length of the buffer, in bytes
	//! \details All generated values are uniformly distributed over the range specified within the
	//!   the contraints of a particular generator.
	//! \note A derived generator \a must override either GenerateBlock() or
	//!    GenerateIntoBufferedTransformation(). They can override both, or have one call the other.
	virtual void GenerateBlock(byte *output, size_t size);

	//! \brief Generate random bytes into a BufferedTransformation
	//! \param target the BufferedTransformation object which receives the bytes
	//! \param channel the channel on which the bytes should be pumped
	//! \param length the number of bytes to generate
	//! \details The default implementation calls GenerateBlock() and pumps the result into
	//!   the  DEFAULT_CHANNEL of the target.
	//! \details All generated values are uniformly distributed over the range specified within the
	//!   the contraints of a particular generator.
	//! \note A derived generator \a must override either GenerateBlock() or
	//!    GenerateIntoBufferedTransformation(). They can override both, or have one call the other.
	virtual void GenerateIntoBufferedTransformation(BufferedTransformation &target, const std::string &channel, lword length);

	//! \brief Generate and discard n bytes
	//! \param n the number of bytes to generate and discard
	virtual void DiscardBytes(size_t n);

	//! \brief Randomly shuffle the specified array
	//! \param begin an iterator to the first element in the array
	//! \param end an iterator beyond the last element in the array
	//! \details The resulting permutation is uniformly distributed.
	template <class IT> void Shuffle(IT begin, IT end)
	{
		// TODO: What happens if there are more than 2^32 elements?
		for (; begin != end; ++begin)
			std::iter_swap(begin, begin + GenerateWord32(0, end-begin-1));
	}

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY
	byte GetByte() {return GenerateByte();}
	unsigned int GetBit() {return GenerateBit();}
	word32 GetLong(word32 a=0, word32 b=0xffffffffL) {return GenerateWord32(a, b);}
	word16 GetShort(word16 a=0, word16 b=0xffff) {return (word16)GenerateWord32(a, b);}
	void GetBlock(byte *output, size_t size) {GenerateBlock(output, size);}
#endif

};

//! \brief Random Number Generator that does not produce random numbers
//! \return reference that can be passed to functions that require a RandomNumberGenerator
//! \details NullRNG() returns a reference that can be passed to functions that require a
//!   RandomNumberGenerator but don't actually use it. The NullRNG() throws NotImplemented
//!   when a generation function is called.
//! \sa ClassNullRNG, PK_SignatureScheme::IsProbabilistic()
CRYPTOPP_DLL RandomNumberGenerator & CRYPTOPP_API NullRNG();

//! \class WaitObjectContainer
class WaitObjectContainer;
//! \class CallStack
class CallStack;

//! \brief Interface for objects that can be waited on.
class CRYPTOPP_NO_VTABLE Waitable
{
public:
	virtual ~Waitable() {}

	//! \brief Maximum number of wait objects that this object can return
	//! \return the maximum number of wait objects
	virtual unsigned int GetMaxWaitObjectCount() const =0;

	//! \brief Retrieves waitable objects
	//! \param container the wait container to receive the references to the objects.
	//! \param callStack  CallStack object used to select waitable objects
	//! \details GetWaitObjects is usually called in one of two ways. First, it can
	//!   be called like <tt>something.GetWaitObjects(c, CallStack("my func after X", 0));</tt>.
	//!   Second, if in an outer  GetWaitObjects() method that itself takes a callStack
	//!   parameter, it can be called like
	//!   <tt>innerThing.GetWaitObjects(c, CallStack("MyClass::GetWaitObjects at X", &callStack));</tt>.
	virtual void GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack) =0;

	//! \brief Wait on this object
	//! \return true if the wait succeeded, false otherwise
	//! \details Wait() is the same as creating an empty container, calling GetWaitObjects(), and then calling
	//!   Wait() on the container.
	bool Wait(unsigned long milliseconds, CallStack const& callStack);
};

//! \brief Default channel for BufferedTransformation
//! \details DEFAULT_CHANNEL is equal to an empty  string
extern CRYPTOPP_DLL const std::string DEFAULT_CHANNEL;

//! \brief Channel for additional authenticated data
//! \details AAD_CHANNEL is equal to "AAD"
extern CRYPTOPP_DLL const std::string AAD_CHANNEL;

//! \brief Interface for buffered transformations
//! \details BufferedTransformation is a generalization of BlockTransformation,
//!   StreamTransformation and HashTransformation.
//! \details A buffered transformation is an object that takes a stream of bytes as input (this may
//!   be done in stages), does some computation on them, and then places the result into an internal
//!   buffer for later retrieval. Any partial result already in the output buffer is not modified
//!   by further input.
//! \details If a method takes a "blocking" parameter, and you pass  false for it, then the method
//!   will return before all input has been processed if the input cannot be processed without waiting
//!   (for network buffers to become available, for example). In this case the method will return true
//!   or a non-zero integer value. When this happens you must continue to call the method with the same
//!   parameters until it returns false or zero, before calling any other method on it or attached
//!   /p BufferedTransformation. The integer return value in this case is approximately
//!   the number of bytes left to be processed, and can be used to implement a progress bar.
//! \details For functions that take a "propagation" parameter, <tt>propagation != 0</tt> means pass on
//!   the signal to attached BufferedTransformation objects, with propagation decremented at each
//!   step until it reaches <tt>0</tt>. <tt>-1</tt> means unlimited propagation.
//! \details \a All of the retrieval functions, like Get() and GetWord32(), return the actual
//!   number of bytes retrieved, which is the lesser of the request number and  MaxRetrievable().
//! \details \a Most of the input functions, like Put() and PutWord32(), return the number of
//!   bytes remaining to be processed. A 0 value means all bytes were processed, and a non-0 value
//!   means bytes remain to be processed.
//! \nosubgrouping
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE BufferedTransformation : public Algorithm, public Waitable
{
public:
	// placed up here for CW8
	static const std::string &NULL_CHANNEL;	// same as DEFAULT_CHANNEL, for backwards compatibility

#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~BufferedTransformation() {}
#endif

	//! \brief Construct a BufferedTransformation
	BufferedTransformation() : Algorithm(false) {}

	//! \brief Provides a reference to this object
	//! \return A reference to this object
	//! \details Useful for passing a temporary object to a function that takes a non-const reference
	BufferedTransformation& Ref() {return *this;}

	//!	\name INPUT
	//@{

		//! \brief Input a byte for processing
		//! \param inByte the 8-bit byte (octet) to be processed.
		//! \param blocking specifies whether the object should block when processing input.
		//! \return the number of bytes that remain in the block (i.e., bytes not processed)
		//! \details <tt>Put(byte)</tt> calls <tt>Put(byte*, size_t)</tt>.
		size_t Put(byte inByte, bool blocking=true)
			{return Put(&inByte, 1, blocking);}

		//! \brief Input a byte buffer for processing
		//! \param inString the byte buffer to process
		//! \param length the size of the string, in bytes
		//! \param blocking specifies whether the object should block when processing input
		//! \return the number of bytes that remain in the block (i.e., bytes not processed)
		//! \details Internally, Put() calls Put2().
		size_t Put(const byte *inString, size_t length, bool blocking=true)
			{return Put2(inString, length, 0, blocking);}

		//! Input a 16-bit word for processing.
		//! \param value the 16-bit value to be processed
		//! \param order the  ByteOrder in which the word should be processed
		//! \param blocking specifies whether the object should block when processing input
		//! \return the number of bytes that remain in the block (i.e., bytes not processed)
		size_t PutWord16(word16 value, ByteOrder order=BIG_ENDIAN_ORDER, bool blocking=true);

		//! Input a 32-bit word for processing.
		//! \param value the 32-bit value to be processed.
		//! \param order the  ByteOrder in which the word should be processed.
		//! \param blocking specifies whether the object should block when processing input.
		//! \return the number of bytes that remain in the block (i.e., bytes not processed)
		size_t PutWord32(word32 value, ByteOrder order=BIG_ENDIAN_ORDER, bool blocking=true);

		//! \brief Request space which can be written into by the caller
		//! \param size the requested size of the buffer
		//! \details The purpose of this method is to help avoid extra memory allocations.
		//! \details size is an \a IN and \a OUT parameter and used as a hint. When the call is made,
		//!    size is the requested size of the buffer. When the call returns,  size is the size of
		//!   the array returned to the caller.
		//! \details The base class implementation sets  size to 0 and returns  NULL.
		//! \note Some objects, like ArraySink, cannot create a space because its fixed. In the case of
		//! an ArraySink, the pointer to the array is returned and the  size is remaining size.
		virtual byte * CreatePutSpace(size_t &size)
			{size=0; return NULL;}

		//! \brief Determines whether input can be modifed by the callee
		//! \return true if input can be modified, false otherwise
		//! \details The base class implementation returns  false.
		virtual bool CanModifyInput() const
			{return false;}

		//! \brief Input multiple bytes that may be modified by callee.
		//! \param inString the byte buffer to process
		//! \param length the size of the string, in bytes
		//! \param blocking specifies whether the object should block when processing input
		//! \return 0 indicates all bytes were processed during the call. Non-0 indicates the
		//!   number of bytes that were \a not processed
		size_t PutModifiable(byte *inString, size_t length, bool blocking=true)
			{return PutModifiable2(inString, length, 0, blocking);}

		//! \brief Signals the end of messages to the object
		//! \param propagation the number of attached transformations the  MessageEnd() signal should be passed
		//! \param blocking specifies whether the object should block when processing input
		//! \details propagation count includes this object. Setting propagation to <tt>1</tt> means this
		//!   object only. Setting propagation to <tt>-1</tt> means unlimited propagation.
		bool MessageEnd(int propagation=-1, bool blocking=true)
			{return !!Put2(NULL, 0, propagation < 0 ? -1 : propagation+1, blocking);}

		//! \brief Input multiple bytes for processing and signal the end of a message
		//! \param inString the byte buffer to process
		//! \param length the size of the string, in bytes
		//! \param propagation the number of attached transformations the  MessageEnd() signal should be passed
		//! \param blocking specifies whether the object should block when processing input
		//! \return the number of bytes that remain in the block (i.e., bytes not processed)
		//! \details Internally, PutMessageEnd() calls Put2() with a modified  propagation to
		//!    ensure all attached transformations finish processing the message.
		//! \details propagation count includes this object. Setting propagation to <tt>1</tt> means this
		//!   object only. Setting propagation to <tt>-1</tt> means unlimited propagation.
		size_t PutMessageEnd(const byte *inString, size_t length, int propagation=-1, bool blocking=true)
			{return Put2(inString, length, propagation < 0 ? -1 : propagation+1, blocking);}

		//! \brief Input multiple bytes for processing
		//! \param inString the byte buffer to process
		//! \param length the size of the string, in bytes
		//! \param messageEnd means how many filters to signal MessageEnd() to, including this one
		//! \param blocking specifies whether the object should block when processing input
		//! \details Derived classes must implement Put2().
		virtual size_t Put2(const byte *inString, size_t length, int messageEnd, bool blocking) =0;

		//! \brief Input multiple bytes that may be modified by callee.
		//! \param inString the byte buffer to process.
		//! \param length the size of the string, in bytes.
		//! \param messageEnd means how many filters to signal MessageEnd() to, including this one.
		//! \param blocking specifies whether the object should block when processing input.
		//! \details Internally, PutModifiable2() calls Put2().
		virtual size_t PutModifiable2(byte *inString, size_t length, int messageEnd, bool blocking)
			{return Put2(inString, length, messageEnd, blocking);}

		//! \class BlockingInputOnly
		//! \brief Exception thrown by objects that have \a not implemented nonblocking input processing
		//! \details BlockingInputOnly inherits from NotImplemented
		struct BlockingInputOnly : public NotImplemented
			{BlockingInputOnly(const std::string &s) : NotImplemented(s + ": Nonblocking input is not implemented by this object.") {}};
	//@}

	//!	\name WAITING
	//@{
		//! \brief Retrieves the maximum number of waitable objects
		unsigned int GetMaxWaitObjectCount() const;

		//! \brief Retrieves waitable objects
		//! \param container the wait container to receive the references to the objects
		//! \param callStack  CallStack object used to select waitable objects
		//! \details GetWaitObjects is usually called in one of two ways. First, it can
		//!    be called like <tt>something.GetWaitObjects(c, CallStack("my func after X", 0));</tt>.
		//!    Second, if in an outer  GetWaitObjects() method that itself takes a callStack
		//!    parameter, it can be called like
		//!    <tt>innerThing.GetWaitObjects(c, CallStack("MyClass::GetWaitObjects at X", &callStack));</tt>.
		void GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack);
	//@} // WAITING

	//!	\name SIGNALS
	//@{

		//! \brief Initialize or reinitialize this object, without signal propagation
		//! \param parameters a set of NameValuePairs to initialize this object
		//! \throws NotImplemented
		//! \details IsolatedInitialize() is used to initialize or reinitialize an object using a variable
		//!   number of  arbitrarily typed arguments. The function avoids the need for multiple constuctors providing
		//!   all possible combintations of configurable parameters.
		//! \details IsolatedInitialize() does not call Initialize() on attached transformations. If initialization
		//!   should be propagated, then use the Initialize() function.
		//! \details If a derived class does not override IsolatedInitialize(), then the base class throws
		//!   NotImplemented.
		virtual void IsolatedInitialize(const NameValuePairs &parameters) {
			CRYPTOPP_UNUSED(parameters);
			throw NotImplemented("BufferedTransformation: this object can't be reinitialized");
		}

		//! \brief Flushes data buffered by this object, without signal propagation
		//! \param hardFlush indicates whether all data should be flushed
		//! \param blocking specifies whether the object should block when processing input
		//! \note  hardFlush must be used with care
		virtual bool IsolatedFlush(bool hardFlush, bool blocking) =0;

		//! \brief Marks the end of a series of messages, without signal propagation
		//! \param blocking specifies whether the object should block when completing the processing on
		//!    the current series of messages
		virtual bool IsolatedMessageSeriesEnd(bool blocking)
			{CRYPTOPP_UNUSED(blocking); return false;}

		//! \brief Initialize or reinitialize this object, with signal propagation
		//! \param parameters a set of NameValuePairs to initialize or reinitialize this object
		//! \param propagation the number of attached transformations the Initialize() signal should be passed
		//! \details Initialize() is used to initialize or reinitialize an object using a variable number of
		//!   arbitrarily typed arguments. The function avoids the need for multiple constuctors providing
		//!   all possible combintations of configurable parameters.
		//! \details propagation count includes this object. Setting propagation to <tt>1</tt> means this
		//!   object only. Setting propagation to <tt>-1</tt> means unlimited propagation.
		virtual void Initialize(const NameValuePairs &parameters=g_nullNameValuePairs, int propagation=-1);

		//! \brief Flush buffered input and/or output, with signal propagation
		//! \param hardFlush is used to indicate whether all data should be flushed
		//! \param propagation the number of attached transformations the  Flush() signal should be passed
		//! \param blocking specifies whether the object should block when processing input
		//! \details propagation count includes this object. Setting propagation to <tt>1</tt> means this
		//!   object only. Setting propagation to <tt>-1</tt> means unlimited propagation.
		//! \note Hard flushes must be used with care. It means try to process and output everything, even if
		//!   there may not be enough data to complete the action. For example, hard flushing a HexDecoder
		//!   would cause an error if you do it after inputing an odd number of hex encoded characters.
		//! \note For some types of filters, like  ZlibDecompressor, hard flushes can only
		//!   be done at "synchronization points". These synchronization points are positions in the data
		//!   stream that are created by hard flushes on the corresponding reverse filters, in this
		//!   example ZlibCompressor. This is useful when zlib compressed data is moved across a
		//!   network in packets and compression state is preserved across packets, as in the SSH2 protocol.
		virtual bool Flush(bool hardFlush, int propagation=-1, bool blocking=true);

		//! \brief Marks the end of a series of messages, with signal propagation
		//! \param propagation the number of attached transformations the  MessageSeriesEnd() signal should be passed
		//! \param blocking specifies whether the object should block when processing input
		//! \details Each object that receives the signal will perform its processing, decrement
		//!    propagation, and then pass the signal on to attached transformations if the value is not 0.
		//! \details propagation count includes this object. Setting propagation to <tt>1</tt> means this
		//!   object only. Setting propagation to <tt>-1</tt> means unlimited propagation.
		//! \note There should be a MessageEnd() immediately before MessageSeriesEnd().
		virtual bool MessageSeriesEnd(int propagation=-1, bool blocking=true);

		//! \brief Set propagation of automatically generated and transferred signals
		//! \param propagation then new value
		//! \details Setting propagation to <tt>0</tt> means do not automaticly generate signals. Setting
		//!    propagation to <tt>-1</tt> means unlimited propagation.
		virtual void SetAutoSignalPropagation(int propagation)
			{CRYPTOPP_UNUSED(propagation);}

		//! \brief Retrieve automatic signal propagation value
		//! \return the number of attached transformations the signal is propogated to. 0 indicates
		//!   the signal is only witnessed by this object
		virtual int GetAutoSignalPropagation() const {return 0;}
public:

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY
		void Close() {MessageEnd();}
#endif
	//@}

	//!	\name RETRIEVAL OF ONE MESSAGE
	//@{

		//! \brief Provides the number of bytes ready for retrieval
		//! \return the number of bytes ready for retrieval
		//! \details All retrieval functions return the actual number of bytes retrieved, which is
		//!   the lesser of the request number and  MaxRetrievable()
		virtual lword MaxRetrievable() const;

		//! \brief Determines whether bytes are ready for retrieval
		//! \returns true if bytes are available for retrieval, false otherwise
		virtual bool AnyRetrievable() const;

		//! \brief Retrieve a 8-bit byte
		//! \param outByte the 8-bit value to be retrieved
		//! \return the number of bytes consumed during the call.
		//! \details Use the return value of  Get to detect short reads.
		virtual size_t Get(byte &outByte);

		//! \brief Retrieve a block of bytes
		//! \param outString a block of bytes
		//! \param getMax the number of bytes to  Get
		//! \return the number of bytes consumed during the call.
		//! \details Use the return value of  Get to detect short reads.
		virtual size_t Get(byte *outString, size_t getMax);

		//! \brief Peek a 8-bit byte
		//! \param outByte the 8-bit value to be retrieved
		//! \return the number of bytes read during the call.
		//! \details Peek does not remove bytes from the object. Use the return value of
		//!     Get to detect short reads.
		virtual size_t Peek(byte &outByte) const;

		//! \brief Peek a block of bytes
		//! \param outString a block of bytes
		//! \param peekMax the number of bytes to  Peek
		//! \return the number of bytes read during the call.
		//! \details Peek does not remove bytes from the object. Use the return value of
		//!     Get to detect short reads.
		virtual size_t Peek(byte *outString, size_t peekMax) const;

		//! \brief Retrieve a 16-bit word
		//! \param value the 16-bit value to be retrieved
		//! \param order the  ByteOrder in which the word should be retrieved
		//! \return the number of bytes consumed during the call.
		//! \details Use the return value of  GetWord16 to detect short reads.
		size_t GetWord16(word16 &value, ByteOrder order=BIG_ENDIAN_ORDER);

		//! \brief Retrieve a 32-bit word
		//! \param value the 32-bit value to be retrieved
		//! \param order the  ByteOrder in which the word should be retrieved
		//! \return the number of bytes consumed during the call.
		//! \details Use the return value of  GetWord16 to detect short reads.
		size_t GetWord32(word32 &value, ByteOrder order=BIG_ENDIAN_ORDER);

		//! \brief Peek a 16-bit word
		//! \param value the 16-bit value to be retrieved
		//! \param order the  ByteOrder in which the word should be retrieved
		//! \return the number of bytes consumed during the call.
		//! \details Peek does not consume bytes in the stream. Use the return value
		//!    of  GetWord16 to detect short reads.
		size_t PeekWord16(word16 &value, ByteOrder order=BIG_ENDIAN_ORDER) const;

		//! \brief Peek a 32-bit word
		//! \param value the 32-bit value to be retrieved
		//! \param order the  ByteOrder in which the word should be retrieved
		//! \return the number of bytes consumed during the call.
		//! \details Peek does not consume bytes in the stream. Use the return value
		//!    of  GetWord16 to detect short reads.
		size_t PeekWord32(word32 &value, ByteOrder order=BIG_ENDIAN_ORDER) const;

		//! move transferMax bytes of the buffered output to target as input

		//! \brief Transfer bytes from this object to another BufferedTransformation
		//! \param target the destination BufferedTransformation
		//! \param transferMax the number of bytes to transfer
		//! \param channel the channel on which the transfer should occur
		//! \return the number of bytes transferred during the call.
		//! \details TransferTo removes bytes from this object and moves them to the destination.
		//! \details The function always returns  transferMax. If an accurate count is needed, then use  TransferTo2.
		lword TransferTo(BufferedTransformation &target, lword transferMax=LWORD_MAX, const std::string &channel=DEFAULT_CHANNEL)
			{TransferTo2(target, transferMax, channel); return transferMax;}

		//! \brief Discard skipMax bytes from the output buffer
		//! \param skipMax the number of bytes to discard
		//! \details Skip() discards bytes from the output buffer, which is the AttachedTransformation(), if present.
		//!   The function always returns skipMax.
		//! \details If you want to skip bytes from a Source, then perform the following.
		//! <pre>StringSource ss(str, false, new Redirector(TheBitBucket()));
		//! ss.Pump(10);    // Skip 10 bytes from Source
		//! ss.Detach(new FilterChain(...));
		//! ss.PumpAll();
		//! </pre>
		virtual lword Skip(lword skipMax=LWORD_MAX);

		//! copy copyMax bytes of the buffered output to target as input

		//! \brief Copy bytes from this object to another BufferedTransformation
		//! \param target the destination BufferedTransformation
		//! \param copyMax the number of bytes to copy
		//! \param channel the channel on which the transfer should occur
		//! \return the number of bytes copied during the call.
		//! \details CopyTo copies bytes from this object to the destination. The bytes are not removed from this object.
		//! \details The function always returns  copyMax. If an accurate count is needed, then use  CopyRangeTo2.
		lword CopyTo(BufferedTransformation &target, lword copyMax=LWORD_MAX, const std::string &channel=DEFAULT_CHANNEL) const
			{return CopyRangeTo(target, 0, copyMax, channel);}

		//! \brief Copy bytes from this object using an index to another BufferedTransformation
		//! \param target the destination BufferedTransformation
		//! \param position the 0-based index of the byte stream to begin the copying
		//! \param copyMax the number of bytes to copy
		//! \param channel the channel on which the transfer should occur
		//! \return the number of bytes copied during the call.
		//! \details CopyTo copies bytes from this object to the destination. The bytes remain in this
		//!   object. Copying begins at the index position in the current stream, and not from an absolute
		//!   position in the stream.
		//! \details The function returns the new position in the stream after transferring the bytes starting at the index.
		lword CopyRangeTo(BufferedTransformation &target, lword position, lword copyMax=LWORD_MAX, const std::string &channel=DEFAULT_CHANNEL) const
			{lword i = position; CopyRangeTo2(target, i, i+copyMax, channel); return i-position;}

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY
		unsigned long MaxRetrieveable() const {return MaxRetrievable();}
#endif
	//@}

	//!	\name RETRIEVAL OF MULTIPLE MESSAGES
	//@{

		//! \brief Provides the number of bytes ready for retrieval
		//! \return the number of bytes ready for retrieval
		virtual lword TotalBytesRetrievable() const;

		//! \brief Provides the number of meesages processed by this object
		//! \return the number of meesages processed by this object
		//! \details NumberOfMessages returns number of times  MessageEnd() has been
		//!    received minus messages retrieved or skipped
		virtual unsigned int NumberOfMessages() const;

		//! \brief Determines if any messages are available for retrieval
		//! \returns true if <tt>NumberOfMessages() &gt; 0</tt>, false otherwise
		//! \details AnyMessages returns true if <tt>NumberOfMessages() &gt; 0</tt>
		virtual bool AnyMessages() const;

		//! \brief Start retrieving the next message
		//! \return true if a message is ready for retrieval
		//! \details GetNextMessage() returns true if a message is ready for retrieval; false
		//!   if no more messages exist or this message is not completely retrieved.
		virtual bool GetNextMessage();

		//! \brief Skip a number of meessages
		//! \return 0 if the requested number of messages was skipped, non-0 otherwise
		//! \details SkipMessages() skips count number of messages. If there is an AttachedTransformation()
		//!   then SkipMessages() is called on the attached transformation. If there is no attached
		//!   transformation, then count number of messages are sent to TheBitBucket() using TransferMessagesTo().
		virtual unsigned int SkipMessages(unsigned int count=UINT_MAX);

		//! \brief Transfer messages from this object to another BufferedTransformation
		//! \param target the destination BufferedTransformation
		//! \param count the number of messages to transfer
		//! \param channel the channel on which the transfer should occur
		//! \return the number of bytes that remain in the current transfer block (i.e., bytes not transferred)
		//! \details TransferMessagesTo2() removes messages from this object and moves them to the destination.
		//!   If all bytes are not transferred for a message, then processing stops and the number of remaining
		//!   bytes is returned. TransferMessagesTo() does not proceed to the next message.
		//! \details A return value of 0 indicates all messages were successfully transferred.
		unsigned int TransferMessagesTo(BufferedTransformation &target, unsigned int count=UINT_MAX, const std::string &channel=DEFAULT_CHANNEL)
			{TransferMessagesTo2(target, count, channel); return count;}

		//! \brief Copy messages from this object to another BufferedTransformation
		//! \param target the destination BufferedTransformation
		//! \param count the number of messages to transfer
		//! \param channel the channel on which the transfer should occur
		//! \return the number of bytes that remain in the current transfer block (i.e., bytes not transferred)
		//! \details CopyMessagesTo copies messages from this object and copies them to the destination.
		//!   If all bytes are not transferred for a message, then processing stops and the number of remaining
		//!   bytes is returned. CopyMessagesTo() does not proceed to the next message.
		//! \details A return value of 0 indicates all messages were successfully copied.
		unsigned int CopyMessagesTo(BufferedTransformation &target, unsigned int count=UINT_MAX, const std::string &channel=DEFAULT_CHANNEL) const;

		//! \brief Skip all messages in the series
		virtual void SkipAll();

		//! \brief Transfer all bytes from this object to another BufferedTransformation
		//! \param target the destination BufferedTransformation
		//! \param channel the channel on which the transfer should occur
		//! \return the number of bytes that remain in the current transfer block (i.e., bytes not transferred)
		//! \details TransferMessagesTo2() removes messages from this object and moves them to the destination.
		//!   Internally TransferAllTo() calls TransferAllTo2().
		void TransferAllTo(BufferedTransformation &target, const std::string &channel=DEFAULT_CHANNEL)
			{TransferAllTo2(target, channel);}

		//! \brief Copy messages from this object to another BufferedTransformation
		//! \param target the destination BufferedTransformation
		//! \param channel the channel on which the transfer should occur
		//! \details CopyAllTo copies messages from this object and copies them to the destination.
		void CopyAllTo(BufferedTransformation &target, const std::string &channel=DEFAULT_CHANNEL) const;

		//! \brief Retrieve the next message in a series
		//! \return true if a message was retreved, false otherwise
		//! \details Internally, the base class implementation returns false.
		virtual bool GetNextMessageSeries() {return false;}
		//! \brief Provides the number of messages in a series
		//! \return the number of messages in this series
		virtual unsigned int NumberOfMessagesInThisSeries() const {return NumberOfMessages();}
		//! \brief Provides the number of messages in a series
		//! \return the number of messages in this series
		virtual unsigned int NumberOfMessageSeries() const {return 0;}
	//@}

	//!	\name NON-BLOCKING TRANSFER OF OUTPUT
	//@{

		// upon return, byteCount contains number of bytes that have finished being transfered,
		// and returns the number of bytes left in the current transfer block

		//! \brief Transfer bytes from this object to another BufferedTransformation
		//! \param target the destination BufferedTransformation
		//! \param byteCount the number of bytes to transfer
		//! \param channel the channel on which the transfer should occur
		//! \param blocking specifies whether the object should block when processing input
		//! \return the number of bytes that remain in the transfer block (i.e., bytes not transferred)
		//! \details TransferTo() removes bytes from this object and moves them to the destination.
		//!   Transfer begins at the index position in the current stream, and not from an absolute
		//!   position in the stream.
		//! \details byteCount is an \a IN and \a OUT parameter. When the call is made,
		//!   byteCount is the requested size of the transfer. When the call returns, byteCount is
		//!   the number of bytes that were transferred.
		virtual size_t TransferTo2(BufferedTransformation &target, lword &byteCount, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true) =0;

		// upon return, begin contains the start position of data yet to be finished copying,
		// and returns the number of bytes left in the current transfer block

		//! \brief Copy bytes from this object to another BufferedTransformation
		//! \param target the destination BufferedTransformation
		//! \param begin the 0-based index of the first byte to copy in the stream
		//! \param end the 0-based index of the last byte to copy in the stream
		//! \param channel the channel on which the transfer should occur
		//! \param blocking specifies whether the object should block when processing input
		//! \return the number of bytes that remain in the copy block (i.e., bytes not copied)
		//! \details CopyRangeTo2 copies bytes from this object to the destination. The bytes are not
		//!   removed from this object. Copying begins at the index position in the current stream, and
		//!   not from an absolute position in the stream.
		//! \details begin is an \a IN and \a OUT parameter. When the call is made, begin is the
		//!   starting position of the copy. When the call returns, begin is the position of the first
		//!   byte that was \a not copied (which may be different tahn  end).  begin can be used for
		//!   subsequent calls to  CopyRangeTo2.
		virtual size_t CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end=LWORD_MAX, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true) const =0;

		// upon return, messageCount contains number of messages that have finished being transfered,
		// and returns the number of bytes left in the current transfer block

		//! \brief Transfer messages from this object to another BufferedTransformation
		//! \param target the destination BufferedTransformation
		//! \param messageCount the number of messages to transfer
		//! \param channel the channel on which the transfer should occur
		//! \param blocking specifies whether the object should block when processing input
		//! \return the number of bytes that remain in the current transfer block (i.e., bytes not transferred)
		//! \details TransferMessagesTo2() removes messages from this object and moves them to the destination.
		//! \details messageCount is an \a IN and \a OUT parameter. When the call is made, messageCount is the
		//!   the number of messages requested to  be transferred. When the call returns, messageCount is the
		//!   number of messages actually transferred.
		size_t TransferMessagesTo2(BufferedTransformation &target, unsigned int &messageCount, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true);

		// returns the number of bytes left in the current transfer block

		//! \brief Transfer all bytes from this object to another BufferedTransformation
		//! \param target the destination BufferedTransformation
		//! \param channel the channel on which the transfer should occur
		//! \param blocking specifies whether the object should block when processing input
		//! \return the number of bytes that remain in the current transfer block (i.e., bytes not transferred)
		//! \details TransferMessagesTo2() removes messages from this object and moves them to the destination.
		size_t TransferAllTo2(BufferedTransformation &target, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true);
	//@}

	//!	\name CHANNELS
	//@{
		//! \brief Exception thrown when a filter does not support named channels
		struct NoChannelSupport : public NotImplemented
			{NoChannelSupport(const std::string &name) : NotImplemented(name + ": this object doesn't support multiple channels") {}};
		//! \brief Exception thrown when a filter does not recognize a named channel
		struct InvalidChannelName : public InvalidArgument
			{InvalidChannelName(const std::string &name, const std::string &channel) : InvalidArgument(name + ": unexpected channel name \"" + channel + "\"") {}};

		//! \brief Input a byte for processing on a channel
		//! \param channel the channel to process the data.
		//! \param inByte the 8-bit byte (octet) to be processed.
		//! \param blocking specifies whether the object should block when processing input.
		//! \return 0 indicates all bytes were processed during the call. Non-0 indicates the
		//!   number of bytes that were \a not processed.
		size_t ChannelPut(const std::string &channel, byte inByte, bool blocking=true)
			{return ChannelPut(channel, &inByte, 1, blocking);}

		//! \brief Input a byte buffer for processing on a channel
		//! \param channel the channel to process the data
		//! \param inString the byte buffer to process
		//! \param length the size of the string, in bytes
		//! \param blocking specifies whether the object should block when processing input
		//! \return 0 indicates all bytes were processed during the call. Non-0 indicates the
		//!   number of bytes that were \a not processed.
		size_t ChannelPut(const std::string &channel, const byte *inString, size_t length, bool blocking=true)
			{return ChannelPut2(channel, inString, length, 0, blocking);}

		//! \brief Input multiple bytes that may be modified by callee on a channel
		//! \param channel the channel to process the data.
		//! \param inString the byte buffer to process
		//! \param length the size of the string, in bytes
		//! \param blocking specifies whether the object should block when processing input
		//! \return 0 indicates all bytes were processed during the call. Non-0 indicates the
		//!   number of bytes that were \a not processed.
		size_t ChannelPutModifiable(const std::string &channel, byte *inString, size_t length, bool blocking=true)
			{return ChannelPutModifiable2(channel, inString, length, 0, blocking);}

		//! \brief Input a 16-bit word for processing on a channel.
		//! \param channel the channel to process the data.
		//! \param value the 16-bit value to be processed.
		//! \param order the  ByteOrder in which the word should be processed.
		//! \param blocking specifies whether the object should block when processing input.
		//! \return 0 indicates all bytes were processed during the call. Non-0 indicates the
		//!   number of bytes that were \a not processed.
		size_t ChannelPutWord16(const std::string &channel, word16 value, ByteOrder order=BIG_ENDIAN_ORDER, bool blocking=true);

		//! \brief Input a 32-bit word for processing on a channel.
		//! \param channel the channel to process the data.
		//! \param value the 32-bit value to be processed.
		//! \param order the  ByteOrder in which the word should be processed.
		//! \param blocking specifies whether the object should block when processing input.
		//! \return 0 indicates all bytes were processed during the call. Non-0 indicates the
		//!   number of bytes that were \a not processed.
		size_t ChannelPutWord32(const std::string &channel, word32 value, ByteOrder order=BIG_ENDIAN_ORDER, bool blocking=true);

		//! \brief Signal the end of a message
		//! \param channel the channel to process the data.
		//! \param propagation the number of attached transformations the  ChannelMessageEnd() signal should be passed
		//! \param blocking specifies whether the object should block when processing input
		//! \return 0 indicates all bytes were processed during the call. Non-0 indicates the
		//!   number of bytes that were \a not processed.
		//! \details propagation count includes this object. Setting propagation to <tt>1</tt> means this
		//!   object only. Setting propagation to <tt>-1</tt> means unlimited propagation.
		bool ChannelMessageEnd(const std::string &channel, int propagation=-1, bool blocking=true)
			{return !!ChannelPut2(channel, NULL, 0, propagation < 0 ? -1 : propagation+1, blocking);}

		//! \brief Input multiple bytes for processing and signal the end of a message
		//! \param channel the channel to process the data.
		//! \param inString the byte buffer to process
		//! \param length the size of the string, in bytes
		//! \param propagation the number of attached transformations the ChannelPutMessageEnd() signal should be passed
		//! \param blocking specifies whether the object should block when processing input
		//! \return the number of bytes that remain in the block (i.e., bytes not processed)
		//! \details propagation count includes this object. Setting propagation to <tt>1</tt> means this
		//!   object only. Setting propagation to <tt>-1</tt> means unlimited propagation.
		size_t ChannelPutMessageEnd(const std::string &channel, const byte *inString, size_t length, int propagation=-1, bool blocking=true)
			{return ChannelPut2(channel, inString, length, propagation < 0 ? -1 : propagation+1, blocking);}

		//! \brief Request space which can be written into by the caller
		//! \param channel the channel to process the data
		//! \param size the requested size of the buffer
		//! \return a pointer to a memroy block with length size
		//! \details The purpose of this method is to help avoid extra memory allocations.
		//! \details size is an \a IN and \a OUT parameter and used as a hint. When the call is made,
		//!    size is the requested size of the buffer. When the call returns,  size is the size of
		//!   the array returned to the caller.
		//! \details The base class implementation sets size to 0 and returns NULL.
		//! \note Some objects, like ArraySink(), cannot create a space because its fixed. In the case of
		//! an ArraySink(), the pointer to the array is returned and the size is remaining size.
		virtual byte * ChannelCreatePutSpace(const std::string &channel, size_t &size);

		//! \brief Input multiple bytes for processing on a channel.
		//! \param channel the channel to process the data.
		//! \param inString the byte buffer to process.
		//! \param length the size of the string, in bytes.
		//! \param messageEnd means how many filters to signal MessageEnd() to, including this one.
		//! \param blocking specifies whether the object should block when processing input.
		//! \return the number of bytes that remain in the block (i.e., bytes not processed)
		virtual size_t ChannelPut2(const std::string &channel, const byte *inString, size_t length, int messageEnd, bool blocking);

		//! \brief Input multiple bytes that may be modified by callee on a channel
		//! \param channel the channel to process the data
		//! \param inString the byte buffer to process
		//! \param length the size of the string, in bytes
		//! \param messageEnd means how many filters to signal MessageEnd() to, including this one
		//! \param blocking specifies whether the object should block when processing input
		//! \return the number of bytes that remain in the block (i.e., bytes not processed)
		virtual size_t ChannelPutModifiable2(const std::string &channel, byte *inString, size_t length, int messageEnd, bool blocking);

		//! \brief Flush buffered input and/or output on a channel
		//! \param channel the channel to flush the data
		//! \param hardFlush is used to indicate whether all data should be flushed
		//! \param propagation the number of attached transformations the  ChannelFlush() signal should be passed
		//! \param blocking specifies whether the object should block when processing input
		//! \return true of the Flush was successful
		//! \details propagation count includes this object. Setting propagation to <tt>1</tt> means this
		//!   object only. Setting propagation to <tt>-1</tt> means unlimited propagation.
		virtual bool ChannelFlush(const std::string &channel, bool hardFlush, int propagation=-1, bool blocking=true);

		//! \brief Marks the end of a series of messages on a channel
		//! \param channel the channel to signal the end of a series of messages
		//! \param propagation the number of attached transformations the  ChannelMessageSeriesEnd() signal should be passed
		//! \param blocking specifies whether the object should block when processing input
		//! \details Each object that receives the signal will perform its processing, decrement
		//!     propagation, and then pass the signal on to attached transformations if the value is not 0.
		//! \details propagation count includes this object. Setting propagation to <tt>1</tt> means this
		//!   object only. Setting propagation to <tt>-1</tt> means unlimited propagation.
		//! \note There should be a MessageEnd() immediately before MessageSeriesEnd().
		virtual bool ChannelMessageSeriesEnd(const std::string &channel, int propagation=-1, bool blocking=true);

		//! \brief Sets the default retrieval channel
		//! \param channel the channel to signal the end of a series of messages
		//! \note this function may not be implemented in all objects that should support it.
		virtual void SetRetrievalChannel(const std::string &channel);
	//@}

	//!	\name ATTACHMENT
	//! \details Some BufferedTransformation objects (e.g. Filter objects) allow other BufferedTransformation objects to be
	//!   attached. When this is done, the first object instead of buffering its output, sends that output to the attached
	//!   object as input. The entire attachment chain is deleted when the anchor object is destructed.

	//@{
		//! \brief Determines whether the object allows attachment
		//! \return true if the object allows an attachment, false otherwise
		//! \details Sources and  Filters will returns true, while  Sinks and other objects will return  false.
		virtual bool Attachable() {return false;}

		//! \brief Returns the object immediately attached to this object
		//! \return the attached transformation
		//! \details AttachedTransformation() returns NULL if there is no attachment. The non-const
		//!   version of AttachedTransformation() always returns NULL.
		virtual BufferedTransformation *AttachedTransformation() {CRYPTOPP_ASSERT(!Attachable()); return 0;}

		//! \brief Returns the object immediately attached to this object
		//! \return the attached transformation
		//! \details AttachedTransformation() returns NULL if there is no attachment. The non-const
		//!   version of AttachedTransformation() always returns NULL.
		virtual const BufferedTransformation *AttachedTransformation() const
			{return const_cast<BufferedTransformation *>(this)->AttachedTransformation();}

		//! \brief Delete the current attachment chain and attach a new one
		//! \param newAttachment the new BufferedTransformation to attach
		//! \throws NotImplemented
		//! \details Detach delete the current attachment chain and replace it with an optional  newAttachment
		//! \details If a derived class does not override  Detach, then the base class throws
		//!   NotImplemented.
		virtual void Detach(BufferedTransformation *newAttachment = 0) {
			CRYPTOPP_UNUSED(newAttachment); CRYPTOPP_ASSERT(!Attachable());
			throw NotImplemented("BufferedTransformation: this object is not attachable");
		}

		//! \brief Add newAttachment to the end of attachment chain
		//! \param newAttachment the attachment to add to the end of the chain
		virtual void Attach(BufferedTransformation *newAttachment);
	//@}

protected:
	//! \brief Decrements the propagation count while clamping at 0
	//! \return the decremented propagation or 0
	static int DecrementPropagation(int propagation)
		{return propagation != 0 ? propagation - 1 : 0;}

private:
	byte m_buf[4];	// for ChannelPutWord16 and ChannelPutWord32, to ensure buffer isn't deallocated before non-blocking operation completes
};

//! \brief An input discarding BufferedTransformation
//! \return a reference to a BufferedTransformation object that discards all input
CRYPTOPP_DLL BufferedTransformation & TheBitBucket();

//! \class CryptoMaterial
//! \brief Interface for crypto material, such as public and private keys, and crypto parameters
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CryptoMaterial : public NameValuePairs
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~CryptoMaterial() {}
#endif

	//! Exception thrown when invalid crypto material is detected
	class CRYPTOPP_DLL InvalidMaterial : public InvalidDataFormat
	{
	public:
		explicit InvalidMaterial(const std::string &s) : InvalidDataFormat(s) {}
	};

	//! \brief Assign values to this object
	//! \details This function can be used to create a public key from a private key.
	virtual void AssignFrom(const NameValuePairs &source) =0;

	//! \brief Check this object for errors
	//! \param rng a RandomNumberGenerator for objects which use randomized testing
	//! \param level the level of thoroughness
	//! \returns true if the tests succeed, false otherwise
	//! \details There are four levels of thoroughness:
	//!   <ul>
	//!   <li>0 - using this object won't cause a crash or exception
	//!   <li>1 - this object will probably function, and encrypt, sign, other operations correctly
	//!   <li>2 - ensure this object will function correctly, and perform reasonable security checks
	//!   <li>3 - perform reasonable security checks, and do checks that may take a long time
	//!   </ul>
	//! \details Level 0 does not require a RandomNumberGenerator. A NullRNG() can be used for level 0.
	//!   Level 1 may not check for weak keys and such. Levels 2 and 3 are recommended.
	//! \sa ThrowIfInvalid()
	virtual bool Validate(RandomNumberGenerator &rng, unsigned int level) const =0;

	//! \brief Check this object for errors
	//! \param rng a RandomNumberGenerator for objects which use randomized testing
	//! \param level the level of thoroughness
	//! \throws InvalidMaterial
	//! \details Internally, ThrowIfInvalid() calls Validate() and throws InvalidMaterial() if validation fails.
	//! \sa Validate()
	virtual void ThrowIfInvalid(RandomNumberGenerator &rng, unsigned int level) const
		{if (!Validate(rng, level)) throw InvalidMaterial("CryptoMaterial: this object contains invalid values");}

	//! \brief Saves a key to a BufferedTransformation
	//! \param bt the destination BufferedTransformation
	//! \throws NotImplemented
	//! \details Save() writes the material to a BufferedTransformation.
	//! \details If the material is a key, then the key is written with ASN.1 DER encoding. The key
	//!   includes an object identifier with an algorthm id, like a subjectPublicKeyInfo.
	//! \details A "raw" key without the "key info" can be saved using a key's DEREncode() method.
	//! \details If a derived class does not override Save(), then the base class throws
	//!   NotImplemented().
	virtual void Save(BufferedTransformation &bt) const
		{CRYPTOPP_UNUSED(bt); throw NotImplemented("CryptoMaterial: this object does not support saving");}

	//! \brief Loads a key from a BufferedTransformation
	//! \param bt the source BufferedTransformation
	//! \throws KeyingErr
	//! \details Load() attempts to read material from a BufferedTransformation. If the
	//!   material is a key that was generated outside the library, then the following
	//!   usually applies:
	//!   <ul>
	//!   <li>the key should be ASN.1 BER encoded
	//!   <li>the key should be a "key info"
	//!   </ul>
	//! \details "key info" means the key should have an object identifier with an algorthm id,
	//!   like a subjectPublicKeyInfo.
	//! \details To read a "raw" key without the "key info", then call the key's BERDecode() method.
	//! \note  Load generally does not check that the key is valid. Call Validate(), if needed.
	virtual void Load(BufferedTransformation &bt)
		{CRYPTOPP_UNUSED(bt); throw NotImplemented("CryptoMaterial: this object does not support loading");}

	//! \brief Determines whether the object supports precomputation
	//! \return true if the object supports precomputation, false otherwise
	//! \sa Precompute()
	virtual bool SupportsPrecomputation() const {return false;}

	//! \brief Perform precomputation
	//! \param precomputationStorage the suggested number of objects for the precompute table
	//! \throws NotImplemented
	//! \details The exact semantics of Precompute() varies, but it typically means calculate
	//!   a table of n objects that can be used later to speed up computation.
	//! \details If a derived class does not override Precompute(), then the base class throws
	//!   NotImplemented.
	//! \sa SupportsPrecomputation(), LoadPrecomputation(), SavePrecomputation()
	virtual void Precompute(unsigned int precomputationStorage) {
		CRYPTOPP_UNUSED(precomputationStorage); CRYPTOPP_ASSERT(!SupportsPrecomputation());
		throw NotImplemented("CryptoMaterial: this object does not support precomputation");
	}

	//! \brief Retrieve previously saved precomputation
	//! \param storedPrecomputation BufferedTransformation with the saved precomputation
	//! \throws NotImplemented
	//! \sa SupportsPrecomputation(), Precompute()
	virtual void LoadPrecomputation(BufferedTransformation &storedPrecomputation)
		{CRYPTOPP_UNUSED(storedPrecomputation); CRYPTOPP_ASSERT(!SupportsPrecomputation()); throw NotImplemented("CryptoMaterial: this object does not support precomputation");}
	//! \brief Save precomputation for later use
	//! \param storedPrecomputation BufferedTransformation to write the precomputation
	//! \throws NotImplemented
	//! \sa SupportsPrecomputation(), Precompute()
	virtual void SavePrecomputation(BufferedTransformation &storedPrecomputation) const
		{CRYPTOPP_UNUSED(storedPrecomputation); CRYPTOPP_ASSERT(!SupportsPrecomputation()); throw NotImplemented("CryptoMaterial: this object does not support precomputation");}

	//! \brief Perform a quick sanity check
	//! \details DoQuickSanityCheck() is for internal library use, and it should not be called by library users.
	void DoQuickSanityCheck() const	{ThrowIfInvalid(NullRNG(), 0);}

#if (defined(__SUNPRO_CC) && __SUNPRO_CC < 0x590)
	// Sun Studio 11/CC 5.8 workaround: it generates incorrect code when casting to an empty virtual base class
	char m_sunCCworkaround;
#endif
};

//! \class GeneratableCryptoMaterial
//! \brief Interface for generatable crypto material, such as private keys and crypto parameters
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE GeneratableCryptoMaterial : virtual public CryptoMaterial
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~GeneratableCryptoMaterial() {}
#endif

	//! \brief Generate a random key or crypto parameters
	//! \param rng a RandomNumberGenerator to produce keying material
	//! \param params additional initialization parameters
	//! \throws KeyingErr if a key can't be generated or algorithm parameters are invalid
	//! \details If a derived class does not override  GenerateRandom, then the base class throws
	//!    NotImplemented.
	virtual void GenerateRandom(RandomNumberGenerator &rng, const NameValuePairs &params = g_nullNameValuePairs) {
		CRYPTOPP_UNUSED(rng); CRYPTOPP_UNUSED(params);
		throw NotImplemented("GeneratableCryptoMaterial: this object does not support key/parameter generation");
	}

	//! \brief Generate a random key or crypto parameters
	//! \param rng a RandomNumberGenerator to produce keying material
	//! \param keySize the size of the key, in bits
	//! \throws KeyingErr if a key can't be generated or algorithm parameters are invalid
	//! \details GenerateRandomWithKeySize calls  GenerateRandom with a NameValuePairs
	//!    object with only "KeySize"
	void GenerateRandomWithKeySize(RandomNumberGenerator &rng, unsigned int keySize);
};

//! \brief Interface for public keys
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PublicKey : virtual public CryptoMaterial
{
};

//! \brief Interface for private keys
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PrivateKey : public GeneratableCryptoMaterial
{
};

//! \brief Interface for crypto prameters
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE CryptoParameters : public GeneratableCryptoMaterial
{
};

//! \brief Interface for asymmetric algorithms
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE AsymmetricAlgorithm : public Algorithm
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~AsymmetricAlgorithm() {}
#endif

	//! \brief Retrieves a reference to CryptoMaterial
	//! \return a reference to the crypto material
	virtual CryptoMaterial & AccessMaterial() =0;

	//! \brief Retrieves a reference to CryptoMaterial
	//! \return a const reference to the crypto material
	virtual const CryptoMaterial & GetMaterial() const =0;

	//! \brief Loads this object from a BufferedTransformation
	//! \param bt a BufferedTransformation object
	//! \deprecated for backwards compatibility, calls <tt>AccessMaterial().Load(bt)</tt>
	void BERDecode(BufferedTransformation &bt)
		{AccessMaterial().Load(bt);}

	//! \brief Saves this object to a BufferedTransformation
	//! \param bt a BufferedTransformation object
	//! \deprecated for backwards compatibility, calls GetMaterial().Save(bt)
	void DEREncode(BufferedTransformation &bt) const
		{GetMaterial().Save(bt);}
};

//! \brief Interface for asymmetric algorithms using public keys
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PublicKeyAlgorithm : public AsymmetricAlgorithm
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~PublicKeyAlgorithm() {}
#endif

	// VC60 workaround: no co-variant return type

	//! \brief Retrieves a reference to a Public Key
	//! \return a reference to the public key
	CryptoMaterial & AccessMaterial()
		{return AccessPublicKey();}
	//! \brief Retrieves a reference to a Public Key
	//! \return a const reference the public key
	const CryptoMaterial & GetMaterial() const
		{return GetPublicKey();}

	//! \brief Retrieves a reference to a Public Key
	//! \return a reference to the public key
	virtual PublicKey & AccessPublicKey() =0;
	//! \brief Retrieves a reference to a Public Key
	//! \return a const reference the public key
	virtual const PublicKey & GetPublicKey() const
		{return const_cast<PublicKeyAlgorithm *>(this)->AccessPublicKey();}
};

//! \brief Interface for asymmetric algorithms using private keys
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PrivateKeyAlgorithm : public AsymmetricAlgorithm
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~PrivateKeyAlgorithm() {}
#endif

	//! \brief Retrieves a reference to a Private Key
	//! \return a reference the private key
	CryptoMaterial & AccessMaterial() {return AccessPrivateKey();}
	//! \brief Retrieves a reference to a Private Key
	//! \return a const reference the private key
	const CryptoMaterial & GetMaterial() const {return GetPrivateKey();}

	//! \brief Retrieves a reference to a Private Key
	//! \return a reference the private key
	virtual PrivateKey & AccessPrivateKey() =0;
	//! \brief Retrieves a reference to a Private Key
	//! \return a const reference the private key
	virtual const PrivateKey & GetPrivateKey() const {return const_cast<PrivateKeyAlgorithm *>(this)->AccessPrivateKey();}
};

//! \brief Interface for key agreement algorithms
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE KeyAgreementAlgorithm : public AsymmetricAlgorithm
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~KeyAgreementAlgorithm() {}
#endif

	//! \brief Retrieves a reference to Crypto Parameters
	//! \return a reference the crypto parameters
	CryptoMaterial & AccessMaterial() {return AccessCryptoParameters();}
	//! \brief Retrieves a reference to Crypto Parameters
	//! \return a const reference the crypto parameters
	const CryptoMaterial & GetMaterial() const {return GetCryptoParameters();}

	//! \brief Retrieves a reference to Crypto Parameters
	//! \return a reference the crypto parameters
	virtual CryptoParameters & AccessCryptoParameters() =0;
	//! \brief Retrieves a reference to Crypto Parameters
	//! \return a const reference the crypto parameters
	virtual const CryptoParameters & GetCryptoParameters() const {return const_cast<KeyAgreementAlgorithm *>(this)->AccessCryptoParameters();}
};

//! \brief Interface for public-key encryptors and decryptors
//! \details This class provides an interface common to encryptors and decryptors
//!   for querying their plaintext and ciphertext lengths.
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_CryptoSystem
{
public:
	virtual ~PK_CryptoSystem() {}

	//! \brief Provides the maximum length of plaintext for a given ciphertext length
	//! \return the maximum size of the plaintext, in bytes
	//! \details This function returns 0 if ciphertextLength is not valid (too long or too short).
	virtual size_t MaxPlaintextLength(size_t ciphertextLength) const =0;

	//! \brief Calculate the length of ciphertext given length of plaintext
	//! \return the maximum size of the ciphertext, in bytes
	//! \details This function returns 0 if plaintextLength is not valid (too long).
	virtual size_t CiphertextLength(size_t plaintextLength) const =0;

	//! \brief Determines whether this object supports the use of a named parameter
	//! \param name the name of the parameter
	//! \return true if the parameter name is supported, false otherwise
	//! \details Some possible parameter names: EncodingParameters(), KeyDerivationParameters()
	//!   and others Parameters listed in argnames.h
	virtual bool ParameterSupported(const char *name) const =0;

	//! \brief Provides the fixed ciphertext length, if one exists
	//! \return the fixed ciphertext length if one exists, otherwise 0
	//! \details "Fixed" here means length of ciphertext does not depend on length of plaintext.
	//!   In this case, it usually does depend on the key length.
	virtual size_t FixedCiphertextLength() const {return 0;}

	//! \brief Provides the maximum plaintext length given a fixed ciphertext length
	//! \return maximum plaintext length given the fixed ciphertext length, if one exists,
	//!   otherwise return 0.
	//! \details FixedMaxPlaintextLength(0 returns the maximum plaintext length given the fixed ciphertext
	//!   length, if one exists, otherwise return 0.
	virtual size_t FixedMaxPlaintextLength() const {return 0;}

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY
	size_t MaxPlainTextLength(size_t cipherTextLength) const {return MaxPlaintextLength(cipherTextLength);}
	size_t CipherTextLength(size_t plainTextLength) const {return CiphertextLength(plainTextLength);}
#endif
};

//! \class PK_Encryptor
//! \brief Interface for public-key encryptors
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_Encryptor : public PK_CryptoSystem, public PublicKeyAlgorithm
{
public:
	//! \brief Exception thrown when trying to encrypt plaintext of invalid length
	class CRYPTOPP_DLL InvalidPlaintextLength : public Exception
	{
	public:
		InvalidPlaintextLength() : Exception(OTHER_ERROR, "PK_Encryptor: invalid plaintext length") {}
	};

	//! \brief Encrypt a byte string
	//! \param rng a RandomNumberGenerator derived class
	//! \param plaintext the plaintext byte buffer
	//! \param plaintextLength the size of the plaintext byte buffer
	//! \param ciphertext a byte buffer to hold the encrypted string
	//! \param parameters a set of NameValuePairs to initialize this object
	//! \pre <tt>CiphertextLength(plaintextLength) != 0</tt> ensures the plaintext isn't too large
	//! \pre <tt>COUNTOF(ciphertext) == CiphertextLength(plaintextLength)</tt> ensures the output
	//!   byte buffer is large enough.
	//! \sa PK_Decryptor
	virtual void Encrypt(RandomNumberGenerator &rng,
		const byte *plaintext, size_t plaintextLength,
		byte *ciphertext, const NameValuePairs &parameters = g_nullNameValuePairs) const =0;

	//! \brief Create a new encryption filter
	//! \param rng a RandomNumberGenerator derived class
	//! \param attachment an attached transformation
	//! \param parameters a set of NameValuePairs to initialize this object
	//! \details \p attachment can be \p NULL. The caller is responsible for deleting the returned pointer.
	//!   Encoding parameters should be passed in the "EP" channel.
	virtual BufferedTransformation * CreateEncryptionFilter(RandomNumberGenerator &rng,
		BufferedTransformation *attachment=NULL, const NameValuePairs &parameters = g_nullNameValuePairs) const;
};

//! \class PK_Decryptor
//! \brief Interface for public-key decryptors
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_Decryptor : public PK_CryptoSystem, public PrivateKeyAlgorithm
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~PK_Decryptor() {}
#endif

	//! \brief Decrypt a byte string
	//! \param rng a RandomNumberGenerator derived class
	//! \param ciphertext the encrypted byte buffer
	//! \param ciphertextLength the size of the encrypted byte buffer
	//! \param plaintext a byte buffer to hold the decrypted string
	//! \param parameters a set of NameValuePairs to initialize this object
	//! \return the result of the decryption operation
	//! \details If DecodingResult::isValidCoding is true, then DecodingResult::messageLength
	//!   is valid and holds the the actual length of the plaintext recovered. The result is undefined
	//!   if decryption failed. If DecodingResult::isValidCoding is false, then DecodingResult::messageLength
	//!   is undefined.
	//! \pre <tt>COUNTOF(plaintext) == MaxPlaintextLength(ciphertextLength)</tt> ensures the output
	//!   byte buffer is large enough
	//! \sa PK_Encryptor
	virtual DecodingResult Decrypt(RandomNumberGenerator &rng,
		const byte *ciphertext, size_t ciphertextLength,
		byte *plaintext, const NameValuePairs &parameters = g_nullNameValuePairs) const =0;

	//! \brief Create a new decryption filter
	//! \param rng a RandomNumberGenerator derived class
	//! \param attachment an attached transformation
	//! \param parameters a set of NameValuePairs to initialize this object
	//! \return the newly created decryption filter
	//! \note the caller is responsible for deleting the returned pointer
	virtual BufferedTransformation * CreateDecryptionFilter(RandomNumberGenerator &rng,
		BufferedTransformation *attachment=NULL, const NameValuePairs &parameters = g_nullNameValuePairs) const;

	//! \brief Decrypt a fixed size ciphertext
	//! \param rng a RandomNumberGenerator derived class
	//! \param ciphertext the encrypted byte buffer
	//! \param plaintext a byte buffer to hold the decrypted string
	//! \param parameters a set of NameValuePairs to initialize this object
	//! \return the result of the decryption operation
	//! \details If DecodingResult::isValidCoding is true, then DecodingResult::messageLength
	//!   is valid and holds the the actual length of the plaintext recovered. The result is undefined
	//!   if decryption failed. If DecodingResult::isValidCoding is false, then DecodingResult::messageLength
	//!   is undefined.
	//! \pre <tt>COUNTOF(plaintext) == MaxPlaintextLength(ciphertextLength)</tt> ensures the output
	//!   byte buffer is large enough
	//! \sa PK_Encryptor
	DecodingResult FixedLengthDecrypt(RandomNumberGenerator &rng, const byte *ciphertext, byte *plaintext, const NameValuePairs &parameters = g_nullNameValuePairs) const
		{return Decrypt(rng, ciphertext, FixedCiphertextLength(), plaintext, parameters);}
};

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY
typedef PK_CryptoSystem PK_FixedLengthCryptoSystem;
typedef PK_Encryptor PK_FixedLengthEncryptor;
typedef PK_Decryptor PK_FixedLengthDecryptor;
#endif

//! \class PK_SignatureScheme
//! \brief Interface for public-key signers and verifiers
//! \details This class provides an interface common to signers and verifiers for querying scheme properties
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_SignatureScheme
{
public:
	//! \class InvalidKeyLength
	//! \brief Exception throw when the private or public key has a length that can't be used
	//! \details InvalidKeyLength() may be thrown by any function in this class if the private
	//!   or public key has a length that can't be used
	class CRYPTOPP_DLL InvalidKeyLength : public Exception
	{
	public:
		InvalidKeyLength(const std::string &message) : Exception(OTHER_ERROR, message) {}
	};

	//! \class KeyTooShort
	//! \brief Exception throw when the private or public key is too short to sign or verify
	//! \details KeyTooShort() may be thrown by any function in this class if the private or public
	//!   key is too short to sign or verify anything
	class CRYPTOPP_DLL KeyTooShort : public InvalidKeyLength
	{
	public:
		KeyTooShort() : InvalidKeyLength("PK_Signer: key too short for this signature scheme") {}
	};

	virtual ~PK_SignatureScheme() {}

	//! \brief Provides the signature length if it only depends on the key
	//! \return the signature length if it only depends on the key, in bytes
	//! \details SignatureLength() returns the signature length if it only depends on the key, otherwise 0.
	virtual size_t SignatureLength() const =0;

	//! \brief Provides the maximum signature length produced given the length of the recoverable message part
	//! \param recoverablePartLength the length of the recoverable message part, in bytes
	//! \return the maximum signature length produced for a given length of recoverable message part, in bytes
	//! \details MaxSignatureLength() returns the maximum signature length produced given the length of the
	//!   recoverable message part.
	virtual size_t MaxSignatureLength(size_t recoverablePartLength = 0) const
	{CRYPTOPP_UNUSED(recoverablePartLength); return SignatureLength();}

	//! \brief Provides the length of longest message that can be recovered
	//! \return the length of longest message that can be recovered, in bytes
	//! \details MaxRecoverableLength() returns the length of longest message that can be recovered, or 0 if
	//!   this signature scheme does not support message recovery.
	virtual size_t MaxRecoverableLength() const =0;

	//! \brief Provides the length of longest message that can be recovered from a signature of given length
	//! \param signatureLength the length of the signature, in bytes
	//! \return the length of longest message that can be recovered from a signature of given length, in bytes
	//! \details MaxRecoverableLengthFromSignatureLength() returns the length of longest message that can be
	//!   recovered from a signature of given length, or 0 if this signature scheme does not support message
	//!   recovery.
	virtual size_t MaxRecoverableLengthFromSignatureLength(size_t signatureLength) const =0;

	//! \brief Determines whether a signature scheme requires a random number generator
	//! \return true if the signature scheme requires a RandomNumberGenerator() to sign
	//! \details if IsProbabilistic() returns false, then NullRNG() can be passed to functions that take
	//!   RandomNumberGenerator().
	virtual bool IsProbabilistic() const =0;

	//! \brief Determines whether the non-recoverable message part can be signed
	//! \return true if the non-recoverable message part can be signed
	virtual bool AllowNonrecoverablePart() const =0;

	//! \brief Determines whether the signature must be input before the message
	//! \return true if the signature must be input before the message during verifcation
	//! \details if SignatureUpfront() returns true, then you must input the signature before the message
	//!   during verification. Otherwise you can input the signature at anytime.
	virtual bool SignatureUpfront() const {return false;}

	//! \brief Determines whether the recoverable part must be input before the non-recoverable part
	//! \return true if the recoverable part must be input before the non-recoverable part during signing
	//! \details RecoverablePartFirst() determines whether you must input the recoverable part before the
	//!   non-recoverable part during signing
	virtual bool RecoverablePartFirst() const =0;
};

//! \class PK_MessageAccumulator
//! \brief Interface for accumulating messages to be signed or verified
//! \details Only Update() should be called from the PK_MessageAccumulator() class. No other functions
//!   inherited from HashTransformation, like DigestSize() and TruncatedFinal(), should be called.
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_MessageAccumulator : public HashTransformation
{
public:
	//! \warning DigestSize() should not be called on PK_MessageAccumulator
	unsigned int DigestSize() const
		{throw NotImplemented("PK_MessageAccumulator: DigestSize() should not be called");}

	//! \warning TruncatedFinal() should not be called on PK_MessageAccumulator
	void TruncatedFinal(byte *digest, size_t digestSize)
	{
		CRYPTOPP_UNUSED(digest); CRYPTOPP_UNUSED(digestSize);
		throw NotImplemented("PK_MessageAccumulator: TruncatedFinal() should not be called");
	}
};

//! \class PK_Signer
//! \brief Interface for public-key signers
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_Signer : public PK_SignatureScheme, public PrivateKeyAlgorithm
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~PK_Signer() {}
#endif

	//! \brief Create a new HashTransformation to accumulate the message to be signed
	//! \param rng a RandomNumberGenerator derived class
	//! \return a pointer to a PK_MessageAccumulator
	//! \details NewSignatureAccumulator() can be used with all signing methods. Sign() will autimatically delete the
	//!   accumulator pointer. The caller is responsible for deletion if a method is called that takes a reference.
	virtual PK_MessageAccumulator * NewSignatureAccumulator(RandomNumberGenerator &rng) const =0;

	//! \brief Input a recoverable message to an accumulator
	//! \param messageAccumulator a reference to a PK_MessageAccumulator
	//! \param recoverableMessage a pointer to the recoverable message part to be signed
	//! \param recoverableMessageLength the size of the recoverable message part
	virtual void InputRecoverableMessage(PK_MessageAccumulator &messageAccumulator, const byte *recoverableMessage, size_t recoverableMessageLength) const =0;

	//! \brief Sign and delete the messageAccumulator
	//! \param rng a RandomNumberGenerator derived class
	//! \param messageAccumulator a pointer to a PK_MessageAccumulator derived class
	//! \param signature a block of bytes for the signature
	//! \return actual signature length
	//! \details Sign() deletes the messageAccumulator, even if an exception is thrown.
	//! \pre <tt>COUNTOF(signature) == MaxSignatureLength()</tt>
	virtual size_t Sign(RandomNumberGenerator &rng, PK_MessageAccumulator *messageAccumulator, byte *signature) const;

	//! \brief Sign and restart messageAccumulator
	//! \param rng a RandomNumberGenerator derived class
	//! \param messageAccumulator a pointer to a PK_MessageAccumulator derived class
	//! \param signature a block of bytes for the signature
	//! \param restart flag indicating whether the messageAccumulator should be restarted
	//! \return actual signature length
	//! \pre <tt>COUNTOF(signature) == MaxSignatureLength()</tt>
	virtual size_t SignAndRestart(RandomNumberGenerator &rng, PK_MessageAccumulator &messageAccumulator, byte *signature, bool restart=true) const =0;

	//! \brief Sign a message
	//! \param rng a RandomNumberGenerator derived class
	//! \param message a pointer to the message
	//! \param messageLen the size of the message to be signed
	//! \param signature a block of bytes for the signature
	//! \return actual signature length
	//! \pre <tt>COUNTOF(signature) == MaxSignatureLength()</tt>
	virtual size_t SignMessage(RandomNumberGenerator &rng, const byte *message, size_t messageLen, byte *signature) const;

	//! \brief Sign a recoverable message
	//! \param rng a RandomNumberGenerator derived class
	//! \param recoverableMessage a pointer to the recoverable message part to be signed
	//! \param recoverableMessageLength the size of the recoverable message part
	//! \param nonrecoverableMessage a pointer to the non-recoverable message part to be signed
	//! \param nonrecoverableMessageLength the size of the non-recoverable message part
	//! \param signature a block of bytes for the signature
	//! \return actual signature length
	//! \pre <tt>COUNTOF(signature) == MaxSignatureLength(recoverableMessageLength)</tt>
	virtual size_t SignMessageWithRecovery(RandomNumberGenerator &rng, const byte *recoverableMessage, size_t recoverableMessageLength,
		const byte *nonrecoverableMessage, size_t nonrecoverableMessageLength, byte *signature) const;
};

//! \class PK_Verifier
//! \brief Interface for public-key signature verifiers
//! \details The Recover* functions throw NotImplemented if the signature scheme does not support
//!   message recovery.
//! \details The Verify* functions throw InvalidDataFormat if the scheme does support message
//!   recovery and the signature contains a non-empty recoverable message part. The
//!   Recover* functions should be used in that case.
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE PK_Verifier : public PK_SignatureScheme, public PublicKeyAlgorithm
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~PK_Verifier() {}
#endif

	//! \brief Create a new HashTransformation to accumulate the message to be verified
	//! \return a pointer to a PK_MessageAccumulator
	//! \details NewVerificationAccumulator() can be used with all verification methods. Verify() will autimatically delete
	//!   the accumulator pointer. The caller is responsible for deletion if a method is called that takes a reference.
	virtual PK_MessageAccumulator * NewVerificationAccumulator() const =0;

	//! \brief Input signature into a message accumulator
	//! \param messageAccumulator a pointer to a PK_MessageAccumulator derived class
	//! \param signature the signature on the message
	//! \param signatureLength the size of the signature
	virtual void InputSignature(PK_MessageAccumulator &messageAccumulator, const byte *signature, size_t signatureLength) const =0;

	//! \brief Check whether messageAccumulator contains a valid signature and message
	//! \param messageAccumulator a pointer to a PK_MessageAccumulator derived class
	//! \return true if the signature is valid, false otherwise
	//! \details Verify() deletes the messageAccumulator, even if an exception is thrown.
	virtual bool Verify(PK_MessageAccumulator *messageAccumulator) const;

	//! \brief Check whether messageAccumulator contains a valid signature and message, and restart messageAccumulator
	//! \param messageAccumulator a reference to a PK_MessageAccumulator derived class
	//! \return true if the signature is valid, false otherwise
	//! \details VerifyAndRestart() restarts the messageAccumulator
	virtual bool VerifyAndRestart(PK_MessageAccumulator &messageAccumulator) const =0;

	//! \brief Check whether input signature is a valid signature for input message
	//! \param message a pointer to the message to be verified
	//! \param messageLen the size of the message
	//! \param signature a pointer to the signature over the message
	//! \param signatureLen the size of the signature
	//! \return true if the signature is valid, false otherwise
	virtual bool VerifyMessage(const byte *message, size_t messageLen,
		const byte *signature, size_t signatureLen) const;

	//! \brief Recover a message from its signature
	//! \param recoveredMessage a pointer to the recoverable message part to be verified
	//! \param messageAccumulator a pointer to a PK_MessageAccumulator derived class
	//! \return the result of the verification operation
	//! \details Recover() deletes the messageAccumulator, even if an exception is thrown.
	//! \pre <tt>COUNTOF(recoveredMessage) == MaxRecoverableLengthFromSignatureLength(signatureLength)</tt>
	virtual DecodingResult Recover(byte *recoveredMessage, PK_MessageAccumulator *messageAccumulator) const;

	//! \brief Recover a message from its signature
	//! \param recoveredMessage a pointer to the recoverable message part to be verified
	//! \param messageAccumulator a pointer to a PK_MessageAccumulator derived class
	//! \return the result of the verification operation
	//! \details RecoverAndRestart() restarts the messageAccumulator
	//! \pre <tt>COUNTOF(recoveredMessage) == MaxRecoverableLengthFromSignatureLength(signatureLength)</tt>
	virtual DecodingResult RecoverAndRestart(byte *recoveredMessage, PK_MessageAccumulator &messageAccumulator) const =0;

	//! \brief Recover a message from its signature
	//! \param recoveredMessage a pointer for the recovered message
	//! \param nonrecoverableMessage a pointer to the non-recoverable message part to be signed
	//! \param nonrecoverableMessageLength the size of the non-recoverable message part
	//! \param signature the signature on the message
	//! \param signatureLength the size of the signature
	//! \return the result of the verification operation
	//! \pre <tt>COUNTOF(recoveredMessage) == MaxRecoverableLengthFromSignatureLength(signatureLength)</tt>
	virtual DecodingResult RecoverMessage(byte *recoveredMessage,
		const byte *nonrecoverableMessage, size_t nonrecoverableMessageLength,
		const byte *signature, size_t signatureLength) const;
};

//! \class SimpleKeyAgreementDomain
//! \brief Interface for domains of simple key agreement protocols
//! \details A key agreement domain is a set of parameters that must be shared
//!   by two parties in a key agreement protocol, along with the algorithms
//!   for generating key pairs and deriving agreed values.
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE SimpleKeyAgreementDomain : public KeyAgreementAlgorithm
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~SimpleKeyAgreementDomain() {}
#endif

	//! \brief Provides the size of the agreed value
	//! \return size of agreed value produced  in this domain
	virtual unsigned int AgreedValueLength() const =0;

	//! \brief Provides the size of the private key
	//! \return size of private keys in this domain
	virtual unsigned int PrivateKeyLength() const =0;

	//! \brief Provides the size of the public key
	//! \return size of public keys in this domain
	virtual unsigned int PublicKeyLength() const =0;

	//! \brief Generate private key in this domain
	//! \param rng a RandomNumberGenerator derived class
	//! \param privateKey a byte buffer for the generated private key in this domain
	//! \pre <tt>COUNTOF(privateKey) == PrivateKeyLength()</tt>
	virtual void GeneratePrivateKey(RandomNumberGenerator &rng, byte *privateKey) const =0;

	//! \brief Generate a public key from a private key in this domain
	//! \param rng a RandomNumberGenerator derived class
	//! \param privateKey a byte buffer with the previously generated private key
	//! \param publicKey a byte buffer for the generated public key in this domain
	//! \pre <tt>COUNTOF(publicKey) == PublicKeyLength()</tt>
	virtual void GeneratePublicKey(RandomNumberGenerator &rng, const byte *privateKey, byte *publicKey) const =0;

	//! \brief Generate a private/public key pair
	//! \param rng a RandomNumberGenerator derived class
	//! \param privateKey a byte buffer for the generated private key in this domain
	//! \param publicKey a byte buffer for the generated public key in this domain
	//! \details GenerateKeyPair() is equivalent to calling GeneratePrivateKey() and then GeneratePublicKey().
	//! \pre <tt>COUNTOF(privateKey) == PrivateKeyLength()</tt>
	//! \pre <tt>COUNTOF(publicKey) == PublicKeyLength()</tt>
	virtual void GenerateKeyPair(RandomNumberGenerator &rng, byte *privateKey, byte *publicKey) const;

	//! \brief Derive agreed value
	//! \param agreedValue a byte buffer for the shared secret
	//! \param privateKey a byte buffer with your private key in this domain
	//! \param otherPublicKey a byte buffer with the other party's public key in this domain
	//! \param validateOtherPublicKey a flag indicating if the other party's public key should be validated
	//! \return true upon success, false in case of failure
	//! \details Agree() derives an agreed value from your private keys and couterparty's public keys.
	//! \details The other party's public key is validated by default. If you have previously validated the
	//!   static public key, use <tt>validateStaticOtherPublicKey=false</tt> to save time.
	//! \pre <tt>COUNTOF(agreedValue) == AgreedValueLength()</tt>
	//! \pre <tt>COUNTOF(privateKey) == PrivateKeyLength()</tt>
	//! \pre <tt>COUNTOF(otherPublicKey) == PublicKeyLength()</tt>
	virtual bool Agree(byte *agreedValue, const byte *privateKey, const byte *otherPublicKey, bool validateOtherPublicKey=true) const =0;

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY
	bool ValidateDomainParameters(RandomNumberGenerator &rng) const
		{return GetCryptoParameters().Validate(rng, 2);}
#endif
};

//! \brief Interface for domains of authenticated key agreement protocols
//! \details In an authenticated key agreement protocol, each party has two
//!   key pairs. The long-lived key pair is called the static key pair,
//!   and the short-lived key pair is called the ephemeral key pair.
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE AuthenticatedKeyAgreementDomain : public KeyAgreementAlgorithm
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~AuthenticatedKeyAgreementDomain() {}
#endif

	//! \brief Provides the size of the agreed value
	//! \return size of agreed value produced  in this domain
	virtual unsigned int AgreedValueLength() const =0;

	//! \brief Provides the size of the static private key
	//! \return size of static private keys in this domain
	virtual unsigned int StaticPrivateKeyLength() const =0;

	//! \brief Provides the size of the static public key
	//! \return size of static public keys in this domain
	virtual unsigned int StaticPublicKeyLength() const =0;

	//! \brief Generate static private key in this domain
	//! \param rng a RandomNumberGenerator derived class
	//! \param privateKey a byte buffer for the generated private key in this domain
	//! \pre <tt>COUNTOF(privateKey) == PrivateStaticKeyLength()</tt>
	virtual void GenerateStaticPrivateKey(RandomNumberGenerator &rng, byte *privateKey) const =0;

	//! \brief Generate a static public key from a private key in this domain
	//! \param rng a RandomNumberGenerator derived class
	//! \param privateKey a byte buffer with the previously generated private key
	//! \param publicKey a byte buffer for the generated public key in this domain
	//! \pre <tt>COUNTOF(publicKey) == PublicStaticKeyLength()</tt>
	virtual void GenerateStaticPublicKey(RandomNumberGenerator &rng, const byte *privateKey, byte *publicKey) const =0;

	//! \brief Generate a static private/public key pair
	//! \param rng a RandomNumberGenerator derived class
	//! \param privateKey a byte buffer for the generated private key in this domain
	//! \param publicKey a byte buffer for the generated public key in this domain
	//! \details GenerateStaticKeyPair() is equivalent to calling GenerateStaticPrivateKey() and then GenerateStaticPublicKey().
	//! \pre <tt>COUNTOF(privateKey) == PrivateStaticKeyLength()</tt>
	//! \pre <tt>COUNTOF(publicKey) == PublicStaticKeyLength()</tt>
	virtual void GenerateStaticKeyPair(RandomNumberGenerator &rng, byte *privateKey, byte *publicKey) const;

	//! \brief Provides the size of ephemeral private key
	//! \return the size of ephemeral private key in this domain
	virtual unsigned int EphemeralPrivateKeyLength() const =0;

	//! \brief Provides the size of ephemeral public key
	//! \return the size of ephemeral public key in this domain
	virtual unsigned int EphemeralPublicKeyLength() const =0;

	//! \brief Generate ephemeral private key
	//! \param rng a RandomNumberGenerator derived class
	//! \param privateKey a byte buffer for the generated private key in this domain
	//! \pre <tt>COUNTOF(privateKey) == PrivateEphemeralKeyLength()</tt>
	virtual void GenerateEphemeralPrivateKey(RandomNumberGenerator &rng, byte *privateKey) const =0;

	//! \brief Generate ephemeral public key
	//! \param rng a RandomNumberGenerator derived class
	//! \param privateKey a byte buffer for the generated private key in this domain
	//! \param publicKey a byte buffer for the generated public key in this domain
	//! \pre <tt>COUNTOF(publicKey) == PublicEphemeralKeyLength()</tt>
	virtual void GenerateEphemeralPublicKey(RandomNumberGenerator &rng, const byte *privateKey, byte *publicKey) const =0;

	//! \brief Generate private/public key pair
	//! \param rng a RandomNumberGenerator derived class
	//! \param privateKey a byte buffer for the generated private key in this domain
	//! \param publicKey a byte buffer for the generated public key in this domain
	//! \details GenerateEphemeralKeyPair() is equivalent to calling GenerateEphemeralPrivateKey() and then GenerateEphemeralPublicKey()
	virtual void GenerateEphemeralKeyPair(RandomNumberGenerator &rng, byte *privateKey, byte *publicKey) const;

	//! \brief Derive agreed value
	//! \param agreedValue a byte buffer for the shared secret
	//! \param staticPrivateKey a byte buffer with your static private key in this domain
	//! \param ephemeralPrivateKey a byte buffer with your ephemeral private key in this domain
	//! \param staticOtherPublicKey a byte buffer with the other party's static public key in this domain
	//! \param ephemeralOtherPublicKey a byte buffer with the other party's ephemeral public key in this domain
	//! \param validateStaticOtherPublicKey a flag indicating if the other party's public key should be validated
	//! \return true upon success, false in case of failure
	//! \details Agree() derives an agreed value from your private keys and couterparty's public keys.
	//! \details The other party's ephemeral public key is validated by default. If you have previously validated
	//!   the static public key, use <tt>validateStaticOtherPublicKey=false</tt> to save time.
	//! \pre <tt>COUNTOF(agreedValue) == AgreedValueLength()</tt>
	//! \pre <tt>COUNTOF(staticPrivateKey) == StaticPrivateKeyLength()</tt>
	//! \pre <tt>COUNTOF(ephemeralPrivateKey) == EphemeralPrivateKeyLength()</tt>
	//! \pre <tt>COUNTOF(staticOtherPublicKey) == StaticPublicKeyLength()</tt>
	//! \pre <tt>COUNTOF(ephemeralOtherPublicKey) == EphemeralPublicKeyLength()</tt>
	virtual bool Agree(byte *agreedValue,
		const byte *staticPrivateKey, const byte *ephemeralPrivateKey,
		const byte *staticOtherPublicKey, const byte *ephemeralOtherPublicKey,
		bool validateStaticOtherPublicKey=true) const =0;

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY
	bool ValidateDomainParameters(RandomNumberGenerator &rng) const
		{return GetCryptoParameters().Validate(rng, 2);}
#endif
};

// interface for password authenticated key agreement protocols, not implemented yet
#if 0
//! \brief Interface for protocol sessions
/*! The methods should be called in the following order:

	InitializeSession(rng, parameters);	// or call initialize method in derived class
	while (true)
	{
		if (OutgoingMessageAvailable())
		{
			length = GetOutgoingMessageLength();
			GetOutgoingMessage(message);
			; // send outgoing message
		}

		if (LastMessageProcessed())
			break;

		; // receive incoming message
		ProcessIncomingMessage(message);
	}
	; // call methods in derived class to obtain result of protocol session
*/
class ProtocolSession
{
public:
	//! Exception thrown when an invalid protocol message is processed
	class ProtocolError : public Exception
	{
	public:
		ProtocolError(ErrorType errorType, const std::string &s) : Exception(errorType, s) {}
	};

	//! Exception thrown when a function is called unexpectedly
	/*! for example calling ProcessIncomingMessage() when ProcessedLastMessage() == true */
	class UnexpectedMethodCall : public Exception
	{
	public:
		UnexpectedMethodCall(const std::string &s) : Exception(OTHER_ERROR, s) {}
	};

	ProtocolSession() : m_rng(NULL), m_throwOnProtocolError(true), m_validState(false) {}
	virtual ~ProtocolSession() {}

	virtual void InitializeSession(RandomNumberGenerator &rng, const NameValuePairs &parameters) =0;

	bool GetThrowOnProtocolError() const {return m_throwOnProtocolError;}
	void SetThrowOnProtocolError(bool throwOnProtocolError) {m_throwOnProtocolError = throwOnProtocolError;}

	bool HasValidState() const {return m_validState;}

	virtual bool OutgoingMessageAvailable() const =0;
	virtual unsigned int GetOutgoingMessageLength() const =0;
	virtual void GetOutgoingMessage(byte *message) =0;

	virtual bool LastMessageProcessed() const =0;
	virtual void ProcessIncomingMessage(const byte *message, unsigned int messageLength) =0;

protected:
	void HandleProtocolError(Exception::ErrorType errorType, const std::string &s) const;
	void CheckAndHandleInvalidState() const;
	void SetValidState(bool valid) {m_validState = valid;}

	RandomNumberGenerator *m_rng;

private:
	bool m_throwOnProtocolError, m_validState;
};

class KeyAgreementSession : public ProtocolSession
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~KeyAgreementSession() {}
#endif

	virtual unsigned int GetAgreedValueLength() const =0;
	virtual void GetAgreedValue(byte *agreedValue) const =0;
};

class PasswordAuthenticatedKeyAgreementSession : public KeyAgreementSession
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~PasswordAuthenticatedKeyAgreementSession() {}
#endif

	void InitializePasswordAuthenticatedKeyAgreementSession(RandomNumberGenerator &rng,
		const byte *myId, unsigned int myIdLength,
		const byte *counterPartyId, unsigned int counterPartyIdLength,
		const byte *passwordOrVerifier, unsigned int passwordOrVerifierLength);
};

class PasswordAuthenticatedKeyAgreementDomain : public KeyAgreementAlgorithm
{
public:
#ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~PasswordAuthenticatedKeyAgreementDomain() {}
#endif

	//! return whether the domain parameters stored in this object are valid
	virtual bool ValidateDomainParameters(RandomNumberGenerator &rng) const
		{return GetCryptoParameters().Validate(rng, 2);}

	virtual unsigned int GetPasswordVerifierLength(const byte *password, unsigned int passwordLength) const =0;
	virtual void GeneratePasswordVerifier(RandomNumberGenerator &rng, const byte *userId, unsigned int userIdLength, const byte *password, unsigned int passwordLength, byte *verifier) const =0;

	enum RoleFlags {CLIENT=1, SERVER=2, INITIATOR=4, RESPONDER=8};

	virtual bool IsValidRole(unsigned int role) =0;
	virtual PasswordAuthenticatedKeyAgreementSession * CreateProtocolSession(unsigned int role) const =0;
};
#endif

//! \brief Exception thrown when an ASN.1 BER decoing error is encountered
class CRYPTOPP_DLL BERDecodeErr : public InvalidArgument
{
public:
	BERDecodeErr() : InvalidArgument("BER decode error") {}
	BERDecodeErr(const std::string &s) : InvalidArgument(s) {}
};

//! \brief Interface for encoding and decoding ASN1 objects
//! \details Each class that derives from ASN1Object should provide a serialization format
//!   that controls subobject layout. Most of the time the serialization format is
//!   taken from a standard, like P1363 or an RFC.
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE ASN1Object
{
public:
	virtual ~ASN1Object() {}

	//! \brief Decode this object from a BufferedTransformation
	//! \param bt BufferedTransformation object
	//! \details Uses Basic Encoding Rules (BER)
	virtual void BERDecode(BufferedTransformation &bt) =0;

	//! \brief Encode this object into a BufferedTransformation
	//! \param bt BufferedTransformation object
	//! \details Uses Distinguished Encoding Rules (DER)
	virtual void DEREncode(BufferedTransformation &bt) const =0;

	//! \brief Encode this object into a BufferedTransformation
	//! \param bt BufferedTransformation object
	//! \details Uses Basic Encoding Rules (BER).
	//! \details This may be useful if DEREncode() would be too inefficient.
	virtual void BEREncode(BufferedTransformation &bt) const {DEREncode(bt);}
};

#ifdef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY
typedef PK_SignatureScheme PK_SignatureSystem;
typedef SimpleKeyAgreementDomain PK_SimpleKeyAgreementDomain;
typedef AuthenticatedKeyAgreementDomain PK_AuthenticatedKeyAgreementDomain;
#endif

NAMESPACE_END

#if CRYPTOPP_MSC_VERSION
# pragma warning(pop)
#endif

#endif
