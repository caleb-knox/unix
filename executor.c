#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include <sysexits.h>
#include<sys/wait.h>
#include "command.h"
#include "executor.h"

void execute_helper(struct tree	*t, int	fd_input, int fd_output, int *execute_status);

int execute(struct tree *t) {

   int x = 0, *execute_status = &x;
   execute_helper(t, STDIN_FILENO, STDOUT_FILENO, execute_status);
   return 0;

}

void execute_helper(struct tree *t, int fd_input, int fd_output, int *execute_status) {

   pid_t pid_none, pid_pipe_one, pid_pipe_two, pid_subshell;
   int status_none, status_subshell, pipe_fd[2];

   /* Check if there is no tree - base case. */
   if (t == NULL) {
      return;
   }

   /*print_tree(t);*/
   
   /* If an and execution fails. */
   if (*execute_status != -1) {

      /* NONE conjunction. */
      if (t->conjunction == NONE) {

         /* exit function. */
         if (strcmp(t->argv[0], "exit") == 0) {

            exit(0);

         /* cd function. */
         } else if (strcmp(t->argv[0], "cd") == 0) {

            /* If there is a location after cd. */
            if (t->argv[1] != NULL) {

               if (chdir(t->argv[1]) == -1) {
                  perror(t->argv[1]);
               }

            /* Just cd returns to the home location. */
            } else {

               if (chdir(getenv("HOME")) == -1) {
                  perror(getenv("HOME"));
               }

            }

         /* All other function calls. */
         } else {

            /* fork failed */
            if ((pid_none = fork()) < 0) {

               perror("fork");

            /* Child process. */
            } else if (pid_none == 0) {

               /* Input redirection. */
               if (t->input != NULL) {
                     
                  if ((fd_input = open(t->input, O_RDONLY)) < 0) {
                     perror("open");
                  }

                  if (dup2(fd_input, STDIN_FILENO) < 0) {
                     perror("dup2");
                  }

                  close(fd_input);

               }

               /* Output redirection. */
               if (t->output != NULL) {
                     
                  if ((fd_output = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, 0664)) < 0) {
                     perror("open");
                  }

                  if (dup2(fd_output, STDOUT_FILENO) < 0) {
                     perror("dup2");
                  }

                  if (close(fd_output) < 0) {
                     perror("close");
                  }

               }

               /* Any other function call. */
               if (execvp(t->argv[0], t->argv) < 0) {
                  fprintf(stderr, "Failed to execute %s\n", t->argv[0]);
                  fflush(stderr);
                  exit(1);
               }

            /* Parent process. */
            } else {

               wait(&status_none);

               /* Seeing if the function call failed. */
               if (WEXITSTATUS(status_none) != 0) {
                  *execute_status = -1;
               }

            }
         }
      
      /* AND conjunction. */
      } else if (t->conjunction == AND) {

         execute_helper(t->left, fd_input, fd_output, execute_status);
         execute_helper(t->right, fd_input, fd_output, execute_status);

      /* PIPE conjunction. */
      } else if (t->conjunction == PIPE) {

         /* Ambiguous redirection adjustment. */
         if ((t->right)->input != NULL && (t->left)->output == NULL) {
            printf("Ambiguous input redirect.\n");
            fflush(stdout);
         }

         if ((t->left)->output != NULL) {
            printf("Ambiguous output redirect.\n");
            fflush(stdout);
         }

         /* Input redirection. */
         if (t->input != NULL) {
                     
            if ((fd_input = open(t->input, O_RDONLY)) < 0) {
               perror("open");
            }

            if (dup2(fd_input, STDIN_FILENO) < 0) {
               perror("dup2");
            }

            if (close(fd_input) < 0) {
               perror("close");
            }

         }

         /* Output redirection. */
         if (t->output != NULL) {
                     
            if ((fd_output = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, 0664)) < 0) {
               perror("open");
            }

            if (dup2(fd_output, STDOUT_FILENO) < 0) {
               perror("dup2");
            }

            if (close(fd_output) < 0) {
               perror("close");
            }

         }

         if (pipe(pipe_fd) < 0) {
            perror("pipe");
            return;
         }

         if ((pid_pipe_one = fork()) < 0) {

            perror("fork");

         /* Child process. */ /* Left side of pipe. */
         } else if (pid_pipe_one == 0) {

            if (close(pipe_fd[0]) < 0) {
               perror("close");
            }

            if (dup2(pipe_fd[1], STDOUT_FILENO) < 0) {
               perror("dup2");
            }

            execute_helper(t->left, fd_input, pipe_fd[1], execute_status);

            if (close(pipe_fd[1]) < 0) {
               perror("close");
            }

            exit(0);

         /* Parent process. */
         } else {

            if ((pid_pipe_two = fork()) < 0) {

               perror("fork");

            /* Child process. */ /* Right side of pipe. */
            } else if (pid_pipe_two == 0) {

               if (close(pipe_fd[1]) < 0) {
                  perror("close");
               }

               if (dup2(pipe_fd[0], STDIN_FILENO) < 0) {
                  perror("dup2");
               }

               execute_helper(t->right, pipe_fd[0], fd_output, execute_status);

               if (close(pipe_fd[0]) < 0) {
                  perror("close");
               }

               exit(0);

            /* Parent process. */
            } else {

               if (close(pipe_fd[0]) < 0) {
                  perror("close");
               }

               if (close(pipe_fd[1]) < 0) {
                  perror("close");
               }

               wait(NULL);

            }

            wait(NULL);

         }

      /* SUBSHELL conjunction. */
      } else if (t->conjunction == SUBSHELL) {
         
         /* Input redirection. */
         if (t->input != NULL) {
                     
            if ((fd_input = open(t->input, O_RDONLY)) < 0) {
               perror("open");
            }

            if (dup2(fd_input, STDIN_FILENO) < 0) {
               perror("open");
            }

            if (close(fd_input) < 0) {
               perror("close");
            }

         }

         /* Output redirection. */
         if (t->output != NULL) {
                     
            if ((fd_output = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, 0664)) < 0) {
               perror("open");
            }

            if (dup2(fd_output, STDOUT_FILENO) < 0) {
               perror("open");
            }

            if (close(fd_output) < 0) {
               perror("close");
            }

         }

         /* Executes in its own process. */
         if ((pid_subshell = fork()) < 0) {

               perror("fork");

            /* Child process. */
            } else if (pid_subshell == 0) {

               execute_helper(t->left, fd_input, fd_output, execute_status);

               exit(0);

            /* Parent process. */
            } else {

               wait(&status_subshell);

            }
      }
   }
}
