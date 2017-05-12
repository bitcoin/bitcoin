// queue.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile queue.h
//! \brief Classes for an unlimited queue to store bytes

#ifndef CRYPTOPP_QUEUE_H
#define CRYPTOPP_QUEUE_H

#include "cryptlib.h"
#include "simple.h"

NAMESPACE_BEGIN(CryptoPP)

class ByteQueueNode;

//! \class ByteQueue
//! \brief Data structure used to store byte strings
//! \details The queue is implemented as a linked list of byte arrays
class CRYPTOPP_DLL ByteQueue : public Bufferless<BufferedTransformation>
{
public:
	//! \brief Construct a ByteQueue
	//! \param nodeSize the initial node size
	//! \details Internally, ByteQueue uses a ByteQueueNode to store bytes, and \p nodeSize determines the
	//!   size of the ByteQueueNode. A value of 0 indicates the ByteQueueNode should be automatically sized,
	//!   which means a value of 256 is used.
	ByteQueue(size_t nodeSize=0);

	//! \brief Copy construct a ByteQueue
	//! \param copy the other ByteQueue
	ByteQueue(const ByteQueue &copy);
	~ByteQueue();

	lword MaxRetrievable() const
		{return CurrentSize();}
	bool AnyRetrievable() const
		{return !IsEmpty();}

	void IsolatedInitialize(const NameValuePairs &parameters);
	byte * CreatePutSpace(size_t &size);
	size_t Put2(const byte *inString, size_t length, int messageEnd, bool blocking);

	size_t Get(byte &outByte);
	size_t Get(byte *outString, size_t getMax);

	size_t Peek(byte &outByte) const;
	size_t Peek(byte *outString, size_t peekMax) const;

	size_t TransferTo2(BufferedTransformation &target, lword &transferBytes, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true);
	size_t CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end=LWORD_MAX, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true) const;

	// these member functions are not inherited
	void SetNodeSize(size_t nodeSize);

	lword CurrentSize() const;
	bool IsEmpty() const;

	void Clear();

	void Unget(byte inByte);
	void Unget(const byte *inString, size_t length);

	const byte * Spy(size_t &contiguousSize) const;

	void LazyPut(const byte *inString, size_t size);
	void LazyPutModifiable(byte *inString, size_t size);
	void UndoLazyPut(size_t size);
	void FinalizeLazyPut();

	ByteQueue & operator=(const ByteQueue &rhs);
	bool operator==(const ByteQueue &rhs) const;
	bool operator!=(const ByteQueue &rhs) const {return !operator==(rhs);}
	byte operator[](lword i) const;
	void swap(ByteQueue &rhs);

	//! \class Walker
	//! \brief A ByteQueue iterator
	class Walker : public InputRejecting<BufferedTransformation>
	{
	public:
		//! \brief Construct a ByteQueue Walker
		//! \param queue a ByteQueue
		Walker(const ByteQueue &queue)
			: m_queue(queue), m_node(NULL), m_position(0), m_offset(0), m_lazyString(NULL), m_lazyLength(0)
				{Initialize();}

		lword GetCurrentPosition() {return m_position;}

		lword MaxRetrievable() const
			{return m_queue.CurrentSize() - m_position;}

		void IsolatedInitialize(const NameValuePairs &parameters);

		size_t Get(byte &outByte);
		size_t Get(byte *outString, size_t getMax);

		size_t Peek(byte &outByte) const;
		size_t Peek(byte *outString, size_t peekMax) const;

		size_t TransferTo2(BufferedTransformation &target, lword &transferBytes, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true);
		size_t CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end=LWORD_MAX, const std::string &channel=DEFAULT_CHANNEL, bool blocking=true) const;

	private:
		const ByteQueue &m_queue;
		const ByteQueueNode *m_node;
		lword m_position;
		size_t m_offset;
		const byte *m_lazyString;
		size_t m_lazyLength;
	};

	friend class Walker;

private:
	void CleanupUsedNodes();
	void CopyFrom(const ByteQueue &copy);
	void Destroy();

	bool m_autoNodeSize;
	size_t m_nodeSize;
	ByteQueueNode *m_head, *m_tail;
	byte *m_lazyString;
	size_t m_lazyLength;
	bool m_lazyStringModifiable;
};

//! use this to make sure LazyPut is finalized in event of exception
class CRYPTOPP_DLL LazyPutter
{
public:
	LazyPutter(ByteQueue &bq, const byte *inString, size_t size)
		: m_bq(bq) {bq.LazyPut(inString, size);}
	~LazyPutter()
		{try {m_bq.FinalizeLazyPut();} catch(const Exception&) {CRYPTOPP_ASSERT(0);}}
protected:
	LazyPutter(ByteQueue &bq) : m_bq(bq) {}
private:
	ByteQueue &m_bq;
};

//! like LazyPutter, but does a LazyPutModifiable instead
class LazyPutterModifiable : public LazyPutter
{
public:
	LazyPutterModifiable(ByteQueue &bq, byte *inString, size_t size)
		: LazyPutter(bq) {bq.LazyPutModifiable(inString, size);}
};

NAMESPACE_END

#ifndef __BORLANDC__
NAMESPACE_BEGIN(std)
template<> inline void swap(CryptoPP::ByteQueue &a, CryptoPP::ByteQueue &b)
{
	a.swap(b);
}
NAMESPACE_END
#endif

#endif
