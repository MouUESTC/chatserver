#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/Callbacks.h>
#include <unordered_map>
#include <functional>
#include <mutex>


#include "json.hpp"
#include <muduo/net/TcpConnection.h>
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

//表示处理消息事件的回调函数
using MesHandler = std::function<void(const TcpConnectionPtr &conn,const json &js,Timestamp)>;

//聊天服务器业务类
class ChatService
{
public:
    //获取单例对象的接口函数
    static ChatService* instance();

    //处理登录的业务
    void login(const TcpConnectionPtr &conn,const json &js,Timestamp time);

    //处理注册的业务
    void reg(const TcpConnectionPtr &conn,const json &js,Timestamp time);

    //一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn,const json &js,Timestamp time);

    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn,const json &js,Timestamp time);

    //创建群组业务
    void createGroup(const TcpConnectionPtr &conn,const json &js,Timestamp time);

    //添加群组业务
    void addGroup(const TcpConnectionPtr &conn,const json &js,Timestamp time);

    //群聊业务
    void groupChat(const TcpConnectionPtr &conn,const json &js,Timestamp time);

    //注销业务
    void loginout(const TcpConnectionPtr &conn,const json &js,Timestamp time);

    //获取消息对应的处理器
    MesHandler getHandler(int mesid);

    //处理客户端异常退出
    void ClientCloseException(const TcpConnectionPtr &conn);

    //服务器异常退出，重置用户在线状态
    void reset();

    //处理redis订阅的消息
    void handleRedisSubscribeMessage(int userid, string message);

private:
    ChatService();

    //存储消息id和其对应的业务处理方法
    unordered_map<int,MesHandler> _msgHandlerMap;

    //存储在线用户的id和对应的通信连接,注意线程安全
    unordered_map<int,TcpConnectionPtr> _userConnMap;

    //定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;
    
    //数据操作类对象
    UserModel _userModel;//用户操作
    offlineMessageModel _offlineMessageModel;//离线消息操作
    FriendModel _friendModel;//好友操作
    GroupModel _groupModel;//群组操作

    //redis操作对象
    Redis _redis;
};
#endif