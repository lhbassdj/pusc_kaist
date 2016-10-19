#include "ardrone.h"
#include "ros/ros.h"
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
// #include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <pthread.h>

#define ONE_SECOND 10

using namespace cv;
using namespace std;


//Came from motionTracking.cpp
//our sensitivity value to be used in the absdiff() function
const static int SENSITIVITY_VALUE = 20;
//size of blur used to smooth the intensity image output from absdiff() function
const static int BLUR_SIZE = 10;
//we'll have just one object to search for
//and keep track of its position.
int theObject[2] = { 0,0 };
//bounding rectangle of the object, we will use the center of this as its position.
Rect objectBoundingRectangle = Rect(0, 0, 0, 0);


//initial min and max HSV filter values.
//these will be changed using trackbars
int H_MIN = 0;
int H_MAX = 256;
int S_MIN = 0;
int S_MAX = 256;
int V_MIN = 0;
int V_MAX = 256;
//default capture width and height
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;
//max number of objects to be detected in frame
const int MAX_NUM_OBJECTS = 50;
//minimum and maximum object area
const int MIN_OBJECT_AREA = 20 * 20;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH / 1.5;
//names that will appear at the top of each window
const String windowName = "Original Image";
const String windowName1 = "HSV Image";
const String windowName2 = "Thresholded Image";
const String windowName3 = "After Morphological Operations";
const String trackbarWindowName = "Trackbars";

bool calibrationMode;//used for showing debugging windows, trackbars etc.
bool mouseIsDragging;//used for showing a rectangle on screen as user clicks and drags mouse
bool mouseMove;
bool rectangleSelected;
Point initialClickPoint, currentMousePoint; //keep track of initial point clicked and current position of mouse
Rect rectangleROI; //this is the ROI that the user has selected
vector<int> H_ROI, S_ROI, V_ROI;// HSV values from the click/drag ROI region stored in separate vectors so that we can sort them easily

//int to string helper function
string intToString(int number) {

	//this function has a number input and string output
	std::stringstream ss;
	ss << number;
	return ss.str();
}
void on_trackbar(int, void*)
{//This function gets called whenever a
 // trackbar position is changed

 //for now, this does nothing.



}
void createTrackbars() {
	//create window for trackbars


	namedWindow(trackbarWindowName, 0);
	//create memory to store trackbar name on window
	char TrackbarName[50];
	sprintf(TrackbarName, "H_MIN", H_MIN);
	sprintf(TrackbarName, "H_MAX", H_MAX);
	sprintf(TrackbarName, "S_MIN", S_MIN);
	sprintf(TrackbarName, "S_MAX", S_MAX);
	sprintf(TrackbarName, "V_MIN", V_MIN);
	sprintf(TrackbarName, "V_MAX", V_MAX);
	//create trackbars and insert them into window
	//3 parameters are: the address of the variable that is changing when the trackbar is moved(eg.H_LOW),
	//the max value the trackbar can move (eg. H_HIGH), 
	//and the function that is called whenever the trackbar is moved(eg. on_trackbar)
	//                                  ---->    ---->     ---->      
	createTrackbar("H_MIN", trackbarWindowName, &H_MIN, 255, on_trackbar);
	createTrackbar("H_MAX", trackbarWindowName, &H_MAX, 255, on_trackbar);
	createTrackbar("S_MIN", trackbarWindowName, &S_MIN, 255, on_trackbar);
	createTrackbar("S_MAX", trackbarWindowName, &S_MAX, 255, on_trackbar);
	createTrackbar("V_MIN", trackbarWindowName, &V_MIN, 255, on_trackbar);
	createTrackbar("V_MAX", trackbarWindowName, &V_MAX, 255, on_trackbar);


}
void clickAndDrag_Rectangle(int event, int x, int y, int flags, void* param) {
	//only if calibration mode is true will we use the mouse to change HSV values
	if (calibrationMode == true) {
		//get handle to video feed passed in as "param" and cast as Mat pointer
		Mat* videoFeed = (Mat*)param;

		if (event == CV_EVENT_LBUTTONDOWN && mouseIsDragging == false)
		{
			//keep track of initial point clicked
			initialClickPoint = Point(x, y);
			//user has begun dragging the mouse
			mouseIsDragging = true;
		}
		/* user is dragging the mouse */
		if (event == CV_EVENT_MOUSEMOVE && mouseIsDragging == true)
		{
			//keep track of current mouse point
			currentMousePoint = Point(x, y);
			//user has moved the mouse while clicking and dragging
			mouseMove = true;
		}
		/* user has released left button */
		if (event == CV_EVENT_LBUTTONUP && mouseIsDragging == true)
		{
			//set rectangle ROI to the rectangle that the user has selected
			rectangleROI = Rect(initialClickPoint, currentMousePoint);

			//reset boolean variables
			mouseIsDragging = false;
			mouseMove = false;
			rectangleSelected = true;
		}

		if (event == CV_EVENT_RBUTTONDOWN) {
			//user has clicked right mouse button
			//Reset HSV Values
			H_MIN = 0;
			S_MIN = 0;
			V_MIN = 0;
			H_MAX = 255;
			S_MAX = 255;
			V_MAX = 255;

		}
		if (event == CV_EVENT_MBUTTONDOWN) {

			//user has clicked middle mouse button
			//enter code here if needed.
		}
	}

}
void recordHSV_Values(Mat frame, Mat hsv_frame) {

	//save HSV values for ROI that user selected to a vector
	if (mouseMove == false && rectangleSelected == true) {

		//clear previous vector values
		if (H_ROI.size() > 0) H_ROI.clear();
		if (S_ROI.size() > 0) S_ROI.clear();
		if (V_ROI.size() > 0)V_ROI.clear();
		//if the rectangle has no width or height (user has only dragged a line) then we don't try to iterate over the width or height
		if (rectangleROI.width < 1 || rectangleROI.height < 1) cout << "Please drag a rectangle, not a line" << endl;
		else {
			for (int i = rectangleROI.x; i < rectangleROI.x + rectangleROI.width; i++) {
				//iterate through both x and y direction and save HSV values at each and every point
				for (int j = rectangleROI.y; j < rectangleROI.y + rectangleROI.height; j++) {
					//save HSV value at this point
					H_ROI.push_back((int)hsv_frame.at<Vec3b>(j, i)[0]);
					S_ROI.push_back((int)hsv_frame.at<Vec3b>(j, i)[1]);
					V_ROI.push_back((int)hsv_frame.at<Vec3b>(j, i)[2]);
				}
			}
		}
		//reset rectangleSelected so user can select another region if necessary
		rectangleSelected = false;
		//set min and max HSV values from min and max elements of each array

		if (H_ROI.size() > 0) {
			//NOTE: min_element and max_element return iterators so we must dereference them with "*"
			H_MIN = *std::min_element(H_ROI.begin(), H_ROI.end());
			H_MAX = *std::max_element(H_ROI.begin(), H_ROI.end());
			cout << "MIN 'H' VALUE: " << H_MIN << endl;
			cout << "MAX 'H' VALUE: " << H_MAX << endl;
		}
		if (S_ROI.size() > 0) {
			S_MIN = *std::min_element(S_ROI.begin(), S_ROI.end());
			S_MAX = *std::max_element(S_ROI.begin(), S_ROI.end());
			cout << "MIN 'S' VALUE: " << S_MIN << endl;
			cout << "MAX 'S' VALUE: " << S_MAX << endl;
		}
		if (V_ROI.size() > 0) {
			V_MIN = *std::min_element(V_ROI.begin(), V_ROI.end());
			V_MAX = *std::max_element(V_ROI.begin(), V_ROI.end());
			cout << "MIN 'V' VALUE: " << V_MIN << endl;
			cout << "MAX 'V' VALUE: " << V_MAX << endl;
		}

	}

	if (mouseMove == true) {
		//if the mouse is held down, we will draw the click and dragged rectangle to the screen
		rectangle(frame, initialClickPoint, Point(currentMousePoint.x, currentMousePoint.y), Scalar(0, 255, 0), 1, 8, 0);
	}


}
void drawObject(int x, int y, Mat &frame) {

	//use some of the openCV drawing functions to draw crosshairs
	//on your tracked image!


	//'if' and 'else' statements to prevent
	//memory errors from writing off the screen (ie. (-25,-25) is not within the window)

	circle(frame, Point(x, y), 20, Scalar(0, 255, 0), 2);
	if (y - 25 > 0)
		line(frame, Point(x, y), Point(x, y - 25), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(x, 0), Scalar(0, 255, 0), 2);
	if (y + 25 < FRAME_HEIGHT)
		line(frame, Point(x, y), Point(x, y + 25), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(x, FRAME_HEIGHT), Scalar(0, 255, 0), 2);
	if (x - 25 > 0)
		line(frame, Point(x, y), Point(x - 25, y), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(0, y), Scalar(0, 255, 0), 2);
	if (x + 25 < FRAME_WIDTH)
		line(frame, Point(x, y), Point(x + 25, y), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(FRAME_WIDTH, y), Scalar(0, 255, 0), 2);

	putText(frame, intToString(x) + "," + intToString(y), Point(x, y + 30), 1, 1, Scalar(0, 255, 0), 2);

}
void morphOps(Mat &thresh) {

	//create structuring element that will be used to "dilate" and "erode" image.
	//the element chosen here is a 3px by 3px rectangle

	Mat erodeElement = getStructuringElement(MORPH_RECT, Size(3, 3));
	//dilate with larger element so make sure object is nicely visible
	Mat dilateElement = getStructuringElement(MORPH_RECT, Size(8, 8));

	erode(thresh, thresh, erodeElement);
	erode(thresh, thresh, erodeElement);


	dilate(thresh, thresh, dilateElement);
	dilate(thresh, thresh, dilateElement);



}
void trackFilteredObject(int &x, int &y, Mat threshold, Mat &cameraFeed, double &currentArea, bool &found) {

	Mat temp;
	threshold.copyTo(temp);
	//these two vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
	//find contours of filtered image using openCV findContours function
	findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
	//use moments method to find our filtered object
	double refArea = 0;
	int largestIndex = 0;
	bool objectFound = false;
	if (hierarchy.size() > 0) {
		int numObjects = hierarchy.size();
		//if number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
		if (numObjects < MAX_NUM_OBJECTS) {
			for (int index = 0; index >= 0; index = hierarchy[index][0]) {

				Moments moment = moments((Mat)contours[index]);
				double area = moment.m00;
				//if the area is less than 20 px by 20px then it is probably just noise
				//if the area is the same as the 3/2 of the image size, probably just a bad filter
				//we only want the object with the largest area so we save a reference area each
				//iteration and compare it to the area in the next iteration.
				//MIN_OBJECT_AREA=400, MAX_OBJECT_AREA=204800
				if (area > MIN_OBJECT_AREA && area<MAX_OBJECT_AREA && area>refArea) {
					x = moment.m10 / area;
					y = moment.m01 / area;
					objectFound = true;
					refArea = area;
					//save index of largest contour to use with drawContours
					largestIndex = index;
				}
				else objectFound = false;


			}
			//let user know you found an object
			if (objectFound == true) {

				putText(cameraFeed, "Tracking Object", Point(0, 50), 2, 1, Scalar(0, 255, 0), 2);
				//draw object location on screen
				drawObject(x, y, cameraFeed);
				//draw largest contour
				drawContours(cameraFeed, contours, largestIndex, Scalar(0, 255, 255), 2);
				currentArea = refArea;
				// std::cout << "position x:" <<x <<"  y: " <<y << endl << "Area :" << refArea <<endl;
			}

		}
		else putText(cameraFeed, "TOO MUCH NOISE! ADJUST FILTER", Point(0, 50), 1, 2, Scalar(0, 0, 255), 2);
		found = objectFound;
	}
}
void searchForMovement(Mat thresholdImage, Mat &cameraFeed) {
	//notice how we use the '&' operator for objectDetected and cameraFeed. This is because we wish
	//to take the values passed into the function and manipulate them, rather than just working with a copy.
	//eg. we draw to the cameraFeed to be displayed in the main() function.
	bool objectDetected = false;
	Mat temp;
	thresholdImage.copyTo(temp);
	//these two vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
	//find contours of filtered image using openCV findContours function
	//findContours(temp,contours,hierarchy,CV_RETR_CCOMP,CV_CHAIN_APPROX_SIMPLE );// retrieves all contours
	findContours(temp, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);// retrieves external contours

																					  //if contours vector is not empty, we have found some objects
	if (contours.size()>0)objectDetected = true;
	else objectDetected = false;

	if (objectDetected) {
		//the largest contour is found at the end of the contours vector
		//we will simply assume that the biggest contour is the object we are looking for.
		vector< vector<Point> > largestContourVec;
		largestContourVec.push_back(contours.at(contours.size() - 1));
		//make a bounding rectangle around the largest contour then find its centroid
		//this will be the object's final estimated position.
		objectBoundingRectangle = boundingRect(largestContourVec.at(0));
		int xpos = objectBoundingRectangle.x + objectBoundingRectangle.width / 2;
		int ypos = objectBoundingRectangle.y + objectBoundingRectangle.height / 2;

		//update the objects positions by changing the 'theObject' array values
		theObject[0] = xpos, theObject[1] = ypos;
	}
	//make some temp x and y variables so we dont have to type out so much
	int x = theObject[0];
	int y = theObject[1];

	//draw some crosshairs around the object
	circle(cameraFeed, Point(x, y), 20, Scalar(0, 255, 0), 2);
	line(cameraFeed, Point(x, y), Point(x, y - 25), Scalar(0, 255, 0), 2);
	line(cameraFeed, Point(x, y), Point(x, y + 25), Scalar(0, 255, 0), 2);
	line(cameraFeed, Point(x, y), Point(x - 25, y), Scalar(0, 255, 0), 2);
	line(cameraFeed, Point(x, y), Point(x + 25, y), Scalar(0, 255, 0), 2);

	//write the position of the object to the screen
	putText(cameraFeed, "Tracking object at (" + intToString(x) + "," + intToString(y) + ")", Point(x, y), 1, 1, Scalar(255, 0, 0), 2);
}


int main(int argc, char **argv)
{

  	ros::init(argc, argv, "drone_tracking");
  	ROS_INFO("Started TUM ArDrone tracking Node.");
  
	//Initial setting for Users
	//Set the image's size
	double MIN_AREA = 3000;
	double MAX_AREA = 3100;
	double diff = 10;

	//Filtered area
	double filteredArea = 0;

	//some boolean variables for different functionality within this
	//program
	bool trackObjects = true;
	bool useMorphOps = true;
	calibrationMode = true;
	bool objectDetected = false;
	//these two can be toggled by pressing 'd' or 't'
	bool debugMode = false;
	bool trackingEnabled = false;
	//pause and resume code
	bool pause = false;
	bool tracking = false;
	

	//Matrix to store each frame of the webcam feed
	Mat webcam;
	//matrix storage for HSV image
	Mat HSV;
	//matrix storage for binary threshold image
	Mat threshold;
	//set up the matrices that we will need
	//the two frames we will be comparing
	Mat frame1, frame2;
	//their grayscale images (needed for absdiff() function)
	Mat grayImage1, grayImage2;
	//resulting difference image
	Mat differenceImage;
	//thresholded difference image (for use in findContours() function)
	Mat thresholdImage;
	//video capture object.
	VideoCapture capture;
	
	
	
	//x and y values for the location of the object
	int x = 0, y = 0;

	//open capture object at location zero (default location for webcam)
	capture.open(0);
	////set height and width of capture frame
	//capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	//capture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
	////must create a window before setting mouse callback
	//namedWindow(windowName);
	//set mouse callback function to be active on "Webcam Feed" window
	//we pass the handle to our "frame" matrix so that we can draw a rectangle to it
	//as the user clicks and drags the mouse
	

	//initiate mouse move and drag to false 
	mouseIsDragging = false;
	mouseMove = false;
	rectangleSelected = false;
	std::cout <<"asdfasdf";
 	// // AR.Drone class
  // 	ARDrone ardrone;

  // 	// Initialize
  // 	if (!ardrone.open()) {
  //     	std::cout << "Failed to initialize." << std::endl;
  //     	return -1;
  // 	}

  //   // Battery
  //   std::cout << "Battery = " << ardrone.getBatteryPercentage() << "[%]" << std::endl;

  //   // Instructions
  //   std::cout << "***************************************" << std::endl;
  //   std::cout << "*       CV Drone sample program       *" << std::endl;
  //   std::cout << "*           - How to play -           *" << std::endl;
  //   std::cout << "***************************************" << std::endl;
  //   std::cout << "*                                     *" << std::endl;
  //   std::cout << "* - Controls -                        *" << std::endl;
  //   std::cout << "*    'Space' -- Takeoff/Landing       *" << std::endl;
  //   std::cout << "*    'Up'    -- Move forward          *" << std::endl;
  //   std::cout << "*    'Down'  -- Move backward         *" << std::endl;
  //   std::cout << "*    'Left'  -- Turn left             *" << std::endl;
  //   std::cout << "*    'Right' -- Turn right            *" << std::endl;
  //   std::cout << "*    'Q'     -- Move upward           *" << std::endl;
  //   std::cout << "*    'A'     -- Move downward         *" << std::endl;
  //   std::cout << "*                                     *" << std::endl;
  //   std::cout << "* - Others -                          *" << std::endl;
  //   std::cout << "*    'C'     -- Change camera         *" << std::endl;
  //   std::cout << "*    'Esc'   -- Exit                  *" << std::endl;
  //   std::cout << "*                                     *" << std::endl;
  //   std::cout << "***************************************" << std::endl;

  //   while (1) {
  //       	// Key input
  //       	int key = cv::waitKey(33);
  //       	if (key == 0x1b) break;

  //       	// Get an image
  //       	Mat ardroneimage = ardrone.getImage();

  //       	// Take off / Landing 
  //       	if (key == ' ') {
  //           	if (ardrone.onGround()) ardrone.takeoff();
  //           	else                    ardrone.landing();
  //       	}

  //       	// Move
  //       	double vx = 0.0, vy = 0.0, vz = 0.0, vr = 0.0;
  //       	if (key == 'i' || key == CV_VK_UP)    vx =  1.0;
  //       	if (key == 'k' || key == CV_VK_DOWN)  vx = -1.0;
  //       	if (key == 'u' || key == CV_VK_LEFT)  vr =  1.0;
  //       	if (key == 'o' || key == CV_VK_RIGHT) vr = -1.0;
  //       	if (key == 'j') vy =  1.0;
  //       	if (key == 'l') vy = -1.0;
  //       	if (key == 'q') vz =  1.0;
  //       	if (key == 'a') vz = -1.0;


  //       	ardrone.move3D(vx, vy, vz, vr);

  //       	// Change camera
  //       	static int mode = 0;
  //       	if (key == 'c') ardrone.setCamera(++mode % 4);


  //       	bool found = false;
		// //store image to matrix
		// //capture.read(webcam);
		// //convert frame from BGR to HSV colorspace
		// cvtColor(ardroneimage, HSV, COLOR_BGR2HSV);
		// //set HSV values from user selected region
		// recordHSV_Values(ardroneimage, HSV);
		// //filter HSV image between values and store filtered image to
		// //threshold matrix
		// inRange(HSV, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), threshold);
		// //perform morphological operations on thresholded image to eliminate noise
		// //and emphasize the filtered object(s)
		// if (useMorphOps)
		// 	morphOps(threshold);
		// //pass in thresholded frame to our object tracking function
		// //this function will return the x and y coordinates of the
		// //filtered object
		// if (trackObjects)
		// 	trackFilteredObject(x, y, threshold, ardroneimage, filteredArea, found);

		// if (found==true && tracking == true) {
		// 	ardrone.landing();
		// }
		
		// setMouseCallback(windowName, clickAndDrag_Rectangle, &ardroneimage);

		// //show frames 
		// if (calibrationMode == true) {
		// 	//create slider bars for HSV filtering
		// 	createTrackbars();
		// 	cv::imshow(windowName1, HSV);
		// 	cv::imshow(windowName2, threshold);
		// }
		// else {
		// 	destroyWindow(windowName1);
		// 	destroyWindow(windowName2);
		// 	destroyWindow(trackbarWindowName);
		// }

		// cv::imshow(windowName, ardroneimage);
		// // sleep(ONE_SECOND);





  //       	// Display the image
  //       	// cv::imshow("camera", ardroneimage);
  //   }

    // See you
    // ardrone.close();

    return 0;

}