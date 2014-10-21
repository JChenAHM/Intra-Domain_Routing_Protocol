#include "RoutingProtocolImpl.h"

RoutingProtocolImpl::RoutingProtocolImpl(Node *n) : RoutingProtocol(n) {
	sys = n;
    sys->time();
    printf("halo\n");
    char* data;
    data = (char*)malloc(sizeof(char));
    // '1' for 1s check, 'n' for neighbor update, 'd' for dv update, 'l' for ls update
    data[0] = '1';
    data[1] = 'n';
    data[2] = 'd';
    data[3] = 'l';
  
    void* void_data1 = &data[0];
    sys->set_alarm(this,1000,void_data1);
  
    void* void_data2 = &data[1];
    sys->set_alarm(this,10000,void_data2);
  
    void* void_data3 = &data[2];
    sys->set_alarm(this,30000, void_data3);   
}

RoutingProtocolImpl::~RoutingProtocolImpl() {
  // add your own code (if needed)
}

void RoutingProtocolImpl::init(unsigned short num_ports, unsigned short router_id, eProtocolType protocol_type) {
    this->proto_type = protocol_type;
    this->router_id = router_id;
    this->num_ports = num_ports;
    short i;
	mySequence = 0;

    // create a port status table, set neighbor id to 0 and cost to infinity 
    for(i=0;i<num_ports;i++) {
		rport entry={i,0,INFINITY_COST,false};
        this->port_table.push_back(entry);
	}  
    // sending the initial ping-pong packet
    for(i=0;i<num_ports;i++) {
        this->detect_neighbor(router_id,i);  
    }
}

void RoutingProtocolImpl::handle_alarm(void *data) {
    char* flag = static_cast<char*>(data);
    int i;
    // enforce 1s check on dv table
    if(*flag == '1') {
		// check whether DV table has timeout entries
        // show_DV_table();
        for(i=0;i<(int)this->DV_table.size(); i++) {
			// erase the entry if its' TTL is <= 0
            if(this->DV_table.at(i).ttl <= 0) {
				cout << "The DV entry for destination  " << this->DV_table.at(i).dest_id << " is time out\n"; 
                this->DV_table.erase(this->DV_table.begin()+i);
			}
		}
        sys->set_alarm(this,1000, data);
    }
    // enforce 10s ping message update
    else if(*flag == 'n') {
        for(i=0;i<this->num_ports;i++) {
			this->detect_neighbor(this->router_id,i);  
        }
        sys->set_alarm(this,10000,data);
    }
    // enforce 30s check on dv update
    else if(*flag == 'd') {
		for(i=0;i<(int)this->port_table.size(); i++) {
			forward_dv(router_id, this->port_table.at(i).neighbor_id, this->port_table.at(i).port_num); 
		}
		cout << "The router ID is " << this->router_id << endl;
        // show_port_table();
        show_forwarding_table();
        // show_DV_table(); 
        sys->set_alarm(this,30000, data);
    }
	/*else if (*flag == 'l') {
		sendLSTable();
		dijkstra();
		sys->set_alarm(this, 30000, data);
	}
	else
		printf("Node %d:\n\t***Error*** Unknown protocl when handling update\n\ttime: %d\n\n",
		router_id, sys->time());*/
}

void RoutingProtocolImpl::recv(unsigned short port, void *packet, unsigned short size) {
	// void pointer can not be dereferenced, used for sending packet
    void* v_pointer; 
    if(port == SPECIAL_PORT) {
        struct sim_pkt_header* pk = static_cast<struct sim_pkt_header*>(packet);
        if(pk->type == DATA) {
        }
        free(packet);
	} else{
		// cout <<"The ID of this router is " << this->router_id << endl;
        char *p = static_cast<char*>(packet);
        ePacketType packet_type = (ePacketType)(*(ePacketType*)p); 
        
        if(packet_type == DATA) {
        } else if(packet_type == PING) {
			// send a PONG message immediately
            *(ePacketType *) (p) = (ePacketType) PONG;
            *(short *) (p+48) = (*(short*)(p+32));
            *(short *) (p+32) = (short) htons(this->router_id);
            v_pointer = p;
            sys->send(port,v_pointer,size);  
		} else if(packet_type == PONG) {
			short sid = (short)ntohs(*(short*)(p+32));
            int time_stamp = (int)ntohl(*(int*) (p+64));
            // cout <<"Received from router " << sid << endl;
            // cout <<"time stamp received is " << time_stamp << endl;
            int current_time = sys->time();
            // cout <<"The current time is " << current_time << endl;     
            // store the port status information
            for (vector<rport>::iterator it = port_table.begin() ; it != port_table.end(); ++it) {
				if((*it).port_num == port) {
					(*it).neighbor_id = sid;
					(*it).cost = current_time - time_stamp;
					(*it).is_alive = true;
				}       
			}
            //show_port_table();
            // initial the forwarding table by adding the info of the router's neighbours                
            /*
            if( this->DV_table.size() == 0) {  
			    dvtable entry={sid,sid,port,current_time - time_stamp,45};
                this->DV_table.push_back(entry);
                update_forwarding_table(sid,sid);
            }*/
            // also make the ttl time back to 45
			merge_route(sid,sid,port,current_time-time_stamp);   
            // show_DV_table();
		} else if(packet_type == DV) {
			short p_size = (short)ntohs(*(short*)(p+16));                       
			short sid = (short)ntohs(*(short*)(p+32));
            // cout <<"Received from router " << sid << endl;
            // cout <<"The size of a packet is" << p_size << endl;       
			int DV_num,i;          
            // get the port number for the neighbor 
            unsigned short port_num;
            int port_cost;
            for(i=0; i< (int)this->port_table.size(); i++) {
				if(this->port_table.at(i).neighbor_id == sid) {
					port_num = this->port_table.at(i).port_num;
					port_cost = this->port_table.at(i).cost;
					break;
				}
			}               
            // number of node, cost pair
            DV_num = (p_size - 64)/32;
            for(i=0;i<DV_num; i++) {
				short node =(short)ntohs( *(short *)(p+64+i*32));
				int cost = (int)ntohl(*(int *) (p+64+i*32+16));
                // cout <<"the destination node is " << node << endl;
                // cout <<"the cost for that node is " << cost << endl;
				merge_route(node,sid,port_num,port_cost+cost);
			}        
        } else if(packet_type == LS) {
			recvLS(port, packet, size);
        } else {
            cout << "Invalid packet type\n"; 
        }
	}
}

void RoutingProtocolImpl::detect_neighbor( unsigned short router_id, unsigned short port_id) {
	char *packet;
	short pp_size = 3*32;
	void *v_pointer;
   
    // the packet consists of three 32bytes sub-packet
    packet = (char*) malloc(pp_size);
    // timestamp for ping message and it is stored in the packet payload
    int t = sys->time();

    *(ePacketType *) (packet) = (ePacketType) PING;
    *(short *) (packet+16) = (short) htons(pp_size);
    *(short *) (packet+32) = (short) htons(router_id);
    *(int *) (packet+64) = (int) htonl(t);

    v_pointer = packet;
    sys->send(port_id,v_pointer,pp_size);    		
}

void RoutingProtocolImpl::forward_dv( unsigned short src_id, unsigned short dest_id, unsigned short port_id ) {
	char *packet;
    void *v_pointer;
    short router_num = this->DV_table.size();
    // packet type row + src,dest row + rows for every router
    short pp_size = 32*(2+router_num);
    packet = (char*)malloc(pp_size);
  
    *(ePacketType *) (packet) = (ePacketType) DV;
    *(short *) (packet+16) = (short) htons(pp_size);
    *(short *) (packet+32) = (short) htons(src_id);
    *(short *) (packet+48) = (short) htons(dest_id);
  
    int i;
    for(i=0;i<(int)this->DV_table.size(); i++) {
		*(short *) (packet+64+i*32) = (short) htons(this->DV_table.at(i).dest_id);
        
        // poisoned reverse
        if(dest_id == this->DV_table.at(i).next_hop) {
			*(int *) (packet+64+i*32+16) = (int) htonl(INFINITY_COST);
        }
        *(int *) (packet+64+i*32+16) = (int) htonl(this->DV_table.at(i).cost);
	}
    v_pointer = packet;
    sys->send(port_id, v_pointer, pp_size);
}

void RoutingProtocolImpl::merge_route (unsigned short dest, unsigned short next, unsigned short port, int new_cost) {
    // 0 indicate the destination id is not find in the current routing table
    int i, find_flag = 0;
    // do not store information when the destination node is itself
    if(dest == this->router_id)
        return;
  
    for(i=0;i<(int)this->DV_table.size(); i++) {
  
        if(this->DV_table.at(i).dest_id == next) {
                this->DV_table.at(i).ttl = 45;
        }
  
        if(this->DV_table.at(i).dest_id == dest) {
			find_flag = 1;
            this->DV_table.at(i).ttl = 45;       
            // update the routing table if the new cost+cost to neighbor is smaller
            if(DV_table.at(i).cost > new_cost) {
				DV_table.at(i).cost = new_cost;
                DV_table.at(i).next_hop = next;
                DV_table.at(i).port_num = port;
				update_forwarding_table(dest,next);
			}
            break;
		}
	}
    // the destination node id is not in the current forwarding table
    if(find_flag == 0) {
        dvtable entry={dest,next,port,new_cost,45};
        this->DV_table.push_back(entry); 
        update_forwarding_table(dest,next);
    }
}

void RoutingProtocolImpl::update_forwarding_table(unsigned short dest, unsigned short next) {
    if(this->forwarding_table.size() == 0) {
        ftable entry = {dest,next};
        this->forwarding_table.push_back(entry);
    }
    else {
        // 0 if the destination id is not in the current forwarding table
        short flag = 0;
        for (vector<ftable>::iterator it = forwarding_table.begin() ; it != forwarding_table.end(); ++it) {
			if((*it).dest_id == dest) {
				(*it).next_hop = next;
				flag =1;
			}
		}
        if(flag == 0) {
			ftable entry = {dest,next};
			this->forwarding_table.push_back(entry);
        }
	}
}

void RoutingProtocolImpl::show_port_table() {
	int i;
    cout <<"The current port table is:" << endl;
    for(i=0;i<(int)this->port_table.size(); i++) {
        cout << "Port ID is " <<  this->port_table.at(i).port_num 
             << " neighbor ID is " << this->port_table.at(i).neighbor_id 
             << " port number is" <<this->port_table.at(i).cost
             << " is alive is" << this->port_table.at(i).is_alive << endl;
	}  
}

void RoutingProtocolImpl::show_DV_table() {
  int i;
  cout <<"The current DV table is:" <<endl;
  for(i=0;i<(int)this->DV_table.size(); i++) {
        cout << "destination ID is " <<  this->DV_table.at(i).dest_id 
             << " next hop is " << this->DV_table.at(i).next_hop 
             << " port number is" <<this->DV_table.at(i).port_num
             <<"  cost is " << this->DV_table.at(i).cost
             <<" TTL is "<<this->DV_table.at(i).ttl << endl;
  }
}

void RoutingProtocolImpl:: show_forwarding_table() {
    int i; 
    cout <<"The current forwarding table is:" << endl;
    for(i=0;i<(int)this->forwarding_table.size(); i++) {
        cout << "destination ID is " <<  this->forwarding_table.at(i).dest_id 
             << " next hop is " << this->forwarding_table.at(i).next_hop << endl;
	}
}

void RoutingProtocolImpl::forward_packet() {
    char **packet;
    packet = (char**) malloc(sizeof(char*));
    *packet = (char*) malloc(32);
  
    *(ePacketType *) (packet[0]) = (ePacketType) DATA;
    *(int *) (packet[0]+15) = (int) htons(10);
}

/*
// check every expiration (link failure, table update, etc.)
bool RoutingProtocolImpl::handleExp() {
	int i;
	bool isChange = false;
	// check expiration of port status 
	for (i = 0; i < num_ports; i++)
	if (port_table.at(i).is_alive && sys->time() - port_table.at(i).update >= 15000) {
		isChange = true;
		port_table.at(i).is_alive = false;
		disableForward(port.at(i).neighbor_id); // assumed disable function
	}

	if (protocol == P_LS)
		// check expiration of LS status 
		checkLSTimeOut();
	else if (protocol == P_DV) {
	
	}

	// LS mode: send LS table if port status has changed
	// DV mode: update DV tables and send it
	if (isChange) {
		if (protocol == P_LS) {
			printf("\tLS table has been updated. Flood the change.\n");
			LSUpdate();
		}
		else if (protocol == P_DV) {

		}
	}
	return true;
}*/
