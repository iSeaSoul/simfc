#include "FCServer.h"

ul_logstat_t FCServer::_logstat;
hash_map FCServer::_bid_info_map;
ub_server_t* FCServer::_serv = NULL;
ub_timer_task_t* FCServer::_timer = NULL;
WordSegmenter* FCServer::_segmenter = new WordSegmenter();
comcfg::Configure FCServer::_fc_conf;

u_int FCServer::_pv_count = 0;
u_int FCServer::_pv_with_adres_count = 0;
u_int FCServer::_adres_num_count = 0;
double FCServer::_resp_total_tm = 0;
bool FCServer::_first_timer_flag = true;

int main() {
    if (FCServer::init()) {
        FCServer::run();
    }

    return 0;
}
