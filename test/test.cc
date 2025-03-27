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

//log info, display frame message
void lidarCallback(const LidarDecodedFrame<LidarPointXYZIRT>  &frame) {  
  cur_frame_time = GetMicroTickCount();
  if (last_frame_time == 0) last_frame_time = GetMicroTickCount();
  if (cur_frame_time - last_frame_time > kMaxTimeInterval) {
    printf("Time between last frame and cur frame is: %d us\n", (cur_frame_time - last_frame_time));
  }
  last_frame_time = cur_frame_time;
  printf("frame:%d points:%u packet:%d start time:%lf end time:%lf\n",frame.frame_index, frame.points_num, frame.packet_num, frame.points[0].timestamp, frame.points[frame.points_num - 1].timestamp) ;
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
  sample.RegRecvCallback(lidarCallback);

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
