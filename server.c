#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#define MYPORT 5000 /* Avoid reserved ports */
#define BACKLOG 5  /* pending connections queue size */
#define ERROR_404_MESSAGE "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY><H1>404 Not Found</H1><P>The requested file was not found on this server.</P></BODY></HTML>"
#include <signal.h>
#include <string.h>
#include <errno.h>

int sockfd, new_fd;

void  INThandler(int sig);
char* parser(char* msg);
void send_file(char* filename, int sockfd);
char *replaceWord(char *s, char *oldW, char *newW); 

int main(int argc, char *argv[])
{
  struct sockaddr_in my_addr;    /* my address */
  struct sockaddr_in their_addr; /* connector addr */
  socklen_t sin_size;
  /* create a socket */
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }

  // ...
  bzero((char *)&my_addr, sizeof(my_addr));
  /* set the address info */
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(8000); 
  my_addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(sockfd, (struct sockaddr *)&my_addr,
           sizeof(struct sockaddr)) == -1)
  {
    perror("bind");
    exit(1);
  }
  if (listen(sockfd, BACKLOG) == -1)
  {
    perror("listen");
    exit(1);
  }
  while (1)
  { 
	signal(SIGINT, INThandler);
	char buffer[1024];
	bzero(buffer, 1024);
    sin_size = sizeof(their_addr);
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
    {
      perror("accept");
      exit(0);
    }
	/* dump out HTTP request on console */
	if (read(new_fd, buffer, 1023) < 0)
	{
		perror("Error from reading socket");
		exit(1);
	}
	
	printf("%s\n", buffer);
	
	char* filename = parser(buffer);
	send_file(filename,new_fd);
	
	close(new_fd);
    
  }
  close(sockfd);
}

void  INThandler(int sig)
{
    signal(sig, SIG_IGN);
    close(sockfd);
	close(new_fd);
    exit(0);
}

char* parser(char* msg)
{
	const char delim[1] = " ";
	char* token = strtok(msg, delim);
	token = strtok(NULL, delim);
	token ++;
	return token;
	int* spacePtr = strstr(token, "%20");
	if (spacePtr != NULL)
		return replaceWord(token, "%20", " ");
	else
		return token; 
}

void send_file(char* filename, int sockfd)
{
	FILE* ptr;
	ptr = fopen(filename, "r");
	if (ptr == NULL)
	{
		send(sockfd, ERROR_404_MESSAGE, strlen(ERROR_404_MESSAGE), 0);
		return;
	}
	fseek(ptr, 0L, SEEK_END);
	long size = ftell(ptr);
	char* buffer = malloc(sizeof(char) * (size + 1));
	size_t num = fread(buffer, sizeof(char), size, ptr);
	buffer[num] = '\0';
	send(sockfd, buffer, num, 0);
	free(buffer);
	fclose(ptr);
}

char *replaceWord(char *s, char *oldW, char *newW) 
{ 
    char *result; 
    int i, cnt = 0; 
    int newWlen = strlen(newW); 
    int oldWlen = strlen(oldW); 
    for (i = 0; s[i] != '\0'; i++) 
    { 
        if (strstr(&s[i], oldW) == &s[i]) 
        { 
            cnt++;  
            i += oldWlen - 1; 
        } 
    } 
    result = (char *)malloc(i + cnt * (newWlen - oldWlen) + 1); 
    i = 0; 
    while (*s) 
    { 
        if (strstr(s, oldW) == s) 
        { 
            strcpy(&result[i], newW); 
            i += newWlen; 
            s += oldWlen; 
        } 
        else
            result[i++] = *s++; 
    } 
  
    result[i] = '\0'; 
    return result; 
} 

