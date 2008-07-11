/***************************************************************************
 *   Copyright (C) 2008 by Artem Y. Polaykov and Jason Ansel               *
 *   DMTCP process relation and interconnection visualisation tool         *
 *   Written by Artem Y. Polyakov <artpol84@gmail.com>                     *
 *          and Jason Ansel <jansel@csail.mit.edu>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 ***************************************************************************/

// System includes
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <getopt.h>
// C++ includes
#include <fstream> 
#include <iostream>
#include <sstream>
// Local includes
#include "jassert.h"
#include "jfilesystem.h"
#include "connectionmanager.h"
#include "dmtcpworker.h"
#include "checkpointcoordinator.h"
#include "mtcpinterface.h"
#include "syscallwrappers.h"
#include "jtimer.h"

bool fullout = false;
bool parent_child = true;
bool sockets = true;
bool usedot = false;
std::string outfile,dotfile;

using namespace dmtcp;

namespace
{
  class InspectTarget
  {
    public:
      InspectTarget (const std::string& path)
        : _mtcpPath(path),
          _dmtcpPath (path + ".dmtcp")
      {
        JASSERT (jalib::Filesystem::FileExists(_mtcpPath)) (_mtcpPath).Text("missing file");
        JASSERT (jalib::Filesystem::FileExists(_dmtcpPath)) (_dmtcpPath).Text("missing file");
        jalib::JBinarySerializeReader rd(_dmtcpPath);
        _conToFd.serialize(rd);
      }

      std::string     _mtcpPath;
      std::string     _dmtcpPath;
      ConnectionToFds _conToFd;
  };

  class GProcess{
    public:
      GProcess(ConnectionToFds &conToFd,bool finfo){
        procname = conToFd.procname();
        hostname = conToFd.hostname();
        inhostname = conToFd.inhostname();
        pid = conToFd.pid();
        ppid = conToFd.ppid();
        _index = _nextIndex();
				fullinfo = finfo;
      }

      int index() { return _index; }
      std::string procname;
      std::string hostname;
      std::string inhostname;
      UniquePid pid;
      UniquePid ppid;
      void writeNode(std::ostringstream &o){
        o << " \"" << _index << "\"" 
          << " [ label=\"" << procname;
				if( fullinfo ){
					time_t tm = pid.time();
					char s[256];
					strftime(s,256,"%H:%M.%F",localtime(&tm));
					o << "[" << pid.pid() << "]@" << inhostname
						<< "\\n" << s ;
				}
        o << "\" shape=box ]\n";
      }

    private:
      int _nextIndex(){
        static int proc_index = 0;
        return proc_index ++;
      }
      int _index;
			bool fullinfo;
  };


	//---------------- Connection description class ---------------------------

  class GConnection{
    public:
      GConnection(TcpConnection &tcpCon); 
      void addProc(ConnectionIdentifier &id, int pindex, std::string hname);
      bool operator == (TcpConnection &tcpCon);
      bool operator == (ConnectionIdentifier &conId);
      ConnectionIdentifier srv() const { return _srv; }
      ConnectionIdentifier cli() const { return _cli; }
      void writeConnection(std::ostringstream &o,int &conCnt);
			bool is_loop(){ return _loop; }
			std::string hostname(){ return _hostname; }
    private:
			bool _loop;
			std::string _hostname;
      ConnectionIdentifier _srv,_cli;
      std::list<int> _sprocs,_cprocs;
  };

  GConnection::GConnection(TcpConnection &tcpCon)
  {
    // Consider only established connections
    switch( tcpCon.tcpType() ){
    case TcpConnection::TCP_ACCEPT:
      _srv = tcpCon.id();
      _cli = tcpCon.getRemoteId();
      break;
    case TcpConnection::TCP_CONNECT:
      _cli = tcpCon.id();
      _srv = tcpCon.getRemoteId();
      break;
    }
		// Check if it is loop connection or not
		if( _srv.pid().hostid() == _cli.pid().hostid() ){
			_loop = true;
		}else{
			_loop = false;
		}
		_hostname = "?";
  }

  bool GConnection::operator == (TcpConnection &tcpCon)
  {
    switch( tcpCon.tcpType() ){
    case TcpConnection::TCP_ACCEPT:
      if( _srv == tcpCon.id() && _cli == tcpCon.getRemoteId() )
        return true;
      return false;
    case TcpConnection::TCP_CONNECT:
      if( _cli == tcpCon.id() && _srv == tcpCon.getRemoteId() )
        return true;
      return false;
    }
  }

  bool GConnection::operator == (ConnectionIdentifier &conId)
  {
    if( _srv == conId || _cli == conId )
      return true;
    return false;
  }

  void GConnection::addProc(ConnectionIdentifier &id,int pindex,std::string hostname)
  {
		// Double check of loop connection
		// in the case host_hash has collision
		if( _hostname == "?" ){
			_hostname = hostname;
		}else if( hostname != _hostname ){
			_loop = false;
		}
		// Save process in connection
    if( id == _srv ){
      _sprocs.push_back(pindex);
    }else{
      _cprocs.push_back(pindex);
    }
  }

  void GConnection::writeConnection(std::ostringstream &o,int &conCnt)
  {
		// If connection have no shared descriptors
		if( _sprocs.size() == _cprocs.size() && _sprocs.size() == 1 ){
      o << " \"" << *_cprocs.begin() << "\" -> \"" << *_sprocs.begin() << "\" [ color=\"#000000\" ]\n";
			return;
		}
		// else: what if no srv or no cli connections?

    // Write connection representation
    o << " \"" << conCnt << "\" [ shape=\"circle\", color=\"#00FF00\"]\n"; 
    // Write processes connected to server side
		// 

    std::list<int>::iterator lit;
    for(lit = _sprocs.begin(); lit != _sprocs.end(); lit++){
      o << " \"" << conCnt << "\" -> \"" << (*lit) << "\" [ color=\"#000000\" ]\n";
    }
    // Write processes connected to client side
    for(lit = _cprocs.begin(); lit != _cprocs.end(); lit++){
      o << " \"" << (*lit) << "\" -> \"" << conCnt << "\" [ color=\"#000000\" ]\n";
    }
    conCnt++;
  }

	//---------------- Connections fraph class ---------------------------

  class ConnectionGraph{
    public:
      ConnectionGraph(ConnectionList &list);
      bool importProcess(ConnectionToFds &conToFd);
      bool exportGraph(std::string ofile);
      std::list<GConnection>::iterator find(TcpConnection &tcpCon);
      void writeGraph(std::ostringstream &o);
    private:
      std::list<GConnection> _connections;
			typedef std::map<std::string,std::list<GProcess> > ClusterProcesses;
			ClusterProcesses _processes;
			typedef std::map<dmtcp::UniquePid,GProcess*> DMTCP_process;
			DMTCP_process _row_processes;
  };

  ConnectionGraph::ConnectionGraph(ConnectionList &list)
  {
    ConnectionList::iterator it;

    for(it = list.begin(); it != list.end(); it++){
      Connection &con = *(it->second);
      if(con.conType() != Connection::TCP)
        continue;
      TcpConnection& tcpCon = con.asTcp();
      if( tcpCon.tcpType() == TcpConnection::TCP_ACCEPT ||
          tcpCon.tcpType() == TcpConnection::TCP_CONNECT ){
        // if it is new connection  (when running full inspection 
        // each connection appears twice - on each node)
        if( find(tcpCon) == _connections.end() )
          _connections.push_back(GConnection(tcpCon));
      }
    }
  }

  std::list<GConnection>::iterator
  ConnectionGraph::find(TcpConnection &tcpCon){
    std::list<GConnection>::iterator it = _connections.begin();
    for(; it != _connections.end(); it++){
      if( (*it) == tcpCon ){
        return it;
      }
    }
    return _connections.end();
  } 

  bool ConnectionGraph::importProcess(ConnectionToFds &conToFd)
  {
    ConnectionToFds::const_iterator cit;
    std::list<GProcess>::iterator pit;

		//    std::cout << "\nimportProcess:\n";

    // Add process to _processes table
    _processes[conToFd.hostname()].push_front(GProcess(conToFd,fullout));
    pit = _processes[conToFd.hostname()].begin();

		// Add to _row_process Map table
		_row_processes[pit->pid] = &(*pit);
		
		std::cout << "Add process: " << pit->pid 
							<< ". Result: " << _row_processes.find(pit->pid)->second->pid << "\n";

    // Run through all connections of the process
    for(cit = conToFd.begin(); cit!=conToFd.end(); cit++){
      ConnectionIdentifier conId = cit->first;
      Connection &con = ConnectionList::Instance()[cit->first];

      // Process only network connections
      if( con.conType() != Connection::TCP )
        continue;
      TcpConnection &tcpCon = con.asTcp();

      // If this is not ESTABLISHED connection
      if( tcpCon.tcpType() != TcpConnection::TCP_ACCEPT &&
          tcpCon.tcpType() != TcpConnection::TCP_CONNECT ){
        continue;
      }

      // Map process to connection
      std::list<GConnection>::iterator gcit = find(tcpCon);
      if( gcit != _connections.end() ){ 
        gcit->addProc(conId,pit->index(),pit->hostname);
      }
    }
  }

  void ConnectionGraph::writeGraph(std::ostringstream &o)
  {
    std::list<GConnection>::iterator cit;
		ClusterProcesses::iterator cpit;
		std::list<GConnection*>::iterator gcit;
		std::map< std::string, std::list<GConnection *> > inhost_conn;
		std::list<GConnection*> interhost_conn;

		// Divide connections on two groups:
		// 1. All communicated processes are at one host
		// 2. communicated processes are at different hosts
    for(cit = _connections.begin(); cit != _connections.end(); cit++){
			// If this is loopback connection - map it
			// for fast access
			if( cit->is_loop() ){
				inhost_conn[cit->hostname()].push_back((GConnection*)&(*cit));
			}else{
				interhost_conn.push_back((GConnection*)&(*cit));
			}
    }

		// Count max process index
    int conCnt = 0;
		for(cpit = _processes.begin(); cpit != _processes.end(); cpit++ ){
			std::list<GProcess>::iterator pit = cpit->second.begin();
			for(; pit != cpit->second.end(); pit++){
				if( pit->index() > conCnt )
					conCnt = pit->index();
			}
		}
    conCnt++;

    // Head of dot-file 
    o << "digraph { \n";

    // Create nodes for processes
		int cnt;
		for(cnt=0, cpit = _processes.begin(); cpit != _processes.end(); cpit++,cnt++ ){
			std::list<GProcess>::iterator pit = cpit->second.begin();
			std::string cur_hostname = pit->hostname;
			o << "subgraph cluster" << cnt << " {\n";
			o << " label=\"" << cur_hostname << "\";\n";
			o << " color=blue;\n";
			// write all processes
			for(; pit != cpit->second.end(); pit++){
				pit->writeNode(o);
			}
			// write all inhost connections
			if( sockets ){
				if( inhost_conn.find(cur_hostname) != inhost_conn.end() ){
					for(gcit = inhost_conn[cur_hostname].begin(); 
							gcit != inhost_conn[cur_hostname].end(); 
							gcit++ ){
						(*gcit)->writeConnection(o,conCnt);
					}
				}
			}

			o << "}\n";
		}

		// write all interhost connections
		if( sockets ){
			for(gcit = interhost_conn.begin(); gcit != interhost_conn.end(); gcit++){
				// Write connection to the file
				(*gcit)->writeConnection(o,conCnt);
			}
		}

		// write Parent - Child relationships
		if( parent_child ){
			DMTCP_process::iterator dit,dit1;
			for(dit = _row_processes.begin(); dit != _row_processes.end(); dit++){
				std::cout << "Inspect process: " << dit->second->procname 
									<< "[" << dit->second->pid << "," 
									<< dit->second->ppid << "]:\n";
				dit1 = _row_processes.find(dit->second->ppid);
				if( dit1 != _row_processes.end() ){
					std::cout << "find " << dit1->second->procname 
										<< "[" << dit1->second->pid << "]\n";
					o << " \"" << dit1->second->index() << "\" -> \"" << dit->second->index() 
						<< "\" [ color=\"#FF0000\", style=\"bold\" ]\n";
				}
			}
		}
    o << "}\n"; 
  }
}

  


static const char* theUsage = 
		"USAGE: dmtcp_inspector [-o<ofile>] [-d<ofile>] [-f] <ckpt1.mtcp> [ckpt2.mtcp...]\n"
		"\t-o <filename> - Output in dot-like format\n"
		"\t-d <filename> - Create graph using dot command (need graphviz package)\n"
		"\t--par-ch-off  - Do not draw parent-child relations\n"
		"\t--sock-off    - Do not draw socket connections\n"
		"\t-f            - Verbose node indication\n";

int main ( int argc, char** argv )
{

	// Process command line options
  int c;

	// No arguments => help
	if( argc == 1 ){
		std::cerr << theUsage;
		return 1;
	}

	// Process arguments
  while (1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = 
    {
    {"out-file", 1, 0, 'o'},
    {"dot", 1, 0, 'd'},
    {"full", 0, 0, 'f'},
    {"help", 0, 0, 'h'},
    {"parch-off", 0, 0, 0},
    {"sock-off", 0, 0, 0},
    {0, 0, 0, 0}
    };

    c = getopt_long(argc, argv,"o:fd:",long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
		case 0:{
			std::string tmp = long_options[option_index].name;
			
			if ( tmp == "parch-off" ){
				std::cout << "Turn off parent-child relation\n";
				parent_child = false;
			} else if( tmp == "sock-off" ){
				std::cout << "Turn off socket connnections\n";
				sockets = false;
			}
		}
			break;
    case 'o':
			outfile = optarg;
			std::cout << "Set output to: " << outfile << "\n";
      break;
    case 'f':
      fullout = true;
			std::cout << "Write full process info\n";
      break;
    case 'd':
      usedot = true;
			dotfile = optarg;
			std::cout << "Write output to DOT command. Result file:" << dotfile << " \n";
      break;
		case 'h':
			std::cerr << theUsage;
			return 1;
    case '?':
			break;

    default:
      printf("?? getopt returned character code 0%o ??\n", c);
    }
  }

  std::vector<InspectTarget> targets;
  if (optind < argc) {
		std::cout << "Loading checkpoint files:\n";
    for(int i = optind; i < argc; i++){
			std::cout << "Load " << argv[i] << "\n";
			if ( targets.size() >0 && targets.back()._dmtcpPath == argv[i] )
				continue;
			targets.push_back ( InspectTarget ( argv[i] ) );
		}
  }else{
		std::cerr << theUsage;
		return 1;
	}

  ConnectionGraph conGr(ConnectionList::Instance());
  for(int i =0; i < targets.size(); i++){
    conGr.importProcess(targets[i]._conToFd);
  }


	std::string out_string;
	std::ostringstream buf(out_string);
	conGr.writeGraph(buf);
	std::cout << buf.str();
	if( usedot ){	
		// Create pipe to dot
		std::string popen_str = "dot -Tpdf -o ";
		popen_str += dotfile;
		std::cout << "Popen arg: " << popen_str 
							<< "\nInput len=" << buf.str().length() << "\n";
		FILE *fp = popen(popen_str.c_str(),"w");
		if( !fp ){
			std::cout << "Error in popen(\"" << dotfile.c_str() << "\",\"w\"\n";
			return 0;
		}
		fprintf(fp,"%s",buf.str().c_str());
		pclose(fp);
	}else{
		std::ofstream o(outfile.c_str());
		o << buf.str();
		o.close();
	}

  return 0;
}

