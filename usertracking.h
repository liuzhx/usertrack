#include <mysql/mysql.h>
#ifndef USERTRACKING_H
#define USERTRACKING_H
#define INET6ADD_MAX_LEN        40
#define NETWORKADD_MAX_LEN   18
#define INET_MAX_LEN                16
#define MAX_HOST                      3000
#define VLANID_MAX_LEN             4
#define LAYER3                            3
#define LAYER2                            2
#define MAXL3 30
#define MAXVLANPRESWITCH 30
#define MAXSWITCHPREL3  300
#define STRNCOPY(dst, src)	strncopy((dst), (src), sizeof(dst)
#define STRMATCH(a,b)	(strcmp((a),(b)) == 0)
#define STRIMATCH(a,b)	(strcasecmp((a),(b)) == 0)
#define SMALL_BUFSIZE 256
#define MEDIUM_BUFSIZE 512
#define BUFSIZE 1024
#define MAXTHREAD_PREROUTER 20
#define MAXROUTER 50
    typedef unsigned char u_char;
typedef struct Inet6Node{
        u_char inet6add[INET6ADD_MAX_LEN];
        u_char networkadd[NETWORKADD_MAX_LEN];
        struct Inet6Node *next;
}Inet6TBL,Inet6List;

typedef struct Sw3Vlan_v6_Node{
 //   u_char Sw3add[18];
            int vlanid;
            Inet6List *inet6list;
              }Sw3Vlan_v6;

typedef struct InetNode{
        u_char inetadd[INET_MAX_LEN];
        u_char networkadd[NETWORKADD_MAX_LEN];
        struct InetNode *next;
}InetTBL,InetList;

typedef struct Sw3Vlan_v4_Node{
 //   u_char Sw3add[18];
            int vlanid;
            InetList *inetlist;
	    int inetcount;//add at 20111117
//	u_char Sql_inetcount[50];
              }Sw3Vlan_v4;


typedef struct Ifindex_Ifname{
        int ifindex;
        u_char ifname[20];
}ifindex_ifname;

struct oid {
  const u_char *OidName;
  oid root[MAX_OID_LEN];
  size_t rootlen;
};
struct Sw_Node{
    int SwitchType;
    int     Vlanid[20] ;
    int TrunkPort[128];
    u_char community[10];
    u_char vender[15];
    u_char SwitchAdd[INET_MAX_LEN];
    struct Sw_Node *next;
};

/*
struct Sw3_Node{
 //   u_char SwitchType;
    u_char SwitchAdd[INET_MAX_LEN];
    struct Sw2_Node *Sw2_Nodes;
};
*/
struct dot1dTpFdbPort_Node{
    u_char SwitchAdd[INET_MAX_LEN];
    int     Vlanid[20] ;
    u_char networkadd[NETWORKADD_MAX_LEN];
    int dot1dBasePort;
};

struct ifName{
    int ifindex;
    int dot1dBasePort;
    u_char ifname[30];
};

struct ifAlias{
    int ifindex;
    int dot1dBasePort;
    u_char ifAlias[30];
};
struct arg{
 //   Sw3Vlan_v6 *Sw3Vlan_v6_TBL;
 //   Sw3Vlan_v4 *Sw3Vlan_v4_TBL;
    int min_pre_thread;
    int max_pre_thread;
    int vlanindex;
    int inetvlanindex;
    int L3num;
    int thread_index;
};
struct netstruct{
   int L3count;
   int switchcount;
   int inetvlancount;
   int vlancount;
};
struct sql_query{
    u_char query[280];//change from 230 to 280 at 20110701
    struct sql_query *nextquery;
};
struct config_struct {
	/* database connection information */
	char   dbhost[SMALL_BUFSIZE];
	char   dbdb[SMALL_BUFSIZE];
	char   dbuser[SMALL_BUFSIZE];
	char   dbpass[SMALL_BUFSIZE];
	unsigned int dbport;
} config_db;
//extern config_db set;
void usage(void);
void initialize (struct oid oids[]);
void snmp_get_and_print(netsnmp_session * ss, oid * theoid, size_t theoid_len);
int sprint_my_v6tbl(int *ifindex, int *vlanindex,  int L3index, Sw3Vlan_v6  Sw3Vlan_v6_TBL[][30], const netsnmp_variable_list * var);
int sprint_my_v4tbl( int *inetifindex, int *inetvlanindex,  int L3index,  Sw3Vlan_v4  Sw3Vlan_v4_TBL[][30], const netsnmp_variable_list * var);
void sprint_ifName(struct ifName  ifName_tbl[],int *ifcount, const netsnmp_variable_list * var);
void sprint_dot1dBasePortIfIndex(struct ifName ifName_tbl[], int ifcount, const netsnmp_variable_list * var);
//void sprint_ifVlan(struct ifName  ifVlan_tbl[],int *ifcount, const netsnmp_variable_list * var);
//sprint_ifindex_vlanid_tbl(int ifindex_vlanid_tbl[], const netsnmp_variable_list * var);
//void  sprint_dot1dBasePortIfIndex(int dot1dBasePortIfIndex[], const netsnmp_variable_list * var);
void *scanl2(void *arg);
 void freeany( int , int ,struct Sw_Node **,struct arg *arg[],int routerindex);
 void init_daemon(void);
void db_connect(const char *database, MYSQL *mysql);
void db_disconnect(MYSQL *mysql);
int db_insert(MYSQL *mysql, const char query[]);
u_char* createsqlquery(struct sql_query* ,struct Sw_Node  * , struct ifName [], int , u_char [],InetList *, Inet6List *,... );

#endif
