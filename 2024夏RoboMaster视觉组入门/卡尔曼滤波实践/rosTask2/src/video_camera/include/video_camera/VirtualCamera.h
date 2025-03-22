#pragma once
#include <iostream>
#include <opencv2/opencv.hpp>
#include <signal.h>



class VirtualCamera
{
private:
    std::set<std::string> picture_type = {"jpg", "png"};
    std::string video_path;
    std::string cam_name;
    bool is_pitcure;//ture表示要打开图片
    cv::VideoCapture cam;
    bool is_opened;
public:
    VirtualCamera(std::string video_path); 
    ~VirtualCamera(); 
    int open();
    int close();
    cv::Mat getFrame();
};


/**
 * @author: 王占坤
 * @description: 创建一个打开视频的虚拟相机
 * @param: std::string video_path 需要打开的视频路径
 * @return: 
 * @throw: 
 */
VirtualCamera::VirtualCamera(std::string video_path):video_path(video_path)
{
    int idx = video_path.find_last_of('.');
    std::string end_with = video_path.substr(idx + 1);
    if(this->picture_type.find(end_with) != this->picture_type.end())
            this->is_pitcure = true;
    else
    {
        this->is_pitcure = false;
    }
}

VirtualCamera::~VirtualCamera()
{
    this->close();
}

inline
int VirtualCamera::open()
{
    if(!is_pitcure)
    {
        try
        {
            cam.open(video_path);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        
        this->is_opened = cam.isOpened();
    }
    return (this->is_opened ? 0 : -1);
}

inline
int VirtualCamera::close()
{
    is_opened = false;
    this->cam.release();
    return 0;
}


inline
cv::Mat VirtualCamera::getFrame()
{
    if(!this->is_opened)
        this->open();
    cv::Mat frame;
    if(!is_pitcure)
    {
        //尝试获取视频图片，如果视频结束则尝试重置视频
        bool status = cam.read(frame); 
        if (!status) {
            std::cout << "Video ended. Try reset " << std::endl; 
            cam.set(cv::CAP_PROP_POS_AVI_RATIO, 0);
            status = cam.read(frame);
            if(!status)
            {
                std::cout << "Video ended. Reset failed" << std::endl;
                raise(SIGINT);
            }
        }
    }
    else{
        //读取图片
        frame = cv::imread(this->cam_name);
        this->is_opened = false;
        if(frame.empty())
        {
            std::cout << "cannot open picture: " << this->cam_name << std::endl;
            raise(SIGINT);
        }
    }
    return frame;
}
