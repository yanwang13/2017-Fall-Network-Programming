#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int main(int argc, char** argv){
    
    FILE *input_file;
    char* split = argv[2];
    input_file = fopen(argv[1], "r");
    
      
	if(argc!=3){
		printf("wrong input format\n");
		exit(1);
	}
	if(input_file==NULL){
		printf("open file failure\n");
		exit(1);
	}	
    
    printf("-------------------------Input file %s-------------------------\n", argv[1]);
    
    char input_line[1024];
    //read file
    while(fgets(input_line, 1023, input_file)!=NULL){
        
        char* str = strtok(input_line, " ");
        //printf("cmd = %s\n", cmd);
        //char* str = strtok(NULL, "\n");
        //printf("str = %s\n", str);
        
        if(strcmp(str, "reverse")==0){
            char tmp[1000];
            str = strtok(NULL, "\n");
            
            strcpy(tmp, str);
            printf("reverse %s\n", tmp);
            
            int n=0;
            for(int i=strlen(str)-1;i>=0;--i){
                //*(tmp+n) = str[i];
                tmp[n] = str[i];
                //printf("%s ", str[i]);
                ++n;
            }
            printf("%s\n", tmp);
        }
        
        /*else if(strcmp(str, "split")==0){
            char *tmp;
            str = strtok(NULL, "\n");
            
            printf("split %s\n", str);
            tmp = strtok(str, split);
            while(tmp!=NULL){
                printf("%s ", tmp);
                tmp = strtok(NULL, split);
            }
            printf("\n");
        }*/
        else if(strcmp(str, "split")==0){
            str = strtok(NULL, "\n");
            printf("split %s\n", str);
            int pre = 0;
            //int flag = 0;//continuos or not
            for(int i=0;i<strlen(str);++i){
                if(str[i]==split[0]){
                    int move = i+1;
                    int j;
                    for(j=1;j<strlen(split);++j, ++move){
                        if(str[move]!=split[j])
                            break;
                    }
                    if(j==strlen(split)){
                        for(int k=pre;k<=i-1;++k)
                            printf("%c", str[k]);
                        //if(flag==0)
                        printf(" ");
                        pre = i + strlen(split);
                        //flag = 1;
                    }
                    //else
                      //  flag = 0;
                        
                }
            }
            for(int i=pre;i<strlen(str);++i)
                printf("%c", str[i]);
            printf("\n");
        }
        else{
            printf("command not included\n");
        }
    }
    
    fclose(input_file);
    printf("-------------------------End of input file %s-------------------------\n", argv[1]);
    //get line from file
    //do commad reverse or split
    //quit when EOF
    
    char user_input[1024];
    //int len;
    printf("*****************************user input*****************************\n");
    
    while(1){
        fgets(user_input, 1023, stdin);
        
        //len = strlen(user_input);
        //if(user_input[len-1]=='\n')
            //user_input[len-1] = '\0';
        
        if(strcmp(user_input, "exit\n")==0)
            break;
        
        char* str = strtok(user_input, " "); //get command
        
        if(strcmp(str, "reverse")==0){
            char tmp[1000];
            str = strtok(NULL, "\n");
            
            strcpy(tmp, str);
            //printf("reverse %s\n", tmp);
            int n=0;
            for(int i=strlen(str)-1;i>=0;--i){
                //*(tmp+n) = str[i];
                tmp[n] = str[i];
                //printf("%s ", str[i]);
                ++n;
            }
            printf("%s\n", tmp);
        }
        /*else if(strcmp(str, "split")==0){
            char *tmp;
            str = strtok(NULL, "\n");
            
            tmp = strtok(str, split);
            while(tmp!=NULL){
                printf("%s ", tmp);
                tmp = strtok(NULL, split);
            }
            printf("\n");
        }*/
        else if(strcmp(str, "split")==0){
            str = strtok(NULL, "\n");
            int pre = 0;
            //int flag = 0;
            for(int i=0;i<strlen(str);++i){
                if(str[i]==split[0]){
                    int move = i+1;
                    int j;
                    for(j=1;j<strlen(split);++j, ++move){
                        if(str[move]!=split[j])
                            break;
                    }
                    if(j==strlen(split)){
                        for(int k=pre;k<=i-1;++k)
                            printf("%c", str[k]);
                        //if(pre!=i-strlen(split))
                        //if(flag==0)
                        printf(" ");
                        pre = i + strlen(split);
                        //flag = 1;
                    }
                    //else
                        //flag = 0;
                        
                }
            }
            for(int i=pre;i<strlen(str);++i)
                printf("%c", str[i]);
            printf("\n");
        }
        else{
            printf("command not include\n");
        }
    }
    //get string from user input
    //do command revers or spit
    //end if input==exit
    return 0;
}
