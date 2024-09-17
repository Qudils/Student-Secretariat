#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "wrapper.h"

// Server universitario:

// Riceve l'aggiunta di nuovi esami
// Riceve la prenotazione di un esame

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

struct Prenotazione
{
    struct Esame esame;
    int NumPrenotazione;
    char Matricola[11];
};

// Funzioni dichiarate
void aggiungi_esame_file(struct Esame esame);
int gestisci_prenotazione(int universita_connessione_socket, struct Richiesta richiesta_ricevuta);
void gestisci_esami_disponibili(int socket, struct Richiesta richiesta_ricevuta);
int incrementaContatoreDaPrenotazioni(struct Esame esame);

int main()
{
    int universita_connessione_socket;       // Socket per la connessione con la segreteria
    int universita_ascolto_socket;           // Socket per l'ascolto delle connessioni in ingresso
    struct sockaddr_in indirizzo_universita; // Indirizzo del server universitario
    struct Richiesta richiesta_ricevuta;     // Richiesta ricevuta dalla segreteria

    // Creazione del socket di ascolto
    universita_ascolto_socket = Socket(AF_INET, SOCK_STREAM, 0);

    indirizzo_universita.sin_family = AF_INET;
    indirizzo_universita.sin_addr.s_addr = htonl(INADDR_ANY);
    indirizzo_universita.sin_port = htons(6940);

    // Associazione del socket all'indirizzo e porta
    Bind(universita_ascolto_socket, (struct sockaddr *)&indirizzo_universita, sizeof(indirizzo_universita));

    // Inizio dell'ascolto
    Ascolta(universita_ascolto_socket, 10);
    printf("Server in ascolto sulla porta 6940...\n");

    // Ciclo per gestire le richieste in ingresso
    while (1)
    {
        // Accettazione di una connessione in ingresso
        universita_connessione_socket = Accetta(universita_ascolto_socket, (struct sockaddr *)NULL, NULL);

        // Creazione di un processo figlio per gestire la richiesta
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("Errore nella fork controlla bene");
            exit(EXIT_FAILURE);
        }

        if (pid == 0)
        { // Processo figlio
            close(universita_ascolto_socket);

            // Lettura della richiesta dal client
            ssize_t bytes_read = read(universita_connessione_socket, &richiesta_ricevuta, sizeof(struct Richiesta));
            if (bytes_read != sizeof(struct Richiesta))
            {
                perror("Errore nella lettura della richiesta");
                close(universita_connessione_socket);
                exit(EXIT_FAILURE);
            }

            // Gestione della richiesta in base al tipo della richiesta
            if (richiesta_ricevuta.TipoRichiesta == 1)
            {

                aggiungi_esame_file(richiesta_ricevuta.esame); // Aggiungi l'esame al file
            }
            else if (richiesta_ricevuta.TipoRichiesta == 2)
            {

                gestisci_prenotazione(universita_connessione_socket, richiesta_ricevuta); // Gestisci la prenotazione
            }
            else if (richiesta_ricevuta.TipoRichiesta == 3)
            {

                gestisci_esami_disponibili(universita_connessione_socket, richiesta_ricevuta); // Gestisci gli esami disponibili
            }
            else
            {
                fprintf(stderr, "Tipo di richiesta non valido\n");
            }

            // Chiusura del socket di connessione
            close(universita_connessione_socket);
            exit(EXIT_SUCCESS);
        }
        else
        { // Processo padre
            close(universita_connessione_socket);
        }
    }

    // Chiusura del socket di ascolto
    close(universita_ascolto_socket);
    return 0;
}

// Funzione per aggiungere un esame al file
void aggiungi_esame_file(struct Esame esame)
{
    FILE *Lista_esami = fopen("esami.txt", "a");
    if (Lista_esami == NULL)
    {
        perror("Errore apertura file esami.txt");
        exit(EXIT_FAILURE);
    }

    // Scrittura dell'esame nel file usando fprintf
    if (fprintf(Lista_esami, "%s,%s\n", esame.nome, esame.data) < 0)
    {
        perror("Errore scrittura file esami.txt");
        fclose(Lista_esami);
        exit(EXIT_FAILURE);
    }

    fclose(Lista_esami);
    printf("\n--------------------------------------------\nEsame aggiunto con successo\n--------------------------------------------");
}

// Funzione per gestire la prenotazione di un esame
int gestisci_prenotazione(int universita_connessione_socket, struct Richiesta richiesta_ricevuta)
{
    struct Prenotazione prenotazione;
    int esito_prenotazione = 1;

    prenotazione.esame = richiesta_ricevuta.esame;

    // Ottieni un numero di prenotazione unico per l'esame specifico dal file `prenotazioni.txt`
    prenotazione.NumPrenotazione = incrementaContatoreDaPrenotazioni(richiesta_ricevuta.esame);

    printf("\n--------------------------------------------\nNumero prenotazione assegnato: %d\n", prenotazione.NumPrenotazione);

    // Ricezione della matricola dallo studente tramite la segreteria
    ssize_t MatricolaStudente = read(universita_connessione_socket, prenotazione.Matricola, sizeof(prenotazione.Matricola) - 1);
    if (MatricolaStudente <= 0)
    {
        if (MatricolaStudente == 0)
        {
            fprintf(stderr, "\nConnessione chiusa dalla segreteria durante la ricezione della matricola.\n");
        }
        else
        {
            perror("Errore ricezione matricola");
        }
        return -1;
    }

    // Aggiungi il terminatore di stringa
    prenotazione.Matricola[MatricolaStudente] = '\0';

    // Invia il numero di prenotazione alla segreteria
    ssize_t numero_prenotazione = write(universita_connessione_socket, &prenotazione.NumPrenotazione, sizeof(prenotazione.NumPrenotazione));
    if (numero_prenotazione != sizeof(prenotazione.NumPrenotazione))
    {
        perror("Errore invio numero prenotazione");
        return -1;
    }

    printf("Prenotazione ricevuta per esame: %s, data: %s, matricola: %s, numero:%d", prenotazione.esame.nome, prenotazione.esame.data, prenotazione.Matricola, prenotazione.NumPrenotazione);

    // Scrivi la prenotazione nel file "prenotazioni.txt"
    FILE *file_prenotazioni = fopen("prenotazioni.txt", "a");
    if (file_prenotazioni == NULL)
    {
        perror("Errore apertura file prenotazioni.txt");
        esito_prenotazione = 0;
    }
    else
    {
        // Assicurati che i dati siano scritti correttamente
        fprintf(file_prenotazioni, "%d,%s,%s,%s\n",
                prenotazione.NumPrenotazione,
                prenotazione.esame.nome,
                prenotazione.esame.data,
                prenotazione.Matricola);
        fclose(file_prenotazioni);
    }
    printf("\nPrenotazione conclusa, invio alla segreteria\n--------------------------------------------\n");

    // Invia l'esito della prenotazione
    ssize_t bytes_written = write(universita_connessione_socket, &esito_prenotazione, sizeof(esito_prenotazione));
    if (bytes_written != sizeof(esito_prenotazione))
    {
        perror("Errore invio esito prenotazione");
        return -1;
    }

    return 0;
}

// Funzione per trovare e incrementare il contatore di prenotazione per un esame specifico
int incrementaContatoreDaPrenotazioni(struct Esame esame)
{
    FILE *file = fopen("prenotazioni.txt", "r");
    if (file == NULL)
    {
        // Se il file non esiste, inizializza il contatore a 1
        return 1;
    }

    char buffer[256];
    int maxContatore = 0;

    // Leggi ogni riga del file e cerca le prenotazioni per l'esame specifico
    while (fgets(buffer, sizeof(buffer), file) != NULL)
    {
        struct Esame esameFile;
        int contatore;
        char matricola[11];

        // Supponendo che il file `prenotazioni.txt` abbia il formato: numero, nome, data, matricola
        sscanf(buffer, "%d,%99[^,],%99[^,],%10s", &contatore, esameFile.nome, esameFile.data, matricola);

        // Controlla se l'esame corrente nel file corrisponde a quello passato alla funzione
        if (strcmp(esameFile.nome, esame.nome) == 0 && strcmp(esameFile.data, esame.data) == 0)
        {
            if (contatore > maxContatore)
            {
                maxContatore = contatore;
            }
        }
    }

    fclose(file);
    return maxContatore + 1; // Incrementa di uno per il nuovo numero di prenotazione
}

// Funzione per gestire la richiesta di esami disponibili
void gestisci_esami_disponibili(int socket, struct Richiesta richiesta_ricevuta)
{
    FILE *Lista_esami = fopen("esami.txt", "r"); // Apriamo il file degli esami
    struct Esame esami_disponibili[100]; // Supponiamo un massimo di 100 esami
    int numero_esami = 0;
    char nome[100], data[100];

    if (Lista_esami == NULL)
    {
        perror("Errore apertura file esami.txt");
        exit(EXIT_FAILURE);
    }

    // Leggiamo gli esami dal file e li filtriamo per nome
    while (fscanf(Lista_esami, "%99[^,],%99[^\n]\n", nome, data) == 2)
    {
        if (strcmp(nome, richiesta_ricevuta.esame.nome) == 0)
        {
            //Copia i dati dell'esame
            strcpy(esami_disponibili[numero_esami].nome, nome);
            strcpy(esami_disponibili[numero_esami].data, data);
            numero_esami++;
        }
    }
    fclose(Lista_esami); // Chiudiamo il file

    // Inviamo il numero di esami disponibili alla segreteria
    if (write(socket, &numero_esami, sizeof(numero_esami)) != sizeof(numero_esami))
    {
        perror("Errore invio numero esami alla segreteria");
        exit(EXIT_FAILURE);
    }

    // Inviamo gli esami disponibili alla segreteria
    if (write(socket, esami_disponibili, sizeof(struct Esame) * numero_esami) != sizeof(struct Esame) * numero_esami)
    {
        perror("Errore invio esami disponibili alla segreteria");
        exit(EXIT_FAILURE);
    }
}
