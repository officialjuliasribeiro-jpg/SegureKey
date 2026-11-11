#include "document_crypto.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include <array>
#include <algorithm>
#include <climits>
#include <vector>

namespace DocumentCrypto {

namespace {

    constexpr size_t SALT_SIZE = 16;
    constexpr size_t NONCE_SIZE = 12;
    constexpr size_t TAG_SIZE = 16;
    constexpr size_t KEY_SIZE = 32;
    constexpr size_t HEADER_SIZE =
        4 + SALT_SIZE + NONCE_SIZE + TAG_SIZE;

    constexpr size_t MAX_FILE_SIZE = 15 * 1024 * 1024;

    bool deriveKey(
        const std::string& password,
        const unsigned char* salt,
        unsigned char* key
    ) {
        return EVP_PBE_scrypt(
            password.c_str(),
            password.size(),
            salt,
            SALT_SIZE,
            32768,
            8,
            1,
            64 * 1024 * 1024,
            key,
            KEY_SIZE
        ) == 1;
    }

}

bool encryptFile(
    const std::string& password,
    const std::vector<unsigned char>& original,
    std::vector<unsigned char>& encrypted,
    std::string& error
) {
    if (password.empty()) {
        error = "Senha não informada.";
        return false;
    }

    if (original.empty() || original.size() > MAX_FILE_SIZE ||
        original.size() > static_cast<size_t>(INT_MAX)) {
        error = "Arquivo vazio ou maior que o limite de 15 MB.";
        return false;
    }

    std::array<unsigned char, SALT_SIZE> salt{};
    std::array<unsigned char, NONCE_SIZE> nonce{};
    std::array<unsigned char, TAG_SIZE> tag{};
    std::array<unsigned char, KEY_SIZE> key{};

    if (RAND_bytes(salt.data(), salt.size()) != 1 ||
        RAND_bytes(nonce.data(), nonce.size()) != 1) {
        error = "Não foi possível gerar os dados aleatórios.";
        return false;
    }

    if (!deriveKey(password, salt.data(), key.data())) {
        error = "Falha na derivação da chave.";
        OPENSSL_cleanse(key.data(), key.size());
        return false;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    if (!ctx) {
        error = "Falha ao iniciar a criptografia.";
        OPENSSL_cleanse(key.data(), key.size());
        return false;
    }

    std::vector<unsigned char> ciphertext(original.size() + 16);

    int length = 0;
    int total = 0;

    bool success =
        EVP_EncryptInit_ex(
            ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr
        ) == 1 &&
        EVP_CIPHER_CTX_ctrl(
            ctx, EVP_CTRL_GCM_SET_IVLEN, NONCE_SIZE, nullptr
        ) == 1 &&
        EVP_EncryptInit_ex(
            ctx, nullptr, nullptr, key.data(), nonce.data()
        ) == 1 &&
        EVP_EncryptUpdate(
            ctx,
            ciphertext.data(),
            &length,
            original.data(),
            static_cast<int>(original.size())
        ) == 1;

    if (success) {
        total = length;

        success = EVP_EncryptFinal_ex(
            ctx,
            ciphertext.data() + total,
            &length
        ) == 1;
    }

    if (success) {
        total += length;

        success = EVP_CIPHER_CTX_ctrl(
            ctx,
            EVP_CTRL_GCM_GET_TAG,
            TAG_SIZE,
            tag.data()
        ) == 1;
    }

    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(key.data(), key.size());

    if (!success) {
        error = "Falha durante a criptografia.";
        return false;
    }

    ciphertext.resize(total);

    encrypted.clear();
    encrypted.reserve(HEADER_SIZE + ciphertext.size());

    encrypted.insert(encrypted.end(), {'S', 'K', 'D', '1'});
    encrypted.insert(encrypted.end(), salt.begin(), salt.end());
    encrypted.insert(encrypted.end(), nonce.begin(), nonce.end());
    encrypted.insert(encrypted.end(), tag.begin(), tag.end());
    encrypted.insert(
        encrypted.end(),
        ciphertext.begin(),
        ciphertext.end()
    );

    return true;
}

bool decryptFile(
    const std::string& password,
    const std::vector<unsigned char>& encrypted,
    std::vector<unsigned char>& original,
    std::string& error
) {
    if (password.empty()) {
        error = "Senha não informada.";
        return false;
    }

    if (encrypted.size() <= HEADER_SIZE ||
        encrypted.size() > HEADER_SIZE + MAX_FILE_SIZE) {
        error = "Arquivo criptografado inválido ou fora do limite.";
        return false;
    }

    if (encrypted[0] != 'S' ||
        encrypted[1] != 'K' ||
        encrypted[2] != 'D' ||
        encrypted[3] != '1') {
        error = "Formato de arquivo desconhecido.";
        return false;
    }

    const unsigned char* salt = encrypted.data() + 4;
    const unsigned char* nonce = salt + SALT_SIZE;
    const unsigned char* tag = nonce + NONCE_SIZE;
    const unsigned char* ciphertext = tag + TAG_SIZE;

    size_t ciphertextSize = encrypted.size() - HEADER_SIZE;

    if (ciphertextSize > static_cast<size_t>(INT_MAX)) {
        error = "Arquivo muito grande.";
        return false;
    }

    std::array<unsigned char, KEY_SIZE> key{};

    if (!deriveKey(password, salt, key.data())) {
        error = "Falha na derivação da chave.";
        OPENSSL_cleanse(key.data(), key.size());
        return false;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    if (!ctx) {
        error = "Falha ao iniciar a descriptografia.";
        OPENSSL_cleanse(key.data(), key.size());
        return false;
    }

    std::vector<unsigned char> plaintext(ciphertextSize + 16);

    int length = 0;
    int total = 0;

    bool success =
        EVP_DecryptInit_ex(
            ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr
        ) == 1 &&
        EVP_CIPHER_CTX_ctrl(
            ctx, EVP_CTRL_GCM_SET_IVLEN, NONCE_SIZE, nullptr
        ) == 1 &&
        EVP_DecryptInit_ex(
            ctx, nullptr, nullptr, key.data(), nonce
        ) == 1 &&
        EVP_DecryptUpdate(
            ctx,
            plaintext.data(),
            &length,
            ciphertext,
            static_cast<int>(ciphertextSize)
        ) == 1;

    if (success) {
        total = length;

        success = EVP_CIPHER_CTX_ctrl(
            ctx,
            EVP_CTRL_GCM_SET_TAG,
            TAG_SIZE,
            const_cast<unsigned char*>(tag)
        ) == 1;
    }

    if (success) {
        success = EVP_DecryptFinal_ex(
            ctx,
            plaintext.data() + total,
            &length
        ) == 1;
    }

    if (success) {
        total += length;
        plaintext.resize(total);
        original = std::move(plaintext);
    } else {
        error = "Senha incorreta ou arquivo adulterado.";
        original.clear();
    }

    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(key.data(), key.size());

    return success;
}

}
