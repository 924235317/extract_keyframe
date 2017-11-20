#ifndef _EXTRACT_FRAME_H_
#define _EXTRACT_FRAME_H_

#include <iostream>
#include <vector>

//opencv
#include <opencv2/opencv.hpp>


int extract_keyframes(std::string file_path, std::vector<cv::Mat> &frames);

#endif

