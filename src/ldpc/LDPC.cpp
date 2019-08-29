#include "LDPC.h"
#include "Memory_Manage.h"
#include "sha256.h"
#include "WELL512a.h"
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <random>
#include <uint256.h>


LDPC::LDPC()
{
}

LDPC::~LDPC()
{
    Delete_2D_Array(this->H, this->m);
    Delete_2D_Array(this->col_in_row, this->wr);
    Delete_2D_Array(this->row_in_col, this->wc);

    Delete_2D_Array(this->LRqtl, this->n);
    Delete_2D_Array(this->LRrtl, this->n);
    Delete_1D_Array(this->LRpt);
    Delete_1D_Array(this->LRft);

    Delete_1D_Array(this->hash_vector);
    Delete_1D_Array(this->output_word);
}

void LDPC::decoding()
{
    double temp3, temp_sign, sign, magnitude;
    memset(this->output_word, 0, sizeof(int)*this->n);

    // Initialization
    for (int i = 0; i < this->n; i++)
    {
        memset(this->LRqtl[i], 0, sizeof(double)*this->m);
        memset(this->LRrtl[i], 0, sizeof(double)*this->m);
        this->LRft[i] = log((1 - this->cross_err) / (this->cross_err))*(double)(this->hash_vector[i] * 2 - 1);
    }
    memset(this->LRpt, 0, sizeof(double)*this->n);

    int i, k, l, m, ind, t, mp;
    //Bit to Check Node Messages --> LRqtl
    for (ind = 1; ind <= this->max_iter; ind++)
    {
        for (t = 0; t < this->n; t++)
        {
            for (m = 0; m < this->wc; m++)
            {
                temp3 = 0;
                for (mp = 0; mp < this->wc; mp++)
                    if (mp != m)
                        temp3 = infinity_test(temp3 + this->LRrtl[t][this->row_in_col[mp][t]]);
                this->LRqtl[t][this->row_in_col[m][t]] = infinity_test(this->LRft[t] + temp3);
            }
        }

        //Check to Bit Node Messages --> LRrtl
        for (k = 0; k < this->m; k++)
        {
            for (l = 0; l < this->wr; l++)
            {
                temp3 = 0.0; sign = 1;
                for (m = 0; m < this->wr; m++)
                {
                    if (m != l)
                    {
                        temp3 = temp3 + func_f(fabs(this->LRqtl[this->col_in_row[m][k]][k]));
                        if (this->LRqtl[this->col_in_row[m][k]][k] > 0.0)
                            temp_sign = 1.0;
                        else
                            temp_sign = -1.0;
                        sign = sign * temp_sign;
                    }
                }
                magnitude = func_f(temp3);
                this->LRrtl[this->col_in_row[l][k]][k] = infinity_test(sign*magnitude);
            }
        }

        //Update the priori-information
        for (m = 0; m < this->n; m++)
        {
            this->LRpt[m] = infinity_test(this->LRft[m]);
            for (k = 0; k < this->wc; k++)
            {
                this->LRpt[m] += this->LRrtl[m][this->row_in_col[k][m]];
                this->LRpt[m] = infinity_test(this->LRpt[m]);
            }
        }
    }
    //Get codeword using the prior-information.
    for (i = 0; i < this->n; i++)
    {
        if (LRpt[i] >= 0)
            this->output_word[i] = 1;
        else
            this->output_word[i] = 0;
    }
}

void LDPC::generate_seeds(uint64_t hash)
{
    uint64_t mask = 0xffff;
    this->seeds.clear();
    this->seed = hash;
    for (int i = 0; i < 16; i++) {
        this->seeds.push_back(static_cast<uint32_t>((hash & mask) >> (i * 4)));
        mask = mask << 4;
    }
}

void LDPC::generate_hv(const unsigned char hash_value[])
{
    memset(this->hash_vector, 0, sizeof(int)*this->n);
    int index = 0;
    for (int i = 0; i < this->n/4; i++)
    {

        switch(toupper(hash_value[i]))
        {
        case '0': hash_vector[index++] = 0; hash_vector[index++] = 0; hash_vector[index++] = 0; hash_vector[index++] = 0; break;
        case '1': hash_vector[index++] = 0; hash_vector[index++] = 0; hash_vector[index++] = 0; hash_vector[index++] = 1; break;
        case '2': hash_vector[index++] = 0; hash_vector[index++] = 0; hash_vector[index++] = 1; hash_vector[index++] = 0; break;
        case '3': hash_vector[index++] = 0; hash_vector[index++] = 0; hash_vector[index++] = 1; hash_vector[index++] = 1; break;
        case '4': hash_vector[index++] = 0; hash_vector[index++] = 1; hash_vector[index++] = 0; hash_vector[index++] = 0; break;
        case '5': hash_vector[index++] = 0; hash_vector[index++] = 1; hash_vector[index++] = 0; hash_vector[index++] = 1; break;
        case '6': hash_vector[index++] = 0; hash_vector[index++] = 1; hash_vector[index++] = 1; hash_vector[index++] = 0; break;
        case '7': hash_vector[index++] = 0; hash_vector[index++] = 1; hash_vector[index++] = 1; hash_vector[index++] = 1; break;
        case '8': hash_vector[index++] = 1; hash_vector[index++] = 0; hash_vector[index++] = 0; hash_vector[index++] = 0; break;
        case '9': hash_vector[index++] = 1; hash_vector[index++] = 0; hash_vector[index++] = 0; hash_vector[index++] = 1; break;
        case 'A': hash_vector[index++] = 1; hash_vector[index++] = 0; hash_vector[index++] = 1; hash_vector[index++] = 0; break;
        case 'B': hash_vector[index++] = 1; hash_vector[index++] = 0; hash_vector[index++] = 1; hash_vector[index++] = 1; break;
        case 'C': hash_vector[index++] = 1; hash_vector[index++] = 1; hash_vector[index++] = 0; hash_vector[index++] = 0; break;
        case 'D': hash_vector[index++] = 1; hash_vector[index++] = 1; hash_vector[index++] = 0; hash_vector[index++] = 1; break;
        case 'E': hash_vector[index++] = 1; hash_vector[index++] = 1; hash_vector[index++] = 1; hash_vector[index++] = 0; break;
        case 'F': hash_vector[index++] = 1; hash_vector[index++] = 1; hash_vector[index++] = 1; hash_vector[index++] = 1; break;
        }
    }
    memcpy(this->output_word, this->hash_vector, sizeof(int)*this->n);
}

bool LDPC::generate_H()
{
    std::vector<int> col_order;
    if (this->H == NULL)
        return false;

    int k = this->m / this->wc;

    for (int i = 0; i < k; i++)
        for (int j = i * this->wr; j < (i + 1) * this->wr; j++)
            this->H[i][j] = 1;

    InitWELLRNG512a(&this->seeds[0]);
    for (int i = 1; i < this->wc; i++) {
        col_order.clear();
        for (int j = 0; j < this->n; j++)
            col_order.push_back(j);

        auto begin = col_order.begin();
        auto end = col_order.end();
        for (auto n = end - begin -1; n >= 1; --n) {
            auto k = WELLRNG512a() % (n + 1);
            if (k != n) {
                std::iter_swap(begin + k, begin + n);
            }
        }

        for (int j = 0; j < this->n; j++) {
            int index = (col_order.at(j) / this->wr + k * i);
            H[index][j] = 1;
        }
    }

    return true;
}

bool LDPC::generate_Q()
{
    for (int i = 0; i < this->wr; i++)
        memset(this->col_in_row[i], 0, sizeof(int)*this->m);
    for (int i = 0; i < this->wc; i++)
        memset(this->row_in_col[i], 0, sizeof(int)*this->n);

    int row_index = 0, col_index = 0;
    for (int i = 0; i < this->m; i++)
    {
        for (int j = 0; j < this->n; j++)
        {
            if (this->H[i][j])
            {
                this->col_in_row[col_index++%this->wr][i] = j;
                this->row_in_col[row_index++/this->n][j] = i;
            }
        }
    }
    return true;
}

void LDPC::print_word(const char name[], int type)
{
    int i = -1;
    int *ptr = NULL;
    FILE *fp;
    if (name)
        fp = fopen(name, "w");
    else
        fp = stdout;

    if (type == 1)
    {
        ptr = this->hash_vector;
        fprintf(fp, "A hash vector\n");
    }
    else if (type == 2)
    {
        ptr = this->output_word;
        fprintf(fp, "An output vector\n");
    }
    else
    {
        fprintf(fp, "The second parameter of this function should be either 0 or 1\n");
        return;
    }

    while (i++ < this->n - 1)
        fprintf(fp,"%d ", ptr[i]);
    fprintf(fp,"\n");

    if (name)
        fclose(fp);
}

void LDPC::print_H(const char name[])
{
    FILE *fp;
    if (name)
        fp = fopen(name, "w");
    else
        fp = stdout;
    fprintf(fp, "The value of seed : %u\n", static_cast<unsigned int>(this->seed));
    fprintf(fp, "The size of H is %d x %d with ", this->m, this->n);
    fprintf(fp, "wc : %d and wr = %d\n", this->wc, this->wr);

    for (int i = 0; i < this->m; i++)
    {
        for (int j = 0; j < this->n; j++)
            fprintf(fp,"%u\t", this->H[i][j]);
        fprintf(fp,"\n");
    }
    if (name)
        fclose(fp);
}

void LDPC::print_Q(const char name[], int type)
{
    FILE *fp;
    if (name)
        fp = fopen(name, "w");
    else
        fp = stdout;
    if (type == 1)
    {
        fprintf(fp, "\nThe row_in_col_matrix\n");
        for (int i = 0; i < this->wc; i++)
        {
            for (int j = 0; j < this->n; j++)
                fprintf(fp, "%d\t", this->row_in_col[i][j] + 1);
            fprintf(fp, "\n");
        }
    }
    else if (type == 2)
    {
        fprintf(fp, "\nThe col_in_row_matrix\n");
        for (int i = 0; i < this->wr; i++)
        {
            for (int j = 0; j < this->m; j++)
                fprintf(fp, "%d\t", this->col_in_row[i][j] + 1);
            fprintf(fp, "\n");
        }
    }
    if (name)
        fclose(fp);
}

bool LDPC::set_difficulty(int n, int wc, int wr)
{
    if (this->is_regular(n, wc, wr))
    {
        this->n = n; this->wc = wc; this->wr = wr;
        this->m = (int)(n*wc / wr);
        return true;
    }
    return false;
}

void LDPC::set_difficulty(int level)
{
    if (level <= 0 || level >= MAX_DIFF_LEVEL)
        printf("error_set_difficulty\n");
    ldpc_level_parameter tmp = ldpc_level_table[level];
    this->level = level;
    this->n = tmp.n;
    this->wc = tmp.wc;
    this->wr = tmp.wr;
    this->m = (int)(this->n*this->wc / this->wr);
}

bool LDPC::is_regular(int n, int wc, int wr)
{
    int m = round(n * wc / wr);
    if (m * wr == n * wc)
        return true;
    printf("A construction of a regular ldpc code can be impossible using the given parameters\n");
    printf("n * wc / wr has to be a positive integer\n");
    return false;
}

bool LDPC::initialization()
{
    Delete_2D_Array(this->H, this->m);
    Delete_2D_Array(this->col_in_row, this->wr);
    Delete_2D_Array(this->row_in_col, this->wc);

    Delete_2D_Array(this->LRqtl, this->n);
    Delete_2D_Array(this->LRrtl, this->n);
    Delete_1D_Array(this->LRpt);
    Delete_1D_Array(this->LRft);

    Delete_1D_Array(this->hash_vector);
    Delete_1D_Array(this->output_word);

    this->H = Allocate_2D_Array_Int(this->m, this->n, "No sufficient memory for H");
    this->col_in_row = Allocate_2D_Array_Int(this->wr, this->m, "No sufficient memory for Q1_col_in_row");
    this->row_in_col = Allocate_2D_Array_Int(this->wc, this->n, "No sufficient memory for Q2_row_in_col");

    this->LRpt = Allocate_1D_Array_Double(this->n, "No sufficient memory for LRqtl");
    this->LRft = Allocate_1D_Array_Double(this->n, "No sufficient memory for LRrtl");
    this->LRrtl = Allocate_2D_Array_Double(this->n, this->m, "No Sufficient memory for LRrtl");
    this->LRqtl = Allocate_2D_Array_Double(this->n, this->m, "No Sufficient memory for LRqtl");

    this->hash_vector = Allocate_1D_Array_Int(this->n, "No sufficient memory for hash_vector");
    this->output_word = Allocate_1D_Array_Int(this->n, "No sufficient memory for output_word");

    if (this->H && this->col_in_row && this->row_in_col && this->LRft && this->LRft && this->LRrtl && this->LRqtl && this->hash_vector && this->output_word)
        return true;
    return false;
}

bool LDPC::decision()
{
    bool flag = false;
    for (int i = 0; i < this->m; i++)
    {
        int sum = 0;
        for (int j = 0; j < this->wr; j++)
            sum = sum + this->output_word[this->col_in_row[j][i]];
        if (sum % 2)
            return flag;
    }
    /*
  Now, we believe that output_word should be a codeword.
  What we remain is to check whether the number of ones in output_word is an element of a set
  constructed from the variables such as from, to and type.

  Just remind how we construct the set using these variables as follows:
  from, to, type: these variables are used to construct a set by following routines:
  If type is 1, then a set has either one or two elements
  if type is 2, then a set has consecutive elements from "from" to "to" with step 2.
  example) from = 6, to = 10, type = 2 => S:= { 6,  8, 10}
  example) from = 6, to = 10, type = 1 => S:= { 6, 10 }
  example) from = 6, to =  6, type = 1 => S:= { 6 }
  */
    ldpc_level_parameter tmp;
    tmp = ldpc_level_table[this->level];
    int num_of_ones = 0;

    for (int i = 0; i < this->n; i++)
        num_of_ones = num_of_ones + output_word[i];	//Remind that each element of output_word is either 0 or 1.
    if (tmp.type == 2)
    {
        for (int i = tmp.from; i <= tmp.to; i = i + 2)
            if (num_of_ones == i)
                return true;
    }
    if ( tmp.type == 1)
    {
        if (num_of_ones == tmp.from || num_of_ones == tmp.to)
            return true;
    }
    return flag;
}

double LDPC::func_f(double x)
{
    if (x >= BIG_INFINITY)
        return (double)(1.0 / BIG_INFINITY);

    else if (x <= (1.0 / BIG_INFINITY))
        return (double)(BIG_INFINITY);

    else
        return (double)(log((exp(x) + 1) / (exp(x) - 1)));
}

double LDPC::infinity_test(double x)
{
    if (x >= Inf)
        return Inf;
    else if (x <= -Inf)
        return -Inf;
    else
        return x;
}
