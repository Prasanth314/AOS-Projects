#include <bits/stdc++.h>
#include <errno.h>
#include <termios.h>
#include <stdio.h>
#include <dirent.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>

#define CTRL_KEY(k) ((k)&01f)
enum editorkey
{
    ARROW_LEFT=1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN
};
using namespace std;
struct dirent *d;
struct editorConfig
{
    int commandmode=0;
    int cx=0,cy=0;
    int rows,cols;
    struct termios orig_termios;
};
struct editorConfig E;
struct visiblerows
{
    int top=0;
    int bottom=0;
};
struct visiblerows VR;
string current_directory=".";
string home=".";
stack<string> leftArw;
stack<string> rightArw;
vector<string> v;
vector<string> tokens;

void die(string s);
void movecursorto();
void disableRawMode();
void enableRawMode();
void goBack();
void goForward();
int readKey();
void processKeyPress();
void clearScreen();
vector<string> getListFilesandDirect();
void display_file_details(string file);
void display_directory_details(vector<string>);
int getCursorPosition(int *r,int *c);
int getWindowSize(int *r,int *c);
void initEditor();
void movetoparent();
void onEnter();
void gotohome();
void scrollUp();
void scrollDown();
void normalMode();
void commandMode();
void executeCommand(string);
void processCommand();
void rename_file();
void copyfiles();
void copy(string);
void executeCommand(string);
string getabsolutepath(string);
void create_file();
void create_dir();
int delete_file();
void delete_dir(string);
void gotolocation();
bool search(string ,string );
int readKeycmdmode();

int max(int,int);
int min(int,int);

int max(int a,int b)
{
    if(a>b)
        return a;
    return b;
}
int min(int a,int b)
{
    if(a<b)
        return a;
    return b;
}
void die(string s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    cout<<s;
    exit(1);
}
void movecursorto()
{
    if(E.cx==-1)
    {
        E.cx++;
        return;
    }
    if(E.cy==-1)
    {
        E.cy++;
        return;
    }
    string cur_pos="\x1b[";
    cur_pos+=to_string(E.cx+1)+";"+to_string(E.cy+1);
    cur_pos+="H";
    write(STDOUT_FILENO, cur_pos.c_str(), cur_pos.length());
    //cout<<"cursor moved"<<endl;
}
void disableRawMode()
{
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&E.orig_termios)==-1)
    die("tc set error");
}
void enableRawMode()
{
    if(tcgetattr(STDIN_FILENO,&E.orig_termios)==-1)
    die("tc get error");
    atexit(disableRawMode);
    struct termios raw=E.orig_termios;

    raw.c_lflag &= ~(ECHO | ICANON);
    // raw.c_iflag&=~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // raw.c_oflag&=~(OPOST);
    // raw.c_cflag|=(CS8);
    // raw.c_lflag&=~(ECHO | ICANON | IEXTEN | ISIG);
    
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw)==-1)
    die("tc set error");
}
void goBack()
{
    if(leftArw.size()==0)
        return;
    string prev=leftArw.top();
    leftArw.pop();
    rightArw.push(current_directory);
    current_directory=prev;
    v=getListFilesandDirect();

    VR.top=0;
    VR.bottom=min(20,v.size());

    display_directory_details(v);
}
void goForward()
{
    if(rightArw.size()==0)
        return;
    string next=rightArw.top();
    rightArw.pop();
    leftArw.push(string(current_directory));
    current_directory=next;
    v=getListFilesandDirect();

    VR.top=0;
    VR.bottom=min(20,v.size());

    display_directory_details(v);
}
int readKeycmdmode()
{
    int nr;
    char c;

    while((nr=read(STDIN_FILENO,&c,1))!=1)
    {
        if(nr==-1 && errno!=EAGAIN)
            die("read");
    }
    return c;
}
int readKey()
{
    int nr;
    char c;

    while((nr=read(STDIN_FILENO,&c,1))!=1)
    {
        if(nr==-1 && errno!=EAGAIN)
            die("read");
    }
    if(c=='\x1b')
    {
        char inp_buf[3];
        if(read(STDIN_FILENO,&inp_buf[0],1)!=1)
            return '\x1b';
        if(read(STDIN_FILENO,&inp_buf[1],1)!=1)
            return '\x1b';
        if(inp_buf[0]=='[')
        {
            switch(inp_buf[1])
            {
                case 'A':return ARROW_UP;
                case 'B':return ARROW_DOWN;
                case 'C':return ARROW_RIGHT;
                case 'D':return ARROW_LEFT;
                default:return '\x1b';
            }
        }
    }
    return c;
}
void gotohome()
{
    leftArw.push(current_directory);
    current_directory=home;
    v=getListFilesandDirect();

    VR.top=0;
    VR.bottom=min(20,v.size());

    display_directory_details(v);
}
void onEnter()
{

    struct stat sb;
        string next=v[VR.top+E.cx];
        string filepath=current_directory+"/"+next;
        if(stat(filepath.c_str(),&sb)!=-1)
        {
            if(S_ISDIR(sb.st_mode))
            {
                
                if(next=="..")
                {
                    movetoparent();
                    return;
                }
                if(next==".")
                    return;
                
                leftArw.push(current_directory);
                current_directory=filepath;
                v=getListFilesandDirect();
                VR.top=0;
                VR.bottom=min(20,v.size());
                display_directory_details(v);
            }
            else
            {
                pid_t pid=fork();
                if(pid==0)
                {
                    // printAlertLine("File opened");
                    execl("/usr/bin/xdg-open","xdg-open",filepath.c_str(),NULL);
                }
            }
        }
}
void processKeyPress()
{
    int c=readKey();
    switch(c)
    {
        case 'q':
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
        break;
        case ARROW_UP:
        E.cx--;
        movecursorto();
        break;
        case ARROW_DOWN:
        E.cx++;
        movecursorto();
        break;
        case ARROW_LEFT:
        goBack();
        break;
        case ARROW_RIGHT:
        goForward();
        break;
        case 10://Clicked Enter
        onEnter();
        break;
        case 127:
        movetoparent();
        break;
        case 'h':
        gotohome();
        break;
        case 'k':
        scrollUp();
        break;
        case 'l':
        scrollDown();
        break;
        case ':':
        E.commandmode=1;
        E.cx=25;
        movecursorto();
        break;
        // default:cout<<c<<endl;
    }
    
    
}

void movetoparent()
{
    //check if its root
    if(current_directory.size()==0)
    {
        // cout<<"you are at root"<<endl;
        return;
    }
    int idx=current_directory.find_last_of("/");
    if(idx==0)  /*  /home    case*/
    {
        idx=1;
    }
    //If not
    

    leftArw.push(current_directory);
    current_directory=current_directory.substr(0,idx);
    v=getListFilesandDirect();
    VR.top=0;
    VR.bottom=min(20,v.size());
    display_directory_details(v);
    // char pwd[FILENAME_MAX];
    // getcwd(pwd,FILENAME_MAX);
    // current_directory=string(pwd);
    if(current_directory.size()==1)
        current_directory="";
}
void clearScreen()
{
    write(STDOUT_FILENO,"\x1b[2J",4);
    write(STDOUT_FILENO,"\x1b[H",3);

}
vector<string> getListFilesandDirect()
{

    v.clear();
    DIR *dr;
    dr=opendir(current_directory.c_str());
    if(dr!=NULL)
    {
        while((d=readdir(dr))!=NULL)
        {
            v.push_back(d->d_name);
        }
        sort(v.begin(),v.end());
    }
    VR.top=0;
    VR.bottom=min(20,v.size());
    closedir(dr);
    return v;
}
void display_file_details(string file)
{

    string filepath=current_directory+"/"+file;
    struct stat sb;
    if(stat(filepath.c_str(),&sb)!=-1)
    {
        string details=file+"\t";
        string type="-";
        auto mode=sb.st_mode;
        if(S_ISDIR(mode))
            type="d";
        else if(S_ISCHR(mode))
            type="c";
        else if(S_ISBLK(mode))
            type="b";
        else if(S_ISFIFO(mode))
            type="p";
        else if(S_ISLNK(mode))
            type="l";
        else if(S_ISSOCK(mode))
            type="s";
        
        details+=type;
        


        details+=(sb.st_mode & S_IRUSR)?"r":"-";
        details+=(sb.st_mode & S_IWUSR)?"w":"-";
        details+=(sb.st_mode & S_IXUSR)?"x":"-";

        details+=(sb.st_mode & S_IRGRP)?"r":"-";
        details+=(sb.st_mode & S_IWGRP)?"w":"-";
        details+=(sb.st_mode & S_IXGRP)?"x":"-";

        details+=(sb.st_mode & S_IROTH)?"r":"-";
        details+=(sb.st_mode & S_IWOTH)?"w":"-";
        details+=(sb.st_mode & S_IXOTH)?"x":"-";

        details+="\t";

        struct passwd *p=getpwuid(sb.st_uid);
        struct group *g=getgrgid(sb.st_uid);
        if(p!=0)
        {
        	string temp(p->pw_name);
        	details+=temp+"\t";
        }
        if(g!=0)
        {
        	string temp(g->gr_name);
        	details+=temp+"\t";
        }

        cout<<details;
        long long file_size=sb.st_size;
        if(file_size>=(1<<30))
            printf("%5lldGB",file_size/(1<<30));
        else if(file_size>=(1<<20))
            printf("%5lldMB",file_size/(1<<20));
        else if(file_size>=(1<<10))
            printf("%5lldKB",file_size/(1<<10));
        else
            printf("%5lldB",file_size);
        char *mod_time=ctime(&sb.st_mtime);
        mod_time[strlen(mod_time)-1]='\0';
        string mtime(mod_time);
        cout<<"\t"<<mtime<<endl;
    }
}
void display_directory_details(vector<string> v)
{
    clearScreen();
    for(int i=VR.top;i<min(v.size(),VR.bottom);i++)
    {
        display_file_details(v[i]);
    }
    // cout<<current_directory<<endl;
    E.cx=0;
    E.cy=0;
    movecursorto();
}
int getCursorPosition(int *r,int *c)
{
    char buf[32];
    unsigned int i = 0;
    if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) 
        return -1;
    while (i < sizeof(buf) - 1) 
    {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) 
            break;
        if (buf[i] == 'R') 
            break;
        i++;
    }
    buf[i] = '\0';
    if (buf[0] != '\x1b' || buf[1] != '[') 
        return -1;
    if (sscanf(&buf[2], "%d;%d", r, c) != 2) 
        return -1;
    return 0;
}
int getWindowSize(int *r,int *c)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) 
    {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) 
            return getCursorPosition(r,c);
        return -1;
    } 
    else 
    {
        *c = ws.ws_col;
        *r = ws.ws_row;
        return 0;
    }
}
void initEditor()
{
    E.cx=0;
    E.cy=0;
    if(getWindowSize(&E.rows,&E.cols)==-1)
        die("Window size error");
}
void scrollUp()
{
    if(VR.top==0)
        return;
    VR.top--;
    VR.bottom--;
    display_directory_details(v);
    // cout<<E.rows<<" "<<E.cols<<endl;
}
void scrollDown()
{
    if(VR.bottom==v.size())
        return;
    VR.top++;
    VR.bottom++;
    display_directory_details(v);
}
void normalMode()
{
    while(!E.commandmode)
    {
        processKeyPress();
    }
    commandMode();
}

string getabsolutepath(string p)
{
    string path;
    if(p[0]=='.'&&p[1]!='.')
    {
        path=current_directory;
        p=p.substr(1);
    }
    else if(p[0]=='~')
    {
        path=home;
        p=p.substr(1);
    }
    else if(p[0]!='/')
    {
        path=current_directory;
    }
    vector<string> pf;
    stringstream ss1(path);
    stringstream ss2(p);
    string tkn;
    while(getline(ss1,tkn,'/'))
    {
        pf.push_back(tkn);
    }
    while(getline(ss2,tkn,'/'))
    {
        if(tkn=="..")
            pf.pop_back();
        else
            pf.push_back(tkn);
    }
    path="";
    for(string x:pf)
    {
        path=path+"/"+x;
    }
    return path;
}
void rename_file()
{
    rename(tokens[1].c_str(),tokens[2].c_str());
    v=getListFilesandDirect();
}
void copyfiles()
{
    // string d=getabsolutepath(tokens.back());
    for(int i=1;i<tokens.size()-1;i++)
    {
        string filepath=getabsolutepath(tokens[i]);
        // cout<<filepath<<endl;

        struct stat sb;
        if(stat(filepath.c_str(),&sb)!=-1)
        {
            auto mode=sb.st_mode;
            if(S_ISDIR(mode))
            {
                DIR *dr;
                dr=opendir(filepath.c_str());
                if(dr!=NULL)
                {
                    while((d=readdir(dr))!=NULL)
                    {
                        string cpfile(d->d_name);
                        if(cpfile!="." && cpfile!="..")
                            copy(filepath+"/"+cpfile);

                    }
                }
                closedir(dr);
            }
            else
            {
                
                copy(filepath);

            }
        }
    }
}
void copy(string s)
{
    // s=getabsolutepath(s);
    string d=getabsolutepath(tokens.back());


    // cout<<d<<endl;
    size_t idx=s.find_last_of('/');
    string filename=s.substr(idx+1);
    struct stat sb;
    if(stat(d.c_str(),&sb)!=-1 && S_ISDIR(sb.st_mode))
    {
        d=d+"/"+filename;
    }
    // cout<<filename<<endl;
    FILE *source,*destn;
    source=fopen(s.c_str(),"r");
    destn=fopen(d.c_str(),"w");
    if(source==NULL)
    {
        perror("Source doesnt exist");
        return;
    }
    if(destn==NULL)
    {
        perror("Destnation doesnt exist");
        return;
    }
    char c;
    while((c=getc(source))!=EOF)
    {
        putc(c,destn);
    }
    struct stat original;
    stat(s.c_str(),&original);
    chmod(d.c_str(),original.st_mode);
    chown(d.c_str(),original.st_uid,original.st_gid);
    fclose(source);
    fclose(destn);

}
void gotolocation()
{
    current_directory=getabsolutepath(tokens[1]);
    // cout<<"Path retn"<<endl;
    v=getListFilesandDirect();
    display_directory_details(v);
    E.cx=25;
    movecursorto();
}
bool search(string directory,string searchfile)
{
    DIR *dr;
    dr=opendir(directory.c_str());
    if(dr!=NULL)
    {
        while((d=readdir(dr))!=NULL)
        {
            string cpfile(d->d_name);
            if(searchfile==cpfile)
                return true;
            struct stat sb;
            if(stat((directory+"/"+cpfile).c_str(),&sb)!=-1 && S_ISDIR(sb.st_mode))
            {
                if(cpfile=="."||cpfile=="..")
                    continue;
                bool r=search(directory+"/"+cpfile,searchfile);
                if(r)
                    return r;
            }
        }
        closedir(dr);
    }
    return false;
}
void move()
{
    copyfiles();
    for(int i=1;i<tokens.size()-1;i++)
    {
        string destn=getabsolutepath(tokens[i]);
        int status=unlink(destn.c_str());
    }
}
void executeCommand(string command)
{
    stringstream ss(command);
    string tkn;
    
    tokens.clear();
    while(getline(ss,tkn,' '))
    {
        tokens.push_back(tkn);
    }
    tkn=tokens[0];
    if(tkn=="copy")
    {
        // cout<<"Called copyfiles"<<endl;
        copyfiles();
        cout<<"\nFile(s) copied"<<endl;
    }
    else if(tkn=="move")
    {
        move();
        cout<<"\nFile(s) moved"<<endl;
    }
    else if(tkn=="rename")
    {
        rename_file();
    }
    else if(tkn=="create_file")
    {
        create_file();
        cout<<"\nFile(s) created"<<endl;
    }
    else if(tkn=="create_dir")
    {
        create_dir();
        cout<<"\nDirectory created"<<endl;
    }
    else if(tkn=="delete_file")
    {
        delete_file();
        cout<<"\nFile(s) deleted"<<endl;
    }
    else if(tkn=="delete_dir")
    {
        
        string destn=getabsolutepath(tokens[1]);
        delete_dir(destn);
        rmdir(destn.c_str());
        cout<<"\ndir deleted"<<endl;
    }
    else if(tkn=="goto")
    {
        gotolocation();
    }
    else if(tkn=="search")
    {
        if(search(current_directory,tokens[1]))
            cout<<"\nTrue"<<endl;
        else
            cout<<"\nFalse"<<endl;
    }
    else if(tkn=="q")
    {
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);

    }
}
void processCommand()
{
    string command;
    while(E.commandmode)
    {
    char c=readKeycmdmode();
    switch(c)
    {
        case 10:
        if(command.size()>0)
        {
        executeCommand(command);
        // cout<<command<<endl;
        command.clear();
        }
        
        display_directory_details(v);
        E.cx=25;
        movecursorto();
        break;
        case 27:
        display_directory_details(v);
        E.commandmode=0;
        E.cx=0;
        E.cy=0;
        movecursorto();
        break;
        // break;
        case 127:command.pop_back();
        display_directory_details(v);
        E.cx=25;
        movecursorto();
        cout<<command;
        fflush(0);
        break;
        default:command.push_back(c);
        display_directory_details(v);
        E.cx=25;
        movecursorto();
        cout<<command;
        fflush(0);
    }
    }
}
void create_file()
{
    string destn=getabsolutepath(tokens.back());
    for(int i=0;i<tokens.size()-1;i++)
    {
        string filename=tokens[i];
        open((destn+"/"+filename).c_str(),O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
    }
}
void create_dir()
{
    string destn=getabsolutepath(tokens.back());
    for(int i=0;i<tokens.size()-1;i++)
    {
        string filename=tokens[i];
        mkdir((destn+"/"+filename).c_str(),S_IRUSR|S_IWUSR|S_IXUSR);
    }
}
int delete_file()
{
    string destn=getabsolutepath(tokens[1]);
    int status=unlink(destn.c_str());
    return status;
}
void delete_dir(string directory)
{
    DIR *dr;
    dr=opendir(directory.c_str());
    if(dr!=NULL)
    {
        while((d=readdir(dr))!=NULL)
        {
            string cpfile(d->d_name);
            if(cpfile=="."||cpfile=="..")
                continue;
            struct stat sb;

            if(stat((directory+"/"+cpfile).c_str(),&sb)!=-1)
            {
                if(S_ISDIR(sb.st_mode))
                {
                    if(cpfile=="."||cpfile=="..")
                        continue;
                    // cout<<cpfile<<endl;
                    delete_dir(directory+"/"+cpfile);
                    rmdir((directory+"/"+cpfile).c_str());
                }
                else
                {
                    // cout<<cpfile<<endl;
                    unlink((directory+"/"+cpfile).c_str());
                }
            }
        }
        closedir(dr);
    }
}
void commandMode()
{
    
    processCommand();
    normalMode();
}
int main()
{
    enableRawMode();
    initEditor();

    v=getListFilesandDirect();
    char pwd[FILENAME_MAX];
    getcwd(pwd,FILENAME_MAX);
    current_directory=string(pwd);
    home=current_directory;
    // cout<<v.size()<<endl;
    VR.top=0;
    VR.bottom=min(20,v.size());
    display_directory_details(v);
    
    // string currentWorkingDir(current_directory);
    // cout<<currentWorkingDir<<endl;
    movecursorto();
    normalMode();

    
    return 0;
}
