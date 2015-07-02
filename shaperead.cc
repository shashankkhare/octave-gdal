#include <octave/oct.h>
#include <octave/ov-struct.h>
#include <octave/oct-map.h>
#include <ogrsf_frmts.h>
#include <gdal_priv.h>
#include "misc.h"

#define SHP_DEBUG
/*
INPUT : shape filepath
RETURNS: int return code,
	 int number of objects,
     strut array  geometric data array of struct containing geometry,bounds
	 metadata which is an array of struct contain name, type tuples,
	 attribute data which is an array of array
*/

DEFUN_DLD (shaperead, args, nargout, "shp read help string")
{
	int nargin=args.length();
	octave_value_list ret_list ;
	int num_items=0;

#ifdef SHP_DEBUG
	octave_stdout<< "Shaperead has " << nargin << " input args and " <<
	             nargout << "output arguments" << "\r\n";
#endif
	if (nargin==0) {
		get_null_values(5,&ret_list);
		return ret_list;
	};

	std::string arg_shape_file = args(0).string_value();
	OGRDataSource *poDS;
	OGRRegisterAll();
	poDS = OGRSFDriverRegistrar::Open(arg_shape_file.c_str(), FALSE );
	if( poDS == NULL ) {
		octave_stdout << "Open failed.\n" ;
		get_null_values(5,&ret_list);
		return ret_list;
	}
	OGRLayer  *poLayer;

	OGRFeature *poFeature;
	poLayer = poDS->GetLayer(0);

#ifdef SHP_DEBUG
	octave_stdout << "Getting layer 0\n" ;
#endif
	int fcount = 0, j=0, num_points=0;
	//is it required?
	poLayer-> ResetReading();
	int num_features = poLayer->GetFeatureCount(TRUE);
#ifdef SHP_DEBUG
	octave_stdout << "Num features " <<  num_features << "\n" ;
#endif
	poLayer-> ResetReading();
	Matrix *featureMatrix;
	OGRPoint *point;
	OGRLineString *line;
	OGRPolygon *poly;
	OGRMultiPoint *mpoint;
	OGRMultiLineString *mline;
	OGRMultiPolygon *mpoly;
	OGRLinearRing *ring;
	OGRwkbGeometryType g_type;
	OGRGeometry *geom;
	dim_vector a(1);
	a(0) = num_features;
	Cell ogr_layer(a);
	//octave_map ogr_layer(dv);

	while( (poFeature = poLayer->GetNextFeature())!= NULL) {
		octave_stdout << "Getting feature" << fcount <<  "\n" ;
	    geom = poFeature->GetGeometryRef();
		g_type=geom->getGeometryType();
		
		switch(g_type) {
		case wkbPoint:
			point = (OGRPoint *) geom;
			featureMatrix = new Matrix(1,2);
			(*featureMatrix)(0,0) = point->getX();
			(*featureMatrix)(0,1) = point->getY();
			fcount ++;
			break;
		case wkbLineString:
			line = (OGRLineString *) geom;
			num_points = line->getNumPoints();
			featureMatrix = new Matrix(num_points,2);
			for (j=0; j<num_points; j++) {
				(*featureMatrix)(j,0) = line->getX(j);
				(*featureMatrix)(j,1) = line->getY(j);
			}
			fcount ++;
			break;

		case  wkbPolygon:
			octave_stdout << "Got a polygon \n" ;

			poly = (OGRPolygon *) geom;
			ring = poly->getExteriorRing();
			num_points = ring->getNumPoints();
			octave_stdout << "numpoints of polygon " << num_points << "\n" ;
			featureMatrix = new Matrix(num_points,2);
			for (j=0; j<num_points; j++) {
				(*featureMatrix)(j,0) = ring->getX(j);
				(*featureMatrix)(j,1) = ring->getY(j);
			}
			fcount++;
			break;

		case  wkbMultiPoint:
			octave_stdout << "Unsupported multipoint type\n" ;
			featureMatrix = new Matrix(0,0);

			fcount++;
			break;

		case wkbMultiLineString:
			octave_stdout << "Unsupported multiline type\n" ;
			featureMatrix = new Matrix(0,0);

			fcount++;
			break;
		case  wkbMultiPolygon:
			octave_stdout << "Unsupported multipolygon type\n"  ;
			featureMatrix = new Matrix(0,0);
			fcount ++;
			break;
		default:
			octave_stdout << "Unknown type\n"  ;
			fcount ++;
			featureMatrix = new Matrix(0,0);
			break;
		}
		octave_scalar_map ogr_feature;
		octave_scalar_map geom_struct;
		octave_stdout << "inserting at " << fcount -1 << "\n";
		ogr_feature.assign(std::string("Geometry"), *featureMatrix);
		ogr_feature.assign(std::string("GeometryType"), g_type);
		ogr_layer(fcount-1)=  ogr_feature;
		

	}
#ifdef SHP_DEBUG
	octave_stdout << "Returning from shaperead" << "\n" ;
#endif
	
	ret_list(0)=0;
	ret_list(1)=fcount;
	ret_list.append(octave_value(ogr_layer));
	ret_list.append("metadata");
	ret_list.append("name");
	poDS->DestroyDataSource(poDS);
	return ret_list;

}
