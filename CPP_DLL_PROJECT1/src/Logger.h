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




// �Զ����������࣬������ std::streambuf
class CustomOutputStream : public std::streambuf
{
public:
	// ��д overflow ����������������ַ�
	virtual int overflow(int c) override {
		if (c != EOF) {
			// ������ʵ���Զ������������߼�
			// ���罫�ַ����������̨�򱣴浽��־�ļ�
			// ����򵥵����������̨
			std::cout.put(static_cast<char>(c));
		}
		return c;
	}

	int main() {
		// �����Զ�������������
		CustomOutputStream customOutputStream;

		// ����ԭʼ�� std::cout ������
		std::streambuf* oldCoutBuffer = std::cout.rdbuf();

		// �ض��� std::cout ���Զ���������
		std::cout.rdbuf(&customOutputStream);

		// ��ʱ std::cout ����������ض��� customOutputStream

		// ���磬���һЩ����
		std::cout << "This will be redirected!" << std::endl;

		// �ָ�ԭʼ�� std::cout ������
		std::cout.rdbuf(oldCoutBuffer);

		// �˺� std::cout ��������ָ���Ĭ�ϵ�����豸

		return 0;
	}
};