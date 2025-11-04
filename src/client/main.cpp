// 标准库头文件
#include <chrono>   // 时间处理
#include <ctime>    // C风格时间函数
#include <iostream> // 输入输出流
#include <string>   // 字符串操作
#include <thread>   // 多线程支持
#include <vector>   // 动态数组
#include <functional> // 函数对象
#include <unordered_map> // 哈希映射



// 网络编程头文件 (Linux Socket API)
#include <arpa/inet.h>  // IP地址转换函数(inet_addr, inet_ntoa等)
#include <netinet/in.h> // Internet地址族(sockaddr_in结构)
#include <sys/socket.h> // Socket编程基础
#include <sys/types.h>  // 系统数据类型定义
#include <unistd.h>     // UNIX标准函数(close, read, write等)

// 项目自定义头文件
#include "group.hpp"  // 群组类定义
#include "json.hpp"   // JSON库(nlohmann/json)
#include "public.hpp" // 公共定义(消息类型枚举等)
#include "user.hpp"   // 用户类定义

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;
// 显示当前登录成功用户的基本信息
void showCurrentUserData();

//控制是否继续显示菜单
bool isMainMenuRunning = false;



// 接收线程
void readTaskHandler(int clientfd);

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clientfd);

// 聊天客户端程序实现,main线程用作发送线程,子线程用作接收线程
int main(int argc, char **argv) {
  if (argc < 3) {
    cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
    exit(-1);
  }

  // 解析通过命令行参数传递的ip和port
  char *ip = argv[1];
  uint16_t port = atoi(argv[2]);

  // 创建client端的socket
  int clientfd = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == clientfd) {
    cerr << "socket create error" << endl;
    exit(-1);
  }

  // 填写client需要连接的server信息ip+port
  sockaddr_in server;
  memset(&server, 0, sizeof(sockaddr_in));

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = inet_addr(ip);

  if (connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)) == -1) {
    cerr << "connect error" << endl;
    close(clientfd);
    exit(-1);
  }

  // main线程用于接收用户输入,负责发送数据
  for (;;) {
    // 显示首页面菜单 登录、注册、退出
    cout << "==========================" << endl;
    cout << "1. login" << endl;
    cout << "2. register" << endl;
    cout << "3. quit" << endl;
    cout << "==========================" << endl;
    cout << "choice:";
    int choice = 0;
    cin >> choice;
    cin.get(); // 读掉缓冲区残留的回车
    
    // 如果输入失败，清除错误状态并丢弃错误输入
    if (cin.fail()) {
      cin.clear(); // 清除错误标志
      cin.ignore(1024, '\n'); // 丢弃错误输入直到换行符
      cerr << "invalid input" << endl;
      continue; // 跳过本次循环，重新显示菜单
    }

    switch (choice) {
    case 1: // login业务
    {
      int id = 0;
      char pwd[50] = {0};
      cout << "userid:";
      cin >> id;
      cin.get(); // 读掉缓冲区残留的回车
      cout << "userpassword:";
      cin.getline(pwd, 50);

      nlohmann::json js;
      js["msgid"] = LOGIN_MSG;
      js["id"] = id;
      js["password"] = pwd;
      string request = js.dump();

      int len = send(clientfd, request.c_str(), request.size() + 1, 0);
      if (len == -1) {
        cerr << "send error" << endl;
        close(clientfd);
        exit(-1);
      } else {

        char buffer[1024] = {0};
        len = recv(clientfd, buffer, 1024, 0);
        if (len == -1) {
          // 接收数据失败
          cerr << "recv error" << endl;
          close(clientfd);
          exit(-1);
        } else {
          nlohmann::json response = nlohmann::json::parse(buffer);
          if (response["errno"] != 0) {
            cerr << "login error" << endl;
            cout << response["errmsg"].get<string>() << endl;
          } else {
            g_currentUser.setId(response["id"].get<int>());
            g_currentUser.setName(response["name"].get<string>());

            // 查询好友列表
            if (response.contains("friends")) {
              vector<string> friendList =
                  response["friends"].get<vector<string>>();
              for (string &str : friendList) {
                nlohmann::json js = nlohmann::json::parse(str);
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"].get<string>());
                user.setState(js["state"].get<string>());
                g_currentUserFriendList.push_back(user);
              }
            }

            // 查询群组列表
            if (response.contains("groups")) {
              vector<string> groupList =
                  response["groups"].get<vector<string>>();
              for (string &str : groupList) {
                // 将群组信息解析出来
                nlohmann::json js = nlohmann::json::parse(str);
                Group group;
                group.setId(js["id"].get<int>());
                group.setName(js["name"].get<string>());
                group.setDesc(js["groupdesc"].get<string>());

                // 将群组用户信息解析出来
                vector<string> userList = js["users"].get<vector<string>>();
                for (string &str : userList) {
                  nlohmann::json js = nlohmann::json::parse(str);
                  GroupUser groupuser;
                  groupuser.setId(js["id"].get<int>());
                  groupuser.setName(js["name"].get<string>());
                  groupuser.setState(js["state"].get<string>());
                  groupuser.setRole(js["role"].get<string>());
                  group.getUsers().push_back(groupuser);
                }
                g_currentUserGroupList.push_back(group);
              }
            }

            // 显示当前登录用户的基本信息
            showCurrentUserData();

            // 显示当前用户的离线消息（区分一对一聊天和群聊消息）
            cout<<"======================offlinemsg======================"<<endl;
            if (response.contains("offlinemsg")) {
              vector<string> offlinemsg =
                  response["offlinemsg"].get<vector<string>>();
              for (string &str : offlinemsg) 
              {
                nlohmann::json js = nlohmann::json::parse(str);
                if(js["msgid"] == ONE_CHAT_MSG)
                {
                  cout << js["time"] << "[" << js["id"].get<int>() << "]" << js["name"].get<string>().c_str() 
                       << "said:" << js["msg"].get<string>().c_str() << endl;
                }
                else if(js["msgid"] == GROUP_CHAT_MSG)
                {
                  cout << "群消息[" << js["groupid"] << "] " << js["time"] << " [" << js["userid"].get<int>() << "]" << js["name"].get<string>()
                       << " said:" << js["msg"].get<string>() << endl;
                }
              }
            } 
            cout<<endl;

            //  登录成功,启动接收线程
            //  创建一个线程用于处理读取任务,只创建一个线程,避免重复创建线程
            static int thread_cnt = 0;
            if(thread_cnt == 0)
            {
              thread readtask_thread(readTaskHandler, clientfd);
              readtask_thread.detach();
              thread_cnt++;
            }

            // 进入聊天主菜单页面
            isMainMenuRunning = true;
            mainMenu(clientfd);
          }
        }
      }
    } break;

    case 2: // register业务
    {
      // 获取用户名和密码
      char name[50] = {0};
      char pwd[50] = {0};

      cout << "name:";
      cin.getline(name, 50);
      cout << "password:";
      cin.getline(pwd, 50);

      nlohmann::json js;
      js["msgid"] = REG_MSG;
      js["name"] = name;
      js["password"] = pwd;
      string request = js.dump();

      int len = send(clientfd, request.c_str(), request.size(), 0);
      if (len == -1) {
        cerr << "send error" << endl;
        close(clientfd);
        exit(-1);
      } else {
        char buffer[1024] = {0};
        len = recv(clientfd, buffer, 1024, 0);
        if (len == -1) {
          cerr << "recv error" << endl;
          close(clientfd);
          exit(-1);
        } else {
          // parse函数的作用是将接收到的JSON格式字符串buffer解析为json对象response,方便后续通过json对象访问数据字段
          nlohmann::json response = nlohmann::json::parse(buffer);
          if (response["msgid"] == REG_MSG_ACK) {
            if (response["errno"] == 0) {
              cout << "注册成功" << endl;
            } else {
              cout << "注册失败" << endl;
              cout << response["errmsg"].get<string>() << endl;
            }
          } else {
            cerr << "recv error" << endl;
            close(clientfd);
            exit(-1);
          }
        }
      }

    } break;
    case 3: // quit业务
    {
      close(clientfd);
      exit(0);
    }
    default: {
      cerr << "invalid input" << endl;
      break;
    }
    }
  }

  return 0;
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData() {
  cout << "======================login user======================" << endl;
  cout << "current login user => id:" << g_currentUser.getId()
       << " name:" << g_currentUser.getName().c_str() << endl;
  cout << "----------------------friend list---------------------" << endl;
  if (!g_currentUserFriendList.empty()) {
    for (User &user : g_currentUserFriendList) {
      cout << user.getId() << " " << user.getName() << " " << user.getState()
           << endl;
    }
  }
  cout << "----------------------group list----------------------" << endl;
  if (!g_currentUserGroupList.empty()) {
    for (Group &group : g_currentUserGroupList) {
      cout << group.getId() << " " << group.getName() << " " << group.getDesc()
           << endl;
      for (GroupUser &user : group.getUsers()) {
        cout << "  " << user.getId() << " " << user.getName() << " " << user.getState()
             << " " << user.getRole() << endl;
      }
    }
  }
  cout << "======================================================" << endl;
}

// 接收线程
void readTaskHandler(int clientfd)
{
  
    for(;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if(len == -1 || len == 0)
        {
            // 连接断开或出错，退出线程
            close(clientfd);
            exit(-1);
        }
        else
        {
            nlohmann::json response = nlohmann::json::parse(buffer);
            // 如果收到一对一聊天消息,直接显示信息
            if(response["msgid"] == ONE_CHAT_MSG)
            {
                // 检查是否是服务器的确认响应（包含errno字段）
                if(response.contains("errno"))
                {
                    // 这是服务器返回的发送确认，不需要显示
                    continue;
                }
                // 这是真正的聊天消息，显示出来
                cout << response["time"] << "[" << response["id"] << "]" << response["name"].get<string>().c_str()
                     << "said:" << response["msg"].get<string>().c_str() << endl;
                continue;
            }
            // 处理添加好友响应
            if(response["msgid"] == ADD_FRIEND_MSG)
            {
                if(response["errno"].get<int>() == 0)
                {
                    cout << "添加好友成功！" << endl;
                }
                else
                {
                    cout << "添加好友失败：" << response["errmsg"].get<string>() << endl;
                }
                continue;
            }

            //处理添加群组
            if(response["msgid"] == ADD_GROUP_MSG)
            {
                if(response["errno"].get<int>() == 0)
                {
                    cout << "添加群组成功！" << endl;
                }
            }

            //创建群组响应
            if(response["msgid"] == CREATE_GROUP_MSG)
            {
                if(response["errno"].get<int>() == 0)
                {
                    cout << "创建群组成功！" << endl;
                }
            }

            //处理群聊响应
            if(response["msgid"] == GROUP_CHAT_MSG)
            {
                // 检查是否是服务器的确认响应（包含errno字段）
                if(response.contains("errno"))
                {
                    // 这是服务器返回的发送确认，不需要显示
                    continue;
                }
                // 这是真正的群聊消息，显示出来
                cout << "群消息[" << response["groupid"] << "]" << response["time"] 
                     << " [" << response["userid"] << "]" << response["name"].get<string>()
                     << " said:" << response["msg"].get<string>() << endl;
                continue;
            }
        }
    }
    // 接收线程正常退出
    cout << "[系统提示] 接收线程已退出" << endl;
};

// "help" 命令处理函数
void help(int fd = 0, string str = "");
// "chat" 命令处理函数
void chat(int, string);
// "addfriend" 命令处理函数
void addfriend(int, string);
// "creategroup" 命令处理函数
void creategroup(int, string);
// "addgroup" 命令处理函数
void addgroup(int, string);
// "groupchat" 命令处理函数
void groupchat(int, string);
// "quit" 命令处理函数
void loginout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令,格式help"},
    {"chat", "一对一聊天,格式chat:friendid:message"},
    {"addfriend", "添加好友,格式addfriend:friendid"},
    {"creategroup", "创建群组,格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组,格式addgroup:groupid"},
    {"groupchat", "群聊,格式groupchat:groupid:message"},
    {"loginout", "注销,格式loginout"}};

// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};


// 主聊天页面程序
void mainMenu(int clientfd)
{
    help();


    char buffer[1024] = {0};
    while(isMainMenuRunning)
    {
      cin.getline(buffer,1024);
      string commandbuf(buffer);
      string command;// 命令

      int idx = commandbuf.find(':');// 查找:
      if(idx == -1)// 没找到:
      {
        command = commandbuf;
      }
      else{
        // 提取命令和参数
        command = commandbuf.substr(0,idx);
      }

      // 查找命令处理函数
      auto it = commandHandlerMap.find(command);
      if(it == commandHandlerMap.end())
      {
        cerr<< "invaild command!"<<endl;
        continue;
      } 

      // 调用相应命令处理函数
      it->second(clientfd,commandbuf.substr(idx + 1));
    }
};




// "help" 命令处理函数
void help(int, string)
{
    cout<<"======================help======================"<<endl;
    cout<<"show command list"<<endl;
    // 遍历commandMap,打印命令列表
    for(auto & p:commandMap){
      cout<<p.first<<":"<<p.second<<endl;
    }
    cout<<endl;
};

// "chat" 命令处理函数
void chat(int clientfd, string str)
{
    int idx = str.find(':');

    if(idx == -1)
    {
      cerr<<"invaild chat command!"<<endl;
    }
    else {
    {
      int friendid = atoi(str.substr(0,idx).c_str());
      string msg = str.substr(idx+1);

      nlohmann::json js;
      js["msgid"] = ONE_CHAT_MSG;
      js["id"] = g_currentUser.getId();
      js["name"] = g_currentUser.getName();
      js["to"] = friendid;
      js["msg"] = msg;
      js["time"] = getCurrentTime();
      string request = js.dump();


      int len = send(clientfd, request.c_str(), request.size() + 1, 0);
      if(len == -1)
      {
        cerr<< "send error"<<endl;
      }
      else {
      {
        cout<< "client send chat message success!"<<endl;
      }
      }
    }
    }
};

// "addfriend" 命令处理函数
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    nlohmann::json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["userid"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string request = js.dump();
    
    int len = send(clientfd, request.c_str(), request.size() + 1, 0);
    if(len == -1)
    {
        cerr<< "send error"<<endl;
        return;
    }
    else
    {
        cout<< "client send addfriend message success!"<<endl;
    }
};

// "creategroup" 命令处理函数
void creategroup(int clientfd, string str) {
  int idx = str.find(':');
  if (idx == -1) {
    cerr << "invaild creategroup command!" << endl;
    return;
  } else {
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1);

    nlohmann::json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string request = js.dump();

    int len = send(clientfd,request.c_str(),request.size() + 1 ,0);
    if(len == -1)
    {
      cerr<< "send error"<<endl;
      return;
    }
    else
    {
      cout<<"client send creatre group message success!"<<endl;
    }
  }
};

// "addgroup" 命令处理函数
void addgroup(int clientfd, string str)
{
    int userid = g_currentUser.getId();
    int groupid = atoi(str.c_str());

    nlohmann::json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["userid"] = userid;
    js["groupid"] = groupid;
    string request = js.dump();

    int len = send(clientfd,request.c_str(),request.size()+1,0);
    if(len == -1)
    {
      cerr<<"client send addgroup message error!"<<endl;
      return;
    }
    else{
      cout<<"client send addgroup message success!"<<endl;
    }
};

// "groupchat" 命令处理函数
void groupchat(int clientfd, string str)
{
    int idx = str.find(':');
    if(idx == -1)
    {
      cerr<<"invaild groupchat command!"<<endl;
      return;
    }
    else{
      int groupid = atoi(str.substr(0,idx).c_str());
      string msg = str.substr(idx+1);

      nlohmann::json js;
      js["msgid"] = GROUP_CHAT_MSG;
      js["userid"] = g_currentUser.getId();
      js["name"] = g_currentUser.getName();
      js["groupid"] = groupid;
      js["msg"] = msg;
      js["time"] = getCurrentTime();
      string request = js.dump();

      int len = send(clientfd,request.c_str(),request.size()+1,0);
      if(len == -1)
      {
        cerr<<"client send groupchat message error!"<<endl;
        return;
      }
      else{
        cout<<"client send groupchat message success!"<<endl;
      }
    }
};

// "quit" 命令处理函数
void loginout(int clientfd, string str)
{
    nlohmann::json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string request = js.dump();
    int len = send(clientfd,request.c_str(),request.size()+1,0);
    if(len == -1)
    {
      cerr<<"client send loginout message error!"<<endl;
      return;
    }
    else{
      isMainMenuRunning = false;
    }
    

};

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto now = chrono::system_clock::now();
    auto time = chrono::system_clock::to_time_t(now);
    struct tm *ptm = localtime(&time);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return string(date);
};