#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <libconfig.h>
#include <time.h>
#include "usertracking.h"
#include <mysql/mysql.h>
//#define DEBUGV4
//#define DEBUGV6
#define SQL
#define NETSNMP_DS_WALK_INCLUDE_REQUESTED                   1
#define NETSNMP_DS_WALK_PRINT_STATISTICS                    2
#define NETSNMP_DS_WALK_DONT_CHECK_LEXICOGRAPHIC	3
#define NETSNMP_DS_WALK_TIME_RESULTS     	                    4
#define NETSNMP_DS_WALK_DONT_GET_REQUESTED	                5
#define CISCO_IETF_IP_MIB_cInetNetToMediaPhysAddress    "1.3.6.1.4.1.9.10.86.1.1.3.1.3"
#define ifAlias                                                        "1.3.6.1.2.1.31.1.1.1.18"
#define IF_MIB_ifName                                                        "1.3.6.1.2.1.31.1.1.1.1"
#define BRIDGE_MIB_dot1dTpFdbPort                                    "1.3.6.1.2.1.17.4.3.1.2"
#define Q_BRIDGE_MIB_dot1qTpFdbPort					"1.3.6.1.2.1.17.7.1.2.2.1.2"//add for h3cE528
#define BRIDGE_MIB_dot1dBasePortIfIndex                           "1.3.6.1.2.1.17.1.4.1.2"
#define IP_MIB_ipNetToMediaPhysAddress                              "1.3.6.1.2.1.4.22.1.2"
//#define CISCO_VTP_MIB_vlanTrunkPortDynamicStatus                "1.3.6.1.4.1.9.9.46.1.6.1.1.14"
extern struct Sw_Node *Sw2_Nodes[MAXL3][MAXSWITCHPREL3];
extern Sw3Vlan_v6   Sw3Vlan_v6_TBL[MAXL3][MAXVLANPRESWITCH];
extern Sw3Vlan_v4      Sw3Vlan_v4_TBL[MAXL3][MAXVLANPRESWITCH];
extern struct sql_query *mysql_querys_insinet[MAXROUTER*MAXTHREAD_PREROUTER];
extern struct sql_query *mysql_querys_insinet6[MAXROUTER*MAXTHREAD_PREROUTER];
extern pthread_cond_t m_Cond;
extern pthread_mutex_t m_Locker;
extern struct sql_query deadhost[50];
extern int DeadHost_num;
void *
scanl2(void *arg)
{
    pthread_mutex_lock(&m_Locker);
    struct arg *args;
    args = (struct arg*)arg;
    int vlanindex = args->vlanindex;
    int inetvlanindex = args->inetvlanindex;
    int L3num = args->L3num;
    int thread_index = args->thread_index;
    int min_pre_thread = args->min_pre_thread;
    int max_pre_thread = args->max_pre_thread;
    struct sql_query *thismysql_querys_insinet = NULL;
    struct sql_query *premysql_querys_insinet = NULL;
    struct sql_query *thismysql_querys_insinet6 = NULL;
    struct sql_query *premysql_querys_insinet6 = NULL;
    pthread_cond_signal(&m_Cond);
    pthread_mutex_unlock(&m_Locker);
    void *sessp=NULL;
    netsnmp_session session, *ss; 
    netsnmp_pdu    *pdu, *response;
    netsnmp_variable_list *vars;
    oid             name[MAX_OID_LEN];
    size_t          name_length;
    oid             root[MAX_OID_LEN];
    size_t          rootlen;
    int             count;
    int         ifcount = 0;
    int             running; 
    int             status;
    int             check;
    int             i,j,m;
    int             _num;
    int             exitval = 0;
    Inet6List       *pinet6 = NULL,*pninet6 = NULL,*pre6 = NULL,*templist6 = NULL;
    InetList        *pinet = NULL,*ppinet = NULL,*pre = NULL,*templist = NULL;
    struct oid 	     *op = NULL;
    struct Sw_Node   *pSwitch = NULL;
    int                      *pVlan = NULL;/*point to Vlanid*/
    int                       *pt = NULL;/*point to trunkport*/
    int                         istrunk = 0;
    u_char              strvlan[5]={0},_strvlan[5] = {0};
    u_char              commatvlan[10]={0};
    int              oidrunnum = 0;
     u_char networkadd_buf[18];
     int dot1baseport_buf;
    struct oid     oids[] = {
        { IF_MIB_ifName},/*IF-MIB ifName*/
        {ifAlias},
        { BRIDGE_MIB_dot1dBasePortIfIndex }, /*BRIDGE-MIB dot1dTpFdbPort*/
        { BRIDGE_MIB_dot1dTpFdbPort  },/*BRIDGE-MIB dot1dBasePortIfIndex*/
	{Q_BRIDGE_MIB_dot1qTpFdbPort},
        { NULL }
    };
    struct ifName ifName_tbl[1200];
    struct dot1dTpFdbPort_Node  dot1dTpFdbPort_Nodes;
    u_char linuxtime[11];
     snmp_sess_init( &session );
     sessp = snmp_sess_open(&session); 
    initialize (oids);
    snmp_sess_close(sessp);
    for(_num = min_pre_thread;_num <=max_pre_thread ; _num++){
        pSwitch = Sw2_Nodes[L3num][_num];

   //  printf("Sw2_Nodes[%d][%d]->SwitchAdd = %s \n",L3num,_num,Sw2_Nodes[L3num][_num]->SwitchAdd);
        for (op = oids; op->OidName; op++) {
	    if (*pSwitch->Vlanid == 4094) break;
            if( strcmp(op->OidName,IF_MIB_ifName) == 0 && strcmp(pSwitch->vender ,"3com")==0 ) 
                continue;
            if(strcmp(op->OidName,ifAlias) == 0 && (strcmp(pSwitch->vender ,"cisco")==0||strcmp(pSwitch->vender ,"h3c")==0))
                continue; 
	    if(strcmp(op->OidName,Q_BRIDGE_MIB_dot1qTpFdbPort)==0 && strcmp(pSwitch->vender ,"h3c")!=0)
		continue;
            if(strcmp(op->OidName,BRIDGE_MIB_dot1dTpFdbPort)==0 && strcmp(pSwitch->vender ,"h3c")==0)
                continue;

            for (pVlan=pSwitch->Vlanid ;*pVlan!=0; pVlan++){
                sprintf(strvlan,"%d",*pVlan);
     //          printf("host %s pvlan %d\n",pSwitch->SwitchAdd ,*pVlan); 
                snmp_sess_init( &session );
                session.peername = pSwitch->SwitchAdd; 
                session.version = SNMP_VERSION_1;
                session.retries = 3;
                session.timeout = 500000;

    /*
     * open an SNMP session
     */
                 sessp = snmp_sess_open(&session); 
                 if (sessp == NULL) {
        /*
         * diagnose snmp_open errors with the input netsnmp_session pointer
         */
                     snmp_sess_perror("snmpwalk", &session);
                   //  SOCK_CLEANUP;
                     exit(1);
                 }
                 ss = snmp_sess_session(sessp);
                 free(ss->community);
                 if(/*strcmp(op->OidName,CISCO_IETF_IP_MIB_cInetNetToMediaPhysAddress) == 0 || 
                    strcmp(op->OidName,IP_MIB_ipNetToMediaPhysAddress) == 0 */
                    strcmp(op->OidName,IF_MIB_ifName) == 0 || strcmp(pSwitch->vender ,"3com") == 0 || strcmp(pSwitch->vender ,"h3c")==0) {
                     ss->community = strdup(pSwitch->community);
                     ss->community_len = strlen(pSwitch->community); 
                 }else if(strcmp(pSwitch->vender ,"cisco") == 0){
                     ss->community = strdup( strcat(strcat(strcat (commatvlan,pSwitch->community), "@"),strvlan) );
                     ss->community_len = strlen(ss->community );
                 }
                 
                 rootlen = MAX_OID_LEN;

        
    /*
     * get first object to start walk
     */
                 oidrunnum = 0;
                 memmove(name, op->root, op->rootlen * sizeof(oid));
                 name_length = op->rootlen;
                 running = 1;
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
                                 if(strcmp(op->OidName,IF_MIB_ifName) == 0 && (strcmp(pSwitch->vender,"cisco") == 0 || strcmp(pSwitch->vender,"h3c") == 0)){
                                         sprint_ifName(ifName_tbl, &ifcount, vars);
                                 }else if(strcmp(op->OidName,ifAlias) == 0 && strcmp(pSwitch->vender,"3com") == 0){
                                     sprint_ifAlias(ifName_tbl, &ifcount, vars);
                                 }else if(strcmp(op->OidName,   BRIDGE_MIB_dot1dBasePortIfIndex ) == 0) {
                                         sprint_dot1dBasePortIfIndex(ifName_tbl, ifcount, vars);
                                 }else if(strcmp(op->OidName,  BRIDGE_MIB_dot1dTpFdbPort ) == 0 || strcmp(op->OidName,Q_BRIDGE_MIB_dot1qTpFdbPort)==0 ) {
                                     	if(strcmp(pSwitch->vender,"h3c") != 0)
                                     		sprint_dot1dTpFdbPort(&dot1dTpFdbPort_Nodes, vars);
                                        if(strcmp(pSwitch->vender,"h3c") == 0)
                                                sprint_dot1qTpFdbPort(&dot1dTpFdbPort_Nodes, vars);
 /*Print v6tbl*/          
                                     for (i=0;i<=vlanindex;i++){
                                         if (Sw3Vlan_v6_TBL[L3num][i].vlanid == *pVlan ){
                                             
                                             pinet6=Sw3Vlan_v6_TBL[L3num][i].inet6list;
                                             while (pinet6->next!= NULL){
                                                 pninet6 = pinet6->next;
                                                 if(strcmp(dot1dTpFdbPort_Nodes.networkadd,pinet6->networkadd) == 0) {
                                                     for ( pt = pSwitch->TrunkPort;*pt!=0;pt++) {
                                                         for(j = 0;j<=ifcount;j++) {
                                                             if(ifName_tbl[j].dot1dBasePort == dot1dTpFdbPort_Nodes.dot1dBasePort)
                                                                 break;
                                                         }

                                                         if (ifName_tbl[j].ifindex == *pt){
                                                             istrunk = 1;
                                                             break;
                                                         }else{
                                                             istrunk = 0;
                                                         }
                                                     }
                                                     if (!istrunk) {
                                                         sprintf(_strvlan,"%d",*pVlan);
                                                         #ifdef DEBUGV6
                                                         printf("Vlan %d ",Sw3Vlan_v6_TBL[L3num][i].vlanid );
                                                         printf("%s ",pinet6->inet6add);
                                                         #endif
                                                         #ifdef SQL
                                                         
                                                         if(mysql_querys_insinet6[thread_index] == NULL) {
                                                             mysql_querys_insinet6[thread_index] = (struct sql_query *)malloc(sizeof(struct sql_query));
                                                             thismysql_querys_insinet6 = mysql_querys_insinet6[thread_index];
                                                              
                                                             createsqlquery(thismysql_querys_insinet6,pSwitch , ifName_tbl,  j, _strvlan,NULL,pinet6);
                                                            thismysql_querys_insinet6->nextquery = NULL;
                                                            premysql_querys_insinet6 = thismysql_querys_insinet6;
                                                         }else{

                                                             thismysql_querys_insinet6 = (struct sql_query *)malloc(sizeof(struct sql_query));
                                                             createsqlquery(thismysql_querys_insinet6,pSwitch , ifName_tbl,  j, _strvlan,NULL,pinet6 );
                                                             thismysql_querys_insinet6->nextquery = premysql_querys_insinet6;
                                                                    premysql_querys_insinet6 = thismysql_querys_insinet6;
                                                                    mysql_querys_insinet6[thread_index] = thismysql_querys_insinet6;
                                                         }
                                                         #endif

                                                     }
						     break;
                                                 }  
                                                 pre6 = pinet6;               
                                                 pinet6 = pninet6;
                                             }
                                              if(strcmp(dot1dTpFdbPort_Nodes.networkadd,pinet6->networkadd) == 0 && pinet6->next == NULL){
                                                     for ( pt = pSwitch->TrunkPort;*pt!=0;pt++) {
                                                         for(j = 0;j<=ifcount;j++) {
                                                             if(ifName_tbl[j].dot1dBasePort == dot1dTpFdbPort_Nodes.dot1dBasePort)
                                                                 break;
                                                         }

                                                         if (ifName_tbl[j].ifindex == *pt){
                                                             istrunk = 1;
                                                             break;
                                                         }else{
                                                             istrunk = 0;
                                                         }
                                                     }
                                                     if (!istrunk){
                                                         sprintf(_strvlan,"%d",*pVlan);
                                                         #ifdef DEBUGV6
                                                         printf("Vlan %d ",Sw3Vlan_v6_TBL[L3num][i].vlanid );
                                                         printf("%s ",pinet6->inet6add);
                                                         #endif
                                                         #ifdef SQL
                                                         
                                                         if(mysql_querys_insinet6[thread_index] == NULL) {
                                                             mysql_querys_insinet6[thread_index] = (struct sql_query *)malloc(sizeof(struct sql_query));
                                                             thismysql_querys_insinet6 = mysql_querys_insinet6[thread_index];
                                                             createsqlquery(thismysql_querys_insinet6,pSwitch , ifName_tbl,  j, _strvlan,NULL,pinet6 );
                                                            thismysql_querys_insinet6->nextquery = NULL;
                                                            premysql_querys_insinet6 = thismysql_querys_insinet6;
                                                         }else{

                                                             thismysql_querys_insinet6 = (struct sql_query *)malloc(sizeof(struct sql_query));
                                                             createsqlquery(thismysql_querys_insinet6,pSwitch , ifName_tbl,  j, _strvlan,NULL,pinet6);
                                                             thismysql_querys_insinet6->nextquery = premysql_querys_insinet6;
                                                                    premysql_querys_insinet6 = thismysql_querys_insinet6;
                                                                    mysql_querys_insinet6[thread_index] = thismysql_querys_insinet6;
                                                         }
                                                         #endif
                                                     }
                                              }
                                             break;
                                         }
                                     }
 /*Print v4 tbl*/
                                     for (i=0;i<=inetvlanindex;i++){
                                         if (Sw3Vlan_v4_TBL[L3num][i].vlanid == *pVlan ){
                                             pinet=Sw3Vlan_v4_TBL[L3num][i].inetlist;
                                             while (pinet->next!= NULL){
                                                 ppinet=pinet->next;
                                                 if(strcmp(dot1dTpFdbPort_Nodes.networkadd,pinet->networkadd) == 0) {
                                                     for ( pt = pSwitch->TrunkPort; *pt != 0; pt++) {
                                                         for(j = 0;j<=ifcount;j++) {
                                                             if(ifName_tbl[j].dot1dBasePort == dot1dTpFdbPort_Nodes.dot1dBasePort)
                                                                 break;
                                                         }
                                                         if (ifName_tbl[j].ifindex == *pt){
                                                             istrunk = 1;
                                                                break;
                                                         }else{
                                                             istrunk = 0;
                                                         }
                                                     }
                                                     if (!istrunk) {
                                                         sprintf(_strvlan,"%d",*pVlan);
                                                         #ifdef DEBUGV4
                                                         printf("%s ",pinet->inetadd);
                                                         printf("%s ", pinet->networkadd);
                                                         printf("%s ->",pSwitch->SwitchAdd);
                                                         for(j = 0;j<=ifcount;j++) {
                                                             if(ifName_tbl[j].dot1dBasePort == dot1dTpFdbPort_Nodes.dot1dBasePort)
                                                                 break;
                                                         }

                                                         printf("%s ",ifName_tbl[j].ifname);
                                                         printf("time = %d\n", time((time_t*)NULL));
                                                         #endif
                                                         #ifdef SQL
                                                         
                                                         if(mysql_querys_insinet[thread_index] == NULL) {
                                                             mysql_querys_insinet[thread_index] = (struct sql_query *)malloc(sizeof(struct sql_query));
                                                             thismysql_querys_insinet = mysql_querys_insinet[thread_index];
                                                             createsqlquery(thismysql_querys_insinet,pSwitch , ifName_tbl,  j, _strvlan,pinet, NULL);
                                                            thismysql_querys_insinet->nextquery = NULL;
                                                            premysql_querys_insinet = thismysql_querys_insinet;
                                                         }else{

                                                             thismysql_querys_insinet = (struct sql_query *)malloc(sizeof(struct sql_query));
                                                             createsqlquery(thismysql_querys_insinet,pSwitch , ifName_tbl,  j, _strvlan,pinet ,NULL);
                                                             thismysql_querys_insinet->nextquery = premysql_querys_insinet;
                                                                    premysql_querys_insinet = thismysql_querys_insinet;
                                                                    mysql_querys_insinet[thread_index] = thismysql_querys_insinet;
                                                         }
                                                         #endif

                                                     }
						 break;
                                                 }
                                                 pre = pinet;
                                                 pinet = ppinet;
                                             }
                                             if( pinet->next == NULL && strcmp(dot1dTpFdbPort_Nodes.networkadd,pinet->networkadd) == 0) {
                                                 for ( pt = pSwitch->TrunkPort;*pt!=0;pt++) {
                                                         for(j = 0;j<=ifcount;j++) {
                                                             if(ifName_tbl[j].dot1dBasePort == dot1dTpFdbPort_Nodes.dot1dBasePort)
                                                                 break;
                                                         }
                                                         if (ifName_tbl[j].ifindex == *pt){
                                                             istrunk = 1;
                                                                break;
                                                         }else{
                                                             istrunk = 0;
                                                         }
                                                 }
                                                 if (!istrunk) {
                                                     sprintf(_strvlan,"%d",*pVlan);
                                                     #ifdef DEBUGV4
                                                     printf("%s ",pinet->inetadd);
                                                     printf("%s ", pinet->networkadd);
                                                     printf("%s ->",pSwitch->SwitchAdd);
                                                         for(j = 0;j<=ifcount;j++) {
                                                             if(ifName_tbl[j].dot1dBasePort == dot1dTpFdbPort_Nodes.dot1dBasePort)
                                                                 break;
                                                         }
                                                          printf("%s ",ifName_tbl[j].ifname);
                                                     printf("time = %d\n", time((time_t*)NULL));
                                                     #endif
                                                         #ifdef SQL
                                                         
                                                         if(mysql_querys_insinet[thread_index] == NULL) {
                                                             mysql_querys_insinet[thread_index] = (struct sql_query *)malloc(sizeof(struct sql_query));
                                                             thismysql_querys_insinet = mysql_querys_insinet[thread_index];
                                                             createsqlquery(thismysql_querys_insinet,pSwitch , ifName_tbl,  j, _strvlan,pinet, NULL);
                                                            thismysql_querys_insinet->nextquery = NULL;
                                                            premysql_querys_insinet = thismysql_querys_insinet;
                                                         }else{

                                                             thismysql_querys_insinet = (struct sql_query *)malloc(sizeof(struct sql_query));
                                                             createsqlquery(thismysql_querys_insinet,pSwitch , ifName_tbl,  j, _strvlan,pinet ,NULL);
                                                             thismysql_querys_insinet->nextquery = premysql_querys_insinet;
                                                                    premysql_querys_insinet = thismysql_querys_insinet;
                                                                    mysql_querys_insinet[thread_index] = thismysql_querys_insinet;
                                                         }
                                                         #endif

                                                 }
                                             }
                                             /*add for no ip mac*/
                                             if( pinet->next == NULL && strcmp(dot1dTpFdbPort_Nodes.networkadd,pinet->networkadd) != 0){
                                                 for ( pt = pSwitch->TrunkPort;*pt!=0;pt++) {
                                                         for(j = 0;j<=ifcount;j++) {
                                                             if(ifName_tbl[j].dot1dBasePort == dot1dTpFdbPort_Nodes.dot1dBasePort)
                                                                 break;
                                                         }
                                                         if (ifName_tbl[j].ifindex == *pt){
                                                             istrunk = 1;
                                                                break;
                                                         }else{
                                                             istrunk = 0;
                                                         }
                                                 }
                                                 if (!istrunk) {
                                                     sprintf(_strvlan,"%d",*pVlan);
                                                   /*
                                                     printf("no ip %s",dot1dTpFdbPort_Nodes.networkadd); 
							printf("%s ->",pSwitch->SwitchAdd);
                                                         for(j = 0;j<=ifcount;j++) {
                                                             if(ifName_tbl[j].dot1dBasePort == dot1dTpFdbPort_Nodes.dot1dBasePort)
                                                                 break;
                                                         }
                                                          printf("%s ",ifName_tbl[j].ifname);
                                                     printf("time = %d\n", time((time_t*)NULL));
						*/	
                                                         #ifdef SQL
                                                         if(mysql_querys_insinet[thread_index] == NULL) {
                                                             mysql_querys_insinet[thread_index] = (struct sql_query *)malloc(sizeof(struct sql_query));
                                                             thismysql_querys_insinet = mysql_querys_insinet[thread_index];
                                                             createsqlquery(thismysql_querys_insinet,pSwitch , ifName_tbl,  j, _strvlan,NULL, NULL,dot1dTpFdbPort_Nodes.networkadd);
                                                            thismysql_querys_insinet->nextquery = NULL;
                                                            premysql_querys_insinet = thismysql_querys_insinet;
                                                         }else{

                                                             thismysql_querys_insinet = (struct sql_query *)malloc(sizeof(struct sql_query));
                                                             createsqlquery(thismysql_querys_insinet,pSwitch , ifName_tbl,  j, _strvlan ,NULL,NULL,dot1dTpFdbPort_Nodes.networkadd);
                                                             thismysql_querys_insinet->nextquery = premysql_querys_insinet;
                                                                    premysql_querys_insinet = thismysql_querys_insinet;
                                                                    mysql_querys_insinet[thread_index] = thismysql_querys_insinet;
                                                         }
                                                         #endif

                                                 }
                                             }
                                             break;
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
                         fprintf(stderr, "thTimeout: No Response from %s\n",
                                 session.peername);
		 	 sprintf(linuxtime,"%d",time((time_t*)NULL));
			 DeadHost_num++;
			 strcat(strcat(strcat(strcat(strcpy(deadhost[DeadHost_num].query,"update swip set status = 0 ,DeadTime = FROM_UNIXTIME("),linuxtime),") where ipadd = '"),session.peername),"' and status != 0");
			 //strcat(strcat(strcat(strcat(strcpy(deadhost[DeadHost_num].query,"update swip set status = 0 ,DeadTime = FROM_UNIXTIME("),linuxtime),") where ipadd = '"),session.peername),"'");
                         running = 0;
                         exitval = 1;
                     } else {                /* status == STAT_ERROR */
                         snmp_sess_perror("snmpwalk", ss);
                         running = 0;
                         exitval = 1;
                     }
                     if (response){
                         snmp_free_pdu(response);
                         response = NULL;
                     }
                 }
                 /*
                 if (oidrunnum == 0 && status == STAT_SUCCESS) {

        *
         * no printed successful results, which may mean we were
         * pointed at an only existing instance.  Attempt a GET, just
         * for get measure.
         *
                     if (!netsnmp_ds_get_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_WALK_DONT_GET_REQUESTED)) {
                         snmp_get_and_print(ss, op->root, op->rootlen);
                     }
                 }
                 */
               
                 if( strcmp(op->OidName,IF_MIB_ifName) == 0 || strcmp(op->OidName,ifAlias)==0){
                   //  memset(commatvlan,0,sizeof(commatvlan));
                      snmp_sess_close(sessp);
                      free((void *)(ss->community));
                     break;
                 }
                 memset(commatvlan,0,sizeof(commatvlan));
                  
                 if (response){
                     snmp_free_pdu(response);
                     response = NULL;
                 }
                 snmp_sess_close(sessp);
              //   if(strcmp(pSwitch->vender ,"3com")==0)
              //       break; 
            }/*Next vlan*/
         }/*Next oid*/
        memset(ifName_tbl,0,500*sizeof(struct ifName));
        ifcount = 0;
     }/*Next Host*/
pthread_exit( (void *)0 ); 
 return (void *)0;
}

