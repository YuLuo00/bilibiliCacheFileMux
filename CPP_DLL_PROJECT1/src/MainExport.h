#pragma once

#ifdef DLL_EXPORT
#define DLLAPI __declspec(dllexport)
#else
#define DLLAPI __declspec(dllimport)
#endif



#include <string>

class DLLAPI TestExample {
public:
    static bool exportFuncExample(const char* fileName, const char* passwd, char** _err) {

        return true;
    }
    static bool testMainFunc(std::string vedioFile, std::string audioFile, std::string outputPath);
};