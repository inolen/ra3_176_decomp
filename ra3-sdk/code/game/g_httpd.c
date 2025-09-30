#include "g_local.h"

#ifndef Q3_VM

#include "../httpd/httpd.h"

// FIXME stubs
extern int threadQuit;

extern pthread_mutex_t threadQuitMutex;

int G_GetCvarMutex(vmCvar_t *cvar);
int G_ReleaseCvarMutex(vmCvar_t *cvar);

int G_GetMutex(pthread_mutex_t *m);
int G_ReleaseMutex(pthread_mutex_t *m);
//

sqlite *httpdDB = NULL;

int httpAuthFlags( httpd *server, char *realm ) {
	char	*err;
	int		rows;
	int		cols;
	char	**res;
	int		flags;

	err = NULL;
	rows = -1;
	cols = -1;
	flags = 0;

	if ( !httpdAuthenticate( server, realm ) ) {
		httpdOutput( server, "Authentication failure." );
		return -1;
	}

	if ( sqlite_get_table_printf( level.db, "SELECT Flags FROM Players WHERE ID=%q AND Password='%q'", &res, &rows, &cols, &err,
	                              server->request.authUser, server->request.authPassword ) ) {
		httpdForceAuthenticate( server, realm );
		httpdOutput( server, "Authentication failure." );
		return -1;
	}

	if ( rows < 1 ) {
		httpdForceAuthenticate( server, realm );
		httpdOutput( server, "Authentication failure." );
		return -1;
	}

	if ( res[cols] ) {
		flags = atoi( res[cols] );
	} else {
		flags = 0;
	}

	sqlite_free_table( res );

	return flags;
}

int httpChkAdmin( httpd *server ) {
	int flags;

	flags = httpAuthFlags( server, "RA3 Admin" );

	if ( flags < 0 ) {
		return flags;
	}

	if ( ( flags & 2 ) == 0 || ( flags & 1 ) == 0 ) {
		httpdForceAuthenticate( server, "RA3 Admin" );
		httpdOutput( server, "Authentication failure." );
		return -1;
	}

	return 0;
}

void httpServer() {
	int		done;
	httpd	*server;
	char	homepath[512];
	char	game[128];

	done = 0;
	server = NULL;

	G_GetCvarMutex( &fs_homepath );
	strncpy( homepath, fs_homepath.string, sizeof( homepath ) );
	G_ReleaseCvarMutex( &fs_homepath );

	G_GetCvarMutex( &fs_game );
	strncpy( game, fs_game.string, sizeof( game ) );
	G_ReleaseCvarMutex( &fs_game );

	G_GetCvarMutex( &net_port );
	server = httpdCreate( NULL, net_port.integer );
	G_ReleaseCvarMutex( &net_port );

	if ( !server ) {
		G_Printf( "Error starting HTTPD.\n" );
		pthread_exit( NULL );
	}

	if ( !G_OpenDB( &httpdDB ) ) {
		G_Printf( "Error opening db from HTTPD thread.\n" );
		httpdDestroy( server );
		pthread_exit( NULL );
	}

	httpdSetFileBase( server, homepath );
	httpdAddWildcardContent( server, "/", NULL, "html/" );
	httpdAddWildcardContent( server, "/admin/", httpChkAdmin, "html/admin/" );
	httpdAddWildcardContent( server, "/logs/pb/svss/", NULL, "pb/svss/" );

	while ( !done ) {
		G_GetMutex( &threadQuitMutex );
		done = threadQuit;
		G_ReleaseMutex( &threadQuitMutex );

		if ( done ) {
			sqlite_close( httpdDB );
			httpdDestroy( server );
			pthread_exit( NULL );
		}

		usleep( 1000 );

		if ( httpdGetConnection( server, 0 ) <= 0 ) {
			// nop
		} else {
			if ( httpdReadRequest( server ) < 0 ) {
				httpdEndRequest( server );
			} else {
				httpdProcessRequest( server );
				httpdEndRequest( server );
			}
		}
	}

	sqlite_close( httpdDB );
	httpdDestroy( server );
}

#endif
