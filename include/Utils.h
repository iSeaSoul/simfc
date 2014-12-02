#ifndef SIMFC_UTILS_H
#define SIMFC_UTILS_H

#include "Config.h"
#include <string.h>
#include <time.h>
#include "scwdef.h"
#include "ub_conf.h"
#include "ub_server.h"
#include "ub_log.h"
#include "ul_thr.h"
#include "im_protocol.h"
#include "ubclient_include.h"
#include "fc_interface.h"

struct clocker_t {
    timeval begin;

    clocker_t() {
        gettimeofday(&begin, NULL);
    }
    void init() {
        gettimeofday(&begin, NULL);
    }
    void showtime() {
        std::cerr << "Used Time = " <<  get_used_time() << "ms.\n";
    }
    double get_used_time() {
        timeval now;
        gettimeofday(&now, NULL);
        double used_time = (now.tv_usec - begin.tv_usec) / 1000.0 +
            (now.tv_sec - begin.tv_sec) * 1000.0;
        return used_time;
    }
};

struct thrd_data_t {
    fc_interface::fc_req_t *fc_req;
    fc_interface::fc_res_t *fc_res;
    bsl::xcompool *p_mempool;
    scw_out_t* pout;
    clocker_t* thrd_clocker;
};

struct bid_item_t {
    std::string term, title, desc1, desc2, targeturl, showurl;
    int winfoid, weight, bid, q;

    void parse(char *line, char sep);
    void assign_fc_val(fc_interface::fc_res_adv_t* res_adv);

    void calc_weight();

    bool operator<(const bid_item_t& rhs) const {
        return weight > rhs.weight;
    }
};

class file_reader_t {
public:
    file_reader_t() {
        ul_pthread_mutex_init(&_mutex, NULL);
    }
    ~file_reader_t() {
        ul_pthread_mutex_destroy(&_mutex);
    }

    int init(int speed);
    char* get_line(char* line, int size);

private:
    long _waitingtm;
    long _interval; 
    timeval _lasttm;

    pthread_mutex_t _mutex;
};

#endif // SIMFC_UTILS_H
