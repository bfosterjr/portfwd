
#include <Windows.h>
#include "portfwd.h"

#ifdef BUILD_DLL

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved)  // reserved
{
    // Perform actions based on the reason for calling.
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Initialize once for each new process.
        // Return FALSE to fail DLL load.
        break;

    case DLL_THREAD_ATTACH:
        // Do thread-specific initialization.
        break;

    case DLL_THREAD_DETACH:
        // Do thread-specific cleanup.
        break;

    case DLL_PROCESS_DETACH:
        // Perform any necessary cleanup.
        break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

#else

#include <stdio.h>
#include "Xgetopt.h"

#define  DEFAULT_XOR    0xDEADBEEF

static
void
_print_usage()
{
    printf("\tPORTFWD Usage:\n\n");
    printf("\tportfwd.exe -l <local port> -a <remote address> -p <remote port> -t(unnel mode)");
    printf("\n\n");
}

int main(int argc, char** argv)
{
    int     c           = 0;
    char*   lport       = NULL;
    char*   rport       = NULL;
    char*   raddress    = NULL;
    BOOL    server      = FALSE;
    ULONG   xorKey      = 0;

    while ((c = getopt(argc, argv, "l:a:p:t")) != EOF)
    {
        switch (c)
        {
        case ('l') :
            lport = optarg;
            break;
        case ('a') :
            raddress = optarg;
            break;
        case ('p') :
            rport = optarg;
            break;
        case('t') :
            xorKey = DEFAULT_XOR;
            break;
        default:
            break;
        }
    }

    if (NULL == rport || 
        NULL == lport || 
        NULL == raddress)
    {
        _print_usage();
    }
    else
    {
        portfwd(lport, raddress, rport, xorKey);
    }
}

#endif
