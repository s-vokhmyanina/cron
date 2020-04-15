#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>
#include <syslog.h>
 
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
    int flag = 0;
    int fd = open("/tmp/mes.daemon.txt", O_CREAT|O_RDWR|O_APPEND, S_IRWXU);
    ftruncate(fd,0);
   
    char buf[] = " ~ALARM~ \n";
   
    while(1)
    {      
        pause();
        if (sig_alarm == true)
        {
            write(fd, buf, sizeof(buf));
            syslog (LOG_NOTICE, " ~ALARM~ ");
            FILE *fp;
            int max_in = 256;
            char input[max_in];
            char stroka[max_in];
            
            int fd_ = open(argv[1],  O_CREAT|O_RDWR|O_APPEND, S_IRWXU);
            if (!fd_)
			{
				write(fd, " OPEN ERROR\n", 12);
				syslog (LOG_NOTICE, "Error opening file with command ");
				flag = -1;
				break;
			}
			
            int len = read(fd_, stroka, 255);
            if (!len)
            {
				write(fd, " READ ERROR\n", 12);
				syslog (LOG_NOTICE, "Error reading file ");
				close(fd_);
				flag = -1;
				break;
			}
            stroka[len] = '\0';
            close(fd_);
            
            int fo = open("out.txt",  O_CREAT|O_RDWR|O_APPEND, S_IRWXU);
            if (!fo)
			{
				write(fd, " OPEN ERROR\n", 12);
				syslog (LOG_NOTICE, "Error opening file for output ");
				flag = -1;
				break;
			}
			
            fp = popen(stroka, "r");
            
            if (fp == NULL)
            {
				write(fd, " PIPE ERROR\n", 12);
				syslog (LOG_NOTICE, "Pipe error ");
				close(fo);
				flag = -1;
				break;
			}
   
            int fp_ = fileno(fp);
            while (read(fp_, input, max_in))
			{
				write(fo, input, sizeof(input));
			}
                    
            write(fo, "\n", 1);     
            write(fo, "----------", 10);    
            write(fo, "\n", 1);            
    
            pclose(fp);
            close(fo);
            write(fd, " COMMAND COMPLETED\n", 19);
			syslog (LOG_NOTICE, "Command completed ");
            sig_alarm = false;
           
        }
        
        if (sig_term == true)
        {
			break;
        }              
    }
    
    close(fd);
    return flag;
}
 
int main(int argc, char* argv[]) //--это наша главная процедура с которой начинается программа
{
    pid_t pid, sid;
 
    if((pid=fork())<0) //--здесь мы пытаемся создать дочерний процесс главного процесса (масло масляное в прямом смысле)
    {                   //--точную копию исполняемой программы
     printf(" Can't fork :c \n"); //--если нам по какой-либо причине это сделать не удается выходим с ошибкой.
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

