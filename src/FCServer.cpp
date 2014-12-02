#include "FCServer.h"

bool FCServer::init() {
    return init_bid_info() &&
        init_server() &&
        init_segmenter() &&
        init_timer();
}

bool FCServer::run() {
    if (ub_run_timer_task(_timer) <= 0) {
        ul_writelog(UL_LOG_WARNING, "Run server timer failed.");
        return false;
    }

    if (ub_server_run(_serv) != 0) {
        ul_writelog(UL_LOG_WARNING, "Run server failed.");
        return false;
    }

    if (ub_join_timer_task(_timer) != 0) {
        ul_writelog(UL_LOG_WARNING, "Join server timer failed.");
        return false;
    }

    if (ub_server_join(_serv) != 0) {
        ul_writelog(UL_LOG_WARNING, "Join server failed.");
        return false;
    }
    return true;
}

bool FCServer::init_server() {
    signal(SIGPIPE, SIG_IGN);

    _logstat.events = 16;
    _logstat.to_syslog = 0;
    _logstat.spec = 0;
    ul_openlog("./log", "fcserver.", &_logstat, 18000000, NULL);

    ub_svr_t svr_info;
    ub_conf_data_t* conf = ub_conf_init("./conf/", "fcserver.conf");
    ub_conf_getsvr(conf, "im", "fc", &svr_info, "FCServer");

    _serv = ub_server_create();
    if (ub_server_load(_serv, &svr_info) != 0) {
        ul_writelog(UL_LOG_WARNING, "Load server info failed.");
        return false;
    }
    if (ub_server_setoptsock(_serv, UBSVR_NODELAY) != 0) {
        ul_writelog(UL_LOG_WARNING, "Set server optsock failed.");
        return false;
    }
    if (ub_server_set_buffer(_serv, NULL, 1 << 24, NULL, 1 << 24) != 0) {
        ul_writelog(UL_LOG_WARNING, "Set server buffer failed.");
        return false;
    }
    if (ub_server_set_user_data(_serv, NULL, sizeof(thrd_data_t)) != 0) {
        ul_writelog(UL_LOG_WARNING, "Set thread data for server failed.");
        return false;
    }
    if (ub_server_set_threadstartfun(_serv, FCServer::init_cb) != 0) {
        ul_writelog(UL_LOG_WARNING, "Set thread start function for server failed.");
        return false;
    }
    if (ub_server_set_callback(_serv, FCServer::search_cb) != 0) {
        ul_writelog(UL_LOG_WARNING, "Set callback for server failed.");
        return false;
    }
    ub_server_set_logprint(_serv, 0);
    return true;
}

int FCServer::logger_timer(void *) {
    if (_first_timer_flag) {
        ul_openlog_r("fc_logger", &_logstat, NULL);
    }
    _first_timer_flag = false;

    ul_writelog(UL_LOG_TRACE, "==== Timer Report (last 10s) ====");
    ul_writelog(UL_LOG_TRACE, "PV = %d", _pv_count);
    ul_writelog(UL_LOG_TRACE, "PVR = %lf", _pv_count == 0? 0 : 1.0 * _pv_with_adres_count / _pv_count);
    ul_writelog(UL_LOG_TRACE, "ASN = %lf", _pv_with_adres_count == 0? 0 : 1.0 * _adres_num_count / _pv_with_adres_count);
    ul_writelog(UL_LOG_TRACE, "Avg Resp Time = %lfms (%lf)", 
            _pv_count == 0? 0.0 : _resp_total_tm / _pv_count, 
            _pv_count == 0? 0.0 : 30000.0 / _pv_count);

    _pv_count = _pv_with_adres_count = _adres_num_count = 0;
    _resp_total_tm = 0.0;

    return 0;
}

bool FCServer::init_timer() {
    if ((_timer = ub_create_timer_task()) == NULL) {
        // std::cerr << "ERROR:: create timer failed." << std::endl;
        ul_writelog(UL_LOG_WARNING, "Create timer failed.");
        return false;
    }
    if (ub_add_timer_task(_timer, logger_timer, NULL, 10000) != 0) {
        // std::cerr << "ERROR:: add timer task failed." << std::endl;
        ul_writelog(UL_LOG_WARNING, "Add timer task failed.");
        return false;
    }
    return true;
}

bool FCServer::init_segmenter() {
    int load_conf_ret = _fc_conf.load("./conf/", "fcserver.conf");
    int wsegment_type = 1;
    const char *worddict_path = NULL;
    const char *conf_path = NULL;

    if (load_conf_ret != 0 || 
            _fc_conf["segment"]["wsegment_type"].selfType() == comcfg::CONFIG_ERROR_TYPE ||
            _fc_conf["segment"]["worddict_path"].selfType() == comcfg::CONFIG_ERROR_TYPE ||
            _fc_conf["segment"]["conf_path"].selfType() == comcfg::CONFIG_ERROR_TYPE) {
        // std::cerr << "WARNING:: Load configuration failed. Use Default instead." << std::endl;
        ul_writelog(UL_LOG_WARNING, "Segmenter configuration Error. Failed to load segmenter.");
        return false;
    } 
    wsegment_type = _fc_conf["segment"]["wsegment_type"].to_int32();
    worddict_path = _fc_conf["segment"]["worddict_path"].to_cstr();
    conf_path = _fc_conf["segment"]["conf_path"].to_cstr();
    // std::cerr << worddict_path << ' ' << conf_path << std::endl;
    std::cerr << "INFO:: Load configuration success. Segment type = " << wsegment_type << '.' << std::endl;


    if (!_segmenter->load(conf_path, worddict_path, wsegment_type)) {
        // std::cerr << "ERROR:: Segmenter Load failed." << std::endl;
        ul_writelog(UL_LOG_WARNING, "Segmenter load failed.");
        return false;
    }
    return true;
}

void parse_line(char *line, bid_item_t &bid_item) {
    bid_item.parse(line, BID_INFO_SEPARATOR);
    bid_item.calc_weight();
}

void FCServer::insert_bid_item(const bid_item_t& bid_info) {
    key term_str = std::string(bid_info.term);
    hash_map_iterator it = _bid_info_map.find(term_str);
    if (it == _bid_info_map.end()) {
        value v = new value_container();
        v->push_back(bid_info);
        _bid_info_map.insert(make_pair(term_str, v));
    } else {
        it->second->push_back(bid_info);
    }
}

bool FCServer::init_bid_info() {
    clocker_t clker;

    bid_item_t input_bid_item;
    char line[BID_INFO_MAX_LINE_LEN];

    int line_num = 0;

    while (true) {
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }
        if (++line_num % 1000000 == 0) {
            std::cerr << "INFO:: Reading line " << line_num << "..." << std::endl;
        }

        // if (line_num >= 2000000) {
        //     break;
        // }

        parse_line(line, input_bid_item);
        insert_bid_item(input_bid_item);
    }

    clker.showtime();

    std::cerr << "INFO:: Read bid info DONE. Sorting..." << std::endl;

    for (hash_map_iterator it = _bid_info_map.begin(); it != _bid_info_map.end(); ++it) {
        sort (it->second->begin(), it->second->end());
    }
    std::cerr << "INFO:: Sort bid info DONE." << std::endl;

    clker.showtime();
    return true;
}

void FCServer::answer_search_query(thrd_data_t* p_thrd_data) {
    const char *fc_str = p_thrd_data->fc_req->query();

    uint32_t src_id = 1;
    uint32_t src_req_num = 10;
    if (p_thrd_data->fc_req->fc_req_srcs_size() > 0) {
        src_id = p_thrd_data->fc_req->m_fc_req_srcs(0)->src_id();
        src_req_num = p_thrd_data->fc_req->m_fc_req_srcs(0)->src_req_num();
    }

    char segment_res[MAX_SEGMENT_CNT][SEGMENT_TERM_MAX_WORD_LEN];
    u_int scount;
    if (!_segmenter->segment(p_thrd_data->pout, 
            const_cast<char*>(fc_str), 
            segment_res[0], 
            SEGMENT_TERM_MAX_WORD_LEN, 
            scount)) {
        // std::cerr << "ERROR:: segment query string failed" << std::endl;
        ul_writelog(UL_LOG_WARNING, "Segment query string failed.");
    }

    // std::cerr << "INFO:: Query " << fc_str << " : " << std::endl;

    value combined_value = new value_container();
    for (u_int sct = 0; sct < scount; ++sct) {
        // std::cerr << "Segment Res " << sct << " " << segment_res[sct] << std::endl;
        hash_map_iterator it = _bid_info_map.find(std::string(segment_res[sct]));
        if (it == _bid_info_map.end()) {
            continue;
        }
        for (u_int i = 0; i < src_req_num && i < (u_int)(it->second->size()); ++i) {
            combined_value->push_back((*(it->second))[i]);
        }
    }
    sort (combined_value->begin(), combined_value->end());

    p_thrd_data->fc_res->m_res_srcs(0)->set_src_id(0);
    u_int req_num = std::min(src_req_num, (u_int)(combined_value->size()));
    for (u_int i = 0; i < req_num; ++i) {
        // std::cerr << (*(combined_value))[i].title << std::endl;
        p_thrd_data->fc_res->m_res_srcs(0)->set_src_id(src_id);
        (*(combined_value))[i].assign_fc_val(p_thrd_data->fc_res->m_res_srcs(0)->m_res_advs(i));
    }

    _pv_count += 1;
    if (req_num > 0) {
        _pv_with_adres_count += 1;
        _adres_num_count += req_num;
    }
}

std::string join_res_string(thrd_data_t* p_thrd_data) {
    string ret = "";
    for (u_int i = 0; i < p_thrd_data->fc_res->m_res_srcs(0)->res_advs_size(); ++i) {
        if (i != 0) ret += " ";
        ret += p_thrd_data->fc_res->m_res_srcs(0)->m_res_advs(i)->title();
    }
    return ret;
}

int FCServer::init_cb() {
    ul_openlog_r("fc_work", &_logstat, NULL);
    
    thrd_data_t* p_thrd_data = (thrd_data_t*)ub_server_get_user_data();
    p_thrd_data->p_mempool = new bsl::xcompool();
    p_thrd_data->p_mempool->create(1 << 24);

    u_int scw_out_flag = SCW_OUT_ALL | SCW_OUT_PROP;
    if ((p_thrd_data->pout = scw_create_out(10000, scw_out_flag)) == NULL) {
        std::cerr << "ERROR:: Init the output buffer error." << std::endl;
        return -1;
    }

    return 0;
}

int FCServer::search_cb() {
    char* req_buf = (char*)ub_server_get_read_buf();
    uint32_t req_buf_size = ub_server_get_read_size();
    char* res_buf = (char*)ub_server_get_write_buf();
    uint32_t res_buf_size = ub_server_get_write_size();

    if (req_buf == NULL || res_buf == NULL) {
        ul_writelog(UL_LOG_WARNING, "Getting buffer for reader or writer failed!");
        return -1;
    }
    if (req_buf_size == 0 || res_buf_size == 0) {
        ul_writelog(UL_LOG_WARNING, "Getting buffer for reader or writer failed!");
        return -1;
    }

    thrd_data_t* p_thrd_data = (thrd_data_t*)ub_server_get_user_data();
    p_thrd_data->thrd_clocker = new clocker_t();
    
    p_thrd_data->p_mempool->clear();
    p_thrd_data->fc_req = fc_interface::fc_req_t::create(p_thrd_data->p_mempool);
    p_thrd_data->fc_res = fc_interface::fc_res_t::create(p_thrd_data->p_mempool);

    itp::req_reader_t<fc_interface::fc_req_t>* req_reader =
                itp::req_reader_t<fc_interface::fc_req_t>::create(p_thrd_data->p_mempool);
    itp::res_writer_t<fc_interface::fc_res_t>* res_writer =
                itp::res_writer_t<fc_interface::fc_res_t>::create(p_thrd_data->p_mempool);

    req_reader->load(req_buf, req_buf_size);
    req_reader->unpack_user_data(p_thrd_data->fc_req);

    answer_search_query(p_thrd_data);

    res_writer->set_status(itp::OK);
    res_writer->set_user_data(p_thrd_data->fc_res);
    res_writer->pack_data(res_buf, res_buf_size);

    double used_time = p_thrd_data->thrd_clocker->get_used_time();
    ul_writelog(UL_LOG_NOTICE, "Query string = %s, resp_tm = %lfms, resp_str = %s",
        p_thrd_data->fc_req->query(), used_time, join_res_string(p_thrd_data).c_str());

    // std::cerr << "INFO:: Query " << p_thrd_data->fc_req->query() << " Done. ";
    // p_thrd_data->thrd_clocker->showtime();
    _resp_total_tm += used_time;
    
    return 0;
}
