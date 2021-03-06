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
#include <stdio.h>
#include <stdlib.h>
#include "SharedLib.h"
#include "SymbolUtils.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <windows.h>
#endif

SYM_API SYM_EXPORT void __putchar(tvmregister_t regi)
{
    uint8_t ch = prog_get_register8(regi, 0);
    if (ch)
        putchar(ch);
}

SYM_API SYM_EXPORT void __puts(tvmregister_t regi)
{
    size_t ptr = (size_t)prog_get_register64(regi, 0);
    if (ptr)
        puts((char*)ptr);
}

SYM_API SYM_EXPORT void __getchar(tvmregister_t regi)
{
    prog_set_register8(regi, 0, getchar());
}

const SymbolTable stdlib[] = {
    {"putchar", __putchar},
    {"puts", __puts},
    {"getchar", __getchar},
    {nullptr, nullptr},
};

SYM_API SYM_EXPORT SymbolTable* std_init()
{
    return (SymbolTable*)stdlib;
}

#ifdef _WIN32
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_THREAD_ATTACH:
        break;
    case DLL_PROCESS_ATTACH:
        // Work around for process synchronization
        // issues with tests. Another option is to
        // call fflush(stdout) after every call.
        setvbuf(stdout, 0, _IONBF, 0);
        break;
    default:
        break;
    }
    return TRUE;
}
#endif
