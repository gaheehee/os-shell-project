## Project #1: My Powerful Shell

### Goal
With the system calls learned in the class and a few additional ones, you are ready to manipulate processes in the system.
Let's build my powerful shell with those system calls.


### Background
- *Shell* is a program that interpretes user inputs, and processes them accordingly. The example includes the Command Prompt in Windows, Bourne Shell in Linux, zsh in macOSX, so forth.

- An user can input a command by writing a sentence on the shell and press the "Enter" key. Upon receiving the input, the shell parses the requests into command *tokens*, and diverges execution according to the first token.

- The shell *always* assumes that the first token is the filename of the executable file to run except for `cd`, `history`, and `!` (see belows for the descriptions of those). The shell executes the executable with the rest of the tokens as the arguments. For example, if an user inputs `ls -al /home/sce213`, the shell will execute `ls` executable with `-al` and `/home/sce213` as its arguments. The shell may execute the executable using the -p variant of the `exec()` system call family so that the executable file is automatically checked from the *`$PATH`* environment variable.


### Problem Specification
- The shell program `posh` (**PO**werful **SH**ell) awaits your command line input after printing out "$" as the prompt. When you enter a line of command, the framework tokenizes the command line with `parse_command()` function, and the framework calls `run_command()` function with the tokens. You need to implement following features starting in `run_command()`.

- Currently, the shell keeps getting input commands and processes them until the user enters `exit`. In that case, the shell program exits.


### <mark>Outline how programs are launched and how arguments are passed</mark>  

쉘에 입력을 하면, append_history함수를 통해 list_head를 이용하여 입력문자열을 history안에 차
곡차곡 쌓고, __process_command, parse_command 함수를 통해 입력 문자열을 token으로 만든다.
이렇게 토큰화 된 것을 run_command함수로 보내어 호출하게 되는데, run_command에서는 가장
첫번째 token이 어떤 명령어인지 체크를 한다. 만약 명령어가 exit라면 쉘을 나가도록 하고, 아니
라면 token중에 | 가 있는지 체크한다. |가 있다면 pipe를 처리하고, 없다면 fork()를 하여 부모 프
로세스에서는 내부적으로 cd, history, ! 명령어를 처리해주고 나머지 명령어들은 자식 프로세스에
서 execvp를 통해 처리해준다. 그리고 부모 프로세스는 자식이 다 처리하고 죽을 때까지 wait한
다.  
	
### <mark>How the command history is maintained and replayed later</mark>  

쉘에서 입력을 받으면 parse하기 전에 append_history함수를 통하여 list_add_tail를 이용하여 list
에 입력을 쌓게 된다. history명령이 쉘에 들어오면, run_command함수에서 첫번째 토큰이 “history”
인지 체크하고 history 명령어라면 list_for_each_entry를 통해 이전에 쌓아 놓은 이전 입력(명령) 
list를 순회하면서 차근차근 하나씩 이전 입력들을 출력하게 된다.
그리고 ! number 명령어 입력을 받으면 run_command에서 ! 명령어인지 체크하고, 맞다면
history list를 순회하면서 number번째 순회에서 이전 명령문자열을 뽑아내어 parse하여 토큰으로
나눈 후, run_command에 토큰을 넣어서 호출시켜 이전 명령어를 실행시켜준다. 이렇게 history에
서 원하는 이전 명령을 ! number 명령어를 통해 재실행시킬 수 있다.  

### <mark>My STRATEGY to implement the pipe</mark>  

우선 명령어에 | 가 있다면, | 앞의 명령어와 | 뒤의 명령어로 나누어 주었다. 그리고나서 파이프
를 하나 생성하였고, fork()를 하여 그 자식 프로세스는 read를 닫고, dup2를 이용하여 파이프의
다른 한쪽에 출력이 쓰여지도록 하였다. 그리고 | 앞의 명령어를 execvp를 이용해 수행하였다. 따
라서 | 앞의 명령어를 수행한 결과를 다른 한쪽의 파이프에서 읽을 수 있게 되었다.
부모프로세스에서는 fork()를 한번 더 하여 자식 프로세스를 하나 더 생성하였다. 이 자식 프로세
스가 write를 닫고, dup2를 이용하여 아까 자식 프로세스가 쓴 출력을 파이프에서 읽어와서 그걸
입력으로 하는 | 뒤에 명령어를 execvp를 통하여 처리하였다.
이렇게 fork()를 두번하여 첫번째 자식 프로세스에서는 | 앞의 명령어를 처리하여 파이프에 넣고,
부모안의 두번째 자식 프로세스에서는 파이프에서 결과를 읽어와 이 결과를 입력으로 하여 | 뒤
의 명령어를 처리하였다.  

### <mark>AND lessons learned</mark>  

Run_command exec을 짜면서, 외부적으로 명령을 처리하는 것, 즉 execvp는 자식 프로세스에서
써야 한다는 것을 알게 되었다.
그리고 pipe를 구현하면서 dup2를 통하여 결과를 다른 쪽에 출력되도록 조정할 수 있음을 알게
되었고, close의 중요성을 알게 되었다.
Close를 제대로 하지 않아 많은 시행착오를 겪었고 그 시행착오 덕분에 확실히 알게 된 것은
write가 열려 있으면 read를 하는 프로세스는 write가 닫히기 전까지 무한하게 대기한다는 것이다.
그리고 이론만으로는 개념이 머리에 확실하게 박히지 않았던 부분들을 확실하게 알아갈 수 있다.  


