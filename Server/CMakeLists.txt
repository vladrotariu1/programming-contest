cmake_minimum_required(VERSION 3.17)
project(Server C)

set(CMAKE_C_STANDARD 99)

add_executable(Server main.c database.c database.h queue.c queue.h helpfunc.c helpfunc.h)
target_link_libraries(Server LINK_PUBLIC sqlite3 pthread)