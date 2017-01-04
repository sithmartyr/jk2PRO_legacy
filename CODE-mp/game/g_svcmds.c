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
static qboolean StringToFilter (char *s, ipFilter_t *f)
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
		if (b[i] != 0)
			m[i] = 255;

		if (!*s)
			break;
		s++;
	}
	
	f->mask = *(unsigned *)m;
	f->compare = *(unsigned *)b;
	
	return qtrue;
}

/*
=================
UpdateIPBans
=================
*/
static void UpdateIPBans (void)
{
	byte	b[4];
	int		i;
	char	iplist[MAX_INFO_STRING];

	*iplist = 0;
	for (i = 0 ; i < numIPFilters ; i++)
	{
		if (ipFilters[i].compare == 0xffffffff)
			continue;

		*(unsigned *)b = ipFilters[i].compare;
		Com_sprintf( iplist + strlen(iplist), sizeof(iplist) - strlen(iplist), 
			"%i.%i.%i.%i ", b[0], b[1], b[2], b[3]);
	}

	trap_Cvar_Set( "g_banIPs", iplist );
}

/*
=================
G_FilterPacket
=================
*/
qboolean G_FilterPacket (char *from)
{
	int		i;
	unsigned	in;
	byte m[4] = {'\0','\0','\0','\0'};
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
			return g_filterBan.integer != 0;

	return g_filterBan.integer == 0;
}

/*
=================
AddIP
=================
*/
static void AddIP( char *str )
{
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
	
	if (!StringToFilter (str, &ipFilters[i]))
		ipFilters[i].compare = 0xffffffffu;

	UpdateIPBans();
}

/*
=================
G_ProcessIPBans
=================
*/
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
			AddIP( t );
		t = s;
	}
}


/*
=================
Svcmd_AddIP_f
=================
*/
void Svcmd_AddIP_f (void)
{
	char		str[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		G_Printf("Usage:  addip <ip-mask>\n");
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	AddIP( str );

}

/*
=================
Svcmd_RemoveIP_f
=================
*/
void Svcmd_RemoveIP_f (void)
{
	ipFilter_t	f;
	int			i;
	char		str[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		G_Printf("Usage:  sv removeip <ip-mask>\n");
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	if (!StringToFilter (str, &f))
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

	G_Printf ( "Didn't find %s.\n", str );
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

	// find the player
	trap_Argv( 1, str, sizeof( str ) );
	cl = ClientForString( str );
	if ( !cl ) {
		return;
	}

	// set the team
	trap_Argv( 2, str, sizeof( str ) );
	SetTeam( &g_entities[cl - level.clients], str );
}

char	*ConcatArgs( int start );

void Svcmd_Say_f(void) {
	char *p = NULL;
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT] = { 0 };

	if (trap_Argc() < 2)
		return;

	p = ConcatArgs(1);

	if (strlen(p) >= MAX_SAY_TEXT) {
		p[MAX_SAY_TEXT - 1] = '\0';
		//G_SecurityLogPrintf("Cmd_Say_f from -1 (server) has been truncated: %s\n", p);
	}

	Q_strncpyz(text, p, sizeof(text));
	Q_strstrip(text, "\n\r", "  ");

	G_LogPrintf("say: server: %s\n", text);
	trap_SendServerCommand(-1, va("print \"server: %s\n\"", text));
}

typedef struct bitInfo_S {
	const char	*string;
} bitInfo_T;

static bitInfo_T adminOptions[] = {
	{ "Amtele" },//0
	//{ "Amfreeze" },//1
	{ "Amtelemark" },//2
	//{ "Amban" },//3
	//{ "Amkick" },//4
	{ "Noclip" },//5
	//{ "Grantadmin" },//6
	//{ "Ammap" },//7
	//{ "Ampsay" },//8
	//{ "Amforceteam" },//9
	//{ "Amlockteam" },//10
	//{ "Amvstr" },//11
	//{ "See IPs" },//12
	//{ "Amrename" },//13
	{ "Amlistmaps" },//14
	//{ "Rebuild highscores (?)" },//15
	//{ "Amwhois" },//16
	//{ "Amlookup" },//17
	//{ "Use hide" },//18
	//{ "See hiders" },//19
	{ "Callvote" },//20
	{ "Killvote" }//21
};
static const int MAX_ADMIN_OPTIONS = ARRAY_LEN(adminOptions);

void Svcmd_ToggleAdmin_f(void) {
	if (trap_Argc() == 1) {
		Com_Printf("Usage: toggleAdmin <admin level (full or junior) admin option>\n");
		return;
	}
	else if (trap_Argc() == 2) {
		int i = 0, level;
		char arg1[8] = { 0 };

		trap_Argv(1, arg1, sizeof(arg1));
		if (!Q_stricmp(arg1, "j") || !Q_stricmp(arg1, "junior"))
			level = 0;
		else if (!Q_stricmp(arg1, "f") || !Q_stricmp(arg1, "full"))
			level = 1;
		else {
			Com_Printf("Usage: toggleAdmin <admin level (full or junior) admin option>\n");
			return;
		}

		for (i = 0; i < MAX_ADMIN_OPTIONS; i++) {
			if (level == 0) {
				if ((g_juniorAdminLevel.integer & (1 << i))) {
					Com_Printf("%2d [X] %s\n", i, adminOptions[i].string);
				}
				else {
					Com_Printf("%2d [ ] %s\n", i, adminOptions[i].string);
				}
			}
			else if (level == 1) {
				if ((g_fullAdminLevel.integer & (1 << i))) {
					Com_Printf("%2d [X] %s\n", i, adminOptions[i].string);
				}
				else {
					Com_Printf("%2d [ ] %s\n", i, adminOptions[i].string);
				}
			}
		}
		return;
	}
	else {
		char arg1[8] = { 0 }, arg2[8] = { 0 };
		int index, level;
		const unsigned long int mask = (1 << MAX_ADMIN_OPTIONS) - 1;

		trap_Argv(1, arg1, sizeof(arg1));
		if (!Q_stricmp(arg1, "j") || !Q_stricmp(arg1, "junior"))
			level = 0;
		else if (!Q_stricmp(arg1, "f") || !Q_stricmp(arg1, "full"))
			level = 1;
		else {
			Com_Printf("Usage: toggleAdmin <admin level (full or junior) admin option>\n");
			return;
		}
		trap_Argv(2, arg2, sizeof(arg2));
		index = atoi(arg2);

		if (index < 0 || index >= MAX_ADMIN_OPTIONS) {
			Com_Printf("toggleAdmin: Invalid range: %i [0, %i]\n", index, MAX_ADMIN_OPTIONS - 1);
			return;
		}

		if (level == 0) {
			trap_Cvar_Set("g_juniorAdminLevel", va("%i", (1 << index) ^ (g_juniorAdminLevel.integer & mask)));
			trap_Cvar_Update(&g_juniorAdminLevel);

			Com_Printf("%s %s^7\n", adminOptions[index].string, ((g_juniorAdminLevel.integer & (1 << index))
				? "^2Enabled" : "^1Disabled"));
		}
		else if (level == 1) {
			trap_Cvar_Set("g_fullAdminLevel", va("%i", (1 << index) ^ (g_fullAdminLevel.integer & mask)));
			trap_Cvar_Update(&g_fullAdminLevel);

			Com_Printf("%s %s^7\n", adminOptions[index].string, ((g_fullAdminLevel.integer & (1 << index))
				? "^2Enabled" : "^1Disabled"));
		}
	}
}

static bitInfo_T voteOptions[] = {
	{ "Capturelimit" },//1
	{ "Clientkick" },//2
	{ "Forcespec" },//3
	{ "Fraglimit" },//4
	{ "g_doWarmup" },//5
	{ "g_gametype" },//6
	{ "kick" },//7
	{ "map" },//8
	{ "map_restart" },//9
	{ "nextmap" },//10
	{ "sv_maxteamsize" },//11
	{ "timelimit" },//12
	{ "vstr" },//13
	{ "poll" },//14
	{ "pause" },//15
	{ "score_restart" }//16
};
static const int MAX_VOTE_OPTIONS = ARRAY_LEN(voteOptions);

void Svcmd_ToggleVote_f(void) {
	if (trap_Argc() == 1) {
		int i = 0;
		for (i = 0; i < MAX_VOTE_OPTIONS; i++) {
			if ((g_allowVote.integer & (1 << i))) {
				//trap_Print("%2d [X] %s\n", i, voteOptions[i].string);
				Com_Printf("%2d [X] %s\n", i, voteOptions[i].string);
			}
			else {
				Com_Printf("%2d [ ] %s\n", i, voteOptions[i].string);
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const unsigned long int mask = (1 << MAX_VOTE_OPTIONS) - 1;

		trap_Argv(1, arg, sizeof(arg));
		index = atoi(arg);

		//DM Start: New -1 toggle all options.
		if (index < -1 || index >= MAX_VOTE_OPTIONS) {  //Whereas we need to allow -1 now, we must change the limit for this value.
			Com_Printf("toggleVote: Invalid range: %i [0-%i, or -1 for toggle all]\n", index, MAX_VOTE_OPTIONS - 1);
			return;
		}

		if (index == -1) {
			for (index = 0; index < MAX_VOTE_OPTIONS; index++) {  //Read every tweak option and set it to the opposite of what it is currently set to.
				trap_Cvar_Set("g_allowVote", va("%i", (1 << index) ^ (g_allowVote.integer & mask)));
				trap_Cvar_Update(&g_allowVote);
				Com_Printf("%s %s^7\n", voteOptions[index].string, ((g_allowVote.integer & (1 << index)) ? "^2Enabled" : "^1Disabled"));
			}
		} //DM End: New -1 toggle all options.

		trap_Cvar_Set("g_allowVote", va("%i", (1 << index) ^ (g_allowVote.integer & mask)));
		trap_Cvar_Update(&g_allowVote);

		Com_Printf("%s %s^7\n", voteOptions[index].string, ((g_allowVote.integer & (1 << index))
			? "^2Enabled" : "^1Disabled"));
	}
}

static bitInfo_T voteTweaks[] = {
	{ "Allow spec callvote in siege gametype" },//1
	{ "Allow spec callvote in CTF/TFFA gametypes" },//2
	{ "Clear vote when going to spectate" },//3
	{ "Dont allow callvote for 30s after mapload" },//4
	{ "Floodprotect callvotes by IP" },//5
	{ "Dont allow map callvotes for 10 minutes at start of each map" },//6
	{ "Add vote delay for map callvotes only" },//7
	{ "Allow voting from spectate" },//8
	{ "Show votes in console" },//9
	{ "Only count voters in pass/fail calculation" },//10
	{ "Fix mapchange after gametype vote" }//11
};
static const int MAX_VOTE_TWEAKS = ARRAY_LEN(voteTweaks);

void Svcmd_ToggleTweakVote_f(void) {
	if (trap_Argc() == 1) {
		int i = 0;
		for (i = 0; i < MAX_VOTE_TWEAKS; i++) {
			if ((g_tweakVote.integer & (1 << i))) {
				Com_Printf("%2d [X] %s\n", i, voteTweaks[i].string);
			}
			else {
				Com_Printf("%2d [ ] %s\n", i, voteTweaks[i].string);
			}
		}
		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const unsigned long int mask = (1 << MAX_VOTE_TWEAKS) - 1;

		trap_Argv(1, arg, sizeof(arg));
		index = atoi(arg);

		//DM Start: New -1 toggle all options.
		if (index < -1 || index >= MAX_VOTE_TWEAKS) {  //Whereas we need to allow -1 now, we must change the limit for this value.
			Com_Printf("tweakVote: Invalid range: %i [0-%i, or -1 for toggle all]\n", index, MAX_VOTE_TWEAKS - 1);
			return;
		}

		if (index == -1) {
			for (index = 0; index < MAX_VOTE_TWEAKS; index++) {  //Read every tweak option and set it to the opposite of what it is currently set to.
				trap_Cvar_Set("g_tweakVote", va("%i", (1 << index) ^ (g_tweakVote.integer & mask)));
				trap_Cvar_Update(&g_tweakVote);
				Com_Printf("%s %s^7\n", voteTweaks[index].string, ((g_tweakVote.integer & (1 << index)) ? "^2Enabled" : "^1Disabled"));
			}
		} //DM End: New -1 toggle all options.

		trap_Cvar_Set("g_tweakVote", va("%i", (1 << index) ^ (g_tweakVote.integer & mask)));
		trap_Cvar_Update(&g_tweakVote);

		Com_Printf("%s %s^7\n", voteTweaks[index].string, ((g_tweakVote.integer & (1 << index))
			? "^2Enabled" : "^1Disabled"));
	}
}

/*
=================
ConsoleCommand

=================
*/
qboolean	ConsoleCommand( void ) {
	char	cmd[MAX_TOKEN_CHARS];

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if (Q_stricmp(cmd, "toggleAdmin") == 0) {
		Svcmd_ToggleAdmin_f();
		return qtrue;
	}

	if (Q_stricmp(cmd, "togglevote") == 0) {
		Svcmd_ToggleVote_f();
		return qtrue;
	}

	if (Q_stricmp(cmd, "tweakvote") == 0) {
		Svcmd_ToggleTweakVote_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "entitylist") == 0 ) {
		Svcmd_EntityList_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "forceteam") == 0 ) {
		Svcmd_ForceTeam_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "game_memory") == 0) {
		Svcmd_GameMem_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "addbot") == 0) {
		Svcmd_AddBot_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "botlist") == 0) {
		Svcmd_BotList_f();
		return qtrue;
	}

/*	if (Q_stricmp (cmd, "abort_podium") == 0) {
		Svcmd_AbortPodium_f();
		return qtrue;
	}
*/
	if (Q_stricmp (cmd, "addip") == 0) {
		Svcmd_AddIP_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "removeip") == 0) {
		Svcmd_RemoveIP_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "listip") == 0) {
		trap_SendConsoleCommand( EXEC_NOW, "g_banIPs\n" );
		return qtrue;
	}

	if (g_dedicated.integer) {
		if (Q_stricmp (cmd, "say") == 0) {
			Svcmd_Say_f();
			return qtrue;
		}
		// everything else will also be printed as a say command
		//trap_SendServerCommand( -1, va("print \"server: %s\"", ConcatArgs(0) ) );
		return qfalse;
	}
	return qfalse;
}

