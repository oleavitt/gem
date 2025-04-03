/***************************************************************************
*
*  TRI.C - The triangle primative and its related functions.
*
***************************************************************************/

#include "ray.h"

int CalcIntersectTri ( OBJECT *obj, RAY *ray, HIT *hits )
{
	TRI *tri;
	DBL d, t, x, y, x0, y0, x1, y1;
	VECTOR P;
	int i, A, B, in;

	triangle_tests++;

	tri = (TRI *) obj -> data;

	/* Intersect ray with plane containing triangle. */
	P.x = ray->O->x - tri->pts[0];
	P.y = ray->O->y - tri->pts[1];
	P.z = ray->O->z - tri->pts[2];

	d = tri->norm[0] * ray->D->x +
			tri->norm[1] * ray->D->y +
			tri->norm[2] * ray->D->z;

	/* ray is parallel to plane if d == 0 */
	if ( fabs ( d ) < EPSILON )
		return 0;

	t = - ( tri->norm[0] * P.x +
					tri->norm[1] * P.y +
					tri->norm[2] * P.z ) / d;

	/* 0 (miss) if behind viewer or too small. */
	if ( t < ray->min_dist || t > ray->max_dist )
		return 0;

	hits->entering = ( d < 0.0 ) ? 1 : 0;

	/*
	 * Project the triangle vertices to the 2D plane
	 * that is most perpendicular to the plane normal.
	 * This has already been pre-computed and the result
	 * is stored in "tri->flags".
	 */
	switch ( tri->flags & 0x03 )  /* Mask out the axis bits. */
	{
		case X_AXIS:
			A = 1;
			B = 2;
			x = ray->O->y + ray->D->y * t;
			y = ray->O->z + ray->D->z * t;
			break;
		case Y_AXIS:
			A = 0;
			B = 2;
			x = ray->O->x + ray->D->x * t;
			y = ray->O->z + ray->D->z * t;
			break;
		default:  /* Z axis. */
			A = 0;
			B = 1;
			x = ray->O->x + ray->D->x * t;
			y = ray->O->y + ray->D->y * t;
			break;
	}

	/*
	 * Compute 2D point on the projected plane and
	 * see if it is within the bounds of the projected triangle.
	 */
	in = 0;
	for ( i = 0; i < 3; i++ )
	{
		x0 = tri->pts[i*3+A];
		y0 = tri->pts[i*3+B];
		if ( i < 2 )
		{
			x1 = tri->pts[(i+1)*3+A];
			y1 = tri->pts[(i+1)*3+B];
		}
		else
		{
			x1 = tri->pts[A];
			y1 = tri->pts[B];
		}

		if ( ( y0 < y && y1 < y ) ||
				 ( y0 > y && y1 > y ) ||
				 ( x0 < x && x1 < x ) ) continue;

		if ( x0 > x && x1 > x )
		{
			in = 1 - in;
			continue;
		}

		d = ( y0 - y ) / ( y0 - y1 );
		d = x0 * ( 1.0 - d ) + x1 * d;

		if ( x < d )
			in = 1 - in;
	}

	if ( in )
	{
		hits->t = t;
		hits->obj = obj;
		triangle_hits++;
		return 1;
	}
	return 0;
} /* end of CalcIntersectTri() */

VOID CalcNormalTri ( OBJECT *obj, VECTOR *Q, VECTOR *N )
{
	TRI *tri;

	tri = (TRI *)obj->data;

	if ( tri->flags & PHONG_FLAG )
	{
		PTRI *ptri;
		DBL area, a1, a2, a3, u, v;
		FLT *P1, *P2, *P3;
		FLT *N1, *N2, *N3;
		int A, B;

		ptri = (PTRI *) tri;

		/*
		 * Project the triangle vertices to the 2D plane
		 * that is most perpendicular to the plane normal.
		 */
		switch ( ptri->flags & 0x03 )  /* Mask out the axis bits. */
		{
			case X_AXIS:
				A = 1;
				B = 2;
				u = Q->y;
				v = Q->z;
				break;
			case Y_AXIS:
				A = 0;
				B = 2;
				u = Q->x;
				v = Q->z;
				break;
			default:  /* Z axis. */
				A = 0;
				B = 1;
				u = Q->x;
				v = Q->y;
				break;
		}

		P1 = ptri->pts;
		P2 = &ptri->pts[3];
		P3 = &ptri->pts[6];

		/* Determine barycentric coordinates of point... */
		area = TriArea ( P1[A], P1[B], P2[A], P2[B], P3[A], P3[B] );
		a1 =   TriArea ( u, v, P2[A], P2[B], P3[A], P3[B] ) / area;
		a2 =   TriArea ( P1[A], P1[B], u, v, P3[A], P3[B] ) / area;
		a3 =   1.0 - a1 - a2;

		/*
		 * ...and use them to interpolate between the vertex normals
		 * to get the real normal.
		 */
		N1 = ptri->norms;
		N2 = &ptri->norms[3];
		N3 = &ptri->norms[6];
		N->x = N1[0] * a1 + N2[0] * a2 + N3[0] * a3;
		N->y = N1[1] * a1 + N2[1] * a2 + N3[1] * a3;
		N->z = N1[2] * a1 + N2[2] * a2 + N3[2] * a3;
		VNorm ( N );
	}
	else   /* Just return the pre-computed plane normal. */
	{
		N->x = tri->norm[0];
		N->y = tri->norm[1];
		N->z = tri->norm[2];
	}
} /* end of CalcNormalTri() */

VOID CalcUVMapTri ( OBJECT *obj, VECTOR *Q, DBL *u, DBL *v )
{
	TRI *tri;
	DBL area, a1, a2, a3, pu, pv;
	FLT *P1, *P2, *P3;
	int A, B;

	tri = (TRI *)obj->data;

	/*
	 * Project the triangle vertices to the 2D plane
	 * that is most perpendicular to the plane normal.
	 */
	switch ( tri->flags & 0x03 )  /* Mask out the axis bits. */
	{
		case X_AXIS:
			A = 1;
			B = 2;
			pu = Q->y;
			pv = Q->z;
			break;
		case Y_AXIS:
			A = 0;
			B = 2;
			pu = Q->x;
			pv = Q->z;
			break;
		default:  /* Z axis. */
			A = 0;
			B = 1;
			pu = Q->x;
			pv = Q->y;
			break;
	}

	P1 = tri->pts;
	P2 = &tri->pts[3];
	P3 = &tri->pts[6];

	/* Determine barycentric coordinates of point... */
	area = TriArea ( P1[A], P1[B], P2[A], P2[B], P3[A], P3[B] );
	a1 =   TriArea ( pu, pv, P2[A], P2[B], P3[A], P3[B] ) / area;
	a2 =   TriArea ( P1[A], P1[B], pu, pv, P3[A], P3[B] ) / area;
	a3 =   1.0 - a1 - a2;

	/*
	 * ...and use them to interpolate between the UV coordinates
	 * for each vertex to get actual UV position.
	 */
	*u = tri->uv[0] * a1 + tri->uv[2] * a2 + tri->uv[4] * a3;
	*v = tri->uv[1] * a1 + tri->uv[3] * a2 + tri->uv[5] * a3;

} /* end of CalcUVMapTri() */

/*
 * Compute the surface area (X 2) of a 2D triagle.
 */
DBL TriArea ( DBL x1, DBL y1, DBL x2, DBL y2, DBL x3, DBL y3 )
{
	return fabs(x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
} /* end of TriArea() */

VOID FixTriNormal ( OBJECT *obj, VECTOR *D, VECTOR *N )
{
	TRI *tri;

	tri = (TRI *)obj->data;

	if ( ( D->x * tri->norm[0] +
				 D->y * tri->norm[1] +
				 D->z * tri->norm[2] ) > 0.0 )
	{          /* see if we need to flip... */
		if ( VDot ( D, N ) > 0.0 )
		{         /* we need to flip */
			VNegate ( N );
		}
	}
} /* end of FixTriNormal() */

VOID CalcExtentsTri ( OBJECT *obj, VECTOR *omin, VECTOR *omax )
{
	TRI *tri;
	FLT *pts;

	tri = (TRI *)obj->data;
	pts = tri->pts;

	omin->x = min(min(pts[0],pts[3]),pts[6]);
	omax->x = max(max(pts[0],pts[3]),pts[6]);
	omin->y = min(min(pts[1],pts[4]),pts[7]);
	omax->y = max(max(pts[1],pts[4]),pts[7]);
	omin->z = min(min(pts[2],pts[5]),pts[8]);
	omax->z = max(max(pts[2],pts[5]),pts[8]);

} /* end of CalcExtentsTri() */

int PointInsideTri ( OBJECT *obj, VECTOR *Q )
{
	TRI *tri;
	VECTOR P, N;

	tri = (TRI *)obj->data;

	P.x = Q->x - tri->pts[0];
	P.y = Q->y - tri->pts[1];
	P.z = Q->z - tri->pts[2];
	N.x = tri->norm[0];
	N.y = tri->norm[1];
	N.z = tri->norm[2];

	if ( VDot ( &P, &N ) > 0.0 )
		return ( obj -> flags & OBJ_FLAG_INVERSE ); /* not inside */
	return ( ! ( obj -> flags & OBJ_FLAG_INVERSE ) ); /* inside */
} /* end of PointInsideTri() */

VOID XformTri ( OBJECT *obj, VECTOR *params, int type )
{
	VECTOR V;
	TRI *tri;
	PTRI *ptri;

	tri = (TRI *)obj->data;

	V.x = tri->pts[0];
	V.y = tri->pts[1];
	V.z = tri->pts[2];
	XformVector ( &V, params, type );
	tri->pts[0] = V.x;
	tri->pts[1] = V.y;
	tri->pts[2] = V.z;
	V.x = tri->pts[3];
	V.y = tri->pts[4];
	V.z = tri->pts[5];
	XformVector ( &V, params, type );
	tri->pts[3] = V.x;
	tri->pts[4] = V.y;
	tri->pts[5] = V.z;
	V.x = tri->pts[6];
	V.y = tri->pts[7];
	V.z = tri->pts[8];
	XformVector ( &V, params, type );
	tri->pts[6] = V.x;
	tri->pts[7] = V.y;
	tri->pts[8] = V.z;

	if ( tri->flags & PHONG_FLAG )
	{
		ptri = (PTRI *)tri;
		V.x = ptri->norms[0];
		V.y = ptri->norms[1];
		V.z = ptri->norms[2];
		XformNormal ( &V, params, type );
		ptri->norms[0] = V.x;
		ptri->norms[1] = V.y;
		ptri->norms[2] = V.z;
		V.x = ptri->norms[3];
		V.y = ptri->norms[4];
		V.z = ptri->norms[5];
		XformNormal ( &V, params, type );
		ptri->norms[3] = V.x;
		ptri->norms[4] = V.y;
		ptri->norms[5] = V.z;
		V.x = ptri->norms[6];
		V.y = ptri->norms[7];
		V.z = ptri->norms[8];
		XformNormal ( &V, params, type );
		ptri->norms[6] = V.x;
		ptri->norms[7] = V.y;
		ptri->norms[8] = V.z;
		ComputeTriFields ( tri, tri->pts, ptri->norms );
	}
	else
	{
		ComputeTriFields ( tri, tri->pts, NULL );
	}

} /* end of XformTri() */

VOID ComputeTriFields ( TRI *tri, FLT *pts, FLT *norms )
{
	int i;
	VECTOR V1, V2, N;
	PTRI *ptri;

	/* Fill in the points. */
	for ( i = 0; i < 9; i++ )
		tri->pts[i] = pts[i];

	/* Compute plane normal. */
	V1.x = pts[3] - pts[0];
	V2.x = pts[6] - pts[0];
	V1.y = pts[4] - pts[1];
	V2.y = pts[7] - pts[1];
	V1.z = pts[5] - pts[2];
	V2.z = pts[8] - pts[2];

	VCross ( &N, &V1, &V2 );
	VNorm ( &N );
	tri->norm[0] = N.x;
	tri->norm[1] = N.y;
	tri->norm[2] = N.z;

	/* Zero out previous axis flags. */
	tri->flags = 0;

	/* If Phong shaded, fill in and normalize the normals. */
	if ( NULL != norms )
	{
		DBL len;
		int j;

		tri->flags = PHONG_FLAG;
		ptri = (PTRI *) tri;

		j = 0;
		for ( i = 0; i < 3; i++ )
		{
			if( ( norms[j] * N.x +
						norms[j+1] * N.y +
						norms[j+2] * N.z ) < 0.0 )
			{
				norms[j] = - norms[j];
				norms[j+1] = - norms[j+1];
				norms[j+2] = - norms[j+2];
			}

			len = sqrt ( norms[j]   * norms[j]
								 + norms[j+1] * norms[j+1]
								 + norms[j+2] * norms[j+2] );

			if ( len > EPSILON )
			{
				ptri->norms[j] = norms[j++] / len;
				ptri->norms[j] = norms[j++] / len;
				ptri->norms[j] = norms[j++] / len;
			}
			else   /* Basket case, use the plane normal. */
			{
				ptri->norms[j++] = N.x;
				ptri->norms[j++] = N.y;
				ptri->norms[j++] = N.z;
			}
		}
	}

	/* Determine axis of greatest projection. */
	if ( fabs ( N.x ) > fabs ( N.z ) )
	{
		if ( fabs ( N.x ) > fabs ( N.y ) )
			tri->flags |= X_AXIS;
		else if ( fabs ( N.y ) > fabs ( N.z ) )
			tri->flags |= Y_AXIS;
		else
			tri->flags |= Z_AXIS;
	}
	else if ( fabs ( N.y ) > fabs ( N.z ) )
	{
		tri->flags |= Y_AXIS;
	}
	else
	{
		tri->flags |= Z_AXIS;
	}

} /* end of ComputeTriFields() */

VOID CloneTri ( OBJECT *destobj, OBJECT *srcobj )
{
	if ( ((TRI *)srcobj->data)->flags & PHONG_FLAG )
	{
		destobj->data = rmalloc ( sizeof(PTRI) );
		memcpy ( destobj->data, srcobj->data, sizeof(PTRI) );
	}
	else
	{
		destobj->data = rmalloc ( sizeof(TRI) );
		memcpy ( destobj->data, srcobj->data, sizeof(TRI) );
	}
} /* end of CloneTri() */

VOID MakeTri ( OBJECT *obj, PARAMS *par )
{
	TRI *tri;
	int i;
	FLT pts[9], norms[9], uv[6], *normals;

	/* Get the vertex points. */
	pts[0] = par->V.x;
	pts[1] = par->V.y;
	pts[2] = par->V.z;
	par = par->next;
	pts[3] = par->V.x;
	pts[4] = par->V.y;
	pts[5] = par->V.z;
	par = par->next;
	pts[6] = par->V.x;
	pts[7] = par->V.y;
	pts[8] = par->V.z;

	/* See if UV parameters follow... */
	if ( par->more && TK_FLOAT == par->next->type )
	{
		/* Get 3 UV pairs as U1,V1,U2,V2,U3,V3. */
		for ( i = 0; i < 6; i++ )
		{
			par = par->next;
			uv[i] = par->V.x;
		}
	}
	else
	{
		uv[0] = 0.0;
		uv[1] = 0.0;
		uv[2] = 1.0;
		uv[3] = 0.0;
		uv[4] = 0.0;
		uv[5] = 1.0;
	}

	/* See if Phong triangle normals follow... */
	if ( par->more )
	{
		par = par->next;
		norms[0] = par->V.x;
		norms[1] = par->V.y;
		norms[2] = par->V.z;
		par = par->next;
		norms[3] = par->V.x;
		norms[4] = par->V.y;
		norms[5] = par->V.z;
		par = par->next;
		norms[6] = par->V.x;
		norms[7] = par->V.y;
		norms[8] = par->V.z;
		normals = norms;
		/* Allocate the Phong triangle data structure... */
		tri = (TRI *)rmalloc ( sizeof(PTRI) );
	}
	else
	{
		normals = NULL;
		/* Allocate the simple triangle data structure... */
		tri = (TRI *)rmalloc ( sizeof(TRI) );
	}

	/* Some assembly required... */
	ComputeTriFields ( tri, pts, normals );

	/* Copy in the UV coordinates... */
	for ( i = 0; i < 6; i++ )
		tri->uv[i] = uv[i];

	/* Configure the object as a triangle type. */
	obj->data = tri;
	obj->type = OBJ_TRI;

	/* Don't shadow test against self. */
	obj->flags |= OBJ_FLAG_NO_SELF_INTERSECT;

} /* end of MakeTri() */

/*
 * end of TRI.C
 */