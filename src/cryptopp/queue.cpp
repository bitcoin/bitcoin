// queue.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "queue.h"
#include "filters.h"

NAMESPACE_BEGIN(CryptoPP)

static const unsigned int s_maxAutoNodeSize = 16*1024;

// this class for use by ByteQueue only
class ByteQueueNode
{
public:
	ByteQueueNode(size_t maxSize)
		: buf(maxSize)
	{
		m_head = m_tail = 0;
		next = 0;
	}

	inline size_t MaxSize() const {return buf.size();}

	inline size_t CurrentSize() const
	{
		return m_tail-m_head;
	}

	inline bool UsedUp() const
	{
		return (m_head==MaxSize());
	}

	inline void Clear()
	{
		m_head = m_tail = 0;
	}

	inline size_t Put(const byte *begin, size_t length)
	{
		// Avoid passing NULL to memcpy
		if (!begin || !length) return length;
		size_t l = STDMIN(length, MaxSize()-m_tail);
		if (buf+m_tail != begin)
			memcpy(buf+m_tail, begin, l);
		m_tail += l;
		return l;
	}

	inline size_t Peek(byte &outByte) const
	{
		if (m_tail==m_head)
			return 0;

		outByte=buf[m_head];
		return 1;
	}

	inline size_t Peek(byte *target, size_t copyMax) const
	{
		size_t len = STDMIN(copyMax, m_tail-m_head);
		memcpy(target, buf+m_head, len);
		return len;
	}

	inline size_t CopyTo(BufferedTransformation &target, const std::string &channel=DEFAULT_CHANNEL) const
	{
		size_t len = m_tail-m_head;
		target.ChannelPut(channel, buf+m_head, len);
		return len;
	}

	inline size_t CopyTo(BufferedTransformation &target, size_t copyMax, const std::string &channel=DEFAULT_CHANNEL) const
	{
		size_t len = STDMIN(copyMax, m_tail-m_head);
		target.ChannelPut(channel, buf+m_head, len);
		return len;
	}

	inline size_t Get(byte &outByte)
	{
		size_t len = Peek(outByte);
		m_head += len;
		return len;
	}

	inline size_t Get(byte *outString, size_t getMax)
	{
		size_t len = Peek(outString, getMax);
		m_head += len;
		return len;
	}

	inline size_t TransferTo(BufferedTransformation &target, const std::string &channel=DEFAULT_CHANNEL)
	{
		size_t len = m_tail-m_head;
		target.ChannelPutModifiable(channel, buf+m_head, len);
		m_head = m_tail;
		return len;
	}

	inline size_t TransferTo(BufferedTransformation &target, lword transferMax, const std::string &channel=DEFAULT_CHANNEL)
	{
		size_t len = UnsignedMin(m_tail-m_head, transferMax);
		target.ChannelPutModifiable(channel, buf+m_head, len);
		m_head += len;
		return len;
	}

	inline size_t Skip(size_t skipMax)
	{
		size_t len = STDMIN(skipMax, m_tail-m_head);
		m_head += len;
		return len;
	}

	inline byte operator[](size_t i) const
	{
		return buf[m_head+i];
	}

	ByteQueueNode *next;

	SecByteBlock buf;
	size_t m_head, m_tail;
};

// ********************************************************

ByteQueue::ByteQueue(size_t nodeSize)
	: Bufferless<BufferedTransformation>(), m_autoNodeSize(!nodeSize), m_nodeSize(nodeSize)
	, m_head(NULL), m_tail(NULL), m_lazyString(NULL), m_lazyLength(0), m_lazyStringModifiable(false)
{
	SetNodeSize(nodeSize);
	m_head = m_tail = new ByteQueueNode(m_nodeSize);
}

void ByteQueue::SetNodeSize(size_t nodeSize)
{
	m_autoNodeSize = !nodeSize;
	m_nodeSize = m_autoNodeSize ? 256 : nodeSize;
}

ByteQueue::ByteQueue(const ByteQueue &copy)
	: Bufferless<BufferedTransformation>(copy), m_lazyString(NULL), m_lazyLength(0)
{
	CopyFrom(copy);
}

void ByteQueue::CopyFrom(const ByteQueue &copy)
{
	m_lazyLength = 0;
	m_autoNodeSize = copy.m_autoNodeSize;
	m_nodeSize = copy.m_nodeSize;
	m_head = m_tail = new ByteQueueNode(*copy.m_head);

	for (ByteQueueNode *current=copy.m_head->next; current; current=current->next)
	{
		m_tail->next = new ByteQueueNode(*current);
		m_tail = m_tail->next;
	}

	m_tail->next = NULL;

	Put(copy.m_lazyString, copy.m_lazyLength);
}

ByteQueue::~ByteQueue()
{
	Destroy();
}

void ByteQueue::Destroy()
{
	for (ByteQueueNode *next, *current=m_head; current; current=next)
	{
		next=current->next;
		delete current;
	}
}

void ByteQueue::IsolatedInitialize(const NameValuePairs &parameters)
{
	m_nodeSize = parameters.GetIntValueWithDefault("NodeSize", 256);
	Clear();
}

lword ByteQueue::CurrentSize() const
{
	lword size=0;

	for (ByteQueueNode *current=m_head; current; current=current->next)
		size += current->CurrentSize();

	return size + m_lazyLength;
}

bool ByteQueue::IsEmpty() const
{
	return m_head==m_tail && m_head->CurrentSize()==0 && m_lazyLength==0;
}

void ByteQueue::Clear()
{
	for (ByteQueueNode *next, *current=m_head->next; current; current=next)
	{
		next=current->next;
		delete current;
	}

	m_tail = m_head;
	m_head->Clear();
	m_head->next = NULL;
	m_lazyLength = 0;
}

size_t ByteQueue::Put2(const byte *inString, size_t length, int messageEnd, bool blocking)
{
	CRYPTOPP_UNUSED(messageEnd), CRYPTOPP_UNUSED(blocking);

	if (m_lazyLength > 0)
		FinalizeLazyPut();

	size_t len;
	while ((len=m_tail->Put(inString, length)) < length)
	{
		inString += len;
		length -= len;
		if (m_autoNodeSize && m_nodeSize < s_maxAutoNodeSize)
			do
			{
				m_nodeSize *= 2;
			}
			while (m_nodeSize < length && m_nodeSize < s_maxAutoNodeSize);
		m_tail->next = new ByteQueueNode(STDMAX(m_nodeSize, length));
		m_tail = m_tail->next;
	}

	return 0;
}

void ByteQueue::CleanupUsedNodes()
{
	// Test for m_head due to Enterprise Anlysis finding
	while (m_head && m_head != m_tail && m_head->UsedUp())
	{
		ByteQueueNode *temp=m_head;
		m_head=m_head->next;
		delete temp;
	}

	// Test for m_head due to Enterprise Anlysis finding
	if (m_head && m_head->CurrentSize() == 0)
		m_head->Clear();
}

void ByteQueue::LazyPut(const byte *inString, size_t size)
{
	if (m_lazyLength > 0)
		FinalizeLazyPut();

	if (inString == m_tail->buf+m_tail->m_tail)
		Put(inString, size);
	else
	{
		m_lazyString = const_cast<byte *>(inString);
		m_lazyLength = size;
		m_lazyStringModifiable = false;
	}
}

void ByteQueue::LazyPutModifiable(byte *inString, size_t size)
{
	if (m_lazyLength > 0)
		FinalizeLazyPut();
	m_lazyString = inString;
	m_lazyLength = size;
	m_lazyStringModifiable = true;
}

void ByteQueue::UndoLazyPut(size_t size)
{
	if (m_lazyLength < size)
		throw InvalidArgument("ByteQueue: size specified for UndoLazyPut is too large");

	m_lazyLength -= size;
}

void ByteQueue::FinalizeLazyPut()
{
	size_t len = m_lazyLength;
	m_lazyLength = 0;
	if (len)
		Put(m_lazyString, len);
}

size_t ByteQueue::Get(byte &outByte)
{
	if (m_head->Get(outByte))
	{
		if (m_head->UsedUp())
			CleanupUsedNodes();
		return 1;
	}
	else if (m_lazyLength > 0)
	{
		outByte = *m_lazyString++;
		m_lazyLength--;
		return 1;
	}
	else
		return 0;
}

size_t ByteQueue::Get(byte *outString, size_t getMax)
{
	ArraySink sink(outString, getMax);
	return (size_t)TransferTo(sink, getMax);
}

size_t ByteQueue::Peek(byte &outByte) const
{
	if (m_head->Peek(outByte))
		return 1;
	else if (m_lazyLength > 0)
	{
		outByte = *m_lazyString;
		return 1;
	}
	else
		return 0;
}

size_t ByteQueue::Peek(byte *outString, size_t peekMax) const
{
	ArraySink sink(outString, peekMax);
	return (size_t)CopyTo(sink, peekMax);
}

size_t ByteQueue::TransferTo2(BufferedTransformation &target, lword &transferBytes, const std::string &channel, bool blocking)
{
	if (blocking)
	{
		lword bytesLeft = transferBytes;
		for (ByteQueueNode *current=m_head; bytesLeft && current; current=current->next)
			bytesLeft -= current->TransferTo(target, bytesLeft, channel);
		CleanupUsedNodes();

		size_t len = (size_t)STDMIN(bytesLeft, (lword)m_lazyLength);
		if (len)
		{
			if (m_lazyStringModifiable)
				target.ChannelPutModifiable(channel, m_lazyString, len);
			else
				target.ChannelPut(channel, m_lazyString, len);
			m_lazyString += len;
			m_lazyLength -= len;
			bytesLeft -= len;
		}
		transferBytes -= bytesLeft;
		return 0;
	}
	else
	{
		Walker walker(*this);
		size_t blockedBytes = walker.TransferTo2(target, transferBytes, channel, blocking);
		Skip(transferBytes);
		return blockedBytes;
	}
}

size_t ByteQueue::CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end, const std::string &channel, bool blocking) const
{
	Walker walker(*this);
	walker.Skip(begin);
	lword transferBytes = end-begin;
	size_t blockedBytes = walker.TransferTo2(target, transferBytes, channel, blocking);
	begin += transferBytes;
	return blockedBytes;
}

void ByteQueue::Unget(byte inByte)
{
	Unget(&inByte, 1);
}

void ByteQueue::Unget(const byte *inString, size_t length)
{
	size_t len = STDMIN(length, m_head->m_head);
	length -= len;
	m_head->m_head -= len;
	memcpy(m_head->buf + m_head->m_head, inString + length, len);

	if (length > 0)
	{
		ByteQueueNode *newHead = new ByteQueueNode(length);
		newHead->next = m_head;
		m_head = newHead;
		m_head->Put(inString, length);
	}
}

const byte * ByteQueue::Spy(size_t &contiguousSize) const
{
	contiguousSize = m_head->m_tail - m_head->m_head;
	if (contiguousSize == 0 && m_lazyLength > 0)
	{
		contiguousSize = m_lazyLength;
		return m_lazyString;
	}
	else
		return m_head->buf + m_head->m_head;
}

byte * ByteQueue::CreatePutSpace(size_t &size)
{
	if (m_lazyLength > 0)
		FinalizeLazyPut();

	if (m_tail->m_tail == m_tail->MaxSize())
	{
		m_tail->next = new ByteQueueNode(STDMAX(m_nodeSize, size));
		m_tail = m_tail->next;
	}

	size = m_tail->MaxSize() - m_tail->m_tail;
	return m_tail->buf + m_tail->m_tail;
}

ByteQueue & ByteQueue::operator=(const ByteQueue &rhs)
{
	Destroy();
	CopyFrom(rhs);
	return *this;
}

bool ByteQueue::operator==(const ByteQueue &rhs) const
{
	const lword currentSize = CurrentSize();

	if (currentSize != rhs.CurrentSize())
		return false;

	Walker walker1(*this), walker2(rhs);
	byte b1, b2;

	while (walker1.Get(b1) && walker2.Get(b2))
		if (b1 != b2)
			return false;

	return true;
}

byte ByteQueue::operator[](lword i) const
{
	for (ByteQueueNode *current=m_head; current; current=current->next)
	{
		if (i < current->CurrentSize())
			return (*current)[(size_t)i];
		
		i -= current->CurrentSize();
	}

	assert(i < m_lazyLength);
	return m_lazyString[i];
}

void ByteQueue::swap(ByteQueue &rhs)
{
	std::swap(m_autoNodeSize, rhs.m_autoNodeSize);
	std::swap(m_nodeSize, rhs.m_nodeSize);
	std::swap(m_head, rhs.m_head);
	std::swap(m_tail, rhs.m_tail);
	std::swap(m_lazyString, rhs.m_lazyString);
	std::swap(m_lazyLength, rhs.m_lazyLength);
	std::swap(m_lazyStringModifiable, rhs.m_lazyStringModifiable);
}

// ********************************************************

void ByteQueue::Walker::IsolatedInitialize(const NameValuePairs &parameters)
{
	CRYPTOPP_UNUSED(parameters);

	m_node = m_queue.m_head;
	m_position = 0;
	m_offset = 0;
	m_lazyString = m_queue.m_lazyString;
	m_lazyLength = m_queue.m_lazyLength;
}

size_t ByteQueue::Walker::Get(byte &outByte)
{
	ArraySink sink(&outByte, 1);
	return (size_t)TransferTo(sink, 1);
}

size_t ByteQueue::Walker::Get(byte *outString, size_t getMax)
{
	ArraySink sink(outString, getMax);
	return (size_t)TransferTo(sink, getMax);
}

size_t ByteQueue::Walker::Peek(byte &outByte) const
{
	ArraySink sink(&outByte, 1);
	return (size_t)CopyTo(sink, 1);
}

size_t ByteQueue::Walker::Peek(byte *outString, size_t peekMax) const
{
	ArraySink sink(outString, peekMax);
	return (size_t)CopyTo(sink, peekMax);
}

size_t ByteQueue::Walker::TransferTo2(BufferedTransformation &target, lword &transferBytes, const std::string &channel, bool blocking)
{
	lword bytesLeft = transferBytes;
	size_t blockedBytes = 0;

	while (m_node)
	{
		size_t len = (size_t)STDMIN(bytesLeft, (lword)m_node->CurrentSize()-m_offset);
		blockedBytes = target.ChannelPut2(channel, m_node->buf+m_node->m_head+m_offset, len, 0, blocking);

		if (blockedBytes)
			goto done;

		m_position += len;
		bytesLeft -= len;

		if (!bytesLeft)
		{
			m_offset += len;
			goto done;
		}

		m_node = m_node->next;
		m_offset = 0;
	}

	if (bytesLeft && m_lazyLength)
	{
		size_t len = (size_t)STDMIN(bytesLeft, (lword)m_lazyLength);
		blockedBytes = target.ChannelPut2(channel, m_lazyString, len, 0, blocking);
		if (blockedBytes)
			goto done;

		m_lazyString += len;
		m_lazyLength -= len;
		bytesLeft -= len;
	}

done:
	transferBytes -= bytesLeft;
	return blockedBytes;
}

size_t ByteQueue::Walker::CopyRangeTo2(BufferedTransformation &target, lword &begin, lword end, const std::string &channel, bool blocking) const
{
	Walker walker(*this);
	walker.Skip(begin);
	lword transferBytes = end-begin;
	size_t blockedBytes = walker.TransferTo2(target, transferBytes, channel, blocking);
	begin += transferBytes;
	return blockedBytes;
}

NAMESPACE_END

#endif
