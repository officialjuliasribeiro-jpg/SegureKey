#include "hill.h"

#include <string>
#include <vector>
#include <cctype>
#include <algorithm>

namespace {

constexpr int MOD = 26;

// Matriz-chave da Cifra de Hill
const int K[2][2] = {
    {3, 3},
    {2, 5}
};

// Matriz inversa módulo 26
const int K_INV[2][2] = {
    {15, 17},
    {20, 9}
};

// Normaliza valores para o intervalo 0 a 25
int mod26(int value) {
    return ((value % MOD) + MOD) % MOD;
}

// Mantém somente letras do alfabeto
std::string normalize(const std::string& text) {

    std::string result;

    for (unsigned char c : text) {

        if (std::isalpha(c) && c < 128) {

            char upper = static_cast<char>(std::toupper(c));

            if (upper >= 'A' && upper <= 'Z') {
                result += upper;
            }
        }
    }

    return result;
}

// Aplica uma matriz 2x2 a blocos de duas letras
std::string transform(
    const std::string& text,
    const int matrix[2][2]
) {

    std::string result;

    for (size_t i = 0; i < text.size(); i += 2) {

        int a = text[i] - 'A';
        int b = text[i + 1] - 'A';

        int c1 = mod26(
            matrix[0][0] * a +
            matrix[0][1] * b
        );

        int c2 = mod26(
            matrix[1][0] * a +
            matrix[1][1] * b
        );

        result += static_cast<char>('A' + c1);
        result += static_cast<char>('A' + c2);
    }

    return result;
}

}

namespace Hill {

// CRIPTOGRAFAR
bool encrypt(
    const std::string& message,
    std::string& encrypted,
    std::string& error
) {

    std::string text = normalize(message);

    if (text.empty()) {
        error = "Digite uma mensagem com letras.";
        return false;
    }

    if (text.size() > 5000) {
        error = "Mensagem muito grande.";
        return false;
    }

    // Completa blocos ímpares com X
    if (text.size() % 2 != 0) {
        text += 'X';
    }

    encrypted = transform(text, K);

    return true;
}


// DESCRIPTOGRAFAR
bool decrypt(
    const std::string& message,
    std::string& decrypted,
    std::string& error
) {

    std::string text = normalize(message);

    if (text.empty()) {
        error = "Digite uma mensagem criptografada.";
        return false;
    }

    if (text.size() > 5000) {
        error = "Mensagem muito grande.";
        return false;
    }

    if (text.size() % 2 != 0) {
        error = "A mensagem precisa ter quantidade par de letras.";
        return false;
    }

    decrypted = transform(text, K_INV);

    return true;
}

}