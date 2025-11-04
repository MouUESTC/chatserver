#include "chatservice.hpp"
#include "muduo/base/Logging.h"
#include "public.hpp"
#include "user.hpp"
#include <mutex>
#include <vector>

using namespace muduo;

// 获取单例对象的接口函数
ChatService *ChatService::instance() {
  static ChatService service;
  return &service;
};

// 注册消息以及对用的Handler回调操作
ChatService::ChatService() {
  //用户基本业务处理回调函数
  _msgHandlerMap.insert(
      {LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
  _msgHandlerMap.insert(
      {REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
  _msgHandlerMap.insert(
      {ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
  _msgHandlerMap.insert(
      {ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
  _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup,
                                                     this, _1, _2, _3)});
  //群组业务处理回调函数
  _msgHandlerMap.insert(
      {ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
  _msgHandlerMap.insert(
      {GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
  _msgHandlerMap.insert(
      {LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});


  //连接redis服务器
  if(_redis.connect()) {
    //设置上报消息的回调
    _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
  }


}

// 服务器异常退出，重置用户在线状态
void ChatService::reset() {
  // 将online状态的用户，设置为offline
  _userModel.resetState();
};

// 获取消息对应的处理器
MesHandler ChatService::getHandler(int msgid) {
  // 记录错误日志，msgid没有对应的事件处理回调
  auto it = _msgHandlerMap.find(msgid);
  if (it == _msgHandlerMap.end()) {
    // 返回一个默认的处理器，空操作
    return [=](const TcpConnectionPtr &conn, const json &js, Timestamp) {
      // 打印错误信息
      LOG_ERROR << "msgid:" << msgid << "can not find handler";
    };
  } else {

    return _msgHandlerMap[msgid];
  }
};

// 处理登录的业务
void ChatService::login(const TcpConnectionPtr &conn, const json &js,
                        Timestamp time) {
  int id = js["id"].get<int>();
  string password = js["password"].get<string>();

  User user = _userModel.query(id);
  if (user.getId() == id && user.getPassword() == password) {
    if (user.getState() == "online") {
      // 该用户已经在线，不允许重复登录
      json response;
      response["msgid"] = LOGIN_MSG_ACK;
      response["errno"] = 2;
      response["errmsg"] = "该账号已经登录";
      conn->send(response.dump());
    } else {
      // 登录成功，记录用户连接信息
      {
        lock_guard<mutex> lock(_connMutex);

        _userConnMap.insert({id, conn});
      }

      //id用户登录成功后，向redis订阅channel(id)
      //也就是说，当有人给id用户发消息时，redis会向channel(id)发送消息，
      //然后ChatService::handleRedisSubscribeMessage函数会处理这个消息
      _redis.subscribe(id);

      // 登录成功，更新用户状态信息为online
      user.setState("online");
      _userModel.updateState(user);

      json response;
      response["msgid"] = LOGIN_MSG_ACK;
      response["errno"] = 0;
      response["id"] = user.getId();
      response["name"] = user.getName();

      // 查询该用户是否有离线消息
      vector<string> vec = _offlineMessageModel.query(id);
      if (!vec.empty()) {
        response["offlinemsg"] = vec;
        LOG_INFO << "有" << vec.size() << "条离线消息";
        // 读取离线消息后，删除该用户的离线消息
        _offlineMessageModel.remove(id);
      }

      // 查询好友信息
      vector<User> Friendvec = _friendModel.query(id);
      if (!Friendvec.empty()) {
        vector<string> FriendInfovec;
        for (User &user : Friendvec) {
          json js;
          js["id"] = user.getId();
          js["name"] = user.getName();
          js["state"] = user.getState();
          FriendInfovec.push_back(js.dump());
        }
        response["friends"] = FriendInfovec;
      }

      // 查询群组信息
      vector<Group> Groupvec = _groupModel.queryGroups(id);
      if (!Groupvec.empty()) {
        vector<string> GroupInfovec;
        for (Group &group : Groupvec) {
          json js;
          js["id"] = group.getId();
          js["name"] = group.getName();
          js["groupdesc"] = group.getDesc();
          vector<string> uservec;
          for(GroupUser &groupuser : group.getUsers()) {
            json js;
            js["id"] = groupuser.getId();
            js["name"] = groupuser.getName();
            js["state"] = groupuser.getState();
            js["role"] = groupuser.getRole();
            uservec.push_back(js.dump());
          }
          js["users"] = uservec;
          GroupInfovec.push_back(js.dump());
        }
        response["groups"] = GroupInfovec;
      }

      

      conn->send(response.dump());
    }

  } else {
    // 该用户不存在或者密码错误，登录失败
    json response;
    response["msgid"] = LOGIN_MSG_ACK;
    response["errno"] = 1;
    response["errmsg"] = "用户名或者密码错误";
    conn->send(response.dump());
  }
};

// 处理注册的业务
void ChatService::reg(const TcpConnectionPtr &conn, const json &js,
                      Timestamp time) {
  string name = js["name"];
  string password = js["password"];

  User user;
  user.setName(name);
  user.setPassword(password);

  bool state = _userModel.insert(user);
  if (state) {
    // 注册成功
    json response;
    response["msgid"] = REG_MSG_ACK;
    response["errno"] = 0;
    response["id"] = user.getId();

    // .dump()用于将json对象序列化为字符串
    conn->send(response.dump());
  } else {
    // 注册失败
    json response;
    response["msgid"] = REG_MSG_ACK;
    response["errno"] = 1;

    conn->send(response.dump());
  }
};

// 处理客户端异常退出
void ChatService::ClientCloseException(const TcpConnectionPtr &conn) {
  User user;
  {
    lock_guard<mutex> lock(_connMutex);

    for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++) {
      if (it->second == conn) {
        user.setId(it->first);
        // 从map表中删除连接信息
        _userConnMap.erase(it);
        break;
      }
    }
  }

  // 更新数据库中的用户状态信息
  if (user.getId() != -1) // 防止没找到
  {
    user.setState("offline");
    _userModel.updateState(user);
  }
};

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, const json &js,
                          Timestamp time) {
  int toid = js["to"].get<int>();

  {
    // 先检查toid用户是否和我在同一个服务器上在线
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(toid);

    if (it != _userConnMap.end()) {
      // toid在线，转发消息 服务器直接推送
      it->second->send(js.dump());
      
      // 给发送方返回成功响应
      json response;
      response["msgid"] = ONE_CHAT_MSG;
      response["errno"] = 0;
      response["msg"] = "send message to " + to_string(toid) + " success";
      conn->send(response.dump());
      return;
    }
  }

  //查询toid用户是否在其他服务器上在线,(通过redis发布出去)
  User user = _userModel.query(toid);
  if(user.getState() == "online") {
    //toid用户在其他服务器上在线，转发消息 服务器直接推送
    _redis.publish(toid, js.dump());

    // 给发送方返回成功响应
    json response;
    response["msgid"] = ONE_CHAT_MSG;
    response["errno"] = 0;
    response["msg"] = "send message to " + to_string(toid) + "through redis successfully";
    conn->send(response.dump());
    return;
  }

  // toid不在线，存储离线消息
  _offlineMessageModel.insert(toid, js.dump());
  
  // 给发送方返回成功响应（离线消息已存储）
  json response;
  response["msgid"] = ONE_CHAT_MSG;
  response["errno"] = 0;
  response["msg"] = "send message to " + to_string(toid) + " success and message is saved as offline message";
  conn->send(response.dump());
};

// 添加好友业务 msgid userid friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, const json &js,
                            Timestamp time) {
  int userid = js["userid"].get<int>();
  int friendid = js["friendid"].get<int>();

  // 存储好友信息
  _friendModel.insert(userid, friendid);
  
  // 发送响应给客户端
  json response;
  response["msgid"] = ADD_FRIEND_MSG;
  response["errno"] = 0;
  response["errmsg"] = " server has received add friend message successfully";
  conn->send(response.dump());
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, const json &js,
                              Timestamp time) {
  int id = js["id"].get<int>();
  string groupname = js["groupname"].get<string>();
  string groupdesc = js["groupdesc"].get<string>();

  // 存储新创建的群组信息
  Group group;
  group.setName(groupname);
  group.setDesc(groupdesc);

  // 创建群组
  if (_groupModel.createGroup(group)) {
    // 添加群组创建者信息
    _groupModel.addGroup(id, group.getId(), "creator");
  }
}

// 添加群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, const json &js,
                           Timestamp time) {
  int userid = js["userid"].get<int>();
  int groupid = js["groupid"].get<int>();

  _groupModel.addGroup(userid, groupid, "normal");
}

// 群聊业务
void ChatService::groupChat(const TcpConnectionPtr &conn, const json &js,
                            Timestamp time) {
  int userid = js["userid"].get<int>();
  int groupid = js["groupid"].get<int>();


  // 查询群组用户信息
    vector<int> Useridvec = _groupModel.queryGroupUsers(userid, groupid);

    for (int id : Useridvec) 
    {
      lock_guard<mutex> lock(_connMutex);
      auto it = _userConnMap.find(id);
      if (it != _userConnMap.end()) 
      {
          //在线用户，直接转发消息
          it->second->send(js.dump());
      } 
      else 
      {
        User user = _userModel.query(id);
        if(user.getState() == "online") {
          //toid用户在其他服务器上在线，通过redis发布,转发服务器直接推送
          _redis.publish(id, js.dump());
        }
        else {
            //离线用户，存储离线消息
            _offlineMessageModel.insert(id, js.dump());
          }
      }
    }
    
    // 给发送方返回成功响应
    json response;
    response["msgid"] = GROUP_CHAT_MSG;
    response["errno"] = 0;
    response["msg"] = "group message sent successfully";
    conn->send(response.dump());
}


// 注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, const json &js,
                           Timestamp time)
{
  int userid = js["id"].get<int>();

  {
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end()) {
      _userConnMap.erase(it);
    }
  }

  //id用户注销登录后，取消订阅channel(id)
  _redis.unsubscribe(userid);

  // 更新数据库中的用户状态信息
  User user(userid,"","","offline");
  _userModel.updateState(user);
}

// 处理redis订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string message) 
{
  lock_guard<mutex> lock(_connMutex);
  auto it = _userConnMap.find(userid);
  if(it != _userConnMap.end()) {
    //在线用户，直接转发消息
    it->second->send(message);
    return;
  }
  else {
    //离线用户，存储离线消息
    _offlineMessageModel.insert(userid, message);
  }
}