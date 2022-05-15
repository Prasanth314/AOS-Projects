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
#include <sys/types.h>
#include <fstream>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>

using namespace std;
#define SERVERPORT 8080


int PORT=5000;

int online=0;

class Filedetails
{
	public:
	string name;
	long long Filesize;
	string group;
	string bitmap;
	char status;
	Filedetails(string n,long long fsize,string bmap,char sttus)
	{
		name=n;
		Filesize=fsize;
		bitmap=bmap;
		status=sttus;
	}
};

map<string,Filedetails*> Share_files;
// map<string,Filedetails*> pending_downloads;
// map<string,Filedetails*> completed_downloads;


int preprocess_command(string &msg);
void download_file(string msg,string response);
bool isFileValid(string file);
int postprocessresponse(string msg,char command[]);
void *initiate_download(void *client);
// void initiate_download(string client_data);
void show_downloads();


bool isFileValid(string file)
{
	ifstream f(file.c_str());
    return f.good();
}
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
long long getFileSize(string file)
{
	struct stat st;
	long long size=-1;
	if(stat(file.c_str(),&st)!=-1)
	{
		size = st.st_size;
	}
	else
	{
		cout<<"\nError in calculating file size"<<endl;
	}
	return size;
}
string download_chunk(string file,string destination,int chunk)
{
	// FILE* fd = NULL;
	// FILE* wd = NULL;


	int fd,wd;
	char buff[524290];
    memset(buff,0,sizeof(buff));
	size_t count=512*1024;
	int idx=file.find_last_of("/");
	string filename=file.substr(idx+1);
	destination+="/"+filename;
    fd = open(file.c_str(),O_RDONLY);
    wd = open(destination.c_str(),O_RDWR|O_CREAT,0777);

    if(fd==-1)
    {
        return "\n fopen() Error!!!\n";
    }
	if(wd==-1)
    {
        return "\n fopen() Error!!!\n";
    }
	// if(pread(fd,buff,count,512*chunk)!=count)
	// {
	// 	return "Chunk not read completely";
	// }
	// if(pwrite(fd,buff,count,512*chunk)!=count)
	// {
	// 	return "Chunk not written completely";
	// }
	count=pread(fd,buff,count,512*1024*chunk);
	// cout<<"Written "<<count<<" bytes for chunk "<<chunk<<endl;
	pwrite(wd,buff,count,512*1024*chunk);
	close(fd);
	close(wd);
	return "SUCCESS";
}
string process_client_command(char buffer[])
{
	string command(buffer);
	vector<string> tokens;
	stringstream ss(command);
	string tkn;
	while(getline(ss,tkn,' '))
	{
		tokens.push_back(tkn);
	}
	if(tokens[0]=="download_chunk")
	{
		return download_chunk(tokens[1],tokens[2],stoi(tokens[3]));
	}
	else if(tokens[0]=="quit")
	{
		return "quit";
	}
	return "quit";
}
void *connection_handler(void *client_sock)
{
	int socket=*(int *)client_sock;
    int noofread;
    char client_command[1024];
    string msg;
    msg="Hi!! I am your connection handler  with socket no";
    write(socket,msg.c_str(),msg.size());
	// cout<<"Printed init msg"<<endl;
    while((noofread=recv(socket,client_command,1024,0))>0)
    {
        client_command[noofread]='\0';
		string response=process_client_command(client_command);
		// Send chunk file
		// cout<<string(client_command)<<endl;
		if(response=="quit")
			break;
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
	close(socket);
    return 0;
}
void *clientasserver(void *address)
{
	struct sockaddr_in add=*(struct sockaddr_in *)address;
	// cout<<add.sin_port<<" Client port"<<endl;
	int sock=0,opt=1;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		cout<<"\n clientasserver Socket creation error \n";
		exit(EXIT_FAILURE);
	}
	// Forcefully attaching socket to the port 8080
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
												&opt, sizeof(opt)))
	{
		perror("clientasserver setsockopt");
		exit(EXIT_FAILURE);
	}
	
	// Forcefully attaching socket to the port 8080
	if (bind(sock, (struct sockaddr *)&add,
								sizeof(add))<0)
	{

		perror("clientasserver bind failed");

		exit(EXIT_FAILURE);
	}
	if (listen(sock, 40) < 0)
	{
		perror("clientasserver listen failed");
		exit(EXIT_FAILURE);
	}
	int addrlen = sizeof(address);
	int new_socket;
	pthread_t thread_id;
	int newsocket;
	while ((new_socket = accept(sock, (struct sockaddr *)&add,
					(socklen_t*)&addrlen)))
	{
		// cout<<"Connection accepted for "<<new_socket<<endl;
        if(pthread_create(&thread_id,NULL,connection_handler,(void *) &new_socket)<0)
        {
            perror("\nCould not create thread\n");
            break;
        }
        // cout<<"A Thread has been assigned\n";
    }
	return 0;
}

int main(int argc, char const *argv[])
{
	string myText;
  	ifstream MyReadFile(argv[2]);
	getline (MyReadFile, myText);
	vector<string> v=split_arguments(myText,' ');

	vector<string> client_info=split_arguments(argv[1],':');


	int sock=0;
	int valread,opt=1;
	struct sockaddr_in address;
	struct sockaddr_in serv_addr;
	int addrlen = sizeof(address);
	PORT=stoi(client_info[1]);
	// printf("%d",PORT);

	string hello = "Hello from client1";
	char command[1024] = {0};
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		cout<<"\n Socket creation error \n";
		return -1;
	}


	
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

	// cout<<address.sin_port<<" main function"<<endl;
	pthread_t thread_id;
	if(pthread_create(&thread_id,NULL,clientasserver,(void *) &address)<0)
	{
		perror("\nCould not create client as server thread\n");
		return 1;
	}





	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(stoi(v[1]));
	
	// Convert IPv4 and IPv6 addresses from text to binary form
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
	{
		cout<<"\nInvalid address/ Address not supported \n";
		return -1;
	}


	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		cout<<"\nServer Connection Failed \n";
		printf("%s",strerror(errno));
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
        write(sock,msg.c_str(),msg.size());
		memset(command,0,1024);
        if((valread=recv(sock,command,1024,0))>0)
		{
			int poststatus=postprocessresponse(msg,command);
			if(poststatus)
				cout<<"\n"<<string(command)<<endl;
		}
        if(strcmp(command,"quit")==0)
            break;
    }
	close(sock);
	return 0;
}
int upload_file(vector<string> tokens,string &msg)
{
	// cout<<"Function enetred"<<endl;
	char *ptr=realpath(tokens[1].c_str(),NULL);
	tokens[1]=string(ptr);
	if(!isFileValid(tokens[1]))
	{
		cout<<"Invalid File"<<endl;
		return 0;
	}
	msg="";
	for(string x:tokens)
	{
		msg+=x+" ";
	}
	// cout<<"Path fetched"<<endl;
	long long fsize=getFileSize(tokens[1]);
	if(fsize==-1)
	{
		return 0;
	}
	// cout<<"Size fetched"<<endl;
	long long chunks=ceill(fsize/((float)512*1024));
	// cout<<"Chunks fetched"<<chunks<<endl;

	if(Share_files.find(tokens[1])!=Share_files.end() && Share_files[tokens[1]]->group==tokens[2])
	{
		cout<<"File already uploaded"<<endl;
		return 0;
	}
	
	string bitmap;

	bitmap.insert(0,chunks,'1');
	msg+=to_string(fsize)+" "+bitmap;
	Filedetails *f=new Filedetails(tokens[1],fsize,bitmap,'U');
	Share_files.insert(pair<string,Filedetails*>(tokens[1],f));
	return 1;
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
		return upload_file(tokens,msg);
		
	}
	else if(tokens[0]=="show_downloads")
	{
		show_downloads();
		return 0;
	}
	else if(tokens[0]=="create_user")
	{
		msg+=" "+to_string(PORT);
	}
	return 1;
}
void show_downloads()
{
	for(auto i:Share_files)
	{
		if(i.second->status!='U')
		{
			cout<<"["<<i.second->status<<"] ["<<i.second->group<<"] "<<i.second->name<<endl;
		}
	}
}
int postprocessresponse(string msg,char command[])
{
	string response(command);
	vector<string> tokens;
	stringstream ss(msg);
	string tkn;
	while(getline(ss,tkn,' '))
	{
		tokens.push_back(tkn);
	}
	if(tokens[0]=="download_file")
	{
		download_file(msg,response);
		return 0;
	}
	return 1;
}
bool compareInterval(vector<string> i1, vector<string> i2)
{
	int a=count(i1[2].begin(),i1[2].end(),'1');
	int b=count(i2[2].begin(),i2[2].end(),'1');
    return (a < b);
}
string get_chunk_container(vector<vector<string>> &address,int k)
{
	for(vector<string> client_data: address)
	{
		if(client_data[2][k]=='1')
		{
			return client_data[0]+" "+client_data[1]+" "+client_data[2];
		}
	}
	return "NIL";
}
vector<string> pieceselection(vector<vector<string>> &address,long long chunks)
{
	sort(address.begin(),address.end(),compareInterval);
	// cout<<"Sorting done"<<endl;
	vector<string> result;
	for(int i=0;i<chunks;i++)
	{
		result.push_back(get_chunk_container(address,i));
	}
	return result;
}

// response	:::	IP  Port  Bitmap \n IP  Port  Bitmap
void download_file(string msg,string response)
{
	vector<string> filedetails;   //download_file  group  file  destination
	stringstream fdet(msg);
	string tkn;
	while(getline(fdet,tkn,' '))
	{
		filedetails.push_back(tkn);
	}
	long long chunks;
	if(Share_files.find(filedetails[3])!=Share_files.end() && Share_files[filedetails[3]]->group==filedetails[1])
	{
		cout<<"File already exists"<<endl;
	}
	else
	{
		long long fsize=getFileSize(filedetails[2]);
		chunks=ceill(fsize/((float)512*1024));
		string bitmap;
		bitmap.insert(0,chunks,'0');
		Filedetails *f=new Filedetails(filedetails[3],fsize,bitmap,'D');
		Share_files.insert(pair<string,Filedetails*>(filedetails[3],f));
	}



	vector<string> tokens;
	stringstream ss(response);
	// cout<<msg<<endl;
	while(getline(ss,tkn,'\n'))
	{
		tokens.push_back(tkn);
	}
	// cout<<tokens[0]<<"from download function"<<endl;
	vector<vector<string>> address;
	for(string clientdata:tokens)
	{
		ss.clear();
		ss.str(clientdata);
		vector<string> add;
		while(getline(ss,tkn,' '))
		{
			add.push_back(tkn);
		}
		address.push_back(add);
	}
	// for(string x:address)
	// {
	// 	cout<<x<<endl;
	// }
	// cout<<address[0]<<address[1]<<address[2]<<endl;


	vector<string> pieces=pieceselection(address,chunks);
	// cout<<"Piece selection done"<<pieces.size()<<endl;


	int NUM_THREADS=address[0][2].size();
	pthread_t threads[NUM_THREADS];
	int chunk_exists[NUM_THREADS]={0};
    int rc;
    long t;

	// cout<<"No of chunks"<<NUM_THREADS<<endl;

	// for(string p:pieces)
	// {
	// 	cout<<p<<endl;
	// }
	// cout<<"Threads"<<NUM_THREADS<<endl;
    for (t = 0; t < NUM_THREADS; t++) 
	{	
		if(pieces[t]=="NIL")
		{
			cout<<"Piece not found"<<endl;
			continue;
		}
		else
		{
			chunk_exists[t]=1;
		}
					  //IP Port Bitmap	FileSource		Destination		chunk
		string client=pieces[t]+" "+filedetails[2]+" "+filedetails[3]+" "+to_string(t);
		// initiate_download(client);
		// cout<<"Piece found for "<<pieces[t]<<endl;
        rc = pthread_create(&threads[t], NULL, initiate_download, (void *)&client);
		usleep(10000);
        if (rc) {
            printf("ERORR in initiating download chunk; return code from pthread_create() is %d\n", rc);
            exit(EXIT_FAILURE);
        }
    }

	int ret;
    for (t = 0; t < NUM_THREADS; t++) {
		if(!chunk_exists[t])
		{
			continue;
		}
        void *retval;
        ret = pthread_join(threads[t], &retval);

        // if (retval == PTHREAD_CANCELED)
        //     printf("The thread was canceled - ");
        // else
        //     printf("Returned value %d - ", *(int *)retval);
		// switch (ret) {
        //     case 0:
        //         printf("The thread joined successfully\n");
        //         break;
        //     case EDEADLK:
        //         printf("Deadlock detected\n");
        //         break;
        //     case EINVAL:
        //         printf("The thread is not joinable\n");
        //         break;
        //     case ESRCH:
        //         printf("No thread with given ID is found\n");
        //         break;
        //     default:
        //         printf("Error occurred when joining the thread\n");
		// }
    }
	// cout<<"File download complete"<<endl;
	Share_files[filedetails[3]]->status='C';
    pthread_exit(NULL);
	


}
// void initiate_download(string client_data)
void *initiate_download(void *client)
{						//IP	Port	Bitmap	Filesource	Destination	ChunkNumber
	string client_data=*(string *)client;


	// cout<<client_data<<endl;
	vector<string> address;
	stringstream ss(client_data);
	string tkn;
	while(getline(ss,tkn,' '))
	{
		address.push_back(tkn);
	}
	string destination=address[4];
	int chunk=stoi(address[5]);
	if(Share_files[destination]->bitmap[chunk]=='0')
	{
			// cout<<"initiate download "<<chunk<<endl;

		int sock=0;
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			cout<<"\n Socket creation error \n";
			return 0;
		}

		struct sockaddr_in serv_addr;
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(stoi(address[1]));

		// Convert IPv4 and IPv6 addresses from text to binary form
		if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
		{
			cout<<"\nInvalid address/ Address not supported while downloading\n";
			return 0;
		}
		if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		{
			cout<<"\nConnection Failed \n";
			return 0;
		}

		string requesting_download;
		char command[1024] = {0};
		int valread;
		if((valread=recv(sock,command,1024,0))>0)
		{
			// cout<<string(command)<<endl;
		}
											//File source 		Destination			Chunk number
		requesting_download="download_chunk "+address[3]+" "+address[4]+" "+address[5];
		// cout<<msg<<endl;
		write(sock,requesting_download.c_str(),requesting_download.size());
		memset(command,0,1024);
		if((valread=recv(sock,command,1024,0))>0)
		{
			// cout<<string(command)<<endl;
		}

		string msg="quit";
		write(sock,msg.c_str(),msg.size());
		close(sock);
	}
	Share_files[destination]->bitmap[chunk]='1';
	pthread_exit(NULL);
	return 0;
}