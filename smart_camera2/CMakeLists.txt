# vim: syntax=cmake
cmake_minimum_required(VERSION 2.8.11)
add_compile_options(-g -rdynamic -ldl -funwind-tables -fPIC -O0)

set(SMART_CAMERA_SRC
	main.c
	others.c
	tjstal.c
	conf.c
	camera_record.c
	v4l2camera.c
	rga.c
	algorithm.c
    )


# import rknn
set(RKNPU_PATH ${CMAKE_SOURCE_DIR}/../../../../../external/rknpu)
include_directories(${RKNPU_PATH}/rknn/include)

# import mpp
set(RKMPP_PATH ${CMAKE_SOURCE_DIR}/../../../../../external/mpp)
include_directories(${RKMPP_PATH}/inc)
include_directories(${RKMPP_PATH}/osal/inc)
include_directories(${RKMPP_PATH}/utils)

include_directories(./rknn_api)

link_libraries("libv4l2.so")
link_libraries("libjpeg.so")
link_libraries("libm.so")
link_libraries("libdrm.so")
link_libraries("libminigui_ths.so")
link_libraries("libpng12.so")
link_libraries("libpthread.so")
link_libraries("librga.so")
link_libraries("libfreetype.so")
link_libraries("librkisp.so")
link_libraries("librkuvc.so")
link_libraries("librockchip_mpp.so")
link_libraries("librknn_runtime.so")

add_executable(smart_camera ${SMART_CAMERA_SRC} )

install(TARGETS smart_camera RUNTIME DESTINATION bin)
install(FILES camera.conf DESTINATION etc)
install(DIRECTORY model_files/ DESTINATION usr/share/smart_camera/)
