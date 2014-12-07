#ifndef PORTFWD_H_
#define PORTFWD_H_

#include <Windows.h>
#pragma comment (lib, "Ws2_32.lib")

#ifdef BUILD_DLL
#define PFWD_EXPORT         __declspec( dllexport ) 
#else
#define PFWD_EXPORT
#endif

#define PFWD_API            __cdecl

PFWD_EXPORT
ULONG
PFWD_API
portfwd
(
    char*   lport,
    char*   raddress,
    char*   rport,
    ULONG   xorKey
);

#endif