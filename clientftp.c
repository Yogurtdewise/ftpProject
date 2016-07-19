/* 
 * Client FTP program
 *
 * NOTE: Starting homework #2, add more comments here describing the overall function
 * performed by server ftp program
 * This includes, the list of ftp commands processed by server ftp.
 *
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#define SERVER_FTP_PORT 14227
#define DATA_CONNECTION_PORT  14228

/* Error and OK codes */
#define OK 0
#define ER_INVALID_HOST_NAME -1
#define ER_CREATE_SOCKET_FAILED -2
#define ER_BIND_FAILED -3
#define ER_CONNECT_FAILED -4
#define ER_SEND_FAILED -5
#define ER_RECEIVE_FAILED -6


/* Function prototypes */

int clntConnect(char *serverName, int *s);
int svcInitServer(int *s);
int sendMessage (int s, char *msg, int  msgSize);
int receiveMessage(int s, char *buffer, int  bufferSize, int *msgSize);


/* List of all global variables */

char userCmd[1024];		/* user typed ftp command line read from keyboard */
char userCmdCopy[1024]; /* used for temporary String manipulation. */
char *cmd;  		    /* ftp command (without argument) extracted from userCmd */
char *argument;         /* argument (without ftp command) extracted from userCmd */
char replyMsg[1024];    /* buffer to receive reply message from server */

char ftpData[100];		/* buffer to send/receive file data to/from client */
int  ftpBytes      = 0; /* Used to count the total number of bytes transferred during ftp */
int  fileBytesRead = 0; /* The number of bytes read by fread */
int  bytesReceived = 0; /* The number of bytes received in a single ftp message. */
FILE *filePtr;          /* Used to point to the temporary file that logged command output */


/*
 * main
 *
 * Function connects to the ftp server using clntConnect function.
 * Reads one ftp command in one line from the keyboard into userCmd array.
 * Sends the user command to the server.
 * Receive reply message from the server.
 * On receiving reply to QUIT ftp command from the server,
 * close the control connection socket and exit from main
 *
 * Parameters
 * argc		- Count of number of arguments passed to main (input)
 * argv  	- Array of pointer to input parameters to main (input)
 *		   It is not required to pass any parameter to main
 *		   Can use it if needed.
 *
 * Return status
 *	OK	- Successful execution until QUIT command from client 
 *	N	- Failed status, value of N depends on the function called or cmd processed
 */

int main(	
	int argc,
	char *argv[]
	)
{
	/* List of local varibale */

	int ccSocket;	  /* Control connection socket - to be used in all client communication */
	int dcSocket;     /* Data connection socket - to be used in data transfer to and from server */
	int listensocket; /* Used to listen for connection requests to the dcSocket. */
	int msgSize;	  /* size of the reply message received from the server */
	int status = OK;

	/*
	 * NOTE: without \n at the end of format string in printf,
         * UNIX will buffer (not flush)
	 * output to display and you will not see it on monitor.
 	 */
	printf("Started execution of client ftp\n");


	 /* Connect to client ftp*/
	printf("Calling clntConnect to connect to the server\n");	/* changed text */

	status=clntConnect("255.255.255.255", &ccSocket);  /* Off-campus IP: "REDACTED" */
	if(status != 0)
	{
		printf("Connection to server failed, exiting main. \n");
		return (status);
	}
	
	/*
	 * Listen for a data connection.
	 * Copied and edited from serverftp.c.
	 */
	printf("Initialize client data connection to server (listen).\n");	/* changed text */

	status=svcInitServer(&listensocket);
	if(status != 0)
	{
		printf("Exiting client ftp due to svcInitServer returned error.\n");
		exit(status);
	}


	/* 
	 * Read an ftp command with argument, if any, in one line from user into userCmd.
	 * Copy ftp command part into ftpCmd and the argument into arg array.
 	 * Send the line read (both ftp cmd part and the argument part) in userCmd to server.
	 * Receive reply message from the server.
	 * until quit command is typed by the user.
	 */

	do
	{
		printf("my ftp> ");
		
		/* Re-initialize first element of cmd and argument */
		cmd      = NULL;
		argument = NULL;
		
		/* Get the user's input and copy to userCmd */
		gets(userCmd);
		
		/* Separate command and argument from userCmd */
		strcpy(userCmdCopy, userCmd);
		cmd = strtok(userCmdCopy, " ");
		argument = strtok(NULL, " ");
		
		/* Send the userCmd to the server, unless cmd is 'send' or 'recv'. (Must confirm send file is readable)*/
		if(strcmp(cmd, "send") != 0 && strcmp(cmd, "recv") != 0){
			printf("Sending message on ccSocket.\n");
			status = sendMessage(ccSocket, userCmd, strlen(userCmd)+1);
		}
		if(status != OK)
		{
		    break;
		}
		
		/* Initiate the 'send' procedure. */
		if(strcmp(cmd, "send") == 0){
		
			if(argument[0] == NULL || strcmp(argument, "") == 0){
				printf("File argument not specified. Data connection will not be opened. No command sent.\n");
			}else{
				filePtr = NULL;
				filePtr = fopen(argument, "r");
				/* Ensure the specified file is readable. Do not send command or open data connection if file not readable.*/
				if(filePtr == NULL){
				printf("Could not open specified file. Data connection will not be opened. No command sent.\n");
				}
			
				/* File is readable. Send command to server & establish data connection.*/
				else{
					printf("send: Sending message on ccSocket.\n");
					status = sendMessage(ccSocket, userCmd, strlen(userCmd)+1); /* Send command on command connection. */
					ftpBytes = 0; /* initialize byte count */
					printf("ftp client is waiting to accept data connection.\n");

					/* wait until connection request comes from server ftp */
					dcSocket = accept(listensocket, NULL, NULL);
	
					printf("Came out of dcSocket accept() function \n");
	
					if(dcSocket < 0)
					{
						perror("Cannot accept data connection: Closing listen socket.\n");
						close(dcSocket);  /* close listen socket */
					}
					/* Data connection successful. Begin file transfer. */
					else{
						printf("Data connection established with server. Sending file.\n");
				
						/* Send the specified file data */
						do{
							printf("Top of send do loop. status: %d\n", status); /* TEST */
							fileBytesRead = 0;
							fileBytesRead = fread(ftpData, 1, 100, filePtr); /* Read error returns number of bytes read. Can not distinguish error. */
							printf("Read packet from file complete.\n"); /* TEST */
							status = sendMessage(dcSocket, ftpData, fileBytesRead); /* Do not alter file data. Do not add an element for NULL. */
							printf("Packet sent. sendMessage status: %d\n", status); /* TEST */
							ftpBytes = ftpBytes + fileBytesRead;
							printf("Bottom of do loop.\n"); /* TEST */
						}while(!feof(filePtr) && status == OK); /* End send loop */
					
						/* Print results & close the data connection socket */
						if(status != OK){
							perror("sendMessage returned not OK: Closing data connection.\n");
						}else{
							printf("\nReached EOF. %d bytes sent. Closing data connection.\n\n", ftpBytes);
						}
						fclose(filePtr);
						close(dcSocket);
					}
					printf("send: receiving message on ccSocket.\n"); /* TEST */
					status = receiveMessage(ccSocket, replyMsg, sizeof(replyMsg), &msgSize); /* Message sent to server. Get reply. */
				}
			}
		}
		/* Initiate the 'recv' procedure. */
		else if(strcmp(cmd, "recv") == 0){
			/* Ensure argument is specified and specified file can be written to before sending 'recv' command. */
			filePtr = NULL;
			filePtr = fopen(argument, "w");
			/* Ensure the specified file is writeable */
			if(filePtr == NULL){
				perror("Could not open/create specified file. Data connection will not be opened.\n");
			}
			/* File is writeable. Send 'recv' command and listen for data connection request from server. */
			else{
				printf("Sending recv command to server.\n");
				status = sendMessage(ccSocket, userCmd, strlen(userCmd)+1);
				if(status != OK)
				{
					break;
				}
				printf("ftp client is waiting to accept data connection.\n");
				/* wait until connection request comes from server ftp */
				dcSocket = accept(listensocket, NULL, NULL);
				printf("Came out of dcSocket accept() function \n");
				if(dcSocket < 0){	
					/* Data connection failed. Close the specified file that was to be written and the data connection. */
					perror("Cannot accept data connection: Closing data connection.\n");
					fclose(filePtr);
					close(dcSocket);
				}
				else{
					/* Data connection successful. Receive data from server and write to the specified file. */
					printf("Data connection established with server. Waiting to receive file.\n");
					ftpBytes = 0; /* initialize byte count */
					do{
						bytesReceived = 0;
						status = receiveMessage(dcSocket, ftpData, sizeof(ftpData), &bytesReceived);
						fwrite(ftpData, 1, bytesReceived, filePtr); /* Success or fail, fwrite returns >= 0. This info is useless. */
						ftpBytes = ftpBytes + bytesReceived;
					}while(bytesReceived > 0 && status == OK); /* end data connection read loop */
			
				fclose(filePtr);
				printf("Received %d bytes. Closing data connection.\n\n", ftpBytes);
				close(dcSocket);
				}
				status = receiveMessage(ccSocket, replyMsg, sizeof(replyMsg), &msgSize);
			}
		}

		/* Receive reply message from the the server if expected (Send is not sent if read error occurs client-side. Therefore no reply expected.) */
		if(strcmp(cmd, "send") != 0 && strcmp(cmd, "recv") != 0){
			printf("Receiving message on ccSocket.\n");
			status = receiveMessage(ccSocket, replyMsg, sizeof(replyMsg), &msgSize);
		}
		if(status != OK)
		{
		    break;
		}
		printf("Bottom of main do loop.\n");
	}
	while (strcmp(userCmd, "quit") != 0);
	
	/*
	 * Note: No response message is sent to serverftp, since serverftp
	 * may be reading ASCII data during a 'send' operation.
	 */
	
	printf("Closing data connection. \n");
	close(listensocket);  /* close control connection socket */

	printf("Closing control connection. \n");
	close(ccSocket);  /* close control connection socket */

	printf("Exiting client main. \n");

	return (status);

}  /* end main() */


/*
 * clntConnect
 *
 * Function to create a socket, bind local client IP address and port to the socket
 * and connect to the server
 *
 * Parameters
 * serverName	- IP address of server in dot notation (input)
 * s		- Control connection socket number (output)
 *
 * Return status
 *	OK			- Successfully connected to the server
 *	ER_INVALID_HOST_NAME	- Invalid server name
 *	ER_CREATE_SOCKET_FAILED	- Cannot create socket
 *	ER_BIND_FAILED		- bind failed
 *	ER_CONNECT_FAILED	- connect failed
 */


int clntConnect (
	char *serverName, /* server IP address in dot notation (input) */
	int *s 		  /* control connection socket number (output) */
	)
{
	int sock;	/* local variable to keep socket number */

	struct sockaddr_in clientAddress;  	/* local client IP address */
	struct sockaddr_in serverAddress;	/* server IP address */
	struct hostent	   *serverIPstructure;	/* host entry having server IP address in binary */


	/* Get IP address os server in binary from server name (IP in dot natation) */
	if((serverIPstructure = gethostbyname(serverName)) == NULL)
	{
		printf("%s is unknown server. \n", serverName);
		return (ER_INVALID_HOST_NAME);  /* error return */
	}

	/* Create control connection socket */
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("cannot create socket ");
		return (ER_CREATE_SOCKET_FAILED);	/* error return */
	}

	/* initialize client address structure memory to zero */
	memset((char *) &clientAddress, 0, sizeof(clientAddress));

	/* Set local client IP address, and port in the address structure */
	clientAddress.sin_family = AF_INET;	/* Internet protocol family */
	clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);  /* INADDR_ANY is 0, which means */
						 /* let the system fill client IP address */
	clientAddress.sin_port = 0;  /* With port set to 0, system will allocate a free port */
			  /* from 1024 to (64K -1) */

	/* Associate the socket with local client IP address and port */
	if(bind(sock,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
	{
		perror("cannot bind");
		close(sock);
		return(ER_BIND_FAILED);	/* bind failed */
	}


	/* Initialize serverAddress memory to 0 */
	memset((char *) &serverAddress, 0, sizeof(serverAddress));

	/* Set ftp server ftp address in serverAddress */
	serverAddress.sin_family = AF_INET;
	memcpy((char *) &serverAddress.sin_addr, serverIPstructure->h_addr, 
			serverIPstructure->h_length);
	serverAddress.sin_port = htons(SERVER_FTP_PORT);

	/* Connect to the server */
	if (connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
	{
		perror("Cannot connect to server ");
		close (sock); 	/* close the control connection socket */
		return(ER_CONNECT_FAILED);  	/* error return */
	}


	/* Store listen socket number to be returned in output parameter 's' */
	*s=sock;

	return(OK); /* successful return */
}  // end of clntConnect() */



/*
 * svcInitServer
 *
 * Copied and edited from serverftp.c.
 *
 * Function to create a socket and to listen for connection request from server
 *    using the created listen socket.
 *
 * Parameters
 * s		- Socket to listen for connection request (output)
 *
 * Return status
 *	OK			- Successfully created listen socket and listening
 *	ER_CREATE_SOCKET_FAILED	- socket creation failed
 */

int svcInitServer (
	int *s 		/*Listen socket number returned from this function */
	)
{
	int sock;
	struct sockaddr_in svcAddr;
	int qlen;

	/*create a socket endpoint */
	if( (sock=socket(AF_INET, SOCK_STREAM,0)) <0)
	{
		perror("cannot create socket");
		return(ER_CREATE_SOCKET_FAILED);
	}

	/*initialize memory of svcAddr structure to zero. */
	memset((char *)&svcAddr,0, sizeof(svcAddr));

	/* initialize svcAddr to have client IP address and client listen port#. */
	svcAddr.sin_family = AF_INET;
	svcAddr.sin_addr.s_addr=htonl(INADDR_ANY);  	 /* client IP address */
	svcAddr.sin_port=htons(DATA_CONNECTION_PORT);    /* client listen port # */

	/* bind (associate) the listen socket number with server IP and port#.
	 * bind is a socket interface function 
	 */
	if(bind(sock,(struct sockaddr *)&svcAddr,sizeof(svcAddr))<0)
	{
		perror("cannot bind");
		close(sock);
		return(ER_BIND_FAILED);	/* bind failed */
	}

	/* 
	 * Set listen queue length to 1 outstanding connection request.
	 * This allows 1 outstanding connect request from server to wait
	 * while processing current connection request, which takes time.
	 * It prevents connection request to fail and server to think client is down
	 * when in fact client is running and busy processing connection request.
	 */
	qlen=1; 


	/* 
	 * Listen for connection request to come from server ftp.
	 * This is a non-blocking socket interface function call, 
	 * meaning, client ftp execution does not block by the 'listen' function call.
	 * Call returns right away so that client can do whatever it wants.
	 * The TCP transport layer will continuously listen for request and
	 * accept it on behalf of client ftp when the connection requests comes.
	 */

	listen(sock,qlen);  /* socket interface function call */

	/* Store listen socket number to be returned in output parameter 's' */
	*s=sock;

	return(OK); /*successful return */
}

/*
 * sendMessage
 *
 * Function to send specified number of octet (bytes) to client ftp
 *
 * Parameters
 * s		- Socket to be used to send msg to client (input)
 * msg  	- Pointer to character arrary containing msg to be sent (input)
 * msgSize	- Number of bytes, including NULL, in the msg to be sent to client (input)
 *
 * Return status
 *	OK		- Msg successfully sent
 *	ER_SEND_FAILED	- Sending msg failed
 */

int sendMessage(
	int s, 		/* socket to be used to send msg to client */
	char *msg, 	/*buffer having the message data */
	int msgSize 	/*size of the message/data in bytes */
	)
{
	int i;


	/* Print the message to be sent byte by byte as character */
	for(i=0;i<msgSize;i++)
	{
		printf("%c",msg[i]);
	}
	printf("\n");

	if((send(s,msg,msgSize,0)) < 0) /* socket interface call to transmit */
	{
		perror("unable to send ");
		return(ER_SEND_FAILED);
	}

	return(OK); /* successful send */
}


/*
 * receiveMessage
 *
 * Function to receive message from client ftp
 *
 * Parameters
 * s		- Socket to be used to receive msg from client (input)
 * buffer  	- Pointer to character arrary to store received msg (input/output)
 * bufferSize	- Maximum size of the array, "buffer" in octent/byte (input)
 *		    This is the maximum number of bytes that will be stored in buffer
 * msgSize	- Actual # of bytes received and stored in buffer in octet/byes (output)
 *
 * Return status
 *	OK			- Msg successfully received
 *	ER_RECEIVE_FAILED	- Receiving msg failed
 */

int receiveMessage (
	int s, 		/* socket */
	char *buffer, 	/* buffer to store received msg */
	int bufferSize, /* how large the buffer is in octet */
	int *msgSize 	/* size of the received msg in octet */
	)
{
	int i;

	*msgSize=recv(s,buffer,bufferSize,0); /* socket interface call to receive msg */

	if(*msgSize<0)
	{
		perror("unable to receive");
		return(ER_RECEIVE_FAILED);
	}

	/* Print the received msg byte by byte */
	for(i=0;i<*msgSize;i++)
	{
		printf("%c", buffer[i]);
	}
	printf("\n");

	return (OK);
}


/*
 * clntExtractReplyCode
 *
 * Function to extract the three digit reply code 
 * from the server reply message received.
 * It is assumed that the reply message string is of the following format
 *      ddd  text
 * where ddd is the three digit reply code followed by or or more space.
 *
 * Parameters
 *	buffer	  - Pointer to an array containing the reply message (input)
 *	replyCode - reply code number (output)
 *
 * Return status
 *	OK	- Successful (returns always success code
 */

int clntExtractReplyCode (
	char	*buffer,    /* Pointer to an array containing the reply message (input) */
	int	*replyCode  /* reply code (output) */
	)
{
	/* extract the codefrom the server reply message */
   sscanf(buffer, "%d", replyCode);

   return (OK);
}  // end of clntExtractReplyCode()

