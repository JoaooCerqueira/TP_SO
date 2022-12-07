#include "Admistrador.h"

void print_utentes(Utilizador *arr, int size)
{
  if (size == 0)
  {
    printf("\n*Ainda nao existe nenhum cliente neste momento*\n");
  }
  for (size_t i = 0; i < size; i++)
  {
    if (arr[i].em_dialogo == 0)
    {
      printf("Cliente %s - %d pid\n", arr[i].nome, arr[i].pid);
      printf("prioridade - %d\n", arr[i].prioridade);
      printf("Especialidade - %s\n", arr[i].especialidade);
      printf("----------------------------------\n");
    }
  }
  for (size_t i = 0; i < size; i++)
  {
    if (arr[i].em_dialogo == 1)
    {
      printf("Cliente %s - %d pid\n", arr[i].nome, arr[i].pid);
      printf("prioridade - %d\n", arr[i].prioridade);
      printf("Medico - %s\n", arr[i].nome_a_atender);
      printf("Especialidade - %s\n", arr[i].especialidade);
      printf("----------------------------------\n");
    }
  }
}

void print_especialistas(Utilizador *arr, TSinal *arr_sinal, int size)
{
  if (size == 0)
  {
    printf("\n*Ainda nao existe nenhum Medico na lista*\n");
  }
  for (size_t i = 0; i < size; i++)
  {
    if (arr[i].em_dialogo == 0)
    {
      printf("Medico %s - %d pid\n", arr[i].nome, arr[i].pid);
      printf("Especialidade - %s\n", arr[i].especialidade);
      printf("Thread funcionando - %d\n", arr_sinal[i].pid);
      printf("----------------------------------\n");
    }
  }
  for (size_t i = 0; i < size; i++)
  {
    if (arr[i].em_dialogo == 1)
    {
      printf("Medico %s - %d pid\n", arr[i].nome, arr[i].pid);
      printf("Especialidade - %s\n", arr[i].especialidade);
      printf("Cliente - %s\n", arr[i].nome_a_atender);
      printf("Thread funcionando - %d\n", arr_sinal[i].pid);
      printf("----------------------------------\n");
    }
  }
}

void encerrar_cliente(Utilizador *arr, int pid, int size)
{
  char cliente_str[40];
  int fd, res;
  for (size_t i = 0; i < size; i++)
  {
    if (arr[i].em_dialogo == 0 && arr[i].pid == pid)
    {
      printf("O cliente %d foi removido com sucesso da lista\n", arr[i].pid);
      sprintf(cliente_str, FIFO_CLI, arr[i].pid);
      strcpy(arr[i].texto, "sair\n");
      int fd = open(cliente_str, O_WRONLY);
      res = write(fd, &arr[i], sizeof(Utilizador));
      close(fd);

      return;
    }
  }
  printf("Nao existe cliente com esse pid\n");
}

void encerrar_medico(Utilizador *arr, int pid, int size)
{
  int fd;
  for (size_t i = 0; i < size; i++)
  {
    if (arr[i].em_dialogo == 0 && arr[i].pid == pid)
    {
      printf("O Medico %d foi removido com sucesso da lista\n", arr[i].pid);
      enviar_mensagem_de_encerramento(&arr[i], FIFO_MED);
      return;
    }
  }
  printf("Nao existe medico com esse pid\n");
}

void enviar_mensagem_de_encerramento(Utilizador *enviar, char *fifo)
{
  int res;
  char str[40];
  sprintf(str, fifo, enviar->pid);
  strcpy(enviar->texto, "sair\n");
  int fd = open(str, O_WRONLY);
  res = write(fd, enviar, sizeof(Utilizador));
  close(fd);
}

int verificaExistencia(Utilizador *arr, int size, int pid)
{
  for (size_t i = 0; i < size; i++)
  {
    if (arr[i].pid == pid)
    {
      return 0;
    }
  }
  return -1;
}

void proximoCliente(Utilizador *arr, int size, int pid)
{
  for (size_t i = 0; i < size; i++)
  {
    if (arr[i].pid = pid)
    {
      arr[i].em_dialogo = 0;
      printf("O medico %s esta pronto para receber outro cliente", arr[i].nome);
    }
  }
}