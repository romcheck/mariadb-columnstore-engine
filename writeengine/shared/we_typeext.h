/* Copyright (C) 2014 InfiniDB, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; version 2 of
   the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA. */

// $Id$

/** @file
 *
 * File contains basic typedefs shared externally with other parts of the
 * system.  These types are placed here rather than in we_type.h in order
 * to decouple we_type.h and we_define.h from external components, so that
 * the other libraries won't have to recompile every time we change something
 * in we_type.h and/or we_define.h.
 */

#ifndef _WE_TYPEEXT_H_
#define _WE_TYPEEXT_H_
#include <stdint.h>
#include <sys/types.h>

/** Namespace WriteEngine */
namespace WriteEngine
{
/************************************************************************
 * Type definitions
 ************************************************************************/
typedef uint64_t           RID; // Row ID

/************************************************************************
 * Dictionary related structure
 ************************************************************************/
struct Token
{
    uint64_t op       :  10;   // ordinal position within a block
    uint64_t fbo      :  36;   // file block number
    uint64_t bc       :  18;   // block count
    Token()                   // constructor, set to null value
    {
        op  = 0x3FE;
        fbo = 0xFFFFFFFFFLL;
        bc = 0x3FFFF;
    }
};

/************************************************************************
 * @brief Bulk parse meta data describing data in a read buffer.
 * An offset of COLPOSPAIR_NULL_TOKEN_OFFSET represents a null token.
 ************************************************************************/
struct ColPosPair            /** @brief Column position pair structure */
{
    int               start;  /** @brief start position */
    int               offset; /** @brief length of token*/
};

/************************************************************************
 * @brief SecondaryShutdown used to terminate a thread when it sees that the
 * JobStatus flag has been set to EXIT_FAILURE (by another thread).
 ************************************************************************/
class SecondaryShutdownException : public std::runtime_error
{
public:
    SecondaryShutdownException(const std::string& msg) :
        std::runtime_error(msg) { }
};

/************************************************************************
 * @brief Generic exception class used to store exception string and error
 * code for a writeengine error.
 ************************************************************************/
class WeException : public std::runtime_error
{
public:
    WeException(const std::string& msg, int err = 0) :
        std::runtime_error(msg), fErrorCode(err) { }
    void errorCode(int code)
    {
        fErrorCode = code;
    }
    int  errorCode() const
    {
        return fErrorCode;
    }
private:
    int fErrorCode;
};
#include <limits.h>
/************************************************************************
 * @brief Simple struct to hold binary 16, 24, 32, 48, 64  bytes.
 ************************************************************************/

//TODO Add big endian support.

template<uint8_t W> struct binary;
typedef binary<16> binary16;
typedef binary<24> binary24;
typedef binary<32> binary32;
typedef binary<48> binary48;
typedef binary<64> binary64;

template<uint8_t W>
struct binary {
    unsigned char data[W]; // May be ok for empty value ?

    binary(){
        //assert(W == 16 || W == 24 || W == 32 || W == 48 || W == 64);      
    }
    
    void operator=(uint64_t v) {
        *((uint64_t *) data) = v;
        memset(data + 8, 0, W - 8);
    }

    binary& operator+=(uint64_t v) {
        uint64_t remaining = ULONG_LONG_MAX - *((uint64_t *) data);

        if (remaining >= v) {
            *((uint64_t *) data) += v;
            remaining = 0;
        } else {
            *((uint64_t *) data) = v - remaining;
            remaining = 1;
        }

        for (uint8_t i = 8; remaining == 1 && i < W; i += 8) {
            if (*((uint64_t *) (data + i)) == ULONG_LONG_MAX) {
                *((uint64_t *) (data + i)) = 0;
            } else {
                *((uint64_t *) (data + i)) += 1;
                remaining = 0;
            }
        }

        return *this;
    }

    inline uint8_t& operator[](const int index) {
        return *((uint8_t*) (data + index));
    }

    inline uint64_t& uint64(const int index) {
        return *((uint64_t*) (data + (index << 3)));
    }

    inline uint8_t words() {
        return W / 8;
    }

    void tohex(char* str) {
        for (int i = W - 1; i >= 0; i--) {
            snprintf(&str[(W - 1 - i)*2], 3, "%02X", data[i]);
        }
    }
};

} //end of namespace

#endif // _WE_TYPEEXT_H_
