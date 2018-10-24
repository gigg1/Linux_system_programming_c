/* 
本仿linux下more程序实现的功能：

	a)	输入命令不需要按多余的回车。
	b)	more反白字符不随文件的内容更新上翻。
	c)	动态获取终端当前行列数，显示的内容的多少随终端的行列数而改变。
	d)	显示文件当前百分比和当前显示第几个文件。
	e)	当标准输出重定向到另一个程序的输出时，从键盘鼠标设备文件实现用户交互。
	f)	输入不存在的文件名显示提示信息。

	该more(程序命名为“moreg”)程序可以分页显示所选文件的内容
	也可以用作其他命令的分页结果显示。
	当前只有两种使用方式：	“moreg filename1 filename2 …”
						“other command | moreg”

 */
#include <stdio.h>  //对光标的操作也使用该头文件
#include <stdlib.h>
#include <termios.h>//获取和设置终端属性

//#include <sys/ioctl.h>//获取终端大小

//获取行列函数头文件夹
#include <unistd.h>  
#include <string.h>        
#include <sys/ioctl.h>  
#include "errno.h"  

#define PAGELEN 63      //测试当前终端的最大行数为63
#define LINELEN 514     //获取文件一行的字符数
int file_sum_size;
void do_more(FILE *fp,int file_sum_size,int file_number,int all_file_number);
int see_more(FILE *cmd,int current_percentage,int file_number,int all_file_number);
int get_screen_row();//获取当前终端的行
int get_screen_col();//获取当前终端的列
int get_real_char_number(char arr[]);//获取字符数组中真正存字符的个数
void do_more_stdin(FILE *fp);
int see_more_stdin(FILE *cmd);


int main(int ac,char* av[]){
        FILE *fp;
        int file_sum_size=0;
        int file_number=0;//当前正在显示的文件编号
        int all_file_number=ac-1;
        //printf("%d\n",ac-1 );
        if(ac==1){
                do_more(stdin,file_sum_size,file_number,all_file_number);
        }else{
                while(--ac){
                        file_number++;
                        if((fp = fopen(* ++av,"r"))!=NULL){
                                printf("\n");
                                printf("--------------------***** %d ****--------------------\n",file_number);
                                printf("\n");
                                //计算文件的字节数（大小）
                                fseek(fp,0L,SEEK_END);  //fseek函数将指针定位在所读文件结尾的位置
                                file_sum_size=ftell(fp);        //ftell函数返回指针相对于文件开头的位置，单位为字节
                                //printf("--------------The Size of This File : %d---------------\n",file_sum_size);
                                fseek(fp,0L,SEEK_SET);
                                do_more(fp,file_sum_size,file_number,all_file_number);
                        }else{
                                printf("\t--Please input right param parameter！\n");
                                printf("\t--For Example : moreg filename\n");
                                printf("\t--For Example : moreg filename1 filemname2 ...\n");
                                exit(1);
                        }
                        struct termios  original_setting,new_setting;
                        FILE *fp_tty;
                        int current_row;
                        current_row=get_screen_row();
                        fp_tty=fopen("/dev/tty","r");
                        //获取当前终端属性,成功返回0
                        if(tcgetattr(fileno(fp_tty),&original_setting)!=0){
                            fprintf(stderr,"fail to get current Terminal attributes!");
                        }
                        new_setting=original_setting;
                        new_setting.c_lflag &=~ICANON;//关闭标准模式标志，使终端处于非规范方式
                        new_setting.c_lflag &=~ECHO;//关闭终端回显模式标志
                        new_setting.c_cc[VMIN]=1;//接到1个字节，read就返回
                        new_setting.c_cc[VTIME]=0;//计时器不设定
                        if(tcsetattr(fileno(fp_tty),TCSANOW,&new_setting)!=0){//重新设置终端属性
                            fprintf(stderr,"fail to set new Terminal attributes!");
                        }
                        printf("\033[%d;%dH",current_row,0);
                        int current_percentage=100;
                        see_more(fp_tty,current_percentage,file_number,all_file_number); 
                        //将光标移到当前more行行首
                        printf("\033[%d;%dH",current_row,0);
                        //将光标之后到本行末尾的内容清除
                        printf("\033[K");

                        fclose(fp);
                        if(tcsetattr(fileno(fp_tty),TCSANOW,&original_setting)!=0){//重新设置终端属性
                                fprintf(stderr,"fail to reset original Terminal attributes!");
                        }
                        
                }
                
        }

        return 0;
}
void do_more(FILE *fp,int file_sum_size,int file_number,int all_file_number){
/*
 *read PAGELEN lines,then call see_more() for futher instructions
 */
        char line[LINELEN];
        int num_of_lines=0;
        int reply;
        int current_display_size=0;
        int current_percentage=0;
        int current_row;
        int current_col;
        struct termios  original_setting,new_setting;

        FILE *fp_tty;
        fp_tty=fopen("/dev/tty","r");
        //获取当前终端属性,成功返回0
        if(tcgetattr(fileno(fp_tty),&original_setting)!=0){
                fprintf(stderr,"fail to get current Terminal attributes!");
        }
        new_setting=original_setting;
        new_setting.c_lflag &=~ICANON;//关闭标准模式标志，使终端处于非规范方式
        new_setting.c_lflag &=~ECHO;//关闭终端回显模式标志
        new_setting.c_cc[VMIN]=1;//接到1个字节，read就返回
        new_setting.c_cc[VTIME]=0;//计时器不设定

        if(tcsetattr(fileno(fp_tty),TCSANOW,&new_setting)!=0){//重新设置终端属性
                fprintf(stderr,"fail to set new Terminal attributes!");
        }

        if(fp_tty==NULL){
                exit(1);
        }
        //动态获取当前终端行列
        current_row=get_screen_row();
        //current_col=get_screen_col();

        while(fgets(line,LINELEN,fp)){
        //while(fgets(line,current_col-1,fp)){
                //if(num_of_lines==PAGELEN){
                
                int realsize=get_real_char_number(line);
                current_col=get_screen_col();
                


                if(num_of_lines>=current_row-1){
                        current_row=get_screen_row();
                        //current_col=get_screen_col();
                        if(file_number==0 && all_file_number==0){
                            current_percentage=0;
                        }else{
                                current_display_size=ftell(fp);//ftell获取当前指针与文件开头的字节数
                                current_percentage=current_display_size*100/file_sum_size;
                        }
                        reply=see_more(fp_tty,current_percentage,file_number,all_file_number);
                        
                        //将光标移到当前more行行首
                        printf("\033[%d;%dH",current_row,0);
                        //将光标之后到本行末尾的内容清除
                        printf("\033[K");


                        //printf("\r\n");//消除原始程序2会有按下空格后more后面又一行文件内容的情况

                        if(reply==-1){
                                reply=current_row-1;
                        }

                        if(reply==0){
                                break;
                        }
                        num_of_lines-=reply;
                }
                //printf("realsize=%d,terminal_Cul=%d,display_row=%d,current_row=%d\n",realsize,current_col,num_of_lines+1,current_row);
                if(realsize>current_col){
                        while(1){
                                realsize=realsize-current_col;
                                num_of_lines++;
                                if(realsize<0){
                                        break;
                                } 
                        }
                }else{
                        num_of_lines++;
                }
                
                if(fputs(line,stdout)==EOF){
                        exit(1);
                }
        }
        if(tcsetattr(fileno(fp_tty),TCSANOW,&original_setting)!=0){//重新设置终端属性
                fprintf(stderr,"fail to reset original Terminal attributes!");
        }
}

int see_more(FILE *cmd,int current_percentage,int file_number,int all_file_number){
/* print message,wait for response,turn # of lines to advance
 * q means no,spaca means yes,CR means one line
 */
        int c='a';
        int i=0; 
        if(file_number==0&&all_file_number==0&&current_percentage==0){
            printf("\033[7m more? \033[m");
            //printf("\033[7m more? ");
        }else{
            printf("\033[7m more?  %d%%  (file %d of %d)\033[m",current_percentage,file_number,all_file_number);
        }
        
        c=fgetc(cmd);
        while(1){
                if(c=='q'){
                        return 0;
                }
                if(c==' '){             //使用当前终端的行数
                        return -1;
                }
                if(c=='\n'){
                        return 1;
                }
                c=fgetc(cmd);
        }
        return 0;
}


int get_screen_row(){
        //get terminal's hight
        int pRow;
        struct winsize size;  
 
        if(isatty(STDOUT_FILENO)==0)  
        {  
                printf("not a tty\n");  
                return -1;    
        }  
 
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &size)<0)  
        {  
                printf("get win size failed: %s\n", strerror(errno));  
                return -1;    
        }  
        pRow = size.ws_row; 
        return pRow;  
}


int get_screen_col(){
        //get terminal's width  
        int pColumn;
        struct winsize size;  
 
        if(isatty(STDOUT_FILENO)==0)  
        {  
                printf("not a tty\n");  
                return -1;    
        }  
 
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &size)<0)  
        {  
                printf("get win size failed: %s\n", strerror(errno));  
                return -1;    
        }  
        pColumn = size.ws_col; 
        return pColumn;  
}

int get_real_char_number(char arr[]){
        int i;
        for(int i=0; ;i++){
                if (arr[i]=='\n' | arr[i]=='\r'|arr[i]=='$'){
                        return i;
                }
        }
}
