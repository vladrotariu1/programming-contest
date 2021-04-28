//
// Created by vlad on 1/5/21.
//

#include "helpfunc.h"

size_t file_size(char * path) {
    struct stat st;
    stat(path, &st);
    return st.st_size;
}