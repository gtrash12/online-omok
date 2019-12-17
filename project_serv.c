#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/select.h>
#include <ctype.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>

#define BUF_SIZE 1024
#define MAX_CLNT 256
#define ROOM_MAX 5
#define RANK_NUM 5
#define NAME_LEN 20

struct room{
    int winning_streak;
    int player[2];
    char defender[NAME_LEN];
    char challenger[NAME_LEN];
}typedef room;

struct rank{
    int winning_streak;
    char name[NAME_LEN];
}typedef rank;


void init();
void * play_game(void * arg);
void error_handling(char * msg);
int find(int fd);
void find_and_exit(int fd);
void exit_room(int room_number,int index);
void rank_in(char * name, int winning_streak);
void print_rooms(char * buf);

pthread_mutex_t mutx;
room rooms[ROOM_MAX];
rank ranks[RANK_NUM+1];
fd_set reads;
int fd_max;

void tty_mode(int how){
    static struct termios original_mode;
    if(how == 0)
        tcgetattr(0, &original_mode);
    else
        tcsetattr(0, TCSANOW, &original_mode);
}

void set_cr_noecho_mode(void){
    struct termios ttystate;
    tcgetattr(0, &ttystate);
    ttystate.c_lflag &= ~ICANON;
    ttystate.c_lflag &= ~ECHO;
    ttystate.c_cc[VMIN] = 1;
    
    tcsetattr(0, TCSANOW, &ttystate);
    
}

void print_board(int board[19][19]){
    system("clear");
    for(int i = 0; i < 19; i ++){
        for(int j = 0; j < 19; j++){

                printf(" ");
            
            if(board[i][j] == 0){
                printf("◻︎");
            }else if(board[i][j] == 1)
                printf("●");
            else if(board[i][j] == 2)
                printf("○");
            
                printf(" ");
        }
        printf("\n");
    }
}


int main(int argc, char *argv[])
{
    char buf[BUF_SIZE], buf2[BUF_SIZE];
    char name[NAME_LEN];
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    struct timeval timeout;
    fd_set cpy_reads;
    int str_len, fd_num, i;
    socklen_t clnt_adr_sz;
    pthread_t t_id;
    int recv_room;
    if(argc!=2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    
    init();
    
    pthread_mutex_init(&mutx, NULL);
    serv_sock=socket(PF_INET, SOCK_STREAM, 0);
    
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_adr.sin_port=htons(atoi(argv[1]));
    
    if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind() error");
    if(listen(serv_sock, 5)==-1)
        error_handling("listen() error");
    
    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    fd_max=serv_sock;
    
    
    void ctrl_c_handler(int);
    tty_mode(0);
    signal(SIGINT, ctrl_c_handler);
    
    while(1)
    {
        
        cpy_reads=reads;
        timeout.tv_sec=5;
        timeout.tv_usec=5000;
        
        if((fd_num=select(fd_max+1, &cpy_reads, 0, 0, &timeout))==-1)
            break;
        
        if(fd_num==0)
            continue;

        for(i=0; i<fd_max+1; i++)
        {
            if(FD_ISSET(i, &cpy_reads))
            {
                if(i == serv_sock)
                {
                    clnt_adr_sz = sizeof(clnt_adr);
                    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
                    FD_SET(clnt_sock, &reads);
                    if(fd_max<clnt_sock)
                        fd_max = clnt_sock;
                    printf("connected client: %d \n", clnt_sock);
                    sprintf(buf2, "!Welcome to rock-paper-scissors survivor game!\n");
                    print_rooms(buf2);
                    write(clnt_sock, buf2, strlen(buf2));
                    usleep(10000);
                    write(clnt_sock, "1", strlen("1"));
                }
                else
                {
                    str_len=read(i, buf, BUF_SIZE);
                    buf[str_len] = 0;
                    if(str_len==0)
                    {
                        find_and_exit(i);
                        FD_CLR(i, &reads);
                        close(i);
                        printf("closed client: %d \n", i);
                    }
                    else if(strncmp(buf,"ranking",strlen("ranking")) == 0){
                        buf2[0] = '!';
                        buf2[1] = 0;
                        for(int j = 0; j < RANK_NUM; j++){
                            if(ranks[j].winning_streak == 0)
                                break;
                            sprintf(buf2,"%s %d. %s winning streak : %d\n",buf2, j+1, ranks[j].name, ranks[j].winning_streak);
                        }
                        if(strlen(buf2) == 0)
                            sprintf(buf2,"- Nobody - \n");
                        print_rooms(buf2);
                        write(i, buf2, strlen(buf2));
                        usleep(100000);
                        write(i, "1", strlen("1"));
                    }
                    else if(strncmp(buf,"r",strlen("r")) == 0){
                        buf2[0] = '!';
                        buf2[1] = 0;
                        print_rooms(buf2);
                        write(i, buf2, strlen(buf2));
                        usleep(100000);
                        write(i, "1", strlen("1"));
                    }
                    else
                    {
                        sscanf(buf,"%d %s",&recv_room, buf2);
                        //printf("[%d]\n",recv_room);
                        if(recv_room > ROOM_MAX || rooms[recv_room].player[1] != -1 || isdigit(buf[0]) == 0){
                            write(i, "!The room is not empty or not exist!\n", strlen("!The room is not empty or not exist!\n"));
                            usleep(100000);
                            buf2[0] = 0;
                            print_rooms(buf2);
                            write(i, buf2, strlen(buf2));
                            usleep(100000);
                            write(i, "1", strlen("1"));
                            continue;
                        }else{
                            if(rooms[recv_room].player[0] == -1){
                                rooms[recv_room].player[0] = i;
                                sscanf(buf2,"%s",rooms[recv_room].defender);
                                sprintf(buf2,"!please wait for challenger... \n");
                                //usleep(10000000);
                                write(i, buf2, strlen(buf2));
                                //
                                //write(i, "2", strlen("2"));
                            }else{
                                rooms[recv_room].player[1] = i;
                                sscanf(buf2,"%s",rooms[recv_room].challenger);
                                sprintf(buf,"!%s vs %s\n  - get ready for the battle -\n",rooms[recv_room].defender,rooms[recv_room].challenger);
                                write(i, buf, strlen(buf));
                                write(rooms[recv_room].player[0], buf, strlen(buf));
                                
                                FD_CLR(i, &reads);
                                FD_CLR(rooms[recv_room].player[0], &reads);
                                pthread_create(&t_id, NULL, play_game, (void*)&recv_room);
                                pthread_detach(t_id);
                                usleep(10000);
                                for(int j =0; j<fd_max+1; j++)
                                {
                                    if(FD_ISSET(j, &cpy_reads) && j != i && j != serv_sock)
                                    {
                                        buf2[0] = 0;
                                        print_rooms(buf2);
                                        write(j, buf2, strlen(buf2));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    tty_mode(1);
    pthread_mutex_destroy(&mutx);
    close(serv_sock);
    return 0;
}
void ctrl_c_handler(int signum){
    tty_mode(1);
    pthread_mutex_destroy(&mutx);
    exit(1);
}

void init(){
    for(int i = 0; i < ROOM_MAX; i ++){
        rooms[i].winning_streak = 0;
        rooms[i].challenger[0] = 0;
        rooms[i].defender[0] = 0;
        rooms[i].player[0] = -1;
        rooms[i].player[1] = -1;
    }
    for(int i = 0; i < RANK_NUM; i++)
        ranks[i].winning_streak = 0;
}

void print_rooms(char * buf){
    sprintf(buf,"%s - LOBBY -\n",buf);
    for(int i = 0; i < ROOM_MAX; i ++){
        if(rooms[i].player[0] == -1){
            sprintf(buf,"%s[Room %d] empty\n",buf, i);
        }
        else if(rooms[i].player[1] != -1){
            sprintf(buf,"%s[Room %d] %s vs %s Playing\n",buf, i,rooms[i].defender, rooms[i].challenger);
        }
        else{
            sprintf(buf,"%s[Room %d] %s wait (winning streak : %d )\n",buf, i,rooms[i].defender, rooms[i].winning_streak);
        }
    }
}

void rank_in(char * name, int winning_streak){
    int i;
    int prev = RANK_NUM;
    for(i = 0; i < RANK_NUM; i ++){
        if(strcmp(ranks[i].name,name) == 0){
            prev = i;
        }
    }
    for(i = RANK_NUM; i > 0; i --){
        if(ranks[i-1].winning_streak < winning_streak){
            if(i - 1 > prev)
                continue;
            if(strcmp(ranks[i-1].name,name) == 0){
                ranks[i-1].winning_streak = winning_streak;
            }else{
                ranks[i].winning_streak = ranks[i-1].winning_streak;
                strcpy(ranks[i].name,ranks[i-1].name);
            }
        }else
            break;
    }
    ranks[i].winning_streak = winning_streak;
    strcpy(ranks[i].name,name);
    
    //여따가 ranks 배열 파일로 출력하면 됨
    //그리고 main 에서 암때나 랭킹파일 읽어서 ranks 에 고대로 다시 저장되게 하셈
}

int find(int fd){
    for(int i = 0; i < ROOM_MAX; i++){
        if(rooms[i].player[0] == fd){
            return i;
        }
    }
    return -1;
}

void find_and_exit(int fd){
    int room_number;
    if((room_number = find(fd)) == -1)
        return;
    
    rooms[room_number].player[0] = -1;
    rooms[room_number].winning_streak = 0;
    rooms[room_number].challenger[0] = 0;
}

void exit_room(int room_number,int index){
    if(index == 0){
        rooms[room_number].player[0] = rooms[room_number].player[1];
        strcpy(rooms[room_number].defender,rooms[room_number].challenger);
    }
    rooms[room_number].player[1] = -1;
    rooms[room_number].challenger[0] = 0;
}

int check_five(int board[19][19], int x, int y, int stone){
    int ser = 0;
    int sx, sy;
    //좌우체크
    sx = x-4;
    sy = y;
    for(int i = 0 ; i < 19; i ++){
        if(sx + i < 0 || sx + i >18)
            continue;
        if(board[sy][sx + i] == stone){
            ser ++;
            if(ser >= 5)
                return 1;
        }
        else
            ser = 0;
    }
    //상하체크
    ser = 0;
    sx = x;
    sy = y-4;
    for(int i = 0 ; i < 19; i ++){
        if(sy + i < 0 || sy + i > 18)
            continue;
        if(board[sy + i][sx] == stone){
            ser ++;
            if(ser >= 5)
                return 1;
        }
        else
            ser = 0;
    }
    // \대각선체크1
    ser = 0;
    sx = x-4;
    sy = y-4;
    for(int i = 0 ; i < 19; i ++){
        if(sx + i < 0 || sx + i >18 || sy + i < 0 || sy + i > 18)
            continue;
        if(board[sy + i][sx + i] == stone){
            ser ++;
            if(ser >= 5)
                return 1;
        }
        else
            ser = 0;
    }
    // /대각선체크
    ser = 0;
    sx = x-4;
    sy = y+4;
    for(int i = 0 ; i < 19; i ++){
        if(sx + i < 0 || sx + i >18 || sy - i < 0 || sy - i > 18)
            continue;
        if(board[sy - i][sx + i] == stone){
            ser ++;
            if(ser >= 5)
                return 1;
        }
        else
            ser = 0;
    }
    
    //
    return 0;
    
}

void * play_game(void * arg)
{
    int room_number=*((int*)arg);
    int str_len=0, i;
    char msg[BUF_SIZE];
    int x;
    int y;
    int defender = rooms[room_number].player[0];
    int challenger = rooms[room_number].player[1];
    int board[19][19];
    int win = 0;
    write(defender, "#1", strlen("#1"));
    write(challenger, "#2", strlen("#2"));
    usleep(100000);
    write(defender, "3", strlen("3"));
    write(challenger, "Wait for opponent...\n", strlen("Wait for opponent...\n"));
    while(1){
        str_len = read(defender, msg, sizeof(msg));
        msg[str_len] = 0;
        if(str_len == 0){
            printf("closed client: %d \n", defender);
            str_len = sprintf(msg,"%s leave this room, you win\n",rooms[room_number].challenger);
            exit_room(room_number,0);
            close(defender);
            defender = -1;
            write(challenger,msg,str_len);
            rooms[room_number].winning_streak = 1;
            write(challenger, "please wait for challenger... \n", strlen("please wait for challenger... \n"));
            read(challenger, msg, sizeof(msg));
            break;
        }
        sscanf(msg,"%d %d",&x, &y);
        board[y][x] = 1;
        if(check_five(board, x,y, 1))
            win = 1;
        
        if(win != 1){
            sprintf(msg,"& %d %d",x, y);
            write(challenger, msg, strlen(msg));
            usleep(100000);
            write(challenger, "3", strlen("3"));
            
            str_len = read(challenger, msg, sizeof(msg));
            msg[str_len] = 0;
            if(str_len == 0){
                printf("closed client: %d \n", challenger);
                str_len = sprintf(msg,"%s leave this room, you win\n",rooms[room_number].challenger);
                exit_room(room_number,1);
                close(challenger);
                challenger = -1;
                write(defender,msg,str_len);
                rooms[room_number].winning_streak ++;
                write(defender, "please wait for challenger... \n", strlen("please wait for challenger... \n"));
                break;
            }
            sscanf(msg,"%d%d",&x, &y);
            board[y][x] = 2;
            if(check_five(board, x,y, 2))
                win = 2;
            if(win == 0){
                sprintf(msg,"& %d %d",x,y);
                write(defender, msg, strlen(msg));
                usleep(100000);
                write(defender, "3", strlen("3"));
            }
        }
        
        //printf("%d %d \n",def_choice, chal_choice);
        if(win == 0)
            continue;
        
        if(win == 1){
            rooms[room_number].winning_streak ++;
            sprintf(msg,"!You win.\nwinning streak : %d\n",rooms[room_number].winning_streak);
            write(defender,msg,strlen(msg));
            write(challenger,"!You lose.\nYou are kicked out From the room\n",strlen("!You lose.\nYou are kicked out From the room\n"));
            exit_room(room_number,1);
            
            pthread_mutex_lock(&mutx);
            rank_in(rooms[room_number].defender,rooms[room_number].winning_streak);
            pthread_mutex_unlock(&mutx);
            msg[0] = 0;
            print_rooms(msg);
            write(challenger, msg, strlen(msg));
            write(defender, "please wait for challenger... \n", strlen("please wait for challenger... \n"));
            usleep(10000);
            write(challenger, "1", strlen("1"));
            break;
        }else{
            write(defender,"!You lose.\nYou are kicked out From the room\n",strlen("!You lose.\nYou are kicked out From the room\n"));
            sprintf(msg,"!You cancel the %s's %d winning streak.\nYou are a new defender of room%d\n",rooms[room_number].challenger,rooms[room_number].winning_streak,room_number);
            write(challenger,msg,strlen(msg));
            rooms[room_number].winning_streak = 1;
            exit_room(room_number,0);
            
            pthread_mutex_lock(&mutx);
            rank_in(rooms[room_number].defender,rooms[room_number].winning_streak);
            pthread_mutex_unlock(&mutx);
            msg[0] = 0;
            print_rooms(msg);
            write(defender, msg, strlen(msg));
            write(challenger, "please wait for challenger... \n", strlen("please wait for challenger... \n"));
            usleep(10000);
            write(defender, "1", strlen("1"));
            break;
        }
    }
    pthread_mutex_lock(&mutx);
    if(defender != -1){
        FD_SET(defender, &reads);
        if(fd_max<defender)
            fd_max = defender;
    }
    if(challenger != -1){
        FD_SET(challenger, &reads);
        if(fd_max<challenger)
            fd_max = challenger;
    }
    pthread_mutex_unlock(&mutx);
    return NULL;
}
void error_handling(char * msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
