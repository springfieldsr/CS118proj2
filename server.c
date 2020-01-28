#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
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
#include <dirent.h>

int sockfd, new_fd, pid;

void  INThandler(int sig);
char* parser(char* msg);
void send_file(char* filename, int sockfd);
char *replaceWord(char *s, char *oldW, char *newW); 
void send_response(int sockfd, int code, char* filename, int length);
void strip_ext(char *fname);

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
	/*char buffer[1024];
	bzero(buffer, 1024);
    sin_size = sizeof(their_addr);
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
    {
      perror("accept");
      exit(0);
    }
	if (read(new_fd, buffer, 1023) < 0)
	{
		perror("Error from reading socket");
		exit(1);
	}
	
	printf("%s\n", buffer);
	
	char* filename = parser(buffer);
	send_file(filename,new_fd);
	close(new_fd);*/
	if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
    {
      perror("accept");
      exit(0);
    }

    pid = fork(); 
    if (pid < 0)
		perror("ERROR on fork");
	if (pid == 0)  { 
		close(sockfd);
		char buffer[1024];
		bzero(buffer, 1024);
		if (read(new_fd, buffer, 1023) < 0)
		{
			perror("Error from reading socket");
			exit(1);
		}
	
		printf("%s\n", buffer);
	
		char* filename = parser(buffer);
		send_file(filename,new_fd);
		return 0;
        }
    else 
        close(new_fd); 
    
  }
  close(sockfd);
}

char* parser(char* msg)
{
	const char delim[1] = " ";
	char* token = strtok(msg, delim);
	token = strtok(NULL, delim);
	token ++;

	int* spacePtr = strstr(token, "%20");
	if (spacePtr != NULL)
		return replaceWord(token, "%20", " ");
	else
		return token; 
}

void send_file(char* filename, int sockfd)
{
	char truefile[100];
	memcpy(truefile, filename, strlen(filename));
	FILE* ptr = fopen(filename, "r");
	if (!ptr)
	{
		DIR *d;
		struct dirent *dir;
		d = opendir(".");
		if (d)
		{
			while ((dir = readdir(d)) != NULL)
			{
				char tempfile[100];
				memcpy(tempfile, dir->d_name, strlen(dir->d_name));
				tempfile[strlen(dir->d_name)] = '\0';
				strip_ext(tempfile);
				int temp = strcmp(filename, tempfile);
				if (temp == 0)
				{
					memcpy(truefile, dir->d_name, strlen(dir->d_name));
					ptr = fopen(dir->d_name, "r");
					break;
				}								
			}
			closedir(d);
		}
	}

	if (ptr == NULL)
	{
		send_response(sockfd, 404, filename, 0);
		send(sockfd, ERROR_404_MESSAGE, strlen(ERROR_404_MESSAGE), 0);
		return;
	}
	if (!fseek(ptr, 0, SEEK_END))
	{
		long size = ftell(ptr);
		if (size < 0)
		{
			send_response(sockfd, 404, filename, 0);
			send(sockfd, ERROR_404_MESSAGE, strlen(ERROR_404_MESSAGE), 0);
			return;
		}
		fseek(ptr, 0, SEEK_SET);
		char* buffer = malloc(sizeof(char) * (size + 1));
		size_t num = fread(buffer, sizeof(char), size, ptr);
		buffer[num] = '\0';
		send_response(sockfd, 200, truefile, num);
		send(sockfd, buffer, num, 0);
		free(buffer);
	}
	fclose(ptr);
	return;
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

void send_response(int socket, int code, char* file_name, int file_length)
{
	char* header;
	if (code == 404)
		header = "HTTP/1.1 404 Not Found\r\n";
	else
		header = "HTTP/1.1 200 OK\r\n";
	char *connection = "Connection: close\r\n";
	char *server = "Server: Yiv/1.0\r\n";
	time_t raw_time;
    time(&raw_time);
    struct tm *time_info = localtime(&raw_time);
    char current_time[32];
    strftime(current_time, 32, "%a, %d %b %Y %T %Z", time_info);
    char date[64] = "Date: ";
    strcat(date, current_time);
    strcat(date, "\r\n");
	
	struct stat attr;
    stat(file_name, &attr);
    //st_mtime gives the modification time
    struct tm* lm_info = localtime(&(attr.st_mtime));
    char lm_time[32];
    strftime(lm_time, 32, "%a, %d %b %Y %T %Z", lm_info);
    char last_modified[64] = "Last-Modified: ";
    strcat(last_modified, lm_time);
    strcat(last_modified, "\r\n");
	
	//content-length section
    char content_length[50] = "Content-Length: ";
    char len_buffer[16];
    sprintf(len_buffer, "%d", file_length);
    strcat(content_length, len_buffer);
    strcat(content_length, "\r\n");
	
	char *dot;
    char *content_type = "Content-Type: text/html\r\n";
    if ((dot = strrchr(file_name, '.')) != NULL){
		if (strcasecmp(dot, ".jpeg") == 0)
            content_type = "Content-Type: image/jpeg\r\n";
        else if (strcasecmp(dot, ".gif") == 0)
            content_type = "Content-Type: image/gif\r\n";
		else if (strcasecmp(dot, ".png") == 0)
			content_type = "Content-Type: image/png\r\n";
		else if (strcasecmp(dot, ".jpg") == 0)
			content_type = "Content-Type: image/jpg\r\n";
		else if (strcasecmp(dot, ".htm") == 0)
			content_type = "Content-Type: text/htm\r\n";
		else if (strcasecmp(dot, ".txt") == 0)
			content_type = "Content-Type: text/txt\r\n";
	}
	else
		if (code != 404)
			content_type = "Content-Type: application/octet-stream";
		
	char hdr_section[6][50];
    strcpy(hdr_section[0], connection);
    strcpy(hdr_section[1], date);
    strcpy(hdr_section[2], server);
    strcpy(hdr_section[3], last_modified);
    strcpy(hdr_section[4], content_length);
    strcpy(hdr_section[5], content_type);

    //construct the message
    char msg[1024];

    //copy status section
    int position = strlen(header);
    memcpy(msg, header, position);
	
	 //copy other sections
    int i;
	int newcode = -1;
	if (code == 200)
		newcode = 0;
    for (i = 0; i < 6; i++)
    {
        if (newcode == 0 || i == 1 || i == 2 || i == 5)
        {
            memcpy(msg + position, hdr_section[i], strlen(hdr_section[i]));
            position += strlen(hdr_section[i]);
        }
    }

    memcpy(msg + position, "\r\n\0", 3);

    //send and print completed response message
    send(socket, msg, strlen(msg), 0);
}

void strip_ext(char *fname)
{
    char *end = fname + strlen(fname);

    while (end > fname && *end != '.') {
        --end;
    }

    if (end > fname) {
        *end = '\0';
    }
}