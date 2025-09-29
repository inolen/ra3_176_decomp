#ifndef ARENA_COMMON_H
#define ARENA_COMMON_H

typedef enum {
	AT_ROCKETARENA,
	AT_CLANARENA,
	AT_REDROVER,
	AT_PRACTICE,
	AT_MAX_ARENA_TYPE,
} arenaType_t;

typedef enum {
	PM_NONE,
	PM_SELF_AND_TEAM,
	PM_TEAM,
	PM_MAX_PROTECT_MODE,
} protectMode_t;

enum {
	TS_WARMUP,
	TS_MATCH,
	TS_ROUND,
	TS_NUM_PERIODS,
};

enum {
	TS_KILLS,
	TS_DEATHS,
	TS_MGSHOT,
	TS_MGHIT,
	TS_SGSHOT,
	TS_SGHIT,
	TS_GLHIT,
	TS_GLSHOT,
	TS_GLSPLASH,
	TS_RLSHOT,
	TS_RLHIT,
	TS_RLSPLASH,
	TS_RGSHOT,
	TS_RGHIT,
	TS_LGSHOT,
	TS_LGHIT,
	TS_PGSHOT,
	TS_PGHIT,
	TS_AWARDS1,
	TS_AWARDS2,
	TS_DMGTAKEN,
	TS_DMGGIVEN,
	TS_NUM_STATS,
};

extern const char *arena_strings[AT_MAX_ARENA_TYPE + 1];
extern char *protect[PM_MAX_PROTECT_MODE + 1];

const char *arenatype_to_string( int type );
char *encode_command( char *cmd, int key );
int encode_int( int value, int key );
int decode_int( int value, int key );

#endif
