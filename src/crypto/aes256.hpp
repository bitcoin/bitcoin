/*
 * aes256.hpp
 *
 * Copyright (c) 2014, Danilo Treffiletti <urban82@gmail.com>
 * All rights reserved.
 *
 *     This file is part of Aes256.
 *
 *     Aes256 is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as
 *     published by the Free Software Foundation, either version 2.1
 *     of the License, or (at your option) any later version.
 *
 *     Aes256 is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public
 *     License along with Aes256.
 *     If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AES256_HPP
#define AES256_HPP

#include <vector>

typedef std::vector<unsigned char> ByteArray;

#define BLOCK_SIZE 16

class Aes256 {

    public:
        Aes256(const ByteArray& key);
        ~Aes256();

        static ByteArray::size_type encrypt(const ByteArray& key, const ByteArray& plain, ByteArray& encrypted);
        static ByteArray::size_type encrypt(const ByteArray& key, const unsigned char* plain, const ByteArray::size_type plain_length, ByteArray& encrypted);
        static ByteArray::size_type decrypt(const ByteArray& key, const ByteArray& encrypted, ByteArray& plain);
        static ByteArray::size_type decrypt(const ByteArray& key, const unsigned char* encrypted, const ByteArray::size_type encrypted_length, ByteArray& plain);

        ByteArray::size_type encrypt_start(const ByteArray::size_type plain_length, ByteArray& encrypted);
        ByteArray::size_type encrypt_continue(const ByteArray& plain, ByteArray& encrypted);
        ByteArray::size_type encrypt_continue(const unsigned char* plain, const ByteArray::size_type plain_length, ByteArray& encrypted);
        ByteArray::size_type encrypt_end(ByteArray& encrypted);

        ByteArray::size_type decrypt_start(const ByteArray::size_type encrypted_length);
        ByteArray::size_type decrypt_continue(const ByteArray& encrypted, ByteArray& plain);
        ByteArray::size_type decrypt_continue(const unsigned char* encrypted, const ByteArray::size_type encrypted_length, ByteArray& plain);
        ByteArray::size_type decrypt_end(ByteArray& plain);

    private:
        ByteArray            m_key;
        ByteArray            m_salt;
        ByteArray            m_rkey;

        unsigned char        m_buffer[3 * BLOCK_SIZE];
        unsigned char        m_buffer_pos;
        ByteArray::size_type m_remainingLength;

        bool                 m_decryptInitialized;

        void check_and_encrypt_buffer(ByteArray& encrypted);
        void check_and_decrypt_buffer(ByteArray& plain);

        void encrypt(unsigned char *buffer);
        void decrypt(unsigned char *buffer);

        void expand_enc_key(unsigned char *rc);
        void expand_dec_key(unsigned char *rc);

        void sub_bytes(unsigned char *buffer);
        void sub_bytes_inv(unsigned char *buffer);

        void copy_key();

        void add_round_key(unsigned char *buffer, const unsigned char round);

        void shift_rows(unsigned char *buffer);
        void shift_rows_inv(unsigned char *buffer);

        void mix_columns(unsigned char *buffer);
        void mix_columns_inv(unsigned char *buffer);
};

#endif /* AES256_HPP */
