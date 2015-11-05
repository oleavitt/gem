/**************************************************************************
*
*  nff.c - A simple NFF scene file parser.
*
**************************************************************************/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#include "nff.h"


/* gobble up number params until start of next name... */
/* ...or EOF is reached */
#define SKIP_ARGS( ) { do { c = fgetc( fp ); }\
	while ( c != EOF && !( isalpha( c ) ||	c == '_' || c == '#' ) );\
	ungetc( c, fp ); }


static void nff_default_message( const char *msg );
static void (*nff_message)( const char *msg );
static void Message( const char *fmt, ... );

int Nff_Parse( const char *filename, RaySetupData *rsd )
{
	double		r, g, b, kd, ks, shine, kt, ior;
	FILE		*fp;
	int			nargs;
	char		buf[ 256 ];
	char		*p;
	int			in_viewport, c;
	Surface		*cur_surface = NULL;
	Object		*obj = NULL;
	Vec3		v1, v2;
	static float pts[ 768 ];

	/* set default fill color and surface properties for objects */
	r = 1.0; g = 1.0; b = 1.0;
	kd = 1.0;
	ks = 0.0; shine = 100.0;
	kt = 0.0; ior = 1.0;

	in_viewport = 0;

	if ( ( fp = fopen( filename, READ ) ) == NULL )
	{
		Message( "nff: Can not open file: %s", filename );
		return NFF_CANT_OPEN_FILE;
	}

	while ( !feof( fp ) )
	{
		fscanf( fp, "%s", buf );

		if ( *buf == '#' )  /* comment line */
		{
			while ( ( c = fgetc( fp ) ) != '\n' && c != EOF )
				; /* eat up everything to until end of line is reached */
			continue;
		}

		if ( feof( fp ) )
			break;

		/* ignore case - make name all CAPS */
		for ( p = buf; *p != '\0'; p++ )
			*p = (char) toupper( *p );
					
		/* check for viewport tokens if we are in the viewport context */
		if ( in_viewport )
		{
			if ( strcmp( buf, "FROM" ) == 0 )    /* viewport from */
			{
				if ( ( nargs = fscanf( fp, "%lf %lf %lf",
					&rsd->viewport.LookFrom.x,
					&rsd->viewport.LookFrom.y,
					&rsd->viewport.LookFrom.z ) ) != 3 )
					Message( "nff: `from' expecting 3 arguments" );
				continue;
			}
			if ( strcmp( buf, "AT" ) == 0 )    /* viewport at */
			{
				if ( ( nargs = fscanf( fp, "%lf %lf %lf",
					&rsd->viewport.LookAt.x,
					&rsd->viewport.LookAt.y,
					&rsd->viewport.LookAt.z ) ) != 3 )
					Message( "nff: `at' expecting 3 arguments" );
				continue;
			}
			if ( strcmp( buf, "UP" ) == 0 )    /* viewport up */
			{
				if ( ( nargs = fscanf( fp, "%lf %lf %lf",
					&rsd->viewport.LookUp.x,
					&rsd->viewport.LookUp.y,
					&rsd->viewport.LookUp.z ) ) != 3 )
					Message( "nff: `up' expecting 3 arguments" );
				continue;
			}
			if ( strcmp( buf, "ANGLE" ) == 0 )    /* viewport angle */
			{
				if ( ( nargs = fscanf( fp, "%lf", &rsd->viewport.ViewAngle ) ) != 1 )
					Message( "nff: `angle' expecting an argument" );
				continue;
			}
			if ( strcmp( buf, "HITHER" ) == 0 )    /* viewport hither */
			{
				if ( ( nargs = fscanf( fp, "%lf", &rsd->min_trace_dist ) ) != 1 )
					Message( "nff: `hither' expecting an argument" );
				continue;
			}
			if ( strcmp( buf, "RESOLUTION" ) == 0 )    /* viewport resolution */
			{
				if ( ( nargs = fscanf( fp, "%d %d",
					&rsd->xres, &rsd->yres ) ) < 1 )
					Message( "nff: `resolution' expecting 1 or 2 arguments" );
				else if ( nargs == 1 ) /* make a square image */
					rsd->yres = rsd->xres;
				continue;
			}
			/* if we get down here we are out the veiwport context */
			in_viewport = 0;			
		}

		if ( strcmp( buf, "V" ) == 0 )    /* viewing frustum definition */
		{
			in_viewport = 1;
			continue;
		}

		if ( strcmp( buf, "B" ) == 0 )    /* background color */
		{
			if ( ( nargs = fscanf( fp, "%lf %lf %lf",
				&rsd->background_color1.x,
				&rsd->background_color1.y,
				&rsd->background_color1.z ) ) != 3 )
				Message( "nff: `b' expecting 3 arguments" );
			continue;
		}

		if ( strcmp( buf, "L" ) == 0 )    /* positional light source */
		{
			Light *l;
			int auto_intensity = LIGHT_FLAG_AUTO_INTENSITY;

			V3Set( &v2, 1.0, 1.0, 1.0 );
			if ( ( nargs = fscanf( fp, "%lf %lf %lf", &v1.x, &v1.y, &v1.z ) ) != 3 )
			{
				Message( "nff: `l' expecting 3 or 6 arguments" );
				continue;
			}
			if ( ( nargs = fscanf( fp, "%lf %lf %lf", &v2.x, &v2.y, &v2.z ) ) == 3 )
				auto_intensity = 0;
			else if ( nargs > 0 )
			{
				Message( "nff: `l' expecting 3 or 6 arguments" );
				continue;
			}
			if ( ( l = Ray_MakePointLight( &v1, &v2, 0.0 ) ) != NULL )
			{
				l->flags |= auto_intensity;
				Ray_AddLight( &rsd->lights, l );
			}
			continue;
		}

		if ( strcmp( buf, "F" ) == 0 )    /* fill color and surface properties */
		{
			if ( ( nargs = fscanf( fp, "%lf %lf %lf %lf %lf %lf %lf %lf",
				&r, &g, &b, &kd, &ks, &shine, &kt, &ior ) ) == 8 )
			{
				Ray_DeleteSurface( cur_surface );
				if ( ( cur_surface = Ray_NewSurface( ) ) != NULL )
				{
					V3Set( &cur_surface->color, r, g, b );
					V3Set( &cur_surface->kd, kd, kd, kd );
					V3Set( &cur_surface->ks, ks, ks, ks );
					cur_surface->spec_power = shine;
					V3Set( &cur_surface->kt, kt, kt, kt );
					cur_surface->ior = ior;
				}
				continue;
			}
			Message( "nff: `f' expecting 8 arguments" );
			continue;
		}

		if ( strcmp( buf, "S" ) == 0 )    /* sphere */
		{
			double rad;
			if ( ( nargs = fscanf( fp, "%lf %lf %lf %lf",
				&v1.x, &v1.y, &v1.z, &rad ) ) == 4 )
			{
				if ( ( obj = Ray_MakeSphere( &v1, rad ) ) != NULL )
				{
					obj->surface = Ray_ShareSurface( cur_surface );
					Ray_AddObject( &rsd->objects, obj );
				}
				continue;
			}
			Message( "nff: `s' expecting 4 arguments" );
			continue;
		}

		if ( strcmp( buf, "C" ) == 0 )    /* cone or cylinder */
		{
			double	baserad, endrad;
			if ( ( nargs = fscanf( fp, "%lf %lf %lf %lf %lf %lf %lf %lf",
				&v1.x, &v1.y, &v1.z, &baserad,
				&v2.x, &v2.y, &v2.z, &endrad ) ) == 8 )
			{
				if ( ( obj = Ray_MakeCone( &v1, &v2, baserad, endrad, 0 ) ) != NULL )
				{
					obj->surface = Ray_ShareSurface( cur_surface );
					Ray_AddObject( &rsd->objects, obj );
				}
				continue;
			}
			Message( "nff: `c' expecting 8 arguments" );
			continue;
		}

		if ( strcmp( buf, "P" ) == 0 )    /* polygon */
		{
			int npts;
			
			if ( fscanf(fp, "%d", &npts ) == 1 )
			{
				if ( npts >= 3 && npts <= 256 )
				{
					int i;
					float *pt;

					pt = pts;
					for ( i = 0; i < npts; i++ )
					{
						if ( fscanf( fp, "%g %g %g", pt, pt+1, pt+2 ) == 3 )
						{
							pt += 3;
						}
						else
						{
							Message( "nff: `p' Expecting %d vertices, got only %d",
								npts, i );
							SKIP_ARGS( );
							break;
						}
					}
					if ( i == npts )
					{
						if ( ( obj = Ray_MakePolygon( pts, npts ) ) != NULL )
						{
							obj->surface = Ray_ShareSurface( cur_surface );
							Ray_AddObject( &rsd->objects, obj );
						}
					}
				}
				else
				{
					Message( "nff: `p' Vertex count must be at least 3 and no"
						" more than 256" );
					SKIP_ARGS( );
				}
			}
			else
			{
				Message( "nff: `p' expecting a vertex count and 3 or more"
					" vertices" );
				SKIP_ARGS( );
			}
			continue;
		}

		if ( strcmp( buf, "PP" ) == 0 )   /* polygonal patch */
		{
			int npts;
			
			if ( fscanf( fp, "%d", &npts ) == 1 )
			{
				if ( npts >= 3 && npts <= 256 )
				{
					int				i;
					float			*pt, *n;
					static float	norms[ 768 ];

					pt = pts;
					n = norms;
					for ( i = 0; i < npts; i++ )
					{
						if ( fscanf(fp, "%g %g %g %g %g %g", pt, pt+1, pt+2,
							n, n+1, n+2 ) == 6 )
						{
							pt += 3;
							n += 3;
						}
						else
						{
							Message("nff: `pp' Expecting %d vertex/normal pairs,"
								" got only %d",
								npts, i );
							SKIP_ARGS( );
							break;
						}
					}
					if ( i == npts )
					{
						if ( npts == 3 )
							obj = Ray_MakeTriangle( pts, norms, NULL );
						else
							obj = Ray_MakePolygon( pts, npts );
						if ( obj != NULL )
						{
							obj->surface = Ray_ShareSurface(cur_surface);
							Ray_AddObject(&rsd->objects, obj);
						}
					}
				}
				else
				{
					Message( "nff: `pp' Vertex/normal count must be at least 3 and"
						" no more than 256" );
					SKIP_ARGS( );
				}
			}
			else
			{
				Message( "nff: `p' expecting a vertex count and 3 or more"
					" vertices" );
				SKIP_ARGS( );
			}
			continue;
		}

		Message( "nff: Unknown entity: %s", buf );
		SKIP_ARGS( );
	}

	Ray_DeleteSurface( cur_surface );
	fclose( fp );

	V3Copy( &rsd->background_color2, &rsd->background_color1 );

	return NFF_OK;
}


/*************************************************************************
*
*  Message output functions.
*
*************************************************************************/

void Message( const char *fmt, ... )
{
	static char	msg[ 512 ];
	va_list		a;

	va_start( a, fmt );
	sprintf( msg, "%s: ", "nff" );
	vsprintf( msg, fmt, a );
	nff_message( msg );
	va_end( a ); 
}

static void nff_default_message( const char *msg )
{
	msg; 
}

void Nff_SetMsgFunc( void (*msgfn)( const char *msg ) )
{
	if ( msgfn != NULL )
		nff_message = msgfn;
	else
		nff_message = nff_default_message;
}