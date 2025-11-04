#include "db.h"

#include<muduo/base/Logging.h>

// 数据库连接配置信息（静态全局变量）
static string server = "127.0.0.1";    // MySQL服务器地址（本地）
static string user = "root";           // 数据库用户名
static string password = "123456";     // 数据库密码
static string dbname = "chat";         // 数据库名称


MySQL::MySQL()
{
    // 初始化MySQL连接句柄，为后续连接做准备
    // mysql_init()分配并初始化一个MYSQL对象
    _conn = mysql_init(nullptr);
}


MySQL::~MySQL()
{
    // 检查连接是否有效
    if (_conn != nullptr)
        mysql_close(_conn);
}

// 连接数据库方法
bool  MySQL::connect()
{
    // 建立到MySQL服务器的实际连接
    // 参数说明：
    // _conn: MySQL连接句柄
    // server: 服务器地址
    // user: 用户名
    // password: 密码
    // dbname: 数据库名
    // 3306: MySQL默认端口号
    // nullptr: Unix套接字（不使用）
    // 0: 客户端标志（使用默认值）
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                    password.c_str(), dbname.c_str(), 3306,
                                    nullptr, 0);
    if (p != nullptr)
    {
        // 设置字符编码为utf8mb4，与数据库编码一致，防止中文乱码
        // C/C++默认的编码字符是ASCII，不设置的话会乱码
        mysql_query(_conn, "set names utf8mb4");
    }
    else{
        LOG_INFO << "connect mysql failed!";
    }
    // 返回连接是否成功（非空指针表示成功）
    return p;
}

// 数据库更新操作方法（用于INSERT、UPDATE、DELETE等SQL语句）
bool  MySQL::update(string sql)
{
    // 执行SQL更新语句
    // mysql_query()成功返回0，失败返回非0
    if (mysql_query(_conn, sql.c_str()))
    {
        // 执行失败时，记录错误日志
        // __FILE__ 是一个预定义宏，表示当前源文件的文件名
        // __LINE__ 表示当前代码所在的行号
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                    << sql << "更新失败!";
        return false;  // 返回失败状态
    }
    return true;  // 返回成功状态
}

// 数据库查询操作方法（用于SELECT等SQL语句）
MYSQL_RES* MySQL::query(string sql)
{
    // 执行SQL查询语句
    if (mysql_query(_conn, sql.c_str()))
    {
        // 查询失败时，记录错误日志
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                    << sql << "查询失败!";
        return nullptr;  // 返回空指针表示失败
    }
    // 返回查询结果集
    // mysql_use_result()从服务器检索结果集
    // 注意：调用者需要使用mysql_free_result()手动释放结果集
    return mysql_use_result(_conn);
}


MYSQL* MySQL::getConnection()
{
    return _conn;
}