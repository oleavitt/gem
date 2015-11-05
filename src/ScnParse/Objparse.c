/*************************************************************************
*
*  objparse.c - Object-specific parse routines.
*
*************************************************************************/

#include "scn.h"


/*************************************************************************
*
*  PARAMS *CompileBlobParams(void)
*
*  Compile a PARAMS list for a blob object from input.
*
*************************************************************************/
PARAMS *CompileBlobParams(void)
{
  PARAMS *plist, *p;
  int token;

  /* Get threshold... */
	if((plist = Compile_Params("F")) == NULL)
		return NULL;
  p = plist;

  /* Get the elements... */
  while(1)
  {
    token = Get_Token();
    switch(token)
    {
      case TK_SPHERE:
      case TK_CYLINDER:
      case TK_PLANE:
        p->next = New_Param();
        p = p->next;
        p->type = (token == TK_SPHERE) ? BLOB_SPHERE :
                  (token == TK_CYLINDER) ? BLOB_CYLINDER :
                  BLOB_PLANE;
        if(token == TK_SPHERE)
          p->next = Compile_Params("V,FO,F");
        else
          p->next = Compile_Params("V,V,FO,F");
        while(p->next != NULL)
          p = p->next;
        continue;    /* re-cycle */
      default:
        break;
    }
    Unget_Token();
    break;  /* from loop */
  }
  return plist;
}


/*************************************************************************
*
*  PARAMS *CompileHeightFieldParams(void)
*
*  Compile a PARAMS list for an implicit function object from input.
*
*************************************************************************/
PARAMS *CompileHeightFieldParams(void)
{
	PARAMS *plist;
  FILE *fp;
  Image *img;

  if(Get_Token() != TK_QUOTESTRING)
  {
    SCN_Message(SCN_MSG_ERROR,
      "height field: Image file name expected. Found `%s'.",
      token_buffer);
    return NULL;
  }
  if((fp = SCN_FindFile(token_buffer, "rb", scn_bitmap_paths,
    SCN_FINDFILE_CHK_CUR_FIRST)) == NULL)
  {
    SCN_Message(SCN_MSG_ERROR,
      "height field: Unable to find image file: `%s'.",
      token_buffer);
    return NULL;
  }
  if((img = Image_HeightFieldLoad(fp, token_buffer)) == NULL)
  {
    fclose(fp);
    SCN_Message(SCN_MSG_ERROR,
      "height field: Error loading image file: `%s'.",
      token_buffer);
    return NULL;
  }
  fclose(fp);

  if((plist = New_Param()) == NULL)
    return NULL;

  plist->data.data = img;
  plist->type = HFIELD_IMAGE;

	return plist;
}


/*************************************************************************
*
*  PARAMS *CompileImplicitParams(void)
*
*  Compile a PARAMS list for an implicit function object from input.
*
*************************************************************************/
PARAMS *CompileImplicitParams(void)
{
	PARAMS *plist, *p;
	int token;

	/* Get the function's expression (required)... */
	if((plist = Compile_Params("R")) == NULL)
		return NULL;
	p = plist;

	/* Get optional stuff... */
	while(1)
	{
		while(p->next != NULL)
			p = p->next;
		token = Get_Token();
		switch(token)
		{
			case TK_BOUND:
				p->next = New_Param();
				p = p->next;
				p->type = IMPLICIT_BOUNDS;
				p->next = Compile_Params("V,V");
				continue;
			case TK_RESOLUTION:
				p->next = New_Param();
				p = p->next;
				p->type = IMPLICIT_RESOLUTION;
				p->next = Compile_Params("FO,FO,F");
				continue;
			default:
				Unget_Token();
				break;
		}
		break;
	}
	return plist;
}


/*************************************************************************
*
*  PARAMS *CompileMeshParams(void)
*
*  Compile a PARAMS list for a mesh object from input.
*
*************************************************************************/
PARAMS *CompileMeshParams(void)
{
  PARAMS *plist, *p, *tricnt_par;
  int cnt;

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
  if(cnt < 3)
  {
    SCN_Message(SCN_MSG_ERROR, "mesh: I need at least 3 vertices.");
    return plist;
  }
	/* This parameter will hold the triangle count. */
	p->next = tricnt_par = New_Param();  
	p = p->next;
	cnt = 0;
  while(Get_Token() == TK_TRIANGLE)
  {
    /* Get an index for each vertex of the triangle. */
    p->next = Compile_Params("F,F,F");
    while(p->next != NULL)
      p = p->next;
		cnt++;
  }
  Unget_Token();
	/* Put the triangle count at the start of the triangle list. */
	tricnt_par->V.x = (double)cnt;
  return plist;
}


/*************************************************************************
*
*  PARAMS *CompileTriangleParams(void)
*
*  Compile a PARAMS list for a triangle object from input.
*
*************************************************************************/
PARAMS *CompileTriangleParams(void)
{
  PARAMS *plist;

  /* Get the vertex list... */
  plist = Compile_Params("V,V,V,V,V,VO,F,F,F,F,F,F");
  return plist;
}


/*************************************************************************
*
*  PARAMS *CompileColoredTriangleParams(void)
*
*  Compile a PARAMS list for a triangle object from input.
*
*************************************************************************/
PARAMS *CompileColoredTriangleParams(void)
{
  PARAMS *plist;

  /* Get the vertex list... */
  plist = Compile_Params("V,V,V,V,V,VO,V,V,V");
  return plist;
}

