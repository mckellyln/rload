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
#include <bits/stdc++.h>
#include <vector>

using namespace std; 

/*

00000A64 2020-08-21 12:47:14.081 14950 15470 "[10.194.172.33:9876{FLJbbdhPG2RVPReuignSK6}[local:FLJbbdhuXZy7rooXzNCU82]] COMPLETE: CurrentCarrierServices.service_CLUEMPO FLJbbdhPG2RVPReuignSK6[local: FLJbbdhuXZy7rooXzNCU82] from 10.194.172.245 complete in 923 msecs memory=109 Mb priority=-2 slavesreply=29815 resultsize=22665 continue=0 WhenFirstRow=2020-08-21T16:47:13.162Z TimeElapsed=18.617s TimeTotalExecute=16.078s NumIndexSeeks=640 NumIndexScans=114 NumIndexWildSeeks=380 NumLeafCacheHits=348 NumNodeCacheHits=174 NumLeafCacheAdds=4 NumNodeCacheAdds=18 NumIndexAccepted=118 NumIndexRowsRead=87 TimeSoapcall=517.146ms TimeFirstExecute=15.033s TimeSortElapsed=18.766us NumGroups=293 NumGroupMax=1052 TimeLocalExecute=1.323s NumAllocations=4227446 fCleanLNBO={ NumStarts=23 NumStops=23 TimeLocalExecute=34.686ms } fCanadaAddress109={ } fDMetaphone1={ NumStarts=43 NumStops=43 TimeLocalExecute=90.248us } fEditDistance={ NumStarts=24 NumStops=24 TimeLocalExecute=5.662us } fgetGlobalId={ NumStarts=6 NumStops=6 TimeLocalExecute=7.521us } fgetLocalId={ NumStarts=6 NumStops=6 TimeLocalExecute=2.332us } fgetCallerId={ NumStarts=6 NumStops=6 TimeLocalExecute=1.400us }"

*/

int main(int argc, char *argv[])
{
    int has_qname = 0;
    char qname[256] = { "" };
    char logfile[256] = { "" };
    char srt_time[256] = { "0" };
    char end_time[256] = { "0" };
    int thres = 0;
    int use_time = 0; 
    int help = 0;
    int c = 0;
    int summary = 0;

    while ((c = getopt(argc, argv, "l:t:q:s:e:hx")) >= 0) {
        switch (c) {
            case 'l': strcpy(logfile, optarg); break;
            case 't': thres = atoi(optarg); break;
            case 'q': has_qname = 1; strcpy(qname, optarg); break;
            case 's': use_time = 1; strcpy(srt_time, optarg); break;
            case 'e': use_time = 1; strcpy(end_time, optarg); break;
            case 'x': summary = 1; break;
            case 'h': help = 1; break;
        }
    }

    if ( (argc < 3) || (help) )
    {
        printf("Error, need -l logfile [-t msec threshold] [-q query name] [-s start-time (HH:MM:SS)] [-e end-time (HH:MM:SS)] [-x (summary)]\n");
        return 1;
    }

    if (has_qname)
    {
        if (strcmp(qname, "*") == 0)
            has_qname = 0;
        else if (strcmp(qname, "-") == 0)
            has_qname = 0;
        else if (strcmp(qname, ".") == 0)
            has_qname = 0;
        else if ((int)strlen(qname) == 0)
            has_qname = 0;
    }

    if (use_time)
    {
        if ((strcmp(srt_time, "0") == 0) && (strcmp(end_time, "0") == 0))
            use_time = 0;
        else if (strcmp(srt_time, "0") == 0)
            strcpy(srt_time, "00:00:00");
        else if (strcmp(end_time, "0") == 0)
            strcpy(end_time, "23:59:59");
    }

    // ===========================================

    FILE *fp = fopen(logfile, "rb");
    if (fp == NULL)
    {
        printf("Error, unable to open <%s>\n", logfile);
        return 1;
    }

    char line[50001] = { "" };

    time_t stime = 0;
    time_t etime = 0;
    time_t qtime = 0;

    std::vector<int> tarray;
    double tsum = 0.0;
    int mint = INT_MAX;
    int maxt = 0;

    while(fgets(line, 42001, fp) != NULL)
    {
        line[42000] = '\0';

        char *p = strstr(line, " complete in ");
        if (p != NULL)
        {
            char id[5001] = { "" };
            char dat[5001] = { "" };
            char tim[5001] = { "" };
            char pid[5001] = { "" };
            char tid[5001] = { "" };
            char info[5001] = { "" };
            int srtn = sscanf(line, "%s%s%s%s%s%s", id, dat, tim, pid, tid, info);
            if (srtn == 6)
            {
                if (use_time == 1)
                {
                    use_time = 2;
                    struct tm tm0, tm1;
                    memset(&tm0, 0, sizeof(struct tm));
                    memset(&tm1, 0, sizeof(struct tm));
                    char date_time0[5300] = { "" };
                    char date_time1[5300] = { "" };
                    sprintf(date_time0, "%s %s", dat, srt_time);
                    strptime(date_time0, "%Y-%m-%d %H:%M:%S", &tm0);
                    stime = mktime(&tm0);
                    sprintf(date_time1, "%s %s", dat, end_time);
                    strptime(date_time1, "%Y-%m-%d %H:%M:%S", &tm1);
                    etime = mktime(&tm1);
                    if (etime <= stime)
                        etime = stime + 1;
                }

                char otim[5000] = { "" };
                strcpy(otim, tim);

                if (use_time == 2)
                {
                    struct tm tma;
                    memset(&tma, 0, sizeof(struct tm));
                    char date_time[5300] = { "" };
                    char *hr = strtok(otim,":");
                    char *min = strtok(NULL,":");
                    char *sec = strtok(NULL,".");
                    char *msc = strtok(NULL," ");
                    sprintf(date_time, "%s %s:%s:%s", dat, hr, min, sec);
                    strptime(date_time, "%Y-%m-%d %H:%M:%S", &tma);
                    qtime = mktime(&tma);

                    if ((qtime < stime) || (qtime > etime))
                        continue;
                }

                char b1[5001] = { "" };
                char b2[5001] = { "" };
                char b3[5001] = { "" };
                int msecs = 0;
                int srtn2 = sscanf(p, "%s%s%d%s", b1, b2, &msecs, b3);
                if (srtn2 == 4)
                {
                    if ( ((thres < 0) && (msecs <= -thres)) || ((thres >= 0) && (msecs >= thres)) )
                    {
                        char b4[5001] = { "" };
                        char b5[5001] = { "" };
                        char qn[5001] = { "" };
                        char *t = strstr(line, " COMPLETE: ");
                        int srtn3 = sscanf(t, "%s%s%s", b4, qn, b5);
                        if (srtn3 == 3)
                        {
                            if ( (!has_qname) || ((has_qname) && (strstr(qn,qname) != NULL)) )
                            {
                                tarray.push_back(msecs);
                                tsum += (double)msecs;
                                if (msecs < mint)
                                    mint = msecs;
                                if (msecs > maxt)
                                    maxt = msecs;
                                printf("%s  %s  %s  %9d  %s   %s\n", id, dat, tim, msecs, b3, qn);
                            }
                        }
                    }
                }
            }
        }

    }

    fclose(fp);

    if ( (summary) && (tarray.size() > 0) )
    {
        int avgt = (int)(tsum / (double)tarray.size());
        int k95 = (int)((double)tarray.size() * 0.95);
        sort(tarray.begin(), tarray.end());
        int &k95val = tarray[k95];
        printf("num: %-6lu   min: %-6d   max: %-6d   avg: %-6d   95%%: %-6d\n", tarray.size(), mint, maxt, avgt, k95val);
    }

    // dont print summary if 0 count as this makes cluster output more confusing ...

    return 0;
}
