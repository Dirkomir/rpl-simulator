
#include "system.h"
#include "math.h"


    /**** global variables ****/

rs_system_t *           rs_system = NULL;


    /**** local function prototypes ****/

static bool             rs_system_mangle_message(node_t *src_node, node_t *dst_node, phy_pdu_t *message);
static bool             rs_system_send_rpl(node_t *src_node, node_t *dst_node, void *rpl_pdu, uint8 icmp_rpl_code);


    /**** exported functions ****/

bool rs_system_create()
{
    rs_system = (rs_system_t *) malloc(sizeof(rs_system_t));

    rs_system->node_list = NULL;
    rs_system->node_count = 0;

    rs_system->no_link_dist_thresh = DEFAULT_NO_LINK_DIST_THRESH;
    rs_system->phy_transmit_mode = DEFAULT_PHY_TRANSMIT_MODE;

    rs_system->width = DEFAULT_SYS_WIDTH;
    rs_system->height = DEFAULT_SYS_HEIGHT;

    rs_system->nodes_mutex = g_mutex_new();

    return TRUE;
}

bool rs_system_destroy()
{
    rs_assert(rs_system != NULL);

    // todo: destroy the nodes
    free(rs_system->node_list);

    g_mutex_free(rs_system->nodes_mutex);

    free(rs_system);

    return TRUE;
}

coord_t rs_system_get_no_link_dist_thresh()
{
    rs_assert(rs_system != NULL);

    return rs_system->no_link_dist_thresh;
}

void rs_system_set_no_link_dist_thresh(coord_t thresh)
{
    rs_assert(rs_system != NULL);

}

uint8 rs_system_get_transmit_mode()
{
    rs_assert(rs_system != NULL);

    return rs_system->phy_transmit_mode;
}

void rs_system_set_transmit_mode(uint8 mode)
{
    rs_assert(rs_system != NULL);

    rs_system->phy_transmit_mode = mode;
}

coord_t rs_system_get_width()
{
    rs_assert(rs_system != NULL);

    return rs_system->width;
}

coord_t rs_system_get_height()
{
    rs_assert(rs_system != NULL);

    return rs_system->height;
}

void rs_system_set_width_height(coord_t width, coord_t height)
{
    rs_assert(rs_system != NULL);

    rs_system->width = width;
    rs_system->height = height;
}

bool rs_system_add_node(node_t *node)
{
    rs_assert(rs_system != NULL);
    rs_assert(node != NULL);

    g_mutex_lock(rs_system->nodes_mutex);

    rs_system->node_list = (node_t **) realloc(rs_system->node_list, (++rs_system->node_count) * sizeof(node_t *));
    rs_system->node_list[rs_system->node_count - 1] = node;

    g_mutex_unlock(rs_system->nodes_mutex);

    return TRUE;
}

bool rs_system_remove_node(node_t *node)
{
    rs_assert(rs_system != NULL);
    rs_assert(node != NULL);

    g_mutex_lock(rs_system->nodes_mutex);

    int i, pos = -1;
    for (i = 0; i < rs_system->node_count; i++) {
        if (rs_system->node_list[i] == node) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        rs_error("node '%s' not found", rs_system->node_list[i]);
        g_mutex_unlock(rs_system->nodes_mutex);
        return FALSE;
    }

    for (i = pos; i < rs_system->node_count - 1; i++) {
        rs_system->node_list[i] = rs_system->node_list[i + 1];
    }

    // todo: call realloc on node_list
    rs_system->node_count--;

    g_mutex_unlock(rs_system->nodes_mutex);

    return TRUE;
}

node_t *rs_system_find_node_by_name(char *name)
{
    rs_assert(rs_system != NULL);
    rs_assert(name != NULL);

    g_mutex_lock(rs_system->nodes_mutex);

    int i;
    node_t *node = NULL;
    for (i = 0; i < rs_system->node_count; i++) {
        if (!strcmp(phy_node_get_name(rs_system->node_list[i]), name)) {
            node = rs_system->node_list[i];
            break;
        }
    }

    g_mutex_unlock(rs_system->nodes_mutex);

    return node;
}

node_t *rs_system_find_node_by_mac_address(char *address)
{
    rs_assert(rs_system != NULL);
    rs_assert(address != NULL);

    g_mutex_lock(rs_system->nodes_mutex);

    int i;
    node_t *node = NULL;
    for (i = 0; i < rs_system->node_count; i++) {
        if (!strcmp(mac_node_get_address(rs_system->node_list[i]), address)) {
            node = rs_system->node_list[i];
            break;
        }
    }

    g_mutex_unlock(rs_system->nodes_mutex);

    return node;
}

node_t *rs_system_find_node_by_ip_address(char *address)
{
    rs_assert(rs_system != NULL);
    rs_assert(address != NULL);

    g_mutex_lock(rs_system->nodes_mutex);

    int i;
    node_t *node = NULL;
    for (i = 0; i < rs_system->node_count; i++) {
        if (!strcmp(ip_node_get_address(rs_system->node_list[i]), address)) {
            node = rs_system->node_list[i];
            break;
        }
    }

    g_mutex_unlock(rs_system->nodes_mutex);

    return node;
}

node_t ** rs_system_get_node_list(uint16 *node_count)
{
    rs_assert(rs_system != NULL);

    g_mutex_lock(rs_system->nodes_mutex);

    if (node_count != NULL) {
        *node_count = rs_system->node_count;
    }

    node_t **list = rs_system->node_list;

    g_mutex_unlock(rs_system->nodes_mutex);

    return list;
}

percent_t rs_system_get_link_quality(node_t *src_node, node_t *dst_node)
{
    rs_assert(rs_system != NULL);
    rs_assert(src_node != NULL);
    rs_assert(dst_node != NULL);

    coord_t distance = sqrt(pow(phy_node_get_x(src_node) - phy_node_get_x(dst_node), 2) + pow(phy_node_get_y(src_node) - phy_node_get_x(dst_node), 2));
    if (distance > rs_system_get_no_link_dist_thresh()) {
        distance = rs_system_get_no_link_dist_thresh();
    }

    percent_t dist_factor = (percent_t) (rs_system->no_link_dist_thresh - distance) / rs_system->no_link_dist_thresh;
    percent_t quality = phy_node_get_tx_power(src_node) * dist_factor;

    return quality;
}

bool rs_system_send_rpl_dis(node_t *src_node, node_t *dst_node)
{
    rs_assert(rs_system != NULL);
    rs_assert(src_node != NULL);
    rs_assert(dst_node != NULL);

    node_execute(src_node, "rpl_event_before_dis_pdu_sent", (node_schedule_func_t) rpl_event_before_dis_pdu_sent, NULL, TRUE);
    bool all_ok = rs_system_send_rpl(src_node, dst_node, NULL, ICMP_CODE_DIS);

    return all_ok;
}

bool rs_system_send_rpl_dio(node_t *src_node, node_t *dst_node, rpl_dio_pdu_t *pdu)
{
    rs_assert(rs_system != NULL);
    rs_assert(src_node != NULL);
    rs_assert(dst_node != NULL);

    node_execute(src_node, "rpl_event_before_dio_pdu_sent", (node_schedule_func_t) rpl_event_before_dio_pdu_sent, pdu, TRUE);
    bool all_ok = rs_system_send_rpl(src_node, dst_node, pdu, ICMP_CODE_DIO);

    return all_ok;
}

bool rs_system_send_rpl_dao(node_t *src_node, node_t *dst_node, rpl_dao_pdu_t *pdu)
{
    rs_assert(rs_system != NULL);
    rs_assert(src_node != NULL);
    rs_assert(dst_node != NULL);

    node_execute(src_node, "rpl_event_before_dao_pdu_sent", (node_schedule_func_t) rpl_event_before_dao_pdu_sent, pdu, TRUE);
    bool all_ok = rs_system_send_rpl(src_node, dst_node, pdu, ICMP_CODE_DAO);

    return all_ok;
}

bool rs_system_process_message(node_t *node, phy_pdu_t *message)
{
    rs_assert(rs_system != NULL);
    rs_assert(node != NULL);
    rs_assert(message != NULL);

    phy_pdu_t *phy_pdu = message;
    if (!node_execute(node, "phy_event_after_pdu_received", (node_schedule_func_t) phy_event_after_pdu_received, phy_pdu, TRUE)) {
        rs_error("failed to execute phy_event_after_pdu_received()");
        return FALSE;
    }

    mac_pdu_t *mac_pdu = phy_pdu->sdu;
    rs_assert(mac_pdu != NULL);
    if (!node_execute(node, "mac_event_after_pdu_received", (node_schedule_func_t) mac_event_after_pdu_received, mac_pdu, TRUE)) {
        rs_error("failed to execute mac_event_after_pdu_received()");
        return FALSE;
    }

    switch (mac_pdu->type) {

        case MAC_TYPE_IP : {
            ip_pdu_t *ip_pdu = mac_pdu->sdu;
            rs_assert(ip_pdu != NULL);
            if (!node_execute(node, "ip_event_after_pdu_received", (node_schedule_func_t) ip_event_after_pdu_received, ip_pdu, TRUE)) {
                rs_error("failed to execute ip_event_after_pdu_received()");
                return FALSE;
            }

            switch (ip_pdu->next_header) {

                case IP_NEXT_HEADER_ICMP: {
                    icmp_pdu_t *icmp_pdu = ip_pdu->sdu;
                    rs_assert(icmp_pdu != NULL);
                    if (!node_execute(node, "icmp_event_after_pdu_received", (node_schedule_func_t) icmp_event_after_pdu_received, icmp_pdu, TRUE)) {
                        rs_error("failed to execute icmp_event_after_pdu_received()");
                        return FALSE;
                    }

                    switch (icmp_pdu->type) {

                        case ICMP_TYPE_RPL:
                            switch (icmp_pdu->code) {

                                case ICMP_CODE_DIS: {
                                    rs_assert(icmp_pdu->sdu == NULL);
                                    if (!node_execute(node, "rpl_event_after_dis_pdu_received", (node_schedule_func_t) rpl_event_after_dis_pdu_received, NULL, TRUE)) {
                                        rs_error("failed to execute rpl_event_after_dis_pdu_received()");
                                        return FALSE;
                                    }

                                    break;
                                }

                                case ICMP_CODE_DIO: {
                                    rpl_dio_pdu_t *rpl_dio_pdu = icmp_pdu->sdu;
                                    rs_assert(rpl_dio_pdu != NULL);
                                    if (!node_execute(node, "rpl_event_after_dio_pdu_received", (node_schedule_func_t) rpl_event_after_dio_pdu_received, rpl_dio_pdu, TRUE)) {
                                        rs_error("failed to execute rpl_event_after_dio_pdu_received()");
                                        return FALSE;
                                    }

                                    break;
                                }

                                case ICMP_CODE_DAO: {
                                    rpl_dao_pdu_t *rpl_dao_pdu = icmp_pdu->sdu;
                                    rs_assert(rpl_dao_pdu != NULL);
                                    if (!node_execute(node, "rpl_event_after_dao_pdu_received", (node_schedule_func_t) rpl_event_after_dao_pdu_received, rpl_dao_pdu, TRUE)) {
                                        rs_error("failed to execute rpl_event_after_dao_pdu_received()");
                                        return FALSE;
                                    }

                                    break;
                                }

                                default:
                                    rs_error("unknown icmp code '0x%02X'", icmp_pdu->code);
                                    return FALSE;
                            }

                            break;

                        default:
                            rs_error("unknown icmp type '0x%02X'", icmp_pdu->type);
                            return FALSE;
                    }

                    break;
                }

                default:
                    rs_error("unknown ip next header '0x%04X'");
                    return FALSE;
            }

            break;
        }

        default:
            rs_error("unknown mac type '0x%04X'", mac_pdu->type);
            return FALSE;
    }

    return TRUE;
}


    /**** local functions ****/

static bool rs_system_mangle_message(node_t *src_node, node_t *dst_node, phy_pdu_t *message)
{
    rs_assert(rs_system != NULL);

    return TRUE;
}

static bool rs_system_send_rpl(node_t *src_node, node_t *dst_node, void *rpl_pdu, uint8 icmp_rpl_code)
{
    rs_assert(rs_system != NULL);
    rs_assert(src_node != NULL);
    rs_assert(dst_node != NULL);


    icmp_pdu_t *icmp_pdu = icmp_pdu_create();
    icmp_pdu_set_sdu(icmp_pdu, ICMP_TYPE_RPL, icmp_rpl_code, rpl_pdu);
    node_execute(src_node, "icmp_event_before_pdu_sent", (node_schedule_func_t) icmp_event_before_pdu_sent, icmp_pdu, TRUE);

    ip_pdu_t *ip_pdu = ip_pdu_create(ip_node_get_address(dst_node), ip_node_get_address(src_node));
    ip_pdu_set_sdu(ip_pdu, IP_NEXT_HEADER_ICMP, icmp_pdu);
    node_execute(src_node, "ip_event_before_pdu_sent", (node_schedule_func_t) ip_event_before_pdu_sent, ip_pdu, TRUE);

    mac_pdu_t *mac_pdu = mac_pdu_create(mac_node_get_address(dst_node), mac_node_get_address(src_node));
    mac_pdu_set_sdu(mac_pdu, MAC_TYPE_IP, ip_pdu);
    node_execute(src_node, "mac_event_before_pdu_sent", (node_schedule_func_t) mac_event_before_pdu_sent, mac_pdu, TRUE);

    phy_pdu_t *phy_pdu = phy_pdu_create();
    phy_pdu_set_sdu(phy_pdu, mac_pdu);
    node_execute(src_node, "phy_event_before_pdu_sent", (node_schedule_func_t) phy_event_before_pdu_sent, phy_pdu, TRUE);


    if (!rs_system_mangle_message(src_node, dst_node, phy_pdu)) {
        rs_error("failed to mangle message");
        return FALSE;
    }

    if (!node_enqueue_pdu(dst_node, phy_pdu, rs_system_get_transmit_mode())) {
        rs_error("failed to enqueue pdu");
        return FALSE;
    }

    return TRUE;
}
