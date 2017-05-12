// channels.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "cryptlib.h"
#include "channels.h"

NAMESPACE_BEGIN(CryptoPP)
USING_NAMESPACE(std)

#if 0
void MessageSwitch::AddDefaultRoute(BufferedTransformation &destination, const std::string &channel)
{
	m_defaultRoutes.push_back(Route(&destination, channel));
}

void MessageSwitch::AddRoute(unsigned int begin, unsigned int end, BufferedTransformation &destination, const std::string &channel)
{
	RangeRoute route(begin, end, Route(&destination, channel));
	RouteList::iterator it = upper_bound(m_routes.begin(), m_routes.end(), route);
	m_routes.insert(it, route);
}

/*
class MessageRouteIterator
{
public:
	typedef MessageSwitch::RouteList::const_iterator RouteIterator;
	typedef MessageSwitch::DefaultRouteList::const_iterator DefaultIterator;

	bool m_useDefault;
	RouteIterator m_itRouteCurrent, m_itRouteEnd;
	DefaultIterator m_itDefaultCurrent, m_itDefaultEnd;

	MessageRouteIterator(MessageSwitch &ms, const std::string &channel)
		: m_channel(channel)
	{
		pair<MapIterator, MapIterator> range = cs.m_routeMap.equal_range(channel);
		if (range.first == range.second)
		{
			m_useDefault = true;
			m_itListCurrent = cs.m_defaultRoutes.begin();
			m_itListEnd = cs.m_defaultRoutes.end();
		}
		else
		{
			m_useDefault = false;
			m_itMapCurrent = range.first;
			m_itMapEnd = range.second;
		}
	}

	bool End() const
	{
		return m_useDefault ? m_itListCurrent == m_itListEnd : m_itMapCurrent == m_itMapEnd;
	}

	void Next()
	{
		if (m_useDefault)
			++m_itListCurrent;
		else
			++m_itMapCurrent;
	}

	BufferedTransformation & Destination()
	{
		return m_useDefault ? *m_itListCurrent->first : *m_itMapCurrent->second.first;
	}

	const std::string & Message()
	{
		if (m_useDefault)
			return m_itListCurrent->second.get() ? *m_itListCurrent->second.get() : m_channel;
		else
			return m_itMapCurrent->second.second;
	}
};

void MessageSwitch::Put(byte inByte);
void MessageSwitch::Put(const byte *inString, unsigned int length);

void MessageSwitch::Flush(bool completeFlush, int propagation=-1);
void MessageSwitch::MessageEnd(int propagation=-1);
void MessageSwitch::PutMessageEnd(const byte *inString, unsigned int length, int propagation=-1);
void MessageSwitch::MessageSeriesEnd(int propagation=-1);
*/
#endif


//
// ChannelRouteIterator
//////////////////////////

void ChannelRouteIterator::Reset(const std::string &channel)
{
	m_channel = channel;
	pair<MapIterator, MapIterator> range = m_cs.m_routeMap.equal_range(channel);
	if (range.first == range.second)
	{
		m_useDefault = true;
		m_itListCurrent = m_cs.m_defaultRoutes.begin();
		m_itListEnd = m_cs.m_defaultRoutes.end();
	}
	else
	{
		m_useDefault = false;
		m_itMapCurrent = range.first;
		m_itMapEnd = range.second;
	}
}

bool ChannelRouteIterator::End() const
{
	return m_useDefault ? m_itListCurrent == m_itListEnd : m_itMapCurrent == m_itMapEnd;
}

void ChannelRouteIterator::Next()
{
	if (m_useDefault)
		++m_itListCurrent;
	else
		++m_itMapCurrent;
}

BufferedTransformation & ChannelRouteIterator::Destination()
{
	return m_useDefault ? *m_itListCurrent->first : *m_itMapCurrent->second.first;
}

const std::string & ChannelRouteIterator::Channel()
{
	if (m_useDefault)
		return m_itListCurrent->second.get() ? *m_itListCurrent->second.get() : m_channel;
	else
		return m_itMapCurrent->second.second;
}


//
// ChannelSwitch
///////////////////

size_t ChannelSwitch::ChannelPut2(const std::string &channel, const byte *begin, size_t length, int messageEnd, bool blocking)
{
	if (m_blocked)
	{
		m_blocked = false;
		goto WasBlocked;
	}

	m_it.Reset(channel);

	while (!m_it.End())
	{
WasBlocked:
		if (m_it.Destination().ChannelPut2(m_it.Channel(), begin, length, messageEnd, blocking))
		{
			m_blocked = true;
			return 1;
		}

		m_it.Next();
	}

	return 0;
}

void ChannelSwitch::IsolatedInitialize(const NameValuePairs& parameters)
{
	CRYPTOPP_UNUSED(parameters);
	m_routeMap.clear();
	m_defaultRoutes.clear();
	m_blocked = false;
}

bool ChannelSwitch::ChannelFlush(const std::string &channel, bool completeFlush, int propagation, bool blocking)
{
	if (m_blocked)
	{
		m_blocked = false;
		goto WasBlocked;
	}

	m_it.Reset(channel);

	while (!m_it.End())
	{
	  WasBlocked:
		if (m_it.Destination().ChannelFlush(m_it.Channel(), completeFlush, propagation, blocking))
		{
			m_blocked = true;
			return true;
		}

		m_it.Next();
	}

	return false;
}

bool ChannelSwitch::ChannelMessageSeriesEnd(const std::string &channel, int propagation, bool blocking)
{
	CRYPTOPP_UNUSED(blocking);
	if (m_blocked)
	{
		m_blocked = false;
		goto WasBlocked;
	}

	m_it.Reset(channel);

	while (!m_it.End())
	{
	  WasBlocked:
		if (m_it.Destination().ChannelMessageSeriesEnd(m_it.Channel(), propagation))
		{
			m_blocked = true;
			return true;
		}

		m_it.Next();
	}

	return false;
}

byte * ChannelSwitch::ChannelCreatePutSpace(const std::string &channel, size_t &size)
{
	m_it.Reset(channel);
	if (!m_it.End())
	{
		BufferedTransformation &target = m_it.Destination();
		const std::string &ch = m_it.Channel();
		m_it.Next();
		if (m_it.End())	// there is only one target channel
			return target.ChannelCreatePutSpace(ch, size);
	}
	size = 0;
	return NULL;
}

size_t ChannelSwitch::ChannelPutModifiable2(const std::string &channel, byte *inString, size_t length, int messageEnd, bool blocking)
{
	ChannelRouteIterator it(*this);
	it.Reset(channel);

	if (!it.End())
	{
		BufferedTransformation &target = it.Destination();
		const std::string &targetChannel = it.Channel();
		it.Next();
		if (it.End())	// there is only one target channel
			return target.ChannelPutModifiable2(targetChannel, inString, length, messageEnd, blocking);
	}

	return ChannelPut2(channel, inString, length, messageEnd, blocking);
}

void ChannelSwitch::AddDefaultRoute(BufferedTransformation &destination)
{
	m_defaultRoutes.push_back(DefaultRoute(&destination, value_ptr<std::string>(NULL)));
}

void ChannelSwitch::RemoveDefaultRoute(BufferedTransformation &destination)
{
	for (DefaultRouteList::iterator it = m_defaultRoutes.begin(); it != m_defaultRoutes.end(); ++it)
		if (it->first == &destination && !it->second.get())
		{
			m_defaultRoutes.erase(it);
			break;
		}
}

void ChannelSwitch::AddDefaultRoute(BufferedTransformation &destination, const std::string &outChannel)
{
	m_defaultRoutes.push_back(DefaultRoute(&destination, outChannel));
}

void ChannelSwitch::RemoveDefaultRoute(BufferedTransformation &destination, const std::string &outChannel)
{
	for (DefaultRouteList::iterator it = m_defaultRoutes.begin(); it != m_defaultRoutes.end(); ++it)
		if (it->first == &destination && (it->second.get() && *it->second == outChannel))
		{
			m_defaultRoutes.erase(it);
			break;
		}
}

void ChannelSwitch::AddRoute(const std::string &inChannel, BufferedTransformation &destination, const std::string &outChannel)
{
	m_routeMap.insert(RouteMap::value_type(inChannel, Route(&destination, outChannel)));
}

void ChannelSwitch::RemoveRoute(const std::string &inChannel, BufferedTransformation &destination, const std::string &outChannel)
{
	typedef ChannelSwitch::RouteMap::iterator MapIterator;
	pair<MapIterator, MapIterator> range = m_routeMap.equal_range(inChannel);

	for (MapIterator it = range.first; it != range.second; ++it)
		if (it->second.first == &destination && it->second.second == outChannel)
		{
			m_routeMap.erase(it);
			break;
		}
}

NAMESPACE_END

#endif
