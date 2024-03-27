/*
masterlist.c - multi-master list
Copyright (C) 2018 mittorn

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
#include "server.h"

#define HEARTBEAT_SECONDS ( sv_nat->integer ? 60.0f : 300.0f ) // 1 or 5 minutes

typedef struct master_s
{
	struct master_s *next;
	qboolean sent;
	qboolean save;
	string address;
	netadr_t adr;

	uint32_t heartbeat_challenge;
	double last_heartbeat;
} master_t;

struct masterlist_s
{
	master_t *list;
	qboolean modified;
} ml;

/*
========================
NET_GetMasterHostByName
========================
*/
static int NET_GetMasterHostByName( master_t *m )
{
	int res = NET_StringToAdrNB( m->address, &m->adr );

	if( res )
		return res;

	m->adr.type = 0;
	if( !res )
		MsgDev( D_INFO, "Can't resolve adr: %s\n", m->address );

	return res;
}

/*
========================
NET_GetMasterFromAdr

Returns a pointer to a master
========================
*/
static master_t *NET_GetMasterFromAdr( netadr_t from )
{
	master_t *master;

	for( master = ml.list; master; master = master->next )
	{
		if( NET_CompareAdr( master->adr, from ) )
		{
			return master;
		}
	}

	return NULL;
}

/*
========================
NET_IsFromMasters

Determines if source matches
one of the master servers
========================
*/
qboolean NET_IsFromMasters( netadr_t from )
{
	return NET_GetMasterFromAdr( from ) != NULL;
}

/*
========================
NET_AnnounceToMaster

Announces server to master
========================
*/
static void NET_AnnounceToMaster( master_t *m )
{
	sizebuf_t msg;
	char buf[16];

	m->heartbeat_challenge = Com_RandomLong( 0, INT_MAX );

	BF_Init( &msg, "Master Join", buf, sizeof( buf ) );
	BF_WriteBytes( &msg, "q\xFF", 2 );
	BF_WriteUBitLong( &msg, m->heartbeat_challenge, sizeof( m->heartbeat_challenge ) << 3 );

	NET_SendPacket( NS_SERVER, BF_GetNumBytesWritten( &msg ), BF_GetData( &msg ), m->adr );

	if( sv_master_verbose_heartbeats->value )
	{
		MsgDev( D_NOTE, "sent heartbeat to %s (%s, 0x%x)\n", m->address, NET_AdrToString( m->adr ), m->heartbeat_challenge );
	}
}

/*
========================
NET_GetMaster
========================
*/
qboolean NET_GetMaster( netadr_t from, uint32_t *challenge, double *last_heartbeat )
{
	master_t *m;

	m = NET_GetMasterFromAdr( from );

	if( m )
	{
		*challenge = m->heartbeat_challenge;
		*last_heartbeat = m->last_heartbeat;
	}

	return m != NULL;
}

/*
========================
NET_SendToMasters

Send request to all masterservers list
return true if would block
========================
*/
qboolean NET_SendToMasters( netsrc_t sock, size_t len, const void *data )
{
	master_t *list;
	qboolean wait = false;

	for( list = ml.list; list; list = list->next )
	{
		if( list->sent )
			continue;

		switch( NET_GetMasterHostByName( list ) )
		{
			case 0:
			{
				list->sent = true;
			    break;
			}
			case 2:
			{
				list->sent = false;
				wait = true;
				break;
			}
			case 1:
			{
				list->sent = true;
				NET_SendPacket( sock, len, data, list->adr );
			    break;
			}
		}
	}

	if( !wait )
	{
		// reset sent state
		for( list = ml.list; list; list = list->next )
			list->sent = false;
	}

	return wait;
}

/*
================
NET_MasterHeartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
void NET_MasterHeartbeat( void )
{
	master_t *m;

	if( !public_server->integer || sv_maxclients->integer == 1 || sv_lan->integer == 1 )
		return; // only public servers send heartbeats

	for( m = ml.list; m; m = m->next )
	{
		// not time to send yet
		if( ( host.realtime - m->last_heartbeat ) < HEARTBEAT_SECONDS )
			continue;

		switch( NET_GetMasterHostByName( m ) )
		{
			case 0:
			{
			    m->last_heartbeat = host.realtime; // try to resolve again on next heartbeat
				break;
			}
			case 2:
			{
			    m->last_heartbeat = MAX_HEARTBEAT; // retry on next frame
			    if( sv_master_verbose_heartbeats->value )
				    MsgDev( D_NOTE, "delay heartbeat to next frame until %s resolves\n", m->address );
				break;
			}
			case 1:
			{
			    m->last_heartbeat = host.realtime;
			    NET_AnnounceToMaster( m );
				break;
			}
		}
	}
}

/*
========================
NET_ForceHeartbeat
========================
*/
void NET_ForceHeartbeat( void )
{
	master_t *m;

	for( m = ml.list; m; m = m->next )
		m->last_heartbeat = MAX_HEARTBEAT;
}

/*
========================
NET_AddMaster

Add master to the list
========================
*/
static void NET_AddMaster( char *addr, qboolean save )
{
	master_t *master, *last;

	for( last = ml.list; last && last->next; last = last->next )
	{
		if( !Q_strcmp( last->address, addr ) ) // already exists
			return;
	}

	master = Mem_Alloc( host.mempool, sizeof( master_t ) );
	Q_strncpy( master->address, addr, MAX_STRING );
	master->sent = false;
	master->save = save;
	master->next = NULL;
	master->adr.type = 0;

	// link in
	if( last )
		last->next = master;
	else
		ml.list = master;
}

static void NET_AddMaster_f( void )
{
	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: addmaster <address>\n");
		return;
	}

	NET_AddMaster( Cmd_Argv( 1 ), true ); // save them into config
	ml.modified = true; // save config
}

/*
========================
NET_ClearMasters

Clear master list
========================
*/
void NET_ClearMasters( void )
{
	while( ml.list )
	{
		master_t *prev = ml.list;
		ml.list = ml.list->next;
		Mem_Free( prev );
	}
}

/*
========================
NET_ClearMasters_f

Clears master list
========================
*/
static void NET_ClearMasters_f( void )
{
	NET_ClearMasters();
}

/*
========================
NET_ListMasters_f

Display current master linked list
========================
*/
static void NET_ListMasters_f( void )
{
	master_t *list;
	int i;

	Msg( "Master servers\n=============\n" );


	for( i = 1, list = ml.list; list; i++, list = list->next )
	{
		Msg( "%d\t%s\n", i, list->address );
	}
}

/*
========================
NET_LoadMasters

Load master server list from xashcomm.lst
========================
*/
void NET_LoadMasters( )
{
	char *afile, *pfile;
	char token[4096];

	pfile = afile = (char*)FS_LoadFile( "xashcomm.lst", NULL, true );

	if( !afile ) // file doesn't exist yet
	{
		MsgDev( D_INFO, "Cannot load xashcomm.lst\n" );
		return;
	}

	// format: master <addr>\n
	while( ( pfile = COM_ParseFile( pfile, token ) ) )
	{
		if( !Q_strcmp( token, "master" ) ) // load addr
		{
			pfile = COM_ParseFile( pfile, token );

			NET_AddMaster( token, true );
		}
	}

	Mem_Free( afile );

	ml.modified = false;
}

/*
========================
NET_SaveMasters

Save master server list to xashcomm.lst, except for default
========================
*/
void NET_SaveMasters( )
{
	file_t *f;
	master_t *m;

	if( !ml.modified )
	{
		MsgDev( D_NOTE, "Master server list not changed\n" );
		return;
	}

	f = FS_Open( "xashcomm.lst", "w", true );

	if( !f )
	{
		MsgDev( D_ERROR, "Couldn't write xashcomm.lst\n" );
		return;
	}

	for( m = ml.list; m; m = m->next )
	{
		if( m->save )
			FS_Printf( f, "master %s\n", m->address );
	}

	FS_Close( f );
}

/*
=================
NET_MasterShutdown

Informs all masters that this server is going down
(ignored by master servers in current implementation)
=================
*/
void NET_MasterShutdown( void )
{
	NET_Config( true, false ); // allow remote
	while( NET_SendToMasters( NS_SERVER, 2, "\x62\x0A" ) );
}

/*
========================
NET_InitMasters

Initialize master server list
========================
*/
void NET_InitMasters()
{
	Cmd_AddRestrictedCommand( "addmaster", NET_AddMaster_f, "add address to masterserver list" );
	Cmd_AddRestrictedCommand( "clearmasters", NET_ClearMasters_f, "clear masterserver list" );
	Cmd_AddCommand( "listmasters", NET_ListMasters_f, "list masterservers" );

	// keep main master always there
	NET_AddMaster( DEFAULT_PRIMARY_MASTER, false );
	NET_AddMaster( FWGS_PRIMARY_MASTER, false );

	NET_LoadMasters( );
}
