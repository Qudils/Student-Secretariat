
# Progetto Reti dei Calcolatori 2023/2024 Gruppo 3 studenti

## Traccia - Università
Scrivere un'applicazione client/server parallelo per gestire gli esami universitari.

### Gruppo 1 studente

**Segreteria:**

- Inserisce gli esami sul server dell'università (salvare in un file o conservare in memoria il dato)

- Inoltra la richiesta di prenotazione degli studenti al server universitario

- Fornisce allo studente le date degli esami disponibili per l'esame scelto dallo studente


**Studente:**

- Chiede alla segreteria se ci siano esami disponibili per un corso

- Invia una richiesta di prenotazione di un esame alla segreteria


**Server universitario:**

- Riceve l'aggiunta di nuovi esami

- Riceve la prenotazione di un esame

### Gruppo 2 studenti

Il server universitario ad ogni richiesta di prenotazione invia alla segreteria il numero di prenotazione progressivo assegnato allo studente e la segreteria a sua volta lo inoltra allo studente 

### Gruppo 3 studenti

Se la segreteria non risponde alla richiesta dello studente questo deve ritentare la connessione per 3 volte. Se le richieste continuano a  fallire allora aspetta un tempo random e ritenta. Simulare un timeout della segreteria in modo da arrivare a testare l'attesa random
