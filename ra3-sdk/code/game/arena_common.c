#include "q_shared.h"
#include "arena_common.h"

const char *arena_strings[AT_MAX_ARENA_TYPE + 1] = {
	"Rocket Arena",
	"Clan Arena",
	"Red Rover",
	"Practice Arena",
	NULL,
 };

const char *arenatype_to_string( int type ) {
	if (type >= 0 && type < AT_MAX_ARENA_TYPE) {
		return arena_strings[type];
	}

	return "?";
}

char *protect[PM_MAX_PROTECT_MODE + 1] = {
	"Damage everyone",
	"Don't damage self or team",
	"Damage self, not team",
	NULL,
 };

char *encode_command( char *cmd, int key ) {
	static char cmdhold[128];
	char *p = cmdhold;
	int sum = 0;

	while ( *cmd ) {
		*p = ( abs( ( cmd[0] + 0x1623 + sum + (int)( p - cmdhold ) ) ^ key ) % 26 ) + 'A';
		p += 1;

		*p = ( abs( ( cmd[0] + 0x424 + sum - (int)( p - cmdhold ) ) ^ key ) % 26 ) + 'A';
		p += 1;

		sum += *( cmd++ );
	}

	*p = 0;

	return cmdhold;
}

int encode_int( int value, int key ) {
	int tmp = value;
	tmp += 0x1623;
	tmp *= key;
	tmp -= 0x424;
	return tmp;
}

int decode_int( int value, int key ) {
	int tmp = value;
	tmp += 0x424;
	tmp /= key;
	tmp -= 0x1623;
	return tmp;
}
