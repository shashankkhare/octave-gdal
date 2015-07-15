LIBS = -lgdal  

project : gdalshaperead.oct gdalread.oct gdalwrite.oct

gdalshaperead.oct : gdalshaperead.cc misc.cpp misc.h
	mkoctfile $(LIBS) -o gdalshaperead.oct gdalshaperead.cc misc.cpp
gdalread.oct : gdalread.cc misc.cpp misc.h
	mkoctfile $(LIBS) -o gdalread.oct gdalread.cc misc.cpp
gdalwrite.oct: gdalwrite.cc 
	mkoctfile $(LIBS) -o gdalwrite.oct gdalwrite.cc

clean:
	rm gdalwrite.o misc.o gdalshaperead.o gdalread.o gdalwrite.oct gdalread.oct gdalshaperead.oct

