#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>

extern char **getaline();

/*
 * Handle exit signals from child processes
 */
void sig_handler(int signal) {
	int status;
	pid_t pid;
	
	if(signal == SIGTTOU){
		printf("SIGTTOU\n");
	}
	if(signal == SIGTTIN){
		printf("SIGTTIN\n");
	}
	if (signal == SIGCHLD){
		while((pid = waitpid(-1, &status, WNOHANG)) > 0);		
	}
}

/*
 * The main shell function
 */ 
main() {
	int i;
	int m;
	char **args; 
	int result;
	int block;
	int output;
	int input;
	int append;
	char *output_filename;
	char *input_filename;
	char *append_filename;
	char **argscurr;
	int go;
	int pipenum;
	int pipef;
	int blocklast;
	int k;

	//setup structure to be sent with signal action.
	struct sigaction act;
	sigset_t set;

	
	sigemptyset(&set);
	sigaddset(&set, SIGTTOU);
	sigaddset(&set, SIGTSTP);
	sigaddset(&set, SIGTTIN);

	act.sa_mask = set;
	act.sa_flags = SA_RESTART;

	//Make sure all signals are set to default
	for (i=0; i<32; i++) {
		sigset(i, SIG_DFL);
	}

	act.sa_handler = sig_handler;

	//Setup signal handler
	sigaction(SIGCHLD, &act, NULL);
	sigaction(SIGTTOU, &act, NULL);
	
	// Shell start
	while(1) {
			
		printf("myshell>");
		args = getaline();
		argscurr = &args[0];
		go = 1;
		pipef = 0;
		pipenum = 0;
		block = 1;
		
		 // Check for an ampersand
		if(args[0] != NULL){
			blocklast = (ampersand(args) == 0);
		}		
		
		//Parse the arguments looking for &&, ||, |, ,;
		for(m=0; args[m] != NULL;m++){		
			if(strcmp(args[m], "&") == 0 && strcmp(args[m+1], "&") == 0) {
				free(args[m]);
				free(args[m+1]);
				args[m] = NULL;
				args[m+1] = NULL;
				
				if(pipef){
					if (go) {
						if (pipeRedirection(argscurr, block, input, input_filename, output, output_filename, append, append_filename) == 1 ) {
							go = 0; 
							}
						}
					} else {
						if (go) {
							if (dcRedirection(argscurr, block, input, input_filename, output, output_filename, append, append_filename) == 1 ) {
								go = 0; 
							}
						}
					}

				argscurr = &args[m+2];		
				m++;
				pipef = 0;
				
			} else if(strcmp(args[m], "|") == 0 && strcmp(args[m+1], "|") == 0) { 
				free(args[m]);
				free(args[m+1]);
				args[m] = NULL;
				args[m+1] = NULL;
				
				if(pipef){
					if (go) {					
						if (pipeRedirection(argscurr, block, input, input_filename, output, output_filename, append, append_filename) != 1 ) {
							go = 0; 
						}
					}				
				} else{
					if (go) {
						if (dcRedirection(argscurr, block, input, input_filename, output, output_filename, append, append_filename) != 1 ) {
							go = 0; 
						}
					}
				}
				argscurr = &args[m+2];
				m++;
				pipef = 0;	
			} else if(strcmp(args[m], ";") == 0) {
				free(args[m]);
				args[m] = NULL;
				
				if(pipef){
					if (go) {
						pipeRedirection(argscurr, block, input, input_filename, output, output_filename, append, append_filename);
					}
				} else{
					if (go) {
						dcRedirection(argscurr, block, input, input_filename, output, output_filename, append, append_filename);
					}				
				}
				
				argscurr = &args[m+1];
				go = 1;
				pipef = 0;
			} else if(strcmp(args[m], "|") == 0) {
				pipenum++;
				go = 1;
				pipef = 1;				
			}
				k++;
		}
		//grab result for part
		if(pipef){
			if (go) {
				pipeRedirection(argscurr, blocklast, input, input_filename, output, output_filename, append, append_filename);
			}				
		}else{
			if (go) {
				dcRedirection(argscurr, blocklast, input, input_filename, output, output_filename, append, append_filename);
			}				
		}  
		
		//free args
		for(k=m-1; k>=0;k--){
			free(args[k]);
		}
	
	} // End of while shell 
}


int pipeRedirection(char **args, int block, int input, char *input_filename, int output, char *output_filename, int append, char *append_filename) {
	// No input, continue
    if(args[0] == NULL) 
      return(0);

    // Check for internal shell commands, such as exit
    if(internal_command(args))
      return(0);		   

	// Check for redirected input
    input = redirect_input(args, &input_filename);

    switch(input) {
    case -1:
      printf("Syntax error!\n");
	  return(-1);
      break;
    case 0:
      break;
    case 1:
      printf("Redirecting input from: %s\n", input_filename);
      break;
    }
	
	// Check for appended output
	append = redirect_append(args, &append_filename);
	
	switch(append) {
	case -1:
      printf("Syntax error!\n");
      return(-1);
      break;
    case 0:
      break;
    case 1:
      printf("Appending output to: %s\n", append_filename);
      break;
    }
	
    // Check for redirected output
    output = redirect_output(args, &output_filename);

    switch(output) {
    case -1:
      printf("Syntax error!\n");
      return(-1);
      break;
    case 0:
      break;
    case 1:
      printf("Redirecting output to: %s\n", output_filename);
      break;
    }

    // Do the command
	return(do_pipe_command(args, block, 
	       input, input_filename, 
	       output, output_filename,
		   append, append_filename));

}

/*
	Execute for arguments that will be piped together. It returns if the action was successful.
*/
int do_pipe_command(char **args, int block, int input, char *input_filename, int output, char *output_filename, int append, char *append_filename) {			   
	int result;
	char **argscurr;
	int x;
	int i,j;
    int pipe_count = 0;
    int fd[100][2],child_pid,status;
	int cmdIndex[50];
	cmdIndex[0] = 0;	
	
	for(x = 0; args[x] != NULL; x++) {
		if(strcmp(args[x], "|") == 0) {
			pipe_count++;
			cmdIndex[pipe_count] = x+1;
			free(args[x]);
			args[x] = NULL;
			  
		}
	}
	
	argscurr = &args[0];
	
    if(pipe_count) {		
        for(i = 0;i < pipe_count;i++){
			// Set up all pipes
            pipe(fd[i]);
        }

        for(i = 0;i <= pipe_count;i++){		
			// Fork a child for each pipe+1 (add one for last cmd)				
            child_pid = fork();
			
			// Check for error in for
			switch(child_pid) {
				case EAGAIN:
					perror("Error EAGAIN: ");
					return;
				case ENOMEM:
					perror("Error ENOMEM: ");
					return;
			} 

            if(!child_pid) {
				// If you're not the first child, replace your stdin with read of the previous pipe
                if(i!=0) {
					if( dup2(fd[i-1][0],0) == -1){
						perror("Error pipe: ");
					}
                }
				// If you're not the last child, replace your stdout with write of next pipe
                if(i!=pipe_count){					
                    if( dup2(fd[i][1],1) == -1){
						perror("Error pipe: ");
					}
                }				
                for(j = 0;j < pipe_count;j++) {   
                        close(fd[j][0]);
                        close(fd[j][1]);
				}
				
				argscurr = &args[cmdIndex[i]];
			
				if (i == pipe_count) {
					if(input)
						freopen(input_filename, "r", stdin);

					if(output)
						freopen(output_filename, "w+", stdout);
  
					if(append)
						freopen(append_filename, "a", stdout);
				}
				
				result = execvp(args[cmdIndex[i]], argscurr);				
				
				if(result == -1) {
					exit(1);
				}
				
				exit(0);
            }
        }
		
        for(i = 0;i < pipe_count;i++)  {
            close(fd[i][0]);
            close(fd[i][1]);
        }
		
		if (waitpid(child_pid, &status, 0) != -1) {
			if (WIFEXITED(status)) {
				result = WEXITSTATUS(status);
			} else {
				printf("Unexpected Result.\n");
			}
		} else {
			perror("waitpid  failed");
		}
		
    } else {
		/*** TODO: May not need ***/
        result = execvp(args[0], args);
    }
	
    return result;	
}

/*
Prepares the argument section to be sent to do_command by checking for redirects, internal commands, or errors.
*/
int dcRedirection(char **args, int block, int input, char *input_filename, int output, char *output_filename, int append, char *append_filename) {
	// No input, continue
    if(args[0] == NULL) 
      return(0);

    // Check for internal shell commands, such as exit
    if(internal_command(args))
      return(0);	   

	// Check for redirected input
    input = redirect_input(args, &input_filename);

    switch(input) {
    case -1:
      printf("Syntax error!\n");
	  return(-1);
      break;
    case 0:
      break;
    case 1:
      printf("Redirecting input from: %s\n", input_filename);
      break;
    }
	
	// Check for appended output
	append = redirect_append(args, &append_filename);
	
	switch(append) {
	case -1:
      printf("Syntax error!\n");
      return(-1);
      break;
    case 0:
      break;
    case 1:
      printf("Appending output to: %s\n", append_filename);
      break;
    }
	
    // Check for redirected output
    output = redirect_output(args, &output_filename);

    switch(output) {
    case -1:
      printf("Syntax error!\n");
      return(-1);
      break;
    case 0:
      break;
    case 1:
      printf("Redirecting output to: %s\n", output_filename);
      break;
    }

    // Do the command
	return(do_command(args, block, 
	       input, input_filename, 
	       output, output_filename,
		   append, append_filename));
}

/*
 * Check for ampersand as the last argument
 */
int ampersand(char **args) {
  int i;
  
  for(i = 1; args[i] != NULL; i++) ;

  if(args[i-1][0] == '&') {
    free(args[i-1]);
    args[i-1] = NULL;
    return 1;
  } else {
    return 0;
  }
  
  return 0;
}

/* 
 * Check for internal commands
 * Returns true if there is more to do, false otherwise 
 */
int internal_command(char **args) {
  if(strcmp(args[0], "exit") == 0) {
    exit(0);
  }
  if(strcmp(args[0], "cd") == 0) {
	chdir(args[1]);
  }
  if(strcmp(args[0], "history") == 0) {
	// todo implement 
  }

  return 0;
}

/* 
 * Do the command and create new processes to be executed. The children set their status based on if their action was successful and the parent catches that status and returns it.
 */
int do_command(char **args, int block, int input, char *input_filename, int output, char *output_filename, int append, char *append_filename) {
	int status;
	int result;
	int parent_id;
	pid_t child_id;

	// Fork the child process
	parent_id = getpid();
	child_id = fork();

	// Check for errors in fork()
	switch(child_id) {
		case EAGAIN:
			perror("Error EAGAIN: ");
			return;
		case ENOMEM:
			perror("Error ENOMEM: ");
		return;
	}

	if(child_id == 0) {
		setpgid(0, 0);

		// Set up redirection in the child process
		if(input)
			freopen(input_filename, "r", stdin);

		if(output)
			freopen(output_filename, "w+", stdout);
  
		if(append)
			freopen(append_filename, "a", stdout);

		// Execute the command
		result = execvp(args[0], args);
	
		if(result == -1){
			exit(1);
		}
	
		exit(0);
	
	} else {
		//this is the parent process
	    setpgid(child_id, child_id);
		setpgid(0, 0);		
		
		if(block) {
			//ignore signals
			sigset(SIGTTOU, SIG_IGN);
			sigset(SIGTTIN, SIG_IGN);
			
			//give terminal control to the child process
			tcsetpgrp(0, child_id);
			
			if (waitpid(child_id, &status, 0) != -1) {
				if (WIFEXITED(status)) {
					result = WEXITSTATUS(status);
				} else {
					printf("Unexpected Result.\n");
				}
			} else {
				perror("waitpid  failed");
			}
			//give terminal control back to parent
			tcsetpgrp(0, parent_id);
			//set previously ignored signals back to default
			sigset(SIGTTOU, SIG_DFL);
			sigset(SIGTTIN, SIG_DFL);
			
			return result;
		}
	}
}

/*
 * Check for input redirection
 */
int redirect_input(char **args, char **input_filename) {
	int i;
	int j;

	for(i = 0; args[i] != NULL; i++) {
		// Look for the <
		if(args[i][0] == '<') {
			free(args[i]);

			// Read the filename
			if(args[i+1] != NULL) {
				*input_filename = args[i+1];
			} else {
				return -1;
			}

			// Adjust the rest of the arguments in the array
			for(j = i; args[j-1] != NULL; j++) {
				args[j] = args[j+2];
			}

			return 1;
		}
	}

	return 0;
}

/*
 * Check for output redirection
 */
int redirect_output(char **args, char **output_filename) {
	int i;
	int j;

	for(i = 0; args[i] != NULL; i++) {
		// Look for the >
		if(args[i][0] == '>') {			
			free(args[i]);

			// Get the filename 
			if(args[i+1] != NULL) {
				*output_filename = args[i+1];
			} else {
				return -1;
			}

			// Adjust the rest of the arguments in the array
			for(j = i; args[j-1] != NULL; j++) {
				args[j] = args[j+2];
			}

			return 1;
		}
	}

	return 0;
}

/*
 * Check for output redirection
 */
int redirect_append(char **args, char **append_filename) {
	int i;
	int j;

	for(i = 0; args[i] != NULL; i++) {
		// Look for the >
		if(args[i][0] == '>' && args[i+1][0] == '>') {
			free(args[i]);
			free(args[i+1]);

			// Get the filename 
			if(args[i+2] != NULL) {
				*append_filename = args[i+2];
			} else {
				return -1;
			}

			// Adjust the rest of the arguments in the array
			for(j = i; args[j-1] != NULL; j++) {
				args[j] = args[j+3];
			}	

			return 1;
		}
	}

	return 0;
}

