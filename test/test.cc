#include "hesai_lidar_sdk.hpp"

//add for rtsp
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#define DEFAULT_RTSP_PORT "554"

static char *port = (char *) DEFAULT_RTSP_PORT;

static GOptionEntry entries[] = {
  {"port", 'p', 0, G_OPTION_ARG_STRING, &port,
      "Port to listen on (default: " DEFAULT_RTSP_PORT ")", "PORT"},
  {NULL}
};

//add for rtsp----end

uint32_t last_frame_time = 0;
uint32_t cur_frame_time = 0;

template <typename... Args>
std::string string_format(const std::string& format, Args... args) {
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
  if (size_s <= 0) {
    throw std::runtime_error("Error during formatting.");
  }
  auto size = static_cast<size_t>(size_s);
  std::unique_ptr<char[]> buf(new char[size]);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1);
}

//log info, display frame message

void lidarUdpPocketCallback(const UdpFrame_t &udpFrame, double timestamp) {  

    //printf("udp pocket size: %ld, %f\n", udpFrame.size(), timestamp);

    int sockfd;  
    struct sockaddr_in servaddr;  
  
    // 创建UDP套接字  
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {  
        std::cerr << "Socket creation failed" << std::endl;
    }
    memset(&servaddr, 0, sizeof(servaddr));  
  
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(1410);
  
    if(inet_pton(AF_INET, "192.168.10.120", &servaddr.sin_addr) <= 0) {                // address 192.168.10.120
        std::cerr << "Invalid address/ Address not supported" << std::endl;
    }  
    // 要发送的数据  
    int len;
    if(1){
        for(int i = 0; i < udpFrame.size(); i++){
            len = sendto(sockfd, udpFrame[i].buffer, udpFrame[i].packet_len, MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));        
            if (len < 0) {  
                  std::cerr << "udp upload lidar strean, Sendto failed "<< std::endl;
            }
        }
        std::cerr << "send udp pcket size= " << udpFrame.size() << std::endl;
    }
    close(sockfd); 
    
    hesai::lidar::PcapSaver saver;
    if(0){
       std::string filename = string_format("%s/%s/lidar0.pcap",  "./path", "folder_name");
       saver.Save(filename, udpFrame, 2368);
    }
}

// Determines whether the PCAP is finished playing
bool IsPlayEnded(HesaiLidarSdk<LidarPointXYZIRT>& sdk)
{
  return sdk.lidar_ptr_->IsPlayEnded();
}

int main(int argc, char *argv[])
{
#ifndef _MSC_VER
  if (system("sudo sh -c \"echo 562144000 > /proc/sys/net/core/rmem_max\"") == -1) {
    printf("Command execution failed!\n");
  }
#endif
  HesaiLidarSdk<LidarPointXYZIRT> sample;
  DriverParam param;

  // assign param
  // param.decoder_param.enable_packet_loss_tool = true;
  param.input_param.source_type = DATA_FROM_LIDAR;
  param.input_param.pcap_path = "Your pcap file path";
  param.input_param.correction_file_path = "Your correction file path";
  param.input_param.firetimes_path = "Your firetime file path";

  param.input_param.device_ip_address = "192.168.1.201";
  param.input_param.ptc_port = 9347;
  param.input_param.udp_port = 2368;
  param.input_param.host_ip_address = "";
  param.input_param.multicast_ip_address = "";

  //init lidar with param
  sample.Init(param);
  float socket_buffer = 262144000;
  sample.lidar_ptr_->source_->SetSocketBufferSize(socket_buffer);

  //assign callback fuction
  sample.RegRecvCallback(lidarUdpPocketCallback);

  sample.Start();

  // You can select the parameters in while():
  // 1.[IsPlayEnded(sample)]: adds the ability for the PCAP to automatically quit after playing the program
  // 2.[1                  ]: the application will not quit voluntarily
  
// add for rtsp-server
  GMainLoop *loop;
  GstRTSPServer *server;
  GstRTSPMountPoints *mounts;
  GstRTSPMediaFactory *factory;
  GOptionContext *optctx;
  GError *error = NULL;

  optctx = g_option_context_new ("<launch line> - Test RTSP Server");
  g_option_context_add_main_entries (optctx, entries, NULL);
  g_option_context_add_group (optctx, gst_init_get_option_group ());
  if (!g_option_context_parse (optctx, &argc, &argv, &error)) {
    g_printerr ("Error parsing options: %s\n", error->message);
    g_option_context_free (optctx);
    g_clear_error (&error);
    return -1;
  }
  g_option_context_free (optctx);

  loop = g_main_loop_new (NULL, FALSE);

  server = gst_rtsp_server_new ();
  g_object_set (server, "service", port, NULL);

  mounts = gst_rtsp_server_get_mount_points (server);
  factory = gst_rtsp_media_factory_new ();
  gst_rtsp_media_factory_set_launch (factory, "( v4l2src device=/dev/video2 ! videoconvert ! video/x-raw, format=(string)UYVY, width=(int)3840, height=(int)2160 ! nvvidconv ! nvv4l2h264enc maxperf-enable=1 bitrate=8000000 ! h264parse ! rtph264pay name=pay0 pt=96 )");

  gst_rtsp_mount_points_add_factory (mounts, "/stream0", factory);
  g_object_unref (mounts);

  gst_rtsp_server_attach (server, NULL);

  g_print ("stream ready at rtsp://127.0.0.1:%s/stream0\n", port);
  g_main_loop_run (loop);  
//add for rtsp-server ---end

  while (!IsPlayEnded(sample))
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  printf("The PCAP file has been parsed and we will exit the program.\n");
  return 0;
}
