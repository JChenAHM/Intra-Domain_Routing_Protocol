#include "RoutingProtocolImpl.h"

RoutingProtocolImpl::RoutingProtocolImpl(Node *n) : RoutingProtocol(n) {
  sys = n;
  sys->time();
  printf("halo\n");
  
  char* test_data;
  test_data = (char*)malloc(sizeof(char));
  
  void* data = test_data;

  sys->set_alarm(this,10000,data);
}

RoutingProtocolImpl::~RoutingProtocolImpl() {
  // add your own code (if needed)
}

void RoutingProtocolImpl::init(unsigned short num_ports, unsigned short router_id, eProtocolType protocol_type) {
  
  this->proto_type = protocol_type;
  this->router_id = router_id;
  short i;
  // create a port status table, set neighbor id to -1 and cost to infinity 
  for(i=0;i<num_ports;i++) {
  
        rport entry={i,-1,INFINITY_COST};
        this->port_table.push_back(entry);
        cout <<"Printing port table"<< this->port_table.at(i).port_num << endl;

  }
  
  
  // sending the initial ping-pong packet
  for(i=0;i<num_ports;i++) {
        
        this->detect_neighbor(router_id,i);  
  }
  
  printf("testing!!!\n");

}

void RoutingProtocolImpl::handle_alarm(void *data) {
  cout << "handle alarm test\n";
}

void RoutingProtocolImpl::recv(unsigned short port, void *packet, unsigned short size) {
  // void pointer can not be dereferenced, used for sending packet
  void* v_pointer;
  
  cout <<"Printing port table"<< this->port_table.at(0).port_num << endl;

  
  if(port == SPECIAL_PORT) {

        struct sim_pkt_header* pk = static_cast<struct sim_pkt_header*>(packet);
        if(pk->type == DATA) {
                cout << "The type is data!\n";
        }
        /*
        unsigned short sid = (unsigned short)ntohs(pk->src);
        unsigned short did = (unsigned short)ntohs(pk->dst);
        cout << "The source is " << sid << endl;
        cout << "The destination is " << did << endl;
        */
        free(packet);


  } else{
        cout <<"The packet is generated from other routers\n";
        cout <<"The ID of this router is " << this->router_id << endl;
        char *p = static_cast<char*>(packet);
        ePacketType packet_type = (ePacketType)(*(ePacketType*)p); 
        
        if(packet_type == DATA) {
        } else if(packet_type == PING) {
                // send a PONG message immediately
                short sid = (short)ntohs(*(short*)(p+32));
                cout <<"Received from router " << sid << endl;
                
                *(ePacketType *) (p) = (ePacketType) PONG;
                *(short *) (p+48) = (*(short*)(p+32));
                *(short *) (p+32) = (short) htons(this->router_id);
                v_pointer = p;
                sys->send(port,v_pointer,size);  
                
        } else if(packet_type == PONG) {
                
                short sid = (short)ntohs(*(short*)(p+32));
                int time_stamp = (int)ntohl(*(int*) (p+64));
                cout <<"Received from router " << sid << endl;
                cout <<"time stamp received is " << time_stamp << endl;
                
                int current_time = sys->time();
                cout <<"The current time is " << current_time << endl;
                
                /* 
                for (vector<rport>::iterator it = port_table.begin() ; it != port_table.end(); ++it) {
                
                        if(it.port_num == port) {
                                it.neighbor_id = sid;
                                it.cost = current_time - time_stamp;
                        }
                }*/
                //store information to te port table
                int i;
                for(i=0; i< (int)this->port_table.size(); i++) {
                        if(this->port_table.at(i).port_num == port) {
                                this->port_table.at(i).neighbor_id = sid;
                                this->port_table.at(i).cost = current_time - time_stamp;
                        }
                        cout << "port num is " <<  this->port_table.at(i).port_num 
                        << " neighbor is " << this->port_table.at(i).neighbor_id 
                        <<" cost is " << this->port_table.at(i).cost << endl; 
                }
                
                // initial the forwarding table by adding the info of the router's neighbours                
                if( this->routing_table.size() == 0) {  
                        rtable entry={sid,sid,current_time - time_stamp};
                        this->routing_table.push_back(entry);
                }
                for(i=0;i<(int)this->routing_table.size(); i++) {
                        cout << "destination ID is " <<  this->routing_table.at(i).dest_id 
                        << " next hop is " << this->routing_table.at(i).next_hop 
                        <<" cost is " << this->routing_table.at(i).cost << endl;
                }
                
                
        } else if(packet_type == DV) {
                
        } else if(packet_type == LS) {
                
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
  
//  cout << "time is sup " << t << endl;
  
  *(ePacketType *) (packet) = (ePacketType) PING;
  *(short *) (packet+15) = (short) htonl(pp_size);
  *(short *) (packet+32) = (short) htons(router_id);
  *(int *) (packet+64) = (int) htonl(t);

  v_pointer = packet;
//  char *id = static_cast<char*>(v_pointer);
//  cout <<"time from receive router" << *(short *) (id+64) << endl;
  sys->send(port_id,v_pointer,pp_size);    		

}


void RoutingProtocolImpl::forward_dv() {
  char *packet;
  short router_num = this->routing_table.size();
  // packet type row + src,dest row + rows for every router
  short pp_size = 32*(2+router_num);

}

void RoutingProtocolImpl::forward_packet() {
  char **packet;
  packet = (char**) malloc(sizeof(char*));
  *packet = (char*) malloc(32);
  
  *(ePacketType *) (packet[0]) = (ePacketType) DATA;
  *(int *) (packet[0]+15) = (int) htons(10);
}





