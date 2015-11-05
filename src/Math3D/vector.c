/*************************************************************************
*
*  vector.c - Functions that operate on vectors.
*
*************************************************************************/

#include "local.h"

void RotatePoint3D(Vec3 *P, double angle, Vec3 *axis)
{
	double c, s, t, tax, tay;
	Vec3 P2;

	c = cos(angle); s = sin(angle); t = 1.0 - c;
	tax = t * axis->x;
	tay = t * axis->y;
	V3Copy(&P2, P);
	P->x = P2.x * (tax * axis->x + c) +
	       P2.y * (tax * axis->y + s * axis->z) +
				 P2.z * (tax * axis->z - s * axis->y);
	P->y = P2.x * (tax * axis->y - s * axis->z) +
	       P2.y * (tay * axis->y + c) +
				 P2.z * (tay * axis->z + s * axis->x);
	P->z = P2.x * (tax * axis->z + s * axis->y) +
	       P2.y * (tay * axis->z - s * axis->x) +
				 P2.z * (t * axis->z * axis->z + c);
}

