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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;


struct params {

	string ip;
	int port;
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



/* Inicializace parametru */
void initParams(struct params *params) {

	params->ip = "127.0.0.1";
	params->port = 25;
	params->file = "";
	params->seconds = 0;
}


/* Funkce pro kontrolu parametru */
int checkParameters(int argc, char **argv, struct params *params) {

	int opt;
	char *error;
	bool pI = false;
	int c = 0;

	/*if(argc > 5) {

		fprintf(stderr, "Error - too many parameters! Pro nápovědu spusťte program jen s parametrem --help.\n");
		return -1;
	}*/

	if(argc < 2) {

		fprintf(stderr, "Error - too few parameters! Pro nápovědu spusťte program jen s parametrem --help.\n");
		return -1;
	}

	if(strcmp(argv[1], "--help") == 0) {
		printf("HELP PLS!\n");
		return 0;
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

				params->port = strtol(argv[optind -1], &error, 10);
				if(*error) {
					fprintf(stderr, "%s\n", "Parametr -p musí obsahovat jen číslice. Pro nápovědu spusťte program jen s parametrem --help.");
					return -1;
				}
				while(optind < argc && *argv[optind] != '-') {
					c++;
					optind++;
				}
				break;

			case 'w':

				params->seconds = strtol(argv[optind - 1], &error, 10);
				if(*error) {
					fprintf(stderr, "%s\n", "Parametr -w musí obsahovat jen číslice. Pro nápovědu spusťte program jen s parametrem --help.");
					return -1;
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
		
		fprintf(stderr, "Chyba - nepovedlo se otevrit vami zadany soubor. Pro nápovědu spusťte program jen s parametrem --help.");
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
    
    //printf("%s\n", vecOfMessages[0].addressess.c_str());
    

    fclose(f);		// soubor uz nebude potreba, muzeme zavrit
    free(buffer);	// buffer jiz take nebude potreba, vycistime pamet
    
	return vecOfMessages;
	
}

int connectToServer(params *params) {

	string hostServer = "localhost";
	int sock;
	struct sockaddr_in server;
    struct hostent *hp;
    
    
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1) {
		
		fprintf(stderr, "Nepodarilo se vytvorit socket.");
    	return(-1);
    }

	/*=====Verify host=====*/

    //server.sin_family = AF_INET;
    hp = gethostbyname(hostServer.c_str()); // parametr - adresa serveru

    if(hp) {

    	bzero(&server, sizeof(server));
    	server.sin_family   = AF_INET;
    	server.sin_port     = htons(params->port);
    	bcopy(hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
    	//memcpy(&(server.sin_addr), hp->h_addr, hp->h_length);
    }
    else {
    	
    	fprintf(stderr, "%s: unknown host\n", hostServer.c_str());
      	return(-1);
    }
    
    if(connect(sock, (struct sockaddr*) &server, sizeof(server)) == -1) {
    	fprintf(stderr, "Nepodarilo se pripojit");
        return -1;
    }

	return sock;
}

void sendMessage(int sock, string text, int context) {

	if(context != 0)	// chceme ziskat welcome message od serveru
		write(sock, text.c_str(), text.size());
}

string recvMessage(int sock) {

	unsigned char c;
	string buffer("");
	int b;

	while(1) {
		
		if(recv(sock, &c, sizeof(unsigned char), 0) < 1)
			break;
		else if(c == '\n')
			break;
		else if(c != '\r')
			buffer +=c;
	}

	return buffer;	
}


int main(int argc, char **argv) {

	struct params params;
	messageVec vecOfMessages;
	string from = "xkalou03@isa.local";
	int sock;
	string buffer("");
	char c;


	initParams(&params);
	if(checkParameters(argc, argv, &params) == -1)
		return -1;

	printf("%s %d %s %d\n", (params.ip).c_str(), params.port, (params.file).c_str(), params.seconds);


	vecOfMessages = checkFile(params.file);	// ziskani vektoru se zpravami a adresy

	// connecting to smtp server and creating socket

	if((sock = connectToServer(&params)) == -1) {
		return -1;
	}

	/* Finally - communication with server */
	
	sendMessage(sock, "", 0);	// ziskani welcome message
	//printf("%s\n", recvMessage(sock).c_str());
	
	sendMessage(sock, "EHLO " + from + "\r\n", 1);	// uvodni zprava - poslani pozdravu EHLO s adresou klienta
	
	while((buffer = recvMessage(sock).c_str()) != "") {	
		
		//printf("%s\n", buffer.c_str());
		
		if(buffer.find("250 ") != string::npos) { // cteme tak dlouho, dokud nenajdeme posledni radek, 
												  // ktery musi zacinat kodem a za nim hned mezera, ostatni radky maji pomlcku za kodem
            break;
        }
	}
	
	// kdo bude emaily posilat
	
	//printf("%s\n", buffer.c_str());
	//printf("%s\n", vecOfMessages[0].addressess.c_str());
	
	// komu budeme data posilat
	int i = 0;
	int pos = 0;
	string temp;
	string delimiter = ",";
	
	while(i < vecOfMessages.size()) {
	
		sendMessage(sock, "MAIL FROM: <" + from + ">\r\n", 1);
	
		// prijmuti odpovedi
		buffer = recvMessage(sock);	
		printf("%s\n", buffer.c_str());
	
		while((pos = vecOfMessages[i].addressess.find(delimiter)) != string::npos) {
		
			temp = vecOfMessages[i].addressess.substr(0, pos);
			
		  	sendMessage(sock, "RCPT TO: <" + temp + ">\r\n", 1);
			buffer = recvMessage(sock);	
			printf("%s\n", buffer.c_str());
			
			vecOfMessages[i].addressess.erase(0, pos + delimiter.length());
		}
		
		sendMessage(sock, "RCPT TO: <" + vecOfMessages[i].addressess + ">\r\n", 1);
		buffer = recvMessage(sock);	
		printf("%s\n", buffer.c_str());
		
		sendMessage(sock, "DATA\r\n", 1);
		buffer = recvMessage(sock);	
		printf("%s\n", buffer.c_str());
		
		sendMessage(sock, vecOfMessages[i].content + "\r\n", 1);
		sendMessage(sock, ".\r\n", 1);
		buffer = recvMessage(sock);	
		printf("%s\n", buffer.c_str());
		
		i++;
	}
	
	sendMessage(sock, "QUIT\r\n", 1);
	buffer = recvMessage(sock);	
		printf("%s\n", buffer.c_str());
	
	return 0;
}






