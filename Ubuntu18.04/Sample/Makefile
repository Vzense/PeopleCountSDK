TARGET := Sample

SOURCE := Sample.cpp
        

FLAGS := -I../Include -I../3rdparty/opencv-3.4.1/include -Wl,-rpath,./:../Lib/:../3rdparty/opencv-3.4.1/lib -Wall -Wconversion -O3 -L../Lib -lVzPeopleCountAPI -L../3rdparty/opencv-3.4.1/lib -lopencv_core -lopencv_highgui -lopencv_imgproc

$(TARGET):$(SOURCE)
	g++ -std=c++11 $(FLAGS) $(SOURCE) -g -o $(TARGET) $(FLAGS)
	
clean:
	rm -rf *.o $(TARGET)
