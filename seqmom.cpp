#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
//#include "header.h"
#include <math.h>

using namespace cv;
using namespace std;

Mat image;
Mat imageGray;
Mat src; Mat src_gray;
int thresh = 100;
int max_thresh = 255;
RNG rng(12345);
// placed here because of
// 1) an issue with passing its pointer
// 2) writing moments into it using the for-loop in seq_mom fxn
float myMoments[4];

// ===================================================================
struct POINT {
   float x,y;
   int checked,removed;
};

// ===================================================================
void classify(std::string , vector<vector<Point> >,
		vector<Vec4i> , int [][8], int );
void Sequential_moments(int no_points, POINT * P,
		int no_moments);
void Signature(int no_points, POINT * P, float * S);
void centroid(int size, POINT * P, float * cgx, float * cgy);
float f_moment(float * S, int n, int r);
float f_central_moment(float * S, int n, int r, float m1);
int computeNumContourPts(Mat );
void writeContourCoords(Mat , POINT arrayPoints[]);

// ===================================================================
int main()
{
	// ======= make an image with colored contours around digits: ========

	vector<vector<Point> > contour;
	vector<Vec4i> hierarchy;
	// an array to hold moments for every char class:
	int mDatabase [4][8]; // 4 moments for 0-7

	// ========= go thru every char to classify =========

	classify("./src/0.jpg", contour, hierarchy, mDatabase, 0);
	cout << "in main\n";
	/* classify("./src/1.jpg", contour, hierarchy, mDatabase, 1);
	classify("./src/2.jpg", contour, hierarchy, mDatabase, 2);
	classify("./src/3.jpg", contour, hierarchy, mDatabase, 3);
	classify("./src/4.jpg", contour, hierarchy, mDatabase, 4);
	classify("./src/5.jpg", contour, hierarchy, mDatabase, 5);
	classify("./src/6.jpg", contour, hierarchy, mDatabase, 6);
	classify("./src/7.jpg", contour, hierarchy, mDatabase, 7); */

	// ========= all moments are now in database =========
    // ============== read the test image: ===============

	//image = imread("./src/cropped4.jpg", 1);

	// ================= thresholding: =====================

	/*
	// {245, 254, 255}, {245, 255, 255}
	// not: {252, 254, 255} - black specks (need to lower blue)
	// not: {xx, 254, 255} - allows for yellow font
	// not: {xx, 255, 254} - allows for yellow font
	int blackThresh[3] = {252, 252, 252};

	// modifying value of every pixel - thresholding:
	for(int i=0 ; i < (image.size().height) ; i++) // row
	{
	    for(int j=0 ; j < (image.size().width) ; j++) // col
	    {
	    	// finding nearly white pixel and making them abs. white:
	    	if ( image.ptr(i)[3*j+2] <= blackThresh[2] &&
	    		image.ptr(i)[3*j+1] <= blackThresh[1] &&
	    		image.ptr(i)[3*j] <= blackThresh[0] )
	    	{
	    		image.ptr(i)[3*j+2] = 0;	// red		(r%3=2)
	    		image.ptr(i)[3*j+1] = 0;	// green	(g%3=1)
	    		image.ptr(i)[3*j] = 0;	// blue		(b%3=0)
	    	}
	    	else // make the rest of the image black:
	    	{
	    		image.ptr(i)[3*j+2] = 255;	// red		(r%3=2)
	    		image.ptr(i)[3*j+1] = 255;	// green	(g%3=1)
	    		image.ptr(i)[3*j] = 255;	// blue		(b%3=0)
	    	}
	    }
	}
	*/

	// ============== done thresholding ==============

	//! converts image from one color space to another
	cvtColor(image, imageGray, CV_BGR2GRAY );
	//! applies fixed threshold to the image
	threshold(imageGray, imageGray, thresh, 255, THRESH_BINARY);
	//! retrieves contours and the hierarchical information from black-n-white image.
	//findContours(imageGray, contour, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0));

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

	//shows the image with colored contours
	imshow("Output", imageGray);
	imwrite( "./src/out.png" , imageGray); // saves the chosen output image

	//cout << "Height: " << image.size().height;
	//cout << " Width: " << image.size().width << endl;


	// ====================== done ========================
	waitKey(0);
	return 0;
}

// ===================================================================
// ===================================================================

void classify(std::string imageFile, vector<vector<Point> > contour,
		vector<Vec4i> hierarchy, int mDatabase [][8], int whichChar)
{
	POINT * arrayPoints; // for dynamic allocation
	int numMoments = 4;
	image = imread(imageFile, 1);

	cout << "in classify 1\n";
	//! converts image from one color space to another
	cvtColor(image, imageGray, CV_BGR2GRAY );
	//! applies fixed threshold to the image
	threshold(imageGray, imageGray, thresh, 255, THRESH_BINARY);
	//! retrieves contours and the hierarchical information from black-n-white image.
	//findContours(imageGray, contour, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0));

	cout << "in classify 2\n";
	// ======= time to find the 4 moments for the char: =======
	// pre-compute no_points(size)(N):
	int numPoints = computeNumContourPts(imageGray);
	arrayPoints = new POINT[numPoints]; // now that we know the "size"
	//
	// write the coords of every point into the points-array:
	//cout << "numPoints: " << numPoints << endl;
	writeContourCoords(imageGray, arrayPoints);
	cout << "in classify after writeContourCoords\n";
	//
	// Needs:
	// no_points(size)(N) - pre-computed numbers of contour pixels &
	// P[]( (x',y') ) - an array of coordinates stored in it.
	Sequential_moments(numPoints, arrayPoints, numMoments);

	cout << "\n" << whichChar << ": \n\n";

	for (int i=0; i < numMoments; i++)
	{
		cout << "Moment " << i << " " << myMoments[i] << endl;
		mDatabase[i][whichChar] = myMoments[i];
	}

	cout << endl;
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
	//float f_moment(),f_central_moment();

	cout << "in seq_mom before for-loop\n";
	for(i=0; i<no_moments; i++)
		myMoments[i]=0.0;
	cout << "in seq_mom after for-loop\n";

	if(no_points > 0) { // 0 = min_length
		cout << "Here 95 - " << no_points << endl;
		for (int z=0; z<no_points; z++)
		{
			cout << "P = " << z
				<< " ; x = " << P[z].x << " , y = " << P[z].y << endl;
		}
		//no_points = 51;
		S = (float *)malloc(no_points * sizeof(float));
		cout << "Here 95-b\n";

		Signature(no_points, P, S);
		cout << "Here 96\n";

		M=(float *)malloc((no_moments+1)*sizeof(float));

		M[0] = f_moment(S, no_points, 1);
		cout << "Here 97\n";

		for(i=1; i<no_moments+1; i++)
			M[i] = f_central_moment(S, no_points, i+1, M[0]);

		//normalization
		cout << "Here 98\n";

		myMoments[0]=(float)sqrt((double)M[1])/M[0];
		for(i=1; i<no_moments; i++)
			myMoments[i]=M[i+1]/(float)sqrt(pow((double)M[1],(double)(i+2)));
	}
	cout << "Here 99\n";
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

// Needs:
//
int computeNumContourPts(Mat imageGray)
{
	//cout << "in computeNumContourPts 1\n";
	// must be ints for processing image:
	int i=0, j=0; // j - width, i - height
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

	//cout << "Height: " << image.size().height;
	//cout << " Width: " << image.size().width << endl;

	//cout << "in computeNumContourPts 2\n";
	// go through the image in diagonal; find the char:
	while( imageGray.ptr(i)[j] == 0 )
	{
		//cout << (int) imageGray.ptr(i)[j] << endl;
		//imageGray.ptr(i)[j] = 255;
		i++;
		j++;
	}
	//cout << "Why does it stop and (3, 3) ?\n";
	//imshow("Output", imageGray);
	//imwrite( "./src/out.png" , imageGray); // saves the chosen output image
	cout << "i = " << i << " ; j = " << j << endl;
	//waitKey(0);
	//return 0; // the start/finish pixel:
	z0x = j;
	z0y = i;
	// current pixel for feeling the contour:
	zix = z0x;
	ziy = z0y;

	// =======================================================
	// change coords of the white pixel (move to the next one)
	// in order to not get stuck on the first one when
	// going through the loops:
	// =======================================================

	// count the white pixel:
	numPoints++;

	dir = 0; // reset direction
	// find the first black neighbor:
	while ( imageGray.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 255 )
	{
		dir = (dir+1) % 8;
	}
	//cout << "z0x = " << z0x << " ; zix = " << zix << endl;
	//cout << "z0y = " << z0y << " ; ziy = " << ziy << endl;
	//cout << "in computeNumContourPts 4 - the problem is after here\n";
	// find the first white neighbor:
	while ( imageGray.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 0 )
	{
		dir = (dir+1) % 8;
	}
	//cout << "z0x = " << z0x << " ; zix = " << zix << endl;
	//cout << "z0y = " << z0y << " ; ziy = " << ziy << endl;
	//cout << "in computeNumContourPts 5 - the problem is before here\n";
	// go to the first white neighbor after the black ones:
	cout << "zix-3 = " << zix << endl;
	cout << "ziy-3 = " << ziy << endl;
	cout << "dir = " << dir << endl;
	zix = zix + dirs[dir][0];
	ziy = ziy + dirs[dir][1];
	cout << "zix-4 = " << zix << endl;
	cout << "ziy-4 = " << ziy << endl;
	cout << "dir = " << dir << endl; // lll
	//cout << "z0x = " << z0x << " ; zix = " << zix << endl;
	//cout << "z0y = " << z0y << " ; ziy = " << ziy << endl;

	// go around 1 time (use the start/finish pixel)
	// and count contour pixels:
	while ( !(zix==z0x && ziy==z0y) ) // (zix!=z0x) && (ziy!=z0y) did not work.
	{
		// count the white pixel:
		numPoints++;

		dir = 0; // reset direction kkk
		// find the first black neighbor:
		//for(int k=0; k<70000000; k++){}
		//cout << "z0x = " << z0x << " ; zix = " << zix << endl;
		//cout << "z0y = " << z0y << " ; ziy = " << ziy << endl;
		while ( imageGray.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 255 )
		{
			cout << "Hi\n";
			dir = (dir+1) % 8;
		}
		//cout << "in computeNumContourPts 6\n";
		// find the first white neighbor:
		while ( imageGray.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 0 )
		{
			cout << "Been here\n";
			dir = (dir+1) % 8;
		}
		//cout << "in computeNumContourPts 7\n";
		// go to the first white neighbor after the black ones:
		zix = zix + dirs[dir][0];
		ziy = ziy + dirs[dir][1];
		cout << "numPoints: " << numPoints << endl;
		cout << "z0x = " << z0x << " ; zix = " << zix << endl;
		cout << "z0y = " << z0y << " ; ziy = " << ziy << endl;
	}

	//cout << "quitting computeNumContourPts - problem is stopping early\n";
	return numPoints;
}

void writeContourCoords(Mat imageGray, POINT arrayPoints[])
{
	cout << "What now?\n";
	// must be ints for processing image:
	int i=0, j=0; // j - width, i - height
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

	// read the image:
	//image = imread(imageFile, 1);

	// go through the image in diagonal; find the char:
	while( imageGray.ptr(i)[j] == 0 )
	{
		i++;
		j++;
	}
	cout << "i = " << i << " ; j = " << j << endl;
	// the start/finish pixel (write it into the array):
	arrayPoints[0].x = j;
	arrayPoints[0].y = i;
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
	while ( imageGray.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 255 )
	{
		dir = (dir+1) % 8;
	}
	//cout << "Here 2\n";
	// find the first white neighbor:
	while ( imageGray.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 0 )
	{
		dir = (dir+1) % 8;
	}
	cout << "zix-1 = " << zix << endl;
	cout << "ziy-1 = " << ziy << endl;
	cout << "dir = " << dir << endl;
	// go to the first white neighbor after the black ones:
	arrayPoints[currPoint].x = zix + dirs[dir][0];
	arrayPoints[currPoint].y = ziy + dirs[dir][1];
	// write it into the array:
	zix = arrayPoints[currPoint].x;
	ziy = arrayPoints[currPoint].y;
	cout << "zix-2 = " << zix << endl;
	cout << "ziy-2 = " << ziy << endl;
	cout << "dir = " << dir << endl; // lll

	// go around 1 time (use the start/finish pixel)
	// and count contour pixels:
	//cout << "Here 3\n";
	cout << "Problem in the following while-loop" << endl;
	while ( !(zix==arrayPoints[0].x && ziy==arrayPoints[0].y) )
	{
		//for(int k=0; k<100000000; k++){}
		//cout << "Here 4\n";
		// count the white pixel:
		currPoint++;

		dir = 0; // reset direction kkk
		// find the first black neighbor:
		/* cout << "Hi-1\n";
		cout << "zix+dirs[dir][0] = " << zix+dirs[dir][0] << endl;
		cout << "ziy+dirs[dir][1] = " << ziy+dirs[dir][1] << endl;
		cout << "value = "
			<< (int) imageGray.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])]
			<< endl; */
		while ( imageGray.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 255 )
		{
			/* cout << "Hi-2\n";
			cout << "zix+dirs[dir][0] = " << zix+dirs[dir][0] << endl;
			cout << "ziy+dirs[dir][1] = " << ziy+dirs[dir][1] << endl;
			cout << "value = "
				<< (int) imageGray.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])]
				<< endl; */
			dir = (dir+1) % 8;
		}
		//cout << "Here 5\n";
		// find the first white neighbor:
		/* cout << "\n Hi-3\n"; // mmm
		cout << "zix+dirs[dir][0] = " << zix+dirs[dir][0] << endl; // pre-4
		cout << "ziy+dirs[dir][1] = " << ziy+dirs[dir][1] << endl; // pre-3
		cout << "value = "
			<< (int) imageGray.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])]
			<< endl; // 0 */
		while ( imageGray.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 0 )
		{
			//cout << "Been here\n";
			dir = (dir+1) % 8;
		}
		//cout << "Here 6\n";
		// go to the first white neighbor after the black ones:
		zix = zix + dirs[dir][0];
		ziy = ziy + dirs[dir][1];
		// write it into the array:
		cout << "numPoints2: " << currPoint << endl;
		cout << "arrayPoints[0].x = " << arrayPoints[0].x << " ; zix = " << zix << endl;
		cout << "arrayPoints[0].y = " << arrayPoints[0].y << " ; ziy = " << ziy << endl;
		arrayPoints[currPoint].x = zix;
		arrayPoints[currPoint].y = ziy;
		//cout << "Here 8\n";
	}
	cout << "quitting writing" << endl;
}

