#define SIZE_ARR 100

typedef struct
{
    //Dados inciciais (enviados para o balcao)
    char nome[SIZE_ARR];
    char sintoma[SIZE_ARR];
    char especialidade[SIZE_ARR];
    int prioridade;
    int pid;

    //Recebido do balcao
    char nome_a_atender[SIZE_ARR];
    int em_dialogo;
    int numero_de_clientes;
    int numero_de_especialistas;
    //Dialogo entre cliente e medico
    char texto[SIZE_ARR];
} Utilizador;
