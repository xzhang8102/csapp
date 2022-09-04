/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void trans(int M, int N, int A[N][M], int B[M][N]);

void transpose_32_by_32(int M, int N, int A[N][M], int B[M][N])
{
    int row, col, r, c, a0, a1, a2, a3, a4, a5, a6, a7;
    for (row = 0; row < 32; row += 8)
    {
        for (col = 0; col < 32; col += 8)
        {
            // A[row][col] marks the start of a block
            for (r = row; r < row + 8; r++)
            {
                c = col;
                a0 = A[r][c];
                a1 = A[r][c + 1];
                a2 = A[r][c + 2];
                a3 = A[r][c + 3];
                a4 = A[r][c + 4];
                a5 = A[r][c + 5];
                a6 = A[r][c + 6];
                a7 = A[r][c + 7];
                B[c][r] = a0;
                B[c + 1][r] = a1;
                B[c + 2][r] = a2;
                B[c + 3][r] = a3;
                B[c + 4][r] = a4;
                B[c + 5][r] = a5;
                B[c + 6][r] = a6;
                B[c + 7][r] = a7;
            }
        }
    }
}

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    switch (M)
    {
    case 32:
        transpose_32_by_32(M, N, A, B);
        break;
    case 64:
        break;
    case 61:
        break;
    default:
        trans(M, N, A, B);
        break;
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
