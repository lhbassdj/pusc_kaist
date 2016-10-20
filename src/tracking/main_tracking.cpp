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
#include "TrackingNode.h"

int main(int argc, char **argv)
{
	ros::init(argc, argv, "drone_tracking");

	ROS_INFO("Started TUM ArDrone Tracking Node.");

	ImageConverter ic;
	ros::spin();

	return 0;
}