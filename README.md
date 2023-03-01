<!DOCTYPE html>
<html lang="it">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
</head>
<body>
    <h1>Relazione Progetto Reti Informatiche (A.A. 2021/22)</h1>
    <p>
    Il sistema di chatting è stato implementato in due file: Server.c e Device.c. 
    In Server.c è stato implementato il server mentre in Device.c è stato implementato il device.
    </p>
    <p>
    Il sistema viene avviato eseguendo il comando bash init.sh che avvia quattro terminali di cui uno per il server e tre per i device.
    </p>
    <p>
    La prima cosa richiesta all’avvio del Device è la porta del Server a cui si deve connettere, dopo di che si apre una finestra che richiede se il Device vuole fare un login oppure una sign up. 
    In tutti i due casi vengono richiesti username e password per effettuare correttamente la registrazione o il login del Device nel sistema. 
    Il Server memorizza le informazioni riguardo ai Device in file specifici, i quali sono creati e aggiornati durante l’esecuzione; tutti i file del Server sono contenuti nella cartella srv, mentre ogni file dei Device è contenuto nella sua cartella personale nominata come l’username. 
    Analizziamo i file del Server:
    </p>
    <ul>
        <li>
            srv/usr_all.txt → in questo file vengono memorizzati gli username registrati nel sistema: è utilizzato, per esempio, in fase di sign up per controllare e quindi evitare che due Device siano registrati al sistema con lo stesso username. 
            E’ compito del Server scorrere il file ed effettuare tali controlli. Il Server utilizza il file anche in fase di login per controllare se l’username inserito è stato registrato nel sistema.
        </li>
        <li>
            srv/usr_psw.txt → in questo file vengono memorizzati username e password dei Device, viene aggiornato in fase di sign up e utilizzato in fase di login per verificare se la password inserita è corretta o meno.
        </li>
        <li>
            srv/usr_log.txt → in questo file vengono memorizzate i seguenti dati: username, porta, timestamp di login, timestamp di logout dei Device registrati nel sistema. 
            Se un Device è online il timestamp di logout è 0, il Server provvederà a modificare il file ogni volta che un Device si logga (modifica timestamp di login) o effettua il logout (modifica timestamp di logout). 
            Questo file è un chiaro riferimento al registro indicato nelle specifiche.
        </li>
        <li>
            srv/usr_online.txt → in questo file vengono memorizzati i seguenti dati relativi agli utenti online: username, timestamp di login e porta. 
            Il Server legge questo file per il comando list, e stampa a video le informazioni in esso contenuto, delimitate da asterischi. È compito del Server aggiornare il file nel caso di login e logout dei Device. 
            Quando il Server termina la sua esecuzione provvede a cancellare tutto il contenuto del file.
        </li>
    </ul>
    <p>
        Dopo aver effettuato il login o la sign up, il Device mostra a video un menù con tutti i comandi che è possibile eseguire.
        I comandi hanging e show sono in linea con quello richiesto dalle specifiche.
    </p>
    <p>
        Ora analizziamo il comportamento del comando chat:
        Nel caso in cui si vuole avviare una chat con un utente offline, il Device contatterà direttamente il Server, il quale chiederà un messaggio (pendente), lo memorizzerà e lo potrà inviare al Device destinatario (l’utente che era offline) solo quando lo richiederà. 
        Il Server per ogni utente registrato al sistema crea dentro srv/ una cartella chiamata come l’username. Non appena un utente x riceverà dei messaggi pendenti, il Server provvederà a creare una cartella pendent dentro la cartella che ha riservato ad x. 
        Dentro pendent il Server creerà un file .txt rinominato come l’username dell’utente y che ha inviato quei messaggi pendenti. 
        Facciamo l’esempio che x sia diego ed y sia lore, quando diego vorrà leggere i messaggi che gli ha inviato lore quando era offline, dovrà comunicarlo al Server tramite, per esempio, il comando ‘show lore’, 
        a questo punto il Server leggerà il file srv/diego/pendent/lore.txt per estrarre i messaggi desiderati.
        Nel caso in cui si vuole avviare una chat con un utente online, il Device chiederà le informazioni (per esempio, la porta) di quell’utente al Server e creerà un canale di comunicazione con esso: prima di tutto lo inviterà ad entrare nella chat,
        l’utente destinatario potrà decidere se accettare o meno l’invito, in caso di rifiuto avviene la stessa politica di chat offline indicata precedentemente, altrimenti verrà aperta la chat vera e propria, con il suddetto menù.
    </p>
    <p>
        Le operazioni permesse sono quindi: scrivere un messaggio, aggiungere un partecipante, condividere un file, eliminare la cronologia e uscire dalla chat. 
        Sotto questo menù abbiamo la cronologia della chat con quell’utente. Ogni Device dentro la sua cartella ha una cartella nominata chat/, che contiene un file con estensione .txt nominato come l’utente con cui ha iniziato una chat. 
        Il file contiene tutta la cronologia della chat stampata a video. La chat è implementata dalla funzione chat() dove è presente la select(): 
        l’insieme dei descrittori analizzati sono tutti quelli con cui il Device ha instaurato una connessione (tutti i partecipanti della chat), il listener e il descrittore del socket comunicante con il Server (se il Server va offline mentre un Device sta chattando con un altro, quando usciranno dalla chat dovranno terminare la loro esecuzione).
        Il formato del messaggio inviato o ricevuto è: [timestamp] username: messaggio (**) 
        timestamp ha il formato [dd-MM-yy|hh-mm-ss]. 
    </p>
    Accanto ad ogni messaggio inviato avremo un asterisco se l’utente destinatario è offline oppure ha lasciato la chat; due asterischi se l’utente destinatario è online e sta ricevendo in tempo reale i messaggi.
    Per i messaggi ricevuti non sono presenti asterischi. Un utente x può aggiungere un utente y tramite il comando \u: a questo punto l’utente x invierà al Server una lista contenente i partecipanti dell’attuale chat, il Server comunicherà a x gli utenti online che possono essere aggiunti alla chat. 
    L’utente x aggiungerà y alla chat e gli comunicherà le informazioni (username e porta) dei partecipanti: y, una volta estratte tutte le informazioni, dovrà collegarsi a tutti loro. Ciò significa che si verranno a creare dei socket di comunicazione tra ogni partecipante. 
    Tali socket verranno poi inseriti nel suddetto insieme dei descrittori che verrà controllato dalla select() della funzione chat(). Ogni partecipante della chat memorizza tutte le informazioni riguardanti gli altri membri, in particolare l’username, la porta e il descrittore di socket con cui sono collegati.
    I primi due servono per poter aggiungere un nuovo membro alla chat, il quale riceverà queste informazioni e potrà collegarsi a tutti i partecipanti; il terzo parametro serve per poter inviare un messaggio a tutti i membri della chat, scorrendo i descrittori memorizzati. 
    Tutti questi dettagli sono memorizzati in un vettore della struttura dati dev_users la quale contiene le informazioni elencate.
    Un utente x può uscire dalla chat tramite il comando \q, gli altri membri riceveranno il messaggio: x è uscito dalla chat ! , il messaggio verrà memorizzato nella cronologia della chat insieme al timestamp corrente. Se un utente x rimane da solo nella chat, comunicherà con il Server iniziando una “chat offline” con l’utente y che lo aveva invitato inizialmente
    o con cui aveva iniziato a comunicare tramite il comando chat. Come detto precedentemente, se il Server durante la chat va offline, l’uscita del Device dalla chat implica la terminazione dell’esecuzione del programma.
    <p>
        Per quanto riguarda lo share dei file, l’utente che effettua la condivisione apre il file che vuole condividere e memorizza in una variabile il contenuto. Prima di tutto invia ai partecipanti il nome del file condiviso e successivamente il contenuto. 
        Una volta avvenuto lo share del file, i membri della chat avranno nella cartella personale il file interessato, mentre nella chat verrà stampato il filename e il suo contenuto. Nella cronologia della chat verrà memorizzato un messaggio del tipo: x ha condiviso un file ! con il timestamp corrente.
        Per testare lo sharing dei file, il programma crea un file di esempio situato nella cartella personale dell’utente, esso è nominato ex_username.txt, dove username è il nominativo dell’utente; quindi, l’utente diego avrà nella cartella diego/ il file ex_diego.txt. 
        Infine, è possibile utilizzare il comando \d per cancellare tutta la cronologia della chat corrente.
    </p>
    <p>
        Per quanto riguarda la disconnessione improvvisa dei Device o del Server, il tutto è stato gestito tramite l’utilizzo della libreria signal.h che permette di implementare degli handler che si attivano nel momento in cui il programma riceve un determinato segnale. Sono stati implementati degli handler per la gestione del SIGINT (CTRL+C da terminale) e SIGTSTP (CTRL+Z da terminale). 
        Per quanto riguarda i Device, gli handler si comportano in modo diverso a seconda della loro esecuzione: per esempio, se un Device riceve SIGINT mentre sta chattando con un altro utente, prima di tutto comunica all’utente che uscirà dalla chat, successivamente potrà terminare la sua esecuzione. Per quanto riguarda il Server,
        l’handler si occupa di comunicare a tutti i Device che il Server sta terminando la sua esecuzione, come avviene nella gestione del comando esc.
    </p>
    <p>
        Le connessioni instaurate nel sistema seguono tutte il protocollo TCP e la conseguente affidabilità. Il Server gestisce le richieste in modo concorrente; per ogni Device online nel sistema viene creato un thread che gestisce le sue richieste. Le primitive utilizzate sono la pthread_create() e la pthread_exit(), quest’ultime sono state preferite alla fork() e alla exit() poiché comportano un overhead più basso e una gestione più efficiente della concorrenza. Per l’implementazione di questa politica è stata inclusa la libreria pthread.h ed è stata aggiunta l’opzione -lpthread al comando di compilazione del Server.
    </p>

</body>
</html>
