#include "video_camera/VirtualCamera.h"
#include "rclcpp/rclcpp.hpp"
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <std_msgs/msg/float64.h>
#include <cv_bridge/cv_bridge.h>
#include <camera_info_manager/camera_info_manager.hpp>
#include <image_transport/image_transport.hpp>
#include <yaml-cpp/yaml.h>
#include <array>
#include <vector>
namespace video_camera
{
  class CameraNode : public rclcpp::Node
  {
  public:
    // 同步构建video的初始化
    CameraNode(const rclcpp::NodeOptions &options) : Node("camera_node", options)
    {
      RCLCPP_INFO(this->get_logger(), "节点已启动：%s.", this->get_name());
      // 设置参数
      image_width = this->declare_parameter<int>("image_width", 1280);
      image_height = this->declare_parameter<int>("image_height", 1080);
      video_name = this->declare_parameter<std::string>("video_name", "armor_vedio.mp4");
      yaml_name = this->declare_parameter<std::string>("yaml_name", "config/camera.yaml");
      use_sensor_data_qos = this->declare_parameter<bool>("use_sensor_data_qos", false);
      fps = this->declare_parameter<int>("fps", 30);

      RCLCPP_INFO(this->get_logger(), "video_name :%s", video_name.c_str());
      // 初始化相机
      camera = std::make_shared<VirtualCamera>(VirtualCamera(video_name));
      if (camera->open() != 0)
      {
        RCLCPP_INFO(this->get_logger(), "视频读取失败");
        return;
      }

      // 初始化yaml
      YAML::Node yaml = YAML::LoadFile(yaml_name);
      YAML::Node intrinsic_node = yaml["intrinsic"];
      YAML::Node distortion_node = yaml["distortion"];
      intrinsic = intrinsic_node.as<std::array<double, 9>>();
      distortion = distortion_node.as<std::vector<double>>();

      auto qos = use_sensor_data_qos ? rmw_qos_profile_sensor_data : rmw_qos_profile_default;

      // 相机话题发布
      camera_pub_ = image_transport::create_camera_publisher(this, "image_raw", qos);

      // 初始化相机信息
      image_msg_.header.frame_id = "odom";
      image_msg_.encoding = "bgr8";

      camera_info_msg_.width = image_width;
      camera_info_msg_.height = image_height;
      camera_info_msg_.distortion_model = "plumb_bob";
      camera_info_msg_.k = intrinsic;
      camera_info_msg_.d = distortion;
      // camera_info_msg_.r=r;
      // camera_info_msg_.p=p;
      camera_info_msg_.binning_x = 0;
      camera_info_msg_.binning_y = 0;
      camera_info_msg_.header.frame_id = "odom";
      camera_info_msg_.header.stamp = this->now();

      // 设置定时器
      timer_ = this->create_wall_timer(std::chrono::milliseconds(1000 / fps), std::bind(&CameraNode::proformVideo, this));

      // 设置fps参数回调
      fps_param_sub_ = std::make_shared<rclcpp::ParameterEventHandler>(this);
      fps_cb_handle_ = fps_param_sub_->add_parameter_callback("fps", [this](const rclcpp::Parameter &p){
        fps = p.as_int();
        timer_->cancel();
        timer_ = this->create_wall_timer(std::chrono::milliseconds(1000/fps), std::bind(&CameraNode::proformVideo, this)); 
        }
      );

      // 设置视频文件参数回调
      video_param_sub_ = std::make_shared<rclcpp::ParameterEventHandler>(this);
      video_cb_handle_ = video_param_sub_->add_parameter_callback("video_name", [this](const rclcpp::Parameter &p){
        video_name = p.as_string();
        camera = std::make_shared<VirtualCamera>(VirtualCamera(video_name));
        if (camera->open() != 0)
        {
          RCLCPP_INFO(this->get_logger(),"视频读取失败");
          return;
        } 
        }
      );
    }

  private:
    /**
     * @brief 获取图像并拷贝到本地，初始化图形信息
     *
     * @return true
     * @return false
     */
    bool getImage()
    {
      cv::Mat frame = camera->getFrame();
      if (frame.empty())
      {
        RCLCPP_INFO(this->get_logger(), "图片读取失败");
        return false;
      }
      image_msg_.header.frame_id = "odom";
      camera_info_msg_.header.stamp = image_msg_.header.stamp = this->now();
      image_msg_.height = frame.rows;
      image_msg_.width = frame.cols;
      image_msg_.step = frame.cols * frame.elemSize();
      image_msg_.data.resize(image_msg_.step * frame.rows);
      memcpy(&image_msg_.data[0], frame.data, image_msg_.data.size());
      return true;
    }

    /**
     * @brief 发布图像
     *
     */
    void proformVideo()
    {
      if (!getImage())
        rclcpp::shutdown();
      camera_pub_.publish(image_msg_, camera_info_msg_);
    }

    // 相机参数
    int image_width;
    int image_height;
    std::shared_ptr<VirtualCamera> camera;

    // 相机发布相关
    image_transport::CameraPublisher camera_pub_;
    sensor_msgs::msg::Image image_msg_;
    sensor_msgs::msg::CameraInfo camera_info_msg_;

    // 视频参数
    std::string video_name;
    std::shared_ptr<rclcpp::ParameterEventHandler> video_param_sub_;
    std::shared_ptr<rclcpp::ParameterCallbackHandle> video_cb_handle_;

    // yaml文件参数
    std::string yaml_name;
    std::vector<double> distortion;
    std::array<double, 9> intrinsic;

    // FPS 参数
    int fps;
    std::shared_ptr<rclcpp::ParameterEventHandler> fps_param_sub_;
    std::shared_ptr<rclcpp::ParameterCallbackHandle> fps_cb_handle_;

    bool use_sensor_data_qos;

    rclcpp::TimerBase::SharedPtr timer_;
  };
}

#include "rclcpp_components/register_node_macro.hpp"

// Register the component with class_loader.
// This acts as a sort of entry point, allowing the component to be discoverable when its library
// is being loaded into a running process.
RCLCPP_COMPONENTS_REGISTER_NODE(video_camera::CameraNode)