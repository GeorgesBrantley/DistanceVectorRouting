#include "ni.h"
#include "router.h"
#include <string.h>
/* Thanks to Prof. Rao for this file. */

/* The following two global variables are defined and used only in routingtable.c for convenience. */

struct route_entry routingTable [MAX_ROUTERS];
/* This is the routing table that contains all the route entries. It is initialized by InitRoutingTbl() and updated
   by UpdateRoutes().*/

int NumRoutes;
/*  This variable denotes the number of routes in the routing table. 
    It is initialized by InitRoutingTbl() on receiving the INIT_RESPONSE and updated by UpdateRoutes() 
    when the routingTable changes.*/


/**
 * This function initializes the routing table with the neighbor information received from the network initializer.
 * It adds a self-route (route to the same router) in the routing table.
 */
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID)
{
    //get info from init reponse
    unsigned int num_neigh = InitResponse->no_nbr;
    struct nbr_cost *neigh = InitResponse->nbrcost;

    //init variables
    NumRoutes = 0;
    
    //add itselft to routerTable
    struct route_entry base;
    //set ID as the argument
    base.dest_id = myID;
    base.next_hop = myID;
    base.cost = 0;

    routingTable[NumRoutes] = base;
    NumRoutes++;

    //iterate through new neighbors list
    int x = 0;
    for (x = 0; x < num_neigh; ++x) {
        struct route_entry newer;
        newer.dest_id = (neigh+x)->nbr;
        //set next hope as destination
        newer.next_hop = newer.dest_id;
        newer.cost = (neigh+x)->cost;
        
        routingTable[NumRoutes] = newer;
        NumRoutes++;
    } // end of for loop

} /* InitRoutingTbl */

/**
 * This function is invoked on receiving route update message from a neighbor. 
 * It installs in the routing table new routes that are previously unknown. 
 * For known routes, it finds the shortest path and updates the routing table if necessary. 
 * It also implements the forced update and split horizon rules.
 * It returns 1 if the routing table changed and 0 otherwise.
 */
int UpdateRoutes (struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID)
{
    //Next hop and dest relation
    //Destination is the final point you are attempting to reach
    //Cost is the total amount of 'time points' it takes to get there
    //Next hop is the instruction to reach the next point. Then you call that point's instructions
    //Split horizon: Never tell a node that you have a path through it to a dest.
    //A->B->C, A doesn't tell B that it has a path to C if it can only go through B
    int flag = 0;    

    //Break down the pkt_RT_UPDATE object
    unsigned int newRoutes = RecvdUpdatePacket->no_routes;
    struct route_entry *newRoutesList = RecvdUpdatePacket->route; 

    int x = 0;
    //get new routes, no questions asked?
    for (x = 0; x < newRoutes; ++x) {
        //smallFlag is for seeing if the route exists
        int smallF = 1;
        int y = 0;
        for (y = 0; y < NumRoutes; ++y) {
            if ((newRoutesList+x)->dest_id == routingTable[y].dest_id) {
               //route exists, stop checking, set it so route isn't added
               smallF = 0;
               //Check for the best route to destination
               if ((newRoutesList+x)->cost + costToNbr < routingTable[y].cost &&
                    (newRoutesList+x)->next_hop != myID && 
                        (( (newRoutesList+x)->next_hop != RecvdUpdatePacket->sender_id)
                        ||( (newRoutesList+x)->next_hop != (newRoutesList+x)->dest_id ))){
                    //check for split horizon (self pointing)
                    //new route is faster! update
                    flag++;
                    routingTable[y].cost = (newRoutesList+x)->cost + costToNbr ;
                    routingTable[y].next_hop = RecvdUpdatePacket->sender_id;
               }else if (routingTable[y].next_hop == RecvdUpdatePacket->sender_id
               && routingTable[y].cost < (newRoutesList+x)->cost + costToNbr) {
                    //forced update
                    //A longer time came in on a used path
                    flag++; 
                    //update cost
                    routingTable[y].cost = (newRoutesList+x)->cost + costToNbr; 
               }
               break;
            }
        }
        //If route destination is NEW
        if (smallF == 1 && (newRoutesList+x)->next_hop != myID) {
            //check for split horizon
            flag++;
            struct route_entry newer;
            newer.dest_id = (newRoutesList+x)->dest_id;
            //change next_hop to sendernode 
            newer.next_hop = RecvdUpdatePacket->sender_id;
            //update cost with cost to sendernode
            newer.cost = (newRoutesList+x)->cost + costToNbr;
            routingTable[NumRoutes] = newer;
            NumRoutes++;
        }

    }
    //returned 0 on no change
    if (flag == 0) {
        return 0;
    }else {
        return 1;
    }
} /* UpdateRoutes */

/**
 * This function is invoked on timer expiry to advertise the routes to neighbors. 
 * It fills the routing table information into a struct pkt_RT_UPDATE, a pointer to which 
 * is passed as an input argument.
 */
void ConvertTabletoPkt (struct pkt_RT_UPDATE *UpdatePacketToSend, int myID)
{
    int x;
    //update packet with info
    UpdatePacketToSend->sender_id = myID;
    UpdatePacketToSend->no_routes = NumRoutes;

    //send everything in routing table
    for (x =0; x < NumRoutes; ++x) {
       UpdatePacketToSend->route[x].dest_id = routingTable[x].dest_id; 
       UpdatePacketToSend->route[x].next_hop = routingTable[x].next_hop;
       UpdatePacketToSend->route[x].cost = routingTable[x].cost; 
    }
} /* ConvertTabletoPkt */

/**
 * This function is invoked whenever the routing table changes.
 * It prints the current routing table information to a log file, a pointer to which 
 * is passed as an input argument. The handout gives the file format.
 */
void PrintRoutes (FILE* Logfile, int myID)
{
    char* LogFileName = calloc(100, sizeof(char));
    sprintf(LogFileName, "router%d.log", myID);
    FILE *L = fopen(LogFileName,"a");
    int x = 0;
    char txt[1000];

    //write to file
    for (x = 0; x < NumRoutes; ++x) {
        char tmp[1000];
        snprintf(tmp,sizeof(tmp), "R%d -> R%d: R%d, %d\n", myID,routingTable[x].dest_id, routingTable[x].next_hop, routingTable[x].cost); 
        strcat(txt,tmp);
    }
    strcat(txt,"\n\n");
    fputs(txt,L); 
    fflush(L);

    fclose(L);
} /* PrintRoutes */

/**
 * This function is invoked on detecting an inactive link to a neighbor (dead nbr). 
 * It checks all the routes that use the dead nbr as the next hop and changes the cost to INFINITY.
 */
void UninstallRoutesOnNbrDeath (int DeadNbr)
{
    //iterate through the array
    int x = 0;
    for (x = 0; x < NumRoutes; ++x) {
        if (DeadNbr == routingTable[x].next_hop){
            //change cost to dead link to INFINITY
            routingTable[x].cost = INFINITY;
        }
    }
} /* UninstallRoutesOnNbrDeath */


 
