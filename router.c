//GEORGES BRANTLEY
#include "ni.h"
#include "router.h"
#include <time.h>

/* Thanks to Prof. Rao for this file. */

int main (int argc, char **argv)
{
    if (argc != 5){
      printf("Usage: ./router <router id> <ni hostname> <ni UDP port> <router UDP port>\n");
      return EXIT_FAILURE;
    }
    
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
    niport = atoi(argv[3]);
    routerport = atoi(argv[4]);

    /* Create and bind the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        printf("Failed to create socket.\n\n");
    bzero((char *) &routeraddr, sizeof(routeraddr));
    routeraddr.sin_port = htons(routerport);
    routeraddr.sin_family = AF_INET;
    routeraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd, (struct sockaddr *)&routeraddr, sizeof(routeraddr));

    /* Code to send INIT_REQUEST goes here */

    /* Code to receive and handle INIT_RESPONSE goes here */
    
    /* Code to initialize the routing table goes here */

    /* Create and clear the LogFile */
    LogFileName = calloc(100, sizeof(char));
    sprintf(LogFileName, "router%d.log", router_id);
    LogFile = fopen(LogFileName, "w");
    fclose(LogFile);

    /* Main Loop: */
    /* On receiving a RT_UPDATE packet, UpdateRoutes is called to modify the routing table according to the protocol.  
       If the UPDATE_INTERVAL timer expires, first ConvertTabletoPkt is called and a RT_UPDATE is sent to all the neighbors. 
       Second, we check to see if any neighbor has not sent a RT_UPDATE for FAILURE_DETECTION seconds, i.e., (3*UPDATE_INTERVAL). 
       All such neighbors are marked as dead, and UninstallRoutesOnNbrDeath is called to check for routes with the dead nbr 
       as next hop and change their costs to INFINITY. */

    while (1)
    {
	FD_ZERO(&rfds);
	FD_SET(sockfd, &rfds);
	select(sockfd+1, &rfds, NULL, NULL, &timeout);
		
        /* Router has received a packet. Update the routing table and inform
         * the neighbors.
         */
        if (FD_ISSET(sockfd, &rfds))
	    {

	    } /* FD_ISSET */

        /* There was no packet received but a timeout has occurred so send an update 
         * to all neighbors
         */
        else
        {
            
        } /* else */
    } /* while */
    
    close(sockfd);
    return 0;
} /* main */


