/**
 *****************************************************************************
 * @file raytrace.h
 *  The ray-tracing library API and definitions for all of the object
 *  types that compose a scene.
 *
 *****************************************************************************
 */

#ifndef RAYTRACE_H
#define RAYTRACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "expr.h"	/* Math primitive types and Expr type. */
#include "image.h"	/* Image type. */
#include "vm.h"		/* Virtual machine functions and types. */

// Forward declarations:
typedef struct tag_object				Object;
typedef struct tag_light				Light;
typedef struct tag_surface				Surface;
typedef struct tag_xform				Xform;
typedef struct tag_shader				Shader;

#define X_AXIS			0
#define Y_AXIS			1
#define Z_AXIS			2

/*************************************************************************
*
*	Projection type codes.
*
*************************************************************************/
enum
{
	VIEWPORT_PERSPECTIVE = 0,
	VIEWPORT_ANAGLYPH
};


/*************************************************************************
*
*	Viewport type
*
*************************************************************************/
typedef struct tag_viewport
{
	/* User settings. */
	Vec3 LookFrom;		/* Where we are looking from. */
	Vec3 LookAt;		/* Where we are looking at. */
	Vec3 LookUp;		/* If we look in this direction, we are looking straight up. */
	double ViewAngle;	/* Viewing angle (in degrees) for perspective FOV. */

	/* Internal data. */
	Vec3 U, V, N;		/* Viewport coordinate system axes. */
} Viewport;


/*************************************************************************
*
*	Structure containing all information needed to setup the renderer.
*
*************************************************************************/
typedef struct tag_raysetupdata
{
	/* Output image resolution. */
	int xres, yres;

	/* Animation frame counters. */
	int start_frame;
	int end_frame;
	int cur_frame;
	double normalized_frame;

	/* Background colors. */
	Vec3 background_color1;
	Vec3 background_color2;

   // Visibility falloff distance and color
   Vec3 visibility_color;
   double visibility_distance;

	/* Up orientation vector. */
	Vec3 up_vector;

	/* Minimum and maximum trace distances. */
	double min_trace_dist;
	double max_trace_dist;
	double min_shadow_dist;

	/* Maximum trace recursion depth. */
	int max_trace_depth;

	/* Minimum number of objects required for bounding tree to be built. */
	int bound_threshold;

	/* Maximum number of objects per bounding box. */
	int max_cluster_size;

	/* If true, generate fake caustics in shadows. */
	int use_fake_caustics;

	/* Global index of refraction for the world. */
	double global_ior;

	/* Viewport settings. */
	Viewport viewport;
	/* For stereograms. */
	Vec3 right_eye_lookfrom;
	int projection_mode;

	/* Main object list. */
	Object *objects;

	/* Main light list. */
	Light *lights;
} RaySetupData;


/*************************************************************************
*
*	Object list element used in CSG.
*
*************************************************************************/
typedef struct tag_olist
{
	struct tag_olist *next;	/* Next object in list. */
	Object *obj;		/* The object. */
} ObjectList;


/*************************************************************************
*
*	Light list element used in CSG.
*
*************************************************************************/
typedef struct tag_llist
{
	struct tag_llist *next;	/* Next light in list. */
	Light *lite;		/* The light. */
} LightList;


/*************************************************************************
*
*	Object transformation data struct.
*
*************************************************************************/
typedef struct tag_xform
{
	Mat4x4 M, I;		/* Normal and inverse transform matrices. */
	int nrefs;			/* Number of reference copies to this xform. */
} Xform;


/*************************************************************************
*
*	Xform action codes.
*
*************************************************************************/
enum
{
	XFORM_TRANSLATE = 0,
	XFORM_SCALE,
	XFORM_ROTATE,
	XFORM_SHEAR,
	XFORM_NUM_XFORM_TYPES
};


/*************************************************************************
*
*	Object type codes.
*
*************************************************************************/
enum
{
	OBJ_BBOX = 0,
	OBJ_BLOB,
	OBJ_BOX,
	OBJ_COLORTRIANGLE,
	OBJ_CONE,
	OBJ_CSGCLIP,
	OBJ_CSGDIFFERENCE,
	OBJ_CSGGROUP,
	OBJ_CSGINTERSECTION,
	OBJ_CSGUNION,
	OBJ_DISC,
	OBJ_HFIELD,
	OBJ_FN_XYZ,
	OBJ_MESH,
	OBJ_POLYGON,
	OBJ_SPHERE,
	OBJ_TORUS,
	OBJ_TRIANGLE,
	OBJ_NUM_OBJECT_TYPES
};


/*************************************************************************
*
*	HitData type - contains info for single ray-object intersection.
*
*************************************************************************/
typedef struct tag_hitdata
{
	struct tag_hitdata *next;	/* Next and previous HitData ptrs. */
	double t;			/* Distance from ray base. */
	int entering;		/* True if ray is entering object. */
	Object *obj;		/* Object hit. */
} HitData;


/*************************************************************************
*
*	RayInitData type - for passing initial parameters between the
*	user and the ray-trace functions.
*
*************************************************************************/
typedef struct tag_rayinitdata
{
	Vec3 B, D;			/* Base and normalized direction of ray. */
	double tmin, tmax;	/* Interval in which to search for intersections. */
	Vec3 color;			/* Color value to return for this ray. */
} RayInitData;


/*************************************************************************
*
*	ObjectProcs type - contains pointers to object-specific functions.
*
*************************************************************************/
typedef struct tag_objectprocs
{
	int type;			/* Object type ID. (see OBJ_XXX enum above) */
	int (*Intersect)(Object *obj, HitData *hits);
	void (*CalcNormal)(Object *obj, Vec3 *P, Vec3 *N);
	int (*IsInside)(Object *obj, Vec3 *P);
	void (*CalcUVMap)(Object *obj, Vec3 *P, double *u, double *v);
	void (*CalcExtents)(Object *obj, Vec3 *omin, Vec3 *omax);
	void (*Transform)(Object *obj, Vec3 *params, int type);
	void (*Copy)(Object *destobj, Object *srcobj);
	void (*Delete)(Object *obj);	/* Delete type-specific data. */
	void (*Draw)(Object *obj);		/* Draw wire frame object. */
} ObjectProcs;


/*************************************************************************
*
*	Bounding box data.
*
*************************************************************************/
typedef struct tag_bbox
{
	Vec3 bmin, bmax;
	int num_objects;
	Object *objects;
} BBoxData;


/*************************************************************************
*
*	Blob data.
*
*************************************************************************/
/*
 * Blob element.
 */
typedef struct tag_bloblet
{
	struct tag_bloblet *next;	/* Next blob elem in main list. */
	int type;			/* Spherical? Cylindrical? Plane? Hemisphere? */
	Vec3 loc, d,		/* Location points. (End vector for cyl.) */
		dir;			/* Normalized direction vector. */
	double rad, rsq,	/* Radius and radius squared. */
		len, lsq,		/* Length and length squared. */
		field,			/* Field strength constant for density eq. */
		d1, d2,			/* "d" parts of plane eqs. for end planes. */
		l_dot_d,		/* Pre-computed cyl. loc dot offset vector. */
		r2, r4;			/* Pre-computed density eq. constants. */
	double c[5];		/* Density eq. coefs. for this blob elem. */
} Bloblet;

/*
 * Blob field of influence intersection data.
 * Two for each element in blob.
 */
typedef struct tag_blobhit
{
	struct tag_blobhit *next;	/* Next interval ahead of this one. */
	double t;			/* "t" value of this interval point. */
	int entering;		/* True if this is the first "t" of an element's interval. */
	Bloblet *be;		/* Blob element bounded by this interval point. */
} BlobHit;

typedef struct tag_blob
{
	Object *bound;		/* Bounding volume, NULL if none. */
	double threshold;	/* The threshold offset. */
	int solver;			/* Which root solving method to use. */
	Bloblet *elems;		/* Link-list of all blob elements. */
	BlobHit **hits;		/* Array of ptrs to all interval data structs for blob. */
	int nrefs;			/* Number of reference copies of blob. */
} BlobData;

/* Blob element type codes. */
#define BLOB_SPHERE		0
#define BLOB_HEMISPHERE	1
#define BLOB_CYLINDER	2
#define BLOB_PLANE		3


/*************************************************************************
*
*	Box type.
*
*************************************************************************/
typedef struct tag_box
{
	float x1, y1, z1, x2, y2, z2;
} BoxData;


/*************************************************************************
*
*	CSG data.
*
*************************************************************************/
typedef struct tag_csg
{
	Object *boundobj;	/* User-supplied bounding object. */
	Object *children;	/* All objects to be intersection tested. */
	ObjectList *olist;	/* Objects in original CSG order. */
	LightList *litelist;	/* Lights that are part of this object. */
	Object *objhit;		/* Closest object hit. */
	HitData *hits;		/* List of all ray/object hits after CSG operation. */
	int nchildren;		/* Number of objects in CSG operation. */
	int nhits;			/* Number of hits in above list. */
} CSGData;


/*************************************************************************
*
*	Cone type.
*
*************************************************************************/
typedef struct tag_cone
{
	Vec3 base_pt,		/* Base point location. */
		end_pt,			/* End point location. */
		rx, ry, rz;		/* Rotation vectors. */
	float base_rad,		/* Base radius. */
		end_rad,		/* End radius. */
		height,			/* Height of cone. */
		slope,			/* Gradient of cone side. */
		ssq,			/* Slope squared. */
		brsq;			/* Base radius squared. */
	int closed;			/* If true, cone is capped at both ends. */
	int nrefs;			/* Number of reference copies of cone in use. */
} ConeData;



/*************************************************************************
*
*	Disc type.
*
*************************************************************************/
typedef struct tag_disc
{
	Vec3 loc, norm;
	double d, inrsq, outrsq;
} DiscData;


/*************************************************************************
*
*	Height field type.
*
*************************************************************************/

/* Height field parameter codes. */
#define HFIELD_IMAGE	0

typedef struct tag_hfhit	/* Data for each ray/triangle hit in HF. */
{
	struct tag_hfhit *next;	/* Next hit in list. */
	Vec3 tri_norm;		/* Normal for triangle. */
	double t;			/* "t" value for ray. */
} HFHit;

typedef struct tag_hflocaldata	/* Local data for each instance */
{								/* of a height field object. */
	HFHit *hits;		/* Ray/triangle hit list. */
	int nhits;			/* # of ray/triangle hits. */               
} HFLocalData;

typedef struct tag_hfield
{
	Vec3 bmin, bmax;	/* Overall bounds of height field. */
	Image *img;			/* Height field image-map. */
	unsigned short *qtree;	/* Quad tree of zmin and zmax values. */
	size_t qtreesize;	/* Size, in bytes, of quad tree. */
	int nrefs;			/* Reference count. */
} HFieldData;


/*************************************************************************
*
*	fn(x, y, z) function type.
*
*************************************************************************/
typedef struct tag_fnxyz
{
	VMExpr *fn;			/* The function. */
	Vec3 bmin, bmax;	/* Corners of box bounding search interval. */
	double xinc,		/* Increments in which to... */
		yinc,			/* ...search for single roots. */
		zinc;
	int nrefs;			/* Reference copy copy counter. */
} FnxyzData;


/*************************************************************************
*
*	3D mesh type.
*
*************************************************************************/
typedef struct tag_meshvert
{
	float x, y, z;		/* Location. */
	float nx, ny, nz;	/* Normal. */
	float u, v;			/* UV coordinates. */
	float r, g, b;		/* Color. */
	int flags;			/* Status flags (see below). */
} MeshVertex;

#define MESH_VERTEX_HAS_NORMAL		1
#define MESH_VERTEX_HAS_UV			2
#define MESH_VERTEX_HAS_COLOR		4

typedef struct tag_meshtri
{
	struct tag_meshtri *next;	/* Next triangle in list. */
	MeshVertex *v1, *v2, *v3;	/* Triangle vertices. */
	Vec3 pnorm;			/* Plane normal. */
	int axis;			/* Axis of greatest 2D projection. */
} MeshTri;

typedef struct tag_meshnode
{
	struct tag_meshnode *next;	/* Next node on this level. */
	struct tag_meshnode *nodes;	/* Sub nodes on tree. */
	Vec3 bmin, bmax;	/* Bounding box of triangles in node. */
	MeshTri *tris;		/* Triangle list. */
} MeshNode;

typedef struct tag_meshhit
{
	struct tag_meshhit *next;	/* Next hit in list. */
	MeshTri *tri;		/* Triangle that was hit. */
	double t;			/* "t" value of ray/triangle intersection. */
	double a, b, c;		/* Barycentric coordinates of ray/triangle hit. */
} MeshHit;

typedef struct tag_meshlocaldata
{
	MeshHit *hits;
	int nhits;
} MeshLocalData;

typedef struct tag_mesh
{
	MeshVertex **vertices;
	int nvertices;
	MeshNode *tree;
	MeshTri *tris;		/* Triangle list (before tree is built). */
	MeshTri *trilast;	/* Last triangle added to list. */
	MeshHit *hits;
	int nrefs;
} MeshData;


/*************************************************************************
*
*	Polygon type.
*
*************************************************************************/
typedef struct tag_polygon
{
	int npts;
	int axis;
	float *pts;
	float nx, ny, nz;
} PolygonData;


/*************************************************************************
*
*	Sphere type.
*
*************************************************************************/
typedef struct tag_sphere
{
	float x, y, z, r;
} SphereData;


/*************************************************************************
*
*	Torus type.
*
*************************************************************************/
typedef struct tag_torus
{
	Vec3 loc;			/* Location. */
	float r, R;			/* Minor & major radii. */
	double R2, r2;		/* Minor & major radii squared. */
	double dr;			/* Precomputed (R^2 - r^2) part of the torus equation. */
	float bound_rsq;	/* Radius of entire torus squared. */
	Vec3 scale;			/* Scaling factor. */
	int solver;			/* What root solving method to use. */
} TorusData;


/*************************************************************************
*
*	Phong shaded and colored triangle types.
*
*************************************************************************/
typedef struct tag_triangle
{
	int axis;			/* 2D projection axis, 0 = X, 1 = Y, 2 = Z. */
	float pnorm[3];		/* Normal for plane of triangle. */
	float pts[9];		/* Points of triangle in 3D space. */
	float uv[6];		/* UV coordinate pairs for each vertex. */
	float norms[9];		/* Normals of triangle in 3D space. */
} TriangleData;


typedef struct tag_colortriangle
{
	int axis;			/* 2D projection axis, 0 = X, 1 = Y, 2 = Z. */
	float pnorm[3];		/* Normal for plane of triangle. */
	float pts[9];		/* Points of triangle in 3D space. */
	float uv[6];		/* UV coordinate pairs for each vertex. */
	float norms[9];		/* Normals of triangle in 3D space. */
	float colors[9];	/* Colors for each vertex. */
	float r, g, b;		/* Interpolated color. */
} ColorTriangleData;


/*************************************************************************
*
*	Object base type - contains data common to all objects.
*
*************************************************************************/

#define OBJ_FLAG_NO_SHADOW			1
#define OBJ_FLAG_NO_SELF_INTERSECT	2
#define OBJ_FLAG_INVERSE			4
#define OBJ_FLAG_TRANSMISSIVE		8
#define OBJ_FLAG_UV					16
#define OBJ_FLAG_GROUP_ONLY			32
#define OBJ_FLAG_SMOOTH				64
#define OBJ_FLAG_TWO_SIDES			128

typedef struct tag_object
{
	Object *next;		/* Next object in list. */
	ObjectProcs *procs;	/* Object-specific functions. */
	Surface *surface;	/* Object surface attributes. */
	unsigned int flags;	/* Special action flags. */
	Xform *T;			/* Transform data. */
	union
	{
		BBoxData *bbox;
		BlobData *blob;
		BoxData *box;
		ColorTriangleData *colortri;
		ConeData *cone;
		CSGData *csg;
		DiscData *disc;
		HFieldData *hf;
		FnxyzData *fnxyz;
		MeshData *mesh;
		PolygonData *polygon;
		SphereData *sphere;
		TorusData *torus;
		TriangleData *triangle;
	} data;				/* Object-specific data. */
	union
	{
		HFLocalData *hf;
		MeshLocalData *mesh;
		void *none;
	} localdata;		/* Object-specific data local to this Object instance. */
} Object;


/*************************************************************************
*
*	Light type codes.
*
*************************************************************************/
enum
{
	LIGHT_POINT = 0,
	LIGHT_INFINITE,
	LIGHT_DIRECTIONAL,
	LIGHT_NUM_LIGHT_TYPES
};

/* Special action flags. */
#define LIGHT_FLAG_NO_SHADOW		1
#define LIGHT_FLAG_NO_SPECULAR		2
#define LIGHT_FLAG_JITTER			4
#define LIGHT_FLAG_AUTO_INTENSITY	8


/*************************************************************************
*
*	Light source data.
*
*************************************************************************/
typedef struct tag_light
{
	Light *next;		/* Next light in list. */
	int type;			/* Type of light source. */
	unsigned int flags;	/* Special action flags. */
	Vec3 loc;			/* Location of light source. */
	Vec3 at;			/* Where directional light source is pointed at. */
	Vec3 dir;			/* Direction of light source. */
	Vec3 color;			/* Color of light source. */
	Vec3 jitter;		/* Amount of random jitter to add to location. */
	double falloff;		/* Falloff factor light sources of known distance. */
	double focus;		/* Power for the angle of distribution of dir. light. */
	double angle_min, angle_max, angle_diff;	/* Spot light cone angles. */
	/* Points to an object if last shadow was completely blocked. */
	Object *block_obj_cached;
} Light;


/*************************************************************************
*
*	Object surface data. Contains shading info for object.
*
*************************************************************************/
typedef struct tag_surface
{
	Xform *T;			// Transform data.
	Vec3 color;			// Base color for surface.
	Vec3 ka;			// Ambient light level.
	Vec3 kd;			// Diffuse light level.
	Vec3 kr;			// Reflection level.
	Vec3 ks;			// Specular light level.
	Vec3 kt;			// Transmission level.
	double spec_power;	// Power coefficient for Phong shading.
	double ior;			// Index of refraction inside surface.
	double outior;		// Index of refraction outside surface.
	int transmissive;	// True if this surface is transmissive.
	int nrefs;			// Number of references to this surface.

	// Shaders that procedurally set the lighting attributes for this
	// surface at runtime.
	//
	Shader *shaders;	// Link list of shaders.
} Surface;

/*************************************************************************
*
*	Shader link list element.
*
*************************************************************************/
typedef struct tag_shader
{
	Shader		*next;		// Next Shader in list.
	// TODO shader: Argument list for parameterized declared shaders.
	VMShader	*vmshader;	// Shared instance of a shader.
} Shader;

/*************************************************************************
*
*	Raytrace library API.
*
*************************************************************************/

/* Main initialization and shutdown functions. */
extern int Ray_Initialize(void);
extern int Ray_Setup(RaySetupData *rsd);
extern void Ray_GetSetup(RaySetupData *rsd);
extern void Ray_Close(void);

/* Object functions. */
/* Simple object primitives. */
extern Object *Ray_MakeDisc(Vec3 *loc, Vec3 *norm, double or, double ir);
extern void Ray_SetDisc(DiscData *disc, Vec3 *loc, Vec3 *norm, double or, double ir);
extern Object *Ray_MakeSphere(Vec3 *loc, double rad);
extern void Ray_SetSphere(SphereData *sphere, Vec3 *loc, double rad);
extern Object *Ray_MakeBox(Vec3 *bmin, Vec3 *bmax);
extern void Ray_SetBox(BoxData *box, Vec3 *bmin, Vec3 *bmax);
extern Object *Ray_MakeCone(Vec3 *base, Vec3 *end,
	double base_rad, double end_rad, int closed);
extern void Ray_SetCone(ConeData *cone, Vec3 *base, Vec3 *end,
	double base_rad, double end_rad, int closed); 
extern Object *Ray_MakeHField(Image *img);
extern Object *Ray_MakeFnxyz(VMExpr *expr, Vec3 *bmin, Vec3 *bmax,
	Vec3 *steps);
extern Object *Ray_MakeTorus(Vec3 *loc, double rmajor, double rminor);
extern void Ray_SetTorus(TorusData *torus, Vec3 *loc, double rmajor, double rminor);
extern Object *Ray_MakeTriangle(float *points, float *normals,
	float *uvpoints);
extern Object *Ray_MakeColorTriangle(float *points, float *normals,
	float *colors);

/* Polygon objects. */
extern Object *Ray_MakePolygon(float *pts, int npts);
extern Object *Ray_MakeNPolygon(int npts);
extern int Ray_PolygonAddVertex(Object *obj, Vec3 *point);
extern int Ray_PolygonFinish(Object *obj);

/* Meta objects. */
extern Object *Ray_MakeBlob(double threshold);
extern int Ray_BlobAddSphere(Object *obj, Vec3 *pt, double radius,
	double field);
extern int Ray_BlobAddCylinder(Object *obj, Vec3 *pt1, Vec3 *pt2,
	double radius, double field);
extern int Ray_BlobAddPlane(Object *obj, Vec3 *pt, Vec3 *dir,
	double dist, double field);
extern int Ray_BlobFinish(Object *obj);

/* CSG objects. */
extern Object *Ray_MakeCSG(int type);
extern int Ray_AddCSGBound(Object *obj, Object *bounddobj);
extern int Ray_AddCSGChild(Object *obj, Object *childobj);
extern int Ray_FinishCSG(Object *obj);
extern int Ray_ObjectIsCSG(Object *obj);

/* Mesh objects. */
extern Object *Ray_MakeMeshFromData(MeshData *mesh, MeshTri *tris,
	int ntris);
extern Object *Ray_BeginMesh(void);
extern Object *Ray_FinishMesh(void);
extern MeshData *Ray_NewMeshData(void);
extern void Ray_DeleteMeshData(MeshData *mesh);
extern MeshVertex *Ray_NewMeshVertex(Vec3 *pt);
extern int Ray_AddMeshVertex(MeshData *mesh, MeshVertex *v);
extern void Ray_DeleteMeshVertex(MeshVertex *v);
extern MeshTri *Ray_NewMeshTri(MeshVertex *v1, MeshVertex *v2,
	MeshVertex *v3);
extern void Ray_AddMeshTri(MeshData *mesh, MeshTri *tri);
extern void Ray_DeleteMeshTri(MeshTri *t);

/* General object modifiers. */
extern Object *Ray_CloneObject(Object *srcobj);
extern Object *Ray_DeleteObject(Object *obj);
extern void Ray_Transform_Object(Object *obj, Vec3 *params, int type);

/* Scene building. */
extern void Ray_AddObject(Object **olist, Object *obj);
extern void Ray_AddLight(Light **llist, Light *lite);
extern void Ray_GetBounds(Object *root, Vec3 *bmin, Vec3 *bmax);
extern void Ray_BuildBounds(Object **root);
extern void Ray_SetBBox(BBoxData *bbox);
extern Object *Ray_MakeBBox(Object *obj_list);

/* Light sources. */
extern Light *Ray_MakePointLight(Vec3 *loc, Vec3 *color, double falloff);
extern Light *Ray_MakeInfiniteLight(Vec3 *dir, Vec3 *color);
extern Light *Ray_DeleteLight(Light *lite);

/* Surface functions. */
extern Surface *Ray_NewSurface(void);
extern void Ray_DeleteSurface(Surface *s);
extern Surface *Ray_ShareSurface(Surface *s);
extern Surface *Ray_CloneSurface(Surface *srcsurf);
extern void Ray_Transform_Surface(Surface *s, Vec3 *params, int type);

// Shader functions
//
// TODO shader: Argument list for parameterized declared shaders.
extern Shader * Ray_AddShader(Shader *existing_shader_list, VMShader *vmshader);
extern void Ray_DeleteShaderList(Shader *shader_list);
extern Shader *Ray_CloneShaderList(Shader *srcshaderlist);
extern VMShader *Ray_ShareVMShader(VMShader *vmshader);
extern void Ray_ReleaseVMShader(VMShader *vmshader);
extern void Ray_RunShader(Shader *shader, void *data);

/* Viewport functions. */
extern void Ray_SetupViewport(Vec3 *fromleft, Vec3 *fromright,
	Vec3 *at, Vec3 *up, double FOVdegrees, int projection);
extern int Ray_TraceRayFromViewport(double u, double v,
	Vec3 *color);
extern void Ray_GetViewportInfo(Viewport *pvp, Vec3 *fromright,
	int *projection_mode);

/* Background functions. */
extern VMStmt *Ray_SetBackgroundShader(VMStmt *stmt);

/* Mapping functions. */
extern void Ray_PlaneMap(Vec3 *P, double *u, double *v);
extern void Ray_DiscMap(Vec3 *P, double *u, double *v);
extern void Ray_CylinderMap(Vec3 *P, double *u, double *v);
extern void Ray_SphereMap(Vec3 *P, double *u, double *v);
extern void Ray_TorusMap(Vec3 *P, double *u, double *v);
extern int Ray_EnvironmentMap(Vec3 *D, double *u, double *v);

/* Transformation functions. */
extern Xform *Ray_NewXform(void);
extern Xform *Ray_CopyXform(Xform *T);
extern Xform *Ray_CloneXform(Xform *T);
extern Xform *Ray_DeleteXform(Xform *T);
extern void PointToWorld(Vec3 *Q, Xform *T);
extern void PointToObject(Vec3 *Q, Xform *T);
extern void DirToWorld(Vec3 *D, Xform *T);
extern void DirToObject(Vec3 *D, Xform *T);
extern void NormToWorld(Vec3 *N, Xform *T);
extern void NormToObject(Vec3 *N, Xform *T);
extern void BBoxToWorld(Vec3 *bmin, Vec3 *bmax, Xform *T);
extern void BBoxToObject(Vec3 *bmin, Vec3 *bmax, Xform *T);
extern void XformVector(Vec3 *V, Vec3 *params, int type);
extern void XformNormal(Vec3 *N, Vec3 *params, int type);
extern void DirToMatrix(Vec3 *dir, Vec3 *rx, Vec3 *ry, Vec3 *rz);
extern void ConcatXforms(Xform *T, Xform *Tnew);
extern void XformXforms(Xform *T, Vec3 *params, int type);

/* Wire frame drawing output functions. */
extern void Ray_DrawScene(
	void (*set_pt)(int pt_ndx, double x, double y, double z),
	void (*move_to)(int pt_ndx),
	void (*line_to)(int pt_ndx));
                   
/*************************************************************************
*
*	Raytrace global variables.
*
*************************************************************************/

/* Background colors. */
extern Vec3 ray_background_color1;
extern Vec3 ray_background_color2;
extern Shader *ray_background_shader_list;

// Visibility falloff
extern Vec3 ray_visibility_color;
extern double ray_visibility_distance;

// Global variables used by shaders to get/set context-specific values
// in the renderer environment.
// These will be initialized appropriately before shaders that refer to
// them are called.
//
extern Vec3 ray_env_color;

/* Up orientation vector. */
extern Vec3 ray_up_vector;

/* Minimum and maximum trace distances. */
extern double ray_min_trace_dist;
extern double ray_max_trace_dist;
extern double ray_min_shadow_dist;

/* Minimum color level for a ray to considered significant. */
extern double ray_min_color_weight;

/* Maximum trace recursion depth. */
extern int ray_max_trace_depth;

/* If true, generate fake caustics in shadows. */
extern int ray_use_fake_caustics;

/* Global index of refraction for the world. */
extern double ray_global_ior;

/* Main object list. */
extern Object* ray_object_list;

/* Main light list. */
extern Light *ray_light_list;

/* Memory usage counter. */
extern size_t ray_mem_used;

/* Ray counters. */
extern unsigned long ray_eye_rays;
extern unsigned long ray_eye_rays_reflected;
extern unsigned long ray_eye_rays_transmitted;
extern unsigned long ray_shadow_rays;
extern unsigned long ray_shadow_rays_transmitted;

/* Object counters. */
extern unsigned long ray_num_objects;

/* Object test/hit counters. */
extern unsigned long ray_box_tests;
extern unsigned long ray_box_hits;
extern unsigned long ray_blob_tests;
extern unsigned long ray_blob_hits;
extern unsigned long ray_colortri_tests;
extern unsigned long ray_colortri_hits;
extern unsigned long ray_cone_tests;
extern unsigned long ray_cone_hits;
extern unsigned long ray_disc_tests;
extern unsigned long ray_disc_hits;
extern unsigned long ray_hfield_tests;
extern unsigned long ray_hfield_hits;
extern unsigned long ray_fnxyz_tests;
extern unsigned long ray_fnxyz_hits;
extern unsigned long ray_mesh_tests;
extern unsigned long ray_mesh_hits;
extern unsigned long ray_polygon_tests;
extern unsigned long ray_polygon_hits;
extern unsigned long ray_sphere_tests;
extern unsigned long ray_sphere_hits;
extern unsigned long ray_torus_tests;
extern unsigned long ray_torus_hits;
extern unsigned long ray_triangle_tests;
extern unsigned long ray_triangle_hits;

/* Number of bounds created. */
extern int ray_num_bounds;
/* Minimum number of objects required for bounding tree to be built. */
extern int ray_bound_threshold;
/* Maximum number of objects per bounding box. */
extern int ray_max_cluster_size;

/*************************************************************************
*
*	Error codes.
*
*************************************************************************/
enum
{
	RAY_ERROR_NONE = 0,
	RAY_ERROR_ALLOC
};

/* Gets set to any of the above error codes. */
extern int ray_error;

#ifdef __cplusplus
}
#endif

#endif  /* RAYTRACE_H */
