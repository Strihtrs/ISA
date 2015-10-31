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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using std::string;


struct params {

	string ip;
	int port;
	string file;
	int seconds;
};


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


int main(int argc, char **argv) {

	struct params params;

	initParams(&params);
	if(checkParameters(argc, argv, &params) == -1)
		return -1;

	printf("%s %d %s %d\n", (params.ip).c_str(), params.port, (params.file).c_str(), params.seconds);




	int socket;
	struct sockaddr_in server;

	return 0;
}