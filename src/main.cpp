#include <stdio.h>
#include <string.h> 
#include <stdlib.h>    
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>   
#include <pthread.h> 
#include <sys/sendfile.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
//path to directory
char *dir = NULL;

//the thread function
void *connection_handler(void *);

/* Parse and return file handle for http get request */
int OpenFile(char * buf, int n){
  int i;
  char path[2000];
  char file_name[1024];
  if (buf==NULL || n<10)
    return 0;
  // Ensure that the string starts with "GET "
  if (!buf[0]=='G' || !buf[1]=='E' || !buf[2]=='T' || !buf[3]==' ')
    return 0;
  // copy the string
  for (i=0; i < n && buf[i+4]!=' '; i++)
    file_name[i]=buf[i+4];  
  strcat(path, dir);
  strcat(path, file_name);  
  // put null on end of path
  //path[i]='\x00';
  // if the syntax doesn't have a space following a path then return
  if (buf[i+4]!=' ' || i == 0)
    return 0;
  printf("Found path: \"%s\"\n", path);
  return (open(path, O_RDONLY));
}

void Respond(int fd, char * buf, int n){
  char badresponse[1024] = {"HTTP/1.0 404 Not Found\r\n\r\n<HTML><HEAD><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\r\n<TITLE>Not Found</TITLE></HEAD><BODY>\r\n<H1>Not Found</H1>\r\n</BODY></HTML>\r\n\r\n"};
  char goodresponse[1024] = {"HTTP/1.0 200 OK\r\n\r\n<HTML><HEAD><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\r\n</HEAD></HTML>\r\n\r\n"};
  int fh; 
  //  printf("%d data received: \n%s\n", fd, buf);
  fh = OpenFile(buf, n);
  if (fh > 0){
    struct stat stat_buf;  /* hold information about input file */
    send(fd, goodresponse, strlen(goodresponse), 0);
    /* size and permissions of fh */
    fstat(fh, &stat_buf);
    sendfile(fd, fh, NULL, stat_buf.st_size);
    close(fh);
  } else {
    send(fd, badresponse, strlen(badresponse), 0);
  }
  close(fd);
}


int main(int argc , char *argv[])
{
  char *ip = NULL;
  char *port = NULL;
  
  int o;

  opterr = 0;

  while ((o = getopt (argc, argv, "h:p:d:")) != -1)
    switch (o)
      {
      case 'h':
        ip = optarg;
        break;
      case 'p':
        port = optarg;
        break;
      case 'd':
        dir = optarg;
        break;
      
      default: /* '?' */
                   fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
                           argv[0]);
                   exit(EXIT_FAILURE);
      }

  printf("ip=%s; port=%s; dir=%s\n", ip, port, dir);
  
  
  //make daemon
  pid_t pid;     
  /* Fork off the parent process */
  pid = fork();
  if (pid < 0) {
      exit(EXIT_FAILURE);
   }
   /* If we got a good PID, then
   we can exit the parent process. */
   if (pid > 0) {
        exit(EXIT_SUCCESS);
   }

   /* Change the file mode mask */
   umask(0); 
 
  
  int socket_desc , client_sock , c;
    struct sockaddr_in server , client;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(atoi(port));
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
	pthread_t thread_id;
	
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
         
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
        //puts("Handler assigned");
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;

}
 
/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char client_message[2000];
     
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
    {
		Respond(sock, client_message, read_size);
    }
     
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
         
    return 0;
} 
