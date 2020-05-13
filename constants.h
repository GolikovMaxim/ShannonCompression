#pragma once

#ifndef SHANNON_COMPRESSION_CONSTANTS_H
#define SHANNON_COMPRESSION_CONSTANTS_H

#include <iostream>
#include <string>
#include <cmath>
#include <cstring>
#include <cstdio>

#define ULL unsigned long long

#define BUFFER_SIZE 1024
#define ALPHABET_SIZE 256
#define NO_CODE "-"

extern unsigned char bit_masks[];
extern unsigned char other_bits_masks[];
extern std::string signature;

//список для работы с кумулятивными вероятностями
#define List struct list
List {
    //List* prev;
    List* next;
    double cumulative_chance;
    double chance;
    int symbol;
};

//для работы с деревом
#define Tree struct tree
#define NO_SYMBOL -1
Tree {
    Tree* left;
    Tree* right;
    int symbol;
};

//коды ошибок
#define WRONG_SIGNATURE 1
#define CORRUPTED_FILE 2
#define NO_SUCH_FILE 3

void clear_buffer(char* buffer);
char change_bit(char symbol, char position, char bit, char c);
void compress_file(FILE* source, FILE* destination);
int decompress_file(FILE* source, FILE* destination);

#endif //SHANNON_COMPRESSION_CONSTANTS_H
