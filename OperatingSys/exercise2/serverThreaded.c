/* A threaded server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define BUFFERLENGTH 256

void error(char *msg)
{
    perror(msg);
    exit(0);
}

FILE *in_file = NULL;
int counter = 0;
char *ptr;
pthread_mutex_t mut,crt; /* the lock */

/* the procedure called for each request */
void *processRequest (void *args) {
  int *newsockfd = (int *) args;
  char buffer[BUFFERLENGTH];
  int n;

  bzero (buffer, BUFFERLENGTH);
  n = read (*newsockfd, buffer, BUFFERLENGTH -1);
  if (n < 0){
    close (*newsockfd); /* important to avoid memory leak */  
    free (newsockfd);
    error ("ERROR reading from socket");
  }

  pthread_mutex_lock(&mut);
  in_file  = fopen(ptr, "a");
  if(in_file == NULL) {
      fprintf(stderr,"unable to open file");
      pthread_mutex_unlock(&mut);
  }
  while(n > 0){

    if(fprintf(in_file, "%d ", counter) < 0){
      fprintf(stderr, "unable to write to file");
      pthread_mutex_unlock(&mut);
      break;
    }  
    if(fprintf(in_file, "%s", buffer)< 0){
      fprintf(stderr, "unable to write to file");
      pthread_mutex_unlock(&mut);
      break;
    }

    counter++;

    n = write (*newsockfd,"I got your message",18);
    if (n < 0){
      close (*newsockfd); /* important to avoid memory leak */  
      free (newsockfd);
      fclose(in_file);
      pthread_mutex_unlock(&mut);
      error ("ERROR writing to socket");
    }

    bzero (buffer, BUFFERLENGTH);
    n = read (*newsockfd, buffer, BUFFERLENGTH -1);
  }
  if(in_file != NULL){
    fclose(in_file);
    pthread_mutex_unlock(&mut);
  }

  close (*newsockfd); /* important to avoid memory leak */  
  free (newsockfd);
  pthread_exit (NULL);
}



int main(int argc, char *argv[])
{
  socklen_t clilen;
  int sockfd, portno;
  char buffer[BUFFERLENGTH];
  struct sockaddr_in6 serv_addr, cli_addr;
  int result;

  if (argc != 3) {
    fprintf (stderr,"usage: %s <port> <filename> \n", argv[0]);
    exit(1);
  } 

  ptr = argv[2];

  /* check port number */
  portno = atoi(argv[1]);
  if ((portno < 0) || (portno > 65535)) {
    fprintf (stderr, "%s: Illegal port number, exiting!\n", argv[0]);
    exit(1);
  }
  /* create socket */
  sockfd = socket (AF_INET6, SOCK_STREAM, 0);
  if (sockfd < 0){
    fprintf(stderr,"ERROR opening socket");
  }

  bzero ((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin6_family = AF_INET6;
  serv_addr.sin6_addr = in6addr_any;
  serv_addr.sin6_port = htons (portno);

  /* bind it */
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
    fprintf(stderr,"ERROR on binding");
    return(1);
  }

  /* ready to accept connections */
  listen (sockfd,5);
  clilen = sizeof (cli_addr);

  /* now wait in an endless loop for connections and process them */
  while (1) {
    pthread_t server_thread;

    int *newsockfd; /* allocate memory for each instance to avoid race condition */
    pthread_attr_t pthread_attr; /* attributes for newly created thread */

    newsockfd  = malloc (sizeof (int));
    if (!newsockfd) {
      fprintf (stderr, "Memory allocation failed!\n");
      exit (1);
    }

    /* waiting for connections */
    *newsockfd = accept( sockfd, 
        (struct sockaddr *) &cli_addr, 
        &clilen);
    if (*newsockfd < 0) 
      error ("ERROR on accept");
    bzero (buffer, BUFFERLENGTH);

    /* create separate thread for processing */
    if (pthread_attr_init (&pthread_attr)) {
      fprintf (stderr, "Creating initial thread attributes failed!\n");
      exit (1);
    }

    if (pthread_attr_setdetachstate (&pthread_attr, PTHREAD_CREATE_DETACHED)) {
      fprintf (stderr, "setting thread attributes failed!\n");
      exit (1);
    }
    result = pthread_create (&server_thread, &pthread_attr, processRequest, (void *) newsockfd);
    if (result != 0) {
      fprintf (stderr, "Thread creation failed!\n");
      exit (1);
    }

  }
  return 0; 
}
