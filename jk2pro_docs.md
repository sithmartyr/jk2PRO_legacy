## Server Cvars

#### Movement
	g_movementStyle			1	//Force movement style for players. 1=JK2 3=CPM
	
#### Dueling 
	g_duelStartHealth			0	
	g_duelStartArmor			0	
	g_duelDistanceLimit			0	
	g_allowGunDuel				1	
	
#### Admin
	g_juniorAdminLevel  0	
	g_fullAdminLevel	0
	g_juniorAdminPass	""
	g_fullAdminPass		""
	g_juniorAdminMsg	""
	g_fullAdminMsg		""
	
#### Other 
	g_consoleMOTD				""
	g_centerMOTDTime			5	
	g_centerMOTD				""
	g_allowSamePlayerNames		0	
	g_tweakVote					0	//Configured with /tweakVote command.
	g_voteTimeout				180	//Time in seconds to lockout callvote aftera failed vote.
	
#### Race/Accounts 
	g_raceMode					0	//0=Noracemode, 1=forcedracemode, 2=player can toggle race mode with /racecommand.
	g_allowRaceTele				0//1=Allow amtele in racemode. 2=Also allow noclip.
	g_forceLogin				0//Force players to login in order to be ingame.
	
## Server RCON Commands 
	gametype	//Instantly changethe gametype of the server without having to reload the map
	toggleAdmin	
	toggleVote	
	tweakVote	
	
#### Vote Tweaks 
	Allow specall vote in siege gametype	//1
	Allow specall vote in CTF/TFFA gametypes	//2
	Clearvote when going to spectate	//3
	Don't allow callvote for 30s after map load	//4
	Flood protect callvotes by IP	//5
	Don't allow map callvotes for 10 minutes at start of each map	//6
	Add vote delay for map call votes only	//7
	Allow voting from spectate	//8
	Show votes in console	//9
	Only count voters in pass/fail calculation	//10
	Fix map change after gametype vote	//11
	
## Server Game Commands 
#### General 
	engage_fullforceduel	
	engage_gunduel	
	move
	race
	
#### AdminCommands
	amtele	
	amtelemark	
	aminfo	
	amlogin	
	amlogout	
	
## Client Cvars ##

#### JK2PRO HUD/DISPLAY 
	cg_speedometer			0	
	cg_speedometerX			132	
	cg_speedometerY			459	
	cg_speedometerSize		0.75	
