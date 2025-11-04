#include<hiredis/hiredis.h>
#include "redis.hpp"
#include <iostream>
#include<thread>
using namespace std;

Redis::Redis()
    : _publish_context(nullptr), _subscribe_context(nullptr)
{
}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }

    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _publish_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文连接
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _subscribe_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    thread t([&]() {
        observer_channel_message();
    });
    t.detach();

    cout << "connect redis-server success!" << endl;

    return true;
}



// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }

    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }

    return true;
}


bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }

    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }

    return true;
}



// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;

    while (REDIS_OK == redisGetReply(this->_subscribe_context, (void **)&reply))
    {
        // 订阅收到的消息是一个带三元素的数组：["message", "channel", "message_content"]
        if (reply != nullptr && reply->type == REDIS_REPLY_ARRAY && reply->elements >= 3)
        {
            // 检查第1个元素（频道名）和第2个元素（消息内容）是否有效
            if (reply->element[1] != nullptr && reply->element[1]->str != nullptr &&
                reply->element[2] != nullptr && reply->element[2]->str != nullptr)
            {
                // 调用回调函数处理消息
                // 参数1：频道ID,也就是userid（转换为整数）
                // 参数2：消息内容字符串
                _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
            }
        }

        // 释放当前回复对象内存
        freeReplyObject(reply);
    }

    // 线程退出提示
    cerr << ">>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<" << endl;
}


// 初始化消息通知回调函数
// 该函数用于注册一个用户自定义的处理函数，当 Redis 订阅频道收到消息时，
// 会自动调用此回调函数进行处理。
// 
// 参数说明：
//   fn - 一个可调用对象（函数、lambda、std::function 等），
//        接收两个参数：
//          int    channel：消息来源的频道号（整数形式）
//          string message：接收到的具体消息内容
// 
// 使用示例：
//   redis.init_notify_handler([](int ch, string msg) {
//       cout << "收到频道 " << ch << " 的消息: " << msg << endl;
//   });
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;  // 将传入的回调函数保存到成员变量中
}


