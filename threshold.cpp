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
	threshold("cropped_5.png");	

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
	
	//Abort if file is not successfully opened
	if(!image.data)
	{
		cout << "File does not exist."<<endl;
		return;
	}
	
	imshow("Original Image", image);	

	dilate(image, image, Mat(), Point(-1,-1), 1);
	cvtColor(image, imageGray, CV_BGR2GRAY);
	
	cout << image.rows<<endl;
	cout << image.cols<<endl;
	r = image.rows/40;
	c = image.cols/40;
	cout << r << endl;
	cout << c << endl;
	
	if(r < 1)
	{
		r = 1;
		c = c + 1;
	}
	
	if(c < 1)
	{
		c = 1;
		r = r + 1;
	}

//	blur (imageGray, imageGray, Size(r, c) );
		
//	imshow("Gray Image", imageGray);

	Mat dst = Mat::zeros(image.rows, image.cols, CV_8UC3);
	threshold(imageGray, imageBin, 0, 255, THRESH_BINARY | THRESH_OTSU);

	erode(imageBin,imageBin, Mat(), Point(-1,-1), 1);
	
	imshow("Binary", imageBin);
	
/*	Mat fg;
	erode(imageBin, fg, Mat(), Point(-1,-1), 1);
	imshow("fg", fg);
	
	Mat bg;
	dilate(imageBin, bg, Mat(), Point(-1,-1), 2);
	threshold(bg, bg, 0, 128, 1);
	imshow("BG", bg);
	
	Mat markers(imageBin.size(), CV_8U, Scalar(0));
	markers = fg+bg;
	imshow("markers", markers);
	
	markers.convertTo(markers, CV_32S);
	watershed(image, markers);
	
	markers.convertTo(markers, CV_8U);
	threshold(markers, markers,0,255, THRESH_BINARY | THRESH_OTSU);
	imshow("TEST", markers);
	
	findContours(imageBin, contour, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0));

	for(int i = 0; i < contour.size(); i++)
	{
		Scalar randomColor = Scalar( rng.uniform(0,255), rng.uniform(0,255), rng.uniform(0,255));
		drawContours (dst, contour, i, randomColor, 2, 8, hierarchy, 0, Point() );
	}

	imshow("Contours", dst);*/
	waitKey(0);
}

