/* Projekt - ISA - smtpklient.cpp	*/
/* Klient SMTP pro odesílání pošty	*/
/* Filip Kalous (xkalou03)   		*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <string>

using std::string;


struct params {

	string ip;
	int port;
	string file;
	int seconds;
};


void initParams(struct params *params) {

	params->ip = "127.0.0.1";
	params->port = 25;
	params->file = "";
	params->seconds = 0;
}




int checkParameters(int argc, char **argv, struct params *params) {

	int opt;
	char *error;


	if(argc > 5) {

		fprintf(stderr, "Error - too many parameters!\n");
		return -1;
	}

	else if(argc < 2) {

		fprintf(stderr, "Error - too few parameters!\n");
		return -1;
	}

	while((opt = getopt(argc, argv, "a:p:w:i:"))) {

		switch(opt) {

			case 'a':

				params->ip = optarg;

				break;

			case 'i':

				params->file = optarg;

				break;

			case 'p':

				params->port = strtol(optarg, &error, 10);
				if(*error)
					return -1;

				break;

			case 'w':

				params->seconds = strtol(optarg, &error, 10);
				if(*error)
					return -1;

				break;
		}
	}

	return 0;
}


int main(int argc, char **argv) {

	struct params params;

	initParams(&params);
	checkParameters(argc, argv, &params);

	return 0;
}