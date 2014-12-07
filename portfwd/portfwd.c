
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "portfwd.h"


#define  MAX_RCV_LEN    2048

typedef struct _SEND_RECV_SOCKETS
{
    SOCKET  clientSocket;
    SOCKET  connectSocket;
    ULONG   xorKey;
} SEND_RECV_SOCKETS, *PSEND_RECV_SOCKETS;



static
BOOL
_xorbuf
(
    char*       buf,
    int         bufLen,
    ULONG       xorKey
)
{
    int     i   = 0;
    PBYTE   xor = (PBYTE)&xorKey;
    for (i = 0; i < bufLen; i++)
    {
        buf[i] = buf[i] ^ xor[i % sizeof(ULONG)];
    }
    return TRUE;
}

static
ULONG
__stdcall
_recv_thread_f
(
    PVOID arg
)
{

    ULONG               retVal          = 0;
    BOOL                done            = FALSE;
    int                 recvSize        = 0;
    DWORD               bytesWritten    = 0;
    PSEND_RECV_SOCKETS  sockets         = (PSEND_RECV_SOCKETS)arg;
    CHAR                recvBuf[MAX_RCV_LEN];

    __try
    {
        do
        {
            done = FALSE;
            recvSize = recv(sockets->clientSocket, recvBuf, sizeof(recvBuf), 0);
            if (0 > recvSize)
            {
                retVal = -1;
                done = TRUE;
            }
            else if (0 == recvSize)
            {
                done = TRUE;

            }
            else if (0 != sockets->xorKey && !_xorbuf(recvBuf, recvSize, sockets->xorKey))
            {
                done = TRUE;
            }
            else if (SOCKET_ERROR == send(sockets->connectSocket, recvBuf, recvSize, 0))
            {
                retVal = -1;
                done = TRUE;
            }

        } while (!done);
    }
    __finally
    {
        shutdown(sockets->clientSocket, SD_SEND);
        shutdown(sockets->connectSocket, SD_SEND);
    }



    return retVal;
}

static
BOOL
_create_send_recv_threads
(
    SOCKET  clientSocket,
    SOCKET  connectSocket,
    ULONG   xorKey
)
{
    BOOL                retVal          = FALSE;
    HANDLE              hThreadRecv1    = NULL;
    HANDLE              hThreadRecv2    = NULL;
    PSEND_RECV_SOCKETS  pRecv1Arg       = NULL;
    PSEND_RECV_SOCKETS  pRecv2Arg       = NULL;
    HANDLE  wait_objs[2] = { 0 };
    DWORD   threadId = 0;

    __try
    {
        if (NULL == (pRecv1Arg = HeapAlloc(GetProcessHeap(), 0, sizeof(*pRecv1Arg))) ||
            NULL == (pRecv2Arg = HeapAlloc(GetProcessHeap(), 0, sizeof(*pRecv2Arg))))
        {

        }
        else if (((pRecv1Arg->clientSocket = clientSocket),FALSE)       ||
                 ((pRecv1Arg->connectSocket = connectSocket), FALSE)    ||
                 ((pRecv1Arg->xorKey = xorKey), FALSE)                  ||
                 ((pRecv2Arg->clientSocket = connectSocket), FALSE)     ||
                 ((pRecv2Arg->connectSocket = clientSocket), FALSE)     ||
                 ((pRecv2Arg->xorKey = xorKey), FALSE) )
        {
            //unreachable
        }
        else if (NULL == (hThreadRecv1 = CreateThread(NULL, 0, _recv_thread_f, (PVOID)pRecv1Arg, 0, &threadId)) ||
                 NULL == (hThreadRecv2 = CreateThread(NULL, 0, _recv_thread_f, (PVOID)pRecv2Arg, 0, &threadId)))
        {

        }
        else
        {
            wait_objs[0] = hThreadRecv1;
            wait_objs[1] = hThreadRecv2;
            WaitForMultipleObjects(2, wait_objs, TRUE, INFINITE);
            retVal = TRUE;
        }
    }
    __finally
    {
        if (NULL != hThreadRecv1)
        {
            TerminateThread(hThreadRecv1, 0);
            CloseHandle(hThreadRecv1);
            hThreadRecv1 = NULL;
        }
        if (NULL != hThreadRecv2)
        {
            TerminateThread(hThreadRecv2, 0);
            CloseHandle(hThreadRecv2);
            hThreadRecv2 = NULL;
        }

        if (NULL != pRecv1Arg)
        {
            HeapFree(GetProcessHeap(),0,pRecv1Arg);
            pRecv1Arg = NULL;
        }
        if (NULL != pRecv2Arg)
        {
            HeapFree(GetProcessHeap(), 0, pRecv2Arg);
            pRecv2Arg = NULL;
        }
    }
    return retVal;
}

static
SOCKET
_connect_server
(
struct addrinfo*  srvinfo
)
{
    SOCKET      connectSocket = INVALID_SOCKET;


    for (; srvinfo != NULL; srvinfo = srvinfo->ai_next) {

        // Create a SOCKET for connecting to server
        if (INVALID_SOCKET == (connectSocket = socket(srvinfo->ai_family,
            srvinfo->ai_socktype, srvinfo->ai_protocol)))
        {
            break;
        }
        else if (SOCKET_ERROR == connect(connectSocket, srvinfo->ai_addr,
            (int)srvinfo->ai_addrlen))
        {
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
        }
        else
        {
            break;
        }
    }

    return connectSocket;
}


static
ULONG
_handle_client
(
    SOCKET  clientSocket,
    char*   raddress,
    char*   rport,
    ULONG   xorKey
)
{
    ULONG   retVal              = -1;
    SOCKET  connectSocket       = INVALID_SOCKET;
    struct  addrinfo *result    = NULL;
    struct  addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    __try
    {
        if (0 != getaddrinfo(raddress, rport, &hints, &result))
        {
            retVal = -1;
        }
        else if (INVALID_SOCKET == (connectSocket = _connect_server(result)))
        {
            retVal = -1;
        }
        else if (!_create_send_recv_threads(clientSocket, connectSocket,xorKey))
        {
            retVal = -1;
        }
        else
        {
            retVal = 0;
        }

    }
    __finally
    {
        if (INVALID_SOCKET != connectSocket)
        {
            closesocket(connectSocket);
        }

        closesocket(clientSocket);
    }
    return retVal;
}


static
ULONG
_do_accept
(
    SOCKET  listenSocket,
    char*   raddress,
    char*   rport,
    ULONG   xorKey
)
{
    ULONG   retVal = -1;
    SOCKET  clientSocket = INVALID_SOCKET;

    do
    {
        if (INVALID_SOCKET == (clientSocket = accept(listenSocket, NULL, NULL)))
        {
            retVal = -1;
        }
        else
        {
            retVal = _handle_client(clientSocket, raddress, rport, xorKey);
        }
    } while (0 == retVal);

    return retVal;
}



PFWD_EXPORT
ULONG
PFWD_API
portfwd
(
    char*   lport,
    char*   raddress,
    char*   rport,
    ULONG   xorKey
)
{
    ULONG           retVal = -1;
    WSADATA         wsaData = { 0 };
    SOCKET          listenSocket = INVALID_SOCKET;
    BOOL            startup = FALSE;
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    __try
    {
        if (NULL == lport || NULL == raddress ||
            NULL == rport)
        {
            retVal = -1;
        }
        else if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData) ||
            ((startup = TRUE), FALSE))
        {
            retVal = -1;
        }
        else if (0 != getaddrinfo(NULL, lport, &hints, &result))
        {
            retVal = -1;
        }
        else if (INVALID_SOCKET == (listenSocket = socket(result->ai_family,
                                    result->ai_socktype, result->ai_protocol)))
        {
            retVal = -1;
        }
        else if (SOCKET_ERROR == bind(listenSocket, result->ai_addr, (int)result->ai_addrlen))
        {
            retVal = -1;
        }
        else if (SOCKET_ERROR == listen(listenSocket, SOMAXCONN))
        {
            retVal = -1;
        }
        else
        {
            retVal = _do_accept(listenSocket, raddress, rport, xorKey);
        }
    }
    __finally
    {

        if (NULL != result)
        {
            freeaddrinfo(result);
            result = NULL;
        }

        if (INVALID_SOCKET != listenSocket)
        {
            closesocket(listenSocket);
            listenSocket = INVALID_SOCKET;
        }

        if (startup)
        {
            (void)WSACleanup();
        }
    }
    return retVal;
}