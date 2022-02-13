#include<iostream>
#include<vector>
#include<thread>
#include<array>
#include<utility>
#include"asio.hpp"
#include"wincontrol.hpp"

void win_control::cls()
{
    win_control::setColor(win_control::Color::c_BLACK,win_control::Color::c_BLACK);
    win_control::goxy(0, 0);
    for (short i = 0; i < 40; i++)
    {
        std::cout << "                                                                                \n";
    }
    win_control::setColor(win_control::Color::c_WHITE,win_control::Color::c_BLACK);
    win_control::goxy(0,0);
}
const int PORT =11451, WIDTH = 40, HEIGHT = 40, MAXCLIENT = 10;
volatile bool gameRunning;
int mode;
win_control::Color colors[]={
    win_control::Color::c_DGREY,
    win_control::Color::c_RED,
    win_control::Color::c_DARKBLUE,
    win_control::Color::c_LYELLOW,
    win_control::Color::c_DGREEN,
    win_control::Color::c_LIME,
    win_control::Color::c_SKYBLUE,
    win_control::Color::c_LIGHTBLUE,
    win_control::Color::c_DPURPLE,
    win_control::Color::c_PURPLE,
    win_control::Color::c_DRED,
};
asio::io_context ioc;
void sendMessage(asio::ip::tcp::socket &soc, const std::string &msg)
{
    try
    {
        asio::write(soc, asio::buffer(msg));
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
}
namespace Server{
    asio::ip::tcp::acceptor ac(ioc);
    std::thread acp;
    asio::ip::tcp::socket disp_sock(ioc); 
    struct Clnt{
        asio::ip::tcp::socket sock;
        int id,color,x,y;
        bool closed;
        Clnt():sock(ioc),id(0),color(0),closed(false){}
    };
    std::vector<Clnt*> clients;

    bool colorVis[20];

    int clientId;
    
    void sendToAll(const std::string &msg,int ex=0){
        if(clients.empty())return;
        for(auto &cl:clients){
            if(cl->closed||cl->id==ex)continue;
            std::cout<<"Send to SB "<<cl->id<<" "<<msg<<std::endl;
            sendMessage(cl->sock,msg);
        }
    }
    void closeAll(){
        if(clients.empty())return;
        for(auto &cl:clients){
            if(!cl->closed)cl->closed=true,cl->sock.close();
        }
    }
    namespace Game{
        int gmap[45][45];
        bool beOutOfBound(int x,int y){
            return x<0||y<0||x>=WIDTH||y>=HEIGHT;
        }
    }
    bool checkLegal(){
        return true;
    }

    void startServer(){
        clientId=0;
        try{
        ac=asio::ip::tcp::acceptor(ioc,asio::ip::tcp::endpoint(asio::ip::tcp::v4(), PORT));
        }catch(...){std::cout<<"sbabs";}
        try{
        acp=std::thread([&]()->void{
            for(;gameRunning;){
                if(clients.size()<MAXCLIENT){
                    Clnt *cl=new Clnt();
                    cl->sock=ac.accept(),cl->id=++clientId;
                    for(int i=1;i<=MAXCLIENT;i++)if(!colorVis[i]){
                        cl->color=i,colorVis[i]=true;break;
                    }
                    int x=rand()%40,y=rand()%40;
                    while(Game::gmap[x][y])x=rand()%40,y=rand()%40;
                    cl->x=x,cl->y=y;

                    std::cout<<"SB "<<cl->id<<": "<<cl->color<<std::endl;
                    clients.push_back(std::move(cl));
                    sendToAll("join;"+std::to_string(cl->id)+";"+std::to_string(cl->color)+";",cl->id);
                    sendMessage(cl->sock,"info;"+std::to_string(cl->id)+";"+std::to_string(cl->color)+";"+std::to_string(cl->x)+";"+std::to_string(cl->y)+";");
                    
                    std::string usersInfo="uinf;";
                    for()

                    std::thread recv=std::thread([cl]()->void{
                        std::array<char, 65536> buf;
                        asio::error_code err;
                        size_t len;
                        for(;gameRunning;){

                            // std::cout<<"SB "<<cl->id<<": "<<cl->color<<std::endl;
                            // while(1);
                            len = cl->sock.read_some(asio::buffer(buf), err);
                            if(std::string(buf.data())=="quit"){
                                std::cout<<"SB "<<cl->id<<" is leaving\n";
                                cl->sock.close();
                                cl->closed=true;
                                colorVis[cl->color]=false;
                                
                                sendToAll("quit;"+std::to_string(cl->id)+";");
                                
                                return;
                            }
                            else{
                                std::string rec=std::string(buf.data()).substr(0,2);
                                if(rec=="up"){

                                }else if(rec=="dn"){

                                }else if(rec=="lf"){

                                }else if(rec=="rt"){

                                }
                                sendToAll(std::string(buf.data()));
                            }
                        }
                    });
                    recv.detach();//?

                    std::cout<<"SB "<<cl->id<<" is coming\n";
                }
                else{
                    disp_sock=ac.accept();
                    sendMessage(disp_sock,"full");
                    disp_sock.close();
                }
            }
        });
        // acp.detach();
        win_control::cls();
        std::cout<<"Hey SB, I'm Listening!\n";
        }catch(...){std::cout<<"FUCK";}
    }
}
namespace Client{
    bool painting;
    namespace Game{
        struct Player{
            int x,y,id,color;
            Player():x(0),y(0){}
        }pls[100];
        int gmap[45][45];
        bool beOutOfBound(int x,int y){
            return x<0||y<0||x>=WIDTH||y>=HEIGHT;
        }
    }
    int x,y,id,color;
    asio::ip::tcp::socket sock(ioc);
    std::thread recv;

    void connect(std::string ipaddr){
        try{
            
            asio::connect(sock,asio::ip::tcp::resolver(ioc).resolve(ipaddr,std::to_string(PORT)));
            // asio::steady_timer timer(ioc);timer.expires_from_now();
        }catch(...){
            win_control::cls();
            std::cout<<"Connection Error!\n";
            win_control::sleep(1000);
            exit(0);
        }
        recv=std::thread([&]()->void{
            asio::ip::tcp::socket &soc=sock;
            std::array<char,65536> buf;
            asio::error_code err;
            size_t len;
            for(;gameRunning;){
                len=sock.read_some(asio::buffer(buf),err);
                if(std::string(buf.data())=="shutdown"){
                    win_control::cls();
                    sock.close();
                    std::cout<<"Server Shutdown\n";
                    win_control::sleep(1000);
                    gameRunning=false;
                    exit(0);
                }
                else if(std::string(buf.data()).substr(0,4)=="quit"){

                }else if(std::string(buf.data()).substr(0,4)=="info"){
                    
                }else if(std::string(buf.data()).substr(0,4)=="uinf"){

                }else{

                }
            }
        });
        recv.detach();
        win_control::cls();
        std::cout<<"Connected\n";
    }

    void startClient(std::string ipAddress=""){
        win_control::cls();

        if(ipAddress==""){
            std::cout<<"Input your fucking ipaddress: ";
            std::cin >> ipAddress;
        }
        connect(ipAddress);
    }
}
void win_control::sendQuitMessage(){
    // std::cout<<"FYCK\n";
    if(mode==1){
        Server::sendToAll("shutdown");
    }else if(mode==2){
        sendMessage(Client::sock,"quit");
    }else{

    }
    gameRunning=false;
}
namespace key_handling{
    void up(){
        sendMessage(Client::sock,"up;"+std::to_string(Client::id)+";");
    }
    void dn(){
        sendMessage(Client::sock,"dn;"+std::to_string(Client::id)+";");
    }
    void lf(){
        sendMessage(Client::sock,"lf;"+std::to_string(Client::id)+";");
    }
    void rt(){
        sendMessage(Client::sock,"rt;"+std::to_string(Client::id)+";");
    }
    void esc(){
        win_control::sendQuitMessage();
    }
}

void win_control::input_record::keyHandler(int keyCode){
    if(mode==1)return;
    switch(keyCode){
        case 'W':{
            key_handling::up();
            break;
        }
        case 'S':{
            key_handling::dn();
            break;
        }
        case 'A':{
            key_handling::lf();
            break;
        }
        case 'D':{
            key_handling::rt();
            break;
        }
        case VK_ESCAPE:{
            key_handling::esc();
            break;
        }
        default:return;
    }
}
int main(){
    win_control::consoleInit();
    srand(time(0));
    gameRunning=true;
    std::cout<<"Input a FUCKING number. 1 to server, 2 to client: ";
    std::cin>>mode;
    if(mode==1)
    {
        Server::startServer();
        
    }
    else mode=2,Client::startClient();
    while(gameRunning){
        win_control::input_record::getInput();
    }
    gameRunning=false;
    std::cout<<gameRunning;
    return 0;
}