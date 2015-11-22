/* Projekt - ISA - smtpklient.cpp	*/
/* Klient SMTP pro odesílání pošty	*/
/* Projekt - ISA - smtpklient.cpp	*/
/* Klient SMTP pro odesílání pošty	*/
/* Filip Kalous (xkalou03)   		*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <string>
#include <vector>
#include <time.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

using namespace std;


struct params {

	string ip;
	string port;
	string file;
	int seconds;
};

struct message {

	string content;
	string addressess;
};

typedef vector<message> messageVec;


void initParams(struct params *params);
messageVec parseFile(FILE *f);
int checkParameters(int argc, char **argv, struct params *params);
messageVec checkFile(string nameOfFile);
int connectToServer(params *params);
void checkError(string error);
void signalAction(int signum);
void sendQuit();




/* Inicializace parametru */
void initParams(struct params *params) {

	params->ip = "127.0.0.1";
	params->port = "25";
	params->file = "";
	params->seconds = 0;
}

/* Globalni promenne */
int sock;	// socket
int signalCatch = 0;	// stavova promenna, kdyz odchytime signal
int state = -1;			// stavova promenna, ktera znaci v jake fazi komunikace jsme



/* Funkce pro kontrolu parametru */
int checkParameters(int argc, char **argv, struct params *params) {

	int opt;
	char *error;
	bool pI = false;
	int c = 0;

	if(argc < 2) {

		fprintf(stderr, "Error - too few parameters! Pro nápovědu spusťte program jen s parametrem --help.\n");
		return -1;
	}

	if(strcmp(argv[1], "--help") == 0) {
		fprintf(stdout, "SMTP Klient posilá uživatelům přes SMTP server zprávy, které najde specifikované v souboru, jehož název se zadáva jako parametr programu.\n\n"
				 		"Zde je popis všech parametrů programu:\n"
						"-a - IP - (nepovinný, výchozí hodnota je 127.0.0.1) IP adresa SMTP serveru (možná IPv4 i IPv6)\n"
						"-p - port - (nepovinný, výchozí hodnota je 25) port, na kterém SMTP server očekává příchozí spojení\n"
					    "-i - soubor - (povinný parametr) cesta k souboru, v kterém se nacházejí adresáti a zprávy pro ně\n"
						"-w - sekund - (nepovinný parametr, výchozí hodnota 0) po odeslaní poslední správy sa neukončí spojení okamžitě, " 
						           "ale klient bude uměle udržovat spojení otevřené po dobu specifikovanou tímto parametrem. " 
						           "Nejvyšší hodnota, kterou je možné zadat je jedna hodina.\n");
		exit(0);
	}

	while((opt = getopt(argc, argv, "a:p:w:i:")) != -1) {

		switch(opt) {

			case 'a':

				params->ip.clear();
				params->ip.append(argv[optind - 1]);

				while(optind < argc && *argv[optind] != '-') {
					c++;
					optind++;
				}
				break;

			case 'i':

				params->file.clear();

				params->file.append(argv[optind - 1]);
				pI = true;
				while(optind < argc && *argv[optind] != '-') {
					c++;
					optind++;
				}
				break;


			case 'p':
				
				strtol(argv[optind -1], &error, 10);
				if(*error) {
					fprintf(stderr, "%s\n", "Parametr -p musí obsahovat pouze čísla. Pro nápovědu spusťte program jen s parametrem --help.");
					return -1;
				}

				params->port.append(argv[optind -1]);

				while(optind < argc && *argv[optind] != '-') {
					c++;
					optind++;
				}
				break;

			case 'w':

				params->seconds = strtol(argv[optind - 1], &error, 10);
				if(*error) {
					fprintf(stderr, "%s\n", "Parametr -w musí obsahovat pouze čísla. Pro nápovědu spusťte program jen s parametrem --help.");
					return -1;
				}
				// osetreni casu zpozdeni
				if(params->seconds > 3600 || params->seconds < 0) {
					fprintf(stderr, "%s\n", "Zadali jste čas větší než hodina nebo nesmyslný čas.");
				}
				
				while(optind < argc && *argv[optind] != '-') {
					c++;
					optind++;
				}
				break;

			default:
				fprintf(stderr, "%s\n", "Neočekáváný parametr. Pro nápovědu spusťte program jen s parametrem --help.");
				exit(0);
				break;
		}
	}

	if(c != 0) {
		fprintf(stderr, "%s\n", "Byl zadaný nesprávný parametr. Pro nápovědu spusťte program jen s parametrem --help.");
		return -1;
	}

	if(!pI) {
		fprintf(stderr, "%s\n", "Nebyl zadan povinny parametr -i. Pro nápovědu spusťte program jen s parametrem --help.");
		return -1;
	}

	return 0;
}

/* Otevreni souboru */
messageVec checkFile(string nameOfFile) {
	
	FILE *f = fopen(nameOfFile.c_str(), "r");
	
	if(f != NULL) {
	 
		return parseFile(f);
	}
	else {
		
		fprintf(stderr, "Chyba - nepovedlo se otevřít vámi zadaný soubor. Pro nápovědu spusťte program jen s parametrem --help.\n");
		exit(-1);
	}	
}

messageVec parseFile(FILE *f) {

	size_t bufferSize = 1000;
    char *buffer = (char*) malloc(bufferSize * sizeof(char));
    struct message tempMes;
    string tempA;
    string tempC;
    messageVec vecOfMessages;	// vektor pro ukladani struktur message


    int lineNumber = 0;
    
    while(getline(&buffer, &bufferSize, f) != -1) { /* Zde si budu ukladat kazdy radek, pro nasledne odeslani emailu */
    
    	tempA              = strtok(buffer, " "); 
    	tempC              = strtok(NULL, "");
    	tempC.erase(tempC.size()-1);		// smazani prebytecneho \n
    	tempMes.content    = tempC;
    	tempMes.addressess = tempA;
    	lineNumber++;
    	vecOfMessages.push_back(tempMes);
    }

    fclose(f);		// soubor uz nebude potreba, muzeme zavrit
    free(buffer);	// buffer jiz take nebude potreba, vycistime pamet
    
	return vecOfMessages;
}

int connectToServer(params *params) {

	int sock;
    struct addrinfo hints;
	struct addrinfo *res;


	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int errHost = getaddrinfo(params->ip.c_str(), params->port.c_str(), &hints, &res);

	if (errHost)
	{
	    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errHost));
	    return -1;
	}

	sock = socket(res->ai_family, res->ai_socktype, 0);
	if(sock == -1) {
		
		fprintf(stderr, "Nepodařilo se vytvořit socket.");
    	return(-1);
    }	

	if(connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
	    perror("Connect");
	    return -1;
	}
    
	return sock;
}

void sendMessage(int sock, string text, int context) {

	state = context;

	if(context != 0)	// chceme ziskat welcome message od serveru
		write(sock, text.c_str(), text.size());
}

string recvMessage(int sock, int *err) {

	unsigned char c;
	string buffer("");
	*err = 0;

	while(1) {
		
		if(recv(sock, &c, sizeof(unsigned char), 0) < 1)
			break;
		else if(c == '\n')
			break;
		else if(c != '\r')
			buffer +=c;
	}

	/* Overeni, zda server neukoncil komunikaci */
	if(buffer.find("421") != string::npos) {

		fprintf(stderr, "%s\n", "Server ukoncil spojeni.");
		close(sock);
		exit(0);
	}

	/* Overeni, zda od serveru prisla zprava o uspechu, pokud ne, musime vyresit chybu. */
	if(!(buffer.find("250") != string::npos || 
	     buffer.find("251") != string::npos || 
	     buffer.find("252") != string::npos || 
	     buffer.find("220") != string::npos || 
	     buffer.find("354") != string::npos || 
	     buffer.find("221") != string::npos)) {

		*err = 1;
		checkError(buffer);
	}

	return buffer;	
}

void checkError(string error) {

	fprintf(stderr, "Error: %s\n", error.c_str());
}

void signalAction(int signum) {

	signalCatch = 1;
	
	if(state == -1) {
		fprintf(stderr, "Program byl ukoncen signalem %d.\n", signum);
		close(sock);
		exit(0);
	}

	else if(state < 4 || state > 5) { 	// okamzite muzu zabit klienta, pokud uz neposilam zpravu na server
		
		sendQuit();
		fprintf(stderr, "Program byl ukoncen signalem %d.\n", signum);
		close(sock);
		exit(0);
	}
}

void sendQuit() {

	sendMessage(sock, "QUIT\r\n", 7);
}


int main(int argc, char **argv) {

	struct params params;
	struct sigaction action;
	messageVec vecOfMessages;
	string from = "xkalou03";
	string buffer;
	int err = 0;

	/* Osetreni signalu sigterm, sigint a sigquit */
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = signalAction;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGQUIT, &action, NULL);
	sigaction(SIGINT, &action, NULL);



	/* Inicializace struktury s parametry programu */
	initParams(&params);
	if(checkParameters(argc, argv, &params) == -1)
		return -1;


	vecOfMessages = checkFile(params.file);	// ziskani vektoru se zpravami a adresy
	
	/* Vytvareni socketu a pripojeni se k serveru, pokud se toto nepovede, koncim s provadenim programu */
	if((sock = connectToServer(&params)) == -1) {
		return -1;
	}

	/* KOMUNIKACE SE SERVEREM */
	
	sendMessage(sock, "", 0);									// ziskani welcome message
	recvMessage(sock, &err);
	
	sendMessage(sock, "EHLO " + from + "\r\n", 1);				// uvodni zprava - poslani pozdravu EHLO s adresou klienta
	
	while((buffer = recvMessage(sock, &err).c_str()) != "") {	
		
		if(buffer.find("250 ") != string::npos) { 				// cteme tak dlouho, dokud nenajdeme posledni radek, 
												  				// ktery musi zacinat kodem 250 a za nim hned mezera, ostatni radky maji pomlcku za kodem
            break;
        }
	}
	
	// komu budeme data posilat
	unsigned int i = 0;
	unsigned int pos;
	string temp;
	string delimiter = ",";
	
	/* Cykleni dokud mam co odesilat */
	while(i < vecOfMessages.size()) {
	
		pos = 0;
		sendMessage(sock, "MAIL FROM: <" + from + ">\r\n", 2);
		buffer = recvMessage(sock, &err);		
	
		/* Cyklus pro postupne parsovani adres */
		while((pos = vecOfMessages[i].addressess.find(delimiter)) != string::npos) {
		
			temp = vecOfMessages[i].addressess.substr(0, pos);
			
		  	sendMessage(sock, "RCPT TO: <" + temp + ">\r\n", 3);
			buffer = recvMessage(sock, &err);
			
			vecOfMessages[i].addressess.erase(0, pos + delimiter.length());
		}
		
		sendMessage(sock, "RCPT TO: <" + vecOfMessages[i].addressess + ">\r\n", 3);
		buffer = recvMessage(sock, &err);
		
		
		sendMessage(sock, "DATA\r\n", 4);
		buffer = recvMessage(sock, &err);

		/* Poslani cele zpravy serveru a znaku . - informuje server o tom, ze konci zprava */
		sendMessage(sock, vecOfMessages[i].content, 5);

		sendMessage(sock, "\r\n.\r\n", 6);
		buffer = recvMessage(sock, &err);
		
		/* Pokud v prubehu odesilani zpravy dorazil signal o ukonceni programu, 
		   zde to zjistim a ukoncim spojeni se serverem a nasledne program */
		if(signalCatch == 1) {
			
			sendQuit();
			buffer = recvMessage(sock, &err);
			close(sock);	
			exit(0);
		}
		
		i++;	// provedeme dalsi iteraci - poslani dalsi zpravy
	}

	int tempSec = 240;	// nastaveni 4 minut (240 sekund)
	
	if(params.seconds != 0) {	// pokud mame udrzovat spojeni

		if(params.seconds < 240)	// pokud je cekani mensi jak 4 minuty, 
									// nemusim na server posilat NOOP zpravu
			sleep(params.seconds);

		else {

			while(42) {				// jinak se cyklim do te doby,
									// nez se dostanu pod 4 minuty
				sleep(tempSec);
				sendMessage(sock, "NOOP\r\n", 8);
				recvMessage(sock, &err);
				params.seconds -= tempSec;
				if(params.seconds < 240) {
					sleep(params.seconds);
					break;
				}
			}
		} 
	}
	
	/* Ukonceni cele konverzace se serverem */
	sendQuit();
	buffer = recvMessage(sock, &err);	
	
	close(sock);
	
	
	return 0;
}