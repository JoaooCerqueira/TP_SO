#include "utils.h"

Utilizador cliente;
Utilizador medico;

int maiorfd(int fd_envio, int fd_retorno);
void print_my_info();
void print_medico_info();

int main(int argc, char *argv[])
{
  setbuf(stdout, NULL);
  //verifica se introduziu o nome do clinte
  if (argc != 2)
  {
    perror("ERRO\n./cliente [nome]\n");
    exit(-1);
  }

  //verificar se existe um fifo
  if (access(FIFO_BALCAO, F_OK) != 0)
  {
    perror("[Erro] o balcao nao esta a funcionar\n");
    exit(1);
  }

  //string para criar fifo
  char str[40];

  int to_balcao;
  int res, fd_retorno, fd_envio;

  //Inserir os dados na estrutura
  cliente.pid = getpid();
  strcpy(cliente.nome, argv[1]);
  cliente.em_dialogo = 0;

  //criar o nome para o fifo (ex: cli+pid)
  sprintf(str, FIFO_CLI, cliente.pid);

  //criar fifo do cliente
  mkfifo(str, 0700);

  //abrir o fifo do cliente para leitura e escrita
  fd_retorno = open(str, O_RDWR);

  //Abrir o fifo do balcao para escrita
  to_balcao = open(FIFO_BALCAO, O_WRONLY);

  //Pede sintomas
  printf("Quais sao os seu sintomas\n");
  fgets(cliente.sintoma, SIZE_ARR - 1, stdin);
  //vai mandar para o balcao a estrutura do cliente;
  res = write(to_balcao, &cliente, sizeof(Utilizador));
  //Recebe a especialidade do fifo do balcao
  res = read(fd_retorno, &cliente, sizeof(Utilizador));

  //verifica maximo de clientes
  if (strcmp(cliente.texto, "sair\n") != 0)
  {
    printf("*Numero de clientes a frente - %d*\n", cliente.numero_de_clientes);
    printf("*Numero de medicos no sistema %s - %d*\n", cliente.especialidade, cliente.numero_de_especialistas);
    printf("\n\n*Foi registado no balcao*\n");
    printf("*Vai ser tratado por um medico de %s*\n\n", cliente.especialidade);

    //valida o sintoma
    print_my_info();

    //variaveis
    fd_set fds;
    struct timeval tempo;

    // 1. - Fica a espera de um medico
    do
    {
      printf("*Pode informar o balcao que pretender sair Escrevendo 'sair'*\n\n");
      FD_ZERO(&fds);
      FD_SET(0, &fds);
      FD_SET(fd_retorno, &fds);
      FD_SET(to_balcao, &fds);
      res = select(maiorfd(to_balcao, fd_retorno) + 1, &fds, NULL, NULL, NULL);
      if (res > 0 && FD_ISSET(0, &fds))
      {
        // 1.1 - Se quiser sair (Escreve sair no stdin)
        fgets(cliente.texto, SIZE_ARR - 1, stdin);
        if (strcmp(cliente.texto, "sair\n") == 0)
        {
          res = write(to_balcao, &cliente, sizeof(Utilizador));
          break;
        }
        else
        {
          printf("So existe o comando sair\n");
        }
      }
      else if (res > 0 && FD_ISSET(to_balcao, &fds))
      {
        // 1.2 - Recebe informacao do balcao para encerrar
        res = read(fd_retorno, &cliente, sizeof(Utilizador));
        if (strcmp(cliente.texto, "sair\n") == 0)
        {
          break;
        }
      }
      else if (res > 0 && FD_ISSET(fd_retorno, &fds))
      {
        // 1.3 - Recebe informacao do medico para inciar a consultaa
        res = read(fd_retorno, &medico, sizeof(Utilizador));
        break;
      }
    } while (1);

    // 2. - Conversa com o medico
    if (strcmp(cliente.texto, "sair\n") != 0)
    {
      printf("Medico chamou-me para a consulta\n\n");
      print_medico_info();
      printf("\n***********Chat Room***********\n\n");
      printf("Medico %s: %s", medico.nome, medico.texto);

      char medico_str[100];
      sprintf(medico_str, FIFO_MED, medico.pid);

      while (strcmp(cliente.texto, "adeus\n") != 0 && strcmp(medico.texto, "adeus\n") != 0)
      {
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(fd_retorno, &fds);
        res = select(fd_retorno + 1, &fds, NULL, NULL, NULL);
        if (res > 0 && FD_ISSET(0, &fds))
        {
          // 2.1 - Envia mensagem para o medico
          fgets(cliente.texto, SIZE_ARR - 1, stdin);
          strcat(cliente.texto, "\0");
          fd_envio = open(medico_str, O_WRONLY);
          res = write(fd_envio, &cliente, sizeof(Utilizador));
          close(fd_envio);
        }
        else if (res > 0 && FD_ISSET(fd_retorno, &fds))
        {
          // 2.2 - Recebe mensagem do medico
          res = read(fd_retorno, &medico, sizeof(Utilizador));
          if (strcmp(medico.texto, "sair\n") == 0)
          {
            printf("O balcao encerrou\n");
            break;
          }
          printf("Medico %s: %s", medico.nome, medico.texto);
        }
      }
      printf("Consulta terminada!\n");
      strcpy(cliente.texto, "sair\n");
      res = write(to_balcao, &cliente, sizeof(Utilizador));
    }
  }

  // 3. - Avisa o balcao que vai encerrar

  //Fechar o fifo
  close(fd_retorno);
  printf("*Fechei a comunicacao com o %s*\n", str);
  close(to_balcao);
  printf("*Fechei a comunicacao com o %s*\n", FIFO_BALCAO);

  //Apagar fifo
  printf("*Apaguei o meu namedpipe*\n");
  unlink(FIFO_CLI);
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
  for (int i = 0; i < maiorfd(strlen(cliente.nome), strlen(cliente.sintoma) + 16); i++)
  {
    printf("─");
  }
  printf("┐\n");

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

  printf("│Sintoma: ");
  fwrite(cliente.sintoma, 1, strlen(cliente.sintoma) - 1, stdout);
  for (int i = 8 + strlen(cliente.sintoma); i < maiorfd(strlen(cliente.nome), strlen(cliente.sintoma) + 16); i++)
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

void print_medico_info()
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
}