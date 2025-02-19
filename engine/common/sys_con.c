/*
sys_con.c - stdout and log
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

#include "common.h"
#ifdef __ANDROID__
#include <android/log.h>
#endif
#include <string.h>
#include <errno.h>

// Don't enable colorized console on win32 and mobile devices
#if !defined( _WIN32 ) && !defined( XASH_MOBILE_PLATFORM )
#define COLORIZE_CONSOLE 1
#else
#define COLORIZE_CONSOLE 0
#endif

typedef struct {
	char		title[64];
	qboolean		log_active;
	char		log_path[MAX_SYSPATH];
	FILE		*logfile;
	int 		logfileno;
} LogData;

static LogData s_ld;

char *Sys_Input( void )
{
#ifdef _WIN32
	return Wcon_Input();
#elif defined( __linux__ ) && !defined( __ANDROID__ ) || defined( __FreeBSD__ ) || defined( __NetBSD__ ) || defined( __OpenBSD__ )
	return Posix_Input();
#else
	return NULL;
#endif
}

void Sys_DestroyConsole( void )
{
	// last text message into console or log
	Sys_Print( "Sys_DestroyConsole: Exiting!\n" );
#ifdef _WIN32
	Wcon_DestroyConsole();
#endif
}

void Sys_CloseLog( void )
{
	char	event_name[64];

	// continue logged
	switch( host.state )
	{
	case HOST_CRASHED:
		Q_strncpy( event_name, "crashed", sizeof( event_name ));
		break;
	case HOST_ERR_FATAL:
		Q_strncpy( event_name, "stopped with error", sizeof( event_name ));
		break;
	default:
		if( !host.change_game ) Q_strncpy( event_name, "stopped", sizeof( event_name ));
		else Q_strncpy( event_name, host.finalmsg, sizeof( event_name ));
		break;
	}

	printf( "\n================================================================================\n");
	printf( "\t%s (build %i) %s at %s\n", s_ld.title, Q_buildnum(), event_name, Q_timestamp( TIME_FULL ));
	printf( "================================================================================\n");

	if( s_ld.logfile )
	{
		fprintf( s_ld.logfile, "\n================================================================================\n");
		fprintf( s_ld.logfile, "\t%s (build %i) %s at %s\n", s_ld.title, Q_buildnum(), event_name, Q_timestamp( TIME_FULL ));
		fprintf( s_ld.logfile, "================================================================================\n");

		fclose( s_ld.logfile );
		s_ld.logfile = NULL;
	}
}

/*
===============================================================================

SYSTEM LOG

===============================================================================
*/

void Sys_InitLog( void )
{
	const char	*mode;

	if( Sys_CheckParm( "-log" ) )
	{
		s_ld.log_active = true;
		Q_strncpy( s_ld.log_path, "engine.log", sizeof( s_ld.log_path ));
	}

	if( host.change_game )
		mode = "a";
	else mode = "w";
	Q_strncpy( s_ld.title, "Xash3D-NG", sizeof ( s_ld.title ) );

	// print log to stdout
	printf( "================================================================================\n" );
	printf( "\t%s (build %i, %s-%s) started at %s\n", s_ld.title, Q_buildnum(), Q_buildos(), Q_buildarch(), Q_timestamp( TIME_FULL ));
	printf( "================================================================================\n" );

	s_ld.logfileno = -1;
	// create log if needed
	if( s_ld.log_active )
	{
		s_ld.logfile = fopen( s_ld.log_path, mode );
		if( !s_ld.logfile )
		{
				s_ld.log_active = false;
				MsgDev( D_ERROR, "Sys_InitLog: can't create log file %s: %s\n", s_ld.log_path, strerror( errno ) );
				return;
		}
		else s_ld.logfileno = fileno( s_ld.logfile );

		fprintf( s_ld.logfile, "================================================================================\n" );
		fprintf( s_ld.logfile, "\t%s (build %i, %s-%s) started at %s\n", s_ld.title, Q_buildnum(), Q_buildos(), Q_buildarch(), Q_timestamp( TIME_FULL ));
		fprintf( s_ld.logfile, "================================================================================\n" );
	}
}

void Sys_PrintLog( const char *pMsg )
{
	time_t		crt_time;
	const struct tm	*crt_tm;
	char logtime[32] = "";
	static char lastchar;

	time( &crt_time );
	crt_tm = localtime( &crt_time );
#ifdef __ANDROID__
	__android_log_print( ANDROID_LOG_DEBUG, "Xash", "%s", pMsg );
#endif

	if( !lastchar || lastchar == '\n')
		strftime( logtime, sizeof( logtime ), "[%H:%M:%S] ", crt_tm ); //short time

#if COLORIZE_CONSOLE == 1
	{
		char colored[4096];
		const char *msg = pMsg;
		int len = 0;
		while( *msg && ( len < 4090 ) )
		{
			static char q3ToAnsi[ 8 ] =
			{
				'0', // COLOR_BLACK
				'1', // COLOR_RED
				'2', // COLOR_GREEN
				'3', // COLOR_YELLOW
				'4', // COLOR_BLUE
				'6', // COLOR_CYAN
				'5', // COLOR_MAGENTA
				0 // COLOR_WHITE
			};

			if( IsColorString( msg ) )
			{
				int color;

				msg++;
				color = q3ToAnsi[ *msg++ % 8 ];
				colored[len++] = '\033';
				colored[len++] = '[';
				if( color )
				{
					colored[len++] = '3';
					colored[len++] = color;
				}
				else
					colored[len++] = '0';
				colored[len++] = 'm';
			}
			else
				colored[len++] = *msg++;
		}
		colored[len] = 0;
		printf( "\033[34m%s\033[0m%s\033[0m", logtime, colored );
	}
#else
#if !defined __ANDROID__ && !defined _WIN32 || defined XASH_DEDICATED
	printf( "%s %s", logtime, pMsg );
	fflush( stdout );
#endif
#endif
	lastchar = pMsg[strlen(pMsg)-1];
	if( !s_ld.logfile )
		return;

	if( !lastchar || lastchar == '\n')
		strftime( logtime, sizeof( logtime ), "[%Y:%m:%d|%H:%M:%S]", crt_tm ); //full time

	fprintf( s_ld.logfile, "%s %s", logtime, pMsg );
	fflush( s_ld.logfile );
}

int Sys_LogFileNo( void )
{
	if( s_ld.logfileno ) fflush( s_ld.logfile );
	return s_ld.logfileno;
}
