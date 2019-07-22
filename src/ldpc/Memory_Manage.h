#pragma once
void *Delete_1D_Array(int *ptr);
void *Delete_1D_Array(double *ptr);
void *Delete_2D_Array(int **ptr, int row);
void *Delete_2D_Array(double **ptr, int row);
int** Allocate_2D_Array_Int(int row, int col, const char msg[]);
int*  Allocate_1D_Array_Int(int len, const char msg[]);
unsigned int** Allocate_2D_Array_UInt(int row, int col, const char msg[]);
unsigned int*  Allocate_1D_Array_UInt(int len, const char msg[]);
unsigned char** Allocate_2D_Array_UChar(int row, int col, const char msg[]);
unsigned char*  Allocate_1D_Array_UChar(int len, const char msg[]);
double** Allocate_2D_Array_Double(int row, int col, const char msg[]);
double*  Allocate_1D_Array_Double(int len, const char msg[]);
