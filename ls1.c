/*purpose list contents of directory or directories*/
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

//void do_ls_l(char dirname[]);
void show_file_info(char *origname,struct stat *info_p,int i);
void mode_to_letters(int mode,char str[]);
void do_ls(char dirname[]);
int comp(const void *p,const void *q);
void displayInfo();
void dostat(char *filename,char *origname,int i);
void sortLs_l(char dirname[]);

#define MAXLEN 1000
#define MAXINFOR 1000
//全局变量用来存放命令行选项
struct globalArgs_t {
    int lsAll;                	/* -A option */
    int lsall;					/* -a option */
    int lsreverse;				/* -r option */
	int lslong;					/* -l option */
	int lsHelp;					/* -? option */
	char **FileOrDirName;		/*要求显示的文件夹或文件列表*/
    int numFileOrDir;			/*要求显示的文件夹或文件列表个数*/              
} globalArgs;

 /*告知getopt()可以处理那个选项以及哪个选项需要参数*/
static const char *optString = "AaRrlH";

//存放将要输出的信息的字符串数组
char *result[MAXINFOR];
char *Aresult;
int indexnumber=0;



int main(int ac,char *av[]){
	int opt=0;
	/*初始化全局变量*/
	globalArgs.lsAll=0;
	globalArgs.lsall=0;
	globalArgs.lsreverse=0;
	globalArgs.lslong=0;
	globalArgs.lsHelp=0;
	globalArgs.FileOrDirName=NULL;
	globalArgs.numFileOrDir=0;

	opt = getopt(ac,av,optString);
    while( opt != -1 ) {
        switch( opt ) {
            case 'A':
                globalArgs.lsAll=1; /* true */
                break;
                 
            case 'a':
                globalArgs.lsall=1;
                break;
         
            case 'r':
                globalArgs.lsreverse=1;
                break;
                 
            case 'l':
                globalArgs.lslong=1;
                break;

            case 'H':
                globalArgs.lsHelp=1;
                break;
                 
            default:

                break;
        }
        opt = getopt( ac, av, optString );
    }
    globalArgs.FileOrDirName = av + optind;
    globalArgs.numFileOrDir = ac - optind;
    //printf("%s\n",globalArgs.FileOrDirName[1]);
    //printf("%d\n",globalArgs.numFileOrDir);
    //printf("%d\n",globalArgs.lsHelp);
    if(globalArgs.lsHelp==1){
    	printf("*********************************************************************************\n");
    	printf("* -A : List all entries except for (.) and (..).\t\t\t\t*\n");
    	printf("* -a : Include directory entries whose names begin with a dot (.).\t\t*\n");
    	printf("* -l : List in long format.The first line is the total number of files.\t\t*\n");
    	printf("* -r : Reverse the order of the sort to get reverse lexicographical order.\t*\n");
    	printf("* -H : List the usage.\t\t\t\t\t\t\t\t*\n");
    	printf("*********************************************************************************\n");
    }else if(globalArgs.numFileOrDir==0){
			do_ls(".");
	}else{
		for(int i=0;i<globalArgs.numFileOrDir;i++){
			indexnumber=0;
			do_ls(globalArgs.FileOrDirName[i]);
			for(int i=0;i<indexnumber;i++){
				free(result[i]);
			}
			indexnumber=0;
			/*
			if(globalArgs.numFileOrDir>1 && i!=globalArgs.numFileOrDir-1){
				printf("\n");
			}
			*/
		}
	}
	return 0;
}


//判断选项，将文件信息放入result
void do_ls(char dirname[]){
	DIR *dir_ptr;
	struct dirent* direntp;
	if((dir_ptr=opendir(dirname))==NULL){
		//不是目录
		struct stat info;
		if(stat(dirname,&info)==-1){
			//也不是文件
			perror(dirname);
		}else{
			//是文件
			Aresult=(char *)malloc(MAXLEN*sizeof(char));
			result[0]=Aresult;
			if(globalArgs.lslong==1){
				show_file_info(dirname,&info,0);
				indexnumber++;
				printf("%s\n",result[0]);
			}else{
				strcpy(result[0],dirname);
				indexnumber++;
				printf("%s\n",result[0]);
			}
			/*
			Aresult=(char *)malloc(MAXLEN*sizeof(char));
			strcpy(Aresult,dirname);
			result[indexnumber]=Aresult;
			indexnumber++;
			*/
			return;
		}
		fprintf(stderr,"lsg : cannot open %s\n",dirname);
	}else{
		//是目录
		while((direntp=readdir(dir_ptr))!=NULL){
			//如果没有-a或-A
			Aresult=(char *)malloc(MAXLEN*sizeof(char));
			if(globalArgs.lsAll==0 && globalArgs.lsall==0){
				//"."及".."及隐藏文件都过滤
				if((direntp->d_name)[0]=='.'){
					free(Aresult);
					continue;
				}
			}
			if(globalArgs.lsall==1){
				//如果有-a （也许还有-A）
				//不过滤，全部显示
			}else if(globalArgs.lsAll==1){
				//没有-a，但是有-A
				if(strcmp(direntp->d_name,".")==0 || strcmp(direntp->d_name,"..")==0){
					free(Aresult);
					continue;
				}
			}
			//printf("11111\n");
			if(globalArgs.lslong==0){
				//如果没有-l
				strcpy(Aresult,direntp->d_name);
				result[indexnumber]=Aresult;
				
			}else if(globalArgs.lslong==1){
				strcpy(Aresult,direntp->d_name);
				result[indexnumber]=Aresult;
			}
			indexnumber++;
			//printf("%s\n",result[indexnumber-1]);
		}
		closedir(dir_ptr);
		//对输出排序
		qsort(result,indexnumber,sizeof(char *),comp);
		displayInfo(dirname);
	}
}

int comp(const void *p,const void *q){
	return (strcmp(*(char**)p,*(char**)q));
}

void sortLs_l(char dirname[]){

	char name[MAXINFOR];
	int i;

	for(i=0;i<indexnumber;i++){
		sprintf(name,"%s/%s",dirname,result[i]);
		//strcpy(name,result[i]);
		//printf("%s %s\n",name,result[i]);
		dostat(name,result[i],i);
	}
}

void dostat(char *filename,char *origname,int i){
	struct stat info;
	if(stat(filename,&info)==-1){
		perror(filename);
	}else{
		show_file_info(origname,&info,i);
	}
}

void show_file_info(char *origname,struct stat *info_p,int i){
	char *uid_to_name(),*ctime(),*gid_to_name(),*filemode();
	void mode_to_letters();
	char modestr[11];
	char infor[MAXLEN];
	mode_to_letters(info_p->st_mode,modestr);
	sprintf(infor,"%s%4d %-8s %-8s %8ld %.20s %s",modestr,(int)info_p->st_nlink,uid_to_name(info_p->st_uid),gid_to_name(info_p->st_gid),(long)info_p->st_size,4+ctime(&info_p->st_mtime),origname);
	//printf("%d\n",i);
	strcpy(result[i],infor);
	//lsprintf("%d\n",i);
	//printf("\n****%s\n",result[i]);
}

void displayInfo(char dirname[]){
	printf("------%s :\t------\n",dirname);
	printf("------Total : %d\t------\n",indexnumber);
	if(globalArgs.lsreverse==0){
		//没有-r
		if(globalArgs.lslong==0){

		}else{
			sortLs_l(dirname);
		}
		for(int k=0;k<indexnumber;k++){
			//printf("%d\n",indexnumber);
			printf("%s\n",result[k]);
		}
	}else if(globalArgs.lsreverse==1){
		//有-r 逆序输出
		if(globalArgs.lslong==0){

		}else{
			sortLs_l(dirname);
		}
		for(int k=indexnumber-1;k>=0;k--){
			printf("%s\n",result[k]);
		}	
	}
}

//utility functions
void mode_to_letters(int mode,char str[]){
	strcpy(str,"----------");
	if(S_ISDIR(mode)){
		str[0]='d';
	}
	if(S_ISCHR(mode)){
		str[0]='c';
	}
	if(S_ISBLK(mode)){
		str[0]='b';
	}
	if(mode & S_IRUSR){
		str[1]='r';
	}
	if(mode & S_IWUSR){
		str[2]='w';
	}
	if(mode & S_IXUSR){
		str[3]='x';
	}
	if(mode & S_IRGRP){
		str[4]='r';
	}
	if(mode & S_IWGRP){
		str[5]='w';
	}
	if(mode & S_IXGRP){
		str[6]='x';
	}
	if(mode & S_IROTH){
		str[7]='r';
	}
	if(mode & S_IWOTH){
		str[8]='w';
	}
	if(mode & S_IXOTH){
		str[9]='x';
	}
	if(mode & S_ISUID){
		str[3]='s';
	}
	if(mode & S_ISGID){
		str[6]='s';
	}
	if(mode & S_ISVTX){
		str[9]='t';
	}
}

#include <pwd.h>

char *uid_to_name(uid_t uid){
	struct passwd *getpwuid(),*pw_ptr;
	static char numstr[10];

	if((pw_ptr=getpwuid(uid))==NULL){
		sprintf(numstr,"%d",uid);
		return numstr;
	}else{
		return pw_ptr->pw_name;
	}
}

#include <grp.h>
char *gid_to_name(gid_t gid){
	struct group *getgrgid(),*grp_ptr;
	static char numstr[10];
	if((grp_ptr=getgrgid(gid))==NULL){
		sprintf(numstr,"%d",gid);
		return numstr;
	}else{
		return grp_ptr->gr_name;
	}
}
