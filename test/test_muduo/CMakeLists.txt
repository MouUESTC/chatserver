
#配置头文件搜索路径
#include_directories()

#配置库文件搜索路径
#link_directories()

#设置需要编译的源文件列表
set(SRC_LIST ./muduo_server.cpp)
#或者把指定路径下的所有源文件放入变量名中SRC_LIST中
#aux_source_directory(. SRC_LIST)

#设置可执行文件最终存储的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#使用指定的源文件向项目添加可执行文件
add_executable(server ${SRC_LIST})

#表示server目标程序需要链接的库
target_link_libraries(server muduo_net muduo_base pthread)