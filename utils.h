#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/select.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include "Utilizador.h"
#include "servico.h"

#define ERROR 33
#define SIZE_ARR 100
#define FIFO_BALCAO "/tmp/BALCAO"
#define FIFO_CLI "/tmp/cli%d"
#define FIFO_MED "/tmp/med%d"
#define FIFO_SINAL "/tmp/sinal%d"

//Threads
typedef struct TSinal TSinal;

struct TSinal
{
  pthread_t tid;
  void *retval;
  Utilizador *lista_de_medicos;
  TSinal *lista_de_sinais;
  int *num_medicos;
  char str[40];
  int pid;
  pthread_mutex_t *m;
};


typedef struct
{
  pthread_t tid;
  int sair;
  Utilizador *lista_de_medicos;
  int *num_medicos;
  Utilizador *lista_de_clientes;
  int *num_clientes;
  void *retval;
} Tlista;


typedef struct
{
	Utilizador *lista_de_clientes;
	int *num_clientes;
	int tempo;
  int continua;
	pthread_mutex_t *m;
}Tle_clientes;


#endif // UTILS_H