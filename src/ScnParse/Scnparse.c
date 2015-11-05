/*************************************************************************
 *
 *  scnparse.c - Misc. functions for the scene script parser module.
 *
 ************************************************************************/

#include "scn.h"

/* Warning and error counters. */
int scn_warning_cnt;
int scn_error_cnt;
int scn_fatal;

/* Animation counters. */
int scn_start_frame;
int scn_end_frame;
int scn_cur_frame;
double scn_normalized_frame;
double scn_normalized_frame2;

/*
 * User supplied message logging proc.
 */
void (*SCN_LogMsg)(const char *msg, int type);

/*
 * Default message logging proc for fn pointer above.
 */
void DefaultLogMsg(const char *msg, int type)
{
  printf(msg);
}

void SCN_ColdStart(void)
{
  SCN_LogMsg = DefaultLogMsg;
  scn_warning_cnt = 0;
  scn_error_cnt = 0;
  scn_fatal = 0;

	scn_start_frame = 0;
	scn_end_frame = 0;
	scn_cur_frame = 0;
	scn_normalized_frame = 0.0;
	scn_normalized_frame2 = 0.0;

  FindFileInitialize();
}

void SCN_WarmStart(void)
{
  scn_warning_cnt = 0;
  scn_error_cnt = 0;
  scn_fatal = 0;
  Init_Expr();
  Image_Initialize();

  /* Calc the normalized frame counters. */
  if(scn_end_frame != 0)
  {
    scn_normalized_frame =
      (double)scn_cur_frame / (double)scn_end_frame;
    scn_normalized_frame2 =
      (double)scn_cur_frame / (double)(scn_end_frame + 1);
  }
  else
  {
    scn_normalized_frame = scn_normalized_frame2 = 0.0;
  }
}

void SCN_Shutdown(void)
{
  Image_Close();
  Close_Expr();
  Close_Tokens();
  FindFileClose();
}


/*
 * If there's more frames to go, bump the frame counter and return 1.
 * Returns 0 if there's no more frames left to go.
 */
int SCN_NextFrame(void)
{
	if(scn_cur_frame < scn_end_frame)
	{
		scn_cur_frame++;
		return 1;
	}
	return 0;
}


void SCN_Message(int type, const char *msg, ...)
{
  va_list ap;
  char str[MAX_MESSAGE_SIZE];
  char outstr[MAX_MESSAGE_SIZE];

  va_start(ap, msg);
  str[0] = '\0';
  switch(type)
  {
    case SCN_MSG_WARNING:
      strcpy(str, "Warning: ");
      scn_warning_cnt++;
      break;
    case SCN_MSG_ERROR:
      strcpy(str, "Error: ");
      scn_error_cnt++;
      break;
    case SCN_MSG_FATAL:
      strcpy(str, "Fatal: ");
      scn_error_cnt++;
      scn_fatal = 1;
      break;
    default:
      break;
  }
  strcat(str, msg);
  vsprintf(outstr, str, ap);
  SCN_LogMsg(outstr, type);
  va_end(ap);
}

