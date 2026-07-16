cd /mnt/F1/code/KR_ROOM
export SLINT_FONT_PATH=/mnt/F1/code/KR_ROOM/1000.TTF
source ./lib.sh
cd target/riscv64gc-unknown-linux-gnu/release



开启测试模式:
export KR_ROOM_TEST_CLOUD_IFACE=eth1
./kr_room_ui

关闭测试模式：
unset KR_ROOM_TEST_CLOUD_IFACE
./kr_room_ui

临时切换AIOT平台地址：
export KR_ROOM_AIOT_SERVER_ADDR=43.108.59.118:30007
./kr_room_ui

恢复数据库AIOT平台地址：
unset KR_ROOM_AIOT_SERVER_ADDR
./kr_room_ui


抓取桥接的H264流
export KR_ROOM_DUMP_BRIDGE_H264=1
# 可选
export KR_ROOM_DUMP_BRIDGE_H264_DIR=/tmp/kr_room_bridge_dump
./kr_room_ui

修改网卡MAC：
iplink set dev eth1 address D0:F8:E7:05:BE:B0
iplink set dev eth1 up

WebRTC登录后循环推送测试H264流：
export KR_ROOM_WEBRTC_TEST_H264=1
# 默认读取当前目录 ./bridge_test.h264
# 可选：指定码流文件
export KR_ROOM_WEBRTC_TEST_H264_PATH=./bridge_test.h264
# 可选：指定发送帧率，默认15fps
export KR_ROOM_WEBRTC_TEST_H264_FPS=15
./kr_room_ui


export KR_ROOM_WEBRTC_TEST_H264=1
export KR_ROOM_WEBRTC_TEST_H264_PATH=./bridge_test.h264
export KR_ROOM_WEBRTC_TEST_H264_FPS=15


测试临时服务器： 192.168.253.40  1883或8883
export KR_ROOM_AIOT_SERVER_ADDR=192.168.253.40:1883
./kr_room_ui
