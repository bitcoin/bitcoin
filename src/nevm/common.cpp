/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Common.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include <nevm/common.h>
#include <nevm/exceptions.h>
//#include <Log.h>
//#include <BuildInfo.h>
using namespace std;
using namespace dev;

namespace dev
{

bytes const NullBytes;
u256 const Invalid256 = ~(u256)0;
std::string const EmptyString;

//void InvariantChecker::checkInvariants(HasInvariants const* _this, char const* _fn, char const* _file, int _line, bool _pre)
//{
//	if (!_this->invariants())
//	{
//		cwarn << (_pre ? "Pre" : "Post") << "invariant failed in" << _fn << "at" << _file << ":" << _line;
//		::boost::exception_detail::throw_exception_(FailedInvariant(), _fn, _file, _line);
//	}
//}
//
//struct TimerChannel: public LogChannel { static const char* name(); static const int verbosity = 0; };
//
//#if defined(_WIN32)
//const char* TimerChannel::name() { return EthRed " ! "; }
//#else
//const char* TimerChannel::name() { return EthRed " ⚡ "; }
//#endif
//
//TimerHelper::~TimerHelper()
//{
//	auto e = std::chrono::high_resolution_clock::now() - m_t;
//	if (!m_ms || e > chrono::milliseconds(m_ms))
//		clog(TimerChannel) << m_id << chrono::duration_cast<chrono::milliseconds>(e).count() << "ms";
//}
//
//uint64_t utcTime()
//{
//	// TODO: Fix if possible to not use time(0) and merge only after testing in all platforms
//	// time_t t = time(0);
//	// return mktime(gmtime(&t));
//	return time(0);
//}
//
//string inUnits(bigint const& _b, strings const& _units)
//{
//	ostringstream ret;
//	u256 b;
//	if (_b < 0)
//	{
//		ret << "-";
//		b = (u256)-_b;
//	}
//	else
//		b = (u256)_b;
//
//	u256 biggest = 1;
//	for (unsigned i = _units.size() - 1; !!i; --i)
//		biggest *= 1000;
//
//	if (b > biggest * 1000)
//	{
//		ret << (b / biggest) << " " << _units.back();
//		return ret.str();
//	}
//	ret << setprecision(3);
//
//	u256 unit = biggest;
//	for (auto it = _units.rbegin(); it != _units.rend(); ++it)
//	{
//		auto i = *it;
//		if (i != _units.front() && b >= unit)
//		{
//			ret << (double(b / (unit / 1000)) / 1000.0) << " " << i;
//			return ret.str();
//		}
//		else
//			unit /= 1000;
//	}
//	ret << b << " " << _units.front();
//	return ret.str();
//}

}
