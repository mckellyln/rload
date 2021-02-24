#define _XOPEN_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <bits/stdc++.h>
#include <iostream>
#include <map>
#include <unordered_map>
#include <list>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

using namespace std; 

/*
  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode ref pointer drops
   19: 0566AD0A:D4F6 3966AD0A:232A 01 00000000:00000000 00:00000000 00000000   500        0 3698559958 2 ffff9cdccddb7800 0
  363: 0566AD0A:D64E 4866AD0A:2329 01 00000000:00000000 00:00000000 00000000   500        0 3698562194 2 ffff9cdcca9b8400 0
  600: 00000000:D73B 00000000:0000 07 00000000:00000000 00:00000000 00000000    29        0 493664694 2 ffff9c80fb3c6000 0
11198: 00000000:00A1 00000000:0000 07 00000000:00000000 00:00000000 00000000     0        0 1320627120 2 ffff9c7f874b0400 0
11902: 00000000:0361 00000000:0000 07 00000000:00000000 00:00000000 00000000     0        0 493716654 2 ffff9cd001b92c00 0
11951: 0100007F:0392 00000000:0000 07 00000000:00000000 00:00000000 00000000     0        0 493697172 2 ffff9cd406ea0400 0
19924: 00000000:22B7 00000000:0000 07 00000000:00000000 00:00000000 00000000   500        0 3697114761 2 ffff9c80fb86d800 37517
65397: 0566AD0A:D458 4E66AD0A:2328 01 00000000:00000000 00:00000000 00000000   500        0 3698562205 2 ffff9c811f016800 0

 2654: 3500007F:0035 00000000:0000 07 00000000:00000000 00:00000000 00000000   101        0 29239 2 0000000000000000 0
 2669: 00000000:0044 00000000:0000 07 00000000:00000000 00:00000000 00000000     0        0 35542 2 0000000000000000 0
 3232: 00000000:0277 00000000:0000 07 00000000:00000000 00:00000000 00000000     0        0 38047 2 0000000000000000 0
*/

unsigned long txthres = 20000;
unsigned long rxthres = 20000;

class Slot
{
public:
    std::string sl;
    std::string lip;
    unsigned long lport;
    std::string rip;
    unsigned long rport;
    unsigned long txm;
    unsigned long rxm;
    unsigned long drps;

    Slot(const char *_sl, const char *_lip, unsigned long _lport, const char *_rip, unsigned long _rport, unsigned long _txm, unsigned long _rxm, unsigned long _drps)
    {
        sl.append(_sl);
        lip.append(_lip);
        lport = _lport;
        rip.append(_rip);
        rport = _rport;
        txm = _txm;
        rxm = _rxm;
        drps = _drps;
    }
};

std::unordered_map<std::string, Slot> umap;

int getstats(int doprint)
{
    // FILE *fp = fopen("udp.file", "rb");
    FILE *fp = fopen("/proc/net/udp", "rb");
    if (fp == NULL)
    {
        printf("Error, unable to open /proc/net/udp file\n");
        return 1;
    }

    char line[50001] = { "" };

    if (fgets(line, 42001, fp) == NULL)
    {
        fclose(fp);
        return 1;
    }

    while(fgets(line, 42001, fp) != NULL)
    {
        char sl[5001] = { "" };
        char laddr[5001] = { "" };
        char raddr[5001] = { "" };
        char st[5001] = { "" };
        char txrx[5001] = { "" };
        char tr1[5001] = { "" };
        char retran[5001] = { "" };
        char ud[5001] = { "" };
        char to[5001] = { "" };
        char inde[5001] = { "" };
        char ref[5001] = { "" };
        char ptr1[5001] = { "" };
        char drop[5001] = { "" };

        char *p1 = NULL;
        char *p2 = NULL;

        // printf("%s", line);

        int srtn = sscanf(line, "%s%s%s%s%s%s%s%s%s%s%s%s%s", sl, laddr, raddr, st, txrx, tr1, retran, ud, to, inde, ref, ptr1, drop);

        if (srtn == 13)
        {
            int lsl = (int)strlen(sl);
            if (lsl > 0)
                sl[lsl-1] = '\0';

            // printf("sl=%s laddr=%s raddr=%s txrx=%s drp=%s\n", sl, laddr, raddr, txrx, drop);

            char las[5001] = { "" };
            char locip[5001] = { "" };
            char locport[5001] = { "" };

            strcpy(las, laddr);
            p1 = strtok(las, ":");
            if (p1)
            {
                strcpy(locip, p1);
                p2 = strtok(NULL, " ");
                if (p2)
                {
                    strcpy(locport, p2);
                }
            }

            unsigned long addr = strtoul(locip, NULL, 16);

            char lip[5001] = { "" };
            sprintf(lip, "%lu.%lu.%lu.%lu", (addr & 0x000000FF), (addr & 0x0000FF00) >> 8, (addr & 0x00FF0000) >> 16, (addr & 0xFF000000) >> 24);

            unsigned long lport = strtoul(locport, NULL, 16);

            // printf("lip = <%s> lport = %lu\n", lip, lport);

            // --------

            char ras[5001] = { "" };
            char remip[5001] = { "" };
            char remport[5001] = { "" };

            strcpy(ras, raddr);
            p1 = strtok(ras, ":");
            if (p1)
            {
                strcpy(remip, p1);
                p2 = strtok(NULL, " ");
                if (p2)
                {
                    strcpy(remport, p2);
                }
            }

            addr = strtoul(remip, NULL, 16);

            char rip[5001] = { "" };
            sprintf(rip, "%lu.%lu.%lu.%lu", (addr & 0x000000FF), (addr & 0x0000FF00) >> 8, (addr & 0x00FF0000) >> 16, (addr & 0xFF000000) >> 24);

            unsigned long rport = strtoul(remport, NULL, 16);

            // printf("rip = <%s> rport = %lu\n", rip, rport);

            // --------

            char txs[5001] = { "" };
            char txms[5001] = { "" };
            char rxms[5001] = { "" };

            strcpy(txs, txrx);
            p1 = strtok(txs, ":");
            if (p1)
            {
                strcpy(txms, p1);
                p2 = strtok(NULL, " ");
                if (p2)
                {
                    strcpy(rxms, p2);
                }
            }

            unsigned long txm = strtoul(txms, NULL, 16);

            unsigned long rxm = strtoul(rxms, NULL, 16);

            // printf("txm = %lu   rxm = %lu\n", txm, rxm);

            // --------

            unsigned long drps = strtoul(drop, NULL, 10);

            // --------

            std::string key;
            key.append(sl);
            key.append(":");
            key.append(lip);
            key.append(":");
            key.append(locport);
            key.append(":");
            key.append(rip);
            key.append(":");
            key.append(remport);

            unsigned long new_drps = drps;
            unsigned long prev_drps = 0;

            auto res = umap.find(key);
            if (res != umap.end())
            {
                prev_drps = res->second.drps;
                new_drps = drps - prev_drps;
                res->second.drps = drps;
            }
            else
            {
                Slot s1(sl, lip, lport, rip, rport, txm, rxm, drps);
                auto m1 = std::pair<std::string, Slot>(key, s1);
                umap.insert(m1);
            }

            if (doprint)
            {
                if ( (txm >= txthres) || (rxm >= rxthres) || (new_drps > 0) )
                {
                    time_t nowt = 0;
                    time(&nowt);
                    struct tm *tn = localtime(&nowt);
                    char dattim[2000] = { "2021-02-24" } ;
                    strftime(dattim, 1000, "%Y-%m-%d %H:%M:%S", tn);

                    printf("%s loc:%14s:%-6lu rem:%14s:%-6lu %10lu %10lu %10lu   (%lu)\n", dattim, lip, lport, rip, rport, txm, rxm, new_drps, prev_drps);
                }
            }

            // --------

        }

    }

    fclose(fp);

    return 0;
}

int main(int argc, char *argv[])
{
    int c = 0;
    int help = 0;
    int delta = 0;
    while ((c = getopt(argc, argv, "t:r:d:h")) >= 0) {
        switch (c) {
            case 't': txthres = atoi(optarg); break;
            case 'r': rxthres = atoi(optarg); break;
            case 'd': delta = atoi(optarg); break;
            case 'h': help = 1; break;
        }
    }

    if (help)
    {
        printf("optional args: [-d sec (0==oneshot)] [-t tx thres] [-r rx thres]\n");
        return 1;
    }

    int srtn = getstats(0);
    if (srtn)
    {
        return 0;
    }

    sleep(1);

    if (delta <= 0)
    {
        getstats(1);
        return 0;
    }

    while (1)
    {
        getstats(1);
        sleep(delta);
    }

    return 0;
}

