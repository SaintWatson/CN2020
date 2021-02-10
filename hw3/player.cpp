#include <stdio.h>
#include "opencv2/opencv.hpp"
#include "hw3.h"
#include <string.h>

using namespace cv;

int main(int argc, char* argv[]){
    
    if(argc != 2)
        ERR_EXIT("Wrong argument\n");
    
    VideoCapture cap(argv[1]);
    printf("ready\n");
    int h = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
    int w = cap.get(CV_CAP_PROP_FRAME_WIDTH);
    Mat simg = Mat::zeros(h,w,CV_8UC3);
    Mat cimg = Mat::zeros(h,w,CV_8UC3);
    int size = simg.total() * simg.elemSize();
    printf("ready\n");

    const char *filename = "example.mpg";
    FILE *fp = fopen(filename, "w");
    // while(true){
        cap >> simg;
        fwrite(simg.data, 1, size, fp);
    // }

    fclose(fp);

    // while(true){
    //     cap >> simg;

    //     uchar *ptr = simg.data;

    //     memcpy(cimg.data, ptr, size);

    //     imshow("Video", cimg);

    //     char c = waitKey(33.3333);
    //     if(c == 27)
    //         break;    
    // }

    destroyAllWindows();

    return 0;
}