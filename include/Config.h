#ifndef SIMFC_CONFIG_H
#define SIMFC_CONFIG_H

#include "ub_conf.h" // for u_int

const u_int MAX_SEGMENT_CNT = 100;
const u_int SEGMENT_TERM_MAX_WORD_LEN = 30;

const u_int BID_INFO_MAX_LINE_LEN = 444;
const char BID_INFO_SEPARATOR = '\t';

const u_int SELECT_VALID_UNIT = 1000;

const u_int MAX_QUERY_LEN = 128;
const u_int MAX_FC_SRC_NUM = 256;
const u_int MAX_FC_ADV_NUM_PER_SRC = 1024;
const char REQ_LINE_SRC_SEPARATOR = '\t'; 
const char REQ_SRC_COL_SEPARATOR = '@'; 

#endif // SIMFC_CONFIG_H
