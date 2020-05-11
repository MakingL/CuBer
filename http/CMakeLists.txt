set(http_SRCS
        HttpServer.cpp
        HttpResponse.cpp
        HttpContext.cpp
        HttpHandler.cpp
        HttpProxy.cpp
        HttpFilter.cpp
        HttpResponseContext.cpp
        HttpProxyHandler.cpp
        HttpTimer.cpp)

add_library(cuber_http ${http_SRCS})
target_link_libraries(cuber_http cuber_net)
