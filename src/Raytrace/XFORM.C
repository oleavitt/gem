/*****************************************************************************
*
*  XFORM.C
*  The transform matrix functions - Based on code and ideas from
*  Graphics-Gems (vol I), 1990 Academic Press, Inc.
*
*****************************************************************************/

#include "ray.h"

/****************************************************************************
*
*   Functions for transforming and inverse-transforming points and normals.
*
****************************************************************************/
void PointToWorld(Vec3 *Q, Xform *T)
{
	M4x4XformPt(Q, &T->M);
} /* end of PointToWorld() */

void PointToObject(Vec3 *Q, Xform *T)
{
	M4x4XformPt(Q, &T->I);
} /* end of PointToObject() */

void DirToWorld(Vec3 *D, Xform *T)
{
	M4x4XformDir(D, &T->M);
} /* end of DirToWorld() */

void DirToObject(Vec3 *D, Xform *T)
{
	M4x4XformDir(D, &T->I);
} /* end of DirToObject() */

void NormToWorld(Vec3 *N, Xform *T)
{
	M4x4XformNormal(N, &T->I);
} /* end of NormToWorld() */

void NormToObject(Vec3 *N, Xform *T)
{
	M4x4XformNormal(N, &T->M);
} /* end of NormToObject() */

/*
 * Determine the minima and maxima along the world axes
 * for a bounding box transformed from object to world space.
 */
void BBoxToWorld(Vec3 *bmin, Vec3 *bmax, Xform *T)
{
	Vec3 tbmin, tbmax, V;
	int i;

	V3Set(&tbmin, HUGE, HUGE, HUGE);
	V3Set(&tbmax, -HUGE, -HUGE, -HUGE);

	for(i=0;i<8;i++)
	{
		V.x = (i & 1) ? bmin->x : bmax->x;
		V.y = (i & 2) ? bmin->y : bmax->y;
		V.z = (i & 4) ? bmin->z : bmax->z;

		PointToWorld(&V, T);

		tbmin.x = fmin(tbmin.x, V.x);
		tbmax.x = fmax(tbmax.x, V.x);
		tbmin.y = fmin(tbmin.y, V.y);
		tbmax.y = fmax(tbmax.y, V.y);
		tbmin.z = fmin(tbmin.z, V.z);
		tbmax.z = fmax(tbmax.z, V.z);
	}

	*bmin = tbmin;
	*bmax = tbmax;

} /* end of BBoxToWorld() */

/*
 * Determine the minima and maxima along the world axes
 * for a bounding box transformed from world to object space.
 */
void BBoxToObject(Vec3 *bmin, Vec3 *bmax, Xform *T)
{
	Vec3 tbmin, tbmax, V;
	int i;

	V3Set(&tbmin, HUGE, HUGE, HUGE);
	V3Set(&tbmax, -HUGE, -HUGE, -HUGE);

	for(i=0;i<8;i++)
	{
		V.x = (i & 1) ? bmin->x : bmax->x;
		V.y = (i & 2) ? bmin->y : bmax->y;
		V.z = (i & 4) ? bmin->z : bmax->z;

		PointToObject(&V, T);

		tbmin.x = fmin(tbmin.x, V.x);
		tbmax.x = fmax(tbmax.x, V.x);
		tbmin.y = fmin(tbmin.y, V.y);
		tbmax.y = fmax(tbmax.y, V.y);
		tbmin.z = fmin(tbmin.z, V.z);
		tbmax.z = fmax(tbmax.z, V.z);
	}

	*bmin = tbmin;
	*bmax = tbmax;

} /* end of BBoxToObject() */

/*****************************************************************************
*
*  XformVector() - Directly transforms a vector, "V", according to
*    transform operation, "type".
*
*****************************************************************************/
void XformVector(Vec3 *V, Vec3 *params, int type)
{
	switch(type)
	{
		case XFORM_SCALE:
			V->x *= params->x;
			V->y *= params->y;
			V->z *= params->z;
			break;
		case XFORM_TRANSLATE:
			V->x += params->x;
			V->y += params->y;
			V->z += params->z;
			break;
		case XFORM_SHEAR:
			{
      	int axis;
				/*
				 * params->x = u shift
				 * params->y = v shift
				 * params->z is the shear axis selector - 0 = X, 1 = Y, 2 = Z
				 */
				axis = ((unsigned)params->z % 3);
				switch(axis)
				{
					case 0:
						V->y += V->x * params->x;
						V->z += V->x * params->y;
						break;
					case 1:
						V->x += V->y * params->x;
						V->z += V->y * params->y;
						break;
					default: /* 2 */
						V->x += V->z * params->x;
						V->y += V->z * params->y;
						break;
				}
			}
			break;
		case XFORM_ROTATE:
			{
				double sin_theta, cos_theta;
				Vec3 V0;

				/* rotate x axis */
				V0 = *V;
				sin_theta = sin(params->x * DTOR);
				cos_theta = cos(params->x * DTOR);
				V->y =   V0.y * cos_theta + V0.z * sin_theta;
				V->z = - V0.y * sin_theta + V0.z * cos_theta;
				/* rotate y axis */
				V0 = *V;
				sin_theta = sin(params->y * DTOR);
				cos_theta = cos(params->y * DTOR);
				V->x =   V0.x * cos_theta - V0.z * sin_theta;
				V->z =   V0.x * sin_theta + V0.z * cos_theta;
				/* rotate z axis */
				V0 = *V;
				sin_theta = sin(params->z * DTOR);
				cos_theta = cos(params->z * DTOR);
				V->x =   V0.x * cos_theta + V0.y * sin_theta;
				V->y = - V0.x * sin_theta + V0.y * cos_theta;
			}
			break;
	}
} /* end of XformVector() */


/*****************************************************************************
*
*  XformNormal() - Directly transforms a normal, "N", according to
*    transform operation, "type".
*
*****************************************************************************/
void XformNormal(Vec3 *N, Vec3 *params, int type)
{
	switch(type)
	{
		case XFORM_SCALE:
			if (params->x > 0.0)
			{
				if(params->x > EPSILON) N->x /= params->x;
				else N->x = HUGE;
			}
			else
			{
				if(params->x < - EPSILON) N->x /= params->x;
				else N->x = - HUGE;
			}
			if(params->y > 0.0)
			{
				if (params->y > EPSILON) N->y /= params->y;
				else N->y = HUGE;
			}
			else
			{
				if(params->y < - EPSILON) N->y /= params->y;
				else N->y = - HUGE;
			}
			if(params->z > 0.0)
			{
				if(params->z > EPSILON) N->z /= params->z;
				else N->z = HUGE;
			}
			else
			{
				if(params->z < - EPSILON) N->z /= params->z;
				else N->z = - HUGE;
			}
			V3Normalize(N);
			break;
		case XFORM_TRANSLATE:
			/* No translation. */
			break;
		case XFORM_SHEAR:
			{
				int axis;
				/*
				 * params->x = u shift
				 * params->y = v shift
				 * params->z is the shear axis selector - 0 = X, 1 = Y, 2 = Z
				 */
				axis = ((unsigned)params->z % 3);
				switch(axis)
				{
					case 0:
						N->x -= N->y * params->x;
						N->x -= N->z * params->y;
						break;
					case 1:
						N->y -= N->x * params->x;
						N->y -= N->z * params->y;
						break;
					default: /* 2 */
						N->z -= N->x * params->x;
						N->z -= N->y * params->y;
						break;
				}
				V3Normalize(N);
			}
			break;
		case XFORM_ROTATE:
			{
				double sin_theta, cos_theta;
				Vec3 V;

				/* rotate x axis */
				V = *N;
				sin_theta = sin(params->x * DTOR);
				cos_theta = cos(params->x * DTOR);
				N->y =   V.y * cos_theta + V.z * sin_theta;
				N->z = - V.y * sin_theta + V.z * cos_theta;
				/* rotate y axis */
				V = *N;
				sin_theta = sin(params->y * DTOR);
				cos_theta = cos(params->y * DTOR);
				N->x =   V.x * cos_theta - V.z * sin_theta;
				N->z =   V.x * sin_theta + V.z * cos_theta;
				/* rotate z axis */
				V = *N;
				sin_theta = sin(params->z * DTOR);
				cos_theta = cos(params->z * DTOR);
				N->x =   V.x * cos_theta + V.y * sin_theta;
				N->y = - V.x * sin_theta + V.y * cos_theta;
			}
			break;
	}
} /* end of XformNormal() */


/*************************************************************************
*
*  DirToMatrix() -
*    Decodes an arbitrary direction vector, "dir", into a 3X3 rotation
*  matrix consisting of the separated rotation sines and cosines
*  for the "X", "Y", and "Z" axes relative to the positive "Y" axis.
*  A dir vector of <0,1,0> will return an identity matrix.
*  The matrix is returned in the vectors, "rx", "ry", and "rz".
*
*************************************************************************/
void DirToMatrix(Vec3 *dir, Vec3 *rx, Vec3 *ry, Vec3 *rz)
{
	Vec3 V;
	double c, s, ang;

	/* Start with an identity matrix (no rotations)... */
	V3Set(rx, 1.0, 0.0, 0.0);
	V3Set(ry, 0.0, 1.0, 0.0);
	V3Set(rz, 0.0, 0.0, 1.0);

	/* Grab a copy of the direction vector and normalize it... */
	V = *dir;
	V3Normalize(&V);

	/* Determine angle for x axis rotation... */
	ang = acos(V.y);
	if(ang < EPSILON)
		return; /* No significant x axis rotations.   */
						/* Y axis rotations will have no effect, either. */

	/* Rotate matrix around x axis... */
	s = sin(ang);
	c = V.y;
	V = *ry;
	ry->y =   V.y * c - V.z * s;
	ry->z =   V.y * s + V.z * c;
	V = *rz;
	rz->y =   V.y * c - V.z * s;
	rz->z =   V.y * s + V.z * c;

	/* If x was rotated 180 degrees... */
	if(fabs(fabs(ang) - PI) < EPSILON)
		return;  /* ...Y axis rotations will have no effect. */

	/* Determine angle for y axis rotation... */
	V = *dir;
	V.y = 0.0;
	V3Normalize(&V);
	ang = acos(V.z);
	if(ang < EPSILON)
		return; /* No significant y axis rotations.   */
	if(V.x < 0.0)
		ang = -ang;

	/* Rotate matrix around y axis... */
	s = sin(ang);
	c = V.z;
	V = *rx;
	rx->x =   V.x * c + V.z * s;
	rx->z = - V.x * s + V.z * c;
	V = *ry;
	ry->x =   V.x * c + V.z * s;
	ry->z = - V.x * s + V.z * c;
	V = *rz;
	rz->x =   V.x * c + V.z * s;
	rz->z = - V.x * s + V.z * c;

} /* end of DirToMatrix() */


/*
 * Add a single transform operation vector to the cummulative transform
 * and inverse-transform matrices in "T".
 */

static Mat4x4 tmpmat;

void XformXforms(Xform *T, Vec3 *params, int type)
{
	double c, s;

	if(T == NULL)
    return;

	switch(type)
	{
		case XFORM_TRANSLATE:
			M4x4Identity(&tmpmat);
			tmpmat.e[3][0] = params->x;
			tmpmat.e[3][1] = params->y;
			tmpmat.e[3][2] = params->z;
			M4x4Mul(&T->M, &T->M, &tmpmat);
			tmpmat.e[3][0] = - params->x;
			tmpmat.e[3][1] = - params->y;
			tmpmat.e[3][2] = - params->z;
			M4x4Mul(&T->I, &tmpmat, &T->I);
			break;
		case XFORM_SCALE:
			M4x4Identity(&tmpmat);
			tmpmat.e[0][0] = params->x;
			tmpmat.e[1][1] = params->y;
			tmpmat.e[2][2] = params->z;
			M4x4Mul(&T->M, &T->M, &tmpmat);
			if(params->x != 0.0) tmpmat.e[0][0] = 1.0 / params->x;
			if(params->y != 0.0) tmpmat.e[1][1] = 1.0 / params->y;
			if(params->z != 0.0) tmpmat.e[2][2] = 1.0 / params->z;
			M4x4Mul(&T->I, &tmpmat, &T->I);
			break;
		case XFORM_SHEAR:
			{
				Vec3 V, W;
				int axis;

				/*
				 * params->x = u shift
				 * params->y = v shift
				 * params->z is the shear axis selector - 0 = X, 1 = Y, 2 = Z
				 */
				axis = ((unsigned)params->z % 3);
				V3Set(&V, 0.0, 0.0, 0.0);
				switch(axis)
				{
					case 0:
						V3Set(&W, 1.0, params->x, params->y);
						V3Normalize(&W);
						V.x = tan(acos(W.x));
						V3Set(&W, 0.0, params->x, params->y);
						break;
					case 1:
						V3Set(&W, params->x, 1.0, params->y);
						V3Normalize(&W);
						V.y = tan(acos(W.y));
						V3Set(&W, params->x, 0.0, params->y);
						break;
					default: /* 2 */
						V3Set( &W, params->x, params->y, 1.0);
						V3Normalize(&W);
						V.z = tan(acos(W.z));
  					V3Set(&W, params->x, params->y, 0.0);
						break;
				}
				V3Normalize(&W);
				M4x4Identity(&tmpmat);
				tmpmat.e[0][0] += V.x * W.x;
				tmpmat.e[0][1]  = V.x * W.y;
				tmpmat.e[0][2]  = V.x * W.z;
				tmpmat.e[1][0]  = V.y * W.x;
				tmpmat.e[1][1] += V.y * W.y;
				tmpmat.e[1][2]  = V.y * W.z;
				tmpmat.e[2][0]  = V.z * W.x;
				tmpmat.e[2][1]  = V.z * W.y;
				tmpmat.e[2][2] += V.z * W.z;
				M4x4Mul(&T->M, &T->M, &tmpmat);

				V3Set(&V, 0.0, 0.0, 0.0);
				switch(axis)
				{
					case 0:
						V3Set(&W, 1.0, -params->x, -params->y);
						V3Normalize(&W);
						V.x = tan(acos(W.x));
						V3Set(&W, 0.0, -params->x, -params->y);
						break;
					case 1:
						V3Set(&W, -params->x, 1.0, -params->y);
						V3Normalize(&W);
						V.y = tan(acos(W.y));
						V3Set(&W, -params->x, 0.0, -params->y);
						break;
					default: /* 2 */
						V3Set(&W, -params->x, -params->y, 1.0);
						V3Normalize(&W);
						V.z = tan(acos(W.z));
						V3Set(&W, -params->x, -params->y, 0.0);
						break;
				}
				V3Normalize(&W);
				M4x4Identity(&tmpmat);
				tmpmat.e[0][0] += V.x * W.x;
				tmpmat.e[0][1]  = V.x * W.y;
				tmpmat.e[0][2]  = V.x * W.z;
				tmpmat.e[1][0]  = V.y * W.x;
				tmpmat.e[1][1] += V.y * W.y;
				tmpmat.e[1][2]  = V.y * W.z;
				tmpmat.e[2][0]  = V.z * W.x;
				tmpmat.e[2][1]  = V.z * W.y;
				tmpmat.e[2][2] += V.z * W.z;
				M4x4Mul(&T->I, &tmpmat, &T->I);
			}
			break;
		case XFORM_ROTATE:
		/* Rotate the "x" axis. */
			if(0.0 != params->x)
			{
				c = cos(params->x * DTOR);
				s = sin(params->x * DTOR);
				M4x4Identity(&tmpmat);
				tmpmat.e[1][1] = c;
				tmpmat.e[1][2] = -s;
				tmpmat.e[2][1] = s;
				tmpmat.e[2][2] = c;
				M4x4Mul(&T->M, &T->M, &tmpmat);
				tmpmat.e[1][2] = s;
				tmpmat.e[2][1] = -s;
				M4x4Mul(&T->I, &tmpmat, &T->I);
			}
		/* Rotate the "y" axis. */
			if(0.0 != params->y)
			{
				c = cos(params->y * DTOR);
				s = sin(params->y * DTOR);
				M4x4Identity(&tmpmat);
				tmpmat.e[0][0] = c;
				tmpmat.e[0][2] = s;
				tmpmat.e[2][0] = -s;
				tmpmat.e[2][2] = c;
				M4x4Mul(&T->M, &T->M, &tmpmat);
				tmpmat.e[0][2] = -s;
				tmpmat.e[2][0] = s;
				M4x4Mul(&T->I, &tmpmat, &T->I);
			}
		/* Rotate the "z" axis. */
			if(0.0 != params->z)
			{
				c = cos(params->z * DTOR);
				s = sin(params->z * DTOR);
				M4x4Identity(&tmpmat);
				tmpmat.e[0][0] = c;
				tmpmat.e[0][1] = -s;
				tmpmat.e[1][0] = s;
				tmpmat.e[1][1] = c;
				M4x4Mul(&T->M, &T->M, &tmpmat);
				tmpmat.e[0][1] = s;
				tmpmat.e[1][0] = -s;
				M4x4Mul(&T->I, &tmpmat, &T->I);
			}
			break;
		default:
			break;
	}
} /* end of XformXforms() */


/*
 * Concatenate the transform matrices of "Tnew" to the ones in T.
 */
void ConcatXforms(Xform *T, Xform *Tnew)
{
	assert(T != NULL);
	assert(Tnew != NULL);
	M4x4Mul(&T->M, &T->M, &Tnew->M);
	M4x4Mul(&T->I, &Tnew->I, &T->I);
} /* end of ConcatXforms() */


/*
 * Allocate and initialize a new tranformation matrix structure.
 */
Xform *Ray_NewXform(void)
{
	Xform *New;

	New = (Xform *)Malloc(sizeof(Xform));
  if(New != NULL)
  {
  	M4x4Identity(&New->M);
	  M4x4Identity(&New->I);
    New->nrefs = 1;
  }
	return New;
} /* end of NewXform() */

/*
 * If an existing Xform, "T", is passed to Ray_CloneXform(),
 * Allocate and initialize a new tranformation matrix structure,
 * and copy in contents of existing Xform. Return pointer to
 * new Xform. Returns NULL if "T" is NULL.
 */
Xform *Ray_CloneXform(Xform *T)
{
	Xform *New;

	if(T == NULL)
		return NULL;

	New = Ray_NewXform();
	memcpy(New, T, sizeof(Xform));
  New->nrefs = 1;

	return New;
} /* end of CloneXform() */

/*
 * If an existing Xform, "T", is passed to Ray_CopyXform(),
 * Return a reference copy of "T".
 * Returns NULL if "T" is NULL.
 */
Xform *Ray_CopyXform(Xform *T)
{
	if(T != NULL)
  	T->nrefs++;
	return T;
} /* end of Ray_CopyXform() */

/*
 * Delete a transform after all references to it have been deleted.
 * Returns NULL if "T" is NULL.
 */
Xform *Ray_DeleteXform(Xform *T)
{
	if((T != NULL) && (--T->nrefs == 0))
    Free(T, sizeof(Xform));
	return NULL;
} /* end of Ray_DeleteXform() */

