/*
RCP does not define the format of messages. For this reason, we define a message struct here.
*/

// message from client to server
struct request_message{
    int seq_number;
    int client_id;
    int type; // 1 for idle, 2 for get, 3 for put
    int buf[2]; // other data like arguments
};

// message from server to client
struct response_message{
    int seq_number;
    int client_id;
    int type; // 0 for ack, 1 for result
    int result; // result of idle, get, or put
};