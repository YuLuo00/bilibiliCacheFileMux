#pragma once
#include "../CPP_DLL_PROJECT1/src/MainExport.h"
#include <msclr/marshal_cppstd.h>

using namespace System::Runtime::InteropServices;
using namespace System;

namespace CppAV {
    public ref class importClassExample {
    public:
        static bool TestMainFunc(String^ vedioFile, String^ audioFile, String^ outputPath) {
            
            std::string _vedio = msclr::interop::marshal_as<std::string>(vedioFile);
            std::string _audio = msclr::interop::marshal_as<std::string>(audioFile);
            std::string _output = msclr::interop::marshal_as<std::string>(outputPath);

            bool ret0 = TestExample::testMainFunc(_vedio, _audio, _output);
            bool ret1 = TestExample::testMainFunc("", _audio, _output);

            return ret0 != ret1;
        }
        static bool importFuncExample(String^ fileName, String^ passwd, [Out] String^% err)
        {
            // Convert managed strings to native strings
            std::string nativeFileName = msclr::interop::marshal_as<std::string>(fileName);
            std::string nativePasswd = msclr::interop::marshal_as<std::string>(passwd);

            // Call the native function
            char* nativeErr;

            bool result = TestExample::exportFuncExample(
                nativeFileName.c_str(), nativePasswd.c_str(), &nativeErr);

            // Convert the native error string to managed string
            err = gcnew String(nativeErr);

            // Free the memory allocated in native code  
            delete[] nativeErr;

            return result;
        }
    };
}