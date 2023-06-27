#include "gap_work.h"

/*
二值化：传入原图，阈值范围
无返回值
*/
void GAP_WORK::value2(Mat img, int max, int min)
{
	Mat temp;
	img.copyTo(temp);
	uchar* pp;
	uchar* ss;

	Size size = img.size();
	for (int i = 0; i < size.height; i++)
	{
		pp = temp.ptr<uchar>(i);
		ss = img.ptr<uchar>(i);
		for (int k = 0; k < size.width; k++)
		{
			if (pp[k] > min && pp[k] < max) ss[k] = 255;
			else ss[k] = 0;
		}
	}
}
/*
预处理：传入图片img，二值阈值范围, roi
返回二值化后的mat
*/
Mat GAP_WORK::preimg(Mat img, int max, int min, Rect roi)
{
	if (img.channels() == 3) {
		cvtColor(img, img, CV_BGR2GRAY);
	}
	Mat temp0;
	img.copyTo(temp0);
	Mat temp = temp0(roi);
	value2(temp, max, min);
	Mat temp2 ;
	temp.copyTo(temp2);
	copyMakeBorder(temp2, temp2, 1, 1, 1, 1, BORDER_CONSTANT, Scalar(0));  //find 轮廓bug
	return temp2;
}
/*
刻画拟合线：传入模板image，直线line，颜色color，偏移量p
无返回值
*/
void GAP_WORK::drawlunkuo(cv::Mat& image, cv::Vec4f line, cv::Scalar color, double p)
{
	double cos_theta = line[0];
	double sin_theta = line[1];
	double x0 = line[2], y0 = line[3] + p;
	double k = sin_theta / cos_theta;
	//cout << fabs(k) << endl;
	double phi = atan2(sin_theta, cos_theta) + 3.14 / 2.0;
	double rho = y0 * cos_theta - x0 * sin_theta;
	if (phi < 3.14 / 4. || phi > 3. * 3.14 / 4.)// ~vertical line
	{
		cv::Point pt1(rho / cos(phi), 0);
		cv::Point pt2((rho - image.rows * sin(phi)) / cos(phi), image.rows);
		cv::line(image, pt1, pt2, cv::Scalar(255), 1);
	}
	else
	{
		cv::Point pt1(0, rho / sin(phi));
		cv::Point pt2(image.cols, (rho - image.cols * cos(phi)) / sin(phi));
		cv::line(image, pt1, pt2, color, 1);
	}
}
/*
补齐间隙：传入拟合后src
返回补齐后间隙
*/
Mat GAP_WORK::fix_img(Mat src)
{
	
	uchar* dataPtr;
	uchar* dstPtr;
	Mat dst(src.size(), CV_8UC1, cv::Scalar(0));
	Size size = src.size();
	
	for (int i = 0; i < size.width; i++)
	{
		
		int begin = 0;
		int end = 0;
		for (int j = 0; j < size.height; j++)
		{
			dataPtr = src.ptr<uchar>(j);
			int p = dataPtr[i];
			if (p > 0) {
				begin = j;
				break;
			}
		}
		for (int k = size.height - 1; k >= 0; k--)
		{
			dataPtr = src.ptr<uchar>(k);
			int p = dataPtr[i];
			if (p > 0) {
				end = k;
				break;
			}
		}
		center_P.push_back(Point(i, (begin+end)/2));
		for (int n = begin; n <= end; n++)
		{
			dstPtr = dst.ptr<uchar>(n);
			dstPtr[i] = 255;

		}
	}
	return dst;
}
/*
间隙轮廓拟合：传入基础轮廓图src
返回拟合后间隙轮廓
*/
Mat GAP_WORK::nihe(Mat src)
{
	uchar* dataPtr;
	Mat dst(src.size(), CV_8UC1, cv::Scalar(0));
	Size size = src.size();
	vector<Point> up; //上轮廓点集合
	vector<Point> down;//下轮廓点集合

	// 获取上下轮廓点集合
	for (int i = 0; i < size.width; i+=2)
	{

		for (int j = 0; j < size.height; j++)
		{
			dataPtr = src.ptr<uchar>(j);
			int p = dataPtr[i];
			if (p == 255) {
				up.push_back(Point(i, j));
				break;
			}
		}
		for (int k = size.height - 2; k >= 0; k--)
		{
			dataPtr = src.ptr<uchar>(k);
			int p = dataPtr[i];
			if (p == 255) {

				down.push_back(Point(i, k));
				break;
			}
		}
	}

	cv::Vec4f line_up;
	cv::Vec4f line_down;

	cv::fitLine(up, line_up, CV_DIST_HUBER, 0, 0.01, 0.01);
	cv::fitLine(down, line_down, CV_DIST_HUBER, 0, 0.01, 0.01);

	//画出拟合后上下轮廓线
	drawlunkuo(dst, line_up, cv::Scalar(255), 0.0);
	drawlunkuo(dst, line_down, cv::Scalar(255), 1.0);
	return dst;
}
/*
间隙宽度测试，传入二值阈值范围max-min，轮廓范围areamax-areamin，区域roi，图片名
256 >=max>min>= -1           areamax>areamin>0
roi 示例Rect rect(0, 600, img.cols, 400); （x，y，width，height）均为int 类型且大于等于0
----------------------------------------------建议width >= 1000
返回结构体，并赋值平均宽度
*/
OUT_PUT GAP_WORK::gap_test(int max, int min, int areamax, int areamin, Rect roi, Mat image)
{
	OUT_PUT output;

	if (image.data == nullptr)//nullptr是c++11新出现的空指针常量
	{
		ERROR_CODE = 1;
		cerr << "图片文件不存在" << endl;
		Mat Error(Size(800, 800), CV_8UC1, cv::Scalar(0));
		output.dst = Error;
		return output; //返回空白背景图
	}
	if (image.cols < 500 || image.rows < 10) {
		ERROR_CODE = 2;
		cerr << "图片无法检测" << endl;
		output.dst = image;
		return output; //返回原图
	}

	if ((roi.x + roi.width > image.cols) || (roi.y + roi.height > image.rows)
		|| roi.width < 500 || roi.height < 10 || roi.x < 0 || roi.y < 0) //roi不合理
	{
		ERROR_CODE = 3;
		cerr << "roi不合理" << endl;
		output.dst = image;
		return output; //返回原图
	}


	Mat src = preimg(image, max, min, roi);
	Rect rect(1, 1, src.cols - 2, src.rows - 2); //扩充1像素后剪裁回原图
	Mat dst(src.size(), CV_8UC1, cv::Scalar(0));
	Mat imgmask(src.size(), CV_8UC1, cv::Scalar(0));
	std::vector<std::vector<cv::Point>>contours_in;//内轮廓
	std::vector<std::vector<cv::Point>>contours_final;

	findContours(src, contours_in, 2, 2);  //找出所有轮廓
	contours_in.erase(std::remove_if(contours_in.begin(), contours_in.end(),
		[areamax](const std::vector<cv::Point>& c) {return cv::contourArea(c) > areamax; }), contours_in.end());//去大连通域
	contours_in.erase(std::remove_if(contours_in.begin(), contours_in.end(),
		[areamin](const std::vector<cv::Point>& c) {return cv::contourArea(c) < areamin; }), contours_in.end());//去小连通域

	if (contours_in.size() == 0) { // 阈值设置不佳 返回roi，间隙宽度设为0.0
		ERROR_CODE = 4;
		cerr << "阈值设置不佳 返回roi" << endl;
		output.dst = src(rect);
		return output; //返回原图
	}
	cv::drawContours(dst, contours_in, -1, cv::Scalar(255), -1);
	cv::drawContours(imgmask, contours_in, -1, cv::Scalar(255), cv::FILLED);
	Mat dst_0 = dst(rect);
	Mat imgmask_0 = imgmask(rect);
	// 处理dst_0 得到拟合线
	dst_0 = nihe(dst_0);
	dst_0 = fix_img(dst_0);
	cvtColor(imgmask_0, imgmask_0, CV_GRAY2BGR);
	findContours(dst_0, contours_final, 2, 2);
	//vector<Point> VPResult = FindBigestContour(contours_final);
	vector<Point> VPResult = contours_final[0];
	Rect rect_lk = boundingRect(contours_final[0]);  //获得外接矩形 并去掉两端
	min_x = rect_lk.x + 200;
	max_x = rect_lk.x + rect_lk.width - 200;
	min_y = rect_lk.y;
	max_y = rect_lk.y + rect_lk.height;
	float dist = 0;
	//vector<Point> P;
	float sum_dist = 0.0;
	int sum_x = 0;
	//P = drawline(dst_0, imgmask_0);

	for (auto& item : center_P)    //遍历轮廓内所有点，画内接圆可视化
	{
		circle(imgmask_0, item, 1, Scalar(255, 0, 0), 1);
		if (item.x >= min_x && item.x <= max_x && item.y >= min_y && item.y <= max_y)
		{
			dist = pointPolygonTest(VPResult, item, true);

			sum_x++; //总宽度数
			sum_dist += dist * 2; //宽度总和

			if (item.x % 200 == 0 && dist >= 3.0)
			{
				circle(imgmask_0, item, dist, Scalar(0, 0, 255), 1);
			}
		}
	}
	center_P.clear();
	float diameter = sum_dist / sum_x;
	output.dst = imgmask_0;
	output.dist = diameter;
	return output;

}