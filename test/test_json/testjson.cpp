#include <iostream>
#include "json.hpp"
#include <vector>
#include <map>
#include <string>

using json = nlohmann::json;
using namespace std;

//json序列化示例1
void func1(){
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "1i si";
    js["msg"] = "hello, what are you doing now?";

    //cout << js <<endl;
    //cout << js.dump(4) <<endl;

    //用于将 JSON 对象序列化成字符串
    //转成字符串，可以通过网络发送
    string sendBuf = js.dump();
    cout << sendBuf.c_str()<<endl;
}

//json序列化代码2
void func2(){
    json js;
    // 直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    js["list"] = vec;

    // 直接序列化一个map容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});
    js["path"] = m;
    cout << js << endl;
}

//json反序列化示例
string func3(){
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "1i si";
    js["msg"] = "hello, what are you doing now?";

    
    string sendBuf = js.dump();
    return sendBuf;
   
}


int main(){
    //func1();
    //func2();
    string recvBuf = func3();
    //数据的反序列化，  json字符串 -》 反序列化 数据对象(看作容器，方便访问)
    json jsbuf = json::parse(recvBuf);
    cout<<jsbuf["msg_type"]<<endl;
    cout<<jsbuf["from"]<<endl;
    cout<<jsbuf["to"]<<endl;
    cout<<jsbuf["msg"]<<endl;

    return 0;
}