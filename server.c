#include "udp.h"

/*
The RPC server should take one command line argument: port
*/
int main(int argc, char *argv[]){
    if (argc != 2) 
    {
        printf("expected usage: ./server port\n");
        exit(1);
    }
    
    int port = atoi(argv[1]);
    
    struct socket s = init_socket(port);

    while(1) {
        //TODO: need to deal with multiple clients, acknowledgements, timeouts, etc.
        struct packet_info packet = receive_packet(s);
    }


}