// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifdef _MSC_VER
#pragma warning(disable:4786)
#pragma warning(disable:4804)
#pragma warning(disable:4805)
#pragma warning(disable:4717)
#endif
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0400
#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0400
#define WIN32_LEAN_AND_MEAN 1
#define __STDC_LIMIT_MACROS // to enable UINT64_MAX from stdint.h
#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>  // to get BSD define
#endif
#ifdef __WXMAC_OSX__
#ifndef BSD
#define BSD 1
#endif
#endif
#ifdef GUI
#include <wx/wx.h>
#include <wx/stdpaths.h>
#include <wx/snglinst.h>
#include <wx/utils.h>
#include <wx/clipbrd.h>
#include <wx/taskbar.h>
#endif
#include <openssl/buffer.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <db_cxx.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <assert.h>
#include <memory>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <set>
#include <algorithm>
#include <numeric>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_recursive_mutex.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/config.hpp>
#include <boost/program_options/detail/config_file.hpp>
#include <boost/program_options/parsers.hpp>

#ifdef __WXMSW__
#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <io.h>
#include <process.h>
#include <malloc.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <fcntl.h>
#endif
#ifdef BSD
#include <netinet/in.h>
#endif


#pragma hdrstop
using namespace std;
using namespace boost;

#include "strlcpy.h"
#include "serialize.h"
#include "uint256.h"
#include "util.h"
#include "key.h"
#include "bignum.h"
#include "base58.h"
#include "script.h"
#include "db.h"
#include "net.h"
#include "irc.h"
#include "main.h"
#include "rpc.h"
#ifdef GUI
#include "uibase.h"
#include "ui.h"
#else
#include "noui.h"
#endif
#include "init.h"

#include "xpm/addressbook16.xpm"
#include "xpm/addressbook20.xpm"
#include "xpm/bitcoin16.xpm"
#include "xpm/bitcoin20.xpm"
#include "xpm/bitcoin32.xpm"
#include "xpm/bitcoin48.xpm"
#include "xpm/bitcoin80.xpm"
#include "xpm/check.xpm"
#include "xpm/send16.xpm"
#include "xpm/send16noshadow.xpm"
#include "xpm/send20.xpm"
#include "xpm/about.xpm"
