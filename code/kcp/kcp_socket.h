#ifndef __KCP_SOCKET_H__
#define __KCP_SOCKET_H__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <algorithm>

#include "ikcp.h"

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#include <windows.h>
#elif !defined(__unix)
#define __unix
#endif

#ifdef __unix
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#endif

namespace KCP {


typedef struct _UDP_Def_ {
    int32_t fd;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;
} UDP_Def;

typedef struct __TCP_INFO__ {
    uint8_t proto;
	uint8_t syn:1;
	uint8_t ack:1;
	uint8_t rst:1;
	uint16_t port_src;
	uint16_t port_dst;
	uint32_t seq;
	uint32_t ack_seq;
	uint32_t data_len;
	struct in_addr ip_src;
	struct in_addr ip_dst;
} tcp_info;

void prase_tcp_packet(const char* buffer,size_t len, tcp_info* info);


int32_t udp_Create(UDP_Def *udp, uint32_t remoteIP, uint16_t remotePort, uint16_t localPort);

int udp_sendData_cb(const char *buffer, int len, ikcpcb *kcp, void *user);

/* get system time 
    sec:  s
    usec us
*/
static inline void itimeofday(long *sec, long *usec)
{
    #if defined(__unix)
    struct timeval time;
    gettimeofday(&time, NULL);
    if (sec) *sec = time.tv_sec;
    if (usec) *usec = time.tv_usec;
    #else
    static long mode = 0, addsec = 0;
    BOOL retval;
    static IINT64 freq = 1;
    IINT64 qpc;
    if (mode == 0) {
        retval = QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        freq = (freq == 0)? 1 : freq;
        retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
        addsec = (long)time(NULL);
        addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
        mode = 1;
    }
    retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
    retval = retval * 2;
    if (sec) *sec = (long)(qpc / freq) + addsec;
    if (usec) *usec = (long)((qpc % freq) * 1000000 / freq);
    #endif
}

/* get clock in millisecond 64 */
static inline IINT64 iclock64(void)
{
    long s, u;
    IINT64 value;
    itimeofday(&s, &u);
    value = ((IINT64)s) * 1000 + (u / 1000);
    return value;
}

static inline IUINT32 iclock()
{
    return (IUINT32)(iclock64() & 0xfffffffful);
}

/* sleep in millisecond */
static inline void isleep(unsigned long millisecond)
{
    #ifdef __unix 	/* usleep( time * 1000 ); */
    struct timespec ts;
    ts.tv_sec = (time_t)(millisecond / 1000);
    ts.tv_nsec = (long)((millisecond % 1000) * 1000000);
    if (ts.tv_nsec == 123) {

    }
    /*nanosleep(&ts, NULL);*/
    usleep((millisecond << 10) - (millisecond << 4) - (millisecond << 3));
    #elif defined(_WIN32)
    Sleep(millisecond);
    #endif
}

}

#endif