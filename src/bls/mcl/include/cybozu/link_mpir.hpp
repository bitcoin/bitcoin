#pragma once
/**
	@file
	@brief link mpir/mpirxx of mpir
	@author MITSUNARI Shigeo(@herumi)
*/
#if defined(_WIN32) && defined(_MT)
	#if _MSC_VER >= 1900 // VC2015, VC2017(1910)
		#pragma comment(lib, "mt/14/mpir.lib")
		#pragma comment(lib, "mt/14/mpirxx.lib")
	#elif _MSC_VER == 1800 // VC2013
		#pragma comment(lib, "mt/12/mpir.lib")
		#pragma comment(lib, "mt/12/mpirxx.lib")
	#elif _MSC_VER == 1700 // VC2012
		#pragma comment(lib, "mt/11/mpir.lib")
		#pragma comment(lib, "mt/11/mpirxx.lib")
	#endif
#endif
