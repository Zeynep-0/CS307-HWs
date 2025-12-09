#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h> 
#include <fcntl.h> 
#include "parser.h"
#include <stdbool.h>

int main(){
    sparser_t parser;
    if(!initParser(&parser)){
        return 0;
    }
    char* line=NULL;
    size_t size=0;
    while(true){
        printf("SUShell$ ");
        fflush(stdout);
        int get=getline(&line,&size,stdin);
        if(get==-1){
            break;
        }
        if(get==1){
            continue;
        }
        if (get > 0 && line[get - 1] == '\n') {
            line[get - 1] = '\0';
        }
        compiledCmd C;
      
        if(compileCommand (&parser, line ,&C)==0){
            printf ("parse error");
            freeCompiledCmd(&C);
            continue;
        }
        if (C.isQuit) {
            printf("Exiting shell...\n");
            freeCompiledCmd(&C);
            break;
        }
        int inp_file=STDIN_FILENO;
        int out_file=STDOUT_FILENO;
       
        if(C.before.argvs != NULL ||C.after.argvs!=NULL || C.inLoop.argvs != NULL){
            int pipe_len=C.before.n+C.after.n+(C.inLoop.n)*C.loopLen;
            if(pipe_len==1){
                int rc=fork();
                if(rc==0){ 
                    if(C.inFile!=NULL){
                        inp_file=open(C.inFile, O_RDONLY);
                        if(inp_file==-1){
                            freeCompiledCmd(&C);
                            exit(1);
                        }
                        dup2(inp_file,STDIN_FILENO);
                        close(inp_file);
                    }
                    if(C.outFile!=NULL){
                        out_file=open(C.outFile, O_WRONLY | O_TRUNC | O_CREAT, 0644);
                        if(out_file==-1){
                            freeCompiledCmd(&C);
                            exit(1);
                        }
                        dup2(out_file,STDOUT_FILENO);
                        close(out_file);
                    }
                    if(C.before.argvs != NULL){
                        execvp(C.before.argvs[0][0],C.before.argvs[0]);
                        exit(1);
                    }
                    if(C.inLoop.argvs != NULL){
                        execvp(C.inLoop.argvs[0][0],C.inLoop.argvs[0]);
                        exit(1);
                    }
                    if(C.after.argvs != NULL){
                        execvp(C.after.argvs[0][0],C.after.argvs[0]);
                        exit(1);
                    }
                }
                else{
                    wait(NULL);
                }
            }else{
                
                int pipes[pipe_len - 1][2];
                for(int i=0;i<pipe_len-1;i=i+1){
                   if (pipe(pipes[i]) == -1) {
                        exit(1);
                    }
                }
                for(int i=0;i<pipe_len;i=i+1){
                    int rc1=fork();
                    if(rc1==0){
                        if (i > 0) {
                                dup2(pipes[i - 1][0], STDIN_FILENO);
                            }

                        if (i < pipe_len - 1) {
                                dup2(pipes[i][1], STDOUT_FILENO);

                        }
                        for (int j = 0; j < pipe_len - 1; j++) {
                                close(pipes[j][0]);
                                close(pipes[j][1]);
                        }                             
                       if(i==0 && C.inFile!=NULL){
                        inp_file=open(C.inFile, O_RDONLY);
                        if(inp_file==-1){
                            freeCompiledCmd(&C);
                            exit(1);
                        }
                        dup2(inp_file,STDIN_FILENO);
                        close(inp_file);
                    }
                    if(i==pipe_len-1 && C.outFile!=NULL){
                        out_file=open(C.outFile, O_WRONLY | O_TRUNC | O_CREAT,0644);
                        if(out_file==-1){
                            freeCompiledCmd(&C);
                            exit(1);
                        }
                        dup2(out_file,STDOUT_FILENO);
                        close(out_file);
                    }

                        if(C.before.argvs != NULL && i< (int)C.before.n){
                            execvp(C.before.argvs[i][0],C.before.argvs[i]);
                            exit(1);
                        }
                        if(C.inLoop.n!=0 && i >= (int)C.before.n && i<(int)(C.before.n+(C.inLoop.n*C.loopLen))){
                            
                            int pos = i - C.before.n;       
                            int cmd_index = pos % C.inLoop.n;    
                            execvp(C.inLoop.argvs[cmd_index][0], C.inLoop.argvs[cmd_index]);                            
                            exit(1);
                        }
                        if(C.after.n != 0 && i>=(int)(C.before.n+(C.inLoop.n*C.loopLen))){
                            execvp(C.after.argvs[i-C.before.n-(C.inLoop.n*C.loopLen)][0],C.after.argvs[i-C.before.n-(C.inLoop.n*C.loopLen)]);
                            exit(1);
                        }
                    }
                    else if (rc1 < 0) {
                        exit(1);
                    } 
                }
                for (int i = 0; i < pipe_len-1; i++) {
                    close(pipes[i][0]);
                    close(pipes[i][1]);
                }
                for (int i = 0; i < pipe_len; i++) {
                    wait(NULL);
                }

            }
        }
        freeCompiledCmd(&C);
    }
    free(line);
    freeParser(&parser);
    return 0;
}