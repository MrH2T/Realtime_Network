#include<iostream>
#include<vector>
#include<thread>
#include<array>
#include<time.h>
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
    win_control::Color::c_WHITE,
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
        bool checkLegal(int x,int y){
            std::cout<<x<<" "<<y<<std::endl;
            if(beOutOfBound(x,y))return false;
            if(gmap[x][y])return false;
            return true;
        }
        void delPlayer(Clnt* cl){
            gmap[cl->x][cl->y]=0;
        }
        void putPlayer(Clnt* cl){
            gmap[cl->x][cl->y]=cl->id;
        }
    }

    std::string info_to_string(Clnt *cl){
        return std::to_string(cl->id)+";"+std::to_string(cl->color)+";"+std::to_string(cl->x)+";"+std::to_string(cl->y)+";";
    }


    void startServer(){
        memset(Game::gmap,0,sizeof(Game::gmap));
        clientId=0;
        try{
        ac=asio::ip::tcp::acceptor(ioc,asio::ip::tcp::endpoint(asio::ip::tcp::v4(), PORT));
        }catch(...){std::cout<<"sbabs";}
        try{
        acp=std::thread([&]()->void{
            for(;gameRunning;){
                if(clients.size()<MAXCLIENT){
                    srand(time(0));
                    Clnt *cl=new Clnt();
                    cl->sock=ac.accept(),cl->id=++clientId;
                    for(int i=1;i<=MAXCLIENT;i++)if(!colorVis[i]){
                        cl->color=i,colorVis[i]=true;break;
                    }
                    int x=rand()%40,y=rand()%40;

                    // x=1,y=1;
                    while(Game::gmap[x][y])x=rand()%40,y=rand()%40;
                    cl->x=x,cl->y=y;
                    std::cout<<"Summon SB "<<cl->id <<" at "<<x<<","<<y<<std::endl;

                    //put player on the map
                    Game::putPlayer(cl);

                    std::cout<<"SB "<<cl->id<<": "<<cl->color<<std::endl;
                    clients.push_back(std::move(cl));
                    sendToAll("join;"+info_to_string(cl),cl->id);
                    sendMessage(cl->sock,"idcl;"+std::to_string(cl->id)+";"+std::to_string(cl->color)+";");
                    
                    std::string usersInfo="uinf;";
                    for(auto &cln:clients){
                        usersInfo+=info_to_string(cln);
                    }
                    sendMessage(cl->sock,usersInfo);

                    std::thread recv=std::thread([cl]()->void{
                        
                        std::array<char, 65536> buf;
                        asio::error_code err;
                        size_t len;
                        for(;gameRunning;){

                            // std::cout<<"SB "<<cl->id<<": "<<cl->color<<std::endl;
                            // while(1);
                            len = cl->sock.read_some(asio::buffer(buf), err);
                            std::string str(buf.data());str.resize(len);
                            std::cout<<"SB "<<cl->id<<" send "<<str<<std::endl;
                            if(str=="quit"){
                                std::cout<<"SB "<<cl->id<<" is leaving\n";
                                cl->sock.close();
                                cl->closed=true;
                                colorVis[cl->color]=false;

                                //del player from the map
                                Game::delPlayer(cl);
                                
                                sendToAll("quit;"+std::to_string(cl->id)+";");
                                
                                return;
                            }
                            else{
                                std::string rec=str.substr(0,2);
                                bool ok=true;
                                int nx=cl->x,ny=cl->y,tx=nx,ty=ny;
                                if(rec=="up"){tx--;if(!Game::checkLegal(nx-1,ny))ok=false;}
                                else if(rec=="dn"){tx++;if(!Game::checkLegal(nx+1,ny))ok=false;}
                                else if(rec=="lf"){ty--;if(!Game::checkLegal(nx,ny-1))ok=false;}
                                else if(rec=="rt"){ty++;if(!Game::checkLegal(nx,ny+1))ok=false;}
                                if(ok){
                                    std::cout<<"SB HAHA "<<nx<<" "<<ny<<";"<<tx<<" "<<ty<<std::endl;
                                    //move player
                                    Game::delPlayer(cl);
                                    cl->x=tx,cl->y=ty;
                                    Game::putPlayer(cl);

                                    sendToAll(str+";"+std::to_string(cl->id)+";");
                                }
                            }
                        }
                    });
                    recv.detach();//? i dont know

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
        std::cout<<"Hi SB, I'm Listening!\n";
        }catch(...){std::cout<<"FUCK";}
    }
}
namespace Client{
    bool painting;
    namespace Game{
        struct Player{
            int x,y,id,color;
            bool quit;
            Player():x(0),y(0),id(0),color(0),quit(false){}
        }pls[128];
        int gmap[45][45],plcnt=0;
        void delPlayer(Player pl){
            gmap[pl.x][pl.y]=0;
        }
        void putPlayer(Player pl){
            gmap[pl.x][pl.y]=pl.color;
        }
    }
    int id,color;
    asio::ip::tcp::socket sock(ioc);
    std::thread recv;

    void drawMap(){
        while(painting);
        painting=true;
        win_control::goxy(0,0);
        for(int i=0;i<40;i++){
            // win_control::goxy(i,0);
            for(int j=0;j<40;j++){
                win_control::setColor(colors[Game::gmap[i][j]],colors[Game::gmap[i][j]]);
                std::cout<<"  ";
            }
            std::cout<<std::endl;
        }
        painting=false;
    }

    void drawAt(int x,int y){
        while(painting);
        painting=true;
        win_control::goxy(x,2*y);
        win_control::setColor(colors[Game::gmap[x][y]],colors[Game::gmap[x][y]]);
        std::cout<<"  ";
        painting=false;
    }

    void connect(std::string ipaddr){
        try{
            
            asio::connect(sock,asio::ip::tcp::resolver(ioc).resolve(ipaddr,std::to_string(PORT)));
            // asio::steady_timer timer(ioc);timer.expires_from_now();
        
            // i want to check if the connection time is out, but it's a bit complex. I'll fix it later.
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
                std::string str=std::string(buf.data());str.resize(len);
                if(str=="shutdown"){
                    win_control::cls();
                    sock.close();
                    std::cout<<"Server Shutdown\n";
                    win_control::sleep(1000);
                    gameRunning=false;
                    exit(0);
                }
                else if(str.substr(0,4)=="quit"){
                    int val=0,tid=0;
                    for(int i=5;i<len;i++){
                        if(str[i]==';'){
                            tid=val;
                            break;
                        }else{
                            val=val*10+str[i]-'0';
                        }
                    }
                    for(auto &pl:Game::pls){
                        if(pl.id==tid){

                            //del the player from the map
                            Game::delPlayer(pl);
                            pl.quit=true;

                            break;
                        }
                    }
                }else if(str.substr(0,4)=="idcl"){
                    int val=0,ind=0;
                    for(int i=5;i<len;i++){
                        if(str[i]==';'){
                            if(ind==0)id=val;
                            else if(ind==1)color=val;
                            val=0,ind++;
                        }else{
                            val=val*10+str[i]-'0';
                        }
                    }
                }else if(str.substr(0,4)=="uinf"){
                    std::cout<<str<<std::endl;
                    int ind=0,val=0;Game::plcnt=1;
                    for(int i=5;i<len;i++){
                        if(str[i]==';'){
                            if(ind%4==0)Game::pls[Game::plcnt].id=val;
                            else if(ind%4==1)Game::pls[Game::plcnt].color=val;
                            else if(ind%4==2)Game::pls[Game::plcnt].x=val;
                            else if(ind%4==3){
                                Game::pls[Game::plcnt].y=val;
                                
                                //put the player on the map
                                Game::putPlayer(Game::pls[Game::plcnt]);
                                
                                drawAt(Game::pls[Game::plcnt].x,Game::pls[Game::plcnt].y);

                                Game::plcnt++;
                            }
                            ind++,val=0;
                        }else{
                            val=val*10+str[i]-'0';
                        }
                    }
                }else if(str.substr(0,4)=="join"){
                    int ind=0,val=0;
                    for(int i=5;i<len;i++){
                        if(str[i]==';'){
                            if(ind%4==0)Game::pls[Game::plcnt].id=val;
                            else if(ind%4==1)Game::pls[Game::plcnt].color=val;
                            else if(ind%4==2)Game::pls[Game::plcnt].x=val;
                            else if(ind%4==3)Game::pls[Game::plcnt].y=val,Game::plcnt++;
                            ind++,val=0;
                        }else{
                            val=val*10+str[i]-'0';
                        }
                    }
                }else{
                    std::string rec=str.substr(0,2);
                    int val=0,tid=0;
                    for(int i=3;i<len;i++){
                        if(str[i]==';'){
                            tid=val;
                            break;
                        }
                        else{
                            val=val*10+str[i]-'0';
                        }
                    }
                    Game::Player &tpl=Game::pls[0];
                    for(auto &pl:Game::pls){
                        if(pl.id==tid){tpl=pl;break;}
                    }
                    Game::delPlayer(tpl),drawAt(tpl.x,tpl.y);
                    if(rec=="up")tpl.x--;
                    if(rec=="dn")tpl.x++;
                    if(rec=="lf")tpl.y--;
                    if(rec=="rt")tpl.y++;
                    Game::putPlayer(tpl),drawAt(tpl.x,tpl.y);
                }
            }
        });
        recv.detach();
        // win_control::cls();
        // std::cout<<"Connected\n";
    }

    void startClient(std::string ipAddress=""){
        win_control::cls();
        memset(Game::gmap,0,sizeof(Game::gmap));

        if(ipAddress==""){
            std::cout<<"Input your fucking ipaddress: ";
            std::cin >> ipAddress;
        }
        drawMap();
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
    Client::sock.close();
    gameRunning=false;
}
namespace key_handling{
    void up(){
        sendMessage(Client::sock,"up");
    }
    void dn(){
        sendMessage(Client::sock,"dn");
    }
    void lf(){
        sendMessage(Client::sock,"lf");
    }
    void rt(){
        sendMessage(Client::sock,"rt");
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