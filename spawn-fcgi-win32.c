/* original code by keathmilligan @ http://redmine.lighttpd.net/boards/2/topics/686 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <ws2tcpip.h>

#define FCGI_LISTENSOCK_FILENO 0

#define UNIX_PATH_LEN 108

typedef unsigned short int sa_family_t;

struct sockaddr_un {
    sa_family_t	sun_family;              /* address family AF_LOCAL/AF_UNIX */
    char		sun_path[UNIX_PATH_LEN]; /* 108 bytes of socket address     */
};

/* Evaluates the actual length of `sockaddr_un' structure. */
#define SUN_LEN(p) ((size_t)(((struct sockaddr_un *) NULL)->sun_path) \
                   + strlen ((p)->sun_path))

void ErrorExit(LPTSTR lpszFunction, const char * file, int line)
{
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError() || WSAGetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    fprintf(stderr, "%s:%d: %s: %s\n", file, line, lpszFunction, dw, lpMsgBuf);
    fflush(stderr);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw);
}

#define ERR(s) ErrorExit(s, __FILE__, __LINE__)

int fcgi_spawn_connection(char *app, char *addr, unsigned short port, int child_count) {
    SOCKET fcgi_fd;
    int socket_type, rc = 0, i;

    struct sockaddr_un fcgi_addr_un;
    struct sockaddr_in fcgi_addr_in;
    struct sockaddr *fcgi_addr;

    socklen_t servlen;

    WORD  wVersion;
    WSADATA wsaData;

    if (child_count > 256) {
        child_count = 256;
    }

    fcgi_addr_in.sin_family = AF_INET;
    if (addr != NULL) {
            fcgi_addr_in.sin_addr.s_addr = inet_addr(addr);
    } else {
            fcgi_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    fcgi_addr_in.sin_port = htons(port);
    servlen = sizeof(fcgi_addr_in);

    socket_type = AF_INET;
    fcgi_addr = (struct sockaddr *) &fcgi_addr_in;

    /*
     * Initialize windows sockets library.
     */
    wVersion = MAKEWORD(2,0);
    if (WSAStartup( wVersion, &wsaData )) {
        ERR("error starting Windows sockets");
    }

    if (-1 == (fcgi_fd = socket(socket_type, SOCK_STREAM, 0))) {
        ERR("socket");
    }

    if (0 == connect(fcgi_fd, fcgi_addr, servlen)) {
        ERR("socket is already used, can't spawn");
    } else {
        /* server is not up, spawn in  */
        int val;
        HANDLE handles[256];

        closesocket(fcgi_fd);

        /* reopen socket */
        if (-1 == (fcgi_fd = socket(socket_type, SOCK_STREAM, 0))) {
            ERR("socket");
        }

        val = 1;
        if (setsockopt(fcgi_fd, SOL_SOCKET, SO_REUSEADDR, 	(const char*)&val, sizeof(val)) < 0) {
            ERR("setsockopt");
        }

        /* create socket */
        if (-1 == bind(fcgi_fd, fcgi_addr, servlen)) {
            ERR("bind failed");
        }

        if (-1 == listen(fcgi_fd, 1024)) {
            ERR("listen");
        }


        for (i=0; i < child_count; i++) {
            PROCESS_INFORMATION pi;
            STARTUPINFO si;

            ZeroMemory(&si,sizeof(STARTUPINFO));
            si.cb = sizeof(STARTUPINFO);
            si.dwFlags = STARTF_USESTDHANDLES;

            /* FastCGI expects the socket to be passed in hStdInput and the rest should be INVALID_HANDLE_VALUE */
            si.hStdOutput = INVALID_HANDLE_VALUE;
            si.hStdInput  = (HANDLE) fcgi_fd;
            si.hStdError  = INVALID_HANDLE_VALUE;

            if (!CreateProcess(NULL,app,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi)) {
                ERR("CreateProcess failed");
            } else {
                handles[i] = pi.hProcess;
                printf("Process %d spawned successfully\n", pi.dwProcessId);
            }
        }

        if (WaitForMultipleObjects(child_count, handles, TRUE, INFINITE) == WAIT_FAILED) {
          ERR("WaitForMultipleObjects failed");
        }

        for (i=0; i < child_count; i++) {
          CloseHandle(handles[i]);
        }

    }

    closesocket(fcgi_fd);

    return rc;
}


void show_version () {
    char *b = "spawn-fcgi-win32 - spawns fastcgi processes\n";
    write(1, b, strlen(b));
}

void show_help () {
    char *b =
"Usage: spawn-fcgi [options] -- \"<fcgiapp> [fcgi app arguments]\"\n"
"\n"
"spawn-fcgi-win32 - spawns fastcgi processes\n"
"\n"
"Options:\n"
" -f <fcgiapp> filename of the fcgi-application\n"
" -a <addr>    bind to ip address\n"
" -p <port>    bind to tcp-port\n"
" -C <childs>  numbers of childs to spawn (default 1)\n"
" -v           show version\n"
" -h           show this help\n"
;
    write(1, b, strlen(b));
}


int main(int argc, char **argv) {
    char *fcgi_app = NULL, *addr = NULL;
    unsigned short port = 0;
    int child_count = 1;
    int o;
    struct sockaddr_un un;

    while (-1 != (o = getopt(argc, argv, "c:f:g:hna:p:u:vC:F:s:P:"))) {
        switch(o) {
        case 'f': fcgi_app = optarg; break;
        case 'a': addr = optarg;/* ip addr */ break;
        case 'p': port = strtol(optarg, NULL, 10);/* port */ break;
        case 'C': child_count = strtol(optarg, NULL, 10);/*  */ break;
        case 'v': show_version(); return 0;
        case 'h': show_help(); return 0;
        default:
            show_help();
            return -1;
        }
    }

    if (optind < argc) {
        fcgi_app = argv[optind];
    }

    if (fcgi_app == NULL || port == 0) {
        show_help();
        return -1;
    }

    return fcgi_spawn_connection(fcgi_app, addr, port, child_count);
}
