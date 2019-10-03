#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>


using namespace std;
using namespace cv;
int main() {
    VideoCapture capture(0);
    Mat frame;
    capture >> frame;
    while(!frame.empty()){
        imshow("usb", frame);
        capture >> frame;
        if(waitKey(10)=='q'){
            break;
        }
    }
    return 0;
}