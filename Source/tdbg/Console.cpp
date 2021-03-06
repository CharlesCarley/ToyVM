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
#include "Console.h"
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#include "ConsoleWindows.h"
#else
#include "ConsoleCurses.h"
#endif  // _WIN32

using namespace std;

Console::Console() :
    m_std(),
    m_size(0),
    m_displayRect({0, 0, 0, 0}),
    m_curColor(CS_WHITE),
    m_lineCount(0),
    m_maxOutput({0, 0})
{
    initializeColorTable();
}

Console::~Console()
{
}

void Console::initializeColorTable(void)
{
    uint8_t i, j, c;
    for (i = 0; i < 16; ++i)
    {
        for (j = 0; j < 16; ++j)
        {
            c = i + j * 16;

            m_colorTable[j][i] = c;
        }
    }
}

uint8_t Console::getColorImpl(uint8_t fg, uint8_t bg)
{
    uint8_t rc = 0;
    if (!m_supportsColor)
        rc = CS_WHITE;
    else
    {
        if (bg == CS_TRANSPARENT)
            bg = CS_BLACK;

        if (bg < 16 && fg < 16)
            rc = m_colorTable[bg][fg];
    }
    return rc;
}

void Console::displayString(const str_t &string, int16_t x, int16_t y)
{
    int16_t i, s, k;

    if (x < 0)
        return;
    if (y < 0)
        return;

    if ((y < m_displayRect.y) || (y >= m_displayRect.h))
        return;
    if ((x < m_displayRect.x) || (x >= m_displayRect.w))
        return;

    s = (int16_t)string.size();

    const char *ptr = string.c_str();

    for (i = 0; i < s; ++i)
    {
        k = x + i;
        if (k >= m_displayRect.w)
            continue;

        k += y * m_displayRect.w;
        if (k < m_size)
            writeChar(ptr[i], m_curColor, k);
    }
}

void Console::displayChar(char ch, int16_t x, int16_t y)
{
    if ((y < 0) || (y >= m_displayRect.h))
        return;
    if ((x < 0) || (x >= m_displayRect.w))
        return;

    int16_t k = x + y * m_displayRect.w;
    if (k < (int16_t)m_size)
        writeChar(ch, m_curColor, k);
}

void Console::displayCharHex(int ch, int16_t x, int16_t y)
{
    if (ch >= 32 && ch < 127)
    {
        ostringstream ss;
        ss << ' ' << (char)ch;
        displayString(ss.str(), x, y);
    }
    else
    {
        ostringstream ss;
        ss << setfill('0') << setw(2);
        ss << hex << uppercase << ch;
        displayString(ss.str(), x, y);
    }
}

void Console::displayInt(int v, int16_t x, int16_t y)
{
    ostringstream ss;
    ss << v;
    displayString(ss.str(), x, y);
}

void Console::displayIntRightAligned(int v, int16_t r, int16_t y)
{
    ostringstream ss;
    ss << v;

    str_t str = ss.str();
    displayString(str, r - (int16_t)str.size(), y);
}

void Console::displayLineHorz(int16_t st, int16_t en, int16_t y)
{
    int16_t i;
    for (i = st; i < en; ++i)
        displayChar('-', i, y);
}

void Console::displayLineVert(int16_t st, int16_t en, int16_t x)
{
    int16_t i;
    for (i = st; i < en; ++i)
        displayChar('|', x, i);
}

void Console::displayOutput(int16_t x, int16_t y)
{
    int16_t st = x, skipln = 0;
    size_t  len = m_std.size(), i;

    m_lineCount += 1;
    if (m_lineCount > m_maxOutput.h)
        skipln += (m_lineCount)-m_maxOutput.h;

    m_lineCount = 0;
    for (i = 0; i < len; i++)
    {
        char ch = m_std.at(i);

        if ((ch == '\n') || (ch == '\r') || (x + 1 > m_maxOutput.right()))
        {
            if (m_lineCount >= skipln)
            {
                x = st;
                ++y;
            }
            if (ch == '\r')
            {
                if (i + 1 < len)
                {
                    if (m_std.at(i + 1) == '\n')
                        ++i;
                }
            }

            m_lineCount++;
        }
        else
        {
            if (m_lineCount >= skipln)
                displayChar(ch, x++, y);
        }
    }
}

int16_t Console::getOutputLineCount()
{
    size_t len = m_std.size(), i;

    int16_t x, y, x0;

    x  = m_maxOutput.x;
    y  = m_maxOutput.y;
    x0 = x;

    m_lineCount = 0;
    for (i = 0; i < len; i++)
    {
        char ch = m_std.at(i);

        if ((ch == '\n') || (ch == '\r') || (x + 1 > m_maxOutput.right()))
        {
            x = x0;
            ++y;

            if (ch == '\r')
            {
                if (i + 1 < len)
                {
                    if (m_std.at(i + 1) == '\n')
                        ++i;
                }
            }

            m_lineCount++;
        }
        else
        {
            ++x;
        }
    }

    return m_lineCount;
}

int Console::pause()
{
    int rc = CCS_NO_INPUT;
    while (rc == CCS_NO_INPUT)
    {
        rc = nextCommand();
        switch (rc)
        {
        case CCS_RESTART:
        case CCS_FORCE_EXIT:
        case CCS_QUIT:
            break;
        default:
            rc = CCS_NO_INPUT;
            break;
        }
    }
    return rc;
}

void Console::clearOutput()
{
    m_std.clear();
}

void Console::appendOutput(const str_t &str)
{
    m_std += str;
}

void Console::setColor(uint8_t fg, uint8_t bg)
{
    m_curColor = getColorImpl(fg, bg);
}

Console *GetPlatformConsole()
{
    Console *ret;

#ifdef _WIN32
    ret = new ConsoleWindows();
#else
    ret = new ConsoleCurses();
#endif  // _WIN32

    if (ret->create() != 0)
    {
        printf("failed to create platform console.\n");
        delete ret;
        ret = nullptr;
    }
    return ret;
}
