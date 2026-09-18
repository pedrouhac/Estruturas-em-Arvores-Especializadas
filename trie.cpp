#include <iostream>
#include <string>
#include <vector>
#include <cctype>   // Para tolower()
#include <fstream>
#include <chrono>

using namespace std;
using namespace std::chrono;

const int TAMANHO_ALFABETO = 26;

// ==========================================
// ANATOMIA DO NÓ (Vetor Estático)
// Decisão de projeto (Fredkin, 1960): cada nó é um "registro" com uma
// célula fixa por símbolo do alfabeto -- acesso O(1) por nível ao custo
// de memória alocada mesmo para letras que não ocorrem.
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
//
// Mudança em relação à versão anterior: em vez de encerrar o programa
// (exit) ao encontrar um caractere fora de 'a'-'z', a função agora
// devolve um status de validade e a palavra inválida é apenas IGNORADA
// (com aviso). Isso é necessário porque o mesmo dicionário será lido por
// inteiro por Trie e Patricia -- se uma palavra travar o programa, as
// duas estruturas deixam de receber exatamente o mesmo conjunto de
// palavras, o que quebraria a comparação exigida no trabalho.
//
// Limitação conhecida (documentar no relatório, Seção 6): o alfabeto
// continua restrito a 'a'-'z' sem acentuação. Um dicionário real em
// português exigiria expandir o alfabeto (ou pré-processar removendo
// acentos), o que impacta diretamente o tamanho do vetor de filhos da
// Trie -- ponto interessante para a análise de custo de memória.
// ==========================================
pair<string, bool> normalizar(const string& palavra) {
    string normalizada = "";
    for (char c : palavra) {
        char letraMin = tolower(static_cast<unsigned char>(c));
        if (letraMin < 'a' || letraMin > 'z') {
            return {palavra, false};
        }
        normalizada += letraMin;
    }
    return {normalizada, true};
}

// ==========================================
// OPERAÇÕES BÁSICAS -- O(k), k = tamanho da chave
// ==========================================
void inserir(TrieNode* raiz, const string& palavraNormalizada) {
    TrieNode* atual = raiz;
    for (char c : palavraNormalizada) {
        int indice = c - 'a';
        if (atual->filhos[indice] == nullptr) {
            atual->filhos[indice] = criarNo();
        }
        atual = atual->filhos[indice];
    }
    atual->fimDePalavra = true;
}

bool buscar(TrieNode* raiz, const string& palavraNormalizada) {
    TrieNode* atual = raiz;
    for (char c : palavraNormalizada) {
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

    if (profundidade == (int)palavra.length()) {
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

void remover(TrieNode*& raiz, const string& palavraNormalizada) {
    raiz = removerRecursivo(raiz, palavraNormalizada, 0);
    if (raiz == nullptr) raiz = criarNo(); // a raiz nunca deve deixar de existir
}

void destruirTrie(TrieNode* atual) {
    if (!atual) return;
    for (int i = 0; i < TAMANHO_ALFABETO; i++) {
        if (atual->filhos[i] != nullptr) destruirTrie(atual->filhos[i]);
    }
    delete atual;
}

// ==========================================
// OPERAÇÃO ESPECÍFICA: AUTOCOMPLETAR (busca por prefixo)
// ==========================================
void coletarPalavras(TrieNode* atual, string prefixoAtual, vector<string>& resultados) {
    if (atual->fimDePalavra) resultados.push_back(prefixoAtual);
    for (int i = 0; i < TAMANHO_ALFABETO; i++) {
        if (atual->filhos[i] != nullptr) {
            coletarPalavras(atual->filhos[i], prefixoAtual + char(i + 'a'), resultados);
        }
    }
}

vector<string> autocompletar(TrieNode* raiz, const string& prefixoNormalizado) {
    TrieNode* atual = raiz;
    vector<string> resultados;
    for (char c : prefixoNormalizado) {
        int indice = c - 'a';
        if (atual->filhos[indice] == nullptr) return resultados;
        atual = atual->filhos[indice];
    }
    coletarPalavras(atual, prefixoNormalizado, resultados);
    return resultados;
}

// ==========================================
// CONTAGEM DE NÓS -- usada na Seção 5 para comparar custo de
// memória (nº de nós instanciados) entre Trie e Patricia com o
// MESMO dicionário de entrada.
// ==========================================
int contarNos(TrieNode* atual) {
    if (!atual) return 0;
    int total = 1;
    for (int i = 0; i < TAMANHO_ALFABETO; i++) {
        total += contarNos(atual->filhos[i]);
    }
    return total;
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
// LEITURA DO DICIONÁRIO COMPARTILHADO
// (o mesmo arquivo "dicionario.txt" é lido também pela Patricia, para
// que as duas estruturas recebam exatamente o mesmo conjunto de
// palavras -- ver Seção 5 do enunciado)
// ==========================================
vector<string> lerDicionario(const string& caminho) {
    vector<string> palavras;
    ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        cerr << "[ERRO] Nao foi possivel abrir o arquivo: " << caminho << endl;
        return palavras;
    }
    string linha;
    while (getline(arquivo, linha)) {
        if (linha.empty()) continue;
        pair<string, bool> resultado = normalizar(linha);
        if (!resultado.second) {
            cerr << "[AVISO] Palavra ignorada (caractere invalido): \"" << linha << "\"" << endl;
            continue;
        }
        palavras.push_back(resultado.first);
    }
    return palavras;
}

// ==========================================
// PROGRAMA PRINCIPAL
// ==========================================
int main(int argc, char* argv[]) {
    string caminhoDicionario = (argc > 1) ? argv[1] : "dicionario.txt";

    TrieNode* raiz = criarNo();

    // --- Estado inicial (Seção 3, item 1) ---
    exportarGraphviz(raiz, "trie_estado_inicial.dot");

    vector<string> palavras = lerDicionario(caminhoDicionario);
    cout << "--- Lidas " << palavras.size() << " palavras de \"" << caminhoDicionario << "\" ---" << endl;

    auto inicio = high_resolution_clock::now();
    for (const string& palavra : palavras) {
        inserir(raiz, palavra);
    }
    auto fim = high_resolution_clock::now();
    auto duracao = duration_cast<microseconds>(fim - inicio);

    cout << "Insercao de " << palavras.size() << " palavras levou " << duracao.count() << " microsegundos." << endl;
    cout << "Numero de nos instanciados na Trie: " << contarNos(raiz) << endl;

    // --- Estado após inserções (Seção 3, item 1) ---
    exportarGraphviz(raiz, "trie_apos_insercoes.dot");

    cout << "\n--- Teste de Busca ---" << endl;
    cout << "Buscar 'carro': " << (buscar(raiz, "carro") ? "Encontrado" : "Nao encontrado") << endl;
    cout << "Buscar 'car': " << (buscar(raiz, "car") ? "Encontrado" : "Nao encontrado (eh apenas prefixo)") << endl;

    cout << "\n--- Teste de Autocompletar (prefixo 'ca') ---" << endl;
    for (const string& s : autocompletar(raiz, "ca")) {
        cout << "- " << s << endl;
    }

    // --- Estado intermediário evidenciando a operação específica ---
    // (o autocompletar não altera a estrutura; o estado relevante para
    // "bifurcação" já está registrado em trie_apos_insercoes.dot, onde
    // o nó "ca" se ramifica em "rro", "rreta", "rta", "sa", "ma", "chorro")

    cout << "\n--- Teste de Remocao ---" << endl;
    cout << "Removendo 'carro'..." << endl;
    remover(raiz, "carro");
    cout << "Buscar 'carro' apos remocao: " << (buscar(raiz, "carro") ? "Encontrado" : "Nao encontrado") << endl;
    cout << "Buscar 'carreta' apos remocao de 'carro': " << (buscar(raiz, "carreta") ? "Encontrado" : "Nao encontrado") << endl;

    // --- Estado após remoção (Seção 3, item 3) ---
    exportarGraphviz(raiz, "trie_apos_remocao.dot");

    destruirTrie(raiz);
    return 0;
}
