 #pragma once

 /**
 *  This file is part of pusc_kaist.
 *
 *  Copyright 2016 Dongjin Kim <wking@kaist.ac.kr> (KAIST)
 *
 *  pusc_kaist is free software: it's based on tum_ardrone from TUM
 *  (Technical University of Munich).
 *  Like tum_ardrone, pusc_kaist is distributed in the hope that
 *  it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pusc_kaist.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRACKINGNODE_H
#define TRACKINGNODE_H

#include "ros/ros.h"
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
// #include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <cv_bridge/cv_bridge.h>
// #include <sensor_msgs/image_encoding.h>
#include <image_transport/image_transport.h>
#include <pthread.h>
#include "std_msgs/Empty.h"

using namespace std;

string intToString(int number);
void on_trackbar(int, void*);
void createTrackbars();
void clickAndDrag_Rectangle(int event, int x, int y, int flags, void* param);
void recordHSV_values(cv::Mat frame, cv::Mat hsv_frame);
void drawObject(int x, int y, cv::Mat &frame);
void morphOps(cv::Mat &thresh);
void trackFilteredObject(int &x, int &y, cv::Mat threshold, cv::Mat &cameraFeed, double &currentArea, bool &found);
void searchForMovement(cv::Mat thresholdImage, cv::Mat &cameraFeed);

class ImageConverter
{
	//Codes are from cv_bridge tutorial.
    ros::NodeHandle nh_;
    image_transport::ImageTransport it_;
    image_transport::Subscriber image_sub_;
    image_transport::Publisher image_pub_;
	ros::Publisher land_pub;
	ros::Subscriber land_sub;

public:
	ImageConverter();
	~ImageConverter();
	void landCb(std_msgs::EmptyConstPtr);
	void imageCb(const sensor_msgs::ImageConstPtr& msg);
};
#endif