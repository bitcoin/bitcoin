#pragma once
/**
	@file
	@brief check msg
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <stdint.h>
#include <set>
#include <memory.h>

namespace bls_util {

const size_t MSG_SIZE = 32;

struct Msg {
	uint8_t v[MSG_SIZE];
	bool operator<(const Msg& rhs) const
	{
		for (size_t i = 0; i < MSG_SIZE; i++) {
			if (v[i] < rhs.v[i]) return true;
			if (v[i] > rhs.v[i]) return false;
		}
		return false;
	}
	bool operator==(const Msg& rhs) const
	{
		return memcmp(v, rhs.v, MSG_SIZE) == 0;
	}
};

inline bool areAllMsgDifferent(const void *buf, size_t bufSize)
{
	size_t n = bufSize / MSG_SIZE;
	if (bufSize != n * MSG_SIZE) {
		return false;
	}
	const Msg *msg = (const Msg*)buf;
	typedef std::set<Msg> MsgSet;
	MsgSet ms;
	for (size_t i = 0; i < n; i++) {
		std::pair<MsgSet::iterator, bool> ret = ms.insert(msg[i]);
		if (!ret.second) return false;
	}
	return true;
}

} // bls_util

