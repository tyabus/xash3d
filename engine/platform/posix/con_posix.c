/*
con_posix.c - reading from stdin
Copyright (C) 2025 Flying With Gauss

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "system.h"

#if !defined( XASH_MOBILE_PLATFORM )
// non-blocking console input
#include <sys/select.h>

char *Posix_Input( void )
{
	fd_set rfds;
	static char line[1024];
	static int len;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&rfds);
	FD_SET(0, &rfds); // stdin
    while( select(1, &rfds, NULL, NULL, &tv ) > 0 )
	{
		if( read( 0, &line[len], 1 ) != 1 )
			return NULL;
		if( line[len] == '\n' || len > 1022 )
		{
			line[ ++len ] = 0;
			len = 0;
			return line;
		}
		len++;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	}
}

#endif // !XASH_MOBILE_PLATFORM