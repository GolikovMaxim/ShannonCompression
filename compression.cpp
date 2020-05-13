#include "constants.h"

unsigned char bit_masks[] = { 128, 64, 32, 16, 8, 4, 2, 1};
unsigned char other_bits_masks[] = { 127, 191, 223, 239, 247, 251, 253, 254};
std::string signature = "GM23";

void clear_buffer(char* buffer) {
    for(int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = 0;
    }
}

List* create_list_node() {
    List* result = new List();
    result->next = nullptr;
    //result->prev = nullptr;
    result->chance = 0;
    result->cumulative_chance = 0;
    result->symbol = -1;
    return result;
}

List* sort_list(List* list) {
    List* first = list;
    for(int i = 0; i < ALPHABET_SIZE; i++) {
        List* current = first;
        List* previous = nullptr;
        while(current->next != nullptr) {
            if(current->chance < current->next->chance) {
                if(current == first) {
                    first = current->next;
                    previous = first;
                    List* tmp = current->next->next;
                    current->next->next = current;
                    current->next = tmp;
                }
                else {
                    List *tmp = current->next->next;
                    current->next->next = current;
                    previous->next = current->next;
                    previous = current->next;
                    current->next = tmp;
                }
            }
            else {
                previous = current;
                current = current->next;
            }
        }
    }
    return first;
}

List* get_chances(FILE* file, ULL* file_size) {
    ULL* frequencies = new ULL[ALPHABET_SIZE];
    auto* buffer = new unsigned char[BUFFER_SIZE];

    for(int i = 0; i < ALPHABET_SIZE; i++) {
        frequencies[i] = 0;
    }

    fseek(file, 0, SEEK_END);
    *file_size = static_cast<unsigned long long int>(ftello(file));
    fseek(file, 0, SEEK_SET);
    while(!feof(file)) {
        size_t count = fread(buffer, 1, BUFFER_SIZE, file);
        for(size_t i = 0; i < count; i++) {
            frequencies[buffer[i]]++;
        }
    }

    List* start = create_list_node();
    List* current = start;
    for(int i = 0; i < ALPHABET_SIZE; i++) {
        if(frequencies[i] != 0) {
            current->symbol = i;
            current->chance = (double)frequencies[i] / *file_size;
            current->next = create_list_node();
            current = current->next;
        }
    }
    start = sort_list(start);

    delete[] frequencies;
    delete[] buffer;
    return start;
}

void set_cumulative_chances(List* chances) {
    List* current = chances;
    double cumulative_chance = 0;
    while(current != nullptr) {
        current->cumulative_chance = cumulative_chance;
        cumulative_chance += current->chance;
        current = current->next;
    }
}

std::string get_code(const double chance, double cumulative_chance) {
    if(chance == 0) {
        return NO_CODE;
    }
    int length = (int)std::ceil(-std::log2(chance));
    if(chance == 1) {
        length = 1;
    }
    char* str = new char[length + 1];
    for(int i = 0; i < length; i++) {
        cumulative_chance *= 2;
        str[i] = (char)(cumulative_chance) + '0'; //знаки после запятой
        cumulative_chance = cumulative_chance - (char)(cumulative_chance);
    }
    str[length] = '\0';
    return std::string(str);
}

std::string* get_codes(List* chances) {
    auto* codes = new std::string[ALPHABET_SIZE];
    while(chances->next != nullptr) {
        codes[chances->symbol] = get_code(chances->chance, chances->cumulative_chance);
        chances = chances->next;
    }
    return codes;
}

char change_bit(char symbol, char position, char bit, char c) {
    char c1 = symbol & other_bits_masks[position];
    char c2 = ((c & bit_masks[bit]) >> (position - bit));
    char c3 = ((c & bit_masks[bit]) << (bit - position));
    return c1 + ((position > bit) ? c2 : c3);
}

char set_bit(char symbol, char position, char bit) {
    return (symbol & other_bits_masks[position]) + bit * bit_masks[position];
}

void write_bytes(FILE* dest, char* buffer, int* buffer_pointer, const std::string& source) {
    for (char c : source) {
        char s = 0;
        for(char i = 0; i < 8; i++) {
            //немного побитовой магии
            buffer[(*buffer_pointer) / 8] = change_bit(buffer[(*buffer_pointer) / 8], (*buffer_pointer) % 8, i, c);
            (*buffer_pointer)++;
            if(*buffer_pointer >= BUFFER_SIZE * 8) {
                fwrite(buffer, 1, BUFFER_SIZE, dest);
                clear_buffer(buffer);
                *buffer_pointer = 0;
            }
        }
    }
}

void write_bytes(FILE* dest, char* buffer, int* buffer_pointer, const char* source, int size) {
    for(int index = 0; index < size; index++) {
        char c = source[index];
        for(char i = 0; i < 8; i++) {
            //еще немного побитовой магии
            buffer[(*buffer_pointer) / 8] = change_bit(buffer[(*buffer_pointer) / 8], (*buffer_pointer) % 8, i, c);
            (*buffer_pointer)++;
            if(*buffer_pointer >= BUFFER_SIZE * 8) {
                fwrite(buffer, 1, BUFFER_SIZE, dest);
                clear_buffer(buffer);
                *buffer_pointer = 0;
            }
        }
    }
}

void write_bits(FILE* dest, char* buffer, int* buffer_pointer, const std::string& source) {
    for (char c : source) {
        //и еще чуть-чуть
        buffer[(*buffer_pointer) / 8] = set_bit(buffer[(*buffer_pointer) / 8], (*buffer_pointer) % 8, c - '0');
        (*buffer_pointer)++;
        if(*buffer_pointer >= BUFFER_SIZE * 8) {
            fwrite(buffer, 1, BUFFER_SIZE, dest);
            clear_buffer(buffer);
            *buffer_pointer = 0;
        }
    }
}

void write_header(FILE* dest, std::string* codes, ULL file_size, char* buffer, int* buffer_pointer) {
    int symbol_types = 0;

    //считаем количество встречающихся типов символов
    for(int i = 0; i < ALPHABET_SIZE; i++) {
        if(codes[i] != NO_CODE) {
            symbol_types++;
        }
    }

    //сигнатура
    write_bytes(dest, buffer, buffer_pointer, signature);
    //записываем количество типов символов
    write_bytes(dest, buffer, buffer_pointer, (char*)&symbol_types, sizeof(int));
    //записываем размер файла
    write_bytes(dest, buffer, buffer_pointer, (char*)&file_size, sizeof(ULL));

    //записываем коды символов
    char letter, code_length;
    for(int i = 0; i < ALPHABET_SIZE; i++) {
        if(codes[i] != NO_CODE) {
            letter = static_cast<char>(i);
            write_bytes(dest, buffer, buffer_pointer, &letter, 1);
            code_length = static_cast<char>(codes[i].size());
            write_bytes(dest, buffer, buffer_pointer, &code_length, 1);
            write_bits(dest, buffer, buffer_pointer, codes[i]);
        }
    }
}

void compress_file(FILE* source, FILE* destination) {
    ULL file_size;
    char* write_buffer = new char[BUFFER_SIZE];
    auto* read_buffer = new unsigned char[BUFFER_SIZE];
    int buffer_pointer = 0;

    clear_buffer(write_buffer);

    //определяем коды символов
    List* chances = get_chances(source, &file_size);
    set_cumulative_chances(chances);
    std::string* codes = get_codes(chances);
    fseek(source, 0, SEEK_SET);

    //записываем информацию, необходимую для декодирования
    write_header(destination, codes, file_size, write_buffer, &buffer_pointer);

    //кодируем файл
    while(!feof(source)) {
        size_t count = fread(read_buffer, 1, BUFFER_SIZE, source);
        for(size_t i = 0; i < count; i++) {
            write_bits(destination, write_buffer, &buffer_pointer, codes[read_buffer[i]]);
        }
    }
    if(buffer_pointer > 0) {
        fwrite(write_buffer, 1, static_cast<size_t>(std::ceil(buffer_pointer / 8.0)), destination);
    }

    delete[] write_buffer;
    delete[] read_buffer;
    delete[] chances;
}