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
#include <fcntl.h> 
#include <unistd.h> 
#define BACKLOG 5  
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
  struct sockaddr_in my_addr;  
  struct sockaddr_in their_addr; 
  socklen_t sin_size;
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }
  bzero((char *)&my_addr, sizeof(my_addr));
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
  return 0;
}

char* parser(char* msg)
{
	const char space[1] = " ";
	char* token = strtok(msg, space);
	token = strtok(NULL, space);
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
		char* not_found = "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY><H1>404 Not Found</H1><P>The requested file was not found on this server.</P></BODY></HTML>";
		int len = strlen(not_found);
		send_response(sockfd, 404, filename, 0);
		send(sockfd, not_found, len, 0);
		return;
	}
	if (!fseek(ptr, 0, SEEK_END))
	{
		long size = ftell(ptr);
		if (size < 0)
		{
			char* not_found = "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY><H1>404 Not Found</H1><P>The requested file was not found on this server.</P></BODY></HTML>";
			int len = strlen(not_found);
			send_response(sockfd, 404, filename, 0);
			send(sockfd, not_found, len, 0);
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
	char *dotPos;
    char *type;
    if ((dotPos = strrchr(file_name, '.')) != NULL){
		if (strcasecmp(dotPos, ".html") == 0)
			type = "Content-Type: text/html\r\n";
		else if (strcasecmp(dotPos, ".jpeg") == 0)
            type = "Content-Type: image/jpeg\r\n";
        else if (strcasecmp(dotPos, ".gif") == 0)
            type = "Content-Type: image/gif\r\n";
		else if (strcasecmp(dotPos, ".png") == 0)
			type = "Content-Type: image/png\r\n";
		else if (strcasecmp(dotPos, ".jpg") == 0)
			type = "Content-Type: image/jpg\r\n";
		else if (strcasecmp(dotPos, ".htm") == 0)
			type = "Content-Type: text/htm\r\n";
		else if (strcasecmp(dotPos, ".txt") == 0)
			type = "Content-Type: text/txt\r\n";
		else
			type = "Content-Type: text/html\r\n";
	}
	else
		if (code != 404)
			type = "Content-Type: application/octet-stream";
		else
			type = "Content-Type: text/html\r\n";
	
	time_t rawtime;
	struct tm * timeinfo;
	time (&rawtime);
	timeinfo = localtime (&rawtime);
    char current[64];
	char timestamp[64] = "Date: ";
    strftime(current, 64, "%a, %d %b %Y %T %Z", timeinfo);
    strcat(current, "\r\n");
	strcat(timestamp, current);
	
	char modifed[32];
	struct stat b;
	char lm[64] = "Last-Modified: ";
	if (!stat(file_name, &b)){
		strftime(modifed, 100, "%a, %d %b %Y %T %Z", localtime( &b.st_mtime));
		strcat(modifed, "\r\n");
		strcat(lm, modifed);
	}
	
    char fileLength[50] = "Content-Length: ";
    char len[30];
    sprintf(len, "%d", file_length);
    strcat(len, "\r\n");
	strcat(fileLength, len);
	
    char msg[1024] = "";
	char *connection = "Connection: close\r\n";
	char *server = "Server: Yiv/1.0\r\n";

	if (code == 404)
	{
		strcat(msg, "HTTP/1.1 404 Not Found\r\n");
		printf("%s\n", timestamp);
		strcat(msg, timestamp);
		strcat(msg, server);
		strcat(msg, type);
		strcat(msg, "\r\n\0");
	}
	else if (code == 200)
	{
		strcat(msg, "HTTP/1.1 200 OK\r\n");
		strcat(msg, connection);
		strcat(msg, timestamp);
		strcat(msg, server);
		strcat(msg, lm);
		strcat(msg, fileLength);
		strcat(msg, type);
		strcat(msg, "\r\n\0");
	}
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