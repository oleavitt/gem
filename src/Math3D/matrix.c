/*************************************************************************
*
*  matrix.c - Matrix functions.
*
*************************************************************************/

#include "local.h"

static Mat3x3 tmp3x3;
static Mat4x4 tmp4x4;

/*************************************************************************
*
*  3x3 matrix functions.
*
*************************************************************************/
void M3x3Copy(Mat3x3 *Mdest, Mat3x3 *Msrc)
	{
	memcpy(Mdest, Msrc, sizeof(Mat3x3));
	}

void M3x3Identity(Mat3x3 *M)
	{
	int i, j;
	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			M->e[i][j] = (i == j) ? 1.0 : 0.0;
	}

void M3x3Transpose(Mat3x3 *M)
	{
	double t;
	t = M->e[0][1]; M->e[0][1] = M->e[1][0]; M->e[1][0] = t;
	t = M->e[0][2]; M->e[0][2] = M->e[2][0]; M->e[2][0] = t;
	t = M->e[1][2]; M->e[1][2] = M->e[2][1]; M->e[2][1] = t;
	}

void M3x3Mul(Mat3x3 *Mout, Mat3x3 *M1, Mat3x3 *M2)
	{
	int i, j;
	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			{
			tmp3x3.e[i][j] = M1->e[i][0] * M2->e[0][j] +
			                 M1->e[i][1] * M2->e[1][j] + 
						  	  	   M1->e[i][2] * M2->e[2][j];
			}
	memcpy(Mout, &tmp3x3, sizeof(Mat3x3));
	}

/*************************************************************************
*
*  M3x3Inverse() - Compute the inverse of matrix I using Gauss-Jordan
*  ellimination with partial pivoting. Return the result in matrix I.
* 
*************************************************************************/
void M3x3Inverse(Mat3x3* I)
	{
	int i, j, k, p;
	double d;

	/* tmp3x3 will evolve from original matrix into identity as I evolves
	 * from identity into inverse of original.
	 */
	memcpy(&tmp3x3, I, sizeof(Mat3x3));  
	M3x3Identity(I);

	/* Loop over the columns of original matrix from left to right, 
	 * elliminating above and below diagonals. 
	 */
	/* Find largest pivot in column j among rows j..3 */
	for(j = 0; j < 3; j++)
		{
		p = j;   /* Row with the largest pivot candidate. */
		for(i = j + 1; i < 3; i++)
			if(fabs(tmp3x3.e[j][i]) > fabs(tmp3x3.e[j][p]))
				p = i;

		/* Swap rows i1 and j in tmpM and I to put pivot on diagonal. */
		for(i = 0; i < 3; i++)
			{
			d = tmp3x3.e[i][p]; tmp3x3.e[i][p] = tmp3x3.e[i][j]; tmp3x3.e[i][j] = d;
			d = I->e[i][p]; I->e[i][p] = I->e[i][j]; I->e[i][j] = d;
			}

		/* Scale row j to have a unit diagonal. */
		d = tmp3x3.e[j][j];
		if(d == 0.0)
			return;  /* Singular matrix can not be inverted. */
		for(i = 0; i < 3; i++)
			{
			I->e[i][j] /= d;
			tmp3x3.e[i][j] /= d;
			}

		/* Elliminate off-diagonal elements in column j of both tmp3x3 and I */
		for(i = 0; i < 3; i++)
			{
			if(i != j)
				{
				for(k = 0; k < 3; k++)
					{
					d = tmp3x3.e[j][i];
					I->e[k][i] -= d * I->e[k][j];
					tmp3x3.e[k][i] -= d * tmp3x3.e[k][j];
					}
				}
			}
		}
	}

/*************************************************************************
*
*  4x4 matrix functions.
*
*************************************************************************/

void M4x4Copy(Mat4x4 *Mdest, Mat4x4 *Msrc)
	{
	memcpy(Mdest, Msrc, sizeof(Mat4x4));
	}

void M4x4Identity(Mat4x4 *M)
	{
	int i, j;
	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			M->e[i][j] = (i == j) ? 1.0 : 0.0;
	}

void M4x4Transpose(Mat4x4 *M)
	{
	double t;
	t = M->e[0][1]; M->e[0][1] = M->e[1][0]; M->e[1][0] = t;
	t = M->e[0][2]; M->e[0][2] = M->e[2][0]; M->e[2][0] = t;
	t = M->e[0][3]; M->e[0][3] = M->e[3][0]; M->e[3][0] = t;
	t = M->e[1][2]; M->e[1][2] = M->e[2][1]; M->e[2][1] = t;
	t = M->e[1][3]; M->e[1][3] = M->e[3][1]; M->e[3][1] = t;
	t = M->e[2][3]; M->e[2][3] = M->e[3][2]; M->e[3][2] = t;
	}

void M4x4Mul(Mat4x4 *Mout, Mat4x4 *M1, Mat4x4 *M2)
	{
	int i, j;
	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			{
			tmp4x4.e[i][j] = M1->e[i][0] * M2->e[0][j] +
			                 M1->e[i][1] * M2->e[1][j] + 
								  		 M1->e[i][2] * M2->e[2][j] + 
									  	 M1->e[i][3] * M2->e[3][j];
			}
	memcpy(Mout, &tmp4x4, sizeof(Mat4x4));
	}

/*************************************************************************
*
*  M4x4Inverse() - Compute the inverse of matrix I using Gauss-Jordan
*  ellimination with partial pivoting. Return the result in matrix I.
* 
*************************************************************************/
void M4x4Inverse(Mat4x4* I)
	{
	int i, j, k, p;
	double d;

	/* tmp4x4 will evolve from original matrix into identity as I evolves
	 * from identity into inverse of original.
	 */
	memcpy(&tmp4x4, I, sizeof(Mat4x4));  
	M4x4Identity(I);

	/* Loop over the columns of original matrix from left to right, 
	 * elliminating above and below diagonals. 
	 */
	/* Find largest pivot in column j among rows j..3 */
	for(j = 0; j < 4; j++)
		{
		p = j;   /* Row with the largest pivot candidate. */
		for(i = j + 1; i < 4; i++)
			if(fabs(tmp4x4.e[j][i]) > fabs(tmp4x4.e[j][p]))
				p = i;

		/* Swap rows i1 and j in tmpM and I to put pivot on diagonal. */
		for(i = 0; i < 4; i++)
			{
			d = tmp4x4.e[i][p]; tmp4x4.e[i][p] = tmp4x4.e[i][j]; tmp4x4.e[i][j] = d;
			d = I->e[i][p]; I->e[i][p] = I->e[i][j]; I->e[i][j] = d;
			}

		/* Scale row j to have a unit diagonal. */
		d = tmp4x4.e[j][j];
		if(d == 0.0)
			return;  /* Singular matrix can not be inverted. */
		for(i = 0; i < 4; i++)
			{
			I->e[i][j] /= d;
			tmp4x4.e[i][j] /= d;
			}

		/* Elliminate off-diagonal elements in column j of both tmp4x4 and I */
		for(i = 0; i < 4; i++)
			{
			if(i != j)
				{
				for(k = 0; k < 4; k++)
					{
					d = tmp4x4.e[j][i];
					I->e[k][i] -= d * I->e[k][j];
					tmp4x4.e[k][i] -= d * tmp4x4.e[k][j];
					}
				}
			}
		}
	}


void M4x4Translate(Mat4x4 *M, Vec3 *V)
	{
	M4x4Identity(&tmp4x4);
	tmp4x4.e[3][0] = V->x;
	tmp4x4.e[3][1] = V->y;
	tmp4x4.e[3][2] = V->z;
	M4x4Mul(M, M, &tmp4x4);
	}


void M4x4Scale(Mat4x4 *M, Vec3 *V)
	{
	M4x4Identity(&tmp4x4);
	tmp4x4.e[0][0] = V->x;
	tmp4x4.e[1][1] = V->y;
	tmp4x4.e[2][2] = V->z;
	M4x4Mul(M, M, &tmp4x4);
	}


void M4x4RotateX(Mat4x4 *M, double angle)
	{
	double c, s;
	c = cos(angle);
	s = sin(angle);
	M4x4Identity(&tmp4x4);
	tmp4x4.e[1][1] = c;
	tmp4x4.e[1][2] = -s;
	tmp4x4.e[2][1] = s;
	tmp4x4.e[2][2] = c;
	M4x4Mul(M, M, &tmp4x4);
	}


void M4x4RotateY(Mat4x4 *M, double angle)
	{
	double c, s;
	c = cos(angle);
	s = sin(angle);
	M4x4Identity(&tmp4x4);
	tmp4x4.e[0][0] = c;
	tmp4x4.e[0][2] = -s;
	tmp4x4.e[2][0] = s;
	tmp4x4.e[2][2] = c;
	M4x4Mul(M, M, &tmp4x4);
	}


void M4x4RotateZ(Mat4x4 *M, double angle)
	{
	double c, s;
	c = cos(angle);
	s = sin(angle);
	M4x4Identity(&tmp4x4);
	tmp4x4.e[0][0] = c;
	tmp4x4.e[0][1] = -s;
	tmp4x4.e[1][0] = s;
	tmp4x4.e[1][1] = c;
	M4x4Mul(M, M, &tmp4x4);
	}


void M4x4Rotate(Mat4x4 *M, Vec3 *axis, double angle)
	{
	double c, s, t, tax, tay;
	c = cos(angle);
	s = sin(angle);
	t = 1.0 - c;
	tax = t * axis->x;
	tay = t * axis->y;
	M4x4Identity(&tmp4x4);
	tmp4x4.e[0][0] = tax * axis->x + c;
	tmp4x4.e[0][1] = tax * axis->y - s * axis->z;
	tmp4x4.e[0][2] = tax * axis->z + s * axis->y;
	tmp4x4.e[1][0] = tax * axis->y + s * axis->z;
	tmp4x4.e[1][1] = tay * axis->y + c;
	tmp4x4.e[1][2] = tay * axis->z - s * axis->x;
	tmp4x4.e[2][0] = tax * axis->z - s * axis->y;
	tmp4x4.e[2][1] = tay * axis->z + s * axis->x;
	tmp4x4.e[2][2] = t * axis->z * axis->z + c;
	M4x4Mul(M, M, &tmp4x4);
	}


void M4x4Perspective(Mat4x4 *M, double height, double zmin, double zmax)
	{
	M4x4Identity(&tmp4x4);
	tmp4x4.e[2][2] =  (height * zmax) / (zmin * (zmax - zmin));
	tmp4x4.e[2][3] =  height / zmin;
	tmp4x4.e[3][2] = -(height * zmax) / (zmax - zmin);
	tmp4x4.e[3][3] =  0.0;
	M4x4Mul(M, M, &tmp4x4);
	}


void M4x4XformPt(Vec3 *V, Mat4x4 *M)
	{
	Vec3 V0 = *V;
	V->x = V0.x*M->e[0][0] + V0.y*M->e[1][0] + V0.z*M->e[2][0] + M->e[3][0];
	V->y = V0.x*M->e[0][1] + V0.y*M->e[1][1] + V0.z*M->e[2][1] + M->e[3][1];
	V->z = V0.x*M->e[0][2] + V0.y*M->e[1][2] + V0.z*M->e[2][2] + M->e[3][2];
	}

void M4x4ProjectPt(Vec3 *V, Mat4x4 *M)
	{
	double w;
	Vec3 V0 = *V;
	V->x = V0.x*M->e[0][0] + V0.y*M->e[1][0] + V0.z*M->e[2][0] + M->e[3][0];
	V->y = V0.x*M->e[0][1] + V0.y*M->e[1][1] + V0.z*M->e[2][1] + M->e[3][1];
	V->z = V0.x*M->e[0][2] + V0.y*M->e[1][2] + V0.z*M->e[2][2] + M->e[3][2];
	w    = V0.x*M->e[0][3] + V0.y*M->e[1][3] + V0.z*M->e[2][3] + M->e[3][3];
	if(w != 0.0)
		{
		V->x /= w;
		V->y /= w;
		V->z /= w;
		}
	}

void M4x4XformDir(Vec3 *V, Mat4x4 *M)
	{
	Vec3 V0 = *V;
	V->x = V0.x*M->e[0][0] + V0.y*M->e[1][0] + V0.z*M->e[2][0];
	V->y = V0.x*M->e[0][1] + V0.y*M->e[1][1] + V0.z*M->e[2][1];
	V->z = V0.x*M->e[0][2] + V0.y*M->e[1][2] + V0.z*M->e[2][2];
	}

void M4x4XformNormal(Vec3 *V, Mat4x4 *M)
	{
	Vec3 V0 = *V;
	V->x = V0.x*M->e[0][0] + V0.y*M->e[0][1] + V0.z*M->e[0][2];
	V->y = V0.x*M->e[1][0] + V0.y*M->e[1][1] + V0.z*M->e[1][2];
	V->z = V0.x*M->e[2][0] + V0.y*M->e[2][1] + V0.z*M->e[2][2];
	V3Normalize(V);
	}

