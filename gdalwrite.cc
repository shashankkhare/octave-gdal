/*
The MIT License (MIT)

Copyright (C) 2015 Shashank Khare

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <octave/oct.h>
#include <octave/ov-struct.h>
#include <octave/oct-map.h>
#include <gdal_priv.h>
#include <cpl_conv.h>
#include <ogr_spatialref.h>
#include "misc.h"

//#define GDALWRITE_DEBUG
/*
INPUT : 
		 bands raster_data,
		 string filepath,
		 struct metadata
RETURNS: int return_code - 0 signifies success,

*/

DEFUN_DLD (gdalwrite, args, nargout, "gdalwrite <band data> <filepath> <metadata>")
{
	int nargin=args.length();
	octave_value_list ret_list(1) ;
	int num_items=0;
	int XSize=0, YSize=0;
    bool option_set_statistics = false;
 
#ifdef GDALWRITE_DEBUG
	octave_stdout<< "gdalwrite has " << nargin << " input args and " <<
	nargout << "output arguments" << "\r\n";
#endif

	if (nargin<3) {
  		ret_list(0)=-1;
		return ret_list; 
	};
	/************** read file path **********/
	std::string arg_file = args(1).string_value();

	/************** read metadata *************/
	octave_scalar_map metadata = args(2).scalar_map_value ();
	Matrix geo_trans_data = metadata.contents(std::string("geotransformation")).matrix_value(); 
	int raster_data_type = metadata.contents(std::string("datatype")).int_value();
	std::string proj_wkt = metadata.contents(std::string("projection")).string_value();
        int rows = metadata.contents(std::string("rows")).int_value();
	int cols = metadata.contents(std::string("cols")).int_value();	
	int num_bands = metadata.contents(std::string("bands")).int_value();
    if (nargin > 3) // if option is specified 
    {
        octave_scalar_map options = args(3).scalar_map_value ();
        option_set_statistics = options.contents(std::string("statistics")).bool_value();
    }
	/************ write raster data ***************/
	GDALRasterBand *poBand;	
	GDALDataset *poDS;
	char **papszOptions = NULL;
	const char *pszFormat = "GTiff";
    GDALDriver *poDriver;
    char **papszMetadata;

    poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

    if( poDriver == NULL ) {
		octave_stdout << "geotiff driver failed to open.\n" ;
		ret_list(0) = -1;
		return ret_list;
    }

    papszMetadata = poDriver->GetMetadata();

	
	poDS = poDriver->Create( arg_file.c_str(), cols, rows, num_bands, (GDALDataType) raster_data_type,
papszOptions );


	if( poDS == NULL ) {
		octave_stdout << "create file failed.\n" ;
        ret_list(0) =-1;
		return ret_list;
	}

	/**************** write geo transformation metadata **************/	
	double adfGeoTransform[6] = { geo_trans_data(0,0), geo_trans_data(0,1), geo_trans_data(0,2), geo_trans_data(0,3), geo_trans_data(0,4), geo_trans_data(0,5) };
	poDS->SetGeoTransform( adfGeoTransform );
    OGRSpatialReference oSRS;
    poDS->SetProjection( proj_wkt.c_str());
     Cell band_cell = args(0).cell_value();
    octave_stdout << "Writing band data \n";
    /************* write band data ********************/
    for (int i=0;i<num_bands;i++) {

        /************ load matrix ****************/	
	Matrix raster_data;
	dim_vector raster_dims = raster_data.dims();
	//read the data into memory
    poBand = poDS->GetRasterBand(i+1);
    GByte *c_data_ptr; 
    int16NDArray tmp_16_arr,tmp_16_transpose;
    uint8NDArray tmp_u8_arr,tmp_u8_transpose;
	Matrix tmp_f64_arr,tmp_f64_transpose;

	octave_scalar_map st_tmp;
	st_tmp = band_cell(i).scalar_map_value();
	octave_value  input_array=st_tmp.contents(std::string("Z")) ;
    	switch ((GDALDataType)raster_data_type) {
		case  GDT_Int16 :
			if (input_array.is_int16_type())   
				tmp_16_arr = input_array.int16_array_value(); 
 			else if (input_array.is_matrix_type())
				tmp_16_arr = input_array.matrix_value();
			else if (input_array.is_uint8_type())
				tmp_16_arr = input_array.uint8_array_value();
			else { octave_stdout << "unsupported data type\n" ;}
                tmp_16_transpose = tmp_16_arr.transpose();
                c_data_ptr = (GByte *)tmp_16_transpose.fortran_vec();
			break;
                
        case GDT_Byte:
			if (input_array.is_int16_type())   
				tmp_u8_arr = input_array.int16_array_value(); 
 			else if (input_array.is_matrix_type())
				tmp_u8_arr = input_array.matrix_value();
			else if (input_array.is_uint8_type())
				tmp_u8_arr = input_array.uint8_array_value();
			else  octave_stdout << "unsupported data type\n" ;
                tmp_u8_transpose = tmp_u8_arr.transpose();                
                c_data_ptr = (GByte *)tmp_u8_transpose.fortran_vec();
			break;
			
		case GDT_Float64:
			if (input_array.is_int16_type())   
				tmp_f64_arr = input_array.int16_array_value(); 
 			else if (input_array.is_matrix_type())
				tmp_f64_arr = input_array.matrix_value();
			else if (input_array.is_uint8_type())
				tmp_f64_arr = input_array.uint8_array_value();
			else  octave_stdout << "unsupported data type\n" ;
            tmp_f64_transpose = tmp_f64_arr.transpose();
			c_data_ptr = (GByte *)tmp_f64_transpose.fortran_vec();
			break;

        default:
			octave_stdout << "Unknown type\n";
			break;
    	 }

    octave_value ndv_oct= st_tmp.contents(std::string("ndv")); 
    if (ndv_oct.is_defined()) { 
        octave_stdout << "ndv defined";
        int ndv = ndv_oct.int_value();		 	
    	poBand -> SetNoDataValue(ndv);
	};    
    double pdfMin, pdfMax, pdfMean, pdfStdDev;    
     
    poBand->RasterIO( GF_Write, 0, 0, cols, rows, 
                      c_data_ptr, cols, rows, (GDALDataType)raster_data_type, 0, 0 );    
    if (option_set_statistics == true) {
        octave_stdout << "setting statistics";    
        poBand->GetStatistics (0,1,&pdfMin, &pdfMax, &pdfMean, &pdfStdDev);
     };    
}//end for looop for bands	

	ret_list(0)=0;
	GDALClose( (GDALDatasetH) poDS );
	return ret_list;
}
