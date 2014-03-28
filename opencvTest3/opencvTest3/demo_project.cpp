#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
//#include "header.h"
#include <math.h>

using namespace cv;
using namespace std;

int thresh = 245;
const int charsToClassify = 11; // 0-9 digits + $ sign
float ctrd[2] = {0.0, 0.0};
RNG rng(12345);

// ===================================================================

// temp arrays placed here because of an issue with passing its pointers:
const int numMoments = 6;
float myMoments[numMoments];
const int numFeatures = 5;
float features[numFeatures];
float moment1 = 0;

// ===================================================================
struct POINT {
   float x, y;
   int checked, removed;
};

// ===================================================================
void classify(std::string , vector<vector<Point> >,
		vector<Vec4i> , float [][charsToClassify], char );
void Sequential_moments(int , POINT * );
void Signature(int , POINT * , float * );
void centroid(int , POINT * , float * , float * );
float f_moment(float * , int , int );
float f_central_moment(float * , int , int , float );
int computeNumContourPts(Mat , bool );
void writeContourCoords(Mat , POINT arrayPoints[], int );
bool isInBounds(Mat imageForBounds, int , int );
void customThresholding(Mat );
void customThresh1D(Mat );
bool detectNextAndMatch(Mat , float [][charsToClassify]);
Mat threshNick(Mat );
void smoothen(Mat img);

// ===================================================================
int main()
{
	Mat imageGray;
	vector<vector<Point> > contour;
	vector<Vec4i> hierarchy;
	// an array to hold moments for every char class:
	float fDatabase [numFeatures][charsToClassify]; // moments database
	int i=0, x=0, y=0;

	// ========= go thru every char to classify =========

	cout << endl << endl << endl;
	classify("../src/0.jpg", contour, hierarchy, fDatabase, '0');
	classify("../src/1.jpg", contour, hierarchy, fDatabase, '1');
	classify("../src/2.jpg", contour, hierarchy, fDatabase, '2');
	classify("../src/3.jpg", contour, hierarchy, fDatabase, '3');
	classify("../src/4.jpg", contour, hierarchy, fDatabase, '4');
	classify("../src/5.jpg", contour, hierarchy, fDatabase, '5');
	classify("../src/6.jpg", contour, hierarchy, fDatabase, '6');
	classify("../src/7.jpg", contour, hierarchy, fDatabase, '7');
	classify("../src/8.jpg", contour, hierarchy, fDatabase, '8');
	classify("../src/9.jpg", contour, hierarchy, fDatabase, '9');
	classify("../src/dollar.jpg", contour, hierarchy, fDatabase, '$');

	// ========== all moments are now in database ===========
    // ============== process the test image: ===============

	Mat image = imread("../src/fullNum.jpg", 1); // converts image from one color space to another
	//cvtColor(image, imageGray, COLOR_BGR2GRAY );
	//imshow("greyscale", imageGray);
	//imwrite( "../src/out0-greyscale.png" , imageGray); // saves the chosen output image
	//threshNick("../src/cd.jpg");	- probably old code
	//customThresh1D(imageGray);	- not necessary
	//Mat imageTest;
	//threshold(imageTest, imageGray, 50, 255, THRESH_BINARY);
	image = threshNick(image);
	Mat imageTest = imread("../src/num001.jpg", 1);
	imshow("thresh", imageTest);
	imwrite( "../src/out1-thresh.png" , imageTest); // saves the chosen output image
	//dilate(imageGray, imageGray, Mat(), Point(-1,-1), 2); // use only for LED-style fonts
	//Size_<int> qm;
	//resize(image, image, qm, 0.0, 0.0, INTER_CUBIC);


	// ============ finished emphasizing numbers on the test image ============
	// ============ seeking dollar signs: ============

	/*
	for(int j=0; j < image.size().height; j++)
	{
		for(int i=0; i < image.size().width; i++)
		{
			if( image.ptr(j)[i] == 255 )
			{
				// yes/no = determine if it's a dollar sign (Eucl. dist. thresh. should depend on num. of moments):
				// yes:
				// no: 
			}
		}
	}
	*/

	// recognition based on Euclidean distances starts here:
	cout << "\nNumber of moments: " << numMoments << "\n\n";

	// !!!
	detectNextAndMatch(imageTest, fDatabase); // needs an isDollarSign flag for further detection.

	// goToNextChar(); 
	//  - traverse top half of contour
	//    = store top, bottom, and right pixels
	//    = then detect next region

	// ====================== done ========================
	//shows the image with colored contours
	imshow("Output", imageTest);
	imwrite( "../src/out2-dilation.png" , imageTest); // saves the chosen output image
	cout << "Done.\n";
	waitKey(0);
	return 0;
}

// ===================================================================
// ===================================================================

void classify(std::string imageFile, 
		vector<vector<Point> > contour,
		vector<Vec4i> hierarchy, float fDatabase [][charsToClassify],
		char whichChar)
{
	cout << "Classifying " << whichChar << "\n";

	POINT * arrayPoints; // for dynamic allocation
	Mat image = imread(imageFile, 1);
	Mat imageGrayClassify;

	//! converts image from one color space to another
	cvtColor(image, imageGrayClassify, COLOR_BGR2GRAY );
	//! applies fixed threshold to the image
	threshold(imageGrayClassify, imageGrayClassify, thresh, 255, THRESH_BINARY);

	// ======= time to find the 4 moments for the char: =======
	// pre-compute no_points(size)(N):
	int numPoints = computeNumContourPts(imageGrayClassify, 0);
	arrayPoints = new POINT[numPoints]; // now that we know the "size"

	// write the coords of every point into the points-array:
	writeContourCoords(imageGrayClassify, arrayPoints, numPoints);

	// Needs:
	// no_points(size)(N) - pre-computed numbers of contour pixels &
	// P[]( (x',y') ) - an array of coordinates stored in it.
	Sequential_moments(numPoints, arrayPoints);

	// output features for the given character:
	cout << "Char " << whichChar << " features: \n";
	//
	// writing features into the 2D-array ("database"):
	for (int i=0; i < numFeatures; i++)
	{
		cout << "Feature " << i+1 << ": " << features[i] << "\n";
		// digits:
		if ( whichChar >= 48 && whichChar <= 57 )
			fDatabase[i][ int(whichChar) - 48 ] = features[i];
		// dollar sign:
		else fDatabase[i][ charsToClassify - 1 ] = features[i];
	}
	cout << endl;

	// ------------------------------------------------------------------------
	/*
	cout << endl;
	if (whichChar == '9')
	{
		for (int i=0; i < numPoints-1; i++)
		{
			cout << pow( pow(abs(arrayPoints[i].x - ctrd[0]),2.0) + pow(abs(arrayPoints[i].y - ctrd[1]),2.0) , 0.5) << endl;
		}
	}
	*/
	// ------------------------------------------------------------------------

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
void Sequential_moments(int no_points, POINT * P)
{
	int i;
	float *S,*M;

	for(i=0; i < numMoments; i++)
		myMoments[i]=0.0;

	// need to determine min_length (min number of contour points)
	// to determine whether the white region is a good candidate
	// for consideration as a character in our database:

	if(no_points > 0) // 0 = min_length 
	{ 
		S = new float[no_points];
		Signature(no_points, P, S);
		M = new float[numMoments+1];
		M[0] = f_moment(S, no_points, 1);
		moment1 = M[0];

		for(i=1; i < numMoments+1; i++)
			M[i] = f_central_moment(S, no_points, i+1, M[0]);

		//normalization:
		myMoments[0]=(float)sqrt((double)M[1])/M[0];
		for(i=1; i < numMoments; i++)
			myMoments[i]=M[i+1]/(float)sqrt(pow((double)M[1],(double)(i+2)));

		// compute the feature vectore for this char:
		//cout << "Now: " << myMoments[1] << endl << endl;
		features[0] = pow( abs(myMoments[1]), 0.5f ) / (moment1);
		//cout << "features[0]: " << features[0] << endl;
		features[1] = (myMoments[2]) / pow( abs(myMoments[1]), 1.5f );
		features[2] = (myMoments[3]) / pow( abs(myMoments[1]), 2.0f );
		// features[3] = F4 = M'5 = M5/(M2^(5/2)):
		features[3] = (myMoments[4]) / pow( abs(myMoments[1]), 2.5f );
		features[4] = (myMoments[5]) / pow( abs(myMoments[1]), 3.0f );

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

  for(i=0; i < no_points; i++)
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
    Mx=(P[i].x+P[i+1].x)/2.0f;
    My=(P[i].y+P[i+1].y)/2.0f;
    dx=P[i].x-P[i+1].x;
    dy=P[i].y-P[i+1].y;
    lm = (float)hypot(dx, dy);
    Summ += lm;
    Sumx += Mx*lm; Sumy += My*lm;
  }
  *cgx=(float)(Sumx/Summ);
  *cgy=(float)(Sumy/Summ);

  ctrd[0] = *cgx;
  ctrd[1] = *cgy;
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
int computeNumContourPts(Mat imgGrayComp, bool testFlag)
{
	// must be ints for processing image:
	int i=0, j=imgGrayComp.size().height/2; // j - y, i - x
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


	// ======= to prevent from getting stuck at white noise pixels of contour: =======
	if(testFlag == 0)
	{
		// dilation x1 (increases workload):
		dilate(imgGrayComp, imgGrayComp, Mat(), Point(-1,-1), 2); // recommended # of steps: 2
		// erosion x1 (decreases workload):
		erode(imgGrayComp, imgGrayComp, Mat(), Point(-1,-1), 1); // recommended # of steps: 1
	}
	else
	{
		//dilation x1 (increases workload):
		dilate(imgGrayComp, imgGrayComp, Mat(), Point(-1,-1), 2); // recommended # of steps: 3
		//erosion x1 (decreases workload):
		erode(imgGrayComp, imgGrayComp, Mat(), Point(-1,-1), 1); // recommended # of steps: 2
	}
	// 3,2 for $
	// 2,1 for 0
	// 2,1 for 

	// find the 1st (next) char in the image:
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

	// count the white pixel:
	numPoints++;

	dir = 0; // reset direction
	// find the first black neighbor:
	while ( imgGrayComp.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 255 )
	{
		dir = (dir+1) % 8;
	}

	// find the first white neighbor: (why not ... == 0?)
	while ( imgGrayComp.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] != 255 )
	{
		dir = (dir+1) % 8;
	}

	// go to the first white neighbor after the black ones:
	zix = zix + dirs[dir][0];
	ziy = ziy + dirs[dir][1];

	// go around 1 time (use the start/finish pixel)
	// and count contour pixels:
	//
	// (zix!=z0x) && (ziy!=z0y) did not work:
	while ( !(zix==z0x && ziy==z0y) )
	{
		// count the white pixel:
		numPoints++;

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
	}

	return numPoints;
}

void writeContourCoords(Mat imgGrayWrite, POINT arrayPoints[],
		int numPoints)
{
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
	//
	// the start/finish pixel (write it into the array):
	arrayPoints[0].x = (float)i;
	arrayPoints[0].y = (float)j;
	// current pixel for feeling the contour:
	zix = (int)arrayPoints[0].x;
	ziy = (int)arrayPoints[0].y;

	// =======================================================
	// change coords of the white pixel (move to the next one)
	// in order to not get stuck on the first one when
	// going through the loops:
	// =======================================================

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

	// go to the first white neighbor after the black ones:
	arrayPoints[currPoint].x = (float)zix + (float)dirs[dir][0];
	arrayPoints[currPoint].y = (float)ziy + (float)dirs[dir][1];
	// write it into the array:
	zix = (int)arrayPoints[currPoint].x;
	ziy = (int)arrayPoints[currPoint].y;

	// go around 1 time (use the start/finish pixel)
	// and count contour pixels:

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

		// go to the first white neighbor after the black ones:
		arrayPoints[currPoint].x = (float)zix + (float)dirs[dir][0];
		arrayPoints[currPoint].y = (float)ziy + (float)dirs[dir][1];

		// write it into the array:
		zix = (int)arrayPoints[currPoint].x;
		ziy = (int)arrayPoints[currPoint].y;
	}
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

	//cout << "\nHeight: " << imageThresh.size().height << endl;
	//cout << "\nWidth: " << imageThresh.size().width << endl;
	//imgGrayComp.at<uchar>(5, 4) = 0;

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

// ================= 1-channel thresholding: =====================
void customThresh1D(Mat imageThresh)
{
	//imgGrayComp.at<uchar>(5, 4) = 0;

	// modifying value of every pixel - thresholding:
	for(int i=0 ; i < (imageThresh.size().height) ; i++) // row
	{
	    for(int j=0 ; j < (imageThresh.size().width) ; j++) // col
	    {
	    	// finding nearly white pixel and making them abs. black:
	    	if ( imageThresh.ptr(i)[j] < 245)
	    	{
	    		imageThresh.ptr(i)[j] = 0;
	    	}
	    	else // make the rest of the image white:
	    	{
	    		imageThresh.ptr(i)[j] = 255;
	    	}
	    }
	}

	// ============== done thresholding ==============
}

bool detectNextAndMatch(Mat imgDtct, float fDatabase[][charsToClassify])
{
	//cout << "Analyzing contour " << whichChar << "\n";

	POINT * arrayPoints; // for dynamic allocation
	float minEuclDist = 9999998;
	int minEuclDistInd = 0;
	//
	float minEuclDist2 = 9999999;
	float confusionDifferential= 0;


	// ======= time to find the 4 moments for the char: =======
	// pre-compute no_points(size)(N):
	int numPoints = computeNumContourPts(imgDtct, 1);
	arrayPoints = new POINT[numPoints]; // now that we know the "size"

	// write the coords of every point into the points-array:
	writeContourCoords(imgDtct, arrayPoints, numPoints);

	// Needs:
	// no_points(size)(N) - pre-computed numbers of contour pixels &
	// P[]( (x',y') ) - an array of coordinates stored in it.
	Sequential_moments(numPoints, arrayPoints);

	// output moments for the current white region:
	//cout << "Char " << whichChar << " moments: \n";

	// given trained database with moments and the
	// moments of the testing object in the image,
	// compute 11 Euclidean distances (0-9 and $):
	for(int i=0; i < charsToClassify; i++)
	{
		// computing the Euclidean distance between train and test images:
		float squareEuclDist = 0;
		for(int f=0; f < numFeatures; f++)
		{
			//cout << "features[f]: " << features[f] << endl;
			squareEuclDist += pow(features[f] - fDatabase[f][i],2);
		}
		float euclDist = pow( squareEuclDist, 0.5f );

		// print Euclidean distance:
		if( i <= charsToClassify-2 ) // 11-2=9 => 0-9
			cout << "Eucl. dist. from " << i;
		else cout << "Eucl. dist. from $";
		cout << ": " << euclDist << endl;

		// update the minimum Euclidean distance:
		if (euclDist < minEuclDist)
		{
			minEuclDist2 = minEuclDist;
			minEuclDist = euclDist;
			minEuclDistInd = i;
		}
	}

	// charsToClassify = 9;
	// 0 1 2 3 4 5 6 7 $;
	// 0 1 2 3 4 5 6 7 8;

	// ------------------------------------------------------------------------
	/*
	cout << endl;
	for (int i=0; i < numPoints-1; i++)
	{
		cout << pow( pow(abs(arrayPoints[i].x - ctrd[0]),2.0) + pow(abs(arrayPoints[i].y - ctrd[1]),2.0) , 0.5) << endl;
	}
	*/
	// ------------------------------------------------------------------------

	delete[] arrayPoints; // release memory before returning (with or without a value).

	// print the recognized character:
	cout << "\nMin. eucl. dist.: " << minEuclDist << endl;
	//cout << "2nd Min. eucl. dist.: " << minEuclDist2 << endl;
	//cout << "Confusion differential (if correct: low - OK, high - very good): " << minEuclDist2-minEuclDist << endl << endl;

	if ( minEuclDistInd >= 0 && minEuclDistInd <= (charsToClassify-2) )
	{
		cout << "Recognized char: " << minEuclDistInd << endl << endl;
		//return false;
	}
	else 
	{
		cout << "Recognized char: $\n\n";
		//return true;
	}

	return 0;
}

//attempts to threshold an image with the name that is passed in
Mat threshNick(Mat image)
{
	vector<vector<Point> > contour;
	vector<Vec4i> hierarchy;
	Mat imageGray, imageBin;
	int largestArea = 0;
	int i = 0;

	cvtColor(image, imageGray, CV_BGR2GRAY);

	blur (imageGray, imageGray, Size(1, 1) );

	imshow("Gray Image", imageGray);
	dilate(imageGray, imageGray, Mat(), Point(-1,-1), 1);
	Mat dst = Mat::zeros(image.rows, image.cols, CV_8UC3);
	threshold(imageGray, imageBin, 0, 255, THRESH_BINARY | THRESH_OTSU);

	erode(imageBin,imageBin, Mat(), Point(-1,-1), 1);

	morphologyEx(imageBin, imageBin, MORPH_OPEN, 1);

	imshow("Otsu", imageBin);

	dst = imageBin;

	Mat imageContour;
	imageBin.copyTo(imageContour);
	findContours(imageContour, contour, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0));

	for(i = 0; i < contour.size(); i++)
	{
		double a = contourArea(contour[i],false);
		Scalar white = Scalar(255,255,255);
		drawContours(imageBin, contour, i, white, CV_FILLED, 8, hierarchy, 0, Point()  );
	}

	imageBin.copyTo(imageContour);
	findContours(imageContour, contour, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0));

	vector<Rect> boundRect( contour.size() );

	for(i = 0; i < contour.size(); i++)
	{
		char path[255], num[10];
		strcpy(path, "../src/num");
        sprintf(num, "%03i", i);
        strcat(path, num);   
        strcat(path, ".jpg");
		boundRect[i] = boundingRect( Mat(contour[i]) );
		boundRect[i].height +=6;
		boundRect[i].width +=6;
		boundRect[i].x -=3;
		boundRect[i].y -=3;
		Mat miniNum;
		miniNum = imageBin(boundRect[i]);
		imwrite(path, miniNum);
	}

	waitKey(0);
	return imageBin;
}

void smoothen(Mat image)
{
	int dirs[8][2];
	dirs[0][0] = -1; dirs[0][1] = -1;
	dirs[1][0] = 0; dirs[1][1] = -1;
	dirs[2][0] = 1; dirs[2][1] = -1;
	dirs[3][0] = 1; dirs[3][1] = 0;
	dirs[4][0] = 1; dirs[4][1] = 1;
	dirs[5][0] = 0; dirs[5][1] = 1;
	dirs[6][0] = -1; dirs[6][1] = 1;
	dirs[7][0] = -1; dirs[7][1] = 0;

	for(int j=0; j < image.size().height; j++)
	{
		for(int i=0; i < image.size().width; i++)
		{
			int blackNeighbors = 0;
			//computeBlackNeighbors();

			if( image.ptr(j)[i] == 255 )
			{
				// yes/no = determine if it's a dollar sign (Eucl. dist. thresh. should depend on num. of moments):
				// yes:
				// no: 
			}
		}
	}
}
