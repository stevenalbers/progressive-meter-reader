#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
//#include "header.h"
#include <math.h>

using namespace cv;
using namespace std;

bool reachedEndOfRgn = 0;

std::string str;
int rightOfObj = 0;
int rightOfRgn = 0;
int gameChoice = 0; // no game chosen
int thresh = 50;
const int charsToClassify = 11; // 0-9 digits + $ sign
float ctrd[2] = {0.0, 0.0}; // for centroidal profiling
RNG rng(12345);

// ===================================================================

// temp arrays placed here because of an issue with passing its pointers:
const int numMoments = 11; //
float myMoments[numMoments];

const int numFeatures = 10;
// should be manually changed to quantity = numMoments-1 since it's a const.

float features[numFeatures];
float moment1 = 0;

// ===================================================================
struct POINT {
   float x, y;
};

// ===================================================================
void train(std::string , float [][charsToClassify], char );
void Sequential_moments(int , POINT * );
void Signature(int , POINT * , float * );
void centroid(int , POINT * , float * , float * );
float f_moment(float * , int , int );
float f_central_moment(float * , int , int , float );
int computeNumContourPts(Mat , vector< vector<int> > & , bool );
void writeContourCoords(Mat , POINT arrayPoints[], int , vector< vector<int> > & , bool );
bool isInBounds(Mat imageForBounds, int , int );
//void customThresholding(Mat );
void customThresh1D(Mat );
void detectNextAndMatch(Mat , Mat , float [][charsToClassify]);
//void threshNick(string );
void smoothen(Mat img);
void clearFlags(vector< vector<int> > & , Mat);
void mapContours(Mat , POINT arrayPoints[], int);

// ===================================================================
//								MAIN()
// ===================================================================
int main()
{
	Mat imageGray; // for greyscale and thresholding
	Mat imgWithContours; // for the image with white object contours
	// an array to hold moments for every char class:
	float fDatabase [numFeatures][charsToClassify]; // moments database
	int i=0, x=0, y=0;
	POINT j0tl; // top left point of region
	POINT j0br; // bottom right point of region
	str = "\n";
	
	// ===========================================  CHOOSING THE GAME  =============================================
	cout << "\nPlease enter the ID of the game you will train and test on: \n";
	cout << "0 - quit \n";
	cout << "1 - CashBurst \n";
	cout << "\nID: ";
	cin >> gameChoice;
	cout << endl;
	// gameChoice = 1; // Cashburst (ID needed for specifying training and testing images).
	while(gameChoice < 0 || gameChoice > 1)
	{
		cout << "Please try again. \n";
		cout << "ID: ";
		cin >> gameChoice;
	}

	// ===========================================  TRAINING  =============================================

	if(gameChoice == 0) return 0; // exit
	else if (gameChoice == 1) // Cashburst
	{
		// top left:
		j0tl.x = 195;
		j0tl.y = 60;
		// bottom right:
		j0br.x = 1164;
		j0br.y = 218;
		rightOfRgn = 1164;
		// horizontal displacement:
		rightOfObj = 195;

		/* POINT j1tl;
		j1tl.x = 100;
		j1tl.y = 200;
		POINT j2tl;
		j2tl.x = 100;
		j2tl.y = 200;
		POINT j3tl;
		j3tl.x = 100;
		j3tl.y = 200;
		POINT j4tl;
		j4tl.x = 100;
		j4tl.y = 200; */
	
		// ========= go thru every char to classify =========
		cout << endl << endl << endl;

		train("../src/trainCashburstJ1/0.jpg", fDatabase, '0');
		train("../src/trainCashburstJ1/1.jpg", fDatabase, '1');
		train("../src/trainCashburstJ1/2.jpg", fDatabase, '2');
		train("../src/trainCashburstJ1/3.jpg", fDatabase, '3');
		train("../src/trainCashburstJ1/4.jpg", fDatabase, '4');
		train("../src/trainCashburstJ1/5.jpg", fDatabase, '5');
		train("../src/trainCashburstJ1/6.jpg", fDatabase, '6');
		train("../src/trainCashburstJ1/7.jpg", fDatabase, '7');
		train("../src/trainCashburstJ1/8.jpg", fDatabase, '8');
		train("../src/trainCashburstJ1/9.jpg", fDatabase, '9');
		train("../src/trainCashburstJ1/dollar.jpg", fDatabase, '$');
	}
	
	// =====================================  TESTING - GREYSCALE AND THRESH  ======================================

	// ========== all moments are now in database ===========
    // ============== process the test image: ===============

	// greyscale:
	Mat image = imread("../src/in.jpg", 1); // converts image from one color space to another
	cvtColor(image, imageGray, COLOR_BGR2GRAY );
	//imshow("greyscale", imageGray);
	//imwrite( "../src/out0-greyscale.png" , imageGray); // saves the chosen output image
	
	// thresholding:
	threshold(imageGray, imageGray, thresh, 255, THRESH_BINARY); // arg3 = threshold
	//imshow("thresh", imageGray);
	//imwrite( "../src/out1-thresh.png" , imageGray); // saves the chosen output image

	//dilate(imageGray, imageGray, Mat(), Point(-1,-1), 2); // use only for LED-style fonts

	// =========================================  TESTING - DETECTION  ============================================
	// recognition based on Euclidean distances starts here:
	cout << "\nNumber of feature moments: " << numFeatures << "\n\n";
	// threshold() used to copy one image into another:
	threshold(imageGray, imgWithContours, thresh, 255, THRESH_BINARY); 

	// create a blank copy of the same size as the testing image:
	for(int j=0; j < imgWithContours.size().height; j++)
		for(int i=0; i < imgWithContours.size().width; i++)
			imgWithContours.ptr(j)[i] = 0;

	while(!reachedEndOfRgn)
	{
		detectNextAndMatch(imageGray, imgWithContours, fDatabase);
	}

	// goToNextChar(); 
	//    = store rightmost pixel of the prev char
	//    = then detect next region

	//shows the dilated version of thresh image:
	imshow("dilErd", imageGray);
	imwrite( "../src/out2-dilErd.png" , imageGray); // saves the chosen output image

	// =========================================  DRAWING CONTOURS  ============================================
	dilate(imgWithContours, imgWithContours, Mat(), Point(-1,-1), 1);
	/*Canny(imgWithContours, imgWithContours, 100, 200, 3);
	vector<vector<Point> > contour;
	vector<Vec4i> hierarchy;
	RNG rng(12345);
	//Mat imgWithContoursColor( imgWithContours.size() , CV_8UC3 );
	//cvtColor(imgWithContours, imgWithContoursColor, CV_GRAY2BGR );
	findContours(imgWithContours, contour, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0));
	Mat drawing = Mat::zeros( imgWithContours.size() , CV_8UC3);
	
	vector<vector<Point> > polygons(contour.size()); // 2D-vtr mats
	vector<Rect> boxes(contour.size()); // a vector of contourss

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
		Scalar color = Scalar( rng.uniform(0,255), rng.uniform(0,255), rng.uniform(0,255));
		//Scalar randomColor = Scalar( 128, 128, 128 );
		//rectangle(imgWithContoursColor, boxes[i].tl(), boxes[i].br(), randomColor, 2, 8, 0);
		drawContours( drawing, contour, i, color, 2, 8, hierarchy );
	}
	*/

	//shows the image with contour contours:
	imshow("contours", imgWithContours);
	imwrite( "../src/out3-contours.png" , imgWithContours); // saves the chosen output image

	// ====================== closing time: ========================
	cout << "\nRecognized text: ";
	cout << str;
	cout << "\n\nDone.\n";
	waitKey(0);
	return 0;
}

// ===================================================================
// ===================================================================

void train(std::string imageFile, float fDatabase [][charsToClassify],
		char whichChar)
{
	cout << "Classifying " << whichChar << "\n";
	
	// ===============================================================================
	POINT * arrayPoints; // for dynamic allocation
	Mat image = imread(imageFile, 1);
	Mat imageGrayClassify;
	//
	vector< vector<int> > imgPxlsFlags (image.size().height, vector<int>(image.size().width));
	clearFlags(imgPxlsFlags, image); // clear after allocation
	// ===============================================================================
	
	//! converts image from one color space to another
	cvtColor(image, imageGrayClassify, COLOR_BGR2GRAY );
	//! applies fixed threshold to the image
	threshold(imageGrayClassify, imageGrayClassify, thresh, 255, THRESH_BINARY);

	// ======= time to find the 4 moments for the char: =======
	// pre-compute no_points(size)(N):
	int numPoints = computeNumContourPts(imageGrayClassify, imgPxlsFlags, 0); // might encounter inf-loop
	clearFlags(imgPxlsFlags, image); // clear after allocation
	arrayPoints = new POINT[numPoints]; // now that we know the "size"

	// write the coords of every point into the points-array:
	writeContourCoords(imageGrayClassify, arrayPoints, numPoints, imgPxlsFlags, 0);
	clearFlags(imgPxlsFlags, image); // clear after allocation

	// Needs:
	// no_points(size)(N) - pre-computed numbers of contour pixels &
	// P[]( (x',y') ) - an array of coordinates stored in it.
	Sequential_moments(numPoints, arrayPoints);

	// output features for the given character:
	cout << "Char " << whichChar << " features: \n";
	//
	// writing features into the 2D-array ("database"):
	for (int i=0; i <= numFeatures-1; i++)
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
		
		// ============= compute the feature vectore for this char: ==============

		// Note: features[3] = F4 = M'5 = M5/(M2^(5/2))
		for(int j=0; j <= numFeatures-1; j++)
		{
			if(j==0) features[j] = pow( abs(myMoments[1]), 0.5 ) / (moment1);
			else
			{
				features[j] = (myMoments[j+1]) / pow( abs(myMoments[1]), 1.0+(j*0.5) );
			}
		}
		
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
int computeNumContourPts(Mat imgGrayComp, vector< vector<int> > & imgPxlsFlags, bool testFlag)
{
	// must be ints for processing the image:
	int i=0, j=imgGrayComp.size().height/2; // default settings for training; j - y, i - x ; 0, imgGrayComp.size().height/2


	if (testFlag) 
	{
		if(gameChoice == 1) // if testing on Cashburst (==1) AND it's J1 - 195x60 ; 969x158
		{
			i = rightOfObj; // horizontal component to start with
			j = ((158-60)/2)+60;
		}
	}

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


	int numOfSteps = 1;
	// ======= to prevent from getting stuck at white noise pixels of contour: =======
	//dilation x? (increases workload):
	dilate(imgGrayComp, imgGrayComp, Mat(), Point(-1,-1), numOfSteps); // recommended # of steps: ?
	//erosion x? (decreases workload):
	erode(imgGrayComp, imgGrayComp, Mat(), Point(-1,-1), numOfSteps); // recommended # of steps: ?
		
	// find the 1st (next) char in the image:
	while( imgGrayComp.ptr(j)[i] == 0 )
	{
		i++;

		if(testFlag)
		{
			if( i >= rightOfRgn ) 
			{
				reachedEndOfRgn = 1;
				return 0;
			}
		}
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

	// find the first unflagged white neighbor: (why not ... == 0? - grey)
	while ( imgGrayComp.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] != 255 || imgPxlsFlags[ ziy+dirs[dir][1] ][ zix+dirs[dir][0] ]==1 )
	{
		dir = (dir+1) % 8;
	}
	
	// go to the first white neighbor after the black ones:
	zix = zix + dirs[dir][0];
	ziy = ziy + dirs[dir][1];
	imgPxlsFlags[ ziy ][ zix ] = 1; // set flag

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
				if ( imgGrayComp.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 255)
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
			else if (imgGrayComp.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 0 || imgPxlsFlags[ ziy+dirs[dir][1] ][ zix+dirs[dir][0] ]==1 )
			{
				dir = (dir+1) % 8; // if black
			}
			else next = false; // if white
		}

		// go to the first white neighbor after the black ones:
		zix = zix + dirs[dir][0];
		ziy = ziy + dirs[dir][1];
		imgPxlsFlags[ ziy ][ zix ] = 1; // set flag
	}
	
	return numPoints;
}

void writeContourCoords(Mat imgGrayWrite, POINT arrayPoints[],
		int numPoints, vector< vector<int> > & imgPxlsFlags, bool testFlag)
{
	// must be ints for processing image:
	int i=0, j=imgGrayWrite.size().height/2; // j - y, i - x ; 0, imgGrayComp.size().height/2
	if (testFlag) // 195x60 ; 969x158
	{
		if(gameChoice == 1)
		{
			// J1:
			i = rightOfObj; // horizontal component to start with
			j = ((158-60)/2)+60;
		}
	}

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

	// count the white pixel:
	currPoint++;

	dir = 0; // reset direction
	// find the first black neighbor:
	while ( imgGrayWrite.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 255 )
	{
		dir = (dir+1) % 8;
	}
	// find the first white neighbor:
	while ( imgGrayWrite.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 0 || imgPxlsFlags[ ziy+dirs[dir][1] ][ zix+dirs[dir][0] ]==1 )
	{
		dir = (dir+1) % 8;
	}
	
	// go to the first white neighbor after the black ones:
	arrayPoints[currPoint].x = zix + dirs[dir][0];
	arrayPoints[currPoint].y = ziy + dirs[dir][1];
	// write it into the array:
	zix = arrayPoints[currPoint].x;
	ziy = arrayPoints[currPoint].y;
	imgPxlsFlags[ ziy ][ zix ] = 1; // set flag
	
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
			else if (imgGrayWrite.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 0 || imgPxlsFlags[ ziy+dirs[dir][1] ][ zix+dirs[dir][0] ]==1 )
			{
				dir = (dir+1) % 8; // if black
			}
			else next = false; // if white
		}

		// go to the first white neighbor after the black ones:
		arrayPoints[currPoint].x = zix + dirs[dir][0];
		arrayPoints[currPoint].y = ziy + dirs[dir][1];

		// write it into the array:
		zix = arrayPoints[currPoint].x;
		ziy = arrayPoints[currPoint].y;
		imgPxlsFlags[ ziy ][ zix ] = 1; // set flag
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

/*
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
*/

// ================= 1-channel thresholding: =====================
/*
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
*/

void detectNextAndMatch(Mat imgDtct, Mat imgWithContours, float fDatabase[][charsToClassify])
{
	//cout << "Analyzing contour " << whichChar << "\n";

	POINT * arrayPoints; // for dynamic allocation
	float minEuclDist = 9999998;
	int minEuclDistInd = 0;
	//
	float minEuclDist2 = 9999999;
	float confusionDifferential= 0;
	//
	vector< vector<int> > imgPxlsFlags (imgDtct.size().height, vector<int>(imgDtct.size().width));
	clearFlags(imgPxlsFlags, imgDtct); // clear after allocation
	// ===============================================================================

	// ======= time to find the 4 moments for the char: =======
	// pre-compute no_points(size)(N):
	int numPoints = computeNumContourPts(imgDtct, imgPxlsFlags, 1);
	if (reachedEndOfRgn) return; // get out; done with the region
	clearFlags(imgPxlsFlags, imgDtct); // clear after allocation
	arrayPoints = new POINT[numPoints]; // now that we know the "size"

	// write the coords of every point into the points-array:
	writeContourCoords(imgDtct, arrayPoints, numPoints, imgPxlsFlags, 1);
	clearFlags(imgPxlsFlags, imgDtct); // clear after allocation

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
		for(int f=0; f <= numFeatures-1; f++)
		{
			//cout << "features[f]: " << features[f] << endl;
			squareEuclDist += pow(features[f] - fDatabase[f][i],2);
		}
		float euclDist = pow( squareEuclDist, 0.5 );

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

	// ============================	code for output for a centroidal profile ===============================
	/*
	cout << endl;
	for (int i=0; i < numPoints-1; i++)
	{
		cout << pow( pow(abs(arrayPoints[i].x - ctrd[0]),2.0) + pow(abs(arrayPoints[i].y - ctrd[1]),2.0) , 0.5) << endl;
	}
	*/
	
	// ============================	EUCLIDEAN DISTANCES OUTPUT ===============================
	// print the recognized character:
	cout << "\nMin. eucl. dist.: " << minEuclDist << endl;
	//cout << "2nd Min. eucl. dist.: " << minEuclDist2 << endl;
	//cout << "Confusion differential (if correct: low - OK, high - very good): " << minEuclDist2-minEuclDist << endl << endl;

	if ( minEuclDistInd >= 0 && minEuclDistInd <= (charsToClassify-2) )
	{
		string regObj;
		stringstream converter;
		converter << minEuclDistInd;
		regObj = converter.str();
		str.append(regObj);
	}
	else 
	{
		str.append("$");
	}

	// ====================== BEFORE WE DESTROY THE ARRAY AND LEAVE, =======================
	// ==================== MAP THE CONTOUR PIXELS AND HIGHLIGHT THEM ======================
	mapContours(imgWithContours, arrayPoints, numPoints);

	// ================ FIND THE RIGHTMOST PIXEL TO DETECT NEXT CHARACTER =================
	for(int z=0; z < numPoints; z++)
	{
		if(rightOfObj < arrayPoints[z].x)
		{
			rightOfObj = arrayPoints[z].x + 4;
		}
	}

	// ============================ CLOSING TIME IN THE FUNCTION =============================
	delete[] arrayPoints; // release memory before returning (with or without a value).
	return;
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

void clearFlags(vector< vector<int> > & imgPxlsFlags , Mat image)
{
	for(int j=0; j < image.size().height; j++)
		for(int i=0; i < image.size().width; i++)
			imgPxlsFlags[j][i] = 0; 
}


void mapContours(Mat imgWithContours, POINT arrayPoints[], int numPoints)
{
	// map the white contour pixels of the given white object onto the copy:

	for(int k=0; k < numPoints; k++)
	{
		// not friendly to "imgWithContours.ptr(arrayPoints[k].y)[arrayPoints[k].x] = 255;"
		int x = arrayPoints[k].x;
		int y = arrayPoints[k].y;
		imgWithContours.ptr(y)[x] = 255;
	}
}

