cmake_minimum_required(VERSION 3.21)
project(projeto C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(projeto
        Admistrador.c
        Admistrador.h
        balcao.c
        cliente.c
        medico.c
        servico.h
        Utilizador.h
        utils.h)
