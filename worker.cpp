#include <iostream>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h> 
#include <sys/stat.h> 
//
#include <iterator>     //std::istream_iterator
//

using namespace std;

#define BUFSIZE 4096
#define MYFIFO "/tmp/myfifo"

void splitbyAmpersand(string& str, vector<string> &arguments){
    stringstream mystringstream(str);
    string segment;

    while(getline(mystringstream, segment, '&'))
    {
       arguments.push_back(segment);
    }
}

string splitAfterEqual(string& str){
    return str.substr(str.find("=") + 1);
}

string splitBeforeSlash(string& str){
    return str.substr(0, str.find("/"));
}

string splitAfterSlash(string& str){
    return str.substr(str.find("/") + 1);
}

void setNames(vector<string> parsedRequest , string &dir , vector<string> &filenames , vector<string> &filters , vector<string> &feildsName){
    for(int i = 0 ; i < parsedRequest.size() ; i++){
        if(parsedRequest[i].find("file=") != string::npos){
            filenames.push_back(splitAfterEqual(parsedRequest[i]));
        }else if(parsedRequest[i].find("dir=") != std::string::npos){
            dir = splitAfterEqual(parsedRequest[i]);
        }else if(parsedRequest[i].find("filter=") != std::string::npos){
            string temp = splitAfterEqual(parsedRequest[i]);
            feildsName.push_back(splitBeforeSlash(temp));
            filters.push_back(splitAfterSlash(temp));
        }
    }
}

void parseData(string request , vector<string> &filenames , vector<string> &filters , vector<string> &feildsName , string &dir){
    vector<string> parsedRequest;
    splitbyAmpersand(request , parsedRequest);
    setNames(parsedRequest , dir , filenames , filters , feildsName);
}

void deletWhiteSpaces(string& str){
    for(int j = 0; j < str.size(); ++j)
        if(str[j] == ' ')
            str.erase(str.begin() + j);
}

void replaceDashWithSpace(string& str){
    for(int i = 0; i < str.size(); i++){  
        if(str[i] == '-')  
            str[i] = ' ';  
    }  
}

void readFile(string dir , vector<string> &data){
    ifstream myReadFile;
    myReadFile.open(dir);
    string output;
    
    if (myReadFile.is_open()) {
        while(getline(myReadFile , output)){
            deletWhiteSpaces(output);
            replaceDashWithSpace(output);
            data.push_back(output);
        }
    }      
    myReadFile.close();
}

int findColumn(string data , string feild){
    stringstream temp(data);
    istream_iterator<string> begin(temp);
    istream_iterator<string> end;
    vector<string> vstrings(begin, end);
    for(int i = 0 ;i < vstrings.size() ; i++)
        if(vstrings[i] == feild)
            return i;
    return -1;
}

bool correctData(int column , string filter , string data){
    stringstream temp(data);
    istream_iterator<string> begin(temp);
    istream_iterator<string> end;
    vector<string> vstrings(begin, end);
    if(vstrings[column] == filter)
        return true;
    return false;
}

void filterData(vector<string> data , vector<string> &filtered , vector<string> filters , vector<string> feildsName){
    vector<string> temp;
    for(int i = 0 ; i < filters.size() ; i++){
        int column = findColumn(data[0] , feildsName[i]);
        temp.clear();
        for(int j = 1 ; j < data.size() ; j++){
            if(correctData(column , filters[i] , data[j])){
                temp.push_back(data[j]);
            }
        }
        if(filters.size() > 1){
            string tmp = data[0];
            data.clear();
            data.push_back(tmp);
            for(int j = 0 ; j < temp.size() ; j++){
                data.push_back(temp[j]);
            }
        }
    }
    for(int i = 0 ; i < temp.size() ; i++)
        filtered.push_back(temp[i]);
}

void handelFiles(vector<string> filenames , vector<string> filters , vector<string> feildsName , string dir , vector<string> &filtered){
    string fullDir;
    vector<string> data;
    for(int i = 0 ; i < filenames.size() ; i++){
        fullDir.clear();
        data.clear();
        fullDir = dir + "/" + filenames[i];
        readFile(fullDir , data);
        filterData(data , filtered , filters , feildsName);
    }
}

void writeThroughNamedPipe(string &request){
    int fd;
    fd = open(MYFIFO, O_WRONLY);
    write(fd, request.c_str(), request.size());
    close(fd);
}

void sendDatatoPresenter(vector<string> filtered , vector<string> filenames , string dir){
    vector<string> data;
    if(filenames.size()>0){
        string fullDir;
        fullDir = dir + "/" + filenames[0];
        readFile(fullDir , data);
    }
    string request;
    request += data[0];
    request += "&";
    for(int i = 0 ; i < filtered.size() ; i++){
        request += filtered[i];
        if(i < filtered.size() - 1)
            request += "&";
    }
    writeThroughNamedPipe(request);
}

int main(int argc, char* argv[]){
    vector<string> filenames , filters , feildsName;
    vector<string> filtered;
    string request , dir;
    int bytesRead;
    char message [BUFSIZE];
    int readEnd = atoi(argv[1]);

    bytesRead = read(readEnd, message, BUFSIZE);
    close(readEnd);
    request = message;
    parseData(request , filenames , filters , feildsName , dir);
    handelFiles(filenames , filters , feildsName , dir , filtered);
    sendDatatoPresenter(filtered , filenames , dir);

    return 0;
}