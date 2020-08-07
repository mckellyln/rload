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
#include <iostream>
#include <map>
#include <unordered_map>
#include <list>
#include <string>
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
10 - if 9 is a dash then query name else SoapRequest or ???

    1         2         3          4     5               6                   7         8             9            <10>
00042168 2020-07-31 09:58:25.700 42675 68625 "[10.173.104.100:9876{52742}] QUERY: 172.16.70.153:9876 -  <Inquiry_Services.Log_Service><address>1717 BEND VIEW LN</address><lastname>STALLARD</lastname><function_name>QSentCISSearch</function_name><ssn>409491698</ssn><city>SEVIERVILLE</city><company_id>1387725</company_id><state>TN</state><transaction_type>I</transaction_type></Inquiry_Services.Log_Service>"

1 - log#
2 - date
3 - time
4 - pid
5 - tid
6 - "query id
7 - COMPLETE:
8 - query name
9 - either a dash (-) or query tx id
10 - from

    1         2         3          4     5               6                    7                8                  9  10      11           12
0004216A 2020-07-31 09:58:25.732 42675 68625 "[10.173.104.100:9876{52742}] COMPLETE: Inquiry_Services.Log_Service - from 172.16.70.153 complete in 32 msecs memory=56 Mb priority=-2 slavesreply=121269 resultsize=1592 continue=0 NumRowsProcessed=4572 NumIndexSeeks=32 NumIndexScans=659 NumIndexWildSeeks=15 NumIndexMerges=3 NumLeafCacheHits=15 NumNodeCacheHits=42 NumLeafCacheAdds=11 NumNodeCacheAdds=7 NumIndexAccepted=667 NumIndexRejected=432 N

    1         2         3          4     5                            6                                   7           8                             9
0004215D 2020-07-31 09:58:25.093 42675 68625 "[10.173.104.100:9876{esp_172.16.70.157_160012413R144982}] QUERY: 172.16.70.157:9876 esp_172.16.70.157_160012413R144982 SoapRequest <Risk_Indicators.InstantID uid='esp_172.16.70.157_160012413R144982'>

    1         2         3          4     5                               6                                 7                8                          9
00042163 2020-07-31 09:58:25.248 42675 68625 "[10.173.104.100:9876{esp_172.16.70.157_160012413R144982}] COMPLETE: Risk_Indicators.InstantID esp_172.16.70.157_160012413R144982 from 172.16.70.157 complete in 156 msecs memory=167 Mb priority=-2 slavesreply=331997 resultsize=5745 continue=0 NumRowsProcessed=62873 NumIndexSeeks=1708 NumIndexScans=7005 NumIndexWildSeeks=1125 NumIndexMerges=4 NumLeafCacheHits=1023 NumNodeCacheHits=1361 NumLeafCacheAdds=146 NumNodeCacheAdds=34 NumIndexAccepted=6778 NumIndexRejected=6331 NumIndexRowsRead=13089 TimeSoapcall=8.803ms TimeSortElapsed=506402ns NumGroups=343 NumGroupMax=2624 NumAllocations=92101 NumAllocationScans=32982 fDMetaphone1={ NumStarted=243 NumStopped=243 CycleLocalExecuteCycles=366065 } fCleanLNBO={ NumStarted=1 NumStopped=1 CycleLocalExecuteCycles=2304431 }"

    1         2         3          4     5      6      7        8   9
00000001 2020-07-14 21:21:46.494 70720 70720 "Roxie starting, build = internal_6.4.28-1"

*/

class Query
{
public:
    int act;
    int hasend;
    uint64_t stime;
    uint64_t etime;
    std::string qid;
    std::string qname;

    Query(char *_qid, uint64_t *_stime, uint64_t *_etime, char *_qname)
    {
        act = 0;
        hasend = 0;
        stime = *_stime;
        etime = *_etime;
        qid.append(_qid);
        qname.append(_qname);
    }
};

int main(int argc, char *argv[])
{
    char logfile[256] = { "" };
    char start_time[256] = { "" };
    char end_time[256] = { "" };
    int do_print = 0;

    if (argc >= 4)
    {
        strcpy(logfile, argv[1]);
        strcpy(start_time, argv[2]);
        strcpy(end_time, argv[3]);
        if (argc > 4)
        {
            if (strcmp(argv[4], "-v") == 0)
            {
                do_print = 10;
                if (argc > 5)
                    do_print = atoi(argv[5]);
            }
        }
    }
    else
    {
        printf("Error, need Roxie log-file, start-time (HH:MM:SS), end-time (HH:MM:SS) [-v [max-active-thresold==10]] arguments\n");
        return 1;
    }

    FILE *fp = fopen(logfile, "rb");
    if (fp == NULL)
    {
        printf("Error, unable to open <%s>\n", logfile);
        return 1;
    }

    char line[50001] = { "" };

    std::unordered_map<std::string, Query> qmap;

    int numq = 0;
    int maxact = 0;
    uint64_t mtime = 0;
    time_t max_time = 0;

    unsigned int prevpid = 0;

    char ftime[256] = { "" };

    time_t stime = 0;
    time_t etime = 0;
    time_t otime = 0;
    uint64_t sxtime = 0;
    uint64_t extime = 0;

    int fpid = 1;
    int fline = 1;
    while(fgets(line, 42001, fp) != NULL)
    {
        line[42000] = '\0';

        unsigned int logid, pid, tid;
        char ldate[256] = { "" };
        char ltime[256] = { "" };
        char qid[256] = { "" };
        char qstate[256] = { "" };
        char misc1[42001] = { "" };
        char misc2[42001] = { "" };
        char misc3[42001] = { "" };
        char misc4[42001] = { "" };

        int rtn = sscanf(line, "%x%s%s%u%u%s%s%s%s%s%s", &logid, ldate, ltime, &pid, &tid, qid, qstate, misc1, misc2, misc3, misc4);
        if (rtn < 8)
            continue;

        char ltime2[256] = { "" };
        strcpy(ltime2, ltime);

        // 09:58:25.700
        char *hr = strtok(ltime2,":");
        char *min = strtok(NULL,":");
        char *sec = strtok(NULL,".");
        char *msc = strtok(NULL," ");

        // printf("ltime = <%s> hr=<%s> min=<%s> sec=<%s> msc=<%s>\n", ltime, hr, min, sec, msc);

        if ( (hr == NULL) || (min == NULL) || (sec == NULL) || (msc == NULL) )
            continue;

        uint64_t msec = strtoul(msc, NULL, 10);

        char date_time[512] = { "" };
        sprintf(date_time, "%s %s:%s:%s", ldate, hr, min, sec);

        if (fline)
        {
            fline = 0;
            struct tm tm0, tm1;
            memset(&tm0, 0, sizeof(struct tm));
            memset(&tm1, 0, sizeof(struct tm));
            char date_time0[512] = { "" };
            char date_time1[512] = { "" };
            sprintf(date_time0, "%s %s", ldate, start_time);
            strptime(date_time0, "%Y-%m-%d %H:%M:%S", &tm0);
            stime = mktime(&tm0);
            otime = stime - 200000;
            stime -= otime;
            sxtime = (stime * 1000);
            sprintf(date_time1, "%s %s", ldate, end_time);
            strptime(date_time1, "%Y-%m-%d %H:%M:%S", &tm1);
            etime = mktime(&tm1);
            etime -= otime;
            extime = (etime * 1000);
        }

        struct tm tm;
        memset(&tm, 0, sizeof(struct tm));
        strptime(date_time, "%Y-%m-%d %H:%M:%S", &tm);
        time_t qtime = mktime(&tm);
        qtime -= otime;
        uint64_t qxtime = (qtime * 1000) + msec;

        if ( (qxtime >= sxtime) && (qxtime <= extime) )
        {
            if ( (!fpid) && (strcmp(qid, "\"Roxie") == 0) && (strcmp(qstate, "starting,") == 0) && (strcmp(misc1, "build") == 0) )
            {
                if ( (qxtime >= sxtime) && (qxtime <= extime) )
                {
                    fpid = 1;

                    printf("-- restart time: %s\n", ltime);
                    printf("Roxie pid (%u) changed (prev: %u) ...\n", pid, prevpid);

                    // printf("qmap.size() = %lu\n", qmap.size());

                    auto iter1 = qmap.begin();
                    while (iter1 != qmap.end())
                    {
                        auto iter2 = iter1;
                        while (iter2 != qmap.end())
                        {
                            if ( (iter2->second.stime <= iter1->second.stime) && (iter2->second.etime >= iter1->second.etime) )
                            {
                                iter1->second.act++;
                            }
                            iter2++;
                        }
                        iter1++;
                    }

                    int numq0 = 0;
                    int maxact0 = 0;
                    uint64_t mtime0 = 0;
                    time_t max_time0 = 0;

                    auto iter = qmap.begin();
                    while (iter != qmap.end())
                    {
                        numq++;
                        numq0++;
                        if (iter->second.act > maxact)
                        {
                            maxact = iter->second.act;
                            mtime = iter->second.stime;
                        }
                        if (iter->second.act > maxact0)
                        {
                            maxact0 = iter->second.act;
                            mtime0 = iter->second.stime;
                        }
                        iter++;
                    }

                    max_time0 = (mtime0 / 1000) + otime;

                    struct tm *mtm = localtime(&max_time0);

                    if (mtm->tm_isdst > 0)
                        mtm->tm_hour -= 1;

                    if (numq0 > 0)
                    {
                        printf("maxact = %u\n", maxact0);
                        printf("max time = %02u:%02u:%02u\n", mtm->tm_hour, mtm->tm_min, mtm->tm_sec);
                        if (do_print && maxact0 >= do_print)
                        {
                            auto iter = qmap.begin();
                            while (iter != qmap.end())
                            {
                                if ( (iter->second.stime <= mtime0) && (iter->second.etime > mtime0) )
                                {
                                    printf("  %s\n", iter->second.qname.c_str());
                                }
                                iter++;
                            }
                        }
                        printf("----------------\n");
                    }

                    qmap.clear();
                }
            }
            else if ( (strcmp(qstate, "QUERY:") == 0) || (strcmp(qstate, "BLIND:") == 0) || (strcmp(qstate, "COMPLETE:") == 0) )
            {
                if (fpid)
                {
                    fpid = 0;
                    prevpid = pid;
                }

                char qid2[256] = { "" };
                strcpy(qid2, &qid[1]);

                // printf("%x %s %s %u %u %s %s\n", logid, ldate, ltime, pid, tid, qid2, qstate);

                char key[300] = { "" };
                sprintf(key, "%s:%u", qid2, tid);

                if ( (strcmp(qstate, "QUERY:") == 0) || (strcmp(qstate, "BLIND:") == 0) )
                {
                    char qname[42001] = { "" };
                    if (misc3[0] == '<')
                    {
                        strcpy(qname, &misc3[1]);
                        for(int i=1;i<(int)strlen(misc3);i++)
                        {
                            if (misc3[i] == '>')
                            {
                                misc3[i] = '\0';
                                strcpy(qname, &misc3[1]);
                                break;
                            }
                        }
                    }
                    else if (misc4[0] == '<')
                    {
                        strcpy(qname, &misc4[1]);
                        for(int i=1;i<(int)strlen(misc4);i++)
                        {
                            if (misc4[i] == '>')
                            {
                                misc4[i] = '\0';
                                strcpy(qname, &misc4[1]);
                                break;
                            }
                        }
                    }
                    else
                        strcpy(qname, "<unknown>");

                    uint64_t e1time = extime + 50000;
                    Query q(&qid2[0], &qxtime, &e1time, qname);
                    auto n1 = std::pair<std::string, Query>(key, q);
                    qmap.insert(n1);
                }
                else // COMPLETE:
                {
                    strcpy(ftime, ltime);
                    auto res = qmap.find(key);
                    if (res != qmap.end())
                    {
                        res->second.etime = qxtime;
                        res->second.hasend = 1;
                    }
                    else
                    {
                        uint64_t s1time = sxtime - 50000;
                        Query q(&qid2[0], &s1time, &qxtime, misc1);
                        auto n1 = std::pair<std::string, Query>(key, q);
                        qmap.insert(n1);
                    }
                }

                if (qmap.size() >= 10000)
                {
                    // printf("qmap.size() = %lu\n", qmap.size());

                    auto iter1 = qmap.begin();
                    while (iter1 != qmap.end())
                    {
                        auto iter2 = iter1;
                        while (iter2 != qmap.end())
                        {
                            if ( (iter2->second.stime <= iter1->second.stime) && (iter2->second.etime >= iter1->second.etime) )
                            {
                                iter1->second.act++;
                            }
                            iter2++;
                        }
                        iter1++;
                    }

                    int numq0 = 0;
                    int maxact0 = 0;
                    uint64_t mtime0 = 0;
                    time_t max_time0 = 0;

                    auto iter = qmap.begin();
                    while (iter != qmap.end())
                    {
                        numq++;
                        numq0++;
                        if (iter->second.act > maxact)
                        {
                            maxact = iter->second.act;
                            mtime = iter->second.stime;
                        }
                        if (iter->second.act > maxact0)
                        {
                            maxact0 = iter->second.act;
                            mtime0 = iter->second.stime;
                        }
                        iter++;
                    }

                    max_time0 = (mtime0 / 1000) + otime;

                    struct tm *mtm = localtime(&max_time0);

                    if (mtm->tm_isdst > 0)
                        mtm->tm_hour -= 1;

                    if (numq0 > 0 && do_print && maxact0 >= do_print)
                    {
                        printf("maxact = %u\n", maxact0);
                        printf("max time = %02u:%02u:%02u\n", mtm->tm_hour, mtm->tm_min, mtm->tm_sec);
                        auto iter = qmap.begin();
                        while (iter != qmap.end())
                        {
                            if ( (iter->second.stime <= mtime0) && (iter->second.etime > mtime0) )
                            {
                                printf("  %s\n", iter->second.qname.c_str());
                            }
                            iter++;
                        }
                        printf("----------------\n");
                    }

                    iter = qmap.begin();
                    while (iter != qmap.end())
                    {
                        if (iter->second.hasend)
                            qmap.erase(iter++);
                        else
                            iter++;
                    }
                }

            }
        }
        else if (qxtime > extime + 50000)
            break;
    }

    fclose(fp);

    // printf("qmap.size() = %lu\n", qmap.size());

    auto iter1 = qmap.begin();
    while (iter1 != qmap.end())
    {
        auto iter2 = iter1;
        while (iter2 != qmap.end())
        {
            if ( (iter2->second.stime <= iter1->second.stime) && (iter2->second.etime >= iter1->second.etime) )
            {
                iter1->second.act++;
            }
            iter2++;
        }
        iter1++;
    }

    int numq0 = 0;
    int maxact0 = 0;
    uint64_t mtime0 = 0;
    time_t max_time0 = 0;

    auto iter = qmap.begin();
    while (iter != qmap.end())
    {
        numq++;
        numq0++;
        if (iter->second.act > maxact)
        {
            maxact = iter->second.act;
            mtime = iter->second.stime;
        }
        if (iter->second.act > maxact0)
        {
            maxact0 = iter->second.act;
            mtime0 = iter->second.stime;
        }
        iter++;
    }

    max_time0 = (mtime0 / 1000) + otime;

    struct tm *mtm = localtime(&max_time0);

    if (mtm->tm_isdst > 0)
        mtm->tm_hour -= 1;

    if (numq0 > 0 && numq0 != numq)
    {
        printf("maxact = %u\n", maxact0);
        printf("max time = %02u:%02u:%02u\n", mtm->tm_hour, mtm->tm_min, mtm->tm_sec);
    }

    if (numq0 > 0 && numq0 != numq)
        printf("----------------\n");

    max_time = (mtime / 1000) + otime;

    mtm = localtime(&max_time);

    if (mtm->tm_isdst > 0)
        mtm->tm_hour -= 1;

    printf("start time: %s\n", start_time);
    printf("end time:   %s\n", end_time);

    printf("tot numq   = %u\n", numq);
    printf("tot maxact = %u\n", maxact);

    printf("tot max time = %02u:%02u:%02u\n", mtm->tm_hour, mtm->tm_min, mtm->tm_sec);

    if (numq0 > 0 && do_print && maxact0 >= do_print)
    {
        auto iter = qmap.begin();
        while (iter != qmap.end())
        {
            if ( (iter->second.stime <= mtime0) && (iter->second.etime > mtime0) )
            {
                printf("  %s\n", iter->second.qname.c_str());
            }
            iter++;
        }
    }

    return 0;
}
