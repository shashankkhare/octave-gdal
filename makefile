project : shaperead.oct gdalread.oct gdalwrite.oct

shaperead.oct : shaperead.cc misc.cpp misc.h
	mkoctfile -lgdal -o shaperead.oct shaperead.cc misc.cpp
gdalread.oct : gdalread.cc misc.cpp misc.h
	mkoctfile -lgdal -o gdalread.oct gdalread.cc misc.cpp
gdalwrite.oct: gdalwrite.cc 
	mkoctfile -lgdal -o gdalwrite.oct gdalwrite.cc

clean:
	rm gdalwrite.o misc.o shaperead.o gdalread.o

