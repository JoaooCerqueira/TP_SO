#include "Admistrador.h"
void deletar_cliente(Utilizador *arr, int *size, int pid);
void deletar_medico(Utilizador *arr, TSinal *arr_lista, int *size, int pid);
int encontrarMaior(Utilizador *arr_clientes, int size);
int quantidade_de_utilizador(int num, Utilizador *lista, char *especialidade);

//Threads
void fechar_thread_de_sinal(int size, TSinal *arr_sinal, int pid);
void *print_utentes_espera(void *dados);

void *sinal_devida(void *arg)
{
    TSinal *sinal = (TSinal *)arg;
    char str[40];
    sprintf(str, FIFO_SINAL, sinal->pid);
    mkfifo(str, 0700);
    strcpy(sinal->str, str);
    fd_set fds;
    struct timeval tempo;
    int fd, res, aux;

    printf("┌──────────────────────────────────────────────┐ \n");
    printf("│Foi adicionado um sinal de vida ao medico %d│\n", sinal->pid);
    printf("└──────────────────────────────────────────────┘ \n\n");
    fd = open(str, O_RDWR);

    do
    {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tempo.tv_sec = 20;
        tempo.tv_usec = 0;
        res = select(fd + 1, &fds, NULL, NULL, &tempo);
        if (res == 0)
        {
            printf("┌────────────────────────────────────────┐ \n");
            printf("│ O medico %d nao envio sinal de vida! │\n", sinal->pid);
            printf("└────────────────────────────────────────┘ \n\n");
            pthread_mutex_lock(sinal->m);
            deletar_medico(sinal->lista_de_medicos, sinal->lista_de_sinais, sinal->num_medicos, sinal->pid);
            pthread_mutex_unlock(sinal->m);
            break;
        }
        else if (res > 0 && FD_ISSET(fd, &fds))
        {
            res = read(fd, &aux, sizeof(int));
            if (aux == 0)
            {
                pthread_mutex_lock(sinal->m);
                deletar_medico(sinal->lista_de_medicos, sinal->lista_de_sinais, sinal->num_medicos, sinal->pid);
                pthread_mutex_unlock(sinal->m);
                break;
            }
            //printf("┌──────────────────────────────────┐ \n");
            // printf("│O medico %d envio sinal de vida!│\n", sinal->pid);
            // printf("└──────────────────────────────────┘ \n\n");
        }

    } while (1);
    close(fd);
    printf("---->Thread do pid %d terminada\n", sinal->pid);
    unlink(str);
    pthread_exit(NULL);
}

void *redirecinar_consulta(void *arg)
{
    Tlista *lista = (Tlista *)arg;
    while (lista->sair != 0)
    {
        for (size_t i = 0; i < (*lista->num_medicos); i++)
        {
            for (size_t j = 0; j < (*lista->num_clientes); j++)
            {
                if (strcmp(lista->lista_de_medicos[i].especialidade, lista->lista_de_clientes[j].especialidade) == 0 && lista->lista_de_medicos[i].em_dialogo == 0 && lista->lista_de_clientes[j].em_dialogo == 0)
                {
                    if (encontrarMaior(lista->lista_de_clientes, *lista->num_clientes) == lista->lista_de_clientes[j].prioridade)
                    {
                        printf("%d", lista->lista_de_clientes[j].em_dialogo);
                        int fd_retorno;
                        int res;
                        char str[40];
                        //Atualizar lista
                        strcpy(lista->lista_de_clientes[j].nome_a_atender, lista->lista_de_medicos[i].nome);
                        strcpy(lista->lista_de_medicos[i].nome_a_atender, lista->lista_de_clientes[j].nome);
                        lista->lista_de_clientes[j].em_dialogo = 1;
                        lista->lista_de_medicos[i].em_dialogo = 1;

                        //Enviar dados para medico para começar dialogo
                        sprintf(str, FIFO_MED, lista->lista_de_medicos[i].pid);
                        fd_retorno = open(str, O_WRONLY);
                        res = write(fd_retorno, &lista->lista_de_clientes[j], sizeof(Utilizador));
                        printf("*Foi encontrado um cliente para o medico %s - %d*\n", lista->lista_de_medicos[i].nome, lista->lista_de_clientes[j].pid);
                        close(fd_retorno);
                    }
                }
            }
        }
    }
    printf("---->Thread de redirecionamento fechado\n");
    pthread_exit(NULL);
}

void acorda(int sig, siginfo_t *info, void *uc){};

int main()
{
    //Verifica se existe fifo
    if (access(FIFO_BALCAO, F_OK) == 0)
    {
        perror("[Erro] ja existe um balcao");
        exit(1);
    }

    int fd;
    int pid;
    int balcao_to_classificador[2], classificador_to_balcao[2];
    pipe(balcao_to_classificador);
    pipe(classificador_to_balcao);

    int id = fork();

    if (id == -1)
    {
        perror("[ERROR]: Nao foi possivel criar um filho\n");
    }

    if (id == 0)
    {
        //Filho
        close(0);
        dup(balcao_to_classificador[0]);
        close(balcao_to_classificador[0]);
        close(balcao_to_classificador[1]);

        close(1);
        dup(classificador_to_balcao[1]);
        close(classificador_to_balcao[1]);
        close(classificador_to_balcao[0]);

        execl("classificador", "classificador", NULL);
        printf("[ERROR]: Nao foi possivel executar o classificador\n");
        exit(ERROR);
    }
    //Pai
    Utilizador utilizador;
    Servico servico;
    Tlista lista;
    int res;
    //thread para ler os clientes de n em n segundos
    pthread_t le;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    //variaveis para select
    fd_set fds;

    //criar fifo
    mkfifo(FIFO_BALCAO, 0700);
    printf(" ┌─────────────┐ \n");
    printf(" │Balcao criado│\n");
    printf(" └─────────────┘\n");
    //abrir o fifo
    fd = open(FIFO_BALCAO, O_RDWR);

    char *max_clientes_str = getenv("MAXCLIENTES");
    if (max_clientes_str == NULL)
    {
        printf("[ERROR] nao existe variavel de ambiente MAXCLIENTES\n");
        exit(0);
    }

    char *max_medicos_str = getenv("MAXMEDICOS");
    if (max_medicos_str == NULL)
    {
        printf("[ERROR] nao existe variavel de ambiente MAXMEDICOS");
        exit(0);
    }

    int MAX_CLIENTES = atoi(max_clientes_str);
    int MAX_MEDICOS = atoi(max_medicos_str);
    printf("MAX_MEDICOS %d\n", MAX_MEDICOS);
    printf("MAX_CLIENTES %d\n", MAX_CLIENTES);
    int aux = 0;
    //Listas
    Utilizador lista_de_clientes[MAX_CLIENTES];
    int num_clientes = 0;
    Utilizador lista_de_medicos[MAX_MEDICOS];
    TSinal sinal[MAX_MEDICOS];
    int num_medicos = 0;
    //estrutura para imprimir clientes em espera;
    Tle_clientes tdados;

    //Fechar os pipes que nao sao utilizados do lado do pai
    close(balcao_to_classificador[0]);
    close(classificador_to_balcao[1]);

    pid = getpid();
    char cmd[40];

    //Thread - Redirecionamento
    lista.sair = 1;
    lista.lista_de_clientes = lista_de_clientes;
    lista.lista_de_medicos = lista_de_medicos;
    lista.num_clientes = &num_clientes;
    lista.num_medicos = &num_medicos;

    //thread para fazer a consulta acontecer
    pthread_create(&lista.tid, NULL, redirecinar_consulta, (void *)&lista);

    //dados para a thread de escrever os clientes em espera
    struct sigaction act;

    act.sa_sigaction = acorda;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &act, NULL);

    //criar thread para ler os clientes de n em n segundos;
    tdados.lista_de_clientes = lista_de_clientes;
    tdados.tempo = 30;
    tdados.continua = 1;
    tdados.m = &mutex;
    tdados.num_clientes = &num_clientes;

    pthread_create(&le, NULL, &print_utentes_espera, (void *)&tdados);

    do
    {
        fflush(stdout);
        printf("\nAdm %d: \n", getpid());
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(fd, &fds);
        res = select(fd + 1, &fds, NULL, NULL, NULL);
        if (res > 0 && FD_ISSET(0, &fds))
        {
            // 1. - Administardor
            fgets(cmd, 40 - 1, stdin);
            strcat(cmd, "\0");
            if (strcmp(cmd, "encerrar\n") == 0)
            {
                break;
            }
            else if (strcmp(cmd, "utentes\n") == 0)
            {
                print_utentes(lista_de_clientes, num_clientes);
            }
            else if (strcmp(cmd, "especialistas\n") == 0)
            {
                print_especialistas(lista_de_medicos, sinal, num_medicos);
            }
            else if (strncmp(cmd, "delut", 5) == 0)
            {
                char num[10];
                strncpy(num, &cmd[6], strlen(cmd) - 6);
                int pid = atoi(num);
                if (pid == 0)
                {
                    printf("delut [PID]\n");
                }
                else
                {
                    encerrar_cliente(lista_de_clientes, pid, num_clientes);
                    deletar_cliente(lista_de_clientes, &num_clientes, pid);
                }
            }
            else if (strncmp(cmd, "delesp", 6) == 0)
            {
                char num[10];
                strncpy(num, &cmd[7], strlen(cmd) - 7);
                int pid = atoi(num);
                if (pid == 0)
                {
                    printf("delesp [PID]\n");
                }
                else
                {
                    encerrar_medico(lista_de_medicos, pid, num_medicos);
                    fechar_thread_de_sinal(num_medicos, sinal, utilizador.pid);
                }
            }
            else if (strncmp(cmd, "freq", 4) == 0)
            {
                char num[10];
                strncpy(num, &cmd[5], strlen(cmd) - 5);
                int tempo = atoi(num);
                tdados.tempo = tempo;
            }
            else
            {
                printf("*[ERRO] Esse comando nao existe!*\n");
                printf("Lista de comandos\n");
                printf("utentes - mostra a lista de utentes em espera e a serem atendidos\n");
                printf("especialistas - mostra a lista de espercialistas em espera e a antenderem um cliente\n");
                printf("delux [PID] - remove um cliente em espera da lista\n");
                printf("delesp [PID] - remove um medico em espera da lista\n");
            }
        }
        else if (res > 0 && FD_ISSET(fd, &fds))
        {
            // 2. - Recebe informacao vinda do Clinte/Medico
            res = read(fd, &utilizador, sizeof(Utilizador));

            // 2.1 - Verifica se é medico ou cliente
            if (strcmp(utilizador.sintoma, "") != 0)
            {
                // 2.2 -Cliente
                if (strcmp(utilizador.texto, "sair\n") == 0)
                {
                    //2.2.1 - Verfica se o cliente quer sair;
                    printf("*O cliente de id %d encerrou a sessao*\n", utilizador.pid);
                    deletar_cliente(lista_de_clientes, &num_clientes, utilizador.pid);
                }
                else if (verificaExistencia(lista_de_clientes, num_clientes, utilizador.pid) == -1)
                {
                    if (num_clientes >= MAX_CLIENTES)
                    {
                        printf("\n[ERRO] Numero maximo de clientes excedido - %d\n", MAX_CLIENTES);
                        enviar_mensagem_de_encerramento(&utilizador, FIFO_CLI);
                    }
                    else
                    {
                        // 2.2.2 - Envia sintoma para o classificador
                        res = write(balcao_to_classificador[1], utilizador.sintoma, strlen(utilizador.sintoma));
                        res = read(classificador_to_balcao[0], servico.relatorio, SIZE_ARR - 1);

                        if (res > 0)
                        {
                            servico.relatorio[res] = '\0';
                        }
                        else
                        {
                            exit(0);
                        }

                        // 2.2.3 - Coloca a especialidade e a prioridade na estrutura do cliente
                        char char2num = servico.relatorio[strlen(servico.relatorio) - 2];
                        utilizador.prioridade = atoi(&char2num);
                        strcpy(utilizador.especialidade, strtok(servico.relatorio, " "));
                        utilizador.numero_de_clientes = quantidade_de_utilizador(num_clientes, lista_de_clientes, utilizador.especialidade);
                        utilizador.numero_de_especialistas = quantidade_de_utilizador(num_medicos, lista_de_medicos, utilizador.especialidade);

                        if (quantidade_de_utilizador(num_clientes, lista_de_clientes, utilizador.especialidade) > 5 - 1)
                        {
                            printf("\n[ERRO] A lista de espera desta especialidade esta cheia - 5\n");
                            enviar_mensagem_de_encerramento(&utilizador, FIFO_CLI);
                            break;
                        }

                        // 2.2.4 - Envia para o cliente a sua especialidade e a prioridade
                        char cliente_str[40];
                        int fd_envio;

                        sprintf(cliente_str, FIFO_CLI, utilizador.pid);
                        fd_envio = open(cliente_str, O_WRONLY);
                        res = write(fd_envio, &utilizador, sizeof(Utilizador));
                        close(fd_envio);

                        // 2.2.5 - Adiciona o cliente a lista de clientes
                        printf("\n*Adicionado a lista de clientes '%s'*\n", utilizador.nome);
                        printf("---->Lista tecnica\n");
                        printf("---->Numero: %d\n---->Sintomas: %s", utilizador.pid, utilizador.sintoma);
                        printf("---->Foi colocado na especialidade: %s\n---->Prioridade %d\n", utilizador.especialidade, utilizador.prioridade);

                        ++num_clientes;

                        lista_de_clientes[num_clientes - 1] = utilizador;
                    }
                }
            }
            // 2.3 - Medico
            else
            {
                // 2.3.1 - Verifica se o medico terminou a sua consulta
                if (strcmp(utilizador.texto, "adeus\n") == 0)
                {
                    proximoCliente(lista_de_medicos, num_medicos, utilizador.pid);
                }
                // 2.3.2 - Verifica se o medico quer encerrar
                else if (strcmp(utilizador.texto, "sair\n") == 0)
                {
                    fechar_thread_de_sinal(num_medicos, sinal, utilizador.pid);
                }
                // 2.3.3 - Verifica se ja existe esse medico (caso o medico envie para o balcao informacao que nao seja - sair ou adeus)
                else if (verificaExistencia(lista_de_medicos, num_medicos, utilizador.pid) == -1)
                {
                    if (num_medicos >= MAX_MEDICOS)
                    {
                        printf("\n[ERRO] Numero maximo de medicos excedido - %d\n", MAX_MEDICOS);
                        enviar_mensagem_de_encerramento(&utilizador, FIFO_MED);
                    }
                    else
                    {
                        // 2.3.5 - Adiona a lista de medicos
                        printf("\n*Adicionado a lista de medicos: '%s'*\n", utilizador.nome);
                        printf("---->Lista tecnica\n");
                        printf("---->Numero: %d\n", utilizador.pid);
                        printf("---->Especialidade: %s\n", utilizador.especialidade);

                        ++num_medicos;
                        lista_de_medicos[num_medicos - 1] = utilizador;

                        pthread_t tid;
                        // 2.3.5 - Cria uma thread para o sinal de vida
                        sinal[num_medicos - 1].tid = tid;
                        sinal[num_medicos - 1].num_medicos = &num_medicos;
                        sinal[num_medicos - 1].lista_de_medicos = lista_de_medicos;
                        sinal[num_medicos - 1].pid = utilizador.pid;
                        sinal[num_medicos - 1].lista_de_sinais = sinal;
                        sinal[num_medicos - 1].m = &mutex;
                        pthread_create(&sinal[num_medicos - 1].tid, NULL, sinal_devida, (void *)&sinal[num_medicos - 1]);
                    }
                }
            }
        }
    } while (1);

    close(classificador_to_balcao[0]);
    write(balcao_to_classificador[1], "#fim\n", 4);
    close(balcao_to_classificador[1]);
    //Espera o classificador fechar
    printf("--->Processo fechado [%d]\n", wait(NULL));
    //Fechar o fifo
    close(fd);
    printf("--->Fechei o fifo...\n");
    //Apagar fifo
    unlink(FIFO_BALCAO);
    printf("--->*Apaguei o fifo%s*\n", FIFO_BALCAO);

    //3. Fechar Threads
    //3.1- Terminar thread de redirecionamento
    lista.sair = 0;
    //3.1- Terminar threads de vida
    for (int i = 0; i < num_medicos; i++)
    {
        char str[20];
        sprintf(str, FIFO_MED, lista_de_medicos[i].pid);
        strcpy(lista_de_medicos[i].texto, "sair\n");
        int fd = open(str, O_WRONLY);
        res = write(fd, &lista_de_medicos[i], sizeof(Utilizador));
        close(fd);
        fechar_thread_de_sinal(num_medicos, sinal, lista_de_medicos[i].pid);
    }
    for (int i = 0; i < num_clientes; i++)
    {
        char str[20];
        sprintf(str, FIFO_CLI, lista_de_clientes[i].pid);
        strcpy(lista_de_clientes[i].texto, "sair\n");
        int fd = open(str, O_WRONLY);
        res = write(fd, &lista_de_clientes[i], sizeof(Utilizador));
        close(fd);
    }
    //3.2- Espera todas as threads acabarem
    for (int i = 0; i < num_medicos; i++)
    {
        pthread_join(sinal[i].tid, &sinal[i].retval);
    }
    pthread_join(lista.tid, &lista.retval);

    //esperar que a thread le termine:
    tdados.continua = 0;
    pthread_kill(le, SIGUSR1);
    pthread_join(le, NULL);
    pthread_mutex_destroy(&mutex);
    printf("--->Theads encerradas\n");
    printf("Balcao Fechado\n");
    exit(0);
}

void deletar_cliente(Utilizador *arr, int *size, int pid)
{
    if (*size == 0)
    {
        printf("---->Nao existe Clientes para remover\n");
        return;
    }
    int i, flag = 0;
    // 1. - Encontrar a posicao no array
    for (i = 0; i < (*size); i++)
    {
        if (arr[i].pid == pid)
        {
            flag = 1;
            break;
        }
    }
    // 2. - Mover
    if (flag == 1)
    {
        for (; i + 1 < (*size); i++)
        {
            arr[i] = arr[i + 1];
        }
        --(*size);
        printf("---->O Utilizador %d foi removido com sucesso\n", pid);
    }
    else
    {
        printf("---->O Cliente que pretende remover nao existe\nUse o comando 'utentes' para ver os clientes na lista\n");
    }
}

void deletar_medico(Utilizador *arr, TSinal *arr_sinal, int *size, int pid)
{
    if (*size == 0)
    {
        printf("---->Nao existe Medicos para remover\n");
        return;
    }
    int i, flag = 0;
    // 1. - Encontrar a posicao no array
    for (i = 0; i < (*size); i++)
    {
        if (arr[i].pid == pid)
        {
            flag = 1;
            break;
        }
    }
    // 2. - Mover
    if (flag == 1)
    {
        for (; i + 1 < (*size); i++)
        {
            arr[i] = arr[i + 1];
            arr_sinal[i] = arr_sinal[i + 1];
        }
        --(*size);
        printf("---->O Utilizador %d foi removido com sucesso\n", pid);
    }
    else
    {
        printf("---->O Medico que pretende remover nao existe\nUse o comando 'especialistas' para ver os medicos na lista\n");
    }
}

void fechar_thread_de_sinal(int size, TSinal *arr_sinal, int pid)
{
    int i, flag = 0;
    for (i = 0; i < size; i++)
    {
        if (arr_sinal[i].pid == pid)
        {
            flag = 1;
            break;
        }
    }
    // 2.3.2.2 - Envia mensagem para a thread para terminar
    if (flag == 1)
    {
        int end, res;
        int flag = 0;
        end = open(arr_sinal[i].str, O_WRONLY);
        res = write(end, &flag, sizeof(int));
        close(end);
    }
    else
    {
        printf("Nao existe essa thread\n");
    }
}

int encontrarMaior(Utilizador *arr_clientes, int size)
{
    int maior = 0;
    for (size_t i = 0; i < size; i++)
    {
        if (arr_clientes[i].prioridade > maior)
        {
            maior = arr_clientes[i].prioridade;
        }
    }
    return maior;
}

void mostra(Tle_clientes *pdados)
{
    char *especialidades[] = {"oftalmologia", "neurologia", "estomatologia", "ortopedia", "geral"};
    for (size_t j = 0; j < 5; j++)
    {
        printf("----->%s:\n", especialidades[j]);
        for (size_t i = 0; i < *pdados->num_clientes; i++)
        {
            if (pdados->lista_de_clientes[i].em_dialogo == 0 && strcmp(especialidades[j], pdados->lista_de_clientes[i].especialidade) == 0)
            {
                printf("Cliente %s - %d pid\n", pdados->lista_de_clientes[i].nome, pdados->lista_de_clientes[i].pid);
                printf("prioridade - %d\n", pdados->lista_de_clientes[i].prioridade);
                printf("Especialidade - %s\n", pdados->lista_de_clientes[i].especialidade);
                printf("----------------------------------\n");
            }
        }
    }
}

void *print_utentes_espera(void *dados)
{
    Tle_clientes *pdados = (Tle_clientes *)dados;
    int n = 0;
    do
    {
        pthread_mutex_lock(pdados->m);
        if (n == 0)
        {
            if (*pdados->num_clientes == 0)
            {
                printf("\n*Ainda nao existe nenhum cliente em espera*\n");
            }
            else
            {
                mostra(pdados);
            }
        }
        pthread_mutex_unlock(pdados->m);
        n = sleep(pdados->tempo);
    } while (pdados->continua);
    printf("Thread de mostra clientes fechada\n");
    pthread_exit(NULL);
}

int quantidade_de_utilizador(int num, Utilizador *lista, char *especialidade)
{
    int count = 0;
    for (size_t i = 0; i < num; i++)
    {
        if (strcmp(lista->especialidade, especialidade) == 0 && lista->em_dialogo == 0)
        {
            ++count;
        }
    }
    return count;
}