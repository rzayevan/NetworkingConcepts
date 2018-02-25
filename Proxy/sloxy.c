/*
 * Sloxy.c
 *  This program is a slow HTTP proxy which receives the HTTP Requests from the web browser
 *  and forward it to the desired web server in chunks. After receiving the response from the web server
 *  it, it requests for the next range of bytes. At the end, it forwards the full requested message it to the web browser of the client.
 *  Some of the code has been obtainied from http://pages.cpsc.ucalgary.ca/~carey/CPSC441/tutorials/proxy.c.txt
 *  The proxy listens on port 8080
 *  Created on: Jan 26, 2018
 */


/* Standard libraries */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

/* Libraries for socket programming */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* Library for parsing strings */
#include <string.h>
#include <strings.h>

/* h_addr?! */
#include <netdb.h>

/* Clean exit! */
#include <signal.h>
#include <unistd.h>

int listening_socket;

/* The function will run after catching Ctrl+c in terminal */
void catcher(int sig) {
	close(listening_socket);
	printf("catcher with signal  %d\n", sig);
	exit(0);

}

int main() {

	/* For catching Crtl+c in terminal */
	signal(SIGINT, catcher);
	int listening_port = 8888;

	/* Initializing the Address */
	struct sockaddr_in proxy_addr;
	proxy_addr.sin_family = AF_INET;
	proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	proxy_addr.sin_port = htons(listening_port);
	printf("Address Initialization: done.\n");

	/* Creating the listening socket for proxy */
	listening_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listening_socket < 0) {
		printf("\nError in socket() call.\n");
		exit(-1);
	} else {
		printf("Listening Socket creation: done.\n");
	}

	/* Binding the socket to address and port */
	int bind_status;
	bind_status = bind(listening_socket, (struct sockaddr *) &proxy_addr,
			sizeof(struct sockaddr_in));
	if (bind_status < 0) {
		printf("\nError in bind() call.\n");
		exit(-1);
	} else {
		printf("Binding: done.\n");
	}

	/* Listening on binded port number */
	int listening_status;
	listening_status = listen(listening_socket, 10);
	if (listening_status < 0) {
		printf("\nError in listen() call.\n");
		exit(-1);
	}
	/* Infinite while loop for listening accepting connection requests */
	while (1) {
		/* Accepting connection requests */
		int data_socket;
		data_socket = accept(listening_socket, NULL, NULL);
		if (data_socket < 0) {
			printf("\nError in accept() call");
			exit(-1);
		} else {
			printf("\nAccepting connection request: done.\n");
		}

		/* Receiving HTTP message from the client */
		char messageIN[1024];
		char messageOUT[100000];
		int recieve_status;
		recieve_status = recv(data_socket, messageIN, sizeof(messageIN), 0);
		if (recieve_status < 0) {
			printf("\nError in recv() call for client recv message\n");
			exit(-1);
		} else {
			printf("\n");
			printf("<<<<< TTP message received from the client >>>>>\n ");
			printf("\n");
		}

		/* Preserving the HTTP request for sending it to the web server later */
		strcpy(messageOUT, messageIN);

		/* Parsing the HTTP message to extract the HOST name of the desired web server */
		char host[1024];
		char URL[1024];
		char PATH[1024];
		int i;

		printf("\n////////////////////////////////////\n");
		printf("MessageIN received: \n%s",messageIN);
		printf("////////////////////////////////////\n");
		// find and parse the GET request, isolating the URL for later use
		char *pathname = strtok(messageIN, "\r\n");
		printf("\nThe top secret request is: %s\n", pathname);
		if (sscanf(pathname, "GET http://%s", URL) == 1)
			printf("\nURL = %s\n", URL);

		// seperate the hostname from the pathname
		for (i = 0; i < strlen(URL); i++) {
			if (URL[i] == '/') {
				strncpy(host, URL, i); //copy out the hostname
				host[i] = '\0';
				break;
			}
		}

		bzero(PATH, 500); // clear junk at beginning of buffer
		for (; i < strlen(URL); i++) {
			strcat(PATH, &URL[i]); //copy out the path
			break;
		}
		printf("\n******************************\n");
		printf("The HOST is: %s\n", host); //firstHalf is the hostname
		printf("The PATH is: %s\n", PATH); //secondHalf is the path
		printf("******************************\n");


		/* Checking the PATH for HTML file*/
    bool isHTML = false;
    if(strstr(PATH, ".html") != NULL){
      isHTML = true;
      printf("\n++++++++++++++ HTML File Detected! Sloxy going into action . . . +++++++++++++++++++\n");
    } else{
			printf("\n+++++++++++++++++++ Not an HTML file! Sloxy deactivated . . . +++++++++++++++++++++++\n");
		}

		/* Creating the TCP socket for connecting to the desired web server */
		// Address initialization
		struct sockaddr_in server_address;
		struct hostent *server;

		// Getting web server's Address by its host name
		server = gethostbyname(host);
		if (server == NULL){
			printf("\nError in gethostbyname() call.\n");
		} else{
			printf("\nWeb server = %s\n", server->h_name);
		}

		// Initialize socket structure
		bzero((char *) &server_address, sizeof(server_address));
		server_address.sin_family = AF_INET;
		bcopy((char *) server->h_addr, (char *) &server_address.sin_addr.s_addr, server->h_length);
		server_address.sin_port = htons(80);

		// Creating the socket
		int web_socket = socket(AF_INET, SOCK_STREAM, 0);
		if (web_socket < 0) {
			printf(
					"Error in socket() call for creating --proxy-WebServer-- socket.\n");
		} else {
			printf("\n");
			printf("<<<<< --proxy-WebServer-- socket creation: done >>>>>\n");
			printf("\n");
		}

		// Connecting to the web server's socket
		int connect_status;
		connect_status = connect(web_socket, (struct sockaddr *) &server_address,
				sizeof(server_address));
		if (connect_status < 0) {
			printf(
					"Error in connect() call for connecting to the web server's socket.\n");
			exit(-1);
		} else {
			printf("\nWeb server's socket connection establishment: done\n ");
		}

		/* HEAD message creation logic; Also includes the creation of string to be used for range request*/
		char headMsg[1024]= "HEAD ";
		char rangeMsg[1024]= "GET ";
		char* headptr= &headMsg[0];
		char* httpVersion = " HTTP/1.1\r\nHost: ";
		char* path_pointer = &PATH[0];
		char* url_pointer = &URL[0];
		char* host_pointer = &host[0];
		char* terminator = "\r\n\r\n";
		char* short_terminator = "\r\n";

		strcat(headMsg, path_pointer);
		strcat(headMsg, httpVersion);
		strcat(headMsg, host_pointer);
		strcat(headMsg, terminator);
		printf("Head message request is : %s\n", headMsg);

		//Forr range requests
		strcat(rangeMsg, "http://");
		strcat(rangeMsg, url_pointer);
		strcat(rangeMsg, httpVersion);
		strcat(rangeMsg, host_pointer);
		strcat(rangeMsg, short_terminator);

		/* Sending the HEAD request of the client to the web server */
		int head_send_status;
		printf("-> Message being sent to server (headMsg): \n%s\n", headMsg);
		head_send_status = send(web_socket, headMsg, sizeof(headMsg), 0);
		if (head_send_status < 0) {
			printf(
					" Error in send() call for sending HTTP request to the web server.\n ");
			exit(-1);
		} else {
			printf("\n");
			printf("<<<<< Sending HTTP request to the server: done >>>>>\n");
			printf("\n");
		}

		/* Receiving the  HEAD response from the web server */
		char h_message_in[10000];
		int head_recv_status;
		head_recv_status = recv(web_socket, h_message_in, sizeof(h_message_in), 0);
		if (head_recv_status < 0) {
			printf(
					" Error in recv() call for receiving web server's HTTP response.\n ");
			exit(-1);
		} else {
			printf("\n");
			printf("<<<<< Receiving web server's HTTP response: done >>>>>\n");
			printf("-> Recieved HEAD message from server is: \n%s\n ", h_message_in);
			printf("\n");
		}

		/* Parsing the data */
		bool isValidCode = false;
		int request_number, request_number2;
		request_number = 0;
		request_number2 = 0;
		int number_status = sscanf(h_message_in, "HTTP/1.1 %d", &request_number);
		if (number_status < 0){
      printf("Error occured when parsing the request number\n");
			exit(-1);
		} else{
      printf("HTTP Request Number is: %d\n", request_number);
    }
		int number_status2 = sscanf(h_message_in, "HTTP/1.0 %d", request_number2);
		if (number_status2 < 0){
      printf("Error occured when parsing the request number\n");
			exit(-1);
		} else{
      printf("HTTP Request Number is: %d\n", request_number2);
    }

		/* Checking for HTTP Codes*/
		if((request_number == 200) || (request_number2==200)){
			printf("Request 200 recevied.\n");
			isValidCode = true;
		}
		if((request_number == 206) || (request_number2==206)){
			printf("Request 206 recevied.\n");
			isValidCode = true;
		}
		if((request_number == 304) || (request_number2==304)){
			printf("Request 304 recevied.\n");
			isValidCode = true;
		}

		if((request_number == 400)|| (request_number2==400)){
			printf("Request 400 recevied.\n");
		}

		/* Check if web server accepts ranges */
		bool acceptRanges = true;
		if (strstr(h_message_in, "Accept-Ranges: bytes") == NULL){
			printf("\n**Web server does not accept ranges! Sloxy deactivated.**\n\n");
			acceptRanges = false;
			isHTML = false;
		}

		/* Determining content legth */
		char *p;
		int content_length;
	 	p = strstr(h_message_in, "Content-Length: ");
		p = strtok(p, "\r\n");
		sscanf(p, "Content-Length: %d", &content_length);

		char clientMsg[10000];
		char req_message_in[10000];


		/* Range request logic */
		int send_range = 50;
		int low = 0;
		int high = send_range - 1;
		char lowStr[1024];
		char highStr[1024];
		char contStr[1024];
		char rangeReq[1024];

		int info_socket;

/* If request is HTML, this statement gets executed */
if(isHTML && acceptRanges && isValidCode){
	while(low <=content_length){

			sprintf(lowStr, "%d", low);
			sprintf(contStr, "%d", content_length);
			sprintf(highStr, "%d", high);
			strcpy(rangeReq, rangeMsg);
			strcat(rangeReq, "Range: bytes=");
			strcat(rangeReq, lowStr);
			strcat(rangeReq, "-");
			strcat(rangeReq, highStr);
			strcat(rangeReq, terminator);
			low = low + send_range;
			if ((high + send_range ) > content_length){
				high = content_length;
			} else{
				high = high + send_range;
			}

		// Creating the socket
	  info_socket = socket(AF_INET, SOCK_STREAM, 0);
		if (info_socket < 0) {
		printf(
				"Error in socket() call for creating --proxy-WebServer-- socket.\n");
		} else {
		printf("\n");
		printf("<<<<< --proxy-WebServer-- socket creation: done >>>>>\n");
		printf("\n");
		}

		// Connecting to the web server's socket
		int connect2_status;
		connect2_status = connect(info_socket, (struct sockaddr *) &server_address,
			sizeof(server_address));
		if (connect2_status < 0) {
		printf(
				"Error in connect() call for connecting to the web server's socket.\n");
		exit(-1);
		} else {
		printf("\nWeb server's socket connection establishment: done\n ");
		}

		/* Sending the HTTP request of the client to the web server*/
		int web_send_status;
		printf("-> Message being sent to server (rangeReq): \n%s\n", rangeReq);
		web_send_status = send(info_socket, rangeReq, sizeof(rangeReq),0);

		if (web_send_status < 0) {
			printf(
					" Error in send() call for sending HTTP request to the web server.\n ");
			exit(-1);
		} else {
			printf("\n");
			printf("<<<<< Sending HTTP request to the server: done >>>>>\n");
			printf("\n");
		}

		/* Receiving the HTTP response from the web server*/
		int web_recv_status;
		web_recv_status = recv(info_socket, req_message_in, sizeof(req_message_in), 0);
		if (web_recv_status < 0) {
			printf(
					" Error in recv() call for receiving web server's HTTP response.\n ");
			exit(-1);
		} else {
			printf("\n");
			printf("<<<<< Receiving web server's HTTP response: done >>>>>\n");
			printf("-> Recieved message from server is: \n%s\n ", req_message_in);
			printf("\n");
		}
	}

} else {	/* if file is not HTML, goes here */
	// Creating the socket
	info_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (info_socket < 0) {
	printf(
			"Error in socket() call for creating --proxy-WebServer-- socket.\n");
	} else {
	printf("\n");
	printf("<<<<< --proxy-WebServer-- socket creation: done >>>>>\n");
	printf("\n");
	}

	// Connecting to the web server's socket
	int connect2_status;
	connect2_status = connect(info_socket, (struct sockaddr *) &server_address,
		sizeof(server_address));
	if (connect2_status < 0) {
	printf(
			"Error in connect() call for connecting to the web server's socket.\n");
	exit(-1);
	} else {
	printf("\nWeb server's socket connection establishment: done\n ");
	}

	/* Sending the HTTP request of the client to the web server*/
	int web_send_status;
	printf("-> Message being sent to server (messageOUT): \n%s\n", messageOUT);
	web_send_status = send(info_socket, messageOUT, sizeof(messageOUT),0);

	if (web_send_status < 0) {
		printf(
				" Error in send() call for sending HTTP request to the web server.\n ");
		exit(-1);
	} else {
		printf("\n");
		printf("<<<<< Sending HTTP request to the server: done >>>>>\n");
		printf("\n");
	}

	/* Receiving the HTTP response from the web server*/
	int web_recv_status;
	web_recv_status = recv(info_socket, req_message_in, sizeof(req_message_in), 0);
	if (web_recv_status < 0) {
		printf(
				" Error in recv() call for receiving web server's HTTP response.\n ");
		exit(-1);
	} else {
		printf("\n");
		printf("<<<<< Receiving web server's HTTP response: done >>>>>\n");
		printf("-> Recieved message from server is: \n%s\n ", req_message_in);
		printf("\n");
	}
}

//==============================================================================
	// Creating the socket
	int user_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (user_socket < 0) {
	printf(
			"Error in socket() call for creating --proxy-WebServer-- socket.\n");
	} else {
	printf("\n");
	printf("<<<<< --proxy-WebServer-- socket creation: done >>>>>\n");
	printf("\n");
	}

	// Connecting to the web server's socket
	int connect3_status;
	connect3_status = connect(user_socket, (struct sockaddr *) &server_address,
		sizeof(server_address));
	if (connect3_status < 0) {
	printf(
			"Error in connect() call for connecting to the web server's socket.\n");
	exit(-1);
	} else {
	printf("\nWeb server's socket connection establishment: done\n ");
	}
	/* Sending the HTTP request of the client to the web server*/
	int user_send_status;
	printf("-> Message being sent to server (messageOUT): \n%s\n", messageOUT);
	user_send_status = send(user_socket, messageOUT, sizeof(messageOUT),0);

	if (user_send_status < 0) {
		printf(
				" Error in send() call for sending HTTP request to the web server.\n ");
		exit(-1);
	} else {
		printf("\n");
		printf("<<<<< Sending HTTP request to the server: done >>>>>\n");
		printf("\n");
	}

	char user_message_in[100000];
	/* Receiving the HTTP response from the web server*/
	int user_recv_status;
	user_recv_status = recv(user_socket, user_message_in, sizeof(user_message_in), 0);
	if (user_recv_status < 0) {
		printf(
				" Error in recv() call for receiving web server's HTTP response.\n ");
		exit(-1);
	} else {
		printf("\n");
		printf("<<<<< Receiving web server's HTTP response: done >>>>>\n");
		printf("-> Recieved message from server is: \n%s\n ", req_message_in);
		printf("\n");
	}

	//==================== Updating the user =====================================
	/* Closing the socket connection with the web server */
	close(info_socket);
	close(user_socket);
	close(web_socket);
	/* Sending the HTTP response to the client */
	int c_send_status;
	c_send_status = send(data_socket, user_message_in, sizeof(user_message_in), 0);
//	c_send_status = send(data_socket, h_message_in, sizeof(h_message_in), 0);
	if (c_send_status < 0) {
		printf(
				"Error in send() call for sending HTTP response to the client.\n");
		exit(-1);
	} else {
		printf("\n");
		printf("<<<<< Sending HTTP response to the client: done >>>>>\n");
		printf("\n");
	}
		/* Closing the socket connection with the client */
		close(data_socket);
		printf("\n !! data socket is closed !!\n");
	}

	close(listening_socket);
	printf("\n !! listening_socket is closed !!\n");
	return 0;

}
