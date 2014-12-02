#ifndef SIMFC_FCSERVER_H
#define SIMFC_FCSERVER_H

#include "fc_interface.h"
#include "WordSegmenter.h"
#include "Utils.h"
#include <time.h>
#include <ext/hash_map> 
#include "Configure.h"
#include "ub_conf.h"
#include "ub_server.h"
#include "ub_log.h"
#include "ul_thr.h"
#include "im_protocol.h"
#include "ubclient_include.h"
#include "ub_timer_task.h"

namespace __gnu_cxx {
    template<> struct hash<std::string> {
        size_t operator()(const std::string& s) const {
            return __stl_hash_string(s.c_str());
        }
    };
}

typedef std::string key;
typedef bsl::deque<bid_item_t>* value;
typedef bsl::deque<bid_item_t> value_container;
typedef __gnu_cxx::hash_map<key, value> hash_map;
typedef __gnu_cxx::hash_map<key, value>::iterator hash_map_iterator;

class FCServer {
public:
    static bool init();
    static bool run();

private:
    static bool init_bid_info();
    static bool init_server();
    static bool init_segmenter();
    static bool init_timer();

    static void insert_bid_item(const bid_item_t&);
    static void answer_search_query(thrd_data_t*);

    static int init_cb();
    static int search_cb();
    static int logger_timer(void *);

    static ul_logstat_t _logstat;
    static hash_map _bid_info_map;
    static ub_server_t* _serv;
    static ub_timer_task_t* _timer;
    static WordSegmenter* _segmenter;
    static comcfg::Configure _fc_conf;

    static u_int _pv_count;
    static u_int _pv_with_adres_count;
    static u_int _adres_num_count;
    static double _resp_total_tm;

    static bool _first_timer_flag;
};

#endif // SIMFC_FCSERVER_H
