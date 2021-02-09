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

00000A64 2020-08-21 12:47:14.081 14950 15470 "[10.194.172.33:9876{FLJbbdhPG2RVPReuignSK6}[local:FLJbbdhuXZy7rooXzNCU82]] COMPLETE: CurrentCarrierServices.service_CLUEMPO FLJbbdhPG2RVPReuignSK6[local: FLJbbdhuXZy7rooXzNCU82] from 10.194.172.245 complete in 923 msecs memory=109 Mb priority=-2 slavesreply=29815 resultsize=22665 continue=0 WhenFirstRow=2020-08-21T16:47:13.162Z TimeElapsed=18.617s TimeTotalExecute=16.078s NumIndexSeeks=640 NumIndexScans=114 NumIndexWildSeeks=380 NumLeafCacheHits=348 NumNodeCacheHits=174 NumLeafCacheAdds=4 NumNodeCacheAdds=18 NumIndexAccepted=118 NumIndexRowsRead=87 TimeSoapcall=517.146ms TimeFirstExecute=15.033s TimeSortElapsed=18.766us NumGroups=293 NumGroupMax=1052 TimeLocalExecute=1.323s NumAllocations=4227446 fCleanLNBO={ NumStarts=23 NumStops=23 TimeLocalExecute=34.686ms } fCanadaAddress109={ } fDMetaphone1={ NumStarts=43 NumStops=43 TimeLocalExecute=90.248us } fEditDistance={ NumStarts=24 NumStops=24 TimeLocalExecute=5.662us } fgetGlobalId={ NumStarts=6 NumStops=6 TimeLocalExecute=7.521us } fgetLocalId={ NumStarts=6 NumStops=6 TimeLocalExecute=2.332us } fgetCallerId={ NumStarts=6 NumStops=6 TimeLocalExecute=1.400us }"

0000F4C9 2021-02-07 13:02:09.861 26616 76578 "Maximum queries active 13 of 100 for pool 9876"

00000001 2020-07-14 05:06:05.588 75653 75653 "Roxie starting, build = internal_6.4.28-1"

TODO:
    collect active query names
    add Soapcall time
    add fCleanLNBO time
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

    int in_range = 0;
    int in_shadow_range = 0;

    if (use_time)
    {
        in_range = 0;
        in_shadow_range = 0;
        if ((strcmp(srt_time, "0") == 0) && (strcmp(end_time, "0") == 0))
            use_time = 0;
        else if (strcmp(srt_time, "0") == 0)
            strcpy(srt_time, "00:00:00");
        else if (strcmp(end_time, "0") == 0)
            strcpy(end_time, "23:59:59");
    }

    if (!use_time)
    {
        in_range = 1;
        in_shadow_range = 1;
    }

    // use shadown start time 1+ min earlier to track active queries
    // match up pid tid QUERY:/BLIND: with pid tid COMPLETE:

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
    time_t shadow_stime = 0;

    std::vector<int> tarray;
    double tsum = 0.0;
    int mint = INT_MAX;
    int maxt = 0;

#if 0
    char save_line[42000] = { "" };
#endif

    std::list<std::string> qlist;

    int roxie_start = 0;

    while(fgets(line, 42001, fp) != NULL)
    {
        line[42000] = '\0';

        char id[5001] = { "" };
        char dat[5001] = { "" };
        char tim[5001] = { "" };
        char pid[5001] = { "" };
        char tid[5001] = { "" };
        char i1[5001] = { "" };
        char i2[5001] = { "" };
        char i3[5001] = { "" };
        char i4[5001] = { "" };
        char i5[5001] = { "" };
        char i6[5001] = { "" };
        char i7[5001] = { "" };
        char i8[5001] = { "" };

        int srtn = sscanf(line, "%s%s%s%s%s%s%s%s%s%s%s%s%s", id, dat, tim, pid, tid, i1, i2, i3, i4, i5, i6, i7, i8);
        if (srtn >= 7)
        {
            if ( (strcmp(i1, "\"Roxie") == 0) && (strcmp(i2, "starting,") == 0) && (strcmp(i3, "build") == 0) )
            {
                // printf("Roxie starting, resetting old count of %lu to 0\n", qlist.size());

                roxie_start = 1;

                qlist.clear();

                char otim[5000] = { "" };
                strcpy(otim, tim);
                struct tm tma;
                memset(&tma, 0, sizeof(struct tm));
                char date_time[5300] = { "" };
                char *hr = strtok(otim,":");
                char *min = strtok(NULL,":");
                char *sec = strtok(NULL,".");
                // char *msc = strtok(NULL," ");
                sprintf(date_time, "%s %s:%s:%s", dat, hr, min, sec);
                strptime(date_time, "%Y-%m-%d %H:%M:%S", &tma);
                shadow_stime = mktime(&tma);
                in_shadow_range = 1;
            }

#if 0
            if ( (srtn == 13) && (in_shadow_range) )
            {
                // printf("i1 = <%s>\n", i1);
     
                if (strcmp(i1,"\"Maximum") == 0)
                {
                    if (strcmp(i2, "queries") == 0)
                    {
                        if (strcmp(i3, "active") == 0)
                        {
                            strcpy(save_line, line);
                            continue;
                        }
                    }
                }
            }
#endif

            if (in_shadow_range)
            {
                if ( (strcmp(i2, "QUERY:") == 0) || (strcmp(i2, "BLIND:") == 0) )
                {
                    // printf("%s %s start\n", pid, tid);
                    std::string key;
                    key.append(pid);
                    key.append(":");
                    key.append(tid);
                    qlist.push_back(key);
                }
            }

            char *p = strstr(line, " complete in ");
            if (p != NULL)
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

                    if (!in_shadow_range)
                        shadow_stime = stime - 60;
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
                    // char *msc = strtok(NULL," ");
                    sprintf(date_time, "%s %s:%s:%s", dat, hr, min, sec);
                    strptime(date_time, "%Y-%m-%d %H:%M:%S", &tma);
                    qtime = mktime(&tma);

                    if (qtime < shadow_stime) 
                        continue;
                    if (qtime > etime)
                        break;
                    in_shadow_range = 1;
                    if (qtime >= stime) 
                        in_range = 1;
                }

                std::string key;
                key.append(pid);
                key.append(":");
                key.append(tid);
                auto res = std::find(qlist.begin(), qlist.end(), key);
                if (res != qlist.end())
                {
                    // printf("found %s %s in qlist\n", pid, tid);
                    qlist.erase(res);
                }

#if 0
                if (save_line[0] != '\0')
                {
                    printf("%s", save_line);
                    printf("qlist size is: %lu\n", qlist.size());
                    save_line[0] = '\0';
                }
#endif

                if (!in_range)
                    continue;

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
                                printf("%s %s %s  %9d  %s   %s\n", id, dat, tim, msecs, b3, qn);
                                printf("active queries: %lu %c\n", qlist.size(), (roxie_start ? ' ' : '*'));
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
