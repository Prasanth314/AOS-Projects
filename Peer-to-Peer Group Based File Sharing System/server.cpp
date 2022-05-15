// Server side
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fstream>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <iostream>
#include <bits/stdc++.h>
using namespace std;


int PORT=8080;

class Client
{
	public:
	string userid,password;
	string ip;
	int port;
	unordered_map<string,string> Files_bit_map;
	int online;
	Client(string id,string pswd,int prt,string ipaddress)
	{
		ip=ipaddress;
		userid=id;
		password=pswd;
		port=prt;
		online=0;
	}
};
class Filedata
{
	public:
	string name;
	long long Filesize;
	vector<Client*> sharing_friends;
	Filedata(string n,long long fsize)
	{
		name=n;
		Filesize=fsize;
	}
};
class Group
{
	public:
	string id;
	Client *owner;
	set<Client*> members;
	unordered_set<string> pending_requests;
	unordered_map<string,Filedata*> Files_list;
	Group(string grpid,Client *c)
	{
		id=grpid;
		owner=c;
		members.insert(c);
	}
	string addFile(string Filepath,long long fsize,Client *c)
	{
		auto itr=Files_list.find(Filepath);
		if(itr==Files_list.end())
		{
			// vector<Client*> a;
			// a.push_back(c);
			Filedata *f=new Filedata(Filepath,fsize);
			f->sharing_friends.push_back(c);
			Files_list.insert(make_pair(Filepath,f));
			return "File uploaded successfully";
		}
		else
		{
			// if(itr->second->sharing_friends.find(c)==itr->second->sharing_friends.end())
			// 	return "You are already a seeder";
			itr->second->sharing_friends.push_back(c);
			return "you are added as a seeder to the file";
		}
	}
	void removeaccess(string filename,Client *c)
	{
		auto &v=Files_list[filename]->sharing_friends;
		auto it=find(v.begin(),v.end(),c);
		if(it!=v.end())
		{
			if(v.size()==1)
			{
				Files_list.erase(filename);
			}
			else
			{
				v.erase(it);
			}
		}
	}
};


vector<string> split_arguments(string s,char delimiter);


void *connection_handler(void *);

string process_command(char buffer[],Client *&c);
string create_user(string usr, string psd,int port);
string login(string usr,string psd,Client *&c);
string create_group(string grpid,Client *&c);
string join_group(string grpid,Client *&c);
string leave_group(string grpid,Client *&c);
string list_requests(string grpid,Client *&c);
string accept_request(string grpid,string userid,Client *&c);
string list_groups(Client *&c);
string list_files(string grpid,Client *&c);
string upload_file(string filepath,string grpid,string fsize,string bitmap,Client *&c);
string download_file(string grpid,string filename,string destination_path,Client *&c);
string logout(Client *&c);
string show_downloads(Client *&c);
string stop_share(string grpid,string filename,Client *&c);



map<string,Client*> users;
map<string,Group*> groups;

vector<string> split_arguments(string s,char delimiter)
{
	vector<string> tokens;
	stringstream ss(s);
	string tkn;
	while(getline(ss,tkn,delimiter))
	{
		tokens.push_back(tkn);
	}
	return tokens;
}
int main(int argc, char const *argv[])
{
	string myText;
  	ifstream MyReadFile(argv[1]);
	getline (MyReadFile, myText);
	vector<string> v=split_arguments(myText,' ');

	PORT=stoi(v[1]);


	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[1024] = {0};
	// char *hello = "Hello from server";
	
	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	
	// Forcefully attaching socket to the port 8080
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
												&opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );
	
	// Forcefully attaching socket to the port 8080
	if (bind(server_fd, (struct sockaddr *)&address,
								sizeof(address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

    pthread_t thread_id;

	while ((new_socket = accept(server_fd, (struct sockaddr *)&address,
					(socklen_t*)&addrlen)))
	{
		cout<<"Connection accepted for "<<new_socket<<endl;
        if(pthread_create(&thread_id,NULL,connection_handler,(void *) &new_socket)<0)
        {
            perror("\nCould not create thread\n");
            return 1;
        }
        // cout<<"A Thread has been assigned\n";
    }

    if(new_socket<0)
    {
        perror("accept");
		exit(EXIT_FAILURE);
    }
    pthread_exit(NULL);
	// valread = read( new_socket , buffer, 1024);
	// printf("%s\n",buffer );
	// send(new_socket , hello , strlen(hello) , 0 );
	cout<<"Hello message sent\n";
	return 0;
}

void *connection_handler(void *client_sock)
{
    int socket=*(int *)client_sock;
    int noofread;
    char client_command[1024];
    string msg;
    msg="Hi!! I am your connection handler  with socket no";
	Client *c=NULL;
    write(socket,msg.c_str(),msg.size());
	// cout<<"Printed init msg"<<endl;
    while((noofread=recv(socket,client_command,1024,0))>0)
    {
        client_command[noofread]='\0';
		string response=process_command(client_command,c);
        write(socket,response.c_str(),response.size());
        memset(client_command,0,1024);
    }
    if(noofread==0)
    {
        cout<<"client disconnected"<<endl;
        fflush(stdout);
    }
    else if(noofread==-1)
    {
        perror("ERROR Receiving clinet msg");
    }

    return 0;
}
string process_command(char buffer[],Client *&c)
{
	string command(buffer);
	// cout<<command<<endl;
	vector<string> tokens;
	stringstream ss(command);
	string tkn;
	while(getline(ss,tkn,' '))
	{
		tokens.push_back(tkn);
	}
	if(tokens[0]=="create_user")
	{
		return create_user(tokens[1],tokens[2],stoi(tokens[3]));
	}
	else if(tokens[0]=="login")
	{
		return login(tokens[1],tokens[2],c);
	}
	else if(tokens[0]=="create_group")
	{
		return create_group(tokens[1],c);
	}
	else if(tokens[0]=="join_group")
	{
		return join_group(tokens[1],c);
	}
	else if(tokens[0]=="leave_group")
	{
		return leave_group(tokens[1],c);
	}
	else if(tokens[0]=="list_requests")
	{
		return list_requests(tokens[1],c);
	}
	else if(tokens[0]=="accept_request")
	{
		return accept_request(tokens[1],tokens[2],c);
	}
	else if(tokens[0]=="list_groups")
	{
		return list_groups(c);
	}
	else if(tokens[0]=="list_files")
	{
		return list_files(tokens[1],c);
	}
	else if(tokens[0]=="upload_file")
	{
		return upload_file(tokens[1],tokens[2],tokens[3],tokens[4],c);
	}
	else if(tokens[0]=="download_file")
	{
		return download_file(tokens[1],tokens[2],tokens[3],c);
	}
	else if(tokens[0]=="logout")
	{
		return logout(c);
	}
	else if(tokens[0]=="show_downloads")
	{
		return show_downloads(c);
	}
	else if(tokens[0]=="stop_share")
	{
		return stop_share(tokens[1],tokens[2],c);
	}
	return "Invalid Command";
}
string create_user(string usr, string psd,int port)
{
	if(users.find(usr)!=users.end())
	{
		return "USER ALREADY EXISTS";
	}
	Client *c=new Client(usr,psd,port,"127.0.0.1");
	users.insert(pair<string,Client*>(usr,c));
	return "USER CREATED";

}
string login(string usr,string psd,Client *&c)
{
	if(users.find(usr)!=users.end() && users[usr]->password==psd)
	{
		c=users[usr];
		c->online=1;
		return "Logged in";
	}
	return "INVALID USERNAME OR PASSWORD";
}
string create_group(string grpid,Client *&c)
{
	if(c==NULL)
	{
		return "You are not logged in";
	}
	if(groups.find(grpid)!=groups.end())
	{
		return "Group already exists";
	}
	Group *g=new Group(grpid,c);
	groups.insert(pair<string,Group*>(grpid,g));
	return "Group created";
}
string join_group(string grpid,Client *&c)
{
	if(c==NULL)
	{
		return "You are not logged in";
	}
	auto itr=groups.find(grpid);
	if(itr==groups.end())
	{
		return "No such group exists";
	}
	Group *g=itr->second;
	if(g->members.find(c)!=g->members.end())
	{
		return "You are already in the group";
	}
	g->pending_requests.insert(c->userid);
	return "Request sent";
}
string leave_group(string grpid,Client *&c)
{
	if(c==NULL)
	{
		return "You are not logged in";
	}
	auto itr=groups.find(grpid);
	if(itr==groups.end())
	{
		return "No such group exists";
	}
	Group *g=itr->second;
	if(g->owner==c)
	{
		groups.erase(grpid);
		free(g);
		return "Group deleted as you are its owner";
	}
	else
	{
		if(g->members.find(c)==g->members.end())
			return "You are not part of the group";
		g->members.erase(c);
		return "Removed you from the group";
	}
}
string list_requests(string grpid,Client *&c)
{
	if(c==NULL)
	{
		return "You are not logged in";
	}

	if(groups[grpid]->owner->userid!=c->userid)
	{
		return "You are not owner!(Access Denied)";
	}
	string response="";
	for(string r:groups[grpid]->pending_requests)
	{
		response+=r+"\n";
	}
	if(response.size()==0)
	{
		return "No pending requests";
	}
	return response;
}
string accept_request(string grpid,string userid,Client *&c)
{
	if(c==NULL)
	{
		return "You are not logged in";
	}
	if(groups[grpid]->owner->userid!=c->userid)
	{
		return "You are not owner!(Access Denied)";
	}
	Client *joinee=users[userid];
	groups[grpid]->members.insert(joinee);
	return "Request has been Accepted";
}
string list_groups(Client *&c)
{
	if(c==NULL)
	{
		return "You are not logged in";
	}
	string response="";
	for(auto itr=groups.begin();itr!=groups.end();itr++)
	{
		response+=itr->first+"\n";
	}
	if(response.size()==0)
	{
		return "No Active Groups";
	}
	return response;
}
string list_files(string grpid,Client *&c)
{
	if(c==NULL)
	{
		return "You are not logged in";
	}
	auto itr=groups.find(grpid);
	if(itr==groups.end())
	{
		return "No such group exists";
	}
	Group *g=itr->second;
	if(g->members.find(c)==g->members.end())
	{
		return "You are not in the group";
	}
	string response="";
	for(auto it=g->Files_list.begin();it!=g->Files_list.end();it++)
	{
		response+=it->first+"\n";
	}
	if(response.size()==0)
	{
		return "No Sharable files in the group";
	}
	return response;

}
string upload_file(string filepath,string grpid,string fsize,string bitmap,Client *&c)
{
	if(c==NULL)
	{
		return "You are not logged in";
	}
	long long filesize=stoll(fsize);
	auto itr=groups.find(grpid);
	if(itr==groups.end())
	{
		return "No such group exists";
	}
	Group *g=itr->second;
	if(g->members.find(c)==g->members.end())
	{
		return "You are not in the group";
	}
	// if(c->Files_bit_map.find(filepath)!=c->Files_bit_map.end())
	// {
	// 	return "File has been already uploaded";
	// }
	c->Files_bit_map.insert(make_pair(filepath,bitmap));
	return groups[grpid]->addFile(filepath,filesize,c);

}
string download_file(string grpid,string filename,string destination_path,Client *&c)
{
	if(c==NULL)
	{
		return "You are not logged in";
	}
	auto itr=groups.find(grpid);
	if(itr==groups.end())
	{
		return "No such group exists";
	}
	Group *g=itr->second;
	if(g->members.find(c)==g->members.end())
	{
		return "You are not in the group";
	}
	auto it=groups[grpid]->Files_list.find(filename);
	if(it==groups[grpid]->Files_list.end())
	{
		return "No such file in the group";
	}
	string response="";
	for(Client *x:it->second->sharing_friends)
	{
		if(x->online)
		{
			response+=x->ip+" "+to_string(x->port)+" "+x->Files_bit_map[filename]+"\n";
		}
	}
	return response;

}
string logout(Client *&c)
{
	if(c==NULL)
	{
		return "You are not logged in";
	}
	c->online=0;
	c=NULL;
	return "Logged out";

}
string show_downloads(Client *&c)
{
	return "PASS";

}
string stop_share(string grpid,string filename,Client *&c)
{
	c->Files_bit_map.erase(filename);
	groups[grpid]->removeaccess(filename,c);
	// c->Files_bit_map.insert(make_pair(filepath,bitmap));

	return "Sharing stopped";

}


