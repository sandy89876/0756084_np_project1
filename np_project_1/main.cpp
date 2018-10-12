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

#define bin_path "/Users/huyuxuan/Desktop/np_project_1/np_project_1/bin"

using namespace std;

vector<string> split_line(string input,char* delimeter);
set<string> get_known_command_set();
void initial_setting();
void set_pipe_array();

struct command{
    string name;
    vector<string> arguments;
};

set<string> command_set;
int line_count = 0;
deque<command> current_job;
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
        
        vector<string> tokens = split_line(inputLine, " ");
        
        if(inputLine.find("setenv") != string::npos){
            setenv(tokens[1].c_str(), tokens[2].c_str(), 1);
            
        }else if(inputLine.find("printenv") != string::npos){
            cout << getenv(tokens[1].c_str()) << endl;
        }else{
            for (vector<string>::iterator it = tokens.begin(); it != tokens.end(); ++it){
                if(*it == "|"){
                    //fork process and execute cmd in current_job
                    cout << "meet |" << endl;
                    
                    
                }else if(*it == ">"){
                    cout << "meet >" << endl;
                }else if((*it).find("|") == 0 && (*it).length() > 1){
                    // |n current_job pop_back and insert into unhandled_job(queue)
                    cout << "meet |n" << endl;
                    
                }else if(command_set.count(*it) != 0){
                    //*it is known command
                    command cur_comm;
                    cur_comm.name = *it;
                    vector<string> parameter;
                    while(*(it+1) != "|" && *(it+1) != ">" && (it+1) != tokens.end()){
                        cout << *it;
                        parameter.push_back(*it);
                        it++;
                    }
                    cur_comm.arguments = parameter;
                    current_job.push_back(cur_comm);
                }else{
                    //Unknown command
                    cout << "Unknown command: [" << *it << "]."<< endl;
                }
            }
            
            
        }
        
        cout << "% ";
    }
    return 0;
}

void initial_setting(){
    setenv("PATH", "bin:.", 1);
    command_set = get_known_command_set();
    set_pipe_array();
    
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


