//
// Created by gkberkylmz on 14.02.2021.
//
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fstream>
#include <list>
#include <sstream>
#include <bits/stdc++.h>
#include <algorithm>

using namespace std;

/// In this project we are required to simulate a ticket office with thellers and clients, we should represent the seats
/// and handle seat reservations according to instructions given in the description
/// First we need to create 3 tellers and clients by representing as threads. There should exist a function for
/// clients and a function for tellers apart from the main function
/// The tellers are created initially, and they print that they are present, then they wait until all client
/// threads are created. client threads also wait for all other clients to be created before starting to coun for their
/// arrival time. After all tellers and clients are created, clients sleep as long as their arrival time to simulate
/// their entrance. When clients enter to the ticket hall they enroll into a FIFO queue. In this project,
/// I implemented the queue within the client function. When a client arrives it tries to lock the queue mutex
/// and assign itself to a teller. If another client is doing the assignment operation, mutex is locked and
/// following clients wait for the one who uses it. Since mutex handles this accumulation as fifo queue, there is no
/// need to implement a queue structure. Then it assigns its id to the teller's pointer and marks the teller as
/// busy. Then teller fetches the data belonging to the id it is given from a global 2D vector. Assigns a seat
/// if the desired seat no is within the capacity and is empty, client gets it. If no seat is left it recieves none,
/// if the desired one is full but within the bounds and there are other seats, teller assigns the seat with min
/// index. Afterwards, they both sleep and client exits. However, teller gets available for new clients if there
/// are other clients left.



vector<bool> seats;///true at index i if seat i is full
bool busyA=false, busyB=false, busyC=false;///true when a/b/c is busy
int clientofA, clientofB, clientofC;///client id of a/b/c
bool clientsCreated=false;///true when all clients are created
bool clientsEnded=false;///true when there's no client left to be served
bool seatsEnded=false;///true when all seats are full
vector<string>clientIDs;///vector to map string ids to integer ids
vector<vector<int>>clientData;///the input given is stored in the 2D array
int clientServed=0;///number of clients that has been served
int clientCount;///number of total clients
fstream out;///fstream object of output file
int seatCount=0;

pthread_mutex_t printMutex=PTHREAD_MUTEX_INITIALIZER;//mutex lock to handle printing to file from multiple threads
pthread_mutex_t queueMutex=PTHREAD_MUTEX_INITIALIZER;//mutex lock to handle assignment of clients to tellers
pthread_mutex_t reserveMutex=PTHREAD_MUTEX_INITIALIZER;//mutex lock to handle reservation from multiple threads

void* teller(void* arg);
void* client(void* arg);

int main(int argc, char** argv){
    ///input read and variable assignments
    fstream file;///input file's fstream object
    string inputdirectory=argv[1];///input file's directory
    file.open(inputdirectory);
    string outputdirectory=argv[2];///output file's directory
    out.open(outputdirectory, ios_base::out);
    out.close();
    out.open(outputdirectory, ios_base::app);

    string theaterName;///name of the theater hall
    file>>theaterName;
    file>>clientCount;
    clientIDs.resize(clientCount+1);
    clientData.resize(clientCount+1);
    out<<"Welcome to the Sync-Ticket!"<<endl;

    ///reading the hall and arranging seat numbers accordingly
    int totalSeats=-1;//seat number of the hall
    if(theaterName.compare("OdaTiyatrosu")==0)
        totalSeats=60;
    else if(theaterName.compare("UskudarStudyoSahne"))
        totalSeats=80;
    else if(theaterName.compare("KucukSahne"))
        totalSeats=200;
    seats.resize(totalSeats);
    seats[0]=true;

    pthread_t a, b, c;//thread ids for tellers
    string aid="A", bid="B", cid="C";//teller ids

    ///creation of teller threads with a,b,c as pthread_t ids and aid,bid,cid as ids:
    pthread_create(&a, NULL, teller, &aid);
    usleep(1000);
    pthread_create(&b, NULL, teller, &bid);
    usleep(1000);
    pthread_create(&c, NULL, teller, &cid);


    ///creation of client threads and reading their data from file and putting them into a 2D global vector
    pthread_t clientThreadIds[clientCount+1];//pthread_t ids of clients in an array
    string s;
    int temp[clientCount+1];
    for(int i=1; i<=clientCount; i++){
        file>>s;
        replace(s.begin(), s.end(), ',', ' ');
        stringstream splitline(s);
        vector<int> cldata;//that client's data
        cldata.resize(3);
        string clientname;
        splitline>>clientname;
        splitline>>cldata[0]>>cldata[1]>>cldata[2];
        clientIDs[i]=clientname;
        clientData[i]=cldata;
        temp[i]=i;
        pthread_create(&clientThreadIds[i], NULL, client, &temp[i]);
    }
    clientsCreated=true;//triggers the functioning of other threads because they wait for this variable to be true
    for(int i=1; i<=clientCount; i++){
        pthread_join(clientThreadIds[i], NULL);//joining of all clients
    }
    clientsEnded=true;//marks the end of client threads

    ///joining of tellers at the end
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    pthread_join(c, NULL);

    out<<"All clients received service."<<endl;


    pthread_exit(0);
}


///the client program that's planned to run as thread
void* client(void* arg){
    ///data gathering
    int* ptr=(int*)arg;
    int id=ptr[0];
    int arrivalTime=clientData[id][0], serviceTime=clientData[id][1], seatNumber=clientData[id][2];

    while(!clientsCreated);///waits until all clients are created

    usleep(arrivalTime*1000);//waits until its arrival time
    bool* tellerBusy;//pointer of the teller to which the client will be assigned

    pthread_mutex_lock(&queueMutex);//locking queue mutex, critical section begins
    ///if no teller is available waits until one of them gets available
    while(busyA && busyB && busyC);

    ///prefers A over B and B over C if the client has the chance
    if(!busyA){
        busyA=true;
        tellerBusy=&busyA;
        clientofA=id;
    }
    else if(!busyB){
        busyB=true;
        tellerBusy=&busyB;
        clientofB=id;
    }
    else{
        busyC=true;
        tellerBusy=&busyC;
        clientofC=id;
    }
    pthread_mutex_unlock(&queueMutex);//unlock mutex, critical section ends

    ///sleep as much as the service time
    serviceTime*=1000;
    usleep(serviceTime);

    pthread_exit(0);
}

///teller thread
void* teller(void* arg){
    string* ptid=(string*)arg;
    string id=*ptid;
    bool* busyPointer;//pointer of the current teller's availability boolean variable
    int* clientIdPointer;//pointer of the assigned client's id

    pthread_mutex_lock(&printMutex);//lock the print mutex, critical section begins
    out<<"Teller "<<id<<" has arrived."<<endl;
    pthread_mutex_unlock(&printMutex);//unlock the print mutex, critical section ends


    ///assign the pointers according to the id of the teller
    if(id=="A") {
        busyPointer = &busyA;
        clientIdPointer=&clientofA;
    }
    else if(id=="B") {
        busyPointer = &busyB;
        clientIdPointer=&clientofB;
    }
    else {
        busyPointer = &busyC;
        clientIdPointer=&clientofC;
    }

    ///wait until all clients are created
    while(!clientsCreated); //waits idle until client threads are created.

    ///loop to iterate as long as clients are not finished
    while(!clientsEnded){
            pthread_mutex_lock(&queueMutex);//lock the queue mutex, critical section
            ///if teller is not busy go to next iteration of bigger loop
            if(!*busyPointer && !clientsEnded) {//when the teller is not busy and clients didn't finish get in and continue
                pthread_mutex_unlock(&queueMutex);//if it will continue, unlock here and continue in the loop
                continue;
            }
            pthread_mutex_unlock(&queueMutex);//if teller is supposed to be do some work or clients are finished, unlock queue mutex critical section ends

            if (!clientsEnded) {//if clients are not ended
                int clientId = *clientIdPointer;//id of the client
                int serviceTime = clientData[clientId][1] * 1000;//service time of the client
                int desiredSeat = clientData[clientId][2];//the seat desired by the client
                int seatGiven=-2;//seat given to the client, default is -2

                ///assigning the seat to the client
                pthread_mutex_lock(&reserveMutex);//lock reservation mutex, critical section begins
                if(desiredSeat<=seats.size() && !seats[desiredSeat]){
                    seats[desiredSeat]=true;
                    seatGiven=desiredSeat;
                }
                else{
                    for(int i=1; i<=seats.size(); i++){
                        if(!seats[i]){
                            seats[i]=true;
                            seatGiven=i;
                            break;
                        }
                    }
                }
                pthread_mutex_unlock(&reserveMutex);//unlock the reservation mutex, critical section ends

                usleep(serviceTime);//spend the time on service

                ///print the output to file
                pthread_mutex_lock(&printMutex);//lock the print mutex, critical section begins
                out<<clientIDs[clientId]<<" requests seat "<<desiredSeat<<", ";
                if(seatGiven==-2)//no seat is given to the client
                    out<<"reserves None. Signed by Teller "<<id<<"."<<endl;
                else//a seat is given to the client
                    out<<"reserves seat "<<seatGiven<<". Signed by Teller "<<id<<"."<<endl;
                //*busyPointer=false;
                pthread_mutex_unlock(&printMutex);//unlock the print mutex, critical section ends

                *busyPointer=false;//mark itself as unbusy
            }
        }
    pthread_exit(0);
}