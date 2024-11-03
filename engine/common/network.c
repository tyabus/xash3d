/*
network.c - network interface
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifdef _WIN32
// Winsock2
#include <ws2tcpip.h>
#else
// BSD sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
// Errors handling
#include <errno.h>
#include <fcntl.h>
#endif
#include "common.h"
#include "mathlib.h"
#include "netchan.h"

#define PORT_ANY		-1
#define MAX_LOOPBACK	4
#define MASK_LOOPBACK	(MAX_LOOPBACK - 1)

#define HAVE_GETADDRINFO

#ifdef _WIN32

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define pHtons htons
#define pConnect connect
#define pInet_Addr inet_addr
#define pRecvFrom recvfrom
#define pSendTo sendto
#define pSocket socket
#define pIoctlSocket ioctlsocket
#define pCloseSocket closesocket
#define pSetSockopt setsockopt
#define pBind bind
#define pGetHostName gethostname
#define pGetSockName getsockname
#define pGetHs
#define pRecv recv
#define pSend send
#define pInet_Ntoa inet_ntoa
#define pNtohs ntohs
#define pGetHostByName gethostbyname
#define pSelect select
#define pGetAddrInfo getaddrinfo

static void NET_InitializeCriticalSections( void );

#else
#define SOCKET_ERROR -1
#define pHtons htons
#define pConnect connect
#define pInet_Addr inet_addr
#define pRecvFrom recvfrom
#define pSendTo sendto
#define pSocket socket
#define pIoctlSocket ioctl
#define pCloseSocket close
#define pSetSockopt setsockopt
#define pBind bind
#define pGetHostName gethostname
#define pGetSockName getsockname
#define pGetHs
#define pRecv recv
#define pSend send
#define pInet_Ntoa inet_ntoa
#define pNtohs ntohs
#define pGetHostByName gethostbyname
#define pSelect select
#define pGetAddrInfo getaddrinfo
#define SOCKET int
#endif

typedef struct
{
	byte	data[NET_MAX_PAYLOAD];
	int	datalen;
} loopmsg_t;

typedef struct
{
	loopmsg_t	msgs[MAX_LOOPBACK];
	int	get, send;
} loopback_t;

typedef struct packetlag_s
{
	byte		*data;	// Raw stream data is stored.
	int			size;
	netadr_t	from;
	float		receivedtime;
	struct packetlag_s	*next;
	struct packetlag_s	*prev;
} packetlag_t;

static loopback_t	loopbacks[NS_COUNT];
static packetlag_t lagdata[NS_COUNT];
static float fakelag = 0.0f; // actual lag value
static qboolean noip = false;
static int		ip_sockets[NS_COUNT];
static qboolean winsockInitialized = false;
//static const char *net_src[2] = { "client", "server" };
static convar_t *net_ip;
static convar_t *net_hostport;
static convar_t *net_clientport;
static convar_t *net_port;
extern convar_t *net_showpackets;
static convar_t	*net_fakelag;
static convar_t	*net_fakeloss;
void NET_Restart_f( void );

#ifdef _WIN32
	static WSADATA winsockdata;
#endif

#ifdef _WIN32
/*
====================
NET_ErrorString
====================
*/
char *NET_ErrorString( void )
{
	switch( WSAGetLastError( ) )
	{
	case WSAEINTR: return "WSAEINTR";
	case WSAEBADF: return "WSAEBADF";
	case WSAEACCES: return "WSAEACCES";
	case WSAEDISCON: return "WSAEDISCON";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEALREADY: return "WSAEALREADY";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE: return "WSAEMSGSIZE";
	case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENETUNREACH: return "WSAENETUNREACH";
	case WSAENETRESET: return "WSAENETRESET";
	case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAENOTCONN: return "WSAENOTCONN";
	case WSAESHUTDOWN: return "WSAESHUTDOWN";
	case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
	case WSAETIMEDOUT: return "WSAETIMEDOUT";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	case WSAELOOP: return "WSAELOOP";
	case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
	case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
	case WSASYSNOTREADY: return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN: return "WSATRY_AGAIN";
	case WSANO_RECOVERY: return "WSANO_RECOVERY";
	case WSANO_DATA: return "WSANO_DATA";
	default: return "NO ERROR";
	}
}
#else
#define NET_ErrorString(x) strerror(errno)
#endif

static inline qboolean NET_IsSocketError( int retval )
{
#ifdef _WIN32
	return retval == SOCKET_ERROR ? true : false;
#else
	return retval < 0 ? true : false;
#endif
}


static void NET_NetadrToSockadr( netadr_t *a, struct sockaddr *s )
{
	Q_memset( s, 0, sizeof( *s ));

	if( a->type == NA_BROADCAST )
	{
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_port = a->port;
		((struct sockaddr_in *)s)->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if( a->type == NA_IP )
	{
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_addr.s_addr = *(int *)&a->ip;
		((struct sockaddr_in *)s)->sin_port = a->port;
	}
}


static void NET_SockadrToNetadr( struct sockaddr *s, netadr_t *a )
{
	if( s->sa_family == AF_INET )
	{
		a->type = NA_IP;
		*(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
		a->port = ((struct sockaddr_in *)s)->sin_port;
	}
}

#if !defined XASH_NO_ASYNC_NS_RESOLVE
#define CAN_ASYNC_NS_RESOLVE
#endif

#ifdef CAN_ASYNC_NS_RESOLVE
static void NET_ResolveThread( void );
#if !defined _WIN32
#include <pthread.h>
#define mutex_lock pthread_mutex_lock
#define mutex_unlock pthread_mutex_unlock
#define exit_thread( x ) pthread_exit(x)
#define create_thread( pfn ) !pthread_create( &nsthread.thread, NULL, (pfn), NULL )
#define detach_thread( x ) pthread_detach(x)
#define mutex_t  pthread_mutex_t
#define thread_t pthread_t

void *Net_ThreadStart( void *unused )
{
	NET_ResolveThread();
	return NULL;
}

#else // WIN32
#define mutex_lock EnterCriticalSection
#define mutex_unlock LeaveCriticalSection
#define detach_thread( x ) CloseHandle(x)
#define create_thread( pfn ) nsthread.thread = CreateThread( NULL, 0, pfn, NULL, 0, NULL )
#define mutex_t CRITICAL_SECTION
#define thread_t HANDLE
DWORD WINAPI Net_ThreadStart( LPVOID unused )
{
	NET_ResolveThread();
	ExitThread(0);
	return 0;
}
#endif

#ifdef DEBUG_RESOLVE
#define RESOLVE_DBG(x) Sys_PrintLog(x)
#else
#define RESOLVE_DBG(x)
#endif //  DEBUG_RESOLVE

static struct nsthread_s
{
	mutex_t mutexns;
	mutex_t mutexres;
	thread_t thread;
	int     result;
	string  hostname;
	qboolean busy;
} nsthread
#ifndef _WIN32
= { PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER }
#endif
;

#ifdef _WIN32
static void NET_InitializeCriticalSections( void )
{
	InitializeCriticalSection( &nsthread.mutexns );
	InitializeCriticalSection( &nsthread.mutexres );
}
#endif

void NET_ResolveThread( void )
{
#ifdef HAVE_GETADDRINFO
	struct addrinfo *ai = NULL, *cur;
	struct addrinfo hints;
	int sin_addr = 0;

	RESOLVE_DBG( "[resolve thread] starting resolve for " );
	RESOLVE_DBG( nsthread.hostname );
	RESOLVE_DBG( " with getaddrinfo\n" );
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_INET;
	if( !pGetAddrInfo( nsthread.hostname, NULL, &hints, &ai ) )
	{
		for( cur = ai; cur; cur = cur->ai_next ) {
			if( cur->ai_family == AF_INET ) {
				sin_addr = *((int*)&((struct sockaddr_in *)cur->ai_addr)->sin_addr);
				freeaddrinfo( ai );
				ai = NULL;
				break;
			}
		}

		if( ai )
			freeaddrinfo( ai );
	}

	if( sin_addr )
		RESOLVE_DBG( "[resolve thread] getaddrinfo success\n" );
	else
		RESOLVE_DBG( "[resolve thread] getaddrinfo failed\n" );
	mutex_lock( &nsthread.mutexres );
	nsthread.result = sin_addr;
	nsthread.busy = false;
	RESOLVE_DBG( "[resolve thread] returning result\n" );
	mutex_unlock( &nsthread.mutexres );
	RESOLVE_DBG( "[resolve thread] exiting thread\n" );
#else
	struct hostent *res;

	RESOLVE_DBG( "[resolve thread] starting resolve for " );
	RESOLVE_DBG( nsthread.hostname );
	RESOLVE_DBG( " with gethostbyname\n" );

	mutex_lock( &nsthread.mutexns );
	RESOLVE_DBG( "[resolve thread] locked gethostbyname mutex\n" );
	res = pGetHostByName( nsthread.hostname );
	if(res)
		RESOLVE_DBG( "[resolve thread] gethostbyname success\n" );
	else
		RESOLVE_DBG( "[resolve thread] gethostbyname failed\n" );

	mutex_lock( &nsthread.mutexres );
	RESOLVE_DBG( "[resolve thread] returning result\n" );
	if( res )
		nsthread.result = *(int *)res->h_addr_list[0];
	else
		nsthread.result = 0;

	nsthread.busy = false;

	mutex_unlock( &nsthread.mutexns );

	RESOLVE_DBG( "[resolve thread] unlocked gethostbyname mutex\n" );

	mutex_unlock( &nsthread.mutexres );

	RESOLVE_DBG( "[resolve thread] exiting thread\n" );
#endif
}

#endif // CAN_ASYNC_NS_RESOLVE

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/

int NET_StringToSockaddr( const char *s, struct sockaddr *sadr, qboolean nonblocking )
{
	int ip = 0;
	char		*colon;
	char		copy[MAX_SYSPATH];
	
	Q_memset( sadr, 0, sizeof( *sadr ));
	{
		((struct sockaddr_in *)sadr)->sin_family = AF_INET;
		((struct sockaddr_in *)sadr)->sin_port = 0;

		Q_strncpy( copy, s, sizeof( copy ));

		// strip off a trailing :port if present
		for( colon = copy; *colon; colon++ )
		{
			if( *colon == ':' )
			{
				*colon = 0;
				((struct sockaddr_in *)sadr)->sin_port = pHtons((short)Q_atoi( colon + 1 ));	
			}
		}

		if( copy[0] >= '0' && copy[0] <= '9' )
		{
			*(int *)&((struct sockaddr_in *)sadr)->sin_addr = pInet_Addr( copy );
		}
		else
		{
#ifdef CAN_ASYNC_NS_RESOLVE
			qboolean asyncfailed = false;
#ifdef _WIN32
			if ( InitializeCriticalSection )
#endif // _WIN32
			{
				if( !nonblocking )
				{
#ifdef HAVE_GETADDRINFO
					struct addrinfo *ai = NULL, *cur;
					struct addrinfo hints;

					memset( &hints, 0, sizeof( hints ) );
					hints.ai_family = AF_INET;
					if( !pGetAddrInfo( copy, NULL, &hints, &ai ) )
					{
						for( cur = ai; cur; cur = cur->ai_next ) {
							if( cur->ai_family == AF_INET ) {
								ip = *((int*)&((struct sockaddr_in *)cur->ai_addr)->sin_addr);
								freeaddrinfo(ai);
								ai = NULL;
								break;
							}
						}

						if( ai )
							freeaddrinfo(ai);
					}
#else
					struct hostent *h;

					mutex_lock( &nsthread.mutexns );
					h = pGetHostByName( copy );
					if( !h )
					{
						mutex_unlock( &nsthread.mutexns );
						return 0;
					}

					ip = *(int *)h->h_addr_list[0];
					mutex_unlock( &nsthread.mutexns );
#endif
				}
				else
				{
					mutex_lock( &nsthread.mutexres );

					if( nsthread.busy )
					{
						mutex_unlock( &nsthread.mutexres );
						return 2;
					}

					if( !Q_strcmp( copy, nsthread.hostname ) )
					{
						ip = nsthread.result;
						nsthread.hostname[0] = 0;
						detach_thread( nsthread.thread );
					}
					else
					{
						Q_strncpy( nsthread.hostname, copy, MAX_STRING );
						nsthread.busy = true;
						mutex_unlock( &nsthread.mutexres );

						if( create_thread( Net_ThreadStart ) )
							return 2;
						else // failed to create thread
						{
							MsgDev( D_ERROR, "NET_StringToSockaddr: failed to create thread!\n");
							nsthread.busy = false;
							asyncfailed = true;
						}
					}

					mutex_unlock( &nsthread.mutexres );
				}
			}
#ifdef _WIN32
			else
				asyncfailed = true;
#else
			if( asyncfailed )
#endif // _WIN32
#endif // CAN_ASYNC_NS_RESOLVE
			{
#ifdef HAVE_GETADDRINFO
				struct addrinfo *ai = NULL, *cur;
				struct addrinfo hints;

				memset( &hints, 0, sizeof( hints ) );
				hints.ai_family = AF_INET;
				if( !pGetAddrInfo( copy, NULL, &hints, &ai ) )
				{
					for( cur = ai; cur; cur = cur->ai_next ) {
						if( cur->ai_family == AF_INET ) {
							ip = *((int*)&((struct sockaddr_in *)cur->ai_addr)->sin_addr);
							freeaddrinfo(ai);
							ai = NULL;
							break;
						}
					}

					if( ai )
						freeaddrinfo(ai);
				}
#else
				struct hostent *h;
				if(!( h = pGetHostByName( copy )))
					return 0;
				ip = *(int *)h->h_addr_list[0];
#endif
			}

			if( !ip )
				return 0;

			*(int *)&((struct sockaddr_in *)sadr)->sin_addr = ip;
		}
	}
	return 1;
}

char *NET_AdrToString( const netadr_t a )
{
	if( a.type == NA_LOOPBACK )
		return "loopback";
	else if( a.type == NA_IP )
		return va( "%i.%i.%i.%i:%i", a.ip.u8[0], a.ip.u8[1], a.ip.u8[2], a.ip.u8[3], pNtohs( a.port ));

	return NULL; // compiler warning
}

char *NET_BaseAdrToString( const netadr_t a )
{
	if( a.type == NA_LOOPBACK )
		return "loopback";
	else if( a.type == NA_IP )
		return va( "%i.%i.%i.%i", a.ip.u8[0], a.ip.u8[1], a.ip.u8[2], a.ip.u8[3] );

	return NULL;
}

/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
qboolean NET_CompareBaseAdr( const netadr_t a, const netadr_t b )
{
	if( a.type != b.type )
		return false;

	if( a.type == NA_LOOPBACK )
		return true;

	if( a.type == NA_IP )
		return a.ip.u32 == b.ip.u32;

	MsgDev( D_ERROR, "NET_CompareBaseAdr: bad address type\n" );
	return false;
}

qboolean NET_CompareAdr( const netadr_t a, const netadr_t b )
{
	if( a.type != b.type )
		return false;

	if( a.type == NA_LOOPBACK )
		return true;

	if( a.type == NA_IP )
		return a.ip.u32 == b.ip.u32 && a.port == b.port;

	MsgDev( D_ERROR, "NET_CompareAdr: bad address type\n" );
	return false;
}

qboolean NET_IsLanAddress( netadr_t adr )
{

	if( adr.type == NA_LOOPBACK )
	{
		return true;
	} else if ( adr.type == NA_IP || adr.type == NA_BROADCAST ) {
		// 127.x.x.x and 10.x.x.x networks
		if( adr.ip.u8[0] == 127 || adr.ip.u8[0] == 10 )
			return true;
		// 192.168.x.x and 172.16.x.x - 172.31.x.x networks
		if( (adr.ip.u8[0] == 192 && adr.ip.u8[1] == 168) || (adr.ip.u8[0] == 172 && (adr.ip.u8[1] >= 16 && adr.ip.u8[1] <= 31)) )
			return true;
	}

	return false;
}

/*
=============
NET_StringToAdr

idnewt
192.246.40.70
=============
*/
qboolean NET_StringToAdr( const char *string, netadr_t *adr )
{
	struct sockaddr s;

	Q_memset( adr, 0, sizeof( netadr_t ));
	if( !Q_stricmp( string, "localhost" ) || !Q_stricmp( string, "loopback" ) )
	{
		adr->type = NA_LOOPBACK;
		return true;
	}

	if( !NET_StringToSockaddr( string, &s, false ))
		return false;
	NET_SockadrToNetadr( &s, adr );

	return true;
}

int NET_StringToAdrNB( const char *string, netadr_t *adr )
{
	struct sockaddr s;
	int res;

	Q_memset( adr, 0, sizeof( netadr_t ));
	if( !Q_stricmp( string, "localhost" )  || !Q_stricmp( string, "loopback" ) )
	{
		adr->type = NA_LOOPBACK;
		return true;
	}

	res = NET_StringToSockaddr( string, &s, true );

	if( res == 0 || res == 2 )
		return res;

	NET_SockadrToNetadr( &s, adr );

	return true;
}


/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/
static qboolean NET_GetLoopPacket( netsrc_t sock, netadr_t *from, byte *data, size_t *length )
{
	loopback_t	*loop;
	int		i;

	if( !data || !length )
		return false;

	loop = &loopbacks[sock];

	if( loop->send - loop->get > MAX_LOOPBACK )
		loop->get = loop->send - MAX_LOOPBACK;

	if( loop->get >= loop->send )
		return false;
	i = loop->get & MASK_LOOPBACK;
	loop->get++;

	Q_memcpy( data, loop->msgs[i].data, loop->msgs[i].datalen );
	*length = loop->msgs[i].datalen;

	Q_memset( from, 0, sizeof( *from ));
	from->type = NA_LOOPBACK;

	return true;
}

static void NET_SendLoopPacket( netsrc_t sock, size_t length, const void *data, netadr_t to )
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock^1];

	i = loop->send & MASK_LOOPBACK;
	loop->send++;

	Q_memcpy( loop->msgs[i].data, data, length );
	loop->msgs[i].datalen = length;
}

static void NET_ClearLoopback( void )
{
	loopbacks[0].send = loopbacks[0].get = 0;
	loopbacks[1].send = loopbacks[1].get = 0;
}

/*
=============================================================================

LAG & LOSS SIMULATION SYSTEM (network debugging)

=============================================================================
*/
/*
==================
NET_RemoveFromPacketList

double linked list remove entry
==================
*/
static void NET_RemoveFromPacketList( packetlag_t *p )
{
	p->prev->next = p->next;
	p->next->prev = p->prev;
	p->prev = NULL;
	p->next = NULL;
}

/*
==================
NET_ClearLaggedList

double linked list remove queue
==================
*/
static void NET_ClearLaggedList( packetlag_t *list )
{
	packetlag_t	*p, *n;

	p = list->next;
	while( p && p != list )
	{
		n = p->next;

		NET_RemoveFromPacketList( p );

		if( p->data )
		{
			Mem_Free( p->data );
			p->data = NULL;
		}

		Mem_Free( p );
		p = n;
	}

	list->prev = list;
	list->next = list;
}

/*
==================
NET_AddToLagged

add lagged packet to stream
==================
*/
static void NET_AddToLagged( netsrc_t sock, packetlag_t *list, packetlag_t *packet, netadr_t *from, size_t length, const void *data, float timestamp )
{
	byte	*pStart;

	if( packet->prev || packet->next )
		return;

	packet->prev = list->prev;
	list->prev->next = packet;
	list->prev = packet;
	packet->next = list;

	pStart = (byte *)Z_Malloc( length );
	memcpy( pStart, data, length );
	packet->data = pStart;
	packet->size = length;
	packet->receivedtime = timestamp;
	memcpy( &packet->from, from, sizeof( netadr_t ));
}

/*
==================
NET_AdjustLag

adjust time to next fake lag
==================
*/
static void NET_AdjustLag( void )
{
	static double	lasttime = 0.0;
	float		diff, converge;
	double		dt;

	dt = host.realtime - lasttime;
	dt = bound( 0.0, dt, 0.1 );
	lasttime = host.realtime;

	if( host_developer->integer >= D_ERROR || !net_fakelag->value )
	{
		if( net_fakelag->value != fakelag )
		{
			diff = net_fakelag->value - fakelag;
			converge = dt * 200.0f;

			if( fabs( diff ) < converge )
				converge = fabs( diff );

			if( diff < 0.0 )
				converge = -converge;

			fakelag += converge;
		}
	}
	else
	{
		MsgDev( D_INFO, "Server must enable dev-mode to activate fakelag\n" );
		Cvar_SetFloat( "net_fakelag", 0.0 );
		fakelag = 0.0f;
	}
}

/*
====================
NET_ClearLagData

clear fakelag list
====================
*/
void NET_ClearLagData( qboolean bClient, qboolean bServer )
{
	if( bClient ) NET_ClearLaggedList( &lagdata[NS_CLIENT] );
	if( bServer ) NET_ClearLaggedList( &lagdata[NS_SERVER] );
}


/*
==================
NET_LagPacket

add fake lagged packet into rececived message
==================
*/
static qboolean NET_LagPacket( qboolean newdata, netsrc_t sock, netadr_t *from, size_t *length, void *data )
{
	static int losscount[2];
	packetlag_t	*newPacketLag;
	packetlag_t	*packet;
	int		ninterval;
	float		curtime;

	if( fakelag <= 0.0f )
	{
		NET_ClearLagData( true, true );
		return newdata;
	}

	curtime = host.realtime;

	if( newdata )
	{
		if( net_fakeloss->value != 0.0f )
		{
			if( host_developer->integer >= D_ERROR )
			{
				losscount[sock]++;
				if( net_fakeloss->value <= 0.0f )
				{
					ninterval = fabs( net_fakeloss->value );
					if( ninterval < 2 ) ninterval = 2;

					if(( losscount[sock] % ninterval ) == 0 )
						return false;
				}
				else
				{
					if( Com_RandomLong( 0, 100 ) <= net_fakeloss->value )
						return false;
				}
			}
			else
			{
				Cvar_SetFloat( "net_fakeloss", 0.0 );
			}
		}

		newPacketLag = (packetlag_t *)Z_Malloc( sizeof( packetlag_t ));
		// queue packet to simulate fake lag
		NET_AddToLagged( sock, &lagdata[sock], newPacketLag, from, *length, data, curtime );
	}

	packet = lagdata[sock].next;

	while( packet != &lagdata[sock] )
	{
		if( packet->receivedtime <= curtime - ( fakelag / 1000.0 ))
			break;

		packet = packet->next;
	}

	if( packet == &lagdata[sock] )
		return false;

	NET_RemoveFromPacketList( packet );

	// delivery packet from fake lag queue
	memcpy( data, packet->data, packet->size );
	memcpy( &net_from, &packet->from, sizeof( netadr_t ));
	*length = packet->size;

	if( packet->data )
		Mem_Free( packet->data );

	Mem_Free( packet );

	return true;
}


/*
==================
NET_GetPacket

Never called by the game logic, just the system event queing
==================
*/
qboolean NET_GetPacket( netsrc_t sock, netadr_t *from, byte *data, size_t *length )
{
	int 		ret = SOCKET_ERROR;
	struct sockaddr	addr;
	socklen_t	addr_len;
	int		net_socket = 0;
	int		protocol;

	Q_memset( &addr, 0, sizeof( struct sockaddr ) );

	if( !data || !length )
		return false;

	NET_AdjustLag();

	if( NET_GetLoopPacket( sock, from, data, length ))
	{
		NET_LagPacket( true, sock, from, length, data );
		return true;
	}

	for( protocol = 0; protocol < 2; protocol++ )
	{
		if( !protocol) net_socket = ip_sockets[sock];

		if( !net_socket ) continue;

		addr_len = sizeof( addr );
		ret = pRecvFrom( net_socket, data, NET_MAX_PAYLOAD, 0, (struct sockaddr *)&addr, &addr_len );

		NET_SockadrToNetadr( &addr, from );

		if( NET_IsSocketError( ret ) )
		{
#ifdef _WIN32
			int err = WSAGetLastError();

			// WSAEWOULDBLOCK and WSAECONNRESET are silent
			if( err == WSAEWOULDBLOCK || err == WSAECONNRESET )
#else
			// WSAEWOULDBLOCK and WSAECONNRESET are silent
			if( errno == EWOULDBLOCK || errno == ECONNRESET )
#endif
				return false;

			MsgDev( D_ERROR, "NET_GetPacket: %s from %s\n", NET_ErrorString(), NET_AdrToString( *from ));
			continue;
		}

		if( ret == NET_MAX_PAYLOAD )
		{
			MsgDev( D_ERROR, "NET_GetPacket: oversize packet from %s\n", NET_AdrToString( *from ));
			continue;
		}

		*length = ret;
		return true;
	}

	if( NET_IsSocketError( ret ) )
	{
		return NET_LagPacket( false, sock, from, length, data );
	}

	return false;
}

/*
==================
NET_SendPacket
==================
*/
void NET_SendPacket( netsrc_t sock, size_t length, const void *data, netadr_t to )
{
	int		ret;
	struct sockaddr	addr;
	SOCKET		net_socket;

	// sequenced packets are shown in netchan, so just show oob
	if( net_showpackets->integer && *(int *)data == -1 )
		MsgDev( D_INFO, "send packet %zu\n", length );

	if( to.type == NA_LOOPBACK )
	{
		NET_SendLoopPacket( sock, length, data, to );
		return;
	}
	else if( to.type == NA_BROADCAST )
	{
		net_socket = ip_sockets[sock];
		if( !net_socket ) return;
	}
	else if( to.type == NA_IP )
	{
		net_socket = ip_sockets[sock];
		if( !net_socket ) return;
	}
	else
	{
		char buf[256];
		Q_strncpy( buf, data,  min( 256, length ));
		MsgDev( D_ERROR, "NET_SendPacket ( %d, %zu, \"%s\", %i ): bad address type %i\n", sock, length, buf, to.type, to.type );
		return;
	}

	NET_NetadrToSockadr( &to, &addr );

	ret = pSendTo( net_socket, data, length, 0, &addr, sizeof( addr ));

#ifdef _WIN32
	if (ret == SOCKET_ERROR)
	{
		int err = WSAGetLastError();

		// WSAEWOULDBLOCK is silent
		if (err == WSAEWOULDBLOCK)
			return;

		// some PPP links don't allow broadcasts
		if ((err == WSAEADDRNOTAVAIL) && ((to.type == NA_BROADCAST) || (to.type == NA_BROADCAST_IPX)))
			return;
	}
#else
	if( ret < 0 )
	{
		// WSAEWOULDBLOCK is silent
		if( errno == EWOULDBLOCK )
			return;

		// some PPP links don't allow broadcasts
		if(( errno == EADDRNOTAVAIL ) && (( to.type == NA_BROADCAST ) || ( to.type == NA_BROADCAST_IPX )))
			return;

		MsgDev( D_ERROR, "NET_SendPacket: %s to %s\n", NET_ErrorString(), NET_AdrToString( to ));
	}
#endif
}

/*
====================
NET_IPSocket
====================
*/
static int NET_IPSocket( const char *netInterface, int port )
{
	int		net_socket;
	struct sockaddr_in	addr;
	dword		_true = 1;

	Q_memset( &addr, 0, sizeof( struct sockaddr_in ) );

	MsgDev( D_NOTE, "NET_UDPSocket( %s, %i )\n", netInterface, port );

#ifdef _WIN32
	if(( net_socket = pSocket( PF_INET, SOCK_DGRAM, IPPROTO_UDP )) == SOCKET_ERROR )
	{
		int err = WSAGetLastError();
		if( err != WSAEAFNOSUPPORT )
			MsgDev( D_WARN, "NET_UDPSocket: socket = %s\n", NET_ErrorString( ));
		return 0;
	}

	if( pIoctlSocket( net_socket, FIONBIO, &_true ) == SOCKET_ERROR )
	{
		MsgDev( D_WARN, "NET_UDPSocket: ioctlsocket FIONBIO = %s\n", NET_ErrorString( ));
		pCloseSocket( net_socket );
		return 0;
	}

	// make it broadcast capable
	if( pSetSockopt( net_socket, SOL_SOCKET, SO_BROADCAST, (char *)&_true, sizeof( _true )) == SOCKET_ERROR )
	{
		MsgDev( D_WARN, "NET_UDPSocket: setsockopt SO_BROADCAST = %s\n", NET_ErrorString( ));
		pCloseSocket( net_socket );
		return 0;
	}
#else
	if(( net_socket = pSocket( PF_INET, SOCK_DGRAM, IPPROTO_UDP )) < 0 )
	{
		if( errno != EAFNOSUPPORT )
			MsgDev( D_WARN, "NET_UDPSocket: socket = %s\n", NET_ErrorString( ));
		return 0;
	}

	if( pIoctlSocket( net_socket, FIONBIO, &_true ) < 0 )
	{
		MsgDev( D_WARN, "NET_UDPSocket: ioctlsocket FIONBIO = %s\n", NET_ErrorString( ));
		pCloseSocket( net_socket );
		return 0;
	}

	// make it broadcast capable
	if( pSetSockopt( net_socket, SOL_SOCKET, SO_BROADCAST, (char *)&_true, sizeof( _true )) < 0 )
	{
		MsgDev( D_WARN, "NET_UDPSocket: setsockopt SO_BROADCAST = %s\n", NET_ErrorString( ));
	}
#endif

	if( !netInterface[0] || !Q_stricmp( netInterface, "localhost" ))
		addr.sin_addr.s_addr = INADDR_ANY;
	else NET_StringToSockaddr( netInterface, (struct sockaddr *)&addr, false );

	if( port == PORT_ANY ) addr.sin_port = 0;
	else addr.sin_port = pHtons((short)port);

	addr.sin_family = AF_INET;

#ifdef _WIN32
	if( pBind( net_socket, (void *)&addr, sizeof( addr )) == SOCKET_ERROR )
#else
	if( pBind( net_socket, (void *)&addr, sizeof( addr )) < 0 )
#endif
	{
		MsgDev( D_WARN, "NET_UDPSocket: bind = %s\n", NET_ErrorString( ));
		pCloseSocket( net_socket );
		return 0;
	}
	return net_socket;
}

/*
====================
NET_OpenIP
====================
*/
static void NET_OpenIP( qboolean changeport )
{
	int	port;
	qboolean sv_nat = Cvar_VariableInteger( "sv_nat" );
	qboolean cl_nat = Cvar_VariableInteger( "cl_nat" );

	net_ip = Cvar_Get( "ip", "localhost", 0, "network ip address" );

	if( changeport && ( net_port->modified || sv_nat ) )
	{
		// reopen socket to set random port
		if( ip_sockets[NS_SERVER] )
			pCloseSocket( ip_sockets[NS_SERVER] );
		ip_sockets[NS_SERVER] = 0;
		net_port->modified = false;
	}

	if( !ip_sockets[NS_SERVER] )
	{
		port = Cvar_VariableInteger("ip_hostport");

		// nat servers selects random port until ip_hostport specified
		if( !port )
		{
			if( sv_nat )
				port = PORT_ANY;
			else
				port = Cvar_VariableInteger("port");
		}

		ip_sockets[NS_SERVER] = NET_IPSocket( net_ip->string, port );
		if( !ip_sockets[NS_SERVER] && Host_IsDedicated() )
			Host_Error( "Couldn't allocate dedicated server IP port.\nMaybe you're trying to run dedicated server twice?\n" );
	}

	// dedicated servers don't need client ports
	if( Host_IsDedicated() ) return;

	if( changeport && ( net_clientport->modified || cl_nat ) )
	{
		// reopen socket to set random port
		if( ip_sockets[NS_CLIENT] )
			pCloseSocket( ip_sockets[NS_CLIENT] );
		ip_sockets[NS_CLIENT] = 0;
		net_clientport->modified = false;
	}

	if( !ip_sockets[NS_CLIENT] )
	{
		port = Cvar_VariableInteger( "ip_clientport" );

		if( !port )
		{
			if( cl_nat )
				port = PORT_ANY;
			else
				port = net_clientport->integer;
		}

		ip_sockets[NS_CLIENT] = NET_IPSocket( net_ip->string, port );
		if( !ip_sockets[NS_CLIENT] ) ip_sockets[NS_CLIENT] = NET_IPSocket( net_ip->string, PORT_ANY );
	}
}

/*
================
NET_GetLocalAddress

Returns the servers' ip address as a string.
================
*/
void NET_GetLocalAddress( void )
{
	char		buff[512];
	struct sockaddr_in	address;
	socklen_t		namelen;

	Q_memset( &net_local, 0, sizeof( netadr_t ));

	if( noip )
	{
		MsgDev( D_INFO, "TCP/IP Disabled.\n" );
	}
	else
	{
		// If we have changed the ip var from the command line, use that instead.
		if( Q_strcmp( net_ip->string, "localhost" ))
		{
			Q_strcpy( buff, net_ip->string );
		}
		else
		{
			pGetHostName( buff, 512 );
		}

		// ensure that it doesn't overrun the buffer
		buff[511] = 0;

		NET_StringToAdr( buff, &net_local );
		namelen = sizeof( address );

		if( pGetSockName( ip_sockets[NS_SERVER], (struct sockaddr *)&address, &namelen ) != 0 )
		{
			MsgDev( D_ERROR, "Could not get TCP/IP address, TCP/IP disabled\nReason: %s\n", NET_ErrorString( ));
			noip = true;
		}
		else
		{
			net_local.port = address.sin_port;
			Msg( "Server IP address: %s\n", NET_AdrToString( net_local ));
		}
	}
}

/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void NET_Config( qboolean multiplayer, qboolean changeport )
{
	static qboolean old_config;
	static qboolean bFirst = true;

	if( old_config == multiplayer && !Host_IsDedicated() && ( SV_Active() || CL_Active() ) )
		return;

	old_config = multiplayer;

	if( !multiplayer && !Host_IsDedicated() )
	{	
		int	i;

		// shut down any existing sockets
		for( i = 0; i < 2; i++ )
		{
			if( ip_sockets[i] )
			{
				pCloseSocket( ip_sockets[i] );
				ip_sockets[i] = 0;
			}
		}
	}
	else
	{	
		// open sockets
		if( !noip ) NET_OpenIP( changeport );

		// Get our local address, if possible
		if( bFirst )
		{
			bFirst = false;
			NET_GetLocalAddress();
		}
	}

	NET_ClearLoopback ();
}

/*
=================
NET_ShowIP_f
=================
*/
void NET_ShowIP_f( void )
{
	string		s;
	int		i;
	struct hostent	*h;
	struct in_addr	in;

	pGetHostName( s, sizeof( s ));

	if( !( h = pGetHostByName( s )))
	{
		Msg( "Can't get host\n" );
		return;
	}

	Msg( "HostName: %s\n", h->h_name );

	for( i = 0; h->h_addr_list[i]; i++ )
	{
		in.s_addr = *(int *)h->h_addr_list[i];
		Msg( "IP: %s\n", pInet_Ntoa( in ));
	}
}

/*
====================
NET_Init
====================
*/
void NET_Init( void )
{
	int i;
#ifdef _WIN32
	int	r;

	NET_InitializeCriticalSections( );

	r = WSAStartup( MAKEWORD( 2, 0 ), &winsockdata );
	if( r )
	{
		MsgDev( D_WARN, "NET_Init: winsock initialization failed: %d\n", r );
		return;
	}
#endif

	net_showpackets = Cvar_Get( "net_showpackets", "0", 0, "show network packets" );
	net_clientport = Cvar_Get( "clientport", "27005", 0, "client tcp/ip port" );
	net_port = Cvar_Get( "port", "27015", 0, "server tcp/ip port" );
	net_ip = Cvar_Get( "ip", "localhost", 0, "local server ip" );

	Cmd_AddCommand( "net_showip", NET_ShowIP_f,  "show hostname and IPs" );
	Cmd_AddCommand( "net_restart", NET_Restart_f, "restart the network subsystem" );

	net_fakelag = Cvar_Get( "net_fakelag", "0", 0, "lag all incoming network data (including loopback) by xxx ms." );
	net_fakeloss = Cvar_Get( "net_fakeloss", "0", 0, "act like we dropped the packet this % of the time." );

	// prepare some network data
	for( i = 0; i < NS_COUNT; i++ )
	{
		lagdata[i].prev = &lagdata[i];
		lagdata[i].next = &lagdata[i];
	}


	if( Sys_CheckParm( "-noip" )) noip = true;

	winsockInitialized = true;
	MsgDev( D_NOTE, "NET_Init()\n" );
}


/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown( void )
{
	if( !winsockInitialized )
		return;

	Cmd_RemoveCommand( "net_showip" );
	Cmd_RemoveCommand( "net_restart" );

	NET_ClearLagData( true, true );

	NET_Config( false, false );
#ifdef _WIN32
	WSACleanup();
#endif
	winsockInitialized = false;
}

/*
=================
NET_Restart_f
=================
*/
void NET_Restart_f( void )
{
	NET_Shutdown();
	NET_Init();
}