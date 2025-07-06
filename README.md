**Relatório: Implementação Paralela do Odd–Even Transposition Sort com OpenMP**  

**1. Introdução**  
Este relatório descreve a implementação e as decisões de engenharia tomadas para paralelizar o algoritmo de ordenação Odd–Even Transposition (variante de Bubble Sort) usando OpenMP em C. O objetivo foi comparar o desempenho em listas de inteiros de diferentes tamanhos (50 000, 100 000 e 500 000 elementos), medindo tempo de execução, speedup e eficiência com 1, 2 e 4 threads.  

**2. Estrutura Geral do Código**  
- Funções de utilidade:  
  - `generate_random_array`: gera uma lista aleatória de valores em [0, 100 000).  
  - `copy_array`: cria cópias independentes para cada experimento.  

- Algoritmos de ordenação:  
  1. `odd_even_sort_serial`: implementa a transposição odd–even de forma completamente sequencial, servindo como baseline puro.  
  2. `parallel_bubble_sort`: paraleliza o mesmo algoritmo em uma única região OpenMP, dividindo as comparações em fases par e ímpar.  

- Rotina de benchmark (`run_test`):  
  - Executa: serial odd–even, OpenMP/1 thread e OpenMP/2 & 4 threads.  
  - Calcula speedup e eficiência relativos ao serial puro e ao caso de 1 thread.  
  - Grava resultados incrementalmente em `results.csv`.  

**3. Decisões Técnicas**  
1. **Afinidade de Threads**  
   - Configurações via variáveis de ambiente:  
     - `OMP_PROC_BIND=TRUE`: fixa cada thread a um core, evitando migrações.  
     - `OMP_PLACES=cores`: considera apenas núcleos físicos (6 cores no 5600X), ignorando hyperthreads.  
   - Benefícios: melhora a localidade de cache e torna resultados mais repetíveis.  

2. **Região Paralela Única**  
   - Em vez de abrir/fechar threads em cada fase, encapsula todo o loop de fases em uma única região `#pragma omp parallel`.  
   - Reduz brutalmente overhead de spawn/join e barreiras implícitas.  

3. **Divisão de Trabalho e Sincronização**  
   - `#pragma omp for nowait schedule()`: foram feitas runs usando (static, 1), (static, 64), (guided, 1), (guided, 64) para avaliar qual divisao melhor se comporta para o trabalho dado.  
   - `#pragma omp barrier` ao fim de cada fase: garante que todas as threads concluam antes de avançar, mantendo a corretude do algoritmo.  

**4. Características da Máquina**  
Os testes foram executados em um ambiente WSL (Windows Subsystem for Linux) rodando Ubuntu, com as seguintes especificações de hardware e software:  

- **Sistema Operacional (WSL):** Ubuntu 20.04 LTS sobre Windows 11.  
- **Processador:** AMD Ryzen 5 5600X, 6 núcleos físicos e 12 threads lógicas, cache L1: 384 KiB, L2: 3 MiB, L3: 32 MiB.  
- **Memória RAM:** 32 GiB DDR4, 2666 MHz.  
- **Configurações de Afinidade:** `OMP_PLACES=cores` e `OMP_PROC_BIND=TRUE`, garantindo binding a núcleos físicos.   

Essas características impactam diretamente:  
- Eficiência na cache L3 de 32 MiB, influenciando a performance de acesso à memória.  
- Overhead do WSL em operações de sistema e I/O (escrita no CSV).  

**5. Formato do CSV**  
```  
list_size,threads,time_seconds,speedup_serial,efficiency_serial,speedup_1thread,efficiency_1thread  
50000,0,0.8766,1.00,100.00,0.00,0.00  
50000,1,1.0555,0.83,83.00,1.00,100.00  
50000,2,0.5639,1.56,77.84,1.58,78.89  
...  
```  
- `threads=0`: linha para `odd_even_sort_serial`.  
- `threads=1`: linha para `parallel_bubble_sort` com 1 thread.  
- `threads=2,4`: linhas completas com ambas as eficiências.  
