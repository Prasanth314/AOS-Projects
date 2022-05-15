// Client side 
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <bits/stdc++.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>

using namespace std;
#define PORT 8080

int preprocess_command(string &msg);


bool isFileValid(string file)
{
	ifstream f(file.c_str());
    return f.good()
}
int main(int argc, char const *argv[])
{
	int sock = 0, valread;
	struct sockaddr_in serv_addr;
	string hello = "Hello from client1";
	char command[1024] = {0};
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		cout<<"\n Socket creation error \n";
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	
	// Convert IPv4 and IPv6 addresses from text to binary form
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
	{
		cout<<"\nInvalid address/ Address not supported \n";
		return -1;
	}

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		cout<<"\nConnection Failed \n";
		return -1;
	}
    // cin>>hello;
	// send(sock , hello.c_str() , hello.size() , 0 );
	// cout<<"Hello message sent\n";
	// valread = read( sock , command, 1024);
	// cout<<buffer;
    
    // write(sock , hello.c_str() , hello.size() );
	cout<<"Server is listening!! Enter command"<<endl;
	valread = recv( sock , command, 1024,0);
	cout<<string(command)<<endl;
    string msg;
    while(1)
    {
        getline(cin,msg);
		int status=preprocess_command(msg);
		if(status==0)
		{
			continue;
		}
		cout<<msg<<endl;
        write(sock,msg.c_str(),msg.size());
		memset(command,0,1024);
        if((valread=recv(sock,command,1024,0))>0)
			cout<<string(command)<<endl;
        if(strcmp(command,"quit")==0)
            break;
    }
	close(sock);
	return 0;
}
int preprocess_command(string &msg)
{
	vector<string> tokens;
	stringstream ss(msg);
	string tkn;
	while(getline(ss,tkn,' '))
	{
		tokens.push_back(tkn);
	}
	if(tokens[0]=="upload_file")
	{
		char *ptr=realpath(tokens[1].c_str(),NULL);
		tokens[1]=string(ptr);
		if(!isFileValid(tokens[1]))
		{
			cout<<"Invaid File"<<endl;
			return 0;
		}
		msg="";
		for(string x:tokens)
		{
			msg+=x+" ";
		}
	}
	return 1;
}