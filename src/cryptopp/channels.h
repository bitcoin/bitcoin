// channels.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile channels.h
//! \brief Classes for multiple named channels

#ifndef CRYPTOPP_CHANNELS_H
#define CRYPTOPP_CHANNELS_H

#include "cryptlib.h"
#include "simple.h"
#include "smartptr.h"
#include "stdcpp.h"

NAMESPACE_BEGIN(CryptoPP)

#if 0
//! Route input on default channel to different and/or multiple channels based on message sequence number
class MessageSwitch : public Sink
{
public:
	void AddDefaultRoute(BufferedTransformation &destination, const std::string &channel);
	void AddRoute(unsigned int begin, unsigned int end, BufferedTransformation &destination, const std::string &channel);

	void Put(byte inByte);
	void Put(const byte *inString, unsigned int length);

	void Flush(bool completeFlush, int propagation=-1);
	void MessageEnd(int propagation=-1);
	void PutMessageEnd(const byte *inString, unsigned int length, int propagation=-1);
	void MessageSeriesEnd(int propagation=-1);

private:
	typedef std::pair<BufferedTransformation *, std::string> Route;
	struct RangeRoute
	{
		RangeRoute(unsigned int begin, unsigned int end, const Route &route)
			: begin(begin), end(end), route(route) {}
		bool operator<(const RangeRoute &rhs) const {return begin < rhs.begin;}
		unsigned int begin, end;
		Route route;
	};

	typedef std::list<RangeRoute> RouteList;
	typedef std::list<Route> DefaultRouteList;

	RouteList m_routes;
	DefaultRouteList m_defaultRoutes;
	unsigned int m_nCurrentMessage;
};
#endif

class ChannelSwitchTypedefs
{
public:
	typedef std::pair<BufferedTransformation *, std::string> Route;
	typedef std::multimap<std::string, Route> RouteMap;

	typedef std::pair<BufferedTransformation *, value_ptr<std::string> > DefaultRoute;
	typedef std::list<DefaultRoute> DefaultRouteList;

	// SunCC workaround: can't use const_iterator here
	typedef RouteMap::iterator MapIterator;
	typedef DefaultRouteList::iterator ListIterator;
};

class ChannelSwitch;

class ChannelRouteIterator : public ChannelSwitchTypedefs
{
public:
	ChannelRouteIterator(ChannelSwitch &cs) : m_cs(cs), m_useDefault(false) {}

	void Reset(const std::string &channel);
	bool End() const;
	void Next();
	BufferedTransformation & Destination();
	const std::string & Channel();

	ChannelSwitch& m_cs;
	std::string m_channel;
	bool m_useDefault;
	MapIterator m_itMapCurrent, m_itMapEnd;
	ListIterator m_itListCurrent, m_itListEnd;

protected:
	// Hide this to see if we break something...
	ChannelRouteIterator();
};

//! Route input to different and/or multiple channels based on channel ID
class CRYPTOPP_DLL ChannelSwitch : public Multichannel<Sink>, public ChannelSwitchTypedefs
{
public:
	ChannelSwitch() : m_it(*this), m_blocked(false) {}
	ChannelSwitch(BufferedTransformation &destination) : m_it(*this), m_blocked(false)
	{
		AddDefaultRoute(destination);
	}
	ChannelSwitch(BufferedTransformation &destination, const std::string &outChannel) : m_it(*this), m_blocked(false)
	{
		AddDefaultRoute(destination, outChannel);
	}

	void IsolatedInitialize(const NameValuePairs &parameters=g_nullNameValuePairs);

	size_t ChannelPut2(const std::string &channel, const byte *begin, size_t length, int messageEnd, bool blocking);
	size_t ChannelPutModifiable2(const std::string &channel, byte *begin, size_t length, int messageEnd, bool blocking);

	bool ChannelFlush(const std::string &channel, bool completeFlush, int propagation=-1, bool blocking=true);
	bool ChannelMessageSeriesEnd(const std::string &channel, int propagation=-1, bool blocking=true);

	byte * ChannelCreatePutSpace(const std::string &channel, size_t &size);

	void AddDefaultRoute(BufferedTransformation &destination);
	void RemoveDefaultRoute(BufferedTransformation &destination);
	void AddDefaultRoute(BufferedTransformation &destination, const std::string &outChannel);
	void RemoveDefaultRoute(BufferedTransformation &destination, const std::string &outChannel);
	void AddRoute(const std::string &inChannel, BufferedTransformation &destination, const std::string &outChannel);
	void RemoveRoute(const std::string &inChannel, BufferedTransformation &destination, const std::string &outChannel);

private:
	RouteMap m_routeMap;
	DefaultRouteList m_defaultRoutes;

	ChannelRouteIterator m_it;
	bool m_blocked;

	friend class ChannelRouteIterator;
};

NAMESPACE_END

#endif
