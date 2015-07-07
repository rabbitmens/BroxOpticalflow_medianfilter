#include "JointWMF.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

using namespace cv;
using namespace std;
int main(int argc, char** argv){

	JointWMF jwmf;

	jwmf.printfunc();

	Mat image;
	image = imread("image1.png", 1); 

	Mat imagefil;
	imagefil = jwmf.filter(image,image,10);
	imwrite("image2.png",imagefil);

	return 0;
}
