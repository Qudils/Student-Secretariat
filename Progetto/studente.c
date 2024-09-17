#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "wrapper.h"

#define MAX_RETRY 3            // Numero di tentativi massimi di connessione
#define MAX_WAIT_TIME 5        // Tempo massimo di attesa casuale in secondi
#define SIMULA_TIMEOUT 0       // Imposta a 1 per simulare timeout, 0 per disabilitare
#define TIMEOUT_SIMULATO_SEC 2 // Tempo di attesa per simulare un timeout

// Studente:
// Chiede alla segreteria se ci siano esami disponibili per un corso
// Invia una richiesta di prenotazione di un esame alla segreteria

// Definizione della struct
struct Esame
{
    char nome[100];
    char data[100];
};

struct Richiesta
{
    int TipoRichiesta;
    struct Esame esame;
};

// Dichiarazioni delle funzioni
int numeroPrenotazione(int socket_studente, int *numero_prenotazione);
void mandaMatricola(int socket_studente, char *matricola);
void mandaEsamePrenotazione(int socket_studente, struct Esame *esameDaPrenotare);
int ConnessioneSegreteria(int *socket_studente, struct sockaddr_in *indirizzo_server_segreteria);
void riceviListaEsami(int socket_studente, int numero_esami, struct Esame *esameCercato);
int conto_esami(int socket_studente, int *numero_esami);
void attendiTempoCasuale();
void simulaTimeout();

int main()
{
    int socket_studente;                            // Socket per la connessione con la segreteria
    struct sockaddr_in indirizzo_server_segreteria; // Indirizzo del server della segreteria
    struct Richiesta richiesta_studente = {};       // Richiesta dello studente
    int numero_esami = 0;                           // Numero di esami disponibili
    struct Esame *esameCercato = NULL;              // Struct di esami disponibili

    printf("\nConnesso alla segreteria Esse4!\n");

    // Menu di scelta per lo studente
    while (1)
    {
        printf("\n\n1 - Vedi esami disponibili \n2 - Prenota un esame\n");
        printf("Scelta: ");
        scanf("%d", &richiesta_studente.TipoRichiesta);
        getchar(); // Pulisce il buffer di input

        // Richiesta di esami disponibili
        if (richiesta_studente.TipoRichiesta == 1)
        {

            printf("Nome esame che vuoi cercare: ");
            fgets(richiesta_studente.esame.nome, sizeof(richiesta_studente.esame.nome), stdin);
            richiesta_studente.esame.nome[strcspn(richiesta_studente.esame.nome, "\n")] = '\0'; // Rimuove il newline

            // Assicura che il nome dell'esame non sia vuoto
            if (strlen(richiesta_studente.esame.nome) > 0)
            {
                socket_studente = ConnessioneSegreteria(&socket_studente, &indirizzo_server_segreteria); // Connessione con la segreteria
                
                // Invia la richiesta
                if (write(socket_studente, &richiesta_studente, sizeof(richiesta_studente)) != sizeof(richiesta_studente))
                {
                    perror("Errore invio richiesta esami");
                    close(socket_studente);
                    exit(1);
                }

                // Contiamo il numero di esami
                numero_esami = conto_esami(socket_studente, &numero_esami);
                
                // Se ci sono esami disponibili
                if (numero_esami > 0)
                {
                    esameCercato = (struct Esame *)malloc(sizeof(struct Esame) * numero_esami); // Allocazione della memoria
                    if (esameCercato == NULL)
                    {
                        perror("Errore di allocazione memoria");
                        close(socket_studente);
                        exit(1);
                    }
                    // Riceviamo la lista degli esami
                    riceviListaEsami(socket_studente, numero_esami, esameCercato);

                    printf("Numero\tNome\tData\n");
                    printf("-----------------\n");
                     
                    // Stampa la lista degli esami
                    for (int i = 0; i < numero_esami; i++)
                    {
                        printf("%d\t%s\t%s\n", i + 1, esameCercato[i].nome, esameCercato[i].data);
                    }

                    free(esameCercato); // Libera la memoria
                }
                else
                {
                    printf("Non ci sono esami disponibili per il corso cercato.\n");
                }

                close(socket_studente);
            }
        }
        else if (richiesta_studente.TipoRichiesta == 2) // Richiesta di prenotazione
        {
            printf("Nome esame che vuoi prenotare: ");
            fgets(richiesta_studente.esame.nome, sizeof(richiesta_studente.esame.nome), stdin);
            richiesta_studente.esame.nome[strcspn(richiesta_studente.esame.nome, "\n")] = '\0'; // Rimuove il newline

            socket_studente = ConnessioneSegreteria(&socket_studente, &indirizzo_server_segreteria);

            if (write(socket_studente, &richiesta_studente, sizeof(richiesta_studente)) != sizeof(richiesta_studente))
            {
                perror("Errore invio richiesta prenotazione");
                close(socket_studente);
                exit(1);
            }

            numero_esami = conto_esami(socket_studente, &numero_esami);

            printf("Ci sono esami disponibili per il corso cercato.\n");

            if (numero_esami > 0)
            {
                printf("Numero esami: %d\n", numero_esami);
                esameCercato = (struct Esame *)malloc(sizeof(struct Esame) * numero_esami);
                if (esameCercato == NULL)
                {
                    perror("Errore di allocazione memoria");
                    close(socket_studente);
                    exit(1);
                }

                riceviListaEsami(socket_studente, numero_esami, esameCercato);

                printf("Numero\tNome\tData\n");
                printf("-----------------\n");

                for (int i = 0; i < numero_esami; i++)
                {
                    printf("%d\t%s\t%s\n", i + 1, esameCercato[i].nome, esameCercato[i].data);
                }

                // Scelta dell'appello a cui prenotarsi
                int scelta_esame;
                printf("\nInserisci il numero dell'esame a cui vuoi prenotarti: ");
                scanf("%d", &scelta_esame);
                getchar(); // Pulisce il buffer di input

                // Verifica che la scelta sia valida
                if (scelta_esame > 0 && scelta_esame <= numero_esami)
                {
                    char matricola[11];
                    printf("Inserisci la tua matricola: ");
                    fgets(matricola, sizeof(matricola), stdin);
                    matricola[strcspn(matricola, "\n")] = '\0'; // Rimuove il newline

                    // Prendi l'esame selezionato
                    struct Esame esameDaPrenotare = esameCercato[scelta_esame - 1];

                    // Invia l'esame da prenotare
                    mandaEsamePrenotazione(socket_studente, &esameDaPrenotare);

                    // invia la matricola
                    mandaMatricola(socket_studente, matricola);

                    // Ricevi il numero di prenotazione
                    int prenotazione;
                    numeroPrenotazione(socket_studente, &prenotazione);

                    printf("Prenotazione completata con successo. Il tuo numero di prenotazione è: %d\n", prenotazione);
                }
                else
                {
                    printf("Scelta non valida.\n");
                }

                free(esameCercato);
            }

            close(socket_studente);
        }
    }

    return 0;
}


int numeroPrenotazione(int socket_studente, int *numero_prenotazione)
{
    // Ricevi il numero di prenotazione
    if (read(socket_studente, numero_prenotazione, sizeof(*numero_prenotazione)) != sizeof(*numero_prenotazione))
    {
        perror("Errore ricezione numero prenotazione");
        exit(1);
    }
    return *numero_prenotazione;
}

void mandaMatricola(int socket_studente, char *matricola)
{
    // Invia la matricola
    size_t length = strlen(matricola); 
    if (write(socket_studente, matricola, length) != length)
    {
        perror("Errore invio matricola");
        exit(1);
    }
}

void mandaEsamePrenotazione(int socket_studente, struct Esame *esameDaPrenotare)
{
    // Invia la struct Esame Esame
    if (write(socket_studente, esameDaPrenotare, sizeof(struct Esame)) != sizeof(struct Esame))
    {
        perror("Errore invio esame prenotazione");
        exit(1);
    }
    printf("Esame inviato dallo studente: Nome = %s, Data = %s\n", esameDaPrenotare->nome, esameDaPrenotare->data);
}

int ConnessioneSegreteria(int *socket_studente, struct sockaddr_in *indirizzo_server_segreteria)
{
    int retry_count = 0;

    // Configurazione dell'indirizzo del server della segreteria
    indirizzo_server_segreteria->sin_family = AF_INET;
    indirizzo_server_segreteria->sin_port = htons(2000);

    if (inet_pton(AF_INET, "127.0.0.1", &indirizzo_server_segreteria->sin_addr) <= 0)
    {
        perror("Errore inet_pton");
        exit(EXIT_FAILURE);
    }

    // Ciclo per tentare la connessione con massimo 3 tentativi
    while (retry_count < MAX_RETRY)
    {
        *socket_studente = Socket(AF_INET, SOCK_STREAM, 0);

        // Simula un timeout solo per i primi tentativi
        if (SIMULA_TIMEOUT && retry_count < MAX_RETRY)
        {
            simulaTimeout(); // Simula il timeout impostando errno
        }

        // Se non c'è errore o se errno non è impostato al valore di timeout, tenta la connessione
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            // Tentativo di connessione
            Connetti(*socket_studente, (struct sockaddr *)indirizzo_server_segreteria, sizeof(*indirizzo_server_segreteria));

            // Se Connetti non solleva errori, la connessione è riuscita
            printf("Connessione alla segreteria riuscita.\n");
            return *socket_studente; // Esci con successo
        }

        // Errore di connessione (simulato o reale), incrementa il contatore e ritenta
        printf("\nTentativo di riconnessione in corso (%d/%d)...\n", retry_count + 1, MAX_RETRY);
        retry_count++;
        sleep(2);                // Pausa breve tra i tentativi
        close(*socket_studente); // Chiudi il socket prima di riprovare
    }

    // Se si superano i tentativi massimi, attende un tempo casuale e resetta il contatore
    attendiTempoCasuale();
    retry_count = 0; // Resetta il contatore dei tentativi dopo l'attesa casuale

    // Ripeti il ciclo di tentativi dopo l'attesa casuale
    while (retry_count < MAX_RETRY)
    {
        *socket_studente = Socket(AF_INET, SOCK_STREAM, 0);

        // Tenta la connessione reale
        Connetti(*socket_studente, (struct sockaddr *)indirizzo_server_segreteria, sizeof(*indirizzo_server_segreteria));

        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            printf("Connessione alla segreteria riuscita dopo attesa.\n");
            return *socket_studente;
        }

        printf("Tentativo di riconnessione in corso (%d/%d)...\n", retry_count + 1, MAX_RETRY);
        retry_count++;
        sleep(2);
        close(*socket_studente);
    }

    // Se il ciclo termina senza successo
    fprintf(stderr, "Impossibile connettersi alla segreteria dopo ripetuti tentativi.\n");
    exit(EXIT_FAILURE);
}

void attendiTempoCasuale()
{
    // Imposta il seme per la generazione casuale basato sul tempo corrente
    srand(time(NULL));
    int tempoAttesa = rand() % MAX_WAIT_TIME + 1; // Genera un numero tra 1 e MAX_WAIT_TIME
    printf("\nAttesa di %d secondi prima di ritentare la connessione...\n", tempoAttesa);
    sleep(tempoAttesa); // Pausa per il tempo generato
}

void riceviListaEsami(int socket_studente, int numero_esami, struct Esame *esameCercato)
{
    // Ricevi la lista degli esami
    if (read(socket_studente, esameCercato, sizeof(struct Esame) * numero_esami) != sizeof(struct Esame) * numero_esami)
    {
        perror("Errore ricezione lista esami");
        exit(1);
    }
}

int conto_esami(int socket_studente, int *numero_esami)
{
    // Ricevi il numero di esami disponibili
    if (read(socket_studente, numero_esami, sizeof(*numero_esami)) != sizeof(*numero_esami))
    {
        perror("Errore ricezione numero esami");
        exit(1);
    }
    return *numero_esami;
}

void simulaTimeout()
{
    // Simula un timeout impostando errno a EAGAIN
    if (SIMULA_TIMEOUT)
    {
        printf("Simulazione di timeout: attesa di %d secondi...\n", TIMEOUT_SIMULATO_SEC);
        sleep(TIMEOUT_SIMULATO_SEC); // Simula un'attesa che rappresenta il timeout
        errno = EAGAIN;              // Simula un errore di timeout impostando errno
    }
}
