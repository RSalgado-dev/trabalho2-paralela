#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

void generate_random_array(int *arr, int n) {
    for (int i = 0; i < n; i++)
        arr[i] = rand() % 100000;
}

void copy_array(int *src, int *dest, int n) {
    for (int i = 0; i < n; i++)
        dest[i] = src[i];
}

void parallel_bubble_sort(int *arr, int n, int num_threads) {
    int i, temp;
    int phase;
    omp_set_num_threads(num_threads);

    for (phase = 0; phase < n; phase++) {
        if (phase % 2 == 0) {
            #pragma omp parallel for private(i, temp) shared(arr, n)
            for (i = 0; i < n - 1; i += 2) {
                if (arr[i] > arr[i + 1]) {
                    temp = arr[i];
                    arr[i] = arr[i + 1];
                    arr[i + 1] = temp;
                }
            }
        } else {
            #pragma omp parallel for private(i, temp) shared(arr, n)
            for (i = 1; i < n - 1; i += 2) {
                if (arr[i] > arr[i + 1]) {
                    temp = arr[i];
                    arr[i] = arr[i + 1];
                    arr[i + 1] = temp;
                }
            }
        }
    }
}

double get_time_in_seconds() {
    return omp_get_wtime();
}

void run_test(int n) {
    int *original = malloc(n * sizeof(int));
    int *arr = malloc(n * sizeof(int));
    generate_random_array(original, n);

    printf("Lista com %d elementos\n", n);

    copy_array(original, arr, n);
    double start = get_time_in_seconds();
    parallel_bubble_sort(arr, n, 1);
    double t1 = get_time_in_seconds() - start;
    printf("1 thread: %.4f segundos\n", t1);

    copy_array(original, arr, n);
    start = get_time_in_seconds();
    parallel_bubble_sort(arr, n, 2);
    double t2 = get_time_in_seconds() - start;
    printf("2 threads: %.4f segundos | Speedup: %.2f | Eficiência: %.2f%%\n", t2, t1 / t2, (t1 / t2) / 2 * 100);

    copy_array(original, arr, n);
    start = get_time_in_seconds();
    parallel_bubble_sort(arr, n, 4);
    double t4 = get_time_in_seconds() - start;
    printf("4 threads: %.4f segundos | Speedup: %.2f | Eficiência: %.2f%%\n", t4, t1 / t4, (t1 / t4) / 4 * 100);

    printf("---------------------------------------------------\n");

    free(original);
    free(arr);
}

int main() {
    srand(time(NULL));
    run_test(50000);
    run_test(100000);
    run_test(500000);
    return 0;
}
