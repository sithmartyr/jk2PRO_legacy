#include "g_local.h"

#define LOCAL_RACE_PATH "races.jks"
#define LOCAL_ACCT_PATH "accts.jks"
#define MAX_TMP_LOG_SIZE 80 * 1024

float floorf(float x) {
	if (x < 0.0)
	{
		x -= 1;
		return (int)x;
	}
	return (int)x;
}

float ceilf(float x) {
	if (x > 0.0)
	{
		x += 1;
		return (int)x;
	}
	return (int)x;
}




unsigned int ip_to_int(const char * ip) {
	unsigned v = 0;
	int i;
	const char * start;

	start = ip;
	for (i = 0; i < 4; i++) {
		char c;
		int n = 0;
		while (1) {
			c = *start;
			start++;
			if (c >= '0' && c <= '9') {
				n *= 10;
				n += c - '0';
			}
			else if ((i < 3 && c == '.') || i == 3)
				break;
			else
				return 0;
		}
		if (n >= 256)
			return 0;
		v *= 256;
		v += n;
	}
	return v;
}

void G_AddPlayerLog(char *name, char *strIP) {
	fileHandle_t f;
	char string[128], string2[128], buf[31 * 1024], cleanName[MAX_NETNAME];
	char *p = NULL;
	int	fLen = 0;
	char*	pch;
	qboolean unique = qtrue;

	p = strchr(strIP, ':');
	if (p) //loda - fix ip sometimes not printing
		*p = 0;

	if (!Q_stricmp(strIP, "")) //No IP or GUID info to be gained.. so forget it
		return;

	Q_strncpyz(cleanName, name, sizeof(cleanName));

	Q_strlwr(cleanName);
	Q_CleanStr(cleanName);

	if (!Q_stricmp(name, "padawan")) //loda fixme, also ignore Padawan[0] etc..
		return;
	if (!Q_stricmp(name, "padawan[0]")) //loda fixme, also ignore Padawan[0] etc..
		return;
	if (!Q_stricmp(name, "padawan[1]")) //loda fixme, also ignore Padawan[0] etc..
		return;

	Com_sprintf(string, sizeof(string), "%s;%s", cleanName, strIP); //Store ip as int or char??.. lets do int

	fLen = trap_FS_FOpenFile(PLAYER_LOG, &f, FS_READ);
	if (!f) {
		Com_Printf("ERROR: Couldn't load player logfile %s\n", PLAYER_LOG);
		return;
	}

	if (fLen >= 24 * 1024) {
		trap_FS_FCloseFile(f);
		Com_Printf("ERROR: Couldn't load player logfile %s, file is too large\n", PLAYER_LOG);
		return;
	}

	trap_FS_Read(buf, fLen, f);
	buf[fLen] = 0;
	trap_FS_FCloseFile(f);

	pch = strtok(buf, "\n");

	while (pch != NULL) {
		if (!Q_stricmp(string, pch)) {
			unique = qfalse;
			break;
		}
		pch = strtok(NULL, "\n");
	}

	if (unique) {
		Com_sprintf(string2, sizeof(string2), "%s\n", string);
		trap_FS_Write(string2, strlen(string2), level.playerLog);
	}
}

int CheckUserExists(char un[16]) {
	fileHandle_t file;
	int fileLen, i = 0, c = 0, x = 0, y = 0;
	char fbuf[31 * 1024], *lnNum, fLine[52] = { 0 }, uname[16] = { 0 };

	fileLen = trap_FS_FOpenFile(LOCAL_ACCT_PATH, &file, FS_READ);
	trap_FS_Read(fbuf, fileLen, file);
	fbuf[fileLen] = 0;
	trap_FS_FCloseFile;

	lnNum = fbuf;
	for (; lnNum[i]; lnNum[i] == '\n' ? i++ : *lnNum++);
	
	for (; ; x++, c++)
	{
		if (fileLen < 12) //The file isn't long enough to even contain a single user's information, so exit the loop here.
			break;
		if (fbuf[x] == 0) //We've reached the end of the file buffer, so break out of the main loop
			break;
		if (fbuf[x] == '\n') {
			for (y = 0; ; y++) //This drains the uname char
			{
				if (uname[y] == 0)
					break;
				uname[y] = 0;
			}
			for (y=0; ; y++) //This fills it with the username from each line
			{
				if (fLine[y] == ' ')
					break;
				else
					uname[y] = fLine[y];
			}
			if (Q_stricmp(un, uname) == 0) { return 1; }
			for (y=0; ; y++)
			{
				if (fLine[y] == 0)
					break;
				fLine[y] = 0;
			}
			c = -1;
		}
		else
			fLine[c] = fbuf[x];
	}
	return 0;
}

int CheckIPExists(char strIP[16]) {
	fileHandle_t file;
	int fileLen, i=0, c=0, x=0, y=0, z=0, spcCnt = 0;
	char fbuf[31 * 1024], *lnNum, fLine[52] = { 0 }, ip[16] = { 0 };

	fileLen = trap_FS_FOpenFile(LOCAL_ACCT_PATH, &file, FS_READ);
	trap_FS_Read(fbuf, fileLen, file);
	fbuf[fileLen] = 0;
	trap_FS_FCloseFile;

	lnNum = fbuf;
	for (; lnNum[i]; lnNum[i] == '\n' ? i++ : *lnNum++);

	
	for (; ; x++, c++)
	{
		if (fileLen < 12)
			break;
		if (fbuf[x] == 0)
			break;
		if (fbuf[x] == '\n') //We've hit the end of the line
		{ 
			for (y = 0; ; y++) //This drains the ip char
			{
				if (ip[y] == 0)
					break;
				ip[y] = 0;
			}
			for (y = 0; ; y++) //This fills it with the IP from each line
			{
				if (fLine[y] == ' ')
					spcCnt++;

				if (spcCnt == 2)
				{
					y++;
					for (z = 0; ; y++, z++)
					{
						
						if (fLine[y] == 0)
							break;
						ip[z] = fLine[y];
					}
					break;
				}
			}
			spcCnt = 0;
			if (Q_stricmp(strIP, ip) == 0) { return 1; }
			for (y = 0; ; y++)
			{
				if (fLine[y] == 0)
					break;
				fLine[y] = 0;
			}
			c = -1;
		}
		else
			fLine[c] = fbuf[x];
	}
	return 0;
}

int GetUserValid(char un[16], char pass[16]) {
	fileHandle_t file;
	int fileLen, i = 0, c = 0, x = 0, y = 0;
	char fbuf[31 * 1024], *lnNum, fLine[52] = { 0 }, uname[16] = { 0 }, pword[16] = { 0 };

	fileLen = trap_FS_FOpenFile(LOCAL_ACCT_PATH, &file, FS_READ);
	trap_FS_Read(fbuf, fileLen, file);
	fbuf[fileLen] = 0;
	trap_FS_FCloseFile;

	lnNum = fbuf;
	for (; lnNum[i]; lnNum[i] == '\n' ? i++ : *lnNum++);

	for (; ; x++, c++)
	{
		if (fileLen < 12) //The file isn't long enough to even contain a single user's information, so exit the loop here.
			break;
		if (fbuf[x] == 0) //We've reached the end of the file buffer, so break out of the main loop
			break;
		if (fbuf[x] == '\n') {
			for (y = 0; ; y++) //This drains the uname char
			{
				if (uname[y] == 0)
					break;
				uname[y] = 0;
			}
			for (y = 0; ; y++) //This fills it with the username from each line
			{
				if (fLine[y] == ' ')
					break;
				else
					uname[y] = fLine[y];
			}
			if (Q_stricmp(un, uname) == 0) { //Username matches, now to check password
				for (c=0, y++; ; y++, c++)
				{
					if (fLine[y] == ' ')
						break;
					pword[c] = fLine[y];
				}
				if (Q_stricmp(pass, pword) == 0) //Password matches too, let them login, return a 1
					return 1;
				else
					return 0;
			}
			for (y = 0; ; y++)
			{
				if (fLine[y] == 0)
					break;
				fLine[y] = 0;
			}
			c = -1;
		}
		else
			fLine[c] = fbuf[x];
	}
	return 0;
}

void Cmd_ACRegister_f(gentity_t *ent) {
	char username[16], password[16], strIP[NET_ADDRSTRMAXLEN] = { 0 };
	char *p = NULL;
	int s;
	unsigned int ip;
	fileHandle_t f;
	char account[64];


	/*if (!g_allowRegistration.integer) { //DM Debug - comment out for debugging purposes.
		trap_SendServerCommand(ent - g_entities, "print \"This server does not allow registration\n\"");
		return;
	}*/

	if (trap_Argc() != 3) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /register <username> <password>\n\"");
		return;
	}

	if (Q_stricmp(ent->client->pers.userName, "")) {
		trap_SendServerCommand(ent - g_entities, "print \"You are already logged in!\n\"");
		return;
	}

	trap_Argv(1, username, sizeof(username));
	trap_Argv(2, password, sizeof(password));

	Q_strlwr(username);
	Q_CleanStr(username);
	Q_strstrip(username, "\n\r;:.?*<>|\\/\"", NULL);

	Q_CleanStr(password);

	if (CheckUserExists(username)) {
		trap_SendServerCommand(ent - g_entities, "print \"This account name has already been taken!\n\"");
		return;
	}

	Q_strncpyz(strIP, ent->client->sess.IP, sizeof(strIP));
	p = strchr(strIP, ':');
	if (p)
		*p = 0;
	//ip = ip_to_int(strIP);

	if (CheckIPExists(strIP)) {
		trap_SendServerCommand(ent - g_entities, "print \"Your IP address already belongs to an account. You are only allowed one account.\n\"");
		return;
	}


	if (ent->client)
		//Com_sprintf(account, sizeof(account), "{ \"Username: %s\", \"Password: %s\", \"IP: %s\" },\n", username, password, strIP);
		Com_sprintf(account, sizeof(account), "%s %s %s\n", username, password, strIP);

	trap_FS_FOpenFile(LOCAL_ACCT_PATH, &f, FS_APPEND_SYNC);
	trap_FS_Write(account, strlen(account), f);
	trap_FS_FCloseFile(f);
}

void Cmd_ACLogin_f(gentity_t *ent) {
	//unsigned int ip, lastip = 0;
	char username[16], enteredPassword[16], password[16] /*enteredKey[32]*/;
	int i;
	//char *p = NULL;
	gclient_t *cl;

	if (!ent->client)
		return;

	if (trap_Argc() != 3) {
		trap_SendServerCommand(ent - g_entities, "print \"Usage: /login <username> <password>\n\"");
		return;
	}

	if (Q_stricmp(ent->client->pers.userName, "")) {
		trap_SendServerCommand(ent - g_entities, "print \"You are already logged in!\n\"");
		return;
	}

	trap_Argv(1, username, sizeof(username));
	trap_Argv(2, enteredPassword, sizeof(password));

	Q_strlwr(username);
	Q_CleanStr(username);

	Q_CleanStr(enteredPassword);

	for (i = 0; i<MAX_CLIENTS; i++) {//Build a list of clients
		if (!g_entities[i].inuse)
			continue;
		cl = &level.clients[i];
		if (!Q_stricmp(username, cl->pers.userName)) {
			trap_SendServerCommand(ent - g_entities, "print \"This account is already logged in!\n\"");
			return;
		}
	}

	if (GetUserValid(username, enteredPassword))
	{
		Q_strncpyz(ent->client->pers.userName, username, sizeof(ent->client->pers.userName));
		trap_SendServerCommand(ent - g_entities, "print \"Login successful!\n\"");
		return;
	}
	else
	{
		trap_SendServerCommand(ent - g_entities, "print \"Incorrect login!\n\"");
		return;
	}
}

void Cmd_ACLogout_f(gentity_t *ent) {
	if (Q_stricmp(ent->client->pers.userName, "")) {
		Q_strncpyz(ent->client->pers.userName, "", sizeof(ent->client->pers.userName));
		trap_SendServerCommand(ent - g_entities, "print \"Logged out.\n\"");
	}
	else
		trap_SendServerCommand(ent - g_entities, "print \"You are not logged in!\n\"");
}

void TimeToString(int duration_ms, char *timeStr, size_t strSize, qboolean noMs) {
	if (duration_ms > (60 * 60 * 1000)) { //thanks, eternal
		int hours, minutes, seconds, milliseconds;
		hours = (int)((duration_ms / (1000 * 60 * 60)) % 24); //wait wut
		minutes = (int)((duration_ms / (1000 * 60)) % 60);
		seconds = (int)(duration_ms / 1000) % 60;
		milliseconds = duration_ms % 1000;
		if (noMs)
			Com_sprintf(timeStr, strSize, "%i:%02i:%02i", hours, minutes, seconds);
		else
			Com_sprintf(timeStr, strSize, "%i:%02i:%02i.%03i", hours, minutes, seconds, milliseconds);
	}
	else if (duration_ms > (60 * 1000)) {
		int minutes, seconds, milliseconds;
		minutes = (int)((duration_ms / (1000 * 60)) % 60);
		seconds = (int)(duration_ms / 1000) % 60;
		milliseconds = duration_ms % 1000;
		if (noMs)
			Com_sprintf(timeStr, strSize, "%i:%02i", minutes, seconds);
		else
			Com_sprintf(timeStr, strSize, "%i:%02i.%03i", minutes, seconds, milliseconds);
	}
	else {
		if (noMs)
			Q_strncpyz(timeStr, va("%.0f", ((float)duration_ms * 0.001)), strSize);
		else
			Q_strncpyz(timeStr, va("%.3f", ((float)duration_ms * 0.001)), strSize);
	}
}

int RaceNameToInteger(char *style) {
	Q_strlwr(style);
	Q_CleanStr(style);

	if (!Q_stricmp(style, "saga") || !Q_stricmp(style, "0"))
		return 0;
	if (!Q_stricmp(style, "jko") || !Q_stricmp(style, "jk2") || !Q_stricmp(style, "1"))
		return 1;
	if (!Q_stricmp(style, "hl2") || !Q_stricmp(style, "hl1") || !Q_stricmp(style, "hl") || !Q_stricmp(style, "qw") || !Q_stricmp(style, "2"))
		return 2;
	if (!Q_stricmp(style, "cpm") || !Q_stricmp(style, "cpma") || !Q_stricmp(style, "3"))
		return 3;
	if (!Q_stricmp(style, "q3") || !Q_stricmp(style, "q3") || !Q_stricmp(style, "4"))
		return 4;
	if (!Q_stricmp(style, "pjk") || !Q_stricmp(style, "5"))
		return 5;
	if (!Q_stricmp(style, "wsw") || !Q_stricmp(style, "warsow") || !Q_stricmp(style, "6"))
		return 6;
	if (!Q_stricmp(style, "rjq3") || !Q_stricmp(style, "q3rj") || !Q_stricmp(style, "7"))
		return 7;
	if (!Q_stricmp(style, "rjcpm") || !Q_stricmp(style, "cpmrj") || !Q_stricmp(style, "8"))
		return 8;
	return -1;
}
