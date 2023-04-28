/*
RCP does not define the format of messages. For this reason, we define a message struct here.
*/

// message from client to server
struct request_message{
    int client_id;
    int seq_number;
    int type; // 0 for init, 1 for idle, 2 for get, 3 for put, 4 for close
    int buf[2]; // other data like arguments
};

// message from server to client
struct response_message{
    int client_id;
    int seq_number;
    int type; // 0 for ack, 1 for result
    int result; // result of get or put. 0 if ack or idle
};