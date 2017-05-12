// ida.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "config.h"

#include "ida.h"
#include "algebra.h"
#include "gf2_32.h"
#include "polynomi.h"
#include "polynomi.cpp"

#include <functional>

ANONYMOUS_NAMESPACE_BEGIN
static const CryptoPP::GF2_32 field;
NAMESPACE_END

NAMESPACE_BEGIN(CryptoPP)

void RawIDA::IsolatedInitialize(const NameValuePairs &parameters)
{
	if (!parameters.GetIntValue("RecoveryThreshold", m_threshold))
		throw InvalidArgument("RawIDA: missing RecoveryThreshold argument");

	CRYPTOPP_ASSERT(m_threshold > 0);
	if (m_threshold <= 0)
		throw InvalidArgument("RawIDA: RecoveryThreshold must be greater than 0");

	m_lastMapPosition = m_inputChannelMap.end();
	m_channelsReady = 0;
	m_channelsFinished = 0;
	m_w.New(m_threshold);
	m_y.New(m_threshold);
	m_inputQueues.reserve(m_threshold);

	m_outputChannelIds.clear();
	m_outputChannelIdStrings.clear();
	m_outputQueues.clear();

	word32 outputChannelID;
	if (parameters.GetValue("OutputChannelID", outputChannelID))
		AddOutputChannel(outputChannelID);
	else
	{
		int nShares = parameters.GetIntValueWithDefault("NumberOfShares", m_threshold);
		CRYPTOPP_ASSERT(nShares > 0);
		if (nShares <= 0) {nShares = m_threshold;}
		for (unsigned int i=0; i< (unsigned int)(nShares); i++)
			AddOutputChannel(i);
	}
}

unsigned int RawIDA::InsertInputChannel(word32 channelId)
{
	if (m_lastMapPosition != m_inputChannelMap.end())
	{
		if (m_lastMapPosition->first == channelId)
			goto skipFind;
		++m_lastMapPosition;
		if (m_lastMapPosition != m_inputChannelMap.end() && m_lastMapPosition->first == channelId)
			goto skipFind;
	}
	m_lastMapPosition = m_inputChannelMap.find(channelId);

skipFind:
	if (m_lastMapPosition == m_inputChannelMap.end())
	{
		if (m_inputChannelIds.size() == size_t(m_threshold))
			return m_threshold;

		m_lastMapPosition = m_inputChannelMap.insert(InputChannelMap::value_type(channelId, (unsigned int)m_inputChannelIds.size())).first;
		m_inputQueues.push_back(MessageQueue());
		m_inputChannelIds.push_back(channelId);

		if (m_inputChannelIds.size() == size_t(m_threshold))
			PrepareInterpolation();
	}
	return m_lastMapPosition->second;
}

unsigned int RawIDA::LookupInputChannel(word32 channelId) const
{
	std::map<word32, unsigned int>::const_iterator it = m_inputChannelMap.find(channelId);
	if (it == m_inputChannelMap.end())
		return m_threshold;
	else
		return it->second;
}

void RawIDA::ChannelData(word32 channelId, const byte *inString, size_t length, bool messageEnd)
{
	int i = InsertInputChannel(channelId);
	if (i < m_threshold)
	{
		lword size = m_inputQueues[i].MaxRetrievable();
		m_inputQueues[i].Put(inString, length);
		if (size < 4 && size + length >= 4)
		{
			m_channelsReady++;
			if (m_channelsReady == size_t(m_threshold))
				ProcessInputQueues();
		}

		if (messageEnd)
		{
			m_inputQueues[i].MessageEnd();
			if (m_inputQueues[i].NumberOfMessages() == 1)
			{
				m_channelsFinished++;
				if (m_channelsFinished == size_t(m_threshold))
				{
					m_channelsReady = 0;
					for (i=0; i<m_threshold; i++)
						m_channelsReady += m_inputQueues[i].AnyRetrievable();
					ProcessInputQueues();
				}
			}
		}
	}
}

lword RawIDA::InputBuffered(word32 channelId) const
{
	int i = LookupInputChannel(channelId);
	return i < m_threshold ? m_inputQueues[i].MaxRetrievable() : 0;
}

void RawIDA::ComputeV(unsigned int i)
{
	if (i >= m_v.size())
	{
		m_v.resize(i+1);
		m_outputToInput.resize(i+1);
	}

	m_outputToInput[i] = LookupInputChannel(m_outputChannelIds[i]);
	if (m_outputToInput[i] == size_t(m_threshold) && i * size_t(m_threshold) <= 1000*1000)
	{
		m_v[i].resize(m_threshold);
		PrepareBulkPolynomialInterpolationAt(field, m_v[i].begin(), m_outputChannelIds[i], &(m_inputChannelIds[0]), m_w.begin(), m_threshold);
	}
}

void RawIDA::AddOutputChannel(word32 channelId)
{
	m_outputChannelIds.push_back(channelId);
	m_outputChannelIdStrings.push_back(WordToString(channelId));
	m_outputQueues.push_back(ByteQueue());
	if (m_inputChannelIds.size() == size_t(m_threshold))
		ComputeV((unsigned int)m_outputChannelIds.size() - 1);
}

void RawIDA::PrepareInterpolation()
{
	CRYPTOPP_ASSERT(m_inputChannelIds.size() == size_t(m_threshold));
	PrepareBulkPolynomialInterpolation(field, m_w.begin(), &(m_inputChannelIds[0]), (unsigned int)(m_threshold));
	for (unsigned int i=0; i<m_outputChannelIds.size(); i++)
		ComputeV(i);
}

void RawIDA::ProcessInputQueues()
{
	bool finished = (m_channelsFinished == size_t(m_threshold));
	unsigned int i;

	while (finished ? m_channelsReady > 0 : m_channelsReady == size_t(m_threshold))
	{
		m_channelsReady = 0;
		for (i=0; i<size_t(m_threshold); i++)
		{
			MessageQueue &queue = m_inputQueues[i];
			queue.GetWord32(m_y[i]);

			if (finished)
				m_channelsReady += queue.AnyRetrievable();
			else
				m_channelsReady += queue.NumberOfMessages() > 0 || queue.MaxRetrievable() >= 4;
		}

		for (i=0; (unsigned int)i<m_outputChannelIds.size(); i++)
		{
			if (m_outputToInput[i] != size_t(m_threshold))
				m_outputQueues[i].PutWord32(m_y[m_outputToInput[i]]);
			else if (m_v[i].size() == size_t(m_threshold))
				m_outputQueues[i].PutWord32(BulkPolynomialInterpolateAt(field, m_y.begin(), m_v[i].begin(), m_threshold));
			else
			{
				m_u.resize(m_threshold);
				PrepareBulkPolynomialInterpolationAt(field, m_u.begin(), m_outputChannelIds[i], &(m_inputChannelIds[0]), m_w.begin(), m_threshold);
				m_outputQueues[i].PutWord32(BulkPolynomialInterpolateAt(field, m_y.begin(), m_u.begin(), m_threshold));
			}
		}
	}

	if (m_outputChannelIds.size() > 0 && m_outputQueues[0].AnyRetrievable())
		FlushOutputQueues();

	if (finished)
	{
		OutputMessageEnds();

		m_channelsReady = 0;
		m_channelsFinished = 0;
		m_v.clear();

		std::vector<MessageQueue> inputQueues;
		std::vector<word32> inputChannelIds;

		inputQueues.swap(m_inputQueues);
		inputChannelIds.swap(m_inputChannelIds);
		m_inputChannelMap.clear();
		m_lastMapPosition = m_inputChannelMap.end();

		for (i=0; i<size_t(m_threshold); i++)
		{
			inputQueues[i].GetNextMessage();
			inputQueues[i].TransferAllTo(*AttachedTransformation(), WordToString(inputChannelIds[i]));
		}
	}
}

void RawIDA::FlushOutputQueues()
{
	for (unsigned int i=0; i<m_outputChannelIds.size(); i++)
		m_outputQueues[i].TransferAllTo(*AttachedTransformation(), m_outputChannelIdStrings[i]);
}

void RawIDA::OutputMessageEnds()
{
	if (GetAutoSignalPropagation() != 0)
	{
		for (unsigned int i=0; i<m_outputChannelIds.size(); i++)
			AttachedTransformation()->ChannelMessageEnd(m_outputChannelIdStrings[i], GetAutoSignalPropagation()-1);
	}
}

// ****************************************************************

void SecretSharing::IsolatedInitialize(const NameValuePairs &parameters)
{
	m_pad = parameters.GetValueWithDefault("AddPadding", true);
	m_ida.IsolatedInitialize(parameters);
}

size_t SecretSharing::Put2(const byte *begin, size_t length, int messageEnd, bool blocking)
{
	if (!blocking)
		throw BlockingInputOnly("SecretSharing");

	SecByteBlock buf(UnsignedMin(256, length));
	unsigned int threshold = m_ida.GetThreshold();
	while (length > 0)
	{
		size_t len = STDMIN(length, buf.size());
		m_ida.ChannelData(0xffffffff, begin, len, false);
		for (unsigned int i=0; i<threshold-1; i++)
		{
			m_rng.GenerateBlock(buf, len);
			m_ida.ChannelData(i, buf, len, false);
		}
		length -= len;
		begin += len;
	}

	if (messageEnd)
	{
		m_ida.SetAutoSignalPropagation(messageEnd-1);
		if (m_pad)
		{
			SecretSharing::Put(1);
			while (m_ida.InputBuffered(0xffffffff) > 0)
				SecretSharing::Put(0);
		}
		m_ida.ChannelData(0xffffffff, NULL, 0, true);
		for (unsigned int i=0; i<m_ida.GetThreshold()-1; i++)
			m_ida.ChannelData(i, NULL, 0, true);
	}

	return 0;
}

void SecretRecovery::IsolatedInitialize(const NameValuePairs &parameters)
{
	m_pad = parameters.GetValueWithDefault("RemovePadding", true);
	RawIDA::IsolatedInitialize(CombinedNameValuePairs(parameters, MakeParameters("OutputChannelID", (word32)0xffffffff)));
}

void SecretRecovery::FlushOutputQueues()
{
	if (m_pad)
		m_outputQueues[0].TransferTo(*AttachedTransformation(), m_outputQueues[0].MaxRetrievable()-4);
	else
		m_outputQueues[0].TransferTo(*AttachedTransformation());
}

void SecretRecovery::OutputMessageEnds()
{
	if (m_pad)
	{
		PaddingRemover paddingRemover(new Redirector(*AttachedTransformation()));
		m_outputQueues[0].TransferAllTo(paddingRemover);
	}

	if (GetAutoSignalPropagation() != 0)
		AttachedTransformation()->MessageEnd(GetAutoSignalPropagation()-1);
}

// ****************************************************************

void InformationDispersal::IsolatedInitialize(const NameValuePairs &parameters)
{
	m_nextChannel = 0;
	m_pad = parameters.GetValueWithDefault("AddPadding", true);
	m_ida.IsolatedInitialize(parameters);
}

size_t InformationDispersal::Put2(const byte *begin, size_t length, int messageEnd, bool blocking)
{
	if (!blocking)
		throw BlockingInputOnly("InformationDispersal");

	while (length--)
	{
		m_ida.ChannelData(m_nextChannel, begin, 1, false);
		begin++;
		m_nextChannel++;
		if (m_nextChannel == m_ida.GetThreshold())
			m_nextChannel = 0;
	}

	if (messageEnd)
	{
		m_ida.SetAutoSignalPropagation(messageEnd-1);
		if (m_pad)
			InformationDispersal::Put(1);
		for (word32 i=0; i<m_ida.GetThreshold(); i++)
			m_ida.ChannelData(i, NULL, 0, true);
	}

	return 0;
}

void InformationRecovery::IsolatedInitialize(const NameValuePairs &parameters)
{
	m_pad = parameters.GetValueWithDefault("RemovePadding", true);
	RawIDA::IsolatedInitialize(parameters);
}

void InformationRecovery::FlushOutputQueues()
{
	while (m_outputQueues[0].AnyRetrievable())
	{
		for (unsigned int i=0; i<m_outputChannelIds.size(); i++)
			m_outputQueues[i].TransferTo(m_queue, 1);
	}

	if (m_pad)
		m_queue.TransferTo(*AttachedTransformation(), m_queue.MaxRetrievable()-4*m_threshold);
	else
		m_queue.TransferTo(*AttachedTransformation());
}

void InformationRecovery::OutputMessageEnds()
{
	if (m_pad)
	{
		PaddingRemover paddingRemover(new Redirector(*AttachedTransformation()));
		m_queue.TransferAllTo(paddingRemover);
	}

	if (GetAutoSignalPropagation() != 0)
		AttachedTransformation()->MessageEnd(GetAutoSignalPropagation()-1);
}

size_t PaddingRemover::Put2(const byte *begin, size_t length, int messageEnd, bool blocking)
{
	if (!blocking)
		throw BlockingInputOnly("PaddingRemover");

	const byte *const end = begin + length;

	if (m_possiblePadding)
	{
		size_t len = std::find_if(begin, end, std::bind2nd(std::not_equal_to<byte>(), byte(0))) - begin;
		m_zeroCount += len;
		begin += len;
		if (begin == end)
			return 0;

		AttachedTransformation()->Put(1);
		while (m_zeroCount--)
			AttachedTransformation()->Put(0);
		AttachedTransformation()->Put(*begin++);
		m_possiblePadding = false;
	}

#if defined(_MSC_VER) && (_MSC_VER <= 1300) && !defined(__MWERKS__)
	// VC60 and VC7 workaround: built-in reverse_iterator has two template parameters, Dinkumware only has one
	typedef std::reverse_bidirectional_iterator<const byte *, const byte> RevIt;
#elif defined(_RWSTD_NO_CLASS_PARTIAL_SPEC)
	typedef std::reverse_iterator<const byte *, std::random_access_iterator_tag, const byte> RevIt;
#else
	typedef std::reverse_iterator<const byte *> RevIt;
#endif
	const byte *x = std::find_if(RevIt(end), RevIt(begin), std::bind2nd(std::not_equal_to<byte>(), byte(0))).base();
	if (x != begin && *(x-1) == 1)
	{
		AttachedTransformation()->Put(begin, x-begin-1);
		m_possiblePadding = true;
		m_zeroCount = end - x;
	}
	else
		AttachedTransformation()->Put(begin, end-begin);

	if (messageEnd)
	{
		m_possiblePadding = false;
		Output(0, begin, length, messageEnd, blocking);
	}
	return 0;
}

NAMESPACE_END
