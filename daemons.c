#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>
#include <syslog.h>
#include <limits.h>
#include <string.h>
#include <sys/wait.h>
#include <semaphore.h>

#define MAX_COUNT_CMD 10

sem_t sema;

bool sig_alarm = false;
bool sig_term = false;
 
void signal_alarm_handler()
{
    sig_alarm = true;
}
 
void signal_term_handler()
{
    sig_term = true;
}
 
int Daemon(char* argv[])
{
   
    syslog (LOG_NOTICE, "My daemon started OO ");
       
    signal(SIGALRM, signal_alarm_handler);
    signal(SIGTERM, signal_term_handler);
    signal(SIGCLD, SIG_IGN);					
    int flag = 0;
    int mes = open("/tmp/mes.daemon.txt", O_CREAT|O_RDWR|O_APPEND, S_IRWXU);
    ftruncate(mes,0);
	if(sem_init(&sema, 0, 1) == -1)
	{
		write(mes, " SEMA INIT ERROR\n", 12);
		syslog (LOG_NOTICE, "Error with init semaphore ");
		return -1;
	}
	
    char buf[] = " ~ALARM~ \n";
    
    while(1)
    {      
        pause();
        if (sig_alarm == true)
        {
            write(mes, buf, sizeof(buf));
            syslog (LOG_NOTICE, " ~ALARM~ ");
            FILE *fp;
            int max_in = 256;
            char input[max_in];
            char cmd[MAX_COUNT_CMD][max_in];
            
            char stroka[max_in];
            
            int fd_ = open(argv[1],  O_CREAT|O_RDWR|O_APPEND, S_IRWXU);
            if (!fd_)
			{
				write(mes, " OPEN ERROR\n", 12);
				syslog (LOG_NOTICE, "Error opening file with command ");
				flag = -1;
				break;
			}
			char c;
			int i = 0;
			int k = 0;
			while(read(fd_, &c, 1))
			{	
				if (c < 0)
				{
					write(mes, " READ CMD ERROR\n", 16);
					syslog (LOG_NOTICE, "Error reading command ");
					flag = -1;
					break;
				}
				else
				{
					if (c != '\n')
					{
						cmd[i][k] = c;
						k++;
					}
					else
					{
						if (k != 0)
							{
								cmd[i][k] = '\0';
								k = 0; 
								i++;
							}
					}
					if (i == MAX_COUNT_CMD)
					{
						write(mes, " CMD LIMIT\n", 11);
						syslog (LOG_NOTICE, "Limit of the number of executed commands, not all commands will be executed ");
						break;
					}
				}
			}
			
            close(fd_);
            if (flag == -1)
			{
				break;
			}
            int fo = open("out.txt",  O_CREAT|O_RDWR|O_APPEND, S_IRWXU);
            if (!fo)
			{
				write(mes, " OPEN ERROR\n", 12);
				syslog (LOG_NOTICE, "Error opening file for output ");
				flag = -1;
				break;
			}
			
            pid_t pid[MAX_COUNT_CMD];

			int count = 1;
			
			for(int k = 0; k < i; k++)
			{
				/*int fd[2];

				if (pipe(fd) < 0)
				{
				write(mes, " PIPE ERROR\n", 12);
				syslog (LOG_NOTICE, "Can't create pipe ");
				flag = -1;
				break;
				}*/
				
				pid[k] = fork();
				if (pid[k] > 0)
				{
					/*char buffer[max_in];
					
					close(fd[1]);	 
					while((count = read(fd[0], buffer, sizeof(buffer))) != 0)
					{
						write(fo, buffer, sizeof(buffer));
					}	
					write(fo, "\n", 1);     
					write(fo, "----------", 10);    
					write(fo, "\n", 1);*/
					 
				}
				else if (pid[k] == 0)
				{
					
					char * token = strtok(cmd[k], " ");
					char * arg[max_in];	
					char num[max_in];
					int j = 0;
					char * cmdd = token; 
					
					while(token != NULL) 
					{
						
						arg[j] = token;
						j++;
						token = strtok(NULL, " ");
					}
					arg[j] = NULL; 
					
					if (sem_wait(&sema) == -1)
					{
						write(mes, " SEMA WAIT ERROR\n", 17);
						syslog (LOG_NOTICE, "Some error with semaphore (wait) ");
						flag = -1;
						break;
					}
					else
					{
						char log_mes[40];
						sprintf(log_mes, "%s%d%s", " COMMAND  (", k+1, ") STARTED\n");
						write(mes, log_mes, sizeof(log_mes));	
						close(1);
						if (dup2(fo, 1) == -1)
				        {
							write(mes, " DUPLICATE ERROR\n", 17);
							syslog (LOG_NOTICE, "Some error with duplicate ");
							flag = -1;
							break;
						}	
						if (execve(cmdd, arg, NULL) == -1)
						{
							write(mes, " EXEC ERROR\n", 12);
							syslog (LOG_NOTICE, "Some error with exec ");
							flag = -1;
							break;
						}	 	
						if (sem_post(&sema) == -1)
						{
							write(mes, " SEMA POST ERROR\n", 17);
							syslog (LOG_NOTICE, "Some error with semaphore (post) ");
							flag = -1;
							break;
						}			
						/*close(fd[0]);
						close(1); 					
						dup2(fd[1],1);*/	
					}
					//exit(0);
				}
				else
				{
					write(mes, " FORK ERROR\n", 12);
					syslog (LOG_NOTICE, "Can't fork process ");
					flag = -1;
					break;
				}
				//write(mes, " SOME COMMAND COMPLETED\n", 24);
				syslog (LOG_NOTICE, "Some command completed ");
			}

            close(fo);
            if (flag == -1)
            {
				break;
			} 
            
            sig_alarm = false;
        }
        if (sig_term == true)
        {
			break;
        }              
    }
    sem_destroy(&sema);
    write(mes, " END\n", 5);
    close(mes);
    return flag;
}
 
int main(int argc, char* argv[]) //--это наша главная процедура с которой начинается программа
{
    pid_t pid, sid;
 
    if((pid=fork())<0) //--здесь мы пытаемся создать дочерний процесс главного процесса (масло масляное в прямом смысле)
    {                   //--точную копию исполняемой программы
     printf(" Can't fork :c\n"); //--если нам по какой-либо причине это сделать не удается выходим с ошибкой.
     exit(1);                //--здесь, кто не совсем понял нужно обратится к man fork
    }
    else if (pid!=0) //--если дочерний процесс уже существует
    {
      exit(0);            //--генерируем немедленный выход из программы(зачем нам еще одна копия программы)
    }
   
    sid = setsid();           //--перевод нашего дочернего процесса в новую сессию
   
    if (sid < 0)  // если не получилось это сделать
    {
                exit(-1);
    }
       
    /*if ((chdir("/")) < 0) //смена директории
    {
            exit(-1);
    }*/
   
    openlog ("My_daemon", LOG_PID | LOG_CONS, LOG_DAEMON);
   
    close(STDIN_FILENO); // перенаправление стандартных потоков в /dev/null в каких-то версиях игнорирование
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
   
    int res = Daemon(argv);           //--ну а это вызов нашего демона с нужным нам кодом (код будет приведен далее)
    if (res == 0)
	{
		syslog (LOG_NOTICE, "Success ^^ ");
	}
	else
	{
		syslog (LOG_NOTICE, "Error :c ");
	}
    syslog (LOG_NOTICE, "My daemon terminated XX ");
    closelog();
   
    return 0;
}


