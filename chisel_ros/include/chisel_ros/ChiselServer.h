// The MIT License (MIT)
// Copyright (c) 2014 Matthew Klingensmith and Ivan Dryanovski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef CHISELSERVER_H_
#define CHISELSERVER_H_

#include <chisel_msgs/ResetService.h>
#include <chisel_msgs/PauseService.h>
#include <chisel_msgs/SaveMeshService.h>
#include <chisel_msgs/GetAllChunksService.h>

#include <memory>
#include <open_chisel/Chisel.h>
#include <open_chisel/ProjectionIntegrator.h>
#include <open_chisel/camera/PinholeCamera.h>
#include <open_chisel/camera/DepthImage.h>
#include <open_chisel/camera/ColorImage.h>
#include <open_chisel/pointcloud/PointCloud.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/CameraInfo.h>
#include <sensor_msgs/PointCloud2.h>
#include <visualization_msgs/Marker.h>
#include <tf/tf.h>
#include <tf/transform_listener.h>
#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/exact_time.h>
#include <nav_msgs/Odometry.h>
#include <open_chisel/camera/FisheyeCamera.h>

namespace chisel_ros
{

typedef float DepthData;
typedef uint8_t ColorData;

class ChiselServer
{
  public:
    struct RosCameraTopic
    {
        std::string imageTopic;
        std::string transform;
        chisel::FisheyeCamera cameraModel;
        ros::Subscriber imageSubscriber;
        ros::Publisher lastPosePublisher;
        ros::Publisher frustumPublisher;
        chisel::Transform lastPose;
        ros::Time lastImageTimestamp;
        bool gotPose;
        bool gotImage;

        message_filters::Subscriber<sensor_msgs::Image> *sub_image;
        message_filters::Subscriber<nav_msgs::Odometry> *sub_odom;
    };

    struct RosPointCloudTopic
    {
        std::string cloudTopic;
        std::string transform;
        ros::Subscriber cloudSubscriber;
        chisel::Transform lastPose;
        ros::Time lastTimestamp;
        bool gotPose;
        bool gotCloud;
        message_filters::Subscriber<sensor_msgs::PointCloud2> *sub_point_cloud;
    };

    ChiselServer();
    ChiselServer(const ros::NodeHandle &nodeHanlde, int chunkSizeX, int chunkSizeY, int chunkSizeZ,
                 float resolution, bool _calc_mesh,
                 std::string camera_model_file, std::string mask_file, int _number_of_threads);
    virtual ~ChiselServer();

    void AdvertiseServices();

    inline chisel::ChiselPtr GetChiselMap()
    {
        return chiselMap;
    }
    inline void SetChiselMap(const chisel::ChiselPtr value)
    {
        chiselMap = value;
    }

    inline const std::string &GetBaseTransform() const
    {
        return baseTransform;
    }
    inline const std::string &GetMeshTopic() const
    {
        return meshTopic;
    }

    void SetupProjectionIntegrator(chisel::TruncatorPtr truncator, uint16_t weight, bool useCarving, float carvingDist);
    void SetupMeshPublisher(const std::string &meshTopic);
    void SetupGridPublisher(const std::string &gridTopic);
    void SetupChunkBoxPublisher(const std::string &boxTopic);

    void PublishMeshes(const ros::Time &stamp);
    void PublishChunkBoxes();
    void PublishLatestChunkBoxes();

    void OdometryCallback(nav_msgs::OdometryConstPtr odom, int i);
    void DepthImageCallback(sensor_msgs::ImageConstPtr depthImage, int i);
    void ColorImageCallback(sensor_msgs::ImageConstPtr colorImage, int i);

    void SubscribeAll(const std::vector<std::string> &depth_imageTopic,
                      const std::vector<std::string> &color_imageTopic,
                      const std::string &transform, const std::vector<std::string> &odom_topic);
    void CallbackAll_0(
        sensor_msgs::ImageConstPtr depth_image,
        sensor_msgs::ImageConstPtr color_image,
        nav_msgs::OdometryConstPtr odom);
    void CallbackAll_1(
        sensor_msgs::ImageConstPtr depth_image,
        sensor_msgs::ImageConstPtr color_image,
        nav_msgs::OdometryConstPtr odom);
    void CallbackAll_2(
        sensor_msgs::ImageConstPtr depth_image,
        sensor_msgs::ImageConstPtr color_image,
        nav_msgs::OdometryConstPtr odom);

    void SubscribePointCloud(const std::string &topic);
    void PointCloudCallback(sensor_msgs::PointCloud2ConstPtr pointcloud);

    void IntegrateLastDepthImage(int i);
    void IntegrateLastPointCloud();
    void FillMarkerTopicWithMeshes(visualization_msgs::Marker *marker, visualization_msgs::Marker *marker2, const ros::Time &stamp);
    inline void SetBaseTransform(const std::string &frameName)
    {
        baseTransform = frameName;
    }

    inline bool HasNewData()
    {
        return hasNewData;
    }

    inline float GetNearPlaneDist() const
    {
        return nearPlaneDist;
    }
    inline float GetFarPlaneDist() const
    {
        return farPlaneDist;
    }
    inline void SetNearPlaneDist(float dist)
    {
        nearPlaneDist = dist;
    }
    inline void SetFarPlaneDist(float dist)
    {
        farPlaneDist = dist;
        cam.setFarPlane(dist);
    }

    bool Reset(chisel_msgs::ResetService::Request &request, chisel_msgs::ResetService::Response &response);
    bool TogglePaused(chisel_msgs::PauseService::Request &request, chisel_msgs::PauseService::Response &response);
    bool SaveMesh(chisel_msgs::SaveMeshService::Request &request, chisel_msgs::SaveMeshService::Response &response);
    bool GetAllChunks(chisel_msgs::GetAllChunksService::Request &request, chisel_msgs::GetAllChunksService::Response &response);

    inline bool IsPaused()
    {
        return isPaused;
    }
    inline void SetPaused(bool paused)
    {
        isPaused = paused;
    }

    void SetDepthImage(const sensor_msgs::ImageConstPtr &img, int);
    void SetColorImage(const sensor_msgs::ImageConstPtr &img, int);

    boost::mutex mtx;

  protected:
    visualization_msgs::Marker CreateFrustumMarker(const chisel::Frustum &frustum);

    ros::NodeHandle nh;

    typedef message_filters::sync_policies::ExactTime<sensor_msgs::Image,
                                                      sensor_msgs::Image,
                                                      nav_msgs::Odometry> MySyncPolicy;
    std::vector< message_filters::Synchronizer<MySyncPolicy>* > sync;

    chisel::ChiselPtr chiselMap;
    tf::TransformListener transformListener;

    chisel::PointCloudPtr lastPointCloud;
    chisel::ProjectionIntegrator projectionIntegrator;
    std::string baseTransform;
    std::string meshTopic;
    std::string gridTopic;
    std::string chunkBoxTopic;
    ros::Publisher meshPublisher;
    ros::Publisher gridPublisher;
    ros::Publisher chunkBoxPublisher;
    ros::Publisher latestChunkPublisher;
    ros::ServiceServer resetServer;
    ros::ServiceServer pauseServer;
    ros::ServiceServer saveMeshServer;
    ros::ServiceServer getAllChunksServer;

    std::vector<RosCameraTopic> depthCamera, colorCamera;
    chisel::FisheyeCamera cam;

    std::vector<std::shared_ptr<chisel::DepthImage<DepthData>>> lastDepthImage;
    std::vector<std::shared_ptr<chisel::ColorImage<ColorData>>> lastColorImage;

    RosPointCloudTopic pointcloudTopic;
    bool hasNewData;
    float nearPlaneDist;
    float farPlaneDist;
    bool isPaused;
    bool calc_mesh;
    int number_of_threads;
};
typedef std::shared_ptr<ChiselServer> ChiselServerPtr;
typedef std::shared_ptr<const ChiselServer> ChiselServerConstPtr;

} // namespace chisel

#endif // CHISELSERVER_H_
