#pragma once
#include <opencv2/opencv.hpp>
#include <iostream>
#include "opencv2/imgproc/imgproc_c.h"
#include <opencv2/highgui/highgui_c.h>
#include <cmath>
using namespace cv;
using namespace std;

struct OUT_PUT {        //声明一个结构体类型Date 
	Mat dst;
	float dist = 0.0;
};
class GAP_WORK
{
public:
	int ERROR_CODE = 0;
	vector<Point> center_P;//中心点集合
	int max_x=0, min_x=0, min_y=0, max_y=0;
	void value2(Mat img, int max, int min);
	Mat preimg(Mat img, int max, int min, Rect roi);
	void drawlunkuo(cv::Mat& image, cv::Vec4f line, cv::Scalar color, double p);
	Mat fix_img(Mat src);
	Mat nihe(Mat src);
	OUT_PUT gap_test(int max, int min, int areamax, int areamin, Rect roi, Mat image);
};



