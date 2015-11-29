tsukasa: tsukasa.c
	gcc tsukasa.c -o tsukasa -Wall -pedantic-errors -std=gnu11 -lwsock32 -lws2_32 -lwininet