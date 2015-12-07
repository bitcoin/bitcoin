// network.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#include "network.h"
#include "wait.h"

#define CRYPTOPP_TRACE_NETWORK 0

NAMESPACE_BEGIN(CryptoPP)

#ifdef HIGHRES_TIMER_AVAILABLE

lword LimitedBandwidth::ComputeCurrentTransceiveLimit()
{
	if (!m_maxBytesPerSecond)
		return ULONG_MAX;

	const double curTime = GetCurTimeAndCleanUp();
	CRYPTOPP_UNUSED(curTime);

	lword total = 0;
	for (OpQueue::size_type i=0; i!=m_ops.size(); ++i)
		total += m_ops[i].second;
	return SaturatingSubtract(m_maxBytesPerSecond, total);
}

double LimitedBandwidth::TimeToNextTransceive()
{
	if (!m_maxBytesPerSecond)
		return 0;

	if (!m_nextTransceiveTime)
		ComputeNextTransceiveTime();

	return SaturatingSubtract(m_nextTransceiveTime, m_timer.ElapsedTimeAsDouble());
}

void LimitedBandwidth::NoteTransceive(lword size)
{
	if (m_maxBytesPerSecond)
	{
		double curTime = GetCurTimeAndCleanUp();
		m_ops.push_back(std::make_pair(curTime, size));
		m_nextTransceiveTime = 0;
	}
}

void LimitedBandwidth::ComputeNextTransceiveTime()
{
	double curTime = GetCurTimeAndCleanUp();
	lword total = 0;
	for (unsigned int i=0; i!=m_ops.size(); ++i)
		total += m_ops[i].second;
	m_nextTransceiveTime =
		(total < m_maxBytesPerSecond) ? curTime : m_ops.front().first + 1000;
}

double LimitedBandwidth::GetCurTimeAndCleanUp()
{
	if (!m_maxBytesPerSecond)
		return 0;

	double curTime = m_timer.ElapsedTimeAsDouble();
	while (m_ops.size() && (m_ops.front().first + 1000 < curTime))
		m_ops.pop_front();
	return curTime;
}

void LimitedBandwidth::GetWaitObjects(WaitObjectContainer &container, const CallStack &callStack)
{
	double nextTransceiveTime = TimeToNextTransceive();
	if (nextTransceiveTime)
		container.ScheduleEvent(nextTransceiveTime, CallStack("LimitedBandwidth::GetWaitObjects()", &callStack));
}

// *************************************************************

size_t NonblockingSource::GeneralPump2(
	lword& byteCount, bool blockingOutput,
	unsigned long maxTime, bool checkDelimiter, byte delimiter)
{
	m_blockedBySpeedLimit = false;

	if (!GetMaxBytesPerSecond())
	{
		size_t ret = DoPump(byteCount, blockingOutput, maxTime, checkDelimiter, delimiter);
		m_doPumpBlocked = (ret != 0);
		return ret;
	}

	bool forever = (maxTime == INFINITE_TIME);
	unsigned long timeToGo = maxTime;
	Timer timer(Timer::MILLISECONDS, forever);
	lword maxSize = byteCount;
	byteCount = 0;

	timer.StartTimer();

	while (true)
	{
		lword curMaxSize = UnsignedMin(ComputeCurrentTransceiveLimit(), maxSize - byteCount);

		if (curMaxSize || m_doPumpBlocked)
		{
			if (!forever) timeToGo = SaturatingSubtract(maxTime, timer.ElapsedTime());
			size_t ret = DoPump(curMaxSize, blockingOutput, timeToGo, checkDelimiter, delimiter);
			m_doPumpBlocked = (ret != 0);
			if (curMaxSize)
			{
				NoteTransceive(curMaxSize);
				byteCount += curMaxSize;
			}
			if (ret)
				return ret;
		}

		if (maxSize != ULONG_MAX && byteCount >= maxSize)
			break;

		if (!forever)
		{
			timeToGo = SaturatingSubtract(maxTime, timer.ElapsedTime());
			if (!timeToGo)
				break;
		}

		double waitTime = TimeToNextTransceive();
		if (!forever && waitTime > timeToGo)
		{
			m_blockedBySpeedLimit = true;
			break;
		}

		WaitObjectContainer container;
		LimitedBandwidth::GetWaitObjects(container, CallStack("NonblockingSource::GeneralPump2() - speed limit", 0));
		container.Wait((unsigned long)waitTime);
	}

	return 0;
}

size_t NonblockingSource::PumpMessages2(unsigned int &messageCount, bool blocking)
{
	if (messageCount == 0)
		return 0;

	messageCount = 0;

	lword byteCount;
	do {
		byteCount = LWORD_MAX;
		RETURN_IF_NONZERO(Pump2(byteCount, blocking));
	} while(byteCount == LWORD_MAX);

	if (!m_messageEndSent && SourceExhausted())
	{
		RETURN_IF_NONZERO(AttachedTransformation()->Put2(NULL, 0, GetAutoSignalPropagation(), true));
		m_messageEndSent = true;
		messageCount = 1;
	}
	return 0;
}

lword NonblockingSink::TimedFlush(unsigned long maxTime, size_t targetSize)
{
	m_blockedBySpeedLimit = false;

	size_t curBufSize = GetCurrentBufferSize();
	if (curBufSize <= targetSize && (targetSize || !EofPending()))
		return 0;

	if (!GetMaxBytesPerSecond())
		return DoFlush(maxTime, targetSize);

	bool forever = (maxTime == INFINITE_TIME);
	unsigned long timeToGo = maxTime;
	Timer timer(Timer::MILLISECONDS, forever);
	lword totalFlushed = 0;

	timer.StartTimer();

	while (true)
	{	
		size_t flushSize = UnsignedMin(curBufSize - targetSize, ComputeCurrentTransceiveLimit());
		if (flushSize || EofPending())
		{
			if (!forever) timeToGo = SaturatingSubtract(maxTime, timer.ElapsedTime());
			size_t ret = (size_t)DoFlush(timeToGo, curBufSize - flushSize);
			if (ret)
			{
				NoteTransceive(ret);
				curBufSize -= ret;
				totalFlushed += ret;
			}
		}

		if (curBufSize <= targetSize && (targetSize || !EofPending()))
			break;

		if (!forever)
		{
			timeToGo = SaturatingSubtract(maxTime, timer.ElapsedTime());
			if (!timeToGo)
				break;
		}

		double waitTime = TimeToNextTransceive();
		if (!forever && waitTime > timeToGo)
		{
			m_blockedBySpeedLimit = true;
			break;
		}

		WaitObjectContainer container;
		LimitedBandwidth::GetWaitObjects(container, CallStack("NonblockingSink::TimedFlush() - speed limit", 0));
		container.Wait((unsigned long)waitTime);
	}

	return totalFlushed;
}

bool NonblockingSink::IsolatedFlush(bool hardFlush, bool blocking)
{
	TimedFlush(blocking ? INFINITE_TIME : 0);
	return hardFlush && (!!GetCurrentBufferSize() || EofPending());
}

// *************************************************************

NetworkSource::NetworkSource(BufferedTransformation *attachment)
	: NonblockingSource(attachment), m_buf(1024*16)
	,  m_putSize(0), m_dataBegin(0), m_dataEnd(0)
	,  m_waitingForResult(false), m_outputBlocked(false)
{
}

unsigned int NetworkSource::GetMaxWaitObjectCount() const
{
	return LimitedBandwidth::GetMaxWaitObjectCount()
		+ GetReceiver().GetMaxWaitObjectCount()
		+ AttachedTransformation()->GetMaxWaitObjectCount();
}

void NetworkSource::GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack)
{
	if (BlockedBySpeedLimit())
		LimitedBandwidth::GetWaitObjects(container, CallStack("NetworkSource::GetWaitObjects() - speed limit", &callStack));
	else if (!m_outputBlocked)
	{
		if (m_dataBegin == m_dataEnd)
			AccessReceiver().GetWaitObjects(container, CallStack("NetworkSource::GetWaitObjects() - no data", &callStack)); 
		else
			container.SetNoWait(CallStack("NetworkSource::GetWaitObjects() - have data", &callStack));
	}

	AttachedTransformation()->GetWaitObjects(container, CallStack("NetworkSource::GetWaitObjects() - attachment", &callStack));
}

size_t NetworkSource::DoPump(lword &byteCount, bool blockingOutput, unsigned long maxTime, bool checkDelimiter, byte delimiter)
{
	NetworkReceiver &receiver = AccessReceiver();

	lword maxSize = byteCount;
	byteCount = 0;
	bool forever = maxTime == INFINITE_TIME;
	Timer timer(Timer::MILLISECONDS, forever);
	BufferedTransformation *t = AttachedTransformation();

	if (m_outputBlocked)
		goto DoOutput;

	while (true)
	{
		if (m_dataBegin == m_dataEnd)
		{
			if (receiver.EofReceived())
				break;

			if (m_waitingForResult)
			{
				if (receiver.MustWaitForResult() &&
					!receiver.Wait(SaturatingSubtract(maxTime, timer.ElapsedTime()),
						CallStack("NetworkSource::DoPump() - wait receive result", 0)))
					break;

				unsigned int recvResult = receiver.GetReceiveResult();
#if CRYPTOPP_TRACE_NETWORK
				OutputDebugString((IntToString((unsigned int)this) + ": Received " + IntToString(recvResult) + " bytes\n").c_str());
#endif
				m_dataEnd += recvResult;
				m_waitingForResult = false;

				if (!receiver.MustWaitToReceive() && !receiver.EofReceived() && m_dataEnd != m_buf.size())
					goto ReceiveNoWait;
			}
			else
			{
				m_dataEnd = m_dataBegin = 0;

				if (receiver.MustWaitToReceive())
				{
					if (!receiver.Wait(SaturatingSubtract(maxTime, timer.ElapsedTime()),
							CallStack("NetworkSource::DoPump() - wait receive", 0)))
						break;

					receiver.Receive(m_buf+m_dataEnd, m_buf.size()-m_dataEnd);
					m_waitingForResult = true;
				}
				else
				{
ReceiveNoWait:
					m_waitingForResult = true;
					// call Receive repeatedly as long as data is immediately available,
					// because some receivers tend to return data in small pieces
#if CRYPTOPP_TRACE_NETWORK
					OutputDebugString((IntToString((unsigned int)this) + ": Receiving " + IntToString(m_buf.size()-m_dataEnd) + " bytes\n").c_str());
#endif
					while (receiver.Receive(m_buf+m_dataEnd, m_buf.size()-m_dataEnd))
					{
						unsigned int recvResult = receiver.GetReceiveResult();
#if CRYPTOPP_TRACE_NETWORK
						OutputDebugString((IntToString((unsigned int)this) + ": Received " + IntToString(recvResult) + " bytes\n").c_str());
#endif
						m_dataEnd += recvResult;
						if (receiver.EofReceived() || m_dataEnd > m_buf.size() /2)
						{
							m_waitingForResult = false;
							break;
						}
					}
				}
			}
		}
		else
		{
			m_putSize = UnsignedMin(m_dataEnd - m_dataBegin, maxSize - byteCount);

			if (checkDelimiter)
				m_putSize = std::find(m_buf+m_dataBegin, m_buf+m_dataBegin+m_putSize, delimiter) - (m_buf+m_dataBegin);

DoOutput:
			size_t result = t->PutModifiable2(m_buf+m_dataBegin, m_putSize, 0, forever || blockingOutput);
			if (result)
			{
				if (t->Wait(SaturatingSubtract(maxTime, timer.ElapsedTime()),
						CallStack("NetworkSource::DoPump() - wait attachment", 0)))
					goto DoOutput;
				else
				{
					m_outputBlocked = true;
					return result;
				}
			}
			m_outputBlocked = false;

			byteCount += m_putSize;
			m_dataBegin += m_putSize;
			if (checkDelimiter && m_dataBegin < m_dataEnd && m_buf[m_dataBegin] == delimiter)
				break;
			if (maxSize != ULONG_MAX && byteCount == maxSize)
				break;
			// once time limit is reached, return even if there is more data waiting
			// but make 0 a special case so caller can request a large amount of data to be
			// pumped as long as it is immediately available
			if (maxTime > 0 && timer.ElapsedTime() > maxTime)
				break;
		}
	}

	return 0;
}

// *************************************************************

NetworkSink::NetworkSink(unsigned int maxBufferSize, unsigned int autoFlushBound)
	: m_maxBufferSize(maxBufferSize), m_autoFlushBound(autoFlushBound)
	, m_needSendResult(false), m_wasBlocked(false), m_eofState(EOF_NONE)
	, m_buffer(STDMIN(16U*1024U+256, maxBufferSize)), m_skipBytes(0) 
	, m_speedTimer(Timer::MILLISECONDS), m_byteCountSinceLastTimerReset(0)
	, m_currentSpeed(0), m_maxObservedSpeed(0)
{
}

float NetworkSink::ComputeCurrentSpeed()
{
	if (m_speedTimer.ElapsedTime() > 1000)
	{
		m_currentSpeed = m_byteCountSinceLastTimerReset * 1000 / m_speedTimer.ElapsedTime();
		m_maxObservedSpeed = STDMAX(m_currentSpeed, m_maxObservedSpeed * 0.98f);
		m_byteCountSinceLastTimerReset = 0;
		m_speedTimer.StartTimer();
//		OutputDebugString(("max speed: " + IntToString((int)m_maxObservedSpeed) + " current speed: " + IntToString((int)m_currentSpeed) + "\n").c_str());
	}
	return m_currentSpeed;
}

float NetworkSink::GetMaxObservedSpeed() const
{
	lword m = GetMaxBytesPerSecond();
	return m ? STDMIN(m_maxObservedSpeed, float(CRYPTOPP_VC6_INT64 m)) : m_maxObservedSpeed;
}

unsigned int NetworkSink::GetMaxWaitObjectCount() const
{
	return LimitedBandwidth::GetMaxWaitObjectCount() + GetSender().GetMaxWaitObjectCount();
}

void NetworkSink::GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack)
{
	if (BlockedBySpeedLimit())
		LimitedBandwidth::GetWaitObjects(container, CallStack("NetworkSink::GetWaitObjects() - speed limit", &callStack));
	else if (m_wasBlocked)
		AccessSender().GetWaitObjects(container, CallStack("NetworkSink::GetWaitObjects() - was blocked", &callStack));
	else if (!m_buffer.IsEmpty())
		AccessSender().GetWaitObjects(container, CallStack("NetworkSink::GetWaitObjects() - buffer not empty", &callStack));
	else if (EofPending())
		AccessSender().GetWaitObjects(container, CallStack("NetworkSink::GetWaitObjects() - EOF pending", &callStack));
}

size_t NetworkSink::Put2(const byte *inString, size_t length, int messageEnd, bool blocking)
{
	if (m_eofState == EOF_DONE)
	{
		if (length || messageEnd)
			throw Exception(Exception::OTHER_ERROR, "NetworkSink::Put2() being called after EOF had been sent");

		return 0;
	}

	if (m_eofState > EOF_NONE)
		goto EofSite;

	{
		if (m_skipBytes)
		{
			assert(length >= m_skipBytes);
			inString += m_skipBytes;
			length -= m_skipBytes;
		}

		m_buffer.Put(inString, length);

		if (!blocking || m_buffer.CurrentSize() > m_autoFlushBound)
			TimedFlush(0, 0);

		size_t targetSize = messageEnd ? 0 : m_maxBufferSize;
		if (blocking)
			TimedFlush(INFINITE_TIME, targetSize);

		if (m_buffer.CurrentSize() > targetSize)
		{
			assert(!blocking);
			m_wasBlocked = true;
			m_skipBytes += length;
			size_t blockedBytes = UnsignedMin(length, m_buffer.CurrentSize() - targetSize);
			return STDMAX<size_t>(blockedBytes, 1);
		}

		m_wasBlocked = false;
		m_skipBytes = 0;
	}

	if (messageEnd)
	{
		m_eofState = EOF_PENDING_SEND;

	EofSite:
		TimedFlush(blocking ? INFINITE_TIME : 0, 0);
		if (m_eofState != EOF_DONE)
			return 1;
	}

	return 0;
}

lword NetworkSink::DoFlush(unsigned long maxTime, size_t targetSize)
{
	NetworkSender &sender = AccessSender();

	bool forever = maxTime == INFINITE_TIME;
	Timer timer(Timer::MILLISECONDS, forever);
	unsigned int totalFlushSize = 0;

	while (true)
	{
		if (m_buffer.CurrentSize() <= targetSize)
			break;
		
		if (m_needSendResult)
		{
			if (sender.MustWaitForResult() &&
				!sender.Wait(SaturatingSubtract(maxTime, timer.ElapsedTime()),
					CallStack("NetworkSink::DoFlush() - wait send result", 0)))
				break;

			unsigned int sendResult = sender.GetSendResult();
#if CRYPTOPP_TRACE_NETWORK
			OutputDebugString((IntToString((unsigned int)this) + ": Sent " + IntToString(sendResult) + " bytes\n").c_str());
#endif
			m_buffer.Skip(sendResult);
			totalFlushSize += sendResult;
			m_needSendResult = false;

			if (!m_buffer.AnyRetrievable())
				break;
		}

		unsigned long timeOut = maxTime ? SaturatingSubtract(maxTime, timer.ElapsedTime()) : 0;
		if (sender.MustWaitToSend() && !sender.Wait(timeOut, CallStack("NetworkSink::DoFlush() - wait send", 0)))
			break;

		size_t contiguousSize = 0;
		const byte *block = m_buffer.Spy(contiguousSize);

#if CRYPTOPP_TRACE_NETWORK
		OutputDebugString((IntToString((unsigned int)this) + ": Sending " + IntToString(contiguousSize) + " bytes\n").c_str());
#endif
		sender.Send(block, contiguousSize);
		m_needSendResult = true;

		if (maxTime > 0 && timeOut == 0)
			break;	// once time limit is reached, return even if there is more data waiting
	}

	m_byteCountSinceLastTimerReset += totalFlushSize;
	ComputeCurrentSpeed();
	
	if (m_buffer.IsEmpty() && !m_needSendResult)
	{
		if (m_eofState == EOF_PENDING_SEND)
		{
			sender.SendEof();
			m_eofState = sender.MustWaitForEof() ? EOF_PENDING_DELIVERY : EOF_DONE;
		}

		while (m_eofState == EOF_PENDING_DELIVERY)
		{
			unsigned long timeOut = maxTime ? SaturatingSubtract(maxTime, timer.ElapsedTime()) : 0;
			if (!sender.Wait(timeOut, CallStack("NetworkSink::DoFlush() - wait EOF", 0)))
				break;

			if (sender.EofSent())
				m_eofState = EOF_DONE;
		}
	}

	return totalFlushSize;
}

#endif	// #ifdef HIGHRES_TIMER_AVAILABLE

NAMESPACE_END
