#include <octave/oct.h>
#include <octave/ov-struct.h>
#include <octave/oct-map.h>
#include <gdal_priv.h>
#include <cpl_conv.h>
#include <cstring>
#include "misc.h"

//#define GEOTIFF_DEBUG

void print_usage()
{
	octave_stdout << "Geotiffread expects two arguments - geotiff filename and data output format.\n";
}
/*
INPUT : string filepath
RETURNS: int return code,
		  matrix data,
		  matrix bounding box ,
		  struct metadata
*/

DEFUN_DLD (geotiffread, args, nargout, "geotiffread help string")
{
	int nargin=args.length();
	octave_value_list ret_list(3) ;
	int num_items=0;
	int XSize=0, YSize=0;
#ifdef GEOTIFF_DEBUG
	octave_stdout<< "geotiffread has " << nargin << " input args and " <<
	             nargout << "output arguments" << "\r\n";
#endif

	if (nargin < 2) {
		get_null_values(3,&ret_list);
		print_usage();
		return ret_list;
	};

	std::string arg_file = args(0).string_value();
	GDALDataset *poDS;
	GDALAllRegister();
	poDS = (GDALDataset *) GDALOpen(arg_file.c_str(), GA_ReadOnly );
	if( poDS == NULL ) {
		octave_stdout << "Open failed.\n" ;
		get_null_values(3,&ret_list);
		return ret_list;
	}
	
	double  adfGeoTransform[6];
	int  rasterX ; //= poDS->GetRasterXSize();
	int  rasterY ; // =  poDS->GetRasterYSize();
	int band_count = poDS->GetRasterCount();


	if( poDS->GetGeoTransform( adfGeoTransform ) == CE_None ) {
	} else {
		//error could not get geotransformation data
		//put origin as 0,0 and pixel size as 1,1
		adfGeoTransform[0] = 0;
		adfGeoTransform[3] = 0;
		adfGeoTransform[1] = 1;
		adfGeoTransform[5] = 1;
	}

	/************************READING RASTER DATA **********************/
	GDALRasterBand  *poBand;
	int             nBlockXSize, nBlockYSize;
	int             bGotMin, bGotMax;
	double          adfMinMax[2];
	int curr_band;
	Cell m_band(band_count);
	int raster_data_type;
	//octave_scalar_map *band_struct;

	for (curr_band=0; curr_band < band_count; curr_band++) {
        Matrix raster_data,X,Y,raster_data_tmp,X_tmp,Y_tmp;
		Matrix bbox(2,2);
		octave_scalar_map band_struct;
		poBand = poDS->GetRasterBand( curr_band+1 );
		poBand->GetBlockSize( &nBlockXSize, &nBlockYSize );
		raster_data_type=(int)poBand->GetRasterDataType();
		adfMinMax[0] = poBand->GetMinimum( &bGotMin );
		adfMinMax[1] = poBand->GetMaximum( &bGotMax );
		if( ! (bGotMin && bGotMax) )
			GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);

		/******************* READING RASTER DATA *******************************************/
		rasterX = poBand->GetXSize();
		rasterY = poBand->GetYSize();
		raster_data = Matrix(dim_vector (rasterX, rasterY ));
		X = Matrix(dim_vector (rasterX, rasterY ));
		Y = Matrix(dim_vector (rasterX, rasterY ));

		int nXBlocks = ( rasterX + nBlockXSize - 1) / nBlockXSize;
		int nYBlocks = (rasterY + nBlockYSize - 1) / nBlockYSize;

		//read the data into memory
		GByte *data_ptr = (GByte  *) raster_data.fortran_vec ();
		int *pafScanline = (int *) CPLMalloc(sizeof(double)*rasterX*rasterY);
		int read_size_x=0, read_size_y=0, nXOff, nYOff;
		CPLErr read_err = poBand->RasterIO(GF_Read, 0, 0, rasterX,
		                                   rasterY, pafScanline, rasterX, rasterY, GDT_Float64, 0, 0);

		if (read_err != CE_None) {
			ret_list(0) = -1;
			return ret_list;
		}
		memcpy(data_ptr, pafScanline, sizeof(double)*rasterX*rasterY);

		//line by line reading of raster...pretty inefficient
		/*
		for (int i=0;i<rasterY;i++) {
				poBand->RasterIO(GF_Read, 0, 0, rasterX,
				                 1, pafScanline, rasterX, 1, GDT_Int16, 0, 0);
				GByte *raster_ptr = data_ptr + i*rasterX;
				memcpy(raster_ptr, pafScanline, 2*rasterX);
		} */

		
		/************ calculate bounds for the band  *********************/		
		double minx = adfGeoTransform[0];
		double maxy = adfGeoTransform[3];
		double maxx = minx + adfGeoTransform[1]*rasterX;
		double miny = maxy + adfGeoTransform[5]*rasterY;
		bbox(0,0) = minx;
		bbox(0,1) = miny;
		bbox(1,0) = maxx;
		bbox(1,1) = maxy;
		band_struct.assign(std::string("bbox"), bbox);
				
        /*********** assign X,Y,Z, min,max to band struct **********/
        Matrix raster_data_transpose = raster_data.transpose();
        Matrix X_trans = X.transpose();
        Matrix Y_trans = Y.transpose();
        
        /************** create X,Y matrices if specified*****************/
		if (args(1).int_value() == 1) {
			//returns center x,y of cells in X,Y matrices.
			for (int j=0; j < rasterY; j++)
				for(int i=0; i<rasterX; i++) {
					X_trans.elem(j,i) = minx + i*adfGeoTransform[1]+ adfGeoTransform[1]/2;
					Y_trans.elem(j,i) = miny + j*adfGeoTransform[5] + adfGeoTransform[5]/2;
				}
		}

       band_struct.assign(std::string("Z"), raster_data_transpose);
		band_struct.assign(std::string("X"), X_trans);
		band_struct.assign(std::string("Y"), Y_trans);
		band_struct.assign(std::string("min"), adfMinMax[0]);
		band_struct.assign(std::string("max"), adfMinMax[1]);	
	    int ndv_present = 0;	
		double ndvalue = poBand->GetNoDataValue(&ndv_present);
		if (ndv_present)
			band_struct.assign(std::string("ndv"), ndvalue);

		m_band(curr_band) = band_struct;
        
		CPLFree(pafScanline);
	
	} // end of band

	/*********************** generate meta data **************/
	Matrix geom(1,6);
	geom(0,0) = adfGeoTransform[0];
	geom(0,1) = adfGeoTransform[1];
	geom(0,2) = adfGeoTransform[2];
	geom(0,3) = adfGeoTransform[3];
	geom(0,4) = adfGeoTransform[4];
	geom(0,5) = adfGeoTransform[5];


	octave_scalar_map meta_data;
	meta_data.assign(std::string("datatype"), raster_data_type);
	meta_data.assign(std::string("geotransformation"), geom);
	meta_data.assign(std::string("projection"), poDS->GetProjectionRef());
	meta_data.assign(std::string("cols"), rasterX);
	meta_data.assign(std::string("rows"), rasterY);
	meta_data.assign(std::string("bands"), band_count);
	ret_list(0)=octave_value(0);
	ret_list(1) = octave_value(m_band);
	ret_list(2) = octave_value(meta_data);

	GDALClose( (GDALDatasetH) poDS );
	return ret_list;
}
