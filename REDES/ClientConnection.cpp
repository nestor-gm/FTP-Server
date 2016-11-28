//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2ª de grado de Ingeniería Informática
//                       
//              This class processes an FTP transactions.
// 
//****************************************************************************



#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h> 
#include <iostream>
#include <dirent.h>

#include "common.h"

#include "ClientConnection.h"
#include "FTPServer.h"





ClientConnection::ClientConnection(int s) {
    int sock = (int)(s);
  
    char buffer[MAX_BUFF];

    control_socket = s;
    // Check the Linux man pages to know what fdopen does.
    fd = fdopen(s, "a+");
    if (fd == NULL){
	std::cout << "Connection closed" << std::endl;

	fclose(fd);
	close(control_socket);
	ok = false;
	return ;
    }
    
    ok = true;
    data_socket = -1;
   
  
  
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
  
}


int connect_TCP( uint32_t address,  uint16_t  port) {
     // Implement your code to define a socket here

    struct sockaddr_in sin;
    struct hostent *hent;
    int s;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    memcpy(&sin.sin_addr, (void*)&address, sizeof(address));

    
     
    s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0)
       errexit("No se puede crear el socket: %s\n", strerror(errno));

    if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
       errexit("No se puede conectar con %s: %s\n",  address, strerror(errno));

    return s;

}






void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;
  
}





    
#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
    if (!ok) {
	 return;
    }
    
    fprintf(fd, "220 Service ready\n");
  
    while(!parar) {

      fscanf(fd, "%s", command);

      if (COMMAND("USER")) 
      {
	        fscanf(fd, "%s", arg);
	        if (strcmp(arg, "root") == 0)
	        {
	            fprintf(fd, "331 User name ok, need password\n");
	        }
	        else 
	        {
	            fprintf(fd, "530 Not logged in\n");
	        }
      }
      
      else if (COMMAND("PWD")) 
      {
	        char aux[200];
	        getcwd(aux,sizeof(aux));
	        fprintf(fd,"257 %s\n,",aux);
	        
      }
      
      else if (COMMAND("PASS")) 
      {
	       fscanf(fd, "%s", arg);
	       if (strcmp(arg, "12345") == 0) 
	       {
	           fprintf(fd, "230 User logged in, proceed.\n");
	       }
	        
	       else 
	       {
	           fprintf(fd, "530 Not logged in\n");
	       }
	   
      }
      else if (COMMAND("PORT")) 
      {
	  
	    unsigned ip[4];
        unsigned port[2];

        fscanf(fd,"%u,%u,%u,%u,%u,%u",&ip[0],&ip[1],&ip[2],&ip[3],&port[0],&port[1]);

        uint32_t aux1;
        uint16_t aux2;

        aux1 = ip[3] << 24 | ip[2] << 16 | ip[1] << 8 | ip[0];

        aux2 = port[0] << 8 | port[1];

        data_socket = connect_TCP(aux1,aux2);

        fprintf(fd,"200 OK\n");
	  
	  
      }
      else if (COMMAND("PASV")) 
      {
        struct sockaddr_in sin;
        socklen_t len_sin=sizeof(sin);
        uint16_t p;
	      
	    int s = define_socket_TCP(0);
	    int passive = getsockname (s, (struct sockaddr *)&sin , &len_sin);
	      
	    uint16_t puerto = sin.sin_port;
	      
	    int p1;
	    int p2;
	      
	    p2 = puerto >> 8;
	    p1 = puerto & 0xFF;
        
        int a1 = 127;
        int a2 = 0;
        int a3 = 0;
        int a4 = 1;
	      
	      if (passive == 0)
	      {
	        fprintf (fd, "227 Entering Passive Mode (%d, %d, %d, %d, %d, %d)\n", a1, a2, a3, a4, p1, p2); fflush(fd);
            len_sin = sizeof(sin);
            data_socket=accept(s, (struct sockaddr*)&sin, &len_sin);
	      }
        
         else 
         {
             
             fprintf(fd,"530 Not logged in\n");
             
         }
      }
      else if (COMMAND("CWD")) 
      {
	  
	        char aux[200];
	        getcwd(aux,sizeof(aux));
	        if ( chdir(aux) == 0)
	        {
	            fprintf (fd, "250 Requested file action okay, completed\n");
	        }
	        else
	        {
	            fprintf(fd, "501 Syntax error in parameters or arguments\n");
	        }
	  
      }
      
      else if (COMMAND("STOR") )
      {
	        FILE* fp = fopen("filename.txt","w+");

            if (fp) {
                
                fprintf(fd, "150 File open.\n"); fflush(fd);
                
                int size_buffer = 512;
                char buffer[size_buffer];
                int recived_datas;
    
                while(recived_datas == size_buffer)
                {
    
                    recived_datas = recv(data_socket,buffer,size_buffer,0);
                    fwrite(buffer,1,recived_datas,fp);
                    fprintf(fd,"125 Data connection already open; transfer starting\n");
    
                }
                close(data_socket);
                fprintf(fd,"226 Closing data connection.\n");
                fclose(fp);
                
                fprintf(fd, "226 Transfer complete.\n"); fflush(fd);
            }
            else {
              fprintf(fd, "425 Can't open data connection.\n"); fflush(fd);
            }
	    
      }
      
      else if (COMMAND("SYST")) 
      {
	       fprintf(fd, "215 UNIX Type: L8\n");
	   
      }
      else if (COMMAND("TYPE")) 
      {
	       fprintf(fd, "200 OK\n");
	  
      }
      
      else if (COMMAND("RETR")) 
      {
	        fscanf(fd,"%s",arg);
            std::cout << "Argument: " << arg << std::endl;
            FILE* fp = fopen(arg,"rb+");

            if (fp) {
                fprintf(fd, "150 File open.\n"); fflush(fd);
      

                int sent_datas;

                int size_buffer = 512;

                char buffer[size_buffer];

                std::cout << "Buffer size = " << size_buffer << std::endl;

                do
                {

                    sent_datas = fread(buffer,1,size_buffer,fp);
                    printf("Code %d |  %s\n",errno,strerror(errno));
                    send(data_socket,buffer,sent_datas,0);
                    printf("Code %d |  %s\n",errno,strerror(errno));
                    printf("%d %d\n", sent_datas, size_buffer);

                }
                while(sent_datas == size_buffer);

                close(data_socket);
                fclose(fp);

                fprintf(fd, "226 Transfer complete.\n"); fflush(fd);
            }
            else {
              fprintf(fd, "425 Can't open data connection.\n"); fflush(fd);
            }
      }
      
      else if (COMMAND("QUIT")) 
      {
          fprintf(fd, "221 Service closing control connection.\n");
          parar = true;
      }
      else if (COMMAND("LIST")) 
      {

	       DIR *dirp = opendir(".");
	       struct dirent *dp;
	       
	       if (dirp){
	           
            fprintf(fd, "125 List started OK.\n"); fflush(fd);
            
            while ((dp = readdir(dirp)) != NULL) {
                send(data_socket, dp->d_name, strlen(dp->d_name),1); 
                fprintf(fd," %s \r\n", dp->d_name);
            }
            closedir(dirp);
            
            fprintf(fd, "250 List completed successfully.\n"); fflush(fd);
	       }
            
            else
            {
                fprintf(fd, "425 Can't open data connection.\n"); fflush(fd);
            }
	
      }
      else if (COMMAND("FEAT"))
      {
        fprintf(fd, "502 Command not implemented.\n"); fflush(fd);

      }
      else if (COMMAND("SIZE"))
      {
        fscanf(fd,"%s",arg);
        fprintf(fd, "502 Command not implemented.\n"); fflush(fd);

      }
      else if (COMMAND("EPSV"))
      {
        fprintf(fd, "502 Command not implemented.\n"); fflush(fd);

      }
      else  
      {
	       fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
	       printf("Comando : %s %s\n", command, arg);
	       printf("Error interno del servidor\n");
      }
      
    }
    
    fclose(fd);

    std::cout << "Cerrando conexión"  << std::endl;
    
    return;
  
};