# car_parking
eagleviewer project 

coder:ZXL

本项目编译环境为jetson orin nx,系统安装使用SDKmanager全项安装,包括cuda、deerpstreamer等。需注意为匹配摄像头驱动，jetpack版本必须为5.1.2，L4T版本35.4.1

hesai SKD build
need boost boost::thread

sudo apt-get install libboost-all-dev

need PCL
sudo apt-get install libpcl-dev
sudo apt-get install libeigen3-dev libvtk7-dev

更换package-----HesaiLidar_SDK_2.0-master

编译前需安装PkgConfig gstreamer-1.0 gstreamer-rtsp-server-1.0
