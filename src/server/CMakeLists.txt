#定义了一个SRC_LIST变量，包含了当前目录下所有的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db SRC_DB_LIST)
aux_source_directory(./model SRC_MODEL_LIST)
aux_source_directory(./redis SRC_REDIS_LIST)


#指定生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${SRC_DB_LIST} ${SRC_MODEL_LIST} ${SRC_REDIS_LIST})


#指定生成可执行文件链接时所需要的依赖的库文件
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient  hiredis pthread)
