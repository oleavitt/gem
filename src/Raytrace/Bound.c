/*************************************************************************
*
*                               bound.c
*
*     This module builds a hierarchical bounding volume tree from the
*  list of objects compiled by the parser. The bounding-box functions
*  are contained here, also. The bounding tree building function,
*  BuildBounds(), which takes in a linked list of objects,
*  compiled by the parser, sorts and divides it along the axis of
*  greatest spatial variation, into two sub-lists. If these sub lists
*  have more than "ray_max_cluster_size" objects in them, the sorting and
*  dividing process is repeated. When the sub lists get down to
*  "ray_max_cluster_size" or smaller, the list is enclosed in a bounding box
*  and a list of bounding boxes is built. If the bounding box list grows
*  greater than "ray_max_cluster_size", it is sorted and divided as above.
*  The net result is a tree of nested bounding boxes with not more than
*  "ray_max_cluster_size" objects or bounding boxes inside each bounding box.
*  "root" points to the start of the object list upon entry. When done,
*  "root" points to the top level bounding box of the tree.
*
*************************************************************************/

#include "ray.h"

/*************************************************************************
 * Public stuff...
 */
/* Number of bounds created. */
int ray_num_bounds;
/* Minimum number of objects required for bounding tree to be built. */
int ray_bound_threshold;
/* Maximum number of objects per bounding box. */
int ray_max_cluster_size;

static void DivideObjectList(Object **olist, Object **new_olist);
static void DivideObjectList2(Object **olist, Object **new_olist);

/*************************************************************************
 *  Procs for the bounding box object type.
 */
static int IntersectBBox(Object *obj, HitData *hits);
static void CalcNormalBBox(Object *obj, Vec3 *P, Vec3 *N);
static int IsInsideBBox(Object *obj, Vec3 *Q);
static void CalcUVMapBBox(Object *obj, Vec3 *P, double *u, double *v);
static void CalcExtentsBBox(Object *obj, Vec3 *omin, Vec3 *omax);
static void TransformBBox(Object *obj, Vec3 *params, int type);
static void CopyBBox(Object *destobj, Object *srcobj);
static void DeleteBBox(Object *obj);
static void DrawBBox(Object *obj);


static ObjectProcs bbox_procs =
{
  OBJ_BBOX,
  IntersectBBox,
  CalcNormalBBox,
  IsInsideBBox,
  CalcUVMapBBox,
  CalcExtentsBBox,
  TransformBBox,
  CopyBBox,
  DeleteBBox,
  DrawBBox
};



/*
 * Objects that are too big for the optimizer, such as infinite planes
 * or anything greater than TOO_BIG, defined below, will be culled out
 * and placed in this list.
 */
#define TOO_BIG 1000000L
static Object *exclusions;

void Ray_BuildBounds(Object **root)
{
  long max_list_size, bound_list_size;
  Object *obj_list, *o, *prev;
  Object *bound_list, *bound;
  Object *new_list, *new_bound;
  Vec3 omin, omax;
  BBoxData *bb;

  /* Do we have anything? */
  if(ray_max_cluster_size < 2 || *root == NULL)
    return;

  /*
   * Objects that are not included in bounding volumes
   * such as infinite planes or anything with no definite extents.
   */
  exclusions = NULL;

  /* Do we have enough objects? */
  max_list_size = 0L;
  o = *root;
  prev = NULL;
  while(o != NULL)
  {
    (o->procs->CalcExtents)(o, &omin, &omax);
    if(omin.x < -TOO_BIG || omax.x > TOO_BIG ||
       omin.y < -TOO_BIG || omax.y > TOO_BIG ||
       omin.z < -TOO_BIG || omax.z > TOO_BIG)
    {
      Object *tmpo;

      if(prev == NULL)  /* First object, start list after "o". */
        *root = o->next;
      else              /* Bridge list over "o". */
        prev->next = o->next;
      tmpo = o;
      o = o->next;
      tmpo->next = exclusions;
      exclusions = tmpo;
    }
    else
    {
      max_list_size++;
      prev = o;
      o = o->next;
    }
  }

  if(*root == NULL) /* Everything was excluded. */
  {
    *root = exclusions;
    return;
  }

  if(max_list_size < ray_bound_threshold ||
       max_list_size <= ray_max_cluster_size)
  {
    /* Seek end of list and tack on excluded objects. */
    o = *root;
    while(o->next != NULL)
      o = o->next;
    o->next = exclusions;
    return;
  }

  obj_list = *root;

  /*
   * Sort, divide and bound object lists until they are all
   * within ray_max_cluster_size. If the list of bounding boxes
   * exceeds ray_max_cluster_size, sort, divide & bound it.
   */
  while(1)
  {
    /* Make initial bounding box. */
    bound_list = Ray_MakeBBox(obj_list);
    max_list_size = bound_list->data.bbox->num_objects;
    while(max_list_size > ray_max_cluster_size)
    {
      max_list_size = 0L;
      bound_list_size = 0L;
      new_bound = NULL;
      new_list = NULL;
      bound = bound_list;
      while(1)
      {
        bb = bound->data.bbox;
        if(bb->num_objects > ray_max_cluster_size)
        {
          DivideObjectList(&bb->objects, &obj_list);
          Ray_SetBBox(bb);
          max_list_size = max(max_list_size, bb->num_objects);

          if(obj_list != NULL)
          {
            /* Make a new bbox for new obj. list. */
            if(new_bound == NULL)
            {
              new_bound = Ray_MakeBBox(obj_list);
              new_list = new_bound;
            }
            else
            {
              new_bound->next = Ray_MakeBBox(obj_list);
              new_bound = new_bound->next;
            }
            bb = new_bound->data.bbox;
            max_list_size = max(max_list_size, bb->num_objects);
          }
        }
        else
        {
          max_list_size = max(max_list_size, bb->num_objects);
        }
        bound_list_size++;

        if(bound->next == NULL)
        {
          /* Add new bbox list to old one and break... */
          bound->next = new_list;
          break;   /* ...from loop */
        }

        bound = bound->next;

      }
    }

    if(bound_list_size <= ray_max_cluster_size)
      break;

    /* Sort, divide and bound the bounding box list. */
    obj_list = bound_list;
  }

  /* Seek end of list and tack on excluded objects. */
  bound = bound_list;
  while(bound->next != NULL)
    bound = bound->next;
  bound->next = exclusions;

  *root = bound_list;

}

/*
 * Divide the object list "olist" along axis of greatest variation,
 * using median cut, and create a new object list, "new_olist".
 * If a second list can not be created, all objects in the list have
 * the same median on all three axes, so an alternate sorting algorithm,
 * DivideObjectList2(), is called.
 */
static void DivideObjectList(Object **olist, Object **new_olist)
{
  Object *lo, *hi, *o, *tmpo;
  Vec3 omin, omax, median, medlo, medhi;
  double width, axis_median, obj_median, tmp;
  long nobj;
  int axis;

  /*
   * Determine axis along which objects in the list are
   * most widely distributed, and get median...
   */
  o = *olist;
  assert(o != NULL);
  V3Set(&medlo, HUGE, HUGE, HUGE);
  V3Set(&medhi, -HUGE, -HUGE, -HUGE);
  V3Set(&median, 0.0, 0.0, 0.0);

  nobj = 0L;
  while(o != NULL)
  {
    (o->procs->CalcExtents)(o, &omin, &omax);

    tmp =(omin.x + omax.x) / 2.0;
    medlo.x = min(medlo.x, tmp);
    medhi.x = max(medhi.x, tmp);
    median.x += tmp;
    tmp =(omin.y + omax.y) / 2.0;
    medlo.y = min(medlo.y, tmp);
    medhi.y = max(medhi.y, tmp);
    median.y += tmp;
    tmp =(omin.z + omax.z) / 2.0;
    medlo.z = min(medlo.z, tmp);
    medhi.z = max(medhi.z, tmp);
    median.z += tmp;

    nobj++;
    o = o->next;
  }

  median.x /= (double)nobj;
  median.y /= (double)nobj;
  median.z /= (double)nobj;

  width = medhi.x - medlo.x;
  axis_median = median.x;
  axis = 0;
  if(medhi.y - medlo.y > width)
  {
    width = medhi.y - medlo.y;
    axis_median = median.y;
    axis = 1;
  }
  if(medhi.z - medlo.z > width)
  {
    axis_median = median.z;
    axis = 2;
  }

  /*
   * Divide up the object list in to two sub-lists.
   * Objects that fall below the median for the dominant axis go in "lo",
   * and those at or above median, in "hi".
   */
  o = *olist;
  lo = NULL;
  hi = NULL;
  while(o != NULL)
  {
    tmpo = o;
    o = o->next;
    (tmpo->procs->CalcExtents)(tmpo, &omin, &omax);
    switch(axis)
    {
      case 0:  obj_median =(omin.x + omax.x) / 2.0; break;
      case 1:  obj_median =(omin.y + omax.y) / 2.0; break;
      default: obj_median =(omin.z + omax.z) / 2.0; break;
    }
    if(obj_median > axis_median)
    {
      tmpo->next = hi;  /* NULL if first. */
      hi = tmpo;
    }
    else
    {
      tmpo->next = lo;  /* NULL if first. */
      lo = tmpo;
    }
  }

  /*
   * If list didn't get split, one of the sub-lists will be NULL
   * split the non-NULL sub-list in half and give half to the NULL
   * sub-list.
   * Happens when all objects in list are of the same extents and
   * in the same place along the dominant axis.
   */
  if(lo == NULL)
  {
    *olist = hi;
    DivideObjectList2(olist, new_olist);
    return;
  }
  if(hi == NULL)
  {
    *olist = lo;
    DivideObjectList2(olist, new_olist);
    return;
  }

  *olist = lo;
  *new_olist = hi;

}


/*
 * Divide the object list "olist" along axis of greatest variation,
 * using median cut, and create a new object list, "new_olist".
 * If a second list can not be created "new_olist" is set to NULL.
 */
static void DivideObjectList2(Object **olist, Object **new_olist)
{
  Object *lo, *hi, *o, *tmpo;
  Vec3 omin, omax, lmin, lmax, lminmax, lmaxmin;
  double delta, split_point, tmp;
  long nobj;
  int axis, low;

  /*
   * Determine axis along which objects in the list are
   * most widely distributed, and get median...
   */
  o = *olist;
  assert(o != NULL);
  V3Set(&lmin, HUGE, HUGE, HUGE);
  V3Set(&lmax, -HUGE, -HUGE, -HUGE);
  V3Set(&lmaxmin, HUGE, HUGE, HUGE);
  V3Set(&lminmax, -HUGE, -HUGE, -HUGE);

  nobj = 0L;
  while(o != NULL)
  {
    (o->procs->CalcExtents)(o, &omin, &omax);

    lmin.x = min(lmin.x, omin.x);
    lmin.y = min(lmin.y, omin.y);
    lmin.z = min(lmin.z, omin.z);
    lmax.x = max(lmax.x, omax.x);
    lmax.y = max(lmax.y, omax.y);
    lmax.z = max(lmax.z, omax.z);
    lminmax.x = max(lminmax.x, omin.x);
    lminmax.y = max(lminmax.y, omin.y);
    lminmax.z = max(lminmax.z, omin.z);
    lmaxmin.x = min(lmaxmin.x, omax.x);
    lmaxmin.y = min(lmaxmin.y, omax.y);
    lmaxmin.z = min(lmaxmin.z, omax.z);

    nobj++;
    o = o->next;
  }


  delta = lminmax.x - lmin.x;
  tmp = lmax.x - lmaxmin.x;
  if(tmp > delta)
  {
    axis = 0;
    low = 1;
    delta = tmp;
    split_point =(lmin.x + lminmax.x) / 2.0;
  }
  else
  {
    axis = 0;
    low = 0;
    delta = tmp;
    split_point =(lmaxmin.x + lmax.x) / 2.0;
  }

  tmp = lminmax.y - lmin.y;
  if(tmp > delta)
  {
    axis = 1;
    low = 1;
    delta = tmp;
    split_point =(lmin.y + lminmax.y) / 2.0;
  }
  tmp = lmax.y - lmaxmin.y;
  if(tmp > delta)
  {
    axis = 1;
    low = 0;
    delta = tmp;
    split_point =(lmaxmin.y + lmax.y) / 2.0;
  }

  tmp = lminmax.z - lmin.z;
  if(tmp > delta)
  {
    axis = 2;
    low = 1;
    delta = tmp;
    split_point =(lmin.z + lminmax.z) / 2.0;
  }
  tmp = lmax.z - lmaxmin.z;
  if(tmp > delta)
  {
    axis = 2;
    low = 0;
    split_point =(lmaxmin.z + lmax.z) / 2.0;
  }

  /*
   * Divide up the object list in to two sub-lists.
   * Objects that fall below the median for the dominant axis go in "lo",
   * and those at or above median, in "hi".
   */
  o = *olist;
  lo = NULL;
  hi = NULL;
  while(o != NULL)
  {
    double objpt;

    tmpo = o;
    o = o->next;
    (tmpo->procs->CalcExtents)(tmpo, &omin, &omax);
    switch(axis)
    {
      case 0:  objpt =(low == 0) ? omax.x : omin.x; break;
      case 1:  objpt =(low == 0) ? omax.y : omin.y; break;
      default: objpt =(low == 0) ? omax.z : omin.z; break;
    }
    if(objpt > split_point)
    {
      tmpo->next = hi;  /* NULL if first. */
      hi = tmpo;
    }
    else
    {
      tmpo->next = lo;  /* NULL if first. */
      lo = tmpo;
    }
  }

  /*
   * If list didn't get split, one of the sub-lists will be NULL
   * split the non-NULL sub-list in half and give half to the NULL
   * sub-list.
   * Happens when all objects in list are of the same extents and
   * in the same place along the dominant axis.
   */
  if(lo == NULL)
  {
    long c;

    assert(hi != NULL);
    lo = hi;
    for(c = nobj/2L; c > 0L; c--)
      lo = lo->next;
    tmpo = lo;
    lo = lo->next;
    tmpo->next = NULL;
    /* log_warning("Bound: Invariant object list (lo)."); */
  }
  if(hi == NULL)
  {
    long c;

    assert(lo != NULL);
    hi = lo;
    for(c = nobj/2L; c > 0L; c--)
      hi = hi->next;
    tmpo = hi;
    hi = hi->next;
    tmpo->next = NULL;
    /* log_warning("Bound: Invariant object list (hi)."); */
  }

  *olist = lo;
  *new_olist = hi;

}



/*
 * Get number of objects in, and extents of,
 * object list on bounding box.
 */
void Ray_SetBBox(BBoxData *bbox)
{
  Object *o;
  Vec3 omin, omax;
  long nobj;

  assert(bbox != NULL);
  o = bbox->objects;
  assert(o != NULL);

  bbox->bmin.x = HUGE;
  bbox->bmax.x = -HUGE;
  bbox->bmin.y = HUGE;
  bbox->bmax.y = -HUGE;
  bbox->bmin.z = HUGE;
  bbox->bmax.z = -HUGE;

  nobj = 0L;

  while(o != NULL)
  {
    (o->procs->CalcExtents)(o, &omin, &omax);
    bbox->bmin.x = min(bbox->bmin.x, omin.x) - EPSILON;
    bbox->bmax.x = max(bbox->bmax.x, omax.x) + EPSILON;
    bbox->bmin.y = min(bbox->bmin.y, omin.y) - EPSILON;
    bbox->bmax.y = max(bbox->bmax.y, omax.y) + EPSILON;
    bbox->bmin.z = min(bbox->bmin.z, omin.z) - EPSILON;
    bbox->bmax.z = max(bbox->bmax.z, omax.z) + EPSILON;
    nobj++;
    o = o->next;
  }

  bbox->num_objects = nobj;

}

/*
 * Allocate and initialize a new Object and BBoxData structure.
 */
Object *Ray_MakeBBox(Object *obj_list)
{
  Object *newobj;
  BBoxData *newbb;

  assert(obj_list != NULL);

  newobj = NewObject();
  newbb = (BBoxData *)Malloc(sizeof(BBoxData));

  /* Some assembly required. */
  newobj->procs = &bbox_procs;
  newobj->data.bbox = newbb;
  newobj->next = NULL;
  newbb->objects = obj_list;
  Ray_SetBBox(newbb);
  ray_num_bounds++;

  return newobj;
}

static int IntersectBBox(Object *obj, HitData *hits)
{
  BBoxData *bb;
  double t1, t2;

  bb = obj->data.bbox;

  if(Intersect_Box(&ct.B, &ct.D, &bb->bmin, &bb->bmax, &t1, &t2))
  {
    if(t1 < ct.tmax)
    {
      if(t1 < ct.tmin)
        t1 = ct.tmin + EPSILON;
      if(t1 < ct.tmax)
      {
        hits->obj = obj;
        hits->t = t1;
        return 1;
      }
    }
  }

  return 0;

}

void CalcExtentsBBox(Object *obj, Vec3 *omin, Vec3 *omax)
{
  BBoxData *bb = obj->data.bbox;

  *omin = bb->bmin;
  *omax = bb->bmax;
} /* end of CalcExtentsBBox() */


void CalcNormalBBox(Object *obj, Vec3 *P, Vec3 *N)
{
}


void CalcUVMapBBox(Object *obj, Vec3 *P, double *u, double *v)
{
  *u = *v = 0.0;
}


int IsInsideBBox(Object *obj, Vec3 *P)
{
  BBoxData * bb = obj->data.bbox;
  Object *o;

  if(P->x < bb->bmin.x || P->x > bb->bmax.x)
    return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */
  if(P->y < bb->bmin.y || P->y > bb->bmax.y)
    return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */
  if(P->z < bb->bmin.z || P->z > bb->bmax.z)
    return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */

  /* We are "inside" if point is inside any of the bounded objects. */
  for(o = bb->objects; o != NULL; o = o->next)
    if(o->procs->IsInside(o, P))
      break;

  return (o != NULL) ?
    (!(obj->flags & OBJ_FLAG_INVERSE)) :
    (obj->flags & OBJ_FLAG_INVERSE);
}


void TransformBBox(Object *obj, Vec3 *params, int type)
{
  if(obj->T == NULL)
    obj->T = Ray_NewXform();
  XformXforms(obj->T, params, type);
}


void CopyBBox(Object *destobj, Object *srcobj)
{
  BBoxData *srcbb = srcobj->data.bbox;
  BBoxData *destbb = (BBoxData *)Malloc(sizeof(BBoxData));
  Object *srco, *desto;

  if(destbb == NULL)
    return;

  *destbb = *srcbb;
  desto = NULL;
  for(srco = srcbb->objects; srco != NULL; srco = srco->next)
  {
    if(desto != NULL)
    {
      desto->next = Ray_CloneObject(srco);
      desto = desto->next;
    }
    else
    {
      desto = Ray_CloneObject(srco);
      destbb->objects = desto;
    }
    if(desto == NULL)
      return;
  }
  destobj->data.bbox = destbb;
}


void DeleteBBox(Object *obj)
{
  BBoxData *bb = obj->data.bbox;
  Object *o;

  while(bb->objects != NULL)
  {
    o = bb->objects;
    bb->objects = o->next;
    Ray_DeleteObject(o);
  }
  Free(bb, sizeof(BBoxData));
}


void PostProcessBBoxChild(Object *parent, Object *obj)
{
  if(obj->T == NULL)
    obj->T = Ray_CopyXform(parent->T);
  else if(parent->T != NULL)
    ConcatXforms(obj->T, parent->T);
  Ray_PostProcessObject(obj);
}


void PostProcessBBox(Object *obj)
{
  BBoxData *bb = obj->data.bbox;
  Object *o;

  for(o = bb->objects; o != NULL; o = o->next)
    PostProcessBBoxChild(obj, o);

  Ray_SetBBox(bb);
  Ray_BuildBounds(&bb->objects);
}


void BBox_GetTextureInfo(Object *obj, Surface **surf, Xform **T)
{
}


/*************************************************************************
*
*  Draws a wire frame view of object.
*
*************************************************************************/

void DrawBBox(Object *obj)
{
  BBoxData *bb = obj->data.bbox;
  Object *o;

  /* TODO: Option for drawing bounding box itself. */

  for(o = bb->objects; o != NULL; o = o->next)
    o->procs->Draw(o);
}
