#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"
class User;

class UserModel
{
public:
    UserModel();
    bool insert(User &user);   //插入用户信息
    User query(int id); //查询用户信息
    bool updateState(User &user); //更新用户状态信息

    //重置用户的状态信息
    void resetState();  
};

#endif