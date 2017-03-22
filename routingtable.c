#include "ni.h"
#include "router.h"

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
   return 0;
} /* UpdateRoutes */

/**
 * This function is invoked on timer expiry to advertise the routes to neighbors. 
 * It fills the routing table information into a struct pkt_RT_UPDATE, a pointer to which 
 * is passed as an input argument.
 */
void ConvertTabletoPkt (struct pkt_RT_UPDATE *UpdatePacketToSend, int myID)
{
} /* ConvertTabletoPkt */

/**
 * This function is invoked whenever the routing table changes.
 * It prints the current routing table information to a log file, a pointer to which 
 * is passed as an input argument. The handout gives the file format.
 */
void PrintRoutes (FILE* Logfile, int myID)
{
} /* PrintRoutes */

/**
 * This function is invoked on detecting an inactive link to a neighbor (dead nbr). 
 * It checks all the routes that use the dead nbr as the next hop and changes the cost to INFINITY.
 */
void UninstallRoutesOnNbrDeath (int DeadNbr)
{
} /* UninstallRoutesOnNbrDeath */


 
