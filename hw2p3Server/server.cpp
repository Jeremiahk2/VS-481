#include <zmq.hpp>
#include <string>
#include <iostream>
#include <list>
#define MESSAGE_LIMIT 1024

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>

#define sleep(n)	Sleep(n)
#endif

struct Client{
    int iteration = 1;
    int id = -1;
};


int main() {
    //  Prepare our context and socket
    zmq::context_t context(2);
    zmq::socket_t socket(context, zmq::socket_type::dealer);
    socket.bind("tcp://*:5555");

    int iterations = 0;
    int numClients = 0;
    while (true) { 
        std::list<Client> clients;
        //  Wait for next request from client
        zmq::message_t reply;
        zmq::recv_result_t received(socket.recv(reply, zmq::recv_flags::dontwait));
        if ( ( received.has_value() && ( EAGAIN != received.value() ) ) ) {
            Client newClient;
            newClient.id = numClients++;
            clients.push_front(newClient);
            std::cout << "RAN\n";
        }
        char rtnString[MESSAGE_LIMIT] = "";
        for (Client i : clients) {
            char clientString[MESSAGE_LIMIT];
            sprintf_s(clientString, "Client %d: Iteration %d\n", i.id, i.iteration);
            strcat_s(rtnString, clientString);
            i.iteration++;
        }
        
        //Slow down for readability

        //  Send reply back to client
        if (numClients > 0) {
            zmq::message_t message;
            memcpy(message.data(), rtnString, strlen(rtnString) + 1);
            socket.send(message, zmq::send_flags::none);
        }
    }
    return 0;
}