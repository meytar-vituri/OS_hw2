#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define BGPROCESS '&'
#define PIPEPROCESS '|'


int set_signal_handling(sig_t new_SIG) {
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_handler = new_SIG;
    return sigaction(SIGINT, &new_action, NULL);
}


int is_piping(int count, char **arglist) {
    int index = -1;
    for (int i = 0; i < count; i++) {
        if (*arglist[i] == PIPEPROCESS) {
            index = i;
            break;
        }
    }
    return index;
}

void exit_with_err(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void execute(char *filename, char **arglist) {
    int running_process = execvp(filename, arglist);
    if (running_process == -1) {
        exit_with_err("Failed excuting");
    }
}

void run_in_background(int count, char **arglist) {
    int pid = fork();
    if (pid == -1) {
        exit_with_err("Failed forking");
    } else if (pid == 0) {
        set_signal_handling(SIG_DFL);
        execute(arglist[0], arglist);
    }
}

void simple_run(int count, char **arglist) {
    int pid = fork();
    if (pid == -1) {
        exit_with_err("Failed forking");
    } else if (pid == 0) {
        set_signal_handling(SIG_DFL);
        execute(arglist[0], arglist);
    } else {
        int status;
        int pid = wait(&status);
        if (WIFEXITED(status)) {
            printf("Child: %d, Exit code: %d \n", pid, WEXITSTATUS(status));
        }
        exit(EXIT_SUCCESS);
    }
}

void pipe_running(int count, char **arglist, int index) {
    arglist[index] = NULL;
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        exit_with_err("Failed piping");
    }
    int readerfd = pipefd[0];
    int writerfd = pipefd[1];
    int pid_writer = fork();
    if (pid_writer == -1) {
        close(readerfd);
        close(writerfd);
        exit_with_err("Failed forking writer");
    } else if (pid_writer == 0) {
        //child process, the writer
        close(readerfd);
        if (dup2(writerfd, STDOUT_FILENO) == -1) {
            exit_with_err("failed duplicating the writer process");
        }
        set_signal_handling(SIG_DFL);
        close(writerfd);
        execute(arglist[0], arglist);
        //waitpid(pid_writer, NULL, 0);
    }
    //parent process, creating another child and using it as a reader
    int pid_reader = fork();
    if (pid_reader == -1) {
        close(readerfd);
        close(writerfd);
        exit_with_err("Failed forking reader");
    } else if (pid_reader == 0) {
        close(writerfd);
        if (dup2(readerfd, STDIN_FILENO) == -1) {
            exit_with_err("failed duplicating the reader process");
        }
        set_signal_handling(SIG_DFL);
        close(readerfd);
        execute(arglist[index + 1], arglist + index + 1);
        //waitpid(readerfd, NULL, 0);
    }


    close(writerfd);
    waitpid(pid_writer, NULL, 0);
    close(readerfd);
    waitpid(pid_reader, NULL, 0);
    exit(EXIT_SUCCESS);
}


int prepare(void) {
    if (set_signal_handling(SIG_IGN) == 0) {
        return 0;
    }
    return -1;
}


int process_arglist(int count, char **arglist) {
    if (*arglist[count-1] == BGPROCESS) {
        arglist[count-1] = NULL;
        run_in_background(count - 1, arglist);
    } else if ((is_piping(count, arglist)) != -1) {
        int index = is_piping(count, arglist);
        pipe_running(count, arglist, index);
    } else{
        simple_run(count, arglist);
    }
    //TODO: wait for all processes to complete
    return 1;
}

int finalize(void){
    return 0;
}




