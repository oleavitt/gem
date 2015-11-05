/**
 *****************************************************************************
 *  @file error.c
 *  Error managment stuff.
 *
 *****************************************************************************
 */

#include "local.h"

/* Client-supplied message output function. */
MSGFN Scn20Message = NULL; 

int g_warning_count;
int g_error_count;
int g_fatal_count;

static char tmpbuf[SCN_MSG_MAX];
static char outmsg[SCN_MSG_MAX];

int error_initialize(void)
{
	g_error_count = g_warning_count = g_fatal_count = 0;
	return 1;
}

void error_close(void)
{
}

void logmsg(const char *fmt, ...)
{
	if(Scn20Message != NULL)
	{
		va_list ap;
		va_start(ap, fmt);
		vsprintf(outmsg, fmt, ap);
		va_end(ap);
		Scn20Message(outmsg);
	}
}

void logmsg0(const char *msg)
{
	if(Scn20Message != NULL)
		Scn20Message(msg);
}

void logwarn(const char *fmt, ...)
{
	g_warning_count++;
	if(Scn20Message != NULL)
	{
		va_list ap;
		va_start(ap, fmt);
		strcpy(tmpbuf, "Warning: ");
		strcat(tmpbuf, fmt);
		vsprintf(outmsg, tmpbuf, ap);
		va_end(ap);
		Scn20Message(outmsg);
	}
}

void logerror(const char *fmt, ...)
{
	g_error_count++;
	if(Scn20Message != NULL)
	{
		va_list ap;
		va_start(ap, fmt);
		strcpy(tmpbuf, "Error: ");
		strcat(tmpbuf, fmt);
		vsprintf(outmsg, tmpbuf, ap);
		va_end(ap);
		Scn20Message(outmsg);
	}
}

void logfatal(const char *fmt, ...)
{
	g_fatal_count++;
	if(Scn20Message != NULL)
	{
		va_list ap;
		va_start(ap, fmt);
		strcpy(tmpbuf, "Fatal: ");
		strcat(tmpbuf, fmt);
		vsprintf(outmsg, tmpbuf, ap);
		va_end(ap);
		Scn20Message(outmsg);
	}
}

void logmemerror(const char *who)
{
	if(who != NULL && *who != '\0')
		logerror("%s: Unable to allocate memory.", who);
	else
		logerror("Unable to allocate memory.");
}
