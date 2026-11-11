#ifndef HILL_H
#define HILL_H

#include <string>

namespace Hill {

    bool encrypt(
        const std::string& message,
        std::string& encrypted,
        std::string& error
    );

    bool decrypt(
        const std::string& message,
        std::string& decrypted,
        std::string& error
    );

}

#endif