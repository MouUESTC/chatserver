/**
    muduo网络库给用户提供了两个主要的类
    TcpServer：用于编写服务器程序
    Tcpclient：用于编写客户端程序

    epoll+线程池
    好处：能够把网络I/0的代码和业务代码区分开
                用户的连接和断开    用户的可读写事件
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

#include <iostream>
#include <functional>
#include <string>


using namespace std::placeholders;
using namespace std;
using namespace muduo;
using namespace muduo::net;


class ChatServer
{
public:
    ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg)
            : _server(loop,listenAddr,nameArg),
            _loop(loop)
    {
        //给服务器注册用户连接创建和断开的回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));

        //给用户注册用户读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));

        //设置服务器端的线程  1个IO线程，3个Worker线程
        _server.setThreadNum(4);
    }

    //开启事件循环
    void start(){
        _server.start();
    }

private:
    //专门处理用户的连接创建和断开 
    void onConnection(const TcpConnectionPtr& conn){
        if(conn->connected()){
            cout<<conn->peerAddress().toIpPort()<<" -> " <<
                conn->localAddress().toIpPort()<<"state:online"<<endl;
        }
        else{
            cout<<conn->peerAddress().toIpPort()<<" -> " <<
                conn->localAddress().toIpPort()<<"state:offline"<<endl;
                conn->shutdown();   //close(fd)
                _loop->quit();
        }

    }

    //专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr& conn, //连接
                            Buffer* buffer,        //缓冲区
                            Timestamp time)      //时间戳
    {
        string buf = buffer->retrieveAllAsString();
        cout<<"recv data:"<< buf <<"time"<<time.toString()<<endl;
        conn->send(buf);
    }

    TcpServer _server;
    EventLoop* _loop;
};

int main()
{       
    EventLoop loop;
    InetAddress addr("127.0.0.1",6000);
    ChatServer server(&loop,addr,"ChatServer");

    server.start(); //listenfd  epoll_ctl-> epoll
    loop.loop();  //epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件

    return 0;
}


