#pragma once

#include <functional>
#include <iostream>
using namespace std;


class LogDebug {
public:
	LogDebug() {};

	template<class T>
	LogDebug operator << (T t)
	{
		log(t);
		return *this;
	};

	template<class T>
	void log(T t) {
		cout << t;
	};

private:
	function<void(void*)> m_logCb = nullptr;
};

#include <iostream>
#include <streambuf>




// 自定义的输出流类，派生自 std::streambuf
class CustomOutputStream : public std::streambuf
{
public:
	// 重写 overflow 函数，捕获输出的字符
	virtual int overflow(int c) override {
		if (c != EOF) {
			// 在这里实现自定义的输出处理逻辑
			// 例如将字符输出到控制台或保存到日志文件
			// 这里简单地输出到控制台
			std::cout.put(static_cast<char>(c));
		}
		return c;
	}

	int main() {
		// 创建自定义的输出流对象
		CustomOutputStream customOutputStream;

		// 保存原始的 std::cout 缓冲区
		std::streambuf* oldCoutBuffer = std::cout.rdbuf();

		// 重定向 std::cout 到自定义的输出流
		std::cout.rdbuf(&customOutputStream);

		// 此时 std::cout 的输出将被重定向到 customOutputStream

		// 例如，输出一些内容
		std::cout << "This will be redirected!" << std::endl;

		// 恢复原始的 std::cout 缓冲区
		std::cout.rdbuf(oldCoutBuffer);

		// 此后 std::cout 的输出将恢复到默认的输出设备

		return 0;
	}
};