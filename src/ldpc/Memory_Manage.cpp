#include "Memory_Manage.h"
#include <memory.h>
#include <stdio.h>

void *Delete_1D_Array(int *ptr)
{
  if (ptr)
    delete [] ptr;
  return NULL;
}

void *Delete_1D_Array(double *ptr)
{
  if (ptr)
    delete [] ptr;
  return NULL;
}

void *Delete_2D_Array(int **ptr, int row)
{
  if (ptr)
  {
    for (int i = 0; i < row; i++)
    {
      if (ptr[i])
      {
        delete [] ptr[i];
        ptr[i] = NULL;
      }
    }
    delete [] ptr;
  }
  return NULL;
}

void *Delete_2D_Array(double **ptr, int row)
{
  if (ptr)
  {
    for (int i = 0; i < row; i++)
    {
      if (ptr[i])
      {
        delete [] ptr[i];
        ptr[i] = NULL;
      }
    }
    delete [] ptr;
  }
  return NULL;
}

int** Allocate_2D_Array_Int(int row, int col, const char msg[])
{
  int **ptr = new int*[row];
  if (!ptr)
  {
    fprintf(stderr, "%s\n", msg);
    return NULL;
  }
  for (int i = 0; i < row; i++)
  {
    ptr[i] = new int[col];
    if (!ptr[i])
    {
      fprintf(stderr, "%s\n", msg);
      return NULL;
    }
    memset(ptr[i], 0, sizeof(int)*col);
  }
  return ptr;
}

int*  Allocate_1D_Array_Int(int len, const char msg[])
{
  int *ptr = new int[len];
  if (!ptr)
  {
    fprintf(stderr, "%s\n", msg);
    return NULL;
  }
  memset(ptr, 0, sizeof(int)*len);
  return ptr;
}

unsigned int** Allocate_2D_Array_UInt(int row, int col, const char msg[])
{
  unsigned int **ptr = new unsigned int*[row];
  if (!ptr)
  {
    fprintf(stderr, "%s\n", msg);
    return NULL;
  }
  for (int i = 0; i < row; i++)
  {
    ptr[i] = new unsigned int[col];
    if (!ptr[i])
    {
      fprintf(stderr, "%s\n", msg);
      return NULL;
    }
    memset(ptr[i], 0, sizeof(unsigned int)*col);
  }
  return ptr;
}

unsigned int*  Allocate_1D_Array_UInt(int len, const char msg[])
{
  unsigned int *ptr = new unsigned int[len];
  if (!ptr)
  {
    fprintf(stderr, "%s\n", msg);
    return NULL;
  }
  memset(ptr, 0, sizeof(unsigned int)*len);
  return ptr;
}

unsigned char** Allocate_2D_Array_UChar(int row, int col, const char msg[])
{
  unsigned char **ptr = new unsigned char*[row];
  if (!ptr)
  {
    fprintf(stderr, "%s\n", msg);
    return NULL;
  }
  for (int i = 0; i < row; i++)
  {
    ptr[i] = new unsigned char[col];
    if (!ptr[i])
    {
      fprintf(stderr, "%s\n", msg);
      return NULL;
    }
    memset(ptr[i], 0, sizeof(unsigned char)*col);
  }
  return ptr;
}

unsigned char*  Allocate_1D_Array_UChar(int len, const char msg[])
{
  unsigned char *ptr = new unsigned char[len];
  if (!ptr)
  {
    fprintf(stderr, "%s\n", msg);
    return NULL;
  }
  memset(ptr, 0, sizeof(unsigned char)*len);
  return ptr;
}

double** Allocate_2D_Array_Double(int row, int col, const char msg[])
{
  double **ptr = new double*[row];
  if (!ptr)
  {
    fprintf(stderr, "%s\n", msg);
    return NULL;
  }
  for (int i = 0; i < row; i++)
  {
    ptr[i] = new double[col];
    if (!ptr[i])
    {
      fprintf(stderr, "%s\n", msg);
      return NULL;
    }
    memset(ptr[i], 0, sizeof(double)*col);
  }
  return ptr;
  
}

double*  Allocate_1D_Array_Double(int len, const char msg[])
{
  double *ptr = new double[len];
  if (!ptr)
  {
    fprintf(stderr, "%s\n", msg);
    return NULL;
  }
  memset(ptr, 0, sizeof(double)* len);
  return ptr;
}
