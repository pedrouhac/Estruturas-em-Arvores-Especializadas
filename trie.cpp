#include <iostream>
#include <string>
#include <vector>
#include <cctype>  
#include <fstream>
#include <chrono>
#include <random>
#include <algorithm>

using namespace std;
using namespace std::chrono;

const int TAMANHO_ALFABETO = 26;

// ==========================================
// UTILITÁRIO DE BENCHMARK 
// ==========================================
template<typename Func>
long long cronometrar(Func operacao) {
    auto inicio = high_resolution_clock::now();
    operacao();
    auto fim = high_resolution_clock::now();
    return duration_cast<microseconds>(fim - inicio).count();
}

// ==========================================
// GRAVAÇÃO DO RESULTADO EM CSV 
// ==========================================
void registrarResultadoCSV(const string& nomeArquivo, const string& estrutura,
                            const string& distribuicao, size_t n,
                            long long tempoInsercaoUs, long long tempoBuscaExistenteUs,
                            long long tempoBuscaInexistenteUs, long long tempoRemocaoUs,
                            long long metricaEstrutural) {
    ifstream teste(nomeArquivo);
    bool arquivoJaExiste = teste.good();
    teste.close();

    ofstream csv(nomeArquivo, ios::app);
    if (!csv.is_open()) return;

    if (!arquivoJaExiste) {
        csv << "estrutura,distribuicao,n,tempo_insercao_us,tempo_busca_existente_us,"
               "tempo_busca_inexistente_us,tempo_remocao_us,metrica_estrutural\n";
    }
    csv << estrutura << "," << distribuicao << "," << n << ","
        << tempoInsercaoUs << "," << tempoBuscaExistenteUs << ","
        << tempoBuscaInexistenteUs << "," << tempoRemocaoUs << ","
        << metricaEstrutural << "\n";
    csv.close();
}

// Gera chaves garantidamente ausentes do dicionário, para medir o
// custo de uma busca malsucedida (percorre até não encontrar o
// caractere, o que em geral é mais rápido que uma busca bem-sucedida
// -- vale comparar os dois tempos no relatório).
vector<string> gerarChavesInexistentes(const vector<string>& base) {
    vector<string> inexistentes;
    inexistentes.reserve(base.size());
    for (const string& palavra : base) {
        inexistentes.push_back(palavra + "zzq"); // sufixo que não deve existir no dicionario
    }
    return inexistentes;
}

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
// CONTAGEM DE NÓS
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
// EXPORTAÇÃO VISUAL 
// ==========================================
void gerarDotRecursivo(TrieNode* atual, int meuId, int& proximoId, ofstream& arquivo) {
    for (int i = 0; i < TAMANHO_ALFABETO; i++) {
        if (atual->filhos[i] != nullptr) {
            int idFilho = ++proximoId;
            char letra = i + 'a';
            string estilo = atual->filhos[i]->fimDePalavra
                ? ", style=filled, fillcolor=lightgrey" : "";
            arquivo << "    node" << idFilho << " [label=\"" << letra << "\"" << estilo << "];\n";
            arquivo << "    node" << meuId << " -> node" << idFilho << ";\n";
            gerarDotRecursivo(atual->filhos[i], idFilho, proximoId, arquivo);
        }
    }
}

void exportarGraphviz(TrieNode* raiz, const string& nomeArquivo) {
    ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) return;
    arquivo << "digraph Trie {\n"
            << "    graph [ranksep=0.6, nodesep=0.4];\n"
            << "    node [shape=circle, fontsize=20, width=0.55, fixedsize=true];\n"
            << "    edge [arrowsize=0.7];\n"
            << "    node0 [label=\"\"];\n";
    int id = 0;
    gerarDotRecursivo(raiz, id, id, arquivo);
    arquivo << "}\n";
    arquivo.close();
}

// ==========================================
// LEITURA DO DICIONÁRIO COMPARTILHADO
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

    // --- Estado inicial---
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

    // --- Estado após inserções ---
    exportarGraphviz(raiz, "trie_apos_insercoes.dot");

    cout << "\n--- Teste de Busca ---" << endl;
    cout << "Buscar 'carro': " << (buscar(raiz, "carro") ? "Encontrado" : "Nao encontrado") << endl;
    cout << "Buscar 'car': " << (buscar(raiz, "car") ? "Encontrado" : "Nao encontrado (eh apenas prefixo)") << endl;

    cout << "\n--- Teste de Autocompletar (prefixo 'ca') ---" << endl;
    for (const string& s : autocompletar(raiz, "ca")) {
        cout << "- " << s << endl;
    }

    cout << "\n--- Teste de Remocao ---" << endl;
    cout << "Removendo 'carro'..." << endl;
    remover(raiz, "carro");
    cout << "Buscar 'carro' apos remocao: " << (buscar(raiz, "carro") ? "Encontrado" : "Nao encontrado") << endl;
    cout << "Buscar 'carreta' apos remocao de 'carro': " << (buscar(raiz, "carreta") ? "Encontrado" : "Nao encontrado") << endl;

    // --- Estado após remoção ---
    exportarGraphviz(raiz, "trie_apos_remocao.dot");

    destruirTrie(raiz);

    // ==========================================
    // BENCHMARK
    // ==========================================
    string distribuicao = (argc > 2) ? argv[2] : "padrao";

    TrieNode* raizBench = criarNo();

    long long tempoInsercao = cronometrar([&]() {
        for (const string& palavra : palavras) inserir(raizBench, palavra);
    });

    // Métrica estrutural (custo de memória) capturada logo após a
    // insercao completa 
    long long numNos = contarNos(raizBench);

    // Busca de chaves existentes: reusa o próprio dicionário lido.
    bool encontrouTudo = true;
    long long tempoBuscaExistente = cronometrar([&]() {
        for (const string& palavra : palavras) {
            if (!buscar(raizBench, palavra)) encontrouTudo = false;
        }
    });

    // Busca de chaves garantidamente ausentes (pior caso de busca malsucedida).
    vector<string> chavesInexistentes = gerarChavesInexistentes(palavras);
    bool encontrouAlgumaInexistente = false; // deve continuar "false" ao final (nenhuma deveria existir)
    long long tempoBuscaInexistente = cronometrar([&]() {
        for (const string& chave : chavesInexistentes) {
            if (buscar(raizBench, chave)) encontrouAlgumaInexistente = true;
        }
    });

    // Remoção de todo o conjunto (para medir o custo total de remoção
    // no mesmo volume de dados que foi inserido).
    long long tempoRemocao = cronometrar([&]() {
        for (const string& palavra : palavras) remover(raizBench, palavra);
    });

    cout << "\n--- Benchmark ---" << endl;
    cout << "Insercao: " << tempoInsercao << " us | Busca(existente): " << tempoBuscaExistente
         << " us (todas encontradas: " << (encontrouTudo ? "sim" : "nao") << ")"
         << " | Busca(inexistente): " << tempoBuscaInexistente
         << " us (falso positivo: " << (encontrouAlgumaInexistente ? "sim" : "nao") << ")"
         << " us | Remocao: " << tempoRemocao << " us" << endl;

    registrarResultadoCSV("resultados.csv", "Trie", distribuicao, palavras.size(),
                           tempoInsercao, tempoBuscaExistente, tempoBuscaInexistente,
                           tempoRemocao, numNos);

    destruirTrie(raizBench);
    return 0;
}