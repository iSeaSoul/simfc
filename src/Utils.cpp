#include "Utils.h"

void bid_item_t::parse(char *line, char sep) {
    std::string winfoidS, bidS, qS;
    std::string *item_name[9] = {
        &winfoidS, &term, &bidS, &qS, &title, &desc1, &desc2, &targeturl, &showurl
    };

    char *cur = line;
    char *tail = NULL;
    int size = 0;
    while ((tail = strchr(cur, sep)) != NULL) {
        *tail = '\0';
        *(item_name[size]) = std::string(cur);
        cur = tail + 1;
        size++;
    }
    *(item_name[size]) = std::string(cur);

    winfoid = atoi(winfoidS.c_str());
    bid = atoi(bidS.c_str());
    q = atoi(qS.c_str());
}

void bid_item_t::assign_fc_val(fc_interface::fc_res_adv_t* res_adv) {
    res_adv->set_winfo_id(winfoid);
    res_adv->set_bid(bid);
    res_adv->set_q(q);

    res_adv->set_title(title.c_str());
    res_adv->set_desc1(desc1.c_str());
    res_adv->set_desc2(desc2.c_str());
    res_adv->set_targeturl(targeturl.c_str());
    res_adv->set_showurl(showurl.c_str());
}

void bid_item_t::calc_weight() {
    weight = bid * q;
}

int file_reader_t::init(int speed) {
    _lasttm.tv_sec = _lasttm.tv_usec = 0;
    _waitingtm = 0;
    if (speed == 0) {
        _interval = 0;
    } else if (speed < 1) {
        _interval = 1000 * 1000;
    } else {
        _interval = 1000 * 1000 / speed;
    }
    return 0;
}

char* file_reader_t::get_line(char* line, int size) {
    ul_pthread_mutex_lock(&_mutex);

    if (_interval != 0) {
        if (_lasttm.tv_sec == 0 && _lasttm.tv_usec == 0) {
            gettimeofday(&_lasttm, NULL);
        } else {
            timeval curtm;
            gettimeofday(&curtm, NULL);
            long elapse = ((curtm.tv_sec - _lasttm.tv_sec) * 1000*1000 + 
                (curtm.tv_usec - _lasttm.tv_usec));
            _waitingtm += _interval - elapse;
            _lasttm = curtm;
            WRITE_LOG(UL_LOG_DEBUG, "time_elapse: %ld, waiting_time:%ld", elapse, _waitingtm);

            if (_waitingtm >= SELECT_VALID_UNIT) {
                long tm_unit = _waitingtm / SELECT_VALID_UNIT * SELECT_VALID_UNIT;
                timeval tmp_tm = {tm_unit / 1000000, tm_unit % 1000000};
                select(1, NULL, NULL, NULL, &tmp_tm);
            } else if (_waitingtm <= SELECT_VALID_UNIT * (-2)) {
                _waitingtm = -SELECT_VALID_UNIT;
            }
        }
    }

    char* ret = fgets(line, size, stdin);
    ul_pthread_mutex_unlock(&_mutex);
    return ret;
}
