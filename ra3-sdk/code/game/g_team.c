// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"

typedef enum _flag_status {
	FLAG_ATBASE = 0,
	FLAG_TAKEN,
	FLAG_DROPPED
} flagStatus_t;

typedef struct teamgame_s
{
	float last_flag_capture;
	int last_capture_team;
	flagStatus_t redStatus;
	flagStatus_t blueStatus;
} teamgame_t;

teamgame_t teamgame;

void Team_SetFlagStatus( int team, flagStatus_t status );

void Team_InitGame(void)
{
	memset(&teamgame, 0, sizeof teamgame);
	teamgame.redStatus = teamgame.blueStatus = -1; // Invalid to force update

	Team_SetFlagStatus( TEAM_RED, FLAG_ATBASE );
	Team_SetFlagStatus( TEAM_BLUE, FLAG_ATBASE );
}

int OtherTeam(int team) {
	if (team==TEAM_RED)
		return TEAM_BLUE;
	else if (team==TEAM_BLUE)
		return TEAM_RED;
	return team;
}

const char *TeamName(int team)  {
	if (team==TEAM_RED)
		return "RED";
	else if (team==TEAM_BLUE)
		return "BLUE";
	else if (team==TEAM_SPECTATOR)
		return "SPECTATOR";
	return "FREE";
}

const char *OtherTeamName(int team) {
	if (team==TEAM_RED)
		return "BLUE";
	else if (team==TEAM_BLUE)
		return "RED";
	else if (team==TEAM_SPECTATOR)
		return "SPECTATOR";
	return "FREE";
}

const char *TeamColorString(int team) {
	if (team==TEAM_RED)
		return S_COLOR_RED;
	else if (team==TEAM_BLUE)
		return S_COLOR_BLUE;
	else if (team==TEAM_SPECTATOR)
		return S_COLOR_YELLOW;
	return S_COLOR_WHITE;
}

// NULL for everyone
void QDECL PrintMsg( gentity_t *ent, const char *fmt, ... ) {
	char		msg[1024];
	va_list		argptr;
	char		*p;
	
	va_start (argptr,fmt);
	if (vsprintf (msg, fmt, argptr) > sizeof(msg)) {
		G_Error ( "PrintMsg overrun" );
	}
	// FIXME why was this removed?
	// va_end (argptr);

	// double quotes are bad
	while ((p = strchr(msg, '"')) != NULL)
		*p = '\'';

	trap_SendServerCommand ( ( (ent == NULL) ? -1 : ent-g_entities ), va("print \"%s\"", msg ));
}

/*
=======
G_BalanceTeams
==============
*/
void G_BalanceTeams( int arenaNum ) {
	float			teamScore[2];
	float			balance;
	float			deltaScore;
	int				i;
	int				redPlayers;
	int				bluePlayers;
	int				bestPlayer;
	int				blueTeam;
	int				redTeam;
	gentity_t		*ent;
	unsigned int	minutes;
	team_t			oldSessionTeam;
	team_t			newSessionTeam;
	int				oldTeam;
	int				newTeam;
	int				oldPlayers;
	int				newPlayers;
	int				bestTeam;
	int				thisMinutes;
	int				bestMinutes;
	float			thisScore;
	float			bestScore;
	float			thisDelta;
	float			bestDelta;

	teamScore[0] = 0.0f;
	teamScore[1] = 0.0f;
	balance = g_autoBalance.value;
	bestPlayer = -1;
	blueTeam = -1;
	redTeam = -1;

	if ( level.arenas[arenaNum].settings.type != AT_CLANARENA || level.arenas[arenaNum].settings.competitionmode != 0 ||
	     g_autoBalance.value == 0.0f ) {
		return;
	}

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse == qfalse ||
		     ent->client->pers.connected != CON_CONNECTED ||
		     ent->client->ps.persistant[PERS_ARENA] != arenaNum ||
		     ent->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
			continue;
		}

		minutes = ( level.time - ent->client->pers.enterTime ) / 60000;

		if ( level.teams[ent->client->ps.persistant[PERS_TEAM]].sessionTeam == TEAM_BLUE ) {
			if ( blueTeam < 0 ) {
				blueTeam = ent->client->ps.persistant[PERS_TEAM];
			}

			/* FIXME making this part of a union / structure made it load blueScore onto the stack
			   first before adding, the original code may have all of the variables in a structure? */
			teamScore[0] += ent->client->pers.damage / ( minutes > 0 ? minutes : 1.0f );
		}

		if ( level.teams[ent->client->ps.persistant[PERS_TEAM]].sessionTeam == TEAM_RED ) {
			if ( redTeam < 0 ) {
				redTeam = ent->client->ps.persistant[PERS_TEAM];
			}

			teamScore[1] += ent->client->pers.damage/ ( minutes > 0 ? minutes : 1.0f );
		}
	}

	if ( redTeam < 0 || blueTeam < 0 ) {
		return;
	}

	redPlayers = count_players_in_team( redTeam );
	bluePlayers = count_players_in_team( blueTeam );

	deltaScore = teamScore[0] > teamScore[1] ? teamScore[0] - teamScore[1] : teamScore[1] - teamScore[0];

	G_Printf( "Autobalance: (%f, %f) %f.\n", teamScore[0], teamScore[1], deltaScore );

	if ( deltaScore > balance ) {
		if ( teamScore[0] > teamScore[1] ) {
			if ( level.teamScores[TEAM_BLUE] <= level.teamScores[TEAM_RED] || bluePlayers < redPlayers ) {
				return;
			}

			bestTeam = blueTeam;
		} else {
			if ( level.teamScores[TEAM_RED] <= level.teamScores[TEAM_BLUE] || redPlayers < bluePlayers ) {
				return;
			}

			bestTeam = redTeam;
		}

		for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
			if ( ent->inuse == qfalse || ent->client->pers.connected != CON_CONNECTED ||
			     ent->client->ps.persistant[PERS_ARENA] != arenaNum || ent->client->ps.persistant[PERS_TEAM] != bestTeam ) {
				continue;
			}

			if ( bestPlayer < 0 ) {
				bestPlayer = i;
			}

			// FIXME this should use level.clients[bestPlayer] otherwise the best player will always
			// be the first valid player on the best team (as ent->client == level.clients[i])
			thisMinutes = ( level.time - ent->client->pers.enterTime ) / 60000;
			bestMinutes = ( level.time - level.clients[i].pers.enterTime ) / 60000;

			thisScore = ent->client->pers.damage / ( thisMinutes > 0 ? thisMinutes : 1.0f );
			bestScore = level.clients[i].pers.damage / ( bestMinutes > 0 ? bestMinutes : 1.0f );

			thisDelta = deltaScore > thisScore ? deltaScore - thisScore : thisScore - deltaScore;
			bestDelta = deltaScore > bestScore ? deltaScore - bestScore : bestScore - deltaScore;

			if ( thisDelta < bestDelta ) {
				bestPlayer = i;
			}
		}

		if ( bestPlayer < 0 ) {
			return;
		}

		oldTeam = level.clients[bestPlayer].ps.persistant[PERS_TEAM];
		oldSessionTeam = level.teams[oldTeam].sessionTeam;
		oldPlayers = oldTeam == blueTeam ? bluePlayers : redPlayers;

		newSessionTeam = oldSessionTeam == TEAM_BLUE ? TEAM_RED : TEAM_BLUE;
		newTeam = oldTeam == blueTeam ? redTeam : blueTeam;
		newPlayers = newTeam == blueTeam ? bluePlayers : redPlayers;

		if ( ( oldPlayers - 1 ) <= 0 ) {
			return;
		}

		level.clients[bestPlayer].ps.persistant[PERS_TEAM] = newTeam;

		set_sessionteam_and_skin( g_entities + bestPlayer, newSessionTeam );

		// FIXME this should use newTeam not newSessionTeam
		show_string( va( "print \"Teams autobalanced. %s" S_COLOR_WHITE " moved to the %s" S_COLOR_WHITE " team.\n\"",
		                 level.clients[bestPlayer].pers.netname, level.teams[newSessionTeam].name ),
		             arenaNum );
	}
}

/*
==============
OnSameTeam
==============
*/
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 ) {
	if ( !ent1->client || !ent2->client ) {
		return qfalse;
	}

	if ( g_gametype.integer < GT_TEAM ) {
		return qfalse;
	}

	if ( ent1->client->ps.persistant[PERS_TEAM] == ent2->client->ps.persistant[PERS_TEAM] ) {
		return qtrue;
	}

	return qfalse;
}

void Team_SetFlagStatus( int team, flagStatus_t status )
{
	qboolean modified = qfalse;

	switch (team) {
	case TEAM_RED :
		if ( teamgame.redStatus != status ) {
			teamgame.redStatus = status;
			modified = qtrue;
		}
		break;
	case TEAM_BLUE :
		if ( teamgame.blueStatus != status ) {
			teamgame.blueStatus = status;
			modified = qtrue;
		}
		break;
	}

	if (modified) {
		char st[4];

		st[0] = '0' + (int)teamgame.redStatus;
		st[1] = '0' + (int)teamgame.blueStatus;
		st[2] = 0;

		trap_SetConfigstring( CS_FLAGSTATUS, st );
	}
}

void Team_CheckDroppedItem( gentity_t *dropped )
{
	if (dropped->item->giTag == PW_REDFLAG)
		Team_SetFlagStatus( TEAM_RED, FLAG_DROPPED );
	else if (dropped->item->giTag == PW_BLUEFLAG)
		Team_SetFlagStatus( TEAM_BLUE, FLAG_DROPPED );
}


/*
================
Team_FragBonuses

Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumlative.  You get one, they are in importance
order.
================
*/
void Team_FragBonuses(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker)
{
	int i;
	gentity_t *ent;
	int flag_pw, enemy_flag_pw;
	int otherteam;
	gentity_t *flag, *carrier = NULL;
	char *c;
	vec3_t v1, v2;
	int team;

	// no bonus for fragging yourself
	if (!targ->client || !attacker->client || targ == attacker)
		return;

	team = targ->client->sess.sessionTeam;
	otherteam = OtherTeam(targ->client->sess.sessionTeam);
	if (otherteam < 0)
		return; // whoever died isn't on a team

	// same team, if the flag at base, check to he has the enemy flag
	if (team == TEAM_RED) {
		flag_pw = PW_REDFLAG;
		enemy_flag_pw = PW_BLUEFLAG;
	} else {
		flag_pw = PW_BLUEFLAG;
		enemy_flag_pw = PW_REDFLAG;
	}

	// did the attacker frag the flag carrier?
	if (targ->client->ps.powerups[enemy_flag_pw]) {
		attacker->client->pers.teamState.lastfraggedcarrier = level.time;
		AddScore(attacker, CTF_FRAG_CARRIER_BONUS);
		attacker->client->pers.teamState.fragcarrier++;
		PrintMsg(NULL, "%s" S_COLOR_WHITE " fragged %s's flag carrier!\n",
			attacker->client->pers.netname, TeamName(team));

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for (i = 0; i < g_maxclients.integer; i++) {
			ent = g_entities + i;
			if (ent->inuse && ent->client->sess.sessionTeam == otherteam)
				ent->client->pers.teamState.lasthurtcarrier = 0;
		}
		return;
	}

	if (targ->client->pers.teamState.lasthurtcarrier &&
		level.time - targ->client->pers.teamState.lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT &&
		!attacker->client->ps.powerups[flag_pw]) {
		// attacker is on the same team as the flag carrier and
		// fragged a guy who hurt our flag carrier
		AddScore(attacker, CTF_CARRIER_DANGER_PROTECT_BONUS);

		attacker->client->pers.teamState.carrierdefense++;
		targ->client->pers.teamState.lasthurtcarrier = 0;

		team = attacker->client->sess.sessionTeam;
		PrintMsg(NULL, "%s" S_COLOR_WHITE " defends %s's flag carrier against an agressive enemy\n",
			attacker->client->pers.netname, TeamName(team));
		return;
	}

	// flag and flag carrier area defense bonuses

	// we have to find the flag and carrier entities

	// find the flag
	switch (attacker->client->sess.sessionTeam) {
	case TEAM_RED:
		c = "team_CTF_redflag";
		break;
	case TEAM_BLUE:
		c = "team_CTF_blueflag";
		break;		
	default:
		return;
	}

	flag = NULL;
	while ((flag = G_Find (flag, FOFS(classname), c)) != NULL) {
		if (!(flag->flags & FL_DROPPED_ITEM))
			break;
	}

	if (!flag)
		return; // can't find attacker's flag

	// find attacker's team's flag carrier
	for (i = 0; i < g_maxclients.integer; i++) {
		carrier = g_entities + i;
		if (carrier->inuse && carrier->client->ps.powerups[flag_pw])
			break;
		carrier = NULL;
	}

	// ok we have the attackers flag and a pointer to the carrier

	// check to see if we are defending the base's flag
	VectorSubtract(targ->r.currentOrigin, flag->r.currentOrigin, v1);
	VectorSubtract(attacker->r.currentOrigin, flag->r.currentOrigin, v2);

	if ( ( ( VectorLength(v1) < CTF_TARGET_PROTECT_RADIUS &&
		trap_InPVS(flag->r.currentOrigin, targ->r.currentOrigin ) ) ||
		( VectorLength(v2) < CTF_TARGET_PROTECT_RADIUS &&
		trap_InPVS(flag->r.currentOrigin, attacker->r.currentOrigin ) ) ) &&
		attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam) {

		// we defended the base flag
		AddScore(attacker, CTF_FLAG_DEFENSE_BONUS);
		attacker->client->pers.teamState.basedefense++;
		if (flag->r.svFlags & SVF_NOCLIENT) {
			PrintMsg(NULL, "%s" S_COLOR_WHITE " defends the %s base.\n",
				attacker->client->pers.netname, 
				TeamName(attacker->client->sess.sessionTeam));
		} else {
			PrintMsg(NULL, "%s" S_COLOR_WHITE " defends the %s flag.\n",
				attacker->client->pers.netname, 
				TeamName(attacker->client->sess.sessionTeam));
		}
		return;
	}

	if (carrier && carrier != attacker) {
		VectorSubtract(targ->r.currentOrigin, carrier->r.currentOrigin, v1);
		VectorSubtract(attacker->r.currentOrigin, carrier->r.currentOrigin, v1);

		if ( ( ( VectorLength(v1) < CTF_ATTACKER_PROTECT_RADIUS &&
			trap_InPVS(carrier->r.currentOrigin, targ->r.currentOrigin ) ) ||
			( VectorLength(v2) < CTF_ATTACKER_PROTECT_RADIUS &&
				trap_InPVS(carrier->r.currentOrigin, attacker->r.currentOrigin ) ) ) &&
			attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam) {
			AddScore(attacker, CTF_CARRIER_PROTECT_BONUS);
			attacker->client->pers.teamState.carrierdefense++;
			PrintMsg(NULL, "%s" S_COLOR_WHITE " defends the %s's flag carrier.\n",
				attacker->client->pers.netname, 
				TeamName(attacker->client->sess.sessionTeam));
			return;
		}
	}
}

/*
================
Team_CheckHurtCarrier

Check to see if attacker hurt the flag carrier.  Needed when handing out bonuses for assistance to flag
carrier defense.
================
*/
void Team_CheckHurtCarrier(gentity_t *targ, gentity_t *attacker)
{
	int flag_pw;

	if (!targ->client || !attacker->client)
		return;

	if (targ->client->sess.sessionTeam == TEAM_RED)
		flag_pw = PW_BLUEFLAG;
	else
		flag_pw = PW_REDFLAG;

	if (targ->client->ps.powerups[flag_pw] &&
		targ->client->sess.sessionTeam != attacker->client->sess.sessionTeam)
		attacker->client->pers.teamState.lasthurtcarrier = level.time;
}


gentity_t *Team_ResetFlag(int team)
{
	char *c;
	gentity_t *ent, *rent = NULL;

	switch (team) {
	case TEAM_RED:
		c = "team_CTF_redflag";
		break;
	case TEAM_BLUE:
		c = "team_CTF_blueflag";
		break;
	default:
		return NULL;
	}

	ent = NULL;
	while ((ent = G_Find (ent, FOFS(classname), c)) != NULL) {
		if (ent->flags & FL_DROPPED_ITEM)
			G_FreeEntity(ent);
		else {
			rent = ent;
			RespawnItem(ent);
		}
	}

	Team_SetFlagStatus( team, FLAG_ATBASE );

	return rent;
}

void Team_ResetFlags(void)
{
	Team_ResetFlag(TEAM_RED);
	Team_ResetFlag(TEAM_BLUE);
}

void Team_ReturnFlagSound(gentity_t *ent, int team)
{
	// play powerup spawn sound to all clients
	gentity_t	*te;

	if (ent == NULL) {
		G_Printf ("Warning:  NULL passed to Team_ReturnFlagSound\n");
		return;
	}

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
	te->s.eventParm = G_SoundIndex( team == TEAM_RED ?
		"sound/teamplay/flagret_red.wav" :
		"sound/teamplay/flagret_blu.wav" );
	te->r.svFlags |= SVF_BROADCAST;
}

void Team_ReturnFlag(int team)
{
	Team_ReturnFlagSound(Team_ResetFlag(team), team);
	PrintMsg(NULL, "The %s flag has returned!\n", TeamName(team));
}

void Team_FreeEntity(gentity_t *ent)
{
	if (ent->item->giTag == PW_REDFLAG)
		Team_ReturnFlag(TEAM_RED);
	else if (ent->item->giTag == PW_BLUEFLAG)
		Team_ReturnFlag(TEAM_BLUE);
}

/*
==============
Team_DroppedFlagThink

Automatically set in Launch_Item if the item is one of the flags

Flags are unique in that if they are dropped, the base flag must be respawned when they time out
==============
*/
void Team_DroppedFlagThink(gentity_t *ent)
{
	if (ent->item->giTag == PW_REDFLAG)
		Team_ReturnFlagSound(Team_ResetFlag(TEAM_RED), TEAM_RED);
	else if (ent->item->giTag == PW_BLUEFLAG)
		Team_ReturnFlagSound(Team_ResetFlag(TEAM_BLUE), TEAM_BLUE);
	// Reset Flag will delete this entity
}

int Team_TouchOurFlag( gentity_t *ent, gentity_t *other, int team ) {
	int i;
	gentity_t *player;
	gclient_t *cl = other->client;
	int our_flag, enemy_flag;
	gentity_t	*te;

	if (cl->sess.sessionTeam == TEAM_RED) {
		our_flag = PW_REDFLAG;
		enemy_flag = PW_BLUEFLAG;
	} else {
		our_flag = PW_BLUEFLAG;
		enemy_flag = PW_REDFLAG;
	}

	if ( ent->flags & FL_DROPPED_ITEM ) {
		// hey, its not home.  return it by teleporting it back
		PrintMsg( NULL, "%s" S_COLOR_WHITE " returned the %s flag!\n", 
			cl->pers.netname, TeamName(team));
		AddScore(other, CTF_RECOVERY_BONUS);
		other->client->pers.teamState.flagrecovery++;
		other->client->pers.teamState.lastreturnedflag = level.time;
		//ResetFlag will remove this entity!  We must return zero
		Team_ReturnFlagSound(Team_ResetFlag(team), team);
		return 0;
	}

	// the flag is at home base.  if the player has the enemy
	// flag, he's just won!
	if (!cl->ps.powerups[enemy_flag])
		return 0; // We don't have the flag

	PrintMsg( NULL, "%s" S_COLOR_WHITE " captured the %s flag!\n",
		cl->pers.netname, TeamName(OtherTeam(team)));

	cl->ps.powerups[enemy_flag] = 0;

	teamgame.last_flag_capture = level.time;
	teamgame.last_capture_team = team;

	// Increase the team's score
	level.teamScores[ other->client->sess.sessionTeam ]++;

	other->client->pers.teamState.captures++;

	// other gets another 10 frag bonus
	AddScore(other, CTF_CAPTURE_BONUS);

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
	te->s.eventParm = G_SoundIndex( our_flag == PW_REDFLAG ?
		"sound/teamplay/flagcap_red.wav" :
		"sound/teamplay/flagcap_blu.wav" );
	te->r.svFlags |= SVF_BROADCAST;

	// Ok, let's do the player loop, hand out the bonuses
	for (i = 0; i < g_maxclients.integer; i++) {
		player = &g_entities[i];
		if (!player->inuse)
			continue;

		if (player->client->sess.sessionTeam !=
			cl->sess.sessionTeam) {
			player->client->pers.teamState.lasthurtcarrier = -5;
		} else if (player->client->sess.sessionTeam ==
			cl->sess.sessionTeam) {
			if (player != other)
				AddScore(player, CTF_TEAM_BONUS);
			// award extra points for capture assists
			if (player->client->pers.teamState.lastreturnedflag + 
				CTF_RETURN_FLAG_ASSIST_TIMEOUT > level.time) {
				PrintMsg( NULL, 
						"%s" S_COLOR_WHITE " gets an assist for returning the %s flag!\n", 
						player->client->pers.netname,
						TeamName(team));
				AddScore (player, CTF_RETURN_FLAG_ASSIST_BONUS);
				other->client->pers.teamState.assists++;
			}
			if (player->client->pers.teamState.lastfraggedcarrier + 
				CTF_FRAG_CARRIER_ASSIST_TIMEOUT > level.time) {
				PrintMsg( NULL, "%s" S_COLOR_WHITE " gets an assist for fragging the %s flag carrier!\n", 
						player->client->pers.netname,
						TeamName(OtherTeam(team)));
				AddScore(player, CTF_FRAG_CARRIER_ASSIST_BONUS);
				other->client->pers.teamState.assists++;
			}
		}
	}
	Team_ResetFlags();

	CalculateRanks();

	return 0; // Do not respawn this automatically
}

int Team_TouchEnemyFlag( gentity_t *ent, gentity_t *other, int team ) {
	gclient_t *cl = other->client;

	// hey, its not our flag, pick it up
	PrintMsg (NULL, "%s" S_COLOR_WHITE " got the %s flag!\n",
		other->client->pers.netname, TeamName(team));
	AddScore(other, CTF_FLAG_BONUS);

	if (team == TEAM_RED)
		cl->ps.powerups[PW_REDFLAG] = INT_MAX; // flags never expire
	else
		cl->ps.powerups[PW_BLUEFLAG] = INT_MAX; // flags never expire

	cl->pers.teamState.flagsince = level.time;

	Team_SetFlagStatus( team, FLAG_TAKEN );
	
	return -1; // Do not respawn this automatically, but do delete it if it was FL_DROPPED
}

int Pickup_Team( gentity_t *ent, gentity_t *other ) {
	int team;
	gclient_t *cl = other->client;
	
	// figure out what team this flag is
	if (strcmp(ent->classname, "team_CTF_redflag") == 0)
		team = TEAM_RED;
	else if (strcmp(ent->classname, "team_CTF_blueflag") == 0)
		team = TEAM_BLUE;
	else {
		PrintMsg ( other, "Don't know what team the flag is on.\n");
		return 0;
	}

	return ((team == cl->sess.sessionTeam) ?
		Team_TouchOurFlag : Team_TouchEnemyFlag)
			(ent, other, team);
}

/*
===========
Target_GetLocation
===========
*/
int Target_GetLocation( gentity_t *ent, char *out, int outSize ) {
	vec3_t		forward;
	vec3_t		right;
	vec3_t		up;
	vec3_t		end;
	vec3_t		start;
	trace_t		tr;
	gentity_t	*loc;
	gentity_t	*best;
	float		bestDist;
	float		dist;
	vec3_t		origin;

	AngleVectors( ent->client->ps.viewangles, forward, right, up );
	VectorCopy( ent->s.pos.trBase, start );
	start[2] += ent->client->ps.viewheight;
	VectorMA( start, 14.0f, forward, start );
	SnapVector( start );
	VectorMA( start, 8192.0f, forward, end );

	trap_Trace( &tr, start, NULL, NULL, end, ent->s.number, -1 );
	SnapVectorTowards( tr.endpos, start );

	best = NULL;
	bestDist = 3 * 8192.0 * 8192.0;
	VectorCopy( tr.endpos, origin );

	for ( loc = level.locationHead; loc; loc = loc->nextTrain ) {
		dist = ( origin[0] - loc->r.currentOrigin[0] ) * ( origin[0] - loc->r.currentOrigin[0] ) +
		       ( origin[1] - loc->r.currentOrigin[1] ) * ( origin[1] - loc->r.currentOrigin[1] ) +
		       ( origin[2] - loc->r.currentOrigin[2] ) * ( origin[2] - loc->r.currentOrigin[2] );

		if ( dist > bestDist ) {
			continue;
		}

		if ( !trap_InPVS( origin, loc->r.currentOrigin ) ) {
			continue;
		}

		bestDist = dist;
		best = loc;
	}

	if ( !best ) {
		return 0;
	}

	if ( best->count ) {
		if ( best->count < 0 ) {
			best->count = 0;
		}
		if ( best->count > 7 ) {
			best->count = 7;
		}
		Com_sprintf( out, outSize, "%c%c%s" S_COLOR_WHITE "", Q_COLOR_ESCAPE, '0' + best->count, best->message );
	} else {
		Com_sprintf( out, outSize, "%s", best->message );
	}

	return 1;
}

/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
gentity_t *Team_GetLocation(gentity_t *ent)
{
	gentity_t		*eloc, *best;
	float			bestlen, len;
	vec3_t			origin;

	best = NULL;
	bestlen = 3*8192.0*8192.0;

	VectorCopy( ent->r.currentOrigin, origin );

	for (eloc = level.locationHead; eloc; eloc = eloc->nextTrain) {
		len = ( origin[0] - eloc->r.currentOrigin[0] ) * ( origin[0] - eloc->r.currentOrigin[0] )
			+ ( origin[1] - eloc->r.currentOrigin[1] ) * ( origin[1] - eloc->r.currentOrigin[1] )
			+ ( origin[2] - eloc->r.currentOrigin[2] ) * ( origin[2] - eloc->r.currentOrigin[2] );

		if ( len > bestlen ) {
			continue;
		}

		if ( !trap_InPVS( origin, eloc->r.currentOrigin ) ) {
			continue;
		}

		bestlen = len;
		best = eloc;
	}

	return best;
}


/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
qboolean Team_GetLocationMsg(gentity_t *ent, char *loc, int loclen)
{
	gentity_t *best;

	best = Team_GetLocation( ent );
	
	if (!best)
		return qfalse;

	if (best->count) {
		if (best->count < 0)
			best->count = 0;
		if (best->count > 7)
			best->count = 7;
		Com_sprintf(loc, loclen, "%c%c%s" S_COLOR_WHITE, Q_COLOR_ESCAPE, best->count + '0', best->message );
	} else
		Com_sprintf(loc, loclen, "%s", best->message);

	return qtrue;
}


/*---------------------------------------------------------------------------*/

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_TEAM_SPAWN_POINTS	16
gentity_t *SelectRandomTeamSpawnPoint( int teamstate, team_t team ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_TEAM_SPAWN_POINTS];
	char		*classname;

	if (teamstate == TEAM_BEGIN) {
		if (team == TEAM_RED)
			classname = "team_CTF_redplayer";
		else if (team == TEAM_BLUE)
			classname = "team_CTF_blueplayer";
		else
			return NULL;
	} else {
		if (team == TEAM_RED)
			classname = "team_CTF_redspawn";
		else if (team == TEAM_BLUE)
			classname = "team_CTF_bluespawn";
		else
			return NULL;
	}
	count = 0;

	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), classname)) != NULL) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		spots[ count ] = spot;
		if (++count == MAX_TEAM_SPAWN_POINTS)
			break;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), classname);
	}

	selection = rand() % count;
	return spots[ selection ];
}


/*
===========
SelectCTFSpawnPoint

============
*/
gentity_t *SelectCTFSpawnPoint ( team_t team, int teamstate, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;

	spot = SelectRandomTeamSpawnPoint ( teamstate, team );

	if (!spot) {
		return SelectSpawnPoint( vec3_origin, origin, angles );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*---------------------------------------------------------------------------*/

static int QDECL SortClients( const void *a, const void *b ) {
	return *(int *)a - *(int *)b;
}


/*
==================
TeamplayLocationsMessage

Format:
	clientNum location health armor weapon powerups

==================
*/
void TeamplayInfoMessage( gentity_t *ent ) {
	char		entry[1024];
	char		string[1400];
	int			stringlength;
	int			i, j;
	gentity_t	*player;
	int			cnt;
	int			h, a;

	// FIXME unused
	(void)SortClients;

	if ( ! ent->client->pers.teamInfo )
		return;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;

	for (i = 0, cnt = 0; i < g_maxclients.integer && cnt < TEAM_MAXOVERLAY; i++) {
		player = g_entities + i;

		if ( player->inuse == qfalse ||
		     player->client->sess.sessionTeam != ent->client->sess.sessionTeam ||
		     player == ent ||
		     player->client->ps.persistant[PERS_ARENA] != ent->client->ps.persistant[PERS_ARENA] ) {
			continue;
		}

		h = player->client->ps.stats[STAT_HEALTH];
		a = player->client->ps.stats[STAT_ARMOR];

		if (h < 0) h = 0;
		if (a < 0) a = 0;

		Com_sprintf (entry, sizeof(entry),
			" %i %i %i %i %i %i", 
			i, player->client->pers.teamState.location, h, a, 
			player->client->ps.weapon, player->s.powerups);
		j = strlen(entry);
		if (stringlength + j > sizeof(string))
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
		cnt++;
	}

	if ( cnt > 0 ) {
		trap_SendServerCommand( ent-g_entities, va("tinfo %i%s", cnt, string) );
	}
}

void CheckTeamStatus(void)
{
	int i;
	gentity_t *loc, *ent;

	if (level.time - level.lastTeamLocationTime > TEAM_LOCATION_UPDATE_TIME) {

		level.lastTeamLocationTime = level.time;

		for (i = 0; i < g_maxclients.integer; i++) {
			ent = g_entities + i;
			if (ent->inuse && 
				(ent->client->sess.sessionTeam == TEAM_RED ||
				ent->client->sess.sessionTeam == TEAM_BLUE)) {
				loc = Team_GetLocation( ent );
				if (loc)
					ent->client->pers.teamState.location = loc->health;
				else
					ent->client->pers.teamState.location = 0;
			}
		}

		for (i = 0; i < g_maxclients.integer; i++) {
			ent = g_entities + i;
			if (ent->inuse && 
				(ent->client->sess.sessionTeam == TEAM_RED ||
				ent->client->sess.sessionTeam == TEAM_BLUE)) {
				TeamplayInfoMessage( ent );
			}
		}
	}
}

/*-----------------------------------------------------------------*/

/*QUAKED team_CTF_redplayer (1 0 0) (-16 -16 -16) (16 16 32)
Only in CTF games.  Red players spawn here at game start.
*/
void SP_team_CTF_redplayer( gentity_t *ent ) {
}


/*QUAKED team_CTF_blueplayer (0 0 1) (-16 -16 -16) (16 16 32)
Only in CTF games.  Blue players spawn here at game start.
*/
void SP_team_CTF_blueplayer( gentity_t *ent ) {
}


/*QUAKED team_CTF_redspawn (1 0 0) (-16 -16 -24) (16 16 32)
potential spawning position for red team in CTF games.
Targets will be fired when someone spawns in on them.
*/
void SP_team_CTF_redspawn(gentity_t *ent) {
}

/*QUAKED team_CTF_bluespawn (0 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for blue team in CTF games.
Targets will be fired when someone spawns in on them.
*/
void SP_team_CTF_bluespawn(gentity_t *ent) {
}



