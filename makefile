project : shaperead.oct geotiffread.oct geotiffwrite.oct

shaperead.oct : shaperead.cc misc.cpp misc.h
	mkoctfile -lgdal -o shaperead.oct shaperead.cc misc.cpp
geotiffread.oct : geotiffread.cc misc.cpp misc.h
	mkoctfile -lgdal -o geotiffread.oct geotiffread.cc misc.cpp
geotiffwrite.oct: geotiffwrite.cc 
	mkoctfile -lgdal -o geotiffwrite.oct geotiffwrite.cc

clean:
	rm geotiffwrite.o misc.o shaperead.o geotiffread.o

