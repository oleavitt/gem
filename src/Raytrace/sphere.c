/*************************************************************************
*
*  sphere.c
*
*  The sphere primitive.
*
*************************************************************************/

#include "ray.h"

static int IntersectSphere(Object *obj, HitData *hits);
static void CalcNormalSphere(Object *obj, Vec3 *P, Vec3 *N);
static int IsInsideSphere(Object *obj, Vec3 *P);
static void CalcUVMapSphere(Object *obj, Vec3 *P, double *u, double *v);
static void CalcExtentsSphere(Object *obj, Vec3 *omin, Vec3 *omax);
static void TransformSphere(Object *obj, Vec3 *params, int type);
static void CopySphere(Object *destobj, Object *srcobj);
static void DeleteSphere(Object *obj);
static void DrawSphere(Object *obj);

unsigned long ray_sphere_tests;
unsigned long ray_sphere_hits;

static ObjectProcs sphere_procs =
{
	OBJ_SPHERE,
	IntersectSphere,
	CalcNormalSphere,
	IsInsideSphere,
	CalcUVMapSphere,
	CalcExtentsSphere,
	TransformSphere,
	CopySphere,
	DeleteSphere,
	DrawSphere
};



void Ray_SetSphere(SphereData *sphere, Vec3 *loc, double rad)
{
	sphere->x = (float) loc->x;
	sphere->y = (float) loc->y;
	sphere->z = (float) loc->z;
	sphere->r = (float) (rad * rad);
}



Object *Ray_MakeSphere(Vec3 *loc, double rad)
{
	Object *obj = NewObject();
	if (obj != NULL)
	{
		SphereData *sphere = (SphereData *) Malloc(sizeof(SphereData));
		if (sphere != NULL)
		{
			Ray_SetSphere(sphere, loc, rad);
			obj->data.sphere = sphere;
			obj->procs = &sphere_procs;
		}
		else
			obj = Ray_DeleteObject(obj);
	}

	return obj;
}


int IntersectSphere(Object *obj, HitData *hits)
{
  SphereData *s;
  Vec3 B, D;
  double a, b, c, d, t1, t2;

	ray_sphere_tests++;
  s = obj->data.sphere;
  B = ct.B;
  D = ct.D;
	if(obj->T != NULL)
	{
		PointToObject(&B, obj->T);
		DirToObject(&D, obj->T);
	}
	B.x -= s->x;
	B.y -= s->y;
	B.z -= s->z;
  a = V3Dot(&D, &D);
  b = V3Dot(&D, &B) * 2.0;
  c = V3Dot(&B, &B) - s->r;
	d = b * b - 4.0 * a * c;
	if(d < 0.0)
  	return 0;
	d = sqrt(d);
	t1 = (-b + d) / (2.0 * a);
	t2 = (-b - d) / (2.0 * a);

	if(t1 > t2)
	{
		d = t1;
		t1 = t2;
		t2 = d;
	}
	if((t2 > ct.tmin)&&(t1 < ct.tmax))
	{
		if(t1 > ct.tmin)
		{
			ray_sphere_hits++; 
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
			ray_sphere_hits++;
			hits->obj = obj;
			hits->t = t2;
			hits->entering = (obj->flags & OBJ_FLAG_INVERSE);
			return 1;
		}
	}
	return 0;
}


void CalcNormalSphere(Object *obj, Vec3 *P, Vec3 *N)
{
	SphereData *s;
	Vec3 PT;

	s = obj->data.sphere;

	PT = *P;
	if(obj->T != NULL)
		PointToObject(&PT, obj->T);
	N->x = PT.x - s->x;
	N->y = PT.y - s->y;
	N->z = PT.z - s->z;
	if(obj->T != NULL)
		NormToWorld(N, obj->T);
  else
  	V3Normalize(N);
}


int IsInsideSphere(Object *obj, Vec3 *P)
{
	SphereData *s;
	Vec3 Pt;

	s = obj->data.sphere;

	Pt = *P;
	if(obj->T != NULL)
		PointToObject(&Pt, obj->T);
  Pt.x -= s->x;
  Pt.y -= s->y;
  Pt.z -= s->z;
	if(V3Dot(&Pt, &Pt) > s->r)
		return (obj->flags & OBJ_FLAG_INVERSE); /* not inside */
	return (! (obj->flags & OBJ_FLAG_INVERSE)); /* inside */
}


void CalcUVMapSphere(Object *obj, Vec3 *P, double *u, double *v)
{
	SphereData *s = obj->data.sphere;
  Vec3 PO;
	double rad;

	PO.x = P->x - s->x;
	PO.y = P->y - s->y;
	PO.z = P->z - s->z;

	rad = -V3Mag(&PO);
	PO.z /= rad;
	if(fabs(PO.z) > 1.0)
		PO.z = (PO.z < 0.0) ? -1.0 : 1.0;
	*v = acos(PO.z) / PI;

	if(*v > 0.0 && *v < 1.0)
	{
		rad = sqrt(PO.x * PO.x + PO.y * PO.y);
		PO.y /= rad;

		if(fabs(PO.y) > 1.0)
			PO.y = (PO.y < 0.0) ? -1.0 : 1.0;
		*u = acos(PO.y) / TWOPI;
		if(PO.x > 0.0)
			*u = 1.0 - *u;
	}
	else
	{
		*u = 0.0;
	}
}


void CalcExtentsSphere(Object *obj, Vec3 *omin, Vec3 *omax)
{
	SphereData *s;
  double rad;

	s = obj->data.sphere;
  rad = sqrt(s->r);

  omin->x = s->x - rad - EPSILON;
  omax->x = s->x + rad + EPSILON;
  omin->y = s->y - rad - EPSILON;
  omax->y = s->y + rad + EPSILON;
  omin->z = s->z - rad - EPSILON;
  omax->z = s->z + rad + EPSILON;

	if(obj->T != NULL)
		BBoxToWorld(omin, omax, obj->T);

}


void TransformSphere(Object *obj, Vec3 *params, int type)
{
	if (obj->T == NULL)
		obj->T = Ray_NewXform();
	XformXforms(obj->T, params, type);
/*
	if(obj->T == NULL)
 	{
		SphereData *s;

		s = obj->data.sphere;

		if(type == XFORM_SCALE)
		{
			if(V3IsIsotropic(params))
			{
				s->r = (float)sqrt(s->r);
				s->r *= (float)params->x;
				s->r *= s->r;
				s->x *= (float)params->x;
				s->y *= (float)params->y;
				s->z *= (float)params->z;
			}
			else
			{
				obj->T = Ray_NewXform();
		 		XformXforms(obj->T, params, type);
			}
		}
		else if(type == XFORM_SHEAR)
		{
			obj->T = Ray_NewXform();
			XformXforms(obj->T, params, type);
		}
		else
    {
      Vec3 V;
			V3Set(&V, s->x, s->y, s->z);
			XformVector(&V, params, type);
      s->x = (float)V.x;
      s->y = (float)V.y;
      s->z = (float)V.z;
    }
	}
	else
		XformXforms(obj->T, params, type);
*/
}


void CopySphere(Object *destobj, Object *srcobj)
{
	if((destobj->data.sphere = (SphereData *)Malloc(sizeof(SphereData)))
    != NULL)
	  memcpy(destobj->data.sphere, srcobj->data.sphere, sizeof(SphereData));
}


void DeleteSphere(Object *obj)
{
	Free(obj->data.sphere, sizeof(SphereData));
}


/*************************************************************************
*
*  Draws a wire frame view of object.
*
*************************************************************************/

#define SIN30 0.5
#define COS30 0.8660254

static Vec3 spts[12][5] =
{
	{
		{ SIN30, 0.0, COS30 },
		{ COS30, 0.0, SIN30 },
		{ 1.0, 0.0, 0.0 },
		{ COS30, 0.0, -SIN30 },
		{ SIN30, 0.0, -COS30 }
	},
	{
		{ SIN30*COS30, SIN30*SIN30, COS30 },
		{ COS30*COS30, SIN30*COS30, SIN30 },
		{ COS30, SIN30, 0.0 },
		{ COS30*COS30, SIN30*COS30, -SIN30 },
		{ SIN30*COS30, SIN30*SIN30, -COS30 }
	},
	{
		{ SIN30*SIN30, COS30*SIN30, COS30 },
		{ COS30*SIN30, COS30*COS30, SIN30 },
		{ SIN30, COS30, 0.0 },
		{ COS30*SIN30, COS30*COS30, -SIN30 },
		{ SIN30*SIN30, COS30*SIN30, -COS30 }
	},
	{
		{ 0.0, SIN30, COS30 },
		{ 0.0, COS30, SIN30 },
		{ 0.0, 1.0, 0.0 },
		{ 0.0, COS30, -SIN30 },
		{ 0.0, SIN30, -COS30 }
	},
	{
		{ -SIN30*SIN30, COS30*SIN30, COS30 },
		{ -COS30*SIN30, COS30*COS30, SIN30 },
		{ -SIN30, COS30, 0.0 },
		{ -COS30*SIN30, COS30*COS30, -SIN30 },
		{ -SIN30*SIN30, COS30*SIN30, -COS30 }
	},
	{
		{ -SIN30*COS30, SIN30*SIN30, COS30 },
		{ -COS30*COS30, SIN30*COS30, SIN30 },
		{ -COS30, SIN30, 0.0 },
		{ -COS30*COS30, SIN30*COS30, -SIN30 },
		{ -SIN30*COS30, SIN30*SIN30, -COS30 }
	},
	{
		{ -SIN30, 0.0, COS30 },
		{ -COS30, 0.0, SIN30 },
		{ -1.0, 0.0, 0.0 },
		{ -COS30, 0.0, -SIN30 },
		{ -SIN30, 0.0, -COS30 }
	},
	{
		{ -SIN30*COS30, -SIN30*SIN30, COS30 },
		{ -COS30*COS30, -SIN30*COS30, SIN30 },
		{ -COS30, -SIN30, 0.0 },
		{ -COS30*COS30, -SIN30*COS30, -SIN30 },
		{ -SIN30*COS30, -SIN30*SIN30, -COS30 }
	},
	{
		{ -SIN30*SIN30, -COS30*SIN30, COS30 },
		{ -COS30*SIN30, -COS30*COS30, SIN30 },
		{ -SIN30, -COS30, 0.0 },
		{ -COS30*SIN30, -COS30*COS30, -SIN30 },
		{ -SIN30*SIN30, -COS30*SIN30, -COS30 }
	},
	{
		{ 0.0, -SIN30, COS30 },
		{ 0.0, -COS30, SIN30 },
		{ 0.0, -1.0, 0.0 },
		{ 0.0, -COS30, -SIN30 },
		{ 0.0, -SIN30, -COS30 }
	},
	{
		{ SIN30*SIN30, -COS30*SIN30, COS30 },
		{ COS30*SIN30, -COS30*COS30, SIN30 },
		{ SIN30, -COS30, 0.0 },
		{ COS30*SIN30, -COS30*COS30, -SIN30 },
		{ SIN30*SIN30, -COS30*SIN30, -COS30 }
	},
	{
		{ SIN30*COS30, -SIN30*SIN30, COS30 },
		{ COS30*COS30, -SIN30*COS30, SIN30 },
		{ COS30, -SIN30, 0.0 },
		{ COS30*COS30, -SIN30*COS30, -SIN30 },
		{ SIN30*COS30, -SIN30*SIN30, -COS30 }
	}
};

void DrawSphere(Object *obj)
{
	SphereData *s;
	Vec3 P;
	int i, n, n2;
	double rad;

	s = obj->data.sphere;
	rad = sqrt(s->r);

	V3Set(&P, s->x, s->y, s->z + rad);
	if(obj->T != NULL)
		PointToWorld(&P, obj->T);
	Set_Pt(0, P.x, P.y, P.z);
	V3Set(&P, s->x, s->y, s->z - rad);
	if(obj->T != NULL)
		PointToWorld(&P, obj->T);
	Set_Pt(1, P.x, P.y, P.z);

	n = 2;
	for(i = 0; i < 12; i++)
	{
		V3Set(&P, spts[i][0].x * rad + s->x,
		          spts[i][0].y * rad + s->y,
							spts[i][0].z * rad + s->z);
		if(obj->T != NULL)
			PointToWorld(&P, obj->T);
		Set_Pt(n, P.x, P.y, P.z);
		n++;
		V3Set(&P, spts[i][1].x * rad + s->x,
		          spts[i][1].y * rad + s->y,
							spts[i][1].z * rad + s->z);
		if(obj->T != NULL)
			PointToWorld(&P, obj->T);
		Set_Pt(n, P.x, P.y, P.z);
		n++;
		V3Set(&P, spts[i][2].x * rad + s->x,
		          spts[i][2].y * rad + s->y,
							spts[i][2].z * rad + s->z);
		if(obj->T != NULL)
			PointToWorld(&P, obj->T);
		Set_Pt(n, P.x, P.y, P.z);
		n++;
		V3Set(&P, spts[i][3].x * rad + s->x,
		          spts[i][3].y * rad + s->y,
							spts[i][3].z * rad + s->z);
		if(obj->T != NULL)
			PointToWorld(&P, obj->T);
		Set_Pt(n, P.x, P.y, P.z);
		n++;
		V3Set(&P, spts[i][4].x * rad + s->x,
		          spts[i][4].y * rad + s->y,
							spts[i][4].z * rad + s->z);
		if(obj->T != NULL)
			PointToWorld(&P, obj->T);
		Set_Pt(n, P.x, P.y, P.z);
		n++;		
	}

	n = 2;
	n2 = n + 5;
	for(i = 0; i < 12; i++)
	{
		if(i == 11)
			n2 = 2;
		Move_To(0);
		Line_To(n);
		Line_To(n2);
		n++; n2++;
		Line_To(n2);
		Line_To(n);
		n++; n2++;
		Line_To(n);
		Line_To(n2);
		n++; n2++;
		Line_To(n2);
		Line_To(n);
		n++; n2++;
		Line_To(n);
		Line_To(n2);
		n++; n2++;
		Line_To(1);
	}
}
