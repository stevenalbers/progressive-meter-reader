#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

using namespace cv;
using namespace std;

// =================================================================================
// =======================     CONSTANTS - misc. vars:     =========================
// =================================================================================

int allowedConf = 2;
Mat imgDil;
bool reachedEndOfRgn = 0;
int currJVal = 0;
int numOfDilErdSteps = 0;
std::string str;
int rightOfChar = 0;
int rightOfJVal = 0;
int gameChoice = 0; // no game chosen
int threshVal = 0; // lowest color value allowed for the chars
const int charsToClassify = 11; // 0-9 digits + $ sign
RNG rng(12345);

// =================================================================================
// =======================      CONSTANTS - moments:      ==========================
// =================================================================================

// temp arrays placed here because of an issue with passing its pointers:
const int numCentralMoments = 4; 
float centralMoments[numCentralMoments];

const int numFeatureMoments = 3;
// should be manually changed to quantity = numCentralMoments-1 since it's a const.

float featureMoments[numFeatureMoments];
float moment1 = 0;

// =================================================================================
// =======================       CONSTANTS - points:       =========================
// =================================================================================

struct POINT {
	float x, y;
};

struct JVALBOUNDS {
	POINT topLeft, bottomRight;
};

const int numJVals = 5;
JVALBOUNDS jValsCoords[numJVals]; // assuming 5 jackpot values per game

POINT ctrd; // for centroidal profiling

// =================================================================================
// ======================       FUNCTION PROTOTYPES:       =========================
// =================================================================================

void train(std::string , float [][charsToClassify], char );
void Sequential_moments(int , POINT * );
void Signature(int , POINT * , float * );
void centroid(int , POINT * , float * , float * );
float f_moment(float * , int , int );
float f_central_moment(float * , int , int , float );
int computeNumContourPts(Mat , vector< vector<int> > & , bool );
void writeContourCoords(Mat , POINT arrayPoints[], int , vector< vector<int> > & , bool );
bool isInBounds(Mat imageForBounds, int , int );
void detectNextAndMatch(Mat , Mat , float [][charsToClassify]);
//void threshNick(string );
void clearFlags(vector< vector<int> > & , Mat);
void mapContours(Mat , POINT arrayPoints[], int);
void locateJValue( int, int, int, int, int );
void prepareDtxnVars( int );
void formatCurrJVal();
void mainMenu();
void cleanImageCopy( Mat );

// =================================================================================================================
//														MAIN()
// =================================================================================================================

int main()
{
	Mat imageGray; // for greyscale and thresholding
	Mat imgWithContours; // for the image with white object contours
	// an array to hold moments for every char class:
	float fDatabase [numFeatureMoments][charsToClassify]; // moments database
	ofstream toFile;
	str = "";
	toFile.open("output.txt");
	
	// ============  CHOOSING THE GAME  ============
	mainMenu();
	if(gameChoice == 0) return 0; // exit

	// =========================================  CASHBURST:  =============================================
	else if (gameChoice == 1) 
	{
		numOfDilErdSteps = 1;
		threshVal = 50;
	}

	// ==========================  INSTANT RICHES (LED-style font) - 1st jackpot value:  ==============================
	else if (gameChoice == 2) 
	{
		numOfDilErdSteps = 1;
		threshVal = 40;
	}
	
	// =====================================  PREPROCESSING - GREYSCALE AND THRESHOLDING  ======================================

	// greyscale:
	Mat image = imread("../src/in.jpg", 1); // converts image from one color space to another
	cvtColor(image, imageGray, COLOR_BGR2GRAY );
	imshow("greyscale", imageGray);
	imwrite( "../src/outA-greyscale.png" , imageGray); // saves the chosen output image
	
	// thresholding:
	threshold(imageGray, imageGray, threshVal, 255, THRESH_BINARY); // arg3 = threshold
	imshow("thresh", imageGray);
	imwrite( "../src/outB-thresh.png" , imageGray); // saves the chosen output image

	// ============================  PREPARATIONS for MAPPING CONTOURS  =============================
	
	cout << "\nNumber of feature moments: " << numFeatureMoments << "\n\n"; // recognition based on Euclidean distances starts here
	// create a blank copy of the same size as the testing image for MAPPING CONTOURS:
	threshold(imageGray, imgWithContours, threshVal, 255, THRESH_BINARY); // threshold() used to copy one image into another
	// clean the copy to make sure it's blank:
	cleanImageCopy( imgWithContours );

	// =========================================  TESTING - DETECTION  ============================================
	
	/*
	// shows the dilated version of thresh image (2 steps for j0, 1 step for j1-4):
	dilate(imageGray, imageGray, Mat(), Point(-1,-1), 2); // use only for LED-style fonts
	imshow("extraDilErd", imageGray);
	imwrite( "../src/outC-extraDilErd.png" , imageGray); // saves the chosen output image
	*/

	// 1. training; 2. testing: a) preparation, b) detection w/ mapping c) displaying recognition result.

	for ( currJVal=0; currJVal < numJVals; currJVal++)
	{
		// =================  TRAINING (FIRST jackpot value) - go thru every char to classify:  ===================

		if(gameChoice == 1) // if cashburst, ... .
		{
			// ===============================  LOCATE and write JACKPOT VALUES  ===================================
			// format: ( int jValID, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY )
			// high resolution:
			locateJValue( 0, 90, 40, 548, 132 );
			locateJValue( 1, 77, 182, 305, 247 );
			locateJValue( 2, 331, 182, 561, 247 );
			locateJValue( 3, 77, 276, 305, 341 );
			locateJValue( 4, 331, 276, 561, 341 );

			if( currJVal == 0 ) // for j0 (assuming j0 is unique)
			{
				// ========= go thru every char to classify =========
				cout << endl << endl << endl;
				train("../src/trainCashburstJ0/0.jpg", fDatabase, '0');
				train("../src/trainCashburstJ0/1.jpg", fDatabase, '1');
				train("../src/trainCashburstJ0/2.jpg", fDatabase, '2');
				train("../src/trainCashburstJ0/3.jpg", fDatabase, '3');
				train("../src/trainCashburstJ0/4.jpg", fDatabase, '4');
				train("../src/trainCashburstJ0/5.jpg", fDatabase, '5');
				train("../src/trainCashburstJ0/6.jpg", fDatabase, '6');
				train("../src/trainCashburstJ0/7.jpg", fDatabase, '7');
				train("../src/trainCashburstJ0/8.jpg", fDatabase, '8');
				train("../src/trainCashburstJ0/9.jpg", fDatabase, '9');
				train("../src/trainCashburstJ0/dollar.jpg", fDatabase, '$');
			}
		
			// once and for j1-4, train on another set (assuming the last 4 are identical):
			if( currJVal == 1 ) 
			{
				cout << endl << endl << endl;
				train("../src/trainCashburstJ14/0.jpg", fDatabase, '0');
				train("../src/trainCashburstJ14/1.jpg", fDatabase, '1');
				train("../src/trainCashburstJ14/2.jpg", fDatabase, '2');
				train("../src/trainCashburstJ14/3.jpg", fDatabase, '3');
				train("../src/trainCashburstJ14/4.jpg", fDatabase, '4');
				train("../src/trainCashburstJ14/5.jpg", fDatabase, '5');
				train("../src/trainCashburstJ14/6.jpg", fDatabase, '6');
				train("../src/trainCashburstJ14/7.jpg", fDatabase, '7');
				train("../src/trainCashburstJ14/8.jpg", fDatabase, '8');
				train("../src/trainCashburstJ14/9.jpg", fDatabase, '9');
				train("../src/trainCashburstJ14/dollar.jpg", fDatabase, '$');
			}	
		}

		if(gameChoice == 2) // if InstantRiches, ... .
		{
			locateJValue( 0, 388, 319, 828, 427 );
			locateJValue( 1, 205, 485, 468, 562 );
			locateJValue( 2, 690, 485, 954, 560 );
			locateJValue( 3, 204, 607, 468, 684 );
			locateJValue( 4, 689, 608, 954, 685 );

			if( currJVal == 0 ) // for j0 (assuming j0 is unique)
			{
				numOfDilErdSteps = 2;
				cout << endl << endl << endl;
				train("../src/trainInstRichJ0/0.jpg", fDatabase, '0');
				train("../src/trainInstRichJ0/1.jpg", fDatabase, '1');
				train("../src/trainInstRichJ0/2.jpg", fDatabase, '2');
				train("../src/trainInstRichJ0/3.jpg", fDatabase, '3');
				train("../src/trainInstRichJ0/4.jpg", fDatabase, '4');
				train("../src/trainInstRichJ0/5.jpg", fDatabase, '5');
				train("../src/trainInstRichJ0/6.jpg", fDatabase, '6');
				train("../src/trainInstRichJ0/7.jpg", fDatabase, '7');
				train("../src/trainInstRichJ0/8.jpg", fDatabase, '8');
				train("../src/trainInstRichJ0/9.jpg", fDatabase, '9');
				train("../src/trainInstRichJ0/dollar.jpg", fDatabase, '$');
			}
		
			// once and for j1-4, train on another set (assuming the last 4 are identical):
			if( currJVal == 1 ) 
			{
				numOfDilErdSteps = 1;
				cout << endl << endl << endl;
				train("../src/trainInstRichJ14/0.jpg", fDatabase, '0');
				train("../src/trainInstRichJ14/1.jpg", fDatabase, '1');
				train("../src/trainInstRichJ14/2.jpg", fDatabase, '2');
				train("../src/trainInstRichJ14/3.jpg", fDatabase, '3');
				train("../src/trainInstRichJ14/4.jpg", fDatabase, '4');
				train("../src/trainInstRichJ14/5.jpg", fDatabase, '5');
				train("../src/trainInstRichJ14/6.jpg", fDatabase, '6');
				train("../src/trainInstRichJ14/7.jpg", fDatabase, '7');
				train("../src/trainInstRichJ14/8.jpg", fDatabase, '8');
				train("../src/trainInstRichJ14/9.jpg", fDatabase, '9');
				train("../src/trainInstRichJ14/dollar.jpg", fDatabase, '$');
			}	
		}

		// jackpot value 0:
		prepareDtxnVars( currJVal ); // set up boundaries FOR THE GIVEN REGION to perform char-by-char detection

		while(!reachedEndOfRgn)
		{
			detectNextAndMatch(imageGray, imgWithContours, fDatabase);
			// Including jValID as an arg would complicate the args list of the function
			// when it is shared by both training and testing phases.
		}

		cout << "Recognized text for J" << currJVal << ": ";
		formatCurrJVal();
		cout << endl << endl << endl << endl;
		str = ""; // clear the string for the next jackpot value.
		reachedEndOfRgn = 0; // reset for the next jackpot value.
	}

	// ================================  display IMAGES WITH CHANGES step-by-step:  =====================================


	// !!!!!!!!!!!!! include 1 image copy (x1 dil) from detectNextAndMatch() !!!!!!!!!!!!!!!!!
	
	// after dilation x1:
	imshow("dil", imgDil);
	imwrite( "../src/outX-dil.png" , imgDil); // saves the chosen output image

	//shows the PROCESSED version of thresh image (dil x1, erd x1):
	imshow("dilErd", imageGray);
	imwrite( "../src/outY-dilErd.png" , imageGray); // saves the chosen output image

	// for LED:
	//if (gameChoice == 2) dilate(imgWithContours, imgWithContours, Mat(), Point(-1,-1), 1); 

	//shows the image with CONTOURS:
	imshow("contours", imgWithContours);
	imwrite( "../src/outZ-contours.png" , imgWithContours); // saves the chosen output image

	// ====================== closing time - display recognition result: ========================
	cout << "\n\nDone.\n";
	toFile.close();
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
	threshold(imageGrayClassify, imageGrayClassify, threshVal, 255, THRESH_BINARY);

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
	for (int i=0; i <= numFeatureMoments-1; i++)
	{
		cout << "Feature " << i+1 << ": " << featureMoments[i] << "\n";
		// digits:
		if ( whichChar >= 48 && whichChar <= 57 )
			fDatabase[i][ int(whichChar) - 48 ] = featureMoments[i];
		// dollar sign:
		else fDatabase[i][ charsToClassify - 1 ] = featureMoments[i];
	}
	cout << endl;

	// ------------------------------------------------------------------------
	/*
	cout << endl;
	if (whichChar == '9')
	{
		for (int i=0; i < numPoints-1; i++)
		{
			cout << pow( pow(abs(arrayPoints[i].x - ctrd.x),2.0) + pow(abs(arrayPoints[i].y - ctrd.y),2.0) , 0.5) << endl;
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

	for(i=0; i < numCentralMoments; i++)
		centralMoments[i]=0.0;

	// need to determine min_length (min number of contour points)
	// to determine whether the white region is a good candidate
	// for consideration as a character in our database:

	if(no_points > 0) // 0 = min_length 
	{ 
		S = new float[no_points];
		Signature(no_points, P, S);
		M = new float[numCentralMoments+1];
		M[0] = f_moment(S, no_points, 1);
		moment1 = M[0];

		for(i=1; i < numCentralMoments+1; i++)
			M[i] = f_central_moment(S, no_points, i+1, M[0]);

		//normalization:
		centralMoments[0]=(float)sqrt((double)M[1])/M[0];
		for(i=1; i < numCentralMoments; i++)
			centralMoments[i]=M[i+1]/(float)sqrt(pow((double)M[1],(double)(i+2)));
		
		// ============= compute the feature vectore for this char: ==============

		// Note: features[3] = F4 = M'5 = M5/(M2^(5/2))
		for(int j=0; j <= numFeatureMoments-1; j++)
		{
			if(j==0) featureMoments[j] = pow( abs(centralMoments[1]), 0.5 ) / (moment1);
			else
			{
				featureMoments[j] = (centralMoments[j+1]) / pow( abs(centralMoments[1]), 1.0+(j*0.5) );
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

  ctrd.x = *cgx;
  ctrd.y = *cgy;
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
		i = rightOfChar; // horizontal component to start with
		j = ( (jValsCoords[currJVal].bottomRight.y-jValsCoords[currJVal].topLeft.y)/2 ) + jValsCoords[currJVal].topLeft.y;
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


	// ======= to prevent from getting stuck at white noise pixels of contour: =======
	//dilation x? (increases workload):
	dilate(imgGrayComp, imgGrayComp, Mat(), Point(-1,-1), numOfDilErdSteps); // recommended # of steps: ?

	// needed for showing the post-dilation image:
	threshold(imgGrayComp, imgDil, threshVal, 255, THRESH_BINARY); // threshold() used to copy one image into another

	//erosion x? (decreases workload):
	erode(imgGrayComp, imgGrayComp, Mat(), Point(-1,-1), numOfDilErdSteps); // recommended # of steps: ?

	// ===============================================================================

	// find the 1st (next) char in the image:
	while( imgGrayComp.ptr(j)[i] == 0 )
	{
		i++;
		//cout << "\ntick 0\n";

		if(testFlag)
		{
			//cout << "\ntick 1\n";
			if( i >= rightOfJVal ) 
			{
				//cout << "\n end of region 1 \n";
				reachedEndOfRgn = 1;
				return 0;
			}
			//else cout << "\ntick 2\n";
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
		i = rightOfChar; // horizontal component to start with
		j = ( (jValsCoords[currJVal].bottomRight.y-jValsCoords[currJVal].topLeft.y)/2 ) + jValsCoords[currJVal].topLeft.y;
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

	// go through the image left-to-right; find the char:
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

void detectNextAndMatch(Mat imgDtct, Mat imgWithContours, float fDatabase[][charsToClassify])
{
	//cout << "Analyzing contour " << whichChar << "\n";

	POINT * arrayPoints; // for dynamic allocation
	float minEuclDist = 9999998;
	int minEuclDistInd = 0;
	//
	float minEuclDist2 = 999999999;
	float confusionDifferential= 0;
	//
	int conf = 999999999;
	//
	vector< vector<int> > imgPxlsFlags (imgDtct.size().height, vector<int>(imgDtct.size().width));
	clearFlags(imgPxlsFlags, imgDtct); // clear after allocation
	// ===============================================================================

	// ======= time to find the moments for the char: =======
	// pre-compute no_points(size)(N):
	int numPoints = computeNumContourPts(imgDtct, imgPxlsFlags, 1);
	if (reachedEndOfRgn)
	{
		//cout << "\n end of region 2 \n";
		return; // get out; done with the region
	}
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
		for(int f=0; f <= numFeatureMoments-1; f++)
		{
			squareEuclDist += pow(featureMoments[f] - fDatabase[f][i],2);
		}
		float euclDist = pow( squareEuclDist, 0.5 );

		// print Euclidean distance:
		if( i <= charsToClassify-2 ) // 11-2=9 => 0-9
			cout << "Eucl. dist. from " << i;
		else cout << "Eucl. dist. from $";
		cout << ": " << euclDist << endl;

		//if(conf > minEuclDist - euclDist) conf = minEuclDist - euclDist;
		// update the minimum Euclidean distance:
		if (euclDist < minEuclDist)
		{
			conf = minEuclDist - euclDist;
			minEuclDist2 = minEuclDist;
			minEuclDist = euclDist; // make the 1st smallest
			minEuclDistInd = i; // record the index of char with the smallest eucl. dist.
		}
	}

	// ============================	code for output for a centroidal profile ===============================
	/*
	cout << endl;
	for (int i=0; i < numPoints-1; i++)
	{
		cout << pow( pow(abs(arrayPoints[i].x - ctrd.x),2.0) + pow(abs(arrayPoints[i].y - ctrd.y),2.0) , 0.5) << endl;
	}
	*/
	
	// =====================================  EUCLIDEAN DISTANCES OUTPUT  ========================================

	// print the recognized character:
	cout << "  =  Min. eucl. dist.: " << minEuclDist << endl << endl;
	//cout << "2nd Min. eucl. dist.: " << minEuclDist2 << endl;
	//cout << "Confusion differential (if correct: low - OK, high - very good): " << minEuclDist2-minEuclDist << endl << endl;

	if ( minEuclDistInd >= 0 && minEuclDistInd <= (charsToClassify-2) )
	{
		if(conf < allowedConf) str.append(" ");
		else
		{
			string regObj;
			stringstream converter;
			converter << minEuclDistInd;
			regObj = converter.str();
			str.append(regObj);
		}
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
		if(rightOfChar < arrayPoints[z].x)
		{
			// Displacement - get off the current set of connected components
			// before you proceed to the next one:
			if(gameChoice == 1 && currJVal==0) rightOfChar = arrayPoints[z].x + 4; // cashburst, j0
			if(gameChoice == 1 && currJVal!=0) rightOfChar = arrayPoints[z].x + 8; // cashburst, j1-4
			if(gameChoice == 2) rightOfChar = arrayPoints[z].x + 2; // instant riches, j0
		}
	}
	/*
	{
	while( imgWithContours.ptr(y)[x] == 255 )
		imgWithContours.ptr(y)[x] = 255;
		rightOfChar = arrayPoints[z].x;
		while() rightOfChar++;
	}
	*/

	// ============================ CLOSING TIME IN THE FUNCTION =============================
	delete[] arrayPoints; // release memory before returning.
	return;
}

// ==================================================================================
// void clearFlags(vector< vector<int> > & imgPxlsFlags , Mat image)
// 
//  - Used to prevent infinite looping during contour analysis.
// ==================================================================================

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

void locateJValue( int jValID, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY )
{
	// top left:
	jValsCoords[jValID].topLeft.x = topLeftX;
	jValsCoords[jValID].topLeft.y = topLeftY;
	// bottom right:
	jValsCoords[jValID].bottomRight.x = bottomRightX;
	jValsCoords[jValID].bottomRight.y = bottomRightY;
}


void prepareDtxnVars( int jValID )
{
	// end of region
	rightOfJVal = jValsCoords[jValID].bottomRight.x;
	// start at horizontal displacement:
	rightOfChar = jValsCoords[jValID].topLeft.x;
}

// ======================================================================================
// void formatCurrJVal()
// 
//  - prints the jackpot value on the fly and places commas and a dot where they belong.
// ======================================================================================

void formatCurrJVal()
{
	for(int s=0 ; s < str.length() ; s++) // format the jackpot value here.
	{
		cout << str.at(s); // copy the char to the right-to-left string
		if(s == str.length()-3) cout << "."; // 3rd char from right - decimal point
		else if( (str.length()-s)%3==0 && str.at(s)!='$' ) cout << ","; // 3rd char from right - decimal point
	}
}

void mainMenu()
{
	cout << "\nPlease enter the ID of the game you will train and test on: \n";
	cout << "0 - quit \n";
	cout << "1 - CashBurst \n";
	cout << "2 - InstantRiches \n";
	cout << "\nID: ";
	cin >> gameChoice;
	cout << endl;

	// gameChoice = 1; // Cashburst (ID needed for specifying training and testing images).
	while(gameChoice < 0 || gameChoice > 2)
	{
		cout << "Please try again. \n";
		cout << "ID: ";
		cin >> gameChoice;
	}
}

void cleanImageCopy( Mat imgWithContours )
{	
	for(int j=0; j < imgWithContours.size().height; j++)
		for(int i=0; i < imgWithContours.size().width; i++)
			imgWithContours.ptr(j)[i] = 0;
}