#ifndef UTF8_H
#define UTF8_H

#include <stdio.h>

/**
 *  \file utf8.h
 *
 *  \brief UTF8 character processing library header
 *  
 *  Provides a set of useful functions in processing of UTF8 characters.
 *
 *  \author João Diogo Ferreira, João Tiago Rainho - April 2022
 */


/** \brief Represents the type of a utf8 char */
enum CharacterType
{
    CONSOANT,
    UNDERSCORE,
    VOWEL,
    DIGIT,
    APOSTROPHE,
    DELIMITER,
    EOFILE,
    NOT_DEFINED
};

/** \brief Reads next utf8 character in the file stream 
 *  
 *  \param ptrFile pointer to the file stream
 *  \param[out] utf8Char pointer to read utf8 character
 * 
 *  \returns The size of the utf8 character, 0 in case EOF is read from the file stream or 
 *           -1 in case of an encoding error or an error reading from the file stream
*/
int readUTF8Char(FILE *ptrFile, unsigned int *utf8Char);

/** \brief Determines the character type of an utf8 character 
 *  
 *  \param utf8Char utf8 character
 * 
 *  \returns corresponding enum character type
*/
enum CharacterType getUTF8CharType(unsigned int utf8Char);

/** \brief Determines the size of an utf8 character through its first byte 
 *  
 *  The size of a utf8 character can range from 1 to 4 bytes. The first bit most
 *  follow a valid utf8 encoding format.
 * 
 *  \param firstByte utf8 character first byte
 *  \returns The size of the utf8 character or 0 in case of an invalid encoding format
*/
int getUTF8CharSize(unsigned char firstByte);

#endif /* UTF8_H */