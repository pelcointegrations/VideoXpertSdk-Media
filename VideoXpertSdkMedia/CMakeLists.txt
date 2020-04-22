cmake_minimum_required(VERSION 2.8)
project(VxSdk-Media)

if (CMAKE_SIZEOF_VOID_P MATCHES 8)
  set( ARCH "x64" )
else(CMAKE_SIZEOF_VOID_P MATCHES 8)
  set( ARCH "Win32" )
endif(CMAKE_SIZEOF_VOID_P MATCHES 8)

add_definitions (-D_SCL_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0601 -D_EXPORTING -D_WINDOWS -D_USRDLL /wd4267 /wd4018)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${ARCH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${ARCH})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${ARCH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${ARCH})

file(GLOB LIB_SOURCES Source/*.* Source/GStreamerSourceAndHeader/*.cpp)
if(UNIX)
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*d3d.*$")
endif()
file(GLOB HEADER_FILES Include/*.*)

include_directories(
  ${CMAKE_SOURCE_DIR}/Include/
  ${CMAKE_SOURCE_DIR}/ThirdParty/GStreamer/1.16.2/${ARCH}/gstreamer_devel/include/
  ${CMAKE_SOURCE_DIR}/ThirdParty/GStreamer/1.16.2/${ARCH}/gstreamer_devel/include/glib-2.0
  ${CMAKE_SOURCE_DIR}/ThirdParty/GStreamer/1.16.2/${ARCH}/gstreamer_devel/include/gstreamer-1.0/
  ${CMAKE_SOURCE_DIR}/ThirdParty/GStreamer/1.16.2/${ARCH}/gstreamer_devel/include/libsoup-2.4/
  ${CMAKE_SOURCE_DIR}/ThirdParty/GStreamer/1.16.2/${ARCH}/gstreamer_devel/include/orc-0.4/
  ${CMAKE_SOURCE_DIR}/ThirdParty/GStreamer/1.16.2/${ARCH}/gstreamer_devel/lib/glib-2.0/include/
  ${CMAKE_SOURCE_DIR}/ThirdParty/Boost/1.62.0.0/Include/
  ${CMAKE_SOURCE_DIR}/packages/VideoXpertSdk/build/native/include/
  ${GST_INCLUDE_DIRS}
)

link_directories(
  ${CMAKE_SOURCE_DIR}/ThirdParty/Boost/1.62.0.0/Lib/${ARCH}/
  ${CMAKE_SOURCE_DIR}/ThirdParty/GStreamer/1.16.2/${ARCH}/gstreamer_devel/lib/
  ${CMAKE_SOURCE_DIR}/packages/VideoXpertSdk/build/native/${ARCH}/lib/
)

add_library(${PROJECT_NAME} SHARED ${LIB_SOURCES})

target_link_libraries(${PROJECT_NAME} optimized VxSdk.lib)
target_link_libraries(${PROJECT_NAME} optimized libboost_regex-vc140-mt-1_62.lib)
if (MSVC)
target_link_libraries(${PROJECT_NAME} optimized d3d9.lib)
endif()
target_link_libraries(${PROJECT_NAME} optimized gstaudio-1.0.lib)
target_link_libraries(${PROJECT_NAME} optimized gstreamer-1.0.lib)
target_link_libraries(${PROJECT_NAME} optimized gobject-2.0.lib)
target_link_libraries(${PROJECT_NAME} optimized glib-2.0.lib)
target_link_libraries(${PROJECT_NAME} optimized intl.lib)
target_link_libraries(${PROJECT_NAME} optimized gstvideo-1.0.lib)
target_link_libraries(${PROJECT_NAME} optimized gstrtp-1.0.lib)
target_link_libraries(${PROJECT_NAME} optimized gstbase-1.0.lib)
target_link_libraries(${PROJECT_NAME} optimized gstrtsp-1.0.lib)
target_link_libraries(${PROJECT_NAME} optimized gstnet-1.0.lib)
target_link_libraries(${PROJECT_NAME} optimized gstsdp-1.0.lib)
target_link_libraries(${PROJECT_NAME} optimized gio-2.0.lib)

set_target_properties(${PROJECT_NAME} PROPERTIES DEFINE_SYMBOL VxSdkMedia_EXPORTS)
