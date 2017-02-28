
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
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <math.h>
#include "cs402.h"
#include "my402list.h"



//global parameters
typedef struct{
	long interarrival_time;
	int n_tokens;
	long service_time;
}initialpacket;

typedef struct{
	struct timeval q1arrive;
	struct timeval q1depart;
	struct timeval q2arrive;
	struct timeval q2depart;
	int tokens;
	int id;
	double transmissiontime;

}packet;

float lambda=1, mu=0.35, r=1.5;
long B=10, P=3, n=20;
char *tfile=NULL;
int trace_driven_mode=0;
int num_packets=0;
FILE *fp;
int tokens_arrived=0;
int tokens_in_bucket=0;
int q2emptyflag=0;
int linecount=0;
int totalpacketcount=0;
int server1packets = 0;
int server2packets = 0;
int ctrlcpressed = 0;
struct timeval start_time, end_time, emulationstart, emulationend;
double totalemulationtime;
My402List *q1list=NULL;
My402List *q2list=NULL;
pthread_t pArr, tDep, server1, server2, sigThread;
int packetsdropped=0;
int tokensdropped=0;
double totaltimeq1 = 0.0;
double totaltimeq2 = 0.0;
double totaltimes1 = 0.0;
double totaltimes2 = 0.0;
double = 0.0;
int totalnumberofpackets = 0;
double totaltimeinsystem=0.0;
double totaltimeinsystemsq = 0.0;
int packetThreadDead = 0;
struct timeval q1entertime, q1departtime;
struct timeval q2entertime, q2departtime;
int q1packets = 0;
int q2packets = 0;
int packetThreadJoined = 0;
float rlambda, rmu, rr;

//signal
sigset_t set;

//function declarations
initialpacket* read_line_from_file();
int get_n_packets();
void readInput(int argc, char* argv[]);
void displayInputArguments();
void createThreads();
void* packetArrival(void *arg);
void* tokenDeposit(void *arg);
void* serverfunc1(void *arg);
void* serverfunc2(void *arg);
void* getSignal(void *arg);
double convert_to_microseconds(struct timeval);
double calculateTime();
void initializequeues();
void addtoqueue1(packet*);
int check_token_bucket();
void addto_q2list();
void cleanuplists(void* arg);
void printStatistics();

//mutex
pthread_mutex_t q1m = PTHREAD_MUTEX_INITIALIZER;

//condition variable
pthread_cond_t q2empty = PTHREAD_COND_INITIALIZER;

//convert_to_microseconds
double convert_to_microseconds(struct timeval t)
{
	return((t.tv_sec*1000000)+t.tv_usec);
}

//calculate Time
double calculateTime()
{
	struct timeval ctime;
	gettimeofday(&ctime, NULL);
	return (convert_to_microseconds(ctime) - convert_to_microseconds(emulationstart))/1000;
}
//initialize queues
void initializequeues()
{
	q1list = (My402List*)malloc(sizeof(My402List));
	My402ListInit(q1list);
	q2list = (My402List*)malloc(sizeof(My402List));
	My402ListInit(q2list);
}
//add to q1
void addtoqueue1(packet* p)
{
	My402ListAppend(q1list, p);
}

//cleanuplists
void cleanuplists(void* arg)
{
	double removeTime;
	pthread_mutex_lock(&q1m);
	while(!My402ListEmpty(q1list))
	{
		My402ListElem* elem = My402ListFirst(q1list);
		removeTime = calculateTime();
		printf("%012.3fms: p%d was removed from Q1\n",removeTime, ((packet*)elem->obj)->id);
		My402ListUnlink(q1list, elem);
	}
	while(!My402ListEmpty(q2list))
	{
		My402ListElem* elem = My402ListFirst(q2list);
		removeTime = calculateTime();
		printf("%012.3fms: p%d was removed from Q2\n",removeTime, ((packet*)elem->obj)->id);
		My402ListUnlink(q2list, elem);
	}
	pthread_cond_broadcast(&q2empty);
	pthread_mutex_unlock(&q1m);
}

//check_token_bucket
int check_token_bucket()
{
	int t;
	My402ListElem* elem;
	if(q1list!=NULL)
	{
		if(!My402ListEmpty(q1list))
		{
			elem = My402ListFirst(q1list);
			t = ((packet*)elem->obj)->tokens;
			if(t<=tokens_in_bucket)
			{
				return 1;
			}
		}
	}
return 0;
}

//addto_q2list
void addto_q2list()
{
	if(q2list!=NULL)
	{
		My402ListElem* elem=My402ListFirst(q1list);
		if(!My402ListAppend(q2list, elem->obj))
		{
			fprintf(stderr, "Error in appending to Q2\n"); exit(0);
		}
		My402ListUnlink(q1list, elem);
	}
	else
	{
		fprintf(stderr, "Adding to q2list error\n"); exit(0);
	}

}

//readInput
void readInput(int argc, char* argv[])
{
	int i=1;
	for(i=1; i<=argc-1; i+=2)
	{
		//lambda
		if(!strcmp(argv[i], "-lambda"))
		{
			if(i == argc-1)
			{
				fprintf(stderr, "Bad Command line.\n Usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n"); exit(0);
			}
			lambda=atof(argv[i+1]);
			rlambda = atof(argv[i+1]);
			if(lambda == 0)
			{
				fprintf(stderr,"Bad Command line\nUsage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n"); exit(0);
			}
			if(lambda<0)
			{
				fprintf(stderr, "Value of lambda should be positive\n"); exit(0);
			}
			if((1/lambda)>10)
			{
			 	lambda=0.1;
			}

		}


		//mu
		else if(!strcmp(argv[i], "-mu"))
		{
			if(i == argc-1)
			{
				fprintf(stderr, "Bad Command line\nUsage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n"); exit(0);
			}
			mu=atof(argv[i+1]);
			rmu = atof(argv[i+1]);
			if(mu == 0)
			{
				fprintf(stderr,"Bad Command line\nUsage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n"); exit(0);
			}
			if(mu<0)
			{
				fprintf(stderr, "Value of mu should be positive\n"); exit(0);
			}
			if((1/mu)>10)
			{
				mu=0.1;
			}


		}

		//r
		else if(!strcmp(argv[i], "-r"))
		{
			if(i == argc-1)
			{
				fprintf(stderr, "Bad Command line\nUsage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n"); exit(0);
			}
			r=atof(argv[i+1]);
			rr =atof(argv[i+1]);
			if(r == 0)
			{
				fprintf(stderr,"Bad Command line\nUsage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n"); exit(0);
			}
			if(r<0)
			{
				fprintf(stderr, "Value of r should be positive\n"); exit(0);
			}
			if((1/r)>10)
			{
				r=0.1;
			}
		}

		//B
		else if(!strcmp(argv[i], "-B"))
		{
			if(i == argc-1)
			{
				fprintf(stderr, "Bad Command line\nUsage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n"); exit(0);
			}
			B=atoi(argv[i+1]);
			if(B == 0)
			{
				fprintf(stderr, "Bad Command line\nUsage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n"); exit(0);
			}
			if(B<0||B>2147483647)
			{
				fprintf(stderr, "Value of B should be greater than 0 and less than 2147483647\n"); exit(0);
			}
		}


		//P
		else if(!strcmp(argv[i], "-P"))
		{
			if(i == argc-1)
			{
				fprintf(stderr, "Bad Command line\nUsage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n"); exit(0);
			}
			P=atoi(argv[i+1]);
			if(P == 0)
			{
				fprintf(stderr, "Bad Command line\nUsage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n"); exit(0);
			}
			if(P<0||P>2147483647)
			{
				fprintf(stderr, "Value of P should be greater than 0 and less than 2147483647\n"); exit(0);
			}

		}

		//n
		else if(!strcmp(argv[i], "-n"))
		{
			if(i == argc-1)
			{
				fprintf(stderr, "Bad Command line\nUsage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n"); exit(0);
			}
			n=atoi(argv[i+1]);
			if(n == 0)
			{
				fprintf(stderr,"Bad Command line\nUsage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n"); exit(0);
			}
			if(n<0||n>2147483647)
			{
				fprintf(stderr, "Value of n should be greater than 0 and less than 2147483647\n"); exit(0);
			}
		}


		//t
		else if(!strcmp(argv[i], "-t"))
		{
			if(i == argc-1)
			{
				fprintf(stderr, "Bad Command line\nUsage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n"); exit(0);
			}
			trace_driven_mode=1;
			tfile=strdup(argv[i+1]);
			if(tfile == NULL)
			{
				fprintf(stderr, "Filename cannot be null\n"); exit(0);
			}
		}
		else
		{
			fprintf(stderr, "Bad Command line\nUsage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n"); exit(0);
		}

	}

return;
}

//get number of packets from the file
int get_num_packets()
{
	char buf[1026];
	int num_p;
	int i=0;
	if(fgets(buf, sizeof(buf), fp)!=NULL)
	{
		buf[strlen(buf)-1]='\0';
		for(i=0;i<strlen(buf);i++)
		{
			if(buf[i]==' '||buf[i]=='\t')
			{
				fprintf(stderr, "Invalid file format\n"); exit(0);
			}

		}
		num_p=atoi(buf);
		if(num_p == 0)
		{
			fprintf(stderr, "Invalid input file\n"); exit(0);
		}
	}
return num_p;
}

//read line from file
initialpacket* read_line_from_file()
{
	int i=0;
	char buf[1026];
	initialpacket* tempPacket=(initialpacket*)malloc(sizeof(initialpacket));
	if(fgets(buf, sizeof(buf), fp)!=NULL)
	{
		char* token[3];
		token[0]=token[1]=token[2]=NULL;
		if(buf[0]==' '||buf[0]=='\t'||buf[strlen(buf)-1]==' '||buf[strlen(buf)-1]=='\t')
		{
			fprintf(stderr, "No leading/trailing characters allowed\n"); exit(0);
		}
		if(strcmp(buf,"\n")!=0)
		{
			char* tok=strtok(buf, " ");
			while(tok!=NULL)
			{
				token[i++]=tok;
				tok=strtok(NULL, " ");
			}

		}
		if(i!=3)
		{
			fprintf(stderr, "Number of fields in a line should be 3"); exit(0);
		}
		if(i==3)
		{
			tempPacket->interarrival_time=atol(token[0]);
			tempPacket->n_tokens=atoi(token[1]);
			tempPacket->service_time=atol(token[2]);
		}
	}
return(tempPacket);
}

//display the parameters
void displayInputArguments()
{
	//printf("Emulation parameters\n");
	if(trace_driven_mode)
		num_packets=get_num_packets();
	printf("Emulation parameters\n");
	if(trace_driven_mode)
		printf("\tnumber to arrive = %d\n",num_packets);
	else
		printf("\tnumber to arrive = %ld\n", n);
	if(!trace_driven_mode)
		printf("\tlambda = %.6g\n", rlambda);
	if(!trace_driven_mode)
		printf("\tmu = %.6g\n", rmu);
	printf("\tr = %.6g\n", rr);
	printf("\tB = %d\n", (int)B);
	if(!trace_driven_mode)
		printf("\tP = %d\n", (int)P);
	if(trace_driven_mode)
		printf("\ttsfile = %s\n", tfile);
}


//getSignal Function
void* getSignal(void *arg)
{
	int sig;
	sigwait(&set, &sig);
	ctrlcpressed = 1;
	//printf("Signal Caught\n");
	pthread_cancel(pArr);
	pthread_cancel(tDep);

return(0);
}

//packetArrival function
void* packetArrival(void *arg)
{
	pthread_cleanup_push(cleanuplists, NULL);
	pthread_testcancel();
	int tokensreq=0, packet_count=0;
	int lastpacketdropped = 0;
	double interarrivaltime, transmissiontime, timetosleep=0.0, timenow=0.0, time_bw_packets=0.0, timeinq1 = 0.0;
	initialpacket* temp=(initialpacket*)malloc(sizeof(initialpacket));
	struct timeval previousdepart, previousarrive, aftersleeptime;

	if(trace_driven_mode)
	{
		n=num_packets;
	}
	linecount=n;
	while(linecount!=0)
	{
		if(trace_driven_mode)
		{
			if(linecount>0)
			{
				temp=read_line_from_file();
				interarrivaltime = (double)(temp->interarrival_time);
				transmissiontime = (temp->service_time);
				tokensreq = temp->n_tokens;
				linecount-=1;
			}
		}
		else
		{
			interarrivaltime=1000.0/(float)lambda;
			transmissiontime=1000.0/(float)mu;
			tokensreq=P;
			linecount-=1;
		}
		gettimeofday(&previousdepart, NULL);

		timetosleep = interarrivaltime*1000;
		usleep(timetosleep);


		pthread_mutex_lock(&q1m);
		if(ctrlcpressed)
			{
				pthread_mutex_unlock(&q1m);
				pthread_testcancel();
			}
		gettimeofday(&aftersleeptime, NULL);



		if(packet_count==0)
		{
			time_bw_packets = (convert_to_microseconds(aftersleeptime) - convert_to_microseconds(emulationstart))/1000;
		}
		else
		{
			time_bw_packets = (convert_to_microseconds(aftersleeptime) - convert_to_microseconds(previousarrive))/1000;
		}


		packet_count++;
		packet *p = (packet*)malloc(sizeof(packet));
		p->tokens = tokensreq;
		p->id = packet_count;
		p->transmissiontime = transmissiontime;
		totalinterarrivaltime+=interarrivaltime;
		totalnumberofpackets++;
		if(totalnumberofpackets == n)
		{
			packetThreadDead = 1;
		}

		if(p->tokens>B)
		{
			timenow = calculateTime();
			printf("%012.3fms: Packet p%d arrives, needs %d tokens, dropped\n",timenow, p->id,p->tokens);
			packetsdropped++;
			if(totalnumberofpackets == n)
			{
				lastpacketdropped = 1;
			}

		}
		else
		{	timenow = calculateTime();
			printf("%012.3fms: Packet p%d arrives, needs %d tokens, inter-arrival time = %.3fms\n",timenow, p->id, p->tokens, 				time_bw_packets);
			gettimeofday(&previousarrive, NULL);

			addtoqueue1(p);
			timenow = calculateTime();
			gettimeofday(&q1entertime, NULL);
			p->q1arrive = q1entertime;
			printf("%012.3fms: Packet p%d enters Q1\n", timenow, p->id);
			if(check_token_bucket() && My402ListLength(q1list)==1)
			{
				tokens_in_bucket-=p->tokens;
				timenow = calculateTime();
				gettimeofday(&q1departtime, NULL);
				p->q1depart = q1departtime;
				timeinq1 = (convert_to_microseconds(q1departtime) - convert_to_microseconds(q1entertime))/1000;
				totaltimeq1+=timeinq1;
				printf("%012.3fms: p%d leaves Q1, time in Q1 = %.6gms, token bucket now has %d tokens\n", timenow, p->id, timeinq1, tokens_in_bucket);

			//check if q2 is empty and append
				addto_q2list();
				q1packets++;
				gettimeofday(&q2entertime, NULL);
				p->q2arrive = q2entertime;
				timenow = calculateTime();
				printf("%012.3fms: p%d enters Q2\n", timenow, p->id);
				pthread_cond_signal(&q2empty);

			}

		}
		if(lastpacketdropped)
		{
			pthread_cond_signal(&q2empty);
		}
		if(totalnumberofpackets == n)
		{
			packetThreadJoined = 1;
			pthread_mutex_unlock(&q1m);
			pthread_join(sigThread, NULL);
		}
	pthread_mutex_unlock(&q1m);

	}

pthread_cleanup_pop(0);
return(0);
}

//tokenDeposit function
void* tokenDeposit(void *arg)
{
	pthread_testcancel();
	double sleeptime, timenow, timeinq1;
	double tarrivaltime = 1000.0/(float)r;
	struct timeval tfinish, tstart, q1arr, q1dep, q2arr;
	int t=0;
	int pk=0;
	while(1)
	{
	gettimeofday(&tfinish, 0);
	sleeptime = tarrivaltime*1000;
	usleep(sleeptime);


	pthread_mutex_lock(&q1m);
	if((totalnumberofpackets)>=n && My402ListEmpty(q1list)==1)
	{
		pthread_mutex_unlock(&q1m);
		break;
	}
	pthread_mutex_unlock(&q1m);

	pthread_mutex_lock(&q1m);
	if(ctrlcpressed)
	{
		pthread_mutex_unlock(&q1m);
		pthread_testcancel();
	}

	gettimeofday(&tstart, 0);

	tokens_arrived++;

	timenow = calculateTime();
	if(tokens_in_bucket+1<=B)
	{
		tokens_in_bucket++;
		printf("%012.3fms: token t%d arrives, token bucket now has %d tokens\n", timenow, tokens_arrived, tokens_in_bucket);
	}
	else
	{
		printf("%012.3fms: token t%d arrives, dropped\n", timenow, tokens_arrived);
		tokensdropped++;

	}

	if(check_token_bucket())
	{
		My402ListElem *elem = My402ListFirst(q1list);
		t = ((packet*)elem->obj)->tokens;
		timenow = calculateTime();
		tokens_in_bucket-=t;
		gettimeofday(&q1dep, NULL);
		((packet*)elem->obj)->q1depart = q1dep;
		q1arr = ((packet*)elem->obj)->q1arrive;
		timeinq1 = (convert_to_microseconds(q1dep) - convert_to_microseconds(q1arr))/1000;
		totaltimeq1+=timeinq1;
		printf("%012.3fms: p%d leaves Q1, time in Q1 = %.6gms , token bucket now has %d tokens\n", timenow, ((packet*)elem->obj)->id, timeinq1, tokens_in_bucket);

		//add to q2
		pk = ((packet*)elem->obj)->id;
		gettimeofday(&q2arr, NULL);
		((packet*)elem->obj)->q2arrive = q2arr;
		addto_q2list();
		q1packets++;
		timenow = calculateTime();
		printf("%012.3fms: p%d enters Q2\n", timenow, pk);
		pthread_cond_signal(&q2empty);

	}
	pthread_mutex_unlock(&q1m);
	}
return(0);
}


//serverfunc1
void* serverfunc1(void *arg)
{

	double servicetime, timenow, timeinq2,timeinsystem;
	struct timeval q2dep, systemdeparttime, systementertime;
	struct timeval beginservice, endservice;
	double serTime;
	int id;

	while(1)
	{
	if(ctrlcpressed)
	{
		pthread_mutex_lock(&q1m);
		pthread_cond_signal(&q2empty);
		pthread_mutex_unlock(&q1m);
		break;
	}
	pthread_mutex_lock(&q1m);
	if(My402ListEmpty(q2list))
	{
		if((server1packets+server2packets+packetsdropped) == n)
		{
			pthread_cond_signal(&q2empty);
			pthread_cancel(sigThread);
			pthread_mutex_unlock(&q1m);
			break;
		}
		pthread_cond_wait(&q2empty, &q1m);
		if(ctrlcpressed)
		{
			pthread_cond_signal(&q2empty);
			pthread_mutex_unlock(&q1m);
			break;
		}
		if((server1packets+server2packets+packetsdropped) == n)
		{
			pthread_cond_signal(&q2empty);
			pthread_cancel(sigThread);
			pthread_mutex_unlock(&q1m);
			break;
		}
	}


	My402ListElem* elem = My402ListFirst(q2list);
	servicetime = ((packet*)elem->obj)->transmissiontime;
	totaltimes1+=servicetime;
	id = ((packet*)elem->obj)->id;
	gettimeofday(&q2dep, NULL);
	((packet*)elem->obj)->q2depart = q2dep;
	systementertime = ((packet*)elem->obj)->q1arrive;
	timeinq2 = (convert_to_microseconds(q2dep) - convert_to_microseconds(((packet*)elem->obj)->q2arrive))/1000;
	My402ListUnlink(q2list, elem);
	q2packets++;
	timenow = calculateTime();
	printf("%012.3fms: p%d leaves Q2, time in Q2 = %.3fms\n", timenow, id, timeinq2);
	totaltimeq2+=timeinq2;
	timenow = calculateTime();
	printf("%012.3fms: p%d begins service at S1, requesting %dms of service\n", timenow, id, (int)servicetime);
	pthread_mutex_unlock(&q1m);

	gettimeofday(&beginservice, NULL);
	usleep(servicetime*1000);
	gettimeofday(&endservice, NULL);
	timenow = calculateTime();
	serTime = (convert_to_microseconds(endservice) - convert_to_microseconds(beginservice))/1000;
	gettimeofday(&systemdeparttime, NULL);
	timeinsystem = (convert_to_microseconds(systemdeparttime) - convert_to_microseconds(systementertime))/1000;

	printf("%012.3fms: p%d departs from S1, service time = %.3fms, time in system = %.3fms\n", timenow, id, serTime, timeinsystem );

	pthread_mutex_lock(&q1m);
	server1packets++;
	totaltimeinsystem+=timeinsystem;
	totaltimeinsystemsq+=pow(timeinsystem, 2);
	pthread_mutex_unlock(&q1m);
}
return(0);
}

//serverfunc2
void* serverfunc2(void *arg)
{
	double servicetime, timenow, timeinq2, timeinsystem;
	struct timeval q2dep, systemdeparttime, systementertime;
	struct timeval beginservice, endservice;
	double serTime;
	int id;
	while(1)
	{
	if(ctrlcpressed)
	{
		pthread_mutex_lock(&q1m);
		pthread_cond_signal(&q2empty);
		pthread_mutex_unlock(&q1m);
		break;
	}
	pthread_mutex_lock(&q1m);
	if(My402ListEmpty(q2list))
	{
		if((server2packets+server1packets+packetsdropped) == n)
		{
			pthread_cond_signal(&q2empty);
			pthread_cancel(sigThread);
			pthread_mutex_unlock(&q1m);
			break;
		}
		pthread_cond_wait(&q2empty, &q1m);

		if(ctrlcpressed)
		{
			pthread_cond_signal(&q2empty);
			pthread_mutex_unlock(&q1m);
			break;
		}
		if((server1packets+server2packets+packetsdropped) == n){
			pthread_cond_signal(&q2empty);
			pthread_cancel(sigThread);
		        pthread_mutex_unlock(&q1m);
			break;
                }

	}
	My402ListElem* elem = My402ListFirst(q2list);
	servicetime = ((packet*)elem->obj)->transmissiontime;
	totaltimes2+=servicetime;
	id = ((packet*)elem->obj)->id;
	gettimeofday(&q2dep, NULL);
	((packet*)elem->obj)->q2depart = q2dep;
	timeinq2 = (convert_to_microseconds(q2dep) - convert_to_microseconds(((packet*)elem->obj)->q2arrive))/1000;
	systementertime = ((packet*)elem->obj)->q1arrive;
	My402ListUnlink(q2list, elem);
	q2packets++;
	timenow = calculateTime();
	printf("%012.3fms: p%d leaves Q2, time in Q2 = %.3fms\n", timenow, id, timeinq2);
	totaltimeq2+=timeinq2;
	timenow = calculateTime();
	printf("%012.3fms: p%d begins service at S2, requesting %dms of service\n", timenow, id, (int)servicetime);
	pthread_mutex_unlock(&q1m);

	gettimeofday(&beginservice, NULL);
	usleep(servicetime*1000);
	gettimeofday(&endservice, NULL);
	timenow = calculateTime();
	serTime = (convert_to_microseconds(endservice) - convert_to_microseconds(beginservice))/1000;
	gettimeofday(&systemdeparttime, NULL);
	timeinsystem = (convert_to_microseconds(systemdeparttime) - convert_to_microseconds(systementertime))/1000;
	printf("%012.3fms: p%d departs from S2, service time = %.3fms, time in system = %.3fms\n", timenow, id, serTime, timeinsystem);

	pthread_mutex_lock(&q1m);
		server2packets++;
		totaltimeinsystem+=timeinsystem;
		totaltimeinsystemsq+=pow(timeinsystem, 2);
	pthread_mutex_unlock(&q1m);
}
return(0);
}

//printStatistics
void printStatistics()
{
	double avgq1packets, avgq2packets, avgs1packets, avgs2packets;
	double avgpacketservicetime, avgpacketiatime, avgtimeinsystem;
	double variance;
	avgq1packets = ((totaltimeq1)/1000)/totalemulationtime;
	avgq2packets = ((totaltimeq2)/1000)/totalemulationtime;
	avgs1packets = ((totaltimes1)/1000)/totalemulationtime;
	avgs2packets = ((totaltimes2)/1000)/totalemulationtime;

	avgpacketservicetime = ((totaltimes1+totaltimes2)/1000)/(server1packets+server2packets);
	avgpacketiatime = (totalinterarrivaltime/totalnumberofpackets);

	avgtimeinsystem = (totaltimeinsystem/(server1packets+server2packets));

	//standard deviation
	variance = ((totaltimeinsystemsq/(server1packets+server2packets)) - pow(avgtimeinsystem, 2));

	printf("\nStatistics:\n\n");
	printf("\taverage packet inter-arrival time = %.6g\n", avgpacketiatime/1000);
	if(server1packets+server2packets == 0)
		printf("\taverage packet service time = n/a. No packets were serviced\n\n");
	else
		printf("\taverage packet service time = %.6g \n\n", avgpacketservicetime);
	if(q1packets!=0)
		printf("\taverage number of packets in Q1 = %.6g\n", avgq1packets);
	else
		printf("\taverage number of packets in Q1 = n/a. No packets arrived at Q1\n");
	if(q2packets!=0)
		printf("\taverage number of packets in Q2 = %.6g\n", avgq2packets);
	else
		printf("\taverage number of packets in Q2 = n/a. No packets arrived at Q2\n");
	if(server1packets!=0)
		printf("\taverage number of packets in S1 = %.6g\n", avgs1packets);
	else
		printf("\taverage number of packets in S1 = n/a. No packets were serviced at S1\n");
	if(server2packets!=0)
		printf("\taverage number of packets in S2 = %.6g\n\n", avgs2packets);
	else
		printf("\taverage number of packets in S2 = n/a. No packets were serviced at S2\n\n");

	if(server1packets+server2packets!=0)
	{
		printf("\taverage time a packet spent in system = %.6g\n", avgtimeinsystem/1000);
		printf("\tstandard deviation for time spent in system = %.6g\n\n", sqrt(variance)/1000);
	}
	else
	{	printf("\taverage time a packet spent in system = n/a. No packets were serviced\n");
		printf("\tstandard deviation for time spent in system = n/a. No packets were serviced\n\n");
	}

	if(tokens_arrived!=0)
		printf("\ttoken drop probability = %.6g\n", (double)tokensdropped/(double)tokens_arrived);
	else
		printf("\ttoken drop probability = n/a. No tokens were dropped\n");
	if(totalnumberofpackets!=0)
		printf("\tpacket drop probability = %.6g\n\n", (double)packetsdropped/(double)totalnumberofpackets);
	else
		printf("\tpacket drop probability = n/a. No packets were dropped\n");
}

//create threads
void createThreads()
{

	int err;
	double timenow;


	gettimeofday(&emulationstart, 0);
	float emulationbegin = 0.0;
	printf("\n%012.3fms: emulation begins\n", emulationbegin);

	err = pthread_create(&pArr, 0, packetArrival, NULL);
	if(err!=0)
	{
		fprintf(stderr, "Error in creating packetArrival thread\n"); exit(0);
	}

	err = pthread_create(&tDep, 0, tokenDeposit, NULL);
	if(err!=0)
	{
		fprintf(stderr, "Error in creating tokenDeposit thread\n"); exit(0);
	}

	err = pthread_create(&server1, 0, serverfunc1, NULL);
	if(err!=0)
	{
		fprintf(stderr, "Error in creating server1func thread\n"); exit(0);
	}

	err = pthread_create(&server2, 0, serverfunc2, NULL);
	if(err!=0)
	{
		fprintf(stderr, "Error in creating server2func thread\n"); exit(0);
	}
	err = pthread_create(&sigThread, 0, getSignal, NULL);
	if(err!=0)
	{
		fprintf(stderr, "Error in creating Signal thread\n"); exit(0);
	}

	pthread_join(pArr, NULL);
	pthread_join(tDep, NULL);
	pthread_join(server1, NULL);
	pthread_join(server2, NULL);
	if(!packetThreadJoined)
		pthread_join(sigThread, NULL);
	timenow = calculateTime();
	gettimeofday(&emulationend, NULL);
	printf("%012.3fms: emulation ends\n", timenow);
	totalemulationtime = (convert_to_microseconds(emulationend) - convert_to_microseconds(emulationstart))/1000000;

}


//main
int main(int argc, char* argv[])
{
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigprocmask(SIG_BLOCK, &set, 0);

	struct stat s;

	readInput(argc, argv);
	if(trace_driven_mode)
	{
		fp = fopen(tfile, "r");
		if(stat(tfile,&s)==0)
		{
			if( s.st_mode & S_IFDIR )
			{
			    fprintf(stderr,"Input argument is a Directory\n");
			    return(0);
			}
		}
		if(fp == NULL)
		{
			if(errno==EACCES)
			{
				fprintf(stderr, "Input file %s cannot be opened - access denied\n", tfile);
				return(0);
			}
			else
			{
				fprintf(stderr, "Input file %s does not exist\n", tfile);
				return(0);
			}
		}
		if(fp == NULL)
		{
			fprintf(stderr, "Input file %s does not exist\n", tfile);
			exit(0);
		}
	}
	displayInputArguments();
	initializequeues();
	createThreads();
	printStatistics();

return(0);
}
