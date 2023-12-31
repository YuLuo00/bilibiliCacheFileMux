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
