#ifndef BASE64_H
#define BASE64_H

#include <string>

class Base64 {
public:
    static std::string encode(const std::string& data);
    static std::string decode(const std::string& encoded);
};

#endif // BASE64_H
