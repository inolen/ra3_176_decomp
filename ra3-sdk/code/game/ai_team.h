// Copyright (C) 1999-2000 Id Software, Inc.
//

/*****************************************************************************
 * name:		ai_team.h
 *
 * desc:		Quake3 bot AI
 *
 * $Archive: /source/code/botai/ai_chat.c $
 * $Author: Mrelusive $ 
 * $Revision: 21 $
 * $Modtime: 11/10/99 3:30p $
 * $Date: 11/10/99 6:08p $
 *
 *****************************************************************************/

void BotTeamAI(bot_state_t *bs);
int BotGetTeamMateCTFPreference(bot_state_t *bs, int teammate);
void BotSetTeamMateCTFPreference(bot_state_t *bs, int teammate, int preference);
