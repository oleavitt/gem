/*************************************************************************
*
*  box.c
*
*  The box primitive.
*
*************************************************************************/

#include "ray.h"

#define is_equal(a,b) (fabs((a)-(b)) < 0.0001)

unsigned long ray_box_tests;
unsigned long ray_box_hits;

static int IntersectBox(Object *obj, HitData *hits);
static void CalcNormalBox(Object *obj, Vec3 *P, Vec3 *N);
static int IsInsideBox(Object *obj, Vec3 *P);
static void CalcUVMapBox(Object *obj, Vec3 *P, double *u, double *v);
static void CalcExtentsBox(Object *obj, Vec3 *omin, Vec3 *omax);
static void TransformBox(Object *obj, Vec3 *params, int type);
static void CopyBox(Object *destobj, Object *srcobj);
static void DeleteBox(Object *obj);
static void DrawBox(Object *obj);

static ObjectProcs box_procs =
{
	OBJ_BOX,
	IntersectBox,
	CalcNormalBox,
	IsInsideBox,
	CalcUVMapBox,
	CalcExtentsBox,
	TransformBox,
	CopyBox,
	DeleteBox,
	DrawBox
};



void Ray_SetBox(BoxData *box, Vec3 *bmin, Vec3 *bmax)
{
	if (bmin->x < bmax->x)
	{
		box->x1 = (float) bmin->x;
		box->x2 = (float) bmax->x;
	}
	else
	{
		box->x1 = (float) bmax->x;
		box->x2 = (float) bmin->x;
	}

	if (bmin->y < bmax->y)
	{
		box->y1 = (float) bmin->y;
		box->y2 = (float) bmax->y;
	}
	else
	{
		box->y1 = (float) bmax->y;
		box->y2 = (float) bmin->y;
	}

	if (bmin->z < bmax->z)
	{
		box->z1 = (float) bmin->z;
		box->z2 = (float) bmax->z;
	}
	else
	{
		box->z1 = (float) bmax->z;
		box->z2 = (float) bmin->z;
	}
}



Object *Ray_MakeBox(Vec3 *bmin, Vec3 *bmax)
{
	Object *obj = NewObject();
	if (obj != NULL)
	{
		BoxData *box = (BoxData *) Malloc(sizeof(BoxData));
		if (box != NULL)
		{
			Ray_SetBox(box, bmin, bmax);
			obj->data.box = box;
			obj->procs = &box_procs;
		}
		else
		 	obj = Ray_DeleteObject(obj);
	}

	return obj;
}


int IntersectBox(Object *obj, HitData *hits)
{
	BoxData *box;
	Vec3 B, D, bmin, bmax;
	double t1, t2;

	box = obj->data.box;

	ray_box_tests++;

	/*
	 * Transform ray base point and direction cosines to box's coordinate
	 * system.
	 */
	B = ct.B;
	D = ct.D;
	if(obj->T != NULL)
	{
		PointToObject(&B, obj->T);
		DirToObject(&D, obj->T);
	}

  V3Set(&bmin, box->x1, box->y1, box->z1);
  V3Set(&bmax, box->x2, box->y2, box->z2);
	if(Intersect_Box(&B, &D, &bmin, &bmax, &t1, &t2))
	{
		if((t2 > ct.tmin)&&(t1 < ct.tmax))
		{
			if(t1 > ct.tmin)
			{
				ray_box_hits++;
				hits->obj = obj;
				hits->t = t1;
				hits->entering = !(obj->flags & OBJ_FLAG_INVERSE);
				if(t2 < ct.tmax)
				{
					hits = GetNextHit(hits);
					hits->obj = obj;
					hits->t = t2;
					hits->entering = (obj->flags & OBJ_FLAG_INVERSE);
					return 2;
				}
        return 1;
			}
			else if(t2 < ct.tmax)
			{
				ray_box_hits++;
				hits->obj = obj;
				hits->t = t2;
				hits->entering = (obj->flags & OBJ_FLAG_INVERSE);
				return 1;
			}
		}
	}
	return 0;
}


int Intersect_Box(Vec3 *B, Vec3 *D, Vec3 *bmin, Vec3 *bmax,
	double *T1, double *T2)
{
	Vec3 tmin, tmax;
	double t1, t2;

	if(fabs(D->x) > EPSILON)
	{
		t1 = (bmin->x - B->x) / D->x;
		t2 = (bmax->x - B->x) / D->x;
		if(t1 < EPSILON && t2 < EPSILON)
			return 0;              /* Behind ya. */
		tmin.x = min(t1, t2);
		tmax.x = max(t1, t2);
	}
	else  /* Ray is parallel to X planes. */
	{
		if(B->x < bmin->x || B->x > bmax->x)
			return 0;        /* Outside of X plane pair. */
		tmin.x = -HUGE;
		tmax.x = HUGE;
	}

	if(fabs(D->y) > EPSILON)
	{
		t1 = (bmin->y - B->y) / D->y;
		t2 = (bmax->y - B->y) / D->y;
		if(t1 < EPSILON && t2 < EPSILON)
			return 0;              /* Behind ya. */
		tmin.y = min(t1, t2);
		tmax.y = max(t1, t2);
	}
	else  /* Ray is parallel to Y planes. */
	{
		if(B->y < bmin->y || B->y > bmax->y)
			return 0;        /* Outside of Y plane pair. */
		tmin.y = -HUGE;
		tmax.y = HUGE;
	}

	if(fabs(D->z) > EPSILON)
	{
		t1 =(bmin->z - B->z) / D->z;
		t2 =(bmax->z - B->z) / D->z;
		if(t1 < EPSILON && t2 < EPSILON)
			return 0;              /* Behind ya. */
		tmin.z = min(t1, t2);
		tmax.z = max(t1, t2);
	}
	else  /* Ray is parallel to Z planes. */
	{
		if(B->z < bmin->z || B->z > bmax->z)
			return 0;        /* Outside of Z plane pair. */
		tmin.z = -HUGE;
		tmax.z = HUGE;
	}

	t1 = tmin.x;
	t2 = tmax.x;
	t1 = max(t1, tmin.y);
	t2 = min(t2, tmax.y);
	t1 = max(t1, tmin.z);
	t2 = min(t2, tmax.z);

	if(t1 < t2)
	{
		*T1 = t1;
		*T2 = t2;
		return 1;
	}
	return 0;
}

void CalcNormalBox(Object *obj, Vec3 *P, Vec3 *N)
{
	BoxData *box;
	Vec3 Pt;

	box = obj->data.box;

	/*
	 * Transform world point, "P", to point in box's coordinate
	 * system, "Pt".
	 */
	Pt = *P;
	if(obj->T != NULL)
		PointToObject(&Pt, obj->T);

	V3Set(N, 0.0, 0.0, 0.0);
	if(is_equal(Pt.x, box->x2))
		N->x = 1.0;
	else if(is_equal(Pt.x, box->x1))
		N->x = -1.0;
	else if(is_equal(Pt.y, box->y2))
		N->y = 1.0;
	else if(is_equal(Pt.y, box->y1))
		N->y = -1.0;
	else if(is_equal(Pt.z, box->z2))
		N->z = 1.0;
	else
		N->z = -1.0;

	if(obj->T != NULL)
		NormToWorld(N, obj->T);
}


int IsInsideBox(Object *obj, Vec3 *P)
{
	BoxData *box = obj->data.box;
	Vec3 Pt = *P;
  double fudge = (obj->flags & OBJ_FLAG_INVERSE) ? EPSILON : -EPSILON;

	/*
	 * Transform world point, "P", to point in box's coordinate
	 * system, "Pt".
	 */
	if(obj->T != NULL)
		PointToObject(&Pt, obj->T);

	if(Pt.x < box->x1 - fudge || Pt.x > box->x2 + fudge)
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */

	if(Pt.y < box->y1 - fudge || Pt.y > box->y2 + fudge)
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */

	if(Pt.z < box->z1 - fudge || Pt.z > box->z2 + fudge)
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */

	return (! (obj->flags & OBJ_FLAG_INVERSE)); /* inside */
}


void CalcUVMapBox(Object *obj, Vec3 *P, double *u, double *v)
{
	/* TODO: How about an environment mapping function here?... */
	*u = *v = 0.0;

	/* Not used */
	P; obj;
}


void CalcExtentsBox(Object *obj, Vec3 *omin, Vec3 *omax)
{
	BoxData *box;

	box = obj->data.box;

  omin->x = box->x1;
  omax->x = box->x2;
  omin->y = box->y1;
  omax->y = box->y2;
  omin->z = box->z1;
  omax->z = box->z2;

	omin->x -= EPSILON;
	omin->y -= EPSILON;
	omin->z -= EPSILON;
	omax->x += EPSILON;
	omax->y += EPSILON;
	omax->z += EPSILON;

	if(obj->T != NULL)
		BBoxToWorld(omin, omax, obj->T);
}


void TransformBox(Object *obj, Vec3 *params, int type)
{
/*
  BoxData *b = obj->data.box;
  float ftmp;
*/
	if (obj->T == NULL)
		obj->T = Ray_NewXform();
	XformXforms(obj->T, params, type);
/*
	if (obj->T == NULL)
	{
		switch(type)
		{
			case XFORM_ROTATE:
			case XFORM_SHEAR:
				obj->T = Ray_NewXform();
				XformXforms(obj->T, params, type);
				break;
			case XFORM_TRANSLATE:
				b->x1 += (float)params->x;
				b->x2 += (float)params->x;
				b->y1 += (float)params->y;
				b->y2 += (float)params->y;
				b->z1 += (float)params->z;
				b->z2 += (float)params->z;
				break;
			case XFORM_SCALE:
				b->x1 *= (float)params->x;
				b->x2 *= (float)params->x;
				b->y1 *= (float)params->y;
				b->y2 *= (float)params->y;
				b->z1 *= (float)params->z;
				b->z2 *= (float)params->z;
				if(b->x1 > b->x2) { ftmp = b->x1; b->x1 = b->x2; b->x2 = ftmp; }
				if(b->y1 > b->y2) { ftmp = b->y1; b->y1 = b->y2; b->y2 = ftmp; }
				if(b->z1 > b->z2) { ftmp = b->z1; b->z1 = b->z2; b->z2 = ftmp; }
				break;
		}
	}
	else
		XformXforms(obj->T, params, type);
*/
}


void CopyBox(Object *destobj, Object *srcobj)
{
	if((destobj->data.box = (BoxData *)Malloc(sizeof(BoxData))) != NULL)
	  memcpy(destobj->data.box, srcobj->data.box, sizeof(BoxData));
}


void DeleteBox(Object *obj)
{
	Free(obj->data.box, sizeof(BoxData));
}


/*************************************************************************
*
*  Draws a wire frame view of object.
*
*************************************************************************/

void DrawBox(Object *obj)
{
  BoxData *b = obj->data.box;
  Vec3 pt;

	if(obj->T != NULL)
	{
		V3Set(&pt, b->x1, b->y1, b->z1);
		PointToWorld(&pt, obj->T);
		Set_Pt(0, pt.x, pt.y, pt.z);
		V3Set(&pt, b->x2, b->y1, b->z1);
		PointToWorld(&pt, obj->T);
		Set_Pt(1, pt.x, pt.y, pt.z);
		V3Set(&pt, b->x1, b->y2, b->z1);
		PointToWorld(&pt, obj->T);
		Set_Pt(2, pt.x, pt.y, pt.z);
		V3Set(&pt, b->x2, b->y2, b->z1);
		PointToWorld(&pt, obj->T);
		Set_Pt(3, pt.x, pt.y, pt.z);
		V3Set(&pt, b->x1, b->y1, b->z2);
		PointToWorld(&pt, obj->T);
		Set_Pt(4, pt.x, pt.y, pt.z);
		V3Set(&pt, b->x2, b->y1, b->z2);
		PointToWorld(&pt, obj->T);
		Set_Pt(5, pt.x, pt.y, pt.z);
		V3Set(&pt, b->x1, b->y2, b->z2);
		PointToWorld(&pt, obj->T);
		Set_Pt(6, pt.x, pt.y, pt.z);
		V3Set(&pt, b->x2, b->y2, b->z2);
		PointToWorld(&pt, obj->T);
		Set_Pt(7, pt.x, pt.y, pt.z);
	}
	else
	{
		Set_Pt(0, b->x1, b->y1, b->z1);
		Set_Pt(1, b->x2, b->y1, b->z1);
		Set_Pt(2, b->x1, b->y2, b->z1);
		Set_Pt(3, b->x2, b->y2, b->z1);
		Set_Pt(4, b->x1, b->y1, b->z2);
		Set_Pt(5, b->x2, b->y1, b->z2);
		Set_Pt(6, b->x1, b->y2, b->z2);
		Set_Pt(7, b->x2, b->y2, b->z2);
	}
	Move_To(0);
	Line_To(1);
	Line_To(3);
	Line_To(2);
	Line_To(0);
	Line_To(4);
	Line_To(5);
	Line_To(7);
	Line_To(6);
	Line_To(4);
	Move_To(1);
	Line_To(5);
	Move_To(3);
	Line_To(7);
	Move_To(2);
	Line_To(6);
}
