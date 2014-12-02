#include "WordSegmenter.h"

WordSegmenter::WordSegmenter() {
    load_success = false;
    pwdict = NULL;
}

WordSegmenter::~WordSegmenter() {
    if (load_success) {
        scw_destroy_worddict(pwdict);
        pwdict = NULL;
    }
}

bool WordSegmenter::load(const char* conf_path, const char* worddict_path, int _segment_type) {
    if ((pgconf = scw_load_conf(conf_path)) == NULL) {
        std::cerr << "ERROR:: Load config file failed." << std::endl;
        return false;
    }
    if ((pwdict = scw_load_worddict(worddict_path)) == NULL) {
        std::cerr << "ERROR:: Load worddict failed." << std::endl;
        return false;
    }
    segment_type = _segment_type;
    load_success = true;
    return true;
}

void WordSegmenter::set_segment_type(int _segment_type) {
    segment_type = _segment_type;
}

bool WordSegmenter::segment(scw_out_t* pout, char *line, char *output, u_int item_len, u_int& segment_count) {
    if (segment_type == 0) {
        // type 0 : no segment
        snprintf(output, item_len, "%s", line);
        segment_count = 1;
        return true;
    }
    int len = strlen(line);
    while (line[len - 1] == '\r' || line[len - 1] == '\n') {
        line[--len] = 0;
    }

    if (scw_segment_words(pwdict, pout, line, len, LANGTYPE_SIMP_CHINESE, NULL) == -1) {
        std::cerr << "ERROR:: segment failed." << std::endl;
        return false;
    }

    if (segment_type == 1) {
        // Basic Word Sep Result
        segment_count = pout->wsbtermcount;
        for (u_int i = 0; i < pout->wsbtermcount; ++i) {
            u_int pos = GET_TERM_POS(pout->wsbtermpos[i]);
            u_int len = GET_TERM_LEN(pout->wsbtermpos[i]);
            snprintf(output + item_len * i, len + 1, "%s", pout->wordsepbuf + pos);
        }
    } else {
        // Phrase Word Sep Result
        segment_count = pout->wpbtermcount;
        for (u_int i = 0; i < pout->wpbtermcount; ++i) {
            u_int pos = GET_TERM_POS(pout->wpbtermpos[i]);
            u_int len = GET_TERM_LEN(pout->wpbtermpos[i]);
            snprintf(output + item_len * i, len + 1, "%s", pout->wpcompbuf + pos);
        }
    }

    return true;
}
