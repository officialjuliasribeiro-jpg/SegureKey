#ifndef DOCUMENT_CRYPTO_H
#define DOCUMENT_CRYPTO_H

#include <string>
#include <vector>

namespace DocumentCrypto {

    bool encryptFile(
        const std::string& password,
        const std::vector<unsigned char>& original,
        std::vector<unsigned char>& encrypted,
        std::string& error
    );

    bool decryptFile(
        const std::string& password,
        const std::vector<unsigned char>& encrypted,
        std::vector<unsigned char>& original,
        std::string& error
    );

}

#endif