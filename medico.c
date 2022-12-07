#include "utils.h"

Utilizador medico;
Utilizador cliente;

int maiorfd(int fd_envio, int fd_retorno);
void print_my_info();
void print_cliente_info();

typedef struct
{
  pthread_t tid;
  int fd;
  int continua;
  void *retval;
} Tsinal;

void *enviar_sinal_de_vida(void *arg)
{
  Tsinal *sinal = (Tsinal *)arg;
  int fd, res, flag = 1;

  char str[40];
  sprintf(str, FIFO_SINAL, getpid());
  while (sinal->continua)
  {
    int n = sleep(20);
    if (n != 0)
    {
      flag = 0;
    }
    fd = open(str, O_WRONLY);
    res = write(fd, &flag, sizeof(int));
    //printf("  -> Sinal de vida enviado\n\n");
    close(fd);
  }
  printf("Thread de sinal fechada\n");
  pthread_exit(NULL);
}

void acorda(int sig, siginfo_t *info, void *uc){};

int main(int argc, char *argv[])
{
  setbuf(stdout, NULL);
  //verifica se introduziu o nome do clinte
  if (argc != 3)
  {
    perror("[ERRO]\n./medico [nome] [especialidade]\n");
    exit(-1);
  }

  //verificar se existe um fifo
  if (access(FIFO_BALCAO, F_OK) != 0)
  {
    perror("[Erro] o balcao nao esta a funcionar\n");
    exit(1);
  }

  struct sigaction act;

  act.sa_sigaction = acorda;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGUSR1, &act, NULL);

  //string para criar fifo
  char str[40];
  int to_balcao, fd_retorno, fd_envio;

  //Inserir nome e especialidade
  strcpy(medico.nome, argv[1]);
  strcpy(medico.especialidade, argv[2]);
  strcpy(medico.sintoma, ""); //serve para indentifar no balcao se é um medico ou um cliente
  medico.em_dialogo = 0;

  //Buscar o pid do medico
  medico.pid = getpid();

  //criar o nome para o fifo (ex: med+pid)
  sprintf(str, FIFO_MED, medico.pid);

  //criar fifo do medico
  mkfifo(str, 0700);

  //Abrir fifo do medico
  fd_retorno = open(str, O_RDWR);

  //Abrir balcao para escrita
  to_balcao = open(FIFO_BALCAO, O_WRONLY);

  //Envia informação para o balcao
  int res;
  res = write(to_balcao, &medico, sizeof(Utilizador));
  //Recebe o cliente - para começar a consulta
  Tsinal sinal;
  fd_set fds;

  //Imprime a informacao do medico
  print_my_info();

  sinal.fd = fd_retorno;
  sinal.continua = 1;
  pthread_create(&sinal.tid, NULL, enviar_sinal_de_vida, (void *)&sinal);

  // 1. - Fica a espera de um cliente
  while (strcmp(medico.texto, "sair\n") != 0)
  {
    do
    {
      FD_ZERO(&fds);
      FD_SET(0, &fds);
      FD_SET(fd_retorno, &fds);
      res = select(fd_retorno + 1, &fds, NULL, NULL, NULL);
      if (res > 0 && FD_ISSET(0, &fds))
      {
        //Enviar informacao para o balcao (sair)
        fgets(medico.texto, SIZE_ARR - 1, stdin);
        strcat(medico.texto, "\0");
        if (strcmp(medico.texto, "sair\n") == 0)
        {
          res = write(to_balcao, &medico, sizeof(Utilizador));
          break;
        }
        else
        {
          printf("\n*So existe o comando sair*\n");
        }
      }
      else if (res > 0 && FD_ISSET(fd_retorno, &fds))
      {
        //E inciado a conversa com o medico
        res = read(fd_retorno, &cliente, sizeof(Utilizador));
        if (strcmp(cliente.texto, "sair\n") == 0)
        {
          strcpy(medico.texto, "sair\n");
          printf("Encerrar sessao [Requesitado por balcao]\n");
        }
        break;
      }
    } while (1);

    if (strcmp(cliente.texto, "sair\n") != 0 && strcmp(medico.texto, "sair\n") != 0)
    {
      print_cliente_info();
      printf("\n***********Chat Room***********\n\n");
      char cliente_str[40];
      sprintf(cliente_str, FIFO_CLI, cliente.pid);
      do
      {
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(fd_retorno, &fds);
        res = select(fd_retorno + 1, &fds, NULL, NULL, NULL);
        if (res > 0 && FD_ISSET(0, &fds))
        {
          fgets(medico.texto, SIZE_ARR - 1, stdin);
          strcat(medico.texto, "\0");
          fd_envio = open(cliente_str, O_WRONLY);
          res = write(fd_envio, &medico, sizeof(Utilizador));
          close(fd_envio);
        }
        else if (res > 0 && FD_ISSET(fd_retorno, &fds))
        {
          res = read(fd_retorno, &cliente, sizeof(Utilizador));
          if (strcmp(cliente.texto, "sair\n") == 0)
          {
            strcpy(medico.texto, "sair\n");
            printf("O balcao encerrou\n");
            break;
          }
          printf("Cliente %s: %s", cliente.nome, cliente.texto);
        }
      } while (strcmp(medico.texto, "sair\n") != 0 && strcmp(medico.texto, "adeus\n") != 0 && strcmp(cliente.texto, "adeus\n") != 0);
      
      if (strcmp(medico.texto, "sair\n") != 0)
      {
        printf("\nConsulta concluida espere pela proxima\n");
      }
      //informa balcao que esta disponivel
      strcpy(medico.texto, "adeus\n");
      res = write(to_balcao, &medico, sizeof(Utilizador));
    }
  }

  //Fechar o fifo
  close(fd_retorno);
  close(to_balcao);
  printf("Fechei o fifo...\n");
  //Apagar fifo
  printf("%s\n", str);
  unlink(str);
  printf("Apaguei o fifo...\n");
  sinal.continua = 0;
  pthread_kill(sinal.tid, SIGUSR1);
  pthread_join(sinal.tid, NULL);
}

int maiorfd(int fd_envio, int fd_retorno)
{
  if (fd_envio > fd_retorno)
  {
    return fd_envio;
  }
  return fd_retorno;
}

void print_my_info()
{
  printf("┌");
  for (int i = 0; i < maiorfd(strlen(medico.nome), strlen(medico.especialidade) + 16); i++)
  {
    printf("─");
  }
  printf("┐\n");
  printf("│Nome: %s", medico.nome);
  for (int i = 6 + strlen(medico.nome); i < maiorfd(strlen(medico.nome), strlen(medico.especialidade) + 16); i++)
  {
    printf(" ");
  }
  printf("│\n");
  printf("│Pid: %d", medico.pid);
  for (int i = 5 + 4; i < maiorfd(strlen(medico.nome), strlen(medico.especialidade) + 16); i++)
  {
    printf(" ");
  }
  printf("│\n");
  printf("│Especialidade: %s", medico.especialidade);
  for (int i = 15 + strlen(medico.especialidade); i < maiorfd(strlen(medico.nome), strlen(medico.especialidade) + 16); i++)
  {
    printf(" ");
  }
  printf("│\n");
  printf("└");
  for (int i = 0; i < maiorfd(strlen(medico.nome), strlen(medico.especialidade) + 16); i++)
  {
    printf("─");
  }
  printf("┘\n\n");
  printf("*Foi registado no balcao*\n");
  printf("*Aguarde ate um cliente chegar*\n\n\n");
}

void print_cliente_info()
{
  printf("\n\nLigacao Efetuada\n");
  printf("┌");
  for (int i = 0; i < maiorfd(strlen(cliente.nome), strlen(cliente.sintoma) + 16); i++)
  {
    printf("─");
  }
  printf("┐\n");

  printf("│Ficha do cliente:");
  for (int i = 17; i < maiorfd(strlen(cliente.nome), strlen(cliente.sintoma) + 16); i++)
  {
    printf(" ");
  }
  printf("│\n");

  printf("│Nome: %s", cliente.nome);
  for (int i = 6 + strlen(cliente.nome); i < maiorfd(strlen(cliente.nome), strlen(cliente.sintoma) + 16); i++)
  {
    printf(" ");
  }
  printf("│\n");

  printf("│Prioridade: %d", cliente.prioridade);
  for (int i = 12 + 1; i < maiorfd(strlen(cliente.nome), strlen(cliente.sintoma) + 16); i++)
  {
    printf(" ");
  }
  printf("│\n");

  printf("│Pid: %d", cliente.pid);
  for (int i = 5 + 4; i < maiorfd(strlen(cliente.nome), strlen(cliente.sintoma) + 16); i++)
  {
    printf(" ");
  }
  printf("│\n");

  printf("└");
  for (int i = 0; i < maiorfd(strlen(cliente.nome), strlen(cliente.sintoma) + 16); i++)
  {
    printf("─");
  }
  printf("┘\n\n");
}