all: aws.c client.c serverA.c serverB.c serverC.c
	g++ -o aws aws.c
	g++ -o client client.c
	g++ -o serverA serverA.c
	g++ -o serverB serverB.c
	g++ -o serverC serverC.c

aws:
	g++ -std=c++11 -ggdb -o aws aws.cpp
aws.o:aws.c
	g++ -g -c -Wall aws.c
client:
	g++ -std=c++11 -ggdb -o client client.cpp
serverA:
	g++ -std=c++11 -ggdb -o serverA serverA.cpp
serverB:
	g++ -std=c++11 -ggdb -o serverB serverB.cpp
serverC:
	g++ -std=c++11 -ggdb -o serverC serverC.cpp
clean:
	$(RM) aws
	$(RM) client
	$(RM) serverA
	$(RM) serverB
	$(RM) serverC
	$(RM) *.o
