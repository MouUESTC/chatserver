#include "groupmodel.hpp"
#include "db.h"
#include <muduo/base/Logging.h>

GroupModel::GroupModel()
{
}

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup (groupname, groupdesc) values ('%s', '%s')", group.getName().c_str(), group.getDesc().c_str());
    LOG_INFO << "sql: " << sql;

    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            //获取插入成功的群组id
            //利用mysql_insert_id()获取刚插入群组的自增ID，并设置到group对象中
            group.setId(mysql_insert_id(mysql.getConnection()));
            LOG_INFO << "群组创建成功,ID: " << group.getId();
            return true;
        }
    }
    return false;
}

// 添加群组用户
bool GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser (userid, groupid, grouprole) values (%d, %d, '%s')", userid, groupid, role.c_str());
    LOG_INFO << "sql: " << sql; 

    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

// 查询用户所在群组,和所在群组的用户信息
vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024] = {0};
    
    // 查询用户所在的群组的id、名称和描述
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join groupuser b on a.id = b.groupid where b.userid = %d", userid);
    LOG_INFO << "sql: " << sql;

    MySQL mysql;
    vector<Group> Groupvec;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            while(MYSQL_ROW row = mysql_fetch_row(res))
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                Groupvec.push_back(group);
            }
            mysql_free_result(res);
        }
    }

    for(Group &group : Groupvec)
    {
        // 查询群组中所有用户的id、name、state和role信息
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a inner join groupuser b on a.id = b.userid where b.groupid = %d", group.getId());
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            while(MYSQL_ROW row = mysql_fetch_row(res))
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    
    //结果是包含所在群组信息和所在群组的用户信息的vector<Group>
    return Groupvec;
};


// 查询群组用户信息，返回值是除userid外其他用户的id列表
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid);
    LOG_INFO << "sql: " << sql;

    MySQL mysql;
    vector<int> Useridvec;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            while(MYSQL_ROW row = mysql_fetch_row(res))
            {
                Useridvec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return Useridvec;
};
