#ifndef ADMINISTRADOR_H
#define ADMINISTRADOR_H

#include "utils.h"

void print_utentes(Utilizador *arr, int size);
void print_especialistas(Utilizador *arr, TSinal *arr_sinal, int size);
void encerrar_cliente(Utilizador *arr, int pid, int size);
void encerrar_medico(Utilizador *arr, int pid, int size);
void enviar_mensagem_de_encerramento(Utilizador *enviar, char *fifo);
int verificaExistencia(Utilizador *arr, int size, int pid);
void proximoCliente(Utilizador *arr, int size, int pid);
#endif // ADMINISTRADOR_H