#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <stdlib.h>
#include <libconfig.h>
#include <unistd.h> 
#include <signal.h> 
#include <sys/param.h> 
#include <stdarg.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <mysql/mysql.h>
#include "usertracking.h"
#define NETSNMP_DISABLE_MIB_LOADING
#define MAXIFINDEX					30
#define DEBUG 1
extern struct Sw_Node *Sw2_Nodes[MAXL3][MAXSWITCHPREL3];
extern Sw3Vlan_v6 	    Sw3Vlan_v6_TBL[MAXL3][MAXVLANPRESWITCH];
extern Sw3Vlan_v4      Sw3Vlan_v4_TBL[MAXL3][MAXVLANPRESWITCH];
extern struct netstruct netstructs[MAXL3];
extern struct config_struct set;
void readswitch(config_t * conf, struct Sw_Node *Sw_Nodes[]){
    struct Sw_Node *Sw_Node, *p;
    int i;
    int j;
    int k;
    int isrouter = -1;
    int count_switch ;
    int count_vlan;
    int count_trunk;
    char Strswitch[] = "switchs.switch";
    char Strswitchadd[35] = {0};
    const char *switchadd = NULL;
    char Strswitchtype[35] = {0}; 
    long int switchtype;
    char Strcommunity[37] = {0};
    const char *community = NULL;
    char Strvender[35] = {0};
    const char *vender;
    char Strvlan[35] = {0};
    char  Strtrunkport[35] = {0};
    char tempi[4] = {0};
    config_setting_t *Cswitch = config_lookup(conf, Strswitch);
    config_setting_t *Cvlan;
    config_setting_t  *Ctrunkport;
    count_switch = config_setting_length(Cswitch);
    for (i = 0; i < count_switch; i++) {
       // config_lookup_string(conf,)
        sprintf(tempi,"%d",i);
       strcat(strcat(strcat(strcat(strcpy(Strswitchadd,Strswitch),".["),tempi),"]"),".switchadd");
       strcat(strcat(strcat(strcat(strcpy(Strswitchtype,Strswitch),".["),tempi),"]"),".switchtype");
       strcat(strcat(strcat(strcat(strcpy(Strcommunity,Strswitch),".["),tempi),"]"),".community");
       strcat(strcat(strcat(strcat(strcpy(Strvlan,Strswitch),".["),tempi),"]"),".vlanid");
       strcat(strcat(strcat(strcat(strcpy(Strtrunkport,Strswitch),".["),tempi),"]"),".trunkport");
       strcat(strcat(strcat(strcat(strcpy(Strvender,Strswitch),".["),tempi),"]"),".vender");
       config_lookup_int(conf,Strswitchtype,&switchtype);
       switch (switchtype) {
       case 3:
           isrouter++;
          Sw_Nodes[isrouter] = (struct Sw_Node*)malloc(sizeof(struct Sw_Node));
          if(Sw_Nodes[isrouter] == NULL) {
              /*
              print some
              */
              exit(1);
          }
           memset(Sw_Nodes[isrouter],0,sizeof(struct Sw_Node));
           Sw_Nodes[isrouter]->SwitchType = switchtype;
           config_lookup_string(conf, Strswitchadd,&switchadd);
           strcpy(Sw_Nodes[isrouter]->SwitchAdd, switchadd);
           config_lookup_string(conf,Strcommunity,&community);
           strcpy(Sw_Nodes[isrouter]->community,community);
           config_lookup_string(conf,Strvender,&vender);
           strcpy(Sw_Nodes[isrouter]->vender,vender);
           Cvlan = config_lookup ( conf,Strvlan );
            count_vlan = config_setting_length(Cvlan);
            Ctrunkport = config_lookup (conf, Strtrunkport);
            count_trunk = config_setting_length(Ctrunkport);
            for(j = 0; j < count_vlan; j++ ) {
                Sw_Nodes[isrouter]->Vlanid[j] = config_setting_get_int_elem(Cvlan,j);
            }
            for (k=0;k< count_trunk;k++) {
                Sw_Nodes[isrouter]->TrunkPort[k] = config_setting_get_int_elem(Ctrunkport,k);
            }   
            Sw_Nodes[isrouter]->next = NULL;
            break;
       case 2:
           if(Sw_Nodes[isrouter]->next == NULL) {
               Sw_Node = (struct Sw_Node*)malloc(sizeof(struct Sw_Node));
               if(Sw_Node != NULL) {
                   memset(Sw_Node,0,sizeof(struct Sw_Node));
                   Sw_Nodes[isrouter]->next = Sw_Node;
                   Sw_Node->SwitchType = switchtype;
                   config_lookup_string(conf, Strswitchadd,&switchadd);
                   strcpy(Sw_Node->SwitchAdd, switchadd);
                   config_lookup_string(conf,Strcommunity,&community);
                   strcpy(Sw_Node->community,community);
                   config_lookup_string(conf,Strvender,&vender);
                   strcpy(Sw_Node->vender,vender);
                   Cvlan = config_lookup ( conf,Strvlan );
                   count_vlan = config_setting_length(Cvlan);
                   Ctrunkport = config_lookup (conf, Strtrunkport);
                   count_trunk = config_setting_length(Ctrunkport);
                   for(j = 0; j < count_vlan; j++ ) {
                       Sw_Node->Vlanid[j] = config_setting_get_int_elem(Cvlan,j);
                   }
                   for (k=0;k< count_trunk;k++) {
                       Sw_Node->TrunkPort[k] = config_setting_get_int_elem(Ctrunkport,k);
                   }   
                   Sw_Node->next = NULL;
                   p = Sw_Node;
                   break;
               }else
                   /*print som*/;
           }else{
               Sw_Node = (struct Sw_Node*)malloc(sizeof(struct Sw_Node));
               if(Sw_Node != NULL) {
                   memset(Sw_Node,0,sizeof(struct Sw_Node));
                   p->next = Sw_Node;
                   Sw_Node->SwitchType = switchtype;
                   config_lookup_string(conf, Strswitchadd,&switchadd);
                   strcpy(Sw_Node->SwitchAdd, switchadd);
                   config_lookup_string(conf,Strcommunity,&community);
                   strcpy(Sw_Node->community,community);
                   config_lookup_string(conf,Strvender,&vender);
                   strcpy(Sw_Node->vender,vender);
                   Cvlan = config_lookup ( conf,Strvlan );
                   count_vlan = config_setting_length(Cvlan);
                   Ctrunkport = config_lookup (conf, Strtrunkport);
                   count_trunk = config_setting_length(Ctrunkport);
                   for(j = 0; j < count_vlan; j++ ) {
                       Sw_Node->Vlanid[j] = config_setting_get_int_elem(Cvlan,j);
                   }
                   for (k=0;k< count_trunk;k++) {
                       Sw_Node->TrunkPort[k] = config_setting_get_int_elem(Ctrunkport,k);
                   }   
                   Sw_Node->next = NULL;
                   p = Sw_Node;
               }else
                   /* print som*/;
           }
           break;
       }
        memset(Strvlan,0,sizeof(Strvlan));
        memset(Strtrunkport,0,sizeof(Strtrunkport));
        memset(Strswitchadd,0,sizeof(Strswitchadd));
        memset(Strswitchtype,0,sizeof(Strswitchtype));
        memset(Strcommunity,0,sizeof(Strcommunity));
        memset(Strvender,0,sizeof(Strvender));
    }

}

void
usage(void)
{
    fprintf(stderr, "USAGE: snmpwalk ");
    snmp_parse_args_usage(stderr);
    fprintf(stderr, " [OID]\n\n");
    snmp_parse_args_descriptions(stderr);
    fprintf(stderr,
            "  -C APPOPTS\t\tSet various application specific behaviours:\n");
    fprintf(stderr, "\t\t\t  p:  print the number of variables found\n");
    fprintf(stderr, "\t\t\t  i:  include given OID in the search range\n");
    fprintf(stderr, "\t\t\t  I:  don't include the given OID, even if no results are returned\n");
    fprintf(stderr,
            "\t\t\t  c:  do not check returned OIDs are increasing\n");
    fprintf(stderr,
            "\t\t\t  t:  Display wall-clock time to complete the request\n");
}
void initialize (struct oid oids[])
{
 //init_snmp("traseuser");
 //netsnmp_init_mib();
  struct oid *op = oids;
  while (op->OidName) {
        op->rootlen = sizeof(op->root)/sizeof(op->root[0]);
   if (!read_objid(op->OidName, op->root, &op->rootlen)) {
            snmp_perror("read_objid:");
            exit(1);
        }
    op++;
  }
}

void
snmp_get_and_print(netsnmp_session * ss, oid * theoid, size_t theoid_len)
{
    netsnmp_pdu    *pdu, *response;
    netsnmp_variable_list *vars;
    int             status;

    pdu = snmp_pdu_create(SNMP_MSG_GET);
    snmp_add_null_var(pdu, theoid, theoid_len);

    status = snmp_synch_response(ss, pdu, &response);
    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
        for (vars = response->variables; vars; vars = vars->next_variable) {
          //  numprinted++;
            print_variable(vars->name, vars->name_length, vars);
        }
    }
    if (response) {
        snmp_free_pdu(response);
    }
}

int
snprint_my_v6tbl( u_char *networkadd_buf, u_char *inet6add_buf, int *ifindex_buf, const netsnmp_variable_list * var)
{
    size_t i;
    size_t out_len = 0;
    int len = 0;
    if(  var->name[17] == 254 )/*don't print link local addresses*/
        return 0; 

     if (var->val_len != 0) {
        for (i = 0; i < var->val_len-2; i++) {
        snprintf(networkadd_buf+out_len,3, "%02X", var->val.string[i]);
        out_len+= 2;
        if (i < var->val_len - 3) {
            *(networkadd_buf+out_len) = ':';
            out_len++;
           }
         }
        *(networkadd_buf+out_len) = '\0';
	} else {
        return 0;
        //snprintf (networkadd_buf,1,0);
	}
    for (i = 17; i < 31; i++) {
        len = snprintf(inet6add_buf, 3, "%lx", var->name[i]);
        inet6add_buf += len;
        i++;
        len = snprintf(inet6add_buf, 4, "%02lx:", var->name[i]);
        inet6add_buf += len;
        }
        len = snprintf(inet6add_buf, 3, "%lx", var->name[31]);
		inet6add_buf += len;
        snprintf(inet6add_buf, 3, "%02lx", var->name[32]);
        *ifindex_buf = (int)var->name[14];
	return 1;
}

int
sprint_my_v6tbl(int *ifindex, int *vlanindex, int L3index, Sw3Vlan_v6  Sw3Vlan_v6_TBL[][30], const netsnmp_variable_list * var)
{
    Inet6List *n=NULL;
	u_char networkadd_buf[NETWORKADD_MAX_LEN];
	u_char inet6add_buf[INET6ADD_MAX_LEN];
	int ifindex_buf = 0;
	if ( snprint_my_v6tbl(networkadd_buf,inet6add_buf,&ifindex_buf,var) == 1 ){
            if (*ifindex != ifindex_buf){
            (*vlanindex)++;
            *ifindex = ifindex_buf;
            Sw3Vlan_v6_TBL[L3index][*vlanindex].vlanid = ifindex_buf;
            }
            if (Sw3Vlan_v6_TBL[L3index][*vlanindex].inet6list == NULL){
                Sw3Vlan_v6_TBL[L3index][*vlanindex].inet6list=(Inet6List *)malloc(sizeof(Inet6List));
                if(Sw3Vlan_v6_TBL[L3index][*vlanindex].inet6list != NULL) {
                    Sw3Vlan_v6_TBL[L3index][*vlanindex].inet6list->next=NULL;
                    memmove((u_char *)Sw3Vlan_v6_TBL[L3index][*vlanindex].inet6list  ->networkadd, (u_char *)networkadd_buf, NETWORKADD_MAX_LEN * sizeof(u_char));
                    memmove((u_char *)Sw3Vlan_v6_TBL[L3index][*vlanindex].inet6list->inet6add, (u_char *)inet6add_buf, INET6ADD_MAX_LEN * sizeof(u_char));
                }
                else
                 /*print som*/   ;
            }else if (Sw3Vlan_v6_TBL[L3index][*vlanindex].inet6list != NULL ){
                n=(Inet6List *)malloc(sizeof(Inet6List));
                if(n != NULL) {
                    n->next=(Sw3Vlan_v6_TBL[L3index][*vlanindex].inet6list);
                    Sw3Vlan_v6_TBL[L3index][*vlanindex].inet6list = n;
                    memmove((u_char *)Sw3Vlan_v6_TBL[L3index][*vlanindex].inet6list->networkadd, (u_char *)networkadd_buf, NETWORKADD_MAX_LEN * sizeof(u_char));
                    memmove((u_char *)Sw3Vlan_v6_TBL[L3index][*vlanindex].inet6list->inet6add, (u_char *)inet6add_buf, INET6ADD_MAX_LEN * sizeof(u_char));
                }else
                    /*print som*/;
            }
    }
    n = NULL;
    return 1;
}


int
snprint_my_v4tbl( u_char *networkadd_buf, u_char *inetadd_buf, int *ifindex_buf, const netsnmp_variable_list * var)
{
    size_t i;
    size_t out_len = 0;
    int len = 0;
     if(  var->name[11] == 127 )
         return 0;
     if (var->val_len != 0) {
        for (i = 0; i < var->val_len; i++){
                snprintf(networkadd_buf+out_len, 3 , "%02X" , var->val.string[i]);
                 out_len+= 2;
                if(i < var->val_len-1) {
                    *(networkadd_buf+out_len) = ':';
                    out_len ++;
                }
        }
        //}
        //*(networkadd_buf+out_len) = '\0';
	} else {
	snprintf (networkadd_buf,1,0);
	}
    for (i = 11; i <= 14; i++) {
        len = snprintf(inetadd_buf, 5, "%d.", var->name[i]);
        inetadd_buf += len;
    }
     *(inetadd_buf-1) = '\0';
        *ifindex_buf = (int)var->name[10];
	return 1;
}
int 
sprint_my_v4tbl( int *inetifindex, int *inetvlanindex, int L3index, Sw3Vlan_v4 Sw3Vlan_v4_TBL[][30], const netsnmp_variable_list * var)
{
    InetList *n=NULL;
	u_char networkadd_buf[NETWORKADD_MAX_LEN];
	u_char inetadd_buf[INET_MAX_LEN];
	int ifindex_buf = 0;
   if (snprint_my_v4tbl(networkadd_buf,inetadd_buf,&ifindex_buf,var)== 1){
       // printf("inetifindex %d",*inetifindex);
       // printf("ifindex_buf %d\n",ifindex_buf);
       if (*inetifindex != ifindex_buf){
        (*inetvlanindex)++;
        *inetifindex = ifindex_buf;
        Sw3Vlan_v4_TBL[L3index][*inetvlanindex].vlanid = ifindex_buf;
        }
   
    if (Sw3Vlan_v4_TBL[L3index][*inetvlanindex].inetlist == NULL){
        Sw3Vlan_v4_TBL[L3index][*inetvlanindex].inetlist=(InetList *)malloc(sizeof(InetList));
        if(Sw3Vlan_v4_TBL[L3index][*inetvlanindex].inetlist != NULL) {
            Sw3Vlan_v4_TBL[L3index][*inetvlanindex].inetlist->next=NULL;
            memmove((u_char *)Sw3Vlan_v4_TBL[L3index][*inetvlanindex].inetlist->networkadd, (u_char *)networkadd_buf, NETWORKADD_MAX_LEN * sizeof(u_char));
            memmove((u_char *)Sw3Vlan_v4_TBL[L3index][*inetvlanindex].inetlist->inetadd, (u_char *)inetadd_buf, INET_MAX_LEN * sizeof(u_char));
	    Sw3Vlan_v4_TBL[L3index][*inetvlanindex].inetcount++;
        }else
            /*print som*/;
    }else if (Sw3Vlan_v4_TBL[L3index][*inetvlanindex].inetlist != NULL ){
        n=(InetList *)malloc(sizeof(InetList));
        if(n != NULL) {
            n->next=Sw3Vlan_v4_TBL[L3index][*inetvlanindex].inetlist;
            Sw3Vlan_v4_TBL[L3index][*inetvlanindex].inetlist = n;
            memmove((u_char *)Sw3Vlan_v4_TBL[L3index][*inetvlanindex].inetlist->networkadd, (u_char *)networkadd_buf, NETWORKADD_MAX_LEN * sizeof(u_char));
            memmove((u_char *)Sw3Vlan_v4_TBL[L3index][*inetvlanindex].inetlist->inetadd, (u_char *)inetadd_buf, INET_MAX_LEN * sizeof(u_char));
	    Sw3Vlan_v4_TBL[L3index][*inetvlanindex].inetcount++;
        }else
            /*print som*/;
    }
/*add inetcount
   if (Sql_InetCount == NULL){
       Sql_InetCount = (struct sql_query *)malloc(sizeof(struct sql_query));
       this_Sql_InetCount = Sql_InetCount;
       strcat(strcat(strcpy(Sql_InetCount->query,"INSERT DELAYED into `IPCount` (`Router`,`Vlan`,`Count`,`Time`) values ('"),L3index),"'"),;
   }	
add inetcount*/
   }
    n = NULL;
    return 1;
}

void
snprint_ifName( int *ifindex_buf, u_char ifname_buf[], const netsnmp_variable_list * var)
{
    int len = 0;
     if (var->val_len != 0) {
       strncpy(ifname_buf,var->val.string,var->val_len);
	*ifindex_buf = var->name[var->name_length - 1];
        }
}


/*
void 
sprint_ifName(u_char  ifName_tbl[][20], const netsnmp_variable_list * var){
    int ifindex_buf = 0;
     if (var->val_len != 0) {
         ifindex_buf = var->name[var->name_length - 1];
         strncpy(ifName_tbl[ifindex_buf],var->val.string,var->val_len);
     }
}
*/
void 
sprint_ifName(struct ifName  ifName_tbl[],int *ifcount, const netsnmp_variable_list * var){
    int ifindex_buf = 0;
     if (var->val_len != 0) {
         ifindex_buf = var->name[var->name_length - 1];
         ifName_tbl[*ifcount].ifindex = ifindex_buf;
         strncpy(ifName_tbl[*ifcount].ifname,var->val.string,var->val_len);
         (*ifcount)++;
     }
}

void
sprint_ifAlias(struct ifName  ifName_tbl[],int *ifcount, const netsnmp_variable_list * var){
    int ifindex_buf = 0;
    u_char unit_buf[2]={0};
    u_char port_buf[3]={0};
     if (var->val_len != 0) {
         ifindex_buf = var->name[var->name_length - 1];
         ifName_tbl[*ifcount].ifindex = ifindex_buf;
         strncpy(unit_buf,var->val.string+21,1);
         strncpy(port_buf,var->val.string+10,2);
         strcat(strcat(strcat(strcat(ifName_tbl[*ifcount].ifname,"Unit:"),unit_buf)," Port:"),port_buf);
         (*ifcount)++;
     }
}
/*
void
sprint_ifVlan(struct ifName  ifVlan_tbl[],int *ifcount, const netsnmp_variable_list * var){
    int ifindex_buf = 0;
    u_char Vlan_buf[5]={0};
     if (var->val_len != 0) {
	if (strncmp("Vlan",var->val.string,4)==0){
           ifindex_buf = var->name[var->name_length - 1];
           ifVlan_tbl[*ifcount].ifindex = ifindex_buf;
           strncpy(Vlan_buf,var->val.string+14,3);
           strcat(ifVlan_tbl[*ifcount].ifname,Vlan_buf);
           (*ifcount)++;
	}
     }
}
*/

/*
void 
sprint_ifindex_vlanid_tbl(int *_vlanindex,ifindex_ifname  ifindex_vlanid_tbl[], const netsnmp_variable_list * var){
	int ifindex_buf=0;
	int n=0;
    int *pifindex_buf = &ifindex_buf;
	u_char ifname_buf[20]={0};
	snprint_ifName(pifindex_buf,ifname_buf,var);
	if (strncmp("Vl",ifname_buf,2)==0){
        ifindex_vlanid_tbl[*_vlanindex].ifindex = ifindex_buf;
        for(n=2;n<=strlen(ifname_buf);n++){
            ifindex_vlanid_tbl[*_vlanindex].ifname[n-2] = ifname_buf[n];
              }

        (*_vlanindex)++;
	  }

}

void
sprint_ifindex_vlanid_tbl(int ifindex_vlanid_tbl[], const netsnmp_variable_list * var){
          ifindex_vlanid_tbl[*var->val.integer] = var->name[14];
          
}
*/
void 
snprint_dot1dTpFdbPort(u_char *networkadd_buf, int *dot1baseport_buf,const netsnmp_variable_list * var){
  
    int i;
    int out_len = 0;
    for (i = 11;i<=16;i++) {
        snprintf(networkadd_buf + out_len,4, "%02X", var->name[i]);
        out_len+= 2;
        if (i < var->name_length-1 ) {
            *(networkadd_buf+out_len) = ':';
            out_len++;
        }
    }
    *(networkadd_buf+out_len) = '\0';
  //  printf("%d\n",var->name_length);
    *dot1baseport_buf = *var->val.integer;
}

void 
sprint_dot1dTpFdbPort(struct dot1dTpFdbPort_Node *dot1dTpFdbPort_Nodes, const netsnmp_variable_list * var){
    u_char   networkadd_buf[NETWORKADD_MAX_LEN];
    int     dot1baseport_buf;
    snprint_dot1dTpFdbPort(networkadd_buf, &dot1baseport_buf, var);
    strcpy(dot1dTpFdbPort_Nodes->networkadd, networkadd_buf);
    dot1dTpFdbPort_Nodes->dot1dBasePort = dot1baseport_buf;
     
}
/* add from E528*/
void
snprint_dot1qTpFdbPort(u_char *networkadd_buf, int *dot1baseport_buf,const netsnmp_variable_list * var){

    int i;
    int out_len = 0;
    for (i = 14;i<=19;i++) {
        snprintf(networkadd_buf + out_len,4, "%02X", var->name[i]);
        out_len+= 2;
        if (i < var->name_length-1 ) {
            *(networkadd_buf+out_len) = ':';
            out_len++;
        }
    }
    *(networkadd_buf+out_len) = '\0';
//    printf("%d\n",var->name_length);
        *dot1baseport_buf = *var->val.integer;
        }
        
void
sprint_dot1qTpFdbPort(struct dot1dTpFdbPort_Node *dot1dTpFdbPort_Nodes, const netsnmp_variable_list * var){
    u_char   networkadd_buf[NETWORKADD_MAX_LEN];
    int     dot1baseport_buf;
    snprint_dot1qTpFdbPort(networkadd_buf, &dot1baseport_buf, var);
    strcpy(dot1dTpFdbPort_Nodes->networkadd, networkadd_buf);
    dot1dTpFdbPort_Nodes->dot1dBasePort = dot1baseport_buf;

}





/*
void 
snprint_dot1dBasePortIfIndex(int *dot1baseport_buf, int *ifindex_buf, const netsnmp_variable_list * var){
    *dot1baseport_buf = var->name[11];
    *ifindex_buf = *var->val.integer;
}
*/
/*
void  
sprint_dot1dBasePortIfIndex(int dot1dBasePortIfIndex[], const netsnmp_variable_list * var){
    dot1dBasePortIfIndex[var->name[11]] =  *var->val.integer;

}
   */
void  
sprint_dot1dBasePortIfIndex(struct ifName ifName_tbl[], int ifcount, const netsnmp_variable_list * var){
    int i;
     for(i = 0; i<=ifcount;i++) {
         if(ifName_tbl[i].ifindex == *var->val.integer) {
             ifName_tbl[i].dot1dBasePort = var->name[11];
             break;
         }
     }

}

void  
sprint_IfIndex_dot1dBasePort(struct ifName ifName_tbl[], int ifcount, const netsnmp_variable_list * var){
    int i;
     for(i = 0; i<=ifcount;i++) {
         if(ifName_tbl[i].dot1dBasePort == *var->val.integer) {
             ifName_tbl[i].ifindex = var->name[11];
             break;
         }
     }

}

 void freeany( int vlanindex, int inetvlanindex,struct Sw_Node **Sw_Nodes,struct arg *args[],int routerindex){
     int i,j;
     Inet6List *pinet6=NULL;
     Inet6List *pninet6=NULL;
     InetList *pinet=NULL;
     InetList *pninet=NULL;
     struct Sw_Node   **pSwitch = NULL;
     struct Sw_Node   *pL2 = NULL;
     struct Sw_Node   *pnL2 = NULL;
     for(i = 0; i < netstructs[0].L3count; i++) {
         for (j=0; j< netstructs[i].vlancount;j++){
             pinet6=Sw3Vlan_v6_TBL[i][j].inet6list;
             while (pinet6->next!= NULL){
                 pninet6=pinet6->next;
                 free(pinet6);
                 pinet6=pninet6;
             }
         //    if(pinet6->next == NULL) 
                 free(pinet6);
                 pinet6 = NULL;
         }
    
         for (j=0; j< inetvlanindex;j++){
             pinet=Sw3Vlan_v4_TBL[i][j].inetlist;
             while (pinet->next!= NULL){
                 pninet=pinet->next;
                 free(pinet);
                 pinet=pninet;
             }
        //     if(pinet->next == NULL) 
                 free(pinet);
                 pinet = NULL;
         }
     }
         for( pSwitch=Sw_Nodes; *pSwitch; pSwitch++){
             pL2 = *pSwitch;
             while( pL2->next != NULL ) {
                 pnL2 = pL2->next;
                 free(pL2);
                 pL2 = pnL2;
             }
             free(pL2);
             pL2 = NULL;
             Sw_Nodes = NULL;
             *pSwitch = NULL;
         }
     
        for(i=0;i<routerindex;i++) {
        free(args[i]);
        }
                                          
} 
/*
int read_usertracking_config(char *file) {
	FILE *fp;
	char buff[BUFSIZE];
	char *buffer;
	char p1[BUFSIZE];
	char p2[BUFSIZE];

	if ((fp = fopen(file, "rb")) == NULL) {
		if (set.log_level == POLLER_VERBOSITY_DEBUG) {
			if (!set.stderr_notty) {
				fprintf(stderr, "ERROR: Could not open config file [%s]\n", file);
			}
		}
		return -1;
	}else{
		if (!set.stdout_notty) {
			fprintf(stdout, "SPINE: Using spine config file [%s]\n", file);
		}

		while(!feof(fp)) {
			buffer = fgets(buff, BUFSIZE, fp);
			if (!feof(fp) && *buff != '#' && *buff != ' ' && *buff != '\n') {
				sscanf(buff, "%15s %255s", p1, p2);

				if (STRIMATCH(p1, "DB_Host"))          STRNCOPY(set.dbhost, p2);
				else if (STRIMATCH(p1, "DB_Database")) STRNCOPY(set.dbdb, p2);
				else if (STRIMATCH(p1, "DB_User"))     STRNCOPY(set.dbuser, p2);
				else if (STRIMATCH(p1, "DB_Pass"))     STRNCOPY(set.dbpass, p2);
				else if (STRIMATCH(p1, "DB_Port"))     set.dbport = atoi(p2);
				else if (!set.stderr_notty) {
					fprintf(stderr,"WARNING: Unrecongized directive: %s=%s in %s\n", p1, p2, file);
				}

				*p1 = '\0';
				*p2 = '\0';
			}
		}

		if (strlen(set.dbpass) == 0) *set.dbpass = '\0';

		return 0;
	}
}

*/
void init_daemon(void) 
{ 
    int pid; 
    int i; 
    if(pid=fork()) 
        exit(0);
    else if(pid< 0) 
        exit(1);
    setsid();
    if(pid=fork()) 
        exit(0);
    else if(pid< 0) 
        exit(1);
for(i=0;i< 65535;++i)
    close(i);
chdir("/tmp");
umask(0);
    return; 
} 

int db_insert(MYSQL *mysql, const char query[]) {
	int    error;
	int    error_count = 0;
	char   query_frag[BUFSIZE];

	/* save a fragment just in case */
	//snprintf(query_frag, BUFSIZE, "%s", query);

	while(1) {
	
			if (mysql_query(mysql, query)) {
				error = mysql_errno(mysql);

				if ((error == 1213) || (error == 1205)) {
					usleep(50000);
					error_count++;

					if (error_count > 30) {
						printf("ERROR: Too many Lock/Deadlock errors occurred!, SQL Fragment:'%s'", query);
						return 0;
					}

					continue;
				}else{
					printf("ERROR: SQL Failed! Error:'%i', Message:'%s', SQL Fragment:'%s'", error, mysql_error(mysql), query);
					return 0;
				}
			}else{
				return 1;
			}

	}
}


void db_connect(const char *database, MYSQL *mysql) {
	int    tries;
	int    timeout;
	int    options_error;
	int    success;
	char   *hostname;
	char   *socket = NULL;
	struct stat socket_stat;

	if ((hostname = strdup(set.dbhost)) == NULL) {
		printf("FATAL: malloc(): strdup() failed");
	}

	/* see if the hostname variable is a file reference.  If so,
	 * and if it is a socket file, setup mysql to use it.
	 */
	if (stat(hostname, &socket_stat) == 0) {
		if (socket_stat.st_mode & S_IFSOCK) {
			socket = strdup (set.dbhost);
			hostname = NULL;
		}
	}else if ((socket = strstr(hostname,":"))) {
		*socket++ = 0x0;
	}

	/* initialalize variables */
	tries   = 5;
	success = 0;
	timeout = 5;
   //my_init();
	mysql_init(mysql);
	if (mysql == NULL) {
		printf("FATAL: MySQL unable to allocate memory and therefore can not connect");
	}

	options_error = mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, (char *)&timeout);
	if (options_error < 0) {
		printf("FATAL: MySQL options unable to set timeout value");
	}

	while (tries > 0) {
		tries--;

		if (!mysql_real_connect(mysql, hostname, set.dbuser, set.dbpass, database, set.dbport, socket, 0)) {
			if ((mysql_errno(mysql) != 1049) && (mysql_errno(mysql) != 2005) && (mysql_errno(mysql) != 1045)) {
				printf("MYSQL: Connection Failed: Error:'%u', Message:'%s'\n", mysql_errno(mysql), mysql_error(mysql));

				success = 0;

				#ifndef SOLAR_THREAD
				usleep(2000);
				#endif
			}else{
				tries   = 0;
				success = 0;
			}
		}else{
			tries   = 0;
			success = 1;
		}
	}

	free(hostname);

	if (!success){
		printf("FATAL: Connection Failed, Error:'%i', Message:'%s'", mysql_errno(mysql), mysql_error(mysql));
	}
}

/*! \fn void db_disconnect(MYSQL *mysql)
 *  \brief closes connection to MySQL database
 *  \param mysql the database connection object
 *
 */

void db_disconnect(MYSQL *mysql) {
	mysql_close(mysql);
}

u_char* createsqlquery(struct sql_query* thismysql_querys,struct Sw_Node   *pSwitch , struct ifName ifName_tbl[], int j, u_char strvlan[],InetList *pinet, Inet6List *pinet6 ,...){
    va_list arg_ptr;
    u_char linuxtime[11]; 
    u_char l3address[INET6ADD_MAX_LEN] = {0};
    u_char sql_query_ins[150] = {0};
    u_char networkadd[NETWORKADD_MAX_LEN] = {0};
    u_char ifindex[6];
//    printf("debug pinet %d  pinet6 %d vlan:%s\n",pinet,pinet6,strvlan);
    if(pinet == NULL && pinet6 != NULL){
        strcpy(l3address,pinet6->inet6add);
        strcpy(networkadd,pinet6->networkadd);
       strcpy(sql_query_ins,"INSERT DELAYED into `UserInfo_6` (`SwAddr`,`IfName`,`IfIndex`,`Vlan`,`MACAddr`,`UserAddr6`,`Time`) values ('");
    } else if(pinet != NULL && pinet6 == NULL){
        strcpy(l3address,pinet->inetadd);
        strcpy(networkadd,pinet->networkadd);
       strcpy(sql_query_ins,"INSERT DELAYED into `UserInfo_4` (`SwAddr`,`IfName`,`IfIndex`,`Vlan`,`MACAddr`,`UserAddr`,`Time`) values ('");
    }/*add from no layeraddress*/
      else if(pinet == NULL && pinet6 == NULL){
        strcpy(l3address,"0.0.0.0");
      //  printf("%s\n",l3address);
        va_start(arg_ptr,pinet6);
        //printf("va_arg(arg_ptr,u_char*) %s\n",va_arg(arg_ptr,u_char*));
        strcpy(networkadd,va_arg(arg_ptr,u_char*));
       va_end(arg_ptr);
       // printf("function %s\n",networkadd);
	strcpy(sql_query_ins,"INSERT DELAYED into `UserInfo_4` (`SwAddr`,`IfName`,`IfIndex`,`Vlan`,`MACaddr`,`UserAddr`,`Time`) values ('");
    }
        sprintf(linuxtime,"%d",time((time_t*)NULL));
    snprintf(ifindex,sizeof(ifindex),"%d",ifName_tbl[j].ifindex);
    
    strcat(
        strcat(
            strcat(
                strcat(
                    strcat(
                        strcat(
                            strcat(
                                strcat(
				    strcat(
				        strcat(
                                            strcat(
                                                strcat(
                                                    strcat(
                                                        strcat(
                              strcpy(thismysql_querys->query,sql_query_ins)
                                                        ,pSwitch->SwitchAdd)
                                                    ,"','")
                                                ,ifName_tbl[j].ifname)
                                            ,"','")
				         ,ifindex)
				     ,"','")
                                ,strvlan)
                            ,"','")
                        ,networkadd)
                    ,"','")
                ,l3address)
            ,"',FROM_UNIXTIME(")
        ,linuxtime)
    ,"))");
    return thismysql_querys->query;
}
