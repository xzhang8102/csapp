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
    int row, col, d, a0, a1, a2, a3, a4, a5, a6, a7;
    for (row = 0; row < 32; row += 8)
    {
        for (col = 0; col < 32; col += 8)
        {
            // A[row][col] marks the start of a block
            for (d = 0; d < 8; d++)
            {
                a0 = A[row + d][col];
                a1 = A[row + d][col + 1];
                a2 = A[row + d][col + 2];
                a3 = A[row + d][col + 3];
                a4 = A[row + d][col + 4];
                a5 = A[row + d][col + 5];
                a6 = A[row + d][col + 6];
                a7 = A[row + d][col + 7];
                B[col][row + d] = a0;
                B[col + 1][row + d] = a1;
                B[col + 2][row + d] = a2;
                B[col + 3][row + d] = a3;
                B[col + 4][row + d] = a4;
                B[col + 5][row + d] = a5;
                B[col + 6][row + d] = a6;
                B[col + 7][row + d] = a7;
            }
        }
    }
}

void transpose_64_by_64(int M, int N, int A[N][M], int B[M][N])
{
    // ref: https://github.com/TsundereChen/csapp-cache-lab/
    int row, col, a0, a1, a2, a3, a4, a5, a6, a7;
    // 8x8 block
    for (row = 0; row < 64; row += 8)
    {
        for (col = 0; col < 64; col += 8)
        {
            // in a block
            // top 4 rows
            for (int r = row; r < row + 4; r++)
            {
                a0 = A[r][col];
                a1 = A[r][col + 1];
                a2 = A[r][col + 2];
                a3 = A[r][col + 3];
                a4 = A[r][col + 4];
                a5 = A[r][col + 5];
                a6 = A[r][col + 6];
                a7 = A[r][col + 7];
                B[col][r] = a0;
                B[col][r + 4] = a7;
                B[col + 1][r] = a1;
                B[col + 1][r + 4] = a6;
                B[col + 2][r] = a2;
                B[col + 2][r + 4] = a5;
                B[col + 3][r] = a3;
                B[col + 3][r + 4] = a4;
            }
            // bottom 4 rows
            for (int d = 0; d < 4; d++)
            {
                a0 = A[row + 4][col + 3 - d];
                a4 = A[row + 4][col + 4 + d];
                a1 = A[row + 5][col + 3 - d];
                a5 = A[row + 5][col + 4 + d];
                a2 = A[row + 6][col + 3 - d];
                a6 = A[row + 6][col + 4 + d];
                a3 = A[row + 7][col + 3 - d];
                a7 = A[row + 7][col + 4 + d];
                B[col + 4 + d][row + 0] = B[col + 3 - d][row + 4];
                B[col + 4 + d][row + 1] = B[col + 3 - d][row + 5];
                B[col + 4 + d][row + 2] = B[col + 3 - d][row + 6];
                B[col + 4 + d][row + 3] = B[col + 3 - d][row + 7];
                B[col + 3 - d][row + 4] = a0;
                B[col + 3 - d][row + 5] = a1;
                B[col + 3 - d][row + 6] = a2;
                B[col + 3 - d][row + 7] = a3;
                B[col + 4 + d][row + 4] = a4;
                B[col + 4 + d][row + 5] = a5;
                B[col + 4 + d][row + 6] = a6;
                B[col + 4 + d][row + 7] = a7;
            }
        }
    }
}

void transpose_61_by_67(int M, int N, int A[N][M], int B[M][N])
{
    // ref: https://github.com/TsundereChen/csapp-cache-lab/
    for (int row = 0; row < 67; row += 16)
    {
        for (int col = 0; col < 64; col += 16)
        {
            for (int r = row; r < row + 16 && r < 67; r++)
            {
                for (int c = col; c < col + 16 && c < 61; c++)
                {
                    B[c][r] = A[r][c];
                }
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
        transpose_64_by_64(M, N, A, B);
        break;
    case 61:
        transpose_61_by_67(M, N, A, B);
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
    // registerTransFunction(trans, trans_desc);
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
