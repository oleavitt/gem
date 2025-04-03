/*************************************************************************
*
*	mesh.c
*
*	The 3D mesh primitive.
*
*************************************************************************/

#include "ray.h"

#define MAX_TRIANGLES_PER_LEAF 8

static int IntersectMesh(Object *obj, HitData *hits);
static void CalcNormalMesh(Object *obj, Vec3 *P, Vec3 *N);
static int IsInsideMesh(Object *obj, Vec3 *P);
static void CalcUVMapMesh(Object *obj, Vec3 *P, double *u, double *v);
static void CalcExtentsMesh(Object *obj, Vec3 *omin, Vec3 *omax);
static void TransformMesh(Object *obj, Vec3 *params, int type);
static void CopyMesh(Object *destobj, Object *srcobj);
static void DeleteMesh(Object *obj);
static void DrawMesh(Object *obj);

static MeshNode *NewMeshNode(void);
static void DeleteMeshNode(MeshNode *n);
static MeshNode *BuildTriTree(MeshTri *tris, int ntris);
static void DeleteTriTree(MeshNode *n);
static MeshLocalData *NewMeshLocalData(void);
static void DeleteMeshLocalData(MeshLocalData *ml);
static MeshHit *NewMeshHit(void);
static void DeleteMeshHit(MeshHit *h);
static MeshHit *GetNextMeshHit(MeshHit *h);
static void PostProcessTri(MeshTri *t);
static void GenerateTriVertexNormals(MeshData *mesh, MeshTri *tris);
static int CalcTriListExtents(MeshTri *tris, int ntris,
	Vec3 *bmin, Vec3 *bmax, double *median);
static void SplitTriList(MeshTri *tris, double median, int axis,
	MeshTri **lo, MeshTri **hi);
static void IntersectTriTree(MeshLocalData *ml, MeshNode *tree,
	Vec3 *B, Vec3 *D);
static void IntersectTriList(MeshLocalData *ml, MeshTri *tris,
	Vec3 *B, Vec3 *D);
static void InsertHit(MeshLocalData *ml, MeshTri *tri, double t,
	double a, double b);

unsigned long ray_mesh_tests;
unsigned long ray_mesh_hits;

static ObjectProcs mesh_procs =
{
	OBJ_MESH,
	IntersectMesh,
	CalcNormalMesh,
	IsInsideMesh,
	CalcUVMapMesh,
	CalcExtentsMesh,
	TransformMesh,
	CopyMesh,
	DeleteMesh,
	DrawMesh
};

/*
 * Points to a mesh object under construction during a
 * BeginMesh/EndMesh session.
 */
static Object *mesh_obj = NULL;

/*************************************************************************
*
*	Functions for interactively building mesh objects.
*
*************************************************************************/
#ifdef OLDCODE

Object *Ray_MakeMesh(PARAMS *par)
{
	Object *obj;
	MeshData *mesh;
	int i, ntris, nverts;
	MeshVertex *v1, *v2, *v3;
	MeshTri *tris, *t;

	if ((mesh = Ray_NewMeshData()) == NULL)
		return NULL;

	/* First parameter is the number of vertices. */
	nverts = (int)par->V.x;
	
	/* Build the vertex array. */
	par = par->next;
	for (i = 0; i < nverts; i++)
	{
		/* Get the point. */
		Eval_Params(par);
		if ((v1 = Ray_NewMeshVertex(&par->V)) == NULL)
			goto fail_create;
		if (!Ray_AddMeshVertex(mesh, v1))
			goto fail_create;
		/* Get the optional stuff. */
		if (par->more)
		{
			/* Normal. */
			par = par->next;
			v1->nx = (float)par->V.x;
			v1->ny = (float)par->V.y;
			v1->nz = (float)par->V.z;
			v1->flags |= MESH_VERTEX_HAS_NORMAL;
			if (par->more)
			{
				/* Color. */
				par = par->next;
				v1->r = (float)par->V.x;
				v1->g = (float)par->V.y;
				v1->b = (float)par->V.z;
				v1->flags |= MESH_VERTEX_HAS_COLOR;
				if (par->more)
				{
					/* UV coordinates. */
					par = par->next;
					v1->u = (float)par->V.x;
					par = par->next;
					v1->v = (float)par->V.x;
					v1->flags |= MESH_VERTEX_HAS_UV;
				}
			}
		}
		par = par->next;
	}
			
	/* Build the triangle list. */
	/* First parameter of triangle list is the number of triangles. */
	ntris = (int)par->V.x;
	tris = t = NULL;
	par = par->next;
	for (i = 0; i < ntris; i++)
	{
		/* Get the vertex indices. */
		Eval_Params(par);
		v1 = mesh->vertices[(int)par->V.x % mesh->nvertices];
		par = par->next;
		v2 = mesh->vertices[(int)par->V.x % mesh->nvertices];
		par = par->next;
		v3 = mesh->vertices[(int)par->V.x % mesh->nvertices];
		par = par->next;
		if ((t = Ray_NewMeshTri(v1, v2, v3)) != NULL)
		{
			t->next = tris;
			tris = t;
		}
	}

	if ((obj = Ray_MakeMeshFromData(mesh, tris, ntris)) == NULL)
		goto fail_create;

	return obj;
	
	fail_create:

	while (tris != NULL)
	{
		t = tris;
		tris = t->next;
		Ray_DeleteMeshTri(t);
	}
	Ray_DeleteMeshData(mesh);

	return NULL;
}
#endif //OLDCODE

/*
 * Create a new mesh Object.
 */
Object *Ray_BeginMesh(void)
{
	if (mesh_obj != NULL)
	{
		assert(0); /* Did we finish last object? */
		return NULL;
	}

	if ((mesh_obj = NewObject()) == NULL)
		return NULL;
	if ((mesh_obj->localdata.mesh = NewMeshLocalData()) == NULL)
		goto fail_create;
	if ((mesh_obj->data.mesh = Ray_NewMeshData()) == NULL)
		goto fail_create;

	mesh_obj->procs = &mesh_procs;

	return mesh_obj;
	
	fail_create:
	DeleteMeshLocalData(mesh_obj->localdata.mesh);
	Ray_DeleteMeshData(mesh_obj->data.mesh);
	Ray_DeleteObject(mesh_obj);
	mesh_obj = NULL;
	return NULL;
}

Object *Ray_FinishMesh(void)
{
	Object *obj = mesh_obj;
	MeshData *mesh;
	MeshTri *t;
	int ntris;

	if (obj == NULL)
	{
		assert(0); /* Did we start an object? */
		return 0;
	}

	mesh = obj->data.mesh;
	assert(mesh != NULL);
	assert(mesh->tris != NULL);

	/* Post-process triangles. */
	for (t = mesh->tris, ntris = 0; t != NULL; t = t->next, ntris++)
		PostProcessTri(t);

	GenerateTriVertexNormals(mesh, mesh->tris);

	/* Build the triangle tree. (do this last) */
	if ((mesh->tree = BuildTriTree(mesh->tris, ntris)) == NULL)
		goto fail_finish;

	mesh_obj = NULL;
	return obj;
	
	fail_finish:
	DeleteMeshLocalData(obj->localdata.mesh);
	Ray_DeleteMeshData(obj->data.mesh);
	Ray_DeleteObject(obj);
	mesh_obj = NULL;
	return NULL;
}

/*
 * Create an Object from an existing MeshData struct.
 */
Object *Ray_MakeMeshFromData(MeshData *mesh, MeshTri *tris, int ntris)
{
	Object *obj;
	MeshTri *t;

	assert(mesh != NULL);
	assert(tris != NULL);

	if ((obj = NewObject()) == NULL)
		return NULL;
	if ((obj->localdata.mesh = NewMeshLocalData()) == NULL)
		goto fail_create;

	obj->data.mesh = mesh;
	obj->procs = &mesh_procs;

	mesh->nrefs = 1;
	
	/* Post-process triangles. */
	for (t = tris; t != NULL; t = t->next)
		PostProcessTri(t);

	GenerateTriVertexNormals(mesh, tris);

	/* Build the triangle tree. (do this last) */
	if ((mesh->tree = BuildTriTree(tris, ntris)) == NULL)
		goto fail_create;

#ifndef NDEBUG
/*	FPrintTree(mesh); */
#endif

	return obj;
	
	fail_create:
	DeleteMeshLocalData(obj->localdata.mesh);
	obj->data.mesh = NULL;
	obj->procs = NULL;
	Ray_DeleteObject(obj);
	return NULL;
}


MeshData *Ray_NewMeshData(void)
{
	MeshData *mesh = (MeshData *)Calloc(1, sizeof(MeshData));
	if (mesh != NULL)
	{
		mesh->nrefs = 1;
		mesh->vertices = NULL;
		mesh->tris = mesh->trilast = NULL;
		mesh->tree = NULL;
	}
	return mesh;
}

void Ray_DeleteMeshData(MeshData *mesh)
{
	if (--mesh->nrefs == 0)
	{
		int i;
		for (i = 0; i < mesh->nvertices; i++)
			Ray_DeleteMeshVertex(mesh->vertices[i]);
		Free(mesh->vertices, mesh->nvertices * sizeof(MeshVertex *));
		DeleteTriTree(mesh->tree);
		Free(mesh, sizeof(MeshData));
	}
}


MeshVertex *Ray_NewMeshVertex(Vec3 *pt)
{
	MeshVertex *v = (MeshVertex *)Calloc(1, sizeof(MeshVertex));
	if ((v != NULL) && (pt != NULL))
	{
		v->x = (float)pt->x;
		v->y = (float)pt->y;
		v->z = (float)pt->z;
	}
	return v;
}

int Ray_AddMeshVertex(MeshData *mesh, MeshVertex *v)
{
	size_t size;
	MeshVertex **newlist;

	assert(mesh != NULL);
	assert(v != NULL);
	
	size = sizeof(MeshVertex *) * mesh->nvertices;
	if ((newlist = (MeshVertex **)Malloc(size +
		sizeof(MeshVertex *))) == NULL)
		return 0;
	if (mesh->vertices != NULL)
		memcpy(newlist, mesh->vertices, size);
	newlist[mesh->nvertices] = v;
	Free(mesh->vertices, size);
	mesh->vertices = newlist;
	mesh->nvertices++;
	return 1;
}

void Ray_DeleteMeshVertex(MeshVertex *v)
{
	Free(v, sizeof(MeshVertex));
}

/*
 *	Creates a new triangle if the vertices define a valid plane
 *	for the triangle. Returns pointer to the triangle if successful.
 *	Returns NULL if triangle is ill-defined or allocation fails.
 */
MeshTri *Ray_NewMeshTri(MeshVertex *v1, MeshVertex *v2, MeshVertex *v3)
{
	MeshTri *t;
/*
// 12/15/01 - Vertices may be modified after tri is added.
// TODO: Perhaps this check should be done in Ray_FinishMesh().
	Vec3 d1, d2, cp;

	d1.x = v2->x - v1->x;
	d1.y = v2->y - v1->y;
	d1.z = v2->z - v1->z;
	d2.x = v3->x - v1->x;
	d2.y = v3->y - v1->y;
	d2.z = v3->z - v1->z;
	V3Cross(&cp, &d1, &d2);
	if (V3Mag(&cp) < 0.000001)
		return NULL;
*/
	if ((t = (MeshTri *)Calloc(1, sizeof(MeshTri))) != NULL)
	{
		t->next = NULL;
		t->v1 = v1;
		t->v2 = v2;
		t->v3 = v3;
	}
	return t;
}

void Ray_AddMeshTri(MeshData *mesh, MeshTri *tri)
{
	assert(mesh != NULL);
	if (tri != NULL)
	{
		if (mesh->trilast != NULL)
			mesh->trilast->next = tri;
		else
			mesh->tris = tri;
		mesh->trilast = tri;
	}
}

void Ray_DeleteMeshTri(MeshTri *t)
{
	Free(t, sizeof(MeshTri));
}

/*************************************************************************
*
*	Functions for the mesh ObjectProcs struct.
*
*************************************************************************/

int IntersectMesh(Object *obj, HitData *hits)
{
	MeshData *m;
	MeshLocalData *ml;
	MeshHit *h;
	Vec3 B, D;
	int i, entering;

	ray_mesh_tests++;
	m = obj->data.mesh;
	ml = obj->localdata.mesh;

	/* Check user-supplied bounding object, if any. */

	/* Transform ray to object space. */
	B = ct.B;
	D = ct.D;
	if (obj->T != NULL)
	{
		PointToObject(&B, obj->T);
		DirToObject(&D, obj->T);
	}

	/* Traverse the triangle tree, testing for intersections. */
	ml->nhits = 0;
	IntersectTriTree(ml, m->tree, &B, &D);

	/* Copy hit data, if any, to caller's hit list. */
	if (ml->nhits)
	{
		ray_mesh_hits++;
		h = ml->hits;
		entering = ((ml->nhits & 1) == 0);
		if (obj->flags & OBJ_FLAG_INVERSE)
			entering = 1 - entering;
		for (i = 0; i < ml->nhits; i++)
		{
			hits->obj = obj;
			hits->t = h->t;
			hits->entering = entering;
			entering = 1 - entering;
			hits = GetNextHit(hits);
			h = h->next;
		}
	}

	return ml->nhits;
}


void CalcNormalMesh(Object *obj, Vec3 *P, Vec3 *N)
{
	MeshLocalData *ml;
	MeshHit *h;
	int i;
	Vec3 Pt;

	ml = obj->localdata.mesh;

	Pt = *P;
	if (obj->T != NULL)
		PointToObject(&Pt, obj->T);

	/* What patch did we hit? */
	h = ml->hits;
	for (i = ml->nhits; i > 0; i--)
	{
		assert(h != NULL);
		if (fabs(h->t - ct.t) < EPSILON)
			break;
		h = h->next;
	}
	assert(i > 0);	/* A match must always be found! */

	/*
	 * If smooth shading is enabled, interpolate from the normals at
	 * each vertex to get actual normal for Pt, else use patch's
	 * plane normal.
	 */
	if (obj->flags & OBJ_FLAG_SMOOTH)
	{
		N->x = h->tri->v1->nx * h->c + h->tri->v2->nx * h->a + h->tri->v3->nx * h->b;
		N->y = h->tri->v1->ny * h->c + h->tri->v2->ny * h->a + h->tri->v3->ny * h->b;
		N->z = h->tri->v1->nz * h->c + h->tri->v2->nz * h->a + h->tri->v3->nz * h->b;
	}
	else
		V3Copy(N, &h->tri->pnorm);

	if (obj->T != NULL)
		NormToWorld(N, obj->T);
	else
		V3Normalize(N);
}


int IsInsideMesh(Object *obj, Vec3 *P)
{
	MeshData *m;
	Vec3 Pt;

	m = obj->data.mesh;

	/*
	 * Are within the object's bounding volume or 
	 * user-supplied bounding object(s)?
	 */

	Pt = *P;
	if (obj->T != NULL)
		PointToObject(&Pt, obj->T);

	/*
	 * Shoot a ray from Pt and count the number of intersections.
	 * If the number is odd, Pt is inside, else it is outside.
	 */

	return ( ! (obj->flags & OBJ_FLAG_INVERSE)); /* inside */
}


void CalcUVMapMesh(Object *obj, Vec3 *P, double *u, double *v)
{
	MeshLocalData *ml;
	MeshHit *h;
	int i;

	ml = obj->localdata.mesh;

	/* What patch did we hit? */
	h = ml->hits;
	for (i = ml->nhits; i > 0; i--)
	{
		assert(h != NULL);
		if (fabs(h->t - ct.t) < EPSILON)
			break;
		h = h->next;
	}
	assert(i > 0);	/* A match must always be found! */

	/*
	 * Interpolate from the UV values at each vertex to get
	 * UV point for P.
	 */
	*u = h->tri->v1->u * h->c + h->tri->v2->u * h->a + h->tri->v3->u * h->b;
	*v = h->tri->v1->v * h->c + h->tri->v2->v * h->a + h->tri->v3->v * h->b;
}


void CalcExtentsMesh(Object *obj, Vec3 *omin, Vec3 *omax)
{
	MeshData *m = obj->data.mesh;
	V3Copy(omin, &m->tree->bmin);
	V3Copy(omax, &m->tree->bmax);	
	if (obj->T != NULL)
		BBoxToWorld(omin, omax, obj->T);
}


void TransformMesh(Object *obj, Vec3 *params, int type)
{
	if (obj->T == NULL)
		obj->T = Ray_NewXform();
	XformXforms(obj->T, params, type);
}


void CopyMesh(Object *destobj, Object *srcobj)
{
	MeshData *m = srcobj->data.mesh;
	if ((destobj->localdata.mesh = NewMeshLocalData()) != NULL)
	{
		m->nrefs++;
		destobj->data.mesh = m;
	}
}


void DeleteMesh(Object *obj)
{
	Ray_DeleteMeshData(obj->data.mesh);
	DeleteMeshLocalData(obj->localdata.mesh);
}


/*************************************************************************
*
*	Local utility functions.
*
*************************************************************************/

void IntersectTriTree(MeshLocalData *ml, MeshNode *tree,
	Vec3 *B, Vec3 *D)
{
	double t1, t2;

	/* Step thru the node list for this level. */
	while (tree != NULL)
	{
		/* See if ray hits this node's bounding box */
		if (Intersect_Box(B, D, &tree->bmin, &tree->bmax, &t1, &t2))
		{
			if ((t2 > ct.tmin) && (t1 < ct.tmax))
			{
				if (tree->nodes != NULL) /* Check child nodes. */
					IntersectTriTree(ml, tree->nodes, B, D);
				else	/* Test triangle list for intersections. */
					IntersectTriList(ml, tree->tris, B, D);
			}
		}
		tree = tree->next;
	}
}


void IntersectTriList(MeshLocalData *ml, MeshTri *tris, Vec3 *B, Vec3 *D)
{
	double t, d, u0, v0, u1, v1, u2, v2, a, b;
	Vec3 P;

	/* Step thru the node list for this level. */
	while (tris != NULL)
	{
		P.x = B->x - tris->v1->x;
		P.y = B->y - tris->v1->y;
		P.z = B->z - tris->v1->z;
		d = V3Dot(&tris->pnorm, D);
		if (fabs(d) > EPSILON)
		{
			t = -V3Dot(&tris->pnorm, &P) / d;
			if ((t > ct.tmin) && (t < ct.tmax))
			{
				/*
				 * Project the triangle vertices to the 2D plane
				 * that is most perpendicular to the plane normal.
				 * This has already been pre-computed and the result
				 * is stored in "tris->axis".
				 */
				switch (tris->axis)
				{
					case X_AXIS:
						u0 = P.y + D->y * t; v0 = P.z + D->z * t;
						u1 = tris->v2->y-tris->v1->y; v1 = tris->v2->z-tris->v1->z;
						u2 = tris->v3->y-tris->v1->y; v2 = tris->v3->z-tris->v1->z;
						break;
					case Y_AXIS:
						u0 = P.x + D->x * t; v0 = P.z + D->z * t;
						u1 = tris->v2->x-tris->v1->x; v1 = tris->v2->z-tris->v1->z;
						u2 = tris->v3->x-tris->v1->x; v2 = tris->v3->z-tris->v1->z;
						break;
					default:	/* Z_AXIS */
						u0 = P.x + D->x * t; v0 = P.y + D->y * t;
						u1 = tris->v2->x-tris->v1->x; v1 = tris->v2->y-tris->v1->y;
						u2 = tris->v3->x-tris->v1->x; v2 = tris->v3->y-tris->v1->y;
						break;
				}

				a = 0.0;
				if (fabs(u1) < EPSILON)
				{
					b = u0 / u2;
					if ((b >= 0.0) && (b <= 1.0))
						a = (v0 - b * v2) / v1;
				}
				else
				{
					b = (v0 * u1 - u0 * v1) / (v2 * u1 - u2 * v1);
					if ((b >= 0.0) && (b <= 1.0))
						a = (u0 - b * u2) / u1;
				}
				
				if ((a >= 0.0) && (b >= 0.0) && ((a + b) <= 1.0))
					InsertHit(ml, tris, t, a, b);
			}
		}
		tris = tris->next;
	}
}


void InsertHit(MeshLocalData *ml, MeshTri *tri, double t, double a, double b)
{
	MeshHit *h, *p, *last, *plast = NULL;
	int i;

	ml->nhits++;
	last = ml->hits;
	for (i = 1; i < ml->nhits; i++)
	{
		plast = last;
		last = GetNextMeshHit(last);
	}
	last->tri = tri;
	last->t = t;
	last->a = a;
	last->b = b;
	last->c = 1.0 - a - b;
	if (plast != NULL)
	{
		plast->next = last->next;
		p = NULL;
		h = ml->hits;
		for (i = 1; i < ml->nhits; i++)
		{
			if (t < h->t)
			{
				last->next = h;
				if (p != NULL)
					p->next = last;
				else
					ml->hits = last;
				break;
			}
			p = h;
			h = h->next;
		}
		if (i == ml->nhits)
			plast->next = last;
	}
}


void PostProcessTri(MeshTri *t)
{
	Vec3 d1, d2;
	/* Precompute the plane normal. */
	d1.x = t->v2->x - t->v1->x;
	d1.y = t->v2->y - t->v1->y;
	d1.z = t->v2->z - t->v1->z;
	d2.x = t->v3->x - t->v1->x;
	d2.y = t->v3->y - t->v1->y;
	d2.z = t->v3->z - t->v1->z;
	V3Cross(&t->pnorm, &d1, &d2);
	V3Normalize(&t->pnorm);
	/* Determine axis of greatest projection. */
	if (fabs(t->pnorm.x) > fabs(t->pnorm.z))
	{
		if (fabs(t->pnorm.x) > fabs(t->pnorm.y))
			t->axis = X_AXIS;
		else
			t->axis = Y_AXIS;
	}
	else if (fabs(t->pnorm.y) > fabs(t->pnorm.z))
		t->axis = Y_AXIS;
	else
		t->axis = Z_AXIS;
}


void GenerateTriVertexNormals(MeshData *mesh, MeshTri *tris)
{
	MeshTri *t;
	MeshVertex *v, **pv;
	float d;
	int i;
	for (t = tris; t != NULL; t = t->next)
	{
		v = t->v1;
		d = v->nx * v->nx + v->ny * v->ny + v->nz * v->nz;
		if ((d < EPSILON) || ((v->flags & MESH_VERTEX_HAS_NORMAL) == 0))
		{
			v->nx = (float)t->pnorm.x;
			v->ny = (float)t->pnorm.y;
			v->nz = (float)t->pnorm.z;
			v->flags |= MESH_VERTEX_HAS_NORMAL;
		}
	}
	for (t = tris; t != NULL; t = t->next)
	{
		v = t->v2;
		d = v->nx * v->nx + v->ny * v->ny + v->nz * v->nz;
		if ((d < EPSILON) || ((v->flags & MESH_VERTEX_HAS_NORMAL) == 0))
		{
			v->nx = (float)t->pnorm.x;
			v->ny = (float)t->pnorm.y;
			v->nz = (float)t->pnorm.z;
			v->flags |= MESH_VERTEX_HAS_NORMAL;
		}
		v = t->v3;
		d = v->nx * v->nx + v->ny * v->ny + v->nz * v->nz;
		if ((d < EPSILON) || ((v->flags & MESH_VERTEX_HAS_NORMAL) == 0))
		{
			v->nx = (float)t->pnorm.x;
			v->ny = (float)t->pnorm.y;
			v->nz = (float)t->pnorm.z;
			v->flags |= MESH_VERTEX_HAS_NORMAL;
		}
	}
	/* Re-normalize the vertex normals. */
	pv = mesh->vertices;
	for (i = mesh->nvertices; i > 0; i--) 
	{
		v = *pv++;
		d = (float)sqrt(v->nx * v->nx + v->ny * v->ny + v->nz * v->nz);
		if (d > EPSILON)
		{
			v->nx /= d;
			v->ny /= d;
			v->nz /= d;
		}
		else	/* Use the plane normal. */
		{
			v->nx = (float)tris->pnorm.x;
			v->ny = (float)tris->pnorm.y;
			v->nz = (float)tris->pnorm.z;
		}
	}
}


void SplitTriList(MeshTri *tris, double median, int axis,
	MeshTri **lo, MeshTri **hi)
{
	MeshTri *t, *l, *h;
	double m;

	l = h = *lo = *hi = NULL;
	for (t = tris; t != NULL; )
	{
		m = (axis == X_AXIS) ? (t->v1->x + t->v2->x + t->v3->x) / 3.0 :
				(axis == Y_AXIS) ? (t->v1->y + t->v2->y + t->v3->y) / 3.0 :
				(t->v1->z + t->v2->z + t->v3->z) / 3.0;
		if (m < median)
		{
			if (l != NULL)
				l->next = t;
			else
				*lo = t;
			l = t;
			t = t->next;
			l->next = NULL;
		}
		else
		{
			if (h != NULL)
				h->next = t;
			else
				*hi = t;
			h = t;
			t = t->next;
			h->next = NULL;
		}
	}
	assert(*lo != NULL);
	assert(*hi != NULL);
}

int CalcTriListExtents(MeshTri *tris, int ntris, 
	Vec3 *bmin, Vec3 *bmax, double *median)
{
	MeshTri *t;
	int axis;
#ifndef NDEBUG
	int i = 0;
#endif
	double mx, my, mz, mxl, myl, mzl, mxh, myh, mzh, m;

	V3Set(bmin, HUGE, HUGE, HUGE);
	V3Set(bmax, -HUGE, -HUGE, -HUGE);
	mxl = myl = mzl = HUGE;
	mxh = myh = mzh = -HUGE;
	mx = my = mz = 0.0;
	for (t = tris; t != NULL; t = t->next)
	{
#ifndef NDEBUG
		i++;
#endif
		if (bmin->x > t->v1->x) bmin->x = t->v1->x;
		if (bmax->x < t->v1->x) bmax->x = t->v1->x;
		if (bmin->y > t->v1->y) bmin->y = t->v1->y;
		if (bmax->y < t->v1->y) bmax->y = t->v1->y;
		if (bmin->z > t->v1->z) bmin->z = t->v1->z;
		if (bmax->z < t->v1->z) bmax->z = t->v1->z;
		if (bmin->x > t->v2->x) bmin->x = t->v2->x;
		if (bmax->x < t->v2->x) bmax->x = t->v2->x;
		if (bmin->y > t->v2->y) bmin->y = t->v2->y;
		if (bmax->y < t->v2->y) bmax->y = t->v2->y;
		if (bmin->z > t->v2->z) bmin->z = t->v2->z;
		if (bmax->z < t->v2->z) bmax->z = t->v2->z;
		if (bmin->x > t->v3->x) bmin->x = t->v3->x;
		if (bmax->x < t->v3->x) bmax->x = t->v3->x;
		if (bmin->y > t->v3->y) bmin->y = t->v3->y;
		if (bmax->y < t->v3->y) bmax->y = t->v3->y;
		if (bmin->z > t->v3->z) bmin->z = t->v3->z;
		if (bmax->z < t->v3->z) bmax->z = t->v3->z;
		m = (t->v1->x + t->v2->x + t->v3->x) / 3.0;
		mxl = fmin(m, mxl);
		mxh = fmax(m, mxh);
		mx += m;
		m = (t->v1->y + t->v2->y + t->v3->y) / 3.0;
		myl = fmin(m, myl);
		myh = fmax(m, myh);
		my += m;
		m = (t->v1->z + t->v2->z + t->v3->z) / 3.0;
		mzl = fmin(m, mzl);
		mzh = fmax(m, mzh);
		mz += m;
	}
	mx /= (double)ntris;
	my /= (double)ntris;
	mz /= (double)ntris;

	assert(i == ntris);

	if ((mxh - mxl) > (mzh - mzl))
	{
		if ((mxh - mxl) > (myh - myl))
		{
			*median = mx;
			axis = X_AXIS;
		}
		else
		{
			*median = my;
			axis = Y_AXIS;
		}
	}
	else if ((myh - myl) > (mzh - mzl))
	{
		*median = my;
		axis = Y_AXIS;
	}
	else
	{
		*median = mz;
		axis = Z_AXIS;
	}

	bmin->x -= EPSILON;
	bmin->y -= EPSILON;
	bmin->z -= EPSILON;
	bmax->x += EPSILON;
	bmax->y += EPSILON;
	bmax->z += EPSILON;

	return axis;
}


MeshNode *BuildTriTree(MeshTri *tris, int ntris)
{
	MeshNode *n;
	MeshTri *lo, *hi, *t;
	double median;
	int axis, nlo, nhi;
	
	if ((n = NewMeshNode()) == NULL)
		return NULL;

	axis = CalcTriListExtents(tris, ntris, &n->bmin, &n->bmax, &median);

	if (ntris > MAX_TRIANGLES_PER_LEAF)
	{
		n->tris = NULL;
		SplitTriList(tris, median, axis, &lo, &hi);
		nlo = 0;
		for (t = lo; t != NULL; t = t->next)
			nlo++;
		nhi = ntris - nlo;
		if ((n->nodes = BuildTriTree(lo, nlo)) != NULL)
			n->nodes->next = BuildTriTree(hi, nhi);
	}
	else
	{
		n->tris = tris;
		n->nodes = NULL;
	}

	return n;
}

void DeleteTriTree(MeshNode *n)
{
	while (n != NULL)
	{
		MeshTri *t;
		MeshNode *tn = n;
		n = tn->next;
		while (tn->tris != NULL)
		{
			t = tn->tris;
			tn->tris = t->next;
			Ray_DeleteMeshTri(t);
		}
		DeleteTriTree(tn->nodes);
		DeleteMeshNode(tn);
	}
}

MeshNode *NewMeshNode(void)
{
	MeshNode *n = (MeshNode *)Calloc(1, sizeof(MeshNode));
	if (n != NULL)
	{
		n->next = n->nodes = NULL;
		n->tris = NULL;
	}
	return n;
}

void DeleteMeshNode(MeshNode *n)
{
	Free(n, sizeof(MeshNode));
}

MeshLocalData *NewMeshLocalData(void)
{
	MeshLocalData *ml = (MeshLocalData *)Calloc(1, sizeof(MeshLocalData));
	ml->hits = NewMeshHit();
	return ml;
}

void DeleteMeshLocalData(MeshLocalData *ml)
{
	MeshHit * h;
	if (ml != NULL)
	{
		while (ml->hits != NULL)
		{
			h = ml->hits;
			ml->hits = h->next;
			DeleteMeshHit(h);
		}
	}
	Free(ml, sizeof(MeshLocalData));
}

MeshHit *NewMeshHit(void)
{
	MeshHit *h = (MeshHit *)Calloc(1, sizeof(MeshHit));
	h->next = NULL;
	return h;
}

void DeleteMeshHit(MeshHit *h)
{
	Free(h, sizeof(MeshHit));
}

MeshHit *GetNextMeshHit(MeshHit *h)
{
	if (h->next == NULL)
		h->next = NewMeshHit();
	return h->next;
}


/*************************************************************************
*
*	Draws a wire frame view of object.
*
*************************************************************************/

void DrawTriTree(MeshNode *n, Xform *T)
{
	while (n != NULL)
	{
		MeshTri *t;
		Vec3 P;
		for (t = n->tris; t != NULL; t = t->next)
		{
			V3Set(&P, t->v1->x, t->v1->y, t->v1->z); 
			if (T != NULL)
				PointToWorld(&P, T);
			Set_Pt(0, P.x, P.y, P.z);
			V3Set(&P, t->v2->x, t->v2->y, t->v2->z); 
			if (T != NULL)
				PointToWorld(&P, T);
			Set_Pt(1, P.x, P.y, P.z);
			V3Set(&P, t->v3->x, t->v3->y, t->v3->z); 
			if (T != NULL)
				PointToWorld(&P, T);
			Set_Pt(2, P.x, P.y, P.z);
			Move_To(0); Line_To(1); Line_To(2); Line_To(0);
		}
		DrawTriTree(n->nodes, T);
		n = n->next;
	}
}

void DrawMesh(Object *obj)
{
	MeshData *m;

	m = obj->data.mesh;
	DrawTriTree(m->tree, obj->T);
}
