# SINCRIA

Manuale utente - Regole GoL, Dual Gate Sequencer ed EUROBOY

Firmware: 3 novembre 2026. Manuale: 10 novembre 2026.

SINCRIA è un sequencer Eurorack basato su una colonia di cellule in una matrice 8 x 8. Include anche un dual sequencer di gate, Snake e il Game of Life interattivo. Gli stessi controlli assumono funzioni diverse secondo la modalità.

## Per iniziare

1. Accendi senza tenere premute combinazioni di gioco.
2. Scegli INT CLOCK e regola il tempo. Premi NEW GAME per una nuova colonia.
3. Collega GateOut al trigger di un inviluppo o di una voce percussiva. CLOCK OUT fornisce il clock degli step; GameEnd segnala la rigenerazione automatica.
4. Tieni ALT MODE e premi NEW GAME per scegliere una regola, oppure ALT MODE e ROTATE per entrare nel dual sequencer.

La scansione parte **in alto a sinistra**, percorre tutta la riga da sinistra a destra e passa alla riga sotto. Il pixel invertito indica la **prossima** cella da leggere. Un cursore scuro può quindi trovarsi sopra una cellula viva.

Per le combo tieni prima ALT MODE, poi premi il secondo tasto. Rilascia i controlli dopo un cambio schermata o modalità. La combo conserva lo stato ALT musicale precedente, anche premendo ALT in anticipo.

| Argomento | Pagina |
| --- | --- |
| Sequencer GoL e comandi ALT | 2 |
| Cinque regole e partenza dal vuoto | 3 |
| Dual Gate Sequencer | 4 |
| Giochi EUROBOY | 5 |
| Esempi di patch | 6 |
| Installazione e problemi comuni | 7 |

[Progetto, sorgenti e schemi](https://github.com/E103-designs/SINCRIA).

<!-- PAGE -->
## Sequencer GoL

Ogni clock accettato legge una delle 64 celle. Una cellula viva produce un impulso su GateOut. Al termine della scansione la colonia può evolvere con la regola scelta. Il clock continua anche negli step vuoti.

| Controllo | Azione normale |
| --- | --- |
| NEW GAME | Nuova colonia casuale non vuota; riporta la scansione all'inizio |
| LOCK | Blocca/sblocca evoluzione e rigenerazione automatica |
| ROTATE | Ruota la colonia |
| SPAWN | Crea una cellula casuale |
| KILL | Elimina una cellula casuale |
| ALT MODE da solo | Tieni circa 250 ms, debounce incluso, per cambiare il set ALT |

Nel GoL il LED ALT indica il set di comandi alternativo attivo. Rilascia ALT dopo averlo attivato: le vecchie funzioni alternative restano selezionate e non richiedono di tenerlo premuto.

| Controllo con ALT attivo | Azione alternativa |
| --- | --- |
| NEW GAME | Reset della scansione; anteprima GateOut se la prima cella è viva, senza clock |
| LOCK | Scorrimento circolare dei bit della griglia |
| ROTATE | Scorrimento circolare delle righe della griglia |
| SPAWN con EXT selezionato | Crea sul cursore; tieni per dipingere a ogni clock |
| KILL con EXT selezionato | Cancella sul cursore; tieni per cancellare a ogni clock |

Durante la pittura KILL prevale su SPAWN. Negli altri casi SPAWN/KILL agiscono una volta per pressione. Puoi modificare la colonia anche con LOCK attivo.

## Clock e uscite

INT fornisce **30-300 BPM**, quattro step per battito. EXT usa il clock su GATE IN/ClkIn. In EXT anche il pulsante GATE IN può avanzare di uno step. CLOCK OUT pulsa a ogni step; GateOut sulle cellule vive; GameEnd quando una nuova colonia casuale sostituisce la precedente.

GameEnd **non** pulsa automaticamente ogni 64 step. Estinzione e ripetizioni esatte di periodo 1-4 possono causare rigenerazione; LOCK la impedisce. Una ripetizione viene scansionata prima della sostituzione. Il GoL musicale conserva il wrapping originale su un solo asse; EUROBOY Life avvolge entrambi gli assi.

Gli impulsi hanno durata nominale di 10 ms. Un retrigger ravvicinato può prolungare un impulso già attivo. Il clock esterno richiede circa 5 ms stabili nei livelli attivo e inattivo. Passare a un ingresso esterno già alto non genera uno step fantasma.

<!-- PAGE -->
## Scegliere la regola GoL

Durante il GoL musicale premi **ALT MODE + NEW GAME**. Scansione e uscite musicali si fermano temporaneamente. Cinque LED nella prima riga identificano le regole, da sinistra a destra: quello selezionato lampeggia, gli altri quattro restano accesi.

| LED | Regola | Codice |
| --- | --- | --- |
| 1 | Conway classica | B3/S23 |
| 2 | SINCRIA | B234/S01 |
| 3 | Sopravvivenza estesa | B3/S234 |
| 4 | Mazectric | B3/S1234 |
| 5 | Coagulations | B378/S235678 |

**B** elenca i numeri di vicini che fanno nascere una cellula morta. **S** quelli che mantengono viva una cellula. SINCRIA crea con 2, 3 o 4 vicini e fa sopravvivere con 0 o 1. Il suo codice è B234/S01, distinto da B3/S234.

| Nel selettore | Azione |
| --- | --- |
| LOCK o SPAWN | Regola precedente |
| KILL o ROTATE | Regola successiva |
| NEW GAME | Applica la regola, genera una colonia casuale e riparte dallo step 1 |
| ALT MODE + KILL, circa un secondo | Applica la regola, svuota la colonia e torna al GoL |

ROTATE è la freccia destra nelle nuove schermate. GATE IN rimane un ingresso clock e non può spostare la scelta. Rilascia i controlli dopo la conferma. Il clock interno riparte con un intervallo intero; quello esterno attende un nuovo fronte valido.

## Partire dal silenzio

Il comando lungo di svuotamento lascia **tutti i 64 LED spenti, cursore compreso**. La rigenerazione resta sospesa finché la colonia è vuota. CLOCK OUT può continuare, mentre GateOut e GameEnd restano inattivi per quella colonia vuota.

Crea una cellula con SPAWN o con la pittura alternativa per ripristinare il funzionamento normale. NEW GAME normale genera un'altra colonia casuale. Con il vecchio set ALT attivo, NEW GAME continua invece a resettare la scansione.

Più condizioni di sopravvivenza non garantiscono evoluzioni più lunghe: una colonia stabile o con periodo breve può ancora rigenerarsi. Usa LOCK per mantenere un pattern musicale. La scelta della regola è in RAM e torna Classica all'accensione; non modifica EUROBOY Life.

<!-- PAGE -->
## Dual Gate Sequencer

Premi **ALT MODE + ROTATE** per passare tra GoL e Dual. Il Dual entra **in pausa**. Le quattro righe superiori formano la sequenza 1, su **GateOut**; le quattro inferiori formano la sequenza 2, su **GameEnd**. Ogni metà contiene 32 step ON/OFF, letti per righe dal proprio angolo in alto a sinistra.

| Controllo nel Dual | Azione |
| --- | --- |
| LOCK / KILL | Cursore su / giù |
| SPAWN / ROTATE | Cursore sinistra / destra |
| NEW GAME | Accende/spegne lo step sotto il cursore |
| ALT MODE + LOCK | Play/pausa di entrambe le sequenze |
| ALT MODE + SPAWN | Imposta l'ultimo step della sequenza sul cursore |
| ALT MODE + NEW GAME | Riporta entrambe le testine allo step 1 senza cambiare i pattern |
| ALT MODE + ROTATE | Torna al GoL |
| GATE IN | Solo clock esterno |

Attraversa la metà della matrice per editare l'altra sequenza. Il cursore avvolge entrambi gli assi. Ogni pressione produce uno spostamento o un toggle, senza ripetizioni automatiche. Puoi editare anche durante il playback.

Il cursore di editing ha un breve lampeggio invertito. Durante il playback due testine lampeggianti indicano i prossimi step da leggere. Sopra uno step acceso il lampeggio oscura il pixel. La visualizzazione non modifica i dati memorizzati.

## Lunghezze indipendenti, clock comune

Le due sequenze usano lo stesso INT/EXT e potenziometro. Ciascuna lunghezza è **1-32**. Vai sull'ultimo step desiderato e premi ALT+SPAWN. Per circa 700 ms il display mostra le posizioni comprese nelle due lunghezze. Gli step esclusi sono nascosti e silenziosi, ma restano memorizzati e tornano allungando la sequenza.

Se una testina è oltre la nuova lunghezza, quel canale torna allo step 1; l'altro conserva la propria posizione. ALT+NEW GAME riallinea entrambi. Una sequenza di 5 step e una di 7 si riallineano dopo **35 clock**.

Il LED ALT è acceso mentre il Dual suona. CLOCK OUT fornisce il clock comune. GameEnd è esclusivamente il gate della sequenza 2, senza eventi aggiunti a fine ciclo. Pausa e cambio modalità portano le uscite basse. Colonia, regola, posizione e stato ALT GoL restano separati. I dati Dual restano in RAM nei cambi modalità, ma si perdono spegnendo.

<!-- PAGE -->
## Giochi EUROBOY

Tieni una combinazione di gioco all'accensione o al reset. Rilascia i controlli dopo la breve animazione iniziale. Per uscire da un gioco riavvia senza combinazioni.

| Combinazione al boot | Modalità |
| --- | --- |
| ALT MODE + SPAWN | Snake |
| ALT MODE + KILL | Life interattivo |
| ALT MODE + SPAWN + KILL | GoL musicale |
| Nessuna combinazione valida | GoL musicale |

In **entrambi i giochi**: LOCK su, KILL giù, SPAWN sinistra e **GATE IN destra**. È diverso dalle nuove schermate musicali, dove ROTATE è destra e GATE IN rimane clock.

## Snake

Il serpente parte con tre segmenti e si muove verso destra. Mangia il pixel del cibo per crescere; nuovo cibo compare solo in una cella libera. Tutti i bordi comunicano. Non puoi invertire immediatamente il verso; viene accettata una sola svolta per movimento.

Il potenziometro regola **700-100 ms per movimento**. INT/EXT non influisce su Snake. La collisione col corpo termina la partita; puoi entrare nella cella di coda che si libera. Riempire i 64 pixel dà la vittoria.

Dopo collisione o vittoria la matrice lampeggia per 600 ms, poi resta ferma. Premi nuovamente SPAWN per ripartire. Una pressione durante il lampeggio non viene eseguita in ritardo.

## Life interattivo

Life parte vuoto, con cursore vicino al centro. In EXT puoi disegnare in pausa: NEW GAME crea una cellula, ROTATE la cancella. INT avvia l'evoluzione. Il potenziometro regola **2.000-100 ms per generazione**. Puoi editare anche in marcia.

Il cursore inverte il pixel per 120 ms e lo ripristina per 600 ms. Una cellula viva si oscura brevemente; una vuota si illumina brevemente. Il cursore non modifica la colonia. Il campo Conway B3/S23 e il cursore avvolgono entrambi gli assi. Colonie vuote, stabili e oscillatori restano senza rigenerazione casuale.

Il selettore richiede 50 ms per confermare il cambio; la ripresa attende un intervallo completo. Qui GATE IN sposta a destra e non fa evolvere la simulazione. Tutte le uscite musicali restano basse nei giochi. Usa l'alimentazione normale del modulo: l'alimentazione completa da USB dipende dall'hardware montato.

<!-- PAGE -->
## Esempi di patch

### Una voce percussiva in evoluzione

Collega GateOut al trigger di un inviluppo o di una voce percussiva e CLOCK OUT al clock di un altro modulo. Avvia GoL in INT e scegli una regola con ALT+NEW GAME. SPAWN/KILL modificano la densità; LOCK mantiene un groove mentre il cursore continua la scansione. GameEnd può resettare un altro sequencer a ogni rigenerazione, creando frasi più lunghe e variabili.

Una cella vuota è una pausa. La regola modifica il pattern dopo ogni scansione, non il tempo o l'altezza dei singoli step. GameEnd segue la rigenerazione, non ogni giro della matrice.

### Due percussioni in 5 e 7

1. Entra nel Dual con ALT+ROTATE. Collega GateOut a una cassa e GameEnd a una seconda percussione.
2. Nella metà superiore accendi gli step 1, 3 e 5 con NEW GAME. Porta il cursore sullo step 5 e premi ALT+SPAWN.
3. Nella metà inferiore accendi gli step 1, 4 e 6. Porta il cursore sullo step 7 e premi ALT+SPAWN.
4. Usa INT oppure collega un clock a GATE IN e scegli EXT. ALT+NEW GAME riallinea; ALT+LOCK avvia.

Le due sequenze si incontrano ogni 35 clock. Puoi cambiare uno step in playback con NEW GAME. Per accorciare un solo canale, vai sul nuovo ultimo step e premi ALT+SPAWN. Tornando al GoL ritrovi la colonia precedente.

### Disegnare una sequenza dal silenzio

Apri il selettore e tieni ALT+KILL per circa un secondo. Rilascia i controlli: la colonia resta vuota. Scegli EXT. Attiva il vecchio set ALT tenendo ALT da solo e rilasciandolo. Tieni SPAWN mentre invii clock per dipingere cellule sul cursore; usa KILL per cancellare. La prima creazione effettiva sblocca il vuoto.

Per mantenere il risultato, esci dalla pittura e attiva LOCK con il set normale. CLOCK OUT continua, GateOut legge le cellule vive e la rigenerazione resta ferma. Con il set ALT attivo LOCK sposta la griglia: controlla il LED ALT prima di bloccare.

### Disegnare senza uscite musicali

Avvia EUROBOY Life con ALT+KILL al boot. Scegli EXT, crea tre cellule adiacenti e passa a INT. Il blinker alterna orientamento indefinitamente. Questo gioco è completamente toroidale e indipendente da regola musicale e rigenerazione.

<!-- PAGE -->
## Installazione e riferimento

Installa **LedControl 1.0.6**. Apri il file corrente `firmware/SINCRIA_Scansione_Righe/SINCRIA_Scansione_Righe.ino` in Arduino IDE. Seleziona **Arduino Nano / ATmega328P** e il bootloader del Nano; è supportato anche Old Bootloader. Il vecchio sketch Rev2 è separato.

Il profilo ingressi predefinito è 1 per gli stadi a transistor invertenti. Pinout conservato; il firmware non rileva revisione o alimentazione. Schemi e istruzioni di compilazione sono nel repository.

| Uscita | GoL musicale | Dual | EUROBOY |
| --- | --- | --- | --- |
| GateOut | Cellula viva | Sequenza 1 | Bassa |
| GameEnd | Rigenerazione automatica | Sequenza 2 | Bassa |
| CLOCK OUT | Ogni step accettato | Clock comune | Bassa |

## Problemi comuni

**La colonia dura poche generazioni.** Estinzione e ripetizioni esatte di periodo 1-4 possono rigenerare. Prova altre regole o LOCK per mantenere un groove. Più sopravvivenza può anche portare prima a una colonia stabile.

**Il vuoto non si ripopola.** È il comportamento voluto dopo lo svuotamento lungo. Crea una cellula o usa NEW GAME normale. Con ALT attivo NEW GAME resetta la scansione invece di randomizzare.

**ROTATE non ruota.** È destra nel Dual e nel selettore, cancella in EUROBOY Life. Torna al GoL musicale per ruotare.

**GATE IN non va a destra.** Nelle nuove schermate musicali è riservato al clock; resta destra nei due giochi EUROBOY.

**Il Dual entra in silenzio.** Parte in pausa e inizialmente vuoto. Accendi gli step, imposta le lunghezze e avvia con ALT+LOCK. Controlla anche INT/EXT e il clock sorgente.

**Un jack agisce come un tasto.** Jack dei comandi e pulsanti condividono l'ingresso elettrico. Il software non distingue le sorgenti. GATE IN è escluso dalla navigazione delle nuove schermate musicali.

**Le impostazioni si perdono spegnendo.** Regola e pattern Dual sono volatili; non c'è salvataggio EEPROM automatico.

Risorse firmware: **12.182 byte Flash, 488 byte SRAM statica**. Il proprietario del progetto ha confermato il funzionamento sul modulo il 10 novembre 2026. Vedi [verifiche](PUBLICATION_CHECKS.md), [sviluppo](DEVELOPMENT.md), [schemi](../hardware/schematics/README.md) e [manuale inglese](USER_GUIDE.md).
