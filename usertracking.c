#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <pthread.h>
#include <time.h>
#include <libconfig.h>
#include <unistd.h> 
#include <signal.h> 
#include <sys/param.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include "usertracking.h"
#include <mysql/mysql.h>
#include <rrd.h>
#define DEBUG 1
#define MAXFILE 65535
#define MAXSQL  4000
#define OUTPUT
//#define DAEMON
//#define  DEBUGSNMP
#define NETSNMP_DISABLE_MIB_LOADING

#define NETSNMP_DS_WALK_INCLUDE_REQUESTED                   1
#define NETSNMP_DS_WALK_PRINT_STATISTICS                    2
#define NETSNMP_DS_WALK_DONT_CHECK_LEXICOGRAPHIC	3
#define NETSNMP_DS_WALK_TIME_RESULTS     	                    4
#define NETSNMP_DS_WALK_DONT_GET_REQUESTED	                5
#define CISCO_IETF_IP_MIB_cInetNetToMediaPhysAddress    "1.3.6.1.4.1.9.10.86.1.1.3.1.3"
#define IF_MIB_ifName                                                        "1.3.6.1.2.1.31.1.1.1.1"
#define BRIDGE_MIB_dot1dTpFdbPort                                    "1.3.6.1.2.1.17.4.3.1.2"
#define BRIDGE_MIB_dot1dBasePortIfIndex                           "1.3.6.1.2.1.17.1.4.1.2"
#define IP_MIB_ipNetToMediaPhysAddress                              "1.3.6.1.2.1.4.22.1.2"
#define CISCO_VLAN_IFTABLE_RELATIONSHIP_MIB_cviRoutedVlanIfIndex "1.3.6.1.4.1.9.9.128.1.1.1.1.3"
struct config_struct set = {
    "localhost","database","mysql_user","mysql_password",3306
};
struct sql_query *mysql_querys_insinet[MAXROUTER*MAXTHREAD_PREROUTER];
struct sql_query *mysql_querys_insinet6[MAXROUTER*MAXTHREAD_PREROUTER];
struct sql_query deadhost[50];
/*
struct sql_query *Sql_InetCount;
struct sql_query *this_Sql_InetCount;
struct sql_query *pre_Sql_InetCount;
*/
int DeadHost_num = -1;
//struct sql_query *mysql_querys_upstates;
MYSQL mysql;
MYSQL_RES *result = NULL;

Sw3Vlan_v6 	    Sw3Vlan_v6_TBL[MAXL3][MAXVLANPRESWITCH];
Sw3Vlan_v4      Sw3Vlan_v4_TBL[MAXL3][MAXVLANPRESWITCH];
struct Sw_Node *Sw2_Nodes[MAXL3][MAXSWITCHPREL3];
 struct netstruct netstructs[MAXL3];
pthread_mutex_t m_Locker;
pthread_cond_t m_Cond;
pthread_mutexattr_t attr;

int
main(int argc, char *argv[])
{
     pid_t pc;
    FILE *fp; 
    pthread_t id[MAXTHREAD_PREROUTER]={0};
    int ret;
    struct arg *args[MAXROUTER] = {NULL};
    void *sessp = NULL, *oidsessp=NULL;
    int liberr, syserr;
    char *errstr;
    netsnmp_session oidsess,session, *ss = NULL; 
    netsnmp_pdu    *pdu = NULL, *response = NULL;
    netsnmp_variable_list *vars = NULL;
    int             arg;
    oid             name[MAX_OID_LEN];
    size_t          name_length;
    oid             root[MAX_OID_LEN];
    size_t          rootlen;
    struct Sw_Node *Sw_Nodes[20];
    int             count;
    int             running;
    int             status;
    int             check;
    int             i,j,k,m;
    int             exitval = 0;
    int		     vlanindex = -1;
    int		     inetvlanindex = -1;
    int thread_index = -1;
    int          routerindex = 0;
    int             ifindex = 9999;
    int             inetifindex = 9999;
    int   l2count=0,min_pre_thread = 0,max_pre_thread =0;
    struct oid 	     *op = NULL;
    struct Sw_Node   **pL3 = NULL;
    struct Sw_Node   **pSwitch = NULL;
    struct Sw_Node   *pL2 = NULL;
    struct Sw_Node   *pnL2 = NULL;
    struct sql_query *query_list_head = NULL,*query_list_head_v6 = NULL,*Sql_InetCount = NULL, *this_Sql_InetCount = NULL ,*pre_Sql_InetCount = NULL;;
   u_char aInetCount[5]={0};  
   u_char avlanid[5] = {0};
   u_char ipcounttime[11] = {0};
   char *createparams[5] ;
 char *updateparams[4]= {0};
 char RRDFile[80] = {0};
 char RRDValue[20] = {0};
 
    u_char              strvlan[5]={0};
    u_char              commatvlan[10]={0};
    int              oidrunnum = 0;
    struct oid     oids[] = {
        { CISCO_IETF_IP_MIB_cInetNetToMediaPhysAddress},/*CISCO-IETF-IP-MIB cInetNetToMediaPhysAddress*/
        {IP_MIB_ipNetToMediaPhysAddress},
        {CISCO_VLAN_IFTABLE_RELATIONSHIP_MIB_cviRoutedVlanIfIndex},
        { NULL }
    };


    int ch;
    u_char *switchfile;
    while ((ch = getopt(argc, argv, "c:")) != -1)
    {
        switch (ch) {
        case 'c':
	    switchfile = optarg;	
            break;
        case '?':
            printf("Unknown option: %c\n",(char)optopt);
            break;
        }
    }
/*
    for(i = 0;i<MAXSQL;i++) {

        if(mysql_querys[i] =(u_char *)malloc(300*sizeof(u_char))==NULL){
            printf("error");
            exit(1);
        }
    }*/
    config_t* conf = &(config_t){};

SOCK_STARTUP;
#ifdef DAEMON
init_daemon();
while (1) {
#endif 
    if ((fp=fopen(switchfile,"rt"))==NULL){
        printf("cannot open file!");
        exit(0);
    }
    memset(Sw3Vlan_v4_TBL,0,sizeof(Sw3Vlan_v4_TBL));
    memset(Sw3Vlan_v6_TBL,0,sizeof(Sw3Vlan_v6_TBL));
    memset(Sw_Nodes,0,sizeof(Sw_Nodes));
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_TIMED_NP);
    pthread_mutex_init(&m_Locker, &attr);
    pthread_cond_init(&m_Cond, NULL);
    config_init(conf);
    config_read(conf, fp);
 /*read switch data*/
    readswitch(conf,Sw_Nodes);
    config_destroy(conf);
     fclose(fp);
     snmp_sess_init( &session );
   sessp = snmp_sess_open(&session); 
    initialize (oids);
    snmp_sess_close(sessp);
    printf("start time = %d\n", time((time_t*)NULL));
    //SOCK_STARTUP;
    for( pL3=Sw_Nodes; *pL3; pL3++){
        for (op = oids; op->OidName; op++) {
            snmp_sess_init( &session );
            session.peername = (*pL3)->SwitchAdd; 
            session.version = SNMP_VERSION_1;
	 //   session.retries = 2;   
         //   session.timeout = 300000; 
            rootlen = MAX_OID_LEN;
    /*
     * open an SNMP session
     */
            sessp = snmp_sess_open(&session); 
            if (sessp == NULL) {
                /* Error codes found in open calling argument */
                snmp_error(&session, &liberr, &syserr, &errstr);
                printf("SNMP create error %s.\n", errstr);
                free(errstr);
                SOCK_CLEANUP;
                return 0;
            }
            ss = snmp_sess_session(sessp);
            free(ss->community);
            ss->community = strdup((*pL3)->community);  
            ss->community_len = strlen((*pL3)->community);
    /*
     * get first object to start walk
     */
        oidrunnum = 0;
        memmove(name, op->root, op->rootlen * sizeof(oid));
        name_length = op->rootlen;
        running = 1;
        #ifndef DEBUGSNMP
        while (running) {
        /*
         * create PDU for GETNEXT request and add object name to request
         */
            pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
            snmp_add_null_var(pdu, name, name_length);

        /*
         * do the request
         */
            status = snmp_sess_synch_response(sessp, pdu, &response);
            if (status == STAT_SUCCESS) {
                if (response->errstat == SNMP_ERR_NOERROR) {
                /*
                 * check resulting variables
                 */
                    for (vars = response->variables; vars;
                          vars = vars->next_variable) {
                        if ((vars->name_length < op->rootlen)
                            || (memcmp(op->root, vars->name, op->rootlen * sizeof(oid))!= 0)) {
                        /*
                         * not part of this subtree
                         */
                            running = 0;
                            continue;
                        }
                            oidrunnum++;
                            if(strcmp(op->OidName,CISCO_IETF_IP_MIB_cInetNetToMediaPhysAddress) == 0 && (*pL3)->SwitchType == 3){
                                sprint_my_v6tbl(&ifindex, &vlanindex,routerindex ,Sw3Vlan_v6_TBL, vars);
                            }else if(strcmp(op->OidName,IP_MIB_ipNetToMediaPhysAddress) == 0 && (*pL3)->SwitchType == 3){
                                sprint_my_v4tbl(&inetifindex, &inetvlanindex, routerindex, Sw3Vlan_v4_TBL, vars);
                            }else if(strcmp(op->OidName,CISCO_VLAN_IFTABLE_RELATIONSHIP_MIB_cviRoutedVlanIfIndex) == 0 ){
                                if((*pL3)->SwitchType == 3){
				    //if (strcmp((*pL3)->SwitchAdd , "210.77.16.118") == 0){*vars->val.integer = 31;vars->name[14] = 102;}
                                    for (i=0;i<=vlanindex;i++){
                                        if (Sw3Vlan_v6_TBL[routerindex][i].vlanid == *vars->val.integer ){
                                            Sw3Vlan_v6_TBL[routerindex][i].vlanid = vars->name[14];
                                        }

                                    }
                                    for (i=0;i<=inetvlanindex; i++){
                                        if( Sw3Vlan_v4_TBL[routerindex][i].vlanid == *vars->val.integer ){
                                            Sw3Vlan_v4_TBL[routerindex][i].vlanid = vars->name[14];
					    //printf("Vlan: %u\n ipcount: %u\n ",Sw3Vlan_v4_TBL[routerindex][i].vlanid,Sw3Vlan_v4_TBL[routerindex][i].inetcount);
					/*add inetcount*/
					sprintf(ipcounttime,"%d",time((time_t*)NULL));
					sprintf(aInetCount,"%d",Sw3Vlan_v4_TBL[routerindex][i].inetcount);
					sprintf(avlanid,"%d",Sw3Vlan_v4_TBL[routerindex][i].vlanid);

				        if (Sql_InetCount == NULL){
                                           Sql_InetCount = (struct sql_query *)malloc(sizeof(struct sql_query));
                                           this_Sql_InetCount = Sql_InetCount;
                                           strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcpy(this_Sql_InetCount->query,"INSERT DELAYED into `IPCount` (`Router`,`Vlan`,`Count`,`Time`) values ('"),session.peername),"','"),avlanid),"','"),aInetCount ),"',FROM_UNIXTIME("),ipcounttime),"))");
					   this_Sql_InetCount->nextquery = NULL;
					   pre_Sql_InetCount = this_Sql_InetCount; 
/*                                           strcat(strcat(strcpy(RRDFile,"/usr/local/usertracking/RRDFile/Vlan"),avlanid),".rrd");
                                           strcat(strcat(strcpy(RRDValue,ipcounttime),":"),aInetCount);
                                           updateparams[0] = "rrdupdate";
                                           updateparams[1] = RRDFile;
                                           updateparams[2] = RRDValue;
                                           updateparams[3] = NULL;
                                           optind = opterr = 0;
                                           rrd_clear_error();
                                       //    printf("RRDFile %s\n",RRDFile);
                                       //    printf("RRDValue %s\n",RRDValue);
                                           rrd_update(3, updateparams);
*/
                                          }else{
					   this_Sql_InetCount = (struct sql_query *)malloc(sizeof(struct sql_query));
                                           strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcpy(this_Sql_InetCount->query,"INSERT DELAYED into `IPCount` (`Router`,`Vlan`,`Count`,`Time`) values ('"),session.peername),"','"),avlanid),"','"),aInetCount ),"',FROM_UNIXTIME("),ipcounttime),"))");
					   this_Sql_InetCount->nextquery = pre_Sql_InetCount;
					   pre_Sql_InetCount = this_Sql_InetCount;
					   Sql_InetCount = this_Sql_InetCount;
/*                                           strcat(strcat(strcpy(RRDFile,"/usr/local/usertracking/RRDFile/Vlan"),avlanid),".rrd");
                                           strcat(strcat(strcpy(RRDValue,ipcounttime),":"),aInetCount);
                                           updateparams[0] = "rrdupdate";
                                           updateparams[1] = RRDFile;
                                           updateparams[2] = RRDValue;
                                           updateparams[3] = NULL;
                                           optind = opterr = 0;
                                           rrd_clear_error();
                                     //      printf("RRDFile %s\n",RRDFile);
                                     //      printf("RRDValue %s\n",RRDValue);
                                           rrd_update(3, updateparams);
*/
					  }
/*add inetcount*/

                                        }
                                    }
                                }
                            }
                            if ((vars->type != SNMP_ENDOFMIBVIEW) &&
                                (vars->type != SNMP_NOSUCHOBJECT) &&
                                (vars->type != SNMP_NOSUCHINSTANCE)) {
                        /*
                         * not an exception value
                         */
                                memmove((char *) name, (char *) vars->name,
                                        vars->name_length * sizeof(oid));
                                name_length = vars->name_length;
                            } else

                        /*
                         * an exception value, so stop
                         */
                                running = 0;
                        }
                    } else {
                /*
                 * error in response, print it
                 */
                        running = 0;
                        if (response->errstat == SNMP_ERR_NOSUCHNAME) {
                            printf("End of MIB\n");
                        } else {
                            fprintf(stderr, "Error in packet.\nReason: %s\n",
                                    snmp_errstring(response->errstat));
                            if (response->errindex != 0) {
                                fprintf(stderr, "Failed object: ");
                                for (count = 1, vars = response->variables;
                                      vars && count != response->errindex;
                                      vars = vars->next_variable, count++)
                                /*EMPTY;*/
                                    if (vars)
                                        fprint_objid(stderr, vars->name,vars->name_length);
                                    fprintf(stderr, "\n");
                            }
                            exitval = 2;
                        }
                    }
                } else if (status == STAT_TIMEOUT) {
                    fprintf(stderr, "Timeout: No Response from %s\n",
                            session.peername);
                    running = 0;
                    exitval = 1;
		} else {                /* status == STAT_ERROR */
                    snmp_sess_perror("traceuser", ss);
                    running = 0;
                    exitval = 1;
                }
                if (response){
                    snmp_free_pdu(response);
                    response = NULL;
                }
        }
        if (response){
            snmp_free_pdu(response);
            response = NULL;
        }
         #endif

        if (response){
            snmp_free_pdu(response);
            response = NULL;
        }
        snmp_sess_close(sessp);
        }/*Next oid*/

        if (response){
            snmp_free_pdu(response);
            response = NULL;
        }
        for(pL2 = *pL3 ; pL2; pL2 = pL2->next) {
            Sw2_Nodes[routerindex][l2count] = pL2;
            l2count++;
        }
        netstructs[routerindex].vlancount = vlanindex;
        netstructs[routerindex].inetvlancount = inetvlanindex;
        netstructs[routerindex].switchcount = l2count;
        routerindex++;
        vlanindex = -1;
        inetvlanindex = -1;
        l2count = 0;
 }/*Next Router*/
 printf("l3endtime = %d\n", time((time_t*)NULL));

    for(i = 0; i < routerindex; i++) {
        netstructs[i].L3count = routerindex;
    }
    #ifdef OUTPUT       
    for(i=0;i<routerindex;i++) {
        args[i] = (struct arg*)malloc(sizeof(struct arg));
        if(args == NULL) {
            exit(1);
        }
    //    pthread_mutex_lock(&l3_Locker);
        args[i]->vlanindex = netstructs[i].vlancount;
        args[i]->inetvlanindex = netstructs[i].inetvlancount;
        args[i]->L3num = i;
        if( netstructs[i].switchcount > MAXTHREAD_PREROUTER){
            for(k = 0;k<MAXTHREAD_PREROUTER;k++) {
                pthread_mutex_lock(&m_Locker);
                thread_index++;
                // pthread_mutex_trylock(&m_Locker);
                min_pre_thread = k*(netstructs[i].switchcount/MAXTHREAD_PREROUTER);
                max_pre_thread = k*(netstructs[i].switchcount/MAXTHREAD_PREROUTER)+netstructs[i].switchcount/MAXTHREAD_PREROUTER-1;
	//	printf("min_pre_thread = %d %s max_pre_thread = %d %s \n", min_pre_thread,Sw2_Nodes[i][min_pre_thread]->SwitchAdd,max_pre_thread,Sw2_Nodes[i][max_pre_thread]->SwitchAdd);
                args[i]->thread_index = thread_index;
                args[i]->min_pre_thread = min_pre_thread;
                args[i]->max_pre_thread = max_pre_thread;
                ret=pthread_create(&id[k],NULL,scanl2,args[i]);

                if(ret != 0){
                    printf("can't create thread: %s\n",strerror(ret));
                    return 1;
                }
                pthread_cond_wait(&m_Cond, &m_Locker);
                pthread_mutex_unlock(&m_Locker);

            }
            for (j = 0;j<MAXTHREAD_PREROUTER;j++) {
                if(id[j] != 0) 
                    pthread_join(id[j],NULL);
            }
            if(netstructs[i].switchcount%MAXTHREAD_PREROUTER !=0 ) {
                pthread_mutex_lock(&m_Locker);
                thread_index++;
                //  pthread_mutex_trylock(&m_Locker);
                min_pre_thread = max_pre_thread+1;
                max_pre_thread = max_pre_thread + netstructs[i].switchcount % MAXTHREAD_PREROUTER;
		//printf("min_pre_thread = %d %s max_pre_thread = %d %s \n", min_pre_thread,Sw2_Nodes[i][min_pre_thread]->SwitchAdd,max_pre_thread,Sw2_Nodes[i][max_pre_thread]->SwitchAdd);
                args[i]->thread_index = thread_index;
                args[i]->min_pre_thread = min_pre_thread;
                args[i]->max_pre_thread = max_pre_thread;
                ret=pthread_create(&id[MAXTHREAD_PREROUTER],NULL,scanl2,args[i]); 
                if(ret != 0){
                    printf("can't create thread: %s\n",strerror(ret));
                    return 1;
                }
                pthread_cond_wait(&m_Cond, &m_Locker);
                pthread_mutex_unlock(&m_Locker);
                pthread_join(id[MAXTHREAD_PREROUTER],NULL);
            }
        }else{
            pthread_mutex_lock(&m_Locker);
            thread_index++;
           // printf("netstructs[i].switchcount < MAXTHREAD_PREROUTER %d\n",thread_index);
            min_pre_thread = 0;
            max_pre_thread = netstructs[i].switchcount-1;
            args[i]->thread_index = thread_index;
            args[i]->min_pre_thread = min_pre_thread;
            args[i]->max_pre_thread = max_pre_thread;
           // pthread_mutex_lock(&m_Locker);
           // thread_index++;
            ret=pthread_create(&id[0],NULL,scanl2,args[i]);
            pthread_cond_wait(&m_Cond, &m_Locker);
            pthread_mutex_unlock(&m_Locker);
             pthread_join(id[0],NULL);
        }

    }
    #endif
     printf("end time = %d\n", time((time_t*)NULL));
//    printf("desdhostsql %s",deadhost[DeadHost_num].query);
    db_connect(set.dbdb, &mysql);
      
                for(i=0;i<MAXROUTER*MAXTHREAD_PREROUTER;i++) {
                    if(mysql_querys_insinet[i] != NULL) {
                        query_list_head = mysql_querys_insinet[i];
                        while(query_list_head->nextquery != NULL) {
//                           printf("%s\n",query_list_head->query);
                           db_insert(&mysql, query_list_head->query);
                            query_list_head = query_list_head->nextquery;
                        }
                        if(query_list_head->nextquery == NULL) {
                            db_insert(&mysql, query_list_head->query);
                        }
                    }
                    if(mysql_querys_insinet6[i] != NULL) {
                        query_list_head_v6 = mysql_querys_insinet6[i];
                        while(query_list_head_v6->nextquery != NULL) {
//                           printf("%s\n",query_list_head_v6->query);
                           db_insert(&mysql, query_list_head_v6->query);
                            query_list_head_v6 = query_list_head_v6->nextquery;
                        }
                        if(query_list_head_v6->nextquery == NULL) {
                            db_insert(&mysql, query_list_head_v6->query);
                        }
                    }
                }   
		query_list_head = NULL; 

		if ( Sql_InetCount != NULL){
		    query_list_head = Sql_InetCount;
		    while(query_list_head->nextquery != NULL) {
//                        printf("%s\n",query_list_head->query);
                        db_insert(&mysql, query_list_head->query);
			query_list_head = query_list_head->nextquery;
                     }
                    if(query_list_head->nextquery == NULL) {
//		      printf("%s\n",query_list_head->query);
                       db_insert(&mysql, query_list_head->query);
                    }
                 }

		for(i=0;i<=DeadHost_num;i++){
//		    printf("SQL:%s",deadhost[i].query);
		    db_insert(&mysql, deadhost[i].query);
		}

 	mysql_free_result(result);
	mysql_close(&mysql);

  freeany( vlanindex, inetvlanindex,Sw_Nodes,args,routerindex);
  pthread_mutex_destroy(&m_Locker);
      pthread_cond_destroy(&m_Cond);
       SOCK_CLEANUP;
#ifdef DAEMON
  sleep(10);
   //memset(id,0,MAXTHREAD_PREROUTER*sizeof(id));
   //exitval = 0;
    vlanindex = -1;
    inetvlanindex = -1;
    routerindex = 0;
    ifindex = 9999;
    inetifindex = 9999;
    l2count=0;min_pre_thread = 0;max_pre_thread =0;
    conf = &(config_t){};
  //  memset(strvlan,0,5*sizeof(strvlan));
  // memset(commatvlan,0,10*sizeof(commatvlan));
    oidrunnum = 0;
// return exitval;
    }
#endif
     	/* connect to database */
return exitval;
}
       
