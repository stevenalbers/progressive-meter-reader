#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>


using namespace cv;
using namespace std;

Mat image;
Mat imageGray;
Mat imageBin;
RNG rng(12345);

void threshold(string);

int main()
{
	threshold("image2-green.png");
	threshold("image2-red.png");
	threshold("image2-orange.png");
	threshold("image2-blue.png");
	threshold("image2-purple.png");

	threshold("image3-1.png");
	threshold("image3-2.png");
	threshold("image3-3.png");
	threshold("image3-4.png");
	threshold("image3-5.png");

	return 0;
}


//attempts to threshold an image with the name that is passed in
void threshold(string name)
{
	int r = 0;
	int c = 0;
	vector<vector<Point> > contour;
	vector<Vec4i> hierarchy;
	image = imread(name, 1);
	int largestArea = 0;
	
//Abort if file is not successfully opened
	if(!image.data)
	{
		cout << "File does not exist."<<endl;
		return;
	}
	
	imshow("Original Image", image);	

	cvtColor(image, imageGray, CV_BGR2GRAY);
	
	blur (imageGray, imageGray, Size(1, 1) );
		
	imshow("Gray Image", imageGray);
	dilate(imageGray, imageGray, Mat(), Point(-1,-1), 1);
	Mat dst = Mat::zeros(image.rows, image.cols, CV_8UC3);
	threshold(imageGray, imageBin, 0, 255, THRESH_BINARY | THRESH_OTSU);

	erode(imageBin,imageBin, Mat(), Point(-1,-1), 1);

	morphologyEx(imageBin, imageBin, MORPH_OPEN, 1);
	
	imshow("Otsu", imageBin);
	
/*	Mat fg;
	erode(imageBin, fg, Mat(), Point(-1,-1), 1);
	imshow("fg", fg);
	
	Mat bg;
	dilate(imageBin, bg, Mat(), Point(-1,-1), 4);
	threshold(bg, bg, 0, 128, 1);
	imshow("BG", bg);
	
	Mat markers(imageBin.size(), CV_8U, Scalar(0));
	markers = fg+bg;
	imshow("markers", markers);
	
	markers.convertTo(markers, CV_32S);
	watershed(image, markers);
	
	markers.convertTo(markers, CV_8U);
	threshold(imageBin, markers,0,255, THRESH_BINARY | THRESH_OTSU);
	imshow("Watershed", markers);
*/	
	findContours(imageBin, contour, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0));

	for(int i = 0; i < contour.size(); i++)
	{
		double a = contourArea(contour[i],false);
		Scalar white = Scalar(255,255,255);
		drawContours(imageBin, contour, i, white, CV_FILLED, 8, hierarchy, 0, Point()  );
	}
	
	imshow("TEST", imageBin);

	waitKey(0);
}

