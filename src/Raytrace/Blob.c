/**************************************************************************
*
*  blob.c
*  The blob primitive and its functions.
*
**************************************************************************/

#include "ray.h"

static int IntersectBlob(Object *obj, HitData *hits);
static void CalcNormalBlob(Object *obj, Vec3 *Q, Vec3 *N);
static int IsInsideBlob(Object *obj, Vec3 *Q);
static void CalcUVMapBlob(Object *obj, Vec3 *P, double *u, double *v);
static void CalcExtentsBlob(Object *obj, Vec3 *omin, Vec3 *omax);
static void TransformBlob(Object *obj, Vec3 *params, int type);
static void CopyBlob(Object *destobj, Object *srcobj);
static void DeleteBlob(Object *obj);
static void DrawBlob(Object *obj);

unsigned long ray_blob_tests;
unsigned long ray_blob_hits;

static ObjectProcs blob_procs =
{
	OBJ_BLOB,
	IntersectBlob,
	CalcNormalBlob,
	IsInsideBlob,
	CalcUVMapBlob,
	CalcExtentsBlob,
	TransformBlob,
	CopyBlob,
	DeleteBlob,
	DrawBlob
};

#define MIN_INTERVAL_SIZE 0.001

static int calc_intervals(BlobData *blob, BlobHit **intervals);
static void calc_substitutions(BlobHit *bi);
static double ox, oy, oz, dx, dy, dz;  /* Ray origin & direction. */
static Vec3 B, D;

/**************************************************************************
*
*  Called first to create an empty blob object with a given threshold.
*    Elements are added by calling Ray_BlobAddSphere(),
*  Ray_BlobAddCylinder() and Ray_BlobAddPlane() one or more times on this
*  object. When the object is finished Ray_BlobFinish() is called to
*  perform any needed post-processing and validation.
*
**************************************************************************/
Object *Ray_MakeBlob(double threshold)
{
	Object *obj;
	BlobData *blob;

	if((obj = NewObject()) == NULL)
		return NULL;

	/* Allocate the main blob data structure... */
	if((blob = (BlobData *)Malloc(sizeof(BlobData))) == NULL)
	{
		Ray_DeleteObject(obj);
		return NULL;
	}
	blob->nrefs = 1;
	blob->threshold = threshold;
	blob->solver = 0;
	blob->bound = NULL;
	blob->elems = NULL;

	obj->data.blob = blob;
	obj->procs = &blob_procs;

	return obj;
}

int Ray_BlobAddSphere(Object *obj, Vec3 *pt, double radius,
	double field)
{
	Bloblet *be;
	BlobData *blob = obj->data.blob;

	assert(obj != NULL);
	assert(blob != NULL);
	
	if ((be = (Bloblet *)Malloc(sizeof(Bloblet))) == NULL)
		return 0;

	be->next = NULL;	
	V3Copy(&be->loc, pt);
	be->rad = radius;
	be->field = field;

	/*
	 * Pre-compute constants for the density eq. in standard form.
	 * Radii that are too small should be checked for during parse.
	 */
	be->rsq = be->rad * be->rad;
	be->r2 = - (2.0 * be->field) / be->rsq;
	be->r4 = be->field / (be->rsq * be->rsq);
	be->type = BLOB_SPHERE;

	/* Add the new sphere element to the blob. */
	if (blob->elems != NULL)
	{
		Bloblet *lastbe;
		for (lastbe = blob->elems; lastbe->next != NULL; lastbe = lastbe->next)
			; /* Seek last element added to list. */
		/* Append our new one. */
		lastbe->next = be;
	}
	else
		blob->elems = be;

	return 1;
}

int Ray_BlobAddCylinder(Object *obj, Vec3 *pt1, Vec3 *pt2,
	double radius, double field)
{
	Bloblet *hemi1, *hemi2, *cyl;
	BlobData *blob = obj->data.blob;

	assert(obj != NULL);
	assert(blob != NULL);

	/*
	 * Cylindrical fields consist of three Bloblet elements.
	 * A cylinder and two hemispheres.
	 */						 

	/* Allocate the elements. */
	if ((cyl = (Bloblet *)Malloc(sizeof(Bloblet))) == NULL)
		return 0;
	if ((hemi1 = (Bloblet *)Malloc(sizeof(Bloblet))) == NULL)
	{
		Free(cyl, sizeof(Bloblet));
		return 0;
	}
	if ((hemi2 = (Bloblet *)Malloc(sizeof(Bloblet))) == NULL)
	{
		Free(hemi1, sizeof(Bloblet));
		Free(cyl, sizeof(Bloblet));
		return 0;
	}

	/* Link them together. */
	cyl->next = hemi1;
	hemi1->next = hemi2;
	hemi2->next = NULL;

	/* Store the origin point, radius and field strength for cylinder. */
	V3Copy(&cyl->loc, pt1);
	cyl->rad = radius;
	cyl->field = field;

	/*
	 * Pre-compute constants for the density eq. in standard form.
	 * Radii that are too small should be checked for during parse.
	 */
	cyl->rsq = cyl->rad * cyl->rad;
	cyl->r2 = -( 2.0 * cyl->field) / cyl->rsq;
	cyl->r4 = cyl->field / (cyl->rsq * cyl->rsq);

	/* Get offset vector of cylinder from the given end point. */
	V3Sub(&cyl->d, pt2, &cyl->loc);

	/*
	 * Get cylinder length squared & length, while we're at it...
	 * Zero length offset vectors should be checked for during parse.
	 */
	cyl->lsq = V3Dot(&cyl->d, &cyl->d);
	cyl->len = sqrt(cyl->lsq);
	V3Copy(&cyl->dir, &cyl->d);
	V3Normalize(&cyl->dir);

	/* Get "d" coeffs. for planes at cylinder ends. */
	cyl->d1 = -V3Dot(&cyl->dir, &cyl->loc);
	cyl->d2 = -V3Dot(&cyl->dir, pt2);

	/* Precompute location dot offset_vector constant. */
	cyl->l_dot_d = V3Dot(&cyl->loc, &cyl->d);

	cyl->type = BLOB_CYLINDER;

	/* Set up the hemispheres for the ends of the cylinder. */
	V3Copy(&hemi1->loc, &cyl->loc);
	V3Sub(&hemi1->dir, pt2, &hemi1->loc);
	V3Normalize(&hemi1->dir);
	hemi1->rad = cyl->rad;
	hemi1->rsq = cyl->rsq;
	hemi1->field = cyl->field;
	hemi1->r2 = cyl->r2;
	hemi1->r4 = cyl->r4;
	hemi1->type = BLOB_HEMISPHERE;

	V3Copy(&hemi2->loc, pt2);
	V3Sub(&hemi2->dir, &cyl->loc, &hemi2->loc);
	V3Normalize(&hemi2->dir);
	hemi2->rad = cyl->rad;
	hemi2->rsq = cyl->rsq;
	hemi2->field = cyl->field;
	hemi2->r2 = cyl->r2;
	hemi2->r4 = cyl->r4;
	hemi2->type = BLOB_HEMISPHERE;

	/*
	 * Finally, add the three new elements that make up the cylinder
	 * element to the blob.
	 */
	if (blob->elems != NULL)
	{
		Bloblet *lastbe;
		for (lastbe = blob->elems; lastbe->next != NULL; lastbe = lastbe->next)
			; /* Seek last element added to list. */
		/* Append our new one. */
		lastbe->next = cyl;
	}
	else
		blob->elems = cyl;

	return 1;
}

int Ray_BlobAddPlane(Object *obj, Vec3 *pt, Vec3 *dir,
	double dist, double field)
{
	Bloblet *be;
	BlobData *blob = obj->data.blob;
	Vec3 Pt;

	assert(obj != NULL);
	assert(blob != NULL);
	
	if ((be = (Bloblet *)Malloc(sizeof(Bloblet))) == NULL)
		return 0;

	be->next = NULL;
	V3Copy(&be->loc, pt);
	V3Copy(&be->dir, dir);
	be->rad = dist;
	be->field = field;

	/*
	 * Normalize the normal vector.
	 * Invalid normals should be checked for during parse.
	 */
	V3Normalize(&be->dir);

	/* Get "d" coeff. for plane that bounds plane's interval. */
	Pt.x = be->dir.x * be->rad;
	Pt.y = be->dir.y * be->rad;
	Pt.z = be->dir.z * be->rad;
	be->d1 = -V3Dot(&be->dir, &Pt);

	/*
	 * Pre-compute constants for the density eq. in standard form.
	 * Radii that are too small shoud be checked for during parse.
	 */
	be->rsq = be->rad * be->rad;
	be->r2 = - (2.0 * be->field) / be->rsq;
	be->r4 = be->field / (be->rsq * be->rsq);
	be->type = BLOB_PLANE;

	/* Add the new plane element to the blob. */
	if (blob->elems != NULL)
	{
		Bloblet *lastbe;
		for (lastbe = blob->elems; lastbe->next != NULL; lastbe = lastbe->next)
			; /* Seek last element added to list. */
		/* Append our new one. */
		lastbe->next = be;
	}
	else
		blob->elems = be;

	return 1;
}

int Ray_BlobFinish(Object *obj)
{
	int num_elems, i;
	BlobData *blob = obj->data.blob;
	Bloblet *be;
	assert(obj != NULL);
	assert(blob != NULL);

	/*
	 * Count the number of elements in blob.
	 * Blobs must have at least one element, this should be checked
	 * for during parse.
	 */
	num_elems = 0;
	for (be = blob->elems; be != NULL; be = be->next)
		num_elems++;

	/* Some assembly required... */
	if ((blob->hits = (BlobHit **)Calloc(num_elems * 2, sizeof(BlobHit *))) == NULL)
		return 0;
	for (i = 0; i < num_elems * 2; i++)
		if ((blob->hits[i] = (BlobHit *)Malloc(sizeof(BlobHit))) == NULL)
			return 0;
	return 1;
}

#ifdef OLDCODE
Object *Ray_MakeBlob(PARAMS *par)
{
	Object *obj;
	int token, num_elems, i;
	PARAMS *p;
	BlobData *b;
	Bloblet *be, *new_be, *hemi1, *hemi2, *cyl;
	static Vec3 pt;


	if((obj = NewObject()) == NULL)
		return NULL;

	/* evaluate parameters */
	p = par;
	while(1)
	{
		Eval_Params(p);
		for(i = p->more; i >= 0; i--)
			p = p->next;
		if(p == NULL)
			break;
		p = p->next;   /* skip element type delimiter */
	}

	/* Allocate the main blob data structure... */
	b = (BlobData *)Malloc(sizeof(BlobData));
	b->nrefs = 1;

	/* Get threshold... */
	b->threshold = par->V.x;
	b->solver = 0;
	b->bound = NULL;

	/* Get blob elements... */
	be = NULL;
	num_elems = 0;
	while((par = par->next) != NULL)
	{
		token = par->type;
		/* Get blob element... */
		switch(token)
		{
			case BLOB_SPHERE:
			case BLOB_PLANE:
			case BLOB_CYLINDER:
				for(i = 0; i < 3; i++)
				{
					/* Allocate a "Bloblet" structure. */
					new_be = (Bloblet *)Malloc(sizeof(Bloblet));
					new_be->next = NULL;
					new_be->field = 1.0; /* Default field strength, if not given. */
					if(be == NULL)  /* First one, start the list. */
					{
						be = new_be;
						b->elems = be;
					}
					else   /* Add to the list. */
					{
						be->next = new_be;
						be = be->next;
					}
					num_elems++;

					if(token != BLOB_CYLINDER)
						break;

					/* Save references to the hemisphere sub-elements for cylinder. */
					if(i == 0) cyl = new_be;
					else if(i == 1) hemi1 = new_be;
					else hemi2 = new_be;
				}
				par = par->next;
				break;
			default:
				break;
		}

		/* Get parameters... */
		switch(token)
		{
			case BLOB_SPHERE:
				V3Copy(&be->loc, &par->V);
				par = par->next;
				be->rad = par->V.x;
				if(par->more) /* Optional field strength was specified, also. */
				{
					par = par->next;
					be->field = par->V.x;
				}

				/* Pre-compute constants for the density eq. in standard form. */
				/* Radii that are too small shoud be checked for during parse. */
				be->rsq = be->rad * be->rad;
				be->r2 = - (2.0 * be->field) / be->rsq;
				be->r4 = be->field / (be->rsq * be->rsq);
				be->type = BLOB_SPHERE;
				continue;

			case BLOB_PLANE:
				V3Copy(&be->loc, &par->V);
				par = par->next;
				V3Copy(&be->dir, &par->V);
				par = par->next;
				be->rad = par->V.x;
				if(par->more) /* Optional field strength was specified, also. */
				{
					par = par->next;
					be->field = par->V.x;
				}

				/* Normalize the normal vector. */
				/* Invalid normals should be checked for during parse. */
				V3Normalize(&be->dir);

				/* Get "d" coeff. for plane that bounds plane's interval. */
				pt.x = be->dir.x * be->rad;
				pt.y = be->dir.y * be->rad;
				pt.z = be->dir.z * be->rad;
				be->d1 = -V3Dot(&be->dir, &pt);

				/* Pre-compute constants for the density eq. in standard form. */
				/* Radii that are too small shoud be checked for during parse. */
				be->rsq = be->rad * be->rad;
				be->r2 = - (2.0 * be->field) / be->rsq;
				be->r4 = be->field / (be->rsq * be->rsq);
				be->type = BLOB_PLANE;
				continue;

			case BLOB_CYLINDER:
				V3Copy(&cyl->loc, &par->V);
				par = par->next;
				V3Copy(&pt, &par->V);   /* end point for cylinder */
				par = par->next;
				cyl->rad = par->V.x;
				if(par->more) /* Optional field strength was specified, also. */
				{
					par = par->next;
					cyl->field = par->V.x;
				}

				/* Pre-compute constants for the density eq. in standard form. */
				/* Radii that are too small shoud be checked for during parse. */
				cyl->rsq = cyl->rad * cyl->rad;
				cyl->r2 = -( 2.0 * cyl->field) / cyl->rsq;
				cyl->r4 = cyl->field / (cyl->rsq * cyl->rsq);

				/* Get offset vector of cylinder. */
				V3Sub(&cyl->d, &pt, &cyl->loc);
				/* Get cylinder length squared & length, while we're at it... */
				/* Zero length offset vectors shoud be checked for during parse. */
				cyl->lsq = V3Dot(&cyl->d, &cyl->d);
				cyl->len = sqrt(cyl->lsq);
				V3Copy(&cyl->dir, &cyl->d);
				V3Normalize(&cyl->dir);

				/* Get "d" coeffs. for planes at cylinder ends. */
				cyl->d1 = -V3Dot(&cyl->dir, &cyl->loc);
				cyl->d2 = -V3Dot(&cyl->dir, &pt);

				/* Precompute location dot offset_vector constant. */
				cyl->l_dot_d = V3Dot(&cyl->loc, &cyl->d);

				cyl->type = BLOB_CYLINDER;

				/* Set up the hemispheres for the ends of the cylinder. */
				V3Copy(&hemi1->loc, &cyl->loc);
				V3Sub(&hemi1->dir, &pt, &hemi1->loc);
				V3Normalize(&hemi1->dir);
				hemi1->rad = cyl->rad;
				hemi1->rsq = cyl->rsq;
				hemi1->field = cyl->field;
				hemi1->r2 = cyl->r2;
				hemi1->r4 = cyl->r4;
				hemi1->type = BLOB_HEMISPHERE;

				V3Copy(&hemi2->loc, &pt);
				V3Sub(&hemi2->dir, &cyl->loc, &hemi2->loc);
				V3Normalize(&hemi2->dir);
				hemi2->rad = cyl->rad;
				hemi2->rsq = cyl->rsq;
				hemi2->field = cyl->field;
				hemi2->r2 = cyl->r2;
				hemi2->r4 = cyl->r4;
				hemi2->type = BLOB_HEMISPHERE;
				continue;

			default:
				break;
		}
		break;
	}

	/*
	 * Blobs must have at least one element, this should be checked
	 * for during parse.
	 */
	/* Some assembly required... */
	b->hits = (BlobHit **)Malloc(sizeof(BlobHit *) * num_elems * 2);
	for (i = 0; i < num_elems * 2; i++)
		b->hits[i] = (BlobHit *)Malloc(sizeof(BlobHit));

	obj->data.blob = b;
	obj->procs = &blob_procs;

	return obj;
}
#endif

int IntersectBlob(Object *obj, HitData *hits)
{
	BlobData *bl;
	BlobHit *first_bi;
	double ray_scale;

	ray_blob_tests++;

	bl = obj->data.blob;

	/* Check user-supplied bound object(s), if present... */
	if (bl->bound != NULL)
	{
		Object *o;
		int nhits = 0;

		for (o = bl->bound; o != NULL && nhits == 0; o = o->next)
			if (o->procs->Intersect(o, hits))
				nhits++;
		if (nhits == 0)
			return 0;
	}

	B = ct.B;
	D = ct.D;
	if (obj->T != NULL)
	{
		PointToObject(&B, obj->T);
		DirToObject(&D, obj->T);
		ray_scale = V3Mag(&D);
		if (ray_scale < EPSILON)
			return 0;
		D.x /= ray_scale;
		D.y /= ray_scale;
		D.z /= ray_scale;
	}
	else
		ray_scale = 1.0;

	if (calc_intervals(bl, &first_bi))
	{
		BlobHit *bi;
		double lo, hi;
		double tc[5], t[4];
		int i, nhits, valid_hits, in, ray_entering = 1;

		bi = first_bi;
		in = 0;
		valid_hits = 0;
		lo = bi->t;
		hi = bi->next->t;
		/* Initialize the eq. totals accumulator. */
		for (i = 1; i < 5; i++)
			tc[i] = 0.0;
		/* Start with blob threshold constant. */
		tc[0] = - bl->threshold;

		for (;;)
		{
			if (bi->entering)
			{
				in++;  /* entering an interval */
				calc_substitutions(bi);
				/* Add to sum total of density eqs. */
				for (i = 0; i < 5 ;i++)
					tc[i] += bi->be->c[i];
			}
			else  /* exiting an interval */
			{
				in--;
				/* Subtract this element out of density eq. totals accumulator. */
				if (in)
				{
					for (i = 0; i < 5; i++)
						tc[i] -= bi->be->c[i];
				}
				else   /* Clear the accumulator. */
				{
					for (i = 1; i < 5; i++)
						tc[i] = 0.0;
					tc[0] = - bl->threshold;
				}
			}

			if (in && (lo < hi))
			{
				nhits = SolvePoly(tc, t, 4, lo, hi);
				if (nhits > 0)
				{
					for (i = 0;i < nhits;i++)
					{
						if (valid_hits++)
						{
							hits = GetNextHit(hits);
						}
						else
						{
							ray_entering = ((nhits & 1) == 0) ? 1 : 0;
							if (obj->flags & OBJ_FLAG_INVERSE)
								ray_entering = 1 - ray_entering;
						}
						hits->t = t[i];
						hits->obj = obj;
						hits->entering = ray_entering;
						ray_entering = 1 - ray_entering;
						hits->t /= ray_scale;
					}
					if (!ct.calc_all)
						break;
				}
			}
			bi = bi->next;
			lo = bi->t;
			if (bi->next == NULL)
				break; /* Done. */
			hi = bi->next->t;
		} /* end of while there are intervals */
		if (valid_hits)
			ray_blob_hits++;
		return valid_hits;
	} /* end of if (calc_intervals()) */

	return 0; /* No intervals found. */
}

/*
 * calc_intervals() - Cycle through all blob elements checking for
 * ray/field-of-influence intersections. For every element hit, add
 * two intervals to a list, "intervals", in order of distance with the
 * closest at the top of the list. Returns 1 if a list was created,
 * or zero if not.
 */
static int calc_intervals(BlobData *blob, BlobHit **intervals)
{
	Bloblet *be;
	BlobHit **be_hit_array, *first_bi, *cur, *prev, *new_hit;
	double a, b, c, d, t1, t2;
	int i;

	first_bi = NULL;
	be_hit_array = blob->hits;

	dx = D.x;
	dy = D.y;
	dz = D.z;

	for (be = blob->elems; be != NULL; be = be->next)
	{
		ox = B.x - be->loc.x;
		oy = B.y - be->loc.y;
		oz = B.z - be->loc.z;
		if (be->type == BLOB_CYLINDER)
		{
			Vec3 cK, cD;

			/*Calculate the ray's base location in the object's coordinate system.*/
			t2 = V3Dot(&B, &be->d) - V3Dot(&be->loc, &be->d);
			t1 = t2 / be->lsq;  /* precomputed length squared */
			cK.x = B.x - be->loc.x - t1 * be->d.x;
			cK.y = B.y - be->loc.y - t1 * be->d.y;
			cK.z = B.z - be->loc.z - t1 * be->d.z;

			t2 = V3Dot(&D, &be->d);
			t1 = t2 / be->lsq;
			cD.x = D.x - t1 * be->d.x;
			cD.y = D.y - t1 * be->d.y;
			cD.z = D.z - t1 * be->d.z;

			a = V3Dot(&cD, &cD);
			b = 2.0 * V3Dot(&cD, &cK);
			c = V3Dot(&cK, &cK) - be->rsq;

			d = b * b - 4.0 * a * c;
			if (d < 0.0)
				continue; /* no roots, we miss */

			d = sqrt(d);
			t2 = (-b + d) / (2.0 * a);
			t1 = (-b - d) / (2.0 * a);

			/* Make sure that t1 is the smaller... */
			if(t1 > t2)
			{
				d = t1;
				t1 = t2;
				t2 = d;
			}

			/* See if interval is within area of interest... */
			if (t1 < ct.tmin)
				t1 = ct.tmin;
			if (t2 < ct.tmin)
				continue;

			/* Factor in the end planes of cylinder. */
			{
				double num, den, t3;

				/* Intersect plane at cylinder base. */
				den = be->dir.x * dx + be->dir.y * dy + be->dir.z * dz;
				num = V3Dot(&be->dir, &B) + be->d1;
				if (fabs(den) > EPSILON)
				{
					if (ISZERO(num))
					{
						if (den < 0.0 || t2 < ct.tmin)
							continue;
						if (t1 < ct.tmin)
							t1 = ct.tmin;
					}
					else
					{
						t3 = -num / den;
						if (num < 0.0)
						{
							if (t2 < t3 || t3 < 0.0)
								continue; /* Miss cylinder. */
							if (t1 < t3)
								t1 = t3;
						}
						else if(t3 >= 0.0)
						{
							if (t1 > t3)
								continue; /* Miss cylinder. */
							if (t2 > t3)
								t2 = t3;
						}
					}

					/* Intersect plane at cylinder end point. */
					num = V3Dot(&be->dir, &B) + be->d2;
					if (ISZERO(num))
					{
						if (den > 0.0 || t2 < ct.tmin)
							continue;
						if (t1 < ct.tmin)
							t1 = ct.tmin;
					}
					else
					{
						t3 = - (num) / den;
						if(num > 0.0)
						{
							if (t2 < t3 || t3 < 0.0)
								continue; /* Miss cylinder. */
							if (t1 < t3)
								t1 = t3;
						}
						else if(t3 >= 0.0)
						{
							if (t1 > t3)
								continue; /* Miss cylinder. */
							if (t2 > t3)
								t2 = t3;
						}
					}
				}
				else /* Ray is paralell to cyl base & end planes. */
				{
					if (num < 0.0)
						continue;
					num = V3Dot(&be->dir, &B) + be->d2;
					if (num > 0.0)
						continue;
				}
			}
		}
		else if (be->type == BLOB_PLANE)
		{
			double num, den;

			/* Intersect plane that bounds interval for plane. */
			den = be->dir.x * dx + be->dir.y * dy + be->dir.z * dz;
			num = be->dir.x * ox + be->dir.y * oy + be->dir.z * oz
				+ be->d1;

			if (fabs(den) > EPSILON)
			{
				t1 = - (num) / den;
				if (num > 0.0)
				{
					if (t1 < ct.tmin)
						continue; /* Miss plane. */
					t2 = ct.tmax;
				}
				else
				{
					if (t1 < ct.tmin)
					{
						t1 = ct.tmin;
						t2 = ct.tmax;
					}
					else
					{
						t2 = t1;
						t1 = ct.tmin;
					}
				}
			}
			else
			{
				if (num > 0.0)
					continue;
				t1 = ct.tmin;
				t2 = ct.tmax;
			}
		}
		else  /* Spheres or hemi-spheres. */
		{
			/*
			 * Test for intersection between ray and sphere.
			 */
			a = - (ox * dx + oy * dy + oz * dz);
			d = be->rsq - (( ox * ox + oy * oy + oz * oz) - a * a);
			if (d < 0.0)
				continue;

			d = sqrt(d);
			t2 = a + d;
			t1 = a - d;

			/* Make sure that t1 is the smaller... */
			if (t1 > t2)
			{
				d = t1;
				t1 = t2;
				t2 = d;
			}

			/* See if interval is within area of interest... */
			if (t1 < ct.tmin)
				t1 = ct.tmin;
			if (t2 < ct.tmin)
				continue;

			/* Factor in plane part if this is a hemi-sphere. */
			if (be->type == BLOB_HEMISPHERE)
			{
				double t3, num, den;

				/* Intersect plane. */
				den = be->dir.x * dx + be->dir.y * dy + be->dir.z * dz;
				num = be->dir.x * ox + be->dir.y * oy + be->dir.z * oz;
				if (fabs(den) > EPSILON)
				{
					if (ISZERO(num))
					{
						if (den > 0.0 || t2 < ct.tmin)
							continue;
						if (t1 < ct.tmin)
							t1 = ct.tmin;
					}
					else
					{
						t3 = - (num) / den;
						if (num > 0.0)
						{
							if (t2 < t3 || t3 < 0.0)
								continue; /* Miss hemi-sphere. */
							if (t1 < t3)
								t1 = t3;
						}
						else if (t3 >= 0.0)
						{
							if (t1 > t3)
								continue; /* Miss hemi-sphere. */
							if (t2 > t3)
								t2 = t3;
						}
					}
				}
				else /* Ray is paralell to plane, just check against ray base. */
					if (num > 0.0)
						continue;
			}
		} /* End of else Spheres or Hemi-Spheres. */

		/* Make sure that t1 is the smaller... */
		if (t1 > t2)
		{
			d = t1;
			t1 = t2;
			t2 = d;
		}

		/* See if interval is within area of interest... */
		if (t1 > ct.tmax || t2 <= ct.tmin)
			continue;

		/* Reject any intervals that are too small. */
		if (fabs(t1 - t2) < MIN_INTERVAL_SIZE)
			continue;

		/* Truncate any parts of interval that are out of bounds... */
		if (t1 < ct.tmin)
			t1 = ct.tmin;
		if (t2 > ct.tmax)
			t2 = ct.tmax;

		for (i = 0; i < 2; i++)
		{
			new_hit = *be_hit_array++;

			if (i == 0)
			{
				new_hit->t = t1;
				new_hit->entering = 1;
			}
			else
			{
				new_hit->t = t2;
				new_hit->entering = 0;
			}
			new_hit->next = NULL;
			new_hit->be = be;

			/* Build hit list in order from closest to farthest. */
			cur = first_bi;
			prev = NULL;
			while (cur != NULL)
			{
				if (new_hit->t <= cur->t)
				{
					new_hit->next = cur;
					if (prev != NULL)  /* Inserting some where inside list. */
						prev->next = new_hit;
					else               /* Put on top of list. */
						first_bi = new_hit;
					break;
				}
				prev = cur;
				cur = cur->next;
			}

			if (cur == NULL)  /* End of list was reached. */
			{
				if (prev == NULL)  /* Nothing was in list, first hit. */
					first_bi = new_hit;
				else
					prev->next = new_hit;
			}
		} /* end of add two intervals loop */
	} /* end of blob elements loop */

	*intervals = first_bi;

	if (first_bi == NULL)
		return 0; /* No intervals in list - No blob intersections. */

	return 1;
} /* end calc_intervals() */

void calc_substitutions(BlobHit *bi)
{
	Bloblet *be;
	double r2, r4, a, b, c, d;

	/*
	 * Whenever a new interval is entered, compute the density
	 * equation coefficients for its associated blob element
	 * and store in blob elements's c[] array for later reference.
	 */

	be = bi->be;

	ox = B.x - be->loc.x;
	oy = B.y - be->loc.y;
	oz = B.z - be->loc.z;

	r2 = be->r2;
	r4 = be->r4;

	if (be->type == BLOB_CYLINDER)
	{
		double dx0, dy0, dz0, t;

		/*
		* Transform ray base to point relative to cylinder's
		* symetrical axis.
		*/
		t = (V3Dot(&B, &be->d) - be->l_dot_d) / be->lsq;
		ox -= t * be->d.x;
		oy -= t * be->d.y;
		oz -= t * be->d.z;

		/* Scale the ray's direction agaist the cylinder's direction. */
		t = (dx * be->d.x + dy * be->d.y + dz * be->d.z) / be->lsq;
		dx0 = dx - t * be->d.x;
		dy0 = dy - t * be->d.y;
		dz0 = dz - t * be->d.z;

		a = dx0 * dx0 + dy0 * dy0 + dz0 * dz0;
		b = ox * dx0 + oy * dy0 + oz * dz0;
		c = ox * ox + oy * oy + oz * oz;
		d = 4.0 * r4 * b;

		be->c[4] = r4 * a * a;
		be->c[3] = a * d;
		be->c[2] = d * b + a *(2.0 * r4 * c + r2);
		be->c[1] = c * d + 2.0 * r2 * b;
		be->c[0] = c * (r4 * c + r2) + be->field;
	}
	else if (be->type == BLOB_PLANE)
	{
		double A, B, C, p0, p1;

		A = be->dir.x;
		B = be->dir.y;
		C = be->dir.z;
		p1 = A * dx + B * dy + C * dz;
		p0 = A * ox + B * oy + C * oz;

		be->c[4] = 0.0;
		be->c[3] = 0.0;
		be->c[2] = r4 * p1 * p1;
		be->c[1] = 2.0 * r4 * p0 * p1 + r2 * p1;
		be->c[0] = r4 * p0 * p0 + r2 * p0 + be->field;
	}
	else /* Spheres and hemispheres. */
	{
		b = ox * dx + oy * dy + oz * dz;
		c = ox * ox + oy * oy + oz * oz;
		d = 4.0 * r4 * b;

		be->c[4] = r4;
		be->c[3] = d;
		be->c[2] = d * b + 2.0 * r4 * c + r2;
		be->c[1] = c * d + 2.0 * r2 * b;
		be->c[0] = c * (r4 * c + r2) + be->field;
	}
} /* end of calc_substitutions() */

void CalcNormalBlob(Object *obj, Vec3 *Q, Vec3 *N)
{
	BlobData *b;
	Bloblet *be;
	double dist, x, y, z, a;
	Vec3 p;

	b = obj->data.blob;

	V3Copy(&p, Q);
	if (obj->T != NULL)
		PointToObject(&p, obj->T);
	V3Zero(N);

	for (be = b->elems; be != NULL; be = be->next)
	{
		x = p.x - be->loc.x;
		y = p.y - be->loc.y;
		z = p.z - be->loc.z;
		if (be->type == BLOB_CYLINDER)
		{
			double t;

			/* get distance of point along cylinder axis from cylinder origin */
			t = x * be->dir.x + y * be->dir.y + z * be->dir.z;
			/* are we within cylinder length? */
			if (t >= 0.0 && t < be->len)  /* yes */
			{
				/* get radius-squared from correponding point along cylinder axis */
				x -= be->dir.x * t;
				y -= be->dir.y * t;
				z -= be->dir.z * t;
				/* If point is outside radius of influence, it doesn't count. */
				if ((dist = x*x + y*y + z*z) >= be->rsq)
					continue;
			}
			else
				continue; /* Outside of cylinder length. */
		}
		else if (be->type == BLOB_PLANE)
		{
			if ((dist = x * be->dir.x + y * be->dir.y + z * be->dir.z)
				>= be->rad)
				continue;
			x = be->dir.x;
			y = be->dir.y;
			z = be->dir.z;
		}
		else   /* Spheres or hemi-spheres. */
		{
			/* If point is outside sphere of influence, it doesn't count. */
			if ((dist = x*x + y*y + z*z) >= be->rsq)
				continue;

			/* See that point is also within the plane if this is a hemisphere. */
			if (be->type == BLOB_HEMISPHERE)
				if ((x * be->dir.x + y * be->dir.y + z * be->dir.z) > 0.0)
					continue;
		}
		/*
		 * Note: Since this is actually the gradient for the inward facing
		 * surface of the density equation(s), we need to flip the sign so
		 * that the normal is pointing out from the blob. The outer surface
		 * of the density equation is "mirrored" on the outside of the
		 * field of influence for the blob element (due to the "W" shape
		 * of the density function) and does not fall within the interval
		 * containing valid blob intersections. These are checked for
		 * and skipped in the intersection tests above.
		 */
		a = - 4.0 * be->r4 * dist - 2.0 * be->r2;
		N->x += x * a;
		N->y += y * a;
		N->z += z * a;
	}

	if (obj->T != NULL)
		NormToWorld(N, obj->T);
	V3Normalize(N);
}

int IsInsideBlob(Object *obj, Vec3 *Q)
{
	BlobData *b;
	Bloblet *be;
	double x, y, z, d, dt;
	Vec3 p;

	b = obj->data.blob;

	V3Copy(&p, Q);
	if (obj->T != NULL)
		PointToObject(&p, obj->T);

	dt = 0.0;
	for (be = b->elems;be != NULL; be = be->next)
	{
		x = p.x - be->loc.x;
		y = p.y - be->loc.y;
		z = p.z - be->loc.z;

		if (be->type == BLOB_CYLINDER)
		{
			double t;

			/* get distance of point along cylinder axis from cylinder origin */
			t = x * be->dir.x + y * be->dir.y + z * be->dir.z;
			/* are we within cylinder length? */
			if (t >= 0.0 && t < be->len)  /* yes */
			{
				/* get radius-squared from correponding point along cylinder axis */
				x -= be->dir.x * t;
				y -= be->dir.y * t;
				z -= be->dir.z * t;
				/* If point is outside radius of influence, it doesn't count. */
				if ((d = x*x + y*y + z*z) >= be->rsq)
					continue;
			}
			else
				continue; /* Outside of cylinder length. */
		}
		else if (be->type == BLOB_PLANE)
		{
			if ((d = x * be->dir.x + y * be->dir.y + z * be->dir.z)
				>= be->rad)
				continue;
		}
		else   /* Spheres or hemi-spheres. */
		{
			/* If point is outside sphere of influence, it doesn't count. */
			if ((d = x*x + y*y + z*z) >= be->rsq)
				continue;

			/* See that point is also within the plane if this is a hemisphere. */
			if (be->type == BLOB_HEMISPHERE)
				if ((x * be->dir.x + y * be->dir.y + z * be->dir.z) > 0.0)
					continue;
		}

		dt += d * (d * be->r4 + be->r2) + be->field;
	}

	if (dt > b->threshold)
		return (!(obj->flags & OBJ_FLAG_INVERSE)); /* inside */
	return (obj->flags & OBJ_FLAG_INVERSE);  /* not inside */
}


void CalcUVMapBlob(Object *obj, Vec3 *P, double *u, double *v)
{
	*u = *v = 0.0;
	/* Not used: */
	//obj; P;
}


void CalcExtentsBlob(Object *obj, Vec3 *omin, Vec3 *omax)
{
	BlobData *b;
	Bloblet *be;

	b = obj->data.blob;

	/* If user supplied a bounding object, use its extents. */
	if (b->bound != NULL)
	{
		(b->bound->procs->CalcExtents)(b->bound, omin, omax);
		return;
	}

	V3Set(omin, HUGE, HUGE, HUGE);
	V3Set(omax, -HUGE, -HUGE, -HUGE);

	for (be = b->elems; NULL != be; be = be->next)
	{
		/* Skip the cylinder, it is accounted for by the two hemispheres. */
		if (be->type == BLOB_CYLINDER)
			continue;
		else if (be->type == BLOB_PLANE)
		{
			V3Set(omin, -HUGE, -HUGE, -HUGE);
			V3Set(omax, HUGE, HUGE, HUGE);
			return;  /* The object is infinite, return now. */
		}

		omin->x = fmin(omin->x, be->loc.x - be->rad);
		omax->x = fmax(omax->x, be->loc.x + be->rad);
		omin->y = fmin(omin->y, be->loc.y - be->rad);
		omax->y = fmax(omax->y, be->loc.y + be->rad);
		omin->z = fmin(omin->z, be->loc.z - be->rad);
		omax->z = fmax(omax->z, be->loc.z + be->rad);
	}

	omin->x -= EPSILON;
	omin->y -= EPSILON;
	omin->z -= EPSILON;
	omax->x += EPSILON;
	omax->y += EPSILON;
	omax->z += EPSILON;

	/* Transform the bounds to their actual world position. */
	if (obj->T != NULL)
		BBoxToWorld(omin, omax, obj->T);
}

void TransformBlob(Object *obj, Vec3 *params, int type)
{
	BlobData *b = obj->data.blob;

	if (obj->T == NULL)
		obj->T = Ray_NewXform();
	XformXforms(obj->T, params, type);

	if (b->bound != NULL)
		Ray_Transform_Object(b->bound, params, type);
}



/*************************************************************************
*
*  CopyBlob - Copy blob-specific data from srcobj to destobj.
*  Blob data can be reference copied since there are no dependencies
*  on a single object instance.
*
*************************************************************************/
void CopyBlob(Object *destobj, Object *srcobj)
{
	BlobData *b = srcobj->data.blob;
	b->nrefs++;
	destobj->data.blob = b;
}


void DeleteBlob(Object *obj)
{
	BlobData *b = obj->data.blob;
	Object *o;
	Bloblet *be;
	int i, num_elems;

	if(--b->nrefs > 0)
		return;  /* BlobData is still shared by other objects. */

	num_elems = 0;
	while (b->elems != NULL)
	{
		be = b->elems;
		b->elems = be->next;
		Free(be, sizeof(Bloblet));
		num_elems++;
	}
	for (i = 0; i < num_elems * 2; i++)
		Free(b->hits[i], sizeof(BlobHit));
	Free(b->hits, sizeof(BlobHit *) * num_elems * 2);
	while (b->bound != NULL)
	{
		o = b->bound;
		b->bound = o->next;
		Ray_DeleteObject(o);
	}
	Free(b, sizeof(BlobData));
}


/*************************************************************************
*
*  Draws a wire frame view of object.
*
*************************************************************************/

void DrawBlob(Object *obj)
{
	Set_Pt(-1, 0.0, 0.0, 0.0);
	/* Not used: */
	//obj;
}
