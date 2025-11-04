#include "usermodel.hpp"
#include "db.h"
#include "muduo/base/Logging.h"
#include "user.hpp"
#include <mysql/mysql.h>

// 构造函数
UserModel::UserModel()
{
}

// 插入用户
bool UserModel::insert(User &user)
{
    
    char sql[1024] = {0};
    sprintf(sql, "insert into user (name, password, state) values ('%s', '%s', '%s')", user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());
    LOG_INFO << "sql: " << sql;
    
    MySQL mysql;
    if(mysql.connect())
    {
        LOG_INFO << "数据库连接成功";
        if(mysql.update(sql))
        {
            // 获取插入成功的用户主键ID
            user.setId(mysql_insert_id(mysql.getConnection()));
            LOG_INFO << "用户插入成功，ID: " << user.getId();
            return true;
        }
        else
        {
            LOG_ERROR << "SQL执行失败: " << sql;
        }
    }
    else
    {
        LOG_ERROR << "数据库连接失败";
    }
    return false;
}


//查询用户信息
User UserModel::query(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);//查询用户信息
    LOG_INFO << "sql: " << sql;

    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
    return User();
}

//更新用户状态信息
bool UserModel::updateState(User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d", user.getState().c_str(), user.getId());
    LOG_INFO << "sql: " << sql;

    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            LOG_INFO << "用户状态更新成功";
            return true;  
        }
        else
        {
            LOG_ERROR << "SQL执行失败: " << sql;
        }
    }
    return false;
}; 



//重置用户的状态信息
void UserModel::resetState()
{
    char sql[1024] = {0};
    sprintf(sql, "update user set state = 'offline' where state = 'online'");
    LOG_INFO << "sql: " << sql;

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
    else{
        LOG_ERROR << "数据库连接失败";
    }
    
}