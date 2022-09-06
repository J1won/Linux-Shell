#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <linux/limits.h>
#include <vector>
#include <string>
#include <fcntl.h>
#include "Tokenizer.h"

// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

using namespace std;

int main () {
    //create copies of stdin/stdout; dup()

    //dup()



    vector<pid_t> bgids;
    char backdir[256]; //parent directory
    char curdir[256]; //curr directory

    for (;;) { //this is a forever loop

        //implement iteration over vector of background pid (vector also declared outside loop)
        //waitpid() - using flag for non-blocking
        //implement date/time with TODO
        //implement username with getlogin()
        //implement curdir with getcwd()
        //
        time_t curtime;
        time(&curtime);
        tm *time_ptr = localtime(&curtime);
        char date[256];
        strftime(date, sizeof(date), "%b %d %T", time_ptr);
        getcwd(curdir, sizeof(curdir));
        // long size = PATH_MAX + 1;
        // char *buf = (char *)malloc((size_t)size);
        // need date/time, username, and absolute path to current dir
       // cout << YELLOW << "Shell$" << NC << " "; 
        cout << YELLOW << date << " " << getenv("USER") << ":" << curdir << "$" << NC << " ";

        
        // get user input command
        string input;
        getline(cin, input);


        if (input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }

        /////////////////////////////////////////////////////////////////////////////


        //chdir()
        //if dir (cd <dir>) is "-" then go to prev working directory
        //variable storing prev working dir (must be declared outside loop)

        // get tokenized commands from user input
        Tokenizer tknr(input); //commands that are split by "|"
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }


        //////************** This block helps us understand how tokenizer works
        // print out every command token-by-token on individual lines
        // prints to cerr to avoid influencing autograder
        // for (auto cmd : tknr.commands) {
        //     for (auto str : cmd->args) {
        //         cerr << "|" << str << "| ";
        //     }
        //     if (cmd->hasInput()) {
        //         cerr << "in< " << cmd->in_file << " ";
        //     }
        //     if (cmd->hasOutput()) {
        //         cerr << "out> " << cmd->out_file << " ";
        //     }
        //     cerr << endl;
        // }
        ///////************

        
         //add check for bg process - add pid to vector if bg and don't waitpid() in par



         
        int stdinpt1 = dup(0);
        int stdoutput1 = dup(1);
       
        for (int i = 0; i < (int)tknr.commands.size(); i++) 
        {

             /////////////////////   CD COMMANDS / directory handling  /////////////////////////////////////////
            if (tknr.commands.at(i) -> args.at(0) == "cd") {
                if(tknr.commands.at(i) -> args.at(1) == "-") {
                    //change directory to most recent one
                    chdir (backdir);
                }else
                {
                    //change directory to first argument
                    chdir(tknr.commands.at(i) -> args.at(1).c_str());
                }

                for(int i = 0; i < 256; i++) {
                    backdir[i] = curdir[i];
                }
                continue;
            }

            /////////////////////   PIPING   /////////////////////////////////////////

            //for piping
            //for cmd : command
            //      call pipe() to make pipe
            //      fork() in child, redirect stdout; in parent, redirect stdin
            //      ^ already written 
            //add checks for first/last command


            int fds[2]; //fds[0] = read end;  fds[1] = write.end 
            pipe(fds); //pipe with read and write end
            //int stdinpt1 = dup(0); //creates copy of file descriptor
            pid_t pid1 = fork();
            if (pid1 < 0) {  // error check
                perror("fork");
                exit(2);
            }
            if(pid1 == 0) //if child
            {

                //1. redirect output to next level
                //2. execute command at this level
                if(i < (int)(tknr.commands.size()-1)) //if not last command
                    dup2(fds[1], 1); //redirecting stdout to pipe write end
                else
                    dup2(stdoutput1, 1);

                ////////////args 
               
                char** args = new char*[tknr.commands.at(i) -> args.size()+1];
                for(int j = 0; j < (int)(tknr.commands.at(i) -> args.size()); j++)
                {
                    args[j] = (char*) tknr.commands.at(i) -> args.at(j).c_str();// fill args with all arguments of the command i
                }
               
                args[tknr.commands.at(i) -> args.size()] = nullptr; //last arg points to null

        
                if (tknr.commands.at(i)->hasInput()) 
                {
                    //fd of input file
                    int fd_inpt = open(tknr.commands.at(i)->in_file.c_str(), O_RDONLY);
                    //stdinput file points to in_file 
                    dup2(fd_inpt, 0);
                }
                
                if (tknr.commands.at(i)->hasOutput()) {
                    //fd of output file
                    int fd_inpt = open(tknr.commands.at(i)->out_file.c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                    //stdoutput file points to out_file 
                    dup2(fd_inpt, 1);
                }

                if (execvp(args[0], args) < 0) {  // error check
                    perror("execvp");
                    exit(2);
                }
            } //end of child process
            else //if parent
            {

                if (!tknr.commands.at(i)->isBackground()) 
                {
                    int status = 0;
                    waitpid(pid1, &status, 0);
                    if (status > 1) 
                    {  
                        exit(status);
                    }
                } else 
                {
                    bgids.push_back(pid1);
                }
                
                //redirect input from the child process
                dup2(fds[0], 0);

                close(fds[1]);
            } //end of parent process

            int status1 = 0;
            for (int b_sz = 0; b_sz < (int)bgids.size(); b_sz++) {
                //WNOHANG
                //collect the status of a dead process if there are any.
                //It prevents wait()/waitpid() from blocking so that your process can go on with other tasks
                waitpid(bgids.at(b_sz), &status1, WNOHANG);

                if (status1 > 0) {
                    bgids.erase(bgids.begin() + b_sz);
                    b_sz--;
                }
            }

        }
        dup2(stdinpt1, 0); //restores stdinpt1

    }
}
