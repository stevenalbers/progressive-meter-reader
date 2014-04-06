#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
//#include "header.h"
#include <math.h>

using namespace cv;
using namespace std;

//Mat image;
Mat imageGray;
int thresh = 100;
int max_thresh = 255;
const int charsToClassify = 9; // 0-9 digits + $ sign
RNG rng(12345);

// temp array of moments placed here because of
// an issue with passing its pointer:
float myMoments[4];

// ===================================================================
struct POINT {
   float x,y;
   int checked,removed;
};

// ===================================================================
void classify(std::string , vector<vector<Point> >,
		vector<Vec4i> , float [][charsToClassify], char );
void Sequential_moments(int no_points, POINT * P,
		int no_moments);
void Signature(int no_points, POINT * P, float * S);
void centroid(int size, POINT * P, float * cgx, float * cgy);
float f_moment(float * S, int n, int r);
float f_central_moment(float * S, int n, int r, float m1);
int computeNumContourPts(Mat );
void writeContourCoords(Mat , POINT arrayPoints[], int );
bool isInBounds(Mat imageForBounds, int y, int x);
void customThresholding(Mat );
void detectNextRegion(Mat , float [][charsToClassify]);

// ===================================================================
int main()
{
	cout << endl << endl << endl;
	// ======= make an image with colored contours around digits: ========

	vector<vector<Point> > contour;
	vector<Vec4i> hierarchy;
	// an array to hold moments for every char class:
	float mDatabase [4][charsToClassify]; // 4 moments for 0-7

	// ========= go thru every char to classify =========

	classify("../src/0.jpg", contour, hierarchy, mDatabase, '0');
	classify("../src/1.jpg", contour, hierarchy, mDatabase, '1');
	classify("../src/2.jpg", contour, hierarchy, mDatabase, '2');
	classify("../src/3.jpg", contour, hierarchy, mDatabase, '3');
	classify("../src/4.jpg", contour, hierarchy, mDatabase, '4');
	classify("../src/5.jpg", contour, hierarchy, mDatabase, '5');
	classify("../src/6.jpg", contour, hierarchy, mDatabase, '6');
	classify("../src/7.jpg", contour, hierarchy, mDatabase, '7');

	classify("../src/dollar.jpg", contour, hierarchy, mDatabase, '$');
	//cout << "Problem here\n\n";

	// ========= all moments are now in database =========
    // ============== read the test image: ===============

	Mat image = imread("../src/testImg.jpg", 1);
	customThresholding(image);

	//! converts image from one color space to another
	cvtColor(image, imageGray, COLOR_BGR2GRAY );
	//! applies fixed threshold to the image
	threshold(imageGray, imageGray, thresh, 255, THRESH_BINARY);
	//! retrieves contours and the hierarchical information from black-n-white image.
	findContours(imageGray, contour, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0));

	// ============ done testing on a the bigger image: ============

	// Needed for defining and drawing contours:
	// (contour.size() = number of characters / outlines)
	vector<vector<Point> > polygons(contour.size()); // 2D-vtr mats
	vector<Rect> boxes(contour.size()); // a vector of contours

	// !!! define contours for drawing:
	for(int i = 0; i < contour.size(); i++)
	{
		//! approximates contour or a curve using Douglas-Peucker algorithm
		approxPolyDP( Mat(contour[i]), polygons[i], 3, true );
		//! computes the bounding rectangle for a contour
		boxes[i] = boundingRect( Mat(polygons[i]) );
	}

	// draw contours
	for(int i = 0; i < contour.size(); i++)
	{
		//Scalar randomColor = Scalar( rng.uniform(0,255), rng.uniform(0,255), rng.uniform(0,255));
		Scalar randomColor = Scalar( 128, 128, 128 );
		rectangle(imageGray, boxes[i].tl(), boxes[i].br(), randomColor, 2, 8, 0);
	}

	//imageGray.ptr(4)[5] = 0;
	// dilation x1 (increases workload):
	//dilate(imageGray, imageGray, Mat(), Point(-1,-1), 1);
	// erosion x1 (decreases workload):
	//erode(imageGray, imageGray, Mat(), Point(-1,-1), 1);

	// recognition based on Euclidean distances starts here:
	detectNextRegion(imageGray, mDatabase);

	//shows the image with colored contours
	imshow("Output", imageGray);
	imwrite( "./src/out.png" , imageGray); // saves the chosen output image

	//cout << "Height: " << image.size().height;
	//cout << " Width: " << image.size().width << endl;

	// ====================== done ========================
	//cout << "My moment: " << mDatabase[3][4] << endl;
	cout << "Done.\n";
	waitKey(0);
	return 0;
}

// ===================================================================
// ===================================================================

void classify(std::string imageFile, 
		vector<vector<Point> > contour,
		vector<Vec4i> hierarchy, float mDatabase [][charsToClassify],
		char whichChar)
{
	cout << "Classifying " << whichChar << "\n";

	POINT * arrayPoints; // for dynamic allocation
	int numMoments = 4;
	Mat image = imread(imageFile, 1);
	//if(image.empty()) cout << "empty\n";
	//else cout << "got it\n";	
	Mat imageGrayClassify;
	
	//! converts image from one color space to another
	cvtColor(image, imageGrayClassify, COLOR_BGR2GRAY );
	cout << "\nHere 1\n\n";
	//! applies fixed threshold to the image
	threshold(imageGrayClassify, imageGrayClassify, thresh, 255, THRESH_BINARY);
	//! retrieves contours and the hierarchical information from black-n-white image.
	//findContours(imageGray, contour, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0));

	// ======= time to find the 4 moments for the char: =======
	// pre-compute no_points(size)(N):
	
	cout << "\nHere 2\n\n";
	int numPoints = computeNumContourPts(imageGrayClassify);
	cout << "\nHere 2a\n\n";
	arrayPoints = new POINT[numPoints]; // now that we know the "size"
	cout << "\nHere 3\n\n";

	// write the coords of every point into the points-array:
	writeContourCoords(imageGrayClassify, arrayPoints, numPoints);

	cout << "\nHere 4\n\n";

	// Needs:
	// no_points(size)(N) - pre-computed numbers of contour pixels &
	// P[]( (x',y') ) - an array of coordinates stored in it.
	Sequential_moments(numPoints, arrayPoints, numMoments);

	// output moments for the given character:
	cout << "Char " << whichChar << " moments: \n";
	//
	// writing moments into the 2D-array ("database"):
	for (int i=0; i < numMoments; i++)
	{
		cout << "Moment " << i << ": " << myMoments[i] << "\n";
		// digits:
		if ( whichChar >= 48 && whichChar <= 57 )
			mDatabase[i][ int(whichChar) - 48 ] = myMoments[i];
		// dollar sign:
		else mDatabase[i][ charsToClassify - 1 ] = myMoments[i];
	}
	cout << endl;

	delete[] arrayPoints;
}

/*  ========================================================================

          Computation of the Sequential Moments of a contour

	            from L. Gupta and M. Srinath:
   "Contour Sequence Moments for the Classification of Closed Planar Shapes"

    Reference: Pattern Recognition, vol. 20, no. 3, pp. 267-272, 1987
    ---------

           ***********************************************

                              by
                          George Bebis

            Dept. of Electrical and Computer Engineeering
                   University of Central Florida
			Orlando, FL 32816

           ***********************************************

  Inputs:
          no_points - The number of contour points
            = (count the num of pixels)

          P - The x, y coordinates of the contour
            = (the ctrd?? use findContour() to compute the ctrd posn)

          no_moments - The desired number of moments
            = (4)

  Output:
          Moments - The computed moments
  ======================================================================== */

// Needs:
// no_points(size)(N) - pre-computed numbers of contour pixels &
// P[]( (x',y') ) - an array of coordinates stored in it.
void Sequential_moments(int no_points, POINT * P,
		int no_moments)
{
	int i;
	float *S,*M;

	for(i=0; i<no_moments; i++)
		myMoments[i]=0.0;

	// need to determine min_length (min number of contour points)
	// to determine whether the white region is a good candidate
	// for consideration as a character in our database:
	if(no_points > 0) { // 0 = min_length

		S = new float[no_points];
		Signature(no_points, P, S);
		M = new float[no_moments+1];
		M[0] = f_moment(S, no_points, 1);

		for(i=1; i<no_moments+1; i++)
			M[i] = f_central_moment(S, no_points, i+1, M[0]);

		//normalization:
		myMoments[0]=(float)sqrt((double)M[1])/M[0];
		for(i=1; i<no_moments; i++)
			myMoments[i]=M[i+1]/(float)sqrt(pow((double)M[1],(double)(i+2)));

		delete[] S;
		delete[] M;
	}
}

// ===================================================================

// Needs:
// no_points(size)(N) - pre-computed numbers of contour pixels &
// P[]( (x',y') ) - an array of coordinates stored in it.
void Signature(int no_points, POINT * P, float * S)
{
  int i;
  float cgx, cgy;

  centroid(no_points, P, &cgx, &cgy);
  //cout << "centroid = " << cgx << " " << cgy << endl;

  for(i=0; i<no_points; i++)
    S[i]=(float)hypot((double)(cgx-P[i].x),(double)(cgy-P[i].y));
}

// ===================================================================

// Needs:
// no_points(size)(N) - pre-computed numbers of contour pixels &
// P[]( (x',y') ) - an array of coordinates stored in it.
//
void centroid(int size, POINT * P, float * cgx, float * cgy)
{
  float Mx, My, dx, dy, Sumx, Sumy, Summ, lm;
  int i;

  Sumx = Sumy = Summ = 0;
  for(i = 0; i < size-1; i++) {
    Mx=(P[i].x+P[i+1].x)/2.0;
    My=(P[i].y+P[i+1].y)/2.0;
    dx=P[i].x-P[i+1].x;
    dy=P[i].y-P[i+1].y;
    lm = hypot(dx, dy);
    Summ += lm;
    Sumx += Mx*lm; Sumy += My*lm;
  }
  *cgx=(float)(Sumx/Summ);
  *cgy=(float)(Sumy/Summ);
}

// ===============================================================

// Needs:
//
float f_moment(float * S, int n, int r)
{
 int i;
 float sum;

 sum=0.0;
 for(i=0; i<n; i++)
   sum += (float)pow((double)S[i],(double)r);
  sum /= (float)n;
  return(sum);
}

// ===================================================================

// Needs:
//
float f_central_moment(float * S, int n, int r, float m1)
{
 int i;
 float sum;

 sum=0.0;
 for(i=0; i<n; i++)
   sum += (float)pow((double)(S[i]-m1),(double)r);
 sum /= (float)n;
 return(sum);
}

// ================================================================

// computes the number of contour pixels of a white region:
//
int computeNumContourPts(Mat imgGrayComp)
{
	// must be ints for processing image:
	int i=0, j=imgGrayComp.size().height/2; // j - width, i - height
	int z0x, z0y, zix, ziy, numPoints = 0, dir = 0;
	// "circular" array (we don't want a warning here):
	/* int dirs[8][2] {
		{-1, -1}, {0, -1}, {1, -1}, {1, 0},
		{1, 1}, {0, 1}, {-1, 1}, {-1, 0}
	}; */
	int dirs[8][2];
	dirs[0][0] = -1; dirs[0][1] = -1;
	dirs[1][0] = 0; dirs[1][1] = -1;
	dirs[2][0] = 1; dirs[2][1] = -1;
	dirs[3][0] = 1; dirs[3][1] = 0;
	dirs[4][0] = 1; dirs[4][1] = 1;
	dirs[5][0] = 0; dirs[5][1] = 1;
	dirs[6][0] = -1; dirs[6][1] = 1;
	dirs[7][0] = -1; dirs[7][1] = 0;

	cout << "\nIn comp 1\n\n";

	// ======= to prevent from getting stuck at white noise pixels: =======
	// dilation x1 (increases workload):
	dilate(imgGrayComp, imgGrayComp, Mat(), Point(-1,-1), 2);
	// erosion x1 (decreases workload):
	erode(imgGrayComp, imgGrayComp, Mat(), Point(-1,-1), 1);

	cout << "\nIn comp 1a\n\n";

	// dollar sign image fix-up due to dilation/erosion issues:
	//imgGrayComp.ptr(4)[5] = 0;
	imgGrayComp.at<uchar>(5, 4) = 0;
	
	cout << "\nIn comp 2\n\n";

	//cout << "in comp 0\n";
	// go through the image in diagonal from the top left corner;
	// find the char in the image:
	while( imgGrayComp.ptr(j)[i] == 0 )
	{
		i++;
	}

	// starting/final contour pixel:
	z0x = i;
	z0y = j;
	// ... is our current pixel:
	zix = z0x;
	ziy = z0y;

	// =======================================================
	// change coords of the white pixel (move to the next one)
	// in order to not get stuck on the first one when
	// going through the loops:
	// =======================================================

	//cout << "in comp 1\n";
	// count the white pixel:
	numPoints++;

	dir = 0; // reset direction
	// find the first black neighbor:
	while ( imgGrayComp.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 255 )
	{
		dir = (dir+1) % 8;
	}
	/* cout << "After 1st while dir = " << dir << endl;
	cout << "z0x = " << z0x << " ; zix = " << zix << endl;
	cout << "z0y = " << z0y << " ; ziy = " << ziy << endl; */

	// (5, 5) = 0
	// (5, 4) = 255

	// find the first white neighbor: (why not ... == 0?)
	while ( imgGrayComp.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] != 255 )
	{
		/* cout << "(" << zix+dirs[dir][0] << "," << ziy+dirs[dir][1]
		    << ") pix val = "
			<< (int) imgGrayComp.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])]
		    << endl; */
		dir = (dir+1) % 8;
	}
	/* cout << "(" << zix+dirs[dir][0] << "," << ziy+dirs[dir][1]
	    << ") pix val = "
		<< (int) imgGrayComp.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])]
	    << endl; */
	//cout << "After 2nd while dir = " << dir << endl;
	// go to the first white neighbor after the black ones:
	zix = zix + dirs[dir][0];
	ziy = ziy + dirs[dir][1];
	//cout << "1: zix = " << zix << endl;
	//cout << "1: ziy = " << ziy << endl;

	/* cout << "zix-4 = " << zix << endl;
	cout << "ziy-4 = " << ziy << endl;
	cout << "dir = " << dir << endl; // lll */
	/* cout << "z0x = " << z0x << " ; zix = " << zix << endl;
	cout << "z0y = " << z0y << " ; ziy = " << ziy << endl; */

	//cout << "in comp 2\n";
	// go around 1 time (use the start/finish pixel)
	// and count contour pixels:
	// cout << "comp 1\n";

	// (zix!=z0x) && (ziy!=z0y) did not work:
	while ( !(zix==z0x && ziy==z0y) )
	{
		// count the white pixel:
		numPoints++;
		//for(int i=0; i<1000000; i++){} // short stall
		//cout << numPoints << ": " << "zix=" << zix << " , ziy=" << ziy << endl;

		dir = 0; // reset direction

		// find the first black or NULL neighbor:
		bool next = true;
		while (next)
		{
			if ( isInBounds(imgGrayComp, ziy+dirs[dir][1], zix+dirs[dir][0]) )
			{
				if ( imgGrayComp.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 255 )
				{
					dir = (dir+1) % 8; // if white
				}
				else next = false; // if black
			}
			else next = false; // if out of bounds
		}

		// find the first white neighbor:
		next = true;
		while(next)
		{
			if ( !isInBounds(imgGrayComp, ziy+dirs[dir][1], zix+dirs[dir][0]) )
			{
				dir = (dir+1) % 8; // if out of bounds
			}
			else if (imgGrayComp.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 0)
			{
				dir = (dir+1) % 8; // if black
			}
			else next = false; // if white
		}

		// go to the first white neighbor after the black ones:
		zix = zix + dirs[dir][0];
		ziy = ziy + dirs[dir][1];

		//for(int i=0; i<10000000; i++){}; // short stall
		/* cout << "numPoints: " << numPoints << endl;
		cout << "z0x = " << z0x << " ; zix = " << zix << endl;
		cout << "z0y = " << z0y << " ; ziy = " << ziy << endl; */
	}
	//cout << "comp 3\n";

	//cout << "quitting computeNumContourPts - problem is stopping early\n";
	return numPoints;
}

void writeContourCoords(Mat imgGrayWrite, POINT arrayPoints[],
		int numPoints)
{
	//cout << "What now-a?\n";
	//image = imread("./src/cropped4.jpg", 1);
	//cout << "What now?-b\n";
	// must be ints for processing image:
	int i=0, j=imgGrayWrite.size().height/2; // j - width, i - height
	int zix, ziy, currPoint = 0, dir = 0;
	// "circular" array (we don't want a warning here):
	int dirs[8][2];
	dirs[0][0] = -1; dirs[0][1] = -1;
	dirs[1][0] = 0; dirs[1][1] = -1;
	dirs[2][0] = 1; dirs[2][1] = -1;
	dirs[3][0] = 1; dirs[3][1] = 0;
	dirs[4][0] = 1; dirs[4][1] = 1;
	dirs[5][0] = 0; dirs[5][1] = 1;
	dirs[6][0] = -1; dirs[6][1] = 1;
	dirs[7][0] = -1; dirs[7][1] = 0;

	// go through the image in diagonal; find the char:
	while( imgGrayWrite.ptr(j)[i] == 0 )
	{
		i++;
	}
	//cout << "i = " << i << " ; j = " << j << endl;
	// the start/finish pixel (write it into the array):
	arrayPoints[0].x = i;
	arrayPoints[0].y = j;
	// current pixel for feeling the contour:
	zix = arrayPoints[0].x;
	ziy = arrayPoints[0].y;

	// =======================================================
	// change coords of the white pixel (move to the next one)
	// in order to not get stuck on the first one when
	// going through the loops:
	// =======================================================

	//cout << "Here 1\n";
	// count the white pixel:
	currPoint++;

	dir = 0; // reset direction
	// find the first black neighbor:
	while ( imgGrayWrite.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 255 )
	{
		dir = (dir+1) % 8;
	}
	// find the first white neighbor:
	while ( imgGrayWrite.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 0 )
	{
		dir = (dir+1) % 8;
	}
	/* cout << "zix-1 = " << zix << endl;
	cout << "ziy-1 = " << ziy << endl;
	cout << "dir = " << dir << endl; */
	// go to the first white neighbor after the black ones:
	arrayPoints[currPoint].x = zix + dirs[dir][0];
	arrayPoints[currPoint].y = ziy + dirs[dir][1];
	// write it into the array:
	zix = arrayPoints[currPoint].x;
	ziy = arrayPoints[currPoint].y;
	/* cout << "zix-2 = " << zix << endl;
	cout << "ziy-2 = " << ziy << endl;
	cout << "dir = " << dir << endl; // lll */

	// go around 1 time (use the start/finish pixel)
	// and count contour pixels:

	//while ( !(zix==arrayPoints[0].x && ziy==arrayPoints[0].y) )
	while ( currPoint!=(numPoints-1) )
	{
		// count the white pixel:
		currPoint++;

		dir = 0; // reset direction

		// find the first black or NULL neighbor:
		bool next = true;
		while (next)
		{
			if ( isInBounds(imgGrayWrite, ziy+dirs[dir][1], zix+dirs[dir][0]) )
			{
				if ( imgGrayWrite.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 255 )
				{
					dir = (dir+1) % 8; // if white
				}
				else next = false; // if black
			}
			else next = false; // if out of bounds
		}

		// find the first white neighbor:
		next = true;
		while(next)
		{
			if ( !isInBounds(imgGrayWrite, ziy+dirs[dir][1], zix+dirs[dir][0]) )
			{
				dir = (dir+1) % 8; // if out of bounds
			}
			else if (imgGrayWrite.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 0)
			{
				dir = (dir+1) % 8; // if black
			}
			else next = false; // if white
		}

		//cout << "Here-ac\n";
		// go to the first white neighbor after the black ones:
		arrayPoints[currPoint].x = zix + dirs[dir][0];
		arrayPoints[currPoint].y = ziy + dirs[dir][1];

		// write it into the array:
		/*
		cout << "numPoints2: " << currPoint << endl;
		cout << "arrayPoints[0].x = " << arrayPoints[0].x << " ; zix = " << zix << endl;
		cout << "arrayPoints[0].y = " << arrayPoints[0].y << " ; ziy = " << ziy << endl;
		*/

		//cout << "Here-ad\n";
		zix = arrayPoints[currPoint].x;
		ziy = arrayPoints[currPoint].y;

		//cout << "Here-ae\n";
	}

	//cout << "quitting writing" << endl;
	//return;
}

// ... because using the image pointer to refer to a
// (non-)existent pixel did not work:
bool isInBounds(Mat imageForBounds, int y, int x)
{
	if ( x>=0 && x<imageForBounds.size().width &&
			y>=0 && y<imageForBounds.size().height)
	return true;
	else return false;
}

void customThresholding(Mat imageThresh)
{
	// ================= thresholding: =====================

	// {245, 254, 255}, {245, 255, 255}
	// not: {252, 254, 255} - black specks (need to lower blue)
	// not: {xx, 254, 255} - allows for yellow font
	// not: {xx, 255, 254} - allows for yellow font
	int blackThresh[3] = {252, 252, 252};

	// modifying value of every pixel - thresholding:
	for(int i=0 ; i < (imageThresh.size().height) ; i++) // row
	{
	    for(int j=0 ; j < (imageThresh.size().width) ; j++) // col
	    {
	    	// finding nearly white pixel and making them abs. white:
	    	if ( imageThresh.ptr(i)[3*j+2] <= blackThresh[2] &&
	    		imageThresh.ptr(i)[3*j+1] <= blackThresh[1] &&
	    		imageThresh.ptr(i)[3*j] <= blackThresh[0] )
	    	{
	    		imageThresh.ptr(i)[3*j+2] = 0;	// red		(r%3=2)
	    		imageThresh.ptr(i)[3*j+1] = 0;	// green	(g%3=1)
	    		imageThresh.ptr(i)[3*j] = 0;	// blue		(b%3=0)
	    	}
	    	else // make the rest of the image black:
	    	{
	    		imageThresh.ptr(i)[3*j+2] = 255;	// red		(r%3=2)
	    		imageThresh.ptr(i)[3*j+1] = 255;	// green	(g%3=1)
	    		imageThresh.ptr(i)[3*j] = 255;		// blue		(b%3=0)
	    	}
	    }
	}

	// ============== done thresholding ==============
}

void detectNextRegion(Mat imgDtct, float mDatabase[][charsToClassify])
{
	//cout << "Analyzing contour " << whichChar << "\n";

	POINT * arrayPoints; // for dynamic allocation
	int numMoments = 4;
	//float EuclDists[charsToClassify];
	float minEuclDist = 99999;
	int minEuclDistInd = 99999;

	//! converts image from one color space to another
	//cvtColor(imgDtct, imgDtct, CV_BGR2GRAY );
	//! applies fixed threshold to the image
	//threshold(imgDtct, imgDtct, thresh, 255, THRESH_BINARY);
	//! retrieves contours and the hierarchical information from black-n-white image.
	//findContours(imageGray, contour, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0));

	// ======= time to find the 4 moments for the char: =======
	// pre-compute no_points(size)(N):
	int numPoints = computeNumContourPts(imgDtct);
	arrayPoints = new POINT[numPoints]; // now that we know the "size"

	// write the coords of every point into the points-array:
	writeContourCoords(imgDtct, arrayPoints, numPoints);

	// Needs:
	// no_points(size)(N) - pre-computed numbers of contour pixels &
	// P[]( (x',y') ) - an array of coordinates stored in it.
	Sequential_moments(numPoints, arrayPoints, numMoments);

	// output moments for the given character:
	//cout << "Char " << whichChar << " moments: \n";

	// given trained database with moments and the
	// moments of the testing object in the image,
	// compute 11 Euclidean distances (0-9 and $):
	for(int i=0; i<charsToClassify; i++)
	{
		//cout << "Debugging computation: "
		//		<< pow(pow(myMoments[0] - mDatabase[0][i], 2), 0.5) << endl;

		float euclDist = pow( pow(myMoments[0] - mDatabase[0][i],2)
				+ pow(myMoments[1] - mDatabase[1][i],2)
				+ pow(myMoments[2] - mDatabase[2][i],2)
				+ pow(myMoments[3] - mDatabase[3][i],2) ,0.5);

		if(i<=charsToClassify-2)
			cout << "Eucl. dist. from " << i;
		else cout << "Eucl. dist. from $";
		cout << ": " << euclDist << endl;

		if (euclDist < minEuclDist)
		{
			minEuclDist = euclDist;
			minEuclDistInd = i;
		}
	}

	// charsToClassify = 9;
	// 0 1 2 3 4 5 6 7 $;
	// 0 1 2 3 4 5 6 7 8;

	cout << "\nMin. eucl. dist.: " << minEuclDist << endl << endl;
	if ( minEuclDistInd >= 0 && minEuclDistInd <= (charsToClassify-2) )
		cout << "Recognized char: " << minEuclDistInd << endl;
	else cout << "Recognized char: $\n";

	/* for (int i=0; i < numMoments; i++)
	{
		cout << "Moment " << i << ": " << myMoments[i] << "\n";
		// digits:
		if ( whichChar >= 48 && whichChar <= 57 )
			mDatabase[i][ int(whichChar) - 48 ] = myMoments[i];
		// dollar sign:
		else mDatabase[i][ charsToClassify - 1 ] = myMoments[i];
	} */
	cout << endl;

	delete[] arrayPoints;
}

