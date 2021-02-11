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

1 - log#
2 - date
3 - time
4 - pid
5 - tid
6 - "query id
7 - QUERY:
8 - input src ip:port
9 - either a dash (-) or query tx id
10 - if 9 is a dash then <query name> else SoapRequest or ???
11 - if 10 is SoapRequest then <query-name

    1         2         3          4     5               6                   7         8             9            <10>
00042168 2020-07-31 09:58:25.700 42675 68625 "[10.173.104.100:9876{52742}] QUERY: 172.16.70.153:9876 -  <Inquiry_Services.Log_Service><address>1717 BEND VIEW LN</address><lastname>STALLARD</lastname><function_name>QSentCISSearch</function_name><ssn>409491698</ssn><city>SEVIERVILLE</city><company_id>1387725</company_id><state>TN</state><transaction_type>I</transaction_type></Inquiry_Services.Log_Service>"

    1         2         3          4     5                      6                                       7           8                            9                   10                    <11
00065B7D 2020-07-14 14:12:53.311 16033 47352 "[10.173.104.100:9876{esp_172.16.70.155_2115711R426920}] QUERY: 172.16.70.155:9876 esp_172.16.70.155_2115711R426920 SoapRequest <Address.AddressCleaning_Batch_Service uid='esp_172.16.70.155_2115711R426920'><DOBMa

000CCABD 2021-02-09 17:21:14.921 20947 47243 "[10.173.105.21:9876{198943}] QUERY: 10.176.71.1:9876 - SoapRequest <Models.FraudAdvisor_Service><otherapplicationidentifier3/><otherapplicationidentifier/><otherapplicationidentifier2/><dlnumber/><additionalwatchlists>false</additionalwatchlists><namesuffix/>

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
    int print_list = 1;
    int cycles = 2500; // guess from prod systems
    int num_wilds = 0;

    while ((c = getopt(argc, argv, "c:l:t:q:s:e:hx0")) >= 0) {
        switch (c) {
            case 'c': cycles = atoi(optarg); break;
            case 'l': strcpy(logfile, optarg); break;
            case 't': thres = atoi(optarg); break;
            case 'q': has_qname = 1; strcpy(qname, optarg); break;
            case 's': use_time = 1; strcpy(srt_time, optarg); break;
            case 'e': use_time = 1; strcpy(end_time, optarg); break;
            case 'x': summary = 1; break;
            case '0': print_list = 0; break;
            case 'h': help = 1; break;
        }
    }

    if ( (argc < 3) || (help) )
    {
        printf("Error, need -l logfile [-t msec threshold] [-q query name] [-s start-time (HH:MM:SS)] [-e end-time (HH:MM:SS)] [-x (summary)] [-0 skip active list] [-c clock Mhz (2500)]\n");
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

    std::unordered_map<std::string, std::string> qmap;

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

        if (srtn >= 8)
        {
            if ( (strcmp(i1, "\"Roxie") == 0) && (strcmp(i2, "starting,") == 0) && (strcmp(i3, "build") == 0) )
            {
                // printf("Roxie starting, resetting old count of %lu to 0\n", qmap.size());

                roxie_start = 1;

                qmap.clear();

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
        }

        if (srtn >= 11)
        {

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

                    // printf("%s", line);
                    // printf("i4 = <%s>\n", i4);
                    // printf("i5 = <%s>\n", i5);
                    // printf("i6 = <%s>\n", i6);

                    // if 9 (i4) is - then 10 is SoapRequest and qname is <11> or qname is <10>, else if 10 is SoapRequest, qname is <11 ...

                    char sqn[5001] = { "" };
                    if (strcmp(i4, "-") == 0)
                    {
                        if (strcmp(i5, "SoapRequest") == 0)
                        {
                            for(int i=0;i<(int)strlen(i6);i++)
                            {
                                if (i6[i] == '>')
                                {
                                    i6[i] = '\0';
                                    break;
                                }
                            }
                            strcpy(sqn, &i6[1]);
                            // printf("sqn0 = %s\n", sqn);
                        }
                        else
                        {
                            for(int i=0;i<(int)strlen(i5);i++)
                            {
                                if (i5[i] == '>')
                                {
                                    i5[i] = '\0';
                                    break;
                                }
                            }
                            strcpy(sqn, &i5[1]);
                            // printf("sqn1 = %s\n", sqn);
                        }
                    }
                    else if (strcmp(i5, "SoapRequest") == 0)
                    {
                        for(int i=0;i<(int)strlen(i6);i++)
                        {
                            if (i6[i] == '>')
                            {
                                i6[i] = '\0';
                                break;
                            }
                        }
                        strcpy(sqn, &i6[1]);
                        // printf("sqn2 = %s\n", sqn);
                    }

                    auto m1 = std::pair<std::string, std::string>(key, sqn);
                    qmap.insert(m1);
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

                auto res = qmap.find(key);
                if (res != qmap.end())
                {
                    // printf("found %s %s in qmap\n", pid, tid);
                    qmap.erase(res);
                }

#if 0
                if (save_line[0] != '\0')
                {
                    printf("%s", save_line);
                    printf("qmap size is: %lu\n", qmap.size());
                    save_line[0] = '\0';
                }
#endif

                if (!in_range)
                    continue;

                char *nws = strstr(line,"NumIndexWildSeeks=");
                if (nws)
                {
                    char nws_str[5001] = { "" };
                    strncpy(nws_str, nws, 61);
                    nws_str[60] = '\0';
                    char *p1 = strtok(nws_str, "=");
                    if (p1)
                    {
                        char *p2 = strtok(NULL, " ");
                        if (p2)
                        {
                            num_wilds = atoi(p2);
                        }
                    }
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

                                char *tsc = NULL;
                                char ts0[101] = "0ms";
                                char *tsp = strstr(line, "TimeSoapcall=");
                                if (tsp != NULL)
                                {
                                    char tsline[101] = { "" };
                                    strncpy(tsline, tsp, 100);
                                    tsline[100] = '\0';
                                    tsc = strtok(tsline, " ");
                                    if (tsc)
                                    {
                                        char *px1 = strtok(tsc, "=");
                                        if (px1)
                                        {
                                            tsc = strtok(NULL, " ");
                                            if (tsc)
                                                strcpy(ts0, tsc);
                                        }
                                    }
                                }

                                char *tac = NULL;
                                char ac0[101] = "0";
                                char *acp = strstr(line, "fCleanLNBO=");
                                if (acp != NULL)
                                {
                                    char acline[101] = { "" };
                                    strncpy(acline, acp, 100);
                                    acline[100] = '\0';
                                    char *px1 = strtok(acline, "{");
                                    if (px1)
                                    {
                                        char *px2 = strtok(NULL, "}");
                                        if (px2)
                                        {
                                            char *px3 = strtok(px2, " ");
                                            if (px3)
                                            {
                                                char *px4 = strtok(NULL, " ");
                                                if (px4)
                                                {
                                                    char *px5 = strtok(NULL, " ");
                                                    if (px5)
                                                    {
                                                        char *px6 = strtok(px5, "=");
                                                        if (px6)
                                                        {
                                                            tac = strtok(NULL, " ");
                                                            if (tac)
                                                            {
                                                                strcpy(ac0, tac);
                                                                // TODO: if no units (pure integer) then convert to approx time ...
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                // printf("%s", line);

                                int l = (int)strlen(qn);
                                if (l > 40)
                                    qn[40] = '\0';

                                char ac1[1001] = { "" };
                                char *tp = strstr(ac0, "s");
                                if (!tp)
                                    tp = strstr(ac0, "ms");
                                if (!tp)
                                    tp = strstr(ac0, "us");
                                if (!tp)
                                    tp = strstr(ac0, "ns");
                                if (!tp)
                                {
                                    unsigned long ac_cycles = strtoul(ac0, NULL, 0);
                                    if (ac_cycles != ULONG_MAX)
                                    {
                                        ac_cycles /= ((unsigned long)cycles * 1000UL);
                                        sprintf(ac1, "%lums", ac_cycles);
                                    }
                                    else
                                        strcpy(ac1, ac0);
                                }
                                else
                                        strcpy(ac1, ac0);

                                printf("%s %s %s %8d  %s   %-40s  %9d  %2lu %c sct=%-10s  act=%s\n",
                                        id, dat, tim, msecs, b3, qn, num_wilds, qmap.size(), (roxie_start ? ' ' : '*'), ts0, ac1);

                                if (print_list)
                                {
                                    int i = 1;
                                    auto iter1 = qmap.begin();
                                    while (iter1 != qmap.end())
                                    {
                                        printf("    %3d  %s\n", i++, iter1->second.c_str());
                                        iter1++;
                                    }
                                }
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
