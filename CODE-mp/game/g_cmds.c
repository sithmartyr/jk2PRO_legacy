// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

#include "../../ui/menudef.h"			// for the voice chats

//rww - for getting bot commands...
int AcceptBotCommand(char *cmd, gentity_t *pl);
//end rww

void BG_CycleInven(playerState_t *ps, int direction);
void BG_CycleForce(playerState_t *ps, int direction);

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	char		entry[1024];
	char		string[1400];
	int			stringlength;
	int			i, j;
	gclient_t	*cl;
	int			numSorted, scoreFlags, accuracy, perfect;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;

	numSorted = level.numConnectedClients;
	
	if (numSorted > MAX_CLIENT_SCORE_SEND)
	{
		numSorted = MAX_CLIENT_SCORE_SEND;
	}

	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		if( cl->accuracy_shots ) {
			accuracy = cl->accuracy_hits * 100 / cl->accuracy_shots;
		}
		else {
			accuracy = 0;
		}
		perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? 1 : 0;

		Com_sprintf (entry, sizeof(entry),
			" %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.sortedClients[i],
			cl->ps.persistant[PERS_SCORE], ping, (level.time - cl->pers.enterTime)/60000,
			scoreFlags, g_entities[level.sortedClients[i]].s.powerups, accuracy, 
			cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
			cl->ps.persistant[PERS_EXCELLENT_COUNT],
			cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], 
			cl->ps.persistant[PERS_DEFEND_COUNT], 
			cl->ps.persistant[PERS_ASSIST_COUNT], 
			perfect,
			cl->ps.persistant[PERS_CAPTURES]);
		j = strlen(entry);
		if (stringlength + j > 1022)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	//still want to know the total # of clients
	i = level.numConnectedClients;

	trap_SendServerCommand( ent-g_entities, va("scores %i %i %i%s", i, 
		level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE],
		string ) );
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
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOCHEATS")));
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MUSTBEALIVE")));
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
StringIsInteger
==================
*/
qboolean StringIsInteger(const char *s) {
	int			i = 0, len = 0;
	qboolean	foundDigit = qfalse;

	for (i = 0, len = strlen(s); i<len; i++)
	{
		//if (!isdigit(s[i]))
		if (!(s[i] >= '0' && s[i] <= '9'))
			return qfalse;

		foundDigit = qtrue;
	}

	return foundDigit;
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

//[videoP - jk2PRO - Serverside - All - Redid and moved sanitizestring2 for partial name recognition - Start]
/*
==================
SanitizeString2

Rich's revised version of SanitizeString
==================
*/
void SanitizeString2(const char *in, char *out)
{
	int i = 0;
	int r = 0;

	while (in[i])
	{
		if (i >= MAX_NAME_LENGTH - 1)
		{ //the ui truncates the name here..
			break;
		}

		if (in[i] == '^')
		{
			if (in[i + 1] >= 48 && //'0'
				in[i + 1] <= 57) //'9'
			{ //only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else
			{ //just skip the ^
				i++;
				continue;
			}
		}

		if (in[i] < 32)
		{
			i++;
			continue;
		}

		out[r] = tolower(in[i]);//lowercase please
		r++;
		i++;
	}
	out[r] = 0;
}
//[videoP - jk2PRO - Serverside - All - Redid and moved sanitizestring2 for partial name recognition - End]

//[videoP - jk2PRO - Serverside - All - Redid and clientnumberfromstring for partial name recognition - Start]
/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
static int JP_ClientNumberFromString(gentity_t *to, const char *s)
{
	gclient_t	*cl;
	int			idnum, i, match = -1;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	idnum = atoi(s);


	//redo
	/*
	if (!Q_stricmp(s, "0")) {
	cl = &level.clients[idnum];
	if ( cl->pers.connected != CON_CONNECTED ) {
	trap->SendServerCommand( to-g_entities, va("print \"Client '%i' is not active\n\"", idnum));
	return -1;
	}
	return 0;
	}
	if (idnum && idnum < 32) {
	cl = &level.clients[idnum];
	if ( cl->pers.connected != CON_CONNECTED ) {
	trap->SendServerCommand( to-g_entities, va("print \"Client '%i' is not active\n\"", idnum));
	return -1;
	}
	return idnum;
	}
	*/
	//end redo

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9' && strlen(s) == 1) //changed this to only recognize numbers 0-31 as client numbers, otherwise interpret as a name, in which case sanitize2 it and accept partial matches (return error if multiple matches)
	{
		idnum = atoi(s);
		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED) {
			trap_SendServerCommand(to - g_entities, va("print \"Client '%i' is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	if ((s[0] == '1' || s[0] == '2') && (s[1] >= '0' && s[1] <= '9' && strlen(s) == 2))  //changed and to or ..
	{
		idnum = atoi(s);
		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED) {
			trap_SendServerCommand(to - g_entities, va("print \"Client '%i' is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	if (s[0] == '3' && (s[1] >= '0' && s[1] <= '1' && strlen(s) == 2))
	{
		idnum = atoi(s);
		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED) {
			trap_SendServerCommand(to - g_entities, va("print \"Client '%i' is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}




	// check for a name match
	SanitizeString2(s, s2);
	for (idnum = 0, cl = level.clients; idnum < level.maxclients; idnum++, cl++){
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		SanitizeString2(cl->pers.netname, n2);

		for (i = 0; i < level.numConnectedClients; i++)
		{
			cl = &level.clients[level.sortedClients[i]];
			SanitizeString2(cl->pers.netname, n2);
			if (strstr(n2, s2))
			{
				if (match != -1)
				{ //found more than one match
					trap_SendServerCommand(to - g_entities, va("print \"More than one user '%s' on the server\n\"", s));
					return -2;
				}
				match = level.sortedClients[i];
			}
		}
		if (match != -1)//uhh
			return match;
	}
	if (!atoi(s)) //Uhh.. well.. whatever. fixes amtele spam problem when teleporting to x y z yaw
		trap_SendServerCommand(to - g_entities, va("print \"User '%s' is not on the server\n\"", s));
	return -1;
}
//[videoP - jk2PRO - Serverside - All - Redid and clientnumberfromstring for partial name recognition - End]

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
	char		name[MAX_TOKEN_CHARS];
	gitem_t		*it;
	int			i;
	qboolean	give_all;
	gentity_t		*it_ent;
	trace_t		trace;
	char		arg[MAX_TOKEN_CHARS];

	if ( !CheatsOk( ent ) ) {
		return;
	}

	trap_Argv( 1, name, sizeof( name ) );

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all)
	{
		i = 0;
		while (i < HI_NUM_HOLDABLE)
		{
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << i);
			i++;
		}
		i = 0;
	}

	if (give_all || Q_stricmp( name, "health") == 0)
	{
		if (trap_Argc() == 3) {
			trap_Argv( 2, arg, sizeof( arg ) );
			ent->health = atoi(arg);
			if (ent->health > ent->client->ps.stats[STAT_MAX_HEALTH]) {
				ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
			}
		}
		else {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << (WP_DET_PACK+1))  - ( 1 << WP_NONE );
		if (!give_all)
			return;
	}
	
	if ( !give_all && Q_stricmp(name, "weaponnum") == 0 )
	{
		trap_Argv( 2, arg, sizeof( arg ) );
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << atoi(arg));
		return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		int num = 999;
		if (trap_Argc() == 3) {
			trap_Argv( 2, arg, sizeof( arg ) );
			num = atoi(arg);
		}
		for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
			ent->client->ps.ammo[i] = num;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		if (trap_Argc() == 3) {
			trap_Argv( 2, arg, sizeof( arg ) );
			ent->client->ps.stats[STAT_ARMOR] = atoi(arg);
		} else {
			ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];
		}

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
	if (Q_stricmp(name, "defend") == 0) {
		ent->client->ps.persistant[PERS_DEFEND_COUNT]++;
		return;
	}
	if (Q_stricmp(name, "assist") == 0) {
		ent->client->ps.persistant[PERS_ASSIST_COUNT]++;
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

void DeletePlayerProjectiles(gentity_t *ent) {
	int i;
	for (i = MAX_CLIENTS; i<MAX_GENTITIES; i++) { //can be optimized more?
		if (g_entities[i].inuse && g_entities[i].s.eType == ET_MISSILE && (g_entities[i].r.ownerNum == ent->s.number)) { //Delete (rocket) if its ours
			G_FreeEntity(&g_entities[i]);
			//trap->Print("This only sometimes prints.. even if we have a missile in the air.  (its num: %i, our num: %i, weap type: %i) \n", hit->r.ownerNum, ent->s.number, hit->s.weapon);
		}
	}
}

void ResetPlayerTimers(gentity_t *ent, qboolean print)
{
	qboolean wasReset = qfalse;;

	if (!ent->client)
		return;
	if (ent->client->pers.stats.startTime || ent->client->pers.stats.startTimeFlag)
		wasReset = qtrue;

	ent->client->pers.stats.startLevelTime = 0;
	ent->client->pers.stats.startTime = 0;
	ent->client->pers.stats.topSpeed = 0;
	ent->client->pers.stats.displacement = 0;
	ent->client->pers.stats.displacementSamples = 0;
	ent->client->pers.stats.startTimeFlag = 0;
	ent->client->pers.stats.topSpeedFlag = 0;
	ent->client->pers.stats.displacementFlag = 0;
	ent->client->pers.stats.displacementFlagSamples = 0;
	ent->client->ps.stats[STAT_JUMPTIME] = 0;
	ent->client->ps.fd.forcePower = 100; //Reset their force back to full i guess!

	ent->client->pers.stats.lastResetTime = level.time; //well im just not sure

	if (ent->client->sess.raceMode) {
		VectorClear(ent->client->ps.velocity); //lel
		ent->client->ps.duelTime = 0;
		ent->client->ps.stats[STAT_ONLYBHOP] = 0; //meh
		//if (ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] == 3) { //this is a sad hack..
		ent->client->ps.powerups[PW_YSALAMIRI] = 0; //beh, only in racemode so wont fuck with ppl using amtele as checkpoints midcourse
		//addlater? ent->client->pers.haste = qfalse;
		//}
		if (ent->client->sess.movementStyle == 7 || ent->client->sess.movementStyle == 8) { //Get rid of their rockets when they tele/noclip..?
			DeletePlayerProjectiles(ent);
		}
	}

	if (wasReset && print)
		//trap->SendServerCommand( ent-g_entities, "print \"Timer reset!\n\""); //console spam is bad
		trap_SendServerCommand(ent - g_entities, "cp \"Timer reset!\n\n\n\n\n\n\n\n\n\n\n\n\"");
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_RaceNoclip_f(gentity_t *ent) {
	trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", ent->client->noclip ? "noclip OFF" : "noclip ON"));
	ent->client->noclip = !ent->client->noclip;
	//DM - addlater ResetPlayerTimers(ent, qtrue);
}

void Cmd_Noclip_f( gentity_t *ent ) {
	/*char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));*/

	if (ent->client && ent->client->ps.duelInProgress && ent->client->pers.lastUserName[0]) {
		gentity_t *duelAgainst = &g_entities[ent->client->ps.duelIndex];
		if (duelAgainst->client && duelAgainst->client->pers.lastUserName[0]) {
			trap_SendServerCommand(ent - g_entities, va("print \"You are not authorized to use this command (noclip) in ranked duels.\n\""));
			return; //Dont allow noclip in ranked duels ever
		}
	}

	//if (!CheatsOk(ent) && ent->client)
	//{
		if (ent->client->sess.fullAdmin)//Logged in as full admin
		{
			if (!(g_fullAdminLevel.integer & (1 << A_NOCLIP)))
			{
				if (ent->client->noclip) {
					ent->client->noclip = qfalse;
					trap_SendServerCommand(ent - g_entities, "print \"noclip OFF\n\"");
					AmTeleportPlayer(ent, ent->client->ps.origin, ent->client->ps.viewangles, qtrue, qtrue);
					//DM - addlater ResetPlayerTimers(ent, qtrue);
				}
				else if (g_allowRaceTele.integer > 1 && ent->client->sess.raceMode) {
					Cmd_RaceNoclip_f(ent);
					return;
				}
				else
					trap_SendServerCommand(ent - g_entities, va("print \"You are not authorized to use this command (noclip).\n\""));
				return;
			}
		}
		else if (ent->client->sess.juniorAdmin)//Logged in as junior admin
		{
			if (!(g_juniorAdminLevel.integer & (1 << A_NOCLIP)))
			{
				if (ent->client->noclip) {
					ent->client->noclip = qfalse;
					trap_SendServerCommand(ent - g_entities, "print \"noclip OFF\n\"");
					AmTeleportPlayer(ent, ent->client->ps.origin, ent->client->ps.viewangles, qtrue, qtrue);
					//DM - addlater ResetPlayerTimers(ent, qtrue);
				}
				else if (g_allowRaceTele.integer > 1 && ent->client->sess.raceMode) {
					Cmd_RaceNoclip_f(ent);
					return;
				}
				else
					trap_SendServerCommand(ent - g_entities, va("print \"You are not authorized to use this command (noclip).\n\""));
				return;
			}
		}
		else//Not logged in
		{
			if (ent->client->noclip) {
				ent->client->noclip = qfalse;
				trap_SendServerCommand(ent - g_entities, "print \"noclip OFF\n\"");
				AmTeleportPlayer(ent, ent->client->ps.origin, ent->client->ps.viewangles, qtrue, qtrue); //Good
				//DM - addlater ResetPlayerTimers(ent, qtrue);
			}
			else if (g_allowRaceTele.integer > 1 && ent->client->sess.raceMode) {
				Cmd_RaceNoclip_f(ent);
				return;
			}
			else
				trap_SendServerCommand(ent - g_entities, "print \"Cheats are not enabled. You must be logged in to use this command (noclip).\n\"");//replaces "Cheats are not enabled on this server." msg
			return;
		}

		if (trap_Argc() == 2) {
			char client[MAX_NETNAME];
			int clientid;
			gentity_t *target = NULL;

			trap_Argv(1, client, sizeof(client));
			clientid = JP_ClientNumberFromString(ent, client);
			if (clientid == -1 || clientid == -2)
				return;
			target = &g_entities[clientid];
			if (!target->client)
				return;
			trap_SendServerCommand(target - g_entities, va("print \"%s\n\"", target->client->noclip ? "noclip OFF" : "noclip ON"));
			if (target->client->sess.raceMode && target->client->noclip)
				AmTeleportPlayer(target, target->client->ps.origin, target->client->ps.viewangles, qtrue, qtrue); //Good
			target->client->noclip = !target->client->noclip;
			//DM - addlater ResetPlayerTimers(target, qtrue);
			return;
		}
		if (trap_Argc() == 1) {
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", ent->client->noclip ? "noclip OFF" : "noclip ON"));
			if (ent->client->sess.raceMode && ent->client->noclip)
				AmTeleportPlayer(ent, ent->client->ps.origin, ent->client->ps.viewangles, qtrue, qtrue); //Good
			ent->client->noclip = !ent->client->noclip;
			//DM - addlater ResetPlayerTimers(ent, qtrue);
			return;
		}
	/*}
	else { //not needed..
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", ent->client->noclip ? "noclip OFF" : "noclip ON"));
		ent->client->noclip = !ent->client->noclip;
	}*/
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
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_TeamTask_f( gentity_t *ent ) {
	char userinfo[MAX_INFO_STRING];
	char		arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if ( trap_Argc() != 2 ) {
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	task = atoi( arg );

	trap_GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap_SetUserinfo(client, userinfo);
	ClientUserinfoChanged(client);
}

void G_Kill(gentity_t *ent) {

	//OSP: pause
	if (level.pause.state != PAUSE_NONE && ent->client && !ent->client->sess.raceMode) {
		return;
	}

	//DM - addlater - duel suicides? 
	/*if ((level.gametype == GT_TOURNAMENT) &&
		level.numPlayingClients > 1 && !level.warmupTime)
	{
		if (!g_allowDuelSuicide.integer)
		{
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "ATTEMPTDUELKILL")));
			return;
		}
	}
	else if (level.gametype >= GT_TEAM && level.numPlayingClients > 1 && !level.warmupTime)
	{
		if (!g_allowTeamSuicide.integer)
		{
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "ATTEMPTDUELKILL")));
			return;
		}
	}*/

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die(ent, ent, ent, 100000, MOD_SUICIDE);
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
	if (ent->health <= 0) {
		return;
	}

	if (g_gametype.integer == GT_TOURNAMENT && level.numPlayingClients > 1 && !level.warmupTime)
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "ATTEMPTDUELKILL")) );
		return;
	}

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die (ent, ent, ent, 100000, MOD_SUICIDE);
}

gentity_t *G_GetDuelWinner(gclient_t *client)
{
	gclient_t *wCl;
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		wCl = &level.clients[i];
		
		if (wCl && wCl != client && /*wCl->ps.clientNum != client->ps.clientNum &&*/
			wCl->pers.connected == CON_CONNECTED && wCl->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return &g_entities[wCl->ps.clientNum];
		}
	}

	return NULL;
}

/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	client->ps.fd.forceDoInit = 1; //every time we change teams make sure our force powers are set right

	if ( client->sess.sessionTeam == TEAM_RED ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEREDTEAM")) );
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
		client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBLUETEAM")));
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
		client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHESPECTATORS")));
	} else if ( client->sess.sessionTeam == TEAM_FREE ) {
		if (g_gametype.integer == GT_TOURNAMENT)
		{
			/*
			gentity_t *currentWinner = G_GetDuelWinner(client);

			if (currentWinner && currentWinner->client)
			{
				trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s %s\n\"",
				currentWinner->client->pers.netname, G_GetStripEdString("SVINGAME", "VERSUS"), client->pers.netname));
			}
			else
			{
				trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBATTLE")));
			}
			*/
			//NOTE: Just doing a vs. once it counts two players up
		}
		else
		{
			trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBATTLE")));
		}
	}

	G_LogPrintf ( "setteam:  %i %s %s\n",
				  client - &level.clients[0],
				  TeamName ( oldTeam ),
				  TeamName ( client->sess.sessionTeam ) );
}

/*
=================
SetTeam
=================
*/
void SetTeam( gentity_t *ent, char *s) {
	int					team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;
	int					teamLeader;
	
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
			//For now, don't do this. The legalize function will set powers properly now.
			/*
			if (g_forceBasedTeams.integer)
			{
				if (ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					team = TEAM_BLUE;
				}
				else
				{
					team = TEAM_RED;
				}
			}
			else
			{
			*/
				team = PickTeam( clientNum );
			//}
		}

		if ( g_teamForceBalance.integer  ) {
			int		counts[TEAM_NUM_TEAMS];

			counts[TEAM_BLUE] = TeamCount( ent->client->ps.clientNum, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( ent->client->ps.clientNum, TEAM_RED );

			// We allow a spread of two
			if ( team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1 ) {
				//For now, don't do this. The legalize function will set powers properly now.
				/*
				if (g_forceBasedTeams.integer && ent->client->ps.fd.forceSide == FORCE_DARKSIDE)
				{
					trap_SendServerCommand( ent->client->ps.clientNum, 
						va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TOOMANYRED_SWITCH")) );
				}
				else
				*/
				{
					trap_SendServerCommand( ent->client->ps.clientNum, 
						va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TOOMANYRED")) );
				}
				return; // ignore the request
			}
			if ( team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1 ) {
				//For now, don't do this. The legalize function will set powers properly now.
				/*
				if (g_forceBasedTeams.integer && ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					trap_SendServerCommand( ent->client->ps.clientNum, 
						va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TOOMANYBLUE_SWITCH")) );
				}
				else
				*/
				{
					trap_SendServerCommand( ent->client->ps.clientNum, 
						va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TOOMANYBLUE")) );
				}
				return; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}

		//For now, don't do this. The legalize function will set powers properly now.
		/*
		if (g_forceBasedTeams.integer)
		{
			if (team == TEAM_BLUE && ent->client->ps.fd.forceSide != FORCE_LIGHTSIDE)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MUSTBELIGHT")) );
				return;
			}
			if (team == TEAM_RED && ent->client->ps.fd.forceSide != FORCE_DARKSIDE)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MUSTBEDARK")) );
				return;
			}
		}
		*/

	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

	// override decision if limiting the players
	if ( (g_gametype.integer == GT_TOURNAMENT)
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
		//client->sess.spectatorTime = level.time;
		if ((g_gametype.integer != GT_TOURNAMENT) || (oldTeam != TEAM_SPECTATOR))	{//so you don't get dropped to the bottom of the queue for changing skins, etc.
			client->sess.spectatorTime = level.time;	
		}
	}

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	client->sess.teamLeader = qfalse;
	if ( team == TEAM_RED || team == TEAM_BLUE ) {
		teamLeader = TeamLeader( team );
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if ( teamLeader == -1 || ( !(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT) ) ) {
			SetLeader( team, clientNum );
		}
	}
	// make sure there is a team leader on the team the player came from
	if ( oldTeam == TEAM_RED || oldTeam == TEAM_BLUE ) {
		CheckTeamLeader( oldTeam );
	}

	BroadcastTeamChange( client, oldTeam );

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );

	ClientBegin( clientNum, qfalse );
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t *ent ) {
	ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;	
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;	
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
	ent->client->ps.weapon = WP_NONE;
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		oldTeam = ent->client->sess.sessionTeam;
		switch ( oldTeam ) {
		case TEAM_BLUE:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PRINTBLUETEAM")) );
			break;
		case TEAM_RED:
			trap_SendServerCommand( ent-g_entities, va("print \"Red team\n\"", G_GetStripEdString("SVINGAME", "PRINTREDTEAM")) );
			break;
		case TEAM_FREE:
			trap_SendServerCommand( ent-g_entities, va("print \"Free team\n\"", G_GetStripEdString("SVINGAME", "PRINTFREETEAM")) );
			break;
		case TEAM_SPECTATOR:
			trap_SendServerCommand( ent-g_entities, va("print \"Spectator team\n\"", G_GetStripEdString("SVINGAME", "PRINTSPECTEAM")) );
			break;
		}
		return;
	}

	if ( ent->client->switchTeamTime > level.time ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSWITCH")) );
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		//&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		//ent->client->sess.losses++;
		&& ent->client->sess.sessionTeam == TEAM_FREE) {//in a tournament game
		//disallow changing teams
		trap_SendServerCommand(ent - g_entities, "print \"Cannot switch teams in Duel\n\"");
		return;
		//FIXME: why should this be a loss???
		//ent->client->sess.losses++;
	}

	trap_Argv( 1, s, sizeof( s ) );

	SetTeam( ent, s );

	ent->client->switchTeamTime = level.time + 5000;
}

/*
=================
Cmd_ForceChanged_f
=================
*/
void Cmd_ForceChanged_f( gentity_t *ent )
{
	char fpChStr[1024];
	const char *buf;
//	Cmd_Kill_f(ent);
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{ //if it's a spec, just make the changes now
		//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "FORCEAPPLIED")) );
		//No longer print it, as the UI calls this a lot.
		WP_InitForcePowers( ent );
		goto argCheck;
	}

	buf = G_GetStripEdString("SVINGAME", "FORCEPOWERCHANGED");

	strcpy(fpChStr, buf);

	trap_SendServerCommand( ent-g_entities, va("print \"%s%s\n\n\"", S_COLOR_GREEN, fpChStr) );

	ent->client->ps.fd.forceDoInit = 1;
argCheck:
	if (g_gametype.integer == GT_TOURNAMENT)
	{ //If this is duel, don't even bother changing team in relation to this.
		return;
	}

	if (trap_Argc() > 1)
	{
		char	arg[MAX_TOKEN_CHARS];

		trap_Argv( 1, arg, sizeof( arg ) );

		if (arg && arg[0])
		{ //if there's an arg, assume it's a combo team command from the UI.
			Cmd_Team_f(ent);
		}
	}
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	int		i;
	char	arg[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	// can't follow another spectator
	if ( level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}

	// first set them to spectator
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator" );
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int		clientnum;
	int		original;

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		//&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		&& ent->client->sess.sessionTeam == TEAM_FREE) {\
		//WTF???
		ent->client->sess.losses++;
	}
	// first set them to spectator
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		SetTeam( ent, "spectator" );
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;
	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		}
		if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}


/*
==================
G_Say
==================
*/

static void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message ) {
	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( other->client->pers.connected != CON_CONNECTED ) {
		return;
	}
	if ( mode == SAY_TEAM  && !OnSameTeam(ent, other) ) {
		return;
	}
	// no chatting to players in tournements
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& other->client->sess.sessionTeam == TEAM_FREE
		&& ent->client->sess.sessionTeam != TEAM_FREE ) {
		return;
	}

	trap_SendServerCommand( other-g_entities, va("%s \"%s%c%c%s\"", 
		mode == SAY_TEAM ? "tchat" : "chat",
		name, Q_COLOR_ESCAPE, color, message));
}

#define EC		"\x19"

void G_Say(gentity_t *ent, gentity_t *target, int mode, const char *chatText) {
	int			j;
	gentity_t	*other;
	int			color;
	char		name[64];
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	char		location[64];

	//jk2pro remove limitation for teamsay for other gametypes
	/*if (g_gametype.integer < GT_TEAM && mode == SAY_TEAM) {
		mode = SAY_ALL;
	}*/

	switch (mode) {
	default:
	case SAY_ALL:
		//G_LogPrintf("say: %s: %s\n", ent->client->pers.netname, chatText);
		Com_sprintf(name, sizeof(name), "%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		//G_LogPrintf("sayteam: %s: %s\n", ent->client->pers.netname, chatText);
		if (Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf(name, sizeof(name), EC"(%s%c%c"EC") (%s)"EC": ",
			ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		else
			Com_sprintf(name, sizeof(name), EC"(%s%c%c"EC")"EC": ",
			ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && g_gametype.integer >= GT_TEAM &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf(name, sizeof(name), EC"[%s%c%c"EC"] (%s)"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		else
			Com_sprintf(name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_MAGENTA;
		break;
	}

	Q_strncpyz(text, chatText, sizeof(text));

	if (target) {
		G_SayTo(ent, target, mode, color, name, text);
		return;
	}

	// echo the text to the console
	if (g_dedicated.integer) {
		G_Printf("%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo(ent, other, mode, color, name, text);
	}
}

/*
==================
Cmd_Say_f
==================
*/
//static void Cmd_Say_f( gentity_t *ent, int mode, qboolean arg0 ) {
static void Cmd_Say_f(gentity_t *ent) {
	char		*p = NULL;

	if (trap_Argc() < 2)//&& !arg0 )
		return;

	/*if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}*/

	p = ConcatArgs(1);

	if (strlen(p) >= MAX_SAY_TEXT) {
		p[MAX_SAY_TEXT - 1] = '\0';
		G_SecurityLogPrintf("Cmd_Say_f from %d (%s^7) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p);
	}

	G_Say( ent, NULL, SAY_ALL, p);
}

static void Cmd_Amsay_f(gentity_t *ent) {
	char *p = NULL;

	if (trap_Argc() < 2)
		return;

	p = ConcatArgs(1);

	if (strlen(p) > MAX_SAY_TEXT)
	{
		p[MAX_SAY_TEXT - 1] = '\0';
		G_SecurityLogPrintf("Cmd_Say_f from %d (%s^7) has been truncated: %s\n", ent->s.clientNum, ent->client->pers.netname, p);
	}

	G_Say(ent, NULL, SAY_ADMIN, p);
}

/*
==================
Cmd_SayTeam_f
==================
*/
static void Cmd_SayTeam_f(gentity_t *ent) {
	char *p = NULL;

	if (trap_Argc() < 2)
		return;

	p = ConcatArgs(1);

	if (strlen(p) >= MAX_SAY_TEXT) {
		p[MAX_SAY_TEXT - 1] = '\0';
		//G_SecurityLogPrintf("Cmd_SayTeam_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p);
	}

	//G_Say( ent, NULL, (level.gametype>=GT_TEAM) ? SAY_TEAM : SAY_ALL, p );
	G_Say(ent, NULL, SAY_TEAM, p);
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f(gentity_t *ent) {
	int			targetNum;
	gentity_t	*target;
	char		*p;
	char		arg[MAX_TOKEN_CHARS];

	if (trap_Argc() < 3) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: tell <player id> <message>\n\"");
		return;
	}

	trap_Argv(1, arg, sizeof(arg));
	targetNum = JP_ClientNumberFromString(ent, arg);
	if (targetNum == -1 || targetNum == -2) {
		return;
	}

	target = &g_entities[targetNum];
	if (!target->inuse || !target->client) {
		return;
	}

	p = ConcatArgs(2);

	if (strlen(p) >= MAX_SAY_TEXT) {
		p[MAX_SAY_TEXT - 1] = '\0';
		G_SecurityLogPrintf("Cmd_Tell_f from %d (%s^7) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p);
	}

	G_LogPrintf("tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p);
	G_Say(ent, target, SAY_TELL, p);
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if (ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say(ent, ent, SAY_TELL, p);
	}
}

static void G_VoiceTo( gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly ) {
	int color;
	char *cmd;

	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( mode == SAY_TEAM && !OnSameTeam(ent, other) ) {
		return;
	}
	// no chatting to players in tournements
	if ( (g_gametype.integer == GT_TOURNAMENT )) {
		return;
	}

	if (mode == SAY_TEAM) {
		color = COLOR_CYAN;
		cmd = "vtchat";
	}
	else if (mode == SAY_TELL) {
		color = COLOR_MAGENTA;
		cmd = "vtell";
	}
	else {
		color = COLOR_GREEN;
		cmd = "vchat";
	}

	trap_SendServerCommand( other-g_entities, va("%s %d %d %d %s", cmd, voiceonly, ent->s.number, color, id));
}

void G_Voice( gentity_t *ent, gentity_t *target, int mode, const char *id, qboolean voiceonly ) {
	int			j;
	gentity_t	*other;

	if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	if ( target ) {
		G_VoiceTo( ent, target, mode, id, voiceonly );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "voice: %s %s\n", ent->client->pers.netname, id);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_VoiceTo( ent, other, mode, id, voiceonly );
	}
}

/*
==================
Cmd_Voice_f
==================
*/
static void Cmd_Voice_f( gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly ) {
	char		*p;

	if ( trap_Argc () < 2 && !arg0 ) {
		return;
	}

	if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}

	G_Voice( ent, NULL, mode, p, voiceonly );
}

/*
==================
Cmd_VoiceTell_f
==================
*/
static void Cmd_VoiceTell_f( gentity_t *ent, qboolean voiceonly ) {
	int			targetNum;
	gentity_t	*target;
	char		*id;
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

	id = ConcatArgs( 2 );

	G_LogPrintf( "vtell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, id );
	G_Voice( ent, target, SAY_TELL, id, voiceonly );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Voice( ent, ent, SAY_TELL, id, voiceonly );
	}
}


/*
==================
Cmd_VoiceTaunt_f
==================
*/
static void Cmd_VoiceTaunt_f( gentity_t *ent ) {
	gentity_t *who;
	int i;

	if (!ent->client) {
		return;
	}

	// insult someone who just killed you
	if (ent->enemy && ent->enemy->client && ent->enemy->client->lastkilled_client == ent->s.number) {
		// i am a dead corpse
		if (!(ent->enemy->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent->enemy, SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		if (!(ent->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent,        SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		ent->enemy = NULL;
		return;
	}
	// insult someone you just killed
	if (ent->client->lastkilled_client >= 0 && ent->client->lastkilled_client != ent->s.number) {
		who = g_entities + ent->client->lastkilled_client;
		if (who->client) {
			// who is the person I just killed
			if (who->client->lasthurt_mod == MOD_STUN_BATON) {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );	// and I killed them with a gauntlet
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );
				}
			} else {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );	// and I killed them with something else
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );
				}
			}
			ent->client->lastkilled_client = -1;
			return;
		}
	}

	if (g_gametype.integer>= GT_TEAM) {
		// praise a team mate who just got a reward
		for(i = 0; i < MAX_CLIENTS; i++) {
			who = g_entities + i;
			if (who->client && who != ent && who->client->sess.sessionTeam == ent->client->sess.sessionTeam) {
				if (who->client->rewardTime > level.time) {
					if (!(who->r.svFlags & SVF_BOT)) {
						G_Voice( ent, who, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					if (!(ent->r.svFlags & SVF_BOT)) {
						G_Voice( ent, ent, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					return;
				}
			}
		}
	}

	// just say something
	G_Voice( ent, NULL, SAY_ALL, VOICECHAT_TAUNT, qfalse );
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

static const char *gameNames[] = {
	"Free For All",
	"Holocron FFA",
	"Jedi Master",
	"Duel",
	"Single Player",
	"Team FFA",
	"N/A",
	"Capture the Flag",
	"Capture the Ysalamiri"
};

/*
==================
Cmd_CallVote_f
==================
*/

qboolean G_VoteCapturelimit(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int n = Com_Clampi(0, 0x7FFFFFFF, atoi(arg2));
	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %i", arg1, n);
	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s", level.voteString);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteClientkick(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int n = atoi(arg2);

	if (n < 0 || n >= level.maxclients) {
		trap_SendServerCommand(ent - g_entities, va("print \"invalid client number %d.\n\"", n));
		return qfalse;
	}

	if (g_entities[n].client->pers.connected == CON_DISCONNECTED) {
		trap_SendServerCommand(ent - g_entities, va("print \"there is no client with the client number %d.\n\"", n));
		return qfalse;
	}

	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %s", arg1, arg2);
	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s %s", arg1, g_entities[n].client->pers.netname);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteFraglimit(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int n = Com_Clampi(0, 0x7FFFFFFF, atoi(arg2));
	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %i", arg1, n);
	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s", level.voteString);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteGametype(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int gt = atoi(arg2);

	// ffa, ctf, tdm, etc
	//if (arg2[0] && isalpha(arg2[0])) {
	if (arg2[0] && ((arg2[0] >= 'a' && arg2[0] <= 'z') || (arg2[0] >= 'A' && arg2[0] <= 'Z'))) {
		gt = BG_GetGametypeForString(arg2);
		if (gt == -1)
		{
			trap_SendServerCommand(ent - g_entities, va("print \"Gametype (%s) unrecognised, defaulting to FFA/Deathmatch\n\"", arg2));
			gt = GT_FFA;
		}
	}
	// numeric but out of range
	else if (gt < 0 || gt >= GT_MAX_GAME_TYPE) {
		trap_SendServerCommand(ent - g_entities, va("print \"Gametype (%i) is out of range, defaulting to FFA/Deathmatch\n\"", gt));
		gt = GT_FFA;
	}

	// logically invalid gametypes, or gametypes not fully implemented in MP
	if (gt == GT_SINGLE_PLAYER) {
		trap_SendServerCommand(ent - g_entities, va("print \"This gametype is not supported (%s).\n\"", arg2));
		return qfalse;
	}

	level.votingGametype = qtrue;
	level.votingGametypeTo = gt;

	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %d", arg1, gt);
	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s %s", arg1, gameNames[gt]);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));

	return qtrue;
}

qboolean G_VoteKick(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int clientid = JP_ClientNumberFromString(ent, arg2);
	gentity_t *target = NULL;

	if (clientid == -1 || clientid == -2)
		return qfalse;

	target = &g_entities[clientid];
	if (!target || !target->inuse || !target->client)
		return qfalse;

	Com_sprintf(level.voteString, sizeof(level.voteString), "clientkick %d", clientid);
	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "kick %s", target->client->pers.netname);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

const char *G_GetArenaInfoByMap(const char *map);

void Cmd_MapList_f(gentity_t *ent) {
	int i, toggle = 0;
	char map[24] = "--", buf[512] = { 0 };

	Q_strcat(buf, sizeof(buf), "Map list:");

	//for (i = 0; i<level.arenas.num; i++) {
	for (i = 0; i < g_numArenas; i++) {
		//Q_strncpyz(map, Info_ValueForKey(level.arenas.infos[i], "map"), sizeof(map));
		Q_strncpyz(map, Info_ValueForKey(g_arenaInfos[i], "map"), sizeof(map));
		Q_StripColor(map);

		if (G_DoesMapSupportGametype(map, g_gametype.integer)) {
			char *tmpMsg = va(" ^%c%s", (++toggle & 1) ? COLOR_GREEN : COLOR_YELLOW, map);
			if (strlen(buf) + strlen(tmpMsg) >= sizeof(buf)) {
				trap_SendServerCommand(ent - g_entities, va("print \"%s\"", buf));
				buf[0] = '\0';
			}
			Q_strcat(buf, sizeof(buf), tmpMsg);
		}
	}

	trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));
}

typedef struct mapname_s {
	const char	*name;
} mapname_t;

int mapnamecmp(const void *a, const void *b) {
	return Q_stricmp((const char *)a, ((mapname_t*)b)->name);
}

mapname_t defaultMaps[] = {
	{ "ctf_bespin" },
	{ "ctf_imperial" },
	{ "ctf_ns_streets" },
	{ "ctf_yavin" },
	{ "duel_bay" },
	{ "duel_carbon" },
	{ "duel_jedi" },
	{ "duel_pit" },
	{ "ffa_bespin" },
	{ "ffa_deathstar" },
	{ "ffa_imperial" },
	{ "ffa_ns_hideout" },
	{ "ffa_raven" },
	{ "ffa_yavin" },
	{ "pit" }
};
static const size_t numDefaultMaps = ARRAY_LEN(defaultMaps);

qboolean IsBaseMap(char *s) {
	if ((mapname_t *)bsearch(s, defaultMaps, numDefaultMaps, sizeof(defaultMaps[0]), mapnamecmp))
		return qtrue;
	return qfalse;
}

int compcstr(const void * a, const void * b) {
	const char * aa = *(const char * *)a;
	const char * bb = *(const char * *)b;
	return strcmp(aa, bb);
}

static qboolean CheckAdminCmd(gentity_t *ent, int command, char *commandString) {
	if (!ent || !ent->client)
		return qfalse;

	if (ent->client && ent->client->sess.fullAdmin) {//Logged in as full admin
		if (!(g_fullAdminLevel.integer & (1 << command))) {
			trap_SendServerCommand(ent - g_entities, va("print \"You are not authorized to use this command (%s).\n\"", commandString));
			return qfalse;
		}
	}
	else if (ent->client && ent->client->sess.juniorAdmin) {//Logged in as junior admin
		if (!(g_juniorAdminLevel.integer & (1 << command))) {
			trap_SendServerCommand(ent - g_entities, va("print \"You are not authorized to use this command (%s).\n\"", commandString));
			return qfalse;
		}
	}
	else {//Not logged in
		trap_SendServerCommand(ent - g_entities, va("print \"You must be logged in to use this command (%s).\n\"", commandString));
		return qfalse;
	}
	return qtrue;
}

void Cmd_AmMapList_f(gentity_t *ent)
{
	char				unsortedMaps[4096], buf[512] = { 0 };
	char*				mapname;
	char*				sortedMaps[512];
	int					i, len, numMaps;
	unsigned int		count = 0, baseMapCount = 0;
	const unsigned int limit = 192, MAX_MAPS = 512;

	if (!CheckAdminCmd(ent, A_LISTMAPS, "amListMaps"))
		return;

	numMaps = trap_FS_GetFileList("maps", ".bsp", unsortedMaps, sizeof(unsortedMaps));
	if (numMaps) {
		if (numMaps > MAX_MAPS)
			numMaps = MAX_MAPS;
		mapname = unsortedMaps;
		for (i = 0; i < numMaps; i++) {
			len = strlen(mapname);
			if (!Q_stricmp(mapname + len - 4, ".bsp"))
				mapname[len - 4] = '\0';
			sortedMaps[i] = mapname;//String_Alloc(mapname);
			mapname += len + 1;
		}

		qsort(sortedMaps, numMaps, sizeof(sortedMaps[0]), compcstr);
		Q_strncpyz(buf, "   ", sizeof(buf));

		for (i = 0; i < numMaps; i++) {
			char *tmpMsg = NULL;

			if (IsBaseMap(sortedMaps[i])) { //Ideally this could be done before the qsort, but that likes to crash it since it changes the array size or something
				baseMapCount++;
				continue;
			}
			tmpMsg = va(" ^3%-32s    ", sortedMaps[i]);
			if (count >= limit) {
				tmpMsg = va("\n   %s", tmpMsg);
				count = 0;
			}
			if (strlen(buf) + strlen(tmpMsg) >= sizeof(buf)) {
				trap_SendServerCommand(ent - g_entities, va("print \"%s\"", buf));
				buf[0] = '\0';
			}
			count += strlen(tmpMsg);
			Q_strcat(buf, sizeof(buf), tmpMsg);
		}
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));
		trap_SendServerCommand(ent - g_entities, va("print \"^5%i maps listed\n\"", numMaps - baseMapCount));
	}
}

qboolean G_VoteMap(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	char s[MAX_CVAR_VALUE_STRING] = { 0 }, bspName[MAX_QPATH] = { 0 }, *mapName = NULL, *mapName2 = NULL;
	fileHandle_t fp = NULL_FILE;
	const char *arenaInfo;

	// didn't specify a map, show available maps
	if (numArgs < 3) {
		Cmd_MapList_f(ent);
		return qfalse;
	}

	if (strchr(arg2, '\\')) {
		trap_SendServerCommand(ent - g_entities, "print \"Can't have mapnames with a \\\n\"");
		return qfalse;
	}

	Com_sprintf(bspName, sizeof(bspName), "maps/%s.bsp", arg2);
	if (trap_FS_FOpenFile(bspName, &fp, FS_READ) <= 0) {
		trap_SendServerCommand(ent - g_entities, va("print \"Can't find map %s on server\n\"", bspName));
		if (fp != NULL_FILE)
			trap_FS_FCloseFile(fp);
		return qfalse;
	}
	trap_FS_FCloseFile(fp);

	if (!G_DoesMapSupportGametype(arg2, g_gametype.integer)) {
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE_MAPNOTSUPPORTEDBYGAME")));
		return qfalse;
	}

	// preserve the map rotation
	trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
	if (*s)
		Com_sprintf(level.voteString, sizeof(level.voteString), "%s %s; set nextmap \"%s\"", arg1, arg2, s);
	else
		Com_sprintf(level.voteString, sizeof(level.voteString), "%s %s", arg1, arg2);

	arenaInfo = G_GetArenaInfoByMap(arg2);
	if (arenaInfo) {
		mapName = Info_ValueForKey(arenaInfo, "longname");
		mapName2 = Info_ValueForKey(arenaInfo, "map");
	}

	if (!mapName || !mapName[0])
		mapName = "ERROR";

	if (!mapName2 || !mapName2[0])
		mapName2 = "ERROR";

	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "map %s (%s)", mapName, mapName2);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteMapRestart(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int n = Com_Clampi(0, 60, atoi(arg2));
	if (numArgs < 3)
		n = 5;
	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %i", arg1, n);
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteNextmap(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	char s[MAX_CVAR_VALUE_STRING];

	trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
	if (!*s) {
		trap_SendServerCommand(ent - g_entities, "print \"nextmap not set.\n\"");
		return qfalse;
	}
	//SiegeClearSwitchData();
	Com_sprintf(level.voteString, sizeof(level.voteString), "vstr nextmap");
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteTimelimit(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	float tl = Com_Clamp(0.0f, 35790.0f, atof(arg2));
	if (Q_isintegral(tl))
		Com_sprintf(level.voteString, sizeof(level.voteString), "%s %i", arg1, (int)tl);
	else
		Com_sprintf(level.voteString, sizeof(level.voteString), "%s %.3f", arg1, tl);
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteWarmup(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int n = Com_Clampi(0, 1, atoi(arg2));
	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %i", arg1, n);
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteTeamSize(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int ts = Com_Clampi(0, 32, atof(arg2)); //uhhh... k


	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %i", arg1, ts);

	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteVSTR(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	char vstr[64] = { 0 };
	char buf[MAX_CVAR_VALUE_STRING];

	Q_strncpyz(vstr, arg2, sizeof(vstr));
	//clean the string?
	Q_strlwr(vstr);
	Q_CleanStr(vstr);

	//Check if vstr exists, if not return qfalse.
	trap_Cvar_VariableStringBuffer(vstr, buf, sizeof(buf));
	if (!Q_stricmp(buf, ""))
		return qfalse;

	Com_sprintf(level.voteString, sizeof(level.voteString), "%s %s", arg1, vstr);

	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VotePoll(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	char question[64] = { 0 };

	Q_strncpyz(question, arg2, sizeof(question));
	//clean the string?
	Q_strlwr(question);
	Q_CleanStr(question);

	Com_sprintf(level.voteString, sizeof(level.voteString), "");

	Q_strncpyz(level.voteDisplayString, va("poll: %s", question), sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, va("poll: %s", question), sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VotePause(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {

	Com_sprintf(level.voteString, sizeof(level.voteString), "pause");
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteReset(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {

	Com_sprintf(level.voteString, sizeof(level.voteString), "resetScores");
	Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

qboolean G_VoteForceSpec(gentity_t *ent, int numArgs, const char *arg1, const char *arg2) {
	int n = atoi(arg2);

	if (n < 0 || n >= level.maxclients) {
		trap_SendServerCommand(ent - g_entities, va("print \"invalid client number %d.\n\"", n));
		return qfalse;
	}

	if (g_entities[n].client->pers.connected == CON_DISCONNECTED) {
		trap_SendServerCommand(ent - g_entities, va("print \"there is no client with the client number %d.\n\"", n));
		return qfalse;
	}

	Com_sprintf(level.voteString, sizeof(level.voteString), "forceteam %i s", n);
	Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "forcespec %s", g_entities[n].client->pers.netname);
	Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	return qtrue;
}

typedef struct voteString_s {
	const char	*string;
	const char	*aliases;	// space delimited list of aliases, will always show the real vote string
	qboolean(*func)(gentity_t *ent, int numArgs, const char *arg1, const char *arg2);
	int			numArgs;	// number of REQUIRED arguments, not total/optional arguments
	unsigned int validGT;	// bit-flag of valid gametypes
	qboolean	voteDelay;	// if true, will delay executing the vote string after it's accepted by g_voteDelay
	const char	*shortHelp;	// NULL if no arguments needed
} voteString_t;

static voteString_t validVoteStrings[] = {
	//	vote string				aliases										# args	valid gametypes							exec delay		short help
	{ "capturelimit",			"caps",				G_VoteCapturelimit,		1,		GTB_CTF | GTB_CTY,						qtrue,			"<num>" },
	{ "clientkick",				NULL,				G_VoteClientkick,		1,		GTB_ALL,								qfalse,			"<clientnum>" },
	{ "forcespec",				"forcespec",		G_VoteForceSpec,		1,		GTB_ALL,								qfalse,			"<clientnum>" },
	{ "fraglimit",				"frags",			G_VoteFraglimit,		1,		GTB_ALL & ~(GTB_SIEGE|GTB_CTF|GTB_CTY), qtrue,			"<num>" },
	{ "g_doWarmup",				"dowarmup warmup",	G_VoteWarmup,			1,		GTB_ALL,								qtrue,			"<0-1>" },
	{ "g_gametype",				"gametype gt mode", G_VoteGametype,			1,		GTB_ALL,								qtrue,			"<num or name>" },
	{ "kick",					NULL,				G_VoteKick,				1,		GTB_ALL,								qfalse,			"<client name>" },
	{ "map",					NULL,				G_VoteMap,				0,		GTB_ALL,								qtrue,			"<name>" },
	{ "map_restart",			"restart",			G_VoteMapRestart,		0,		GTB_ALL,								qtrue,			"<optional delay>" },
	{ "nextmap",				NULL,				G_VoteNextmap,			0,		GTB_ALL,								qtrue,			NULL },
	{ "sv_maxteamsize",			"teamsize",			G_VoteTeamSize,			1,		GTB_TEAM|GTB_SIEGE|GTB_CTY|GTB_CTF,		qtrue,			"<num>" },
	{ "timelimit",				"time",				G_VoteTimelimit,		1,		GTB_ALL,								qtrue,			"<num>" },
	{ "vstr",					"vstr",				G_VoteVSTR,				1,		GTB_ALL,								qtrue,			"<vstr name>" },
	{ "poll",					"poll",				G_VotePoll,				1,		GTB_ALL,								qfalse,			"<poll question>" },
	{ "pause",					"pause",			G_VotePause,			0,		GTB_ALL,								qfalse,			NULL },
	{ "score_restart",			"NULL",				G_VoteReset,			0,		GTB_ALL,								qfalse,			NULL },
};
static const int validVoteStringsSize = ARRAY_LEN(validVoteStrings);

void TimeToString(int duration_ms, char *timeStr, size_t strSize, qboolean noMs);
VoteFloodProtect_t	voteFloodProtect[voteFloodProtectSize];//32 courses, 9 styles, 10 spots on highscore list
void Cmd_CallVote_f(gentity_t *ent) {
	int				i = 0, numArgs = 0;
	char			arg1[MAX_CVAR_VALUE_STRING] = { 0 };
	char			arg2[MAX_CVAR_VALUE_STRING] = { 0 };
	voteString_t	*vote = NULL;

	// not allowed to vote at all
	if (!g_allowVote.integer) {
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE")));
		return;
	}

	if ((g_tweakVote.integer & TV_MAPLOADTIMEOUT) && (level.startTime > (level.time - 1000 * 30))) { //Dont let a vote be called within 30sec of mapload ever
		trap_SendServerCommand(ent - g_entities, "print \"You are not allowed to callvote within 30 seconds of map load.\n\"");//print to wait X more minutes..seconds?
		return;
	} //fuck this stupid thing.. why does it work on 1 server but not the other..	

	if ((g_fullAdminLevel.integer & (1 << A_CALLVOTE)) || (g_juniorAdminLevel.integer & (1 << A_CALLVOTE))) { //Admin only voting mode
		if (ent->client->sess.fullAdmin)//Logged in as full admin
		{
			if (!(g_fullAdminLevel.integer & (1 << A_CALLVOTE)))
			{
				trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command (amLockTeam).\n\"");
				return;
			}
		}
		else if (ent->client->sess.juniorAdmin)//Logged in as junior admin
		{
			if (!(g_juniorAdminLevel.integer & (1 << A_CALLVOTE)))
			{
				trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command (amLockTeam).\n\"");
				return;
			}
		}
		else//Not logged in
		{
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE")));
			return;
		}
	}

	// vote in progress
	if (level.voteTime) {
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEINPROGRESS")));
		return;
	}

	// can't vote as a spectator, except in (power)duel.. fuck this logic

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || (ent->client->sess.sessionTeam == TEAM_FREE && g_gametype.integer >= GT_TEAM)) { //If we are in spec or racemode
		if (g_gametype.integer == GT_SAGA && g_tweakVote.integer & TV_ALLOW_SAGASPECVOTE) {
		}
		else if (g_gametype.integer >= GT_TEAM && g_tweakVote.integer & TV_ALLOW_CTFTFFASPECVOTE) {
		}
		else if (g_tweakVote.integer & TV_ALLOW_SPECVOTE) {
		}
		else {
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSPECVOTE")));
			return;
		}
	}

	/*
	if ( level.gametype != GT_DUEL && level.gametype != GT_POWERDUEL && (ent->client->sess.sessionTeam == TEAM_SPECTATOR || (ent->client->sess.sessionTeam == TEAM_FREE && level.gametype >= GT_TEAM))) {
	if (level.gametype >= GT_TEAM || !g_tweakVote.integer) {
	trap->SendServerCommand( ent-g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOSPECVOTE" ) ) );
	return;
	}
	}
	*/

	if (g_tweakVote.integer & TV_FLOODPROTECTBYIP) {
		char ourIP[NET_ADDRSTRMAXLEN] = { 0 };
		char *p = NULL;
		int j;

		Q_strncpyz(ourIP, ent->client->sess.IP, sizeof(ourIP));
		p = strchr(ourIP, ':');
		if (p)
			*p = 0;

		//trap->Print("Checking if client can vote: his ip is %s\n", ourIP);

		//Check if we are allowed to call vote
		if (g_voteTimeout.integer) {
			for (j = 0; j<voteFloodProtectSize; j++) {
				//trap->Print("Searching slot: %i (%s, %i)\n", j, voteFloodProtect[j].ip, voteFloodProtect[j].voteTimeoutUntil);
				if (!Q_stricmp(voteFloodProtect[j].ip, ourIP)) {
					//trap->Print("Found clients IP in array!\n");
					const int voteTimeout = voteFloodProtect[j].failCount + 1 * 1000 * g_voteTimeout.integer;

					if (voteFloodProtect[j].voteTimeoutUntil && (voteFloodProtect[j].voteTimeoutUntil > trap_Milliseconds())) { //compare this to something other than level.time ?
						//trap->Print("Client has just failed a vote, dont let them call this new one!\n");
						char timeStr[32];
						TimeToString((voteFloodProtect[j].voteTimeoutUntil - trap_Milliseconds()), timeStr, sizeof(timeStr), qtrue);
						trap_SendServerCommand(ent - g_entities, va("print \"Please wait %s before calling a new vote.\n\"", timeStr));
						return;
					}
					break;
				}
				else if (!voteFloodProtect[j].ip[0]) {
					//trap->Print("Finished array search without finding clients IP! They have not failed a vote yet!\n");
					break;
				}
			}
		}

		//trap->Print("Client is allowed to call vote!\n");

		//We are allowed to call a vote if we get here
		Q_strncpyz(level.callVoteIP, ourIP, sizeof(level.callVoteIP));
	}

	// make sure it is a valid command to vote on
	numArgs = trap_Argc();
	trap_Argv(1, arg1, sizeof(arg1));
	if (numArgs > 1)
		Q_strncpyz(arg2, ConcatArgs(2), sizeof(arg2));

	// filter ; \n \r
	if (Q_strchrs(arg1, ";\r\n") || Q_strchrs(arg2, ";\r\n")) {
		trap_SendServerCommand(ent - g_entities, "print \"Invalid vote string.\n\"");
		return;
	}

	if ((g_tweakVote.integer & TV_MAPCHANGELOCKOUT) && !Q_stricmp(arg1, "map") && (g_gametype.integer == GT_FFA) && (level.startTime > (level.time - 1000 * 60 * 10))) { //Dont let a map vote be called within 10 mins of map load if we are in ffa
		char timeStr[32];
		TimeToString((1000 * 60 * 10 - (level.time - level.startTime)), timeStr, sizeof(timeStr), qtrue);
		trap_SendServerCommand(ent - g_entities, va("print \"The server just changed to this map, please wait %s before calling a map vote.\n\"", timeStr));
		return;
	}

	// check for invalid votes
	for (i = 0; i<validVoteStringsSize; i++) {
		if (!(g_allowVote.integer & (1 << i)))
			continue;

		if (!Q_stricmp(arg1, validVoteStrings[i].string))
			break;

		// see if they're using an alias, and set arg1 to the actual vote string
		if (validVoteStrings[i].aliases) {
			char tmp[MAX_TOKEN_CHARS] = { 0 }, *p = NULL;
			//const char *delim = " ";
			char delim = ' ';
			Q_strncpyz(tmp, validVoteStrings[i].aliases, sizeof(tmp));
			//p = (char *)strtok((char)tmp, (const char*)delim);
			p = strchr(tmp, delim);
			while (p != NULL) {
				if (!Q_stricmp(arg1, p)) {
					Q_strncpyz(arg1, validVoteStrings[i].string, sizeof(arg1));
					goto validVote;
				}
				//p = (char *)strtok(NULL, (const char*)delim);
				p = strchr(NULL, delim);
			}
		}
	}
	// invalid vote string, abandon ship
	if (i == validVoteStringsSize) {
		char buf[1024] = { 0 };
		int toggle = 0;
		trap_SendServerCommand(ent - g_entities, "print \"Invalid vote string.\n\"");
		trap_SendServerCommand(ent - g_entities, "print \"Allowed vote strings are: \"");
		for (i = 0; i<validVoteStringsSize; i++) {
			if (!(g_allowVote.integer & (1 << i)))
				continue;

			toggle = !toggle;
			if (validVoteStrings[i].shortHelp) {
				Q_strcat(buf, sizeof(buf), va("^%c%s %s ",
					toggle ? COLOR_GREEN : COLOR_YELLOW,
					validVoteStrings[i].string,
					validVoteStrings[i].shortHelp));
			}
			else {
				Q_strcat(buf, sizeof(buf), va("^%c%s ",
					toggle ? COLOR_GREEN : COLOR_YELLOW,
					validVoteStrings[i].string));
			}
		}

		//FIXME: buffer and send in multiple messages in case of overflow
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", buf));
		return;
	}

validVote:
	vote = &validVoteStrings[i];
	if (!(vote->validGT & (1 << g_gametype.integer))) {
		trap_SendServerCommand(ent - g_entities, va("print \"%s is not applicable in this gametype.\n\"", arg1));
		return;
	}

	if (numArgs < vote->numArgs + 2) {
		trap_SendServerCommand(ent - g_entities, va("print \"%s requires more arguments: %s\n\"", arg1, vote->shortHelp));
		return;
	}

	level.votingGametype = qfalse;

	if (g_tweakVote.integer & TV_MAPCHANGEVOTEDELAY) {
		if (!Q_stricmp(arg1, "map"))
			level.voteExecuteDelay = vote->voteDelay ? g_voteDelay.integer : 0;
		else
			level.voteExecuteDelay = vote->voteDelay ? 3000 : 0;
	}
	else
		level.voteExecuteDelay = vote->voteDelay ? g_voteDelay.integer : 0;

	// there is still a vote to be executed, execute it and store the new vote
	if (level.voteExecuteTime) { //bad idea
		if (g_tweakVote.integer) { //wait what is this..
			trap_SendServerCommand(ent - g_entities, "print \"You are not allowed to call a new vote at this time.\n\"");//print to wait X more minutes..seconds?
			return;
		}
		else {
			level.voteExecuteTime = 0;
			trap_SendConsoleCommand(EXEC_APPEND, va("%s\n", level.voteString));
		}
	}

	// pass the args onto vote-specific handlers for parsing/filtering
	if (vote->func) {
		if (!vote->func(ent, numArgs, arg1, arg2))
			return;
	}
	// otherwise assume it's a command
	else {
		Com_sprintf(level.voteString, sizeof(level.voteString), "%s \"%s\"", arg1, arg2);
		Q_strncpyz(level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString));
		Q_strncpyz(level.voteStringClean, level.voteString, sizeof(level.voteStringClean));
	}
	Q_strstrip(level.voteStringClean, "\"\n\r", NULL);

	trap_SendServerCommand(-1, va("print \"%s^7 %s (%s^7)\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLCALLEDVOTE"), level.voteStringClean));
	G_LogPrintf("%s^7 %s (%s^7)\n", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLCALLEDVOTE"), level.voteStringClean);

	// start the voting, the caller automatically votes yes
	level.voteTime = level.time;
	if (!Q_stricmp(arg1, "poll"))
		level.voteYes = 0;
	else
		level.voteYes = 1;
	level.voteNo = 0;

	for (i = 0; i<level.maxclients; i++) {
		level.clients[i].mGameFlags &= ~PSG_VOTED;
		level.clients[i].pers.vote = 0;
	}

	if (Q_stricmp(arg1, "poll")) {
		ent->client->mGameFlags |= PSG_VOTED;
		ent->client->pers.vote = 1;
	}

	trap_SetConfigstring(CS_VOTE_TIME, va("%i", level.voteTime));
	//trap->SetConfigstring( CS_VOTE_STRING,	level.voteDisplayString );	
	trap_SetConfigstring(CS_VOTE_STRING, va("%s", level.voteDisplayString));	 //dunno why this has to be done here..

	trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteYes));
	trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteNo));
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f(gentity_t *ent) {
	char		msg[64] = { 0 };
	int i;

	if (!level.voteTime) {
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEINPROG")));
		return;
	}
	if (ent->client->mGameFlags & PSG_VOTED) {
		trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEALREADY")));
		return;
	}
	if (g_gametype.integer != GT_TOURNAMENT && g_gametype.integer)
	{
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR && !(g_tweakVote.integer & TV_ALLOW_SPECVOTE)) {
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEASSPEC")));
			return;
		}
	}

	if (g_tweakVote.integer & TV_FLOODPROTECTBYIP) { //Dont let ppl with same IP have more than 1 vote
		char ourIP[NET_ADDRSTRMAXLEN] = { 0 };
		char *n = NULL;
		gclient_t	*cl;

		Q_strncpyz(ourIP, ent->client->sess.IP, sizeof(ourIP));
		n = strchr(ourIP, ':');
		if (n)
			*n = 0;

		for (i = 0; i<MAX_CLIENTS; i++)
		{//Build a list of clients
			char *tmpMsg = NULL;
			if (!g_entities[i].inuse)
				continue;

			cl = &level.clients[i];
			if (cl->pers.netname[0])
			{
				char theirIP[NET_ADDRSTRMAXLEN] = { 0 };
				char *p = NULL;

				Q_strncpyz(theirIP, cl->sess.IP, sizeof(theirIP));
				p = strchr(theirIP, ':');
				if (p) //loda - fix ip sometimes not printing in amstatus?
					*p = 0;

				if (!Q_stricmp(theirIP, ourIP)) {
					if (cl->mGameFlags & PSG_VOTED) {
						trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEALREADY")));
						return;
					}
				}
			}
		}
	}

	ent->client->mGameFlags |= PSG_VOTED;

	trap_Argv(1, msg, sizeof(msg));

	if (tolower(msg[0]) == 'y' || msg[0] == '1') {
		level.voteYes++;
		ent->client->pers.vote = 1;
		trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteYes));

		if (g_tweakVote.integer & TV_SHOW_VOTES)
			trap_SendServerCommand(-1, va("print \"%s^7 voted yes\n\"", ent->client->pers.netname));
		else
			trap_SendServerCommand(ent - g_entities, va("print \"%s (Yes)\n\"", G_GetStringEdString("MP_SVGAME", "PLVOTECAST")));
		G_LogPrintf("%s^7 voted yes\n", ent->client->pers.netname);

	}
	else {
		level.voteNo++;
		ent->client->pers.vote = 2;
		trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteNo));
		if (g_tweakVote.integer & TV_SHOW_VOTES)
			trap_SendServerCommand(-1, va("print \"%s^7 voted no\n\"", ent->client->pers.netname));
		else
			trap_SendServerCommand(ent - g_entities, va("print \"%s (No)\n\"", G_GetStringEdString("MP_SVGAME", "PLVOTECAST")));
		G_LogPrintf("%s^7 voted no\n", ent->client->pers.netname);
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

qboolean G_TeamVoteLeader(gentity_t *ent, int cs_offset, team_t team, int numArgs, const char *arg1, const char *arg2) {
	int clientid = numArgs == 2 ? ent->s.number : JP_ClientNumberFromString(ent, arg2);
	gentity_t *target = NULL;

	if (clientid == -1 || clientid == -2)
		return qfalse;

	target = &g_entities[clientid];
	if (!target || !target->inuse || !target->client)
		return qfalse;

	if (target->client->sess.sessionTeam != team)
	{
		trap_SendServerCommand(ent - g_entities, va("print \"User %s is not on your team\n\"", arg2));
		return qfalse;
	}

	Com_sprintf(level.teamVoteString[cs_offset], sizeof(level.teamVoteString[cs_offset]), "leader %d", clientid);
	Q_strncpyz(level.teamVoteDisplayString[cs_offset], level.teamVoteString[cs_offset], sizeof(level.teamVoteDisplayString[cs_offset]));
	Q_strncpyz(level.teamVoteStringClean[cs_offset], level.teamVoteString[cs_offset], sizeof(level.teamVoteStringClean[cs_offset]));
	return qtrue;
}

/*
==================
Cmd_CallTeamVote_f
==================
*/
void Cmd_CallTeamVote_f( gentity_t *ent ) {
	int		i, team, cs_offset;
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !g_allowVote.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTE")) );
		return;
	}

	if ( level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEALREADY")) );
		return;
	}
	if ( ent->client->pers.teamVoteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MAXTEAMVOTES")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSPECVOTE")) );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	arg2[0] = '\0';
	for ( i = 2; i < trap_Argc(); i++ ) {
		if (i > 2)
			strcat(arg2, " ");
		trap_Argv( i, &arg2[strlen(arg2)], sizeof( arg2 ) - strlen(arg2) );
	}

	if( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	if ( !Q_stricmp( arg1, "leader" ) ) {
		char netname[MAX_NETNAME], leader[MAX_NETNAME];

		if ( !arg2[0] ) {
			i = ent->client->ps.clientNum;
		}
		else {
			// numeric values are just slot numbers
			for (i = 0; i < 3; i++) {
				if ( !arg2[i] || arg2[i] < '0' || arg2[i] > '9' )
					break;
			}
			if ( i >= 3 || !arg2[i]) {
				i = atoi( arg2 );
				if ( i < 0 || i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Bad client slot: %i\n\"", i) );
					return;
				}

				if ( !g_entities[i].inuse ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Client %i is not active\n\"", i) );
					return;
				}
			}
			else {
				Q_strncpyz(leader, arg2, sizeof(leader));
				Q_CleanStr(leader);
				for ( i = 0 ; i < level.maxclients ; i++ ) {
					if ( level.clients[i].pers.connected == CON_DISCONNECTED )
						continue;
					if (level.clients[i].sess.sessionTeam != team)
						continue;
					Q_strncpyz(netname, level.clients[i].pers.netname, sizeof(netname));
					Q_CleanStr(netname);
					if ( !Q_stricmp(netname, leader) ) {
						break;
					}
				}
				if ( i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"%s is not a valid player on your team.\n\"", arg2) );
					return;
				}
			}
		}
		Com_sprintf(arg2, sizeof(arg2), "%d", i);
	} else {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent-g_entities, "print \"Team vote commands are: leader <player>.\n\"" );
		return;
	}

	Com_sprintf( level.teamVoteString[cs_offset], sizeof( level.teamVoteString[cs_offset] ), "%s %s", arg1, arg2 );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED )
			continue;
		if (level.clients[i].sess.sessionTeam == team)
			trap_SendServerCommand( i, va("print \"%s called a team vote.\n\"", ent->client->pers.netname ) );
	}

	// start the voting, the caller autoamtically votes yes
	level.teamVoteTime[cs_offset] = level.time;
	level.teamVoteYes[cs_offset] = 1;
	level.teamVoteNo[cs_offset] = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam == team)
			level.clients[i].ps.eFlags &= ~EF_TEAMVOTED;
	}
	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, va("%i", level.teamVoteTime[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_STRING + cs_offset, level.teamVoteString[cs_offset] );
	trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );
}

/*
==================
Cmd_TeamVote_f
==================
*/
void Cmd_TeamVote_f( gentity_t *ent ) {
	int			team, cs_offset;
	char		msg[64];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOTEAMVOTEINPROG")) );
		return;
	}
	if ( ent->client->ps.eFlags & EF_TEAMVOTED ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEALREADYCAST")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTEASSPEC")) );
		return;
	}

	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PLTEAMVOTECAST")) );

	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.teamVoteYes[cs_offset]++;
		trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	} else {
		level.teamVoteNo[cs_offset]++;
		trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );	
	}

	// a majority will be determined in TeamCheckVote, which will also account
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
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOCHEATS")));
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
Cmd_Stats_f
=================
*/
void Cmd_Stats_f( gentity_t *ent ) {
/*
	int max, n, i;

	max = trap_AAS_PointReachabilityAreaIndex( NULL );

	n = 0;
	for ( i = 0; i < max; i++ ) {
		if ( ent->client->areabits[i >> 3] & (1 << (i & 7)) )
			n++;
	}

	//trap_SendServerCommand( ent-g_entities, va("print \"visited %d of %d areas\n\"", n, max));
	trap_SendServerCommand( ent-g_entities, va("print \"%d%% level coverage\n\"", n * 100 / max));
*/
}

int G_ItemUsable(playerState_t *ps, int forcedUse)
{
	vec3_t fwd, fwdorg, dest, pos;
	vec3_t yawonly;
	vec3_t mins, maxs;
	vec3_t trtest;
	trace_t tr;

	if (ps->usingATST)
	{
		return 0;
	}
	
	if (ps->pm_flags & PMF_USE_ITEM_HELD)
	{ //force to let go first
		return 0;
	}

	if (!forcedUse)
	{
		forcedUse = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
	}

	switch (forcedUse)
	{
	case HI_MEDPAC:
		if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
		{
			return 0;
		}

		if (ps->stats[STAT_HEALTH] <= 0)
		{
			return 0;
		}

		return 1;
	case HI_SEEKER:
		if (ps->eFlags & EF_SEEKERDRONE)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SEEKER_ALREADYDEPLOYED);
			return 0;
		}

		return 1;
	case HI_SENTRY_GUN:
		if (ps->fd.sentryDeployed)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_ALREADYPLACED);
			return 0;
		}

		yawonly[ROLL] = 0;
		yawonly[PITCH] = 0;
		yawonly[YAW] = ps->viewangles[YAW];

		VectorSet( mins, -8, -8, 0 );
		VectorSet( maxs, 8, 8, 24 );

		AngleVectors(yawonly, fwd, NULL, NULL);

		fwdorg[0] = ps->origin[0] + fwd[0]*64;
		fwdorg[1] = ps->origin[1] + fwd[1]*64;
		fwdorg[2] = ps->origin[2] + fwd[2]*64;

		trtest[0] = fwdorg[0] + fwd[0]*16;
		trtest[1] = fwdorg[1] + fwd[1]*16;
		trtest[2] = fwdorg[2] + fwd[2]*16;

		trap_Trace(&tr, ps->origin, mins, maxs, trtest, ps->clientNum, MASK_PLAYERSOLID);

		if ((tr.fraction != 1 && tr.entityNum != ps->clientNum) || tr.startsolid || tr.allsolid)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_NOROOM);
			return 0;
		}

		return 1;
	case HI_SHIELD:
		mins[0] = -8;
		mins[1] = -8;
		mins[2] = 0;

		maxs[0] = 8;
		maxs[1] = 8;
		maxs[2] = 8;

		AngleVectors (ps->viewangles, fwd, NULL, NULL);
		fwd[2] = 0;
		VectorMA(ps->origin, 64, fwd, dest);
		trap_Trace(&tr, ps->origin, mins, maxs, dest, ps->clientNum, MASK_SHOT );
		if (tr.fraction > 0.9 && !tr.startsolid && !tr.allsolid)
		{
			VectorCopy(tr.endpos, pos);
			VectorSet( dest, pos[0], pos[1], pos[2] - 4096 );
			trap_Trace( &tr, pos, mins, maxs, dest, ps->clientNum, MASK_SOLID );
			if ( !tr.startsolid && !tr.allsolid )
			{
				return 1;
			}
		}
		G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SHIELD_NOROOM);
		return 0;
	default:
		return 1;
	}
}

extern int saberOffSound;
extern int saberOnSound;

void Cmd_ToggleSaber_f(gentity_t *ent)
{
	if (!saberOffSound || !saberOnSound)
	{
		saberOffSound = G_SoundIndex("sound/weapons/saber/saberoffquick.wav");
		saberOnSound = G_SoundIndex("sound/weapons/saber/saberon.wav");
	}

	if (ent->client->ps.saberInFlight)
	{
		return;
	}

	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (ent->client->ps.weapon != WP_SABER)
	{
		return;
	}

//	if (ent->client->ps.duelInProgress && !ent->client->ps.saberHolstered)
//	{
//		return;
//	}

	if (ent->client->ps.duelTime >= level.time)
	{
		return;
	}

	if (ent->client->ps.saberLockTime >= level.time)
	{
		return;
	}

	if (ent->client && ent->client->ps.weaponTime < 1)
	{
		if (ent->client->ps.saberHolstered)
		{
			ent->client->ps.saberHolstered = qfalse;
			G_Sound(ent, CHAN_AUTO, saberOnSound);
		}
		else
		{
			ent->client->ps.saberHolstered = qtrue;
			G_Sound(ent, CHAN_AUTO, saberOffSound);

			//prevent anything from being done for 400ms after holster
			ent->client->ps.weaponTime = 400;
		}
	}
}

void Cmd_SaberAttackCycle_f(gentity_t *ent)
{
	int selectLevel = 0;

	if ( !ent || !ent->client )
	{
		return;
	}
	/*
	if (ent->client->ps.weaponTime > 0)
	{ //no switching attack level when busy
		return;
	}
	*/

	if (ent->client->saberCycleQueue)
	{ //resume off of the queue if we haven't gotten a chance to update it yet
		selectLevel = ent->client->saberCycleQueue;
	}
	else
	{
		selectLevel = ent->client->ps.fd.saberAnimLevel;
	}

	selectLevel++;
	if ( selectLevel > ent->client->ps.fd.forcePowerLevel[FP_SABERATTACK] )
	{
		selectLevel = FORCE_LEVEL_1;
	}
/*
#ifndef FINAL_BUILD
	switch ( selectLevel )
	{
	case FORCE_LEVEL_1:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sfast\n\"", S_COLOR_BLUE) );
		break;
	case FORCE_LEVEL_2:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %smedium\n\"", S_COLOR_YELLOW) );
		break;
	case FORCE_LEVEL_3:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sstrong\n\"", S_COLOR_RED) );
		break;
	}
#endif
*/
	if (ent->client->ps.weaponTime <= 0)
	{ //not busy, set it now
		ent->client->ps.fd.saberAnimLevel = selectLevel;
	}
	else
	{ //can't set it now or we might cause unexpected chaining, so queue it
		ent->client->saberCycleQueue = selectLevel;
	}
}

qboolean G_OtherPlayersDueling(void)
{
	int i = 0;
	gentity_t *ent;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->inuse && ent->client && ent->client->ps.duelInProgress)
		{
			return qtrue;
		}
		i++;
	}

	return qfalse;
}

void Cmd_ForceDuel_f(gentity_t *ent) {
	Cmd_EngageDuel_f(ent, 1);
}

void Cmd_GunDuel_f(gentity_t *ent)
{
	char weapStr[64];//Idk, could be less
	int weap = WP_NONE;

	if (!g_allowGunDuel.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"Command not allowed. (gunduel).\n\"");
		return;
	}

	if (trap_Argc() > 2)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /engage_gunduel or engage_gunduel <weapon name>.\n\"");
		return;
	}

	if (trap_Argc() == 1)//Use current weapon
	{
		weap = ent->s.weapon;
		if (weap > LAST_USEABLE_WEAPON) //last usable is 16
			weap = WP_NONE;
	}
	else if (trap_Argc() == 2)//Use specified weapon
	{
		int i = 0;
		trap_Argv(1, weapStr, sizeof(weapStr));

		while (weapStr[i]) {
			weapStr[i] = tolower(weapStr[i]);
			i++;
		}

		if (!Q_stricmp("stun", weapStr) || !Q_stricmp("stunbaton", weapStr))
			weap = WP_STUN_BATON; //+ 14; //16 //17
		else if (!Q_stricmp("pistol", weapStr) || !Q_stricmp("bryar", weapStr))
			weap = WP_BRYAR_PISTOL;
		else if (!Q_stricmp("blaster", weapStr) || !Q_stricmp("e11", weapStr))
			weap = WP_BLASTER;
		else if (!Q_stricmp("sniper", weapStr) || !Q_stricmp("disruptor", weapStr))
			weap = WP_DISRUPTOR;
		else if (!Q_stricmp("bowcaster", weapStr))
			weap = WP_BOWCASTER;
		else if (!Q_stricmp("repeater", weapStr))
			weap = WP_REPEATER;
		else if (!Q_stricmp("demp2", weapStr) || !Q_stricmp("demp", weapStr))
			weap = WP_DEMP2;
		else if (!Q_stricmp("flechette", weapStr) || !Q_stricmp("shotgun", weapStr))
			weap = WP_FLECHETTE;
		else if (!Q_stricmp("rocket", weapStr) || !Q_stricmp("rockets", weapStr) || !Q_stricmp("rocketlauncher", weapStr))
			weap = WP_ROCKET_LAUNCHER;
		else if (!Q_stricmp("detpack", weapStr) || !Q_stricmp("detpacks", weapStr))
			weap = WP_DET_PACK;
		else if (!Q_stricmp("thermal", weapStr) || !Q_stricmp("thermals", weapStr) || !Q_stricmp("grenade", weapStr) || !Q_stricmp("grenades", weapStr) || !Q_stricmp("dets", weapStr))
			weap = WP_THERMAL;
		else if (!Q_stricmp("tripmine", weapStr) || !Q_stricmp("trip", weapStr) || !Q_stricmp("trips", weapStr))
			weap = WP_TRIP_MINE;
		else if (!Q_stricmp("all", weapStr))
			weap = LAST_USEABLE_WEAPON + 3; //16

	}

	if (weap == WP_NONE || weap == WP_SABER || (weap > (LAST_USEABLE_WEAPON + 3))) {
		//trap_SendServerCommand( ent-g_entities, "print \"Invalid weapon specified, using default case: Bryar\n\"" );
		weap = WP_BRYAR_PISTOL;//pff
	}

	Cmd_EngageDuel_f(ent, weap + 1);//Fuck it right here
}

void Cmd_EngageDuel_f(gentity_t *ent, int dueltype)
{
	trace_t tr;
	vec3_t forward, fwdOrg;

	//Private duel cvar on? Check this first
	if (!g_privateDuel.integer)
		return;


	if (g_gametype.integer == GT_TOURNAMENT || g_gametype.integer >= GT_TEAM)
	{ //rather pointless in this mode..
		if (dueltype == 0 || dueltype == 1)
			trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NODUEL_GAMETYPE")));
		else
			trap_SendServerCommand(ent - g_entities, "print \"This gametype does not support gun dueling.\n\"");
		return;
	}

	if (ent->client->ps.duelTime >= level.time)
		return;

	if ((dueltype == 0 || dueltype == 1) && ent->client->ps.weapon != WP_SABER)
		return;

	/*
	if (!ent->client->ps.saberHolstered)
	{ //must have saber holstered at the start of the duel
		return;
	}
	*/
	//NOTE: No longer doing this..

	if (ent->client->ps.saberInFlight)
		return;

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
		return;

	if (ent->client->ps.duelInProgress)
		return;

	if (ent->client->sess.raceMode)
		return;

	/*//New: Don't let a player duel if he just did and hasn't waited 10 seconds yet (note: If someone challenges him, his duel timer will reset so he can accept)
	if (ent->client->ps.fd.privateDuelTime > level.time)
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "CANTDUEL_JUSTDID")) );
		return;
	}

	if (G_OtherPlayersDueling())
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "CANTDUEL_BUSY")) );
		return;
	}*/

	AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );

	fwdOrg[0] = ent->client->ps.origin[0] + forward[0]*256;
	fwdOrg[1] = ent->client->ps.origin[1] + forward[1]*256;
	fwdOrg[2] = (ent->client->ps.origin[2]+ent->client->ps.viewheight) + forward[2]*256;

	JP_Trace(&tr, ent->client->ps.origin, NULL, NULL, fwdOrg, ent->s.number, MASK_PLAYERSOLID, qfalse, 0, 0);

	if (tr.fraction != 1 && tr.entityNum < MAX_CLIENTS)
	{
		gentity_t *challenged = &g_entities[tr.entityNum];

		if (!challenged || !challenged->client || !challenged->inuse ||
			challenged->health < 1 || challenged->client->ps.stats[STAT_HEALTH] < 1 ||
			(challenged->client->ps.weapon != WP_SABER && (dueltype == 0 || dueltype == 1)) || challenged->client->ps.duelInProgress ||
			challenged->client->ps.saberInFlight)
		{
			return;
		}

		if (g_gametype.integer >= GT_TEAM && OnSameTeam(ent, challenged))
			return;

		ent->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
		ent->client->ps.forceHandExtendTime = level.time + 1000; //Moved this up here

//jk2PRO - Serverside - Fullforce Duels + Duel Messages - Start
		if (challenged->client->ps.duelIndex == ent->s.number && (challenged->client->ps.duelTime + 2000) >= level.time && 
			(
			((dueltypes[challenged->client->ps.clientNum] == dueltype) && (dueltype == 0 || dueltype == 1))
			||
			(dueltypes[challenged->client->ps.clientNum] != 0 && (dueltypes[challenged->client->ps.clientNum] != 1) && dueltype > 1)
			))
		{
			//trap_SendServerCommand( /*challenged-g_entities*/-1, va("print \"%s %s %s!\n\"", challenged->client->pers.netname, G_GetStripEdString("SVINGAME", "PLDUELACCEPT"), ent->client->pers.netname) );

			ent->client->ps.duelInProgress = qtrue;
			challenged->client->ps.duelInProgress = qtrue;

			if (dueltype == 0 || dueltype == 1)
				dueltypes[ent->client->ps.clientNum] = dueltype;//why isn't this syncing the weapons they use? gun duels
			else {
				dueltypes[ent->client->ps.clientNum] = dueltypes[challenged->client->ps.clientNum];//dueltype;//okay this is why
			}

			ent->client->ps.duelTime = level.time + 2000; //loda fixme
			challenged->client->ps.duelTime = level.time + 2000; //loda fixme

			switch (dueltype)
			{
			case 0:
				//Saber Duel
				G_AddEvent(ent, EV_PRIVATE_DUEL, 0);
				G_AddEvent(challenged, EV_PRIVATE_DUEL, 0);
				trap_SendServerCommand(-1, va("print \"%s^7 %s %s^7! (Saber)\n\"", challenged->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELACCEPT"), ent->client->pers.netname));
				break;
			case 1://FF Duel
				G_AddEvent(ent, EV_PRIVATE_DUEL, 1);
				G_AddEvent(challenged, EV_PRIVATE_DUEL, 1);
				//trap_SendServerCommand(-1, va("print \"%s^7 %s %s^7! (Force)\n\"", challenged->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELACCEPT"), ent->client->pers.netname));
				break;
			default://Gun Duel
				G_AddEvent(ent, EV_PRIVATE_DUEL, dueltypes[ent->client->ps.clientNum]);
				G_AddEvent(challenged, EV_PRIVATE_DUEL, dueltypes[challenged->client->ps.clientNum]);
				trap_SendServerCommand(-1, va("print \"%s^7 %s %s^7! (Gun)\n\"", challenged->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELACCEPT"), ent->client->pers.netname));
				break;
			}

			//Old stuff, the switch above replaces this :)
			//G_AddEvent(ent, EV_PRIVATE_DUEL, 1);
			//G_AddEvent(challenged, EV_PRIVATE_DUEL, 1);

			//Holster their sabers now, until the duel starts (then they'll get auto-turned on to look cool)

			if (!ent->client->ps.saberHolstered)
			{
				G_Sound(ent, CHAN_AUTO, saberOffSound);
				ent->client->ps.weaponTime = 400;
				ent->client->ps.saberHolstered = qtrue;
			}
			if (!challenged->client->ps.saberHolstered)
			{
				G_Sound(challenged, CHAN_AUTO, saberOffSound);
				challenged->client->ps.weaponTime = 400;
				challenged->client->ps.saberHolstered = qtrue;
			}
			if (g_duelStartHealth.integer)
			{
				ent->health = ent->client->ps.stats[STAT_HEALTH] = g_duelStartHealth.integer;
				challenged->health = challenged->client->ps.stats[STAT_HEALTH] = g_duelStartHealth.integer;
			}
			if (g_duelStartArmor.integer)
			{
				ent->client->ps.stats[STAT_ARMOR] = g_duelStartArmor.integer;
				challenged->client->ps.stats[STAT_ARMOR] = g_duelStartArmor.integer;
			}

			ent->client->ps.fd.forcePower = ent->client->ps.fd.forcePowerMax; 
			challenged->client->ps.fd.forcePower = challenged->client->ps.fd.forcePowerMax;//Give them both max force power!

			ent->client->ps.fd.forceRageRecoveryTime = 0;
			challenged->client->ps.fd.forceRageRecoveryTime = 0; //Get rid of rage recovery when duel starts!
			if (dueltypes[challenged->client->ps.clientNum] > 1) { //2) { //1 ?
				int weapon = dueltypes[challenged->client->ps.clientNum] - 1; // - 2;
				if (weapon == LAST_USEABLE_WEAPON + 3) { //All weapons and ammo.
					int i;
					for (i = 1; i <= LAST_USEABLE_WEAPON; i++) {
						ent->client->ps.stats[STAT_WEAPONS] |= (1 << i);
						challenged->client->ps.stats[STAT_WEAPONS] |= (1 << i);
					}
					ent->client->ps.ammo[AMMO_BLASTER] = challenged->client->ps.ammo[AMMO_BLASTER] = 300;
					ent->client->ps.ammo[AMMO_POWERCELL] = challenged->client->ps.ammo[AMMO_POWERCELL] = 300;
					ent->client->ps.ammo[AMMO_METAL_BOLTS] = challenged->client->ps.ammo[AMMO_METAL_BOLTS] = 300;
					ent->client->ps.ammo[AMMO_ROCKETS] = challenged->client->ps.ammo[AMMO_ROCKETS] = 10;//hm
					ent->client->ps.ammo[AMMO_THERMAL] = challenged->client->ps.ammo[AMMO_THERMAL] = 10;
					ent->client->ps.ammo[AMMO_TRIPMINE] = challenged->client->ps.ammo[AMMO_TRIPMINE] = 10;
					ent->client->ps.ammo[AMMO_DETPACK] = challenged->client->ps.ammo[AMMO_DETPACK] = 10;
				}
				else if (weapon != WP_STUN_BATON) {
					ent->client->ps.ammo[weaponData[weapon].ammoIndex] = 999; //gun duel ammo
					challenged->client->ps.ammo[weaponData[weapon].ammoIndex] = 999; //gun duel ammo
				}

				ent->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
				challenged->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
				ent->client->ps.forceHandExtendTime = level.time + 2000;
				challenged->client->ps.forceHandExtendTime = level.time + 2000; //2 seconds of weaponlock at start of duel
			}

			Q_strncpyz(ent->client->pers.lastUserName, ent->client->pers.userName, sizeof(ent->client->pers.lastUserName));
			Q_strncpyz(challenged->client->pers.lastUserName, challenged->client->pers.userName, sizeof(challenged->client->pers.lastUserName));
			ent->client->pers.duelStartTime = level.time;
			challenged->client->pers.duelStartTime = level.time;
		}
		else
		{
			char weapStr[64];
			//Print the message that a player has been challenged in private, only announce the actual duel initiation in private
			switch (dueltype) {
			case 0:
				trap_SendServerCommand(challenged - g_entities, va("cp \"%s ^7%s\n^2(Saber Duel)\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGE")));
				trap_SendServerCommand(ent - g_entities, va("cp \"%s %s\n^2(Saber Duel)\n\"", G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGED"), challenged->client->pers.netname));
				break;
			case 1:
				trap_SendServerCommand(challenged - g_entities, va("cp \"%s ^7%s\n^4(Force Duel)\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGE")));
				trap_SendServerCommand(ent - g_entities, va("cp \"%s %s\n^4(Force Duel)\n\"", G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGED"), challenged->client->pers.netname));
				break;
			default:
				switch (dueltype - 1) {
				case 1:	Com_sprintf(weapStr, sizeof(weapStr), "Stun baton"); break;
				case 3:	Com_sprintf(weapStr, sizeof(weapStr), "Bryar"); break;
				case 4:	Com_sprintf(weapStr, sizeof(weapStr), "E11"); break;
				case 5:	Com_sprintf(weapStr, sizeof(weapStr), "Sniper"); break;
				case 6:	Com_sprintf(weapStr, sizeof(weapStr), "Bowcaster");	break;
				case 7:	Com_sprintf(weapStr, sizeof(weapStr), "Repeater"); break;
				case 8:	Com_sprintf(weapStr, sizeof(weapStr), "Demp2");	break;
				case 9: Com_sprintf(weapStr, sizeof(weapStr), "Flechette"); break;
				case 10: Com_sprintf(weapStr, sizeof(weapStr), "Rocket"); break;
				case 11: Com_sprintf(weapStr, sizeof(weapStr), "Thermal"); break;
				case 12: Com_sprintf(weapStr, sizeof(weapStr), "Tripmine"); break;
				case 13: Com_sprintf(weapStr, sizeof(weapStr), "Detpack"); break;
				default: Com_sprintf(weapStr, sizeof(weapStr), "Gun"); break;
				}
				trap_SendServerCommand(challenged - g_entities, va("cp \"%s ^7%s\n^4(%s Duel)\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGE"), weapStr));
				trap_SendServerCommand(ent - g_entities, va("cp \"%s %s\n^4(%s Duel)\n\"", G_GetStringEdString("MP_SVGAME", "PLDUELCHALLENGED"), challenged->client->pers.netname, weapStr));
				break;
			}
			//trap_SendServerCommand( challenged-g_entities, va("cp \"%s %s\n\"", ent->client->pers.netname, G_GetStripEdString("SVINGAME", "PLDUELCHALLENGE")) );
			//trap_SendServerCommand( ent-g_entities, va("cp \"%s %s\n\"", G_GetStripEdString("SVINGAME", "PLDUELCHALLENGED"), challenged->client->pers.netname) );
		}
//jk2PRO - Serverside - Fullforce Duels + Duel Messages - End

		challenged->client->ps.fd.privateDuelTime = 0; //reset the timer in case this player just got out of a duel. He should still be able to accept the challenge.

		//ent->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
		//ent->client->ps.forceHandExtendTime = level.time + 1000;

		ent->client->ps.duelIndex = challenged->s.number;
		ent->client->ps.duelTime = level.time + 2000;//Is this where the freeze comes from?
		if (dueltype == 0 || dueltype == 1)
			dueltypes[ent->client->ps.clientNum] = dueltype;
		else if (!ent->client->ps.duelInProgress)
			dueltypes[ent->client->ps.clientNum] = dueltype;
	}
}

//[videoP - jk2PRO - Serverside - All - Aminfo Function - Start]
/*
=================
Cmd_Aminfo_f
=================
*/
void Cmd_Aminfo_f(gentity_t *ent)
{
	char buf[MAX_STRING_CHARS - 64] = { 0 };

	if (!ent || !ent->client)
		return;

	//Q_strncpyz(buf, va("^5 Hi there, %s^5.  This server is using the jk2PRO mod.\n", ent->client->pers.netname), sizeof(buf));
	trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^5 Hi there, %s^5.  This server is using the jk2PRO mod.\n\n\"", ent->client->pers.netname));
	trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^3Have you tried out Full Force Duel Mode yet? Use /engage_fullforceduel while looking at your opponent\n\""));
	trap_SendServerCommand(ent->client->ps.clientNum, va("print \"^3to challenge them to a FF duel.  Also try out /engage_gunduel! You can even bind these commands!\n\""));
	return;
}
//[videoP - jk2PRO - Serverside - All - Aminfo Function - End]

//[videoP - jk2PRO - Serverside - All - Amlogin Function - Start]
/*
=================
Cmd_Amlogin_f
=================
*/
void Cmd_Amlogin_f(gentity_t *ent)
{
	char	pass[MAX_STRING_CHARS];

	trap_Argv(1, pass, sizeof(pass)); //Password

	if (!ent->client)
		return;

	if (trap_Argc() == 1)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Usage: amLogin <password>\n\"");
		return;
	}
	if (trap_Argc() == 2)
	{
		if (ent->client->sess.juniorAdmin || ent->client->sess.fullAdmin)
		{
			trap_SendServerCommand(ent - g_entities, "print \"You are already logged in. Type in /amLogout to remove admin status.\n\"");
			return;
		}
		if (!Q_stricmp(pass, ""))
		{
			trap_SendServerCommand(ent - g_entities, "print \"Usage: amLogin <password>\n\"");
			return;
		}
		if (!Q_stricmp(pass, g_fullAdminPass.string))
		{
			if (!Q_stricmp("", g_fullAdminPass.string))
				return;
			ent->client->sess.fullAdmin = qtrue;
			trap_SendServerCommand(ent - g_entities, "print \"^2You are now logged in with full admin privileges.\n\"");
			if (Q_stricmp(g_fullAdminMsg.string, ""))
				trap_SendServerCommand(-1, va("print \"%s ^7%s\n\"", ent->client->pers.netname, g_fullAdminMsg.string));
			return;
		}
		if (!Q_stricmp(pass, g_juniorAdminPass.string))
		{
			if (!Q_stricmp("", g_juniorAdminPass.string))
				return;
			ent->client->sess.juniorAdmin = qtrue;
			trap_SendServerCommand(ent - g_entities, "print \"^2You are now logged in with junior admin privileges.\n\"");
			if (Q_stricmp(g_juniorAdminMsg.string, ""))
				trap_SendServerCommand(-1, va("print \"%s ^7%s\n\"", ent->client->pers.netname, g_juniorAdminMsg.string));
			return;
		}
		else
		{
			trap_SendServerCommand(ent - g_entities, "print \"^3Failed to log in: Incorrect password!\n\"");
			return;
		}
	}
	else
	{
		trap_SendServerCommand(ent - g_entities, "print \"^3Failed to log in: Invalid argument count!\n\"");
		return;
	}
}
//[videoP - jk2PRO - Serverside - All - Amlogin Function - End]

//[videoP - jk2PRO - Serverside - All - Movement Function - Start]
int RaceNameToInteger(char *style);
static void Cmd_MovementStyle_f(gentity_t *ent)
{
	char mStyle[32];
	int style;

	if (!ent->client)
		return;

	if (trap_Argc() != 2) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /move <saga, jko, qw, cpm, q3, pjk, wsw, rjq3, or rjcpm>.\n\"");
		return;
	}

	if (!g_raceMode.integer) {
		trap_SendServerCommand(ent - g_entities, "print \"This command is not allowed in this gamemode!\n\"");
		return;
	}

	/*
	if (level.gametype != GT_FFA) {
	trap->SendServerCommand(ent-g_entities, "print \"This command is not allowed in this gametype!\n\"");
	return;
	}
	*/

	if (!ent->client->sess.raceMode) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in racemode to use this command!\n\"");
		return;
	}

	if (VectorLength(ent->client->ps.velocity)) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be standing still to use this command!\n\"");
		return;
	}

	trap_Argv(1, mStyle, sizeof(mStyle));

	style = RaceNameToInteger(mStyle);

	if (style >= 0 && style <= 8) {
		ent->client->sess.movementStyle = style;
		AmTeleportPlayer(ent, ent->client->ps.origin, ent->client->ps.viewangles, qtrue, qtrue); //Good

		if (style == 7 || style == 8) {
			ent->client->ps.stats[STAT_WEAPONS] =  (1 << WP_SABER) + (1 << WP_ROCKET_LAUNCHER);
		}
		else {
			ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_SABER) + (1 << WP_DISRUPTOR);
			ent->client->ps.ammo[AMMO_ROCKETS] = 0;
		}

		if (ent->client->pers.stats.startTime || ent->client->pers.stats.startTimeFlag) {
			trap_SendServerCommand(ent - g_entities, "print \"Movement style updated: timer reset.\n\"");
			ResetPlayerTimers(ent, qtrue);
		}
		else
			trap_SendServerCommand(ent - g_entities, "print \"Movement style updated.\n\"");
	}
	else
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /move <siege, jka, qw, cpm, q3, pjk, wsw, rjq3, rjcpm, swoop, or jetpack>.\n\"");
}

static void Cmd_JumpChange_f(gentity_t *ent)
{
	char jLevel[32];
	int level;

	if (!ent->client)
		return;

	if (!ent->client->sess.raceMode) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be in racemode to use this command!\n\"");
		return;
	}

	if (trap_Argc() != 2) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /jump <level>\n\"");
		return;
	}

	if (VectorLength(ent->client->ps.velocity)) {
		trap_SendServerCommand(ent - g_entities, "print \"You must be standing still to use this command!\n\"");
		return;
	}

	trap_Argv(1, jLevel, sizeof(jLevel));
	level = atoi(jLevel);

	if (level > 0 && level < 4) {
		ent->client->ps.fd.forcePowerLevel[FP_LEVITATION] = level;
		AmTeleportPlayer(ent, ent->client->ps.origin, ent->client->ps.viewangles, qtrue, qtrue); //Good
		if (ent->client->pers.stats.startTime || ent->client->pers.stats.startTimeFlag) {
			trap_SendServerCommand(ent - g_entities, va("print \"Jumplevel updated (%i): timer reset.\n\"", level));
			ResetPlayerTimers(ent, qtrue);
		}
		else
			trap_SendServerCommand(ent - g_entities, va("print \"Jumplevel updated (%i).\n\"", level));
	}
	else
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /jump <level>\n\"");
}
//[videoP - jk2PRO - Serverside - All - Movement & Jump Command Functions - End]

//[videoP - jk2PRO - Serverside - All - RaceTele Function - Start]
void Cmd_RaceTele_f(gentity_t *ent)
{
	if (trap_Argc() > 2 && trap_Argc() != 4)
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /amTele to teleport to your telemark or /amTele <client> or /amtele <X Y Z>\n\"");
	if (trap_Argc() == 1) {//Amtele to telemark
		if (ent->client->pers.telemarkOrigin[0] != 0 || ent->client->pers.telemarkOrigin[1] != 0 || ent->client->pers.telemarkOrigin[2] != 0 || ent->client->pers.telemarkAngle != 0) {
			vec3_t	angles = { 0, 0, 0 };
			angles[YAW] = ent->client->pers.telemarkAngle;
			angles[PITCH] = ent->client->pers.telemarkPitchAngle;
			AmTeleportPlayer(ent, ent->client->pers.telemarkOrigin, angles, qtrue, qtrue);
		}
		else
			trap_SendServerCommand(ent - g_entities, "print \"No telemark set!\n\"");
	}

	if (trap_Argc() == 2)//Amtele to player
	{
		char client[MAX_NETNAME];
		int clientid = -1;
		vec3_t	angles = { 0, 0, 0 }, origin;

		trap_Argv(1, client, sizeof(client));
		clientid = JP_ClientNumberFromString(ent, client);

		if (clientid == -1 || clientid == -2)
			return;

		origin[0] = g_entities[clientid].client->ps.origin[0];
		origin[1] = g_entities[clientid].client->ps.origin[1];
		origin[2] = g_entities[clientid].client->ps.origin[2] + 96;

		AmTeleportPlayer(ent, origin, angles, qtrue, qtrue);
	}

	if (trap_Argc() == 4)
	{
		char x[32], y[32], z[32];
		vec3_t angles = { 0, 0, 0 }, origin;

		trap_Argv(1, x, sizeof(x));
		trap_Argv(2, y, sizeof(y));
		trap_Argv(3, z, sizeof(z));

		origin[0] = atoi(x);
		origin[1] = atoi(y);
		origin[2] = atoi(z);

		AmTeleportPlayer(ent, origin, angles, qtrue, qtrue);
	}
}
//[videoP - jk2PRO - Serverside - All - RaceTele Function - End]

//[videoP - jk2PRO - Serverside - All - Amtele Function - Start]
/*
=================
Cmd_Amtele_f
=================
*/
void Cmd_Amtele_f(gentity_t *ent)
{
	gentity_t	*teleporter;
	char client1[MAX_NETNAME], client2[MAX_NETNAME];
	char x[32], y[32], z[32], yaw[32];
	int clientid1 = -1, clientid2 = -1;
	vec3_t	angles = { 0, 0, 0 }, origin;
	qboolean droptofloor = qfalse, race = qfalse;

	if (!ent->client)
		return;

	if (ent->client->sess.fullAdmin)
	{
		if (!(g_fullAdminLevel.integer & (1 << A_ADMINTELE)))
		{
			if (ent->client->sess.raceMode && g_allowRaceTele.integer)
				Cmd_RaceTele_f(ent);
			else if (g_raceMode.integer && g_allowRaceTele.integer)
				trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command (amTele) outside of racemode.\n\"");
			else
				trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command (amTele).\n\"");
			return;
		}
	}
	else if (ent->client->sess.juniorAdmin)
	{
		if (!(g_juniorAdminLevel.integer & (1 << A_ADMINTELE)))
		{
			if (ent->client->sess.raceMode && g_allowRaceTele.integer)
				Cmd_RaceTele_f(ent);
			else if (g_raceMode.integer && g_allowRaceTele.integer)
				trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command (amTele) outside of racemode.\n\"");
			else
				trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command (amTele).\n\"");
			return;
		}
	}
	else//not logged in
	{
		if (ent->client->sess.raceMode && g_allowRaceTele.integer)
			Cmd_RaceTele_f(ent);
		else if (g_raceMode.integer && g_allowRaceTele.integer)
			trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command (amTele) outside of racemode.\n\"");
		else
			trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command (amTele).\n\"");
		return;
	}

	if (ent->client->sess.raceMode) {
		droptofloor = qtrue;
		race = qtrue;
	}

	if (trap_Argc() > 6)
	{
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /amTele or /amTele <client> or /amTele <client> <client> or /amTele <X> <Y> <Z> <YAW> or /amTele <player> <X> <Y> <Z> <YAW>.\n\"");
		return;
	}

	if (trap_Argc() == 1)//Amtele to telemark
	{
		if (ent->client->pers.telemarkOrigin[0] != 0 || ent->client->pers.telemarkOrigin[1] != 0 || ent->client->pers.telemarkOrigin[2] != 0 || ent->client->pers.telemarkAngle != 0)
		{
			angles[YAW] = ent->client->pers.telemarkAngle;
			angles[PITCH] = ent->client->pers.telemarkPitchAngle;
			AmTeleportPlayer(ent, ent->client->pers.telemarkOrigin, angles, droptofloor, race);
		}
		else
			trap_SendServerCommand(ent - g_entities, "print \"No telemark set!\n\"");
		return;
	}

	if (trap_Argc() == 2)//Amtele to player
	{
		trap_Argv(1, client1, sizeof(client1));
		clientid1 = JP_ClientNumberFromString(ent, client1);

		if (clientid1 == -1 || clientid1 == -2)
			return;

		origin[0] = g_entities[clientid1].client->ps.origin[0];
		origin[1] = g_entities[clientid1].client->ps.origin[1];
		origin[2] = g_entities[clientid1].client->ps.origin[2] + 96;
		AmTeleportPlayer(ent, origin, angles, droptofloor, race);
		return;
	}

	if (trap_Argc() == 3)//Amtele player to player
	{
		trap_Argv(1, client1, sizeof(client1));
		trap_Argv(2, client2, sizeof(client2));
		clientid1 = JP_ClientNumberFromString(ent, client1);
		clientid2 = JP_ClientNumberFromString(ent, client2);

		if (clientid1 == -1 || clientid1 == -2 || clientid2 == -1 || clientid2 == -2)
			return;

		if (g_entities[clientid1].client && (g_entities[clientid1].client->sess.fullAdmin) || (ent->client->sess.juniorAdmin && g_entities[clientid1].client->sess.juniorAdmin))//He has admin
		{
			if (g_entities[clientid1].client->ps.clientNum != ent->client->ps.clientNum)//Hes not me
			{
				trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command on this player (amTele).\n\"");
				return;
			}
		}

		teleporter = &g_entities[clientid1];

		origin[0] = g_entities[clientid2].client->ps.origin[0];
		origin[1] = g_entities[clientid2].client->ps.origin[1];
		origin[2] = g_entities[clientid2].client->ps.origin[2] + 96;

		AmTeleportPlayer(teleporter, origin, angles, droptofloor, qfalse);
		return;
	}

	if (trap_Argc() == 4)//|| trap->Argc() == 5)//Amtele to origin (if no angle specified, default 0?)
	{
		trap_Argv(1, x, sizeof(x));
		trap_Argv(2, y, sizeof(y));
		trap_Argv(3, z, sizeof(z));

		origin[0] = atoi(x);
		origin[1] = atoi(y);
		origin[2] = atoi(z);

		/*if (trap->Argc() == 5)
		{
		trap->Argv(4, yaw, sizeof(yaw));
		angles[YAW] = atoi(yaw);
		}*/

		AmTeleportPlayer(ent, origin, angles, droptofloor, race);
		return;
	}

	if (trap_Argc() == 5)//Amtele to angles + origin, OR Amtele player to origin
	{
		trap_Argv(1, client1, sizeof(client1));
		clientid1 = JP_ClientNumberFromString(ent, client1);

		if (clientid1 == -1 || clientid1 == -2)//Amtele to origin + angles
		{
			trap_Argv(1, x, sizeof(x));
			trap_Argv(2, y, sizeof(y));
			trap_Argv(3, z, sizeof(z));

			origin[0] = atoi(x);
			origin[1] = atoi(y);
			origin[2] = atoi(z);

			trap_Argv(4, yaw, sizeof(yaw));
			angles[YAW] = atoi(yaw);

			AmTeleportPlayer(ent, origin, angles, droptofloor, race);
		}
		else//Amtele other player to origin
		{
			if (g_entities[clientid1].client && (g_entities[clientid1].client->sess.fullAdmin) || (ent->client->sess.juniorAdmin && g_entities[clientid1].client->sess.juniorAdmin))//He has admin
			{
				if (g_entities[clientid1].client->ps.clientNum != ent->client->ps.clientNum)//Hes not me
				{
					trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command on this player (amTele).\n\"");
					return;
				}
			}

			teleporter = &g_entities[clientid1];

			trap_Argv(2, x, sizeof(x));
			trap_Argv(3, y, sizeof(y));
			trap_Argv(4, z, sizeof(z));

			origin[0] = atoi(x);
			origin[1] = atoi(y);
			origin[2] = atoi(z);

			AmTeleportPlayer(teleporter, origin, angles, droptofloor, qfalse);
		}
		return;
	}

	if (trap_Argc() == 6)//Amtele player to angles + origin
	{
		trap_Argv(1, client1, sizeof(client1));
		clientid1 = JP_ClientNumberFromString(ent, client1);

		if (clientid1 == -1 || clientid1 == -2)
			return;

		if (g_entities[clientid1].client && (g_entities[clientid1].client->sess.fullAdmin) || (ent->client->sess.juniorAdmin && g_entities[clientid1].client->sess.juniorAdmin))//He has admin
		{
			if (g_entities[clientid1].client->ps.clientNum != ent->client->ps.clientNum)//Hes not me
			{
				trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command on this player (amTele).\n\"");
				return;
			}
		}

		teleporter = &g_entities[clientid1];

		trap_Argv(2, x, sizeof(x));
		trap_Argv(3, y, sizeof(y));
		trap_Argv(4, z, sizeof(z));

		origin[0] = atoi(x);
		origin[1] = atoi(y);
		origin[2] = atoi(z);

		trap_Argv(5, yaw, sizeof(yaw));
		angles[YAW] = atoi(yaw);

		AmTeleportPlayer(teleporter, origin, angles, droptofloor, qfalse);
		return;
	}

}
//[videoP - jk2PRO - Serverside - All - Amtele Function - End]

//[videoP - jk2PRO - Serverside - All - Amtelemark Function - Start]
/*
=================
Cmd_Amtelemark_f
=================
*/
void Cmd_Amtelemark_f(gentity_t *ent)
{
	if (!ent->client)
		return;

	if (ent->client && ent->client->ps.duelInProgress && ent->client->pers.lastUserName[0]) {
		gentity_t *duelAgainst = &g_entities[ent->client->ps.duelIndex];
		if (duelAgainst->client && duelAgainst->client->pers.lastUserName[0]) {
			trap_SendServerCommand(ent - g_entities, va("print \"You are not authorized to use this command (amtele) in ranked duels.\n\""));
			return; //Dont allow amtele in ranked duels ever..
		}
	}

	if (ent->client->sess.fullAdmin) {//Logged in as full admin
		if (!(g_fullAdminLevel.integer & (1 << A_TELEMARK))) {
			if (!ent->client->sess.raceMode && g_raceMode.integer && g_allowRaceTele.integer) {
				trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command (amTelemark) outside of racemode.\n\"");
				return;
			}
			else if (ent->client->sess.raceMode && !g_allowRaceTele.integer) {
				trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command (amTelemark).\n\"");
				return;
			}
		}
	}
	else if (ent->client->sess.juniorAdmin) {//Logged in as junior admin
		if (!(g_juniorAdminLevel.integer & (1 << A_TELEMARK))) {
			if (!ent->client->sess.raceMode && g_raceMode.integer && g_allowRaceTele.integer) {
				trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command (amTelemark) outside of racemode.\n\"");
				return;
			}
			else if (ent->client->sess.raceMode && !g_allowRaceTele.integer) {
				trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command (amTelemark).\n\"");
				return;
			}
		}
	}
	else {//Not logged in
		if (!ent->client->sess.raceMode && g_raceMode.integer && g_allowRaceTele.integer) {
			trap_SendServerCommand(ent - g_entities, "print \"You are not authorized to use this command (amTelemark) outside of racemode.\n\"");
			return;
		}
		else if (!g_allowRaceTele.integer || !g_raceMode.integer) {
			trap_SendServerCommand(ent - g_entities, "print \"You must be logged in to use this command (amTelemark).\n\"");
			return;
		}
	}

	VectorCopy(ent->client->ps.origin, ent->client->pers.telemarkOrigin);
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR && (ent->client->ps.pm_flags & PMF_FOLLOW))
		ent->client->pers.telemarkOrigin[2] += 58;
	ent->client->pers.telemarkAngle = ent->client->ps.viewangles[YAW];
	ent->client->pers.telemarkPitchAngle = ent->client->ps.viewangles[PITCH];
	trap_SendServerCommand(ent - g_entities, va("print \"Teleport Marker: ^3<%i, %i, %i> %i, %i\n\"",
		(int)ent->client->pers.telemarkOrigin[0], (int)ent->client->pers.telemarkOrigin[1], (int)ent->client->pers.telemarkOrigin[2], (int)ent->client->pers.telemarkAngle, (int)ent->client->pers.telemarkPitchAngle));
}
//[videoP - jk2PRO - Serverside - All - Amtelemark Function - End]

//[videoP - jk2PRO - Serverside - All - Race Function - Start]
/*
=================
Cmd_Race_f
=================
*/
void Cmd_Race_f(gentity_t *ent)
{
	if (!ent->client)
		return;

	if (ent->client->ps.powerups[PW_NEUTRALFLAG] || ent->client->ps.powerups[PW_REDFLAG] || ent->client->ps.powerups[PW_BLUEFLAG])
		return;

	if (g_raceMode.integer < 2) {
		trap_SendServerCommand(ent - g_entities, "print \"^5This command is not allowed!\n\"");
		ent->client->sess.raceMode = qfalse;
		return;
	}

	if (g_gametype.integer != GT_FFA) {
		trap_SendServerCommand(ent - g_entities, "print \"^5This command is not allowed in this gametype!\n\"");
		ent->client->sess.raceMode = qfalse;
		return;
	}

	if (ent->client->sess.raceMode) {//Toggle it
		ent->client->sess.raceMode = qfalse;
		ent->client->pers.noFollow = qfalse;
		ent->client->pers.practice = qfalse;
		ent->r.svFlags &= ~SVF_SINGLECLIENT; //ehh?
		ent->s.weapon = WP_SABER; //Don't drop our weapon
		Cmd_ForceChanged_f(ent);//Make sure their jump level is valid.. if leaving racemode :S
		trap_SendServerCommand(ent - g_entities, "print \"^5Race mode toggled off.\n\"");
	}
	else {
		ent->client->sess.raceMode = qtrue;
		trap_SendServerCommand(ent - g_entities, "print \"^5Race mode toggled on.\n\"");
	}

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		G_Kill(ent); //stop aboooose!
		ent->client->ps.persistant[PERS_SCORE] = 0;
		ent->client->ps.persistant[PERS_KILLED] = 0;
		ent->client->pers.enterTime = level.time; //reset scoreboard kills/deaths i guess... and time?
	}
	return;
}

/*
=================
Cmd_Amlogout_f
=================
*/
void Cmd_Amlogout_f(gentity_t *ent)
{
	if (!ent->client)
		return;
	if (ent->client->sess.fullAdmin || ent->client->sess.juniorAdmin)
	{
		ent->client->sess.fullAdmin = qfalse;
		ent->client->sess.juniorAdmin = qfalse;
		trap_SendServerCommand(ent - g_entities, "print \"You are no longer an admin.\n\"");
	}
	else
		return;
}
//[JK2PRO - Serverside - All - Amlogout Function - End]

/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum ) {
	gentity_t *ent;
	char	cmd[MAX_TOKEN_CHARS];

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;		// not fully in game yet
	}


	trap_Argv( 0, cmd, sizeof( cmd ) );

	if (Q_stricmp(cmd, "aminfo") == 0) {
		Cmd_Aminfo_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "amlogin") == 0) {
		Cmd_Amlogin_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "amlogout") == 0) {
		Cmd_Amlogout_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "amtele") == 0) {
		if (level.intermissiontime)
		{
			trap_SendServerCommand(clientNum, va("print \"You cannot perform this task (%s) during the intermission.\n\"", cmd));
			return;
		}
		Cmd_Amtele_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "amtelemark") == 0) {
		if (level.intermissiontime)
		{
			trap_SendServerCommand(clientNum, va("print \"You cannot perform this task (%s) during the intermission.\n\"", cmd));
			return;
		}
		Cmd_Amtelemark_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "engage_fullforceduel") == 0) {
		if (level.intermissiontime)
			return;
		Cmd_ForceDuel_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "engage_gunduel") == 0) {
		if (level.intermissiontime)
			return;
		Cmd_GunDuel_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "race") == 0) {
		Cmd_Race_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "move") == 0) {
		Cmd_MovementStyle_f(ent);
		return;
	}

	if (Q_stricmp(cmd, "jump") == 0) {
		Cmd_JumpChange_f(ent);
		return;
	}

	//rww - redirect bot commands
	if (strstr(cmd, "bot_") && AcceptBotCommand(cmd, ent))
	{
		return;
	}
	//end rww

	if (Q_stricmp (cmd, "say") == 0) {
		//Cmd_Say_f (ent, SAY_ALL, qfalse);
		Cmd_Say_f(ent);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0) {
		//Cmd_Say_f (ent, SAY_TEAM, qfalse);
		Cmd_SayTeam_f(ent);
		return;
	}
	if (Q_stricmp (cmd, "tell") == 0) {
		Cmd_Tell_f ( ent );
		return;
	}
	//DM - jk2PRO - Disable all voice commands, why tf do they exist
	/*if (Q_stricmp (cmd, "vsay") == 0) {
		Cmd_Voice_f (ent, SAY_ALL, qfalse, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "vsay_team") == 0) {
		Cmd_Voice_f (ent, SAY_TEAM, qfalse, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "vtell") == 0) {
		Cmd_VoiceTell_f ( ent, qfalse );
		return;
	}
	if (Q_stricmp (cmd, "vosay") == 0) {
		Cmd_Voice_f (ent, SAY_ALL, qfalse, qtrue);
		return;
	}
	if (Q_stricmp (cmd, "vosay_team") == 0) {
		Cmd_Voice_f (ent, SAY_TEAM, qfalse, qtrue);
		return;
	}
	if (Q_stricmp (cmd, "votell") == 0) {
		Cmd_VoiceTell_f ( ent, qtrue );
		return;
	}
	if (Q_stricmp (cmd, "vtaunt") == 0) {
		Cmd_VoiceTaunt_f ( ent );
		return;
	}*/
	if (Q_stricmp(cmd, "score") == 0) {
		Cmd_Score_f(ent);
		return;
	}

	// ignore all other commands when at intermission
	if (level.intermissiontime)
	{
		qboolean giveError = qfalse;

		if (!Q_stricmp(cmd, "give"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "god"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "notarget"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "noclip"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "kill"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "teamtask"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "levelshot"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "follow"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "follownext"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "followprev"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "team"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "forcechanged"))
		{ //special case: still update force change
			Cmd_ForceChanged_f (ent);
			return;
		}
		else if (!Q_stricmp(cmd, "where"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "callvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "vote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "callteamvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "teamvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "gc"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "setviewpos"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "stats"))
		{
			giveError = qtrue;
		}

		if (giveError)
		{
			trap_SendServerCommand( clientNum, va("print \"You cannot perform this task (%s) during the intermission.\n\"", cmd ) );
		}
		else
		{
			//Cmd_Say_f (ent, qfalse, qtrue);
			Cmd_Say_f(ent);
		}
		return;
	}

	if (Q_stricmp(cmd, "give") == 0)
	{
		Cmd_Give_f(ent);
	}
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "teamtask") == 0)
		Cmd_TeamTask_f (ent);
	else if (Q_stricmp (cmd, "levelshot") == 0)
		Cmd_LevelShot_f (ent);
	else if (Q_stricmp (cmd, "follow") == 0)
		Cmd_Follow_f (ent);
	else if (Q_stricmp (cmd, "follownext") == 0)
		Cmd_FollowCycle_f (ent, 1);
	else if (Q_stricmp (cmd, "followprev") == 0)
		Cmd_FollowCycle_f (ent, -1);
	else if (Q_stricmp (cmd, "team") == 0)
		Cmd_Team_f (ent);
	else if (Q_stricmp (cmd, "forcechanged") == 0)
		Cmd_ForceChanged_f (ent);
	else if (Q_stricmp (cmd, "where") == 0)
		Cmd_Where_f (ent);
	else if (Q_stricmp (cmd, "callvote") == 0)
		Cmd_CallVote_f (ent);
	else if (Q_stricmp (cmd, "vote") == 0)
		Cmd_Vote_f (ent);
	else if (Q_stricmp (cmd, "callteamvote") == 0)
		Cmd_CallTeamVote_f (ent);
	else if (Q_stricmp (cmd, "teamvote") == 0)
		Cmd_TeamVote_f (ent);
	else if (Q_stricmp (cmd, "gc") == 0)
		Cmd_GameCommand_f( ent );
	else if (Q_stricmp (cmd, "setviewpos") == 0)
		Cmd_SetViewpos_f( ent );
	else if (Q_stricmp (cmd, "stats") == 0)
		Cmd_Stats_f( ent );
	else if (Q_stricmp(cmd, "#mm") == 0 && CheatsOk( ent ))
	{
		G_PlayerBecomeATST(ent);
	}
	else if (Q_stricmp(cmd, "thedestroyer") == 0 && CheatsOk( ent ) && ent && ent->client && ent->client->ps.saberHolstered && ent->client->ps.weapon == WP_SABER)
	{
		Cmd_ToggleSaber_f(ent);

		if (!ent->client->ps.saberHolstered)
		{
			if (ent->client->ps.dualBlade)
			{
				ent->client->ps.dualBlade = qfalse;
				//ent->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
			}
			else
			{
				ent->client->ps.dualBlade = qtrue;

				trap_SendServerCommand( -1, va("print \"%sTHE DESTROYER COMETH\n\"", S_COLOR_RED) );
				G_ScreenShake(vec3_origin, NULL, 10.0f, 800, qtrue);
				//ent->client->ps.fd.saberAnimLevel = FORCE_LEVEL_3;
			}
		}
	}
#ifdef _DEBUG
	else if (Q_stricmp(cmd, "gotocoord") == 0 && CheatsOk( ent ))
	{
		char		x[64], y[64], z[64];
		vec3_t		xyz;

		if (trap_Argc() < 3)
		{
			return;
		}

		trap_Argv( 1, x, sizeof( x ) );
		trap_Argv( 2, y, sizeof( y ) );
		trap_Argv( 3, z, sizeof( z ) );

		xyz[0] = atof(x);
		xyz[1] = atof(y);
		xyz[2] = atof(z);

		VectorCopy(xyz, ent->client->ps.origin);
	}
#endif

	/*
	else if (Q_stricmp (cmd, "offwithmyhead") == 0)
	{
		DismembermentTest(ent);
	}
	*/
	else
	{
		if (Q_stricmp(cmd, "addbot") == 0)
		{ //because addbot isn't a recognized command unless you're the server, but it is in the menus regardless
			trap_SendServerCommand( clientNum, va("print \"You can only add bots as the server.\n\"" ) );
		}
		else
		{
			trap_SendServerCommand( clientNum, va("print \"unknown cmd %s\n\"", cmd ) );
		}
	}
}
