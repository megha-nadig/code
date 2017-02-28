
/*
 * Author:      Megha Nadig Basappa (mnadig@usc.edu)
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>
#include "cs402.h"
#include "my402list.h"
#include <ctype.h>
#include <unistd.h>
#define PRECISION (100)


double balance=0;
void beginList(FILE *, My402List*);
void addNode(My402List*, char*);
void displayList(My402List* list);
char* printAmt(char*);
void BubbleSortForwardList(My402List*);
char* reverse_string(char*);
char* rev_str(char* str);

typedef struct table
{
	char* type;
	time_t time;
	char* amount;
	char* description;
}transaction;

//begin list
void beginList(FILE *fp, My402List *list)
{
	char buf[1026];
	
	while(fgets(buf, sizeof(buf), fp)!=NULL)
	{
		if(strlen(buf)>1024)
		{
			fprintf(stderr, "The file length should not exceed 1024\n");
			exit(0);
		}
		addNode(list, buf);		
	}
}



//add node to a list
void addNode(My402List *list, char *input)
{
	transaction* tobj=(transaction*)malloc(sizeof(transaction));
	char* start_ptr = input;
	char* tab_ptr = strchr(start_ptr, '\t');
	char* fields[6];
	fields[0]=fields[1]=fields[2]=fields[3]=fields[4]=fields[5]=NULL;
	int count=0;
	while(tab_ptr!=NULL)
	{
		*tab_ptr++='\0';
		fields[count++]=start_ptr;
		start_ptr=tab_ptr;
		tab_ptr=strchr(start_ptr, '\t');	
	}
	if(count!=3){ fprintf(stderr, "Input file is not in the right format. Each line should have 4 fields\n"); exit(0);}
	if(count==3)
	{
		tab_ptr=strchr(start_ptr, '\n');
		tab_ptr='\0';
		fields[count]=start_ptr;
			
	}
	//type
	if(strlen(fields[0])!=1)
	{
		fprintf(stderr, "Type should be 1 char long\n"); exit(0);
	}
	
	else if(!strcmp(fields[0],"+"))
	{
		tobj->type=strdup(fields[0]);
	}
	else if(!strcmp(fields[0],"-"))
	{
		 tobj->type=strdup(fields[0]);	
	}
	else
	{
		fprintf(stderr, "Type should be + or -\n"); exit(0);
	}	
	
	//time
	if(strlen(fields[1])>10)
	{
		fprintf(stderr, "Bad timestamp\n"); exit(0);
	}
	else
	{
		unsigned long ttime=strtoul(fields[1],NULL,0);
		if(ttime>=0 && ttime<=time(NULL))
		{
			tobj->time=(time_t)ttime;
		}
		else
		{
			fprintf(stderr, "Timestamp should be between 0 and current time\n");
			exit(0);
		}
		
	}
	
	//amount
	double chkamt=atof(fields[2]);
	char* AMT=strdup(fields[2]);
	char* chkintstrlen=strtok(AMT, ".");
	char* chkdecstrlen=strtok(NULL, ".");
	if(strlen(chkintstrlen)>7){fprintf(stderr, "Amount too big\n"); exit(0);}
	if(strlen(chkdecstrlen)>2){fprintf(stderr, "Only two digits allowed after the point\n"); exit(0);}
	if(chkamt>999999999){ fprintf(stderr, "Too big an amount\n"); exit(0);}
	char* point=strchr(fields[2], '.');
	if(point==NULL){ fprintf(stderr, "Amount should have 2 decimal places\n"); exit(0);}
	long intpart=(int)(chkamt);
	float floatpart=((int)(chkamt*PRECISION)%PRECISION);
	if(intpart>999999999){ fprintf(stderr,"Amount too big\n"); exit(0);}
	if(floatpart>99){ fprintf(stderr, "Decimal places limited to 2\n"); exit(0);}
	char *amountVal=strdup(fields[2]);
	if(amountVal[0]=='.'){ fprintf(stderr, "Amount should contain at least one digit to the right of the point.\n"); exit(0);}
	tobj->amount=strdup(fields[2]);

	//description
	if(strlen(fields[3])==0){ fprintf(stderr, "Description cannot be null\n"); exit(0);}
	tobj->description=strdup(fields[3]);
	

	int status=My402ListAppend(list, tobj);
	if(!status){ fprintf(stderr, "Error in adding a node\n"); exit(0);}
		
}
//display
void displayList(My402List* list)
{
	if(My402ListEmpty(list))
	{
		fprintf(stderr, "File cannot be empty\n"); exit(0);
	}
	fprintf(stdout,"+-----------------+--------------------------+----------------+----------------+\n");
	fprintf(stdout,"|       Date      | Description              |         Amount |        Balance |\n");
	fprintf(stdout,"+-----------------+--------------------------+----------------+----------------+\n");

My402ListElem *elem=NULL;
elem=(My402ListElem*)malloc(sizeof(My402ListElem));

	for(elem=My402ListFirst(list);elem!=NULL;elem=My402ListNext(list, elem))
	{
	//date
		time_t ttime=((transaction*)elem->obj)->time;
		char day[4], month[4], date[3], year[5];

		char* temp=ctime(&ttime);
		strncpy(day, temp, 3);
		day[3]='\0';

		strncpy(month, &temp[4], 3);
		month[3]='\0';

		strncpy(date, &temp[8], 2);
		date[2]='\0';
	
		strncpy(year, &temp[20], 4);
		year[4]='\0';

		fprintf(stdout, "| %s %s %s %s |", day, month, date, year);

	//description
		char *truncateddesc = ((transaction*)elem->obj)->description;
		while(isspace(*truncateddesc)){ ++truncateddesc; }
		int length=strlen(truncateddesc);
		truncateddesc[length-1]='\0';
		fprintf(stdout, " %-24.24s |", truncateddesc);

	//amount
		
		char* amt=((transaction*)elem->obj)->amount;
		char* ttype=strdup(((transaction*)elem->obj)->type);
		double convertedAmt = atof(amt);
		if(convertedAmt>=10000000)
		{ 
			if(!strcmp(ttype, "+"))
			{
				fprintf(stdout, "  \?,\?\?\?,\?\?\?.\?\?  |");	
			}	
			if(!strcmp(ttype, "-"))
			{
				fprintf(stdout, " (\?,\?\?\?,\?\?\?.\?\?) |");
			}
			
		}
		else
		{	
			
			if(!strcmp(ttype, "+"))
			{
				fprintf(stdout, "  %12s  |", printAmt(amt));
			}
			if(!strcmp(ttype, "-"))
			{
				fprintf(stdout, " (%12s) |", printAmt(amt)); 
			}
		}

	//balance
			if(!strcmp(ttype,"+"))
			{
				balance=(balance+convertedAmt);
			
			}
			if(!strcmp(ttype, "-"))
			{
				balance=(balance-convertedAmt);
			}	
			if(balance>=10000000){ fprintf(stdout, "  \?,\?\?\?,\?\?\?.\?\?  |\n"); }
			if(balance<=(-10000000)){ fprintf(stdout, " (\?,\?\?\?,\?\?\?.\?\?) |\n");}
			if(balance<0 && balance>(-10000000))
			{ 
				balance=balance*(-1);
				char* bal=(char*)malloc(sizeof(char)*15);
				sprintf(bal, "%.2f", balance);
				fprintf(stdout, " (%12s) |\n", printAmt(bal));
				balance=balance*(-1);
			} 
			if(balance>0 && balance<10000000)
			{
				
				char* bal=(char*)malloc(sizeof(char)*15);
				sprintf(bal, "%.2f", balance);
				fprintf(stdout, "  %12s  |\n", printAmt(bal));
			}
			
			
		
	}//for
fprintf(stdout, "+-----------------+--------------------------+----------------+----------------+\n");				
	
}

//Reverse
char* rev_str(char* str)
{
	char temp;
	int i=0,j=strlen(str)-1;
while (i < j)
{
      temp = str[i];
      str[i] = str[j];
      str[j] = temp;
      i++;
      j--;
}
return str;
}

//printAmt
char* printAmt(char* amount)
{
	char* displayAmt=(char*)malloc(sizeof(char)*15);
	char* intpart=strtok(amount, ".");
	char* decpart=strtok(NULL, ".");
	char* str1=rev_str(intpart);
	char* str2=rev_str(decpart);
	int i=0,j=0,k=0;
	for(i=0;i<strlen(decpart);i++)
	{
		displayAmt[k++]=str2[i];
	}
	displayAmt[k++]='.';
	for(i=0; i<strlen(intpart);i++)
	{
		if(i==3||i==6)
		{
			displayAmt[k++]=',';
			displayAmt[k++]=str1[i];
		}
		else
		{
			displayAmt[k++]=str1[i];
		}
	}
	for(j=k;j<12;j++)
	{
		displayAmt[k++]=' ';
	}
	displayAmt[k]='\0';

return rev_str(displayAmt);
	
}

//sort
void BubbleSortForwardList(My402List *pList)
{
    My402ListElem *elem=(My402ListElem*)malloc(sizeof(My402ListElem));
    My402ListElem *next_elem=(My402ListElem*)malloc(sizeof(My402ListElem));
    My402ListElem *temp=(My402ListElem*)malloc(sizeof(My402ListElem));

        for (elem=My402ListFirst(pList); elem!=NULL; elem=My402ListNext(pList, elem))
	{
		for(next_elem=My402ListNext(pList, elem); next_elem!=NULL; next_elem=My402ListNext(pList,next_elem))
		{
			if(((transaction*)elem->obj)->time > ((transaction*)next_elem->obj)->time)
			{
				temp=elem->obj;
				elem->obj=next_elem->obj;
				next_elem->obj=temp;		
			}
			else if(((transaction*)elem->obj)->time == ((transaction*)next_elem->obj)->time)
			{
				fprintf(stderr, "Timestamp values cannot be duplicate");
				exit(0);
			}
		}
            	
    	}
	free(elem);
	free(next_elem);
	
}



//main
int main(int argc,char** argv)
{
	FILE *fp=NULL;
	My402List list;
	My402ListInit(&list);
	struct stat s;
	if(argc==2 && (!strcmp(argv[1], "sort")))
	{
		fp=stdin;
		if(fp==NULL){ fprintf(stderr, "File cannot be null\n"); exit(0);}
		beginList(fp, &list);
		BubbleSortForwardList(&list);
		displayList(&list);
	}
	if(argc==2 && strcmp(argv[1], "sort"))
	{
		fprintf(stderr, "Usage: ./warmup1 sort\n"); exit(0);
	}
	
	if(argc==3 && strcmp(argv[1], "sort"))
	{
		fprintf(stderr, "Usage: ./warmup1 sort [filename]\n"); exit(0);
	}		

	if(argc==3)
	{
		fp=fopen(argv[2], "r");
		if( stat(argv[2],&s)==0)
		{
			if( s.st_mode & S_IFDIR )
			{
			    fprintf(stderr,"Input argument is a Directory\n");
			    return(0);
			}
			
		}
		if(fp==NULL)
		{
			if(errno==EACCES)
			{
				fprintf(stderr, "Input file %s cannot be opened - access denied\n", argv[2]);
				return(0);
			}
			else
			{
				fprintf(stderr, "Input file %s does not exist\n", argv[2]);
				return(0);
			}
		}
		
						
			if(fp==NULL){ fprintf(stderr, "Input file %s does not exist\n", argv[2]); exit(0);}
			beginList(fp, &list);
			BubbleSortForwardList(&list);
			displayList(&list);	

		
	}

	if(argc<2 || argc>3)
	{
		fprintf(stdout, "Usage: ./warmup1 sort [filename] (or) ./warmup1 sort\n"); exit(0);
	}
	
return (0);
}	 		
	
	
	

