#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <time.h>

// --- Flush de cache simulando working set frio --- 
// Nas primeiras execuções, tivemos eficiência acima de 100%, com isso implementamos flush de cache, mas decidimos não usar para avaliar também o impacto do cache
// entretanto após remover o flush a eficiência estabilizou em torno de abaixo de 100%, tendo poucos casos acima de 100%
void flush_caches() {
    static const size_t FLUSH_SIZE = 100 * 1024 * 1024; // 100 MiB > L3 (32 MiB)
    static char *buffer = NULL;
    if (!buffer) {
        buffer = malloc(FLUSH_SIZE);
        if (!buffer) {
            fprintf(stderr, "Falha ao alocar buffer de flush\n");
            exit(EXIT_FAILURE);
        }
        memset(buffer, 0, FLUSH_SIZE);
    }
    volatile char sink;
    for (size_t i = 0; i < FLUSH_SIZE; i++) {
        sink = buffer[i];
    }
}

// Gera array aleatório de tamanho n
void generate_random_array(int *arr, int n) {
    for (int i = 0; i < n; i++)
        arr[i] = rand() % 100000;
}

// Copia src → dest
void copy_array(int *src, int *dest, int n) {
    for (int i = 0; i < n; i++)
        dest[i] = src[i];
}

// Serial odd–even transposition sort
void odd_even_sort_serial(int *A, int n) {
    int phase, i, tmp;
    for (phase = 0; phase < n; phase++) {
        if (phase % 2 == 0) {
            for (i = 0; i < n - 1; i += 2) {
                if (A[i] > A[i+1]) {
                    tmp    = A[i];
                    A[i]   = A[i+1];
                    A[i+1] = tmp;
                }
            }
        } else {
            for (i = 1; i < n - 1; i += 2) {
                if (A[i] > A[i+1]) {
                    tmp    = A[i];
                    A[i]   = A[i+1];
                    A[i+1] = tmp;
                }
            }
        }
    }
}

// Parallel odd–even transposition com OpenMP (uma única região paralela)
void parallel_bubble_sort(int *A, int n, int num_threads) {
    omp_set_num_threads(num_threads);
    #pragma omp parallel
    {
        int phase, i, tmp;
        for (phase = 0; phase < n; phase++) {
            if (phase % 2 == 0) {
                #pragma omp for schedule(guided,1)
                for (i = 0; i < n - 1; i += 2) {
                    if (A[i] > A[i+1]) {
                        tmp    = A[i];
                        A[i]   = A[i+1];
                        A[i+1] = tmp;
                    }
                }
            } else {
                #pragma omp for schedule(guided,1)
                for (i = 1; i < n - 1; i += 2) {
                    if (A[i] > A[i+1]) {
                        tmp    = A[i];
                        A[i]   = A[i+1];
                        A[i+1] = tmp;
                    }
                }
            }
        }
    }
}

double get_time() {
    return omp_get_wtime();
}

// CSV incremental:
static FILE *csv_fp = NULL;
void init_csv(const char *filename) {
    csv_fp = fopen(filename, "a+");
    if (!csv_fp) {
        fprintf(stderr, "Erro ao abrir %s para escrita\n", filename);
        exit(EXIT_FAILURE);
    }
    fseek(csv_fp, 0, SEEK_END);
    if (ftell(csv_fp) == 0) {
        // escreve cabeçalho se arquivo vazio
        fprintf(csv_fp, "list_size,threads,time_seconds,speedup_serial,efficiency_serial,speedup_1thread,efficiency_1thread\n");
    }
}

void append_csv(int n, int th, double time, double sp_serial, double ef_serial, double sp_1t, double ef_1t) {
    if (!csv_fp) return;
    fprintf(csv_fp, "%d,%d,%.4f,%.2f,%.2f,%.2f,%.2f\n", n, th, time,
            sp_serial, ef_serial, sp_1t, ef_1t);
    fflush(csv_fp);
}

void run_test(int n) {
    int *orig = malloc(n * sizeof(int));
    int *arr  = malloc(n * sizeof(int));
    if (!orig || !arr) {
        fprintf(stderr, "Erro de alocação de arrays\n");
        exit(EXIT_FAILURE);
    }

    generate_random_array(orig, n);
    printf("=== Lista com %d elementos ===\n", n);

    // 1) Serial odd–even (baseline)
    //flush_caches(); 
    copy_array(orig, arr, n);
    double t0 = get_time(); odd_even_sort_serial(arr, n);
    t0 = get_time() - t0;
    printf("Odd–even serial:    %.4f s\n", t0);
    // grava no CSV: threads=0, só eficiência serial
    append_csv(n, 0, t0, 1.0, 100.0, 0.0, 0.0);

    // 2) OpenMP 1 thread
    //flush_caches(); 
    copy_array(orig, arr, n);
    double t1 = get_time(); parallel_bubble_sort(arr, n, 1);
    t1 = get_time() - t1;
    printf("OpenMP 1 thread:    %.4f s\n", t1);
    double sp1 = t0 / t1;
    double ef1 = sp1 / 1.0 * 100.0;
    // grava no CSV: eficiência serial e 1 thread
    append_csv(n, 1, t1, sp1, ef1, 1.0, 100.0);

    // 3) OpenMP 2 & 4 threads
    for (int th = 2; th <= 4; th *= 2) {
        //flush_caches(); 
        copy_array(orig, arr, n);
        double tN = get_time(); parallel_bubble_sort(arr, n, th);
        tN = get_time() - tN;

        double sp_serial = t0 / tN;
        double ef_serial = sp_serial / th * 100.0;
        double sp_1t     = t1 / tN;
        double ef_1t     = sp_1t / th * 100.0;

        printf("\n% d threads:\n", th);
        printf("  Tempo total:        %.4f s\n", tN);
        printf("  Speedup→serial:     %.2f  | Eff→serial: %.2f%%\n", sp_serial, ef_serial);
        printf("  Speedup→1 thread:   %.2f  | Eff→1 thread:  %.2f%%\n", sp_1t, ef_1t);

        // grava no CSV
        append_csv(n, th, tN, sp_serial, ef_serial, sp_1t, ef_1t);
    }

    printf("--------------------------------------\n\n");
    free(orig); free(arr);
}

int main() {
    srand((unsigned)time(NULL));

    // inicializa CSV
    init_csv("dynamic_1.csv");

    // set affinity
    setenv("OMP_PROC_BIND", "TRUE", 1);
    setenv("OMP_PLACES", "cores", 1);

    run_test(50000);
    run_test(100000);
    run_test(500000);

    if (csv_fp) fclose(csv_fp);
    return 0;
}
