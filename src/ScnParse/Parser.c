/*************************************************************************
 *
 *  parser.c - Parser for scene script.
 *
 ************************************************************************/

#include "scn.h"

int no_texture_flag;

static Vec3 v1, v2, v3;
static double f1, f2;
static int obj_nest_level, lite_nest_level, declare_flag;
static char decl_name[MAX_TOKEN_BUF_SIZE];

static void Exec_Texture(STATEMENT *stmt);

RaySetupData *RSD;

/*************************************************************************
 *
 *    viewport { }
 *
 ************************************************************************/
static void Exec_Viewport(STATEMENT *stmt)
{
  PARAMS *par;
  STATEMENT *s;

  for(s = stmt->block; s != NULL; s = s->next)
  {
    par = s->params;
    switch(s->token)
    {
      case TK_ANGLE:
        Eval_Params(par);
        RSD->viewport.ViewAngle = par->V.x;
        break;

      case TK_AT:
        Eval_Params(par);
        RSD->viewport.LookAt = par->V;
        break;

/*    case TK_DIR:
        Eval_Params(par);
        RSD->viewport.LookDir = par->V;
        break;       */

      case TK_FROM:
        Eval_Params(par);
        RSD->viewport.LookFrom = par->V;
        break;

      case TK_ROTATE:
      Eval_Params(par);
        XformVector(&RSD->viewport.LookFrom, &par->V, XFORM_ROTATE);
        XformVector(&RSD->viewport.LookAt, &par->V, XFORM_ROTATE);
/*        if(V3Mag(&vp->dir) > EPSILON)
          XformVector(&vp->dir, &par->V, XFORM_ROTATE); */
        break;
      case TK_SCALE:
        Eval_Params(par);
        XformVector(&RSD->viewport.LookFrom, &par->V, XFORM_SCALE);
        XformVector(&RSD->viewport.LookAt, &par->V, XFORM_SCALE);
/*        if(V3Mag(&vp->dir) > EPSILON)
          XformVector(&vp->dir, &par->V, XFORM_SCALE); */
        break;
      case TK_SHEAR:
        Eval_Params(par);
        XformVector(&RSD->viewport.LookFrom, &par->V, XFORM_SHEAR);
        XformVector(&RSD->viewport.LookAt, &par->V, XFORM_SHEAR);
/*        if(V3Mag(&vp->dir) > EPSILON)
          XformVector(&vp->dir, &par->V, XFORM_SHEAR); */
        break;
      case TK_TRANSLATE:
        Eval_Params(par);
        XformVector(&RSD->viewport.LookFrom, &par->V, XFORM_TRANSLATE);
        XformVector(&RSD->viewport.LookAt, &par->V, XFORM_TRANSLATE);
        break;

#ifndef NDEBUG
      default:
        SCN_Message(SCN_MSG_ERROR, "%s: Unknown token in Exec_Viewport()",
          __FILE__);
        break;
#endif
    }
  }
}


static STATEMENT *Parse_Viewport_Stmt(void)
{
  int token;
  STATEMENT *stmt, *s, *block_last;
  const char block[] = "viewport";

  (void)Expect(TK_LEFTBRACE, "{", block);
  stmt = New_Statement(TK_VIEWPORT);
  stmt->exec = Exec_Viewport;
  block_last = s = NULL;

  for(token = Get_Token();
     (token != TK_RIGHTBRACE) && (scn_error_cnt == 0);
     token = Get_Token())
  {
    switch(token)
    {
      case TK_ANGLE:
        s = New_Statement(token);
        s->params = Compile_Params("F");
        break;

      case TK_AT:
      case TK_FROM:
/*    case TK_DIR:  */
      case TK_ROTATE:
      case TK_SCALE:
      case TK_SHEAR:
      case TK_TRANSLATE:
        s = New_Statement(token);
        s->params = Compile_Params("V");
        break;

      default:
        Err_Unknown(token, "}", block);
        break;
    }

    if(s != NULL)
    {
      if(block_last != NULL)
        block_last->next = s;
      else
        stmt->block = s;
      block_last = s;
      s = NULL;
    }
  }

  if(scn_error_cnt)
  {
    Delete_Statement(stmt);
    stmt = NULL;
  }

  return stmt;
}


/*************************************************************************
 *
 *    global_settings { }
 *
 ************************************************************************/
static void Exec_Global_Settings(STATEMENT *stmt)
{
  PARAMS *par;
  STATEMENT *s;

  for(s = stmt->block; s != NULL; s = s->next)
  {
    par = s->params;
    switch(s->token)
    {
      case TK_BACKGROUND:
         Eval_Params(par);
         RSD->background_color1 = par->V;
         if(par->more)
          par = par->next;
         RSD->background_color2 = par->V;
        break;

      case TK_RESOLUTION:
         Eval_Params(par);
         RSD->xres = (int)par->V.x;
         if(par->more)
          par = par->next;
         RSD->yres = (int)par->V.x;
        break;

      case TK_IOR:
         Eval_Params(par);
        RSD->global_ior = par->V.x;
        break;

      case TK_MAX_TRACE_DIST:
         Eval_Params(par);
        RSD->max_trace_dist = par->V.x;
        break;

      case TK_MIN_TRACE_DIST:
         Eval_Params(par);
        RSD->min_trace_dist = par->V.x;
        break;

      case TK_MAX_TRACE_DEPTH:
         Eval_Params(par);
        RSD->max_trace_depth = (int)par->V.x;
        break;

      case TK_MIN_SHADOW_DIST:
         Eval_Params(par);
        RSD->min_shadow_dist = par->V.x;
        break;

      case TK_CAUSTICS:
         Eval_Params(par);
        RSD->use_fake_caustics = (int)par->V.x;
        break;

      case TK_UP:
         Eval_Params(par);
        RSD->up_vector = par->V;
        break;

#ifndef NDEBUG
      default:
        SCN_Message(SCN_MSG_ERROR,
          "%s: Unknown token in Exec_Global_Settings()", __FILE__);
        break;
#endif
    }
  }
}


static STATEMENT *Parse_Global_Settings_Stmt(void)
{
  int token;
  STATEMENT *stmt, *s, *block_last;
  const char block[] = "global_settings";

  (void)Expect(TK_LEFTBRACE, "{", block);
  stmt = New_Statement(TK_GLOBAL_SETTINGS);
  stmt->exec = Exec_Global_Settings;
  block_last = s = NULL;

  for(token = Get_Token();
     (token != TK_RIGHTBRACE) && (scn_error_cnt == 0);
      token = Get_Token())
  {
    switch(token)
    {
      case TK_BACKGROUND:
      case TK_RESOLUTION:
        s = New_Statement(token);
        s->params = Compile_Params("VO,V");
        break;

      case TK_CAUSTICS:
      case TK_IOR:
      case TK_MAX_TRACE_DEPTH:
      case TK_MAX_TRACE_DIST:
      case TK_MIN_SHADOW_DIST:
      case TK_MIN_TRACE_DIST:
        s = New_Statement(token);
        s->params = Compile_Params("F");
        break;

      case TK_UP:
        s = New_Statement(token);
        s->params = Compile_Params("V");
        break;

      default:
        Err_Unknown(token, "}", block);
        break;
    }

    if(s != NULL)
    {
      if(block_last != NULL)
        block_last->next = s;
      else
        stmt->block = s;
      block_last = s;
      s = NULL;
    }
  }

  if(scn_error_cnt)
  {
    Delete_Statement(stmt);
    stmt = NULL;
  }

  return stmt;
}


/*************************************************************************
 *
 *    light { }
 *
 ************************************************************************/
static void Exec_Light(STATEMENT *stmt)
{
  PARAMS *par;
  STATEMENT *s;
  Light *lite;

  lite_nest_level++;

  V3Set(&v1, 0.0, 0.0, 0.0);
  V3Set(&v2, 0.0, 0.0, 0.0);
  lite = Ray_MakePointLight(&v1, &v2, 1.0);

  for(s = stmt->block; s != NULL; s = s->next)
  {
    par = s->params;
    switch(s->token)
    {
      case TK_COLOR:
        Eval_Params(par);
        lite->color = par->V;
         break;

      case TK_FROM:
        Eval_Params(par);
        lite->loc = par->V;
        break;

      case TK_NO_SHADOW:
        lite->flags |= LIGHT_FLAG_NO_SHADOW;
        break;
      case TK_NO_SPECULAR:
        lite->flags |= LIGHT_FLAG_NO_SPECULAR;
        break;
      case TK_AMBIENT:
        lite->flags |= LIGHT_FLAG_NO_SHADOW;
        lite->flags |= LIGHT_FLAG_NO_SPECULAR;
        break;

      case TK_ROTATE:
        Eval_Params(par);
        XformVector(&lite->loc, &par->V, XFORM_ROTATE);
        break;
      case TK_SCALE:
        Eval_Params(par);
        XformVector(&lite->loc, &par->V, XFORM_SCALE);
        break;
      case TK_SHEAR:
        Eval_Params(par);
        XformVector(&lite->loc, &par->V, XFORM_SHEAR);
        break;
      case TK_TRANSLATE:
        Eval_Params(par);
        XformVector(&lite->loc, &par->V, XFORM_TRANSLATE);
        break;

#ifndef NDEBUG
      default:
        SCN_Message(SCN_MSG_ERROR, "%s: Unknown token in Exec_Light()", __FILE__);
        break;
#endif
    }
  }

  lite_nest_level--;

  if(lite_nest_level == 0)
    Ray_AddLight(&RSD->lights, lite);
}


static STATEMENT *Parse_Light_Stmt(void)
{
  int token;
  STATEMENT *stmt, *s, *block_last;
  const char block[] = "light";

  (void)Expect(TK_LEFTBRACE, "{", block);
  stmt = New_Statement(TK_LIGHT);
  stmt->exec = Exec_Light;
  block_last = s = NULL;

  for(token = Get_Token();
     (token != TK_RIGHTBRACE) && (scn_error_cnt == 0);
      token = Get_Token())
  {
    switch(token)
    {
      case TK_NO_SHADOW:
      case TK_NO_SPECULAR:
      case TK_AMBIENT:
        s = New_Statement(token);
        break;

      case TK_COLOR:
      case TK_FROM:
      case TK_ROTATE:
      case TK_SCALE:
      case TK_SHEAR:
      case TK_TRANSLATE:    
        s = New_Statement(token);
        s->params = Compile_Params("V");
        break;

      default:
        Err_Unknown(token, "}", block);
        break;
    }

    if(s != NULL)
    {
      if(block_last != NULL)
        block_last->next = s;
      else
        stmt->block = s;
      block_last = s;
      s = NULL;
    }
  }

  if(scn_error_cnt)
  {
    Delete_Statement(stmt);
    stmt = NULL;
  }

  return stmt;
}


/*************************************************************************
 *
 *    object { }
 *
 ************************************************************************/
static void Exec_Object(STATEMENT *stmt)
{
  PARAMS *par;
  STATEMENT *s;
  Object *obj;
  Object *child_objs, *cur_child;

  obj_nest_level++;

  /*
   * Create a new object of the appropriate type and
   * get object-specific parameters...
   */
  s = stmt->block;
  par = stmt->params;
  switch(stmt->token)
  {
    case TK_BLOB:
      obj = Ray_MakeBlob(par);
      break;

    case TK_BOX:
      V3Set(&v1, -1.0, -1.0, -1.0);
      V3Set(&v2, 1.0, 1.0, 1.0);
      if(par != NULL)
      {
        Eval_Params(par);
        v1 = par->V;
        if(par->more)
        {
          par = par->next;
          v2 = par->V;
        }
      }
      obj = Ray_MakeBox(&v1, &v2);
      break;

    case TK_CLOSED_CONE:
    case TK_CLOSED_CYLINDER:
    case TK_CONE:
    case TK_CYLINDER:
      V3Set(&v1, 0.0, 0.0, -1.0);
      V3Set(&v2, 0.0, 0.0, 1.0);
      f1 = 1.0;
      f2 = (stmt->token == TK_CONE || stmt->token == TK_CLOSED_CONE)
        ? 0.0 : 1.0;
      if(par != NULL)
      {
        Eval_Params(par);
        v1 = par->V;
        if(par->more)
        {
          par = par->next;
          v2 = par->V;
          if(par->more)
          {
            par = par->next;
            f1 = par->V.x;
            if(par->more)
            {
              par = par->next;
              f2 = par->V.x;
            }
          }
        }
      }
      if(stmt->token == TK_CYLINDER || stmt->token == TK_CLOSED_CYLINDER)
        f2 = f1;
      obj = Ray_MakeCone(&v1, &v2, f1, f2,
        (stmt->token == TK_CLOSED_CYLINDER || stmt->token == TK_CLOSED_CONE));
      break;

    case TK_DISC:
      V3Set(&v1, 0.0, 0.0, 0.0);  /* Default location. */
      V3Set(&v2, 0.0, 0.0, 1.0);  /* Default normal. */
      f1 = 1.0;                   /* Default outer radius. */
      f2 = 0.0;                   /* Default inner radius. */
      if(par != NULL)
      {
        Eval_Params(par);
        v1 = par->V;
        if(par->more)
        {
          par = par->next;
          v2 = par->V;
          if(par->more)
          {
            par = par->next;
            f1 = par->V.x;
            if(par->more)
            {
              par = par->next;
              f2 = par->V.x;
            }
          }
        }
      }
      obj = Ray_MakeDisc(&v1, &v2, f1, f2);
      break;

    case TK_EXTRUDE:
      obj = MakeExtrudeObject(par);
      break;

    case TK_HEIGHT_FIELD:
      obj = Ray_MakeHField(par);
      break;

    case TK_IMPLICIT:
      obj = Ray_MakeImplicit(par);
      break;

    case TK_MESH:
      obj = Ray_MakeMesh(par);
      break;

    case TK_POLYGON:
      obj = Ray_MakePolygon(par);
      break;

    case TK_SPHERE:
      f1 = 1.0;
      V3Set(&v1, 0.0, 0.0, 0.0);
      if(par != NULL)
      {
        Eval_Params(par);
        v1 = par->V;
        if(par->more)
        {
          par = par->next;
          f1 = par->V.x;
        }
      }
      obj = Ray_MakeSphere(&v1, f1);
      break;

    case TK_TORUS:
      f1 = 0.5;
			f2 = 0.5;
      V3Set(&v1, 0.0, 0.0, 0.0);
      if(par != NULL)
      {
        Eval_Params(par);
        v1 = par->V;
        if(par->more)
        {
          par = par->next;
          f1 = par->V.x;
	        if(par->more)
		      {
			      par = par->next;
				    f2 = par->V.x;
					}
        }
      }
      obj = Ray_MakeTorus(&v1, f1, f2);
      break;

    case TK_COLORED_TRIANGLE:
      obj = Ray_MakeColorTriangle(par);
      break;
    case TK_TRIANGLE:
      obj = Ray_MakeTriangle(par);
      break;

    case TK_OBJECT:
    case TK_ADD:
    case TK_UNION:
    case TK_DIFFERENCE:
    case TK_INTERSECTION:
    case TK_CLIP:
      child_objs = cur_child = NULL;
      for( ; (s != NULL && s->exec == Exec_Object); s = s->next)
      {
        Exec_Object(s);
        if(cur_child != NULL)
        {
          cur_child->next = s->data.obj;
          cur_child = cur_child->next;
        }
        else
          child_objs = cur_child = s->data.obj;
      }
      obj = (stmt->token == TK_DIFFERENCE) ?
        Ray_MakeCSGDifference(child_objs, NULL) :
        (stmt->token == TK_INTERSECTION) ?
        Ray_MakeCSGIntersection(child_objs, NULL) :
        (stmt->token == TK_CLIP) ?
        Ray_MakeCSGClip(child_objs, NULL) :
				(stmt->token == TK_OBJECT) ?
        Ray_MakeCSGGroup(child_objs, NULL) :
        Ray_MakeCSGUnion(child_objs, NULL);
      break;

    case DECL_OBJECT:
      obj = Ray_CloneObject(stmt->decl.object);
      break;

#ifndef NDEBUG
    default:
      SCN_Message(SCN_MSG_ERROR, "%s: Unknown object type in Exec_Object()",
        __FILE__);
      break;
#endif
  }

  for( ; s != NULL; s = s->next)
  {
    par = s->params;
    switch(s->token)
    {
      case DECL_TEXTURE:
      case TK_TEXTURE:
        if(!no_texture_flag)
        {
					Ray_DeleteSurface(obj->surface);
          Exec_Texture(s->block);
          obj->surface = s->block->data.surface;
          if(obj->surface->transmissive)
            obj->flags |= OBJ_FLAG_TRANSMISSIVE;
        }
        break;

      case TK_ROTATE:
        Eval_Params(par);
        Ray_Transform_Object(obj, &par->V, XFORM_ROTATE);
        break;
      case TK_SCALE:
        Eval_Params(par);
        Ray_Transform_Object(obj, &par->V, XFORM_SCALE);
        break;
      case TK_SHEAR:
        Eval_Params(par);
        Ray_Transform_Object(obj, &par->V, XFORM_SHEAR);
        break;
      case TK_TRANSLATE:
        Eval_Params(par);
        Ray_Transform_Object(obj, &par->V, XFORM_TRANSLATE);
        break;

      case TK_INVERSE:
        obj->flags |= OBJ_FLAG_INVERSE;
        break;
      case TK_NO_SHADOW:
        obj->flags |= OBJ_FLAG_NO_SHADOW;
        break;
      case TK_SMOOTH:
        obj->flags |= OBJ_FLAG_SMOOTH;
        break;

#ifndef NDEBUG
      default:
        SCN_Message(SCN_MSG_ERROR, "%s: Unknown token in Exec_Object()", __FILE__);
        break;
#endif
    }
  }

  obj_nest_level--;

  if(obj_nest_level == 0 && declare_flag == 0)
  {
    Ray_AddObject(&RSD->objects, obj);
  }

  stmt->data.obj = obj;
}

static STATEMENT *Parse_Object_Stmt(int token)
{
  STATEMENT *stmt, *s, *block_last;
  Object *decl_obj;
  const char block[] = "object";

  if((cur_token->flags & TKFLAG_OBJECT) == 0)
    return NULL;

  decl_obj = (Object *)cur_token->data;

  (void)Expect(TK_LEFTBRACE, "{", block);

  if(scn_error_cnt)
    return NULL;

  stmt = New_Statement(token);
  stmt->exec = Exec_Object;
  block_last = s = NULL;

  switch(token)
  {
    case TK_BLOB:
      stmt->params = CompileBlobParams();
      break;

    case TK_BOX:
/*    case TK_PLANE: */
      stmt->params = Compile_Params("OVO,V");
      break;

    case TK_CLOSED_CONE:
    case TK_CONE:
    case TK_DISC:
      stmt->params = Compile_Params("OVO,VO,FO,F");
      break;

    case TK_CLOSED_CYLINDER:
    case TK_CYLINDER:
      stmt->params = Compile_Params("OVO,VO,F");
      break;

		case TK_EXTRUDE:
      stmt->params = CompileExtrudeParams();
			break;

    case TK_HEIGHT_FIELD:
      stmt->params = CompileHeightFieldParams();
      break;

    case TK_OBJECT:
      if((block_last = Parse_Object_Stmt(Get_Token())) != NULL)
      {
        stmt->block = block_last;
        while((block_last->next = Parse_Object_Stmt(Get_Token())) != NULL)
          block_last = block_last->next;
      }
      else
        SCN_Message(SCN_MSG_ERROR,
          "%s: Object group requires at least one object.", block);
      Unget_Token();
      break;

    case DECL_OBJECT:
      stmt->decl.object = decl_obj;
      break;

    case TK_IMPLICIT:
      stmt->params = CompileImplicitParams();
      break;

    case TK_MESH:
      stmt->params = CompileMeshParams();
      break;

    case TK_POLYGON:
      stmt->params = Compile_Params("LV");
      if(stmt->params == NULL || stmt->params->more < 2)
        SCN_Message(SCN_MSG_ERROR, "polygon: I need at least 3 points.");
      break;

    case TK_SPHERE:
      stmt->params = Compile_Params("OVO,F");
      break;

    case TK_TORUS:
      stmt->params = Compile_Params("OVO,FO,F");
      break;

    case TK_COLORED_TRIANGLE:
      stmt->params = CompileColoredTriangleParams();
      break;
    case TK_TRIANGLE:
      stmt->params = CompileTriangleParams();
      break;

    case TK_ADD:
    case TK_UNION:
    case TK_INTERSECTION:
    case TK_DIFFERENCE:
    case TK_CLIP:
      if((block_last = Parse_Object_Stmt(Get_Token())) != NULL)
      {
        stmt->block = block_last;
        while((block_last->next = Parse_Object_Stmt(Get_Token())) != NULL)
          block_last = block_last->next;
      }
      else
        SCN_Message(SCN_MSG_ERROR,
          "%s: CSG operation requires at least one object.", block);
      Unget_Token();
      break;

    default:
      SCN_Message(SCN_MSG_ERROR,
        "Unknown object type in Parse_Object_Stmt()");
      break;
  }

  s = NULL;
  for(token = Get_Token();
     (token != TK_RIGHTBRACE) && (scn_error_cnt == 0);
      token = Get_Token())
  {
    switch(token)
    {
      case DECL_TEXTURE:
      case TK_TEXTURE:
        s = New_Statement(token);
        s->block = Parse_Texture_Stmt();
        break;

      case TK_ROTATE:
      case TK_SCALE:
      case TK_SHEAR:
      case TK_TRANSLATE:
        s = New_Statement(token);
        s->params = Compile_Params("V");
        break;

      case TK_INVERSE:
      case TK_NO_SHADOW:
      case TK_SMOOTH:
        s = New_Statement(token);
        break;

      default:
        Err_Unknown(token, "}", block);
        break;
    }

    if(s != NULL)
    {
      if(block_last != NULL)
        block_last->next = s;
      else
        stmt->block = s;
      block_last = s;
      s = NULL;
    }
  }

  if(scn_error_cnt)
  {
    Delete_Statement(stmt);
    stmt = NULL;
  }

  return stmt;
}

/*************************************************************************
 *
 *    texture { }
 *
 ************************************************************************/
static void Set_Surface_Color(PARAMS *par, Surface *surf, int token)
{
  Vec3 *v;

  v = (token == TK_COLOR) ? &surf->color :
      (token == TK_AMBIENT) ? &surf->ka :
      (token == TK_DIFFUSE) ? &surf->kd :
      (token == TK_REFLECTION) ? &surf->kr :
      (token == TK_TRANSMISSION) ? &surf->kt :
      (token == TK_SPECULAR) ? &surf->ks : NULL;
  if(v == NULL)
     SCN_Message(SCN_MSG_ERROR, "Bad token in Set_Surface_Color()");
  if(token == TK_TRANSMISSION)
    surf->transmissive = 1;

  Eval_Params(par);
  *v = par->V;
}


static void Exec_Texture(STATEMENT *stmt)
{
  PARAMS *par;
  STATEMENT *s;
  Surface *surf;

  surf = (stmt->token == TK_TEXTURE) ? Ray_NewSurface() :
    (stmt->block != NULL) ? Ray_CloneSurface(stmt->decl.surface) :
    Ray_ShareSurface(stmt->decl.surface);

  for(s = stmt->block; s != NULL; s = s->next)
   {
    par = s->params;
    switch(s->token)
    {
/*      case TK_BUMP: */
      case TK_COLOR:
      case TK_AMBIENT:
      case TK_DIFFUSE:
      case TK_REFLECTION:
      case TK_TRANSMISSION:
      case TK_SPECULAR:
        Set_Surface_Color(par, surf, s->token);
        break;

      case TK_SPECULAR_SIZE:
        Eval_Params(par);
        surf->spec_power = par->V.x;
        break;

      case TK_IOR:
        Eval_Params(par);
        surf->ior = par->V.x;
        if(par->more)
        {
          par = par->next;
          surf->outior = par->V.x;
        }
        break;

      case TK_STATEMENT:
			  {
				  STATEMENT *s2, *s1;
					for(s1 = s->data.stmt, s2 = NULL; s1 != NULL; s1 = s1->next)
					{
					  if(s2 != NULL)
						{
						  s2->next = Copy_Statement(s1);
							s2 = s2->next;
						}
						else
						{
						  surf->rtproc = s2 = Copy_Statement(s1);
						}
					}
          surf->rtproc = s2;

          surf->transmissive = 1;
				}
        break;
#ifndef NDEBUG
      default:
        SCN_Message(SCN_MSG_ERROR, "%s: Unknown token in Exec_Texture()", __FILE__);
        break;
#endif
    }
  }

  stmt->data.surface = surf;
}


STATEMENT *Parse_Texture_Stmt(void)
{
  int token;
  STATEMENT *stmt, *s, *block_last;
  STATEMENT *cur_rtstmt;
  const char block[] = "texture";

  cur_rtstmt = NULL;

  if(cur_token->token == DECL_TEXTURE)
  {
    stmt = New_Statement(DECL_TEXTURE);
    stmt->decl.surface = (Surface *)cur_token->data;
    if(Get_Token() != TK_LEFTBRACE)
    {
      Unget_Token();
      stmt->block = NULL;
      return stmt;
    }
  }
  else
  {
    (void)Expect(TK_LEFTBRACE, "{", block);
    stmt = New_Statement(TK_TEXTURE);
  }
  stmt->exec = Exec_Texture;
  block_last = s = NULL;

  for(token = Get_Token();
     (token != TK_RIGHTBRACE) && (scn_error_cnt == 0);
      token = Get_Token())
  {
    switch(token)
    {
      case TK_COLOR:
      case TK_AMBIENT:
      case TK_DIFFUSE:
      case TK_REFLECTION:
      case TK_TRANSMISSION:
      case TK_SPECULAR:
        s = New_Statement(token);
        s->params = Compile_Params("V");
        break;

      case TK_IOR:
        s = New_Statement(token);
        s->params = Compile_Params("FO,F");
        break;

      case TK_SPECULAR_SIZE:
        s = New_Statement(token);
        s->params = Compile_Params("F");
        break;

      /*
       * User defined texture proc.
       */
      case DECL_PROC:
        if(cur_rtstmt == NULL)
        {
          s = New_Statement(TK_STATEMENT);
          s->data.stmt =
            Compile_Expr_User_Proc_Stmt((Proc *)cur_token->data);
          cur_rtstmt = s->data.stmt;
        }
        else    /* Add additional texture procs to list. */
        {
          cur_rtstmt->next =
            Compile_Expr_User_Proc_Stmt((Proc *)cur_token->data);
          cur_rtstmt = cur_rtstmt->next;
        }
        break;

      default:
        Err_Unknown(token, "}", block);
        break;
    }

    if(s != NULL)
    {
      if(block_last != NULL)
        block_last->next = s;
      else
        stmt->block = s;
      block_last = s;
      s = NULL;
    }
  }

  if(scn_error_cnt)
  {
    Delete_Statement(stmt);
    stmt = NULL;
  }

  return stmt;
}


static void Exec_No_Textures(STATEMENT *stmt)
{
  no_texture_flag = 1;
}

static STATEMENT *Parse_No_Textures_Stmt(void)
{
  STATEMENT *stmt = New_Statement(TK_NO_TEXTURES);
  stmt->exec = Exec_No_Textures;
  return stmt;
}


static void Exec_Textures(STATEMENT *stmt)
{
  no_texture_flag = 0;
}

static STATEMENT *Parse_Textures_Stmt(void)
{
  STATEMENT *stmt = New_Statement(TK_TEXTURES);
  stmt->exec = Exec_Textures;
  return stmt;
}


static void Exec_SetSurfaceAttr(STATEMENT *stmt)
{
  Eval_Params(stmt->params);
  switch(stmt->token)
  {
    case TK_COLOR: rt_surface->color = stmt->params->V; break;
    case TK_AMBIENT: rt_surface->ka = stmt->params->V; break;
    case TK_DIFFUSE: rt_surface->kd = stmt->params->V; break;
    case TK_REFLECTION: rt_surface->kr = stmt->params->V; break;
    case TK_SPECULAR: rt_surface->ks = stmt->params->V; break;
    case TK_TRANSMISSION: rt_surface->kt = stmt->params->V; break;
    case TK_IOR: rt_surface->ior = stmt->params->V.x; break;
    case TK_SPECULAR_SIZE: rt_surface->spec_power = stmt->params->V.x; break;
  }
}


STATEMENT *Parse_Block(const char *block_name)
{
  int token, one_liner;
  STATEMENT *stmt, *s, *block_last;

  block_last = stmt = NULL;

  if((token = Get_Token()) == TK_LEFTBRACE)
   {
    token = Get_Token();
    one_liner = 0;
  }
  else
    one_liner = 1;

  while((token != TK_RIGHTBRACE) && (scn_error_cnt == 0))
  {
    if((s = Parse_Object_Stmt(token)) == NULL)
     {
      if((s = Parse_Proc_Stmt(token)) == NULL)
      {
        switch(token)
        {
           case TK_CAMERA:
           case TK_VIEWPORT:
             s = Parse_Viewport_Stmt();
             break;

           case TK_GLOBAL_SETTINGS:
            s = Parse_Global_Settings_Stmt();
             break;

           case TK_LIGHT:
            s = Parse_Light_Stmt();
             break;

          case TK_NO_TEXTURES:
            s = Parse_No_Textures_Stmt();
            break;

          case TK_TEXTURES:
            s = Parse_Textures_Stmt();
            break;

          case DECL_LVALUE:   /* Assignment. */
            s = Parse_Expr_Stmt();
             break;

          case TK_FLOAT:     /* Local variable declarations. */
          case TK_VECTOR:
            s = Parse_Local_Decl(cur_proc, token, block_name);
            break;

          case TK_COLOR:
          case TK_AMBIENT:
          case TK_DIFFUSE:
          case TK_REFLECTION:
          case TK_TRANSMISSION:
          case TK_SPECULAR:
            s = New_Statement(token);
            s->params = Compile_Params("V");
            s->exec = Exec_SetSurfaceAttr;
            break;

          case TK_IOR:
          case TK_SPECULAR_SIZE:
            s = New_Statement(token);
            s->params = Compile_Params("F");
            s->exec = Exec_SetSurfaceAttr;
            break;

          default:
            Err_Unknown(token, (one_liner) ? NULL : "}", block_name);
            break;
        }
      }
    }

    if(s != NULL)
    {
      if(block_last != NULL)
        block_last->next = s;
      else
        stmt = s;
      block_last = s;
      while(block_last->next != NULL)
        block_last = block_last->next;
    }

    if(one_liner)
      break;

    token = Get_Token();
  }

  return stmt;
}


void SCN_Parse(const char *fname, RaySetupData *rsd)
{
  int token, type;
  STATEMENT *s;

  RSD = rsd;
  obj_nest_level = lite_nest_level = proc_nest_level =
    declare_flag = 0;

  Init_Tokens(fname);
  if(scn_error_cnt)
    return;

  token = Get_Token();
  while((token != EOF) && (scn_error_cnt == 0))
  {
    /*
     * Check for object declarations...
     */
    if(cur_token->flags & TKFLAG_OBJECT)
     {
      TOKEN tmp_token = *cur_token;
      if(Get_Token() == TK_STRING)
       {
        strcpy(decl_name, token_buffer);
        declare_flag = 1;
        *cur_token = tmp_token;
        if((s = Parse_Object_Stmt(token)) != NULL)
        {
          STATEMENT *stmp;
          Execute_Program(s);
          Add_Symbol(decl_name, s->data.obj, DECL_OBJECT);
          while(s != NULL)
          {
            stmp = s;
            s = s->next;
            Delete_Statement(stmp);
          }
        }
        declare_flag = 0;
        token = Get_Token();
        continue;
      }
      else
        Unget_Token();
      *cur_token = tmp_token;
    }

    if((s = Parse_Object_Stmt(token)) == NULL)
     {
      if((s = Parse_Proc_Stmt(token)) == NULL)
       {
        switch(token)
         {
          case TK_CAMERA:
          case TK_VIEWPORT:
            s = Parse_Viewport_Stmt();
            break;

          case TK_LIGHT:
            s = Parse_Light_Stmt();
            break;

          case TK_GLOBAL_SETTINGS:
            s = Parse_Global_Settings_Stmt();
            break;

          case TK_NO_TEXTURES:
            no_texture_flag = 1;
            break;

          case TK_TEXTURES:
            no_texture_flag = 0;
            break;

          case DECL_LVALUE:   /* Assignment. */
            Unget_Token();
            (void)Parse_Expr(&v1);
            break;

          case TK_FLOAT:      /* Declarations. */
          case TK_VECTOR:
          case TK_TEXTURE:
            type = token;
            if((token = Get_Token()) == TK_STRING)
              Parse_Decl(type);
            else
              Err_Unknown(token, "identifier", NULL);
            break;

          case TK_PROC:
            Parse_Proc_Decl(token);
            break;

    			case TK_IMAGE_MAP_FILE_PATHS:
			    	do
		  			{
    					Expect(TK_QUOTESTRING, "\"quoted name\"", NULL);
				    	SCN_AddPath(&scn_bitmap_paths, token_buffer);
		    			token = Get_Token();
    				}
				    while(token == OP_COMMA);
		    		Unget_Token();
    				break;

    			case TK_INCLUDE_FILE_PATHS:
			    	do
		  			{
    					Expect(TK_QUOTESTRING, "\"quoted name\"", NULL);
				    	SCN_AddPath(&scn_include_paths, token_buffer);
		    			token = Get_Token();
    				}
				    while(token == OP_COMMA);
		    		Unget_Token();
    				break;

          default:
            Err_Unknown(token, NULL, NULL);
            break;
         }
      }
    }

    if(s != NULL)
    {
      STATEMENT *stmp;
      Execute_Program(s);
      while(s != NULL)
       {
        stmp = s;
        s = s->next;
        Delete_Statement(stmp);
       }
    }

    token = Get_Token();
  }

  Close_Tokens();
}

