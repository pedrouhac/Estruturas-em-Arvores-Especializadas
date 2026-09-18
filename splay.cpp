#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <chrono>

using namespace std;
using namespace std::chrono;

// ==========================================================
// ÁRVORE SPLAY (Self-Adjusting Binary Search Tree)
// Baseada no artigo "Self-Adjusting Binary Search Trees"
// (Sleator & Tarjan, 1985).
//
// Propriedade central (conforme suas anotações): a Splay NÃO se
// preocupa em manter-se balanceada por invariantes estruturais (como a
// AVL faz com o fator de balanceamento). Em vez disso, ela reorganiza
// a árvore a CADA acesso, trazendo o elemento acessado até a raiz por
// meio de uma sequência de rotações chamada "splaying". A garantia de
// eficiência é apenas AMORTIZADA: O(log n) por operação em média ao
// longo de uma sequência, mesmo que uma operação isolada custe O(n).
// ==========================================================

struct SplayNode {
    int chave;
    SplayNode* esquerda;
    SplayNode* direita;
    SplayNode* pai;
};

SplayNode* criarNo(int chave) {
    SplayNode* no = new SplayNode;
    no->chave = chave;
    no->esquerda = no->direita = no->pai = nullptr;
    return no;
}

// ==========================================================
// ROTAÇÕES BÁSICAS (mesmas rotações da AVL, mas aqui usadas para
// REORGANIZAR por acesso, e não para corrigir um fator de balanceamento)
// ==========================================================
void rotacionarDireita(SplayNode*& raiz, SplayNode* x) {
    SplayNode* y = x->esquerda;
    x->esquerda = y->direita;
    if (y->direita) y->direita->pai = x;
    y->pai = x->pai;
    if (!x->pai) raiz = y;
    else if (x == x->pai->direita) x->pai->direita = y;
    else x->pai->esquerda = y;
    y->direita = x;
    x->pai = y;
}

void rotacionarEsquerda(SplayNode*& raiz, SplayNode* x) {
    SplayNode* y = x->direita;
    x->direita = y->esquerda;
    if (y->esquerda) y->esquerda->pai = x;
    y->pai = x->pai;
    if (!x->pai) raiz = y;
    else if (x == x->pai->esquerda) x->pai->esquerda = y;
    else x->pai->direita = y;
    y->esquerda = x;
    x->pai = y;
}

// ==========================================================
// SPLAYING -- traz o nó "x" até a raiz por meio dos 3 casos clássicos:
//   Caso 1 (Zig)     : pai de x é a raiz -> 1 rotação, encerra o processo.
//   Caso 2 (Zig-Zig) : x e seu pai são "do mesmo lado" do avô
//                      (ambos filhos esquerdos ou ambos direitos) ->
//                      rotaciona primeiro o avô, depois o pai.
//   Caso 3 (Zig-Zag) : x e seu pai estão em lados opostos do avô
//                      (padrão "boomerang") -> rotaciona o pai, depois
//                      o avô.
// ==========================================================
void splay(SplayNode*& raiz, SplayNode* x) {
    while (x->pai != nullptr) {
        SplayNode* pai = x->pai;
        SplayNode* avo = pai->pai;

        if (!avo) {
            // Caso 1: Zig (terminal -- só existem 2 níveis até a raiz)
            if (x == pai->esquerda) rotacionarDireita(raiz, pai);
            else rotacionarEsquerda(raiz, pai);
        } else if (x == pai->esquerda && pai == avo->esquerda) {
            // Caso 2: Zig-Zig (esquerda-esquerda)
            rotacionarDireita(raiz, avo);
            rotacionarDireita(raiz, pai);
        } else if (x == pai->direita && pai == avo->direita) {
            // Caso 2: Zig-Zig (direita-direita)
            rotacionarEsquerda(raiz, avo);
            rotacionarEsquerda(raiz, pai);
        } else if (x == pai->direita && pai == avo->esquerda) {
            // Caso 3: Zig-Zag (esquerda-direita, "boomerang")
            rotacionarEsquerda(raiz, pai);
            rotacionarDireita(raiz, avo);
        } else {
            // Caso 3: Zig-Zag (direita-esquerda, "boomerang")
            rotacionarDireita(raiz, pai);
            rotacionarEsquerda(raiz, avo);
        }
    }
}

// ==========================================================
// BUSCA -- O(log n) amortizado.
// Regra do artigo: se a chave FOR encontrada, faz-se o splay do nó
// encontrado. Se NÃO for encontrada, faz-se o splay do último nó
// visitado (o pai da posição de folha onde a busca terminou).
// ==========================================================
bool buscar(SplayNode*& raiz, int chave) {
    SplayNode* atual = raiz;
    SplayNode* ultimoVisitado = nullptr;

    while (atual) {
        ultimoVisitado = atual;
        if (chave == atual->chave) {
            splay(raiz, atual);
            return true;
        }
        atual = (chave < atual->chave) ? atual->esquerda : atual->direita;
    }

    if (ultimoVisitado) splay(raiz, ultimoVisitado);
    return false;
}

// ==========================================================
// INSERÇÃO -- O(log n) amortizado.
// Regra do artigo: ao inserir um novo nó, esse nó é sempre promovido
// (splay) até a raiz.
// ==========================================================
SplayNode* inserir(SplayNode*& raiz, int chave) {
    SplayNode* atual = raiz;
    SplayNode* pai = nullptr;

    while (atual) {
        pai = atual;
        if (chave == atual->chave) {
            splay(raiz, atual); // já existe: tratamos como um acesso
            return atual;
        }
        atual = (chave < atual->chave) ? atual->esquerda : atual->direita;
    }

    SplayNode* novo = criarNo(chave);
    novo->pai = pai;
    if (!pai) raiz = novo;
    else if (chave < pai->chave) pai->esquerda = novo;
    else pai->direita = novo;

    splay(raiz, novo);
    return novo;
}

// ==========================================================
// REMOÇÃO -- O(log n) amortizado.
// Regra do artigo: ao remover um nó, o PAI desse nó é promovido à raiz.
// Como nossa implementação já traz o próprio nó removido para a raiz
// antes de apagá-lo (via buscar/splay), usamos a técnica equivalente
// de "join": encontrar o maior elemento da subárvore esquerda (splay
// dele) e pendurar a subárvore direita nele.
// ==========================================================
void remover(SplayNode*& raiz, int chave) {
    if (!raiz) return;
    if (!buscar(raiz, chave)) return; // não encontrado (mas já fez o splay do último nó visitado)

    SplayNode* alvo = raiz; // buscar já garantiu que a chave está na raiz
    SplayNode* esquerda = alvo->esquerda;
    SplayNode* direita = alvo->direita;
    if (esquerda) esquerda->pai = nullptr;
    if (direita) direita->pai = nullptr;
    delete alvo;

    if (!esquerda) { raiz = direita; return; }
    if (!direita) { raiz = esquerda; return; }

    SplayNode* maiorEsquerda = esquerda;
    while (maiorEsquerda->direita) maiorEsquerda = maiorEsquerda->direita;
    splay(esquerda, maiorEsquerda); // maiorEsquerda vira raiz da subárvore esquerda (sem filho direito)
    maiorEsquerda->direita = direita;
    direita->pai = maiorEsquerda;
    raiz = maiorEsquerda;
}

void destruirSplay(SplayNode* no) {
    if (!no) return;
    destruirSplay(no->esquerda);
    destruirSplay(no->direita);
    delete no;
}

// ==========================================================
// PERCURSO EM ORDEM -- para conferir que a propriedade de BST
// continua válida após as reorganizações.
// ==========================================================
void emOrdem(SplayNode* no, vector<int>& saida) {
    if (!no) return;
    emOrdem(no->esquerda, saida);
    saida.push_back(no->chave);
    emOrdem(no->direita, saida);
}

int altura(SplayNode* no) {
    if (!no) return 0;
    int esq = altura(no->esquerda);
    int dir = altura(no->direita);
    return 1 + max(esq, dir);
}

// ==========================================================
// EXPORTAÇÃO VISUAL (Graphviz / DOT)
// ==========================================================
void gerarDotRecursivo(SplayNode* no, int meuId, int& proximoId, ofstream& arquivo) {
    if (no->esquerda) {
        int idFilho = ++proximoId;
        arquivo << "    node" << meuId << " -> node" << idFilho << " [label=\"E\"];\n";
        arquivo << "    node" << idFilho << " [label=\"" << no->esquerda->chave << "\"];\n";
        gerarDotRecursivo(no->esquerda, idFilho, proximoId, arquivo);
    }
    if (no->direita) {
        int idFilho = ++proximoId;
        arquivo << "    node" << meuId << " -> node" << idFilho << " [label=\"D\"];\n";
        arquivo << "    node" << idFilho << " [label=\"" << no->direita->chave << "\"];\n";
        gerarDotRecursivo(no->direita, idFilho, proximoId, arquivo);
    }
}

void exportarGraphviz(SplayNode* raiz, const string& nomeArquivo) {
    ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) return;
    arquivo << "digraph Splay {\n    node [shape=circle];\n";
    if (raiz) {
        arquivo << "    node0 [label=\"" << raiz->chave << "\", style=filled, fillcolor=lightgrey];\n";
        int id = 0;
        gerarDotRecursivo(raiz, 0, id, arquivo);
    }
    arquivo << "}\n";
    arquivo.close();
}

// ==========================================================
// LEITURA DO DATASET COMPARTILHADO
// Mesmo princípio usado em trie.cpp/patricia.cpp: ler de um arquivo
// externo para que outras estruturas baseadas em chaves comparáveis
// (ex.: Treap) possam usar exatamente o mesmo conjunto de dados.
// ==========================================================
vector<int> lerDataset(const string& caminho) {
    vector<int> chaves;
    ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        cerr << "[ERRO] Nao foi possivel abrir o arquivo: " << caminho << endl;
        return chaves;
    }
    string linha;
    while (getline(arquivo, linha)) {
        if (linha.empty()) continue;
        try {
            chaves.push_back(stoi(linha));
        } catch (...) {
            cerr << "[AVISO] Linha ignorada (nao e um inteiro valido): \"" << linha << "\"" << endl;
        }
    }
    return chaves;
}

// ==========================================================
// PROGRAMA PRINCIPAL
// ==========================================================
int main(int argc, char* argv[]) {
    string caminhoDataset = (argc > 1) ? argv[1] : "dataset_numerico.txt";

    SplayNode* raiz = nullptr;

    // --- Estado inicial (Seção 3, item 1) ---
    exportarGraphviz(raiz, "splay_estado_inicial.dot");

    vector<int> chaves = lerDataset(caminhoDataset);
    cout << "--- Lidas " << chaves.size() << " chaves de \"" << caminhoDataset << "\" ---" << endl;

    auto inicio = high_resolution_clock::now();
    for (int chave : chaves) {
        inserir(raiz, chave);
    }
    auto fim = high_resolution_clock::now();
    auto duracao = duration_cast<microseconds>(fim - inicio);

    cout << "Insercao de " << chaves.size() << " chaves levou " << duracao.count() << " microsegundos." << endl;
    cout << "Altura da arvore apos insercoes: " << altura(raiz) << endl;
    cout << "Raiz apos insercoes (deve ser a ULTIMA chave inserida, pela regra de splay-on-insert): "
         << raiz->chave << endl;

    // --- Estado após inserções (Seção 3, item 1) ---
    exportarGraphviz(raiz, "splay_apos_insercoes.dot");

    // --- Demonstração do mecanismo de reorganização (Seção 3, item 2) ---
    // Acessamos uma chave que ficou "profunda" na árvore (a primeira
    // inserida, 50, que foi empurrada para baixo pelas inserções
    // seguintes) para evidenciar o splaying trazendo-a de volta à raiz.
    cout << "\n--- Teste de Busca (evidenciando reorganizacao) ---" << endl;
    cout << "Buscando 50 (deve subir ate a raiz apos o acesso)..." << endl;
    buscar(raiz, 50);
    cout << "Raiz apos buscar 50: " << raiz->chave << endl;
    exportarGraphviz(raiz, "splay_apos_busca.dot");

    cout << "\nBuscando 999 (chave inexistente -- deve fazer splay do ultimo no visitado)..." << endl;
    bool achou = buscar(raiz, 999);
    cout << "Encontrado: " << (achou ? "sim" : "nao") << " | Raiz apos a tentativa: " << raiz->chave << endl;

    // --- Estado após remoção/reorganização (Seção 3, item 3) ---
    cout << "\n--- Teste de Remocao ---" << endl;
    cout << "Removendo 50..." << endl;
    remover(raiz, 50);
    cout << "Raiz apos remover 50: " << raiz->chave << endl;
    exportarGraphviz(raiz, "splay_apos_remocao.dot");

    vector<int> ordenado;
    emOrdem(raiz, ordenado);
    cout << "\nPercurso em ordem (deve continuar ordenado, confirmando a propriedade de BST): ";
    for (int v : ordenado) cout << v << " ";
    cout << endl;

    destruirSplay(raiz);
    return 0;
}
