/*************************************************************************
*
*  tracestk.c
*
*  The ray-trace recursion stack.
*
*************************************************************************/

#include "ray.h"

/* Maximum trace recursion depth. */
int ray_max_trace_depth;

/* Current and previous trace levels. */
TraceStack ct, pt;

/* The stack array. */
static TraceStack **tstack;
static int tslevel;


static TraceStack *DeleteTraceStackElem(TraceStack *ts)
{
  if(ts != NULL)
  {
    DeleteHits(ts->hits);
      Free(ts, sizeof(TraceStack));
  }
  return NULL;
}


static TraceStack *NewTraceStackElem(void)
{
  TraceStack *ts = (TraceStack *)Malloc(sizeof(TraceStack));
  if(ts != NULL)
  {
    memset(ts, 0, sizeof(TraceStack));
    ts->hits = NewHit();
    if(ts->hits == NULL)
      ts = DeleteTraceStackElem(ts);
  }
  return ts;
}


int InitializeTraceStack(void)
{
  ray_max_trace_depth = 20;
  tstack = NULL;
  tslevel = 0;
  return 1;
}


int SetupTraceStack(void)
{
  int i;
  TraceStack *ts;

  if(ray_max_trace_depth < 0)
    ray_max_trace_depth = 0;

  tstack = (TraceStack **)Calloc(ray_max_trace_depth + 1,
    sizeof(TraceStack *));
  if(tstack == NULL)
    return 0;

  for(i = 0; i <= ray_max_trace_depth; i++)
  {
    ts = NewTraceStackElem();
    if(ts == NULL)
      return 0;
    ts->trace_level = i;
    tstack[i] = ts;
  }

  tslevel = 0;
  ct = *tstack[0];
  pt = ct;

  return 1;
}


void CloseTraceStack(void)
{
  int i;

  if(tstack != NULL)
  {
    for(i = 0; i <= ray_max_trace_depth; i++)
      DeleteTraceStackElem(tstack[i]);
    Free(tstack, sizeof(TraceStack *) * (ray_max_trace_depth + 1));
  }
  tstack = NULL;
}


void PushTraceStack(void)
{
  assert(tslevel < ray_max_trace_depth);
  pt = ct;
  *tstack[tslevel++] = ct;
  ct = *tstack[tslevel];
}


void PopTraceStack(void)
{
  assert(tslevel >= 0);
  pt = ct;
  ct = *tstack[--tslevel];
}


void UpdateTraceStack(void)
{
  *tstack[tslevel] = ct;
}

