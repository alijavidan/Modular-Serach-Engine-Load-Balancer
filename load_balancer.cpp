#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sstream>      // std::stringstream
#include <string.h>		//std::strcmp
#include <sys/wait.h> 

using namespace std;

#define READ_END 0
#define WRITE_END 1
#define MYFIFO "/tmp/myfifo"
#define FLAG 0666

void deletWhiteSpaces(string& str){
    for(int j = 0; j < str.size(); ++j)
        if(str[j] == ' ')
            str.erase(str.begin() + j);
}

void splitbyDash(string& str, vector<string> &arguments){
    stringstream mystringstream(str);
    string segment;

    while(getline(mystringstream, segment, '-'))
    {
       arguments.push_back(segment);
    }
}

string splitAfterEqual(string& str){
    return str.substr(str.find("=") + 1);
}

string splitBeforeEqual(string& str){
    return str.substr(0, str.find("=")); 
}

char* sprintfd(int number){
    char *sprintf_buffer = (char *)malloc(sizeof(char)*10);
    sprintf(sprintf_buffer, "%d", number);
    return sprintf_buffer;
}

void setArguments(vector<string> arguments , vector <string> &feildsName , vector <string> &filters  , string &sortingValue , string &sortType , int &prc_cnt , string &dir){
    for(int i = 0 ; i < arguments.size() ; i++){
        if(arguments[i].find("processes") != std::string::npos){
            prc_cnt = stoi(splitAfterEqual(arguments[i]));
        }else if(arguments[i].find("dir") != std::string::npos){
            dir = splitAfterEqual(arguments[i]);
        }else if(arguments[i].find("descending") != std::string::npos || arguments[i].find("ascending") != std::string::npos){
            sortType = splitAfterEqual(arguments[i]);
            sortingValue = splitBeforeEqual(arguments[i]);
        }else{
            feildsName.push_back(splitBeforeEqual(arguments[i]));
            filters.push_back(splitAfterEqual(arguments[i]));
        }
    }
}

void readDirectory(const string& name, vector<string>& v){
    DIR* dirp = opendir(name.c_str());
    struct dirent * dp;

    if((dp = readdir(dirp)) == NULL){
        cout << "wrong directory" << endl;
        return;
    }

    while ((dp = readdir(dirp)) != NULL) {
        if( strcmp(dp->d_name , ".") == 0 || strcmp(dp->d_name , ".." ) == 0 || strcmp(dp->d_name , ".DS_Store") == 0)
            continue;
        v.push_back(dp->d_name);
    }
    closedir(dirp);
}

void writeThroughNamedPipe(string &request){
    int fd;
    mkfifo(MYFIFO, FLAG);
    fd = open(MYFIFO, O_WRONLY);
    write(fd, request.c_str(), request.size());
    close(fd);
}

void crtPresenterProcess(string sortingValue, string sortType , int prc_cnt){
    pid_t pid;
    string request = "";

    if((pid = fork()) < 0)
        cout << "fork fail\n";
    else if(pid == 0){
        char *args[] = {"./presenter" , NULL};
        execvp(args[0], args);
    }else{
        if(sortingValue != "")
            request = to_string(prc_cnt) + "&" + sortingValue + "&" + sortType;
        else 
            request = to_string(prc_cnt);
        writeThroughNamedPipe(request);
    }
}

void crtWorkerProcesses(int prc_cnt , vector <string> filesname , string dir , vector<string> feildsName , vector<string> filters){
    string request;
    int numOffiles , countUsedfile = 0;
    numOffiles = filesname.size();
    int filesPerprocess = (numOffiles / prc_cnt);
    int modeFileperProcess = numOffiles % prc_cnt;

    for(int i = 0 ; i < prc_cnt ; i++){
            
        int fd[2];
        pid_t pid;
        
        if(pipe(fd) == -1){
            cout << "pipe fail\n";
            break;
        }

        if((pid = fork()) < 0){
            cout << "fork fail\n";
            break;
        }
        else if(pid == 0){
            close(fd[WRITE_END]);
            char *args[] = {"./worker", sprintfd(fd[READ_END]), NULL};
            execvp(args[0], args);
            close ( fd[READ_END]);
        }else{
            request.clear();
            close(fd[READ_END]);
            request.append("dir=");
            request.append(dir);
            request.append("&");

            if(modeFileperProcess > 0){
                for(int j = 0 ; j < filesPerprocess + 1 ; j++ ){
                    request.append("file=");
                    request.append(filesname[countUsedfile]);
                    request.append("&");
                    countUsedfile++;
                }
                modeFileperProcess--;
            }else{
                for(int j = 0 ; j < filesPerprocess ; j++ ){
                    request.append("file=");
                    request.append(filesname[countUsedfile]);
                    request.append("&");
                    countUsedfile++;
                }
            }

            for(int k = 0 ; k < feildsName.size() ; k++){
                request.append("filter=");
                request.append(feildsName[k]);
                request.append("/");
                request.append(filters[k]);
                if( k < feildsName.size() - 1)
                    request.append("&");
            }

            write (fd[WRITE_END], request.c_str(), request.size());
            close (fd[WRITE_END]);
            wait(&pid);
        }
    }
}

void run(string userCommand){
    vector<string> arguments;
	string sortType = "" , sortingValue = "" , dir;
    int prc_cnt = 0;
    vector <string> filesname , feildsName , filters;
    
    deletWhiteSpaces(userCommand);
    splitbyDash(userCommand , arguments);
    setArguments(arguments , feildsName, filters , sortingValue , sortType , prc_cnt , dir);
    readDirectory(dir , filesname);
    crtPresenterProcess(sortingValue , sortType , prc_cnt);
    crtWorkerProcesses(prc_cnt , filesname , dir , feildsName , filters);
}

int main(){
    string userCommand;

    while(true){
        getline(cin,userCommand);
        if(userCommand == "quit")
            return 0;
        run(userCommand);
    }
}