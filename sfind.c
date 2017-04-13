#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <dirent.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>


typedef struct{
	int name;
	char* filename;
	int type;
	char* typename;
	int perm;
	char* permname;
	int print;
	int delete;
}Args;



//PROTOS
int createChild(const char* fileName, Args* args);

//handler CRTL-C
static void sigint_child_handler(int signo)
{
	sigset_t newmask, oldmask;
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGUSR1);
	sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	sigsuspend(&oldmask);
	sigprocmask(SIG_SETMASK, &oldmask, NULL);
}

static void sigusr_handler(int signo)
{
	char* line = "Process continue!\n";
	write(STDOUT_FILENO,line,strlen(line));
}

//handler CRTL-C
static void sigint_handler(int signo)
{
	char* line = "In SIGINT handler ...\n";
	write(STDOUT_FILENO,line,strlen(line));
	line = "Are you sure you want to terminate (Y/N)?\n";
	write(STDOUT_FILENO,line,strlen(line));
	char input;
	scanf("%s", &input);
	input = toupper(input);
	pid_t precessGroup_pid = getpgid(getpid());
	switch(input)
	{
		case 'Y':
			line = "Process terminated!\n";
			write(STDOUT_FILENO,line,strlen(line));
			killpg(precessGroup_pid, SIGKILL);
		break;
		case 'N':
			killpg(precessGroup_pid, SIGUSR1);
		break;
	}
}

int readDirInfo(char* actualDir, Args* args){
	chdir(actualDir);
	strcat(actualDir,"/");
	DIR *directory;
	if ((directory = opendir(".")) == NULL){
		perror("Error Reading Dir\n");
	}

	struct dirent *file;
	struct stat file_info;
	while((file = readdir(directory)) != NULL){
		char *fileName = (*file).d_name;
		//printf("Current file name: %s\n",fileName);
		if (stat(fileName,&file_info)==-1){
			printf("Failed to open directory %s\n", fileName);
			perror("stat");
			return 1;
		}
		if (S_ISDIR(file_info.st_mode)){
			if (fileName[0] == '.' && (fileName[1] == '\0' || fileName[1] == '.')){
				continue;
			}
			//printf("%d: Directory:%s\n",getpid(), fileName);
			createChild(fileName, args);
		}
		else if (S_ISREG(file_info.st_mode)){
			if((*args).print && strcmp((*args).filename, fileName) == 0){
				write(STDOUT_FILENO,actualDir,strlen(actualDir));
				write(STDOUT_FILENO,fileName,strlen(fileName));
				write(STDOUT_FILENO,"\n",1);
			}
			else if((*args).delete && strcmp((*args).filename, fileName) == 0){
				execlp("rm", "rm", fileName, NULL);
			}
		}
	}
	closedir(directory);
	return 0;
}

char* getNextDir(const char* fileName){
	char cwd[1024];
	if(getcwd(cwd,sizeof(cwd)) == NULL){
		perror("Error reading cwd\n");
	}
	else{
		//printf("Current Working Dir:%s\n",cwd);
	}
	char* nextDirPath = cwd;
	strcat(nextDirPath,"/");
	strcat(nextDirPath,fileName);
	/*char* line = "Next dir path:";
	write(STDOUT_FILENO,line,strlen(line));
	write(STDOUT_FILENO,nextDirPath,strlen(nextDirPath));
	write(STDOUT_FILENO,"\n",1);*/
	return nextDirPath;
}

int createChild(const char* fileName, Args* args){
	char *nextDirPath = getNextDir(fileName);
	pid_t pid = fork();
	if (pid == 0) //filho
	{
		//printf("%d: my parent is %d, Opening %s\n",getpid(),getppid(),fileName);
		readDirInfo(nextDirPath, args);
		exit(0);
	}
	else if (pid > 0){ //parent
		int status;
		wait(&status);
		//printf("%d: Child %d terminated\n",getpid(),pid);
	}
	else{ //error
		return 1;
	}
	return 0;
}


int main(int argc, char **argv){
	/*Add SIGINT Handler*/
	struct sigaction actionINT;
	actionINT.sa_handler = sigint_handler;
	sigemptyset(&actionINT.sa_mask);
	actionINT.sa_flags = 0;
	if (sigaction(SIGINT, &actionINT, NULL) < 0)
	{
		perror("Unable to install SIGINT handler\n");
		return 1;
	}
	signal(SIGUSR1, SIG_IGN);

	if(argc != 5 && argc != 3){
		char * line = "Args: [PATH] -[NAME | TYPE] [FILENAME | TYPE | PERM] -[PRINT | DELETE]\n";
		write(STDOUT_FILENO,line,strlen(line));	
		return 1;
	}

	char* actualDir;
	/*Change dir*/
	if (argc > 1){
		if(argv[1][0] != '-'){
			actualDir = argv[1];
		}
	}

	Args args;
	int i = 2;
	if(argc == 5){
		while(argv[i] != NULL){
			char* a = argv[i];
			if(strcmp(a,"-name") == 0){
					args.name = 1;
					args.filename = argv[++i];
			}
			else if(strcmp(a, "-type") == 0){
					args.type = 1;
					args.typename = argv[++i];
			}
			else if(strcmp(a, "-perm") == 0){
					args.perm = 1;
					args.permname = argv[++i];
			}
			else if(strcmp(a, "-print") == 0){
					args.print = 1;
			}
			else if(strcmp(a, "-delete") == 0){
					args.delete = 1;
			}
			i++;
		}
	}
	else
	{
		args.name = 1;
		args.filename = argv[i];
		args.print = 1;
	}

	pid_t pid = fork();
	if (pid == 0) //filho
	{
		signal(SIGINT, &sigint_child_handler);
		signal(SIGUSR1, &sigusr_handler);
		readDirInfo(getNextDir(actualDir), &args);
		exit(0);
	}
	else if (pid > 0){ //parent
		int status;
		while(wait(&status) != pid);
	}
	return 0;

}
