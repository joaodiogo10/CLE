#ifndef UTF8_H
#define UTF8_H

#include <stdio.h>

enum CharacterType
{
    CONSOANT,
    UNDERSCORE,
    VOWEL,
    DIGIT,
    APOSTROPHE,
    DELIMITER,
    EOFILE,
    NOT_DEFINED,
    ERROR
};

/** \brief read UTF8 value size of utf8 char */
int  readUTF8Char(FILE *ptrFile, unsigned int *utf8Char);

enum CharacterType getUTF8CharType(unsigned int utf8Char);

int getUTF8CharSize(unsigned char firstByte);

#endif /* UTF8_H */