#include <iostream>
#include <string>
#include <vector>
#include <limits>

using namespace std;

struct Registro {
    string titulo;
    string informacao;
};

vector<Registro> cofre;

void cadastrar() {
    Registro novo;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "\n=== NOVO REGISTRO ===\n";

    cout << "Titulo: ";
    getline(cin, novo.titulo);

    cout << "Informacao: ";
    getline(cin, novo.informacao);

    cofre.push_back(novo);

    cout << "\nRegistro cadastrado com sucesso!\n";
}

void listar() {
    cout << "\n=== MEU COFRE ===\n";

    if (cofre.empty()) {
        cout << "Nenhum registro cadastrado.\n";
        return;
    }

    for (int i = 0; i < cofre.size(); i++) {
        cout << "\nRegistro " << i + 1 << endl;
        cout << "Titulo: " << cofre[i].titulo << endl;
        cout << "Informacao: " << cofre[i].informacao << endl;
    }
}

void excluir() {
    listar();

    if (cofre.empty()) {
        return;
    }

    int numero;

    cout << "\nDigite o numero do registro que deseja excluir: ";
    cin >> numero;

    if (numero < 1 || numero > cofre.size()) {
        cout << "Numero invalido.\n";
        return;
    }

    cofre.erase(cofre.begin() + numero - 1);

    cout << "Registro excluido com sucesso!\n";
}

void cofrePessoal() {
    int opcao;

    do {
        cout << "\n===== ORGANIZADOR FINANCEIRO =====\n";
        cout << "1 - Cadastrar informacao\n";
        cout << "2 - Visualizar registros\n";
        cout << "3 - Excluir registro\n";
        cout << "0 - Voltar ao menu principal\n";
        cout << "Escolha: ";

        cin >> opcao;

        switch (opcao) {
            case 1:
                cadastrar();
                break;

            case 2:
                listar();
                break;

            case 3:
                excluir();
                break;

            case 0:
                cout << "Voltando...\n";
                break;

            default:
                cout << "Opcao invalida!\n";
        }

    } while (opcao != 0);
}

int main() {
    int opcao;

    do {
        cout << "\n========== SECUREKEY ==========\n";
        cout << "1 - Organizador financeiro\n";
        cout << "2 - Gerenciador de senhas\n";
        cout << "3 - Cofre financeiro\n";
        cout << "4 - Documentos protegidos\n";
        cout << "5 - Mensagens privadas\n";
        cout << "6 - SecurePay (simulacao)\n";
        cout << "7 - Central de seguranca\n";
        cout << "8 - Laboratorio matematico\n";
        cout << "0 - Sair\n";
        cout << "Escolha uma opcao: ";

        cin >> opcao;

        switch (opcao) {
            case 1:
                cofrePessoal();
                break;

            case 0:
                cout << "Encerrando SecureKey...\n";
                break;

            default:
                cout << "Modulo ainda em desenvolvimento.\n";
        }

    } while (opcao != 0);

    return 0;
}