//
//  main.cpp
//  np_project_1
//
//  Created by 胡育旋 on 2018/10/11.
//  Copyright © 2018 胡育旋. All rights reserved.
//

#include <iostream>
#include <string>
#include <stdlib.h>
#include <fstream>
#include <dirent.h>
#include <unistd.h>

#include <vector>
#include <set>
#include <deque>
#include <queue>

#define bin_path "/Users/huyuxuan/Desktop/np_project_1/np_project_1/bin"

using namespace std;

struct command{
    string name;
    vector<string> arguments;
};

struct unhandled_command{
    command cmd_element;
    string output_type;//stdout or both(stdout.stderr)
    int exe_line_num;
};

vector<string> split_line(string input,char* delimeter);
set<string> get_known_command_set();
void initial_setting();
void set_pipe_array();
unhandled_command create_unhandled_cmd(command cmd,int n,string output_type);

//utility
bool is_stderr_numbered_pipe(string token);
bool is_stdout_numbered_pipe(string token);
bool is_ordinary_pipe(string token);
bool is_output_to_file(string token);


set<string> command_set;
int line_count = 0;
deque<command> current_job_queue;

class compare{
public:
    bool operator()(unhandled_command x,unhandled_command y){
        return x.exe_line_num >= y.exe_line_num;
    };
};
priority_queue<unhandled_command,vector<unhandled_command>,compare> unhandled_jobs;


int pipe_array[2][2];

int main(int argc, const char * argv[]) {
    string inputLine;
    
    initial_setting();
    /*
    for (set<string>::iterator it=command_set.begin(); it!=command_set.end(); ++it)
        cout << ' ' << *it << endl;
     */
    cout << "% ";
    
    while(getline(cin,inputLine)){
        //count line for |n
        if(inputLine != "\0"){
            line_count++;
        }
        if(inputLine.compare("exit") == 0){
            return 0;
        }
        
        while(unhandled_jobs.size() > 0 && unhandled_jobs.top().exe_line_num == line_count){
            unhandled_command tmp_cmd = unhandled_jobs.top();
            unhandled_jobs.pop();
            cout << "pop unhandled cmd:" << tmp_cmd.cmd_element.name << tmp_cmd.exe_line_num << endl;
            //insert tmp_cmd at the beginning of the current_job
            current_job_queue.push_front(tmp_cmd.cmd_element);
        }
        
        vector<string> tokens = split_line(inputLine, " ");
        
        if(inputLine.find("setenv") != string::npos){
            setenv(tokens[1].c_str(), tokens[2].c_str(), 1);
            
        }else if(inputLine.find("printenv") != string::npos){
            cout << getenv(tokens[1].c_str()) << endl;
        }else{
            for (vector<string>::iterator it = tokens.begin(); it != tokens.end(); ++it){
                if(*it == ">"){
                    cout << "meet >" << endl;
                    
                    //push {> and *it+1} into current_job
                    //pipe previous comm output to this input
                    //execute cmd, when cmd=='>', write file
                    command cur_comm;
                    cur_comm.name = *it;
                    vector<string> parameter;
                    parameter.push_back(*++it);
                    cur_comm.arguments = parameter;
                    current_job_queue.push_back(cur_comm);
                    
                }else if(is_stdout_numbered_pipe(*it)){
                    cout << "meet |n" << endl;
                    // |n current_job pop_back and insert into unhandled_job(queue)
                    
                    int n = stoi((*it).substr(1,(*it).length()));
                    n += line_count;
                    command last_cmd = current_job_queue.back();
                    current_job_queue.pop_back();
                    unhandled_command tmp = create_unhandled_cmd(last_cmd, n, "stdout");
                    unhandled_jobs.push(tmp);
                    
                }else if(is_stdout_numbered_pipe(*it)){
                    cout << "meet !n" << endl;
                    // !n current_job pop_back and insert into unhandled_job(queue)
                    
                    int n = stoi((*it).substr(1,(*it).length()));
                    n += line_count;
                    command last_cmd = current_job_queue.back();
                    current_job_queue.pop_back();
                    unhandled_command tmp = create_unhandled_cmd(last_cmd, n, "both");
                    unhandled_jobs.push(tmp);
                }else if(*it == "|"){
                    cout << "meet |" << endl;
                 
                }else if(command_set.count(*it) != 0){
                    //*it is known command
                    command cur_comm;
                    cur_comm.name = *it;
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
        
        //execute all cmd in current_job
        while(current_job_queue.size() > 0){
            command current_job = current_job_queue.front();
            current_job_queue.pop_front();
            cout << "execute job:" << current_job.name << endl;
        }
        
        cout << "% ";
    }
    return 0;
}

void initial_setting(){
    setenv("PATH", "bin:.", 1);
    command_set = get_known_command_set();
    
}

void set_pipe_array(){
    for(int i = 0; i < 2; i++){
        if(pipe(pipe_array[i]) == -1){
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
}

set<string> get_known_command_set(){
    set<string> result;
    DIR *pDIR;
    struct dirent *entry;
    if(pDIR = opendir(bin_path)){
        cout << "known commands:";
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

// |n
bool is_stdout_numbered_pipe(string token){
    if(token.find("|") == 0 && token.length() > 1) return true;
    return false;
}

// !n
bool is_stderr_numbered_pipe(string token){
    if(token.find("!") == 0 && token.length() > 1) return true;
    return false;
}

bool is_ordinary_pipe(string token){
    if(token == "|") return true;
    return false;
}

bool is_output_to_file(string token){
    if(token == ">") return true;
    return false;
}

unhandled_command create_unhandled_cmd(command cmd,int n,string output_type){
    unhandled_command result;
    result.cmd_element = cmd;
    result.exe_line_num = n;
    result.output_type = output_type;
    return result;
}


