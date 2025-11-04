#include "chatserver.hpp"
#include "chatservice.hpp"

#include <signal.h>

using namespace std;



//处理服务器ctrl+c结束后，重置user状态
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv)
{
    const char *ip = "127.0.0.1";
    uint16_t port = 6000;

    // 支持命令行参数: ./ChatServer <ip> <port>
    if (argc >= 2) {
        ip = argv[1];
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
    }

    signal(SIGINT,resetHandler);
    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}