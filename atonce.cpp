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
#include <unistd.h>

#define MAXVAL 5.0

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

void getFlowField(const Mat& u, const Mat& v, Mat& flowField);//,float *flowmax,float *flowmin);
void opticalflow(Mat& prev, Mat& cur, BroxOpticalFlow flow,GpuMat& d_fu, GpuMat& d_fv );

int main(int argc, char** argv){

	Primeorder order = ROM;
	char originpath[100] = "/media/disk1/bgsim/Dataset/MP2Cooking/oriimages";
	char savepath[100] = "/media/disk1/bgsim/Dataset/MP2Cooking/orioptical5";

	BroxOpticalFlow flow(0.12,5.0,0.9,3,50,20);

	JointWMF jwmf;
	jwmf.printfunc();

	Mat image, filteredimage;
	
    char foldpath[100];
    char savefoldpath[100];
    char previmagepath[100];
    char curimagepath[100];
    char saveimagepath[100];
    
    Mat prev_image;
    Mat cur_image;
    Mat Finalim;
    Mat mdfu,mdfv;
    GpuMat d_fu, d_fv;
    Mat flowFieldForward;
    
    struct dirent **dptrimg;
    
    struct stat info;
	int curnum = 12640;
    int endnum = 14105;

    while(curnum < endnum+1){
        
        sprintf(foldpath,"%s/%d",originpath,curnum);
        //char nextfoldpath[100];
        //sprintf(nextfoldpath,"%s/%d",originpath,curnum+1);
    	
        sprintf(savefoldpath,"%s/%d",savepath,curnum);
        
        mkdir(savefoldpath,0700);
        //if( stat( foldpath, &info ) != 0 ){
        ////if( stat( nextfoldpath, &info ) != 0 ){
        //    printf("wait for fold %d ...\n", curnum+1);
        //    usleep(10000*1000);
        //}
        //else if( info.st_mode & S_IFDIR ){  // S_ISDIR() doesn't exist on my windows 
            sprintf(foldpath,"%s/%d",originpath,curnum);
        	printf("cur folder : %s\n",foldpath);
            curnum ++;
            struct dirent **dptrimg = NULL;
            int numimages = scandir(foldpath,&dptrimg,findfiles,versionsort);

            for(int nim = 0 ; nim < numimages-1 ; nim ++){
                
                sprintf(previmagepath,"%s/%s",foldpath,dptrimg[nim]->d_name);
                sprintf(curimagepath,"%s/%s",foldpath,dptrimg[nim+1]->d_name);
                sprintf(saveimagepath,"%s/%s",savefoldpath,dptrimg[nim]->d_name);
                

                //sprintf(previmagepath,"%s/%s",foldpath,previmagename);
                //sprintf(curimagepath,"%s/%s",foldpath,curimagename);
                //sprintf(saveimagepath,"%s/%s",savefoldpath,previmagename);                

                prev_image = imread(previmagepath, 1); 
                cur_image = imread(curimagepath, 1); 


                if ( order == RMO){
                    //Median
                    prev_image = jwmf.filter(prev_image,prev_image,10);
                    cur_image = jwmf.filter(cur_image,cur_image,10);

                    //Optical flow
                    
                    opticalflow(prev_image,cur_image,flow,d_fu,d_fv);
                    //float maxflow;
                    //float minflow;
                    
                    mdfu = Mat(d_fu);
                    mdfv = Mat(d_fv);
                    
                    // flowFiledForward: 2D 0~255
                    getFlowField(mdfu, mdfv, flowFieldForward);//,&maxflow,&minflow);

                    Finalim = flowFieldForward.clone();
                }
                else if( order == ROM){
                    //Optical flow
                    //GpuMat d_fu, d_fv;
                    opticalflow(prev_image,cur_image,flow,d_fu,d_fv);
                    //float maxflow;
                    //float minflow;
                    //Mat flowFieldForward;
                    // flowFiledForward: 2D 0~255

                    
                    mdfu = Mat(d_fu);
                    mdfv = Mat(d_fv);
                    
                    getFlowField(mdfu, mdfv, flowFieldForward);//,&maxflow,&minflow);

                    //Median
                    Finalim = jwmf.filter(flowFieldForward,flowFieldForward,10);
    
                }	
                else if( order == RMOM){
                    //Median
                    prev_image = jwmf.filter(prev_image,prev_image,10);
                    cur_image = jwmf.filter(cur_image,cur_image,10);

                    //Optical flow
                    //GpuMat d_fu, d_fv;
                    opticalflow(prev_image,cur_image,flow,d_fu,d_fv);
                    //float maxflow;
                    //float minflow;
                    //Mat flowFieldForward;
                    
                    mdfu = Mat(d_fu);
                    mdfv = Mat(d_fv);
                    
                    // flowFiledForward: 2D 0~255
                    getFlowField(mdfu, mdfv, flowFieldForward);//,&maxflow,&minflow);

                    Finalim = jwmf.filter(flowFieldForward,flowFieldForward,10);
                }
                imwrite(saveimagepath,Finalim);
			}
            for(int i = 0 ; i < numimages ; i ++){
                free(dptrimg[i]);
            }
            free(dptrimg);
            
        //}
        //else{
        //    printf( "%s is no directory\n", foldpath );
        //}
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

void getFlowField(const Mat& u, const Mat& v, Mat& flowField)//,float *flowmax,float *flowmin)
{
    //float maxDisplacement = 0.0001f;
    //float minDisplacement = -0.0001f;

    float maxvalue = MAXVAL;
    float minvalue = -MAXVAL;

    //for (int i = 0; i < u.rows; ++i)
    //{
    //    const float* ptr_u = u.ptr<float>(i);
    //    const float* ptr_v = v.ptr<float>(i);
    //
    //    for (int j = 0; j < u.cols; ++j)
    //    {
    //        //float dmax = max(fabsf(ptr_u[j]), fabsf(ptr_v[j]));
    //        //float dmin = min(fabsf(ptr_u[j]), fabsf(ptr_v[j]));
    //
    //        float dmax = max(ptr_u[j], ptr_v[j]);
    //        float dmin = min(ptr_u[j], ptr_v[j]);
    //
    //        if (dmax > maxDisplacement)
    //            maxDisplacement = dmax;
    //        if(dmin<minDisplacement)
    //            minDisplacement =dmin;
    //
    //    }
    //}

    flowField.create(u.size(), CV_8UC3);

    for (int i = 0; i < flowField.rows; ++i)
    {
        const float* ptr_u = u.ptr<float>(i);
        const float* ptr_v = v.ptr<float>(i);


        Vec3b* row = flowField.ptr<Vec3b>(i);

        for (int j = 0; j < flowField.cols; ++j)
        {
            row[j][0] = 0;
//            row[j][1] = static_cast<unsigned char> (mapValue (ptr_v[j], minDisplacement, maxDisplacement, 0.0f, 255.0f));
//            row[j][2] = static_cast<unsigned char> (mapValue ( ptr_u[j], minDisplacement, maxDisplacement, 0.0f, 255.0f));

            row[j][1] = static_cast<unsigned char> (mapValue (ptr_v[j], minvalue, maxvalue, 0.0f, 255.0f));
            row[j][2] = static_cast<unsigned char> (mapValue ( ptr_u[j], minvalue, maxvalue, 0.0f, 255.0f));

	    if( row[j][1] < 0 ) row[j][1] = 0.0f;
	    if( row[j][1] > 255.0f ) row[j][1] = 255.0f;
	    if( row[j][2] < 0 ) row[j][2] = 0.0f;
	    if( row[j][2] > 255.0f ) row[j][2] = 255.0f;

        }
    }

    //*flowmax = maxDisplacement;
    //*flowmin = minDisplacement;
}


