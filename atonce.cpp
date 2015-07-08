#include "JointWMF.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/gpu/gpu.hpp>
#include <iostream>
#include <dirent.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#define ONEFOLDER true

enum Primeorder { RMO =1, ROM =2, RMOM =3}; // RGB , Median , Optical Flow

using namespace cv;
using namespace std;
using namespace cv::gpu;

int findfiles(const struct dirent *dir) // fucking . and ..
{
    const char *s =dir->d_name;
    int len = strlen(s)-4;
    if(len >=0)
    {
        return 1;
    }
    else
    return 0;
}

void getFlowField(const Mat& u, const Mat& v, Mat& flowField,float *flowmax,float *flowmin);
void opticalflow(Mat& prev, Mat& cur, BroxOpticalFlow flow,GpuMat& d_fu, GpuMat& d_fv );

int main(int argc, char** argv){


	Primeorder order = RMOM;
	char originpath[100] = "/media/disk1/bgsim/action/dataset/objjpgimages";
	char savepath[100] = "/media/disk1/bgsim/action/dataset/RMOMopticalimages";

	BroxOpticalFlow flow(0.12,5.0,0.9,3,50,20);


	JointWMF jwmf;
	jwmf.printfunc();

	Mat image, filteredimage;
//	imagefil = jwmf.filter(image,image,10);

	struct dirent **dptrsub = NULL;

	int numsubjects = scandir(originpath,&dptrsub,findfiles,versionsort);

	if (ONEFOLDER) numsubjects = 1;

        if (numsubjects >0)
        {
            for(int i=0;i <numsubjects;i++) //n = number of people;
            {
		char subjectpath[150];
                sprintf(subjectpath,"%s/%s",originpath,dptrsub[i]->d_name);

		char savesubjectpath[150];
                sprintf(savesubjectpath,"%s/%s",savepath,dptrsub[i]->d_name);
                mkdir(savesubjectpath,0700);

		struct dirent **dptract = NULL;
                int numaction = scandir(subjectpath,&dptract,findfiles,versionsort);

		if (ONEFOLDER) numaction = 1;

		for(int na = 0 ; na < numaction ; na ++){
			char actionpath[100];
	                sprintf(actionpath,"%s/%s",subjectpath,dptract[na]->d_name);

			char saveactionpath[100];
        	        sprintf(saveactionpath,"%s/%s",savesubjectpath,dptract[na]->d_name);
        	        mkdir(saveactionpath,0700);
	
			struct dirent **dptriter = NULL;
        	        int numiterate = scandir(actionpath,&dptriter,findfiles,versionsort);

			if (ONEFOLDER) numiterate = 1;

			for(int ni = 0 ; ni < numiterate ; ni ++){
				char iteratepath[100];
	                	sprintf(iteratepath,"%s/%s",actionpath,dptriter[ni]->d_name);

				printf("current folder = %s\n",iteratepath);

				char saveiterpath[100];
        	        	sprintf(saveiterpath,"%s/%s",saveactionpath,dptriter[ni]->d_name);
				mkdir(saveiterpath,0700);

				struct dirent **dptrimage = NULL;
        	        	int numimage = scandir(iteratepath,&dptrimage,findfiles,versionsort);

				for(int nim = 0 ; nim < numimage-1 ; nim ++){
					char previmagepath[100];
	                		sprintf(previmagepath,"%s/%s",iteratepath,dptrimage[nim]->d_name);

					char curimagepath[100];
	                		sprintf(curimagepath,"%s/%s",iteratepath,dptrimage[nim+1]->d_name);

					char saveimagepath[100];

        	        		sprintf(saveimagepath,"%s/%s",saveiterpath,dptrimage[nim]->d_name);

					Mat prev_image = imread(previmagepath, 1); 
					Mat cur_image = imread(curimagepath, 1); 

					//	imwrite(saveimagepath,prev_image);

					Mat Finalim;

					if ( order == RMO){
						//Median
						prev_image = jwmf.filter(prev_image,prev_image,10);
						cur_image = jwmf.filter(cur_image,cur_image,10);

						//Optical flow
						GpuMat d_fu, d_fv;
                        		        opticalflow(prev_image,cur_image,flow,d_fu,d_fv);
						float maxflow;
						float minflow;
		                                Mat flowFieldForward;
						// flowFiledForward: 2D 0~255
                		               	getFlowField(Mat(d_fu), Mat(d_fv), flowFieldForward,&maxflow,&minflow);
	
						Finalim = flowFieldForward.clone();
					}
					else if( order == ROM){
						//Optical flow
						GpuMat d_fu, d_fv;
                        	         	opticalflow(prev_image,cur_image,flow,d_fu,d_fv);
						float maxflow;
						float minflow;
		                       	        Mat flowFieldForward;
						// flowFiledForward: 2D 0~255
                		               	getFlowField(Mat(d_fu), Mat(d_fv), flowFieldForward,&maxflow,&minflow);

						//Median
						Finalim = jwmf.filter(flowFieldForward,flowFieldForward,10);
					}	
					else if( order == RMOM){
						//Median
						prev_image = jwmf.filter(prev_image,prev_image,10);
						cur_image = jwmf.filter(cur_image,cur_image,10);

						//Optical flow
						GpuMat d_fu, d_fv;
                       		          	opticalflow(prev_image,cur_image,flow,d_fu,d_fv);
						float maxflow;
						float minflow;
	                        	        Mat flowFieldForward;
						// flowFiledForward: 2D 0~255
               		                	getFlowField(Mat(d_fu), Mat(d_fv), flowFieldForward,&maxflow,&minflow);

						Finalim = jwmf.filter(flowFieldForward,flowFieldForward,10);
					}
					

		                        imwrite(saveimagepath,Finalim);
					//imwrite(saveimagepath,cur_image);
				}
			}
		}
	    }
	}

	return 0;
}

void opticalflow(Mat& prev_image, Mat& cur_image, BroxOpticalFlow flow,GpuMat& d_fu, GpuMat& d_fv )
{
	prev_image.convertTo(prev_image,CV_32F, 1.0/255.0);
        cur_image.convertTo(cur_image,CV_32F, 1.0/255.0);

	Mat prev_imageGray;
        Mat cur_imageGray;

        cvtColor(prev_image,prev_imageGray,COLOR_BGR2GRAY);
        cvtColor(cur_image,cur_imageGray,COLOR_BGR2GRAY);

	prev_imageGray.convertTo(prev_imageGray,CV_32FC1);
	cur_imageGray.convertTo(cur_imageGray,CV_32FC1);
        GpuMat frame0(prev_imageGray);
        GpuMat frame1(cur_imageGray);
        	
        flow(frame0,frame1,d_fu,d_fv);
}
template <typename T> inline T clamp (T x, T a, T b)
{
    return ((x) > (a) ? ((x) < (b) ? (x) : (b)) : (a));
}

template <typename T> inline T mapValue(T x, T a, T b, T c, T d)
{
    x = clamp(x, a, b);
    return c + (d - c) * (x - a) / (b - a);
}

void getFlowField(const Mat& u, const Mat& v, Mat& flowField,float *flowmax,float *flowmin)
{
    float maxDisplacement = 0.0001f;
    float minDisplacement = -0.0001f;

    for (int i = 0; i < u.rows; ++i)
    {
        const float* ptr_u = u.ptr<float>(i);
        const float* ptr_v = v.ptr<float>(i);

        for (int j = 0; j < u.cols; ++j)
        {
            //float dmax = max(fabsf(ptr_u[j]), fabsf(ptr_v[j]));
            //float dmin = min(fabsf(ptr_u[j]), fabsf(ptr_v[j]));

            float dmax = max(ptr_u[j], ptr_v[j]);
            float dmin = min(ptr_u[j], ptr_v[j]);

            if (dmax > maxDisplacement)
                maxDisplacement = dmax;
            if(dmin<minDisplacement)
                minDisplacement =dmin;

        }
    }

    flowField.create(u.size(), CV_8UC3);

    for (int i = 0; i < flowField.rows; ++i)
    {
        const float* ptr_u = u.ptr<float>(i);
        const float* ptr_v = v.ptr<float>(i);


        Vec3b* row = flowField.ptr<Vec3b>(i);

        for (int j = 0; j < flowField.cols; ++j)
        {
            row[j][0] = 0;
            row[j][1] = static_cast<unsigned char> (mapValue (ptr_v[j], minDisplacement, maxDisplacement, 0.0f, 255.0f));
            //row[j][1] = static_cast<unsigned char> (mapValue (-ptr_v[j], minDisplacement, maxDisplacement, 0.0f, 255.0f));
            row[j][2] = static_cast<unsigned char> (mapValue ( ptr_u[j], minDisplacement, maxDisplacement, 0.0f, 255.0f));
           //row[j][3] = 255;

            //row[j][3] = 255;
        }
    }

    *flowmax = maxDisplacement;
    *flowmin = minDisplacement;
}


