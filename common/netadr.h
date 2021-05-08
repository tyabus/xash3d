/*
This part from Quake 2 qcommon/qcommon.h header file

Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef NETADR_H
#define NETADR_H

typedef enum {NA_LOOPBACK = 1, NA_BROADCAST, NA_IP, NA_IPX, NA_BROADCAST_IPX} netadrtype_t;

typedef struct netadr_s
{
	netadrtype_t	type;
	unsigned char	ip[4];
	unsigned char	ipx[10];
	unsigned short	port;
} netadr_t;

#endif // NETADR_H
