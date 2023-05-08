/*
crashhandler.c - advanced crashhandler
Copyright (C) 2016 Mittorn

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

/*
================
Sys_Crash

Crash handler, called from system
================
*/

// Posix signal handler
#include "library.h"
#define _GNU_SOURCE
#if defined(__APPLE__)
#define _XOPEN_SOURCE
#include <sys/ucontext.h>
#endif
#define __USE_GNU
#if !defined(__OpenBSD__) || !defined(__APPLE__)
#include <ucontext.h>
#endif
#include <dlfcn.h>
#include <signal.h>
#include <sys/mman.h>

int printframe( char *buf, int len, int i, void *addr )
{
	Dl_info dlinfo;
	if( len <= 0 ) return 0; // overflow
	if( dladdr( addr, &dlinfo ))
	{
		if( dlinfo.dli_sname )
			return Q_snprintf( buf, len, "% 2d: %p <%s+%lu> (%s)\n", i, addr, dlinfo.dli_sname,
					(unsigned long)addr - (unsigned long)dlinfo.dli_saddr, dlinfo.dli_fname ); // print symbol, module and address
		else
			return Q_snprintf( buf, len, "% 2d: %p (%s)\n", i, addr, dlinfo.dli_fname ); // print module and address
	}
	else
		return Q_snprintf( buf, len, "% 2d: %p\n", i, addr ); // print only address
}

struct sigaction oldFilter;

static void Sys_Crash( int signal, siginfo_t *si, void *context)
{
	void *trace[32];

	char message[4096], stackframe[256];
	int len, stacklen, logfd, i = 0;
#if defined(__OpenBSD__)
	struct sigcontext *ucontext = (struct sigcontext*)context;
#else
	ucontext_t *ucontext = (ucontext_t*)context;
#endif
#if defined(__x86_64__)
	#if defined(__FreeBSD__)
		void *pc = (void*)ucontext->uc_mcontext.mc_rip, **bp = (void**)ucontext->uc_mcontext.mc_rbp, **sp = (void**)ucontext->uc_mcontext.mc_rsp;
	#elif defined(__NetBSD__)
		void *pc = (void*)ucontext->uc_mcontext.__gregs[REG_RIP], **bp = (void**)ucontext->uc_mcontext.__gregs[REG_RBP], **sp = (void**)ucontext->uc_mcontext.__gregs[REG_RSP];
	#elif defined(__OpenBSD__)
		void *pc = (void*)ucontext->sc_rip, **bp = (void**)ucontext->sc_rbp, **sp = (void**)ucontext->sc_rsp;
	#elif defined(__APPLE__)
		void *pc = (void*)ucontext->uc_mcontext->__ss.__rip, **bp = (void**)ucontext->uc_mcontext->__ss.__rbp, **sp = (void**)ucontext->uc_mcontext->__ss.__rsp;
	#else
		void *pc = (void*)ucontext->uc_mcontext.gregs[REG_RIP], **bp = (void**)ucontext->uc_mcontext.gregs[REG_RBP], **sp = (void**)ucontext->uc_mcontext.gregs[REG_RSP];
	#endif
#elif defined(__i386__)
	#if defined(__FreeBSD__)
		void *pc = (void*)ucontext->uc_mcontext.mc_eip, **bp = (void**)ucontext->uc_mcontext.mc_ebp, **sp = (void**)ucontext->uc_mcontext.mc_esp;
	#elif defined(__NetBSD__)
		void *pc = (void*)ucontext->uc_mcontext.__gregs[REG_EIP], **bp = (void**)ucontext->uc_mcontext.__gregs[REG_EBP], **sp = (void**)ucontext->uc_mcontext.__gregs[REG_ESP];
	#elif defined(__OpenBSD__)
		void *pc = (void*)ucontext->sc_eip, **bp = (void**)ucontext->sc_ebp, **sp = (void**)ucontext->sc_esp;
	#elif defined(__APPLE__)
		void *pc = (void*)ucontext->uc_mcontext->__ss.__rip, **bp = (void**)ucontext->uc_mcontext->__ss.__rbp, **sp = (void**)ucontext->uc_mcontext->__ss.__rsp;
	#else
		void *pc = (void*)ucontext->uc_mcontext.gregs[REG_EIP], **bp = (void**)ucontext->uc_mcontext.gregs[REG_EBP], **sp = (void**)ucontext->uc_mcontext.gregs[REG_ESP];
	#endif
#elif defined(__aarch64__) // arm not tested
	void *pc = (void*)ucontext->uc_mcontext.pc, **bp = (void*)ucontext->uc_mcontext.regs[29], **sp = (void*)ucontext->uc_mcontext.sp;
#elif defined(__arm__)
	void *pc = (void*)ucontext->uc_mcontext.arm_pc, **bp = (void*)ucontext->uc_mcontext.arm_fp, **sp = (void*)ucontext->uc_mcontext.arm_sp;
#else
#error "Unknown arch!!!"
#endif
	// Safe actions first, stack and memory may be corrupted
	#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined (__APPLE__)
		len = Q_snprintf( message, 4096, "Sys_Crash: signal %d, err %d with code %d at %p\n", signal, si->si_errno, si->si_code, si->si_addr );
	#else
		len = Q_snprintf( message, 4096, "Sys_Crash: signal %d, err %d with code %d at %p %p\n", signal, si->si_errno, si->si_code, si->si_addr, si->si_ptr );
	#endif
	write(2, message, len);
	// Flush buffers before writing directly to descriptors
	fflush( stdout );
	fflush( stderr );
	// Now get log fd and write trace directly to log
	logfd = Sys_LogFileNo();
	write( logfd, message, len );
	write( 2, "Stack backtrace:\n", 17 );
	write( logfd, "Stack backtrace:\n", 17 );
	strncpy(message + len, "Stack backtrace:\n", 4096 - len);
	len += 17;
	size_t pagesize = sysconf(_SC_PAGESIZE);
	do
	{
		int line = printframe( message + len, 4096 - len, ++i, pc);
		write( 2, message + len, line );
		write( logfd, message + len, line );
		len += line;
		//if( !dladdr(bp,0) ) break; // Only when bp is in module
		if( ( mprotect((char *)(((unsigned long) bp + (pagesize-1)) & ~(pagesize-1)), pagesize, PROT_READ | PROT_WRITE | PROT_EXEC ) == -1) &&
			( mprotect((char *)(((unsigned long) bp + (pagesize-1)) & ~(pagesize-1)), pagesize, PROT_READ | PROT_EXEC ) == -1) &&
			( mprotect((char *)(((unsigned long) bp + (pagesize-1)) & ~(pagesize-1)), pagesize, PROT_READ | PROT_WRITE ) == -1) &&
			( mprotect((char *)(((unsigned long) bp + (pagesize-1)) & ~(pagesize-1)), pagesize, PROT_READ ) == -1) )
			break;
		if( ( mprotect((char *)(((unsigned long) bp[0] + (pagesize-1)) & ~(pagesize-1)), pagesize, PROT_READ | PROT_WRITE | PROT_EXEC ) == -1) &&
			( mprotect((char *)(((unsigned long) bp[0] + (pagesize-1)) & ~(pagesize-1)), pagesize, PROT_READ | PROT_EXEC ) == -1) &&
			( mprotect((char *)(((unsigned long) bp[0] + (pagesize-1)) & ~(pagesize-1)), pagesize, PROT_READ | PROT_WRITE ) == -1) &&
			( mprotect((char *)(((unsigned long) bp[0] + (pagesize-1)) & ~(pagesize-1)), pagesize, PROT_READ ) == -1) )
			break;
		pc = bp[1];
		bp = (void**)bp[0];
	}
	while( bp && i < 128 );
	// Try to print stack
	write( 2, "Stack dump:\n", 12 );
	write( logfd, "Stack dump:\n", 12 );
	strncpy( message + len, "Stack dump:\n", 4096 - len );

	len += 12;
	if( ( mprotect((char *)(((unsigned long) sp + (pagesize-1)) & ~(pagesize-1)), pagesize, PROT_READ | PROT_WRITE | PROT_EXEC ) != -1) ||
			( mprotect((char *)(((unsigned long) sp + (pagesize-1)) & ~(pagesize-1)), pagesize, PROT_READ | PROT_EXEC ) != -1) ||
			( mprotect((char *)(((unsigned long) sp + (pagesize-1)) & ~(pagesize-1)), pagesize, PROT_READ | PROT_WRITE ) != -1) ||
			( mprotect((char *)(((unsigned long) sp + (pagesize-1)) & ~(pagesize-1)), pagesize, PROT_READ ) != -1) )
		for( i = 0; i < 32; i++ )
		{
			int line = printframe( message + len, 4096 - len, i, sp[i] );
			write( 2, message + len, line );
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
