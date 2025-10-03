// Copyright (C) 1999-2000 Id Software, Inc.
//

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"


/*
==============================================================================

PACKET FILTERING
 

You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

g_filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/

// extern	vmCvar_t	g_banIPs;
// extern	vmCvar_t	g_filterBan;


typedef struct ipFilter_s
{
	unsigned	mask;
	unsigned	compare;
} ipFilter_t;

#define	MAX_IPFILTERS	1024

static ipFilter_t	ipFilters[MAX_IPFILTERS];
static int			numIPFilters;

/*
=================
StringToFilter
=================
*/
static qboolean StringToFilter (char *s, unsigned *mask, unsigned *compare)
{
	char	num[128];
	int		i, j;
	byte	b[4];
	byte	m[4];
	
	for (i=0 ; i<4 ; i++)
	{
		b[i] = 0;
		m[i] = 0;
	}
	
	for (i=0 ; i<4 ; i++)
	{
		if (*s < '0' || *s > '9')
		{
			if (*s == '*') // 'match any'
			{
				// b[i] and m[i] to 0
				s++;
				if (!*s)
					break;
				s++;
				continue;
			}
			G_Printf( "Bad filter address: %s\n", s );
			return qfalse;
		}
		
		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi(num);
		m[i] = 255;

		if (!*s)
			break;
		s++;
	}
	
	*mask = *(int *)m;
	*compare = *(int *)b;
	
	return qtrue;
}

/*
=================
UpdateIPBans
=================
*/
#ifdef Q3_VM
static void UpdateIPBans (void)
{
	byte	b[4];
	byte	m[4];
	int		i,j;
	char	iplist_final[MAX_CVAR_VALUE_STRING];
	char	ip[64];

	*iplist_final = 0;
	for (i = 0 ; i < numIPFilters ; i++)
	{
		if (ipFilters[i].compare == 0xffffffff)
			continue;

		*(unsigned *)b = ipFilters[i].compare;
		*(unsigned *)m = ipFilters[i].mask;
		*ip = 0;
		for (j = 0 ; j < 4 ; j++)
		{
			if (m[j]!=255)
				Q_strcat(ip, sizeof(ip), "*");
			else
				Q_strcat(ip, sizeof(ip), va("%i", b[j]));
			Q_strcat(ip, sizeof(ip), (j<3) ? "." : " ");
		}		
		if (strlen(iplist_final)+strlen(ip) < MAX_CVAR_VALUE_STRING)
		{
			Q_strcat( iplist_final, sizeof(iplist_final), ip);
		}
		else
		{
			Com_Printf("g_banIPs overflowed at MAX_CVAR_VALUE_STRING\n");
			break;
		}
	}

	trap_Cvar_Set( "g_banIPs", iplist_final );
}
#endif

/*
=================
G_FilterPacket
=================
*/
char *G_FilterPacket (char *from)
{
#ifdef Q3_VM
	int		i;
	unsigned	in;
	byte m[4];
	char *p;

	i = 0;
	p = from;
	while (*p && i < 4) {
		m[i] = 0;
		while (*p >= '0' && *p <= '9') {
			m[i] = m[i]*10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':')
			break;
		i++, p++;
	}
	
	in = *(unsigned *)m;

	for (i=0 ; i<numIPFilters ; i++)
		if ( (in & ipFilters[i].mask) == ipFilters[i].compare)
			return g_filterBan.integer != 0 ? "You are banned from this server." : "";

	return g_filterBan.integer == 0 ? "You are not allowed on this server." : "";
#else
	static char reason[255];

	int i;
	int ip;
	byte octets[4];
	char *s;
	char *err;
	int rows;
	int cols;
	char **res;

	err = NULL;
	rows = -1;
	cols = -1;

	for ( i = 0, s = from; *s && i < 4; i++, s++ ) {
		octets[i] = 0;

		while ( *s >= '0' && *s <= '9' ) {
			octets[i] = octets[i] * 10 + ( *s - '0' );
			s++;
		}

		if ( *s == '\0' || *s == ':' ) {
			break;
		}
	}

	ip = *(int *)octets;

	memset( reason, 0, sizeof( reason ) );

	if ( sqlite_get_table_printf( level.db, "SELECT IPMask,Compare,Reason FROM %s WHERE (IPMask & %d) = Compare", &res, &rows, &cols, &err,
	                              tblNames[TBL_BANS], ip ) ) {
		G_LogPrintf( "<DB> Query error: %s\n", err );
	}

	if ( rows && g_filterBan.integer ) {
		if ( rows > 1 ) {
			G_LogPrintf( "<DB> Warning: Multiple entries for IP.\n" );
		}
		Com_sprintf( reason, sizeof( reason ), "Banned: %s", res[cols + 2] );
	}

	if ( !rows && !g_filterBan.integer ) {
		Com_sprintf( reason, sizeof( reason ), "Your IP is not on the allow list." );
	}

	sqlite_free_table( res );

	return reason;
#endif
}

/*
=================
AddIP
=================
*/
static void AddIP( char *ip, char *reason )
{
#ifdef Q3_VM
	int		i;

	for (i = 0 ; i < numIPFilters ; i++)
		if (ipFilters[i].compare == 0xffffffff)
			break;		// free spot
	if (i == numIPFilters)
	{
		if (numIPFilters == MAX_IPFILTERS)
		{
			G_Printf ("IP filter list is full\n");
			return;
		}
		numIPFilters++;
	}
	
	if (!StringToFilter (ip, &ipFilters[i].mask, &ipFilters[i].compare))
		ipFilters[i].compare = 0xffffffffu;

	UpdateIPBans();
#else
	unsigned int mask;
	unsigned int compare;
	char *err;

	err = NULL;

	if ( !StringToFilter( ip, &mask, &compare ) ) {
		G_LogPrintf( "StringToFilter failed.\n" );
		return;
	}

	if ( sqlite_exec_printf( level.db, "INSERT INTO %s (IP,IPMask,Compare,Reason) VALUES('%s', %d, %d, '%s')", NULL, NULL, &err,
	                         tblNames[TBL_BANS], ip, mask, compare, reason ) ) {
		G_LogPrintf( "<DB> Query error: %s.\n", err );
		return;
	}
#endif
}

/*
=================
G_ProcessIPBans
=================
*/
#ifdef Q3_VM
void G_ProcessIPBans(void) 
{
	char *s, *t;
	char		str[MAX_TOKEN_CHARS];

	Q_strncpyz( str, g_banIPs.string, sizeof(str) );

	for (t = s = g_banIPs.string; *t; /* */ ) {
		s = strchr(s, ' ');
		if (!s)
			break;
		while (*s == ' ')
			*s++ = 0;
		if (*t)
			AddIP( t, NULL );
		t = s;
	}
}
#endif


/*
=================
Svcmd_AddIP_f
=================
*/
void Svcmd_AddIP_f (void)
{
	char	ip[MAX_TOKEN_CHARS];
	char	reason[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 3 ) {
		G_Printf("Usage:  addip <ip-mask> <reason>\n");
		return;
	}

	trap_Argv( 1, ip, sizeof( ip ) );
	trap_Argv( 2, reason, sizeof( reason ) );

	AddIP( ip, reason );
}

/*
=================
Svcmd_ListIP_f
=================
*/
void Svcmd_ListIP_f( void ) {
#ifdef Q3_VM
	trap_SendConsoleCommand( EXEC_NOW, "g_banIPs\n" );
#else
	char *err;
	int rows;
	int cols;
	int i;
	char **res;

	err = NULL;
	rows = -1;
	cols = -1;

	if ( sqlite_get_table_printf( level.db, "SELECT IP,Reason FROM %s", &res, &rows, &cols, &err, tblNames[TBL_BANS] ) ) {
		G_LogPrintf( "<DB> Query error: %s\n", err );
	}

	G_Printf( "%d bans found:\n", rows );

	for ( i = 0; i < rows; i++ ) {
		G_Printf( "%s \"%s\"\n", res[cols * i + cols], res[cols * i + cols + 1] );
	}
#endif
}

/*
=================
Svcmd_RemoveIP_f
=================
*/
void Svcmd_RemoveIP_f (void)
{
#ifdef Q3_VM
	ipFilter_t	f;
	int			i;
	char		ip[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		G_Printf("Usage:  sv removeip <ip-mask>\n");
		return;
	}

	trap_Argv( 1, ip, sizeof( ip ) );

	if (!StringToFilter (ip, &f.mask, &f.compare))
		return;

	for (i=0 ; i<numIPFilters ; i++) {
		if (ipFilters[i].mask == f.mask	&&
			ipFilters[i].compare == f.compare) {
			ipFilters[i].compare = 0xffffffffu;
			G_Printf ("Removed.\n");

			UpdateIPBans();
			return;
		}
	}

	G_Printf ( "Didn't find %s.\n", ip );
#else
	unsigned int mask;
	unsigned int compare;
	char ip[MAX_TOKEN_CHARS];
	char *err;

	err = NULL;

	if ( trap_Argc() < 2 ) {
		G_Printf( "Usage:  removeip <ip-mask>\n" );
		return;
	}

	trap_Argv( 1, ip, sizeof( ip ) );

	if ( !StringToFilter( ip, &mask, &compare ) ) {
		return;
	}

	if ( sqlite_exec_printf( level.db, "DELETE FROM %s WHERE IPMask = %d AND Compare = %d", NULL, NULL, &err, tblNames[TBL_BANS], mask,
	                         compare ) ) {
		G_LogPrintf( "<DB> Query error: %s\n", err );
	}

	// FIXME Should be "Removed"
	if ( sqlite_changes( level.db ) ) {
		G_Printf( "Remoevd.\n" );
		return;
	}

	G_Printf( "Could not find %s.\n", ip );
#endif
}

/*
===================
Svcmd_EntityList_f
===================
*/
void	Svcmd_EntityList_f (void) {
	int			e;
	gentity_t		*check;

	check = g_entities+1;
	for (e = 1; e < level.num_entities ; e++, check++) {
		if ( !check->inuse ) {
			continue;
		}
		G_Printf("%3i:", e);
		switch ( check->s.eType ) {
		case ET_GENERAL:
			G_Printf("ET_GENERAL          ");
			break;
		case ET_PLAYER:
			G_Printf("ET_PLAYER           ");
			break;
		case ET_ITEM:
			G_Printf("ET_ITEM             ");
			break;
		case ET_MISSILE:
			G_Printf("ET_MISSILE          ");
			break;
		case ET_MOVER:
			G_Printf("ET_MOVER            ");
			break;
		case ET_BEAM:
			G_Printf("ET_BEAM             ");
			break;
		case ET_PORTAL:
			G_Printf("ET_PORTAL           ");
			break;
		case ET_SPEAKER:
			G_Printf("ET_SPEAKER          ");
			break;
		case ET_PUSH_TRIGGER:
			G_Printf("ET_PUSH_TRIGGER     ");
			break;
		case ET_TELEPORT_TRIGGER:
			G_Printf("ET_TELEPORT_TRIGGER ");
			break;
		case ET_INVISIBLE:
			G_Printf("ET_INVISIBLE        ");
			break;
		case ET_GRAPPLE:
			G_Printf("ET_GRAPPLE          ");
			break;
		default:
			G_Printf("%3i                 ", check->s.eType);
			break;
		}

		if ( check->classname ) {
			G_Printf("%s", check->classname);
		}
		G_Printf("\n");
	}
}

gclient_t	*ClientForString( const char *s ) {
	gclient_t	*cl;
	int			i;
	int			idnum;

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' ) {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			Com_Printf( "Bad client slot: %i\n", idnum );
			return NULL;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			G_Printf( "Client %i is not connected\n", idnum );
			return NULL;
		}
		return cl;
	}

	// check for a name match
	for ( i=0 ; i < level.maxclients ; i++ ) {
		cl = &level.clients[i];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !Q_stricmp( cl->pers.netname, s ) ) {
			return cl;
		}
	}

	G_Printf( "User %s is not on the server\n", s );

	return NULL;
}

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
void	Svcmd_ForceTeam_f( void ) {
	gclient_t	*cl;
	char		str[MAX_TOKEN_CHARS];
	gentity_t	*ent;

	// find the player
	trap_Argv( 1, str, sizeof( str ) );
	cl = ClientForString( str );
	if ( !cl ) {
		return;
	}

	// set the team
	trap_Argv( 2, str, sizeof( str ) );
	ent = &g_entities[cl - level.clients];

	if ( !Q_stricmp( str, "spectator" ) || !Q_stricmp( str, "s" ) ) {
		G_LeaveTeam( ent );
		if ( ent && ent->client ) {
			ent->client->ps.persistant[PERS_SCORE] = 0;
		}
		return;
	}

	if ( level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.type == AT_CLANARENA ) {
		if ( !Q_stricmp( str, "red" ) || !Q_stricmp( str, "r" ) ) {
			G_LeaveTeam( ent );
			G_JoinTeam( ent, 1 );
			return;
		} else if ( !Q_stricmp( str, "blue" ) || !Q_stricmp( str, "b" ) ) {
			G_LeaveTeam( ent );
			G_JoinTeam( ent, 2 );
			return;
		}
	} else {
		G_Printf( "Can't force players to other than spectator in arenas that are not clan arena.\n" );
	}
}

/*
===================
Svcmd_Adm_CP_f
===================
*/
void Svcmd_Adm_CP_f() {
	int clientNum;
	char arg[MAX_TOKEN_CHARS];

	if ( trap_Argc() == 3 ) {
		trap_Argv( 1, arg, sizeof( arg ) );
		clientNum = atoi( arg );
		trap_Argv( 2, arg, sizeof( arg ) );
	} else {
		clientNum = -1;
		trap_Argv( 1, arg, sizeof( arg ) );
	}

	trap_SendServerCommand( clientNum, va( "cpi 2 \"%s\"", arg ) );
}

/*
===================
G_Pause
===================
*/
void G_Pause( int arenaNum ) {
	level.arenas[arenaNum].resumeState = level.arenas[arenaNum].state;
	level.arenas[arenaNum].state = ROUND_PAUSED;
	level.arenas[arenaNum].pauseTime = level.time;
	level.arenas[arenaNum].paused = 1;
	set_arena_configstring( arenaNum );
}

/*
===================
G_UnPause
===================
*/
void G_UnPause( int arenaNum ) {
	level.arenas[arenaNum].state = ROUND_UNPAUSED;
	level.arenas[arenaNum].nextCheckFrame = 0;
}

/*
===================
Svcmd_Pause_f
===================
*/
void Svcmd_Pause_f() {
	int argc;
	char arg[MAX_TOKEN_CHARS];
	int arenaNum;

	argc = trap_Argc();

	if ( argc == 2 ) {
		trap_Argv( 1, arg, sizeof( arg ) );
		arenaNum = atoi( arg );

		// FIXME should be >= MAX_ARENAS
		if ( arenaNum < 1 || arenaNum > 1024 ) {
			Com_Printf( "Invalid arena: %d.\n", arenaNum );
			return;
		}

		if ( level.arenas[arenaNum].state != ROUND_RUNNING ) {
			Com_Printf( "You can't pause right now.\n" );
			return;
		}

		if ( !level.arenas[arenaNum].paused ) {
			G_Pause( arenaNum );

			Com_Printf( "Game in arena %d paused.\n", arenaNum );
		} else {
			Com_Printf( "Game in arena %d is already paused.\n", arenaNum );
		}
	} else {
		Com_Printf( "Invalid numer of arguments: pause <arena>\n" );
	}
}

/*
===================
Svcmd_UnPause_f
===================
*/
void Svcmd_UnPause_f() {
	int argc;
	char arg[MAX_TOKEN_CHARS];
	int arenaNum;

	argc = trap_Argc();

	if ( argc == 2 ) {
		trap_Argv( 1, arg, sizeof( arg ) );
		arenaNum = atoi( arg );

		/* FIXME should be >= MAX_ARENAS */
		if ( arenaNum < 1 || arenaNum > 1024 ) {
			Com_Printf( "Invalid arena: %d.\n", arenaNum );
			return;
		}

		if ( level.arenas[arenaNum].paused ) {
			G_UnPause( arenaNum );

			Com_Printf( "Game in arena %d unpaused.\n", arenaNum );
		} else {
			Com_Printf( "Game in arena %d is not paused.\n", arenaNum );
		}
	} else {
		Com_Printf( "Invalid numer of arguments: unpause <arena>\n" );
	}
}

/*
=================
ConsoleCommand

=================
*/
char	*ConcatArgs( int start );

qboolean ConsoleCommand( void ) {
	char		cmd[MAX_TOKEN_CHARS];
	gentity_t	*ent;

#ifndef Q3_VM
	(void)ipFilters;
	(void)numIPFilters;
#endif

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( !Q_stricmp( cmd, "entitylist" ) ) {
		Svcmd_EntityList_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "forceteam" ) ) {
		Svcmd_ForceTeam_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "game_memory" ) ) {
		Svcmd_GameMem_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "addbot" ) ) {
		Svcmd_AddBot_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "botlist" ) ) {
		Svcmd_BotList_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "abort_podium" ) ) {
		Svcmd_AbortPodium_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "addip" ) ) {
		Svcmd_AddIP_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "removeip" ) ) {
		Svcmd_RemoveIP_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "listip" ) ) {
		Svcmd_ListIP_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "adm_cp" ) ) {
		Svcmd_Adm_CP_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "pause" ) ) {
		Svcmd_Pause_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "unpause" ) ) {
		Svcmd_UnPause_f();
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "movenow" ) ) {
		ent = NULL;

		while ( ( ent = G_Find( ent, FOFS( classname ), "misc_portal_camera" ) ) ) {
			G_Printf( "found one -- %f %f %f\n", ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] );

			ent->s.origin[0] += 20;
			ent->s.origin[1] += 20;
		}

		while ( ( ent = G_Find( ent, FOFS( classname ), "misc_portal_surface" ) ) ) {
			G_Printf( "found one -- %f %f %f\n", ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] );

			ent->think = locateCamera;
			ent->nextthink = level.time + 100;
		}

		return qtrue;
	}

	if ( g_dedicated.integer ) {
		if ( Q_stricmp( cmd, "say" ) == 0 ) {
			trap_SendServerCommand( -1, va( "print \"server: %s\"", ConcatArgs( 1 ) ) );
			return qtrue;
		}

		// everything else will also be printed as a say command
		trap_SendServerCommand( -1, va( "print \"server: %s\"", ConcatArgs( 0 ) ) );
		return qtrue;
	}

	return qfalse;
}
