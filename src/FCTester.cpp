#include "Utils.h"
#include <iostream>
#include "ub_log.h"
#include "ul_thr.h"
#include "ub_conf.h"
#include "ubclient_include.h"
#include "im_protocol.h"
#include "fc_interface.h"

ul_logstat_t g_logstat;
file_reader_t g_file_reader;
ub::UbClientManager g_ubc;

struct req_src_t {
    uint32_t src_id;
    uint32_t src_req_num;
};

struct req_t {
    char query[MAX_QUERY_LEN];
    req_src_t req_srcs[MAX_FC_SRC_NUM];
    uint32_t req_src_num;
};

int split(char *input, char sep, char *output, uint32_t item_size, uint32_t item_len) {
    char *cur = input;
    char *tail = NULL;
    int size = 0;
    while ((tail = strchr(cur, sep)) != NULL) {
        *tail = '\0';
        snprintf(output + item_len * size, item_len, "%s", cur);
        cur = tail + 1;
        size++;
    }
    snprintf(output + item_len * size, item_len, "%s", cur);
    return size + 1;
}

int parse_line(char *line, req_t &req) {
    char buf[MAX_FC_SRC_NUM + 1][MAX_QUERY_LEN];
    int num = split(line, REQ_LINE_SRC_SEPARATOR, buf[0], MAX_FC_SRC_NUM + 1, MAX_QUERY_LEN);
    snprintf(req.query, sizeof(req.query), "%s", buf[0]);
    req.req_src_num = num - 1;
    for (int i = 0; i < req.req_src_num; ++i) {
        char col_buf[2][MAX_QUERY_LEN];
        split(buf[i + 1], REQ_SRC_COL_SEPARATOR, col_buf[0], 2, MAX_QUERY_LEN);
        req.req_srcs[i].src_id = atoi(col_buf[0]);
        req.req_srcs[i].src_req_num= atoi(col_buf[1]);
    }
    return 0;
}

int output_res(fc_interface::fc_res_t *fc_res) {
    for (u_int i = 0; i < fc_res->res_srcs_size(); ++i) {
        fc_interface::fc_res_src_t *fc_res_src = fc_res->m_res_srcs(i);
        std::cout << "--------------------------------src[" << i << "] src_id:" << fc_res_src->src_id()
             << " --------------------------------" << std::endl;
        for (u_int j = 0; j < fc_res_src->res_advs_size();++j) {
            fc_interface::fc_res_adv_t *fc_res_adv = fc_res_src->m_res_advs(j);
            std::cout << "adv[" << j << "] winfo_id: " << fc_res_adv->winfo_id()
                         << " bid: " << fc_res_adv->bid() << std::endl;
            std::cout << fc_res_adv->title() << std::endl;
            std::cout << fc_res_adv->targeturl() << std::endl;
            std::cout << fc_res_adv->desc1() << std::endl;
            std::cout << fc_res_adv->desc2() << std::endl;
            std::cout << fc_res_adv->showurl() << std::endl;
        }
    }
}

void *fc_tester_work(void *arg) {
    ul_openlog_r("fc_tester_work", &g_logstat, NULL);

    thrd_data_t thrd_data;
    thrd_data.p_mempool = new bsl::xcompool();
    thrd_data.p_mempool->create(1 << 24);

    ub::nshead_talkwith_t currTalk;
    currTalk.reqbuf = new char[1 << 24];
    currTalk.resbuf = new char[1 << 24];
    currTalk.maxreslen = 1 << 24;
    currTalk.defaultserverarg.key = -1;

    char line[1024];

    while (true) {
        if (g_file_reader.get_line(line, sizeof(line)) == NULL) {
            break;
        }
        
        req_t input_req;
        parse_line(line, input_req);
        
        thrd_data.p_mempool->clear();
        
        thrd_data.fc_req = fc_interface::fc_req_t::create(thrd_data.p_mempool);
        thrd_data.fc_res = fc_interface::fc_res_t::create(thrd_data.p_mempool);
        
        thrd_data.fc_req->set_query(input_req.query);
        for (u_int i = 0; i < input_req.req_src_num; ++i) {
            thrd_data.fc_req->m_fc_req_srcs(i)->set_src_id(input_req.req_srcs[i].src_id);
            thrd_data.fc_req->m_fc_req_srcs(i)->set_src_req_num(input_req.req_srcs[i].src_req_num);
        }
        
        itp::req_writer_t<fc_interface::fc_req_t>* req_writer =
                itp::req_writer_t<fc_interface::fc_req_t>::create(thrd_data.p_mempool);
        itp::res_reader_t<fc_interface::fc_res_t>* res_reader =
                itp::res_reader_t<fc_interface::fc_res_t>::create(thrd_data.p_mempool);
        
        req_writer->set_user_data(thrd_data.fc_req);
        req_writer->pack_data(&currTalk.reqhead, currTalk.reqbuf, 1 << 24);
        
        g_ubc.nshead_singletalk("fcserver", &currTalk);
        res_reader->load(&currTalk.reshead, currTalk.resbuf, currTalk.reshead.body_len);
        res_reader->unpack_user_data(thrd_data.fc_res);
        
        // output_res(thrd_data.fc_res);
    }

    delete []currTalk.reqbuf;
    delete []currTalk.resbuf;
    return NULL;
}

int main(int /*argc*/, char ** /*argv*/) {
    g_logstat.events = 4;
    g_logstat.to_syslog = 0;
    g_logstat.spec = 0;
    ul_openlog("./log", "fctesterbatch.", &g_logstat, 18000000, NULL);
    g_ubc.init("conf/", "fctester.conf");

    int thread_num = 30;
    pthread_t* thread = (pthread_t*)malloc(sizeof(pthread_t*) * thread_num);
    for (int i = 0; i < thread_num; i++) {
        ul_pthread_create(&thread[i], NULL, fc_tester_work, NULL);
    }

    for (int i = 0; i < thread_num; i++) {
        ul_pthread_join(thread[i], NULL);
    }

    free(thread);
    g_ubc.cancelAll();
    g_ubc.close();

    return 0;
}
