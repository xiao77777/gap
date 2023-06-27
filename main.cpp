#pragma once


#include "gap_work.h"
using namespace cv;

int main()
{
	GAP_WORK t;
	
	clock_t start, end1;
	start = clock();
	Mat a = imread("1.bmp", 0);
	//Mat a;
	OUT_PUT OUT;
	for (size_t i = 0; i < 20; i++)
	{
		Rect roi(0, 1400, 4000, 300);
		OUT = t.gap_test(80, -1, 5000000, 500, roi, a);
		std::cout << OUT.dist << std::endl;
	}
	
	
	end1 = clock();
	std::cout << "（4000*300）20次总时长："<<  (float)(end1 - start) * 1000 / CLOCKS_PER_SEC <<"ms"<< std::endl;
	imshow("out", OUT.dst);
	waitKey(0);
	return 0;
}


