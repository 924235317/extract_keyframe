#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <fstream>
#include <ctime>
#include <pthread.h>

#include "extract_frame.hpp"
#include <opencv2/opencv.hpp>

std::queue<std::string> que;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
bool isSave = true;

void *thread_func(void * arg) {
    clock_t time_start, time_stop;
    int num_decode = 0; 

    while (!que.empty()) {
        std::cout << "============================" << std:: endl;
        std::cout << "thread id: " << pthread_self() << std::endl;
        
        pthread_mutex_lock(&mut);
        std::cout << "thread id: " << pthread_self();
        std::string file_name = que.front();
        que.pop();
        std::cout << " take file: " << file_name << std::endl; 
        pthread_mutex_unlock(&mut);
        
        std::cout << file_name << std::endl;
        std::vector<cv::Mat> frames;

        time_start = clock();
        extract_keyframes(file_name, frames);
        time_stop = clock();
        std::cout << "Extract time: ";
        std::cout << float((time_stop - time_start) / CLOCKS_PER_SEC);
        std::cout << std::endl;

        std::cout << frames.size() << std::endl;
        for (int i = 0; i < frames.size(); i ++) {
            //cv::imshow(std::to_string(pthread_self()).c_str(), frames.at(i));    
            std::string save_name = "./images/"
                                    + std::to_string(pthread_self())
                                    + "_"
                                    + std::to_string(i) + ".jpg";
            if (isSave)
                cv::imwrite(save_name.c_str(), frames.at(i));
            cv::waitKey(1);
        }
        
        //cv::destroyWindow(id.c_str());
        std::cout << "thread id: " << pthread_self() << std::endl;
        std::cout << "-----------------------------" << std:: endl;
        num_decode++;
    }
    std::cout << "***************" << std:: endl;
    std::cout << "thread id: " << pthread_self();
    std::cout << " total decode: " << num_decode << std::endl;
    std::cout << "***************" << std:: endl;
}


int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: ";
        std::cerr << argv[0];
        std::cerr << " /path/to/file/list";
        std::cerr << " /save/picture/flag";
        std::cerr << std::endl;
        return -1;
    }
    
    //load video list
    std::ifstream file_input;
    file_input.open(argv[1]);
    assert(file_input.is_open());
    while(!file_input.eof()) {
        std::string video_name;
        std::getline(file_input, video_name);
        std::cout << "name: " << video_name << std::endl;
        que.push(video_name.c_str());
    }
    
    //save flag
    isSave = bool(argv[2]);
   
    //thread
    pthread_t thread[3];
    for(int i = 0; i < 3; i++) {
        pthread_create(&thread[i], NULL, thread_func,NULL);
    }

    for (int i = 0; i < 3; i++) {
        pthread_join(thread[i], NULL);
    }


    return 0;
}
