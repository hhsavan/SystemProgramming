#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <time.h>

#define M 30
#define N 40

void print_matrix(const char* desc, int m, int n, complex double* a) {
    printf("\n %s\n", desc);
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            printf(" (%6.2f, %6.2f)", creal(a[i * n + j]), cimag(a[i * n + j]));
        }
        printf("\n");
    }
}
void deneme2();

int main() {
    int i, j;
    srand(time(NULL)); // Seed for random number generation

    // Start time measurement
    clock_t start_time = clock();

    // Allocate memory for the matrix
    complex double* A = (complex double*)malloc(M * N * sizeof(complex double));

    // Initialize the matrix with random non-zero complex numbers
    for (i = 0; i < M; i++) {
        for (j = 0; j < N; j++) {
            double real_part = (double)(rand() % 100 + 1); // Random real part between 1 and 100
            double imag_part = (double)(rand() % 100 + 1); // Random imaginary part between 1 and 100
            A[i * N + j] = real_part + imag_part * I;
        }
    }

    // Print the original matrix
    print_matrix("Original Matrix A", M, N, A);

    // For simplicity, we assume that we have computed the SVD and have the components U, S, and V*
    // Here we manually initialize these components for demonstration purposes
    complex double* U = (complex double*)malloc(M * M * sizeof(complex double));
    complex double* S = (complex double*)malloc(M * sizeof(complex double)); // Singular values are real
    complex double* V = (complex double*)malloc(N * N * sizeof(complex double));

    // Initialize U, S, and V with example values
    // Note: In a real scenario, these would be computed using SVD
    for (i = 0; i < M; i++) {
        for (j = 0; j < M; j++) {
            U[i * M + j] = (i == j) ? 1.0 + 0.0 * I : 0.0 + 0.0 * I;
        }
    }

    for (i = 0; i < M; i++) {
        S[i] = 1.0 / (i + 1.0); // Example singular values
    }

    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            V[i * N + j] = (i == j) ? 1.0 + 0.0 * I : 0.0 + 0.0 * I;
        }
    }

    // Compute the pseudo-inverse A_pinv = V * S_pinv * U*
    complex double* A_pinv = (complex double*)malloc(N * M * sizeof(complex double));

    // Compute S_pinv
    complex double* S_pinv = (complex double*)malloc(M * N * sizeof(complex double));
    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            if (i == j && i < M) {
                S_pinv[i * M + j] = 1.0 / S[i];
            } else {
                S_pinv[i * M + j] = 0.0 + 0.0 * I;
            }
        }
    }

    // Compute V * S_pinv
    complex double* VS_pinv = (complex double*)malloc(N * M * sizeof(complex double));
    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            VS_pinv[i * M + j] = 0.0 + 0.0 * I;
            for (int k = 0; k < M; k++) {
                VS_pinv[i * M + j] += V[i * N + k] * S_pinv[k * M + j];
            }
        }
    }

    // Compute A_pinv = (V * S_pinv) * U*
    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            A_pinv[i * M + j] = 0.0 + 0.0 * I;
            for (int k = 0; k < M; k++) {
                A_pinv[i * M + j] += VS_pinv[i * M + k] * conj(U[j * M + k]);
            }
        }
    }

    // Print the pseudo-inverse matrix
    print_matrix("Pseudo-Inverse Matrix A_pinv", N, M, A_pinv);

    // Free allocated memory
    free(A);
    free(U);
    free(S);
    free(V);
    free(S_pinv);
    free(VS_pinv);
    free(A_pinv);

    // End time measurement
    clock_t end_time = clock();

    // Calculate the time taken
    double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Time taken to compute the pseudoinverse: %f seconds\n", time_taken);

    return 0;
}

