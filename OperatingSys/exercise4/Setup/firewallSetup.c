#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#define READ 'L'
#define WRITE 'W'
#define FILENAME "/proc/firewallExtension"

struct stat sb;

int main (int argc, char **argv)
{
    int res;
    int filed;

    if(argc > 1 && argc < 4){

        char command = *argv[1]; //L or W

        if ((argc == 2 && command == READ) || (argc == 3 && command == WRITE)){
            
            filed = open (FILENAME, O_RDWR);
            
                if (filed == -1) {
                fprintf (stderr, "Could not open file %s \n", FILENAME);
                exit (1);
            }
            
            if (command == READ) { // L;
		res = read(filed,NULL,0);
                //res = write(filed, argv[1], sizeof(argv[1])); //sending L
            }

            if (command == WRITE) { //W
                char *filename = argv[2];
                
                FILE *file;                    //Opening file
                file = fopen(filename,"r");
                
                if(!file){
                    printf("ERROR: Cannot execute file "); //cannot open rule file
                }
                
                else {

                fseek(file,0,SEEK_END);
                int len = ftell(file);
                rewind(file);
                
                char *rules = (char *) malloc((sizeof(char)*len)+1);
                memset(rules,0,sizeof(char)*len);

                //strcat(rules,&command); //Adds W command to first byte

                char c; //current char in file
                int crntLineChar = 0; //size of current line we are looping
                int maxCharLine = 0; //max char a line takes

                while((c = fgetc(file)) != EOF || !feof(file)){
                    if (c == '\n'){
                        if (maxCharLine < ++crntLineChar){
                            maxCharLine = crntLineChar; //new max
                        }
                        crntLineChar = 0; //reset
                    }
                    crntLineChar++;
                }
                rewind(file); //back to start
        

                char *crntLine = (char *) malloc(sizeof(char)*maxCharLine);
                    
                while(fgets(crntLine, maxCharLine, file) != NULL){
                    int space = -1;
                    for(int i=0;i<maxCharLine;i++){
                        //find index of space
                        if (crntLine[i] == ' '){
                            space = i; //filepath

                            break;
                        }
                    }


		  int len = strlen(crntLine);
		  int crnL = 0;
		  if(crntLine[len-1]=='\n'){
		  	crntLine[len-1]=0;
			crnL = 1;
		   }

		   char *port = malloc(sizeof(char)*space);
		   strncpy(port,crntLine,space);

                    if(space > 0){
                        for(int j = 0; j < strlen(port) ;j++){
                            if(!isdigit(port[j])){
                                res = 0; //dont write to kernel
                                printf("ERROR: Ill-formed file\n");
                                free(crntLine);
				free(port);
                                break;
                            }
                        }
                    }
                    if(res == 0){
                        break;
                   }
	         

                    //printf("%s \n",&crntLine[space+1]);
                    if (stat(&crntLine[space+1],&sb) == 0 && sb.st_mode & S_IXUSR){
                
                    }
                    else{
                        //not executable
                        res = 0;
                        printf("ERROR: Ill-formed file 2\n");
                        free(crntLine);
			free(port);
                        break;
                    }

		      if(crnL){
		      	crntLine[len-1]='\n';
			}
		
                    strcat(rules, crntLine); //putting sentence in rules buffer
                }

                if(res){
                    //not illformed file
                    res = write(filed, rules, sizeof(command) + (sizeof(char)*len)+1);
                    free(rules);
                }

                fclose(file); //closing file

                }
            }
            
            close(filed); //close file
            return res; //return status
        }
    }

    printf ("%s <L> or <W filename>\n",argv[0]);
    return 0;


}
