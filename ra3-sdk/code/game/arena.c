#include "g_local.h"

// FIXME temporary stubs
extern void G_BalanceTeams(int arenaNum);
//

int trueframetime = 50;
int idmap;

static void SelectArenaMessage( gentity_t *ent );
static int can_create_team( int arenaNum );
static void spawn_in_arena( gentity_t *ent, gentity_t *spot, int param_3 );

void update_arena_menus();
void SendTeamChanged( int playerNum, int sessionTeam, int teamNum, int arenaNum );
int count_valid_teams( int arenaNum );
void arena_init_single( int arenaNum );
void G_ShowVotes( int arenaNum, const arenaSettings_t *ballot );

int SpotCompareX( const void *pa, const void *pb ) {
	const gentity_t *a = *(const gentity_t **)pa;
	const gentity_t *b = *(const gentity_t **)pb;
	return (int)( a->s.origin[0] - b->s.origin[0] );
}

int SpotCompareY( const void *pa, const void *pb ) {
	const gentity_t *a = *(const gentity_t **)pa;
	const gentity_t *b = *(const gentity_t **)pb;
	return (int)( a->s.origin[1] - b->s.origin[1] );
}

gentity_t *SelectRandomArenaSpawnPoint( int arenaNum, int sortByDistance, int useNearHalf ) {
	gentity_t *spot;
	int numSpots;
	int selection;
	int i;
	int j;
	gentity_t *spots[128];
	vec3_t bounds[2];

	numSpots = 0;
	spot = NULL;

	while ( ( spot = G_Find( spot, FOFS( classname ), "info_player_deathmatch" ) ) ) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}

		if ( idmap == 0 && spot->arena != arenaNum ) {
			continue;
		}

		spots[numSpots] = spot;
		numSpots++;
	}

	if ( numSpots == 0 ) {
		G_Printf( "WARNING: No spawnpoint found for arena %d (idmap = %d).\n", arenaNum, idmap );

		if ( !( spot = G_Find( NULL, FOFS( classname ), "info_player_deathmatch" ) ) ) {
			if ( !( spot = G_Find( NULL, FOFS( classname ), "info_player_intermission" ) ) ) {
				trap_Error( va( "NO spawn points found for arena %d (idmap = %d)\n", arenaNum, idmap ) );
			}
		}

		return spot;
	} else if ( numSpots == 1 ) {
		return spots[0];
	} else {
		selection = rand() % ( numSpots / 2 );

		if ( sortByDistance ) {
			bounds[0][0] = spots[0]->s.origin[0];
			bounds[1][0] = spots[0]->s.origin[0];
			bounds[0][1] = spots[0]->s.origin[1];
			bounds[1][1] = spots[0]->s.origin[1];

			for ( i = 1; i < numSpots; i++ ) {
				for ( j = 0; j < 2; j++ ) {
					if ( bounds[0][j] > spots[i]->s.origin[j] ) {
						bounds[0][j] = spots[i]->s.origin[j];
					}

					if ( bounds[1][j] < spots[i]->s.origin[j] ) {
						bounds[1][j] = spots[i]->s.origin[j];
					}
				}
			}

			bounds[0][0] = fabs( bounds[1][0] - bounds[0][0] );
			bounds[0][1] = fabs( bounds[1][1] - bounds[0][1] );

			if ( bounds[0][0] > bounds[0][1] ) {
				qsort( spots, numSpots, sizeof( spots[0] ), SpotCompareX );
			} else {
				qsort( spots, numSpots, sizeof( spots[0] ), SpotCompareY );
			}

			if ( !useNearHalf ) {
				selection += numSpots / 2;
			}
		}

		return spots[selection];
	}
}

int compare_scores( const void *a, const void *b ) {
	int diff;

	diff = ( *(const gentity_t **)b )->client->ps.persistant[PERS_SCORE] -
	       ( *(const gentity_t **)a )->client->ps.persistant[PERS_SCORE];

	if ( diff > 0 ) {
		return 1;
	} else if ( diff < 0 ) {
		return -1;
	} else {
		return 0;
	}
}

int get_sorted_player_list( gentity_t **list, int arenaNum ) {
	int count;
	gentity_t *ent;
	int i;

	count = 0;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		list[count++] = ent;
	}

	if ( !count ) {
		return 0;
	}

	qsort( list, count, sizeof( list[0] ), compare_scores );

	return count;
}

void update_team_scores( int arenaNum ) {
	arenaState_t *arena;

	arena = &level.arenas[arenaNum];

	if ( arena->numTeams >= 2 ) {
		level.teamScores[TEAM_RED] = level.teams[arena->teams[0]].score;
		level.teamScores[TEAM_BLUE] = level.teams[arena->teams[1]].score;
	} else {
		level.teamScores[TEAM_RED] = level.teamScores[TEAM_BLUE] = 0;
	}
}

void update_hud_scores( int arenaNum ) {
	static int oldscore1 = 0;
	static int oldscore2 = 0;

	arenaState_t *arena;
	gentity_t *players[MAX_CLIENTS];
	int playersCount;
	int score1;
	int score2;

	arena = &level.arenas[arenaNum];

	if ( arena->settings.type == AT_PRACTICE || arena->settings.type == AT_REDROVER ) {
		playersCount = get_sorted_player_list( players, arenaNum );

		if ( playersCount >= 2 ) {
			score2 = players[1]->client->ps.persistant[PERS_SCORE];
		} else {
			score2 = 0;
		}

		if ( playersCount >= 1 ) {
			score1 = players[0]->client->ps.persistant[PERS_SCORE];
		} else {
			score1 = 0;
		}

		if ( score1 != oldscore1 || score2 != oldscore2 ) {
			show_string( va( "tscores %d %d", score1, score2 ), arenaNum );
		}

		oldscore1 = score1;
		oldscore2 = score2;
	} else {
		show_string( va( "tscores %d %d",
		                 level.teams[arena->teams[0]].score,
		                 level.teams[arena->teams[1]].score ),
		             arenaNum );
	}
}

int count_players_in_arena( int arenaNum ) {
	int i;
	gentity_t *ent;
	int count;

	count = 0;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		count++;
	}

	return count;
}

int count_players_in_team( int teamNum ) {
	int i;
	gentity_t *ent;
	int count;

	count = 0;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_TEAM] != teamNum ) {
			continue;
		}

		count++;
	}

	return count;
}

void checkFlawless( int arenaNum, int teamNum ) {
	int i;
	gentity_t *ent;
	qboolean flawless;
	int defaultHealth;
	int defaultArmor;

	flawless = qtrue;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_TEAM] != teamNum ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		defaultHealth = level.arenas[arenaNum].settings.health;
		defaultArmor = level.arenas[arenaNum].settings.armor;

		if ( ent->client->ps.stats[STAT_ARMOR] == defaultArmor &&
		     ent->client->ps.stats[STAT_HEALTH] == defaultHealth ) {
			continue;
		}

		flawless = qfalse;

		break;
	}

	if ( flawless ) {
		for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
			if ( ent->inuse == qfalse ||
			     ent->client->pers.connected != CON_CONNECTED ||
			     ent->client->ps.persistant[PERS_TEAM] != teamNum ||
			     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
				continue;
			}

			G_AddEvent( ent, EV_FLAWLESS, arenaNum );

			break;
		}
	}
}

void send_teams_to_player( gentity_t *player ) {
	gentity_t *ent;
	int i;
	int arenaNum;

	arenaNum = player->client->ps.persistant[PERS_ARENA];

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		trap_SendServerCommand( player - g_entities, va( "%s %d %d %d",
		                                                 encode_command( "tc", player->client->pers.sessionKey ),
		                                                 encode_int( i, player->client->pers.sessionKey ),
		                                                 encode_int( ent->client->sess.sessionTeam, player->client->pers.sessionKey ),
		                                                 encode_int( ent->client->ps.persistant[PERS_TEAM], player->client->pers.sessionKey ) ) );
	}
}

void move_to_arena_request( gentity_t *ent, int arenaNum ) {
	if ( ent->client->pers.connected != CON_CONNECTED ) {
		return;
	}

	if ( ent->client->ps.persistant[PERS_ARENA] ) {
		return;
	}

	if ( arenaNum >= 1 && arenaNum <= level.lastArena ) {
		if ( level.arenas[arenaNum].settings.lockarena ) {
			trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "Sorry, that arena is locked\n\n\n\n\n\n\n\n\n\"" );
			SelectArenaMessage( ent );
			return;
		}

		if ( level.arenas[arenaNum].settings.maxplayers &&
		            count_players_in_arena( arenaNum ) >= level.arenas[arenaNum].settings.maxplayers ) {
			trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "Sorry, that arena is full\n\n\n\n\n\n\n\n\n\"" );
			SelectArenaMessage( ent );
			return;
		}

		trap_SendServerCommand( ent - g_entities, "specoff" );

		ent->client->ps.persistant[PERS_SCORE] = 0;
		ent->client->ps.persistant[PERS_ARENA] = arenaNum;
		ent->client->pers.damage = 0;

		arena_spawn( ent );

		ClientUserinfoChanged( ent - g_entities );

		send_teams_to_player( ent );

		update_arena_menus();
	} else {
		trap_Printf( va( "BAD ARENA NUMBER %s: %d\n", ent->client->pers.netname, arenaNum ) );
	}
}

void Cmd_MovetoArena_f( gentity_t *ent ) {
	int arenaNum;
	char arg[20];

	trap_Argv( 1, arg, sizeof( arg ) );
	arg[sizeof( arg ) - 1] = 0;

	arenaNum = atoi( arg ) + 1;

	move_to_arena_request( ent, arenaNum );
}

void Cmd_AdminChangeMap_f( gentity_t *ent ) {
	char userpass[20];
	char adminpass[20];
	char mapname[25];
	fileHandle_t f;

	trap_Argv( 1, userpass, sizeof( userpass ) );
	trap_Argv( 2, mapname, sizeof( mapname ) );

	trap_Cvar_VariableStringBuffer( "g_adminpass", adminpass, sizeof( adminpass ) );

	if ( !adminpass[0] || strcmp( userpass, adminpass ) ) {
		trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "Invalid password\"" );
		return;
	}

	if ( trap_FS_FOpenFile( va( "maps/%s.bsp", mapname ), &f, FS_READ ) <= 0 ) {
		trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "Invalid map\"" );
		return;
	}

	trap_FS_FCloseFile( f );

	trap_SendConsoleCommand( EXEC_APPEND, va( "map %s\n", mapname ) );
}

void add_team_to_arena( int teamNum, int arenaNum ) {
	level.arenas[arenaNum].teams[level.arenas[arenaNum].numTeams] = teamNum;
	level.teams[teamNum].arena = arenaNum;
	level.arenas[arenaNum].numTeams++;

	update_arena_menus();
}

int find_available_team() {
	int teamNum;

	for ( teamNum = 1; teamNum < MAX_TEAMS && level.teams[teamNum].available; teamNum++ ) {
		// nop
	}

	if ( teamNum == MAX_TEAMS ) {
		trap_Error( "MAX TEAMS REACHED \n" );
	}

	return teamNum;
}

void create_new_team( gentity_t *ent ) {
	int teamNum = find_available_team();

	memset( &level.teams[teamNum], 0, sizeof( level.teams[teamNum] ) );

	level.teams[teamNum].available = 1;
	level.teams[teamNum].captain = ent;
	level.teams[teamNum].locked = 0;
	level.teams[teamNum].muted = 0;

	Com_snprintf( level.teams[teamNum].name, sizeof( level.teams[teamNum].name ), "%s's Team", ent->client->pers.netname );
	Q_CleanStr( level.teams[teamNum].name );
	Q_CleanStr( level.teams[teamNum].name );
	level.teams[teamNum].name[sizeof( level.teams[teamNum].name ) - 1] = 0;
	trap_SetConfigstring( CS_TEAMS + teamNum, level.teams[teamNum].name );

	level.teams[teamNum].arena = ent->client->ps.persistant[PERS_ARENA];
	ent->client->ps.persistant[PERS_TEAM] = teamNum;

	G_ResetTimeouts( teamNum );

	add_team_to_arena( teamNum, ent->client->ps.persistant[PERS_ARENA] );

	if ( level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.type == AT_PRACTICE ) {
		trap_SendServerCommand( ent - g_entities, "linepos 0" );
	} else {
		trap_SendServerCommand( ent - g_entities, va( "linepos %d",
		                                              MAX( level.arenas[ent->client->ps.persistant[PERS_ARENA]].numTeams - 2, 0 ) ) );
	}
}

void update_team_menus( int arenaNum ) {
	int i;
	gentity_t *ent;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
			SelectTeamMessage( ent );
		}
	}
}

void update_arena_menus() {
	int i;
	gentity_t *ent;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_ARENA] != 0 ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
			SelectArenaMessage( ent );
		}
	}
}

int can_join_team( gentity_t *ent, int teamNum ) {
	qboolean full;

	if ( teamNum < 0 || teamNum >= MAX_TEAMS ) {
		return 0;
	}

	if ( !level.teams[teamNum].available ) {
		trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "That team is no longer available\n\n\n\n\n\n\n\n\n\"" );
		return 0;
	}

	if ( level.arenas[level.teams[teamNum].arena].settings.competitionmode && level.teams[teamNum].locked ) {
		trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "Sorry, that team is locked\n\n\n\n\n\n\n\n\n\"" );
		return 0;
	}

	full = count_players_in_team( teamNum ) >= level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.playersperteam &&
	       ( level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.type == AT_ROCKETARENA ||
	         level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.type == AT_PRACTICE );

	if ( full ) {
		trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "Sorry, that team is full\n\n\n\n\n\n\n\n\n\"" );
		return 0;
	}

	if ( level.teams[teamNum].arena != ent->client->ps.persistant[PERS_ARENA] ) {
		return 0;
	}

	return 1;
}

int is_setting_set( const char *key, const char *s, char **value ) {
	*value = Info_ValueForKey( s, key );

	if ( !**value ) {
		return 0;
	}

	return 1;
}

void send_voteinfo_to_arena( int arenaNum ) {
	arenaSettings_t *settings;
	arenaSettings_t *ballot;
	char voteinfo[1024];
	int typeChanged;

	settings = &level.arenas[arenaNum].settings;
	ballot = &level.arenas[arenaNum].ballot;
	typeChanged = 0;

	sprintf( voteinfo, "voteon \"%s\" ", level.arenas[arenaNum].voteCaller->client->pers.netname );

	if ( settings->type != ballot->type ) {
		Q_strcat( voteinfo, sizeof( voteinfo ), va( "\\type\\%d", ballot->type ) );
		typeChanged = 1;
	}

	if ( settings->playersperteam != ballot->playersperteam ) {
		Q_strcat( voteinfo, sizeof( voteinfo ), va( "\\ppt\\%d", ballot->playersperteam ) );
	}

	if ( settings->health != ballot->health ) {
		Q_strcat( voteinfo, sizeof( voteinfo ), va( "\\health\\%d", ballot->health ) );
	}

	if ( settings->armor != ballot->armor ) {
		Q_strcat( voteinfo, sizeof( voteinfo ), va( "\\armor\\%d", ballot->armor ) );
	}

	if ( settings->rounds != ballot->rounds ) {
		Q_strcat( voteinfo, sizeof( voteinfo ), va( "\\rnds\\%d", ballot->rounds ) );
	}

	if ( settings->healthprotect != ballot->healthprotect ) {
		Q_strcat( voteinfo, sizeof( voteinfo ), va( "\\hpro\\%d", ballot->healthprotect ) );
	}

	if ( settings->armorprotect != ballot->armorprotect ) {
		Q_strcat( voteinfo, sizeof( voteinfo ), va( "\\apro\\%d", ballot->armorprotect ) );
	}

	if ( settings->weapons != ballot->weapons ) {
		Q_strcat( voteinfo, sizeof( voteinfo ), va( "\\weap\\%d", ballot->weapons ) );
	}

	if ( settings->fallingdamage != ballot->fallingdamage ) {
		Q_strcat( voteinfo, sizeof( voteinfo ), va( "\\fdamage\\%d", ballot->fallingdamage ) );
	}

	if ( settings->excessive != ballot->excessive ) {
		Q_strcat( voteinfo, sizeof( voteinfo ), va( "\\xcess\\%d", ballot->excessive ) );
	}

	if ( settings->competitionmode != ballot->competitionmode ) {
		Q_strcat( voteinfo, sizeof( voteinfo ), va( "\\comp\\%d", ballot->competitionmode ) );
	}

	if ( settings->unbalanced != ballot->unbalanced ) {
		Q_strcat( voteinfo, sizeof( voteinfo ), va( "\\ubal\\%d", ballot->unbalanced ) );
	} else if ( typeChanged ) {
		Q_strcat( voteinfo, sizeof( voteinfo ), "\\comp\\0" );
		ballot->competitionmode = 0;
	}

	if ( settings->lockarena != ballot->lockarena ) {
		Q_strcat( voteinfo, sizeof( voteinfo ), va( "\\lock\\%d", ballot->lockarena ) );
	}

	show_string( voteinfo, arenaNum );
}

void apply_settings_string( arenaSettings_t *settings, const char *voteinfo, arenaSettings_t *defaults, int force ) {
	char *value;

	if ( is_setting_set( "reset", voteinfo, &value ) ) {
		if ( atoi( value ) == 1 ) {
			*settings = *defaults;
			return;
		}
	}

	if ( is_setting_set( "type", voteinfo, &value ) ) {
		if ( defaults->allow_voting_gametype || force ) {
			settings->type = atoi( value );
		}
	}

	if ( is_setting_set( "ppt", voteinfo, &value ) ) {
		if ( defaults->allow_voting_playersperteam || force ) {
			settings->playersperteam = atoi( value );
		}
	}

	if ( is_setting_set( "health", voteinfo, &value ) ) {
		if ( defaults->allow_voting_healtharmor || force ) {
			settings->health = atoi( value );
		}
	}

	if ( is_setting_set( "armor", voteinfo, &value ) ) {
		if ( defaults->allow_voting_healtharmorprotect || force ) {
			settings->armor = atoi( value );
		}
	}

	if ( is_setting_set( "rnds", voteinfo, &value ) ) {
		if ( defaults->allow_voting_rounds || force ) {
			settings->rounds = atoi( value );
		}
	}

	if ( is_setting_set( "hpro", voteinfo, &value ) ) {
		if ( defaults->allow_voting_healtharmorprotect || force ) {
			settings->healthprotect = atoi( value );
		}
	}

	if ( is_setting_set( "apro", voteinfo, &value ) ) {
		if ( defaults->allow_voting_healtharmorprotect || force ) {
			settings->armorprotect = atoi( value );
		}
	}

	if ( is_setting_set( "weap", voteinfo, &value ) ) {
		if ( defaults->allow_voting_weapons || force ) {
			settings->weapons = atoi( value );
		}
	}

	if ( is_setting_set( "fdamage", voteinfo, &value ) ) {
		if ( defaults->allow_voting_fallingdamage || force ) {
			settings->fallingdamage = atoi( value );
		}
	}

	if ( is_setting_set( "xcess", voteinfo, &value ) ) {
		if ( defaults->allow_voting_excessive || force ) {
			settings->excessive = atoi( value );
		}
	}

	if ( is_setting_set( "comp", voteinfo, &value ) ) {
		if ( defaults->allow_voting_competitionmode || force ) {
			settings->competitionmode = atoi( value );
		}
	}

	if ( is_setting_set( "ubal", voteinfo, &value ) ) {
		if ( defaults->allow_voting_competitionmode || force ) {
			settings->unbalanced = atoi( value );
		}
	}

	if ( is_setting_set( "lock", voteinfo, &value ) ) {
		if ( defaults->allow_voting_lockarena || force ) {
			settings->lockarena = atoi( value );
		}
	}

	settings->dirty = 1;
}

void start_voting( gentity_t *caller, int arenaNum ) {
	int i;
	gentity_t *ent;

	if ( level.arenas[arenaNum].state == ROUND_RUNNING ||
	     level.arenas[arenaNum].state == ROUND_COUNTDOWN_ROUND ||
	     level.arenas[arenaNum].state == ROUND_INIT_ROUND ) {
		level.arenas[arenaNum].voteEndTime = level.time + 300000;
	} else {
		level.arenas[arenaNum].voteEndTime = level.time + 30000;
	}

	level.arenas[arenaNum].votesNeeded = level.arenas[arenaNum].votesNo = level.arenas[arenaNum].votesYes = 0;
	level.arenas[arenaNum].voteCaller = caller;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		ent->client->pers.voted = 0;

		level.arenas[arenaNum].votesNeeded++;
	}

	/* caller implicitly votes yes */
	caller->client->pers.voted = 1;
	level.arenas[arenaNum].votesYes = 1;

	send_voteinfo_to_arena( arenaNum );
}

void apply_settings_changes( int arenaNum, arenaSettings_t *ballot ) {
	int i;
	gentity_t *ent;

	G_ShowVotes( arenaNum, ballot );

	if ( ballot->type != level.arenas[arenaNum].settings.type ||
	     ballot->competitionmode != level.arenas[arenaNum].settings.competitionmode ) {
		/* reset arena if type has changed */
		for ( i = 0; i < MAX_TEAMS; i++ ) {
			if ( level.teams[i].arena == arenaNum ) {
				level.teams[i].available = 0;
				level.teams[i].valid = 0;
				level.teams[i].muted = 0;
			}
		}

		level.arenas[arenaNum].settings = *ballot;
		arena_init_single( arenaNum );

		for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
			if ( ent->inuse == qfalse ||
			     ent->client->pers.connected != CON_CONNECTED ||
			     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
				continue;
			}

			ent->client->ps.persistant[PERS_TEAM] = TEAM_SPECTATOR;
		}

		for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
			if ( ent->inuse == qfalse ||
			     ent->client->pers.connected != CON_CONNECTED ||
			     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
				continue;
			}

			ClientSpawn( ent );
		}
	} else {
		/* directly apply settings if type is unchanged */
		level.arenas[arenaNum].settings = *ballot;
		set_arena_configstring( arenaNum );
	}
}

void clear_vote_retries( int arenaNum ) {
	gentity_t *ent;
	int i;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		ent->client->pers.votesRemaining = votetries_setting;
	}
}

void check_voting( int arenaNum ) {
	qboolean passed;

	if ( level.arenas[arenaNum].voteEndTime == 0.0f ) {
		return;
	}

	if ( level.arenas[arenaNum].voteEndTime > ( level.time + 30000 ) ) {
		level.arenas[arenaNum].voteEndTime = level.time + 30000;
	}

	if ( ( level.arenas[arenaNum].votesYes + level.arenas[arenaNum].votesNo ) < level.arenas[arenaNum].votesNeeded &&
	     level.arenas[arenaNum].voteEndTime > level.time ) {
		return;
	}

	if ( level.arenas[arenaNum].ballot.lockarena == 1 && level.arenas[arenaNum].settings.lockarena == 0 ) {
		passed = level.arenas[arenaNum].votesYes >= level.arenas[arenaNum].settings.lockcount &&
		         level.arenas[arenaNum].votesNo == 0 &&
		         level.arenas[arenaNum].votesNeeded - level.arenas[arenaNum].votesYes <= 2;
	} else {
		passed = level.arenas[arenaNum].votesYes - level.arenas[arenaNum].votesNo >= level.arenas[arenaNum].votesNeeded / 3.0;
	}

	level.arenas[arenaNum].voteEndTime = 0;

	if ( passed ) {
		apply_settings_changes( arenaNum, &level.arenas[arenaNum].ballot );

		show_string( va( "cp2 \"" S_COLOR_YELLOW "Changes passed!\nYes votes: %d No Votes: %d\"",
		                 level.arenas[arenaNum].votesYes, level.arenas[arenaNum].votesNo ),
		             arenaNum );

		show_string( va( "print \"Vote passed    Yes votes: %d    No votes: %d\n\"",
		                 level.arenas[arenaNum].votesYes, level.arenas[arenaNum].votesNo ),
		             arenaNum );

		clear_vote_retries( arenaNum );
	} else {
		show_string( va( "cp2 \"" S_COLOR_YELLOW "Changes failed!\nYes votes: %d No Votes: %d\"",
		                 level.arenas[arenaNum].votesYes, level.arenas[arenaNum].votesNo ),
		             arenaNum );

		show_string( va( "print \"Vote failed    Yes votes: %d    No votes: %d\n\"",
		                 level.arenas[arenaNum].votesYes, level.arenas[arenaNum].votesNo ),
		             arenaNum );
	}

	show_string( "voteoff", arenaNum );
}

void Cmd_RA3Vote_f( gentity_t *ent, int vote ) {
	int arenaNum = ent->client->ps.persistant[PERS_ARENA];

	if ( arenaNum < 1 || arenaNum > level.lastArena ) {
		return;
	}

	if ( level.arenas[arenaNum].voteEndTime == 0.0f ) {
		trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "A vote is not in progress\"" );
		return;
	}

	if ( ent->client->pers.voted == 1 ) {
		trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "You have already voted\"" );
		return;
	}

	ent->client->pers.voted = 1;

	if ( vote ) {
		level.arenas[arenaNum].votesYes++;
	} else {
		level.arenas[arenaNum].votesNo++;
	}
}

void Cmd_Propose_f( gentity_t *ent ) {
	int arenaNum;
	char arg[MAX_STRING_CHARS];

	arenaNum = ent->client->ps.persistant[PERS_ARENA];

	if ( arenaNum < 1 || arenaNum > level.lastArena ) {
		return;
	}

	if ( level.arenas[arenaNum].voteEndTime != 0.0f ) {
		trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "A vote is already in progress\"" );
		return;
	}

	if ( !ent->client->pers.votesRemaining ) {
		trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "You have too many failed votes\"" );
		return;
	}

	ent->client->pers.votesRemaining--;

	trap_Argv( 1, arg, sizeof( arg ) );
	arg[sizeof( arg ) - 1] = 0;

	/* initialize ballot state */
	level.arenas[arenaNum].ballot = level.arenas[arenaNum].settings;
	apply_settings_string( &level.arenas[arenaNum].ballot, arg, &level.arenas[arenaNum].defaults, 0 );

	start_voting( ent, arenaNum );
}

void Cmd_AdminArena_f( gentity_t *ent ) {
	int arenaNum;
	char arg1[20];
	char adminpass[20];
	char arg2[8];
	char arg3[MAX_STRING_CHARS];

	trap_Argv( 1, arg1, sizeof( arg1 ) );
	trap_Argv( 2, arg2, sizeof( arg2 ) );
	arenaNum = atoi( arg2 );

	trap_Cvar_VariableStringBuffer( "g_adminpass", adminpass, sizeof( adminpass ) );

	if ( !adminpass[0] || strcmp( arg1, adminpass ) ) {
		trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "Invalid password\"" );
		return;
	}

	if ( arenaNum < 1 || arenaNum > level.lastArena ) {
		return;
	}

	if ( level.arenas[arenaNum].voteEndTime != 0.0f ) {
		level.arenas[arenaNum].voteEndTime = 0;
		show_string( "voteoff", arenaNum );
		return;
	}

	trap_Argv( 3, arg3, sizeof( arg3 ) );
	arg3[sizeof( arg3 ) - 1] = '\0';

	/* initialize ballot state */
	level.arenas[arenaNum].ballot = level.arenas[arenaNum].settings;
	apply_settings_string( &level.arenas[arenaNum].ballot, arg3, &level.arenas[arenaNum].defaults, 1 );

	/* immediately apply it */
	apply_settings_changes( arenaNum, &level.arenas[arenaNum].ballot );

	trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "Changes Applied\"" );
}

void Cmd_PLUpdate_f( gentity_t *ent ) {
	char arg[64];

	memset( arg, 0, sizeof( arg ) );
	trap_Argv( 1, arg, sizeof( arg ) );
	arg[sizeof( arg ) - 1] = 0;

	ent->client->pers.pl = CLAMP( atoi( arg ), 0, 100 );
}

void G_JoinTeam( gentity_t *ent, int teamNum ) {
	int i;

	if ( ent->client->ps.persistant[PERS_TEAM] ) {
		trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "You must leave your current team first!\"" );
		return;
	}

	if ( teamNum == 0 ) {
		if ( !can_create_team( ent->client->ps.persistant[PERS_ARENA] ) ) {
			trap_SendServerCommand( ent - g_entities, "cp \"" S_COLOR_YELLOW "This arena does not allow you to\n" S_COLOR_YELLOW "create a team\n\n\n\n\n\n\n\n\n\n\"" );
			SelectTeamMessage( ent );
			return;
		}

		create_new_team( ent );
	} else {
		arenaState_t *arena;

		if ( !can_join_team( ent, teamNum ) ) {
			SelectTeamMessage( ent );
			return;
		}

		ent->client->ps.persistant[PERS_TEAM] = teamNum;

		arena = &level.arenas[ent->client->ps.persistant[PERS_ARENA]];

		if ( arena->settings.competitionmode && !level.teams[teamNum].captain ) {
			level.teams[teamNum].captain = ent;

			show_string( va( "print \"%s" S_COLOR_WHITE " is now the team captain of the %s.\n\"",
			                 ent->client->pers.netname, level.teams[teamNum].name ),
			             ent->client->ps.persistant[PERS_ARENA] );
		}

		if ( arena->settings.type == AT_CLANARENA || arena->settings.type == AT_REDROVER ) {
			trap_SendServerCommand( ent - g_entities, "linepos 0" );
		} else {
			for ( i = 0; i < arena->numTeams; i++ ) {
				if ( arena->teams[i] == teamNum ) {
					break;
				}
			}

			trap_SendServerCommand( ent - g_entities, va( "linepos %d", MAX( i - 1, 0 ) ) );
		}
	}

	ent->client->pers.originalTeam = TEAM_SPECTATOR;

	update_team_menus( ent->client->ps.persistant[PERS_ARENA] );

	trap_SendServerCommand( ent - g_entities, "inarena" );

	if ( level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.type == AT_REDROVER ||
	     level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.type == AT_PRACTICE ||
	     level.arenas[ent->client->ps.persistant[PERS_ARENA]].state == ROUND_WARMUP ) {
		arena_spawn( ent );
	} else {
		SendTeamChanged( ent - g_entities, TEAM_SPECTATOR, ent->client->ps.persistant[PERS_TEAM], ent->client->ps.persistant[PERS_ARENA] );
	}
}

void Cmd_JoinTeam_f( gentity_t *ent ) {
	char arg[20];
	int teamNum;

	trap_Argv( 1, arg, sizeof( arg ) );
	arg[sizeof( arg ) - 1] = 0;

	teamNum = atoi( arg );

	G_JoinTeam( ent, teamNum );
}

void G_ResetTeamName( gentity_t *ent ) {
	int teamNum = ent->client->ps.persistant[PERS_TEAM];
	int arenaNum = ent->client->ps.persistant[PERS_ARENA];
	int playersCount = count_players_in_team( teamNum );

	if ( !level.arenas[arenaNum].settings.competitionmode ||
		 !level.teams[teamNum].valid ||
		 playersCount != 1 ) {
		return;
	}

	if ( teamNum == level.arenas[arenaNum].teams[1] ) {
		strcpy( level.teams[teamNum].name, "Blue Team" );
	} else {
		strcpy( level.teams[teamNum].name, "Red Team" );
	}

	trap_SendServerCommand( -1, va( "print \"Team %d's name reset to %s.\n\"",
	                                teamNum, level.teams[teamNum].name ) );
}

void G_LeaveTeam( gentity_t *ent ) {
	if ( !ent->client->ps.persistant[PERS_TEAM] ) {
		return;
	}

	G_NewTeamCaptain( ent );
	G_ResetTeamName( ent );
	memset( level.teams[ent->client->ps.persistant[PERS_TEAM]].invites, 0, sizeof( level.teams[0].invites ) );
	ent->client->ps.persistant[PERS_TEAM] = 0;

	trap_SendServerCommand( ent - g_entities, "linepos -1" );
	ClientSpawn( ent );

	update_arena_menus();
}

void Cmd_LeaveTeam_f( gentity_t *ent ) {
	G_LeaveTeam( ent );
}

void Cmd_LeaveArena_f( gentity_t *ent ) {
	G_NewTeamCaptain( ent );
	G_ResetTeamName( ent );
	memset( level.teams[ent->client->ps.persistant[PERS_TEAM]].invites, 0, sizeof( level.teams[0].invites ) );
	ent->client->ps.persistant[PERS_TEAM] = 0;
	ent->client->ps.persistant[PERS_ARENA] = 0;

	ClientSpawn( ent );
	ClientUserinfoChanged( ent - g_entities );

	update_arena_menus();
}

char *get_arena_name( int arenaNum ) {
	gentity_t *spot;

	spot = NULL;

	while ( ( spot = G_Find( spot, FOFS( classname ), "info_player_intermission" ) ) ) {
		if ( spot->arena == arenaNum ) {
			return spot->message;
		}
	}

	return va( "Arena Number %d", arenaNum );
}

static void SelectArenaMessage( gentity_t *ent ) {
	char desc[1024];
	char arenas[1400];
	int arenasLen;
	int i;
	int descLen;

	arenas[0] = 0;
	arenasLen = 0;

	for ( i = 1; i <= level.lastArena; i++ ) {
		Com_sprintf( desc, sizeof( desc ), " \"%s\" %i %i %i",
		             get_arena_name( i ),
		             level.arenas[i].settings.type,
		             count_players_in_arena( i ),
		             count_valid_teams( i ) );
		descLen = strlen( desc );

		if ( ( arenasLen + descLen ) > (int)( sizeof( arenas ) - 100 ) ) {
			break;
		}

		strcpy( arenas + arenasLen, desc );
		arenasLen += descLen;
	}

	trap_SendServerCommand( ent - g_entities, va( "arenas %i%s", i - 1, arenas ) );
}

static int can_create_team( int arenaNum ) {
	return level.arenas[arenaNum].settings.type == AT_ROCKETARENA ||
	       level.arenas[arenaNum].settings.type == AT_PRACTICE;
}

void SelectTeamMessage( gentity_t *ent ) {
	char desc[1024];
	char teams[1400];
	int teamsLen;
	int i;
	int descLen;
	int arenaNum;
	int numInvalid;
	char *create;
	int teamNum;
	int playersCount;
	int locked;

	numInvalid = 0;
	teams[0] = 0;
	teamsLen = 0;

	arenaNum = ent->client->ps.persistant[PERS_ARENA];

	if ( arenaNum < 1 || arenaNum > level.lastArena ) {
		trap_Printf( va( "BAD ARENA NUMBER IN SELECT TEAM MESSAGE %s: %d\n", ent->client->pers.netname, arenaNum ) );
		SelectArenaMessage( ent );
		return;
	}

	for ( i = 0; i < level.arenas[arenaNum].numTeams; i++ ) {
		teamNum = level.arenas[arenaNum].teams[i];

		playersCount = count_players_in_team( teamNum );

		if ( !playersCount && !level.teams[teamNum].valid ) {
			numInvalid++;
			continue;
		}

		locked = playersCount >= level.arenas[arenaNum].settings.playersperteam &&
		         ( level.arenas[arenaNum].settings.type == AT_ROCKETARENA || level.arenas[arenaNum].settings.type == AT_PRACTICE );

		if ( level.arenas[arenaNum].settings.lockarena ) {
			locked = 1;
		}

		Com_sprintf( desc, sizeof( desc ), " \"%s\" %i %i %i", level.teams[teamNum].name, playersCount, locked, teamNum );
		descLen = strlen( desc );

		if ( ( teamsLen + descLen ) > (int)( sizeof( teams ) - 100 ) ) {
			break;
		}

		strcpy( teams + teamsLen, desc );
		teamsLen += descLen;
	}

	if ( can_create_team( arenaNum ) ) {
		create = "\"Create a Team\" 0 0 0";
		i++;
	} else {
		create = "";
	}

	trap_SendServerCommand( ent - g_entities, va( "teams %i %s%s", i - numInvalid, create, teams ) );
}

void give_weapons( gentity_t *ent ) {
	arenaState_t *arena;
	int i;

	arena = &level.arenas[ent->client->ps.persistant[PERS_ARENA]];

	if ( arena->settings.health ) {
		ent->health = ent->client->ps.stats[STAT_HEALTH] = arena->settings.excessive ? arena->settings.health * 3 : arena->settings.health;
	} else {
		ent->health = ent->client->ps.stats[STAT_HEALTH] = 100;
	}

	ent->client->ps.stats[STAT_WEAPONS] = 1 << WP_GAUNTLET;
	ent->client->ps.ammo[WP_GAUNTLET] = -1;

	for ( i = 1; i < 10; i++ ) {
		if ( arena->settings.weapons & ( 1 << ( i - 1 ) ) ) {
			ent->client->ps.stats[STAT_WEAPONS] |= ( 1 << ( i + 1 ) );
		}
	}

	ent->client->ps.ammo[WP_MACHINEGUN] = ( arena->settings.type == AT_PRACTICE || arena->state == ROUND_WARMUP ) ? -1 : arena->settings.bullets;
	ent->client->ps.ammo[WP_SHOTGUN] = ( arena->settings.type == AT_PRACTICE || arena->state == ROUND_WARMUP ) ? -1 : arena->settings.shells;
	ent->client->ps.ammo[WP_GRENADE_LAUNCHER] = ( arena->settings.type == AT_PRACTICE || arena->state == ROUND_WARMUP ) ? -1 : arena->settings.grenades;
	ent->client->ps.ammo[WP_ROCKET_LAUNCHER] = ( arena->settings.type == AT_PRACTICE || arena->state == ROUND_WARMUP ) ? -1 : arena->settings.rockets;
	ent->client->ps.ammo[WP_LIGHTNING] = ( arena->settings.type == AT_PRACTICE || arena->state == ROUND_WARMUP ) ? -1 : arena->settings.cells;
	ent->client->ps.ammo[WP_RAILGUN] = ( arena->settings.type == AT_PRACTICE || arena->state == ROUND_WARMUP ) ? -1 : arena->settings.slugs;
	ent->client->ps.ammo[WP_PLASMAGUN] = ( arena->settings.type == AT_PRACTICE || arena->state == ROUND_WARMUP ) ? -1 : arena->settings.plasma;
	ent->client->ps.ammo[WP_BFG] = ( arena->settings.type == AT_PRACTICE || arena->state == ROUND_WARMUP ) ? -1 : arena->settings.bfgammo;

	ent->client->ps.stats[STAT_ARMOR] = arena->settings.armor;
}

void track_change( gentity_t *ent, int dir, int respawn ) {
	qboolean canNotTrack;
	gclient_t *last;
	gclient_t *next;
	int i;

	canNotTrack = qfalse;

	last = &level.clients[ent->client->sess.spectatorClient];

	/* check if last can still be followed */
	if ( last->pers.connected != CON_CONNECTED ||
	     last->sess.sessionTeam == TEAM_SPECTATOR ||
	     last->ps.persistant[PERS_ARENA] != ent->client->ps.persistant[PERS_ARENA] ||
	     last->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ||
	     last == ent->client ) {
		canNotTrack = qtrue;
	}

	/* check if allowed to follow last */
	if ( level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.competitionmode &&
	     ent->client->ps.persistant[PERS_TEAM] != last->ps.persistant[PERS_TEAM] ) {
		if ( level.teams[last->ps.persistant[PERS_TEAM]].invites[ent - g_entities] != INVITE_NONE && !canNotTrack ) {
			canNotTrack = qfalse;
		} else {
			canNotTrack = qtrue;
		}
	}

	/* walk clients */
	i = ent->client->sess.spectatorClient;

	do {
		i += dir;

		if ( i >= level.maxclients ) {
			i = 0;
		}

		if ( i < 0 ) {
			i = level.maxclients - 1;
		}

		next = &level.clients[i];

		/* check if next can be followed */
		if ( next->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( next->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ||
		     next->sess.sessionTeam == TEAM_SPECTATOR ||
		     next->ps.persistant[PERS_ARENA] != ent->client->ps.persistant[PERS_ARENA] ||
		     next == ent->client ) {
			continue;
		}

		/* check if allowed to follow next */
		if ( level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.competitionmode != 0 &&
			 ent->client->ps.persistant[PERS_TEAM] != next->ps.persistant[PERS_TEAM] &&
			 level.teams[next->ps.persistant[PERS_TEAM]].invites[ent - g_entities] == INVITE_NONE ) {
			continue;
		}

		/* can be followed, break out */
		canNotTrack = qfalse;
		break;
	} while ( next != last );

	if ( next != last || !canNotTrack ) {
		ent->client->sess.spectatorClient = i;

		trap_SendServerCommand( ent - g_entities, va( "print \"Tracking %s\n\"", next->pers.netname ) );
	} else {
		ent->client->sess.spectatorState = SPECTATOR_FREE;

		if ( respawn ) {
			spawn_in_arena( ent, 0, 1 );
		}

		trap_SendServerCommand( ent - g_entities, va( "print \"No one to track\n\"" ) );
	}
}

void track_next( gentity_t *ent ) {
	track_change( ent, 1, 1 );
}

void track_prev( gentity_t *ent ) {
	track_change( ent, -1, 1 );
}

char *omode_descriptions[5] = {
	"BAD",
	"Free Flying",
	"In Eyes",
	"Trackcam",
	"BAD",
};

void change_omode( gentity_t *ent ) {
	spectatorState_t oldState;

	oldState = ent->client->sess.spectatorState;

	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		return;
	}

	ent->client->sess.spectatorState += 1;

	if ( ent->client->sess.spectatorState > SPECTATOR_UNKNOWN ) {
		ent->client->sess.spectatorState = SPECTATOR_FREE;
	}

	trap_SendServerCommand( ent - g_entities, va( "print \"Switched Observer Mode to: %s\n\"",
	                                              omode_descriptions[ent->client->sess.spectatorState] ) );

	if ( ( oldState == SPECTATOR_FOLLOW || oldState == SPECTATOR_UNKNOWN ) &&
	     ent->client->sess.spectatorState == SPECTATOR_FREE ) {
		spawn_in_arena( ent, &g_entities[ent->client->sess.spectatorClient], 0 );
	} else {
		spawn_in_arena( ent, NULL, 0 );
	}
}

void SetObserverMode( gentity_t *ent ) {
	gclient_t *target;

	if ( ent->r.svFlags & SVF_BOT ) {
		return;
	}

	switch ( ent->client->sess.spectatorState ) {
	case SPECTATOR_FREE: {
		StopFollowing( ent );
	} break;

	case SPECTATOR_FOLLOW:
	case SPECTATOR_UNKNOWN: {
		target = &level.clients[ent->client->sess.spectatorClient];

		/* if target is invalid, try to find a new one */
		if ( target->pers.connected != CON_CONNECTED ||
		     target->sess.sessionTeam == TEAM_SPECTATOR ||
		     target->ps.persistant[PERS_ARENA] != ent->client->ps.persistant[PERS_ARENA] ||
		     target->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ||
		     target == ent->client ) {
			track_change( ent, 1, 1 );
		}
		/* if not allowed to follow target, try to find a new one */
		else if ( level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.competitionmode &&
		     ent->client->ps.persistant[PERS_TEAM] != target->ps.persistant[PERS_TEAM] &&
		     level.teams[target->ps.persistant[PERS_TEAM]].invites[ent - g_entities] == INVITE_NONE ) {
			track_change( ent, 1, 1 );
		}
	} break;

	default: {
	} break;
	}
}

static void spawn_in_arena( gentity_t *ent, gentity_t *spot, int doneTracking ) {
	int sortByDistance;
	int useNearHalf;
	int telefragged;
	int i;

	sortByDistance = ent->client->sess.sessionTeam != TEAM_SPECTATOR && level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.type != AT_PRACTICE;
	useNearHalf = ent->client->sess.sessionTeam == level.arenas[ent->client->ps.persistant[PERS_ARENA]].nearTeam;

	if ( !spot ) {
		spot = SelectRandomArenaSpawnPoint( ent->client->ps.persistant[PERS_ARENA], sortByDistance, useNearHalf );
	}

	if ( spot->client ) {
		G_SetOrigin( ent, spot->client->ps.origin );
		VectorCopy( spot->client->ps.origin, ent->client->ps.origin );
	} else {
		G_SetOrigin( ent, spot->s.origin );
		VectorCopy( spot->s.origin, ent->client->ps.origin );
	}

	ent->client->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd( ent->client - level.clients, &ent->client->pers.cmd );

	if ( !spot->client ) {
		SetClientViewAngle( ent, spot->s.angles );
	}

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR && !doneTracking ) {
		if ( level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.competitionmode ) {
			ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		}

		SetObserverMode( ent );
	} else {
		VectorSet( ent->client->ps.velocity, 0.0f, 0.0f, 0.0f );
		ent->client->telefragDir = -1;
		StopFollowing( ent );

		trap_UnlinkEntity( ent );
		telefragged = G_KillBox( ent );
		trap_LinkEntity( ent );

		if ( telefragged ) {
			ent->s.solid = 0;
		}

		give_weapons( ent );

		memset( ent->client->pers.combatStats, 0, sizeof( ent->client->pers.combatStats ) );

		if ( !( ent->client->ps.stats[STAT_WEAPONS] & ( 1 << ent->client->ps.weapon ) ) ) {
			ent->client->ps.weapon = WP_NONE;
		}

		ent->client->ps.weaponstate = WEAPON_READY;
	}

	ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	ent->client->ps.pm_time = 100;
	ent->client->ps.pm_flags |= PMF_UNKNOWN;

	ent->client->respawnTime = level.time;
	ent->client->inactivityTime = level.time + g_inactivity.integer * 1000;
	ent->client->latched_buttons = 0;

	ent->client->ps.torsoAnim = TORSO_STAND;
	ent->client->ps.legsAnim = LEGS_IDLE;

	if ( level.intermissiontime ) {
		MoveClientToIntermission( ent );
	} else {
		G_UseTargets( spot, ent );

		if ( !ent->client->ps.weapon ) {
			ent->client->ps.weapon = 1;

			for ( i = WP_GRAPPLING_HOOK; i > WP_NONE; i-- ) {
				if ( ent->client->ps.stats[STAT_WEAPONS] & ( 1 << i ) ) {
					ent->client->ps.weapon = i;
					break;
				}
			}
		}
	}

	ent->client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;

	ClientThink( ent - g_entities );

	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}

	ClientEndFrame( ent );

	BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
}

int other_team( team_t team ) {
	if ( team == TEAM_RED ) {
		return TEAM_BLUE;
	} else {
		return TEAM_RED;
	}
}

void SendTeamChanged( int playerNum, int sessionTeam, int teamNum, int arenaNum ) {
	gentity_t *ent;
	int i;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		trap_SendServerCommand( i, va( "%s %d %d %d",
		                               encode_command( "tc", ent->client->pers.sessionKey ),
		                               encode_int( playerNum, ent->client->pers.sessionKey ),
		                               encode_int( sessionTeam, ent->client->pers.sessionKey ),
		                               encode_int( teamNum, ent->client->pers.sessionKey ) ) );
	}
}

void set_sessionteam_and_skin( gentity_t *ent, team_t sessionTeam ) {
	int playerNum;
	int teamChanged;
	char buf[MAX_INFO_STRING];
	char *oldTeamString;
	int oldTeamEncoded;
	int oldTeam;

	playerNum = ent - g_entities;
	teamChanged = 0;

	/* update it */
	ent->client->sess.sessionTeam = sessionTeam;

	/* compare against the config string in case ent->client->sess.sessionTeam is out of sync */
	trap_GetConfigstring( CS_PLAYERS + playerNum, buf, sizeof( buf ) );
	oldTeamString = Info_ValueForKey( buf, "t" );
	oldTeamEncoded = atoi( oldTeamString );
	oldTeam = decode_int( oldTeamEncoded, ent->client->pers.sessionKey );

	if ( oldTeam != sessionTeam ) {
		teamChanged = 1;
	}

	SendTeamChanged( playerNum, sessionTeam, ent->client->ps.persistant[PERS_TEAM], ent->client->ps.persistant[PERS_ARENA] );

	/* synchronize config string */
	if ( ent->r.svFlags & SVF_BOT ) {
		G_Printf( "set_sessionteam_and_skin: Userinfo change for bot!\n" );
		ClientUserinfoChanged( playerNum );
	} else if ( teamChanged ) {
		ClientUserinfoChanged( playerNum );
	}
}

void arena_spawn( gentity_t *ent ) {
	qboolean teamChangeSent;
	int arenaNum;
	qboolean wasRed;

	teamChangeSent = qfalse;
	arenaNum = -1;

	if ( ent->r.svFlags & SVF_BOT ) {
		arenaNum = ent->client->ps.persistant[PERS_ARENA];

		G_Printf( "arena_spawn: 1: SVF_BOT=%d\n", ent->r.svFlags & SVF_BOT );
		G_Printf( "arena_spawn: bot: %s/%d.\n", ent->client->pers.netname, ent->client->ps.persistant[PERS_ARENA] );

		if ( level.arenas[arenaNum].settings.type != AT_ROCKETARENA && level.arenas[arenaNum].settings.type != AT_PRACTICE ) {
			G_Printf( "arena_spawn: bot: unsupported arena type.\n" );
			return;
		}

		if ( !ent->client->ps.persistant[PERS_TEAM] ) {
			G_Printf( "arena_spawn: bot: new team.\n" );
			G_JoinTeam( ent, 0 );
		}

		spawn_in_arena( ent, NULL, 0 );

		G_Printf( "arena_spawn: 2: SVF_BOT=%d\n", ent->r.svFlags & SVF_BOT );

		return;
	}

	if ( !ent->client->ps.persistant[PERS_ARENA] ) {
		SelectArenaMessage( ent );
	} else if ( !ent->client->ps.persistant[PERS_TEAM] ) {
		SelectTeamMessage( ent );
	} else {
		arenaNum = ent->client->ps.persistant[PERS_ARENA];

		if ( level.arenas[arenaNum].settings.type == AT_REDROVER ) {
			if ( level.arenas[arenaNum].state == ROUND_RUNNING ||
			     level.arenas[arenaNum].state == ROUND_COUNTDOWN_ROUND ||
			     level.arenas[arenaNum].state == ROUND_INIT_ROUND )  {
				/* set originalTeam when joining after fill_arena has been called. the player hasn't
				   yet spawned, and ent->client->ps.persistant[PERS_TEAM] holds the joined team */
				if ( ent->client->pers.originalTeam == TEAM_SPECTATOR ) {
					wasRed = ent->client->ps.persistant[PERS_TEAM] == level.arenas[arenaNum].teams[1];
					ent->client->pers.originalTeam = wasRed ? TEAM_BLUE : TEAM_RED;
				} else {
					wasRed = ent->client->ps.persistant[PERS_TEAM] == level.arenas[arenaNum].teams[0];
				}

				ent->client->ps.persistant[PERS_TEAM] = wasRed ? level.arenas[arenaNum].teams[1] : level.arenas[arenaNum].teams[0];

				set_sessionteam_and_skin( ent, wasRed ? TEAM_BLUE : TEAM_RED );
				teamChangeSent = qtrue;
			}
		}

		if ( level.arenas[arenaNum].settings.type == AT_PRACTICE ) {
			set_sessionteam_and_skin( ent, TEAM_RED );
			teamChangeSent = qtrue;
		}

		if ( level.arenas[arenaNum].state == ROUND_WARMUP ) {
			set_sessionteam_and_skin( ent, level.teams[ent->client->ps.persistant[PERS_TEAM]].sessionTeam );
			teamChangeSent = qtrue;
		}
	}

	spawn_in_arena( ent, NULL, 0 );

	if ( teamChangeSent && arenaNum > -1 ) {
		if ( level.arenas[arenaNum].state == ROUND_RUNNING || level.arenas[arenaNum].state == ROUND_WARMUP ) {
			ent->takedamage = qtrue;
			ent->client->ps.pm_flags &= ~PMF_UNKNOWN;
		} else {
			ent->takedamage = qfalse;
			ent->client->ps.pm_flags |= PMF_UNKNOWN;
		}
	} else {
		SendTeamChanged( ent - g_entities, ent->client->sess.sessionTeam,
		                 ent->client->ps.persistant[PERS_TEAM], ent->client->ps.persistant[PERS_ARENA] );
	}
}

void restore_orig_teams( int arenaNum ) {
	int i;
	gentity_t *ent;
	int nextTeam;

	nextTeam = TEAM_RED;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR && ent->client->pers.originalTeam != TEAM_SPECTATOR ) {
			ent->client->ps.persistant[PERS_TEAM] = nextTeam;

			nextTeam = nextTeam == TEAM_RED ? TEAM_BLUE : TEAM_RED;
		}
	}
}

void remove_players_from_teams( int arenaNum ) {
	int i;
	gentity_t *ent;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
			ent->client->ps.persistant[PERS_TEAM] = TEAM_SPECTATOR;

			arena_spawn( ent );
		}
	}
}

int fight_done( int arenaNum ) {
	int winningTeam;
	int i;
	gentity_t *ent;

	winningTeam = -1;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_ARENA] != arenaNum ||
			ent->takedamage == qfalse ||
			ent->client->ps.pm_type == PM_DEAD ) {
			continue;
		}

		if ( winningTeam == -1 ) {
			winningTeam = ent->client->ps.persistant[PERS_TEAM];
			continue;
		}

		if ( winningTeam != ent->client->ps.persistant[PERS_TEAM] ) {
			return -2;
		}
	}

	return winningTeam;
}

void set_damage( arenaState_t *arena, qboolean takedamage, int unknown ) {
	int team1, team2;
	gentity_t *ent;
	int i;

	team1 = arena->numTeams > 0 ? arena->teams[0] : -1;
	team2 = arena->numTeams > 1 ? arena->teams[1] : -1;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_TEAM] != team1 &&
		     ent->client->ps.persistant[PERS_TEAM] != team2 ) {
			continue;
		}

		ent->takedamage = takedamage;

		if ( unknown ) {
			ent->client->ps.pm_flags |= PMF_UNKNOWN;
		} else {
			ent->client->ps.pm_flags &= ~PMF_UNKNOWN;
		}
	}
}

void show_string( const char *text, int arenaNum ) {
	gentity_t *ent;
	int i;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		trap_SendServerCommand( i, text );
	}
}

void show_countdown( int arenaNum ) {
	if ( level.arenas[arenaNum].countdown != 0 ) {
		if ( level.arenas[arenaNum].countdown < 4 ) {
			level.arenas[arenaNum].winstring[0] = '\0';
		}

		if ( level.arenas[arenaNum].state == ROUND_COUNTDOWN_MATCH ) {
			show_string( va( "cp \"%sWaiting for next match to begin\n%d\"",
			                 level.arenas[arenaNum].winstring,
			                 level.arenas[arenaNum].countdown ),
			             arenaNum );
		} else {
			if ( level.arenas[arenaNum].numTeams > 1 ) {
				if ( level.arenas[arenaNum].settings.rounds == 1 || level.arenas[arenaNum].settings.type == AT_REDROVER ) {
					show_string( va( "cp \"%s%s vs. %s\n" S_COLOR_YELLOW "%d\"",
					                 level.arenas[arenaNum].winstring,
					                 level.teams[level.arenas[arenaNum].teams[0]].name,
					                 level.teams[level.arenas[arenaNum].teams[1]].name,
					                 level.arenas[arenaNum].countdown ),
					             arenaNum );
				} else {
					show_string( va( "cp \"%s%s vs. %s\nRound %d of %d\n" S_COLOR_YELLOW "%d\"",
					                 level.arenas[arenaNum].winstring,
					                 level.teams[level.arenas[arenaNum].teams[0]].name,
					                 level.teams[level.arenas[arenaNum].teams[1]].name,
					                 level.arenas[arenaNum].round,
					                 level.arenas[arenaNum].settings.rounds,
					                 level.arenas[arenaNum].countdown ),
					             arenaNum );
				}
			}

			switch ( level.arenas[arenaNum].countdown ) {
			case 3:
				show_string( "play \"sound/feedback/three\"", arenaNum );
				break;

			case 2:
				show_string( "play \"sound/feedback/two\"", arenaNum );
				break;

			case 1:
				show_string( "play \"sound/feedback/one\"", arenaNum );
				break;
			}
		}
	} else {
		show_string( "cp \"" S_COLOR_RED "FIGHT!\"", arenaNum );
		show_string( "play \"sound/feedback/fight\"", arenaNum );
	}
}

void make_team_spectators( int teamNum ) {
	int i;
	gentity_t *ent;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_TEAM] != teamNum ) {
			continue;
		}

		if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) {
			respawn( ent );
		}

		ent->client->sess.sessionTeam = TEAM_SPECTATOR;
	}
}

void clear_arena( arenaState_t *arena, int resetScores ) {
	int i;

	arena->nearTeam = ( rand() & 1 ) ? TEAM_RED : TEAM_BLUE;

	for ( i = 0; i < 2; i++ ) {
		if ( arena->numTeams <= i ) {
			break;
		}

		if ( resetScores ) {
			level.teams[arena->teams[i]].score = 0;
		}

		level.teams[arena->teams[i]].sessionTeam = i == 0 ? TEAM_RED : TEAM_BLUE;

		make_team_spectators( arena->teams[i] );
	}
}

int fill_arena( int arenaNum ) {
	gentity_t *spots[128];
	gentity_t *spot;
	arenaState_t *arena;
	int numSpots;
	int failed;
	int maxSpots;
	gentity_t *ent;
	int i;
	int selection;

	spot = NULL;
	arena = &level.arenas[arenaNum];
	numSpots = 0;
	failed = 0;
	maxSpots = 0;

	memset( spots, 0, sizeof( spots ) );

	while ( ( spot = G_Find( spot, FOFS( classname ), "info_player_deathmatch" ) ) ) {
		if ( idmap == 0 && spot->arena != arenaNum ) {
			continue;
		}

		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}

		if ( numSpots >= (int)( sizeof( spots ) / sizeof( spots[0] ) ) ) {
			break;
		}

		spots[numSpots++] = spot;
	}

	maxSpots = numSpots;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ||
		     ( ent->client->ps.persistant[PERS_TEAM] != arena->teams[0] && ent->client->ps.persistant[PERS_TEAM] != arena->teams[1] ) ) {
			continue;
		}

		if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}

		if ( numSpots < 1 ) {
			failed++;
			break;
		}

		selection = rand() % maxSpots;

		while ( !spots[selection] ) {
			selection = rand() % maxSpots;
		}

		set_sessionteam_and_skin( ent, level.teams[ent->client->ps.persistant[PERS_TEAM]].sessionTeam );

		if ( level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.type == AT_REDROVER ) {
			ent->client->pers.originalTeam = ent->client->ps.persistant[PERS_TEAM];
		}

		spawn_in_arena( ent, spots[selection], 0 );

		numSpots--;

		spots[selection] = NULL;
	}

	return failed;
}

int count_valid_teams( int arenaNum ) {
	int i;
	int count;

	count = 0;

	for ( i = 0; i < level.arenas[arenaNum].numTeams; i++ ) {
		if ( level.teams[level.arenas[arenaNum].teams[i]].available == qfalse ||
		     count_players_in_team( level.arenas[arenaNum].teams[i] ) <= 0 ) {
			continue;
		}

		count++;
	}

	return count;
}

void send_line_position( int arenaNum ) {
	int linePos;
	gentity_t *ent;
	int i;

	for ( linePos = 0; linePos < level.arenas[arenaNum].numTeams; linePos++ ) {
		for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
			if ( ent->inuse == qfalse ||
			     ent->client->pers.connected != CON_CONNECTED ||
			     ent->client->ps.persistant[PERS_TEAM] != level.arenas[arenaNum].teams[linePos] ) {
				continue;
			}

			trap_SendServerCommand( i, va( "linepos %d", MAX( linePos - 1, 0 ) ) );
		}
	}
}

void remove_team_from_line( arenaState_t *arena, int linePos ) {
	int i;

	for ( i = linePos; i < arena->numTeams - 1; i++ ) {
		arena->teams[i] = arena->teams[i + 1];
	}

	arena->numTeams--;

	if ( can_create_team( arena - level.arenas ) ) {
		send_line_position( arena - level.arenas );
	}
}

void cycle_teams( arenaState_t *arena, int winningTeamNum ) {
	int losingTeamNum;

	losingTeamNum = arena->teams[0] == winningTeamNum ? arena->teams[1] : arena->teams[0];

	arena->teams[0] = winningTeamNum;
	remove_team_from_line( arena, 1 );

	arena->teams[arena->numTeams] = losingTeamNum;
	arena->numTeams++;
}

void set_telefrag_dir( gentity_t *ent, gentity_t *hit ) {
	static int lastdir = 0;

	if ( ent->client->telefragDir == -1 ) {
		ent->client->telefragDir = ( lastdir += 225 ) % 360;
	}

	if ( hit->client->telefragDir == -1 ) {
		hit->client->telefragDir = ( lastdir += 225 ) % 360;
	}

	if ( hit->client->telefragDir == ent->client->telefragDir ) {
		hit->client->telefragDir = ( hit->client->telefragDir + 225 ) % 360;
	}
}

void telefrag_bounce_client( gentity_t *ent ) {
	ent->client->telefragBounceEndFrame = level.framenum + 500 / trueframetime;
	ent->client->ps.velocity[2] = 150 + ( rand() % 300 );
}

int check_telefrag( int arenaNum ) {
	int i;
	int count;
	gentity_t *ent;
	qboolean telefragged;

	count = 0;

	for ( i = 0; i < level.maxclients; i++ ) {
		ent = &g_entities[i];

		if ( ent->inuse == qfalse ||
		     ent->client == NULL ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
			if ( ent->client->telefragBounceEndFrame && ent->client->telefragBounceEndFrame <= level.framenum ) {
				G_Printf( "TFRETRY: %s %d\n", ent->client->pers.netname, ent->client->telefragDir );

				ent->clipmask = MASK_PLAYERSOLID;
				ent->s.solid = MASK_PLAYERSOLID;

				trap_UnlinkEntity( ent );
				telefragged = G_KillBox( ent );
				trap_LinkEntity( ent );

				if ( !telefragged ) {
					G_Printf( "TFDONE: %s\n", ent->client->pers.netname );
				}
			}

			if ( ent->client->telefragBounceEndFrame ) {
				count++;
			}
		}
	}

	return count;
}

void check_teams( int arenaNum ) {
	int i;
	arenaState_t *arena;

	arena = &level.arenas[arenaNum];

	if ( arena->settings.dirty ) {
		if ( !count_players_in_arena( arenaNum ) ) {
			apply_settings_changes( arenaNum, &arena->defaults );
		}
	}

	if ( !can_create_team( arenaNum ) ) {
		return;
	}

	for ( i = arena->numTeams - 1; i >= 0; i-- ) {
		if ( !count_players_in_team( arena->teams[i] ) ) {
			trap_Printf( va( "removing team %d from arena %d\n", arena->teams[i], arenaNum ) );

			level.teams[arena->teams[i]].available = 0;

			remove_team_from_line( arena, i );
		}
	}
}

void clear_player_scores( int arenaNum ) {
	gentity_t *ent;
	int i;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		ent->client->ps.persistant[PERS_SCORE] = 0;
	}
}

void G_ClearScores( int arenaNum ) {
	gentity_t *ent;
	int i;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		ent->client->ps.persistant[PERS_SCORE] = 0;
		ent->client->pers.damage = 0;
	}
}

void Cmd_UpdStats_f( gentity_t *ent ) {
	char arg[MAX_STRING_CHARS];
	int unknown;

	unknown = 0;

	trap_Argv( 1, arg, sizeof( arg ) );
	unknown = atoi( arg );

	G_SendStatsupdate( ent, unknown );
}

void Cmd_SpecWho_f( gentity_t *ent ) {
	int i;

	i = 0;

	if ( !level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.competitionmode ) {
		trap_SendServerCommand( ent - g_entities, "print \"Competition mode isn't enabled.\n\"" );
		return;
	}

	trap_SendServerCommand( ent - g_entities, "stprint \" " S_COLOR_YELLOW "ID Name\n\"" );

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( level.teams[ent->client->ps.persistant[PERS_TEAM]].invites[i] == INVITE_NONE ||
		     g_entities[i].inuse == qfalse ||
		     level.clients[i].pers.connected != CON_CONNECTED ) {
			continue;
		}

		trap_SendServerCommand( ent - g_entities, va( "stprint \"" S_COLOR_GREEN "%2d " S_COLOR_WHITE "%-18.18s" S_COLOR_WHITE "\n\"",
		                                              i, Q_StaticClean( level.clients[i].pers.netname ) ) );
	}
}

void Cmd_SpecRevoke_f( gentity_t *ent ) {
	char arg[MAX_STRING_CHARS];
	int i;

	trap_Argv( 1, arg, sizeof( arg ) );

	if ( !arg[0] ) {
		trap_SendServerCommand( ent - g_entities, "print \"Syntax: specrevoke clientnumber\n\"" );
		return;
	}

	if ( !level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.competitionmode ) {
		trap_SendServerCommand( ent - g_entities, "print \"You can't revoke invitations if competition mode isn't enabled.\n\"" );
		return;
	}

	i = atoi( arg );

	if ( i < 0 || i > MAX_CLIENTS || !g_entities[i].inuse || level.clients[i].pers.connected != CON_CONNECTED ||
	     level.clients[i].ps.persistant[PERS_ARENA] != ent->client->ps.persistant[PERS_ARENA] || i == ent->client->ps.clientNum ) {
		trap_SendServerCommand( ent - g_entities, "print \"Invalid player.\n\"" );
		return;
	}

	if ( level.teams[ent->client->ps.persistant[PERS_TEAM]].invites[i] == INVITE_NONE ) {
		trap_SendServerCommand( ent - g_entities, "print \"Client is not invited to spectate.\n\"" );
		return;
	}

	trap_SendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " removed the spectator invitation from %s" S_COLOR_WHITE ".\n\"",
	                                ent->client->pers.netname, level.clients[i].pers.netname ) );

	level.teams[ent->client->ps.persistant[PERS_TEAM]].invites[i] = INVITE_NONE;
}

void Cmd_SpecInvite_f( gentity_t *ent ) {
	char arg[MAX_STRING_CHARS];
	int i;

	trap_Argv( 1, arg, sizeof( arg ) );

	if ( !arg[0] ) {
		trap_SendServerCommand( ent - g_entities, "print \"Syntax: specinvite clientnumber\n\"" );
		return;
	}

	if ( !level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.competitionmode ) {
		trap_SendServerCommand( ent - g_entities, "print \"You can't invite spectators if competition mode isn't enabled.\n\"" );
		return;
	}

	i = atoi( arg );

	if ( i < 0 || i > MAX_CLIENTS || !g_entities[i].inuse || level.clients[i].pers.connected != CON_CONNECTED ||
	     level.clients[i].ps.persistant[PERS_ARENA] != ent->client->ps.persistant[PERS_ARENA] || i == ent->client->ps.clientNum ) {
		trap_SendServerCommand( ent - g_entities, "print \"Invalid player.\n\"" );
		return;
	}

	if ( level.teams[ent->client->ps.persistant[PERS_TEAM]].invites[i] != INVITE_NONE ) {
		trap_SendServerCommand( ent - g_entities, "print \"Client is already invited to spectate.\n\"" );
		return;
	}

	trap_SendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " invited %s" S_COLOR_WHITE " to spectate his/her team.\n\"",
	                                ent->client->pers.netname, level.clients[i].pers.netname ) );

	level.teams[ent->client->ps.persistant[PERS_TEAM]].invites[i] = INVITE_SPEC;
}

void Cmd_CompReady_f( gentity_t *ent ) {
	char arg[MAX_STRING_CHARS];
	int ready;

	ready = ent->client->readyToExit;

	if ( !level.arenas[ent->client->ps.persistant[PERS_ARENA]].settings.competitionmode ) {
		trap_SendServerCommand( ent - g_entities, "print \"ready only works in competition mode\n\"" );
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if ( !arg[0] ) {
		ready = ready ^ 1;
	} else {
		ready = atoi( arg );

		if ( ready > 1 ) {
			ready = 1;
		}

		if ( ready < 0 ) {
			ready = 0;
		}
	}

	ent->client->readyToExit = ready;

	if ( ready ) {
		trap_SendServerCommand( ent - g_entities, "print \"You are " S_COLOR_CYAN "READY\n\"" );
	} else {
		trap_SendServerCommand( ent - g_entities, "print \"You are " S_COLOR_CYAN "NOT READY\n\"" );
	}
}

#ifndef Q3_VM

void Cmd_Aliases_f( gentity_t *ent ) {
	char *err;
	int rows;
	int cols;
	char **res;
	char arg[MAX_STRING_CHARS];
	int clientNum;
	char buf[MAX_STRING_CHARS * 2];
	char aliases[10][MAX_NETNAME];
	char fmt[MAX_STRING_CHARS];
	char tmp[256];
	int i;
	int len;

	err = NULL;
	rows = -1;
	cols = -1;

	if ( !sv_punkbuster.integer || !g_trackPlayers.integer ) {
		trap_SendServerCommand( ent - g_entities, "print \"This server does not track player aliases.\n\"" );
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if ( !arg[0] ) {
		trap_SendServerCommand( ent - g_entities, "print \"Syntax: aliases clientnumber\n\"" );
		return;
	}

	clientNum = atoi( arg );

	if ( clientNum < 0 || clientNum > MAX_CLIENTS || !g_entities[clientNum].inuse ||
	     level.clients[clientNum].pers.connected != CON_CONNECTED || !level.clients[clientNum].pers.dbId ) {
		trap_SendServerCommand( ent - g_entities, "print \"Invalid player.\n\"" );
		return;
	}

	if ( sqlite_get_table_printf( level.db, "SELECT Name, Aliases FROM %s WHERE ID=%d", &res, &rows, &cols, &err, 
	                              tblNames[TBL_PLAYERS], level.clients[clientNum].pers.dbId ) ) {
		G_LogPrintf( "<DB> Query error: %s\n", err );
		return;
	}

	if ( rows >= 1 ) {
		len = 0;

		strcpy( buf, res[cols + 1] );

		for ( i = 0; i < (int)( sizeof( aliases ) / sizeof( aliases[0] ) ); i++ ) {
			strcpy( aliases[i], Info_ValueForKey( buf, va( "a%d", i ) ) );

			if ( *aliases[i] ) {
				sprintf( tmp, " %s\n", aliases[i] );
				strcpy( fmt + len, tmp );
				len += strlen( tmp );
			}
		}

		fmt[len] = '\0';

		trap_SendServerCommand( ent - g_entities, va( "print \"Known aliases for %s:\n%s", res[cols], fmt ) );
	} else {
		trap_SendServerCommand( ent - g_entities, "print \"Could not find any data.\n\"" );
	}

	sqlite_free_table( res );
}

#endif

void Cmd_Players_f( gentity_t *ent ) {
	int i;
	char userinfo[MAX_INFO_STRING];
	int snaps;
	int rate;
	int timeNudge;

	trap_SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "ID Name               GUID      Snaps  Rate    Timenudge\n\"" );

	for ( i = 0; i < level.maxclients; i++ ) {
		if ( g_entities[i].inuse == qfalse || g_entities[i].client == NULL || g_entities[i].client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		trap_GetUserinfo( i, userinfo, sizeof( userinfo ) );

		snaps = atoi( Info_ValueForKey( userinfo, "snaps" ) );
		rate = atoi( Info_ValueForKey( userinfo, "rate" ) );
		timeNudge = atoi( Info_ValueForKey( userinfo, "cl_timeNudge" ) );

		trap_SendServerCommand( ent - g_entities, va( "print \"" S_COLOR_GREEN "%2d " S_COLOR_WHITE "%-18.18s %8.8s %3d    %6d %3d\n\"", i,
		                                              Q_StaticClean( g_entities[i].client->pers.netname ), pb_guids[i].string + 24, snaps,
		                                              rate, timeNudge ) );
	}
}

void clear_stats( int arenaNum, int teamNum ) {
	gentity_t *ent;
	int i;
	int j;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		for ( j = 0; j < TS_NUM_STATS; j++ ) {
			ent->client->pers.topStats[teamNum - 1][j] += ent->client->pers.topStats[teamNum][j];
			ent->client->pers.topStats[teamNum][j] = 0;
		}
	}
}

void get_redrover_winstring( int arenaNum ) {
	gentity_t *players[MAX_CLIENTS];
	int playersCount;
	int i;
	int plural;

	playersCount = 0;
	playersCount = get_sorted_player_list( players, arenaNum );

	if ( !playersCount ) {
		return;
	}

	i = 1;

	while ( i < playersCount && players[i]->client->ps.persistant[PERS_SCORE] == players[0]->client->ps.persistant[PERS_SCORE] ) {
		i++;
	}

	plural = i >= 2;
	strcpy( level.arenas[arenaNum].winstring, players[0]->client->pers.netname );

	while ( --i > 0 ) {
		Q_strcat( level.arenas[arenaNum].winstring, sizeof( level.arenas[arenaNum].winstring ), va( ", %s", players[i]->client->pers.netname ) );
	}

	if ( plural ) {
		Q_strcat( level.arenas[arenaNum].winstring, sizeof( level.arenas[arenaNum].winstring ), " have won the match!\n" );
	} else {
		Q_strcat( level.arenas[arenaNum].winstring, sizeof( level.arenas[arenaNum].winstring ), " has won the match!\n" );
	}
}

void respawn_stuck_players( int arenaNum ) {
	int i;
	gentity_t *ent;

	for ( i = 0; i < level.maxclients; i++ ) {
		ent = &g_entities[i];

		if ( ent->inuse == qfalse ||
		     ent->client == NULL ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
			if ( ent->client->telefragBounceEndFrame ) {
				ent->client->telefragBounceEndFrame = 0;

				spawn_in_arena( ent, NULL, 0 );
			}
		}
	}
}

void respawn_all( int arenaNum ) {
	int i;
	gentity_t *ent;

	for ( i = 0; i < level.maxclients; i++ ) {
		ent = g_entities + i;

		if ( ent->inuse == qfalse ||
		     ent->client == NULL ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
			respawn( ent );
		}
	}
}

void show_string_ready( int arenaNum ) {
	gentity_t *ent;
	int i;
	char *status;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ) {
			continue;
		}

		if ( ent->client->ps.persistant[PERS_TEAM] != level.arenas[arenaNum].teams[0] &&
		     ent->client->ps.persistant[PERS_TEAM] != level.arenas[arenaNum].teams[1] ) {
			status = "Spectating";
		} else {
			status = ent->client->readyToExit ? "READY" : "NOT READY (\\ready)";
		}

		trap_SendServerCommand( i, va( "cp \"%sWaiting for ready up...\nYou are %s\"",
		                               level.arenas[arenaNum].winstring, status ) );
	}
}

void arena_think( int arenaNum ) {
	arenaState_t *arena;
	int winningTeam;
	int n;

	arena = &level.arenas[arenaNum];

	switch ( arena->state ) {
	case ROUND_PAUSED: {
		check_voting( arenaNum );
		check_teams( arenaNum );

		if ( level.time >= arena->pauseTime + arena->timeoutSeconds * 1000 ) {
			G_UnPause( arenaNum );
		}
	} break;

	case ROUND_UNPAUSED: {
		check_voting( arenaNum );
		check_teams( arenaNum );

		if ( arena->nextCheckFrame == 0 ) {
			arena->nextCheckFrame = level.framenum + ( 1000 / trueframetime );
			arena->countdown = 10;
		} else if ( arena->nextCheckFrame < level.framenum ) {
			arena->countdown--;

			if ( arena->countdown == 0 ) {
				level.arenas[arenaNum].paused = 0;
				level.arenas[arenaNum].state = level.arenas[arenaNum].resumeState;
				level.arenas[arenaNum].resumeState = level.arenas[arenaNum].pauseTime = -1;
				level.arenas[arenaNum].timeoutTeam = -1;
				set_arena_configstring( arenaNum );
				break;
			}

			arena->nextCheckFrame = level.framenum + ( 1000 / trueframetime );
			show_countdown( arenaNum );
		}
	} break;

	case ROUND_INIT:
	case ROUND_INIT_MATCH: {
		if ( arena->settings.type == AT_PRACTICE ) {
			arena->state = ROUND_RUNNING;
			break;
		}

		check_teams( arenaNum );
		check_voting( arenaNum );

		n = count_valid_teams( arenaNum );

		if ( n < 2 ) {
			if ( arena->state == ROUND_INIT_MATCH ) {
				arena->state = ROUND_INIT;
			}

			if ( n > 0 && level.time - arena->lastWaitingMessageTime > 2000 ) {
				show_string( "cp \"Waiting for more players to join\"", arenaNum );
				arena->lastWaitingMessageTime = level.time;
			}

			break;
		}

		if ( arena->settings.type == AT_CLANARENA && arena->state == ROUND_INIT ) {
			arena->state = ROUND_COUNTDOWN_MATCH;
		} else {
			arena->round = 1;
			arena->state = ROUND_INIT_ROUND;
			arena->readyCleared = 0;

			clear_arena( arena, 1 );

			show_string( "tscores 0 0", arenaNum );
		}
	} break;

	case ROUND_INIT_ROUND: {
		int failed;
		int oldState;
		int preserveState;

		if ( arena->waitEndTime < level.time ) {
			oldState = -1;
			preserveState = arena->settings.competitionmode != 0 && arena->round == 1 && arena->roundTied == 0;

			if ( ( arena->settings.competitionmode != 0 && ( arena->round > 1 || ( arena->round == 1 && arena->roundTied != 0 ) ) ) ) {
			   if ( arena->numTeams <= 1 || ( count_players_in_team( arena->teams[0] ) != count_players_in_team( arena->teams[1] ) && arena->settings.unbalanced == 0 ) ) {
					show_string( va( "cp \"%sWaiting for even teams...\"", arena->winstring ), arenaNum );
					arena->waitEndTime = level.time + 3000;
					break;
				}
			}

			if ( arena->round == 1 && arena->roundTied == 0 && preserveState == 0 ) {
				show_string( va( "aarecord %d", 2 ), arenaNum );
			}

			G_BalanceTeams( arenaNum );

			if ( preserveState != 0 ) {
				oldState = arena->state;
				arena->state = ROUND_WARMUP;
			}

			failed = fill_arena( arenaNum );

			if ( preserveState != 0 ) {
				arena->state = oldState;
			}

			if ( failed == 0 ) {
				show_string( "initmodels", arenaNum );

				if ( preserveState ) {
					arena->state = ROUND_WARMUP;
					arena->readyCleared = 0;
					ClearClientReady( arenaNum );
					SetClientReadyMasks( 0, arenaNum );
					set_arena_configstring( arenaNum );
					set_damage( arena, 1, 0 );
				} else {
					arena->state = ROUND_COUNTDOWN_ROUND;
				}

				arena->roundTied = 0;
			} else {
				arena->waitEndTime = level.time + 3000;
				show_string( va( "cp \"%sWaiting for open spawns...\"", arena->winstring ), arenaNum );
			}
		}
	} break;

	case ROUND_WARMUP: {
		int numReady;
		int numNotReady;
		int numTotal;
		int readyMask;

		if ( arena->waitEndTime > level.time ) {
			break;
		}

		check_voting( arenaNum );

		if ( arena->numTeams <= 1 ||
			 ( count_players_in_team( arena->teams[0] ) != count_players_in_team( arena->teams[1] ) && arena->settings.unbalanced == 0 ) ) {
			show_string( va( "cp \"%sWaiting for even teams...\"", arena->winstring ), arenaNum );
			arena->waitEndTime = level.time + 3000;
			break;
		}

		if ( arena->round == 1 ) {
			if ( !arena->readyCleared ) {
				arena->readyCleared = 1;
				ClearClientReady( arenaNum );
			}

			CountReadyClients( &numReady, &numNotReady, &readyMask, &numTotal, arenaNum );
			SetClientReadyMasks( readyMask, arenaNum );

			if ( numReady != numTotal ) {
				if ( arena->waitEndTime < level.time ) {
					show_string_ready( arenaNum );
					arena->waitEndTime = level.time + 3000;
				}

				break;
			}
		}

		arena->waitEndTime = level.time - 1;
		arena->state = ROUND_RESPAWN;
		respawn_all( arenaNum );
		ClearClientReady( arenaNum );
		SetClientReadyMasks( 0, arenaNum );
		G_ClearScores( arenaNum );
		clear_stats( arenaNum, 2 );
		clear_stats( arenaNum, 1 );
		set_damage( arena, 0, 1 );
		arena->countdown = 0;
		arena->waitEndTime = level.time - 1;
	} break;

	case ROUND_RESPAWN: {
		int failed;

		if ( arena->waitEndTime < level.time ) {
			if ( arena->nextCheckFrame == 0 ) {
				arena->nextCheckFrame = level.framenum + ( 1000 / trueframetime );
				arena->countdown = 10;
			} else if ( arena->nextCheckFrame < level.framenum ) {
				if ( arena->countdown == 0 ) {
					failed = fill_arena( arenaNum );

					if ( failed ) {
						arena->waitEndTime = level.time + 3000;
						show_string( va( "cp \"%sWaiting for open spawns...\"", arena->winstring ), arenaNum );
					} else {
						arena->nextCheckFrame = 0;
						arena->state = ROUND_COUNTDOWN_ROUND;
						set_arena_configstring( arenaNum );
						show_string( "initmodels", arenaNum );
						show_string( va( "aarecord %d", 2 ), arenaNum );
					}

					break;
				}

				arena->countdown--;
				arena->nextCheckFrame = level.framenum + ( 1000 / trueframetime );
				show_string( va( "cp \"%sWaiting for next match to begin\n%d\"",
								 level.arenas[arenaNum].winstring, level.arenas[arenaNum].countdown ),
							 arenaNum );
			}
		}
	} break;

	case ROUND_COUNTDOWN_MATCH:
	case ROUND_COUNTDOWN_ROUND: {
		n = check_telefrag( arenaNum );

		check_voting( arenaNum );

		if ( arena->nextCheckFrame == 0 ) {
			arena->nextCheckFrame = level.framenum + ( 1000 / trueframetime );
			if ( arena->state == ROUND_COUNTDOWN_MATCH ) {
				arena->countdown = 10;
			} else {
				arena->countdown = 5;
			}
			show_countdown( arenaNum );
		} else if ( arena->nextCheckFrame < level.framenum ) {
			arena->countdown--;

			if ( arena->countdown == 0 && n && arena->numCountdownRestarts < 2 ) {
				arena->countdown = 6;
				arena->nextCheckFrame = level.framenum + ( 1000 / trueframetime );
				respawn_stuck_players( arenaNum );
				strcpy( arena->winstring, "Restarting due to potential telefrags\n" );
				arena->numCountdownRestarts++;
			} else {
				if ( arena->state != ROUND_COUNTDOWN_MATCH || arena->countdown ) {
					show_countdown( arenaNum );
				}

				if ( arena->countdown == 0 ) {
					arena->numCountdownRestarts = 0;
					trap_Printf( va( "Running round %d of %d\n", arena->round, arena->settings.rounds ) );
					arena->nextCheckFrame = 0;

					if ( count_valid_teams( arenaNum ) < 2 ) {
						trap_Printf( va( "Not enough teams to start in arena %d\n", arenaNum ) );
						arena->state = ROUND_INIT;
					} else if ( arena->state == ROUND_COUNTDOWN_MATCH ) {
						arena->state = ROUND_INIT_MATCH;
					} else {
						arena->state = ROUND_RUNNING;

						if ( arena->settings.type == AT_REDROVER ) {
							clear_player_scores( arenaNum );
						}

						clear_stats( arenaNum, 2 );

						if ( arena->round == 1 ) {
							clear_stats( arenaNum, 1 );
						}

						set_damage( arena, 1, 0 );
					}
				} else {
					arena->nextCheckFrame = level.framenum + ( 1000 / trueframetime );
				}
			}
		}
	} break;

	case ROUND_RUNNING: {
		check_telefrag( arenaNum );

		if ( ( arena->settings.type == AT_PRACTICE || arena->settings.type == AT_REDROVER ) && level.time - arena->lastHudUpdateTime > 2000 ) {
			update_hud_scores( arenaNum );
			arena->lastHudUpdateTime = level.time;
		}

		if ( arena->settings.type == AT_PRACTICE ) {
			check_voting( arenaNum );
			check_teams( arenaNum );
		} else {
			winningTeam = fight_done( arenaNum );

			if ( winningTeam > -2 ) {
				arena->state = ROUND_OVER;
				set_damage( arena, 1, 1 );
				send_end_of_round_stats( arenaNum );
				checkFlawless( arenaNum, winningTeam );
			}
		}
	} break;

	case ROUND_OVER: {
		if ( arena->nextCheckFrame == 0 ) {
			arena->nextCheckFrame = level.framenum + ( 3000 / trueframetime );
		} else if ( arena->nextCheckFrame >= level.framenum ) {
			// nop
		} else {
			winningTeam = fight_done( arenaNum );
			set_damage( arena, 0, 1 );
			arena->nextCheckFrame = 0;

			if ( winningTeam == -1 ) {
				Com_sprintf( arena->winstring, sizeof( arena->winstring ), "It was a tie!\n" );

				if ( arena->numTeams < 2 || ( count_players_in_team( arena->teams[0] ) == 0 && count_players_in_team( arena->teams[1] ) == 0 ) ) {
					trap_Printf( va( "Not enough teams/players to continue match in arena %d\n", arenaNum ) );
					arena->round = arena->settings.rounds + 1;
				}

				arena->roundTied = 1;
			} else {
				level.teams[winningTeam].score++;

				update_hud_scores( arenaNum );

				if ( level.teams[winningTeam].score > arena->settings.rounds / 2 || arena->settings.type == AT_REDROVER ) {
					if ( arena->settings.type == AT_REDROVER ) {
						get_redrover_winstring( arenaNum );
					} else {
						Com_sprintf( arena->winstring, sizeof( arena->winstring ), "%s has won the match!\n", level.teams[winningTeam].name );
					}

					arena->round = arena->settings.rounds + 1;
				} else {
					Com_sprintf( arena->winstring, sizeof( arena->winstring ), "%s has won the round!\n", level.teams[winningTeam].name );

					arena->round++;
				}
			}

			if ( arena->settings.type == AT_REDROVER ) {
				restore_orig_teams( arenaNum );
			}

			if ( arena->round <= arena->settings.rounds ) {
				arena->state = ROUND_INIT_ROUND;
				arena->readyCleared = 0;
				clear_arena( arena, 0 );
				break;
			}

			G_ResetTimeouts( arena->teams[0] );
			G_ResetTimeouts( arena->teams[1] );

			if ( winningTeam != -1 && can_create_team( arenaNum ) ) {
				cycle_teams( arena, winningTeam );
				send_line_position( arenaNum );
			}

			arena->state = ROUND_INIT;

			show_string( va( "aastop %d", 10 ), arenaNum );
		}
	} break;
	}
}

void arena_think_all() {
	int i;

	if ( level.intermissiontime ) {
		return;
	}

	i = level.framenum % ( level.lastArena * 2 );

	if ( !( i & 1 ) ) {
		arena_think( ( i / 2 ) + 1 );
	}

	if ( i % 100 ) {
		CalculateRanks();
	}
}

void create_pickup_team( int arenaNum, const char *name ) {
	int teamNum;

	teamNum = find_available_team();

	memset( &level.teams[teamNum], 0, sizeof( level.teams[teamNum] ) );

	level.teams[teamNum].locked = 0;
	level.teams[teamNum].available = 1;
	level.teams[teamNum].captain = NULL;
	level.teams[teamNum].valid = 1;
	level.teams[teamNum].muted = 0;
	strcpy( level.teams[teamNum].name, name );
	level.teams[teamNum].arena = arenaNum;

	G_ResetTimeouts( teamNum );

	add_team_to_arena( teamNum, arenaNum );
}

void set_arena_configstring( int arenaNum ) {
	trap_SetConfigstring( CS_ARENAS + arenaNum, va( "\\type\\%d\\ppt\\%d\\rnds\\%d\\weap\\%d\\armor\\%d\\health\\%d\\lock\\%d\\comp\\%d\\ubal\\%d\\xcess\\%d\\apro\\%d\\hpro\\%d\\fdamage\\%d\\avgt\\%d\\avha\\%d\\avppt\\%d\\avrounds\\%d\\avhapt\\%d\\avweap\\%d\\avfdamage\\%d\\avgrapple\\%d\\avxs\\%d\\avla\\%d\\avcm\\%d\\wrmp\\%d\\ps\\%d",
	                                                level.arenas[arenaNum].settings.type,
	                                                level.arenas[arenaNum].settings.playersperteam,
	                                                level.arenas[arenaNum].settings.rounds,
	                                                level.arenas[arenaNum].settings.weapons,
	                                                level.arenas[arenaNum].settings.armor,
	                                                level.arenas[arenaNum].settings.health,
	                                                level.arenas[arenaNum].settings.lockarena,
	                                                level.arenas[arenaNum].settings.competitionmode,
	                                                level.arenas[arenaNum].settings.unbalanced,
	                                                level.arenas[arenaNum].settings.excessive,
	                                                level.arenas[arenaNum].settings.armorprotect,
	                                                level.arenas[arenaNum].settings.healthprotect,
	                                                level.arenas[arenaNum].settings.fallingdamage,
	                                                level.arenas[arenaNum].settings.allow_voting_gametype,
	                                                level.arenas[arenaNum].settings.allow_voting_healtharmor,
	                                                level.arenas[arenaNum].settings.allow_voting_playersperteam,
	                                                level.arenas[arenaNum].settings.allow_voting_rounds,
	                                                level.arenas[arenaNum].settings.allow_voting_healtharmorprotect,
	                                                level.arenas[arenaNum].settings.allow_voting_weapons,
	                                                level.arenas[arenaNum].settings.allow_voting_fallingdamage,
	                                                level.arenas[arenaNum].settings.allow_voting_grapple,
	                                                level.arenas[arenaNum].settings.allow_voting_excessive,
	                                                level.arenas[arenaNum].settings.allow_voting_lockarena,
	                                                level.arenas[arenaNum].settings.allow_voting_competitionmode,
	                                                level.arenas[arenaNum].state == ROUND_WARMUP ? 1 : 0,
	                                                level.arenas[arenaNum].state == ROUND_PAUSED ? level.arenas[arenaNum].pauseTime + level.arenas[arenaNum].timeoutSeconds * 1000 : 0 ) );
}

void arena_init_single( int arenaNum ) {
	level.arenas[arenaNum].state = ROUND_INIT;
	level.arenas[arenaNum].unknown1 = 2;
	level.arenas[arenaNum].numTeams = 0;
	level.arenas[arenaNum].nextCheckFrame = 0;
	level.arenas[arenaNum].countdown = 0;
	level.arenas[arenaNum].voteEndTime = 0;
	level.arenas[arenaNum].round = 0;

	switch ( level.arenas[arenaNum].settings.type ) {
		case AT_CLANARENA:
		case AT_REDROVER: {
			create_pickup_team( arenaNum, "Red Team" );
			create_pickup_team( arenaNum, "Blue Team" );
		} break;
	}

	set_arena_configstring( arenaNum );
}

void arena_init() {
	int i;

	level.restarting = 0;

	memset( level.arenas, 0, sizeof( level.arenas ) );
	memset( level.teams, 0, sizeof( level.teams ) );

	if ( level.lastArena == 0 ) {
		level.lastArena = 1;
		idmap = 1;
	} else {
		idmap = 0;
	}

	load_config( level.lastArena + 1 );
	set_config( 1, level.lastArena );

	for ( i = 0; i <= level.lastArena; i++ ) {
		arena_init_single( i );
	}

	load_motd();

	trueframetime = (int)( 1000 / trap_Cvar_VariableIntegerValue( "sv_fps" ) );
}

void G_ShowVotes( int arenaNum, const arenaSettings_t *ballot ) {
	const char *enabled[2] = { "off", "on" };

	char tmpa[1024];
	char tmpb[1024];
	int settingsLen;
	int i;
	int n;
	int firstWeapon;

	if ( level.arenas[arenaNum].settings.type != ballot->type ) {
		show_string( va( "print \"Settings changed: Type from %s to %s\n\"",
		                 arenatype_to_string( level.arenas[arenaNum].settings.type ),
		                 arenatype_to_string( ballot->type ) ),
		             arenaNum );
	}

	if ( level.arenas[arenaNum].settings.rounds != ballot->rounds ) {
		show_string( va( "print \"Settings changed: Rounds from %d to %d\n\"",
		                 level.arenas[arenaNum].settings.rounds, ballot->rounds ),
		             arenaNum );
	}

	if ( level.arenas[arenaNum].settings.health != ballot->health ) {
		show_string( va( "print \"Settings changed: Health from %d to %d\n\"",
		                 level.arenas[arenaNum].settings.health, ballot->health ),
		             arenaNum );
	}

	if ( level.arenas[arenaNum].settings.armor != ballot->armor ) {
		/* FIXME should print Armor */
		show_string( va( "print \"Settings changed: Health from %d to %d\n\"",
		                 level.arenas[arenaNum].settings.armor, ballot->armor ),
		             arenaNum );
	}

	if ( level.arenas[arenaNum].settings.playersperteam != ballot->playersperteam ) {
		show_string( va( "print \"Settings changed: Players per team from %d to %d\n\"",
		                 level.arenas[arenaNum].settings.playersperteam, ballot->playersperteam ),
		             arenaNum );
	}

	if ( level.arenas[arenaNum].settings.competitionmode != ballot->competitionmode ) {
		show_string( va( "print \"Settings changed: Competition Mode set to: %s\n\"",
		                 enabled[ballot->competitionmode] ),
		             arenaNum );
	}

	if ( level.arenas[arenaNum].settings.unbalanced != ballot->unbalanced ) {
		show_string( va( "print \"Settings changed: Allow unbalanced teams set to: %s\n\"",
		                 enabled[ballot->unbalanced] ),
		             arenaNum );
	}

	if ( level.arenas[arenaNum].settings.armorprotect != ballot->armorprotect ) {
		show_string( va( "print \"Settings changed: Armorprotect to %s\n\"",
		                 protect[ballot->armorprotect] ),
		             arenaNum );
	}

	if ( level.arenas[arenaNum].settings.healthprotect != ballot->healthprotect ) {
		show_string( va( "print \"Settings changed: Healthprotect to %s\n\"",
		                 protect[ballot->healthprotect] ),
		             arenaNum );
	}

	if ( level.arenas[arenaNum].settings.fallingdamage != ballot->fallingdamage ) {
		show_string( va( "print \"Settings changed: Fallingdamage set to %s\n\"",
		                 enabled[ballot->fallingdamage] ),
		             arenaNum );
	}

	if ( level.arenas[arenaNum].settings.weapons != ballot->weapons ) {
		settingsLen = 0;
		firstWeapon = 1;

		for ( i = 0; i < 8; i++ ) {
			if ( ( ballot->weapons & ( 1 << i ) ) != ( level.arenas[arenaNum].settings.weapons & ( 1 << i ) ) ) {
				if ( firstWeapon ) {
					Com_sprintf( tmpb, sizeof( tmpb ), " %s to %s", WeapShort( i ), enabled[( ballot->weapons >> i ) & 1] );
					firstWeapon = 0;
				} else {
					Com_sprintf( tmpb, sizeof( tmpb ), ", %s to %s", WeapShort( i ), enabled[( ballot->weapons >> i ) & 1] );
				}

				n = strlen( tmpb );

				if ( settingsLen + n > (int)sizeof( tmpa ) ) {
					break;
				}

				memcpy( tmpa + settingsLen, tmpb, n );
				settingsLen += n;
			}
		}

		tmpa[settingsLen] = 0;

		show_string( va( "print \"Settings changed:%s\n\"", tmpa ), arenaNum );
	}
}

const char *WeapShort( int id ) {
	switch ( id ) {
	case 0:
		return "MG";
	case 1:
		return "SG";
	case 2:
		return "GL";
	case 3:
		return "RL";
	case 4:
		return "LG";
	case 5:
		return "RG";
	case 6:
		return "PG";
	case 7:
		return "BFG";
	default:
		return "";
	}
}
