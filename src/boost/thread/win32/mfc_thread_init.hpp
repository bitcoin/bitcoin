#ifndef BOOST_THREAD_WIN32_MFC_THREAD_INIT_HPP
#define BOOST_THREAD_WIN32_MFC_THREAD_INIT_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2008 Anthony Williams
// (C) Copyright 2011-2012 Vicente J. Botet Escriba


// check if we use MFC
#ifdef _AFXDLL
# if defined(_AFXEXT)

// can't use ExtRawDllMain from afxdllx.h as it also defines the symbol _pRawDllMain
extern "C"
inline BOOL WINAPI ExtRawDllMain(HINSTANCE, DWORD dwReason, LPVOID)
{
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    // save critical data pointers before running the constructors
    AFX_MODULE_STATE* pModuleState = AfxGetModuleState();
    pModuleState->m_pClassInit = pModuleState->m_classList;
    pModuleState->m_pFactoryInit = pModuleState->m_factoryList;
    pModuleState->m_classList.m_pHead = NULL;
    pModuleState->m_factoryList.m_pHead = NULL;
  }
  return TRUE; // ok
}

extern "C" __declspec(selectany) BOOL (WINAPI * const _pRawDllMainOrig)(HINSTANCE, DWORD, LPVOID) = &ExtRawDllMain;

# elif defined(_USRDLL)

extern "C" BOOL WINAPI RawDllMain(HINSTANCE, DWORD dwReason, LPVOID);
extern "C" __declspec(selectany) BOOL (WINAPI * const _pRawDllMainOrig)(HINSTANCE, DWORD, LPVOID) = &RawDllMain;

# endif
#endif

#endif
