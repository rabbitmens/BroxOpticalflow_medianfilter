#include "JointWMF.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <dirent.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
using namespace cv;
using namespace std;

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

int main(int argc, char** argv){

	JointWMF jwmf;

	jwmf.printfunc();

	char originpath[100] = "/media/disk1/bgsim/action/dataset/objjpgimages";
	char savepath[100] = "/media/disk1/bgsim/action/dataset/filteredimages";

	Mat image, filteredimage;
//	imagefil = jwmf.filter(image,image,10);

	struct dirent **dptrsub = NULL;

	int numsubjects = scandir(originpath,&dptrsub,findfiles,versionsort);

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

		for(int na = 0 ; na < numaction ; na ++){
			char actionpath[100];
	                sprintf(actionpath,"%s/%s",subjectpath,dptract[na]->d_name);

			char saveactionpath[100];
        	        sprintf(saveactionpath,"%s/%s",savesubjectpath,dptract[na]->d_name);
        	        mkdir(saveactionpath,0700);
	
			struct dirent **dptriter = NULL;
        	        int numiterate = scandir(actionpath,&dptriter,findfiles,versionsort);

			for(int ni = 0 ; ni < numiterate ; ni ++){
				char iteratepath[100];
	                	sprintf(iteratepath,"%s/%s",actionpath,dptriter[ni]->d_name);

				char saveiterpath[100];
        	        	sprintf(saveiterpath,"%s/%s",saveactionpath,dptriter[ni]->d_name);
				mkdir(saveiterpath,0700);

				struct dirent **dptrimage = NULL;
        	        	int numimage = scandir(iteratepath,&dptrimage,findfiles,versionsort);

				for(int nim = 0 ; nim < numimage ; nim ++){
					char imagepath[100];
	                		sprintf(imagepath,"%s/%s",iteratepath,dptrimage[nim]->d_name);

					char saveimagepath[100];
        	        		sprintf(saveimagepath,"%s/%s",saveiterpath,dptrimage[nim]->d_name);
					
					image = imread(imagepath, 1); 

					filteredimage = jwmf.filter(image,image,10);
					imwrite(saveimagepath,filteredimage);
				}
			}
		}
	    }
	}

	return 0;
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


