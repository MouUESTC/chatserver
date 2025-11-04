#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"

using namespace std;

class GroupModel {
public:
    GroupModel();


    // 创建群组
    bool createGroup(Group &group);

    // 添加群组用户
    bool addGroup(int userid, int groupid, string role);

    // 查询用户所在群组信息
    vector<Group> queryGroups(int userid);

    // 查询群组用户信息，返回值是除userid外其他用户的id列表
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif