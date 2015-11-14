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
void checkError(string error, int sock);
void signalAction(int signum);
void sendQuit();




/* Inicializace parametru */
void initParams(struct params *params) {

	params->ip = "127.0.0.1";
	params->port = "25";
	params->file = "";
	params->seconds = 0;
}

int sock;
int signalCatch = 0;
int state = 0;



/* Funkce pro kontrolu parametru */
int checkParameters(int argc, char **argv, struct params *params) {

	int opt;
	char *error;
	bool pI = false;
	int c = 0;
	int checkPort = 0;

	/*if(argc > 5) {

		fprintf(stderr, "Error - too many parameters! Pro nápovědu spusťte program jen s parametrem --help.\n");
		return -1;
	}*/

	if(argc < 2) {

		fprintf(stderr, "Error - too few parameters! Pro nápovědu spusťte program jen s parametrem --help.\n");
		return -1;
	}

	if(strcmp(argv[1], "--help") == 0) {
		fprintf(stdout, "SMTP Klient posilá uživatelům přes SMTP server zprávy, které najde specifikované v souboru, jehož název se zadáva jako parametr programu.\n\n"
				 		"Zde je popis všech parametrů programu:\n"
						"IP - (nepovinný, výchozí hodnota je 127.0.0.1) IP adresa SMTP serveru (možná IPv4 i IPv6)\n"
						"port - (nepovinný, výchozí hodnota je 25) port, na kterém SMTP server očekává příchozí spojení\n"
					    "súbor - (povinný parametr) cesta k souboru, v kterém se nacházejí adresáti a zprávy pro ně\n"
						"sekúnd - (nepovinný parametr, výchozí hodnota 0) po odeslaní poslední správy sa neukončí spojení okamžitě, " 
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
				
				checkPort = strtol(argv[optind -1], &error, 10);
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
		
		fprintf(stderr, "Chyba - nepovedlo se otevřít vámi zadaný soubor. Pro nápovědu spusťte program jen s parametrem --help.");
		exit(-1);
	}	
}

messageVec parseFile(FILE *f) {

	
	size_t bufferSize = 100;
    char *buffer = (char*) malloc(bufferSize * sizeof(char));
    struct message tempMes;
    string tempA;
    string tempC;
    messageVec vecOfMessages;	// vektor pro ukladani struktur message


    int lineNumber = 0;
    
    while(getline(&buffer, &bufferSize, f) != -1) { /* Zde si budu ukladat kazdy radek, pro nasledne odeslani emailu */
    
    	tempA = strtok(buffer, " "); 
    	tempC = strtok(NULL, "");
    	tempMes.content = tempC;
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
	struct sockaddr_in server;
    struct addrinfo hints = { 0 };
	struct addrinfo *res;


	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int errHost = getaddrinfo(params->ip.c_str(), params->port.c_str(), &hints, &res);

	if (errHost)
	{
	    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errHost));
	    return 1;
	}

	sock = socket(res->ai_family, res->ai_socktype, 0);
	if(sock == -1) {
		
		fprintf(stderr, "Nepodarilo se vytvorit socket.");
    	return(-1);
    }	

	if(connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
	    perror("connect");
	    return 1;
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
	int b;
	*err = 0;

	while(1) {
		
		if(recv(sock, &c, sizeof(unsigned char), 0) < 1)
			break;
		else if(c == '\n')
			break;
		else if(c != '\r')
			buffer +=c;
	}
	
	if(!(buffer.find("250") != string::npos || 
	     buffer.find("251") != string::npos || 
	     buffer.find("252") != string::npos || 
	     buffer.find("220") != string::npos || 
	     buffer.find("354") != string::npos || 
	     buffer.find("221") != string::npos)) {

		*err = 1;
		checkError(buffer, sock);
	}

	return buffer;	
}

void checkError(string error, int sock) {

	int err = 0;
	printf("Error: %s\n", error.c_str());
	sendQuit();
	exit(0);
}

void signalAction(int signum) {

	fprintf(stderr, "Dostal jsem signal %d na zabiti!\n", signum);
	signalCatch = 1;

	if(state < 4 || state > 5) {
		
		sendQuit();
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
	string buffer("");
	char c;
	int err = 0;

	/* Osetreni signalu sigterm, sigint a sigquit */
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = signalAction;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGQUIT, &action, NULL);
	sigaction(SIGINT, &action, NULL);




	initParams(&params);
	if(checkParameters(argc, argv, &params) == -1)
		return -1;


	vecOfMessages = checkFile(params.file);	// ziskani vektoru se zpravami a adresy

	// connecting to smtp server and creating socket

	if((sock = connectToServer(&params)) == -1) {
		return -1;
	}

	/* Finally - communication with server */
	
	sendMessage(sock, "", 0);	// ziskani welcome message
	printf("%s\n", recvMessage(sock, &err).c_str());
	
	sendMessage(sock, "EHLO " + from + "\r\n", 1);	// uvodni zprava - poslani pozdravu EHLO s adresou klienta
	
	while((buffer = recvMessage(sock, &err).c_str()) != "") {	
		
		if(buffer.find("250 ") != string::npos) { // cteme tak dlouho, dokud nenajdeme posledni radek, 
												  // ktery musi zacinat kodem a za nim hned mezera, ostatni radky maji pomlcku za kodem
            break;
        }
	}
	
	// komu budeme data posilat
	int i = 0;
	int pos;
	string temp;
	string delimiter = ",";
	
	/* Cykleni dokud mam co odesilat */
	while(i < vecOfMessages.size()) {
	
		pos = 0;
		sendMessage(sock, "MAIL FROM: <" + from + ">\r\n", 2);
	
		// prijmuti odpovedi
		buffer = recvMessage(sock, &err);	
		//printf("%s\n", buffer.c_str());
	
	
		/* Cyklus pro postupne parsovani adres */
		   
		while((pos = vecOfMessages[i].addressess.find(delimiter)) != string::npos) {
		
			temp = vecOfMessages[i].addressess.substr(0, pos);
			
		  	sendMessage(sock, "RCPT TO: <" + temp + ">\r\n", 3);
			buffer = recvMessage(sock, &err);	
			//printf("%s\n", buffer.c_str());
			
			vecOfMessages[i].addressess.erase(0, pos + delimiter.length());
		}
		
		sendMessage(sock, "RCPT TO: <" + vecOfMessages[i].addressess + ">\r\n", 3);
		buffer = recvMessage(sock, &err);	
		//printf("%s\n", buffer.c_str());
		
		sendMessage(sock, "DATA\r\n", 4);
		buffer = recvMessage(sock, &err);	
		//printf("%s\n", buffer.c_str());

		/* Poslani cele zpravy serveru a znaku . - informuje server o tom, ze konci zprava */
		sendMessage(sock, vecOfMessages[i].content, 5);

		sendMessage(sock, "\r\n.\r\n", 6);
		buffer = recvMessage(sock, &err);
		
		if(signalCatch == 1) {
			
			sendMessage(sock, "QUIT\r\n", 7);
			buffer = recvMessage(sock, &err);	
			exit(0);
		}
		
		i++;
	}
	
	if(params.seconds != 0) {
		
		sleep(params.seconds);
	}
	
	/* Ukonceni cele konverzace se serverem */
		sendMessage(sock, "QUIT\r\n", 7);
		buffer = recvMessage(sock, &err);	
		printf("%s\n", buffer.c_str());
		
	//printf("%d\n", err);
	
	
	return 0;
}






