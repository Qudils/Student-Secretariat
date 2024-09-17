#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include "wrapper.h"

// Definizione delle struct
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

// Funzioni dichiarate
int ascolto_studenti(struct sockaddr_in *indirizzo_segreteria);
void esami_disponibili(struct Esame esame, int segreteria_connessione_socket);
void invio_esame_server(int socket_esami, struct Richiesta ricezione_esami);
int connessione_universita(struct sockaddr_in *indirizzo_universita);
void mandaEsameNuovoServer(struct Esame esame);
void inserisci_nuovo_esame(void);
void MandaPrenotazioneEsame(int socket_prenotazione_esame, struct Esame esame);
void RiceviMatricola(int segreteria_connessione_socket, int socket_prenotazione_esame);
void EsitoPrenotazione(int segreteria_connessione_socket, int socket_prenotazione_esame);
void MandaNumeroPrenotazione(int segreteria_connessione_socket, int socket_prenotazione_esame);
void riceviEsame(int socket, struct Esame *esame);

int main()
{
    int segreteria_ascolto_socket;
    struct sockaddr_in indirizzo_segreteria;
    int scelta;

    // Inizializzazione del socket per l'ascolto degli studenti
    segreteria_ascolto_socket = ascolto_studenti(&indirizzo_segreteria);

    // Menu per la segreteria
    while (1)
    {
        printf("\nScegli un'azione:\n");
        printf("1. Aggiungi un nuovo esame\n");
        printf("2. Ascolta le richieste degli studenti\n");
        printf("Inserisci la tua scelta: ");
        scanf("%d", &scelta);
        getchar(); // Consuma il newline lasciato da scanf

        // Aggiunta di un nuovo esame
        if (scelta == 1)
        {
            inserisci_nuovo_esame();
            continue;
        }
        else if (scelta == 2) // Ascolto delle richieste degli studenti
        {
            printf("\nAscolto studenti sulla porta 2000...\n");

            // Processo figlio per gestire le richieste in modo continuo
            pid_t pid = fork();

            if (pid < 0)
            {
                perror("Errore nella fork");
                exit(EXIT_FAILURE);
            }

            if (pid == 0)
            { // Processo figlio
                while (1)
                {
                    int segreteria_connessione_socket;
                    struct Richiesta richiesta_ricevuta;

                    // Gestione continua delle richieste degli studenti
                    segreteria_connessione_socket = Accetta(segreteria_ascolto_socket, (struct sockaddr *)NULL, NULL);

                    if (read(segreteria_connessione_socket, &richiesta_ricevuta, sizeof(struct Richiesta)) != sizeof(struct Richiesta))
                    {
                        perror("Errore nella lettura della richiesta");
                        close(segreteria_connessione_socket);
                        continue; // Continua a gestire altre richieste
                    }

                    if (richiesta_ricevuta.TipoRichiesta == 1)
                    {
                        // Richiesta di visualizzare esami disponibili
                        esami_disponibili(richiesta_ricevuta.esame, segreteria_connessione_socket);
                    }
                    else if (richiesta_ricevuta.TipoRichiesta == 2)
                    {
                        // Gestione della prenotazione esame
                        int socket_prenotazione_esame;
                        struct sockaddr_in indirizzo_universita;
                        struct Richiesta prenotazione_esame;
                        prenotazione_esame.TipoRichiesta = 2; // TipoRichiesta 2 per prenotare un esame

                        socket_prenotazione_esame = connessione_universita(&indirizzo_universita);

                        printf("Connessione al server universitario riuscita\n");

                        esami_disponibili(richiesta_ricevuta.esame, segreteria_connessione_socket);
                        riceviEsame(segreteria_connessione_socket, &richiesta_ricevuta.esame);

                        MandaPrenotazioneEsame(socket_prenotazione_esame, richiesta_ricevuta.esame);
                        RiceviMatricola(segreteria_connessione_socket, socket_prenotazione_esame);

                        EsitoPrenotazione(segreteria_connessione_socket, socket_prenotazione_esame);
                        MandaNumeroPrenotazione(segreteria_connessione_socket, socket_prenotazione_esame);

                        printf("Prenotazione esame completata\n--------------------------------------------\n");

                        close(socket_prenotazione_esame);
                    }
                    else
                    {
                        fprintf(stderr, "Tipo di richiesta non valido\n");
                    }

                    close(segreteria_connessione_socket); // Chiudi il socket dopo aver gestito la richiesta
                }

                // Uscita dal processo figlio in caso di errore critico (opzionale)
                close(segreteria_ascolto_socket);
                exit(EXIT_SUCCESS);
            }
            else
            { // Processo padre

                while (1)
                {
                    wait(NULL); // Attende la terminazione di qualsiasi processo figlio
                }
            }
        }
        else
        {
            printf("Scelta non valida. Riprova.\n");
        }
    }

    close(segreteria_ascolto_socket);
    return 0;
}

void MandaNumeroPrenotazione(int segreteria_connessione_socket, int socket_prenotazione_esame)
{
    int numero_prenotazione;

    // Ricezione del numero di prenotazione dal server universitario
    if (read(socket_prenotazione_esame, &numero_prenotazione, sizeof(numero_prenotazione)) != sizeof(numero_prenotazione))
    {
        perror("Errore ricezione numero prenotazione dal server universitario");
        exit(EXIT_FAILURE);
    }

    // Invio del numero di prenotazione al client studente tramite la segreteria
    if (write(segreteria_connessione_socket, &numero_prenotazione, sizeof(numero_prenotazione)) != sizeof(numero_prenotazione))
    {
        perror("Errore invio numero prenotazione allo studente");
        exit(EXIT_FAILURE);
    }

    printf("Numero prenotazione inviato allo studente: %d\n", numero_prenotazione);
}

// Funzione per ricevere la struttura Esame
void riceviEsame(int socket, struct Esame *esame)
{
    if (read(socket, esame, sizeof(struct Esame)) != sizeof(struct Esame))
    {
        perror("Errore ricezione esame");
        exit(1);
    }
    // Messaggio per verificare che i dati siano correttamente ricevuti nella segreteria
    printf("Ricevuta prenotazione esame: Nome = %s, Data = %s\n", esame->nome, esame->data);
}

// Funzione per inviare la richiesta di prenotazione al server universitario
void MandaPrenotazioneEsame(int socket_prenotazione_esame, struct Esame esame)
{
    struct Richiesta richiesta_prenotazione;
    richiesta_prenotazione.TipoRichiesta = 2;
    richiesta_prenotazione.esame = esame;

    // Debug per confermare l'invio con i dati corretti
    printf("Inviando richiesta prenotazione dell'esame Esame: %s, Data: %s\n", richiesta_prenotazione.esame.nome, richiesta_prenotazione.esame.data);

    if (write(socket_prenotazione_esame, &richiesta_prenotazione, sizeof(richiesta_prenotazione)) != sizeof(richiesta_prenotazione))
    {
        perror("Errore invio richiesta prenotazione al server universitario");
        exit(EXIT_FAILURE);
    }
}
// Funzione per ricevere la matricola dallo studente tramite la segreteria
void RiceviMatricola(int segreteria_connessione_socket, int socket_prenotazione_esame)
{
    char matricola[11]; // Array per la matricola, massimo 10 caratteri + terminatore nullo

    // Ricezione della matricola dallo studente
    ssize_t MatricolaStudente = read(segreteria_connessione_socket, matricola, sizeof(matricola)); // Leggi fino a 10 caratteri
    if (MatricolaStudente <= 0)
    {
        if (MatricolaStudente == 0)
        {
            fprintf(stderr, "Connessione chiusa dallo studente durante la ricezione della matricola.\n");
        }
        else
        {
            perror("Errore ricezione matricola dallo studente");
        }
        exit(EXIT_FAILURE);
    }

    // Assicurati che la stringa sia terminata correttamente
    matricola[MatricolaStudente] = '\0'; // Aggiungi il terminatore nullo alla fine

    // Invia la matricola al server universitario
    if (write(socket_prenotazione_esame, matricola, sizeof(matricola)) != sizeof(matricola))
    {
        perror("Errore invio matricola al server universitario");
        exit(EXIT_FAILURE);
    }
}
// Funzione per inviare l'esito della prenotazione allo studente
void EsitoPrenotazione(int segreteria_connessione_socket, int socket_prenotazione_esame)
{
    int esito_prenotazione;
    // Ricezione dell'esito della prenotazione dal server universitario
    if (read(socket_prenotazione_esame, &esito_prenotazione, sizeof(esito_prenotazione)) <= 0)
    {
        perror("Errore ricezione esito prenotazione dal server universitario");
        exit(EXIT_FAILURE);
    }
    // Invio dell'esito della prenotazione allo studente
    if (write(segreteria_connessione_socket, &esito_prenotazione, sizeof(esito_prenotazione)) != sizeof(esito_prenotazione))
    {
        perror("Errore invio esito prenotazione allo studente");
        exit(EXIT_FAILURE);
    }
}

// Funzione per ascoltare le richieste degli studenti
int ascolto_studenti(struct sockaddr_in *indirizzo_segreteria)
{
    // Inizializzazione del socket per l'ascolto degli studenti
    int segreteria_ascolto_socket = Socket(AF_INET, SOCK_STREAM, 0);

    // Configurazione dell'indirizzo della segreteria
    indirizzo_segreteria->sin_family = AF_INET;
    indirizzo_segreteria->sin_addr.s_addr = htonl(INADDR_ANY);
    indirizzo_segreteria->sin_port = htons(2000);

    // Collegamento del socket all'indirizzo della segreteria
    Bind(segreteria_ascolto_socket, (struct sockaddr *)indirizzo_segreteria, sizeof(*indirizzo_segreteria));
    // Ascolto delle richieste degli studenti (max 10 client)
    Ascolta(segreteria_ascolto_socket, 10);

    return segreteria_ascolto_socket;
}

// Funzione per inviare la lista di esami disponibili allo studente
void esami_disponibili(struct Esame esame, int segreteria_connessione_socket)
{
    int socket_esami;
    struct sockaddr_in indirizzo_universita;
    struct Richiesta ricezione_esami;
    struct Esame esami_disponibili[100]; // Assumiamo che ci siano al massimo 100 esami
    int numero_esami = 0;

    ricezione_esami.TipoRichiesta = 3; // TipoRichiesta 3 per ottenere gli esami disponibili
    ricezione_esami.esame = esame;

    socket_esami = connessione_universita(&indirizzo_universita);
    invio_esame_server(socket_esami, ricezione_esami);

    // Riceviamo il numero di esami dal server universitario
    if (read(socket_esami, &numero_esami, sizeof(numero_esami)) != sizeof(numero_esami))
    {
        perror("Errore ricezione numero esami dal server universitario");
        close(socket_esami);
        exit(EXIT_FAILURE);
    }

    // Inviamo il numero di esami allo studente
    if (write(segreteria_connessione_socket, &numero_esami, sizeof(numero_esami)) != sizeof(numero_esami))
    {
        perror("Errore invio numero esami allo studente");
        close(socket_esami);
        exit(EXIT_FAILURE);
    }

    // Riceviamo gli esami disponibili dal server universitario
    if (read(socket_esami, esami_disponibili, sizeof(struct Esame) * numero_esami) != sizeof(struct Esame) * numero_esami)
    {
        perror("Errore ricezione lista esami dal server universitario");
        close(socket_esami);
        exit(EXIT_FAILURE);
    }

    // Inviamo la lista di esami disponibili allo studente
    if (write(segreteria_connessione_socket, esami_disponibili, sizeof(struct Esame) * numero_esami) != sizeof(struct Esame) * numero_esami)
    {
        perror("Errore invio lista esami allo studente");
        close(socket_esami);
        exit(EXIT_FAILURE);
    }

    printf("--------------------------------------------\nLista esami inviata allo studente\n");
}

// Funzione per connettersi al server universitario
int connessione_universita(struct sockaddr_in *indirizzo_universita)
{
    int socket_esami = Socket(AF_INET, SOCK_STREAM, 0);

    indirizzo_universita->sin_family = AF_INET;
    indirizzo_universita->sin_port = htons(6940);

    if (inet_pton(AF_INET, "127.0.0.1", &indirizzo_universita->sin_addr) <= 0)
    {
        perror("Errore inet 127.0.0.1");
        exit(EXIT_FAILURE);
    }

    Connetti(socket_esami, (struct sockaddr *)indirizzo_universita, sizeof(*indirizzo_universita));

    return socket_esami;
}

// Funzione per inviare la richiesta di esame al server universitario
void invio_esame_server(int socket_esami, struct Richiesta ricezione_esami)
{
    if (write(socket_esami, &ricezione_esami, sizeof(ricezione_esami)) != sizeof(ricezione_esami))
    {
        perror("Errore invio esame");
        exit(EXIT_FAILURE);
    }
}

// Funzione per verificare se la data ha il formato DD-MM-YYYY
int verifica_data(const char *data)
{
    // Controlla che il formato sia esattamente DD-MM-YYYY
    if (strlen(data) != 10 || data[2] != '-' || data[5] != '-')
    {
        return 0;
    }

    // Controlla che giorno, mese e anno siano numeri
    for (int i = 0; i < 10; i++)
    {
        if (i != 2 && i != 5 && !isdigit(data[i]))
        {
            return 0;
        }
    }

    // La data ha il formato corretto
    return 1;
}

// Funzione per inserire un nuovo esame
void inserisci_nuovo_esame()
{
    struct Esame nuovo_esame;

    printf("\n--------------------------------------------\nInserisci il nome dell'esame: ");
    fgets(nuovo_esame.nome, sizeof(nuovo_esame.nome), stdin);
    nuovo_esame.nome[strcspn(nuovo_esame.nome, "\n")] = '\0';

    do
    {
        printf("Inserisci la data dell'esame (DD-MM-YYYY): ");
        fgets(nuovo_esame.data, sizeof(nuovo_esame.data), stdin);
        nuovo_esame.data[strcspn(nuovo_esame.data, "\n")] = '\0';
        
        if (!verifica_data(nuovo_esame.data))
        {
            printf("Formato data non valido! Riprova.\n");
        }
    } while (!verifica_data(nuovo_esame.data));
    mandaEsameNuovoServer(nuovo_esame);
}

// Funzione per inviare un nuovo esame al server universitario
void mandaEsameNuovoServer(struct Esame esame)
{
    int socket_segreteria;
    struct sockaddr_in indirizzo_universita;
    struct Richiesta aggiuntaEsame;

    aggiuntaEsame.TipoRichiesta = 1;
    aggiuntaEsame.esame = esame;

    socket_segreteria = connessione_universita(&indirizzo_universita);
    // Invio dell'esame al server universitario
    if (write(socket_segreteria, &aggiuntaEsame, sizeof(aggiuntaEsame)) != sizeof(aggiuntaEsame))
    {
        perror("Errore manda esame server");
        exit(EXIT_FAILURE);
    }

    printf("Esame aggiunto con successo!\n--------------------------------------------\n");

    close(socket_segreteria);
}