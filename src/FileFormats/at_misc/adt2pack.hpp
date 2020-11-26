/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2020 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef ADT2PACK_HPP
#define ADT2PACK_HPP

#include <stddef.h>
#include <stdint.h>

class adt2pack
{
    // DEFAULT COMPRESSION: buffer 4k, dictionary 8kb
    const size_t WIN_SIZE_DEF = 1 << 12;
    const size_t DIC_SIZE_DEF = 1 << 13;
    // ULTRA COMPRESSION: buffer 32k, dictionary 16kb
    const size_t WIN_SIZE_MAX = 1 << 15;
    const size_t DIC_SIZE_MAX = 1 << 14;

    static const int MATCH_BIT = 8;
    static const int MAX_MATCH = 1 << MATCH_BIT;
    static const int THRESHOLD = 2;
    static const int PBIT = 14;
    static const int TBIT = 15;
    static const int CBIT = 16;
    static const int DIC_BIT = 14;
    static const int CODE_BIT = 16;
    static const int NC = 255 + MAX_MATCH + 2 - THRESHOLD;
    static const int NP = DIC_BIT + 1;
    static const int NT = CODE_BIT + 3;
    static const int MAX_HASH_VAL = 3 * (1 << DIC_BIT) + (((1 << DIC_BIT) << 9) + 1) * 255;
    static const int PERC_FLAG = 32768;

    int WIN_SIZE = WIN_SIZE_DEF;
    int DIC_SIZE = DIC_SIZE_DEF;

    uint16_t l_tree[2 * (NC - 1)], r_tree[2 * (NC - 1)];

    uint16_t p_table[255];
    uint8_t p_len[NT - 1];
    uint16_t c_table[4095];
    uint8_t c_len[NC - 1];
    uint16_t heap[NC];
    uint16_t len_count[16];
    uint16_t c_freq[2*(NC-1)];
    uint16_t p_freq[2*(NP-1)];
    uint16_t t_freq[2*(NT-1)];
    uint16_t c_code[NC - 1];
    uint16_t p_code[NT - 1];

    int *freq, *sort_ptr,*pos_ptr;
    uint8_t *buf, *len, *stream, *child_count, *level;
    int *parent, *previous,*next;
    int bits,heap_size,remain,
        dec_counter,match_len;
    int bit_buf,sbit_buf,bit_count,
        block_size,depth,c_pos,pos,out_pos,
        match_pos,dec_ptr,out_mask,avail;

    uint8_t *input_buffer, *output_buffer;
    uint32_t input_buffer_idx,output_buffer_idx;
    uint32_t size_unpacked,input_buffer_size;

    uint16_t ReadDataBlock(uint8_t *ptr, size_t size)
    {
        uint16_t result;
      If (input_buffer_size-input_buffer_idx >= size) then
        result := size
      else result := input_buffer_size-input_buffer_idx;
      Move(input_buffer^[input_buffer_idx],ptr^,result);
      Inc(input_buffer_idx,result);
      ReadDataBlock := result;
      If NOT really_no_status_refresh then
        show_progress(input_buffer_idx,3);
    }

    procedure WriteDataBlock(ptr: Pointer; size: Word);
    begin
      Move(ptr^,output_buffer^[output_buffer_idx],size);
      Inc(output_buffer_idx,size);
      If NOT really_no_status_refresh then
        show_progress(output_buffer_idx,3);
    end;

    uint16_t GetBits(int bits)
    {
        uint16_t ret = bit_buf >> (16-bits);
        FillBitBuffer(bits);
        return ret;
    }

    void FillBitBuffer(int bits)
    {
        bit_buf = (bit_buf << bits);
        while(bits > bit_count)
        {
          bits -= bit_count;
          bit_buf = bit_buf | (sbit_buf << bits);
          if(input_buffer_idx <= input_buffer_size)
            {
              sbit_buf = input_buffer[input_buffer_idx];
              input_buffer_idx++;
            }
          else sbit_buf = 0;
          bit_count = 8;
        }

      bit_count -= bits;
      bit_buf = bit_buf | (sbit_buf >> bit_count);
    };

    uint16_t DecodeChar()
    {
        uint16_t chr, mask;

        if(block_size == 0)
        {
            block_size = GetBits(16);
            ReadPtrLen(NT,TBIT,3);
            ReadCharLen;
            ReadPtrLen(NP,PBIT,-1);
        }

        block_size--;

        chr = c_table[bit_buf >> (16-12)];
        if(chr >= NC)
        {
            mask := 1 SHL (16-13);
            do
            {
                if(bit_buf & mask != 0)
                    chr = r_tree[chr];
                else
                    chr = l_tree[chr];
                mask = mask >> 1;
            }
            while(chr < NC);
        }
        FillBitBuffer(c_len[chr]);
        return chr;
    }

    uint16_t DecodePtr()
    {

        uint16_t ptr,mask;

          ptr = p_table[bit_buf >> (16-8)];
          if(ptr >= NP)
            {
              mask = 1 << (16-9);
              do
              {
                if((bit_buf & mask) != 0)
                    ptr = r_tree[ptr];
                else
                    ptr = l_tree[ptr];
                mask = mask >> 1;
              }
              while(ptr < NP);
            }
          FillBitBuffer(p_len[ptr]);
          if(ptr != 0)
            {
              ptr--;
              ptr = (1 << ptr) + GetBits(ptr);
            }
      return ptr;
    }

    void DecodeBuffer(int count, uint8_t *buffer)
    {
        int idx, idx2;

        idx2 = 0;
        dec_counter--;

        while(dec_counter >= 0)
        {
            buffer[idx2] = buffer[dec_ptr];
            dec_ptr = (dec_ptr + 1) & (DIC_SIZE - 1);
            idx2++;
            if(idx2 == count)
                break;;
            dec_counter--;
        }

    //    Repeat
        do
        {
            idx = DecodeChar();
            if(idx <= 255)
            {
              buffer[idx2] = idx;
              idx2++;
              if(idx2 == count)
                break;
            }
            else
            {
                 dec_counter = idx-(256-THRESHOLD);
                 dec_ptr = (idx2 - DecodePtr() - 1) & (DIC_SIZE - 1);
                 dec_counter--;
                 while(dec_counter >= 0)
                 {
                     buffer[idx2] = buffer[dec_ptr];
                     dec_ptr = dec_ptr+1 & DIC_SIZE-1;
                     idx2++;
                     if(idx2 == count)
                         break;
                     dec_counter--;
                 }
            }
        //until FALSE;
        } while(false);
    }

public:
    bool lzh_decompress(QVector<uint8_t> &source, QVector<uint8_t> &dest)
    {
        uint8_t *ptr;
        size_t  size_temp;
        bool ultra_compression_flag;
        size_t size = source.size();

        size_t LZH_decompress = 0;
        input_buffer = source.data();
        input_buffer_idx = 0;
        ultra_compression_flag = BOOLEAN(input_buffer^[input_buffer_idx]);
        input_buffer_idx++;
        input_buffer_size = size;
        output_buffer = dest.data();
        output_buffer_idx = 0;
        Move(input_buffer[input_buffer_idx], size_unpacked, SizeOf(size_unpacked));
        Inc(input_buffer_idx, SizeOf(size_unpacked));
        size = size_unpacked;

        progress_old_value = NULL;
        progress_value = size;

        if(ultra_compression_flag)
        {
            WIN_SIZE = WIN_SIZE_MAX;
            DIC_SIZE = DIC_SIZE_MAX;
        }
        else
        {
            WIN_SIZE = WIN_SIZE_DEF;
            DIC_SIZE = DIC_SIZE_DEF;
        }

        ptr = (uint8_t *)malloc(DIC_SIZE);
        bit_buf = 0;
        sbit_buf = 0;
        bit_count = 0;
        FillBitBuffer(16);
        block_size = 0;
        dec_counter = 0;

        while(size > 0)
        {
            if(size > DIC_SIZE)
                size_temp = DIC_SIZE;
            else
                size_temp = size;
            DecodeBuffer(size_temp,ptr);
            WriteDataBlock(ptr, size_temp);
            size -= size_temp;
        }

        free(ptr);
        LZH_decompress = size_unpacked;

        return true;
    }
};


#endif // ADT2PACK_HPP
