#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termio.h>

#define BUF_SIZE 1024
#define NAME_LEN 20

#define MYTURNSTR "Arrow keys : move\nEnter key : put\n"

void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);

char name[NAME_LEN]="[DEFAULT]";
char read_msg[BUF_SIZE];
char write_msg[BUF_SIZE];
int block = 0;

int board[19][19];
int x = 9,y = 9;
int stone = 0;

static struct termios initial_settings, new_settings;
static int peek_character = -1;


void init_keyboard()
{
    tcgetattr(0,&initial_settings);
    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &new_settings);
}



void close_keyboard()
{
    tcsetattr(0, TCSANOW, &initial_settings);
}



int _kbhit()
{
    unsigned char ch;
    int nread;
    if (peek_character != -1) return 1;
    new_settings.c_cc[VMIN]=0;
    tcsetattr(0, TCSANOW, &new_settings);
    nread = read(0,&ch,1);
    new_settings.c_cc[VMIN]=1;
    tcsetattr(0, TCSANOW, &new_settings);
    if(nread == 1)
    {
        peek_character = ch;
        return 1;
    }
    return 0;
}

int _getch()
{
    char ch;
    if(peek_character != -1)
    {
        ch = peek_character;
        peek_character = -1;
        return ch;
    }
    read(0,&ch,1);
    return ch;
}

void print_board(char* str){
    system("clear");
    printf("%s\n",str);
    for(int i = 0; i < 19; i ++){
        for(int j = 0; j < 19; j++){
            if(x == j && y == i)
                printf("[");
            else
                printf(" ");
            
            if(board[i][j] == 0){
                printf("◻︎");
            }else if(board[i][j] == 1)
                printf("●");
            else if(board[i][j] == 2)
                printf("✖︎");
            
            if(x == j && y == i)
                printf("]");
            else
                printf(" ");
        }
        printf("\n");
    }
}

void init_board(){
    for(int i = 0; i < 19; i ++){
        for(int j = 0; j < 19; j ++){
            board[i][j] = 0;
        }
    }
}

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    int str_len, type;
    if(argc!=4) {
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }
    argv[NAME_LEN] = 0;
    
    sprintf(name, "[%s]", argv[3]);
    sock=socket(PF_INET, SOCK_STREAM, 0);
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));
    
    
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
        error_handling("connect() error");
    
    while(1)
    {
        str_len=read(sock, read_msg, BUF_SIZE);
        read_msg[str_len]=0;
        //printf("[%d] ",str_len);
        if(str_len==0)
            break;
        if(str_len == 1){
            sscanf(read_msg,"%d",&type);
            /*
             if(type == 1){
             block = 1;
             }*/
            block = type;
        }else{
            if(read_msg[0] == '!'){
                system("clear");
                fputs(&read_msg[1], stdout);
            }else if(read_msg[0] == '#'){
                sscanf(&read_msg[1], "%d", &stone);
                init_board();
                x= 9;
                y= 9;
                print_board("");
            }else if(read_msg[0] == '&'){
                int ox, oy;
                printf("%s\n",read_msg);
                sscanf(&read_msg[1], "%d%d", &ox,&oy);
                board[oy][ox] = stone == 1 ? 2 : 1;
            }else if(read_msg[0] == '~'){
                print_board(&read_msg[1]);
            }else{
                fputs(read_msg, stdout);
            }
            continue;
        }
        init_keyboard();
        new_settings.c_cc[VMIN]=0;
        tcsetattr(0, TCSANOW, &new_settings);
        while(1){
            int nread = read(0,NULL,1);
            if(nread == 0)
                break;
        }
        new_settings.c_cc[VMIN]=1;
        tcsetattr(0, TCSANOW, &new_settings);
        close_keyboard();
        if(block == 1)
            printf("select room number (refresh : r, ranking : ranking): ");
        //printf("-block : %d- \n",block);
        write_msg[0] = 0;
        
        if(block == 3){
            print_board(MYTURNSTR);
            init_keyboard();
            while (1) {
                if (_kbhit()) {
                    int ch = _getch();
                    if(ch == 27){
                        if(_getch() == 91){
                            ch = _getch();
                            // key DOWN
                            if(ch == 66){
                                if(y < 18){
                                    y ++;
                                     print_board(MYTURNSTR);
                                }
                            }
                            //key UO
                            else if(ch == 65){
                                if(y > 0){
                                    y --;
                                    print_board(MYTURNSTR);
                                }
                            }
                            //key right
                            else if(ch == 67){
                                if(x < 18){
                                    x ++;
                                    print_board(MYTURNSTR);
                                }
                            }
                            // key left
                            else if(ch == 68){
                                if(x > 0){
                                    x --;
                                    print_board(MYTURNSTR);
                                }
                            }
                        }
                    }else if (ch == 10){
                        if(board[y][x] == 0){
                            board[y][x] = stone;
                            print_board("Opponent's turn");
                            break;
                        }
                    }
                }
            }
            close_keyboard();
            sprintf(write_msg, "%d %d", x, y);
        }else{
            fgets(write_msg, BUF_SIZE, stdin);
        }
        
        if(!strcmp(write_msg,"q\n") || !strcmp(write_msg,"Q\n"))
            break;
        
        if(block == 1 && strcmp(write_msg,"ranking") != 0){
            sprintf(write_msg,"%s %s\n", write_msg, name);
        }
        block = 0;
        write(sock, write_msg, strlen(write_msg));
    }
    
    close(sock);
    return 0;
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
