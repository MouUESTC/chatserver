#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp"

#include <vector>
using namespace std; 

// 提供好友信息的操作接口方法
class FriendModel{
public:
  FriendModel();
  // 添加好友信息
  void insert(int userid, int friendid);

  //返回用户的好友列表信息
  vector<User> query(int userid);
};

#endif
