#include "constants.h"

void write_info() {
    std::cout << "Specify what you want to do:" << std::endl
              << "1. To compress file you need arguments '-c' and file name" << std::endl
              << "2. To decompress file you need arguments '-d' and file name" << std::endl;
}

void error(int error_id) {
    switch(error_id) {
        case WRONG_SIGNATURE:
            std::cout << "Wrong signature in source file, seems it not fits to this program." << std::endl;
            break;
        case CORRUPTED_FILE:
            std::cout << "Seems that file is corrupted, sorry." << std::endl;
            break;
        case NO_SUCH_FILE:
            std::cout << "Cannot find file." << std::endl;
            break;
    }
}

int main(int argc, char** argv) {
    if(argc != 3) {
        write_info();
    }
    else {
        if(strcmp("-c", argv[1]) == 0) {
            FILE* source = fopen(argv[2], "rb");
            FILE* destination = fopen((std::string(argv[2]) + ".gma").data(), "wb");
            if(source == nullptr || destination == nullptr) {
                error(NO_SUCH_FILE);
                return EXIT_FAILURE;
            }
            compress_file(source, destination);
        }
        else if(strcmp("-d", argv[1]) == 0) {
            FILE* source = fopen(argv[2], "rb");
            FILE* destination = fopen("a.out", "wb");
            if(source == nullptr || destination == nullptr) {
                error(NO_SUCH_FILE);
                return EXIT_FAILURE;
            }
            error(decompress_file(source, destination));
        }
        else {
            write_info();
        }
    }
    return EXIT_SUCCESS;
}