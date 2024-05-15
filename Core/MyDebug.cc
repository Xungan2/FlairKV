#include "Core/Debug.h"
#include "Core/MyDebug.h"

namespace LogCabin {

#define NO_MYDEBUG

namespace MyDebug {
    std::string log_file;
    std::ofstream fout;
}

void MyDebug_init(const Core::Config& config) {
#ifndef NO_MYDEBUG
    MyDebug::log_file = config.read<std::string>("myDebugFile");

    MyDebug::fout.open(MyDebug::log_file, std::ios::out);
    if (!MyDebug::fout.is_open())
        PANIC("ERROR: cannot open my debug file.");
#endif
}

void MyDebug_write(const char* msg) {
#ifndef NO_MYDEBUG
    if (MyDebug::fout.is_open()) {
        MyDebug::fout<<msg<<std::endl;
        MyDebug::fout.flush();
    }
    else
        std::cout<<msg<<std::endl;
#endif
}

}