/*************************************************************************
*
*  inter.c - Handles the details of testing ray/object intersectons
*    within the bounding box tree.
*
*************************************************************************/

#include "ray.h"


/*************************************************************************
 *  Bounding box queue element.
 */
struct BBQ
{
  struct BBQ *pool_next,    /* Next BBQ in global pool. */
             *next;         /* Next BBQ in local list. */
  BBoxData *bbox;           /* Enqueued bounding box. */
  double t;                 /* Closest positive "t" value for BBox. */
};

/*************************************************************************
 *  Local stuff...
 */
static struct BBQ *bbq_pool;

static struct BBQ *fetch_bbq(void)
{
  struct BBQ *newbbq;

  for(newbbq = bbq_pool; newbbq != NULL; newbbq = newbbq->pool_next)
    if(newbbq->bbox == NULL)
    {
      newbbq->next = NULL;
      return newbbq;
    }

  newbbq = (struct BBQ *)Malloc(sizeof(struct BBQ));
  newbbq->pool_next = bbq_pool;  /* NULL if this is the first one. */
  newbbq->next = NULL;
  newbbq->bbox = NULL;
  bbq_pool = newbbq;
  return newbbq;
}


int FindClosestIntersection(Object *first_obj, HitData *hits)
{
  Object *obj, *closest_obj;
  struct BBQ *first_bbox_queued, *last_bbox_queued, *bbq;
  double closest_t;
  int entering = 0;

  last_bbox_queued = NULL;
  first_bbox_queued = NULL;
  ct.calc_all = 0;
  closest_t = HUGE;
  closest_obj = NULL;
  obj = first_obj;

  while(obj != NULL)
  {
    if( ! ((ct.ray_flags & RAY_SHADOW) &&
          ((obj->flags & OBJ_FLAG_NO_SHADOW) ||
          ((obj == ct.baseobj) && (obj->flags & OBJ_FLAG_NO_SELF_INTERSECT))))
      )
    {
      if((obj->procs->Intersect)(obj, hits))
      {
        if(obj->procs->type == OBJ_BBOX)
        {
          /*
           * Place bounding boxes in queue to be tested
           * after the non-BBox objects in this list are tested.
           */
          bbq = fetch_bbq();
          bbq->bbox = obj->data.bbox;
          bbq->t = hits->t;
          if(last_bbox_queued == NULL)
            first_bbox_queued = bbq;
          else
            last_bbox_queued->next = bbq;
          last_bbox_queued = bbq;
        }
        else if(hits->t < closest_t)
        {
          closest_obj = hits->obj;
          closest_t = hits->t;
          entering = hits->entering;
        }
      }
    }

    obj = obj->next;

    if(obj == NULL)
    {
      /*
       * End of object list was reached. If there are
       * bounding boxes, pull them from the queue until
       * one that has a "t" value that is closer than the
       * closest object hit, if any, is found and recycle.
       * "obj" will point to a new list if a closer BBox
       * is found.
       */
      while(first_bbox_queued != NULL)
      {
        /* Pull first bounding box from the queue... */
        bbq = first_bbox_queued;
        first_bbox_queued = first_bbox_queued->next;
        if(first_bbox_queued == NULL)  /* This was the only one... */
          last_bbox_queued = NULL;        /* ...Clear the queue. */
        /* Get potential new object list to test... */
        obj = bbq->bbox->objects;
        /* Flag this queue element as "free" in the global pool. */
        bbq->bbox = NULL;
        /* See if BBox is closer than closest object... */
        if(bbq->t < closest_t)
          break;  /* BBox is closer, recycle with the new list... */
        /* BBox is not closer, discard the new list... */
        obj = NULL;
        /* Move on to next BBox, if any. */
      }
    }
  }

  if(closest_obj != NULL)
  {
    ct.objhit = closest_obj;
    ct.t = closest_t;
    ct.entering = entering;
    ct.Q.x = ct.B.x + closest_t * ct.D.x;
    ct.Q.y = ct.B.y + closest_t * ct.D.y;
    ct.Q.z = ct.B.z + closest_t * ct.D.z;
    closest_obj->procs->CalcNormal(closest_obj, &ct.Q, &ct.N);
    if(V3Dot(&ct.N, &ct.D) > 0.0)
    {
      ct.N.x = -ct.N.x;
      ct.N.y = -ct.N.y;
      ct.N.z = -ct.N.z;
    }
    return 1;
  }

  return 0;
}


int FindAllIntersections(Object *first_obj, HitData *hits)
{
  Object *obj;
  int nhits, nobjhits;
  struct BBQ *first_bbox_queued, *last_bbox_queued, *bbq;

  last_bbox_queued = NULL;
  first_bbox_queued = NULL;
  nhits = 0;
  ct.calc_all++;
  obj = first_obj;

  while(obj != NULL)
  {
    if( ! ((ct.ray_flags & RAY_SHADOW) &&
          ((obj->flags & OBJ_FLAG_NO_SHADOW) ||
          ((obj == ct.baseobj) && (obj->flags & OBJ_FLAG_NO_SELF_INTERSECT))))
      )
    {
      nobjhits = (obj->procs->Intersect)(obj, hits);
      if(nobjhits)
      {
        if(obj->procs->type == OBJ_BBOX)
        {
          /*
           * Place bounding boxes in queue to be tested
           * after the non-BBox objects in this list are tested.
           */
          bbq = fetch_bbq();
          bbq->bbox = obj->data.bbox;
          bbq->t = hits->t;
          if(last_bbox_queued == NULL)
            first_bbox_queued = bbq;
          else
            last_bbox_queued->next = bbq;
          last_bbox_queued = bbq;
        }
        else
        {
          nhits += nobjhits;
          while(--nobjhits)
            hits = hits->next;
          hits = GetNextHit(hits);
        }
      }
    }
    obj = obj->next;

    if(obj == NULL)
    {
      /*
       * End of object list was reached. If there are
       * bounding boxes, pull next one from the queue and recycle.
       * "obj" will point to a new list of objects.
       */
      if(first_bbox_queued != NULL)
      {
        /* Pull first bounding box from the queue... */
        bbq = first_bbox_queued;
        first_bbox_queued = first_bbox_queued->next;
        if(first_bbox_queued == NULL)  /* This was the only one... */
          last_bbox_queued = NULL;        /* ...Clear the queue. */
        /* Get new object list to test... */
        obj = bbq->bbox->objects;
        /* Flag this queue element as "free" in the global pool. */
        bbq->bbox = NULL;
        /* Recycle with new object list. */
      }
    }
  }
  ct.calc_all--;

  return nhits;
}


void SortHits(HitData *hits, int nhits)
{
  HitData tmphit;
  HitData *h1, *h2;
  int i, j;

  if(nhits < 2)
    return;

	for(i = nhits - 1; i >= 1; i--)
  {
    h1 = hits;
    h2 = hits->next;
		for(j = 1; j <= i; j++)
    {
			if(h1->t > h2->t)
			{
				tmphit = *h1;
				h1->obj = h2->obj;
				h1->entering = h2->entering;
				h1->t = h2->t;
        h2->obj = tmphit.obj;
        h2->entering = tmphit.entering;
        h2->t = tmphit.t;
			}
      h1 = h1->next;
      h2 = h2->next;
    }
  }
}


HitData *NewHitData(void)
{
  HitData *hit = (HitData *)Malloc(sizeof(HitData));
  if(hit != NULL)
  {
    memset(hit, 0, sizeof(HitData));
    hit->next = NULL;
    hit->obj = NULL;
  }
  return hit;
}


HitData *DeleteHits(HitData *hits)
{
  HitData *h;
  while(hits != NULL)
  {
    h = hits;
    hits = hits->next;
    Free(h, sizeof(HitData));
  }
  return NULL;
}


HitData *GetNextHit(HitData *hit)
{
  if(hit->next == NULL)
    hit->next = NewHitData();
  return hit->next;
}


void InitializeInter(void)
{
  bbq_pool = NULL;
}


void CloseInter(void)
{
  struct BBQ *bbq;

  while((bbq = bbq_pool) != NULL)
  {
    bbq_pool = bbq_pool->pool_next;
    Free(bbq, sizeof(struct BBQ));
  }
}

