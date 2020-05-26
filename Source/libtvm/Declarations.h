/*
-------------------------------------------------------------------------------
    Copyright (c) 2020 Charles Carley.

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------
*/
#ifndef _Declarations_h_
#define _Declarations_h_

#include <stdint.h>
#include <string>
#include <vector>
#include <stack>
#include <set>
#include <unordered_map>
#include "SharedLib.h"

#define INS_ARG 3
#define MAX_KWD 5

typedef std::string        str_t;
typedef std::vector<str_t> strvec_t;
typedef std::set<str_t>    strset_t;

typedef union Register {
    uint8_t  b[8];
    uint16_t w[4];
    uint32_t l[2];
    uint64_t x;
} Register;

typedef Register Registers[10];


enum RegisterArg
{
    A0_1 = 1 << 0,
    A0_2 = 1 << 1,
    A0_4 = 1 << 2,
    A1_1 = 1 << 3,
    A1_2 = 1 << 4,
    A1_4 = 1 << 5,
    A2_1 = 1 << 6,
    A2_2 = 1 << 7,
    A2_4 = 1 << 8,
};

const uint16_t SizeFlags[3][3] = {
    {A0_1, A0_2, A0_4},
    {A1_1, A1_2, A1_4},
    {A2_1, A2_2, A2_4},
};

enum ProgramFlags
{
    PF_E = 1 << 0,
    PF_G = 1 << 1,
    PF_L = 1 << 2,
};

struct Token
{
    uint8_t  op;
    uint8_t  reg;
    Register ival;
    int32_t  type;
    str_t    value;
    int32_t  index;
    bool     hasComma;
};

enum ParseResult
{
    PS_ERROR = -3,
    // Common error return code
    PS_UNDEFINED,
    // return value for actions that need to scan
    // for more information before returning a token.
    PS_CONTINUE,
    PS_OK,
    PS_MAX
};

enum ParserState
{
    ST_INITIAL = PS_MAX + 1,
    ST_ID,
    ST_DIGIT,
    ST_SECTION,
    ST_CONTINUE,
    ST_MAX,
};

enum TokenCode
{
    TOK_OPCODE = ST_MAX + 1,
    TOK_REGISTER,
    TOK_IDENTIFIER,
    TOK_DIGIT,
    TOK_LABEL,
    TOK_SECTION,
    TOK_MAX,
};

// The value here needs to start at zero
// and match the method table index in the program.
// class. The only reason is to prevent having to do
// a lookup during execution.
enum Opcode
{
    OP_BEG = 0,  // unused padding
    OP_RET,      // ret
    OP_MOV,      // mov r(n), src
    OP_GTO,      // call address
    OP_INC,      // inc, r(n)
    OP_DEC,      // dec, r(n)
    OP_CMP,      // cmp, r(n), src
    OP_JMP,      // jump
    OP_JEQ,      // jump ==
    OP_JNE,      // jump !
    OP_JLT,      // jump <
    OP_JGT,      // jump >
    OP_JLE,      // jump <=
    OP_JGE,      // jump >=
    OP_ADD,      // add r(n), src
    OP_SUB,      // sub r(n), src
    OP_MUL,      // mul r(n), src
    OP_DIV,      // div r(n), src
    OP_SHR,      // shr r(n), src
    OP_SHL,      // shl r(n), src
                 // ---- debugging ----
    OP_PRG,      // print register
    OP_PRI,      // print all registers
    OP_MAX
};

enum ArgType
{
    AT_NULL,
    AT_REGI,
    AT_SVAL,
    AT_ADDR,
    AT_RVAL,
};

typedef char Keyword[MAX_KWD + 1];

struct KeywordMap
{
    Keyword        word;
    uint8_t        op;
    uint8_t        narg;
    const uint8_t* argv;
};

enum SectionCodes
{
    SEC_DAT = 0xFF,
    SEC_TXT,
    SEC_STR,
};

enum InstructionFlags
{
    IF_REG0 = 0x01,
    IF_REG1 = 0x02,
    IF_REG2 = 0x04,
    IF_ADDR = 0x08,
    IF_SYMA = 0x10,
    IF_SYMU = 0x20,
    IF_MAXF = 0x40,
};

struct TVMHeader
{
    uint8_t  code[2];
    uint16_t flags;
    uint32_t dat;
    uint32_t str;
    uint32_t sym;
};

struct TVMSection
{
    uint16_t flags;
    uint16_t align;
    uint32_t entry;
    uint32_t size;
    uint32_t start;
};

struct Instruction
{
    uint8_t  op;
    uint8_t  flags;
    uint16_t sizes;
    uint8_t  argc;
    uint64_t argv[INS_ARG];
    uint64_t label;
    uint64_t sym;
    str_t    lname;
};

typedef void (*Symbol)(tvmregister_t);

struct SymbolTable
{
    const char* name;
    Symbol      callback;
};

typedef SymbolTable* (*ModuleInit)();

struct ExecInstruction
{
    uint8_t  op;
    uint8_t  flags;
    uint8_t  argc;
    uint64_t argv[INS_ARG];
    Symbol   call;
};


using Instructions     = std::vector<Instruction>;
using IndexToPosition  = std::unordered_map<size_t, size_t>;
using LabelMap         = std::unordered_map<str_t, size_t>;
using SymbolMap        = std::unordered_map<str_t, Symbol>;
using SymbolLookup     = std::unordered_map<str_t, str_t>;
using DynamicLib       = std::vector<void*>;
using StringMap        = std::unordered_map<str_t, size_t>;
using ExecInstructions = std::vector<ExecInstruction>;
using Stack            = std::stack<uint64_t>;


#define _RELITAVE_TIME_CHECK_BEGIN                                    \
    {                                                                 \
        chrono::high_resolution_clock::time_point begintick, endtick; \
        begintick = chrono::high_resolution_clock().now();            \
        {
#define _RELITAVE_TIME_CHECK_END                                      \
    }                                                                 \
    endtick = chrono::high_resolution_clock().now();                  \
    cout << __FUNCTION__ << " exec("                                  \
         << fixed << setprecision(6)                                  \
         << ((chrono::duration<double>(endtick - begintick).count())) \
         << "s)"                                                      \
         << endl;                                                     \
    }

#endif  // _Declarations_h_