// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

int gTrigFallSound;

void InitTrigger( gentity_t *self ) {
	if (!VectorCompare (self->s.angles, vec3_origin))
		G_SetMovedir (self->s.angles, self->movedir);

	trap_SetBrushModel( self, self->model );
	self->r.contents = CONTENTS_TRIGGER;		// replaces the -1 from trap_SetBrushModel
	self->r.svFlags = SVF_NOCLIENT;
}

// the wait time has passed, so set back up for another activation
void multi_wait( gentity_t *ent ) {
	ent->nextthink = 0;
}


// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void multi_trigger( gentity_t *ent, gentity_t *activator ) {
	gentity_t *rofftarget = NULL, *testent = NULL;
	gentity_t *te;
	int i = MAX_CLIENTS;

	if (ent->teamnodmg &&
		activator && activator->client &&
		ent->teamnodmg == activator->client->sess.sessionTeam &&
		!g_ff_objectives.integer)
	{
		return;
	}

	if (ent->spawnflags & 1)
	{
		if (!activator->client)
		{
			return;
		}

		if (!(activator->client->pers.cmd.buttons & BUTTON_USE))
		{
			return;
		}
	}

	ent->activator = activator;
	if ( ent->nextthink ) {
		return;		// can't retrigger until the wait is over
	}

	if ( activator->client ) {
		if ( ( ent->spawnflags & 2 ) &&
			activator->client->sess.sessionTeam != TEAM_RED ) {
			return;
		}
		if ( ( ent->spawnflags & 4 ) &&
			activator->client->sess.sessionTeam != TEAM_BLUE ) {
			return;
		}
	}

	G_UseTargets (ent, ent->activator);

	if (ent->roffname && ent->roffid != -1)
	{
		if (ent->rofftarget)
		{
			while (i < MAX_GENTITIES)
			{
				testent = &g_entities[i];

				if (testent && testent->targetname && strcmp(testent->targetname, ent->rofftarget) == 0)
				{
					rofftarget = testent;
					break;
				}
				i++;
			}
		}
		else
		{
			rofftarget = activator;
		}

		if (rofftarget)
		{
			trap_ROFF_Play(rofftarget->s.number, ent->roffid, qfalse);

			//Play it at the same time on the client, so that we can catch client-side notetrack events and not have to send
			//them over from the server (this wouldn't work for things like effects due to lack of ability to precache them
			//on the server)

			//remember the entity's original position in case of a server-side "loop" notetrack
			VectorCopy(rofftarget->s.pos.trBase, rofftarget->s.origin2);
			VectorCopy(rofftarget->s.apos.trBase, rofftarget->s.angles2);

			te = G_TempEntity( rofftarget->s.pos.trBase, EV_PLAY_ROFF );
			te->s.eventParm = ent->roffid;
			te->s.weapon = rofftarget->s.number;
			te->s.trickedentindex = 0;

			//But.. this may not produce desired results for clients who connect while a ROFF is playing.

			rofftarget->roffid = ent->roffid; //let this entity know the most recent ROFF played on him
		}
	}

	if ( ent->wait > 0 ) {
		ent->think = multi_wait;
		ent->nextthink = level.time + ( ent->wait + ent->random * crandom() ) * 1000;
	} else {
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent->touch = 0;
		ent->nextthink = level.time + FRAMETIME;
		ent->think = G_FreeEntity;
	}
}

void Use_Multi( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	multi_trigger( ent, activator );
}

void Touch_Multi( gentity_t *self, gentity_t *other, trace_t *trace ) {
	if( !other->client ) {
		return;
	}
	multi_trigger( self, other );
}

/*QUAKED trigger_multiple (.5 .5 .5) ? USE_BUTTON RED_ONLY BLUE_ONLY
USE_BUTTON - Won't fire unless player is in it and pressing use button (in addition to any other conditions)
RED_ONLY - Only red team can use
BLUE_ONLY - Only blue team can use

"roffname"		If set, will play a roff upon activation
"rofftarget"	If set with roffname, will activate the roff an entity with
				this as its targetname. Otherwise uses roff on activating entity.
"wait" : Seconds between triggerings, 0.5 default, -1 = one time only.
"random"	wait variance, default is 0
Variable sized repeatable trigger.  Must be targeted at one or more entities.
so, the basic time between firing is a random time between
(wait - random) and (wait + random)
*/
void SP_trigger_multiple( gentity_t *ent ) {
	if (ent->roffname)
	{
		ent->roffid = trap_ROFF_Cache(ent->roffname);
	}
	else
	{
		ent->roffid = -1;
	}

	G_SpawnFloat( "wait", "0.5", &ent->wait );
	G_SpawnFloat( "random", "0", &ent->random );

	if ( ent->random >= ent->wait && ent->wait >= 0 ) {
		ent->random = ent->wait - FRAMETIME;
		G_Printf( "trigger_multiple has random >= wait\n" );
	}

	ent->touch = Touch_Multi;
	ent->use = Use_Multi;

	InitTrigger( ent );
	trap_LinkEntity (ent);
}



/*
==============================================================================

trigger_always

==============================================================================
*/

void trigger_always_think( gentity_t *ent ) {
	G_UseTargets(ent, ent);
	G_FreeEntity( ent );
}

/*QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
void SP_trigger_always (gentity_t *ent) {
	// we must have some delay to make sure our use targets are present
	ent->nextthink = level.time + 300;
	ent->think = trigger_always_think;
}


/*
==============================================================================

trigger_push

==============================================================================
*/

void trigger_push_touch (gentity_t *self, gentity_t *other, trace_t *trace ) {

	if ( !other->client ) {
		return;
	}

	BG_TouchJumpPad( &other->client->ps, &self->s );
}


/*
=================
AimAtTarget

Calculate origin2 so the target apogee will be hit
=================
*/
void AimAtTarget( gentity_t *self ) {
	gentity_t	*ent;
	vec3_t		origin;
	float		height, gravity, time, forward;
	float		dist;

	VectorAdd( self->r.absmin, self->r.absmax, origin );
	VectorScale ( origin, 0.5, origin );

	ent = G_PickTarget( self->target );
	if ( !ent ) {
		G_FreeEntity( self );
		return;
	}

	height = ent->s.origin[2] - origin[2];
	gravity = g_gravity.value;
	time = sqrt( height / ( .5 * gravity ) );
	if ( !time ) {
		G_FreeEntity( self );
		return;
	}

	// set s.origin2 to the push velocity
	VectorSubtract ( ent->s.origin, origin, self->s.origin2 );
	self->s.origin2[2] = 0;
	dist = VectorNormalize( self->s.origin2);

	forward = dist / time;
	VectorScale( self->s.origin2, forward, self->s.origin2 );

	self->s.origin2[2] = time * gravity;
}


/*QUAKED trigger_push (.5 .5 .5) ?
Must point at a target_position, which will be the apex of the leap.
This will be client side predicted, unlike target_push
*/
void SP_trigger_push( gentity_t *self ) {
	InitTrigger (self);

	// unlike other triggers, we need to send this one to the client
	self->r.svFlags &= ~SVF_NOCLIENT;

	// make sure the client precaches this sound
	G_SoundIndex("sound/weapons/force/jump.wav");

	self->s.eType = ET_PUSH_TRIGGER;
	self->touch = trigger_push_touch;
	self->think = AimAtTarget;
	self->nextthink = level.time + FRAMETIME;
	trap_LinkEntity (self);
}

void Use_target_push( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	if ( !activator->client ) {
		return;
	}

	if ( activator->client->ps.pm_type != PM_NORMAL && activator->client->ps.pm_type != PM_FLOAT ) {
		return;
	}

	VectorCopy (self->s.origin2, activator->client->ps.velocity);

	// play fly sound every 1.5 seconds
	if ( activator->fly_sound_debounce_time < level.time ) {
		activator->fly_sound_debounce_time = level.time + 1500;
		G_Sound( activator, CHAN_AUTO, self->noise_index );
	}
}

qboolean ValidRaceSettings(int restrictions, gentity_t *player)
{ //How 2 check if cvars were valid the whole time of run.. and before? since you can get a headstart with higher g_speed before hitting start timer? :S
	//Make most of this hardcoded into racemode..? speed, knockback, debugmelee, stepslidefix, gravity
	int style;
	if (!player->client)
		return qfalse;
	if (!player->client->ps.stats[STAT_RACEMODE])
		return qfalse;

	style = player->client->sess.movementStyle;

	if (((style == MV_RJQ3) || (style == MV_RJCPM)) && g_knockback.value != 1000.0f)
		return qfalse;
	if (style != MV_CPM && style != MV_Q3 && style != MV_WSW && style != MV_RJQ3 && style != MV_RJCPM) { //Ignore forcejump restrictions if in onlybhop movement modes
		if (restrictions & (1 << 0)) {//flags 1 = restrict to jump1
			if (player->client->ps.fd.forcePowerLevel[FP_LEVITATION] != 1) {
				trap_SendServerCommand(player - g_entities, "cp \"^3Warning: this course requires force jump level 1!\n\n\n\n\n\n\n\n\n\n\"");
				return qfalse;
			}
		}
		else if (restrictions & (1 << 1)) {//flags 2 = restrict to jump2
			if (player->client->ps.fd.forcePowerLevel[FP_LEVITATION] != 2) {
				trap_SendServerCommand(player - g_entities, "cp \"^3Warning: this course requires force jump level 2!\n\n\n\n\n\n\n\n\n\n\"");
				return qfalse;
			}
		}
		else if (restrictions & (1 << 2)) {//flags 4 = only jump3
			if (player->client->ps.fd.forcePowerLevel[FP_LEVITATION] != 3) {
				trap_SendServerCommand(player - g_entities, "cp \"^3Warning: this course requires force jump level 3!\n\n\n\n\n\n\n\n\n\n\"");
				return qfalse;
			}
		}
	}
	//if (player->client->pers.haste && !(restrictions & (1 << 3)))
	//	return qfalse; //IF client has haste, and the course does not allow haste, dont count it.

	//if (sv_cheats.integer)
	//	return qfalse;
	if (!g_smoothClients.integer)
		return qfalse;
	//if (sv_fps.integer != 20 && sv_fps.integer != 30 && sv_fps.integer != 40)//Dosnt really make a difference.. but eh.... loda fixme
	//	return qfalse;
	//if (sv_pluginKey.integer > 0) {//RESTRICT PLUGIN
		//if (!player->client->pers.validPlugin && player->client->pers.userName[0]) { //Meh.. only do this if they are logged in to keep the print colors working right i guess..
		//	trap->SendServerCommand(player - g_entities, "cp \"^3Warning: a newer client plugin version\nis required!\n\n\n\n\n\n\n\n\n\n\""); //Since times wont be saved if they arnt logged in anyway
		//	return qfalse;
		//}
	//}
	if (player->client->pers.noFollow)
		return qfalse;
	if (player->client->pers.practice)
		return qfalse;
	if ((restrictions & (1 << 5)) && (level.gametype == GT_CTF || level.gametype == GT_CTY))//spawnflags 32 is FFA_ONLY
		return qfalse;

	return qtrue;
}

qboolean InTrigger(vec3_t interpOrigin, gentity_t *trigger)
{
	vec3_t		mins, maxs;
	static const vec3_t	pmins = { -15, -15, DEFAULT_MINS_2 };
	static const vec3_t	pmaxs = { 15, 15, DEFAULT_MAXS_2 };

	VectorAdd(interpOrigin, pmins, mins);
	VectorAdd(interpOrigin, pmaxs, maxs);

	if (trap_EntityContact(mins, maxs, trigger))
		return qtrue;//Player is touching the trigger
	return qfalse;//Player is not touching the trigger
}

int InterpolateTouchTime(gentity_t *activator, gentity_t *trigger)
{ //We know that last client frame, they were not touching the flag, but now they are.  Last client frame was pmoveMsec ms ago, so we only want to interp inbetween that range.
	vec3_t	interpOrigin, delta;
	int lessTime = 0;

	VectorCopy(activator->client->ps.origin, interpOrigin);
	VectorScale(activator->s.pos.trDelta, 0.001f, delta);//Delta is how much they travel in 1 ms.

	VectorSubtract(interpOrigin, delta, interpOrigin);//Do it once before we loop

	while (InTrigger(interpOrigin, trigger)) {//This will be done a max of pml.msec times, in theory, before we are guarenteed to not be in the trigger anymore.
		lessTime++; //Add one more ms to be subtracted
		VectorSubtract(interpOrigin, delta, interpOrigin); //Keep Rewinding position by a tiny bit, that corresponds with 1ms precision (delta*0.001), since delta is per second.
		if (lessTime >= activator->client->pmoveMsec || lessTime >= 8) { //activator->client->pmoveMsec
			break; //In theory, this should never happen, but just incase stop it here.
		}
	}

	//trap->SendServerCommand( -1, va("chat \"Subtracted %i milliseconds with interp\"", lessTime) );

	return lessTime;
}

static int GetTimeMS() {
	return trap_Milliseconds();
}

void TimerStart(gentity_t *trigger, gentity_t *player, trace_t *trace) {//JK2PRO Timers
	int lessTime;

	if (!player->client)
		return;
	if (player->client->sess.sessionTeam != TEAM_FREE)
		return;
	if (player->r.svFlags & SVF_BOT)
		return;
	if (GetTimeMS() - player->client->pers.stats.startTime < 500)//Some built in floodprotect per player?
		return;
	if (player->client->pers.stats.lastResetTime == level.time) //Dont allow a starttimer to start in the same frame as a resettimer (called from noclip or amtele)
		return;
	if (player->client->ps.pm_type != PM_NORMAL && player->client->ps.pm_type != PM_FLOAT && player->client->ps.pm_type != PM_FREEZE)
		return;

	//jk2pro - addlater - auto demo record stuff
	/*if (player->client->pers.recordingDemo && player->client->pers.keepDemo) {
		//We are still recording a demo that we want to keep?
		//Stop and rename it
		//trap->SendServerCommand( player-g_entities, "chat \"RECORDING STOPPED (at startline), HIGHSCORE\"");
		trap_SendConsoleCommand(EXEC_APPEND, va("svstoprecord %i;wait 20;svrenamedemo demos/temp/%s.dm_26 demos/races/%s.dm_26\n", player->client->ps.clientNum, player->client->pers.oldDemoName, player->client->pers.demoName));
		player->client->pers.recordingDemo = qfalse;
	}


	if ((sv_autoRaceDemo.integer) && !(player->client->pers.noFollow) && !(player->client->pers.practice) && player->client->sess.raceMode && !sv_cheats.integer && player->client->pers.userName[0]) {
		if (!player->client->pers.recordingDemo) { //Start the new demo
			player->client->pers.recordingDemo = qtrue;
			//trap->SendServerCommand( player-g_entities, "chat \"RECORDING STARTED\"");
			trap_SendConsoleCommand(EXEC_APPEND, va("svrecord temp/%s %i\n", player->client->pers.userName, player->client->ps.clientNum));
		}
		else { //Check if we should "restart" the demo
			if (!player->client->lastStartTime || (level.time - player->client->lastStartTime > 5000)) {
				player->client->pers.recordingDemo = qtrue;
				//trap->SendServerCommand( player-g_entities, "chat \"RECORDING RESTARTED\"");
				trap_SendConsoleCommand(EXEC_APPEND, va("svstoprecord %i;wait 20;svrecord temp/%s %i\n", player->client->ps.clientNum, player->client->pers.userName, player->client->ps.clientNum));
			}
		}
	}
	player->client->lastStartTime = level.time;
	player->client->pers.keepDemo = qfalse;*/
	//jk2pro - addlater - END auto demo record stuff

	if (trigger->noise_index)
		G_Sound(player, CHAN_AUTO, trigger->noise_index);//could just use player instead of trigger->activator ?   How do we make this so only the activator hears it?

	player->client->pers.startLag = GetTimeMS() - level.frameStartTime + level.time - player->client->pers.cmd.serverTime; 

	player->client->pers.stats.startLevelTime = level.time; //Should this use trap milliseconds instead.. 
	player->client->pers.stats.startTime = GetTimeMS();
	lessTime = InterpolateTouchTime(player, trigger);
	player->client->pers.stats.startTime -= lessTime;
	player->client->pers.stats.topSpeed = 0;
	player->client->pers.stats.displacement = 0;
	player->client->pers.stats.displacementSamples = 0;

	if (player->client->ps.stats[STAT_RACEMODE]) {
		player->client->ps.duelTime = level.time - lessTime;//player->client->pers.stats.startTime;//level.time;

		player->client->ps.stats[STAT_HEALTH] = player->health = player->client->ps.stats[STAT_MAX_HEALTH];
		player->client->ps.stats[STAT_ARMOR] = 25;

		if (!player->client->pers.userName[0]) //In racemode but not logged in
			trap_SendServerCommand(player - g_entities, "cp \"^3Warning: You are not logged in!\n\n\n\n\n\n\n\n\n\n\"");
		else if (player->client->pers.noFollow)
			trap_SendServerCommand(player - g_entities, "cp \"^3Warning: times are not valid while hidden!\n\n\n\n\n\n\n\n\n\n\""); //Since times wont be saved if they arnt logged in anyway
		else if (player->client->pers.practice)
			trap_SendServerCommand(player - g_entities, "cp \"^3Warning: times are not valid in practice mode!\n\n\n\n\n\n\n\n\n\n\""); //Since times wont be saved if they arnt logged in anyway
	}
}

void TimeToString(int duration_ms, char *timeStr, size_t strSize, qboolean noMS);
//void G_AddRaceTime(char *account, char *courseName, int duration_ms, int style, int topspeed, int average, int clientNum); //should this be extern?
void TimerStop(gentity_t *trigger, gentity_t *player, trace_t *trace) {//JK2PRO Timers
	if (!player->client)
		return;
	if (player->client->sess.sessionTeam != TEAM_FREE)
		return;
	if (player->r.svFlags & SVF_BOT)
		return;
	if (player->client->ps.pm_type != PM_NORMAL && player->client->ps.pm_type != PM_FLOAT && player->client->ps.pm_type != PM_FREEZE)
		return;

	//multi_trigger(trigger, player);

	if (player->client->pers.stats.startTime) {
		char style[32] = { 0 }, timeStr[32] = { 0 }, playerName[MAX_NETNAME] = { 0 };
		char c[4] = S_COLOR_RED;
		float time = GetTimeMS() - player->client->pers.stats.startTime;
		int average, restrictions = 0, nameColor = 7;
		qboolean valid = qfalse;
		const int endLag = GetTimeMS() - level.frameStartTime + level.time - player->client->pers.cmd.serverTime;
		const int diffLag = player->client->pers.startLag - endLag;

		if (diffLag > 0) {//Should this be more trusting..?.. -20? -30?
			time += diffLag;
		}
		//else 
		//time -= 10; //Clients time was massively fucked due to lag, improve it up the minimum ammount..

		//if (player->client->sess.fullAdmin) //think we can finally remove this debug print
		//trap->SendServerCommand( player-g_entities, va("chat \"Msec diff due to warp (added if > 0): %i\"", diffLag));

		//trap->SendServerCommand( player-g_entities, va("chat \"diffLag: %i\"", diffLag));

		time -= InterpolateTouchTime(player, trigger);//Other is the trigger_multiple that set this off
		time /= 1000.0f;
		if (time < 0.001f)
			time = 0.001f;
		//average = floorf(player->client->pers.stats.displacement / ((level.time - player->client->pers.stats.startLevelTime) * 0.001f)) + 0.5f;//Should use level time for this 
		if (player->client->pers.stats.displacementSamples)
			average = /*floorf(*/((player->client->pers.stats.displacement * sv_fps.value) / player->client->pers.stats.displacementSamples);// + 0.5f);
		else
			average = player->client->pers.stats.topSpeed;

		if (trigger->spawnflags)//Get the restrictions for the specific course (only allow jump1, or jump2, etc..)
			restrictions = trigger->spawnflags;

		if (ValidRaceSettings(restrictions, player)) {
			valid = qtrue;
			if (player->client->pers.userName && player->client->pers.userName[0])
				Q_strncpyz(c, S_COLOR_CYAN, sizeof(c));
			else
				Q_strncpyz(c, S_COLOR_GREEN, sizeof(c));
		}

		if (valid && (player->client->ps.stats[STAT_MOVEMENTSTYLE] == 1) && trigger->awesomenoise_index && (time <= trigger->speed)) //Play the awesome noise if they were fast enough
			G_Sound(player, CHAN_AUTO, trigger->awesomenoise_index);//Just play it in jk2 physics for now...
		else if (trigger->noise_index)
			G_Sound(player, CHAN_AUTO, trigger->noise_index);

		if (player->client->ps.stats[STAT_RACEMODE]) {
			if (player->client->ps.stats[STAT_MOVEMENTSTYLE] == MV_SAGA)
				Q_strncpyz(style, "siege", sizeof(style));
			else if (player->client->ps.stats[STAT_MOVEMENTSTYLE] == MV_JK2)
				Q_strncpyz(style, "jk2", sizeof(style));
			else if (player->client->ps.stats[STAT_MOVEMENTSTYLE] == MV_QW)
				Q_strncpyz(style, "qw", sizeof(style));
			else if (player->client->ps.stats[STAT_MOVEMENTSTYLE] == MV_CPM)
				Q_strncpyz(style, "cpm", sizeof(style));
			else if (player->client->ps.stats[STAT_MOVEMENTSTYLE] == MV_Q3)
				Q_strncpyz(style, "q3", sizeof(style));
			else if (player->client->ps.stats[STAT_MOVEMENTSTYLE] == MV_PJK)
				Q_strncpyz(style, "pjk", sizeof(style));
			else if (player->client->ps.stats[STAT_MOVEMENTSTYLE] == MV_WSW)
				Q_strncpyz(style, "wsw", sizeof(style));
			else if (player->client->ps.stats[STAT_MOVEMENTSTYLE] == MV_RJQ3)
				Q_strncpyz(style, "rjq3", sizeof(style));
			else if (player->client->ps.stats[STAT_MOVEMENTSTYLE] == MV_RJCPM)
				Q_strncpyz(style, "rjcpm", sizeof(style));
		}
		else if (g_movementStyle.integer == MV_SAGA)
			Q_strncpyz(style, "siege", sizeof(style));
		else if (g_movementStyle.integer == MV_JK2)
			Q_strncpyz(style, "jk2", sizeof(style));
		else if (g_movementStyle.integer == MV_QW)
			Q_strncpyz(style, "qw", sizeof(style));
		else if (g_movementStyle.integer == MV_CPM)
			Q_strncpyz(style, "cpm", sizeof(style));
		else if (g_movementStyle.integer == MV_Q3)
			Q_strncpyz(style, "q3", sizeof(style));
		else if (g_movementStyle.integer == MV_PJK)
			Q_strncpyz(style, "pjk", sizeof(style));
		else if (g_movementStyle.integer == MV_WSW)
			Q_strncpyz(style, "wsw", sizeof(style));
		else if (g_movementStyle.integer == MV_RJQ3)
			Q_strncpyz(style, "rjq3", sizeof(style));
		else if (g_movementStyle.integer == MV_RJCPM)
			Q_strncpyz(style, "rjcpm", sizeof(style));

		/*
		if (time >= 60.0f) { //LODA FIXME, make this use the
		int minutes, seconds, milliseconds;

		minutes = (int)time / 60;
		seconds = (int)time % 60;
		milliseconds = ((int)(time*1000)%1000); //milliseconds = fmodf(time, milliseconds);
		Com_sprintf(timeStr, sizeof(timeStr), "%i:%02i.%03i", minutes, seconds, milliseconds);
		}
		else
		Q_strncpyz(timeStr, va("%.3f", time), sizeof(timeStr));
		*/

		TimeToString((int)(time * 1000), timeStr, sizeof(timeStr), qfalse);

		Q_strncpyz(playerName, player->client->pers.netname, sizeof(playerName));
		Q_StripColor(playerName);
		nameColor = 7 - (player->client->ps.clientNum % 8);//sad hack
		if (nameColor < 2)
			nameColor = 2;
		else if (nameColor > 7 || nameColor == 5)
			nameColor = 7;

		if (trigger->message) {
			trap_SendServerCommand(-1, va("print \"^3%-16s%s completed in ^3%-12s%s max:^3%-10i%s average:^3%-10i%s style:^3%-10s%s by ^%i%s\n\"",
				trigger->message, c, timeStr, c, (int)(player->client->pers.stats.topSpeed), c, average, c, style, c, nameColor, playerName));
		}
		else {
			//Q_strcat(courseName, sizeof(courseName), " ()");
			trap_SendServerCommand(-1, va("print \"%sCompleted in ^3%-12s%s max:^3%-10i%s average:^3%-10i%s style:^3%-10s%s by ^%i%s\n\"",
				c, timeStr, c, (int)(player->client->pers.stats.topSpeed), c, average, c, style, c, nameColor, playerName));
		}
		if (valid) {
			char strIP[NET_ADDRSTRMAXLEN] = { 0 };
			char *p = NULL;
			Q_strncpyz(strIP, player->client->sess.IP, sizeof(strIP));
			p = strchr(strIP, ':');
			if (p)
				*p = 0;
			//DM - addlater
			/*if (player->client->pers.userName[0]) { //omg
				G_AddRaceTime(player->client->pers.userName, trigger->message, (int)(time * 1000), player->client->ps.stats[STAT_MOVEMENTSTYLE], (int)floorf(player->client->pers.stats.topSpeed + 0.5f), average, player->client->ps.clientNum);
			}*/
		}

		player->client->pers.stats.startLevelTime = 0;
		player->client->pers.stats.startTime = 0;
		player->client->pers.stats.topSpeed = 0;
		player->client->pers.stats.displacement = 0;
		if (player->client->ps.stats[STAT_RACEMODE])
			player->client->ps.duelTime = 0;
	}
}

void TimerCheckpoint(gentity_t *trigger, gentity_t *player, trace_t *trace) {//JK2PRO Timers
	if (!player->client)
		return;
	if (player->client->sess.sessionTeam != TEAM_FREE)
		return;
	if (player->r.svFlags & SVF_BOT)
		return;
	if (player->client->ps.pm_type != PM_NORMAL && player->client->ps.pm_type != PM_FLOAT && player->client->ps.pm_type != PM_FREEZE)
		return;
	if (player->client->pers.stats.startTime && trigger && trigger->spawnflags & 2) { //Instead of a checkpoint, make it reset their time (they went out of bounds or something)
		player->client->pers.stats.startTime = 0;
		if (player->client->sess.raceMode)
			player->client->ps.duelTime = 0;
		trap_SendServerCommand(player - g_entities, "cp \"Timer reset\n\n\n\n\n\n\n\n\n\n\""); //Send message?
		return;
	}

	if (player->client->pers.stats.startTime && (level.time - player->client->pers.stats.lastCheckpointTime > 1000)) { //make this more accurate with interp? or dosnt really matter ...
		int i;
		int time = GetTimeMS() - player->client->pers.stats.startTime;
		const int endLag = GetTimeMS() - level.frameStartTime + level.time - player->client->pers.cmd.serverTime;
		const int diffLag = player->client->pers.startLag - endLag;
		int average;
		//const int average = floorf(player->client->pers.stats.displacement / ((level.time - player->client->pers.stats.startLevelTime) * 0.001f)) + 0.5f; //Could this be more accurate?
		if (player->client->pers.stats.displacementSamples)
			average = floorf(((player->client->pers.stats.displacement * sv_fps.value) / player->client->pers.stats.displacementSamples) + 0.5f);
		else
			average = player->client->pers.stats.topSpeed;

		if (diffLag > 0) {//Should this be more trusting..?.. -20? -30?
			time += diffLag;
		}
		//else 
		//time -= 10; //Clients time was massively fucked due to lag, improve it up the minimum ammount..

		if (time < 1)
			time = 1;

		/*
		if (trigger && trigger->spawnflags & 1)//Minimalist print loda fixme get rid of target shit
		trap->SendServerCommand( player-g_entities, va("cp \"^3%.3fs^5, avg ^3%i^5u\n\n\n\n\n\n\n\n\n\n\"", (float)time * 0.001f, average));
		else
		trap->SendServerCommand( player-g_entities, va("chat \"^5Checkpoint: ^3%.3f^5, max ^3%i^5, average ^3%i^5 ups\"", (float)time * 0.001f, player->client->pers.stats.topSpeed, average));
		*/

		if (player->client->pers.showCenterCP)
			trap_SendServerCommand(player - g_entities, va("cp \"^3%.3fs^5, avg ^3%i^5u\n\n\n\n\n\n\n\n\n\n\"", (float)time * 0.001f, average));
		if (player->client->pers.showChatCP)
			trap_SendServerCommand(player - g_entities, va("chat \"^5Checkpoint: ^3%.3f^5, max ^3%i^5, average ^3%i^5 ups\"", (float)time * 0.001f, (int)floorf(player->client->pers.stats.topSpeed + 0.5f), average));

		for (i = 0; i<MAX_CLIENTS; i++) {//Also print to anyone spectating them..
			if (!g_entities[i].inuse)
				continue;
			if ((level.clients[i].sess.sessionTeam == TEAM_SPECTATOR) && (level.clients[i].ps.pm_flags & PMF_FOLLOW) && (level.clients[i].sess.spectatorClient == player->client->ps.clientNum))
			{
				//if (trigger && trigger->spawnflags & 1)//Minimalist print loda fixme get rid of target shit 
				if (level.clients[i].pers.showCenterCP)
					trap_SendServerCommand(i, va("cp \"^3%.3fs^5, avg ^3%i^5u\n\n\n\n\n\n\n\n\n\n\"", (float)time * 0.001f, average));
				if (level.clients[i].pers.showChatCP)
					trap_SendServerCommand(i, va("chat \"^5Checkpoint: ^3%.3f^5, max ^3%i^5, average ^3%i^5 ups\"", (float)time * 0.001f, (int)floorf(player->client->pers.stats.topSpeed + 0.5f), average));
			}
		}

		player->client->pers.stats.lastCheckpointTime = level.time; //For built in floodprotect
	}
}

void SP_trigger_timer_start(gentity_t *self)
{
	char	*s;
	InitTrigger(self);

	if (G_SpawnString("noise", "", &s)) {
		if (s && s[0])
			self->noise_index = G_SoundIndex(s);
		else
			self->noise_index = 0;
	}

	self->touch = TimerStart;
	trap_LinkEntity(self);
}

void SP_trigger_timer_checkpoint(gentity_t *self)
{
	char	*s;
	InitTrigger(self);

	if (G_SpawnString("noise", "", &s)) {
		if (s && s[0])
			self->noise_index = G_SoundIndex(s);
		else
			self->noise_index = 0;
	}
	self->touch = TimerCheckpoint;
	trap_LinkEntity(self);
}

void SP_trigger_timer_stop(gentity_t *self)
{
	char	*s;
	InitTrigger(self);

	if (G_SpawnString("noise", "", &s)) {
		if (s && s[0])
			self->noise_index = G_SoundIndex(s);
		else
			self->noise_index = 0;
	}
	if (G_SpawnString("awesomenoise", "", &s)) {
		if (s && s[0])
			self->awesomenoise_index = G_SoundIndex(s);
		else
			self->awesomenoise_index = 0;
	}

	//For every stop trigger, increment numCourses and put its name in array
	if (self->message && self->message[0]) {
		Q_strncpyz(level.courseName[level.numCourses], self->message, sizeof(level.courseName[0]));
		Q_strlwr(level.courseName[level.numCourses]);
		Q_CleanStr(level.courseName[level.numCourses]);
		level.numCourses++;
	}
	else if (level.numCourses == 0) { //hmmmmmmmmm!
		Q_strncpyz(level.courseName[level.numCourses], "", sizeof(level.courseName[0]));
		level.numCourses = 1;
	}

	self->touch = TimerStop;
	trap_LinkEntity(self);
}

/*QUAKED target_push (.5 .5 .5) (-8 -8 -8) (8 8 8) bouncepad
Pushes the activator in the direction.of angle, or towards a target apex.
"speed"		defaults to 1000
if "bouncepad", play bounce noise instead of windfly
*/
void SP_target_push( gentity_t *self ) {
	if (!self->speed) {
		self->speed = 1000;
	}
	G_SetMovedir (self->s.angles, self->s.origin2);
	VectorScale (self->s.origin2, self->speed, self->s.origin2);

	if ( self->spawnflags & 1 ) {
		self->noise_index = G_SoundIndex("sound/weapons/force/jump.wav");
	} else {
		self->noise_index = 0;	//G_SoundIndex("sound/misc/windfly.wav");
	}
	if ( self->target ) {
		VectorCopy( self->s.origin, self->r.absmin );
		VectorCopy( self->s.origin, self->r.absmax );
		self->think = AimAtTarget;
		self->nextthink = level.time + FRAMETIME;
	}
	self->use = Use_target_push;
}

/*
==============================================================================

trigger_teleport

==============================================================================
*/

void trigger_teleporter_touch (gentity_t *self, gentity_t *other, trace_t *trace ) {
	gentity_t	*dest;

	if ( !other->client ) {
		return;
	}
	if ( other->client->ps.pm_type == PM_DEAD ) {
		return;
	}
	// Spectators only?
	if ( ( self->spawnflags & 1 ) && 
		other->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		return;
	}


	dest = 	G_PickTarget( self->target );
	if (!dest) {
		G_Printf ("Couldn't find teleporter destination\n");
		return;
	}

	TeleportPlayer( other, dest->s.origin, dest->s.angles );
}


/*QUAKED trigger_teleport (.5 .5 .5) ? SPECTATOR
Allows client side prediction of teleportation events.
Must point at a target_position, which will be the teleport destination.

If spectator is set, only spectators can use this teleport
Spectator teleporters are not normally placed in the editor, but are created
automatically near doors to allow spectators to move through them
*/
void SP_trigger_teleport( gentity_t *self ) {
	InitTrigger (self);

	// unlike other triggers, we need to send this one to the client
	// unless is a spectator trigger
	if ( self->spawnflags & 1 ) {
		self->r.svFlags |= SVF_NOCLIENT;
	} else {
		self->r.svFlags &= ~SVF_NOCLIENT;
	}

	// make sure the client precaches this sound
	G_SoundIndex("sound/weapons/force/speed.wav");

	self->s.eType = ET_TELEPORT_TRIGGER;
	self->touch = trigger_teleporter_touch;

	trap_LinkEntity (self);
}


/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.5 .5 .5) ? START_OFF CAN_TARGET SILENT NO_PROTECTION SLOW
Any entity that touches this will be hurt.
It does dmg points of damage each server frame
Targeting the trigger will toggle its on / off state.

SILENT			supresses playing the sound
SLOW			changes the damage rate to once per second
NO_PROTECTION	*nothing* stops the damage

"dmg"			default 5 (whole numbers only)
If dmg is set to -1 this brush will use the fade-kill method

*/
void hurt_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	if (activator && activator->inuse && activator->client)
	{
		self->activator = activator;
	}
	else
	{
		self->activator = NULL;
	}

	if ( self->r.linked ) {
		trap_UnlinkEntity( self );
	} else {
		trap_LinkEntity( self );
	}
}

void hurt_touch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	int		dflags;

	if ( !other->takedamage ) {
		return;
	}

	if ( self->timestamp > level.time ) {
		return;
	}

	if (self->damage == -1 && other && other->client && other->health < 1)
	{
		other->client->ps.fallingToDeath = 0;
		respawn(other);
		return;
	}

	if (self->damage == -1 && other && other->client && other->client->ps.fallingToDeath)
	{
		return;
	}

	if ( self->spawnflags & 16 ) {
		self->timestamp = level.time + 1000;
	} else {
		self->timestamp = level.time + FRAMETIME;
	}

	// play sound
	if ( !(self->spawnflags & 4) && self->damage != -1 ) {
		G_Sound( other, CHAN_AUTO, self->noise_index );
	}

	if (self->spawnflags & 8)
		dflags = DAMAGE_NO_PROTECTION;
	else
		dflags = 0;

	if (self->damage == -1 && other && other->client && other->client->sess.raceMode) { //Racemode falling to death
		if (self->activator && self->activator->inuse && self->activator->client)
		{
			G_Damage(other, self->activator, self->activator, NULL, NULL, 9999, dflags | DAMAGE_NO_PROTECTION, MOD_TRIGGER_HURT);
		}
		else
		{
			G_Damage(other, self, self, NULL, NULL, 9999, dflags | DAMAGE_NO_PROTECTION, MOD_TRIGGER_HURT);
		}
	}
	else if (self->damage == -1 && other && other->client)
	{
		if (other->client->ps.otherKillerTime > level.time)
		{ //we're as good as dead, so if someone pushed us into this then remember them
			other->client->ps.otherKillerTime = level.time + 20000;
			other->client->ps.otherKillerDebounceTime = level.time + 10000;
		}
		other->client->ps.fallingToDeath = level.time;

		self->timestamp = 0; //do not ignore others
		G_EntitySound(other, CHAN_VOICE, G_SoundIndex("*falling1.wav"));
	}
	else	
	{
		int dmg = self->damage;

		if (dmg == -1)
		{ //so fall-to-blackness triggers destroy evertyhing
			dmg = 99999;
			self->timestamp = 0;
		}
		if (self->activator && self->activator->inuse && self->activator->client)
		{
			G_Damage (other, self->activator, self->activator, NULL, NULL, dmg, dflags|DAMAGE_NO_PROTECTION, MOD_TRIGGER_HURT);
		}
		else
		{
			G_Damage (other, self, self, NULL, NULL, dmg, dflags|DAMAGE_NO_PROTECTION, MOD_TRIGGER_HURT);
		}
	}
}

void SP_trigger_hurt( gentity_t *self ) {
	InitTrigger (self);

	gTrigFallSound = G_SoundIndex("*falling1.wav");

	self->noise_index = G_SoundIndex( "sound/weapons/force/speed.wav" );
	self->touch = hurt_touch;

	if ( !self->damage ) {
		self->damage = 5;
	}

	self->r.contents = CONTENTS_TRIGGER;

	if ( self->spawnflags & 2 ) {
		self->use = hurt_use;
	}

	// link in to the world if starting active
	if ( ! (self->spawnflags & 1) ) {
		trap_LinkEntity (self);
	}
	else if (self->r.linked)
	{
		trap_UnlinkEntity(self);
	}
}


/*
==============================================================================

timer

==============================================================================
*/


/*QUAKED func_timer (0.3 0.1 0.6) (-8 -8 -8) (8 8 8) START_ON
This should be renamed trigger_timer...
Repeatedly fires its targets.
Can be turned on or off by using.

"wait"			base time between triggering all targets, default is 1
"random"		wait variance, default is 0
so, the basic time between firing is a random time between
(wait - random) and (wait + random)

*/
void func_timer_think( gentity_t *self ) {
	G_UseTargets (self, self->activator);
	// set time before next firing
	self->nextthink = level.time + 1000 * ( self->wait + crandom() * self->random );
}

void func_timer_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	self->activator = activator;

	// if on, turn it off
	if ( self->nextthink ) {
		self->nextthink = 0;
		return;
	}

	// turn it on
	func_timer_think (self);
}

void SP_func_timer( gentity_t *self ) {
	G_SpawnFloat( "random", "1", &self->random);
	G_SpawnFloat( "wait", "1", &self->wait );

	self->use = func_timer_use;
	self->think = func_timer_think;

	if ( self->random >= self->wait ) {
		self->random = self->wait - FRAMETIME;
		G_Printf( "func_timer at %s has random >= wait\n", vtos( self->s.origin ) );
	}

	if ( self->spawnflags & 1 ) {
		self->nextthink = level.time + FRAMETIME;
		self->activator = self;
	}

	self->r.svFlags = SVF_NOCLIENT;
}


