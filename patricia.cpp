#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <fstream>
#include <chrono>

using namespace std;
using namespace std::chrono;

// ==========================================================
// ÁRVORE PATRICIA (Radix Tree compacta) -- Abordagem Moderna
//
// Decisão de projeto (Seção 2 do relatório): em vez de seguir a proposta
// original de Morrison (1968), que opera sobre alfabeto estritamente
// binário e usa contagem de bits para decidir os desvios, esta
// implementação adota a "abordagem moderna" de armazenar o FRAGMENTO DE
// STRING (rótulo) diretamente em cada nó. Duas justificativas:
//   1) Facilidade de manipulação: a operação central da estrutura -- o
//      "split" (quebra de um nó em um prefixo comum e dois sufixos) --
//      é muito mais simples de implementar com fatiamento de string
//      (substr) do que com deslocamento de bits.
//   2) Clareza visual: o enunciado exige a geração de representações
//      visuais da estrutura; um nó rotulado "car" que se bifurca em
//      "ro" e "ta" é imediatamente compreensível, enquanto uma árvore
//      binária pura geraria grafos abstratos e pouco didáticos.
//
// Invariante estrutural mantida (Radix Tree): todo nó interno deve ter
// pelo meno 2 filhos -- por isso a remoção inclui uma etapa de
// MESCLAGEM (merge) de nós com um único filho.
// ==========================================================

struct PatriciaNode {
    string rotulo;              // fragmento de chave armazenado neste nó
    bool fimDePalavra;
    vector<PatriciaNode*> filhos;

    PatriciaNode(const string& r, bool fim) : rotulo(r), fimDePalavra(fim) {}
};

PatriciaNode* criarNo(const string& rotulo, bool fim) {
    return new PatriciaNode(rotulo, fim);
}

// ==========================================================
// NORMALIZAÇÃO -- IDÊNTICA à usada na Trie, para garantir que as duas
// estruturas recebam exatamente o mesmo conjunto de palavras a partir
// do mesmo arquivo "dicionario.txt".
// ==========================================================
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

// Tamanho do maior prefixo comum entre duas strings (mecânica de "Mismatch")
int prefixoComum(const string& a, const string& b) {
    int i = 0;
    int limite = min(a.size(), b.size());
    while (i < limite && a[i] == b[i]) i++;
    return i;
}

// ==========================================================
// INSERÇÃO -- O(k), k = tamanho da chave, com possível "split"
// ==========================================================
void inserirRec(PatriciaNode* no, const string& chave) {
    for (PatriciaNode*& filho : no->filhos) {
        int lcp = prefixoComum(filho->rotulo, chave);
        if (lcp == 0) continue; // este filho não compartilha nenhuma letra com a chave

        if (lcp == (int)filho->rotulo.size()) {
            // Rótulo do filho foi totalmente consumido: ou terminamos
            // aqui, ou continuamos descendo por esse filho.
            string resto = chave.substr(lcp);
            if (resto.empty()) {
                filho->fimDePalavra = true;
            } else {
                inserirRec(filho, resto);
            }
            return;
        }

        // Ponto de divergência (mismatch) no MEIO do rótulo do filho:
        // é necessário QUEBRAR o nó (split).
        string comum = filho->rotulo.substr(0, lcp);
        string restoAntigo = filho->rotulo.substr(lcp);
        string restoNovo = chave.substr(lcp);

        PatriciaNode* intermediario = criarNo(comum, restoNovo.empty());
        filho->rotulo = restoAntigo;              // filho antigo encolhe para o sufixo
        intermediario->filhos.push_back(filho);    // e vira filho do novo nó intermediário

        if (!restoNovo.empty()) {
            // a nova palavra diverge no meio -> vira um segundo filho
            intermediario->filhos.push_back(criarNo(restoNovo, true));
        }

        filho = intermediario; // substitui o ponteiro na lista de filhos do pai
        return;
    }

    // Nenhum filho existente compartilha prefixo com a chave: novo ramo folha.
    no->filhos.push_back(criarNo(chave, true));
}

void inserir(PatriciaNode* raiz, const string& palavraNormalizada) {
    if (palavraNormalizada.empty()) return;
    inserirRec(raiz, palavraNormalizada);
}

// ==========================================================
// BUSCA -- O(k)
// ==========================================================
bool buscarRec(PatriciaNode* no, const string& chave) {
    for (PatriciaNode* filho : no->filhos) {
        const string& rot = filho->rotulo;
        if (chave.size() >= rot.size() && chave.compare(0, rot.size(), rot) == 0) {
            string resto = chave.substr(rot.size());
            if (resto.empty()) return filho->fimDePalavra;
            return buscarRec(filho, resto);
        }
    }
    return false;
}

bool buscar(PatriciaNode* raiz, const string& palavraNormalizada) {
    if (palavraNormalizada.empty()) return false;
    return buscarRec(raiz, palavraNormalizada);
}

// ==========================================================
// REMOÇÃO -- O(k), com mesclagem (merge) para preservar a invariante
// "todo nó interno tem pelo menos 2 filhos".
// ==========================================================
void removerRec(PatriciaNode* no, const string& chave) {
    for (size_t i = 0; i < no->filhos.size(); i++) {
        PatriciaNode* filho = no->filhos[i];
        const string& rot = filho->rotulo;
        if (chave.size() < rot.size() || chave.compare(0, rot.size(), rot) != 0) continue;

        string resto = chave.substr(rot.size());
        if (resto.empty()) {
            filho->fimDePalavra = false;
        } else {
            removerRec(filho, resto);
        }

        // Pós-processamento (bottom-up): o nó "filho" precisa ser
        // removido (se virou folha morta) ou mesclado com seu único
        // filho remanescente (para manter a compressão da Patricia).
        if (filho->filhos.empty() && !filho->fimDePalavra) {
            delete filho;
            no->filhos.erase(no->filhos.begin() + i);
        } else if (filho->filhos.size() == 1 && !filho->fimDePalavra) {
            PatriciaNode* neto = filho->filhos[0];
            neto->rotulo = filho->rotulo + neto->rotulo;
            no->filhos[i] = neto;
            delete filho;
        }
        return;
    }
    // chave não encontrada: nenhuma alteração (operação segura/no-op)
}

void remover(PatriciaNode* raiz, const string& palavraNormalizada) {
    if (palavraNormalizada.empty()) return;
    removerRec(raiz, palavraNormalizada);
}

void destruirPatricia(PatriciaNode* no) {
    if (!no) return;
    for (PatriciaNode* filho : no->filhos) destruirPatricia(filho);
    delete no;
}

// ==========================================================
// CONTAGEM DE NÓS -- para comparar, com o MESMO dicionário, o custo de
// memória (nº de nós instanciados) entre Trie e Patricia (Seção 5).
// ==========================================================
int contarNos(PatriciaNode* no) {
    if (!no) return 0;
    int total = 1;
    for (PatriciaNode* filho : no->filhos) total += contarNos(filho);
    return total;
}

// ==========================================================
// EXPORTAÇÃO VISUAL (Graphviz / DOT)
// Cada aresta é rotulada com o FRAGMENTO DE STRING armazenado no nó
// filho -- exatamente o que a anotação de estudo recomendava para
// tornar o grafo legível (ex.: nó "car" bifurcando em "ro" e "ta").
// ==========================================================
void gerarDotRecursivo(PatriciaNode* no, int meuId, int& proximoId, ofstream& arquivo) {
    if (no->fimDePalavra) {
        arquivo << "    node" << meuId << " [style=filled, fillcolor=lightgrey];\n";
    }
    for (PatriciaNode* filho : no->filhos) {
        int idFilho = ++proximoId;
        arquivo << "    node" << meuId << " -> node" << idFilho
                << " [label=\"" << filho->rotulo << "\"];\n";
        gerarDotRecursivo(filho, idFilho, proximoId, arquivo);
    }
}

void exportarGraphviz(PatriciaNode* raiz, const string& nomeArquivo) {
    ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) return;
    arquivo << "digraph Patricia {\n    node [shape=circle];\n    node0 [label=\"\"];\n";
    int id = 0;
    gerarDotRecursivo(raiz, id, id, arquivo);
    arquivo << "}\n";
    arquivo.close();
}

// ==========================================================
// LEITURA DO DICIONÁRIO COMPARTILHADO (idêntica à da Trie)
// ==========================================================
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

// ==========================================================
// PROGRAMA PRINCIPAL
// ==========================================================
int main(int argc, char* argv[]) {
    string caminhoDicionario = (argc > 1) ? argv[1] : "dicionario.txt";

    PatriciaNode* raiz = criarNo("", false); // raiz é um nó "vazio" que só agrupa os ramos

    // --- Estado inicial (Seção 3, item 1) ---
    exportarGraphviz(raiz, "patricia_estado_inicial.dot");

    vector<string> palavras = lerDicionario(caminhoDicionario);
    cout << "--- Lidas " << palavras.size() << " palavras de \"" << caminhoDicionario << "\" ---" << endl;

    auto inicio = high_resolution_clock::now();
    for (const string& palavra : palavras) {
        inserir(raiz, palavra);
    }
    auto fim = high_resolution_clock::now();
    auto duracao = duration_cast<microseconds>(fim - inicio);

    cout << "Insercao de " << palavras.size() << " palavras levou " << duracao.count() << " microsegundos." << endl;
    cout << "Numero de nos instanciados na Patricia: " << contarNos(raiz) << endl;

    // --- Estado após inserções, evidenciando bifurcações/splits (Seção 3, item 2) ---
    exportarGraphviz(raiz, "patricia_apos_insercoes.dot");

    cout << "\n--- Teste de Busca ---" << endl;
    cout << "Buscar 'carro': " << (buscar(raiz, "carro") ? "Encontrado" : "Nao encontrado") << endl;
    cout << "Buscar 'car': " << (buscar(raiz, "car") ? "Encontrado" : "Nao encontrado (eh apenas prefixo)") << endl;

    cout << "\n--- Teste de Remocao (evidenciando mesclagem) ---" << endl;
    cout << "Removendo 'carro'..." << endl;
    remover(raiz, "carro");
    cout << "Buscar 'carro' apos remocao: " << (buscar(raiz, "carro") ? "Encontrado" : "Nao encontrado") << endl;
    cout << "Buscar 'carreta' apos remocao de 'carro': " << (buscar(raiz, "carreta") ? "Encontrado" : "Nao encontrado") << endl;

    // --- Estado após remoção/reorganização (Seção 3, item 3) ---
    exportarGraphviz(raiz, "patricia_apos_remocao.dot");

    destruirPatricia(raiz);
    return 0;
}
