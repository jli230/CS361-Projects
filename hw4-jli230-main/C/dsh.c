#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/wait.h>

void exec_command(char* command) {
    char *sep;
    char* program = strtok(command," ");
    char *args[16]={program};
    int i=1;

    // TODO: search the path instead of running "program directly"
    while((args[i++]=strtok(NULL," ")));
    for (int j = 1; j < i-1; j++) {
        if (strcmp(args[j], "<") == 0) {
            int fd = open(args[j+1], O_CREAT | O_RDWR , 0755);
            if (dup2(fd, STDIN_FILENO) != -1) {
                close(fd);
                args[j]=0;
            } else {
                fprintf(stderr, "Error in STDIN\n");
            }
        } else if (strcmp(args[j], ">") == 0) {
            int fd = open(args[j+1], O_CREAT | O_RDWR , 0755);
            if (dup2(fd, STDOUT_FILENO) != -1) {
                close(fd);
                args[j]=0;
            } else {
                fprintf(stderr, "Error in STDOUT\n");
            }
        } else if (strcmp(args[j], "2>") == 0) {
            int fd = open(args[j+1], O_CREAT | O_RDWR , 0755);
            if (dup2(fd, STDERR_FILENO) != -1) {
                close(fd);
                args[j]=0;
            } else {
                fprintf(stderr, "Error in STDERR\n");
            }
        }
    }


    char* path = getenv("PATH");
    char* paths = strtok(path, ":");

    char *envp[] =
    {
        "HOME=/",
        "PATH=/bin:/usr/bin",
    };
    while (paths != NULL) { 
        //printf("path-loop %s\n", paths);
        char pathcomm[100];
        strcpy(pathcomm, paths);
        strcat(pathcomm, "/");
        //printf("Paths: %s\n", pathcomm);
        execve(strcat(pathcomm, program),args, envp);
        paths = strtok(NULL, ":");
    }
    fprintf(stderr,"dsh: command not found: %s\n",program);
    exit(0);
}

void run(char*);
void run_pipeline(char* head, char* tail) {
    int fd[2];
    pipe(fd);
    int origin=dup(0);
    int origout=dup(1);
    if(!fork()) {
        //printf("Forking once\n");
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        exec_command(head);
    } else {
        wait(0);
    }
    dup2(fd[0], STDIN_FILENO);
    close(fd[1]);
    run(tail+1);
    close(fd[0]);
    dup2(origin, 0);
    //fprintf(stderr,"Uh-oh, I don't know how to do pipes.");
}

void run_sequence(char* head, char* tail) { 
    run(head);
    run(tail);
    //fprintf(stderr,"Uh-oh, I don't know how to do sequences.");
}

void run(char *line) {
    //printf("Running line :%s\n", line);
    char *sep;
    char *sep2;
    if((sep=strstr(line,";"))) {
        *sep=0;        
        run_sequence(line,sep+1);
    }
    else if((sep=strstr(line,"|"))) {
        *sep=0;        
        run_pipeline(line,sep+1);
    } 
    /* else if((sep=strstr(line,"<"))) {
        int chain = 0;
        if ((sep2=strstr(sep,"<")) || (sep2=strstr(sep,">"))) {
            *sep2=0;
            chain = 1;
        }

        *sep = 0;
        //printf("Line fed:%s\n", sep+2);
        
        int fd = open(sep+2, O_CREAT | O_RDWR | O_APPEND, 0755);
        if (dup2(fd, STDIN_FILENO) != -1) {
            close(fd);
            //fprintf(stderr, "Error in STDOUT\n");
            //printf("%s %s", line, sep+1);
            //fflush(NULL);
            // cat < hi.txt > hello.txt 2> error.txt
            if (!fork()) 
                run(line);
            else wait(0);
            if (chain==1) {
                run(sep2+1);
            }
        } else {
            fprintf(stderr, "Error in STDIN\n");
        }
    } else if((sep=strstr(line,">"))) {
        int chain = 0;
        if ((sep2=strstr(sep,"<")) || (sep2=strstr(sep,">"))) {
            *sep2=0;
            chain = 1;
        }
        // open file (sep+1)
        // dup2(fd, number for stdout)
        *sep = 0;
        //printf("Line fed:%s\n", sep+2);
        // char filedest[200]; 
        // snprintf(filedest,200,"%s/%s",getenv("PWD"), sep+2);
        int fd = open(sep+2, O_CREAT | O_RDWR | O_APPEND, 0755);
        if (dup2(fd, STDOUT_FILENO) != -1) {
            dup2(fd, STDERR_FILENO);
            close(fd);
            //fprintf(stderr, "Error in STDOUT\n");
            //printf("%s %s", line, sep+1);
            //fflush(NULL);
            if (!fork()) 
                run(line);
            else wait(0);
            if (chain==1) {
                run(sep2+1);
            }
        } else {
            fprintf(stderr, "Error in STDOUT\n");
        }
    } */
    else {
        // printf("Executing command\n");
        if(!fork()) {
         exec_command(line);
        }
        else wait(0);        
    }
}


int main(int argc, char** argv) {
    char *line=0;
    size_t size=0;

    char folder[100];
    snprintf(folder,100,"%s/.dsh",getenv("HOME"));
    mkdir(folder,0755);
    // TODO: need to create the appropriate session folder
    //       to put our <N>.stdout and <N>.stderr files in.
    int counter = 0;
    while(1) {
        snprintf(folder,100,"%s/.dsh/%d",getenv("HOME"), counter);
        
        if (mkdir(folder,0755) == -1) {
            counter++;
        } else {
            break;
        }
    }
    char filedest[200];
    char filedesterr[200];
    int filecount = 0;
    snprintf(filedest,200,"%s/%d.stdout",folder, filecount);
    snprintf(filedesterr,200,"%s/%d.stderr",folder, filecount);
    // printf("%s\n", folder);
    // printf("%s\n", filedest);
    // printf("%i\n", argc);
    // printf("%s\n", argv[1]);
    //printf("dsh> ");
    // handy copies of original file descriptors
    int origin=dup(0);
    int origout=dup(1);
    int origerr=dup(2);


    while(getline(&line,&size,stdin) > 0) {
        // TODO: temporarily redirect stdio fds to
        //       files. This will be inherited by children.
        // commented code block below is suppose to redirect terminal output to <N>.stdout file where N is the number of times
        // the while loop has executed, but the snprintf line appears to cause the exec_command function to not 
        // run the command fed into it
        // printf("Line fed: %s\n", line);
        // printf("File destination: %s\n", filedest);
        int fd = open(filedest, O_CREAT | O_RDWR , 0755);
        int fderr = open(filedesterr, O_CREAT | O_RDWR , 0755);
        int fdin = open("/dev/null", O_WRONLY);
        if (dup2(fd, STDOUT_FILENO) != -1) {
            dup2(fdin, STDIN_FILENO);
            dup2(fderr, STDERR_FILENO);
            close(fderr);
            close(fdin);
            close(fd);
            // if (!fork()) {
            line[strlen(line)-1]=0; // kill the newline
            run(line);
        //     // }
        //     // else wait(0);
        } else {
            fprintf(stderr, "Error in creating child process\n");
            printf("Error in creating child process\n");
            exit(0);
        }

        // TODO: restore the stdio fds before interacting
        //       with the user again
        dup2(origin, 0);
        dup2(origout, 1);
        dup2(origerr, 2);
        filecount++;
        snprintf(filedest,200,"%s/%d.stdout",folder, filecount);
        snprintf(filedesterr,200,"%s/%d.stderr",folder, filecount);
        //printf("dsh> ");
   }
   printf("Exiting program...\n");
}
