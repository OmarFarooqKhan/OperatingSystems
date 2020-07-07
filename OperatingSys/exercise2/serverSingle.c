/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#define BUFFERLENGTH 256
int counter = 0;
FILE *in_file = NULL; 

/* displays error messages from system calls */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     socklen_t clilen;
     int sockfd, newsockfd, portno;
     char buffer[BUFFERLENGTH];
     struct sockaddr_in6 serv_addr, cli_addr;
     int n;


     if (argc != 3) {
         fprintf (stderr,"usage: %s <port> filename \n", argv[0]);
         exit(1);
     }

     /* check port number */
     portno = atoi(argv[1]);
     if ((portno <  0) || (portno > 65535)) { 
         fprintf (stderr, "%s: Illegal port number, exiting!\n", argv[0]);
   exit(1);
     }
     /* create socket */
     sockfd = socket (AF_INET6, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero ((char *) &serv_addr, sizeof(serv_addr));
     serv_addr.sin6_family = AF_INET6;
     serv_addr.sin6_addr = in6addr_any;
     serv_addr.sin6_port = htons (portno);

     /* bind it */
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");

     /* ready to accept connections */
     listen (sockfd,5);
     clilen = sizeof (cli_addr);

     
     /* now wait in an endless loop for connections and process them */
     while (1) {
       

       /* waiting for connections */
      newsockfd = accept( sockfd, 
      (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) 
      error ("ERROR on accept");
      bzero (buffer, BUFFERLENGTH);
       
        /* read the data */
      FILE *in_file  = fopen(argv[2], "a");
      if(in_file != NULL){

      while(read (newsockfd, buffer, BUFFERLENGTH -1) > 0 ){
        if(fprintf(in_file, "%d ", counter) < 0){
          close(newsockfd);
          fprintf(stderr, "unable to write to file");
          break;
        }  
        if(fputs(buffer,in_file) == EOF ){
          close(newsockfd);
          fprintf(stderr, "unable to write to file");
          break;
        }
        bzero (buffer, BUFFERLENGTH);
        counter++;

        n = write (newsockfd,"I got your message",18);
        if (n < 0) {
          close(newsockfd);
          error ("ERROR writing to socket");
        }
        bzero (buffer, BUFFERLENGTH);
      }


      n = read (newsockfd, buffer, BUFFERLENGTH -1);
      if (n < 0){
        close(newsockfd);
        error ("ERROR reading from socket");
      }

      fclose(in_file);

      } 

      close(newsockfd);
     /* important to avoid memory leak */
    }
     return 0;
}
