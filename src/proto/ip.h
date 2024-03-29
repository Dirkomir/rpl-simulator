/*
   RPL Simulator.

   Copyright (c) Calin Crisan 2010

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef IP_H_
#define IP_H_

#include "../base.h"
#include "../node.h"

#define MAC_TYPE_IP                 0x86DD

#define IP_ROUTE_TYPE_CONNECTED     0
#define IP_ROUTE_TYPE_MANUAL        1
#define IP_ROUTE_TYPE_RPL_DAO       2
#define IP_ROUTE_TYPE_RPL_DIO       3


typedef struct ip_send_info_t {

    node_t *                incoming_node;
    node_t **               next_hop_list;
    uint16                  next_hop_count;
    int16                   next_hop_index;

} ip_send_info_t;

typedef struct ip_neighbor_t {

    node_t *                node;
    sim_time_t              last_packet_time;

} ip_neighbor_t;

typedef struct ip_flow_label_t {

    bool                    going_down;
    bool                    from_sibling;
    bool                    rank_error;
    bool                    forward_error;
    uint16                  sender_rank;

} ip_flow_label_t;

    /* a struct defining a route record */
typedef struct ip_route_t {

    char *                  dst;
    uint8                   prefix_len;
    node_t *                next_hop;

    uint8                   type;
    void *                  further_info;

    sim_time_t              update_time;

        /* destination expressed as a bit-array, a performance workaround */
    uint8 *                 dst_bit_expanded;

} ip_route_t;

    /* info that a node supporting IP should store */
typedef struct ip_node_info_t {

    char *                  address;

    ip_route_t **           route_list;
    uint16                  route_count;

    ip_neighbor_t **        neighbor_list;
    uint16                  neighbor_count;

    bool                    busy;
    uint32                  enqueued_count;


} ip_node_info_t;

    /* fields contained in a IP packet */
typedef struct ip_pdu_t {

    char *                  dst_address;
    char *                  src_address;

    ip_flow_label_t *       flow_label;

    uint16                  next_header;
    void *                  sdu;

    bool                    queued; /* workaround for IP queue/buffer management */

} ip_pdu_t;


extern uint16               ip_event_node_wake;
extern uint16               ip_event_node_kill;

extern uint16               ip_event_pdu_send;
extern uint16               ip_event_pdu_send_timeout_check;
extern uint16               ip_event_pdu_receive;

extern uint16               ip_event_neighbor_cache_timeout_check;


bool                        ip_init();
bool                        ip_done();

ip_send_info_t *            ip_send_info_create(node_t *incoming_node, node_t *first_next_hop, node_t **next_hop_list, uint16 next_hop_count);
void                        ip_send_info_destroy(ip_send_info_t *ip_send_info);

ip_pdu_t *                  ip_pdu_create(char *src_address, char *dst_address);
void                        ip_pdu_destroy(ip_pdu_t *pdu);
ip_pdu_t *                  ip_pdu_duplicate(ip_pdu_t *pdu);
void                        ip_pdu_set_sdu(ip_pdu_t *pdu, uint16 next_header, void *sdu);

void                        ip_node_init(node_t *node, char *address);
void                        ip_node_done(node_t *node);

void                        ip_node_set_address(node_t *node, const char *address);

ip_route_t *                ip_node_add_route(node_t *node, char *dst, uint8 prefix_len, node_t *next_hop, uint8 type, void *further_info);
void                        ip_node_rem_route(node_t *node, ip_route_t *route);
void                        ip_node_rem_routes(node_t *node, char *dst, int8 prefix_len, node_t *next_hop, int8 type);
ip_route_t **               ip_node_get_routes(node_t *node, uint16 *route_count, char *dst, int8 prefix_len, node_t *next_hop, int8 type);
ip_route_t *                ip_node_get_next_hop_route(node_t *node, char *dst_address);

ip_neighbor_t *             ip_node_add_neighbor(node_t *node, node_t *neighbor_node);
bool                        ip_node_rem_neighbor(node_t *node, ip_neighbor_t *neighbor);
ip_neighbor_t *             ip_node_find_neighbor_by_node(node_t *node, node_t *neighbor_node);

bool                        ip_node_send(node_t *node, char *dst_ip_address, uint16 next_header, void *sdu);
bool                        ip_node_forward(node_t *node, node_t *incoming_node, ip_pdu_t *pdu);
bool                        ip_node_receive(node_t *node, node_t *incoming_node, ip_pdu_t *pdu);


#endif /* IP_H_ */
