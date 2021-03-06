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
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "BinaryWriter.h"
#include <stdio.h>
#include <iostream>
#include "SymbolUtils.h"

inline uint16_t getAlignment(size_t al)
{
    uint16_t rem = (al % 16);
    if (rem > 0)
        return (16 - rem);
    return 0;
}

BinaryWriter::BinaryWriter(const str_t& modpath) :
    m_fp(0),
    m_loc(0),
    m_ins(),
    m_sizeOfCode(0),
    m_sizeOfData(0),
    m_sizeOfSym(0),
    m_sizeOfStr(0),
    m_addrMap(),
    m_labels(),
    m_header({}),
    m_modpath(modpath)
{
}

BinaryWriter::~BinaryWriter()
{
    if (m_fp)
        fclose((FILE*)m_fp);
}

void BinaryWriter::mergeInstructions(const Instructions& insl)
{
    m_ins.reserve(m_ins.size() + insl.size());
    Instructions::const_iterator it = insl.begin(), end = insl.end();
    while (it != end)
        m_ins.push_back(*it++);
}

int BinaryWriter::mergeDataDeclarations(const DataLookup& data)
{
    // There should be no conflict between labels defined
    // elsewhere in the file. The scanner will weed out duplicates
    // before execution reaches this step.
    int status = PS_OK;

    DataLookup::const_iterator it = data.begin(), end = data.end();
    while (it != end && status == PS_OK)
    {
        const str_t& name = it->first;
        if (m_dataDecl.find(name) != m_dataDecl.end())
        {
            printf("duplicate label '%s'\n", name.c_str());
            status = PS_ERROR;
        }
        else
            m_dataDecl[name] = it->second;

        ++it;
    }
    return status;
}

int BinaryWriter::mergeLabels(const LabelMap& map)
{
    int status = PS_OK;

    LabelMap::const_iterator it = map.begin();
    while (it != map.end() && status == PS_OK)
    {
        if (m_labels.find(it->first) != m_labels.end())
        {
            printf("duplicate label '%s'\n", it->first.c_str());
            status = PS_ERROR;
        }
        else
        {
            m_labels[it->first]   = it->second;
            m_addrMap[it->second] = 0;
        }

        ++it;
    }
    return status;
}

void BinaryWriter::write(const void* v, size_t size)
{
    if (m_fp)
        fwrite(v, 1, size, (FILE*)m_fp);
}

void BinaryWriter::write8(uint8_t v)
{
    write(&v, sizeof(uint8_t));
}

void BinaryWriter::write16(uint16_t v)
{
    write(&v, sizeof(uint16_t));
}

void BinaryWriter::write32(uint32_t v)
{
    write(&v, sizeof(uint32_t));
}

void BinaryWriter::write64(uint64_t v)
{
    write(&v, sizeof(uint64_t));
}

int BinaryWriter::open(const char* fname)
{
    if (m_fp)
        fclose((FILE*)m_fp);

    m_fp = fopen(fname, "wb");
    if (!m_fp)
    {
        printf("failed to open '%s' for writing.\n", fname);
        return PS_ERROR;
    }
    return PS_OK;
}

uint64_t BinaryWriter::addToStringTable(const str_t& symname)
{
    LabelMap::iterator it = m_strtab.find(symname);
    if (it != m_strtab.end())
        return it->second;

    m_sizeOfStr += symname.size();
    m_sizeOfStr += 1;

    size_t size       = m_strtab.size();
    m_strtab[symname] = size;

    m_orderedString.push_back(symname);
    return size;
}

uint64_t BinaryWriter::addToDataTable(const DataDeclaration& dt)
{
    uint64_t startAddr = m_sizeOfData;

    LabelMap::iterator it = m_datatab.find(dt.lname);
    if (it != m_datatab.end())
        return it->second;
    else
        m_datatab[dt.lname] = startAddr;

    if (dt.type == SEC_ASCII)
    {
        m_sizeOfData += m_dataTable.writeString(dt.sval.c_str(),
                                                dt.sval.size());
    }
    else if (dt.type == SEC_ZERO)
    {
        m_sizeOfData += m_dataTable.fill((size_t)dt.ival, 0);
    }
    else
    {
        m_sizeOfData += m_dataTable.write64(dt.ival);
    }
    return startAddr;
}

uint64_t BinaryWriter::addLinkedSymbol(const str_t& symname, const str_t& libname)
{
    if (m_linkedLibraries.find(libname) == m_linkedLibraries.end())
    {
        m_linkedLibraries.insert(libname);
        m_sizeOfSym += libname.size();
        m_sizeOfSym += 1;
    }
    return addToStringTable(symname);
}

int BinaryWriter::mapInstructions(void)
{
    uint64_t label  = PS_UNDEFINED;
    uint64_t insp   = 0;
    uint64_t lookup = 0;
    int      status = PS_OK;

    using InstPtr = std::vector<Instruction*>;

    InstPtr symbols;

    Instructions::iterator it = m_ins.begin();
    while (it != m_ins.end())
    {
        Instruction& ins = (*it++);

        // look for changes in the label index then
        // save the index of the first change.
        if (ins.label != label)
        {
            label            = ins.label;
            m_addrMap[label] = insp;
        }

        // store it so it can be resolved
        // after all labels have been indexed
        if (!ins.lname.empty())
            symbols.push_back(&ins);

        ++insp;
    }

    InstPtr::iterator symit = symbols.begin();
    while (symit != symbols.end())
    {
        Instruction* irp = (*symit++);

        // modify the first argument so that it points
        // to the correct instruction index.
        lookup = findLabel(irp->lname);
        if (lookup != -1)
        {
            // It points to a local label
            irp->argv[0] = lookup;
            irp->flags |= IF_ADDR;
        }
        else
        {
            DataLookup::iterator it;
            it = m_dataDecl.find(irp->lname);
            if (it != m_dataDecl.end())
            {
                // It points to a data entry
                if (irp->flags & IF_REG2)
                    irp->argv[2] = addToDataTable(it->second);
                else
                    irp->argv[1] = addToDataTable(it->second);
                irp->flags |= IF_ADRD;
            }
            else
            {
                // It points to an unknown symbol
                // that may reside in a shared library.
                StringLookup::iterator it = m_symbols.find(irp->lname);
                if (it != m_symbols.end())
                {
                    irp->argv[0] = addLinkedSymbol(irp->lname, it->second);
                    irp->flags |= IF_SYMU;
                }
                else
                {
                    printf("failed to locate '%s'\n", irp->lname.c_str());
                    status = PS_ERROR;
                }
            }
        }
    }
    return status;
}

size_t BinaryWriter::calculateInstructionSize(void)
{
    size_t i;
    size_t size = 0;

    Instructions::iterator it = m_ins.begin(), endp = m_ins.end();
    while (it != endp)
    {
        Instruction& ins = (*it++);
        ins.sizes        = 0;

        size += 6;  // op, nr, flags, sizes
        if (ins.flags & IF_RIDX)
            size += 1;

        for (i = 0; i < ins.argc; ++i)
        {
            if (ins.argv[i] < 0xFF)
            {
                ins.sizes |= SizeFlags[i][0];
                size += 1;
            }
            else if (ins.argv[i] < 0xFFFF)
            {
                ins.sizes |= SizeFlags[i][1];
                size += 2;
            }
            else if (ins.argv[i] < 0xFFFFFFFF)
            {
                ins.sizes |= SizeFlags[i][2];
                size += 4;
            }
            else
            {
                size += 8;
            }
        }
    }
    return size;
}

uint64_t BinaryWriter::findLabel(const str_t& name)
{
    if (!name.empty())
    {
        LabelMap::iterator it = m_labels.find(name);
        if (it != m_labels.end())
        {
            IndexToPosition::iterator fidx = m_addrMap.find(it->second);
            if (fidx != m_addrMap.end())
                return fidx->second;
        }
    }
    return -1;
}

int BinaryWriter::loadSharedLibrary(const str_t& lib)
{
    int status = PS_OK;

    LibHandle shlib = LoadSharedLibrary(lib, m_modpath);
    if (shlib != nullptr)
    {
        str_t     lookup = lib + "_init";
        LibSymbol sym    = GetSymbolAddress(shlib, lookup.c_str());
        if (sym != nullptr)
        {
            SymbolTable* avail = ((ModuleInit)sym)();
            if (avail == nullptr)
            {
                printf("symbol initialization failed in %s\n",
                       (m_modpath + lib).c_str());
                status = PS_ERROR;
            }

            int i = 0;
            while (avail != nullptr && avail[i].name != nullptr && status == PS_OK)
            {
                const str_t            str = avail[i].name;
                StringLookup::iterator it  = m_symbols.find(str);
                if (it == m_symbols.end())
                    m_symbols[str] = lib;
                else
                {
                    printf("duplicate symbol %s found in library %s\n",
                           str.c_str(),
                           lib.c_str());

                    printf("first seen in %s\n", it->second.c_str());
                    status = PS_ERROR;
                }
                ++i;
            }
        }
        else
        {
            printf("failed to find function '%s' in %s\n",
                   lookup.c_str(),
                   (m_modpath).c_str());
            status = PS_ERROR;
        }

        UnloadSharedLibrary(shlib);
    }
    else
    {
        printf("failed to load library '%s'\n", lib.c_str());
        status = PS_ERROR;
    }
    return status;
}

int BinaryWriter::resolve(strvec_t& modules)
{
    int status = PS_OK;

    strvec_t::iterator it = modules.begin();
    while (it != modules.end() && status == PS_OK)
        status = loadSharedLibrary(*it++);
    return status;
}

int BinaryWriter::writeHeader()
{
    if (!m_fp)
        return PS_ERROR;

    m_header.code[0] = 'T';
    m_header.code[1] = 'V';
    m_header.flags   = 0;

    size_t offset = sizeof(TVMHeader);
    if (mapInstructions() != PS_OK)
    {
        printf("failed to find one or more required symbols\n");
        return PS_ERROR;
    }

    m_sizeOfCode = calculateInstructionSize();
    if (m_sizeOfCode == 0)
    {
        printf("no instructions to write\n");
        return PS_ERROR;
    }

    offset += sizeof(TVMSection);
    offset += m_sizeOfCode;
    offset += getAlignment(m_sizeOfCode);

    if (m_sizeOfData != 0)
    {
        m_header.dat = (uint32_t)offset;
        offset += sizeof(TVMSection);
        offset += m_sizeOfData;
        offset += getAlignment(m_sizeOfData);
    }

    if (m_sizeOfSym != 0)
    {
        m_header.sym = (uint32_t)offset;
        offset += sizeof(TVMSection);
        offset += m_sizeOfSym;
        offset += getAlignment(m_sizeOfSym);
    }

    if (m_sizeOfStr != 0)
        m_header.str = (uint32_t)offset;

    write(&m_header, sizeof(TVMHeader));
    return PS_OK;
}

size_t BinaryWriter::writeDataSection(void)
{
    TVMSection sec = {};
    sec.size       = (uint32_t)m_sizeOfData;
    sec.entry      = m_header.dat;
    sec.align      = getAlignment(m_sizeOfData);

    write(&sec, sizeof(TVMSection));
    write(m_dataTable.ptr(), m_dataTable.size());

    int pb = sec.align;
    while (pb--)
        write8(0);
    return m_sizeOfData;
}

size_t BinaryWriter::writeCodeSection(void)
{
    TVMSection sec = {};
    sec.size       = (uint32_t)m_sizeOfCode;
    sec.start      = (uint32_t)sizeof(TVMHeader);
    sec.align      = getAlignment(m_sizeOfCode);

    uint64_t entry = findLabel("main");
    if (entry == -1)
    {
        printf("failed to find main entry point\n");
        return PS_ERROR;
    }
    sec.entry = (uint32_t)entry;

    write(&sec, sizeof(TVMSection));
    int i;

    Instructions::iterator it = m_ins.begin(), end = m_ins.end();
    while (it != end)
    {
        const Instruction& ins = (*it++);

        write8(ins.op);
        write8(ins.argc);
        write16(ins.flags);
        write16(ins.sizes);

        if (ins.flags & IF_RIDX)
            write8(ins.index);

        for (i = 0; i < ins.argc; ++i)
        {
            if (ins.sizes & SizeFlags[i][0])
                write8((uint8_t)ins.argv[i]);
            else if (ins.sizes & SizeFlags[i][1])
                write16((uint16_t)ins.argv[i]);
            else if (ins.sizes & SizeFlags[i][2])
                write32((uint32_t)ins.argv[i]);
            else
                write64(ins.argv[i]);
        }
    }

    int pb = sec.align;
    while (pb--)
        write8(0);
    return m_sizeOfCode;
}

size_t BinaryWriter::writeSymbolSection(void)
{
    TVMSection sec = {};
    sec.size       = (uint32_t)m_sizeOfSym;
    sec.entry      = m_header.sym;
    sec.align      = getAlignment(m_sizeOfSym);

    write(&sec, sizeof(TVMSection));

    strset_t::iterator it = m_linkedLibraries.begin();
    while (it != m_linkedLibraries.end())
    {
        const str_t& str = (*it++);
        write(str.c_str(), str.size());
        write8(0);
    }

    int pb = sec.align;
    while (pb--)
        write8(0);
    return m_sizeOfSym;
}

size_t BinaryWriter::writeStringSection(void)
{
    TVMSection sec = {};
    sec.size       = (uint32_t)m_sizeOfStr;
    sec.entry      = m_header.str;
    sec.align      = getAlignment(m_sizeOfStr);
    write(&sec, sizeof(TVMSection));

    strvec_t::iterator it = m_orderedString.begin();
    while (it != m_orderedString.end())
    {
        const str_t& str = (*it++);
        write(str.c_str(), str.size());
        write8(0);
    }

    int pb = sec.align;
    while (pb--)
        write8(0);
    return m_sizeOfStr;
}

int BinaryWriter::writeSections()
{
    if (!m_fp)
        return PS_ERROR;

    size_t size;
    if (m_sizeOfCode != 0)
    {
        size = writeCodeSection();
        if (size != m_sizeOfCode)
            return PS_ERROR;
    }

    if (m_sizeOfData != 0)
    {
        size = writeDataSection();
        if (size != m_sizeOfData)
            return PS_ERROR;
    }

    if (m_sizeOfSym != 0)
    {
        size = writeSymbolSection();
        if (size != m_sizeOfSym)
            return PS_ERROR;
    }

    if (m_sizeOfStr != 0)
    {
        size = writeStringSection();
        if (size != m_sizeOfStr)
            return PS_ERROR;
    }
    return PS_OK;
}
