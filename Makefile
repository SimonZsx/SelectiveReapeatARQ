all: tclient tserver

tclient: test-client.c rdt.h
	g++ test-client.c -o tclient -Wall

tserver: test-server.c rdt.h
	g++ test-server.c -o tserver -Wall

clean:
	rm -f tclient tserver
