# 🌳 Estruturas em Árvores Especializadas

[![Linguagem](https://img.shields.io/badge/Linguagem-C++17-blue.svg)](https://isocpp.org/)
[![Linguagem Auxiliar](https://img.shields.io/badge/Scripts-Python_3-yellow.svg)](https://www.python.org/)
[![Status](https://img.shields.io/badge/Status-Concluído-success.svg)]()
[![Instituição](https://img.shields.io/badge/CEFET--MG-Engenharia_de_Computação-darkred.svg)]()

Repositório destinado ao trabalho prático da disciplina de Algoritmos e Estruturas de Dados Avançadas. O projeto aborda a modelagem, implementação em C++ e análise comparativa de desempenho de cinco estruturas de dados hierárquicas não convencionais, contrastando-as entre si e com as tradicionais BST e AVL.

## 📌 Estruturas Implementadas

Cada estrutura foi implementada de forma independente, contemplando operações de **inserção, busca e remoção**, além de uma **operação específica** e um sistema de **exportação de rastreamento visual** (Graphviz/SVG).

1. **Trie (Árvore de Prefixos)**
   - Voltada para recuperação rápida de *strings* em tempo $O(L)$.
   - *Operação Específica:* Autocompletar (Busca por Prefixo).
2. **Árvore Patricia (Radix Tree Compacta)**
   - Otimização espacial da Trie via compressão de nós unários (operação de *split*).
   - *Operação Específica:* Autocompletar adaptado para fatiamento de *strings*.
3. **Árvore Splay**
   - Árvore autoajustável focada em acessos desiguais e localidade temporal de referência via *Splaying* (Zig, Zig-Zig, Zig-Zag).
   - *Operação Específica:* Demonstração da reorganização do *Working Set* em acessos repetidos.
4. **Árvore Treap (Tree + Heap)**
   - Árvore híbrida de balanceamento probabilístico baseada em chaves numéricas e prioridades aleatórias (Max-Heap).
   - *Operação Específica:* Atualização dinâmica de prioridade de um nó existente.
5. **KD-Tree (K-Dimensional Tree)**
   - Particionamento espacial multidimensional (implementado para K=2, coordenadas X e Y). Remocão via Algoritmo de Bentley.
   - *Operação Específica:* Busca pelo Vizinho Mais Próximo (*Nearest Neighbor*).

---

## 📂 Estrutura do Repositório

```text
📦 Estruturas-em-Arvores-Especializadas
 ┣ 📜 trie.cpp             # Implementação da Trie
 ┣ 📜 patricia.cpp         # Implementação da Patricia
 ┣ 📜 splay.cpp            # Implementação da Splay
 ┣ 📜 treap.cpp            # Implementação da Treap
 ┣ 📜 kdtree.cpp           # Implementação da KD-Tree
 ┣ 📜 experimentos.py      # Scripts para geração de datasets massivos
 ┣ 📂 datasets_benchmark   # Arquivos .txt (aleatórios e ordenados)
 ┣ 📂 output               # Exportação de resultados .csv e binários gerados
 ┗ 📂 *_data               # Arquivos .dot e .svg para demonstração visual