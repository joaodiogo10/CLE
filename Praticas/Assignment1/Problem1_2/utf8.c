#include "utf8.h"
// Space character (0x20)
// A tab character (0x9)
// A newline character (0xA)
// A carriage return character (0xD)

// Separation symbol:
// Hyphen (- 0x2D)
// Double quotation mark (" 0x22 - “ 0xE2809C - ” 0xe2809D)
// Bracket ([ 0x5B - ] 0x5D)
// Parentheses (( 0x28 - ) 0x29)

// Punctuation symbol:
// Full point (. 0x2E)
// Comma (, 0x2C)
// Colon (: 0x3A)
// Semicolon (; 0x3B)
// Question mark (? 0x3F)
// Exclamation point (! 0x21)
// Dash (—, 0xE28093)
// Ellipsis (…, 0xE280A6) 
const unsigned int delimiters[] = {0x20, 0x9, 0xA, 0xD,                                     // Space character
                                   0x2D, 0x22, 0xE2809C, 0xe2809D, 0x5D, 0x29,              // Separation symbol
                                   0x2E, 0x2C, 0x3A, 0x3B, 0x3F, 0x21, 0xE28093, 0xE280A6}; // Punctuation symbol

//ç (0xC3A7)
//Ç (0xC387)
const unsigned int specialConsoants[] = {0xC3A7, 0xC387}; // c

// a (0x61), á (0xC3A1), à (0xC3A0), â (0xC3A2), ã (0xC3A3), A (0x41), Á (0xC381), À (0xC380), Â (0xC382), Ã (0xC383)
// e (0x65), é (0xC3A9), è (0xC3A8), ê (0xC3AA), E (0x45), É (0xC389), È (0xC388), Ê (0xC38A)
// i (0x69), í (0xC3AD), ì (0xC3AC), I (0x49), Í (0xC38D), Ì (0xC38C)
// o (0x6F), ó (0xC3B3), ò (0xC3B2), ô (0xC3B4), õ (0xC3B5), O (0x4F), Ó (0xC393), Ò (0xC392), Ô (0xC394), Õ (0xC395)
// u (0x75), ú (0xC3BA), ù (0xC3B9), U (0x55), Ú (0xC39A), Ù (0xC399)
const unsigned int vowels[] = {0x61, 0xC3A1, 0xC3A0, 0xC3A2, 0xC3A3, 0x41, 0xC381, 0xC380, 0xC382, 0xC383, // a
                               0x65, 0xC3A9, 0xC3A8, 0xC3AA, 0x45, 0xC389, 0xC388, 0xC38A,                 // e
                               0x69, 0xC3AD, 0xC3AC, 0x49, 0xC38D, 0xC38C,                                 // i
                               0x6F, 0xC3B3, 0xC3B2, 0xC3B4, 0xC3B5, 0x4F, 0xC393, 0xC392, 0xC394, 0xC395, // o
                               0x75, 0xC3BA, 0xC3B9, 0x55, 0xC39A, 0xC399};                                // u


// underscore (_ 0x5F)
const unsigned int underscore = 0x5F;

// apostrophe ('  0x27)
const unsigned int apostrophe = 0x27;

#define ASCII_FIRST_UPPER_CASE_LETTER 0x41
#define ASCII_LAST_UPPER_CASE_LETTER 0x5A

#define ASCII_FIRST_LOWER_CASE_LETTER 0x61
#define ASCII_LAST_LOWER_CASE_LETTER 0x7A

int readUTF8Char(FILE *ptrFile, unsigned int *utf8Char)
{
    unsigned char buffer[3];
    if (fread(&buffer, sizeof(char), 1, ptrFile) != sizeof(char))
    {
        if (ferror(ptrFile) != 0)
            return 0;

        if (feof(ptrFile) != 0)
            return 0;
    }

    unsigned char tmp = buffer[0] & 0xF8;
    *utf8Char = buffer[0];

    if (tmp >> 7 == 0) // 1 byte character
    {
        return 1;
    }
    else if (tmp >> 5 == 0b110) // 2 bytes character
    {
        if (fread(&buffer, sizeof(char), 1, ptrFile) != sizeof(char))
            return 0;

        *utf8Char = (*utf8Char << 8) | buffer[0];
        return 2;
    }
    else if (tmp >> 4 == 0b1110) // 3 bytes character
    {
        if (fread(&buffer, sizeof(char), 2, ptrFile) != sizeof(char) * 2)
            return 0;

        *utf8Char = (*utf8Char << 16) | (buffer[0] << 8) | buffer[1];

        return 3;
    }
    else if (tmp >> 3 == 0b1110) // 4 bytes character
    {
        if (fread(&buffer, sizeof(char), 3, ptrFile) != sizeof(char) * 3)
            return 0;

        *utf8Char = (*utf8Char << 24) | (buffer[0] << 16) | (buffer[1] << 8) | buffer[0];
        return 4;
    }
    // invalid
    return 0;
}

enum CharacterType getUTF8CharType(unsigned int utf8Char)
{
    // Check if is a delimiter
    for (int i = 0; i < sizeof(delimiters) / sizeof(*delimiters); i++)
    {
        if (delimiters[i] == utf8Char)
            return DELIMITER;
    }

    // check if is a apostrophe
    if (utf8Char == apostrophe)
        return APOSTROPHE;

    // Check if is a vowel
    for (int i = 0; i < sizeof(vowels) / sizeof(*vowels); i++)
    {
        if (vowels[i] == utf8Char)
            return VOWEL;
    }

    // Check if is a consoant
    for (int i = 0; i < sizeof(specialConsoants) / sizeof(*specialConsoants); i++)
    {
        if (specialConsoants[i] == utf8Char)
        {
            return CONSOANT;
        }
    }
    if ((ASCII_LAST_LOWER_CASE_LETTER >= utf8Char && utf8Char >= ASCII_FIRST_LOWER_CASE_LETTER) ||
        (ASCII_LAST_UPPER_CASE_LETTER >= utf8Char && utf8Char >= ASCII_FIRST_UPPER_CASE_LETTER))
    {
        return CONSOANT;
    }

    // Check if is a digit
    if (utf8Char >= 0x30 && utf8Char <= 0x39)
        return DIGIT;

    // Check if is an underscore
    if (utf8Char == underscore)
    {
        return UNDERSCORE;
    }

    return NOT_DEFINED;
}

int getUTF8CharSize(unsigned char firstByte)
{  
    if (firstByte >> 7 == 0) return 1;
    else if (firstByte >> 5 == 0b110) return 2;
    else if (firstByte >> 4 == 0b1110) return 3;
    else if (firstByte >> 3 == 0b1110) return 4;
    else return 4;
}