#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>      // std::istringstream, std::ws
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

using namespace cv;
using namespace std;

// =================================================================================
// =======================     CONSTANTS - misc. vars:     =========================
// =================================================================================

const int jRgnInDataSize = 6,
	numOfCandidates = 2,
	charsToClassify = 11, /* 0-9 digits + $ sign */
	numJVals = 5;

int outDisplt = 20,
	arrI[numOfCandidates-1], /* array of indices, one per eucl. dist. */
	vertMiddle=0,
	allowedConf = 2,
	currJVal = 0,
	rightOfChar = 0,
	rightOfJVal = 0,
	gameChoice = 0, /* no game chosen */
	threshVal = 0; // lowest color value allowed for the chars

Mat imgDil;
string str;
bool reachedEndOfRgn = 0;
float arrDist[numOfCandidates-1];
RNG rng(12345);

// =================================================================================
// =======================      CONSTANTS - moments:      ==========================
// =================================================================================

// temp arrays placed here because of an issue with passing its pointers:
const int numCentralMoments = 4; // recommended: 4
float centralMoments[numCentralMoments];

const int numFeatureMoments = 3; // recommended: 4-1=3
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
	int numDilErds;
	int specThresh; // will depend on file I/O or Otsu's method.
	string text;
};

JVALBOUNDS jValRgns[numJVals]; // assuming 5 jackpot values per game

POINT ctrd; // for centroidal profiling

// =================================================================================
// ======================       FUNCTION PROTOTYPES:       =========================
// =================================================================================

void train(ofstream & , string , float [][charsToClassify], char );
void Sequential_moments(ofstream & , int , POINT * );
void Signature(ofstream & , int , POINT * , float * );
void centroid(int , POINT * , float * , float * );
float f_moment(float * , int , int );
float f_central_moment(float * , int , int , float );
int computeNumContourPts(Mat , vector< vector<int> > & , bool );
void writeContourCoords(Mat , POINT arrayPoints[], int , vector< vector<int> > & , bool );
bool isInBounds(Mat imageForBounds, int , int );
void detectNextAndMatch(ofstream & toDbgFile, Mat , Mat , float [][charsToClassify]);
void clearFlags(vector< vector<int> > & , Mat);
void mapContours(Mat , POINT arrayPoints[], int);
void locateJValue( int, int, int, int, int, int, string );
void prepareDtxnVars( int );
void formatCurrJVal(ofstream & , ofstream & , int);
void mainMenu();
void cleanImageCopy( Mat );
string chooseFontSizeChar(string , int , char & );
void storeJRgnInData(ifstream & , int [], string );
void otsuThresh(Mat );

// =================================================================================================================
//														MAIN()
// =================================================================================================================

int main()
{
	cout << "Initializing core variables" << endl;

	Mat image;
	Mat imageGray; // for greyscale
	Mat imgThreshDilErd; // for thresholding
	Mat imgTest; // new for every j-value
	Mat imgWithContours; // for the image with white object contours
	// an array to hold moments for every char class:
	float fDatabase [numFeatureMoments][charsToClassify]; // moments database
	ifstream fromInputFile;
	ofstream toDbgFile;
	toDbgFile.open("../src/DebugOut.txt");
	ofstream toUsrOutFile;
	toUsrOutFile.open("../src/Output.txt");
	str = "";
	toUsrOutFile << "Region Name|Meter Value\n";
	
	// =====================================  CHOOSING THE GAME:  =========================================
	mainMenu();
	if(gameChoice == 0) return 0; // exit

	// =========================================  CASHBURST:  =============================================
	else if (gameChoice == 1) 
	{
		threshVal = 50;
		fromInputFile.open("../src/CB.txt");
	}

	// ==========================  INSTANT RICHES (LED-style font) - 1st jackpot value:  ==============================
	else if (gameChoice == 2) 
	{
		threshVal = 40;
		fromInputFile.open("../src/IR.txt");
	}

	// =====================================  PREPROCESSING - GREYSCALE AND THRESHOLDING  ======================================

	// greyscale:
	image = imread("../src/in.jpg", 1); // converts image from one color space to another
	cvtColor(image, imageGray, COLOR_BGR2GRAY );
	imwrite( "../src/outA-greyscale.png" , imageGray); // saves the chosen output image

	// thresholing the openCV way - with a hand-picked threshold value:
	threshold(imageGray, imgThreshDilErd, threshVal, 255, THRESH_BINARY); // arg3 = threshold
	imwrite( "../src/outB-thresh.png" , imgThreshDilErd); // saves the chosen output image
	// We can also use Otsu's method.
	
	// ============================  PREPARATIONS for MAPPING CONTOURS  =============================
	toDbgFile << "\nNumber of feature moments: " << numFeatureMoments << "\n\n"; // recognition based on Euclidean distances starts here
	// create a blank copy of the same size as the testing image for MAPPING CONTOURS:
	threshold(imgThreshDilErd, imgWithContours, threshVal, 255, THRESH_BINARY); // threshold() used to copy one image into another
	// clean the copy to make sure it's blank:
	cleanImageCopy( imgWithContours );

	// =========================================  TESTING - DETECTION  ============================================
	// For every j-value:
	//  - 1. training; 2. testing: a) preparation, b) detection w/ mapping c) displaying recognition result.

	for ( currJVal=0; currJVal < numJVals; currJVal++)
	{
		// =================  TRAINING (FIRST jackpot value) - go thru every char to classify:  ===================

		if(gameChoice == 1) // if cashburst, ... .
		{
			// =================================== TEXT FILE INPUT =====================================

			int jRgnInData [jRgnInDataSize];	// jID, topLeft.x, topLeft.y, btmRt.x, btmRt.y, numDilSteps, "Gem text"
			string jRgnInStr;

			storeJRgnInData(fromInputFile, jRgnInData, jRgnInStr);

			if( currJVal == 0 ) // for j0 (assuming j0 is unique)
			{
				// ========= go thru every char to classify =========
				toDbgFile << endl << endl << endl;
				for(int i=0; i < charsToClassify; i++)
				{
					char charASCII=0;
					string font = "../src/trainCashburstJ0/";
					string trainPath = chooseFontSizeChar(font, i, charASCII);
					train(toDbgFile, trainPath, fDatabase, charASCII); // jValRegs, currJVal, numDilErds -> train
				}
			}
		
			// once and for j1-4, train on another set (assuming the last 4 are identical):
			if( currJVal == 1 ) 
			{
				toDbgFile << endl << endl << endl;
				for(int i=0; i < charsToClassify; i++)
				{
					char charASCII=0;
					string font = "../src/trainCashburstJ1/";
					string trainPath = chooseFontSizeChar(font, i, charASCII);
					train(toDbgFile, trainPath, fDatabase, charASCII);
				}
			}
			
			// once and for j1-4, train on another set (assuming the last 4 are identical):
			if( currJVal == 2 ) 
			{
				toDbgFile << endl << endl << endl;
				for(int i=0; i < charsToClassify; i++)
				{
					char charASCII=0;
					string font = "../src/trainCashburstJ2/";
					string trainPath = chooseFontSizeChar(font, i, charASCII);
					train(toDbgFile, trainPath, fDatabase, charASCII);
				}
			}
			
			// once and for j1-4, train on another set (assuming the last 4 are identical):
			if( currJVal == 3 ) 
			{
				toDbgFile << endl << endl << endl;
				for(int i=0; i < charsToClassify; i++)
				{
					char charASCII=0;
					string font = "../src/trainCashburstJ3/";
					string trainPath = chooseFontSizeChar(font, i, charASCII);
					train(toDbgFile, trainPath, fDatabase, charASCII);
				}
			}
			
			// once and for j1-4, train on another set (assuming the last 4 are identical):
			if( currJVal == 4 ) 
			{
				toDbgFile << endl << endl << endl;
				for(int i=0; i < charsToClassify; i++)
				{
					char charASCII=0;
					string font = "../src/trainCashburstJ4/";
					string trainPath = chooseFontSizeChar(font, i, charASCII);
					train(toDbgFile, trainPath, fDatabase, charASCII);
				}
			}
		}

		if(gameChoice == 2) // if InstantRiches, ... .
		{
			// =================================== TEXT FILE INPUT =====================================	
			int jRgnInData [jRgnInDataSize];	// jID, topLeft.x, topLeft.y, btmRt.x, btmRt.y, numDilSteps, "Gem text"
			string jRgnInStr;

			storeJRgnInData(fromInputFile, jRgnInData, jRgnInStr);

			if( currJVal == 0 ) // for j0 (assuming j0 is unique)
			{
				toDbgFile << endl << endl << endl;
				for(int i=0; i < charsToClassify; i++)
				{
					char charASCII=0;
					string font = "../src/trainInstRichJ0/";
					string trainPath = chooseFontSizeChar(font, i, charASCII);
					train(toDbgFile, trainPath, fDatabase, charASCII);
				}
			}
		
			// once and for j1-4, train on another set (assuming the last 4 are identical):
			if( currJVal == 1 ) 
			{
				toDbgFile << endl << endl << endl;
				for(int i=0; i < charsToClassify; i++)
				{
					char charASCII=0;
					string font = "../src/trainInstRichJ1/";
					string trainPath = chooseFontSizeChar(font, i, charASCII);
					train(toDbgFile, trainPath, fDatabase, charASCII);
				}
			}

			
			if( currJVal == 2 ) 
			{
				toDbgFile << endl << endl << endl;
				for(int i=0; i < charsToClassify; i++)
				{
					char charASCII=0;
					string font = "../src/trainInstRichJ2/";
					string trainPath = chooseFontSizeChar(font, i, charASCII);
					train(toDbgFile, trainPath, fDatabase, charASCII);
				}
			}

			
			if( currJVal == 3 ) 
			{
				toDbgFile << endl << endl << endl;
				for(int i=0; i < charsToClassify; i++)
				{
					char charASCII=0;
					string font = "../src/trainInstRichJ3/";
					string trainPath = chooseFontSizeChar(font, i, charASCII);
					train(toDbgFile, trainPath, fDatabase, charASCII);
				}
			}

			
			if( currJVal == 4 ) 
			{
				toDbgFile << endl << endl << endl;
				for(int i=0; i < charsToClassify; i++)
				{
					char charASCII=0;
					string font = "../src/trainInstRichJ4/";
					string trainPath = chooseFontSizeChar(font, i, charASCII);
					train(toDbgFile, trainPath, fDatabase, charASCII);
				}
			}
		}

		// jackpot value 0:
		prepareDtxnVars( currJVal ); // set up boundaries FOR THE GIVEN REGION to perform char-by-char detection

		while(!reachedEndOfRgn)
		{
			threshold(imgThreshDilErd, imgTest, threshVal, 255, THRESH_BINARY);
			detectNextAndMatch(toDbgFile, imgTest, imgWithContours, fDatabase);
			// Including jValID as an arg would complicate the args list of the function
			// when it is shared by both training and testing phases.
		}

		cout << "\nRecognized text for J" << currJVal << ": ";
		toDbgFile << "\nRecognized text for J" << currJVal << ": ";
		formatCurrJVal(toDbgFile, toUsrOutFile, currJVal);
		toUsrOutFile << endl;
		str = ""; // clear the string for the next jackpot value.
		reachedEndOfRgn = 0; // reset for the next jackpot value.
	}

	// ================================  display IMAGES WITH CHANGES step-by-step:  =====================================

	// !!!!!!!!!!!!! include 1 image copy (x1 dil) from detectNextAndMatch() !!!!!!!!!!!!!!!!!
	
	// after dilation x1:
	imwrite( "../src/outX-dil.png" , imgDil); // saves the chosen output image
	//shows the PROCESSED version of thresh image (dil x1, erd x1):
	imwrite( "../src/outY-dilErd.png" , imgThreshDilErd); // saves the chosen output image
	//shows the image with CONTOURS:
	imwrite( "../src/outZ-contours.png" , imgWithContours); // saves the chosen output image
	
	// ====================== closing time - display recognition result: ========================
	cout << "\n\nDone.\n";
	toUsrOutFile.close();
	toDbgFile.close();
	fromInputFile.close();
	waitKey(0);
	cout << endl << endl;
	return 0;
}

// ===================================================================
// ===================================================================

void train(ofstream & toDbgFile, string imageFile, float fDatabase [][charsToClassify],
		char whichChar)
{
	toDbgFile << "Classifying " << whichChar << "\n";
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
	toDbgFile << "Num of points: " << numPoints << endl; 
	clearFlags(imgPxlsFlags, image); // clear after allocation
	arrayPoints = new POINT[numPoints]; // now that we know the "size"

	// write the coords of every point into the points-array:
	writeContourCoords(imageGrayClassify, arrayPoints, numPoints, imgPxlsFlags, 0);
	clearFlags(imgPxlsFlags, image); // clear after allocation

	// Needs:
	// no_points(size)(N) - pre-computed numbers of contour pixels &
	// P[]( (x',y') ) - an array of coordinates stored in it.
	Sequential_moments(toDbgFile, numPoints, arrayPoints);

	/* c2
	// output features for the given character:
	toDbgFile << "Char " << whichChar << " moments: \n";
	//
	for(int c=0; c<numCentralMoments; c++)
	{
		toDbgFile << "        Central " << c+1 << ": " << centralMoments[c] << "\n";
	}
	*/

	// writing features into the 2D-array ("database"):
	for (int i=0; i <= numFeatureMoments-1; i++)
	{
		// c2 toDbgFile << "Feature " << i+1 << ": " << featureMoments[i] << "\n";
		// digits:
		if ( whichChar >= 48 && whichChar <= 57 )
			fDatabase[i][ int(whichChar) - 48 ] = featureMoments[i];
		// dollar sign:
		else fDatabase[i][ charsToClassify - 1 ] = featureMoments[i];
	}
	toDbgFile << endl;

	// ------------------------------------------------------------------------
	/*
	toDbgFile << endl;
	if (whichChar == '9')
	{
		for (int i=0; i < numPoints-1; i++)
		{
			toDbgFile << pow( pow(abs(arrayPoints[i].x - ctrd.x),2.0) + pow(abs(arrayPoints[i].y - ctrd.y),2.0) , 0.5) << endl;
		}
	}
	*/
	// ------------------------------------------------------------------------

	delete[] arrayPoints;

	/* if( currJVal == 2 )
	{
		if(whichChar=='7') imshow("7", imageGrayClassify);
		else if(whichChar=='$') imshow("$", imageGrayClassify);
	} */
}

/*  ========================================================================
          Computation of the Sequential Moments of a contour
	            from L. Gupta and M. Srinath:
   "Contour Sequence Moments for the Classification of Closed Planar Shapes"
    Reference: Pattern Recognition, vol. 20, no. 3, pp. 267-272, 1987
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
//
void Sequential_moments(ofstream & toDbgFile, int no_points, POINT * P)
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
		Signature(toDbgFile, no_points, P, S);
		M = new float[numCentralMoments+1];
		M[0] = f_moment(S, no_points, 1);
		moment1 = M[0];

		for(i=1; i < numCentralMoments+1; i++)
			M[i] = f_central_moment(S, no_points, i+1, M[0]);

		//normalization:
		centralMoments[0]=(float)sqrt((double)M[1])/M[0];
		toDbgFile << "        Central 0: " << centralMoments[0] << "\n"; // c3
		for(i=1; i < numCentralMoments; i++)
		{
			centralMoments[i]=M[i+1]/(float)sqrt(pow((double)M[1],(double)(i+2)));
			toDbgFile << "        Central " << i << ": " << centralMoments[i] << "\n"; // c3
		}
		
		// ============= compute the feature vectore for this char: ==============

		// Note: features[3] = F4 = M'5 = M5/(M2^(5/2))
		for(int j=0; j <= numFeatureMoments-1; j++)
		{
			if(j==0)
			{
				featureMoments[j] = pow( abs(centralMoments[1]), 0.5 ) / (moment1);
				toDbgFile << "Feature 0: " << featureMoments[j] << "\n"; // c3
			}
			else
			{
				featureMoments[j] = (centralMoments[j+1]) / pow( abs(centralMoments[1]), 1.0+(j*0.5) );
				toDbgFile << "Feature " << j << ": " << featureMoments[j] << "\n"; // c3
			}
		}
		
		delete[] S;
		delete[] M;
	}
}

/*
// ===================================================================

Measures distance between the centroid and each one of the contour points.

// Needs:
// no_points(size)(N) - pre-computed numbers of contour pixels &
// P[]( (x',y') ) - an array of coordinates stored in it.
*/
void Signature(ofstream & toDbgFile, int no_points, POINT * P, float * S)
{
  int i;
  float cgx, cgy;

  centroid(no_points, P, &cgx, &cgy);
  //toDbgFile << "centroid = " << cgx << " " << cgy << endl;

  toDbgFile << "-------------------------------------\n";
  for(i=0; i < no_points; i++)
  {
	S[i]=(float)hypot((double)(cgx-P[i].x),(double)(cgy-P[i].y));
	if(i<=10) toDbgFile << S[i] << endl;
  }
  toDbgFile << endl << endl;
  toDbgFile << "-------------------------------------\n";
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
	for(i=0; i<n; i++) sum += (float)pow((double)(S[i]-m1),(double)r);
	sum /= (float) n;

	return(sum);
}

// ================================================================

// computes the number of contour pixels of a white region:
//
int computeNumContourPts(Mat imgDtct, vector< vector<int> > & imgPxlsFlags, bool testFlag)
{
	//cout << "Computing the number of contour points for the current white object" << endl;

	// must be ints for processing the image:
	int i=0, j=imgDtct.size().height/2; // default settings for training; j - y, i - x ; 0, imgDtct.size().height/2


	if (testFlag) 
	{
		i = rightOfChar; // horizontal component to start with
		j = ( (jValRgns[currJVal].bottomRight.y-jValRgns[currJVal].topLeft.y)/2 ) + jValRgns[currJVal].topLeft.y;
		vertMiddle = j;
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
	dilate(imgDtct, imgDtct, Mat(), Point(-1,-1), jValRgns[currJVal].numDilErds ); // recommended # of steps: ?
	// needed for showing the post-dilation image:
	threshold(imgDtct, imgDil, threshVal, 255, THRESH_BINARY); // threshold() used to copy one image into another
	//erosion x? (decreases workload):
	erode(imgDtct, imgDtct, Mat(), Point(-1,-1), jValRgns[currJVal].numDilErds); // recommended # of steps: ?

	// ===============================================================================

	// find the 1st (next) char in the image:
	while( imgDtct.ptr(j)[i] == 0 )
	{
		i++;

		if(testFlag)
		{
			if( i >= rightOfJVal ) 
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
	while ( imgDtct.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 255 )
	{
		dir = (dir+1) % 8;
	}

	// find the first unflagged white neighbor: (why not ... == 0? - grey)
	while ( imgDtct.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] != 255 || imgPxlsFlags[ ziy+dirs[dir][1] ][ zix+dirs[dir][0] ]==1 )
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
			if ( isInBounds(imgDtct, ziy+dirs[dir][1], zix+dirs[dir][0]) )
			{
				if ( imgDtct.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 255)
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
			if ( !isInBounds(imgDtct, ziy+dirs[dir][1], zix+dirs[dir][0]) )
			{
				dir = (dir+1) % 8; // if out of bounds
			}
			else if (imgDtct.ptr(ziy+dirs[dir][1])[(zix+dirs[dir][0])] == 0 || imgPxlsFlags[ ziy+dirs[dir][1] ][ zix+dirs[dir][0] ]==1 )
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
	cout << "Writing contour coordinates to an array" << endl;

	// must be ints for processing image:
	int i=0, j=imgGrayWrite.size().height/2; // j - y, i - x ; 0, imgDtct.size().height/2


	if (testFlag) // 195x60 ; 969x158
	{
		i = rightOfChar; // horizontal component to start with
		j = ( (jValRgns[currJVal].bottomRight.y-jValRgns[currJVal].topLeft.y)/2 ) + jValRgns[currJVal].topLeft.y;
		vertMiddle = j;
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

void detectNextAndMatch(ofstream & toDbgFile, Mat imgDtct, Mat imgWithContours, float fDatabase[][charsToClassify])
{
	cout << "Detecting and matching a character" << endl;

	POINT * arrayPoints; // for dynamic allocation
	bool showInaccurate = 1;

	for (int i=0; i < numOfCandidates; i++)
	{
		arrDist[i] = 99999999;
		arrI[i] = 0;
	}
	//
	vector< vector<int> > imgPxlsFlags (imgDtct.size().height, vector<int>(imgDtct.size().width));
	clearFlags(imgPxlsFlags, imgDtct); // clear after allocation
	// ===============================================================================

	// ======= time to find the moments for the char: =======
	// pre-compute no_points(size)(N):
	int numPoints = computeNumContourPts(imgDtct, imgPxlsFlags, 1);
	toDbgFile << "Num of points: " << numPoints << endl; 
	if (reachedEndOfRgn)
	{
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
	Sequential_moments(toDbgFile, numPoints, arrayPoints);

	// given trained database with moments and the moments of the testing object in the image,
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
			toDbgFile << "Eucl. dist. from " << i;
		else toDbgFile << "Eucl. dist. from $";
		toDbgFile << ": " << euclDist << endl;

		// update the minimum Euclidean distance:
		if (euclDist < arrDist[0])
		{
			arrDist[1] = arrDist[0]; // make the 1st smallest
			arrI[1] = arrI[0];

			arrDist[0] = euclDist; // make the 1st smallest
			arrI[0] = i; // record the index of char with the smallest eucl. dist.
		}

		if (euclDist < arrDist[1] && euclDist > arrDist[0])
		{
			arrDist[1] = euclDist; // make the 1st smallest
			arrI[1] = i;
		}
	}

	// ============================	code for output for a centroidal profile ===============================
	/*
	toDbgFile << endl;
	for (int i=0; i < numPoints-1; i++)
	{
		toDbgFile << pow( pow(abs(arrayPoints[i].x - ctrd.x),2.0) + pow(abs(arrayPoints[i].y - ctrd.y),2.0) , 0.5) << endl;
	}
	*/
	
	// =====================================  EUCLIDEAN DISTANCES OUTPUT  ========================================

	/* c2 - central moments of testing $ 
	            match those of template 7.
	toDbgFile << "  =      Min. eucl. dist.: " << arrDist[0] << endl;
	toDbgFile << "  =  2nd Min. eucl. dist.: " << arrDist[1] << endl;
	
	for(int c=0; c<numCentralMoments; c++)
	{
		toDbgFile << "        Central " << c+1 << ": " << centralMoments[c] << "\n";
	}

	for(int f=0; f<numFeatureMoments; f++)
	{
		toDbgFile << "Feature " << f+1 << ": " << featureMoments[f] << "\n";
	}
	toDbgFile << endl << endl; */
	
	if ( arrI[0] >= 0 && arrI[0] <= (charsToClassify-2) ) // between 0-9 incl.
	{
		if(showInaccurate)
		{
			string rcgnChar;
			stringstream converter;
			converter << arrI[0]; 
			rcgnChar = converter.str();
			str.append(rcgnChar);
		}
		else if( abs(arrDist[1]-arrDist[0]) < allowedConf )
		{
			str.append(" ");
		}
		else
		{
			string rcgnChar;
			stringstream converter;
			converter << arrI[0]; 
			rcgnChar = converter.str();
			str.append(rcgnChar);
		}
	}
	else 
	{
		if(showInaccurate) str.append("$");
		else if( abs(arrDist[1]-arrDist[0]) < allowedConf )
		{
			str.append(" ");
		}
		else str.append("$");
	}

	// ====================== BEFORE WE DESTROY THE ARRAY AND LEAVE, =======================
	// ==================== MAP THE CONTOUR PIXELS AND HIGHLIGHT THEM ======================
	mapContours(imgWithContours, arrayPoints, numPoints);

	// ================ FIND THE RIGHTMOST PIXEL TO DETECT NEXT CHARACTER =================
	for(int z=0; z < numPoints; z++)
	{
		if(rightOfChar < arrayPoints[z].x)
		{
			rightOfChar = arrayPoints[z].x;
		}
	}

	// Displacement - get off the current set of connected components
	// before you proceed to the next one:
	rightOfChar++; // 1 pxl to the right.

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
	//cout << "Clearing visitation flags for all pixels" << endl;

	for(int j=0; j < image.size().height; j++)
		for(int i=0; i < image.size().width; i++)
			imgPxlsFlags[j][i] = 0; 
}

// ================================================================================
//		map the white contour pixels of the given white object onto the copy:
// ================================================================================

void mapContours(Mat imgWithContours, POINT arrayPoints[], int numPoints)
{
	cout << "Mapping contours onto a black image" << endl;

	for(int k=0; k < numPoints; k++)
	{
		// not friendly to "imgWithContours.ptr(arrayPoints[k].y)[arrayPoints[k].x] = 255;"
		int x = arrayPoints[k].x;
		int y = arrayPoints[k].y;
		imgWithContours.ptr(y)[x] = 255;
	}
}

void locateJValue( int jValID, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY, int numDilErds, string text )
{
	cout << "Defining coordinates for a jackpot value" << endl;
	// top left:
	jValRgns[jValID].topLeft.x = topLeftX;
	jValRgns[jValID].topLeft.y = topLeftY;
	// bottom right:
	jValRgns[jValID].bottomRight.x = bottomRightX;
	jValRgns[jValID].bottomRight.y = bottomRightY;
	jValRgns[jValID].numDilErds = numDilErds;
	jValRgns[jValID].text = text;
}

void prepareDtxnVars( int jValID )
{
	cout << "Assigning to white-object-detection tracking variables" << endl;
	// end of region
	rightOfJVal = jValRgns[jValID].bottomRight.x;
	// start at horizontal displacement:
	rightOfChar = jValRgns[jValID].topLeft.x;
}

// ======================================================================================
// void formatCurrJVal()
// 
//  - prints the jackpot value on the fly and places commas and a dot where they belong.
// ======================================================================================

void formatCurrJVal(ofstream & toDbgFile, ofstream & toUsrOutFile, int jVal)
{
	//cout << "Formatting the current jackpot value" << endl;
	toUsrOutFile << jValRgns[jVal].text << "|";
	
	toDbgFile << str; // copy the char to the right-to-left string
	toUsrOutFile << str; // copy the char to the right-to-left string
	cout << str;

	cout << endl << endl;
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
	cout << "Preparing a blank image for mapping contours" << endl;

	for(int j=0; j < imgWithContours.size().height; j++)
		for(int i=0; i < imgWithContours.size().width; i++)
			imgWithContours.ptr(j)[i] = 0;
}

string chooseFontSizeChar(string trainPath, int i, char & charASCII)
{
	if( i != charsToClassify-1 )
	{
		string strI = "";
		stringstream converter;
		converter << i;
		strI = converter.str();
		trainPath.append(strI);
		charASCII = 48+i;
	}
	else
	{
		trainPath.append("dollar");
		charASCII = '$';
	}

	trainPath.append(".jpg");

	return trainPath;
}

void storeJRgnInData(ifstream & fromInputFile, int jRgnInData [], string jRgnInStr)
{
	for(int a=0; a<jRgnInDataSize; a++)
	{
		fromInputFile >> jRgnInData[a];	// read int (id, tlx, tly, brx, bry, dilErds)
		fromInputFile >> jRgnInStr;		// skip a string
	}
	fromInputFile.ignore(1,' ');				// skip " "
	getline (fromInputFile, jRgnInStr, '\n');	// read text for j-value
	locateJValue( jRgnInData[0], jRgnInData[1], jRgnInData[2], 
		jRgnInData[3], jRgnInData[4], jRgnInData[5], jRgnInStr );	// pass to a j-value-element
}

void otsuThresh(Mat imageGray)
{
	//
}
