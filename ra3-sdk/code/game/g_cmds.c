// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

// FIXME temporary stubs
int Target_GetLocation( gentity_t *ent, char *out, int outSize );
//

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	char entry[1024];
	char string[1400];
	int stringLength;
	int i;
	int n;
	int scoreCount;
	int scoreIndex;
	gclient_t *cl;
	int numSorted;
	int unknown;
	int arenaNum;
	int ping;
	int top;
	int j;

	// send the latest information on all clients
	arenaNum = ent->client->ps.persistant[PERS_ARENA];

	string[0] = 0;
	stringLength = 0;

	// FIXME unused
	unknown = 0;

	numSorted = level.numConnectedClients;

	if (numSorted > 32) {
		numSorted = 32;
	}

	// FIXME having both i and scoreIndex is unnecessary
	for ( i = 0, scoreCount = 0, scoreIndex = 0; scoreIndex < numSorted; i++ ) {
		top = 0;

		cl = &level.clients[level.sortedClients[i]];
		scoreIndex++;

		if ( cl->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		scoreCount++;

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		for (j = WP_NUM_WEAPONS - 1; j >= 0; j--) {
			top <<= 1;
			top |= cl->pers.weaponRank[j] == 0;
		}

		Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i %i %i", level.sortedClients[i],
					 cl->ps.persistant[PERS_SCORE], ping, ( level.time - cl->pers.enterTime ) / 60000, cl->pers.pl,
					 cl->pers.damage, top );
		n = strlen(entry);

		if (stringLength + n > 1024) {
			break;
		}

		strcpy (string + stringLength, entry);
		stringLength += n;
	}

	update_team_scores( ent->client->ps.persistant[PERS_ARENA] );

	trap_SendServerCommand( ent - g_entities, va( "scores %i %i %i%s", scoreCount, level.teamScores[TEAM_RED],
	                                              level.teamScores[TEAM_BLUE], string ) );
}


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	DeathmatchScoreboardMessage( ent );
}



/*
==================
CheatsOk
==================
*/
qboolean	CheatsOk( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"You must be alive to use this command.\n\""));
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString( char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;		// skip color code
			continue;
		}
		if ( *in < 32 ) {
			in++;
			continue;
		}
		*out++ = tolower( *in++ );
	}

	*out = 0;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t	*cl;
	int			idnum;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			trap_SendServerCommand( to-g_entities, va("print \"Bad client slot: %i\n\"", idnum));
			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap_SendServerCommand( to-g_entities, va("print \"Client %i is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString( s, s2 );
	for ( idnum=0,cl=level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) {
			return idnum;
		}
	}

	trap_SendServerCommand( to-g_entities, va("print \"User %s is not on the server\n\"", s));
	return -1;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (gentity_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			i;
	qboolean	give_all;
	gentity_t		*it_ent;
	trace_t		trace;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	name = ConcatArgs( 1 );

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all || Q_stricmp( name, "health") == 0)
	{
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_NUM_WEAPONS) - 1 - 
			( 1 << WP_GRAPPLING_HOOK ) - ( 1 << WP_NONE );
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
			ent->client->ps.ammo[i] = 999;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		ent->client->ps.stats[STAT_ARMOR] = 200;

		if (!give_all)
			return;
	}

	if (Q_stricmp(name, "excellent") == 0) {
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "impressive") == 0) {
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "gauntletaward") == 0) {
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem (name);
		if (!it) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;
		G_SpawnItem (it_ent, it);
		FinishSpawningItem(it_ent );
		memset( &trace, 0, sizeof( trace ) );
		Touch_Item (it_ent, ent, &trace);
		if (it_ent->inuse) {
			G_FreeEntity( it_ent );
		}
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (gentity_t *ent)
{
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent ) {
	if ( !CheatsOk( ent ) ) {
		return;
	}

	// doesn't work in single player
	if ( g_gametype.integer != 0 ) {
		trap_SendServerCommand( ent-g_entities, 
			"print \"Must be in g_gametype 0 for levelshot\n\"" );
		return;
	}

	BeginIntermission();
	trap_SendServerCommand( ent-g_entities, "clientLevelShot" );
}


/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}
	if ( ent->health <= 0 ) {
		return;
	}
	if ( ent->client->ps.pm_flags & PMF_UNKNOWN ) {
		return;
	}
	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die (ent, ent, ent, 100000, MOD_SUICIDE);
}

/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	if ( client->sess.sessionTeam == TEAM_RED ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " joined the red team.\n\"",
			client->pers.netname) );
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " joined the blue team.\n\"",
		client->pers.netname));
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " joined the spectators.\n\"",
		client->pers.netname));
	} else if ( client->sess.sessionTeam == TEAM_FREE ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " joined the battle.\n\"",
		client->pers.netname));
	}
}

/*
=================
SetTeam
=================
*/
void SetTeam( gentity_t *ent, char *s ) {
	int					team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;

	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( !Q_stricmp( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if ( !Q_stricmp( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	} else if ( g_gametype.integer >= GT_TEAM ) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
		}

		if ( g_teamForceBalance.integer ) {
			int		counts[TEAM_NUM_TEAMS];

			counts[TEAM_BLUE] = TeamCount( ent->client->ps.clientNum, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( ent->client->ps.clientNum, TEAM_RED );

			// We allow a spread of two
			if ( team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1 ) {
				trap_SendServerCommand( ent->client->ps.clientNum, 
					"cp \"Red team has too many players.\n\"" );
				return; // ignore the request
			}
			if ( team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1 ) {
				trap_SendServerCommand( ent->client->ps.clientNum, 
					"cp \"Blue team has too many players.\n\"" );
				return; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}

	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

	// override decision if limiting the players
	if ( g_gametype.integer == GT_TOURNAMENT
		&& level.numNonSpectatorClients >= 2 ) {
		team = TEAM_SPECTATOR;
	} else if ( g_maxGameClients.integer > 0 && 
		level.numNonSpectatorClients >= g_maxGameClients.integer ) {
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	oldTeam = client->sess.sessionTeam;
	if ( team == oldTeam && team != TEAM_SPECTATOR ) {
		return;
	}

	//
	// execute the team change
	//

	// if the player was dead leave the body
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		CopyToBodyQue(ent);
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if ( oldTeam != TEAM_SPECTATOR ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die (ent, ent, ent, 100000, MOD_SUICIDE);

	}
	// they go to the end of the line for tournements
	if ( team == TEAM_SPECTATOR ) {
		client->sess.spectatorTime = level.time;
	}

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	BroadcastTeamChange( client, oldTeam );

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );

	ClientBegin( clientNum );
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t *ent ) {
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;

	ent->client->ps.stats[STAT_SPECTATOR] = SPECTATOR_NOT;
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
}


/*
==================
G_Say
==================
*/
#define	MAX_SAY_TEXT	150

#define SAY_WORLD 0
#define SAY_TEAM  1
#define SAY_TELL  2
#define SAY_ARENA 3

static void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message ) {
	if ( !other ) {
		return;
	}

	if ( !other->inuse ) {
		return;
	}

	if ( !other->client ) {
		return;
	}

	if ( mode == SAY_TEAM && ( OnSameTeam( ent, other ) == qfalse ||
	                           ent->client->sess.sessionTeam != other->client->sess.sessionTeam ) ) {
		return;
	}

	if ( mode == SAY_ARENA && ent->client->ps.persistant[PERS_ARENA] != other->client->ps.persistant[PERS_ARENA] ) {
		return;
	}

	if ( g_gametype.integer == GT_TOURNAMENT && other->client->sess.sessionTeam == TEAM_FREE &&
	     ent->client->sess.sessionTeam != TEAM_FREE ) {
		return;
	}

	trap_SendServerCommand( other - g_entities,
	                        va( "%s \"%s%c%c%s\"", mode == SAY_TEAM ? "tchat" : "chat", name, Q_COLOR_ESCAPE, color, message ) );
}

void G_Say( gentity_t *ent, gentity_t *target, int mode, char *chatText ) {
	int			i;
	gentity_t	*other;
	int			color;
	char		name[64];
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	char		location[64];
	char		*p;
	int			floodLines;
	int			floodSeconds;
	int			floodLimit;
	int			timeSinceFloodReset;

	if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM ) {
		mode = SAY_WORLD;
	}

	if ( ent->client->ps.persistant[PERS_ARENA] == 0 ) {
		mode = SAY_WORLD;
	}

	// check flood protection
	sscanf( g_chatFlood.string, "%d:%d:%d", &floodLines, &floodSeconds, &floodLimit );

	if ( floodLines && floodSeconds &&
	     ( level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.competitionmode == 0 || mode != SAY_TEAM ) ) {
		timeSinceFloodReset = level.time - ent->client->pers.floodResetTime;

		if ( timeSinceFloodReset <= floodSeconds * 1000 && ent->client->pers.lineCount >= floodLines ) {
			trap_SendServerCommand(
				ent - g_entities,
				va( "print \"Flood protection active, you can\'t say more than %d lines in %d seconds.\n\"",
			        floodLines, floodSeconds ) );

			if ( floodLimit > 0 ) {
				if ( ent->client->pers.floodReset ) {
					ent->client->pers.floodCount++;
					ent->client->pers.floodReset = 0;
				}

				if ( ent->client->pers.floodCount >= floodLimit ) {
					trap_SendServerCommand( ent - g_entities, "print \"You were dropped due to flood protection\n\"" );
					trap_DropClient( ent - g_entities,
					                 va( "dropped. Flood protection trigger limit reached (%d).", floodLimit ) );
				}
			}

			return;
		}

		if ( level.time >= ( ent->client->pers.floodResetTime + floodSeconds * 1000 ) ) {
			ent->client->pers.lineCount = 0;
			ent->client->pers.floodResetTime = level.time;
			ent->client->pers.floodReset = 1;
		}

		ent->client->pers.lineCount++;
	}

	// FIXME should do this after copying to text, so chatText can remain a const pointer
	// trim trailing unescaped color
	p = chatText + strlen(chatText) - 1;
	while (*p == '^') {
		p--;
	}
	*++p = 0;

	switch ( mode ) {
	default:
	case SAY_WORLD:
		G_LogPrintf( "say_world: %d %d: %s: %s\n", ent->client->ps.clientNum, ent->client->ps.persistant[PERS_ARENA],
		             ent->client->pers.netname, chatText );
		Com_sprintf( name, sizeof( name ), "%c%c[world]%c%c %s%c%c: ", Q_COLOR_ESCAPE, COLOR_YELLOW, Q_COLOR_ESCAPE,
		             COLOR_WHITE, ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;

	case SAY_ARENA:
		G_LogPrintf( "say: %d %d: %s: %s\n", ent->client->ps.clientNum, ent->client->ps.persistant[PERS_ARENA],
		             ent->client->pers.netname, chatText );
		Com_sprintf( name, sizeof( name ), "%s%c%c: ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;

	case SAY_TEAM:
		G_LogPrintf( "sayteam: %d %d: %s: %s\n", ent->client->ps.clientNum, ent->client->ps.persistant[PERS_ARENA],
		             ent->client->pers.netname, chatText );
		if ( Team_GetLocationMsg( ent, location, sizeof( location ) ) ) {
			Com_sprintf( name, sizeof( name ), "(%s%c%c) (%s): ", ent->client->pers.netname, Q_COLOR_ESCAPE,
			             COLOR_WHITE, location );
		} else {
			Com_sprintf( name, sizeof( name ), "(%s%c%c): ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		}
		color = COLOR_CYAN;
		break;

	case SAY_TELL:
		if ( target && g_gametype.integer >= GT_TEAM &&
		     target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
		     Team_GetLocationMsg( ent, location, sizeof( location ) ) ) {
			Com_sprintf( name, sizeof( name ), "[%s%c%c] (%s): ", ent->client->pers.netname, Q_COLOR_ESCAPE,
			             COLOR_WHITE, location );
		} else {
			Com_sprintf( name, sizeof( name ), "[%s%c%c]: ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		}
		color = COLOR_MAGENTA;
		break;
	}

	Q_strncpyz( text, chatText, sizeof( text ) );

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text );
	} else {
		// send it to all the apropriate clients
		for ( i = 0; i < level.maxclients; i++ ) {
			other = &g_entities[i];

			if ( ent->client->pers.ignore[i] != 1 ) {
				G_SayTo( ent, other, mode, color, name, text );
			}
		}
	}
}

/*
==================
Cmd_Ignore_f
==================
*/
static void Cmd_Ignore_f( gentity_t *ent ) {
	char	arg[MAX_TOKEN_CHARS];
	int		clientNum;
	int		i;
	int		numIgnored;

	numIgnored = 0;

	if ( trap_Argc() < 2 ) {
		trap_SendServerCommand( ent - g_entities,
		                        "print \"Usage: ignore <client number>. You are currently ignoring:\n\"" );

		for ( i = 0; i < 64; i++ ) {
			if ( level.clients[i].pers.connected == CON_CONNECTED ) {
				if ( level.clients[i].pers.ignore[ent - g_entities] == 1 ) {
					trap_SendServerCommand( ent - g_entities,
											va( "print \"  %d: %s\n\"", i, g_entities[i].client->pers.netname ) );
					numIgnored++;
				}
			}
		}

		if ( !numIgnored ) {
			trap_SendServerCommand( ent - g_entities, "print \"  None\n\"" );
		}

		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	clientNum = atoi( arg );

	// FIXME should be clientNum >= MAX_CLIENTS
	if ( clientNum < 0 || clientNum > MAX_CLIENTS || !g_entities[clientNum].inuse ||
	     level.clients[clientNum].pers.connected != CON_CONNECTED ) {
		trap_SendServerCommand( ent - g_entities, "print \"Invalid player\n\"" );
		return;
	}

	level.clients[clientNum].pers.ignore[ent - g_entities] ^= 1;

	if ( level.clients[clientNum].pers.ignore[ent - g_entities] ) {
		trap_SendServerCommand( ent - g_entities,
		                        va( "print \"Ignoring %s\n\"", level.clients[clientNum].pers.netname ) );
	} else {
		trap_SendServerCommand( ent - g_entities,
		                        va( "print \"No longer ignoring %s\n\"", level.clients[clientNum].pers.netname ) );
	}
}

/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( gentity_t *ent, int mode, qboolean arg0 ) {
	char	*p;
	char	text[MAX_TOKEN_CHARS];
	char	location[64];
	char	color[2];
	int		i;
	int		argLen;
	int		textLen;
	int		n;

	textLen = 0;

	if ( level.teams[ent->client->ps.persistant[PERS_TEAM]].muted && mode != SAY_TEAM &&
	     level.teams[ent->client->ps.persistant[PERS_TEAM]].captain != ent ) {
		trap_SendServerCommand( ent - g_entities,
		                        "print \"The team captain has muted the team. You can only say_team.\n\"" );
		return;
	}

	if ( trap_Argc() < 2 && !arg0 ) {
		return;
	}

	if ( arg0 ) {
		p = ConcatArgs( 0 );
	} else {
		p = ConcatArgs( 1 );
	}

	argLen = strlen( p );

	if ( mode == SAY_TEAM ) {
		if ( mode == SAY_WORLD || mode == SAY_ARENA ) {
			memcpy( color, S_COLOR_GREEN, 2 );
		} else if ( mode == SAY_TEAM ) {
			memcpy( color, S_COLOR_CYAN, 2 );
		} else if ( mode == SAY_TELL ) {
			memcpy( color, S_COLOR_MAGENTA, 2 );
		}

		for ( i = 0; i < argLen && i < MAX_TOKEN_CHARS; i++ ) {
			if ( p[i] == '\\' ) {
				switch ( p[++i] ) {
				case 'g':
					if ( ent->client->pers.lastDamage.targetNum != -1 ) {
						n = strlen( level.clients[ent->client->pers.lastDamage.targetNum].pers.netname );

						if ( textLen + n + 2 >= (int)sizeof( text ) ) {
							break;
						}

						memcpy( text + textLen, level.clients[ent->client->pers.lastDamage.targetNum].pers.netname,
								n );
						textLen += n;
						memcpy( text + textLen, color, 2 );
						textLen += 2;
					} else {
						memcpy( text + textLen, "N/A", 3 );
						textLen += 3;
					}
					break;

				case 't':
					if ( ent->client->pers.lastDamage.attackerNum != -1 ) {
						n = strlen( level.clients[ent->client->pers.lastDamage.attackerNum].pers.netname );

						if ( textLen + n + 2 >= (int)sizeof( text ) ) {
							break;
						}

						memcpy( text + textLen, level.clients[ent->client->pers.lastDamage.attackerNum].pers.netname,
								n );
						textLen += n;
						memcpy( text + textLen, color, 2 );
						textLen += 2;
					} else {
						memcpy( text + textLen, "N/A", 3 );
						textLen += 3;
					}
					break;

				case 'k':
					if ( ent->client->pers.lastDamage.victimNum != -1 ) {
						n = strlen( level.clients[ent->client->pers.lastDamage.victimNum].pers.netname );

						if ( textLen + n + 2 >= (int)sizeof( text ) ) {
							break;
						}

						memcpy( text + textLen, level.clients[ent->client->pers.lastDamage.victimNum].pers.netname,
								n );
						textLen += n;
						memcpy( text + textLen, color, 2 );
						textLen += 2;
					} else {
						memcpy( text + textLen, "N/A", 3 );
						textLen += 3;
					}
					break;

				case 'l':
					if ( Team_GetLocationMsg( ent, location, sizeof( location ) ) ) {
						n = strlen( location );

						if ( textLen + n + 2 >= (int)sizeof( text ) ) {
							break;
						}

						memcpy( text + textLen, location, n );
						textLen += n;
						memcpy( text + textLen, color, 2 );
						textLen += 2;
					} else {
						memcpy( text + textLen, "N/A", 3 );
						textLen += 3;
					}
					break;

				case 'p':
					if ( Target_GetLocation( ent, location, sizeof( location ) ) ) {
						n = strlen( location );

						if ( textLen + n + 2 >= (int)sizeof( text ) ) {
							break;
						}

						memcpy( text + textLen, location, n );
						textLen += n;
						memcpy( text + textLen, color, 2 );
						textLen += 2;
					} else {
						memcpy( text + textLen, "N/A", 3 );
						textLen += 3;
					}
					break;

				default:
					text[textLen++] = '\\';
					text[textLen++] = p[i];
					break;
				}

				i++;
			}

			text[textLen++] = p[i];
		}

		text[textLen] = 0;

		G_Say( ent, NULL, mode, text );
	} else {
		G_Say( ent, NULL, mode, p );
	}

}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f( gentity_t *ent ) {
	int			targetNum;
	gentity_t	*target;
	char		*p;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	p = ConcatArgs( 2 );

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	G_Say( ent, ent, SAY_TELL, p );
}


static char	*gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

void Cmd_GameCommand_f( gentity_t *ent ) {
	int		player;
	int		order;
	char	str[MAX_TOKEN_CHARS];

	trap_Argv( 1, str, sizeof( str ) );
	player = atoi( str );
	trap_Argv( 2, str, sizeof( str ) );
	order = atoi( str );

	if ( player < 0 || player >= MAX_CLIENTS ) {
		return;
	}
	if ( order < 0 || order > sizeof(gc_orders)/sizeof(char *) ) {
		return;
	}
	G_Say( ent, &g_entities[player], SAY_TELL, gc_orders[order] );
	G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->s.origin ) ) );
}


/*
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f( gentity_t *ent ) {
	int		i;
	int		len3;
	int		len2;
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];
	char	arg3[MAX_STRING_TOKENS];
	int		unknown1;
	int		found;
	char	*err;
	int		rows;
	int		cols;
	char	**res;
	int		votePercent;
	char	command[20];
	int		len;
	char	unknown2[MAX_STRING_TOKENS];

	// FIXME unused
	UNUSED(unknown1);
	UNUSED(unknown2);

	found = 0;
	err = NULL;
	rows = -1;
	cols = -1;

	if ( !g_allowVote.integer ) {
		trap_SendServerCommand( ent - g_entities, "print \"Voting not allowed here.\n\"" );
		return;
	}

	if ( level.voteTime ) {
		trap_SendServerCommand( ent - g_entities, "print \"A vote is already in progress.\n\"" );
		return;
	}

	if ( ent->client->pers.voteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent - g_entities, "print \"You have called the maximum number of votes.\n\"" );
		return;
	}

	if ( level.time < ( level.voteFailTime + g_voteInterval.integer * 1000 ) ) {
		trap_SendServerCommand( ent - g_entities,
		                        va( "print \"Voting is disabled for another %d seconds\n\"",
		                            ( ( level.voteFailTime + g_voteInterval.integer * 1000 ) - level.time ) / 1000 ) );
		return;
	}

	if ( !ent->client->pers.votesRemaining ) {
		trap_SendServerCommand( ent - g_entities,
		                        "print \"" S_COLOR_YELLOW "You have too many failed votes to propose again\n\"" );
		return;
	}

#ifndef Q3_VM
	// query available commands
	if ( sqlite_get_table_printf( level.db, "SELECT Votestring,Description FROM %s", &res, &rows, &cols, &err,
	                              tblNames[TBL_VOTES] ) ) {
		G_LogPrintf( "<DB> Query error: %s\n", err );
	}

	trap_Argv( 1, arg1, sizeof( arg1 ) );
	trap_Argv( 2, arg2, sizeof( arg2 ) );
	trap_Argv( 3, arg3, sizeof( arg3 ) );

	// initialize tables
	if ( !rows ) {
		G_LogPrintf( "<DB> %s table empty, inserting defaults.\n", tblNames[TBL_VOTES] );

		if ( sqlite_exec_printf( level.db,
		                         "INSERT INTO %s VALUES(\"map_restart\", \"Restart the current map.\", \"\", -1)", NULL,
		                         NULL, &err, tblNames[TBL_VOTES] ) ) {
			G_LogPrintf( "<DB> Query error: %s\n", err );
		}

		if ( sqlite_exec_printf( level.db, "INSERT INTO %s VALUES(\"map\", \"Change to another map.\", \"\", -1)", NULL,
		                         NULL, &err, tblNames[TBL_VOTES] ) ) {
			G_LogPrintf( "<DB> Query error: %s\n", err );
		}

		if ( sqlite_exec_printf(
				 level.db, "INSERT INTO %s VALUES(\"nextmap\", \"Change to the next map in the rotation.\", \"\", -1)",
				 NULL, NULL, &err, tblNames[TBL_VOTES] ) ) {
			G_LogPrintf( "<DB> Query error: %s\n", err );
		}

		if ( sqlite_exec_printf( level.db,
		                         "INSERT INTO %s VALUES(\"kick\", \"Kick a player from the server.\", \"\", -1)", NULL,
		                         NULL, &err, tblNames[TBL_VOTES] ) ) {
			G_LogPrintf( "<DB> Query error: %s\n", err );
		}

		if ( sqlite_exec_printf(
				 level.db,
				 "INSERT INTO %s VALUES(\"clientkick\", \"Kick a player from the server by client-id.\", \"\", -1)",
				 NULL, NULL, &err, tblNames[TBL_VOTES] ) ) {
			G_LogPrintf( "<DB> Query error: %s\n", err );
		}

		// try again
		sqlite_free_table( res );

		if ( sqlite_get_table_printf( level.db, "SELECT Votestring,Description FROM %s", &res, &rows, &cols, &err,
		                              tblNames[TBL_VOTES] ) ) {
			G_LogPrintf( "<DB> Query error: %s\n", err );
		}

		G_LogPrintf( "<DB> Defaults created.\n" );
	}

	// make sure it is a valid command to vote on
	for ( i = 0; i < rows; i++ ) {
		if ( !strncasecmp( arg1, res[i * cols + cols], sizeof( command ) ) ) {
			len = strlen( res[i * cols + cols] );
			strncpy( arg1, res[i * cols + cols], sizeof( command ) );
			arg1[len] = '\0';
			found = 1;
			break;
		}
	}

	if ( !found ) {
		trap_SendServerCommand( ent - g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent - g_entities, "print \"Vote commands are:\n\"" );
		for ( i = 0; i < rows; i++ ) {
			trap_SendServerCommand( ent - g_entities,
			                        va( "print \"%20.20s - %s\n\"", res[i * cols + cols], res[i * cols + cols + 1] ) );
		}
		sqlite_free_table( res );
		return;
	}

	sqlite_free_table( res );

	if ( Q_strrchr( arg1, ';' ) || Q_strrchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent - g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent - g_entities, "print \"Cannot include a semicolon in a vote.\n\"" );
		return;
	}

	len3 = strlen( arg3 );
	len2 = strlen( arg2 );

	// query lua command / percentage parameters for the vote
	if ( sqlite_get_table_printf( level.db, "SELECT Command, Percentage FROM %s WHERE Votestring='%s'", &res, &rows,
	                              &cols, &err, tblNames[TBL_VOTES], arg1 ) ) {
		G_LogPrintf( "<DB> Query error: %s\n", err );
	}

	strncpy( command, res[cols], sizeof( command ) );
	votePercent = atoi( res[cols + 1] );

	if ( votePercent > 0 ) {
		level.votePassPercent = votePercent;
	} else {
		level.votePassPercent = -1;
	}

	sqlite_free_table( res );
#else
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	trap_Argv( 2, arg2, sizeof( arg2 ) );
	trap_Argv( 3, arg3, sizeof( arg3 ) );

	if ( Q_strrchr( arg1, ';' ) || Q_strrchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent - g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent - g_entities, "print \"Cannot include a semicolon in a vote.\n\"" );
		return;
	}

	len3 = strlen( arg3 );
	len2 = strlen( arg2 );

	level.votePassPercent = -1;
#endif

	// handle command
	if ( !Q_stricmp( arg1, "map" ) ) {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );

		if ( len3 > 0 ) {
			Com_snprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
			              "Map change: %s " S_COLOR_WHITE "(%s" S_COLOR_WHITE ")", arg2, arg3 );
		} else {
			Com_snprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
			              "Map change: %s " S_COLOR_WHITE "(by %s" S_COLOR_WHITE ")", arg2, ent->client->pers.netname );
		}
	} else if ( !Q_stricmp( arg1, "clientkick" ) ) {
		// FIXME it feels more likely that arg2 was compared to an unsigned
		// constant, but I can't get the generated code to match that way
		if ( !*(unsigned char *)arg2 ) {
			trap_SendServerCommand( ent - g_entities, "print \"Kick by number:\n\"" );

			for ( i = 0; i < level.maxclients; i++ ) {
				if ( g_entities[i].inuse && g_entities[i].client &&
				     g_entities[i].client->pers.connected == CON_CONNECTED ) {
					trap_SendServerCommand( ent - g_entities,
					                        va( "print \"  %d: %s\n\"", i, g_entities[i].client->pers.netname ) );
				}
			}

			return;
		}

		i = atoi( arg2 );

		if ( i < 0 || i >= MAX_CLIENTS ) {
			return;
		}

		if ( g_entities[i].inuse == qfalse || g_entities[i].client == qfalse ||
		     g_entities[i].client->pers.connected != CON_CONNECTED ) {
			return;
		}

		if ( len3 > 0 ) {
			Com_snprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
			              "Kick: %s " S_COLOR_WHITE "(%s" S_COLOR_WHITE ")", g_entities[i].client->pers.netname, arg3 );
		} else {
			Com_snprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
			              "Kick: %s " S_COLOR_WHITE "(by %s" S_COLOR_WHITE ")", g_entities[i].client->pers.netname,
			              ent->client->pers.netname );
		}

		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
	} else if ( !Q_stricmp( arg1, "kick" ) ) {
		if ( len3 > 0 ) {
			Com_snprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
			              "Kick: %s " S_COLOR_WHITE "(%s" S_COLOR_WHITE ")", arg2, arg3 );
		} else {
			Com_snprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
			              "Kick: %s " S_COLOR_WHITE "(by %s" S_COLOR_WHITE ")", arg2, ent->client->pers.netname );
		}

		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
	} else if ( !Q_stricmp( arg1, "map_restart" ) ) {
		if ( len2 > 0 ) {
			Com_snprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
			              "Restart map (%s" S_COLOR_WHITE ")", arg2 );
		} else {
			Com_snprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
			              "Restart map (by %s" S_COLOR_WHITE ")", ent->client->pers.netname );
		}

		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s", arg1 );
	} else if ( !Q_stricmp( arg1, "nextmap" ) ) {
		if ( len2 > 0 ) {
			Com_snprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Next map (%s" S_COLOR_WHITE ")",
			              arg2 );
		} else {
			Com_snprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
			              "Next map (by %s" S_COLOR_WHITE ")", ent->client->pers.netname );
		}

		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s", arg1 );
	} else if ( rows > 0 ) {
		Com_snprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s %s", arg1, arg2, arg3 );
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s %s", command, arg2, arg3 );
	}
#ifdef Q3_VM
	else {
		trap_SendServerCommand( ent - g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent - g_entities, "print \"Vote commands are: map_restart, nextmap, map <mapname>, clientkick <n> and kick <player>.\n\"" );
	}
#endif

	// start the vote
	ent->client->pers.votesRemaining--;

	trap_SendServerCommand( -1, va( "print \"%s called a vote.\n\"", ent->client->pers.netname ) );

	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;

	for ( i = 0; i < level.maxclients; i++ ) {
		level.clients[i].ps.eFlags &= ~EF_VOTED;
	}
	ent->client->ps.eFlags |= EF_VOTED;

	trap_SetConfigstring( CS_VOTE_TIME, va( "%i", level.voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING, level.voteDisplayString );
	trap_SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes ) );
	trap_SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo ) );
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char		msg[64];

	if ( !level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, "print \"No vote in progress.\n\"" );
		return;
	}
	if ( ent->client->ps.eFlags & EF_VOTED ) {
		trap_SendServerCommand( ent-g_entities, "print \"Vote already cast.\n\"" );
		return;
	}

	trap_SendServerCommand( ent-g_entities, "print \"Vote cast.\n\"" );

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.voteYes++;
		trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	} else {
		level.voteNo++;
		trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );	
	}

	// a majority will be determined in G_CheckVote, which will also account
	// for players entering or leaving
}


/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return;
	}
	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles );
}

/*
=================
Cmd_ReferencedPaks_f
=================
*/
void Cmd_ReferencedPaks_f( gentity_t *ent ) {
	char	paks[MAX_TOKEN_CHARS];
	char	buf[MAX_TOKEN_CHARS];
	int		i, j, len;

	trap_Cvar_VariableStringBuffer( "sv_referencedpaknames", paks, sizeof( paks ) );

	len = strlen( paks );
	j = 0;

	for ( i = 0; i < len; i++ ) {
		if ( paks[i] == ' ' ) {
			if ( paks[i + 1] != '\0' && paks[i + 1] != ' ' ) {
				buf[j] = '\n';
				j++;
			}
		} else {
			buf[j] = paks[i];
			j++;
		}
	}

	buf[j] = 0;

	trap_SendServerCommand( ent - g_entities,
	  va( "print \"" S_COLOR_CYAN "Referenced Pakfiles:\n" S_COLOR_YELLOW "--------------------\n" S_COLOR_WHITE "%s\n\"", buf ) );
}

/*
=================
Cmd_NextMap_f
=================
*/
void Cmd_NextMap_f( gentity_t *ent ) {
	trap_SendServerCommand( ent - g_entities, va( "print \"Next map is %s (in %d minutes).\n\"",
	                                              get_next_map( current_mapname() ), g_timeLeft.integer > 0 ? g_timeLeft.integer : -1 ) );
}

/*
=================
Cmd_Chpass_f
=================
*/
void Cmd_Chpass_f( gentity_t *ent ) {
	int		argc;
	char	arg[MAX_TOKEN_CHARS];

	argc = trap_Argc();

	if ( !sv_punkbuster.integer || !g_trackPlayers.integer || !ent->client->pers.dbId ) {
		trap_SendServerCommand( ent - g_entities, "print \"Player tracking is not enabled on this server.\n\"" );
		return;
	}

	if ( argc == 2 && ent && ent->client ) {
		int len = 0;

		memset( arg, 0, sizeof( arg ) );
		trap_Argv( 1, arg, sizeof( arg ) );

		if ( !arg[0] ) {
			trap_SendServerCommand( ent - g_entities, "print \"Can't set empty password.\n\"" );
			return;
		}

		len = strlen( arg );

		if ( len > 8 || len < 4 ) {
			trap_SendServerCommand( ent - g_entities, "print \"Password must be between 4 and 8 characters.\n\"" );
			return;
		}

		memcpy( ent->client->pers.password, arg, len );

		trap_SendServerCommand( ent - g_entities,
		                        "print \"Password has been set and will be updated when you disconnect.\n\"" );
	} else if ( ent && ent->client ) {
		trap_SendServerCommand( ent - g_entities,
		                        va( "print \"Current password is '%s'.\n\"", ent->client->pers.password ) );
	}
}

/*
=================
Cmd_Chname_f
=================
*/
void Cmd_Chname_f( gentity_t *ent ) {
	int	argc;

	// FIXME unused
	argc = trap_Argc();

	if ( !sv_punkbuster.integer || !g_trackPlayers.integer || !ent->client->pers.dbId ) {
		trap_SendServerCommand( ent - g_entities, "print \"Player tracking is not enabled on this server.\n\"" );
		return;
	}

	if ( ent && ent->client ) {
		ent->client->pers.trackName ^= 1;

		if ( ent->client->pers.trackName ) {
			trap_SendServerCommand(
				ent - g_entities,
				"print \"Your primary name will be set to your current name when you disconnect.\n\"" );
		} else {
			trap_SendServerCommand( ent - g_entities,
			                        "print \"Your name will NOT be updated when you disconnect.\n\"" );
		}
	}
}

/*
=================
Cmd_Timeout_f
=================
*/
void Cmd_Timeout_f( gentity_t *ent ) {
	int		argc;
	char	arg[MAX_TOKEN_CHARS];
	int		unknown;

	argc = trap_Argc();

	// FIXME unused
	UNUSED(arg);
	UNUSED(unknown);

	if ( argc == 1 && ent && ent->client ) {
		int teamNum = ent->client->ps.persistant[PERS_TEAM];
		int arenaNum = ent->client->ps.persistant[PERS_ARENA];

		if ( level.teams[teamNum].timeoutsRemaining < 1 ) {
			trap_SendServerCommand( ent - g_entities, "print \"Your team is out of timeouts.\n\"" );
			return;
		}

		// FIXME should be MAX_ARENAS
		if ( arenaNum < 1 || arenaNum > 1024 ) {
			trap_SendServerCommand( ent - g_entities, va( "print \"Invalid arena: %d.\n\"", arenaNum ) );
			return;
		}

		if ( !level.arenas[arenaNum].settings.competitionmode ) {
			trap_SendServerCommand( ent - g_entities, "print \"You can only timeout in competition mode.\n\"" );
			return;
		}

		if ( level.arenas[arenaNum].state != ROUND_RUNNING ) {
			trap_SendServerCommand( ent - g_entities, "print \"You can't pause right now.\n\"" );
			return;
		}

		if ( !level.arenas[arenaNum].paused ) {
			G_Pause( arenaNum );

			level.teams[ent->client->ps.persistant[PERS_TEAM]].timeoutsRemaining--;
			level.arenas[arenaNum].timeoutTeam = teamNum;

			trap_SendServerCommand( ent - g_entities,
			                        va( "print \"Game in arena %d paused. Your team has %d timeouts remaining.\n\"",
			                            arenaNum, level.teams[teamNum].timeoutsRemaining ) );
		} else {
			trap_SendServerCommand( ent - g_entities, va( "Game in arena %d is already paused.\n", arenaNum ) );
		}
	} else {
		trap_SendServerCommand( ent - g_entities, "Invalid numer of arguments: timeout\n" );
	}
}

/*
=================
Cmd_Timein_f
=================
*/
void Cmd_Timein_f( gentity_t *ent ) {
	int		argc;
	char	arg[MAX_TOKEN_CHARS];

	argc = trap_Argc();

	// FIXME unused
	UNUSED(arg);

	if ( argc == 1 && ent && ent->client ) {
		int teamNum = ent->client->ps.persistant[PERS_TEAM];
		int arenaNum = ent->client->ps.persistant[PERS_ARENA];

		// FIXME should be MAX_ARENAS
		if ( arenaNum < 1 || arenaNum > 1024 ) {
			trap_SendServerCommand( ent - g_entities, va( "print \"Invalid arena: %d.\n\"", arenaNum ) );
			return;
		}

		if ( !level.arenas[arenaNum].settings.competitionmode ) {
			trap_SendServerCommand( ent - g_entities, "print \"You can only timein in competition mode.\n\"" );
			return;
		}

		if ( level.arenas[arenaNum].timeoutTeam != teamNum ) {
			trap_SendServerCommand( ent - g_entities, "print \"You can\'t timein the other teams timeout.\n\"" );
			return;
		}

		if ( level.arenas[arenaNum].paused ) {
			G_UnPause( arenaNum );

			trap_SendServerCommand( ent - g_entities, va( "print \"Game in arena %d unpaused.\n\"", arenaNum ) );
		} else {
			trap_SendServerCommand( ent - g_entities, va( "Game in arena %d is not paused.\n", arenaNum ) );
		}
	} else {
		trap_SendServerCommand( ent - g_entities, "Invalid numer of arguments: timeout\n" );
	}
}

/*
=================
Cmd_Start_f
=================
*/
void Cmd_Start_f( gentity_t *ent ) {
	if ( ent->client && ent->client->ps.persistant[PERS_ARENA] ) {
		send_teams_to_player( ent );

		if ( ent->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
			SelectTeamMessage( ent );
		}
	}
}

/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum ) {
	gentity_t	*ent;
	char		cmd[MAX_TOKEN_CHARS];
	int			sessionKey;

	ent = g_entities + clientNum;

	if ( !ent->client ) {
		return;		// not fully in game yet
	}

	sessionKey = ent->client->pers.sessionKey;

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if (ent->client->pers.connected != CON_CONNECTED) {
		return;
	}

	if ( !Q_stricmp( cmd, "say" ) ) {
		Cmd_Say_f( ent, SAY_ARENA, qfalse );
		return;
	}
	if ( !Q_stricmp( cmd, "say_team" ) ) {
		Cmd_Say_f( ent, SAY_TEAM, qfalse );
		return;
	}
	if ( !Q_stricmp( cmd, "say_world" ) ) {
		Cmd_Say_f( ent, SAY_WORLD, qfalse );
		return;
	}
	if ( !Q_stricmp( cmd, "tell" ) ) {
		Cmd_Tell_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "score" ) ) {
		Cmd_Score_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "stats" ) ) {
		Cmd_Stats_f( ent, 1 );
		return;
	}
	if ( !Q_stricmp( cmd, "topshots" ) ) {
		Cmd_TopShots_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "adminmap" ) ) {
		Cmd_AdminChangeMap_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, encode_command( "adminarena", sessionKey ) ) ) {
		Cmd_AdminArena_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, encode_command( "moveto", sessionKey ) ) ) {
		Cmd_MovetoArena_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, encode_command( "jointeam", sessionKey ) ) ) {
		Cmd_JoinTeam_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, encode_command( "leaveteam", sessionKey ) ) ) {
		Cmd_LeaveTeam_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, encode_command( "leavearena", sessionKey ) ) ) {
		Cmd_LeaveArena_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "pl" ) ) {
		Cmd_PLUpdate_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "start" ) ) {
		Cmd_Start_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, encode_command( "upd_stats", sessionKey ) ) ) {
		Cmd_UpdStats_f( ent );
		return;
	}
#ifndef Q3_VM
	if ( !Q_stricmp( cmd, "aliases" ) ) {
		Cmd_Aliases_f( ent );
		return;
	}
#endif
	if ( !Q_stricmp( cmd, "players" ) ) {
		Cmd_Players_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "ignore" ) ) {
		Cmd_Ignore_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "refpaks" ) ) {
		Cmd_ReferencedPaks_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "next_map" ) ) {
		Cmd_NextMap_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "ready" ) ) {
		Cmd_CompReady_f( ent );
		return;
	}

	// ignore all other commands when at intermission
	if (level.intermissiontime) {
		return;
	}

	if ( !Q_stricmp( cmd, encode_command( "propose", sessionKey ) ) ) {
		Cmd_Propose_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "voteyes" ) ) {
		Cmd_RA3Vote_f( ent, 1 );
		return;
	}
	if ( !Q_stricmp( cmd, "voteno" ) ) {
		Cmd_RA3Vote_f( ent, 0 );
		return;
	}
	if ( !Q_stricmp( cmd, "specrevoke" ) ) {
		Cmd_SpecRevoke_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "specwho" ) ) {
		Cmd_SpecWho_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "specinvite" ) ) {
		Cmd_SpecInvite_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "teamname" ) ) {
		Cmd_TeamName_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "teamlock" ) ) {
		Cmd_TeamLock_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "teamunlock" ) ) {
		Cmd_TeamUnlock_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "teamkick" ) ) {
		Cmd_TeamKick_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "teamcaptain" ) ) {
		Cmd_TeamCaptain_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "teammute" ) ) {
		Cmd_TeamMute_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "teamunmute" ) ) {
		Cmd_TeamUnmute_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "timeout" ) ) {
		Cmd_Timeout_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "timein" ) ) {
		Cmd_Timein_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "chpass" ) ) {
		Cmd_Chpass_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "chname" ) ) {
		Cmd_Chname_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "give" ) ) {
		Cmd_Give_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "god" ) ) {
		Cmd_God_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "notarget" ) ) {
		Cmd_Notarget_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "noclip" ) ) {
		Cmd_Noclip_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "kill" ) ) {
		Cmd_Kill_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "levelshot" ) ) {
		Cmd_LevelShot_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "follow" ) ) {
		Cmd_Follow_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "follownext" ) ) {
		Cmd_FollowCycle_f( ent, 1 );
		return;
	}
	if ( !Q_stricmp( cmd, "followprev" ) ) {
		Cmd_FollowCycle_f( ent, -1 );
		return;
	}
	if ( !Q_stricmp( cmd, "team" ) ) {
		Cmd_Team_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "where" ) ) {
		Cmd_Where_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "callvote" ) ) {
		Cmd_CallVote_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "vote" ) ) {
		Cmd_Vote_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "gc" ) ) {
		Cmd_GameCommand_f( ent );
		return;
	}
	if ( !Q_stricmp( cmd, "setviewpos" ) ) {
		Cmd_SetViewpos_f( ent );
		return;
	}

	trap_SendServerCommand( clientNum, va( "print \"unknown cmd %s\n\"", cmd ) );
}
