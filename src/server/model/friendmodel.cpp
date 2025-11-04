#include "friendmodel.hpp"
#include <muduo/base/Logging.h>
#include "db.h"

FriendModel::FriendModel()
{
}

//添加好友信息
void FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into friend (user_id, friend_id) values (%d, %d)", userid, friendid);
    LOG_INFO << "sql: " << sql;

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
    else
    {
        LOG_ERROR << "数据库连接失败";
    }
}

//返回用户的好友列表信息
vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on a.id = b.friend_id where b.user_id = %d", userid);
    LOG_INFO << "sql: " << sql;

    MySQL mysql;
    vector<User> vec;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            while(MYSQL_ROW row = mysql_fetch_row(res))
            {
                User user;
                user.setId(atoi(row[0]));    // id
                user.setName(row[1]);        // name
                user.setState(row[2]);       // state
                // 不设置password,好友列表不需要密码
                vec.push_back(user);
            }
            mysql_free_result(res);
            return vec;
        }
        else
        {
            LOG_ERROR << "数据库查询失败";
        }
    }
    return vec;
}