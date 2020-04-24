# EE450 Spring 2020 Socket Programming Project

Albert Chan
8764674229

## What I have done

I implemented 5 components: 3 backend servers(2 database-like servers and a computing server), a main server (AWS) that is the link for processing client requests and the 3 backend servers, and a client program for a user to interface with the whole system.

## Code files

aws.cpp: The main server that communicates with the client and 3 backend servers. It boots up setting up its UDP and TCP sockets waiting for a request from client via TCP. It then takes the requests map id and queries server A if it contains it, if it doesn't then it will query server B. If neither has the map id, it will send an appropriate error to client. Otherwise it will receive map info that it will use to check for the requested vertices, send an error to client if it can't find the vertices. Otherwise it will forward the user request query and map data to server C. It will wait for server C to send back the calculations, then forward back to client.

client.cpp: Program allowing a user to interface with the system. Boots up and places the command line arguments into a struct, sets up a tcp socket to forward request to AWS. Prints the response from AWS whether it is an error or the results of a successful request.

serverA.cpp and serverB.cpp: Starts up reading their respective map file to build an array of map ids and strings of the map data to be parsed by AWS and serverC, then sets up UDP port for requests from AWS. Iterates through its array of maps for a matching id and forwards the map data in string form. Sends an error if map id is not found.

serverC.cpp: Sets up UDP port and waits for request from AWS, takes the received string and parses to build a map and retrieve user query. Uses source and destination vertice to run Dijkstra to find shortest path, store the path (each hop) into an array, calculate delays, store all those results into a struct to return to AWS.

## Message format

memset() and memcpy(), for sending data as structs:
client to AWS
```
struct InputArguments
{
	int			map_id;
	int			source_vertex;
	int			dest_vertex;
	int			file_size;
};
```
AWS to client, for successful result data
server C to AWS
```
struct CalculationResults
{
    int         path[11];
    double      distance;
    double      tran_delay;
    double      prop_delay;
    double      total_delay;
};
```


string to char[] buffer, strings containing the map data:
AWS to client, for error
```
"No map"
"no source"
"no destination"
```
AWS to server A and server B
```
"A"
```
server A and server B to AWS, both successful map data and error; aws to server C
```
"(<source vertex>-<destination vertex>-<file size>-)<mapID>-<propagation speed>-<transmission speed>-<first end vertex> <second end vertex> <distance between the two vertices>,<first end vertex> <second end vertex> <distance between the two vertices>... ... ...<first end vertex> <second end vertex> <distance between the two vertices>"
```

## Idiosyncrasy

I could have improved performance of server A and server B by having another map data structure that holds the map id and the index in the map array to quickly find the index, rather than iterate through the whole index each query which would be return faster results if more maps are added. But as this project is limited to 52 maps, and I didn't realize this until the time writing, I did not implement it yet.

## Reused code

For socket related code I used snippets from the Beej Guide to Networking Programming, code relating to the UDP and TCP sockets have comments.

For graph and Dijkstra's algorithm I used(also commented in serverC.cpp):
https://stackoverflow.com/questions/53184552/dijkstras-algorithm-w-adjacency-list-map-c
https://algorithmsinfinite.blogspot.com/2018/10/dijkstras-algorithm-shortest-path_10.html
https://en.wikipedia.org/wiki/Dijkstra's_algorithm
https://www.geeksforgeeks.org/printing-paths-dijkstras-shortest-path-algorithm/