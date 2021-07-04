///////////////////////////////////////////////////////////////////
//   Cisternino.ino                                              //
//   Created by Emanuele Signoretta, 05 feb 2021 .               //
//                                                               //
///////////////////////////////////////////////////////////////////


//Aggiunta librerie

#include<Every.h>
#include<FishinoEEPROM.h>
#include <SPI.h>
#include <Fishino.h>
#include <JSONStreamingParser.h>
#include <FishGram.h>
#include <EVERY.h>
#include <ctype.h>
#include <string.h>

#define trigPin 2 //Pin che genera l'impulso
#define echoPin 3 //Pin per la lettura del ritorno
#define slot_soglia 2 //slot EEPROM di partenza per il salvataggio della soglia di allerta
#define slot_intervallo 9 // slot EEPROM di partenza per il salvataggio dell'intervallo di lettura
#define EEPROM_Data_Start 20 //slot EEPROM di partenza per il salvataggio dei dati degli utenti
#define num_utenti 3 //numero utenti presenti nella configurazione. MODIFICARE IN BASE ALLE NECESSITA'
uint32_t intervallo; //intervallo di lettura
uint32_t soglia; // soglia di allerta
int misura;
int scelta_utente = 0;
int new_tid = 0;
int new_len = 0;
int new_used = 0;
char new_name [11];
long duration;
int menu = 0; //variabile per la gestione della logica a menù



typedef struct { //crea un tipo aggregato
  uint32_t telegram_id = 00000000;
  int used = 0;
  int len = 0;
  char name[11] ;
} User;

User utenti [num_utenti];

#define RECT // OR CIRC


#if defined(RECT)

float larghezza = 1;
float lunghezza = 2;
float altezza = 4;

float superficie = larghezza * lunghezza; //calcola la superficie dello specchio d'acqua

float volume = superficie * altezza * 1000; //calcola il volume della cisterna (in metri cubi)

float valore_massimo_misurazione;

#elif defined(CIRC)

float pi = 3.1415;
float raggio = 0;
float altezza = 1;
float superficie = ((raggio) ^ 2 * pi);  //calcola la superficie dello specchio d'acqua

float volume = superficie * altezza * 1000; //calcola il volume della cisterna (in metri cubi)

float valore_massimo_misurazione;

#else
#error "Formato non supportato"
#endif





//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// CONFIGURATION DATA    -- ADAPT TO YOUR NETWORK !!!
// DATI DI CONFIGURAZIONE -- ADATTARE ALLA PROPRIA RETE WiFi !!!
#ifndef __MY_NETWORK_H

// here pur SSID of your network
// inserire qui lo SSID della rete WiFi
#define MY_SSID ""

// here put PASSWORD of your network. Use "" if none
// inserire qui la PASSWORD della rete WiFi -- Usare "" se la rete non è protetta
#define MY_PASS ""

// here put required IP address (and maybe gateway and netmask!) of your Fishino
// comment out this lines if you want AUTO IP (dhcp)
// NOTE : if you use auto IP you must find it somehow !
// inserire qui l'IP desiderato ed eventualmente gateway e netmask per il fishino
// commentare le linee sotto se si vuole l'IP automatico
// nota : se si utilizza l'IP automatico, occorre un metodo per trovarlo !
#define IPADDR  192, 168,   1, 132
#define GATEWAY 192, 168,   1, 1
#define NETMASK 255, 255, 255, 0

// here put your bot's Telegram token -- see pdf instructions for setup
// inserire qui la token del bot Telegram -- vedere le istruzioni nel pdf allegato
#define MY_TELEGRAM_TOKEN ""

#endif
//                    END OF CONFIGURATION DATA                      //
//                       FINE CONFIGURAZIONE                         //
///////////////////////////////////////////////////////////////////////

// define ip address if required
// NOTE : if your network is not of type 255.255.255.0 or your gateway is not xx.xx.xx.1
// you should set also both netmask and gateway
#ifdef IPADDR
IPAddress ip(IPADDR);
#ifdef GATEWAY
IPAddress gw(GATEWAY);
#else
IPAddress gw(ip[0], ip[1], ip[2], 1);
#endif
#ifdef NETMASK
IPAddress nm(NETMASK);
#else
IPAddress nm(255, 255, 255, 0);
#endif
#endif

// this one is optional, just to show wifi connection details
// questa è opzionale, solo per mostrare i dettagli della connessione wifi
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  // stampa lo SSID della rete:
  Serial.print("SSID: ");
  Serial.println(Fishino.SSID());

  // print your WiFi shield's IP address:
  // stampa l'indirizzo IP della rete:
  IPAddress ip = Fishino.localIP();
  Serial << F("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  // stampa la potenza del segnale di rete:
  long rssi = Fishino.RSSI();
  Serial << F("signal strength (RSSI):");
  Serial.print(rssi);
  Serial << F(" dBm\n");
}

int getSensorDistance(void) {


  int distance;

  digitalWrite(trigPin, LOW);

  delayMicroseconds(5);

  // Fa generare l'impulso al sensore settando al livello logico alto il trigPin per 10 microsecondi
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Legge l'echoPin. pulseIn() restituisce la durata dell'impulso (lunghezza dell'impulso) in microsecondi:
  duration = pulseIn(echoPin, HIGH);

  // calcola la distanza
  distance = duration * 0.034 / 2;
  return distance;
}

// fishgram event handler -- just display message on serial port
bool FishGramHandler(uint32_t id, const char *firstName, const char *lastName, const char *message) {
  misura = getSensorDistance();   //legge i dati dal sensore richiamando la funzione creata precedntemente TODO Sistemare questo comando

  if (firstName)
    Serial << firstName << " ";
  else
    Serial << F("Qualcuno ");
  Serial << F("ha scritto ") << message << "\n";

  int len = 0;

  while (message[len] != '\0')
    len++;

  char msg_conv[len + 1];
  strcpy(msg_conv, message);
  int i = 0;
  while (msg_conv[i]) {
    msg_conv[i] =  tolower(msg_conv[i]);
    i++;
  }

  //String msg_conv = String(message);
  //msg_conv.toLowerCase();
  Serial.print("Il messaggio convertito ai caratteri lowercase è: ");
  Serial.println(msg_conv); //stampa la nuova stringa convertita


  if (menu == 0) {  //se la variabile menu è pari a zero, è possibile richiamare questi comandi:

    EVERY((intervallo * 60000)) { //moltiplica la variabile intervallo per 60000. questo perchè l'intervallo indica i minuti, e in ciascun minuto sono presenti 60k ms
      int check = 3; //per evitare che il sistema vada in loop sulla lettura dei dati abbiamo impostato un massimo di 3 tentativi di lettura. Se tutti e tre dovessero andare 
                     // a vuoto il sistema ci informerà dell'errore di lettura. 
      if (check != 0 && check != -1) {
        int mis = getSensorDistance();
        if (mis == 0 || mis > valore_massimo_misurazione) {
          check--;
          delay(1000); //attende 1s per evitare che gli ultrasuoni vadano a creare interferenze}
        }
          int vol_misurazione = (mis / 100) * superficie; //la misurazione è in cm. Dividiamo per 100 per ottenere la misurazione in metri
          if (intervallo != 0 && vol_misurazione <= soglia) { //controlla che le notifiche non siano disabilitate e che il valore letto sia inferiore alla soglia di allerta

            for (int i = 0; i < num_utenti; i++) {
              if (utenti[i].used == 1) {
                String messaggio_alert = "Ciao ";
                messaggio_alert += String (utenti[i].name);
                messaggio_alert += ", il livello dell'acqua è inferiore rispetto alla soglia impostata.\n";
                messaggio_alert += "Il livello è ";
                messaggio_alert += String (mis);
                messaggio_alert += " litri.";
                FishGram.sendMessage(utenti[i].telegram_id, messaggio_alert.c_str());
              }
            }
          }
            check = -1;
        }
      
      if (check == 0){ //Se si ottengono 3 misurazioni consecutive errate, l'utente viene informato dell'errore
            for (int i = 0; i < num_utenti; i++) {
              if (utenti[i].used == 1) {
                String messaggio_alert = "Ciao ";
                messaggio_alert += String (utenti[i].name);
                messaggio_alert += ", ho provato a effettuare le misurazioni non risultavano corrette. \n";
                messaggio_alert += "Prova a controllare che il sensore sia funzionante e posizionato correttamente";
                FishGram.sendMessage(utenti[i].telegram_id, messaggio_alert.c_str());
              }
            }
      }
      
      }
    
      if (strcmp(msg_conv, "distanza") == 0) { //se si invia il testo "distanza" otteniamo la distanza tra il sensore e lo specchio d'acqua, prima in cm e poi in m

        String risposta = "La distanza in centimetri e' ";
        risposta += String(misura);
        risposta += " cm. \nIn metri la distanza è ";
        risposta += String(float(misura) / 100);
        FishGram.sendMessage(id, risposta.c_str());
      }

      if (strcmp(msg_conv, "volume") == 0) { // se si invia il testo "volume", ottentiamo i volume istantaneo della cisterna

        String risposta = "Il volume attuale e' ";
        risposta += String((misura / 100) * superficie);
        risposta += " metri cubi";
        FishGram.sendMessage(id, risposta.c_str());
      }

      if (strcmp(msg_conv, "id") == 0) { //se si invia il comando "id" riceviamo il Telegram ID associato al nostro account

        String risposta_id = "Il tuo id e' ";
        risposta_id += String(id);
        FishGram.sendMessage(id, risposta_id.c_str());
      }

      else if (strcmp(msg_conv, "capacita'") == 0) { // se si invia il comando "capacità" riceviamo il volume nominale della cisterna

        String risposta_cap = "La capacità del serbatoio e' ";
        risposta_cap += String(volume);
        risposta_cap += " litri.";
        FishGram.sendMessage(id, risposta_cap.c_str());
      }

      else if (strcmp(msg_conv, "imposta soglia") == 0) { // "imposta soglia" per impostare la soglia di allerta

        String risposta_menu_soglia = "Sei entrato nel menu' per la configurazione della soglia di allerta.\n";
        risposta_menu_soglia += "L'attuale valore è: ";
        risposta_menu_soglia += String(soglia);
        risposta_menu_soglia += ".\nInserisci il valore dell'acqua al di sotto del quale riceverai le notifiche.";
        FishGram.sendMessage(id, risposta_menu_soglia.c_str());
        menu = 1; //entra nel menù 1
      }

      else if (strcmp(msg_conv, "imposta intervallo") == 0) { //imposta l'intervallo di verifica del livello dell'acqua

        String risposta_menu_intervallo = "Sei entrato nel menu' per la configurazione dell'intervallo di tempo di verifica.\n";
        risposta_menu_intervallo += "Il valore attuale è: ";
        risposta_menu_intervallo += String(intervallo);
        risposta_menu_intervallo += ".\nOgni quanti minuti vuoi ricevere le eventuali notifiche?\nN.B.: Impostando l'intervallo a 0, verranno disabilitate le notifiche.";
        FishGram.sendMessage(id, risposta_menu_intervallo.c_str());
        menu = 2; //entra nel menù 2
      }

      else if (strcmp(msg_conv, "utenti") == 0) { //mostra la configurazione attuale degli utenti e chiede se vogliamo apportare delle modifiche

        String risposta_user1 = "Sei entrato nel menu' di configurazione dei destinatari delle notifiche. \n";
        risposta_user1 += "Il numero massimo di utenti e' ";
        risposta_user1 += String(num_utenti);
        risposta_user1 += ".\nPer variarlo è necessario modificare lo sketch.\n ";
        risposta_user1 += "La configurazione attuale e': \n";

        for (int i = 0; i < num_utenti; i++) { //esegue un ciclo for per aggiungere alla stringa i dati degli utenti

          risposta_user1 += "Utente num. ";
          risposta_user1 += String(i + 1); //aggiunge 1 per numerare a partire da 1 anzichè dell'indice i (=0)
          risposta_user1 += ". Nome: ";

          //  for (int n = 0; n < utenti[i].len && utenti[i].name[n] != '\0'; i++) {
          risposta_user1 += utenti[i].name; // aggiunge il nome dell'utemte
          //}


          risposta_user1 += ". Id:  ";
          risposta_user1 += String(utenti[i].telegram_id); //aggiunge il corrispettivo Telegram ID
          risposta_user1 += " \n\n";

        }
        risposta_user1 += "\n\n\n";

        risposta_user1 += "Vuoi apportare delle modifiche? (Y/n)";
        FishGram.sendMessage(id, risposta_user1.c_str());

        menu = 3; //entra nel menù 3

      }

    }

    else if (menu != 0) {

      if (strcmp(msg_conv, "annulla") == 0)  { // se siamo entrati in un menù con valore >= 1, possiamo tornare alla homepage digitando il comando "annulla"

        FishGram.sendMessage(id, "Fatto, ho annullato la tua richiesta!");
        //reimposta il valore delle variabili temporanee

        new_tid = 0;
        new_name[0] = '\0';
        new_len = 0;
        menu = 0; //ritorna alla homepage
      }

      if (menu == 1) { // menù impostazione soglia
        uint32_t soglia_new = (uint32_t)(atoi(msg_conv)); //converte il messaggio ricevuto in un intero e successivamente fa il cast a uint32_t

        String risposta_imp_soglia = "Hai inserito il valore:  ";
        risposta_imp_soglia += String(soglia_new);
        if (soglia_new >= volume) //se il valore inserito è maggiore del volume della cisterna, il sistema ci avvisa
          risposta_imp_soglia += ".\nIl valore di soglia non può essere maggiore del volume del serbatoio!";
        else { // in caso contrario salva i dati nell'EEPROM, aggiorna il valore della variabile "soglia" e ci avvisa
          FishinoEEPROM.write32u(slot_soglia, soglia_new);
          soglia = soglia_new;
          risposta_imp_soglia += ".\nFatto! Ho salvato il valore nell'EEPROM.";
        }
        FishGram.sendMessage(id, risposta_imp_soglia.c_str());
        menu = 0; //ritorna alla "homepage"


      }

      else if (menu == 2) { // menù impostazione intervallo di notifica

        uint32_t intervallo_new = (uint32_t)(atoi(msg_conv)); //converte il messaggio ricevuto in un intero e successivamente fa il cast a uint32_t


        String risposta_imp_intervallo = "Hai inserito il valore:  ";
        risposta_imp_intervallo += String(intervallo_new);
        FishGram.sendMessage(id, risposta_imp_intervallo.c_str());

        FishinoEEPROM.write32u(slot_intervallo, intervallo_new); //salbva il valore nell'EEPROM
        intervallo = intervallo_new;  //aggiorna la variabile "intervallo"
        String risposta_imp_intervallo_1 = "\nFatto! Ho impostato scritto correttamente il valore nell'EEPROM.";
        if (intervallo_new == 0) {
          Serial.println("Intervallo impostato a 0. Notifiche disabilitate");
          risposta_imp_intervallo_1 += "\nIntervallo impostato a 0. Notifiche disabilitate"; // Se il valore è pari a zero, ci avvisa che le notifiche verranno disabilitate
        }

        FishGram.sendMessage(id, risposta_imp_intervallo_1.c_str());
        menu = 0; //torna alla homepage

      }


      else if (menu == 3) { // menù editing utenti, precedentemente richiamato dal comando "utenti". Ci è stato chiesto se vogliamo modificare la configurazione attuale:
        if (strcmp(msg_conv, "n") == 0) {
          FishGram.sendMessage(id, "Fatto, ho annullato la tua richiesta!"); //se vogliamo mantenerla invariata basta digitare "n" per tornare alla homepage
          menu = 0;
        }

        else if ((strcmp(msg_conv, "y") == 0) || (strcmp(msg_conv, "s") == 0)) { // se vogliamo modificarla basta risponere "y" oppure "s" e ci verrà chiesto quale utene vogliamo modficare

          FishGram.sendMessage(id, "Scegli quale utente aggiungere o aggiornare");

          menu = 4; //entra nel menù 4

        }

      }

      else if (menu == 4) { // se nel menù precedente abbiamo accettato di modificare la configurazione, adesso dobbiamo scegliere quale utente editare

        scelta_utente = atoi (msg_conv); //converte il messaggio ricevuto in intero
        if (scelta_utente <= 0 || scelta_utente > num_utenti) { //se il numero inserito è <= 0 oppure maggiore del numero degli utenti il sistema ci avvisa dell'errore e torna nell'homepage
          FishGram.sendMessage(id, "Scelta non valida, riprova!");
          menu = 0; //torna all'home page
        }

        else {
          String risp_scelta_utente = "Hai scelto l'utente #"; //se la scelta è corretta il sistema ci riepiloga i dati dell'utente scelto: nome e Telegram ID e ci chiede se modificarlo o eliminarlo
          risp_scelta_utente += String (scelta_utente);
          risp_scelta_utente += ": ";
          risp_scelta_utente += String (utenti[scelta_utente - 1].name);
          risp_scelta_utente += "   ";
          risp_scelta_utente += String (utenti[scelta_utente - 1].telegram_id);
          risp_scelta_utente += "\nCosa vuoi fare? Scrivi D per eliminare, A per aggiungere o per aggiornare.";
          FishGram.sendMessage(id, risp_scelta_utente.c_str());
          menu = 5; // entra nel menù 5

        }

      }


      else if (menu == 5) { //chiede di inserire il Telegram ID ed entra nel menu 6


        if (strcmp(msg_conv, "d") == 0) { //se alla proposta di eliminare o aggiornare i dati di un utente premiamo ripsondiamo "d", questi vengono eliminati

          utenti[scelta_utente - 1].telegram_id = 0; //azzera il Telegram ID
          utenti[scelta_utente - 1].used = 0; //imposta il valore dell'utente a 0, cioè non utilizzato
          utenti[scelta_utente - 1].len = 0; //azzera la lunghezza della stringa
          utenti[scelta_utente - 1].name[0] =  '\0'; //azzera la stringa contenente il nome

          FishinoEEPROM.write8u(EEPROM_Data_Start + 17 * (scelta_utente - 1), utenti[scelta_utente - 1].telegram_id); //azzera il Telegram ID sull'EEPROM
          FishinoEEPROM.write8u(EEPROM_Data_Start + 17 * (scelta_utente - 1) + 1, utenti[scelta_utente - 1].used); //imposta il valore dell'utente a 0, cioè non utilizzato sull'EEPROM
          FishinoEEPROM.write8u(EEPROM_Data_Start + 17 * (scelta_utente - 1) + 2, utenti[scelta_utente - 1].len); //azzera la lunghezza della stringa sull'EEPROM
          FishinoEEPROM.write8u(EEPROM_Data_Start + 17 * (scelta_utente - 1) + 3, utenti[scelta_utente - 1].name[0]);  //azzera la stringa contenente il nome sull'EEPROM


          FishGram.sendMessage(id, "Utente eliminato!"); //ci viene notificata l'avvenuta eliminazione dell'utente

          scelta_utente = 0; //reimposta il valore della scelta utente a 0
          menu = 0; //torna alla homepage
        }

        else  if (strcmp(msg_conv, "a") == 0) {
          FishGram.sendMessage(id, "Inserisci il Telegram ID: ");
          menu = 6; //entra nel menù 6
        }
      }

      else if (menu == 6) {  //inserimento Telegram ID
        int n = 0;
        //TODO implementare controllo caratteri usando msg_conv[n].isAlpha. Così facendo si possono inserire solo numeri
        new_tid = atoi (msg_conv); //converte il messaggio convertito in intero e salva il valore nella variabile temporanea "new_tid"
        String risposta_menu6 = "Hai inserito il valore ";
        risposta_menu6 +=  String (new_tid);
        risposta_menu6 += ".\nSe hai sbagliato il valore scrivi 'annulla', altrimenti inserisci il nome dell'utente (max 10 caratteri)."; //ci chiede di inserire il nome dell'utente
        FishGram.sendMessage(id, risposta_menu6.c_str());
        menu = 7;//entra nel menù 7


      }
      else if (menu == 7) { //inserimento nome

        int i = 0; // crea una variabile temporanea e la imposta a zero
        while (i < 10 && msg_conv[i] != '\0') {//Per questioni di spazio sull'EEPROM il sistema accetta massimo 10 caratteri più il terminatore '\0'. Esegue un ciclo while fino a quando il valore di i è minore di 10 o non viene rilevato il terminatore

          //utenti[scelta_utente - 1].name[i] = msg_conv[i];
          new_name[i] = msg_conv[i]; //copia il carattere di indice i dal messaggio in ingresso al corrispettivo della variabile temporanea "new_name"
          new_len++; //incremenenta il valore della lunghezza della stringa new_len
          i++;
        }
        new_name[new_len + 1] = '\0'; //aggiunge il terminatore
        new_len++;
        String risposta_menu7 = "Hai inserito il nome ";
        risposta_menu7 += String(new_name);
        risposta_menu7 += ".\nSe hai sbagliato a inserire il nome scrivi 'annulla'.";
        risposta_menu7 += "\nConfermi le scelte inserite? (Y/n)"; //l sistema ci chiede se vogliamo congermare i dati inseriti finora
        FishGram.sendMessage(id, risposta_menu7.c_str());

        menu = 8;//entra nel menù 8


      }

      else if (menu == 8) {//per salvare i dati inseriti

        if ((strcmp (msg_conv, "y") == 0) || (strcmp (msg_conv, "s") == 0)) {// se si risponde "y" oppure "s" salva i dati sull'EEPROM


          utenti[scelta_utente - 1].telegram_id = new_tid; //passa il valore del Telegram ID dalla variabile temporanea alla variabile definitiva
          FishinoEEPROM.write32u(EEPROM_Data_Start + 17 * i, utenti[scelta_utente - 1].telegram_id); //salva il valore definitivo del Telegram ID sull'EEPROM

          utenti[scelta_utente - 1].used = 1; //imposta lo stato dell'utente a 1 (usato)
          FishinoEEPROM.write8u(EEPROM_Data_Start + 4 + 17 * i, utenti[scelta_utente - 1].used); //salva i dati sullo stato sull'EEPROM

          utenti[scelta_utente - 1].len = new_len;  //passa il valore della lunghezza della stringa dalla variabile temporanea alla variabile definitiva
          FishinoEEPROM.write8u(EEPROM_Data_Start + 5 + 17 * i, utenti[scelta_utente - 1].len); //salva il valore definitivo della lunghezza della stringa sull'EEPROM

          for (int n = 0; n < (utenti[scelta_utente - 1].len); n++) //avvia un ciclo for per ciascun carattere della stringa: parte dall'indice i=0 fino alla lunghezza della stinga
          {
            utenti[scelta_utente - 1].name[n] =  new_name[n]; //passa il carattere di indice i dalla stringa temporanea alla stringa definitiva
            FishinoEEPROM.write8u(EEPROM_Data_Start + 6 + n + 17 * i, utenti[scelta_utente - 1].name[n]); //salva il carattere di indice i dalla stringa definitiva all'opportuno slot EEPROM
            //Serial.print(utenti[i].name[n]);
          }
          utenti[scelta_utente - 1].name[ utenti[scelta_utente - 1].len] = '\0'; //aggiunge il terminatore alla stringa definitivs
          //FishinoEEPROM.write8u(EEPROM_Data_Start + 6 + n + 17 * i,  utenti[scelta_utente - 1].name[ utenti[scelta_utente - 1].len]); //salva il terminatore sull'opportuno slot dell'EEPROM
          // TODO sistemare salvataggio carattere terminatore

          //for (int i = 0; i<
          FishGram.sendMessage(id, "Fatto, i dati sono stati salvati!"); //comunica l'avvenuto salvataggio

        }
        // strcpy(temp, input);              // flash: 2366, SRAM: 254, works

        else if ((strcmp (msg_conv, "n")) == 0) {

          FishGram.sendMessage(id, "Fatto, i dati non verranno salvati!");

        }
        //resetta le variabili temporanee
        new_tid = 0;
        new_len = 0; //TODO probabilmente va settata a 1
        new_name[0] = '\0';

        scelta_utente = 0; //resetta la variabile temporanea della scelta utente
        menu = 0; //torna alla homepage
      }

    }




    return true;

  }



  uint32_t tim;
  void setup(void) {
    // Initialize serial and wait for port to open
    // Inizializza la porta seriale e ne attende l'apertura
    Serial.begin(115200);

    // only for Leonardo needed
    // necessario solo per la Leonardo
    while (!Serial)
      ;

    // Define inputs and outputs
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    // reset and test WiFi module
    // resetta e testa il modulo WiFi
    while (!Fishino.reset())
      Serial << F("Fishino RESET FAILED, RETRYING...\n");
    Serial << F("Fishino WiFi RESET OK\n");

    // go into station mode
    // imposta la modalità stazione
    Fishino.setMode(STATION_MODE);

    // set PHY mode to 11G
    // some new access points don't like 11B mode
    Fishino.setPhyMode(PHY_MODE_11G);

    // try forever to connect to AP
    // tenta la connessione finchè non riesce
    Serial << F("Connecting to AP...");
    while (!Fishino.begin(MY_SSID, MY_PASS)) {
      Serial << ".";
      delay(2000);
    }
    Serial << "OK\n";

    // setup IP or start DHCP client
    // imposta l'IP statico oppure avvia il client DHCP
#ifdef IPADDR
    Fishino.config(ip, gw, nm);
    //  Fishino.setDNS(gw);
#else
    Fishino.staStartDHCP();
#endif

    // wait till connection is established
    Serial << F("Waiting for IP...");

    while (Fishino.status() != STATION_GOT_IP) {
      Serial << ".";
      delay(500);
    }
    Serial << F("OK\n");


    // print connection status on serial port
    // stampa lo stato della connessione sulla porta seriale
    printWifiStatus();


    tim = millis();

    // start FishGram
    FishGram.messageEvent(FishGramHandler);
    FishGram.begin(F(MY_TELEGRAM_TOKEN));

    if (char(FishinoEEPROM.read8u(0)) != '#') {//necessario per pulire l'EEPROM al primo untilizzo. controlla che nello slot non sia presente il carattere '#'
      FishinoEEPROM.eraseAll(); //se presente pulisce l'intera EEPROM
      Serial.println("EEPROM resettata"); //ci comunica di sul monitor seriale dell'avvenuto reset dell'EEPROM
      FishinoEEPROM.write8u(0, '#');//inserisce il carattere '#' nell'EEPROM
    }
    else {
      Serial.println("EEPROM già resettata");//in caso contrario non fa nulla, si limita ad avvisarci sul monitor seriale

      intervallo = FishinoEEPROM.read32u(slot_intervallo);
      soglia = FishinoEEPROM.read32u(slot_soglia);

      for (int i = 0; i < num_utenti; i++) {

        utenti[i].len = FishinoEEPROM.read8u(EEPROM_Data_Start + 5 + 17 * i);

        //se l'EEPROM è stata resettata il valore risulterà 255, mandando in crash lo sketch. Evitiamo tutto ciò eseguendo un check sull'apposito slot EEPROM
        if (utenti[i].len != 255) {// se il valore è diverso da 255 legge tutti i dati dall'EEPROM, in particolare

          utenti[i].telegram_id = FishinoEEPROM.read32u(EEPROM_Data_Start + 17 * i); //importa il Telegram ID
          utenti[i].used = FishinoEEPROM.read8u(EEPROM_Data_Start + 4 + 17 * i); //imposta lo stato dell'utente a 1, cioè utilizzato

          for (int n = 0; n < (utenti[i].len); n++) { //esegue un ciclo for per importare i caratteri della stringa del nome utente, va dall'indice i=0 alla lunghezza della stringa
            utenti[i].name[n] =  char(FishinoEEPROM.read8u(EEPROM_Data_Start + 6 + n + 17 * i)); //importa il carattere di indice i dall'EEPROM
          }
          utenti[i].name[ utenti[i].len] = '\0'; //aggiunge il terminatore
        }

        else { //in caso contrario

          utenti[i].telegram_id = 0; //imposta il Telegram ID dell'utente a 0
          utenti[i].len = 0; //imposta la lunghezza della stringa a 0.
          utenti[i].name[0] = '\0'; //la stringa contenente il nome dell'utente viene settata come vuota
          utenti[i].used = 0; //lo stato dell'utente viene impostato a 0, cioè non utilizzato

        }
      }
    }
  }

  void loop(void) {
    // process FishGram data
    FishGram.loop();

    if (millis() > tim)
    {
      Serial << F("Free RAM : ") << Fishino.freeRam() << "\n";
      tim = millis() + 3000;
    }

  }
