// Copyright (C) 1999-2000 Id Software, Inc.
//

/*****************************************************************************
 * name:		be_ai_char.h
 *
 * desc:		bot characters
 *
 * $Archive: /source/code/botlib/be_ai_char.h $
 * $Author: Mrelusive $ 
 * $Revision: 2 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 10/05/99 3:42p $
 *
 *****************************************************************************/

//loads a bot character from a file
int BotLoadCharacter(char *charfile, int skill);
//frees a bot character
void BotFreeCharacter(int character);
//float characteristic
float Characteristic_Float(int character, int index);
//bounded float characteristic
float Characteristic_BFloat(int character, int index, float min, float max);
//integer characteristic
int Characteristic_Integer(int character, int index);
//bounded integer characteristic
int Characteristic_BInteger(int character, int index, int min, int max);
//string characteristic
void Characteristic_String(int character, int index, char *buf, int size);
//free cached bot characters
void BotShutdownCharacters(void);
