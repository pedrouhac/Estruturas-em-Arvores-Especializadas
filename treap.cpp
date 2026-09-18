#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <random>
#include <cmath>

using namespace std;
using namespace std::chrono;

// ==========================================================
// ÁRVORE TREAP (Tree + Heap)
//
// Combina duas propriedades simultâneas (conforme suas anotações):
//   1) Propriedade de BST em relação às CHAVES: esquerda < nó < direita.
//   2) Propriedade de HEAP (max-heap) em relação às PRIORIDADES:
//      a prioridade de um nó é sempre maior que a de seus filhos.
//
// As prioridades são geradas aleatoriamente na inserção. É esse
// sorteio que dá o balanceamento PROBABILÍSTICO da estrutura (esperado
// O(log n)), sem precisar de nenhuma regra determinística de
// balanceamento como a AVL usa.
// ==========================================================

struct TreapNode {
    int chave;
    int prioridade;
    TreapNode* esquerda;
    TreapNode* direita;
};

TreapNode* criarNo(int chave, int prioridade) {
    TreapNode* no = new TreapNode;
    no->chave = chave;
    no->prioridade = prioridade;
    no->esquerda = no->direita = nullptr;
    return no;
}

// Gerador de prioridades aleatórias. Usamos uma semente FIXA (42) para
// que a demonstração seja reprodutível (mesmas imagens toda vez que
// rodar). Em experimentos reais (Seção 5), troque por
// std::random_device para prioridades verdadeiramente aleatórias a
// cada execução.
mt19937 geradorAleatorio(42);
uniform_int_distribution<int> distribuicaoPrioridade(1, 1000);
int gerarPrioridade() { return distribuicaoPrioridade(geradorAleatorio); }

// ==========================================================
// ROTAÇÕES BÁSICAS (mesma mecânica da AVL/Splay, mas aqui disparadas
// pela comparação de PRIORIDADE, não por fator de balanceamento nem
// por acesso)
// ==========================================================
TreapNode* rotacionarDireita(TreapNode* y) {
    TreapNode* x = y->esquerda;
    y->esquerda = x->direita;
    x->direita = y;
    return x;
}

TreapNode* rotacionarEsquerda(TreapNode* x) {
    TreapNode* y = x->direita;
    x->direita = y->esquerda;
    y->esquerda = x;
    return y;
}

// ==========================================================
// INSERÇÃO -- O(log n) esperado.
// Desce como uma BST comum pela chave; na volta da recursão, se o
// filho tiver prioridade MAIOR que o pai, rotaciona para restaurar a
// propriedade de heap.
// ==========================================================
TreapNode* inserirComPrioridade(TreapNode* raiz, int chave, int prioridade) {
    if (!raiz) return criarNo(chave, prioridade);

    if (chave < raiz->chave) {
        raiz->esquerda = inserirComPrioridade(raiz->esquerda, chave, prioridade);
        if (raiz->esquerda->prioridade > raiz->prioridade) {
            raiz = rotacionarDireita(raiz);
        }
    } else if (chave > raiz->chave) {
        raiz->direita = inserirComPrioridade(raiz->direita, chave, prioridade);
        if (raiz->direita->prioridade > raiz->prioridade) {
            raiz = rotacionarEsquerda(raiz);
        }
    }
    // chave == raiz->chave: já existe, não duplica.
    return raiz;
}

TreapNode* inserir(TreapNode* raiz, int chave) {
    return inserirComPrioridade(raiz, chave, gerarPrioridade());
}

// ==========================================================
// BUSCA -- O(log n) esperado. Igual a uma BST comum: a prioridade não
// participa da busca, só da manutenção da forma da árvore.
// ==========================================================
bool buscar(TreapNode* raiz, int chave) {
    if (!raiz) return false;
    if (chave == raiz->chave) return true;
    return (chave < raiz->chave) ? buscar(raiz->esquerda, chave) : buscar(raiz->direita, chave);
}

// ==========================================================
// REMOÇÃO -- O(log n) esperado.
// Desce até encontrar o nó; então gira-o para baixo (sempre na direção
// do filho de MAIOR prioridade) até que ele vire uma folha, e só então
// é removido -- assim a propriedade de heap nunca é violada durante o
// processo.
// ==========================================================
TreapNode* remover(TreapNode* raiz, int chave) {
    if (!raiz) return nullptr;

    if (chave < raiz->chave) {
        raiz->esquerda = remover(raiz->esquerda, chave);
    } else if (chave > raiz->chave) {
        raiz->direita = remover(raiz->direita, chave);
    } else {
        // Encontrado.
        if (!raiz->esquerda) {
            TreapNode* temp = raiz->direita;
            delete raiz;
            return temp;
        }
        if (!raiz->direita) {
            TreapNode* temp = raiz->esquerda;
            delete raiz;
            return temp;
        }
        // Dois filhos: gira na direção do filho de maior prioridade
        // para empurrar o nó-alvo para baixo, mantendo a propriedade
        // de heap, e continua removendo recursivamente.
        if (raiz->esquerda->prioridade > raiz->direita->prioridade) {
            raiz = rotacionarDireita(raiz);
            raiz->direita = remover(raiz->direita, chave);
        } else {
            raiz = rotacionarEsquerda(raiz);
            raiz->esquerda = remover(raiz->esquerda, chave);
        }
    }
    return raiz;
}

// ==========================================================
// OPERAÇÃO ESPECÍFICA: ATUALIZAÇÃO DE PRIORIDADE
// Demonstra explicitamente a "alteração de prioridades" mencionada no
// enunciado (Seção 3): remove o nó e o reinsere com uma nova
// prioridade, forçando uma nova sequência de rotações.
// ==========================================================
TreapNode* atualizarPrioridade(TreapNode* raiz, int chave, int novaPrioridade) {
    if (!buscar(raiz, chave)) return raiz; // chave não existe, nada a fazer
    raiz = remover(raiz, chave);
    raiz = inserirComPrioridade(raiz, chave, novaPrioridade);
    return raiz;
}

void destruirTreap(TreapNode* no) {
    if (!no) return;
    destruirTreap(no->esquerda);
    destruirTreap(no->direita);
    delete no;
}

int altura(TreapNode* no) {
    if (!no) return 0;
    return 1 + max(altura(no->esquerda), altura(no->direita));
}

// ==========================================================
// EXPORTAÇÃO VISUAL (Graphviz / DOT)
// Cada nó mostra chave e prioridade -- essencial para visualizar por
// que uma rotação ocorreu (o filho tinha prioridade maior que o pai).
// ==========================================================
void gerarDotRecursivo(TreapNode* no, int meuId, int& proximoId, ofstream& arquivo) {
    if (no->esquerda) {
        int idFilho = ++proximoId;
        arquivo << "    node" << idFilho << " [label=\"" << no->esquerda->chave
                << "\\n(p=" << no->esquerda->prioridade << ")\"];\n";
        arquivo << "    node" << meuId << " -> node" << idFilho << " [label=\"E\"];\n";
        gerarDotRecursivo(no->esquerda, idFilho, proximoId, arquivo);
    }
    if (no->direita) {
        int idFilho = ++proximoId;
        arquivo << "    node" << idFilho << " [label=\"" << no->direita->chave
                << "\\n(p=" << no->direita->prioridade << ")\"];\n";
        arquivo << "    node" << meuId << " -> node" << idFilho << " [label=\"D\"];\n";
        gerarDotRecursivo(no->direita, idFilho, proximoId, arquivo);
    }
}

void exportarGraphviz(TreapNode* raiz, const string& nomeArquivo) {
    ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) return;
    arquivo << "digraph Treap {\n    node [shape=circle];\n";
    if (raiz) {
        arquivo << "    node0 [label=\"" << raiz->chave << "\\n(p=" << raiz->prioridade
                << ")\", style=filled, fillcolor=lightgrey];\n";
        int id = 0;
        gerarDotRecursivo(raiz, 0, id, arquivo);
    }
    arquivo << "}\n";
    arquivo.close();
}

// ==========================================================
// LEITURA DO DATASET COMPARTILHADO (mesmo arquivo usado pela Splay)
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

    TreapNode* raiz = nullptr;

    // --- Estado inicial (Seção 3, item 1) ---
    exportarGraphviz(raiz, "treap_estado_inicial.dot");

    vector<int> chaves = lerDataset(caminhoDataset);
    cout << "--- Lidas " << chaves.size() << " chaves de \"" << caminhoDataset << "\" ---" << endl;

    auto inicio = high_resolution_clock::now();
    for (int chave : chaves) {
        raiz = inserir(raiz, chave);
    }
    auto fim = high_resolution_clock::now();
    auto duracao = duration_cast<microseconds>(fim - inicio);

    cout << "Insercao de " << chaves.size() << " chaves levou " << duracao.count() << " microsegundos." << endl;
    cout << "Altura da arvore apos insercoes: " << altura(raiz)
         << " (para " << chaves.size() << " chaves, log2(n) ~= "
         << (chaves.empty() ? 0.0 : log2((double)chaves.size())) << ")" << endl;
    cout << "Raiz apos insercoes (chave com a maior prioridade sorteada): "
         << raiz->chave << " (p=" << raiz->prioridade << ")" << endl;

    // --- Estado após inserções (Seção 3, item 1) ---
    exportarGraphviz(raiz, "treap_apos_insercoes.dot");

    cout << "\n--- Teste de Busca ---" << endl;
    cout << "Buscar 45: " << (buscar(raiz, 45) ? "Encontrado" : "Nao encontrado") << endl;
    cout << "Buscar 999: " << (buscar(raiz, 999) ? "Encontrado" : "Nao encontrado") << endl;

    // --- Estado intermediário: alteração de prioridade forçando rotações (Seção 3, item 2) ---
    cout << "\n--- Teste de Atualizacao de Prioridade (operacao especifica) ---" << endl;
    cout << "Atribuindo prioridade maxima (9999) a chave 10, forcando-a a subir..." << endl;
    raiz = atualizarPrioridade(raiz, 10, 9999);
    cout << "Raiz apos atualizar prioridade de 10: " << raiz->chave << " (p=" << raiz->prioridade << ")" << endl;
    exportarGraphviz(raiz, "treap_apos_prioridade.dot");

    // --- Estado após remoção (Seção 3, item 3) ---
    cout << "\n--- Teste de Remocao ---" << endl;
    cout << "Removendo 10..." << endl;
    raiz = remover(raiz, 10);
    cout << "Buscar 10 apos remocao: " << (buscar(raiz, 10) ? "Encontrado" : "Nao encontrado") << endl;
    exportarGraphviz(raiz, "treap_apos_remocao.dot");

    destruirTreap(raiz);
    return 0;
}
