/**********************************************************************
 * Copyright (c) 2021
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>

#include "types.h"
#include "list_head.h"
#include "parser.h"

//추가함
#include <sys/wait.h>
#include <sys/types.h>

struct entry {
   struct list_head list;
   char *string;
};

extern struct list_head history;

LIST_HEAD(history);	//이건 원래 있던거

/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */

static int run_command(int nr_tokens, char *tokens[])
{
	int status;		// child process의 종료 리턴 값
	pid_t pid, child_pid;	
	pid_t c1pid, c2pid;
	int pipefd[2];


	if (strcmp(tokens[0], "exit") == 0) return 0;


	char *pipe_before[MAX_NR_TOKENS] = { NULL };
	char *pipe_after[MAX_NR_TOKENS] = { NULL };
	int check = 0;
	int pipe_idx = 0;
	int j;


	for(j=0; tokens[j]!= NULL; j++){		// pipe있는지 체크
		if(strcmp(tokens[j],"|") == 0){
			check = 1;
			pipe_idx = j;
		}
	}

	if(check == 1){		//pipe 있으면,

		//pipe 앞 뒤 명령어 나눠서 넣어줌
		int i;
		for(i=0; i < pipe_idx; i++){	//앞 명령
			pipe_before[i] = tokens[i];
		} 
		pipe_before[i] = NULL;

		i = i+1;
		j = 0;
		while(tokens[i]!=NULL){		//NULL이면 빠져나옴		//뒤 명령
			pipe_after[j] = tokens[i];
			j++;
			i++;
		}
		pipe_after[j] = NULL;




		if(pipe(pipefd) == -1){
			perror("pipe");
			exit(EXIT_FAILURE);
		}	
								//write_end가 열려있으면  
								//read_end를 가지는 프로세스는
								//write_end가 닫히기 전까지 무한히 대기한다.
		c1pid = fork();			//pipefd[1]쓰기전용 pipefd[0] 읽기전용

		if(c1pid == 0){		//child

			close(pipefd[0]);	//read 닫기
			dup2(pipefd[1],1);
			close(pipefd[1]);	//write 닫기

			execvp(pipe_before[0],pipe_before);

		}else {				//parent

			c2pid =fork();

			if(c2pid == 0){

				close(pipefd[1]);
				dup2(pipefd[0],0);
				close(pipefd[0]);

				execvp(pipe_after[0],pipe_after);
				
			}else{

				close(pipefd[1]);

				wait(NULL);
				wait(NULL);
			}
						
		}

	}
	else{	//pipe 없으면,


		pid = fork(); 	// create child process
						// parent: pid=child의 PID, chile: pid=0, 실패시 -1

		if(pid > 0){	// parent임

			child_pid = pid;	// child_pid에 자식 PID 저장

			if (strcmp(tokens[0], "cd") == 0){	

				if ( nr_tokens > 1){
					chdir(tokens[1]);

					if(strncmp(tokens[1], "~", strlen("~")) == 0){
						chdir(getenv("HOME"));
					}

				}else{							
					chdir(getenv("HOME"));
				}
			
			}
			else if(strcmp(tokens[0], "history") == 0){		// history 호출 시 dump

				struct entry* temp;
				int index = 0;

    			list_for_each_entry(temp,&history, list) {
        			fprintf(stderr, "%2d: %s", index, temp->string);
					index++;
    			}

			}
			else if(strcmp(tokens[0],"!") == 0 ){

				int num = atoi(tokens[1]);
				int numcheck = 0;
				struct entry* temp1;
				char *tokens_1[MAX_NR_TOKENS] = { NULL };
				int nr_tokens_1 = 0;
				char *a = malloc(sizeof(char) * MAX_NR_TOKENS);

				list_for_each_entry(temp1, &history, list){

					if(numcheck == num){			
						strcpy(a, temp1->string);

						if (parse_command(a, &nr_tokens_1, tokens_1) == 0)
							return 1;

						return run_command(nr_tokens_1, tokens_1);

					}
					numcheck++;
				}

			}

			waitpid(pid, &status, 0);

		}
		else if(pid == 0){		//child

			if (strcmp(tokens[0], "cd") == 0 || strcmp(tokens[0], "history") == 0 || strcmp(tokens[0],"!") == 0 ){		// 내부적으로 처리한것들
					exit(0);
			}
			else if(execvp(tokens[0], tokens) == -1) {		// 외부적으로 execvp가 처리함

				fprintf(stderr, "Unable to execute %s\n", tokens[0]);	//처리 못하는 명령어
				exit(0);

			}
				
			exit(0);
		}
	}
	return -EINVAL;
}


/***********************************************************************
 * struct list_head history
 *
 * DESCRIPTION
 *   Use this list_head to store unlimited command history.
 */


/***********************************************************************
 * append_history()
 *
 * DESCRIPTION
 *   Append @command into the history. The appended command can be later
 *   recalled with "!" built-in command
 */
struct entry* node;

static void append_history(char * const command)
{
	node = malloc(sizeof(struct entry));
    node->string = (char*)malloc(sizeof(char) * MAX_COMMAND_LEN);
    
    strcpy(node->string, command);

    list_add_tail(&node->list, &history);
	//fprintf(stderr, "%s", node->string);
}


/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
static int initialize(int argc, char * const argv[])
{
	return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
static void finalize(int argc, char * const argv[])
{

}


/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******      */
/*          ****** BUT YOU MAY CALL SOME IF YOU WANT TO.. ******      */
static int __process_command(char * command)
{
	char *tokens[MAX_NR_TOKENS] = { NULL };
	int nr_tokens = 0;

	if (parse_command(command, &nr_tokens, tokens) == 0)
		return 1;
	
	return run_command(nr_tokens, tokens);
}

static bool __verbose = true;
static const char *__color_start = "[0;31;40m";
static const char *__color_end = "[0m";

static void __print_prompt(void)
{
	char *prompt = "$";
	if (!__verbose) return;

	fprintf(stderr, "%s%s%s ", __color_start, prompt, __color_end);
}

/***********************************************************************
 * main() of this program.
 */
int main(int argc, char * const argv[])
{
	char command[MAX_COMMAND_LEN] = { '\0' };
	int ret = 0;
	int opt;

	while ((opt = getopt(argc, argv, "qm")) != -1) {
		switch (opt) {
		case 'q':
			__verbose = false;
			break;
		case 'm':
			__color_start = __color_end = "\0";
			break;
		}
	}

	if ((ret = initialize(argc, argv))) return EXIT_FAILURE;

	/**
	 * Make stdin unbuffered to prevent ghost (buffered) inputs during
	 * abnormal exit after fork()
	 */
	setvbuf(stdin, NULL, _IONBF, 0);

	while (true) {
		__print_prompt();

		if (!fgets(command, sizeof(command), stdin)) break;

		append_history(command);
		ret = __process_command(command);
		
		if (!ret) break;
	}

	finalize(argc, argv);

	return EXIT_SUCCESS;
}
