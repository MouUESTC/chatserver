#include <iostream>
#include "json.hpp"
#include <vector>
#include <map>

using json = nlohmann::json;
using namespace std;

int main() {
    // 创建一个简单的 JSON 对象
    json j;
    j["name"] = "John Doe";
    j["age"] = 30;
    j["is_student"] = false;
    j["subjects"] = {"Math", "Physics", "Computer Science"};

    // 输出 JSON 对象
    cout << "JSON Object:" << endl;
    cout << j.dump(4) << endl;  // 使用 4 个空格进行格式化输出

    // 将 JSON 对象序列化为字符串
    string serialized_json = j.dump();
    cout << "\nSerialized JSON String:" << endl;
    cout << serialized_json << endl;

    // 解析 JSON 字符串
    string json_str = R"({"name":"Alice","age":25,"is_student":true})";
    json parsed_json = json::parse(json_str);

    // 输出解析后的 JSON 对象
    cout << "\nParsed JSON Object:" << endl;
    cout << parsed_json.dump(4) << endl;

    // 访问 JSON 对象中的值
    string name = parsed_json["name"];
    int age = parsed_json["age"];
    bool is_student = parsed_json["is_student"];

    cout << "\nExtracted Values:" << endl;
    cout << "Name: " << name << endl;
    cout << "Age: " << age << endl;
    cout << "Is Student: " << is_student << endl;

    return 0;
}
