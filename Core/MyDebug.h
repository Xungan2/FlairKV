#ifndef LOGCABIN_MYDEBUG_MYDEBUG_H
#define LOGCABIN_MYDEBUG_MYDEBUG_H

#include <iostream>
#include <fstream>

#include "Core/Config.h"
#include "Core/Debug.h"

namespace LogCabin {

namespace Core {
namespace StringUtil {
std::string format(const char* format, ...)
    __attribute__((format(printf, 1, 2)));
}
}

void MyDebug_init(const Core::Config& config);
void MyDebug_write(const char* msg);

#define MYLOG_WRITE(_format, ...) do { \
    MyDebug_write( \
        ::LogCabin::Core::StringUtil::format( \
            _format, ##__VA_ARGS__).c_str()); \
} while(0)

}

#endif