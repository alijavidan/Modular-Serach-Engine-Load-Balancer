#include <iostream>
#include <fstream>
#include <stdio.h> 
#include <string> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sstream>
#include <sys/types.h> 
#include <unistd.h>
#include <vector>

using namespace std;

#define MYFIFO "/tmp/myfifo"

void splitbyAmpersand(string& str, vector<string> &arguments){
    stringstream mystringstream(str);
    string segment;

    while(getline(mystringstream, segment, '&'))
    {
       arguments.push_back(segment);
    }
}

void setVariables(string request , string &sortingValue , string &sortType , int &prc_cnt){
    vector<string> arguments;
    splitbyAmpersand(request , arguments);
    if(arguments.size() > 1){
        prc_cnt = atoi(arguments[0].c_str());
        sortingValue = arguments[1];
        sortType = arguments[2];
    }else{
        prc_cnt = atoi(arguments[0].c_str());
    }
}

void readThroughNamedPipe(string &request){
    ifstream file(MYFIFO);
    getline(file , request);
    file.close();
}

void getDatasFromWorkerProcesses(int prc_cnt , vector<string> &answer , string sortType , string sortingValue , string &header){
    int count = 0;
    string data;
    vector<string> datas;

    for(int i = 0 ; i < prc_cnt ; i++){
        datas.clear();
        readThroughNamedPipe(data);
        splitbyAmpersand(data , datas);
        header = datas[0];
        for(int j = 0 ; j < datas.size() ; j++){
            if(j != 0)
                answer.push_back(datas[j]);
        }
    }
}

void printResults(vector <string> answer , string sortType){
    if(sortType == "descending")
        for(int i = answer.size() - 1 ; i >= 0; i--)
            cout << answer[i] << endl;
    else if(sortType == "ascending")
        for(int i = 0 ; i < answer.size(); i++)
            cout << answer[i] << endl;
}

int main(){
    string sortingValue , sortType , header;
    int prc_cnt , fd;
    string request;
    vector <string> answer;

    readThroughNamedPipe(request);
    setVariables(request , sortingValue , sortType , prc_cnt);
    getDatasFromWorkerProcesses(prc_cnt , answer , sortType , sortingValue , header);
    printResults(answer , sortType);

    return 0; 
}