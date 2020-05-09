set(config_SRCS
        ServerConfig.cpp)

set(INC_DIR include)
include_directories(${INC_DIR})

add_library(cuber_config ${config_SRCS})
target_link_libraries(cuber_config yaml-cpp)

