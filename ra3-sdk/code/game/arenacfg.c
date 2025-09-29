#include "g_local.h"

#define MAX_DEF_BLOCKS 256
#define MAX_DEF_DEPTH 3

enum {
	DT_NONE,
	DT_STRING,
	DT_BLOCK
};

typedef struct defblock_s {
	int numKeys;
	int numValues;
	char *key;
	int type;
	union {
		struct defblock_s *block;
		char *str;
	} value;
} defblock_t;

typedef struct {
	int count;
	defblock_t *block;
	defblock_t *item;
} defframe_t;

int votetries_setting = 2;
int def_pickup_type = AT_CLANARENA;

int num_definition_blocks = 0;
defblock_t *definition_blocks = NULL;

int weapon_vals[WP_NUM_WEAPONS - WP_MACHINEGUN] = {
	0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100,
};

int weapons;
int armor;
int health;
int playersperteam;
int rounds;
int minping;
int maxping;
int game_type;
int rocket_speed;
int shells;
int bullets;
int slugs;
int grenades;
int rockets;
int cells;
int plasma;
int bfgammo;
int fastswitch;
int armorprotect;
int healthprotect;
int fallingdamage;
int allow_voting_healtharmor;
int allow_voting_gametype;
int allow_voting_ping;
int allow_voting_playersperteam;
int allow_voting_rounds;
int allow_voting_maxteams;
int allow_voting_healtharmorprotect;
int allow_voting_weapons;
int allow_voting_fallingdamage;
int allow_voting_grapple;
int allow_voting_excessive;
int allow_voting_competitionmode;
int allow_voting_lockarena;
int lock_arena;
int lock_count;
int competition_mode;
int unbalanced;
int excessive;
int maxplayers;

defframe_t stack[32];
const defblock_t *map_loop;
const defblock_t *map_block;
const defblock_t **arena_blocks;

int has_val( const char *haystack, const char *needle ) {
	char tmp[1024];
	char *tok;

	strcpy( tmp, haystack );

	tok = strtok( tmp, " " );

	while ( tok ) {
		if ( !strcmp( tok, needle ) ) {
			return 1;
		}

		tok = strtok( NULL, " " );
	}

	return 0;
}

const char *get_val( const char *haystack, int index ) {
	static char fnd[1024];
	char tmp[1024];
	char *tok;

	strcpy( tmp, haystack );

	tok = strtok( tmp, " " );

	while ( tok && index ) {
		index -= 1;
		tok = strtok( NULL, " " );
	}

	if ( !tok ) {
		fnd[0] = 0;
	} else {
		strcpy( fnd, tok );
	}

	return fnd;
}

const defblock_t *find_key( const char *key, int type, defblock_t *blocks, int numBlocks ) {
	char tmp[1024];
	int i;
	char *tok;

	tok = NULL;

	for ( i = 0; i < numBlocks; i++ ) {
		if ( blocks[i].type != type ) {
			continue;
		}

		strcpy( tmp, blocks[i].key );

		tok = strtok( tmp, " " );

		while ( tok ) {
			if ( !strcmp( tok, key ) ) {
				return &blocks[i];
			}

			tok = strtok( NULL, " " );
		}
	}

	return NULL;
}

int arenatype_from_string( const char *str ) {
	const char *types[AT_MAX_ARENA_TYPE] = {
		"rocketarena",
		"clanarena",
		"redrover",
		"practice",
	};
	int i;

	for ( i = 0; i < AT_MAX_ARENA_TYPE; i++ ) {
		if ( !strcmp( str, types[i] ) ) {
			return i;
		}
	}

	return def_pickup_type;
}

void get_settings( defblock_t *blocks, int numBlocks ) {
	const defblock_t *block;
	int weapons_mask, i, weapon;
	const char *game_type_string;

	if ( ( block = find_key( "weapons", DT_STRING, blocks, numBlocks ) ) ) {
		weapons_mask = 0;

		for ( i = 0; i < (int)( sizeof( weapon_vals ) / sizeof( weapon_vals[0] ) ); i++ ) {
			if ( i == 8 ) {
				weapon = 0;
			} else {
				weapon = i + WP_MACHINEGUN;
			}

			if ( has_val( block->value.str, va( "%d", weapon ) ) ) {
				weapons_mask |= weapon_vals[i];
			}
		}

		weapons = weapons_mask;
	}
	if ( ( block = find_key( "gametype", DT_STRING, blocks, numBlocks ) ) ) {
		game_type_string = get_val( block->value.str, 0 );
		game_type = arenatype_from_string( game_type_string );
	}
	if ( ( block = find_key( "armor", DT_STRING, blocks, numBlocks ) ) ) {
		armor = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "health", DT_STRING, blocks, numBlocks ) ) ) {
		health = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "minping", DT_STRING, blocks, numBlocks ) ) ) {
		minping = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "maxping", DT_STRING, blocks, numBlocks ) ) ) {
		maxping = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "playersperteam", DT_STRING, blocks, numBlocks ) ) ) {
		playersperteam = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "rounds", DT_STRING, blocks, numBlocks ) ) ) {
		rounds = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "rocketspeed", DT_STRING, blocks, numBlocks ) ) ) {
		rocket_speed = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "shells", DT_STRING, blocks, numBlocks ) ) ) {
		shells = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "bullets", DT_STRING, blocks, numBlocks ) ) ) {
		bullets = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "slugs", DT_STRING, blocks, numBlocks ) ) ) {
		slugs = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "grenades", DT_STRING, blocks, numBlocks ) ) ) {
		grenades = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "rockets", DT_STRING, blocks, numBlocks ) ) ) {
		rockets = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "cells", DT_STRING, blocks, numBlocks ) ) ) {
		cells = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "plasma", DT_STRING, blocks, numBlocks ) ) ) {
		plasma = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "bfgammo", DT_STRING, blocks, numBlocks ) ) ) {
		bfgammo = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "fastswitch", DT_STRING, blocks, numBlocks ) ) ) {
		fastswitch = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "armorprotect", DT_STRING, blocks, numBlocks ) ) ) {
		armorprotect = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "healthprotect", DT_STRING, blocks, numBlocks ) ) ) {
		healthprotect = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "fallingdamage", DT_STRING, blocks, numBlocks ) ) ) {
		fallingdamage = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "allowvotinggametype", DT_STRING, blocks, numBlocks ) ) ) {
		allow_voting_gametype = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "allowvotinghealtharmor", DT_STRING, blocks, numBlocks ) ) ) {
		allow_voting_healtharmor = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "allowvotingping", DT_STRING, blocks, numBlocks ) ) ) {
		allow_voting_ping = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "allowvotingplayersperteam", DT_STRING, blocks, numBlocks ) ) ) {
		allow_voting_playersperteam = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "allowvotingrounds", DT_STRING, blocks, numBlocks ) ) ) {
		allow_voting_rounds = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "allowvotingmaxteams", DT_STRING, blocks, numBlocks ) ) ) {
		allow_voting_maxteams = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "allowvotinghealtharmorprotect", DT_STRING, blocks, numBlocks ) ) ) {
		allow_voting_healtharmorprotect = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "allowvotingweapons", DT_STRING, blocks, numBlocks ) ) ) {
		allow_voting_weapons = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "allowvotingfallingdamage", DT_STRING, blocks, numBlocks ) ) ) {
		allow_voting_fallingdamage = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "allowvotinggrapple", DT_STRING, blocks, numBlocks ) ) ) {
		allow_voting_grapple = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "allowvotingexcessive", DT_STRING, blocks, numBlocks ) ) ) {
		allow_voting_excessive = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "allowvotingcompetitionmode", DT_STRING, blocks, numBlocks ) ) ) {
		allow_voting_competitionmode = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "allowvotinglockarena", DT_STRING, blocks, numBlocks ) ) ) {
		allow_voting_lockarena = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "lockarena", DT_STRING, blocks, numBlocks ) ) ) {
		lock_arena = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "lockcount", DT_STRING, blocks, numBlocks ) ) ) {
		lock_count = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "competitionmode", DT_STRING, blocks, numBlocks ) ) ) {
		competition_mode = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "unbalanced", DT_STRING, blocks, numBlocks ) ) ) {
		unbalanced = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "excessive", DT_STRING, blocks, numBlocks ) ) ) {
		excessive = atoi( get_val( block->value.str, 0 ) );
	}
	if ( ( block = find_key( "maxplayers", DT_STRING, blocks, numBlocks ) ) ) {
		maxplayers = atoi( get_val( block->value.str, 0 ) );
	}
}

void set_config( int first_arena, int last_arena ) {
	int i;
	const defblock_t *arenablock;

	for ( i = first_arena; i <= last_arena; i++ ) {
		weapons = 0x7f;
		armor = 100;
		health = 100;
		playersperteam = 1;

		if ( idmap == 0 ) {
			rounds = 1;
		} else {
			rounds = 9;
		}

		if ( idmap == 0 ) {
			game_type = AT_ROCKETARENA;
		} else {
			game_type = def_pickup_type;
		}

		shells = 100;
		bullets = 200;
		slugs = 50;
		grenades = 20;
		rockets = 50;
		cells = 150;
		plasma = 100;
		bfgammo = 20;
		fastswitch = 0;
		armorprotect = 2;
		healthprotect = 1;
		fallingdamage = 1;
		allow_voting_healtharmor = 1;
		allow_voting_gametype = 1;
		allow_voting_ping = 1;
		allow_voting_playersperteam = 1;
		allow_voting_rounds = 1;
		allow_voting_healtharmorprotect = 1;
		allow_voting_weapons = 1;
		allow_voting_fallingdamage = 1;
		allow_voting_grapple = 0;
		allow_voting_excessive = 0;
		allow_voting_competitionmode = 1;
		allow_voting_lockarena = 1;
		lock_arena = 0;
		lock_count = 6;
		competition_mode = 0;
		unbalanced = 0;
		excessive = 0;
		maxplayers = 0;

		get_settings( definition_blocks, num_definition_blocks );

		if ( map_block ) {
			get_settings( map_block->value.block, map_block->numValues );
		}

		if ( map_block ) {
			arenablock = arena_blocks[i];

			if ( arenablock ) {
				get_settings( arenablock->value.block, arenablock->numValues );
			}
		}

		level.arenas[i].settings.weapons = weapons;
		level.arenas[i].settings.armor = armor;
		level.arenas[i].settings.health = health;
		level.arenas[i].settings.playersperteam = playersperteam;
		level.arenas[i].settings.rounds = rounds;
		level.arenas[i].settings.type = game_type;
		level.arenas[i].settings.shells = shells;
		level.arenas[i].settings.bullets = bullets;
		level.arenas[i].settings.slugs = slugs;
		level.arenas[i].settings.grenades = grenades;
		level.arenas[i].settings.rockets = rockets;
		level.arenas[i].settings.cells = cells;
		level.arenas[i].settings.plasma = plasma;
		level.arenas[i].settings.bfgammo = bfgammo;
		level.arenas[i].settings.fastswitch = fastswitch;
		level.arenas[i].settings.armorprotect = armorprotect;
		level.arenas[i].settings.healthprotect = healthprotect;
		level.arenas[i].settings.fallingdamage = fallingdamage;
		level.arenas[i].settings.allow_voting_gametype = allow_voting_gametype;
		level.arenas[i].settings.allow_voting_healtharmor = allow_voting_healtharmor;
		level.arenas[i].settings.allow_voting_playersperteam = allow_voting_playersperteam;
		level.arenas[i].settings.allow_voting_rounds = allow_voting_rounds;
		level.arenas[i].settings.allow_voting_healtharmorprotect = allow_voting_healtharmorprotect;
		level.arenas[i].settings.allow_voting_weapons = allow_voting_weapons;
		level.arenas[i].settings.allow_voting_fallingdamage = allow_voting_fallingdamage;
		level.arenas[i].settings.allow_voting_grapple = allow_voting_grapple;
		level.arenas[i].settings.allow_voting_excessive = allow_voting_excessive;
		level.arenas[i].settings.allow_voting_competitionmode = allow_voting_competitionmode;
		level.arenas[i].settings.allow_voting_lockarena = allow_voting_lockarena;
		level.arenas[i].settings.lockarena = lock_arena;
		level.arenas[i].settings.lockcount = lock_count;
		level.arenas[i].settings.competitionmode = competition_mode;
		level.arenas[i].settings.unbalanced = unbalanced;
		level.arenas[i].settings.excessive = excessive;
		level.arenas[i].settings.maxplayers = maxplayers;
		level.arenas[i].settings.dirty = 0;

		level.arenas[i].defaults = level.arenas[i].settings;
	}
}

int ra_isalnum( char c ) {
	int i = (int)c;

	if ( ( i >= '0' && i <= '9' ) ||
	     ( i >= 'A' && i <= 'Z' ) ||
	     ( i >= 'a' && i <= 'z' ) ||
	     ( i == '_' ) ) {
		return 1;
	}

	return 0;
}

const char *next_token( const char *str ) {
	static const char *token = NULL;
	static char foo[1024];
	char *ptr;

	if ( str ) {
		token = str;
	} else if ( !str && !token ) {
		return NULL;
	}

	if ( token[0] == '\0'
#ifndef Q3_VM
		|| token[0] == '\n'
#endif
		) {
		return NULL;
	}

	ptr = foo;

#ifdef Q3_VM
	while (1) {
		int chomped = 0;

		/* chomp comments */
		if ( token[0] == '/' && token[1] == '/' ) {
			while ( token[0] != '\n' ) {
				if ( !token[0] ) {
					return NULL;
				}

				token++;
			}

			chomped = 1;
		}

		/* chomp newlines */
		if ( token[0] == '\r' || token[0] == '\n' ) {
			while ( token[0] == '\r' || token[0] == '\n' ) {
				if ( !token[0] ) {
					return NULL;
				}

				token++;
			}

			chomped = 1;
		}

		if (!chomped) {
			break;
		}
	}
#endif

	if ( !ra_isalnum( token[0] ) ) {
		*( ptr++ ) = *( token++ );

		if ( token[0] == '/' && ptr[-1] == '/' ) {
			*( ptr++ ) = *( token++ );
		}

		*( ptr++ ) = 0;

		return foo;
	}

	while ( ra_isalnum( token[0] ) ) {
		*( ptr++ ) = *( token++ );
	}

	*( ptr++ ) = 0;

	return foo;
}

defblock_t *new_def_block() {
	return G_Alloc( sizeof( defblock_t ) * MAX_DEF_BLOCKS );
}

void add_val( char *buf, const char *val ) {
	strcat( buf, " " );
	strcat( buf, val );
}

defblock_t *new_def_item( defblock_t *block, char *key ) {
	defblock_t *self = block;
	self->numKeys = 0;
	self->numValues = 0;
	self->key = key;
	self->key[0] = 0;
	self->type = DT_NONE;
	return self;
}

#ifdef Q3_VM

int read_block( const char *cfg, defblock_t *block ) {
	int depth;
	char tmpstr[512];
	char tmpkey[512];
	defblock_t tmpblocks[MAX_DEF_DEPTH][128];
	char line[1024];

	defblock_t *item;
	const char *tok;
	int state;
	int count;
	int n;

	depth = 0;
	count = 0;
	item = new_def_item( block++, tmpkey );
	state = 0;

	tok = next_token( cfg );

	while ( tok ) {
		if ( state == 0 ) {
			if ( tok[0] == '{' ) {
				/* copy key from temporary storage */
				item->key = G_Alloc( strlen( item->key ) + 1 );
				strcpy( item->key, tmpkey );

				/* init temporary storage for nested values */
				item->type = DT_BLOCK;
				item->value.block = tmpblocks[depth];

				/* push state to stack */
				stack[depth].count = count;
				stack[depth].block = block;
				stack[depth].item = item;

				/* init state for nested value */
				count = 0;
				block = item->value.block;
				item = new_def_item( block++, tmpkey );
				depth += 1;

				if ( depth >= MAX_DEF_DEPTH ) {
					G_LogPrintf( "Error reading config file: too many levels {}\n" );
					return 0;
				}
			} else if ( tok[0] == ':' ) {
				/* copy key from temporary storage */
				item->key = G_Alloc( strlen( item->key ) + 1 );
				strcpy( item->key, tmpkey );

				/* init temporary storage for value */
				item->type = DT_STRING;
				item->value.str = tmpstr;
				tmpstr[0] = 0;

				state = 1;
			} else if ( tok[0] == '}' ) {
				if ( depth == 0 ) {
					G_LogPrintf( "Error reading config file: unbalanced {}\n" );
					return 0;
				}

				depth--;

				/* copy nested values from temporary storage */
				item = stack[depth].item;
				item->numValues = count;
				item->value.block = G_Alloc( count * sizeof( defblock_t ) );
				memcpy( item->value.block, tmpblocks[depth], count * sizeof( defblock_t ) );

				/* setup next item */
				block = stack[depth].block;
				count = stack[depth].count;
				item = new_def_item( block++, tmpkey );
				state = 0;
				count++;
			} else {
				/* append token to key */
				add_val( item->key, tok );
				item->numKeys++;
			}
		} else if ( state == 1 ) {
			if ( tok[0] == ';' ) {
				/* copy value from temporary storage */
				item->value.str = G_Alloc( strlen( tmpstr ) + 1 );
				strcpy( item->value.str, tmpstr );

				/* setup next item */
				item = new_def_item( block++, tmpkey );
				state = 0;
				count++;
			} else {
				/* append token to value */
				add_val( item->value.str, tok );
				item->numValues++;
			}
		}

		tok = next_token( NULL );
	}

	if ( depth != 0 ) {
		G_LogPrintf( "Error reading config file: unbalanced {}\n" );
		return 0;
	}

	return 1;
}


int read_config( const char *filename ) {
	char cfg[8192];
	fileHandle_t f;
	int len;
	int n;

	if ( !definition_blocks ) {
		definition_blocks = new_def_block();
		num_definition_blocks = 0;
	}

	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( len >= sizeof( cfg ) ) {
		trap_Printf( va( S_COLOR_RED "%s too large, is %i, max %i", filename, len, sizeof( cfg ) ) );
		trap_FS_FCloseFile( f );
		return 0;
	}

	trap_FS_Read( cfg, len, f );
	cfg[len] = 0;
	trap_FS_FCloseFile( f );

	n = read_block( cfg, &definition_blocks[num_definition_blocks] );

	num_definition_blocks += n;

	return 1;
}

#else

int read_block( FILE *fp, defblock_t *block ) {
	int depth;
	char tmpstr[512];
	char tmpkey[512];
	defblock_t tmpblocks[MAX_DEF_DEPTH][128];
	char line[1024];

	defblock_t *item;
	const char *tok;
	int state;
	int count;
	int n;

	depth = 0;
	count = 0;
	item = new_def_item( block++, tmpkey );
	state = 0;

	while ( 1 ) {
		n = fscanf( fp, "%s", line );

		if ( n <= 0 ) {
			if ( depth != 0 ) {
				G_LogPrintf( "Error reading config file: unbalanced {}\n" );
				return 0;
			}

			return count;
		}

		tok = next_token( line );

		while ( tok ) {
			/* chomp comments */
			if ( tok[0] == '/' && tok[1] == '/' ) {
				while ( 1 ) {
					n = fgetc( fp );

					if ( n < 1 ) {
						return count;
					}

					if ( n == (int)'\n' ) {
						break;
					}
				}

				break;
			}

			if ( state == 0 ) {
				if ( tok[0] == '{' ) {
					/* copy key from temporary storage */
					item->key = G_Alloc( strlen( item->key ) + 1 );
					strcpy( item->key, tmpkey );

					/* init temporary storage for nested values */
					item->type = DT_BLOCK;
					item->value.block = tmpblocks[depth];

					/* push state to stack */
					stack[depth].count = count;
					stack[depth].block = block;
					stack[depth].item = item;

					/* init state for nested value */
					count = 0;
					block = item->value.block;
					item = new_def_item( block++, tmpkey );
					depth += 1;

					if ( depth >= MAX_DEF_DEPTH ) {
						G_LogPrintf( "Error reading config file: too many levels {}\n" );
						return 0;
					}
				} else if ( tok[0] == ':' ) {
					/* copy key from temporary storage */
					item->key = G_Alloc( strlen( item->key ) + 1 );
					strcpy( item->key, tmpkey );

					/* init temporary storage for value */
					item->type = DT_STRING;
					item->value.str = tmpstr;
					tmpstr[0] = 0;

					state = 1;
				} else if ( tok[0] == '}' ) {
					if ( depth == 0 ) {
						G_LogPrintf( "Error reading config file: unbalanced {}\n" );
						return 0;
					}

					depth--;

					/* copy nested values from temporary storage */
					item = stack[depth].item;
					item->numValues = count;
					item->value.block = G_Alloc( count * sizeof( defblock_t ) );
					memcpy( item->value.block, tmpblocks[depth], count * sizeof( defblock_t ) );

					/* setup next item */
					block = stack[depth].block;
					count = stack[depth].count;
					item = new_def_item( block++, tmpkey );
					state = 0;
					count++;
				} else {
					/* append token to key */
					add_val( item->key, tok );
					item->numKeys++;
				}
			} else if ( state == 1 ) {
				if ( tok[0] == ';' ) {
					/* copy value from temporary storage */
					item->value.str = G_Alloc( strlen( tmpstr ) + 1 );
					strcpy( item->value.str, tmpstr );

					/* setup next item */
					item = new_def_item( block++, tmpkey );
					state = 0;
					count++;
				} else {
					/* append token to value */
					add_val( item->value.str, tok );
					item->numValues++;
				}
			}

			tok = next_token( NULL );
		}
	}
}

void read_config( FILE *fp ) {
	int n;

	if ( !definition_blocks ) {
		definition_blocks = new_def_block();
		num_definition_blocks = 0;
	}

	n = read_block( fp, &definition_blocks[num_definition_blocks] );

	num_definition_blocks += n;
}

#endif

const char *current_mapname() {
	static char mapname[128];
	char info[1024];

	trap_GetServerinfo( info, sizeof( info ) );

	strncpy( mapname, Info_ValueForKey( info, "mapname" ), sizeof( mapname ) - 1 );
	mapname[sizeof( mapname ) - 1] = '\0';

	return mapname;
}

void load_config( int numArenas ) {
#ifdef Q3_VM
	char cfgname[MAX_QPATH];
#else
	FILE *fp;
	char cfgpath[4096];
	char cfgname[4096];
#endif
	const char *mapname;
	const defblock_t *mapblock;
	const defblock_t *arenablock;
	const defblock_t *block;
	int i;
	fileHandle_t f;
	char tmpbuf[8192];
	int len;

#ifdef Q3_VM
	trap_Cvar_VariableStringBuffer( "arenacfg", cfgname, sizeof( cfgname ) );

	if ( !cfgname[0] ) {
		strcpy( cfgname, "arena.cfg" );
	}

	if ( !read_config( cfgname ) ) {
		G_LogPrintf( "Error: Couldn\'t read %s\n", cfgname );
		return;
	}
#else
	trap_Cvar_VariableStringBuffer( "fs_game", cfgpath, sizeof( cfgpath ) );

	if ( !cfgpath[0] ) {
		strcpy( cfgpath, "." );
	}

	trap_Cvar_VariableStringBuffer( "arenacfg", cfgname, sizeof( cfgname ) );

	if ( !cfgname[0] ) {
		strcpy( cfgname, "arena.cfg" );
	}

	strcat( cfgpath, "/" );
	strcat( cfgpath, cfgname );

	if ( !( fp = fopen( cfgpath, "r" ) ) ) {
		G_LogPrintf( "Error: Couldn\'t read %s\n", cfgpath );
		return;
	}

	read_config( fp );
	fclose( fp );
#endif

	mapname = current_mapname();

	if ( ( block = find_key( "votetries", DT_STRING, definition_blocks, num_definition_blocks ) ) ) {
		votetries_setting = atoi( get_val( block->value.str, 0 ) );
	}

	if ( ( map_loop = find_key( "maploop", DT_STRING, definition_blocks, num_definition_blocks ) ) ) {
		G_LogPrintf( "Map loop read\n" );
	} else {
		G_LogPrintf( "No map loop found\n" );
	}

	if ( ( block = find_key( "defpickup", DT_STRING, definition_blocks, num_definition_blocks ) ) ) {
		G_LogPrintf( "Default pickup mode is: %s\n", get_val( block->value.str, 0 ) );

		def_pickup_type = arenatype_from_string( get_val( block->value.str, 0 ) );
	}

	arena_blocks = G_Alloc( sizeof( defblock_t * ) * numArenas );

	if ( !( mapblock = find_key( mapname, DT_BLOCK, definition_blocks, num_definition_blocks ) ) ) {
#ifdef Q3_VM
		if ( read_config( va( "%s.cfg", mapname ) ) ) {
			mapblock = find_key( mapname, DT_BLOCK, definition_blocks, num_definition_blocks );
		}
#else
		len = trap_FS_FOpenFile( va( "%s.cfg", mapname ), &f, FS_READ );

		if ( len > 0 ) {
			trap_FS_Read( tmpbuf, len, f );

			if ( ( fp = fopen( "arena.tmp", "w" ) ) ) {
				fwrite( tmpbuf, len, 1, fp );
				fclose( fp );
			}

			if ( ( fp = fopen( cfgpath, "a" ) ) ) {
				tmpbuf[len] = '\0';
				fprintf( fp, "\r\n%s\r\n", tmpbuf );
				fclose( fp );
			}

			fp = fopen( "arena.tmp", "r" );
			read_config( fp );
			fclose( fp );

			mapblock = find_key( mapname, DT_BLOCK, definition_blocks, num_definition_blocks );

			trap_FS_FCloseFile( f );
		}
#endif
	}

	if ( mapblock ) {
		G_LogPrintf( "arena.cfg info for map found: %s\n", mapname );

		map_block = mapblock;

		for ( i = 0; i < numArenas; i++ ) {
			arenablock = find_key( va( "%d", i ), DT_BLOCK, mapblock->value.block, mapblock->numValues );
			arena_blocks[i] = arenablock;
		}
	} else {
		G_LogPrintf( "arena.cfg info for map not found: %s\n", mapname );

		map_block = NULL;
	}
}

const char *get_next_map( const char *map ) {
	int i;
	const char *next;

	if ( !map_loop ) {
		return current_mapname();
	}

	if ( !has_val( map_loop->value.str, map ) ) {
		return get_val( map_loop->value.str, 0 );
	}

	for ( i = 0; i < map_loop->numValues; i++ ) {
		if ( !strcmp( map, get_val( map_loop->value.str, i ) ) ) {
			next = get_val( map_loop->value.str, ++i );

			if ( !*next ) {
				return get_val( map_loop->value.str, 0 );
			}

			return next;
		}
	}

	return current_mapname();
}

void print_map_loop() {
}

void load_motd() {
}
