//GEORGES BRANTLEY
#include "ni.h"
#include "router.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Thanks to Prof. Rao for this file. */

int main (int argc, char **argv)
{
    if (argc != 5){
        printf("Usage: ./router <router id> <ni hostname> <ni UDP port> <router UDP port>\n");
        return EXIT_FAILURE;
    }

    //DEAD keeps track of all timeouts per router 
    int DEAD[MAX_ROUTERS];
    //CONVERGE keeps track of when its converged
    int CONVERGED = CONVERGE_TIMEOUT;
    //TIME KEEPER (ESTIMATE)
    int TIME = 0;

    //initialized everything to -1
    int a;
    for (a=0; a<MAX_ROUTERS; ++a)
        DEAD[a] = -1;

    int router_id, niport, routerport, sockfd;
    char *nihostname, *LogFileName;
    struct sockaddr_in routeraddr;
    FILE *LogFile;
    fd_set rfds;
    struct timeval timeout;

    timeout.tv_sec =  UPDATE_INTERVAL;
    timeout.tv_usec = 0;

    /* Store command line arguments */
    router_id = atoi(argv[1]);
    nihostname = argv[2];

    //ni port is the destination
    niport = atoi(argv[3]);
    //routerport is what I should be listening on
    routerport = atoi(argv[4]);

    /* Create and bind the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        printf("Failed to create socket.\n\n");
    bzero((char *) &routeraddr, sizeof(routeraddr));
    routeraddr.sin_port = htons(niport);
    routeraddr.sin_family = AF_INET;
    routeraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //add ip
    struct hostent *hp = gethostbyname(nihostname);
    memcpy((void*)&routeraddr.sin_addr,hp->h_addr_list[0], hp->h_length);

    bind(sockfd, (struct sockaddr *)&routeraddr, sizeof(routeraddr));

    //size thingy
    socklen_t addrlen =sizeof(routeraddr);
    //routerID is router_id

    //SEND (INIT REQUEST)
    struct pkt_INIT_REQUEST sendinfo;
    sendinfo.router_id = htonl(router_id);
    sendto(sockfd,(struct pkt_INIT_REQUEST*)&sendinfo,sizeof(sendinfo),0,
            (struct sockaddr *)&routeraddr,addrlen);

    struct pkt_INIT_RESPONSE *resp;
    char buffer[1000];
    //CATCH (INIT RESPONSE)
    recvfrom(sockfd,buffer, sizeof(struct pkt_INIT_RESPONSE),0,
            (struct sockaddr*)&routeraddr, &addrlen); 
    resp = (struct pkt_INIT_RESPONSE*)&buffer;
    ntoh_pkt_INIT_RESPONSE(resp);
    //printf("NEIGHBOORS: %d\n",resp->no_nbr);

    //GET NEIGHBOORS IN A LIST
    int NEIGHS[MAX_ROUTERS];
    struct nbr_cost *neigh;
    neigh = resp->nbrcost;
    int numNeighs = resp->no_nbr;
    int dd = 0;
    for (dd=0; dd < numNeighs;++dd){
        NEIGHS[dd] = (neigh+dd)->nbr; 
    }
    /* Code to initialize the routing table goes here */
    InitRoutingTbl(resp,router_id); 

    /* Create and clear the LogFile */
    LogFileName = calloc(100, sizeof(char));
    sprintf(LogFileName, "router%d.log", router_id);
    LogFile = fopen(LogFileName, "w");
    fclose(LogFile);
    PrintRoutes(LogFile,router_id);

    /* Main Loop: */
    /* On receiving a RT_UPDATE packet, UpdateRoutes is called to modify the routing table according to the protocol.  
       If the UPDATE_INTERVAL timer expires, first ConvertTabletoPkt is called and a RT_UPDATE is sent to all the neighbors. 
       Second, we check to see if any neighbor has not sent a RT_UPDATE for FAILURE_DETECTION seconds, i.e., (3*UPDATE_INTERVAL). 
       All such neighbors are marked as dead, and UninstallRoutesOnNbrDeath is called to check for routes with the dead nbr 
       as next hop and change their costs to INFINITY. */
    while (1)
    {
        TIME+= UPDATE_INTERVAL;
        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);
        select(sockfd+1, &rfds, NULL, NULL, &timeout);

        /* Router has received a packet. Update the routing table and inform
         * the neighbors.
         */
        if (FD_ISSET(sockfd, &rfds))
        {
            //GET RT_UPDATE
            struct pkt_RT_UPDATE *update;
            bzero(buffer,1000);
            recvfrom(sockfd,buffer, sizeof(struct pkt_RT_UPDATE),0
                    ,(struct sockaddr*)&routeraddr,&addrlen);
            update = (struct pkt_RT_UPDATE*)&buffer;

            //translate
            ntoh_pkt_RT_UPDATE(update);
            //get cost
            int cost = 0;
            struct route_entry *r = update->route;
            int x = 0;
            int dop = update->no_routes;
            for (x = 0;x < dop; ++x) {
                if ((r+x)->dest_id == router_id) {
                    cost = (r+x)->cost;
                    break;
                }
            }
            //UPDATE DEAD, set SENDER_ID to FAILURE_DETECTION (MAX)
            DEAD[update->sender_id] = FAILURE_DETECTION;

            //printf("UPDATE RECIEVED, from %d\n",update->sender_id);
            //Update table
            int i = UpdateRoutes(update,cost,router_id);
            //Call UpdateRoutes, if change, send updates to neighboors
            if (i > 0) {
                //PRINT TABLES
                PrintRoutes(LogFile,router_id);
                CONVERGED = CONVERGE_TIMEOUT +1;
                //printf("UPDATED TABLE\n");
                //USE SEND TO
                //change to routing tables
                //update neighboors
            } // end of 'if table changed' 
        } /* FD_ISSET */

        /* There was no packet received but a timeout has occurred so send an update 
         * to all neighbors
         */
        else {
            //update
            //printf("TIMEOUT %d\n",(int)timeout.tv_sec);
            //SEND UPDATE VALUES to all neighbors!
            //update timeout value
            timeout.tv_sec = UPDATE_INTERVAL;
            timeout.tv_usec = 0;
            //GO through dead, subtrack 1 from each positive number
            int flag = 0;
            int x;
            for (x=0; x<MAX_ROUTERS;++x){
                //if positive (filled)
                if (DEAD[x] > 0) {
                    //subtract 1,  
                    DEAD[x]--;
                    //check if its 0, if 0, router has gone rogue
                    if (DEAD[x] == 0) {
                        //Call UninstallRoutesOnNbrDeath with the id of dead link
                        //printf("KILLED %d\n",x);
                        UninstallRoutesOnNbrDeath(x);
                        PrintRoutes(LogFile,router_id);
                        CONVERGED = CONVERGE_TIMEOUT+1;
                        //Set to -1, ignore
                        DEAD[x] = -1;
                        flag++;
                    }
                }
            }
            //Use CovnertTabletoPkt to update neighboors
            //send struct
            //not a pointer, cause not changing global
            struct pkt_RT_UPDATE sender; 
            ConvertTabletoPkt(&sender,router_id);
            //loop through and find  neighboors 
            x = 0;
            for (x = 0; x < numNeighs; ++x) {
                //printf("Sending Update to Neighbors %d/%d\n",NEIGHS[x],numNeighs);
                //neighboor!
                //send update    
                sender.dest_id = NEIGHS[x];
                hton_pkt_RT_UPDATE(&sender);
                //send it out!     
                sendto(sockfd,(struct pkt_RT_UPDATE*)&sender,sizeof(sender),0,
                        (struct sockaddr *)&routeraddr, sizeof(routeraddr));
                ntoh_pkt_RT_UPDATE(&sender);
            }
        } /* else */
        //CHECK FOR CONVERGED
        if (CONVERGED >= 0)
            CONVERGED--;
        if (CONVERGED == 0) {
            //print 
            char* LogFileName = calloc(100, sizeof(char));
            sprintf(LogFileName, "router%d.log", router_id);
            FILE *L = fopen(LogFileName,"a");
            char txt[1000];
            snprintf(txt,sizeof(txt),"%d:Converged\n\n",TIME);
            fputs(txt,L);
            fflush(L);
            fclose(L);
        }
        //slow things down
        sleep(UPDATE_INTERVAL);

    } /* while */
    close(sockfd);
    return 0;
} /* main */


