/*
 * This is an implementation of Viscous protocol.
 * Copyright (C) 2017  Abhijit Mondal
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <ctype.h>
#include <iostream>
//#include <stdio.h>
//#include <stdlib.h>
#include <unistd.h>

#include "ListentoNetworkEvents.hh"
#include "TrafficGenerator.h"
#include "TcpTrafficGenerator.h"
#include "testThreadPool.h"
#include "PacketHijacking.h"
#include "TcpMultiplexingTrafficGenerator.h"
#include "QualityTest.hh"
#include "TimeTest.hh"

#include <signal.h>

void my_handler(int signum)
{
    if (signum == SIGUSR1)
    {
        exit(0);
    }
}

struct SetupOptions{
    char *progname;
    int port;
    appByte *serverIp;
    appByte *filePath;
    bool server;
    bool client;
    bool normalTcp;
    bool test;
    int experiment;
    int numThread;
    int numConn;
    int nfqId;
    char qualityTest;
    bool networkMonitor;
};
SetupOptions expSetup = {
    .progname = NULL,
    .port = 8989,
    .serverIp = NULL,
    .filePath = NULL,
    .server = false,
    .client = false,
    .normalTcp = false,
    .test = false,
    .experiment = 1,
    .numThread = 1,
    .numConn = 1,
    .nfqId = 1,
    .qualityTest = 0,
    .networkMonitor = false,
};


void usages(std::ostream &strm, char *progname){
    strm << progname  << " -c <serverIP> -f <file> [-t] [-p <serverPort>]"<< std::endl;
    strm << progname  << " -s [-p <serverPort>] [-t] [-f <fileprefix>]"<< std::endl;
    strm << progname << " -x" << std::endl;
    strm << "=============================" << std::endl;
    strm << "-s " << std::endl << "\t run as server" << std::endl << std::endl;
    strm << "-c [server_ip] " << std::endl << "\t run as client and connect to server_ip" << std::endl << std::endl;
    strm << "-f [filepath] " << std::endl << "\t file path to read or file prefix to write" << std::endl << std::endl;
    strm << "-p [port/server port] " << std::endl << "\t numThread" << std::endl << std::endl;
    strm << "-t " << std::endl << "\t run normat tcp" << std::endl << std::endl;
    strm << "-x " << std::endl << "\t run unit test <not completed>" << std::endl << std::endl;
    strm << "-e [experiment_number] " << std::endl << "\t experiment number defualt is 1" << std::endl << std::endl;
    strm << "-T " << std::endl << "\t Number of threads" << std::endl << std::endl;
    strm << "-C " << std::endl << "\t Number of connections" << std::endl << std::endl;
    strm << "-q [nfqId] " << std::endl << "\t NFQ Id for packet hijacking" << std::endl << std::endl;
    strm << "-N " << std::endl << "\t Test Network (with network hack)" << std::endl << std::endl;
    strm << "-h " << std::endl << "\t show this text" << std::endl << std::endl;
}

void exp1Server(){
    LOGI("inside exp1server");
    if(expSetup.normalTcp)
        TcpTrafficGeneratorReciever::startServer(expSetup.port, expSetup.filePath);
    else
        TrafficGeneratorServer::startServer(expSetup.port, expSetup.filePath);

}

void exp1Client(){
    if(!expSetup.filePath){
        usages(std::cerr, expSetup.progname);
        exit(__LINE__);
    }
    if(expSetup.normalTcp)
        TcpTrafficGeneratorSender::startClient(expSetup.serverIp, expSetup.port, expSetup.filePath);
    else
        TrafficGeneratorClient::startClient(expSetup.serverIp, expSetup.port, expSetup.filePath);
}

void exp2Client(){
    if(expSetup.numConn == 0 or expSetup.numThread == 0){
        usages(std::cerr, expSetup.progname);
        exit(__LINE__);
    }
    if(expSetup.normalTcp)
        TcpTrafficGeneratorSender2::startClient(expSetup.serverIp, expSetup.port, expSetup.numThread, expSetup.numConn);
//        TcpMultiplexing::Sender::startClient(expSetup.serverIp, expSetup.port, expSetup.numThread, expSetup.numConn);
    else
        TrafficGeneratorClient2::startClient(expSetup.serverIp, expSetup.port, expSetup.numThread, expSetup.numConn);
}

void exp3Client(){
    if(expSetup.nfqId == 0){
        usages(std::cerr, expSetup.progname);
        exit(__LINE__);
    }
    if(expSetup.normalTcp){
        std::cout << "not implemented yet" << std::endl;
        exit(__LINE__);
    }
//    else
//        PacketHijack::startClient(expSetup.serverIp, expSetup.port, expSetup.nfqId);
}

void exp1(){
    if(expSetup.server)
        exp1Server();
    if(expSetup.client)
        exp1Client();
}

void exp2(){
    if(expSetup.server)
        exp1Server();
    if(expSetup.client)
        exp2Client();
}

void exp3(){
    if(expSetup.server)
        exp1Server();
    if(expSetup.client)
        exp3Client();
}

void experiemts(){
    switch(expSetup.experiment){
        case 1:
            exp1();
            break;
        case 2:
            exp2();
            break;
        case 3:
            exp3();
            break;
        default:
            exp1();
    }
}

void runQtes(char q){
    switch(q){
    case 'p':
        Quality::testPacketWithReadData();
        break;
    case 't':
        TimeTest::timeTest();
        break;
    }
}

int
main (int argc, char **argv)
{

//    int index;
    int c;


    signal(SIGUSR1, my_handler);

    opterr = 0;

    expSetup.progname = argv[0];

    while ((c = getopt (argc, argv, "sc:f:hp:txNe:T:C:q:Q:")) != -1)
    {
        switch (c)
        {
        case 'c':
            expSetup.serverIp = (appByte *)optarg;
            expSetup.client = true;
            break;
        case 'f':
            expSetup.filePath = (appByte *)optarg;
            break;
        case 's':
            expSetup.server = true;
            break;
        case 'p':
            expSetup.port = atoi(optarg);
            break;
        case 't':
            expSetup.normalTcp = true;
            break;
        case 'x':
            expSetup.test = true;
            break;
        case 'e':
            expSetup.experiment = atoi(optarg);
            break;

        case 'T':
            expSetup.numThread = atoi(optarg);
            break;

        case 'C':
            expSetup.numConn = atoi(optarg);
            break;

        case 'q':
            expSetup.nfqId = atoi(optarg);
            break;

        case 'N':
            expSetup.networkMonitor = true;
            break;

        case 'Q':
            expSetup.qualityTest = optarg[0];
            break;

        case '?':
        case 'h':
            usages(std::cout, argv[0]);
            return 1;
        default:
            usages(std::cerr, argv[0]);
            exit(0);
        }
    }

    if(expSetup.qualityTest){
        runQtes(expSetup.qualityTest);
        return 0;
    }

    if(expSetup.networkMonitor){
        listenOnNetworkEvent();
        return 0;
    }
    else if(expSetup.test){
        threadTest();
        return 0;
    }

    if(expSetup.server == expSetup.client){
        std::cerr << "invalid syntax" << std::endl;
        usages(std::cerr, *argv);
        return 1;
    }

    experiemts();
    return 0;
}
