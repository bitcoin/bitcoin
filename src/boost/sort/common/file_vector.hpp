//----------------------------------------------------------------------------
/// @file file_vector.hpp
/// @brief This file contains functions for to work with random data and files
///        Have functions for to create a vector with random data, and
///        functions for lo load a vector of numbers or strings from the file
///
/// @author Copyright (c) 2015 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanyingfile LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_COMMON_FILE_VECTOR_HPP
#define __BOOST_SORT_COMMON_FILE_VECTOR_HPP

#include <ios>
#include <cstdio>
#include <cstdlib>
#include <ciso646>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <cstdint>

namespace boost
{
namespace sort
{
namespace common
{
//
//-----------------------------------------------------------------------------
//  function : generate_file
/// @brief Generate a binary file filed with random numbers of 64 bits
/// @param [in] filename : name of the file
/// @param [in] NElem : number of 64 bits numbers to insert in the file
/// @exception
/// @return
/// @remarks
//-----------------------------------------------------------------------------
static int generate_file(const std::string & filename, size_t NElem)
{   //------------------------------- begin ----------------------------------
    std::ofstream ofile;
    ofile.open(filename, std::ios_base::out | std::ios_base::binary |
                         std::ios_base::trunc);
    if (ofile.bad())
    {
        throw std::ios_base::failure("could not open file \n");
    };
    std::mt19937_64 my_rand(0);

    for (size_t i = 0; i < NElem; ++i)
    {
        uint64_t Aux = my_rand();
        ofile.write((char *) &Aux, 8);
    }
    ofile.close();
    return 0;
};
//
//-----------------------------------------------------------------------------
//  function : fill_vector_uint64
/// @brief : fill a vector of uint64_t elements from a file
/// @param [in] filename : name of the file
/// @param [in] V : vector to fill
/// @param [in] NElem : number of elements for to read from the file
/// @exception
/// @return
/// @remarks
//-----------------------------------------------------------------------------
static int fill_vector_uint64(const std::string & filename,
                              std::vector<uint64_t> & V, size_t NElem)
{   //----------------------- begin ------------------------------------------
    std::ifstream input(filename, std::ios_base::in | std::ios_base::binary);
    if (input.fail())
    {
        throw std::ios_base::failure("could not open file \n");
    };
    //------------------------------------------------------------------------
    // Calculate the lenght of the file and the number of elements inside
    //------------------------------------------------------------------------
    input.seekg(0, std::ios_base::end);
    size_t length = input.tellg();
    size_t uCount = length / 8;
    if (uCount < NElem)
    {
        throw std::ios_base::failure("incorrect lenght of the file\n");
    };
    V.clear();
    V.reserve(NElem);

    uint64_t Aux = 0;
    input.seekg(0, std::ios_base::beg);
    for (size_t i = 0; i < NElem; ++i)
    {
        input.read(reinterpret_cast<char *>(&Aux), 8);
        V.push_back(Aux);
    };
    input.close();
    return 0;
};

//
//-----------------------------------------------------------------------------
//  function :write_file_uint64
/// @brief Write a file with the contnt of a vector of Uint64_t elements
/// @param [in] V : vector from read the numbersl
/// @param [in] filename : name of the file
/// @exception
/// @return
/// @remarks
//-----------------------------------------------------------------------------
static int write_file_uint64 (const std::vector<uint64_t> & V,
                              const std::string & filename)
{   //--------------------------------- begin --------------------------------
    std::ofstream ofile;
    ofile.open(filename,
                    std::ios_base::out | std::ios_base::binary
                                    | std::ios_base::trunc);
    if (ofile.bad())
    {
        throw std::ios_base::failure("could not open file \n");
    };
    for (size_t i = 0; i < V.size(); ++i)
    {
        ofile.write((char *) &(V[i]), 8);
    }
    ofile.close();
    return 0;
};
//
//-----------------------------------------------------------------------------
//  function : fill_vector_string
/// @brief fill a vector of strings from a file
/// @param [in] filename : name of the file from read the strings
/// @param [in] V : vector where store the strings
/// @param [in] NElem : Number of strings for to read from the file
/// @exception
/// @return
/// @remarks
//-----------------------------------------------------------------------------
static int fill_vector_string (const std::string & filename,
                               std::vector<std::string> & V, size_t NElem)
{   //----------------------- begin ------------------------------------------
    std::ifstream input(filename, std::ios_base::in | std::ios_base::binary);
    if (input.fail())
    {
        throw std::ios_base::failure("could not open file \n");
    };
    //------------------------------------------------------------------------
    // Calculate the lenght of the file and the number of elements inside
    //------------------------------------------------------------------------
    input.seekg(0, std::ios_base::end);
    V.clear();
    V.reserve(NElem);

    std::string inval;
    input.seekg(0, std::ios_base::beg);

    for (size_t i = 0; i < NElem; ++i)
    {
        if (!input.eof())
        {
            input >> inval;
            V.push_back(inval);
            inval.clear();
        }
        else
        {
            throw std::ios_base::failure("Insuficient lenght of the file\n");
        };
    };
    input.close();
    return 0;
};

//
//-----------------------------------------------------------------------------
//  function :write_file_string
/// @brief : write a file with the strings of a vector
/// @param [in] V : vector from read the sttrings
/// @param [in] filename : file where store the strings
/// @exception
/// @return
/// @remarks
//-----------------------------------------------------------------------------
static int write_file_string (const std::vector<std::string> & V,
                             const std::string & filename)
{   //--------------------------------- begin --------------------------------
    std::ofstream ofile;
    ofile.open(filename,
                    std::ios_base::out | std::ios_base::binary
                                    | std::ios_base::trunc);
    if (ofile.bad())
    {
        throw std::ios_base::failure("could not open file \n");
    };
    for (size_t i = 0; i < V.size(); ++i)
    {
        ofile.write((char *) &(V[i][0]), V[i].size());
        ofile.put(0x0);
    }
    ofile.close();
    return 0;
};
//---------------------------------------------------------------------------
/// @struct uint64_file_generator
/// @brief This struct is a number generator from a file, with several options
///        for to limit the numbers between 0 and Max_Val
/// @remarks
//---------------------------------------------------------------------------
struct uint64_file_generator
{   //----------------------------------------------------------------------
    //                  VARIABLES
    //----------------------------------------------------------------------
    std::ifstream input;
    size_t NMax, Pos;
    size_t Max_Val;
    std::string s;

    //----------------------------------------------------------------------
    //                    FUNCTIONS
    //----------------------------------------------------------------------
    uint64_file_generator(const std::string & filename)
    {   //---------------------------- begin ---------------------------------
        s = filename;
        input.open(filename, std::ios_base::in | std::ios_base::binary);
        if (input.fail() or input.bad())
        {
            throw std::ios_base::failure("could not open file \n");
        };
        //--------------------------------------------------------------------
        // Calculate the lenght of the file and the number of elements inside
        //--------------------------------------------------------------------
        input.seekg(0, std::ios_base::end);
        size_t length = input.tellg();
        NMax = length / 8;
        Pos = 0;
        Max_Val = ~((size_t) 0);
        input.seekg(0);
    };

    void set_max_val(size_t MV){ Max_Val = MV; };

    size_t size() const { return NMax; };

    uint64_t get(void)
    {
        uint64_t Aux;
        input.read(reinterpret_cast<char *>(&Aux), 8);
        return (Aux % Max_Val);
    };

    uint64_t operator ( )(){ return get(); };

    void reset(void) { input.seekg(0, std::ios_base::beg); };

    ~uint64_file_generator() { if (input.is_open()) input.close(); };
};
//
//****************************************************************************
};// end namespace benchmark
};// end namespace sort
};// end namespace boost
//****************************************************************************
//
#endif
