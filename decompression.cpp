#include "constants.h"

unsigned char get_bit_in_byte(unsigned char symbol, unsigned char position, unsigned char bit) {
    return ((symbol & bit_masks[position]) >> (7 - position)) * bit_masks[bit];
}

unsigned char get_bit(unsigned char symbol, unsigned char position) {
    return ((symbol & bit_masks[position]) >> (7 - position));
}

unsigned char* read_bytes(FILE* source, unsigned char* buffer, int* buffer_pointer, int count) {
    auto* result = new unsigned char[count + 1];
    for(int index = 0; index < count; index++) {
        unsigned char c = 0;
        for(unsigned char i = 0; i < 8; i++) {
            if(*buffer_pointer == 0) {
                fread(buffer, 1, BUFFER_SIZE, source);
            }
            c += get_bit_in_byte(buffer[(*buffer_pointer) / 8], (*buffer_pointer) % 8, i);
            (*buffer_pointer)++;
            if(*buffer_pointer >= BUFFER_SIZE * 8) {
                *buffer_pointer = 0;
            }
        }
        result[index] = c;
    }
    result[count] = '\0';
    return result;
}

unsigned char* read_bits(FILE* source, unsigned char* buffer, int* buffer_pointer, int count) {
    auto* result = new unsigned char[count + 1];
    for(int index = 0; index < count; index++) {
        if(*buffer_pointer == 0) {
            fread(buffer, 1, BUFFER_SIZE, source);
        }
        unsigned char c = get_bit(buffer[(*buffer_pointer) / 8], (*buffer_pointer) % 8) + '0';
        (*buffer_pointer)++;
        if(*buffer_pointer >= BUFFER_SIZE * 8) {
            *buffer_pointer = 0;
        }
        result[index] = c;
    }
    result[count] = '\0';
    return result;
}

Tree* create_node() {
    Tree* result = new Tree();
    result->left = nullptr;
    result->right = nullptr;
    result->symbol = NO_SYMBOL;
    return result;
}

Tree* build_tree(FILE* source, unsigned char* buffer, int* buffer_pointer, const int symbol_types) {
    Tree* root = create_node();
    for(int i = 0; i < symbol_types; i++) {
        unsigned char* letter = read_bytes(source, buffer, buffer_pointer, 1);
        unsigned char* code_length = read_bytes(source, buffer, buffer_pointer, 1);
        unsigned char* code = read_bits(source, buffer, buffer_pointer, *code_length);
        Tree* node = root;
        for(unsigned char l = 0; l < *code_length; l++) {
            if(code[l] == '0') {
                if(node->left == nullptr) {
                    node->left = create_node();
                }
                node = node->left;
            }
            else {
                if(node->right == nullptr) {
                    node->right = create_node();
                }
                node = node->right;
            }
        }
        node->symbol = *letter;
        delete[] letter;
        delete[] code_length;
        delete[] code;
    }
    return root;
}

//очистка дерева
void delete_tree(Tree* root) {
    if(root->left != nullptr) {
        delete_tree(root->left);
    }
    if(root->right != nullptr) {
        delete_tree(root->right);
    }
    delete root;
}

int decompress_file(FILE* source, FILE* destination) {
    ULL file_size;
    int symbol_types;
    auto* write_buffer = new unsigned char[BUFFER_SIZE];
    auto* read_buffer = new unsigned char[BUFFER_SIZE];
    int buffer_pointer = 0;
    int write_buffer_pointer = 0;

    //читаем данные о файле
    unsigned char* sign = read_bytes(source, read_buffer, &buffer_pointer, static_cast<int>(signature.size()));
    if(std::strcmp(signature.data(), (char*)sign) != 0) {
        return WRONG_SIGNATURE;
    }
    unsigned char* _c_symbol_types = read_bytes(source, read_buffer, &buffer_pointer, sizeof(int));
    unsigned char* _c_file_size = read_bytes(source, read_buffer, &buffer_pointer, sizeof(ULL));
    symbol_types = *(int*)_c_symbol_types;
    file_size = *(ULL*)_c_file_size;
    if(symbol_types <= 0 || file_size <= 0) {
        return CORRUPTED_FILE;
    }
    delete[] sign;
    delete[] _c_file_size;
    delete[] _c_symbol_types;

    //строим дерево для декодирования
    Tree* tree = build_tree(source, read_buffer, &buffer_pointer, symbol_types);

    //декодируем файл
    for(ULL i = 0; i < file_size; i++) {
        Tree* node = tree;
        unsigned char* bit;
        while(node->symbol == NO_SYMBOL) {
            bit = read_bits(source, read_buffer, &buffer_pointer, 1);
            if(*bit == '0') {
                node = node->left;
            }
            else {
                node = node->right;
            }
            delete[] bit;
        }
        write_buffer[write_buffer_pointer] = static_cast<unsigned char>(node->symbol);
        write_buffer_pointer++;
        if(write_buffer_pointer == BUFFER_SIZE) {
            fwrite(write_buffer, 1, BUFFER_SIZE, destination);
            write_buffer_pointer = 0;
        }
    }
    if(write_buffer_pointer > 0) {
        fwrite(write_buffer, 1, static_cast<size_t>(write_buffer_pointer), destination);
    }

    delete[] write_buffer;
    delete[] read_buffer;
    delete_tree(tree);
    return EXIT_SUCCESS;
}