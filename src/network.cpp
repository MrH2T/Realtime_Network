#include<iostream>
#include<vector>
#include<thread>
#include<array>
#include<time.h>
#include<utility>
#include"asio.hpp"
#include"wincontrol.hpp"

const std::string version="0.0.4";
const int gameTick = 10;
int tick=0;

void win_control::cls()
{
    win_control::setColor(win_control::Color::c_BLACK,win_control::Color::c_BLACK);
    win_control::goxy(0, 0);
    for (short i = 0; i < 40; i++)
    {
        goxy(i,0);
        std::cout<<"                                                                                  ";
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

    int clientId,clientConnected;
    
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

                if(clientConnected<MAXCLIENT){
                    srand(time(0));
                    Clnt *cl=new Clnt();
                    cl->sock=ac.accept(),cl->id=++clientId;
                    clientConnected++;
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
                    std::cout<<"Send to SB "<<cl->id<<" "<<"idcl;"+std::to_string(cl->id)+";"+std::to_string(cl->color)+";"<<std::endl;
                    sendMessage(cl->sock,"idcl;"+std::to_string(cl->id)+";"+std::to_string(cl->color)+";");
                    
                    std::string usersInfo="uinf;";
                    for(auto &cln:clients){
                        if(cln->closed)continue;
                        usersInfo+=info_to_string(cln);
                    }
                    sendMessage(cl->sock,usersInfo);

                    std::thread recv=std::thread([cl]()->void{
                        
                        std::array<char, 65536> buf;
                        asio::error_code err;
                        size_t len;
                        bool versed=false;
                        for(;gameRunning;){

                            // std::cout<<"SB "<<cl->id<<": "<<cl->color<<std::endl;
                            // while(1);
                            len = cl->sock.read_some(asio::buffer(buf), err);
                            std::string str(buf.data());str.resize(len);
                            std::cout<<"SB "<<cl->id<<" send "<<str<<std::endl;

                            auto solveCmd=([&](std::string ss)->bool{
                                if(ss=="")return true;
                                if(ss.substr(0,4)=="vers"){
                                    versed=true;
                                    std::string vers=ss.substr(5);
                                    if(vers!=(version+";")){
                                        
                                        std::cout<<"SB "<<cl->id<<"'s version is "<<vers<<", not "<<version<<std::endl;

                                        sendMessage(cl->sock,"wrvr;");
                                        cl->sock.close();
                                        cl->closed=true;
                                        colorVis[cl->color]=false;

                                        clientConnected--;

                                        Game::delPlayer(cl);

                                        sendToAll("quit;"+std::to_string(cl->id)+";");
                                        return false;

                                    }
                                    return true;
                                }
                                if(!versed){
                                    sendMessage(cl->sock,"wrvr;");
                                    cl->sock.close();
                                    cl->closed=true;
                                    colorVis[cl->color]=false;

                                    clientConnected--;

                                    Game::delPlayer(cl);

                                    sendToAll("quit;"+std::to_string(cl->id)+";");
                                    return false;
                                }
                                if(ss=="regt;"){
                                    std::string usersInfo="uinf;";
                                    for(auto &cln:clients){
                                        if(cln->closed)continue;
                                        usersInfo+=info_to_string(cln);
                                    }
                                    sendMessage(cl->sock,usersInfo);
                                    return true;
                                }
                                if(ss=="quit;"){
                                    std::cout<<"SB "<<cl->id<<" is leaving\n";
                                    // while(1);
                                    cl->sock.close();
                                    cl->closed=true;
                                    colorVis[cl->color]=false;

                                    clientConnected--;

                                    //del player from the map
                                    Game::delPlayer(cl);
                                    
                                    sendToAll("quit;"+std::to_string(cl->id)+";");
                                    
                                    return false;
                                }
                                else{
                                    std::string rec=ss.substr(0,2);
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
                                        sendToAll(rec+";"+std::to_string(cl->id)+";");
                                    }
                                }
                                return true;
                            });
                            str+="sb";len+=2;
                            std::string tmp="";
                            for(int i=0;i<len;i++){
                                if((str[i]>='a')&&(str[i]<='z')){
                                    if(i&&str[i-1]==';'||i==0){

                                        if(!solveCmd(tmp))return;
                                        tmp="";
                                    }
                                }
                                tmp+=str[i];
                            }
                            
                        }
                    });
                    recv.detach();//? i dont know

                    std::cout<<"SB "<<cl->id<<" is coming\n";
                }
                else{
                    disp_sock=ac.accept();
                    sendMessage(disp_sock,"full;");
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
        struct Bullet{
            int x,y,color,dist,tow;//0u 1d 2l 3r
            bool no;
            Bullet():x(0),y(0),color(0),dist(0),tow(0),no(false){}
        };
        std::vector<Bullet> bults;
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
            win_control::goxy(i,0);
            for(int j=0;j<40;j++){
                // win_control::setColor(win_control::Color::c_WHITE,win_control::Color::c_BLACK);
                // std::putchar(' '),std::putchar(' ');
                win_control::setColor(colors[Game::gmap[i][j]],colors[Game::gmap[i][j]]);
                std::putchar(' '),std::putchar(' ');

            }
            win_control::setColor(colors[color],colors[color]);
            std::putchar(' ');
            // std::cout<<color;
            // if(i!=39)std::cout<<std::endl;
        }
        painting=false;
    }

    void drawAt(int x,int y){
        while(painting);
        painting=true;
        win_control::goxy(x,2*y);
        win_control::setColor(colors[Game::gmap[x][y]],colors[Game::gmap[x][y]]);
        std::putchar(' '),std::putchar(' ');
        painting=false;
    }

    void drawBullets(){
        while(painting);
        painting=true;
        if(Game::bults.empty())return void(painting=false);
        for(auto &bul:Game::bults){

        }

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
            win_control::sleep(2000);
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
                // while(painting);
                // painting=true;
                // win_control::setColor(win_control::Color::c_WHITE,win_control::Color::c_BLACK);
                // win_control::goxy(2,90);
                // std::cout<<str;
                // painting=false;
                auto solveCmd=([&](std::string ss)->void{
                    // win_control::goxy(20,90);
                    // std::cout<<ss<<"sfhaiud\naiosdj";
                    if(ss=="")return;
                    if(ss=="full;"){
                        win_control::cls();
                        sock.close();
                        std::cout<<"Server is full\n";
                        win_control::sleep(2000);
                        gameRunning=false;
                        exit(0);
                    }
                    if(ss=="wrvr;"){
                        win_control::cls();
                        sock.close();
                        std::cout<<"Wrong version. Please check your version whether it's right\n";
                        win_control::sleep(2000);
                        gameRunning=false;
                        exit(0);
                    }
                    if(ss=="shutdown;"){
                        win_control::cls();
                        sock.close();
                        std::cout<<"Server Shutdown\n";
                        win_control::sleep(2000);
                        gameRunning=false;
                        exit(0);
                    }
                    else if(ss.substr(0,4)=="quit"){
                        int val=0,tid=0;
                        for(int i=5;i<len;i++){
                            if(ss[i]==';'){
                                tid=val;
                                break;
                            }else{
                                val=val*10+ss[i]-'0';
                            }
                        }
                        for(auto &pl:Game::pls){
                            if(pl.id==tid){

                                //del the player from the map
                                Game::delPlayer(pl);
                                drawAt(pl.x,pl.y);
                                pl.quit=true;

                                break;
                            }
                        }
                    }else if(ss.substr(0,4)=="idcl"){
                        int val=0,ind=0;
                        for(int i=5;i<len;i++){
                            if(ss[i]==';'){
                                if(ind==0)id=val;
                                else if(ind==1)color=val;
                                val=0,ind++;
                            }else{
                                val=val*10+ss[i]-'0';
                            }
                        }
                        win_control::goxy(20,90);
                        win_control::setColor(win_control::Color::c_WHITE,win_control::Color::c_BLACK);
                        // std::cout<<id<<" "<<color;
                        drawMap();
                    }else if(ss.substr(0,4)=="uinf"){
                        // std::cout<<ss<<std::endl;
                        memset(Game::gmap,0,sizeof(Game::gmap));
                        memset(Game::pls,0,sizeof(Game::pls));
                        Game::plcnt=1;
                        int ind=0,val=0;
                        for(int i=5;i<len;i++){
                            if(ss[i]==';'){
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
                                val=val*10+ss[i]-'0';
                            }
                        }
                    }else if(ss.substr(0,4)=="join"){
                        int ind=0,val=0;
                        for(int i=5;i<len;i++){
                            if(ss[i]==';'){
                                if(ind%4==0)Game::pls[Game::plcnt].id=val;
                                else if(ind%4==1)Game::pls[Game::plcnt].color=val;
                                else if(ind%4==2)Game::pls[Game::plcnt].x=val;
                                else if(ind%4==3){
                                    Game::pls[Game::plcnt].y=val;
                                    Game::putPlayer(Game::pls[Game::plcnt]);
                                    drawAt(Game::pls[Game::plcnt].x,Game::pls[Game::plcnt].y);
                                    Game::plcnt++;
                                }
                                ind++,val=0;
                            }else{
                                val=val*10+ss[i]-'0';
                            }
                        }
                    }else{
                        std::string rec=ss.substr(0,2);
                        
                        
                        int val=0,tid=0;
                        for(int i=3;i<len;i++){
                            if(ss[i]==';'){
                                tid=val;
                                break;
                            }
                            else{
                                val=val*10+ss[i]-'0';
                            }
                        }
                        Game::Player *tpl=NULL;
                        for(int i=0;i<Game::plcnt;i++){
                            if(Game::pls[i].id==tid){tpl=&Game::pls[i];break;}
                        }
                        Game::delPlayer(*tpl),drawAt(tpl->x,tpl->y);
                        if(rec=="up")tpl->x--;
                        if(rec=="dn")tpl->x++;
                        if(rec=="lf")tpl->y--;
                        if(rec=="rt")tpl->y++;
                        Game::putPlayer(*tpl),drawAt(tpl->x,tpl->y);
                    }
                });
                str+="sb";len+=2;
                std::string tmp="";
                for(int i=0;i<len;i++){
                    if((str[i]>='a')&&(str[i]<='z')){
                        if(i&&str[i-1]==';'||i==0){
                            solveCmd(tmp);
                            tmp="";
                        }
                    }
                    tmp+=str[i];
                }
            }
        });
        recv.detach();
        sendMessage(sock,"vers;"+version+";");
        // win_control::cls();
        // std::cout<<"Connected\n";
    }

    void startClient(std::string ipAddress=""){
        win_control::cls();

        if(ipAddress==""){
            std::cout<<"Input your fucking ipaddress: ";
            std::cin >> ipAddress;
        }
        memset(Game::gmap,0,sizeof(Game::gmap));
        drawMap();
        connect(ipAddress);
    }
    
}
void win_control::sendQuitMessage(){
    // std::cout<<"FYCK\n";
    if(mode==1){
        Server::sendToAll("shutdown;");
    }else if(mode==2){
        sendMessage(Client::sock,"quit;");
    }else{

    }
    Client::sock.close();
    gameRunning=false;
}
namespace key_handling{
    bool onPress[128];
    void dup(){
        onPress['W']=1;
    }
    void uup(){
        onPress['W']=0;
    }
    void ddn(){
        onPress['S']=1;
    }
    void udn(){
        onPress['S']=0;
    }
    void dlf(){
        onPress['A']=1;
    }
    void ulf(){
        onPress['A']=0;
    }
    void drt(){
        onPress['D']=1;
    }
    void urt(){
        onPress['D']=0;
    }
    void esc(){
        win_control::sendQuitMessage();
    }
    void refresh(){
        sendMessage(Client::sock,"regt;");
        Client::drawMap();
    }
}

void win_control::input_record::keyDownHandler(int keyCode){
    if(mode==1)return;
    switch(keyCode){
        case 'W':{
            key_handling::dup();
            key_handling::udn();
            break;
        }
        case 'S':{
            key_handling::ddn();
            key_handling::uup();
            break;
        }
        case 'A':{
            key_handling::dlf();
            key_handling::urt();
            break;
        }
        case 'D':{
            key_handling::drt();
            key_handling::ulf();
            break;
        }
        case 'R':{
            key_handling::refresh();
            break;
        }
        case VK_ESCAPE:{
            key_handling::esc();
            break;
        }
        default:return;
    }
}
void win_control::input_record::keyUpHandler(int keyCode){
    if(mode==1)return;
    switch(keyCode){
        case 'W':{
            key_handling::uup();
            break;
        }
        case 'S':{
            key_handling::udn();
            break;
        }
        case 'A':{
            key_handling::ulf();
            break;
        }
        case 'D':{
            key_handling::urt();
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
    std::thread listenKey=std::thread([&]()->void{
        while(gameRunning){
            win_control::input_record::getInput();
        }
    });

    int lastMoveTick=-2,lastDrawTick=-2;

    while(gameRunning){
        if(key_handling::onPress['W']&&tick-lastMoveTick>=6){lastMoveTick=tick,sendMessage(Client::sock,"up;");}
        if(key_handling::onPress['S']&&tick-lastMoveTick>=6){lastMoveTick=tick,sendMessage(Client::sock,"dn;");}
        if(key_handling::onPress['A']&&tick-lastMoveTick>=6){lastMoveTick=tick,sendMessage(Client::sock,"lf;");}
        if(key_handling::onPress['D']&&tick-lastMoveTick>=6){lastMoveTick=tick,sendMessage(Client::sock,"rt;");}

        if(tick-lastDrawTick>=3)lastDrawTick=tick,Client::drawBullets();
        tick++;
        win_control::sleep(gameTick);
    }
    gameRunning=false;
    std::cout<<gameRunning;
    return 0;
}