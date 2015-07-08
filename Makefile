#MYFLAGS=$(shell pkg-config --cflags --libs opencv)
MYFLAGS=$(shell pkg-config --cflags --libs opencv)

all :
	#g++ simpledemo.cpp -o demo $(MYFLAGS)
	#g++ median.cpp -o median $(MYFLAGS)
	#g++ -L/usr/local/cuda/lib64 opticalflow.cpp -o opticalflow $(MYFLAGS)
	g++ -L/usr/local/cuda/lib64 atonce.cpp -o atonce $(MYFLAGS)

clean :
	#rm -f demo median opticalflow
	rm -f atonce
