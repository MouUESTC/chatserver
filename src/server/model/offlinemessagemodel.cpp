#include "offlinemessagemodel.hpp"
#include "db.h"
#include <muduo/base/Logging.h>
#include <vector>

using namespace std;
using namespace muduo;

// 构造函数
offlineMessageModel::offlineMessageModel()
{
}

void offlineMessageModel::insert(int userid, string message)
{
    char sql[4096] = {0};
    sprintf(sql, "insert into offlinemessage (userid, message) values (%d, '%s')", userid, message.c_str());
    
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

//删除用户的离线消息
void offlineMessageModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = %d", userid);
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

//查询用户的离线消息
vector<string> offlineMessageModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql,"select message from offlinemessage where userid = %d",userid);
    LOG_INFO << "sql: " << sql;

    vector<string> vec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            while(MYSQL_ROW row = mysql_fetch_row(res))
            {
                vec.push_back(row[0]);
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
}; 