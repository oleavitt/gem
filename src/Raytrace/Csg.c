/*************************************************************************
*
*  csg.c
*
*  The CSG object types.
*
*  TODO: Fix CSG_GetTextureInfo() so that clipped objects return the
*    correct child texture.
*
*************************************************************************/

#include "ray.h"

int csg_nest_level;

static int csg_non_group;

#define USE_IS_INSIDE

static int IntersectCSGGroup(Object *obj, HitData *hits);
static int IsInsideCSGGroup(Object *obj, Vec3 *P);
static void DrawCSGGroup(Object *obj);

static int IntersectCSGUnion(Object *obj, HitData *hits);
static int IsInsideCSGUnion(Object *obj, Vec3 *P);
static void DrawCSGUnion(Object *obj);

static int IntersectCSGDifference(Object *obj, HitData *hits);
static int IsInsideCSGDifference(Object *obj, Vec3 *P);
static void DrawCSGDifference(Object *obj);

static int IntersectCSGIntersection(Object *obj, HitData *hits);
static int IsInsideCSGIntersection(Object *obj, Vec3 *P);
static void DrawCSGIntersection(Object *obj);

static int IntersectCSGClip(Object *obj, HitData *hits);
static int IsInsideCSGClip(Object *obj, Vec3 *P);
static void DrawCSGClip(Object *obj);

static void CalcNormalCSG(Object *obj, Vec3 *P, Vec3 *N);
static void CalcUVMapCSG(Object *obj, Vec3 *P, double *u, double *v);
static void CalcExtentsCSG(Object *obj, Vec3 *omin, Vec3 *omax);
static void TransformCSG(Object *obj, Vec3 *params, int type);
static void CopyCSG(Object *destobj, Object *srcobj);
static void DeleteCSG(Object *obj);


static ObjectProcs csggroup_procs =
{
  OBJ_CSGGROUP,
  IntersectCSGGroup,
  CalcNormalCSG,
  IsInsideCSGGroup,
  CalcUVMapCSG,
  CalcExtentsCSG,
  TransformCSG,
  CopyCSG,
  DeleteCSG,
  DrawCSGGroup
};


static ObjectProcs csgunion_procs =
{
  OBJ_CSGUNION,
  IntersectCSGUnion,
  CalcNormalCSG,
  IsInsideCSGUnion,
  CalcUVMapCSG,
  CalcExtentsCSG,
  TransformCSG,
  CopyCSG,
  DeleteCSG,
  DrawCSGUnion
};


static ObjectProcs csgdifference_procs =
{
  OBJ_CSGDIFFERENCE,
  IntersectCSGDifference,
  CalcNormalCSG,
  IsInsideCSGDifference,
  CalcUVMapCSG,
  CalcExtentsCSG,
  TransformCSG,
  CopyCSG,
  DeleteCSG,
  DrawCSGDifference
};


static ObjectProcs csgintersection_procs =
{
  OBJ_CSGINTERSECTION,
  IntersectCSGIntersection,
  CalcNormalCSG,
  IsInsideCSGIntersection,
  CalcUVMapCSG,
  CalcExtentsCSG,
  TransformCSG,
  CopyCSG,
  DeleteCSG,
  DrawCSGIntersection
};


static ObjectProcs csgclip_procs =
{
  OBJ_CSGCLIP,
  IntersectCSGClip,
  CalcNormalCSG,
  IsInsideCSGClip,
  CalcUVMapCSG,
  CalcExtentsCSG,
  TransformCSG,
  CopyCSG,
  DeleteCSG,
  DrawCSGClip
};


int Ray_ObjectIsCSG(Object *obj)
{
	if (obj != NULL)
	{
		return ((obj->procs->type == OBJ_CSGGROUP) ||
		        (obj->procs->type == OBJ_CSGUNION) ||
		        (obj->procs->type == OBJ_CSGDIFFERENCE) ||
		        (obj->procs->type == OBJ_CSGINTERSECTION) ||
		        (obj->procs->type == OBJ_CSGCLIP));
	}
	return 0;
}


Object *Ray_MakeCSG(int type)
{
	Object *obj = NewObject();

	if(obj != NULL)
	{
		CSGData *csg = (CSGData *)Malloc(sizeof(CSGData));
		if(csg != NULL)
		{
			csg->children = NULL;
			csg->nchildren = 0;  /* Not used in group objects. */
			csg->olist = NULL;   /* Not used in group objects. */
			csg->litelist = NULL;
			csg->boundobj = NULL;
			csg->hits = NewHitData();
			obj->data.csg = csg;
			obj->procs = (type == OBJ_CSGUNION) ? &csgunion_procs :
				(type == OBJ_CSGDIFFERENCE) ? &csgdifference_procs :
				(type == OBJ_CSGINTERSECTION) ? &csgintersection_procs :
				(type == OBJ_CSGCLIP) ? &csgclip_procs :
				&csggroup_procs;
		}
		else
			obj = Ray_DeleteObject(obj);
	}
	return obj;
}


int Ray_AddCSGBound(Object *obj, Object *boundobj)
{
	CSGData *csg;

	if (boundobj == NULL)
	{
		assert(0);
		return 0;
	}
	if (obj == NULL)
	{
		assert(0);
		return 0;
	}
	csg = obj->data.csg;
	if (csg == NULL)
	{
		assert(0);
		return 0;
	}

	if (csg->boundobj != NULL)
	{
		Object *lastobj = csg->boundobj;
		while (lastobj->next != NULL)
			lastobj = lastobj->next;
		lastobj->next = boundobj;
	}
	else
		csg->boundobj = boundobj;
	
	return 1;
}


int Ray_AddCSGChild(Object *obj, Object *childobj)
{
	CSGData		*csg;
	ObjectList	*ol;

	if (childobj == NULL)
	{
		assert(0);
		return 0;
	}
	if (obj == NULL)
	{
		assert(0);
		return 0;
	}
	csg = obj->data.csg;
	if (csg == NULL)
	{
		assert(0);
		return 0;
	}

	if (csg->children != NULL)
	{
		Object *lastobj = csg->children;
		while (lastobj->next != NULL)
			lastobj = lastobj->next;
		lastobj->next = childobj;
	}
	else
		csg->children = childobj;
	
	if (obj->procs->type == OBJ_CSGGROUP || obj->procs->type == OBJ_CSGCLIP)
		return 1;

	ol = (ObjectList *)Malloc(sizeof(ObjectList));
	if (ol == NULL)
		return 0;
	ol->obj = childobj;
	ol->next = NULL;
	if (csg->olist != NULL)
	{
		ObjectList *lastol = csg->olist;
		while (lastol->next != NULL)
			lastol = lastol->next;
		lastol->next = ol;
	}
	else
		csg->olist = ol;
	csg->nchildren++;

	return 1;
}


int Ray_FinishCSG(Object *obj)
{
	CSGData *csg;

	assert(obj != NULL);
	csg = obj->data.csg;
	assert(csg != NULL);
	if (csg->children == NULL)
		return 0;
	return 1;
}


int IntersectCSGGroup(Object *obj, HitData *hits)
{
  CSGData *csg = obj->data.csg;
  HitData *h, *h2;
  int nhits;
/*	int i, inside, entering; */
  double tmp;

	/* Check user-supplied bound object(s), if present... */
	if(csg->boundobj != NULL)
	{
		Object *o;
		nhits = 0;
		for(o = csg->boundobj; o != NULL && nhits == 0; o = o->next)
		{
			if(o->procs->Intersect(o, hits))
			{
				nhits++;
				break;
			}
		}
		if(nhits == 0)
			return 0;
	}

  tmp = ct.tmax;
	ct.tmax = HUGE;
  nhits = FindAllIntersections(csg->children, hits);
	ct.tmax = tmp;
	if(nhits == 0)
		return 0;
  SortHits(hits, nhits);
	/*
	 * Determine if ray is originating from within object...
	 * If inside is positive, ray is originating from within the object.
   */
/*
	inside = 0;
  h = hits;
	for(i = nhits; i > 0; i--)
  {
		if(h->entering)
	    inside--;
    else
			inside++;
		h = h->next;
	}
  if(inside > 0)
		entering = (obj->flags & OBJ_FLAG_INVERSE) ? 1 : 0;
	else
    entering = (obj->flags & OBJ_FLAG_INVERSE) ? 0 : 1;
*/
  h = hits;
  h2 = csg->hits;
  csg->nhits = 0;
  while(nhits-- && (h->t < ct.tmax))
  {
    h2->obj = h->obj;
    hits->obj = obj;
/*    h2->entering = hits->entering = entering; */
    h2->entering = hits->entering;
    h2->t = hits->t = h->t;
    h2 = GetNextHit(h2);
    hits = hits->next;
    csg->nhits++;
/*    entering = 1 - entering; */
	  h = h->next;
		if(!ct.calc_all) break;
  }

  return csg->nhits;
}


#ifdef USE_IS_INSIDE
int IntersectCSGUnion(Object *obj, HitData *hits)
{
  CSGData *csg = obj->data.csg;
  HitData *h, *h2;
  int i, nhits, inside, entering;
  double tmp;
  Vec3 Q;
  ObjectList *ol;

	/* Check user-supplied bound object(s), if present... */
	if(csg->boundobj != NULL)
	{
		Object *o;
		nhits = 0;
		for(o = csg->boundobj; o != NULL && nhits == 0; o = o->next)
		{
			if(o->procs->Intersect(o, hits))
			{
				nhits++;
				break;
			}
		}
		if(nhits == 0)
			return 0;
	}

  /* Get all ray/object intersections for each child... */
  tmp = ct.tmax;
  ct.tmax = HUGE;
  nhits = FindAllIntersections(csg->children, hits);
  ct.tmax = tmp;
  if(nhits == 0)
    return 0;

  SortHits(hits, nhits);

  /*
   * Determine if ray is originating from within object...
   * If inside is positive, ray is originating from within the object.
   */
  inside = 0;
  h = hits;
  for(i = nhits; i > 0; i--)
  {
    if(h->entering)
      inside--;
    else
      inside++;
    h = h->next;
  }

  if(inside > 0)
    entering = (obj->flags & OBJ_FLAG_INVERSE) ? 1 : 0;
  else
    entering = (obj->flags & OBJ_FLAG_INVERSE) ? 0 : 1;

  h = hits;
  h2 = csg->hits;
  csg->nhits = 0;
  while(nhits-- && (h->t < ct.tmax))
  {
    Q.x = ct.B.x + ct.D.x * h->t;
    Q.y = ct.B.y + ct.D.y * h->t;
    Q.z = ct.B.z + ct.D.z * h->t;
    /*
     * If point is not inside any of the other objects,
     * add it to the final hit list.
     */
    for(ol = csg->olist; ol != NULL; ol = ol->next)
      if(ol->obj != h->obj && ol->obj->procs->IsInside(ol->obj, &Q))
        break;
    if(ol == NULL)  /* Point not inside other objects. */
    {
      h2->obj = h->obj;
      hits->obj = obj;
      h2->entering = hits->entering = entering;
      h2->t = hits->t = h->t;
      h2 = GetNextHit(h2);
      hits = hits->next;
      csg->nhits++;
      entering = 1 - entering;
			if(!ct.calc_all) break;
    }
    h = h->next;
  }

  return csg->nhits;
}

#else

int IntersectCSGUnion(Object *obj, HitData *hits)
{
  CSGData *csg = obj->data.csg;
  HitData *h, *h2;
  int nhits, i, inside;
  double tmp;

	/* Check user-supplied bound object(s), if present... */
	if(csg->boundobj != NULL)
	{
		Object *o;
		nhits = 0;
		for(o = csg->boundobj; o != NULL && nhits == 0; o = o->next)
		{
			if(o->procs->Intersect(o, hits))
			{
				nhits++;
				break;
			}
		}
		if(nhits == 0)
			return 0;
	}

  /* Get all ray/object intersections for each child... */
  tmp = ct.tmax;
  ct.tmax = HUGE;
  nhits = FindAllIntersections(csg->children, hits);
  ct.tmax = tmp;
  if(nhits == 0)
    return 0;

  SortHits(hits, nhits);

  /* Determine if ray is originating from within object... */
  /* If inside is positive, ray is originating from within the object. */
  inside = 0;
  h = hits;
  for(i = nhits; i > 0; i--)
  {
    if(h->entering)
      inside--;
    else
      inside++;
    h = h->next;
  }
  if(inside < 0)
    inside = 0;


  /* Cull out hits that are inside other sibling objects. */
  h = hits;
  h2 = csg->hits;
  csg->nhits = 0;
  for(i = nhits; (i > 0)&&(h->t < ct.tmax); i--)
  {
    if(h->entering)
    {
      if(!inside)
      {
        h2->obj = h->obj;
        h2->entering = hits->entering = !(obj->flags & OBJ_FLAG_INVERSE);
        h2->t = hits->t = h->t;
        h2 = GetNextHit(h2);
        hits->obj = obj;
        hits = hits->next;
        csg->nhits++;
				if(!ct.calc_all) break;
      }
      inside++;
    }
    else
    {
      inside--;
      if(!inside)
      {
        h2->obj = h->obj;
        h2->entering = hits->entering = (obj->flags & OBJ_FLAG_INVERSE);
        h2->t = hits->t = h->t;
        h2 = GetNextHit(h2);
        hits->obj = obj;
        hits = hits->next;
        csg->nhits++;
				if(!ct.calc_all) break;
      }
    }
    h = h->next;
  }

  return csg->nhits;
}

#endif

int IntersectCSGDifference(Object *obj, HitData *hits)
{
  CSGData *csg = obj->data.csg;
  HitData *h, *h2;
  int i, nhits, inside_first, inside_other, entering;
  double tmp;
  Vec3 Q;
  ObjectList *ol;

	/* Check user-supplied bound object(s), if present... */
	if(csg->boundobj != NULL)
	{
		Object *o;
		nhits = 0;
		for(o = csg->boundobj; o != NULL && nhits == 0; o = o->next)
		{
			if(o->procs->Intersect(o, hits))
			{
				nhits++;
				break;
			}
		}
		if(nhits == 0)
			return 0;
	}

  /* Get all ray/object intersections for each child... */
  tmp = ct.tmax;
  ct.tmax = HUGE;
  nhits = FindAllIntersections(csg->children, hits);
  ct.tmax = tmp;
  if(nhits == 0)
    return 0;

  SortHits(hits, nhits);

  /* Determine if ray is originating from within object... */
  /* If inside is positive, ray is originating from within the object. */
  inside_first = 0;
  inside_other = 0;
  h = hits;
  for(i = nhits; i > 0; i--)
  {
    if(h->obj == csg->olist->obj)
    {
      if(h->entering)
        inside_first--;
      else
        inside_first++;
    }
    else
    {
      if(h->entering)
        inside_other--;
      else
        inside_other++;
    }
    h = h->next;
  }
  if(inside_first < 0)
    inside_first = 0;
  if(inside_other < 0)
    inside_other = 0;

  if(inside_first && !inside_other)
    entering = (obj->flags & OBJ_FLAG_INVERSE) ? 1 : 0;
  else
    entering = (obj->flags & OBJ_FLAG_INVERSE) ? 0 : 1;

  h = hits;
  h2 = csg->hits;
  csg->nhits = 0;
  while(nhits-- && (h->t < ct.tmax))
  {
    Q.x = ct.B.x + ct.D.x * h->t;
    Q.y = ct.B.y + ct.D.y * h->t;
    Q.z = ct.B.z + ct.D.z * h->t;
    if(h->obj == csg->olist->obj)
    {
      /*
       * If point is not inside any of the other objects,
       * add it to the final hit list.
       */
      for(ol = csg->olist->next; ol != NULL; ol = ol->next)
        if(ol->obj->procs->IsInside(ol->obj, &Q))
          break;
      if(ol == NULL)  /* Point not inside other objects. */
      {
        h2->obj = h->obj;
        hits->obj = obj;
        h2->entering = hits->entering = entering;
        h2->t = hits->t = h->t;
        h2 = GetNextHit(h2);
        hits = hits->next;
        csg->nhits++;
        entering = 1 - entering;
				if(!ct.calc_all) break;
      }
    }
    else
    {
      /*
       * If point is not inside any of the other objects,
       * and is inside the first object add it to the final hit list.
       */
      ol = csg->olist;
      if(ol->obj->procs->IsInside(ol->obj, &Q))
      {
        for(ol = ol->next; ol != NULL; ol = ol->next)
          if(ol->obj != h->obj)
          {
            if(ol->obj->procs->IsInside(ol->obj, &Q))
              break;
          }
        if(ol == NULL)  /* Point not inside other objects. */
        {
          h2->obj = h->obj;
          hits->obj = obj;
          h2->entering = hits->entering = entering;
          h2->t = hits->t = h->t;
          h2 = GetNextHit(h2);
          hits = hits->next;
          csg->nhits++;
          entering = 1 - entering;
					if(!ct.calc_all) break;
        }
      }
    }
    h = h->next;
  }

  return csg->nhits;
}

#ifdef USE_IS_INSIDE
int IntersectCSGIntersection(Object *obj, HitData *hits)
{
  CSGData *csg = obj->data.csg;
  HitData *h, *h2;
  int i, nhits, inside, entering;
  double tmp;
  Vec3 Q;
  ObjectList *ol;

	/* Check user-supplied bound object(s), if present... */
	if(csg->boundobj != NULL)
	{
		Object *o;
		nhits = 0;
		for(o = csg->boundobj; o != NULL && nhits == 0; o = o->next)
		{
			if(o->procs->Intersect(o, hits))
			{
				nhits++;
				break;
			}
		}
		if(nhits == 0)
			return 0;
	}

  /* Get all ray/object intersections for each child... */
  tmp = ct.tmax;
  ct.tmax = HUGE;
  nhits = FindAllIntersections(csg->children, hits);
  ct.tmax = tmp;
  if(nhits == 0)
    return 0;

  SortHits(hits, nhits);

  /*
   * Determine if ray is originating from within object...
   * If inside is equal to the number of children, ray is
   * originating from within the object.
   */
  inside = 0;
  h = hits;
  for(i = nhits; i > 0; i--)
  {
    if(h->entering)
      inside--;
    else
      inside++;
    h = h->next;
  }

  if(inside == csg->nchildren)
    entering = (obj->flags & OBJ_FLAG_INVERSE) ? 1 : 0;
  else
    entering = (obj->flags & OBJ_FLAG_INVERSE) ? 0 : 1;

  h = hits;
  h2 = csg->hits;
  csg->nhits = 0;
  while(nhits-- && (h->t < ct.tmax))
  {
    Q.x = ct.B.x + ct.D.x * h->t;
    Q.y = ct.B.y + ct.D.y * h->t;
    Q.z = ct.B.z + ct.D.z * h->t;
    /*
     * If point is inside all of the other objects,
     * add it to the final hit list.
     */
    for(ol = csg->olist; ol != NULL; ol = ol->next)
      if(ol->obj != h->obj && ! ol->obj->procs->IsInside(ol->obj, &Q))
        break;
    if(ol == NULL)  /* Point not inside other objects. */
    {
      h2->obj = h->obj;
      hits->obj = obj;
      h2->entering = hits->entering = entering;
      h2->t = hits->t = h->t;
      h2 = GetNextHit(h2);
      hits = hits->next;
      csg->nhits++;
      entering = 1 - entering;
			if(!ct.calc_all) break;
    }
    h = h->next;
  }

  return csg->nhits;
}

#else

int IntersectCSGIntersection(Object *obj, HitData *hits)
{
  CSGData *csg = obj->data.csg;
  HitData *h, *h2;
  int i, nhits, inside, entering;
  double tmp;

	/* Check user-supplied bound object(s), if present... */
	if(csg->boundobj != NULL)
	{
		Object *o;
		nhits = 0;
		for(o = csg->boundobj; o != NULL && nhits == 0; o = o->next)
		{
			if(o->procs->Intersect(o, hits))
			{
				nhits++;
				break;
			}
		}
		if(nhits == 0)
			return 0;
	}

  /* Get all ray/object intersections for each child... */
  tmp = ct.tmax;
  ct.tmax = HUGE;
  nhits = FindAllIntersections(csg->children, hits);
  ct.tmax = tmp;
  if(nhits == 0)
    return 0;

  SortHits(hits, nhits);

  /*
   * Determine if ray is originating from within object...
   * If inside is equal to the number of children, ray is
   * originating from within the object.
   */
  inside = 0;
  h = hits;
  for(i = nhits; i > 0; i--)
  {
    if(h->entering)
      inside--;
    else
      inside++;
    h = h->next;
  }

  if(inside == csg->nchildren)
    entering = (obj->flags & OBJ_FLAG_INVERSE) ? 1 : 0;
  else
    entering = (obj->flags & OBJ_FLAG_INVERSE) ? 0 : 1;

  h = hits;
  h2 = csg->hits;
  csg->nhits = 0;
  while(nhits-- && (h->t < ct.tmax))
  {
    if(h->entering)
      inside++;
    if(inside == csg->nchildren)  /* Point inside all other objects. */
    {
      h2->obj = h->obj;
      hits->obj = obj;
      h2->entering = hits->entering = entering;
      h2->t = hits->t = h->t;
      h2 = GetNextHit(h2);
      hits = hits->next;
      csg->nhits++;
      entering = 1 - entering;
			if(!ct.calc_all) break;
    }
    if(!h->entering)
      inside--;
    h = h->next;
  }

  return csg->nhits;
}

#endif

int IntersectCSGClip(Object *obj, HitData *hits)
{
  CSGData *csg = obj->data.csg;
  Object *o, *clip_objs;
  HitData *h, *h2;
  Vec3 Q;
  int nhits;

	/* Check user-supplied bound object(s), if present... */
	if(csg->boundobj != NULL)
	{
		Object *o;
		nhits = 0;
		for(o = csg->boundobj; o != NULL && nhits == 0; o = o->next)
		{
			if(o->procs->Intersect(o, hits))
			{
				nhits++;
				break;
			}
		}
		if(nhits == 0)
			return 0;
	}

  /* Get all ray/object intersections for each child... */
  clip_objs = csg->children->next;
  csg->children->next = NULL;
  nhits = FindAllIntersections(csg->children, hits);
  csg->children->next = clip_objs;
  if(nhits == 0)
    return 0;

  h = hits;
  h2 = csg->hits;
  csg->nhits = 0;
  while(nhits--)
  {
    Q.x = ct.B.x + ct.D.x * h->t;
    Q.y = ct.B.y + ct.D.y * h->t;
    Q.z = ct.B.z + ct.D.z * h->t;
    for(o = clip_objs; o != NULL; o = o->next)
      if(o->procs->IsInside(o, &Q))
        break;
    if(o != NULL)
    {
      h2->obj = h->obj;
      hits->obj = obj;
      h2->entering = hits->entering = h->entering;
      h2->t = hits->t = h->t;
      h2 = GetNextHit(h2);
      hits = hits->next;
      csg->nhits++;
			if(!ct.calc_all) break;
    }
    h = h->next;
  }

  return csg->nhits;
}


int IsInsideCSGGroup(Object *obj, Vec3 *P)
{
  CSGData *csg = obj->data.csg;
	Object *o;

  /* We are "inside" if point is inside any of the child objects. */
  for(o = csg->children; o != NULL; o = o->next)
    if(o->procs->IsInside(o, P))
      return (!(obj->flags & OBJ_FLAG_INVERSE));
  return (obj->flags & OBJ_FLAG_INVERSE);
}


int IsInsideCSGUnion(Object *obj, Vec3 *P)
{
  CSGData *csg = obj->data.csg;
  ObjectList *ol;

  /* We are "inside" if point is inside any of the child objects. */
  for(ol = csg->olist; ol != NULL; ol = ol->next)
    if(ol->obj->procs->IsInside(ol->obj, P))
      return (!(obj->flags & OBJ_FLAG_INVERSE));
  return (obj->flags & OBJ_FLAG_INVERSE);
}


int IsInsideCSGDifference(Object *obj, Vec3 *P)
{
  CSGData *csg = obj->data.csg;
  ObjectList *ol;

  /*
   * We are "inside" if point is inside first object but not
   * inside any of the other objects.
   */
  ol = csg->olist;
  if(ol->obj->procs->IsInside(ol->obj, P))
  {
    for(ol = ol->next; ol != NULL; ol = ol->next)
      if(ol->obj->procs->IsInside(ol->obj, P))
        return (obj->flags & OBJ_FLAG_INVERSE);
    return (!(obj->flags & OBJ_FLAG_INVERSE));
  }
  return (obj->flags & OBJ_FLAG_INVERSE);
}


int IsInsideCSGIntersection(Object *obj, Vec3 *P)
{
  CSGData *csg = obj->data.csg;
  ObjectList *ol;

  /* We are "inside" if point is inside all of the child objects. */
  for(ol = csg->olist; ol != NULL; ol = ol->next)
    if(!(ol->obj->procs->IsInside(ol->obj, P)))
      return (obj->flags & OBJ_FLAG_INVERSE);
  return(!(obj->flags & OBJ_FLAG_INVERSE));
}


int IsInsideCSGClip(Object *obj, Vec3 *P)
{
  CSGData *csg = obj->data.csg;

  /* We are "inside" if point is inside the first object. */
  return(csg->children->procs->IsInside(csg->children, P));
}


/*************************************************************************
*
*  These functions are common to all CSG types.
*
*************************************************************************/

void CalcNormalCSG(Object *obj, Vec3 *P, Vec3 *N)
{
  CSGData *csg = obj->data.csg;
  HitData *h;
  int i;

  /*
   * Find matching "t" value in hit list for the current "t" value
   * of the ray and call CalcNormal() of corresponding child object.
   */
  h = csg->hits;
  for(i = csg->nhits; i > 0; i--)
  {
    assert(h != NULL);
    if(fabs(h->t - ct.t) < EPSILON)
      break;
    h = h->next;
  }
  assert(i > 0);  /* A match must always be found! */
  h->obj->procs->CalcNormal(h->obj, P, N);
/*	csg->hits->obj->procs->CalcNormal(csg->hits->obj, P, N); */
}


void CalcUVMapCSG(Object *obj, Vec3 *P, double *u, double *v)
{
  CSGData *csg = obj->data.csg;
  HitData *h;
  int i;

  /*
   * Find matching "t" value in hit list for the current "t" value
   * of the ray and call CalcNormal() of corresponding child object.
   */
  h = csg->hits;
  for(i = csg->nhits; i > 0; i--)
  {
    assert(h != NULL);
    if(fabs(h->t - ct.t) < EPSILON)
      break;
    h = h->next;
  }
  assert(i > 0);  /* A match must always be found! */
  h->obj->procs->CalcUVMap(h->obj, P, u, v);
}


void CalcExtentsCSG(Object *obj, Vec3 *omin, Vec3 *omax)
{
  CSGData *csg = obj->data.csg;
  Object *o;
  Vec3 bmin, bmax;

  /* Get cummulative bounds of all child objects. */
  o = csg->children;
  o->procs->CalcExtents(o, omin, omax);
  for(o = o->next; o != NULL; o = o->next)
  {
    o->procs->CalcExtents(o, &bmin, &bmax);
    if(bmin.x < omin->x)
      omin->x = bmin.x;
    if(bmax.x > omax->x)
      omax->x = bmax.x;
    if(bmin.y < omin->y)
      omin->y = bmin.y;
    if(bmax.y > omax->y)
      omax->y = bmax.y;
    if(bmin.z < omin->z)
      omin->z = bmin.z;
    if(bmax.z > omax->z)
      omax->z = bmax.z;
  }
}


void TransformCSG(Object *obj, Vec3 *params, int type)
{
  if(obj->T == NULL)
    obj->T = Ray_NewXform();
  XformXforms(obj->T, params, type);
}


void CopyCSG(Object *destobj, Object *srcobj)
{
	CSGData		*srccsg = srcobj->data.csg;
	CSGData		*destcsg = (CSGData *)Malloc(sizeof(CSGData));
	Object		*srco, *o;
	ObjectList	*destol;
	int			need_olist;

	need_olist = ((srcobj->procs->type != OBJ_CSGGROUP) &&
		(srcobj->procs->type != OBJ_CSGCLIP));
	destobj->data.csg = destcsg;
	if (destcsg != NULL)
	{
		*destcsg = *srccsg;
		o = NULL;
		destol = NULL;
		for (srco = srccsg->children; srco != NULL; srco = srco->next)
		{
			if (o != NULL)
			{
				o->next = Ray_CloneObject(srco);
				o = o->next;
				if (need_olist)
				{
					destol->next = (ObjectList *)Malloc(sizeof(ObjectList));
					destol = destol->next;
				}
			}
			else
			{
				o = destcsg->children = Ray_CloneObject(srco);
				if (need_olist)
					destol = destcsg->olist = (ObjectList *)Malloc(sizeof(ObjectList));
			}
			if (need_olist)
			{
				if (destol == NULL)
				{
					Ray_DeleteObject(o);
					break;
				}
				destol->obj = o;
				destol->next = NULL;				
			}
		}
		o = NULL;
		for (srco = srccsg->boundobj; srco != NULL; srco = srco->next)
		{
			if (o != NULL)
			{
				o->next = Ray_CloneObject(srco);
				o = o->next;
			}
			else
				o = destcsg->boundobj = Ray_CloneObject(srccsg->boundobj);
		}
		destcsg->hits = NewHitData();
	}
}


void DeleteCSG(Object *obj)
{
  CSGData *csg = obj->data.csg;
  Object *o;
  ObjectList *ol;

  while (csg->children != NULL)
  {
    o = csg->children;
    csg->children = o->next;
    Ray_DeleteObject(o);
  }
  while (csg->boundobj != NULL)
  {
    o = csg->boundobj;
    csg->boundobj = o->next;
    Ray_DeleteObject(o);
  }
  while (csg->olist != NULL)
  {
    ol = csg->olist;
    csg->olist = ol->next;
    Free(ol, sizeof(ObjectList));
  }
  DeleteHits(csg->hits);
  Free(csg, sizeof(CSGData));
}


void PostProcessCSGChild(Object *parent, Object *obj)
{
	if(obj->T == NULL)
		obj->T = Ray_CopyXform(parent->T);
	else if(parent->T != NULL)
		ConcatXforms(obj->T, parent->T);
	if((obj->T != NULL) && (obj->procs->type == OBJ_POLYGON))
		MatrixTransformPolygon(obj->data.polygon, obj->T);
	Ray_PostProcessObject(obj);
}


void PostProcessCSG(Object *obj)
{
	CSGData *csg = obj->data.csg;
	Object *o;

	if(csg_nest_level == 0)
		csg_non_group = 0;

	csg_nest_level++;
	if(obj->procs->type != OBJ_CSGGROUP)
		csg_non_group++;

	if(csg_non_group == 0)
		obj->flags |= OBJ_FLAG_GROUP_ONLY;
	for(o = csg->children; o != NULL; o = o->next)
		PostProcessCSGChild(obj, o);
	for(o = csg->boundobj; o != NULL; o = o->next)
		PostProcessCSGChild(obj, o);

	csg_nest_level--;
	if(obj->procs->type != OBJ_CSGGROUP)
		csg_non_group--;

	if(obj->procs->type == OBJ_CSGCLIP)
	{
		assert(csg->children->next != NULL);
		Ray_BuildBounds(&csg->children->next);
	}
	else
		Ray_BuildBounds(&csg->children);
}


void CSG_GetTextureInfo(Object *obj, Surface **surf, Xform **T)
{
	CSGData *csg = obj->data.csg;
	HitData *h;
	int i;

	/*
	 * Find matching "t" value in hit list for the current "t" value
	 * of the ray and call CalcNormal() of corresponding child object.
	 */
	h = csg->hits;
	for (i = csg->nhits; i > 0; i--)
	{
		assert(h != NULL);
		if (fabs(h->t - ct.t) < EPSILON)
			break;
		h = h->next;
	}
	assert(i > 0);  /* A match must always be found! */
	Object_GetTextureInfo(h->obj, surf, T);

	if (*surf == NULL && obj->surface != NULL)
	{
		*surf = obj->surface;
		*T = obj->T;
	}
}


/*************************************************************************
*
*  Draws a wire frame view of object.
*
*************************************************************************/

void DrawCSGGroup(Object *obj)
{
  CSGData *csg = obj->data.csg;
  Object *o;

  for(o = csg->children; o != NULL; o = o->next)
    o->procs->Draw(o);
}

void DrawCSGUnion(Object *obj)
{
  CSGData *csg = obj->data.csg;
  Object *o;

  for(o = csg->children; o != NULL; o = o->next)
    o->procs->Draw(o);
}

void DrawCSGDifference(Object *obj)
{
  CSGData *csg = obj->data.csg;
  Object *o;

  for(o = csg->children; o != NULL; o = o->next)
    o->procs->Draw(o);
}

void DrawCSGIntersection(Object *obj)
{
  CSGData *csg = obj->data.csg;
  Object *o;

  for(o = csg->children; o != NULL; o = o->next)
    o->procs->Draw(o);
}

void DrawCSGClip(Object *obj)
{
	CSGData *csg = obj->data.csg;
	csg->children->procs->Draw(csg->children);
}

