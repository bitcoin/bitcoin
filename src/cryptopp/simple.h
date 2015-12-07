// simple.h - written and placed in the public domain by Wei Dai

//! \file simple.h
//! \brief Classes providing simple keying interfaces.

#ifndef CRYPTOPP_SIMPLE_H
#define CRYPTOPP_SIMPLE_H

#include "config.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(push)
# pragma warning(disable: 4127 4189)
#endif

#include "cryptlib.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class ClonableImpl
//! \brief Base class for identifying alogorithm
//! \tparam BASE base class from which to derive
//! \tparam DERIVED class which to clone
template <class DERIVED, class BASE>
class CRYPTOPP_NO_VTABLE ClonableImpl : public BASE
{
public:
	Clonable * Clone() const {return new DERIVED(*static_cast<const DERIVED *>(this));}
};

//! \class AlgorithmImpl
//! \brief Base class for identifying alogorithm
//! \tparam BASE an Algorithm derived class
//! \tparam ALGORITHM_INFO an Algorithm derived class
//! \details AlgorithmImpl provides StaticAlgorithmName from the template parameter BASE
template <class BASE, class ALGORITHM_INFO=BASE>
class CRYPTOPP_NO_VTABLE AlgorithmImpl : public BASE
{
public:
	static std::string CRYPTOPP_API StaticAlgorithmName() {return ALGORITHM_INFO::StaticAlgorithmName();}
	std::string AlgorithmName() const {return ALGORITHM_INFO::StaticAlgorithmName();}
};

//! \class InvalidKeyLength
//! \brief Exception thrown when an invalid key length is encountered
class CRYPTOPP_DLL InvalidKeyLength : public InvalidArgument
{
public:
	explicit InvalidKeyLength(const std::string &algorithm, size_t length) : InvalidArgument(algorithm + ": " + IntToString(length) + " is not a valid key length") {}
};

//! \class InvalidRounds
//! \brief Exception thrown when an invalid number of rounds is encountered
class CRYPTOPP_DLL InvalidRounds : public InvalidArgument
{
public:
	explicit InvalidRounds(const std::string &algorithm, unsigned int rounds) : InvalidArgument(algorithm + ": " + IntToString(rounds) + " is not a valid number of rounds") {}
};

// *****************************

//! \class Bufferless
//! \brief Base class for bufferless filters
//! \tparam T the class or type
template <class T>
class CRYPTOPP_NO_VTABLE Bufferless : public T
{
public:
	bool IsolatedFlush(bool hardFlush, bool blocking)
		{CRYPTOPP_UNUSED(hardFlush); CRYPTOPP_UNUSED(blocking); return false;}
};

//! \class Unflushable
//! \brief Base class for unflushable filters
//! \tparam T the class or type
template <class T>
class CRYPTOPP_NO_VTABLE Unflushable : public T
{
public:
	bool Flush(bool completeFlush, int propagation=-1, bool blocking=true)
		{return ChannelFlush(DEFAULT_CHANNEL, completeFlush, propagation, blocking);}
	bool IsolatedFlush(bool hardFlush, bool blocking)
		{CRYPTOPP_UNUSED(hardFlush); CRYPTOPP_UNUSED(blocking); assert(false); return false;}
	bool ChannelFlush(const std::string &channel, bool hardFlush, int propagation=-1, bool blocking=true)
	{
		if (hardFlush && !InputBufferIsEmpty())
			throw CannotFlush("Unflushable<T>: this object has buffered input that cannot be flushed");
		else 
		{
			BufferedTransformation *attached = this->AttachedTransformation();
			return attached && propagation ? attached->ChannelFlush(channel, hardFlush, propagation-1, blocking) : false;
		}
	}

protected:
	virtual bool InputBufferIsEmpty() const {return false;}
};

//! \class InputRejecting
//! \brief Base class for input rejecting filters
//! \tparam T the class or type
//! \details T should be a BufferedTransformation derived class
template <class T>
class CRYPTOPP_NO_VTABLE InputRejecting : public T
{
public:
	struct InputRejected : public NotImplemented
		{InputRejected() : NotImplemented("BufferedTransformation: this object doesn't allow input") {}};

	//!	\name INPUT
	//@{

	//! \brief Input a byte array for processing
	//! \param inString the byte array to process
	//! \param length the size of the string, in bytes
	//! \param messageEnd means how many filters to signal MessageEnd() to, including this one
	//! \param blocking specifies whether the object should block when processing input
	//! \throws InputRejected
	//! \returns the number of bytes that remain in the block (i.e., bytes not processed)
	//! \details Internally, the default implmentation throws InputRejected.
	size_t Put2(const byte *inString, size_t length, int messageEnd, bool blocking)
		{CRYPTOPP_UNUSED(inString); CRYPTOPP_UNUSED(length); CRYPTOPP_UNUSED(messageEnd); CRYPTOPP_UNUSED(blocking); throw InputRejected();}
	//@}

	//!	\name SIGNALS
	//@{
	bool IsolatedFlush(bool hardFlush, bool blocking)
		{CRYPTOPP_UNUSED(hardFlush); CRYPTOPP_UNUSED(blocking); return false;}
	bool IsolatedMessageSeriesEnd(bool blocking)
		{CRYPTOPP_UNUSED(blocking); throw InputRejected();}
	size_t ChannelPut2(const std::string &channel, const byte *inString, size_t length, int messageEnd, bool blocking)
		{CRYPTOPP_UNUSED(channel); CRYPTOPP_UNUSED(inString); CRYPTOPP_UNUSED(length); CRYPTOPP_UNUSED(messageEnd); CRYPTOPP_UNUSED(blocking); throw InputRejected();}
	bool ChannelMessageSeriesEnd(const std::string& channel, int messageEnd, bool blocking)
		{CRYPTOPP_UNUSED(channel); CRYPTOPP_UNUSED(messageEnd); CRYPTOPP_UNUSED(blocking); throw InputRejected();}
	//@}
};

//! \class CustomFlushPropagation
//! \brief Provides interface for custom flush signals
//! \tparam T the class or type
//! \details T should be a BufferedTransformation derived class
template <class T>
class CRYPTOPP_NO_VTABLE CustomFlushPropagation : public T
{
public:
	//!	\name SIGNALS
	//@{
	virtual bool Flush(bool hardFlush, int propagation=-1, bool blocking=true) =0;
	//@}

private:
	bool IsolatedFlush(bool hardFlush, bool blocking)
		{CRYPTOPP_UNUSED(hardFlush); CRYPTOPP_UNUSED(blocking); assert(false); return false;}
};

//! \class CustomSignalPropagation
//! \brief Provides interface for initialization of derived filters
//! \tparam T the class or type
//! \details T should be a BufferedTransformation derived class
template <class T>
class CRYPTOPP_NO_VTABLE CustomSignalPropagation : public CustomFlushPropagation<T>
{
public:
	virtual void Initialize(const NameValuePairs &parameters=g_nullNameValuePairs, int propagation=-1) =0;

private:
	void IsolatedInitialize(const NameValuePairs &parameters)
		{CRYPTOPP_UNUSED(parameters); assert(false);}
};

//! \class Multichannel
//! \brief Provides multiple channels support for custom flush signal processing
//! \tparam T the class or type
//! \details T should be a BufferedTransformation derived class
template <class T>
class CRYPTOPP_NO_VTABLE Multichannel : public CustomFlushPropagation<T>
{
public:
	bool Flush(bool hardFlush, int propagation=-1, bool blocking=true)
		{return this->ChannelFlush(DEFAULT_CHANNEL, hardFlush, propagation, blocking);}
	bool MessageSeriesEnd(int propagation=-1, bool blocking=true)
		{return this->ChannelMessageSeriesEnd(DEFAULT_CHANNEL, propagation, blocking);}
	byte * CreatePutSpace(size_t &size)
		{return this->ChannelCreatePutSpace(DEFAULT_CHANNEL, size);}
	size_t Put2(const byte *inString, size_t length, int messageEnd, bool blocking)
		{return this->ChannelPut2(DEFAULT_CHANNEL, inString, length, messageEnd, blocking);}
	size_t PutModifiable2(byte *inString, size_t length, int messageEnd, bool blocking)
		{return this->ChannelPutModifiable2(DEFAULT_CHANNEL, inString, length, messageEnd, blocking);}

//	void ChannelMessageSeriesEnd(const std::string &channel, int propagation=-1)
//		{PropagateMessageSeriesEnd(propagation, channel);}
	byte * ChannelCreatePutSpace(const std::string &channel, size_t &size)
		{CRYPTOPP_UNUSED(channel); size = 0; return NULL;}
	bool ChannelPutModifiable(const std::string &channel, byte *inString, size_t length)
		{this->ChannelPut(channel, inString, length); return false;}

	virtual size_t ChannelPut2(const std::string &channel, const byte *begin, size_t length, int messageEnd, bool blocking) =0;
	size_t ChannelPutModifiable2(const std::string &channel, byte *begin, size_t length, int messageEnd, bool blocking)
		{return ChannelPut2(channel, begin, length, messageEnd, blocking);}

	virtual bool ChannelFlush(const std::string &channel, bool hardFlush, int propagation=-1, bool blocking=true) =0;
};

//! \class AutoSignaling
//! \brief Provides auto signaling support
//! \tparam T the class or type
//! \details T should be a BufferedTransformation derived class
template <class T>
class CRYPTOPP_NO_VTABLE AutoSignaling : public T
{
public:
	AutoSignaling(int propagation=-1) : m_autoSignalPropagation(propagation) {}

	void SetAutoSignalPropagation(int propagation)
		{m_autoSignalPropagation = propagation;}
	int GetAutoSignalPropagation() const
		{return m_autoSignalPropagation;}

private:
	int m_autoSignalPropagation;
};

//! \class Store
//! \brief Acts as a Source for pre-existing, static data
//! \tparam T the class or type
//! \details A BufferedTransformation that only contains pre-existing data as "output"
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Store : public AutoSignaling<InputRejecting<BufferedTransformation> >
{
public:
	Store() : m_messageEnd(false) {}

	void IsolatedInitialize(const NameValuePairs &parameters)
	{
		m_messageEnd = false;
		StoreInitialize(parameters);
	}

	unsigned int NumberOfMessages() const {return m_messageEnd ? 0 : 1;}
	bool GetNextMessage();
	unsigned int CopyMessagesTo(BufferedTransformation &target, unsigned int count=UINT_MAX, const std::string &channel=DEFAULT_CHANNEL) const;

protected:
	virtual void StoreInitialize(const NameValuePairs &parameters) =0;

	bool m_messageEnd;
};

//! \class Sink
//! \brief Implementation of BufferedTransformation's attachment interface
//! \details Sink is a cornerstone of the Pipeline trinitiy. Data flows from
//!   Sources, through Filters, and then terminates in Sinks. The difference
//!   between a Source and Filter is a Source \a pumps data, while a Filter does
//!   not. The difference between a Filter and a Sink is a Filter allows an
//!   attached transformation, while a Sink does not.
//! \details A Sink doesnot produce any retrievable output.
//! \details See the discussion of BufferedTransformation in cryptlib.h for
//!   more details.
class CRYPTOPP_DLL CRYPTOPP_NO_VTABLE Sink : public BufferedTransformation
{
public:
	size_t TransferTo2(BufferedTransformation &target, lword &transferBytes, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true)
		{CRYPTOPP_UNUSED(target); CRYPTOPP_UNUSED(transferBytes); CRYPTOPP_UNUSED(channel); CRYPTOPP_UNUSED(blocking); transferBytes = 0; return 0;}
	size_t CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end=LWORD_MAX, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true) const
		{CRYPTOPP_UNUSED(target); CRYPTOPP_UNUSED(begin); CRYPTOPP_UNUSED(end); CRYPTOPP_UNUSED(channel); CRYPTOPP_UNUSED(blocking); return 0;}
};

//! \class BitBucket
//! \brief Acts as an input discarding Filter or Sink
//! \tparam T the class or type
//! \details The BitBucket discards all input and returns 0 to the caller
//!   to indicate all data was processed.
class CRYPTOPP_DLL BitBucket : public Bufferless<Sink>
{
public:
	std::string AlgorithmName() const {return "BitBucket";}
	void IsolatedInitialize(const NameValuePairs &params)
		{CRYPTOPP_UNUSED(params);}
	size_t Put2(const byte *inString, size_t length, int messageEnd, bool blocking)
		{CRYPTOPP_UNUSED(inString); CRYPTOPP_UNUSED(length); CRYPTOPP_UNUSED(messageEnd); CRYPTOPP_UNUSED(blocking); return 0;}
};

NAMESPACE_END

#if CRYPTOPP_MSC_VERSION
# pragma warning(pop)
#endif

#endif
