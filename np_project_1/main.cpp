//
//  main.cpp
//  np_project_1
//
//  Created by 胡育旋 on 2018/10/11.
//  Copyright © 2018 胡育旋. All rights reserved.
//

#include <iostream>

#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <algorithm>

#include <vector>
#include <set>
#include <deque>
#include <queue>

#include "utility.h"

#define bin_path "/Users/huyuxuan/Desktop/np_project_1/np_project_1/bin"

using namespace std;

struct command{
    string name;
    vector<string> arguments;
    bool before_numbered_pipe = false;
    bool need_pipe_out = false;
    int pipe_arr[2]={0,1};
    int exe_line_num = 0;
    string output_type;//stdout or both(stdout.stderr)
};

vector<string> split_line(string input,char* delimeter);
set<string> get_known_command_set();
void initial_setting();
void set_pipe_array(int* pipe_array);
void parse_cmd(vector<string> tokens);
void set_current_cmd_pipe_out(command cmd);
void write_file(string fileName);
void execute_cmd(command cmd);

set<string> command_set;
int line_count = 0;
deque<command> current_job_queue;

class compare{
public:
    bool operator()(command x,command y){
        return x.exe_line_num > y.exe_line_num;
    };
};
priority_queue<command,vector<command>,compare> unhandled_jobs;


int main(int argc, const char * argv[]) {
    string inputLine;
    
    initial_setting();

    cout << "% ";
    
    while(getline(cin,inputLine)){
        //count line for |n
        if(inputLine != "\0"){
            line_count++;
        }
        if(inputLine.compare("exit") == 0){
            return 0;
        }
        
        vector<string> tokens = split_line(inputLine, " ");
        
        if(inputLine.find("setenv") != string::npos){
            setenv(tokens[1].c_str(), tokens[2].c_str(), 1);
            command_set = get_known_command_set();
            
        }else if(inputLine.find("printenv") != string::npos){
            cout << getenv(tokens[1].c_str()) << endl;
        }else{
            parse_cmd(tokens);
        }
        
        for(deque<command>::iterator it = current_job_queue.begin(); it != current_job_queue.end(); it++){
            //pipe fork exec
            
            //if this command hasn't create pipe before,create one
            if(!it->before_numbered_pipe){
                set_pipe_array(it->pipe_arr);
            }
            
            if(it == current_job_queue.begin()){
                //record pipe num of unhandled cmd
                vector<int*> unhandled_pipeNum_arr;
                
                int p_id = fork();
                if(p_id < 0) cout << "can't fork process" << endl;
                else if(p_id == 0){
                    //get data from unhandled command
                    while(unhandled_jobs.size() > 0 && unhandled_jobs.top().exe_line_num == line_count){
                        command unhand_cmd = unhandled_jobs.top();
                        unhandled_pipeNum_arr.push_back(unhand_cmd.pipe_arr);
                        unhandled_jobs.pop();
                        cout << "child pop unhandled cmd:" << unhand_cmd.name << unhand_cmd.exe_line_num << "and dup" << endl;
                        dup2(unhand_cmd.pipe_arr[1], STDIN_FILENO);
                    }
                    
                    //set current command pipe_out
                    if((*it).need_pipe_out){
                        set_current_cmd_pipe_out(*it);
                    }
                   
                    execute_cmd(*it);
                   
                }else{
                    while(unhandled_jobs.size() > 0 && unhandled_jobs.top().exe_line_num == line_count){
                        command unhand_cmd = unhandled_jobs.top();
                        unhandled_pipeNum_arr.push_back(unhand_cmd.pipe_arr);
                        unhandled_jobs.pop();
                        cout << "parent pop unhandled cmd:" << unhand_cmd.name << unhand_cmd.exe_line_num  << endl;
                    }
                    //wait for cmd executing finish
                    int status;
                    wait(&status);
                    cout << "child process end" << endl;
                    //close all finished unhandled_command pipe
                    while(unhandled_pipeNum_arr.size() > 0){
                        int *tmp = unhandled_pipeNum_arr.back();
                        cout << "close finished unhandled_command pipe " << tmp[0] << " and " << tmp[1] << endl;
                        close(tmp[0]);
                        close(tmp[1]);
                        unhandled_pipeNum_arr.pop_back();
                    }
                }
            }
            else{
                //this is not first job
                int p_id = fork();
                if(p_id < 0) cout << "can't fork process" << endl;
                else if(p_id == 0){
                    //if previous command need pipe_out, dup this cmd's pipe_in to previous cmd's pipe_out
                    //if this cmd need pipe_out, dup this cmd's pipe_out to stdout
                    //execute
                    
                    if((it-1)->need_pipe_out){
                        dup2((it-1)->pipe_arr[1], STDIN_FILENO);
                    }
                    
                    //set current command pipe_out
                    if((*it).need_pipe_out){
                        set_current_cmd_pipe_out(*it);
                    }
                    
                    if(is_output_to_file((*it).name)){
                        write_file((*it).arguments[0]);
                    }else{
                        execute_cmd(*it);
                    }
                       
                }else{
                    //wait for cmd executing finish
                    int status;
                    wait(&status);
                }
            }
            

            //execute finish
            if(!it->need_pipe_out){
                //if this job doesn't need to pipe out,close pipe
                cout << "close this job pipe" << endl;
                close(it->pipe_arr[0]);
                close(it->pipe_arr[1]);
            }
            //close previous job pipe
            if(it != current_job_queue.begin()){
                cout << "close previous job pipe" << endl;
                command prev_cmd = *(it-1);
                close((it-1)->pipe_arr[0]);
                close((it-1)->pipe_arr[1]);
            }
            
        }
        current_job_queue.clear();
        
        cout << "% ";
    }
    return 0;
}

void write_file(string fileName){
    FILE *fp = fopen(fileName.c_str(),"w");
    
    if(fp == NULL) cout << "open file " << fileName << " error" << endl;
    else{
        int fp_num = fileno(fp);
        dup2(fp_num, STDOUT_FILENO); // write stdout to file
    }
}

void set_current_cmd_pipe_out(command cmd){
    if(cmd.output_type == "stdout"){
        close(cmd.pipe_arr[0]);
        dup2(cmd.pipe_arr[1], STDOUT_FILENO);
    }else if(cmd.output_type == "both"){
        close(cmd.pipe_arr[0]);
        dup2(cmd.pipe_arr[1], STDOUT_FILENO);
        dup2(cmd.pipe_arr[1], STDERR_FILENO);
    }
}

void set_pipe_array(int* pipe_array){
    if(pipe(pipe_array) == -1){
        cout << "can't create pipe" << endl;
    }
}

void parse_cmd(vector<string> tokens){
    for (vector<string>::iterator it = tokens.begin(); it != tokens.end(); ++it){
        if(*it == ">"){
            
            //push {> and *it+1} into current_job
            //pipe previous comm output to this input
            //execute cmd, when cmd=='>', write file
            
            //set previous job need pipe out
            command *prev_cmd = &current_job_queue.back();
            prev_cmd->need_pipe_out = true;
            prev_cmd->output_type = "stdout";
            
            //push > into current_job_queue
            command cur_comm;
            cur_comm.name = *it;
            vector<string> parameter;
            parameter.push_back(*++it);
            cur_comm.arguments = parameter;
            current_job_queue.push_back(cur_comm);
            
        }else if(is_stdout_numbered_pipe(*it)){
            // |n current_job pop_back and insert into unhandled_job(queue)
            
            int n = stoi((*it).substr(1,(*it).length()));
            n += line_count;
            command *last_cmd = &current_job_queue.back();
            last_cmd->before_numbered_pipe = true;
            last_cmd->exe_line_num = n;
            last_cmd->need_pipe_out = true;
            last_cmd->output_type = "stdout";
            set_pipe_array(last_cmd->pipe_arr);
            
            unhandled_jobs.push(*last_cmd);
        }else if(is_stderr_numbered_pipe(*it)){
            // !n current_job pop_back and insert into unhandled_job(queue)
            
            int n = stoi((*it).substr(1,(*it).length()));
            n += line_count;
            command *last_cmd = &current_job_queue.back();
            last_cmd->before_numbered_pipe = true;
            last_cmd->exe_line_num = n;
            last_cmd->need_pipe_out = true;
            last_cmd->output_type = "both";
            set_pipe_array(last_cmd->pipe_arr);
            
            unhandled_jobs.push(*last_cmd);
            
        }else if(*it == "|"){
            command *last_cmd = &current_job_queue.back();
            last_cmd->need_pipe_out = true;
            set_pipe_array(last_cmd->pipe_arr);
            
        }else if(command_set.count(*it) != 0){
            //*it is known command
            command cur_comm;
            cur_comm.name = *it;
            
            //push all parameters
            vector<string> parameter;
            while(!is_ordinary_pipe(*(it+1)) && !is_output_to_file(*(it+1)) && !is_stdout_numbered_pipe(*(it+1)) && !is_stderr_numbered_pipe(*(it+1)) && (it+1) != tokens.end()){
                cout << *it;
                parameter.push_back(*++it);
                if(it == tokens.end()) break;
            }
            cur_comm.arguments = parameter;
            current_job_queue.push_back(cur_comm);
        }else{
            //Unknown command
            cout << "Unknown command: [" << *it << "]."<< endl;
        }
    }
}

set<string> get_known_command_set(){

    set<string> result;
    DIR *pDIR;
    struct dirent *entry;
    const char* env_path = getenv("PATH");
    char* path;
    path = strdup(env_path);
    char* tmppath;
    tmppath = strtok(path,":");
    cout << "known commands:";
    
    if(pDIR = opendir(tmppath)){
        while(entry = readdir(pDIR)){
            //ignore . .. .DS_store
            if( entry->d_name[0] != '.'){
                cout << entry->d_name << " ";
                result.insert(entry->d_name);
            }
        }
        closedir(pDIR);
    }
    
    result.insert(">");
    cout << endl;
    return result;
}

vector<string> split_line(string input,char* delimeter){
    char *comm = new char[input.length()+1];
    strcpy(comm, input.c_str());
    
    char* token = strtok(comm, delimeter);
    vector<string> result;
    while(token != NULL){
        result.push_back(token);
        token = strtok(NULL, delimeter);
    }
    return result;
}

void initial_setting(){
    setenv("PATH", "bin:.", 1);
    command_set = get_known_command_set();
    
}

void execute_cmd(command cmd){
    //change cmd info to execvp required data type
    vector<char*>  vc_char;
    char *name = new char[cmd.name.length()+1];
    strcpy(name,cmd.name.c_str());
    vc_char.push_back(name);
    if(cmd.arguments.size() > 0){
        transform(cmd.arguments.begin(), cmd.arguments.end(), back_inserter(vc_char), convert);
        vc_char.push_back(NULL);
    }
    char **arg = &vc_char[0];
    cout << "execute job " << arg[0] << endl;
    execvp(arg[0], arg);
    
}
