/*************************************************************************
*
*  extrude.c - Parses the "extrude" object type's information from
*  the input stream and generates the parameter list for an extruded
*  mesh object.
*
*************************************************************************/

#include "scn.h"


PARAMS *CompileExtrudeParams(void)
{
  PARAMS *plist, *p;
  int cnt, token;

  /* The first param will hold the vertex count. */
	plist = p = New_Param();
  /* Get the vertex list... */
  cnt = 0;
  while(Get_Token() == TK_VERTEX)
  {
    /*
		 * Get vertex, optional normal, optional color, and
		 * optional UV coordinates.
		 */
    p->next = Compile_Params("VO,VO,VO,F,F");
    while(p->next != NULL)
      p = p->next;
    cnt++;
  }
  Unget_Token();
	/* Now that we have a vertex count, put it in the first parameter. */
	plist->V.x = (double)cnt;
  if(cnt < 2)
  {
    SCN_Message(SCN_MSG_ERROR, "extrude: I need at least 2 vertices.");
    return plist;
  }

	/* Get the segments... */
	cnt = 0;
  while((token = Get_Token()) == TK_SEGMENT)
  {
		(void)Expect(TK_LEFTBRACE, "{", "segment");
		if(scn_error_cnt)
			break;
		p->next = New_Param();
		p = p->next;
		p->type = TK_SEGMENT;
	  for(token = Get_Token();
		   (token != TK_RIGHTBRACE) && (scn_error_cnt == 0);
			 token = Get_Token())
		{
			switch(token)
			{
  		/*
			 * Since we know that there is always one vector parameter for all
			 * of the transform cases, the "TK_VECTOR" type param will be
			 * aliased as the transform param. When the param list is
			 * executed we'll just just change the type back to TK_VECTOR
			 * before passing it Eval_Params().
			 * This shortcut helps keep the parameter list short.
			 */
				case TK_ROTATE:
				case TK_SCALE:
				case TK_SHEAR:
				case TK_TRANSLATE:
					p->next = Compile_Params("V");
					p = p->next;
					p->type = token;
					break;
				case TK_REPEAT:
					p->next = Compile_Params("F");
					p = p->next;
					p->type = token;
					break;
				default:
					Err_Unknown(token, NULL, "segment");
					break;
			}
		}
		/* Add end of segment marker. */
		p->next = New_Param();
		p = p->next;
		p->type = TK_RIGHTBRACE;
		cnt++;
	}
	if(token == TK_SMOOTH)
	{
		p->next = New_Param();
		p = p->next;
		p->type = TK_SMOOTH;
	}
	else
		Unget_Token();
	
  if(cnt < 1)
  {
    SCN_Message(SCN_MSG_ERROR, "extrude: I need at least 1 segment.");
    return plist;
  }

  return plist;
}


Object *MakeExtrudeObject(PARAMS *par)
{
	Object *obj;
	MeshData *mesh;
	MeshTri *tris = NULL, *t;
	MeshVertex *v1, *v2, *pv1, *pv2, **newverts, **prevverts;
	Vec3 V;
	Xform *T = NULL, *Tlocal = NULL;   /* Transform matrix for segments. */
	int i, j, k, ntris, nverts, nsegs, reps, smooth;
	double u, v;

	if((mesh = Ray_NewMeshData()) == NULL)
		return NULL;

	/* First parameter is the number of vertices. */
	nverts = (int)par->V.x;
	
  /* Get the initial vertices... */
	par = par->next;
	for(i = 0; i < nverts; i++)
	{
		/* Get the point. */
		Eval_Params(par);
		if((v1 = Ray_NewMeshVertex(&par->V)) == NULL)
			goto fail_create;
		if(!Ray_AddMeshVertex(mesh, v1))
			goto fail_create;
		/* Get the optional stuff. */
		if(par->more)
		{
			/* Normal. */
			par = par->next;
			v1->nx = (float)par->V.x;
			v1->ny = (float)par->V.y;
			v1->nz = (float)par->V.z;
			v1->flags |= MESH_VERTEX_HAS_NORMAL;
			if(par->more)
			{
				/* Color. */
				par = par->next;
				v1->r = (float)par->V.x;
				v1->g = (float)par->V.y;
				v1->b = (float)par->V.z;
				v1->flags |= MESH_VERTEX_HAS_COLOR;
				if(par->more)
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
			
	/*
	 * Get the segments.
	 * Create a new set of vertices as transformed copies of the
	 * previous set of vertices.
	 * Create the triangles that connect the two sets of vertices to
	 * form a segment.
	 */
	if((T = Ray_NewXform()) == NULL)
		goto fail_create;
	if((Tlocal = Ray_NewXform()) == NULL)
		goto fail_create;
  ntris = nsegs = 0;
	tris = t = NULL;
	smooth = 0;
	while(par != NULL)
	{
		switch(par->type)
		{
			case TK_SEGMENT:
				M4x4Identity(&Tlocal->M);
				M4x4Identity(&Tlocal->I);
				reps = 1; /* Repeat count. */
				break;
			case TK_SMOOTH:
				smooth = 1;
				break;
			case TK_RIGHTBRACE: /* End of segment. */
				while(reps--)
				{
					nsegs++;
					/*
					 * Create a new set of vertices as transformed copies of the
					 * original set.
					 */
					ConcatXforms(T, Tlocal);
					for(i = 0; i < nverts; i++)
					{
						if((v1 = Ray_NewMeshVertex(NULL)) == NULL)
							goto fail_create;
						*v1 = *mesh->vertices[i];
						V.x = v1->x; V.y = v1->y; V.z = v1->z;
						PointToWorld(&V, T);
						v1->x = (float)V.x; v1->y = (float)V.y; v1->z = (float)V.z;
						V.x = v1->nx; V.y = v1->ny; V.z = v1->nz;
						if(!(ISZERO(V.x) && ISZERO(V.y) && ISZERO(V.z)))
						{
							NormToWorld(&V, T);
							V3Normalize(&V);
							v1->nx = (float)V.x; v1->ny = (float)V.y; v1->nz = (float)V.z;
						}
						if(!Ray_AddMeshVertex(mesh, v1))
							goto fail_create;
					}
					/* Create connecting triangles. */
					prevverts = mesh->vertices + nverts * (nsegs - 1);
					newverts = mesh->vertices + nverts * nsegs;
					pv1 = *prevverts++;
					v1 = *newverts++;
					for(i = 1; i < nverts; i++)
					{
						pv2 = *prevverts++;
						v2 = *newverts++;
						if((t = Ray_NewMeshTri(pv1, pv2, v1)) != NULL)
						{
							t->next = tris;
							tris = t;
							ntris++;
						}
						if((t = Ray_NewMeshTri(v2, v1, pv2)) != NULL)
						{
							t->next = tris;
							tris = t;
							ntris++;
						}
						pv1 = pv2;
						v1 = v2;
					}
					/* If closed, connect the end vertices. */
				}
				break;
  		/*
			 * Since we know that there is always one vector parameter for all
			 * of the transform cases, the "TK_VECTOR" type param is
			 * actually the transform param, so we'll just just change
			 * the type to TK_VECTOR before passing it Eval_Params().
			 * This shortcut helps keep the parameter list short.
			 */
			case TK_ROTATE:
				par->type = TK_VECTOR;
        Eval_Params(par);
				XformXforms(Tlocal, &par->V, XFORM_ROTATE);
				break;
			case TK_SCALE:
				par->type = TK_VECTOR;
        Eval_Params(par);
				XformXforms(Tlocal, &par->V, XFORM_SCALE);
				break;
			case TK_SHEAR:
				par->type = TK_VECTOR;
        Eval_Params(par);
				XformXforms(Tlocal, &par->V, XFORM_SHEAR);
				break;
			case TK_TRANSLATE:
				par->type = TK_VECTOR;
        Eval_Params(par);
				XformXforms(Tlocal, &par->V, XFORM_TRANSLATE);
				break;
			case TK_REPEAT:
				par->type = TK_FLOAT;
        Eval_Params(par);
				reps = max((int)par->V.x, 0);
				break;
		}
		par = par->next;
	}

	Ray_DeleteXform(T);
	Ray_DeleteXform(Tlocal);
	
	/* If smooth, generate normals where none were explicitly defined. */

	/* Generate UV coordinates where none were explicitly defined. */
	k = 0;
	for(i = 0; i <= nsegs; i++)
	{
	  u = (double)i / (double)nsegs;
		for(j = 0; j < nverts; j++)
		{
			v = (double)j / (double)(nverts - 1);
			v1 = mesh->vertices[k++];
			v1->u = (float)u;
			v1->v = (float)v;
		}
	}

	if((obj = Ray_MakeMeshFromData(mesh, tris, ntris)) == NULL)
		goto fail_create;
	
	if(smooth)
		obj->flags |= OBJ_FLAG_SMOOTH;

	return obj;

	fail_create:

	while(tris != NULL)
	{
		t = tris;
		tris = t->next;
		Ray_DeleteMeshTri(t);
	}
	Ray_DeleteMeshData(mesh);
	Ray_DeleteXform(T);
	Ray_DeleteXform(Tlocal);

	return NULL;
}

