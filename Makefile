all:
	g++ -std=c++11 -ggdb -o aws aws.cpp
	g++ -std=c++11 -ggdb -o client client.cpp
	g++ -std=c++11 -ggdb -o serverA serverA.cpp
	g++ -std=c++11 -ggdb -o serverB serverB.cpp
	g++ -std=c++11 -ggdb -o serverC serverC.cpp
.PHONY: aws
aws:
	./aws
client:
	g++ -std=c++11 -ggdb -o client client.cpp
.PHONY: serverA
serverA:
	./serverA
.PHONY: serverB
serverB:
	./serverB
.PHONY: serverC
serverC:
	./serverC
clean:
	$(RM) aws
	$(RM) client
	$(RM) serverA
	$(RM) serverB
	$(RM) serverC
	$(RM) *.o
