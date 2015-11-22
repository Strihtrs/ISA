# ISA Projekt
# Filip Kalous (xkalou03)

CC=g++

stmpklient: smtpklient.cpp
	$(CC) -Wall -Wextra -pedantic smtpklient.cpp -o smtpklient
