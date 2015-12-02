// ida.h - written and placed in the public domain by Wei Dai

//! \file ida.h
//! \brief Classes for Information Dispersal Algorithm (IDA)

#ifndef CRYPTOPP_IDA_H
#define CRYPTOPP_IDA_H

#include "cryptlib.h"
#include "mqueue.h"
#include "filters.h"
#include "channels.h"
#include "secblock.h"
#include "stdcpp.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

/// base class for secret sharing and information dispersal
class RawIDA : public AutoSignaling<Unflushable<Multichannel<Filter> > >
{
public:
	RawIDA(BufferedTransformation *attachment=NULL)
		: m_threshold (0), m_channelsReady(0), m_channelsFinished(0)
			{Detach(attachment);}

	unsigned int GetThreshold() const {return m_threshold;}
	void AddOutputChannel(word32 channelId);
	void ChannelData(word32 channelId, const byte *inString, size_t length, bool messageEnd);
	lword InputBuffered(word32 channelId) const;

	void IsolatedInitialize(const NameValuePairs &parameters=g_nullNameValuePairs);
	size_t ChannelPut2(const std::string &channel, const byte *begin, size_t length, int messageEnd, bool blocking)
	{
		if (!blocking)
			throw BlockingInputOnly("RawIDA");
		ChannelData(StringToWord<word32>(channel), begin, length, messageEnd != 0);
		return 0;
	}

protected:
	virtual void FlushOutputQueues();
	virtual void OutputMessageEnds();

	unsigned int InsertInputChannel(word32 channelId);
	unsigned int LookupInputChannel(word32 channelId) const;
	void ComputeV(unsigned int);
	void PrepareInterpolation();
	void ProcessInputQueues();

	typedef std::map<word32, unsigned int> InputChannelMap;
	InputChannelMap m_inputChannelMap;
	InputChannelMap::iterator m_lastMapPosition;
	std::vector<MessageQueue> m_inputQueues;
	std::vector<word32> m_inputChannelIds, m_outputChannelIds, m_outputToInput;
	std::vector<std::string> m_outputChannelIdStrings;
	std::vector<ByteQueue> m_outputQueues;
	int m_threshold;
	unsigned int m_channelsReady, m_channelsFinished;
	std::vector<SecBlock<word32> > m_v;
	SecBlock<word32> m_u, m_w, m_y;
};

/// a variant of Shamir's Secret Sharing Algorithm
class SecretSharing : public CustomFlushPropagation<Filter>
{
public:
	SecretSharing(RandomNumberGenerator &rng, int threshold, int nShares, BufferedTransformation *attachment=NULL, bool addPadding=true)
		: m_rng(rng), m_ida(new OutputProxy(*this, true))
	{
		Detach(attachment);
		IsolatedInitialize(MakeParameters("RecoveryThreshold", threshold)("NumberOfShares", nShares)("AddPadding", addPadding));
	}

	void IsolatedInitialize(const NameValuePairs &parameters=g_nullNameValuePairs);
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking);
	bool Flush(bool hardFlush, int propagation=-1, bool blocking=true) {return m_ida.Flush(hardFlush, propagation, blocking);}

protected:
	RandomNumberGenerator &m_rng;
	RawIDA m_ida;
	bool m_pad;
};

/// a variant of Shamir's Secret Sharing Algorithm
class SecretRecovery : public RawIDA
{
public:
	SecretRecovery(int threshold, BufferedTransformation *attachment=NULL, bool removePadding=true)
		: RawIDA(attachment)
		{IsolatedInitialize(MakeParameters("RecoveryThreshold", threshold)("RemovePadding", removePadding));}

	void IsolatedInitialize(const NameValuePairs &parameters=g_nullNameValuePairs);

protected:
	void FlushOutputQueues();
	void OutputMessageEnds();

	bool m_pad;
};

/// a variant of Rabin's Information Dispersal Algorithm
class InformationDispersal : public CustomFlushPropagation<Filter>
{
public:
	InformationDispersal(int threshold, int nShares, BufferedTransformation *attachment=NULL, bool addPadding=true)
		: m_ida(new OutputProxy(*this, true)), m_pad(false), m_nextChannel(0)
	{
		Detach(attachment);
		IsolatedInitialize(MakeParameters("RecoveryThreshold", threshold)("NumberOfShares", nShares)("AddPadding", addPadding));
	}

	void IsolatedInitialize(const NameValuePairs &parameters=g_nullNameValuePairs);
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking);
	bool Flush(bool hardFlush, int propagation=-1, bool blocking=true) {return m_ida.Flush(hardFlush, propagation, blocking);}

protected:
	RawIDA m_ida;
	bool m_pad;
	unsigned int m_nextChannel;
};

/// a variant of Rabin's Information Dispersal Algorithm
class InformationRecovery : public RawIDA
{
public:
	InformationRecovery(int threshold, BufferedTransformation *attachment=NULL, bool removePadding=true)
		: RawIDA(attachment), m_pad(false)
		{IsolatedInitialize(MakeParameters("RecoveryThreshold", threshold)("RemovePadding", removePadding));}

	void IsolatedInitialize(const NameValuePairs &parameters=g_nullNameValuePairs);

protected:
	void FlushOutputQueues();
	void OutputMessageEnds();

	bool m_pad;
	ByteQueue m_queue;
};

class PaddingRemover : public Unflushable<Filter>
{
public:
	PaddingRemover(BufferedTransformation *attachment=NULL)
		: m_possiblePadding(false), m_zeroCount(0) {Detach(attachment);}

	void IsolatedInitialize(const NameValuePairs &parameters)
		{CRYPTOPP_UNUSED(parameters); m_possiblePadding = false;}
	size_t Put2(const byte *begin, size_t length, int messageEnd, bool blocking);

	// GetPossiblePadding() == false at the end of a message indicates incorrect padding
	bool GetPossiblePadding() const {return m_possiblePadding;}

private:
	bool m_possiblePadding;
	lword m_zeroCount;
};

NAMESPACE_END

#endif
