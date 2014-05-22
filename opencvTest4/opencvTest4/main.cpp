/*
 * Authors:		Andrey Gaganov - I/O, recognition (image processing), etc.
 *				George Bebis - recognition (algorithms)
 * 
 * Date:		1/1/2014 - 5/1/2014
 *
 * Update:		Problems resolved (see below)
 * 
 *	- variables are passed to functions instead of being used as 
 *	  global variables - resolved the problem of unintended change of values
 *	  in the variables.
 *	- Otsu's thresholding algorithm significantly improved recognition accuracy
 *	  (compared to global thresholding).
 *	- dilation/erosion proved to be ineffective in some cases;
 *	  custom noise reduction is mandatory.
 */

/*	Additional notes:
 *
 *	- Order of recognition: 
 *
 *		
 */


#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <math.h>

using namespace cv;
using namespace std;

// =================================================================================
// =======================     Misc. vars:     =========================
// =================================================================================

// ===================== Quick demo flags (global consts) : ==========================
const bool	// smoothening flag:
			enSmth = 0,
			// Otsu's thresholding method - has to be enabled for higher accuracy:
			enOtsu = 1,			
			// custom noise reduction; has to be enabled ! 
			// (helps out of infinite loops):
			enNoiseRdxn = 1,
			// write images to src folder for debugging purposes:
			writeImgs = 1,
			// debugging statements:
			dbgStts = 0,
			// demoMode - cashburst by default:
			demoMode = 1,
			// enables formatting value as currency (rejected):
			enFormat = 0;

const int jRgnInputDataSize = 6,
	numOfCandidates = 2,
	charsToClassify = 11, /* 0-9 digits + $ sign */
	numJVals = 5;

int outDisplt = 20,				// rejected indentation for the user output file.
	arrI[numOfCandidates-1],	// array of indices, one per eucl. dist.
	allowedConf = 2,
	currThreshVal = 0,			// lowest color value allowed for the chars
	pxNeiWinSize = 5,			// window size for every pixel's neighbors 
	gaussSigma = 1.5;

bool 
	// first char = $
	dollarSign = 0;


float arrDist[numOfCandidates-1];
RNG rng(12345);

// =================================================================================
// =======================      CONSTANTS - moments:      ==========================
// =================================================================================

// temp arrays placed here because of an issue with passing its pointers:
const int numCentralMoments = 4; // highly recommended: 4
float centralMoments[numCentralMoments];

const int numFeatureMoments = 3; // highly recommended: 4-1=3
// should be manually changed to quantity = numCentralMoments-1 since it's a const.
float featureMoments[numFeatureMoments];

// =================================================================================
// =======================       CONSTANTS - points:       =========================
// =================================================================================

struct IMGPOINT { int x, y; };

IMGPOINT dirs[8] = {	
	{-1,-1}, {0,-1}, { 1,-1}, { 1,0}, 
	{ 1, 1}, {0, 1}, {-1, 1}, {-1,0}	
};

struct ROIINFO {	
	IMGPOINT topLeft, bottomRight;
	int height, width, numDilErds;
	string text;					
};

IMGPOINT ctrd; // for centroidal profiling

// =================================================================================
// ======================       FUNCTION PROTOTYPES:       =========================
// =================================================================================

void train(ofstream & , ROIINFO [] , int , int , string , 
		   float [][charsToClassify], char , int & , bool , bool );
void Sequential_moments(ofstream & , int , IMGPOINT * );
void Signature(ofstream & , int , IMGPOINT * , float * );
void centroid(int , IMGPOINT * , float * , float * );
float f_moment(float * , int , int );
float f_central_moment(float * , int , int , float );
int countContPts(Mat , vector< vector<int> > & , bool , int & , bool & );
void wrtContCoords(Mat , IMGPOINT arrPts[], int , vector< vector<int> > & , bool , int & );
bool isInBounds(Mat img4Bnds, int , int );
void dtctAndMatch(ofstream & toDbgFile, int , Mat , Mat, 
						float [][charsToClassify], int & , string & , bool & );
void clearFlags(vector< vector<int> > & , Mat);
void mapContours(Mat , IMGPOINT arrPts[], int);
void locateJValue( int [] , string , ROIINFO [] );
void formatCurrJVal(ofstream & , ofstream & , int , string & , ROIINFO [] );
void mainMenu(int & );
string chooseFontSizeChar(string , int , char & );
void storeRgnInputData(ifstream & , int [], string , ROIINFO [] );
void drawHistogram( int , Mat );
Mat reduceImgNoise(Mat );
string insValIntoStr( string , int , string );
string insStrIntoStr( string , string , string );
void attachROI(Mat , int , Mat , ROIINFO [] );
void smoothen( Mat & , IMGPOINT [] , int );

/* =================================================================================================================
 *													MAIN()
 * 
 *	input parameters:
 *	 - image name (without an extension)
 * 
 * =================================================================================================================
 */

int main( int argc, char *argv[] )
{
	if( argc != 2 )
	{
		cout << "\n\nPlease enter a proper set of arguments\n\n";
		return -1;
	}

	cout << "\nInitializing/declaring core variables\n";

	ROIINFO jValRgns[numJVals]; // assuming 5 jackpot values per game

	Mat imgTestColor;
	Mat imgTestColorBin;
	Mat imgTestColorBinCont;
	Mat imgTestGrey; // for greyscale
	Mat imgTestOtsu; // for thresholding
	Mat imgDummy;
	Mat imgWithContours; // for the image with white object contours
	
	// ======================= Quick demo flags (vars) : ============================================
	bool	// Gaussian filtering helps with local ROI filtering:
			enGauss = 1,
			// allows for ending the seq. dtxn of chars in ROI:
			endOfRgn = 0;
	
	// jackpot ROI tracker:
	int currJID = 0, rightOfChar = 0, gameChoice = 0;
	// an array to hold moments for every char class:
	float fDatabase [numFeatureMoments][charsToClassify]; // moments database
	ifstream fromInputFile;
	ofstream toDbgFile;
	toDbgFile.open("C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/DebugOut.txt");
	ofstream toUsrOutFile;
	string jValStr = "";
	string origFontStr = "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/inputImages/";
	string imgName = "", imgNameWext = "", txtNameWext = "";
	
	imgName = argv[1];

	cout << endl << endl;
	//imgNameWext = insStrIntoStr( "../src/inputImages/" , imgName , ".jpg");
	imgNameWext = insStrIntoStr( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/inputImages/" , imgName , ".jpg");
	imgTestColor = imread( imgNameWext , 1);
	cout << endl << endl << imgNameWext << endl << endl;
	txtNameWext = insStrIntoStr( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/" , imgName , ".txt");
	toUsrOutFile.open( txtNameWext );
	toUsrOutFile << "Region Name|Meter Value\n";
	
	// =====================================  CHOOSING THE GAME:  =========================================
	if(demoMode) gameChoice = 1;
	else mainMenu(gameChoice);

	if(gameChoice == 0) return 0; // exit

	// =========================================  CASHBURST:  =============================================
	else if (gameChoice == 1) 
	{
		currThreshVal = 50; // in case of global thresholding
		fromInputFile.open("C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/inputData/CB.txt");
		origFontStr = "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/trainCashburstJ";
	}

	// ==========================  INSTANT RICHES (LED-style font) - 1st jackpot value:  ==============================
	else if (gameChoice == 2) 
	{
		currThreshVal = 40; // in case of global thresholding
		fromInputFile.open("C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/inputData/IR.txt");
		origFontStr = "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/trainInstRichJ";
	}

	else return 0; // incorrect game choice (game does not exist).

	// =====================================  TESTING - GREYSCALE  ======================================

	cvtColor(imgTestColor, imgTestGrey, COLOR_BGR2GRAY ); // greyscale: one color space to another	
	if(writeImgs) imwrite( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/outA-greyscale.png" , imgTestGrey);

	imgTestColor.copyTo(imgTestColorBin);
	imgTestColor.copyTo(imgTestColorBinCont);
	
	// =========================================  TESTING - DETECTION  ============================================
	
	for ( currJID=0; currJID < numJVals; currJID++)
	{
		// =================  TRAINING (FIRST jackpot value) - go thru every char to classify:  ===================

		int jRgnInputData [jRgnInputDataSize];	// jID, topLeft.x, topLeft.y, btmRt.x, btmRt.y, numDilSteps, "Gem text"
		string jRgnInputStr;
		string s;

		//if( currJID == 0 || currJID == 4 ) enGauss = 1;
		//else enGauss = 0;

		// file input - ROI data to memory:
		storeRgnInputData(fromInputFile, jRgnInputData, jRgnInputStr, jValRgns); 

		//cout << "Extracting and processing ROI: J" << currJID << endl;
		Mat imgRoiGrey(imgTestGrey, Rect( jValRgns[currJID].topLeft.x, 
										jValRgns[currJID].topLeft.y, 
										jValRgns[currJID].width, 
										jValRgns[currJID].height
									)
		); // produces greyscale ROI

		// store the grey ROI in the source folder:
		if(writeImgs)
		{
			s = insValIntoStr( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/outB-imgRoiGrey-J" , currJID, ".png");
			imwrite( s , imgRoiGrey );
		}

		if( enGauss )
		{
			GaussianBlur( imgRoiGrey, imgRoiGrey, Size(3,3), gaussSigma);
			s = insValIntoStr( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/outB-imgRoiGrey-J" , currJID, ".png");
			if(writeImgs) imwrite( s , imgRoiGrey); // saves the chosen output image
		}

		// draw a histogram for the binary ROI in the source folder:
		drawHistogram(currJID, imgRoiGrey);
		
		// thresholding:
		Mat imgRoiBin;
		// get Otsu threshold AND process greyscale to binary:
		if(enOtsu)
		{
			currThreshVal = threshold(imgRoiGrey, imgRoiBin
				, 0    // the value doesn't matter for Otsu thresholding
				, 255  // we could choose any non-zero value. 255 (white) makes it easy to see the binary image
				, THRESH_OTSU | THRESH_BINARY );
		}
		// otherwise, a default (global) threshold value is used:
		else currThreshVal = threshold(imgRoiGrey, imgRoiBin, currThreshVal, 255, THRESH_BINARY );

		// store the binary ROI in the source folder:
		s = insValIntoStr( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/outC-imgRoiBin-J" , currJID, ".png");
		if(writeImgs) imwrite( s , imgRoiBin );

		// Two ways to get rid of noise: dilation/erosion or custom noise reduction:
		if( gameChoice == 2 ) // reserved for instant riches
		{
			// ROI:
			dilate(imgRoiBin, imgRoiBin, Mat(), Point(-1,-1), jValRgns[currJID].numDilErds );
			erode(imgRoiBin, imgRoiBin, Mat(), Point(-1,-1), jValRgns[currJID].numDilErds);
		}

		// custom noise reduction the contours (as a substitute for dilation/erosion):
		if(enNoiseRdxn)
		{
			imgRoiBin = reduceImgNoise(imgRoiBin);
			// store the noise-less binary ROI in the source folder:
			s = insValIntoStr( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/outD-imgNoiseRdxn-J" , currJID, ".png");
			if(writeImgs) imwrite( s , imgRoiBin );
		}


		attachROI(imgTestColorBin, currJID, imgRoiBin, jValRgns);
		if(writeImgs) imwrite( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/outZ-testColorROIs.png" , imgTestColorBin);

		// ========= go thru every char to classify =========
		toDbgFile << endl << endl << endl;
		cout << "Classifying templates for J" << currJID << endl;
		for(int i=0; i < charsToClassify; i++)
		{
			if(dbgStts) cout << "\nhere-aa\n";
			char charASCII=0;
			if(dbgStts) cout << "\nhere-ab\n";
			string font = insValIntoStr( origFontStr, currJID, "/" );
			if(dbgStts) cout << "\nhere-ac\n";
			string trainPath = chooseFontSizeChar(font, i, charASCII);
			if(dbgStts) cout << "\nhere-b\n";
			train(toDbgFile, jValRgns, gameChoice, currJID, trainPath, 
				fDatabase, charASCII, rightOfChar, 
				enGauss, endOfRgn );
		}

		cout << "Performing testing on J" << currJID << endl;
		// create a clean black image for contours:
		threshold(imgRoiBin, imgWithContours, 255, 255, THRESH_BINARY);

		dollarSign = 1;
		while(!endOfRgn)
		{
			// send the ROI to this function:
			dtctAndMatch(toDbgFile, currJID, imgRoiBin, imgWithContours, 
				fDatabase, rightOfChar, jValStr, endOfRgn);
			//cout << "\nCharacter processed\n";
			dollarSign = 0;
		}

		attachROI(imgTestColorBinCont, currJID, imgWithContours, jValRgns);
		if(writeImgs) imwrite( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/outZ-testColorROIs-Cont.png" , imgTestColorBinCont);

		cout << "\nRecognized text for J" << currJID << ": ";
		toDbgFile << "\nRecognized text for J" << currJID << ": ";
		formatCurrJVal(toDbgFile, toUsrOutFile, currJID, jValStr, jValRgns );
		toUsrOutFile << endl;
		jValStr = ""; // clear the string for the next jackpot value.
		endOfRgn = 0; // reset for the next jackpot value.:
		rightOfChar = 0; // reset scanner.
	}
	
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

void train(ofstream & toDbgFile, ROIINFO jValRgns [] , int gameChoice, 
		   int currJID, string tmpltPath, float fDatabase [][charsToClassify], 
		   char whichChar, int & rightOfChar, 
		   bool enGauss, bool endOfRgn )
{
	toDbgFile << "Classifying " << whichChar << endl;
	if(dbgStts) cout << "\nhere-c\n";

	bool testFlag = 0;

	// ===============================================================================
	IMGPOINT * arrPts; // for dynamic allocation
	Mat imgTmpltColor = imread(tmpltPath, 1);
	Mat imgTmpltGray;
	Mat imgTmpltBin;
	Mat imgTmpltSmth;
	//
	vector< vector<int> > imgPxlsFlags (imgTmpltColor.size().height, vector<int>(imgTmpltColor.size().width));
	clearFlags(imgPxlsFlags, imgTmpltColor); // clear after allocation
	// ===============================================================================

	//! converts image from one color space to another
	cvtColor(imgTmpltColor, imgTmpltGray, COLOR_BGR2GRAY );
	
	if( enGauss )
	{
		GaussianBlur( imgTmpltGray, imgTmpltGray, Size(3,3), gaussSigma);
		if(writeImgs) imwrite( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/outB-greyGauss.png" , imgTmpltGray); // saves the chosen output image
	}
	
	if(dbgStts) cout << "\nhere-d\n";

	// uses Otsu's threshold value from the ROI (for consistency; same font/size - one threshold):
	threshold(imgTmpltGray, imgTmpltBin, currThreshVal, 255, THRESH_BINARY);
	if(writeImgs) imwrite( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/outC-imgTmpltBin.png", imgTmpltBin);

	if( gameChoice == 2 ) // reserved for instant riches
	{
		// template:
		dilate(imgTmpltBin, imgTmpltBin, Mat(), Point(-1,-1), jValRgns[currJID].numDilErds ); 
		//if(writeImgs) imwrite( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/wTrain-imgDbg.png", imgTmpltBin);
		erode(imgTmpltBin, imgTmpltBin, Mat(), Point(-1,-1), jValRgns[currJID].numDilErds );
		//if(writeImgs) imwrite( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/xTrain-imgDbg.png", imgTmpltBin);
	}

	// custom noise reduction:
	if(enNoiseRdxn) imgTmpltBin = reduceImgNoise(imgTmpltBin);
	
	if(dbgStts) cout << "\nhere-e\n";

	// ======= time to find the 4 moments for the char: =======
	// pre-compute no_points(size)(N):
	//if(writeImgs) imwrite( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/zTrain-imgDbg.png", imgTmpltBin);
	int numPts = countContPts(imgTmpltBin, imgPxlsFlags, 0, rightOfChar, endOfRgn); // might encounter inf-loop
	toDbgFile << "Num of points: " << numPts << endl; 
	clearFlags(imgPxlsFlags, imgTmpltColor); // clear after allocation
	arrPts = new IMGPOINT[numPts]; // now that we know the "size"

	// write the coords of every point into the points-array:
	wrtContCoords(imgTmpltBin, arrPts, numPts, imgPxlsFlags, 0, rightOfChar);
	clearFlags(imgPxlsFlags, imgTmpltBin); // clear after allocation

	// smoothening:
	if(enSmth)
	{
		smoothen(imgTmpltBin, arrPts, numPts);
		//string s = insValIntoStr( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/outHist-J" , currJID, ".png");
		//if(writeImgs) imwrite( s , outImg);
	}

	// Needs:
	// no_points(size)(N) - pre-computed numbers of contour pixels &
	// P[]( (x',y') ) - an array of coordinates stored in it.
	Sequential_moments(toDbgFile, numPts, arrPts);

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

	delete[] arrPts;
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
void Sequential_moments(ofstream & toDbgFile, int no_points, IMGPOINT * P)
{
	int i;
	float *S,*M;
	float moment1 = 0;

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
void Signature(ofstream & toDbgFile, int no_points, IMGPOINT * P, float * S)
{
  int i;
  float cgx, cgy;

  centroid(no_points, P, &cgx, &cgy);
  //toDbgFile << "centroid = " << cgx << " " << cgy << endl;

  toDbgFile << "-------------------------------------\n";
  for(i=0; i < no_points; i++)
  {
	S[i]=(float)hypot((double)(cgx-P[i].x),(double)(cgy-P[i].y));
	if(i<=200) toDbgFile << S[i] << endl;
  }
  toDbgFile << endl << endl;
  toDbgFile << "-------------------------------------\n";
}

// ===================================================================

// Needs:
// no_points(size)(N) - pre-computed numbers of contour pixels &
// P[]( (x',y') ) - an array of coordinates stored in it.
//
void centroid(int size, IMGPOINT * P, float * cgx, float * cgy)
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
	float sum = 0.0;

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
	float sum = 0.0;

	for(i=0; i<n; i++) sum += (float)pow((double)(S[i]-m1),(double)r);

	sum /= (float) n;

	return(sum);
}

// ================================================================

// computes the number of contour pixels of a white region:
//
int countContPts(Mat imgDtct, vector< vector<int> > & imgPxlsFlags, 
						 bool testFlag, int & rightOfChar, bool & endOfRgn)
{
	int i=0, j=imgDtct.size().height/2;

	int z0x, z0y, zix, ziy, numPts = 0, dir = 0;

	if (testFlag) i = rightOfChar; // horizontal component to start with

	// ===============================================================================

	// find the 1st (next) char in the image:
	while( imgDtct.ptr(j)[i] == 0 )
	{
		i++;

		if(testFlag)
		{
			if( i >= imgDtct.size().width ) // signal when out of image bounds
			{
				endOfRgn = 1;
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
	numPts++;

	dir = 0; // reset direction
	// find the first black neighbor:
	while ( imgDtct.ptr(ziy+dirs[dir].y)[(zix+dirs[dir].x)] == 255 )
		dir = (dir+1) % 8;

	// Find the first unflagged white neighbor: (why not ... == 0? - grey).
	//  - [dir][0,1]; dir - direction index; 0,1 - rel. x,y of px neighbor.
	while ( imgDtct.ptr(ziy+dirs[dir].y)[(zix+dirs[dir].x)] != 255 || imgPxlsFlags[ ziy+dirs[dir].y ][ zix+dirs[dir].x ]==1 )
		dir = (dir+1) % 8;

	// go to the first white neighbor after the black ones:
	zix = zix + dirs[dir].x;
	ziy = ziy + dirs[dir].y;
	imgPxlsFlags[ ziy ][ zix ] = 1; // set flag

	// image processing debugging:
	//threshold(imgDtct, imgDbg, currThreshVal, 255, THRESH_BINARY);
	//if(writeImgs) imwrite( "../src/stgImgs/out-imgDbg.png" , imgDbg);

	// go around 1 time (use the start/finish pixel)
	// and count contour pixels:
	//
	// (zix!=z0x) && (ziy!=z0y) did not work:
	while ( !(zix==z0x && ziy==z0y) )
	{
		// count the white pixel:
		numPts++;

		dir = 0; // reset direction

		// find the first black or NULL neighbor:
		bool next = true;
		while (next)
		{
			if ( isInBounds(imgDtct, ziy+dirs[dir].y, zix+dirs[dir].x) )
			{
				if ( imgDtct.ptr(ziy+dirs[dir].y)[(zix+dirs[dir].x)] == 255)
					dir = (dir+1) % 8; // if white

				else next = false; // if black
			}
			else next = false; // if out of bounds
		}

		// find the first white neighbor:
		next = true;
		while(next)
		{
			if ( !isInBounds(imgDtct, ziy+dirs[dir].y, zix+dirs[dir].x) )
			{
				dir = (dir+1) % 8; // if out of bounds
			}
			else if (imgDtct.ptr(ziy+dirs[dir].y)[(zix+dirs[dir].x)] == 0 || imgPxlsFlags[ ziy+dirs[dir].y ][ zix+dirs[dir].x ]==1 )
			{
				dir = (dir+1) % 8; // if black
			}
			else next = false; // if white
		}

		// go to the first white neighbor after the black ones:
		zix = zix + dirs[dir].x;
		ziy = ziy + dirs[dir].y;
		imgPxlsFlags[ ziy ][ zix ] = 1; // set flag
	}
	
	return numPts;
}

void wrtContCoords(Mat imgGrayWrite, IMGPOINT arrPts[],
		int numPts, vector< vector<int> > & imgPxlsFlags, bool testFlag, 
		int & rightOfChar)
{
	//cout << "Writing contour coordinates to an array" << endl;

	// must be ints for processing image:
	int i=0, j=imgGrayWrite.size().height/2; // j - y, i - x ; 0, imgDtct.size().height/2


	if (testFlag) i = rightOfChar; // horizontal component to start with

	int zix, ziy, currPoint = 0, dir = 0;

	// go through the image left-to-right; find the char:
	while( imgGrayWrite.ptr(j)[i] == 0 ) i++;
	
	// the start/finish pixel (write it into the array):
	arrPts[0].x = i;
	arrPts[0].y = j;
	// current pixel for feeling the contour:
	zix = arrPts[0].x;
	ziy = arrPts[0].y;

	// =======================================================
	// change coords of the white pixel (move to the next one)
	// in order to not get stuck on the first one when
	// going through the loops:
	// =======================================================

	// count the white pixel:
	currPoint++;

	dir = 0; // reset direction
	// find the first black neighbor:
	while ( imgGrayWrite.ptr(ziy+dirs[dir].y)[(zix+dirs[dir].x)] == 255 )
		dir = (dir+1) % 8;

	// find the first white neighbor:
	while ( imgGrayWrite.ptr(ziy+dirs[dir].y)[(zix+dirs[dir].x)] == 0 
		|| imgPxlsFlags[ ziy+dirs[dir].y ][ zix+dirs[dir].x ]==1 )
		dir = (dir+1) % 8;

	// go to the first white neighbor after the black ones:
	arrPts[currPoint].x = zix + dirs[dir].x;
	arrPts[currPoint].y = ziy + dirs[dir].y;
	// write it into the array:
	zix = arrPts[currPoint].x;
	ziy = arrPts[currPoint].y;
	imgPxlsFlags[ ziy ][ zix ] = 1; // set flag
	
	// go around 1 time (use the start/finish pixel)
	// and count contour pixels:

	while ( currPoint!=(numPts-1) )
	{
		// count the white pixel:
		currPoint++;

		dir = 0; // reset direction

		// find the first black or NULL neighbor:
		bool next = true;
		while (next)
		{
			if ( isInBounds(imgGrayWrite, ziy+dirs[dir].y, zix+dirs[dir].x) )
			{
				if ( imgGrayWrite.ptr(ziy+dirs[dir].y)[(zix+dirs[dir].x)] == 255 )
					dir = (dir+1) % 8; // if white
				else next = false; // if black
			}
			else next = false; // if out of bounds
		}

		// find the first white neighbor:
		next = true;
		while(next)
		{
			if ( !isInBounds(imgGrayWrite, ziy+dirs[dir].y, zix+dirs[dir].x) )
				dir = (dir+1) % 8; // if out of bounds

			else if (imgGrayWrite.ptr(ziy+dirs[dir].y)[(zix+dirs[dir].x)] == 0 || imgPxlsFlags[ ziy+dirs[dir].y ][ zix+dirs[dir].x ]==1 )
				dir = (dir+1) % 8; // if black

			else next = false; // if white
		}

		// go to the first white neighbor after the black ones:
		arrPts[currPoint].x = zix + dirs[dir].x;
		arrPts[currPoint].y = ziy + dirs[dir].y;

		// write it into the array:
		zix = arrPts[currPoint].x;
		ziy = arrPts[currPoint].y;
		imgPxlsFlags[ ziy ][ zix ] = 1; // set flag
	}
}

/* 
 * ... because using the image pointer to refer to a
 *
 * (non-)existent pixel did not work:
 *
 */

bool isInBounds(Mat img4Bnds, int y, int x)
{
	if ( x>=0 && x<img4Bnds.size().width &&
			y>=0 && y<img4Bnds.size().height)
	return true;
	else return false;
}

/* 
 * void dtctAndMatch():
 * 
 *	- ROI already pre-processed in main().
 */

void dtctAndMatch(ofstream & toDbgFile, int currJID, Mat imgDtct, 
						Mat imgWithContours, float fDatabase[][charsToClassify], 
						int & rightOfChar, string & jValStr, bool & endOfRgn)
{
	//cout << "Detecting and matching a character" << endl;
		
	IMGPOINT * arrPts; // for dynamic allocation
	bool testFlag = 1;
	bool showInaccurate = 1;

	for (int i=0; i < numOfCandidates; i++)
	{
		arrDist[i] = 99999999;
		arrI[i] = 0;
	}
	
	vector< vector<int> > imgPxlsFlags (imgDtct.size().height, vector<int>(imgDtct.size().width));
	clearFlags(imgPxlsFlags, imgDtct); // clear after allocation

	// ======= time to find the moments for the char: =======
	// pre-compute no_points(size)(N):
	//if(writeImgs) imwrite( "../src/stgImgs/z-imgDbg.png", imgDtct);
	int numPts = countContPts(imgDtct, imgPxlsFlags, 1, rightOfChar, endOfRgn);
	toDbgFile << "Num of points: " << numPts << endl; 

	if (endOfRgn) return; // get out; done with the region

	clearFlags(imgPxlsFlags, imgDtct); // clear after allocation
	arrPts = new IMGPOINT[numPts]; // now that we know the "size"

	// write the coords of every point into the points-array:
	wrtContCoords(imgDtct, arrPts, numPts, imgPxlsFlags, 1, rightOfChar);
	clearFlags(imgPxlsFlags, imgDtct); // clear after allocation

	// smoothening:
	if(enSmth)
	{
		smoothen(imgDtct, arrPts, numPts);
		string s = insValIntoStr( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/outSmth-" , currJID, ".png");
		Mat outImg;
		if(writeImgs) imwrite( s , outImg);
	}

	// Needs:
	// no_points(size)(N) - pre-computed numbers of contour pixels &
	// P[]( (x',y') ) - an array of coordinates stored in it.
	Sequential_moments(toDbgFile, numPts, arrPts);

	// given trained database with moments and the moments of the testing object in the image,
	// compute 11 Euclidean distances (0-9 and $):
	for(int i=0; i < charsToClassify; i++)
	{
		// computing the Euclidean distance between train and test images:
		float squareEuclDist = 0;

		for(int f=0; f <= numFeatureMoments-1; f++)
			squareEuclDist += pow(featureMoments[f] - fDatabase[f][i],2);

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
	
	// =====================================  EUCLIDEAN DISTANCES OUTPUT  ========================================

	/* c2 - central moments of testing $ 
	            match those of template 7.
	toDbgFile << "  =      Min. eucl. dist.: " << arrDist[0] << endl;
	toDbgFile << "  =  2nd Min. eucl. dist.: " << arrDist[1] << endl;
	
	for(int c=0; c<numCentralMoments; c++) 
		toDbgFile << "        Central " << c+1 << ": " << centralMoments[c] << "\n";

	for(int f=0; f<numFeatureMoments; f++)
		toDbgFile << "Feature " << f+1 << ": " << featureMoments[f] << "\n";

	toDbgFile << endl << endl; */
	
	if ( arrI[0] >= 0 && arrI[0] <= (charsToClassify-2) ) // between 0-9 incl.
	{
		if(showInaccurate) jValStr.append( to_string(arrI[0]) );
		else if( abs(arrDist[1]-arrDist[0]) < allowedConf )
			jValStr.append(" ");
		else jValStr.append( to_string(arrI[0]) );
	}
	else 
	{
		if(showInaccurate) jValStr.append("$");
		else if( abs(arrDist[1]-arrDist[0]) < allowedConf )
			jValStr.append(" ");
		else jValStr.append("$");
	}
	
	// ================ FIND THE RIGHTMOST PIXEL TO DETECT NEXT CHARACTER =================
	for(int z=0; z < numPts; z++)
		if(rightOfChar < arrPts[z].x)
			rightOfChar = arrPts[z].x;

	// Displacement - get off the current set of connected components
	// before you proceed to the next one:
	rightOfChar++; // 1 px to the right.

	// ====================== BEFORE WE DESTROY THE ARRAY AND LEAVE, =======================
	// ==================== MAP THE CONTOUR PIXELS AND HIGHLIGHT THEM ======================
	// create a blank image of the same size for contours:
	mapContours(imgWithContours, arrPts, numPts);

	// ============================ CLOSING TIME IN THE FUNCTION =============================
	delete[] arrPts; // release memory before returning.

	return;
} // end of dtctAndMatch()

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

void mapContours(Mat imgWithContours, IMGPOINT arrPts[], int numPts)
{
	//cout << "\nMapping contours onto a black image";

	for(int k=0; k < numPts; k++)
	{
		// not friendly to "imgWithContours.ptr(arrPts[k].y)[arrPts[k].x] = 255;"
		int x = arrPts[k].x;
		int y = arrPts[k].y;
		imgWithContours.ptr(y)[x] = 255;
	}
}

void locateJValue( int jRgnInputData [] , string text , ROIINFO jValRgns [] )
{
	int jValID = jRgnInputData[0];

	//cout << "Defining coordinates for a jackpot value" << endl;
	// top left:
	jValRgns[jValID].topLeft.x = jRgnInputData[1];
	jValRgns[jValID].topLeft.y = jRgnInputData[2];
	// bottom right:
	jValRgns[jValID].bottomRight.x = jRgnInputData[3];
	jValRgns[jValID].bottomRight.y = jRgnInputData[4];
	// dimensions:
	jValRgns[jValID].width = jRgnInputData[3] - jRgnInputData[1];
	jValRgns[jValID].height = jRgnInputData[4] - jRgnInputData[2];
	// other data:
	jValRgns[jValID].numDilErds = jRgnInputData[5];
	jValRgns[jValID].text = text;
}

// ======================================================================================
// void formatCurrJVal()
// 
//  - prints the jackpot value on the fly and places commas and a dot where they belong.
// ======================================================================================

void formatCurrJVal(ofstream & toDbgFile, ofstream & toUsrOutFile, int jVal, 
					string & jValStr, ROIINFO jValRgns [] )
{
	//cout << "Formatting the current jackpot value" << endl;

	toUsrOutFile << jValRgns[jVal].text << "|";
	
	for(int s=0 ; s < jValStr.length() ; s++) // format the jackpot value here.
	{
		toDbgFile << jValStr.at(s); // copy the char to the right-to-left string
		toUsrOutFile << jValStr.at(s); // copy the char to the right-to-left string
		cout << jValStr.at(s);

		// The decision was made to pass j-value to UI unformatted:
		if(s == jValStr.length()-3) // 3rd char from right - decimal point
		{
			toDbgFile << ".";
			if(enFormat) toUsrOutFile << ".";
			cout << ".";
		}

		else if( (jValStr.length()-s)%3==0 && jValStr.at(s)!='$' ) // 3rd char from right - decimal point
		{
			toDbgFile << ",";
			if(enFormat) toUsrOutFile << ",";
			cout << ",";
		}
		// ===========================================================
	}

	cout << endl << endl;
}

void mainMenu(int & gameChoice)
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

string chooseFontSizeChar(string trainPath, int i, char & charASCII)
{
	if( i != charsToClassify-1 )
	{
		trainPath.append( to_string(i) );
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

void storeRgnInputData(ifstream & fromInputFile, int jRgnInputData [], string jRgnInputStr, ROIINFO jValRgns [] )
{
	for(int a=0; a<jRgnInputDataSize; a++)
	{
		fromInputFile >> jRgnInputData[a];	// read int (id, tlx, tly, brx, bry, dilErds)
		fromInputFile >> jRgnInputStr;		// skip a string
	}
	fromInputFile.ignore(1,' ');			// skip " "
	getline (fromInputFile, jRgnInputStr, '\n');	// read text for j-value
	locateJValue( jRgnInputData, jRgnInputStr, jValRgns);	// pass to a j-value-element
}

void drawHistogram(int currJID, Mat imageGray)
{
	//cout << "Drawing histogram for j-value region" << endl;

	Mat imgDummy( 400, 400, CV_8UC1, Scalar(0) );
	float  histogram[256];		// histogram values
	float  histNormalized[256];	// normalized histogram values

	// histogram elements assignment:
	for(int hi=0; hi<=255; hi++)
	{
		histogram[hi] = 0;
		histNormalized[hi] = 0;
	}

	// incr. the px val amounts on the histogram:
	for(int a=0; a<imageGray.size().height; a++)
		for(int b=0; b<imageGray.size().width; b++)
			histogram[imageGray.ptr( a )[ b ]]++;

	// Create normalised histogram values
	int size = imageGray.size().height * imageGray.size().width;

	for (int I=0; I<=255; I++)
		histNormalized[I] = histogram[I]/(float)size;



	// Histogram ready. Now we plot it on a blank image.
	// 1) Copy:
	//threshold(imageGray, imgDummy, 255, 255, THRESH_BINARY);
	// 2) Plot:
	//		a) Find max num of points of same px val:
	float maxHist = 0;
	for (int I=0; I<=255; I++)
	{
		if(histNormalized[I] > maxHist) maxHist = histNormalized[I];
	}
	//		b) Do the actual plotting to fit the image:
	for (int I=0; I<=255; I++)
	{
		// Some decrementation is performed to avoid references to non-existent addresses:
		int heightFloor = imgDummy.size().height-1;
		int pxValAmtScld = 1.00*histNormalized[I]*heightFloor/maxHist;
		// if pxValAmt = 0,	we assign to length-address;	=> need to bring height down by 1
		// if pxValAmt = max,	we assign to element 0;			=> OK. 
		imgDummy.ptr( heightFloor - pxValAmtScld )[imgDummy.size().width * I/255] = 255;
	}
	
	// the plot thickens:
	dilate(imgDummy, imgDummy, Mat(), Point(-1,-1), 2 ); 

	// Marking the optimal threshold on the histogram:
	for(int d=0; d<imgDummy.size().height; d++)
		imgDummy.ptr(d)[ imgDummy.size().width*currThreshVal/255 ] = 128;

	// 3) Write:
	string s = insValIntoStr( "C:/Users/Andrey/Documents/Visual Studio 2012/Projects/opencvTest4/src/stgImgs/outHist-J" , currJID, ".png");
	if(writeImgs) imwrite( s , imgDummy);

	return;
}

/*
 *	custom noise reduction contours on the image
 *
 */

Mat reduceImgNoise(Mat imgRough)
{
	int pxValSum = 0;
	int numOfNeighbors = 0;
	float neighpxAvg = 0;
	Mat imgSmth;

	imgRough.copyTo(imgSmth);

	for(int imgY=0; imgY<imgSmth.size().height; imgY++) // every px row
	{
		for(int imgX=0; imgX<imgSmth.size().width; imgX++) // every px in the row
		{
			for(int dir=0; dir<8; dir++) // every neighbor of the curr px
			{
				if ( isInBounds(imgSmth, imgY+dirs[dir].y, imgX+dirs[dir].x) ) // if in the bounds
				{
					pxValSum += imgSmth.ptr (imgY+dirs[dir].y) [imgX+dirs[dir].x];
					numOfNeighbors++;
				}
			}

			neighpxAvg = pxValSum / numOfNeighbors;

			// 3/8 neighbors or less are white - mark it black;
			// 5/8 neighbors or more are white - mark it white;
			// 4/8 neighbors are white - leave the px val as it is:
			if  (neighpxAvg <= 95 ) imgSmth.ptr(imgY)[imgX] = 0;
			if  (neighpxAvg >= 159 ) imgSmth.ptr(imgY)[imgX] = 255;
			
			// reset for every pixel:
			pxValSum = 0;
			numOfNeighbors = 0;
			neighpxAvg = 0;
		}
	}
	
	//cout << "Finished custom noise reduction contours on the image" << endl;

	return imgSmth;
}

string insValIntoStr( string leftEnd, int currJID, string rightEnd )
{
	string wholeStr = leftEnd;
	wholeStr.append( to_string(currJID) );
	wholeStr.append( rightEnd );

	return wholeStr;
}

string insStrIntoStr( string leftEnd, string middle, string rightEnd)
{
	string wholeStr = leftEnd;
	wholeStr.append( middle );
	wholeStr.append( rightEnd );

	return wholeStr;
}

void attachROI(Mat imgTestColor, int currJID, Mat img2atch, ROIINFO jValRgns [] )
{
	//cout << endl << "Attaching the ROI to a copy of the color testing image";
	
	int numChannels = 3;
	int roiX = jValRgns[currJID].topLeft.x;
	int roiY = jValRgns[currJID].topLeft.y;
	int roiH = jValRgns[currJID].height;
	int roiW = jValRgns[currJID].width;
	
	for(int b=0; b<roiH; b++)
	{
		for(int a=0; a<roiW; a++)
		{
			for(int c=0; c<numChannels; c++)
				imgTestColor.ptr(roiY+b)[ numChannels*(roiX+a) +c] = img2atch.ptr(b)[a];
		}
	}
	
	//cout << endl << "Finished attaching the ROI";

	return;
}

void smoothen( Mat & imgRoughCont, IMGPOINT arrPts [] , int numPts)
{
	int a = pxNeiWinSize / 2;	// half the window size truncated
	int z = numPts - a;			// ending point for smoothening
	
	// for every "internal" pixel (safeguarded by window size from both ends):
	for( int t=a ; t<z ; t++ ) 
	{
		int sumX = 0;				// sum of x's of neighbors
		int sumY = 0;				// sum of y's of neighbors
		int newX = 0;				
		int newY = 0;

		for(int s=1; s<=a; s++)
		{
			// "left" neighbor:
			sumX += arrPts[a-s].x;
			sumY += arrPts[a-s].y;
			// "right" neighbor:
			sumX += arrPts[a+s].x;
			sumY += arrPts[a+s].y;
		}
		
		newX = sumX / (pxNeiWinSize - 1);
		newY = sumY / (pxNeiWinSize - 1);

		arrPts[t].x = newX;
		arrPts[t].y = newY;

		imgRoughCont.ptr (newY) [newX] = 255;
	}
}