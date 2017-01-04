#include "g_local.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

double floorf(double x) {
	return (int)x;
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
