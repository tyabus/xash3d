/*
crashhandler_libunwind.c - advanced crashhandler with libunwind
Copyright (C) 2022 Mr0maks

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

#define _GNU_SOURCE
#define __USE_GNU
#if !defined(__OpenBSD__)
#include <ucontext.h>
#endif

#include <signal.h>
#include <sys/mman.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

struct sigaction oldFilter;

/*
================
Sys_Crash

Crash handler, called from system
================
*/

static void Sys_Crash( int signal, siginfo_t *si, void *ucontext)
{
	char message[4096], symbol[256];
	int len, line, logfd, i = 0;
	unw_word_t ip, sp, off;
	unw_cursor_t cursor;
	unw_context_t context;

	unw_getcontext( &context );
	unw_init_local( &cursor, &context );

	// Safe actions first, stack and memory may be corrupted
	len = Q_snprintf( message, sizeof( message ), "Ver: Xash3D-NG " XASH_VERSION " (build %i-%s, %s-%s)\n", Q_buildnum(), Q_buildcommit(), Q_buildos(), Q_buildarch() );

	#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
		len += Q_snprintf( message, sizeof( message ), "Sys_Crash: signal %d, err %d with code %d at %p\n", signal, si->si_errno, si->si_code, si->si_addr );
	#else
		len += Q_snprintf( message, sizeof( message ), "Sys_Crash: signal %d, err %d with code %d at %p %p\n", signal, si->si_errno, si->si_code, si->si_addr, si->si_ptr );
	#endif

	// Flush buffers before writing directly to descriptors
	fflush( stdout );
	fflush( stderr );

	// Now get log fd and write trace directly to log
	logfd = Sys_LogFileNo();

	write( STDERR_FILENO, message, len );
	write( logfd, message, len );
	write( STDERR_FILENO, "Stack backtrace:\n", 17 );
	write( logfd, "Stack backtrace:\n", 17 );
	strncpy(message + len, "Stack backtrace:\n", sizeof( message ) - len);
	len += 17;

	while( unw_step(&cursor) > 0 )
	{
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_SP, &sp);

		if( unw_get_proc_name( &cursor, symbol, sizeof(symbol), &off ) == 0 )
		{
			line = Q_snprintf( message + len, sizeof( message ) - len, "#%-2d 0x%016" PRIxPTR " sp=0x%016" PRIxPTR " %s + 0x%" PRIxPTR " %s\n", ++i, ip, sp, symbol, off, (unw_is_signal_frame(&cursor) > 0) ? "<" : "");
		} else {
			line = Q_snprintf( message + len, sizeof( message ) - len, "#%-2d 0x%016" PRIxPTR " sp=0x%016" PRIxPTR " %s + 0x%" PRIxPTR " %s\n", ++i, ip, sp, symbol, off, (unw_is_signal_frame(&cursor) > 0) ? "<" : "" );
		}

		write( STDERR_FILENO, message + len, line );
		write( logfd, message + len, line );
		len += line;
	}

	// Put MessageBox as Sys_Error
	Msg( "%s\n", message );
#ifdef XASH_SDL
	SDL_SetWindowGrab( host.hWnd, SDL_FALSE );
#endif
	MSGBOX( message );

	// Log saved, now we can try to save configs and close log correctly, it may crash
	if( host.type == HOST_NORMAL )
		CL_Crashed();
	host.state = HOST_CRASHED;
	host.crashed = true;

	Sys_Quit();
}

void Sys_SetupCrashHandler( void )
{
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = Sys_Crash;
	act.sa_flags = SA_SIGINFO | SA_ONSTACK;
	sigaction(SIGSEGV, &act, &oldFilter);
	sigaction(SIGABRT, &act, &oldFilter);
	sigaction(SIGBUS, &act, &oldFilter);
	sigaction(SIGILL, &act, &oldFilter);
}

void Sys_RestoreCrashHandler( void )
{
	sigaction( SIGSEGV, &oldFilter, NULL );
	sigaction( SIGABRT, &oldFilter, NULL );
	sigaction( SIGBUS, &oldFilter, NULL );
	sigaction( SIGILL, &oldFilter, NULL );
}
