#include <iostream>
#include <string>
#include <vector>
#include <cctype>   // Para tolower()
#include <cstdlib>  // Para exit()
#include <fstream>
#include <chrono>

using namespace std;
using namespace std::chrono;

const int TAMANHO_ALFABETO = 26;

// ==========================================
// ANATOMIA DO NÓ (Vetor Estático)
// ==========================================
struct TrieNode {
    TrieNode* filhos[TAMANHO_ALFABETO];
    bool fimDePalavra;
};

TrieNode* criarNo() {
    TrieNode* novoNo = new TrieNode;
    novoNo->fimDePalavra = false;
    for (int i = 0; i < TAMANHO_ALFABETO; i++) {
        novoNo->filhos[i] = nullptr;
    }
    return novoNo;
}

// ==========================================
// NORMALIZAÇÃO E VALIDAÇÃO DE ENTRADA
// ==========================================
string normalizarEValidar(const string& palavra) {
    string normalizada = "";
    for (char c : palavra) {
        char letraMin = tolower(c); // Converte para minúscula
        
        // Verifica se está fora do intervalo 'a' (97) a 'z' (122) da tabela ASCII
        if (letraMin < 'a' || letraMin > 'z') {
            cout << "\n[ERRO FATAL] Caractere invalido detectado: '" << c << "' na palavra \"" << palavra << "\"." << endl;
            cout << "O sistema aceita APENAS letras de A a Z. Nao utilize espacos, numeros ou acentos." << endl;
            exit(EXIT_FAILURE); // Encerra o programa imediatamente
        }
        normalizada += letraMin;
    }
    return normalizada;
}

// ==========================================
// OPERAÇÕES BÁSICAS
// ==========================================
void inserir(TrieNode* raiz, const string& palavraBruta) {
    string palavra = normalizarEValidar(palavraBruta);
    TrieNode* atual = raiz;
    
    for (char c : palavra) {
        int indice = c - 'a';
        if (atual->filhos[indice] == nullptr) {
            atual->filhos[indice] = criarNo();
        }
        atual = atual->filhos[indice];
    }
    atual->fimDePalavra = true;
}

bool buscar(TrieNode* raiz, const string& palavraBruta) {
    string palavra = normalizarEValidar(palavraBruta);
    TrieNode* atual = raiz;
    
    for (char c : palavra) {
        int indice = c - 'a';
        if (atual->filhos[indice] == nullptr) {
            return false;
        }
        atual = atual->filhos[indice];
    }
    return atual->fimDePalavra;
}

// ==========================================
// REMOÇÃO E PREVENÇÃO DE MEMORY LEAK
// ==========================================
bool estaVazio(TrieNode* raiz) {
    for (int i = 0; i < TAMANHO_ALFABETO; i++) {
        if (raiz->filhos[i] != nullptr) return false;
    }
    return true;
}

TrieNode* removerRecursivo(TrieNode* atual, const string& palavra, int profundidade = 0) {
    if (!atual) return nullptr;

    if (profundidade == palavra.length()) {
        if (atual->fimDePalavra) atual->fimDePalavra = false;
        
        if (estaVazio(atual)) {
            delete atual;
            return nullptr;
        }
        return atual;
    }

    int indice = palavra[profundidade] - 'a';
    atual->filhos[indice] = removerRecursivo(atual->filhos[indice], palavra, profundidade + 1);

    if (estaVazio(atual) && !atual->fimDePalavra) {
        delete atual;
        return nullptr;
    }

    return atual;
}

void remover(TrieNode* raiz, const string& palavraBruta) {
    string palavra = normalizarEValidar(palavraBruta);
    removerRecursivo(raiz, palavra, 0);
}

void destruirTrie(TrieNode* atual) {
    if (!atual) return;
    for (int i = 0; i < TAMANHO_ALFABETO; i++) {
        if (atual->filhos[i] != nullptr) {
            destruirTrie(atual->filhos[i]);
        }
    }
    delete atual;
}

// ==========================================
// EXPORTAÇÃO VISUAL (Graphviz / DOT)
// ==========================================
void gerarDotRecursivo(TrieNode* atual, int& idAtual, ofstream& arquivo) {
    int meuId = idAtual;
    if (atual->fimDePalavra) {
        arquivo << "    node" << meuId << " [style=filled, fillcolor=lightgrey];\n";
    }
    for (int i = 0; i < TAMANHO_ALFABETO; i++) {
        if (atual->filhos[i] != nullptr) {
            int idFilho = ++idAtual;
            char letra = i + 'a';
            arquivo << "    node" << meuId << " -> node" << idFilho << " [label=\"" << letra << "\"];\n";
            gerarDotRecursivo(atual->filhos[i], idAtual, arquivo);
        }
    }
}

void exportarGraphviz(TrieNode* raiz, const string& nomeArquivo) {
    ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) return;
    arquivo << "digraph Trie {\n    node [shape=circle];\n";
    int id = 0;
    gerarDotRecursivo(raiz, id, arquivo);
    arquivo << "}\n";
    arquivo.close();
}

// ==========================================
// TESTES PRINCIPAIS
// ==========================================
int main() {
    TrieNode* raiz = criarNo();

    cout << "--- Iniciando Insercoes ---" << endl;
    inserir(raiz, "Carro"); // A funcao ira normalizar para "carro"
    inserir(raiz, "CARRETA"); // A funcao ira normalizar para "carreta"
    inserir(raiz, "casa");
    inserir(raiz, "dado");
    
    exportarGraphviz(raiz, "trie_padronizada.dot");

    cout << "Buscar 'carro': " << (buscar(raiz, "CaRrO") ? "Encontrado" : "Nao Encontrado") << endl;
    
    // O codigo abaixo ira disparar o alerta e encerrar o programa
    cout << "\n--- Testando caractere invalido ---" << endl;
    inserir(raiz, "caminhão"); // O acento 'ã' vai acionar o exit(1)

    destruirTrie(raiz);
    return 0;
}