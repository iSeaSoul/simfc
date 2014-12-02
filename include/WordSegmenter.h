#ifndef SIMFC_WORDSEGMENTER_H
#define SIMFC_WORDSEGMENTER_H

#include "scwdef.h"

class WordSegmenter {
public:
    WordSegmenter();
    virtual ~WordSegmenter();

    bool load(const char*, const char*, int _segment_type = 0);
    void set_segment_type(int _segment_type);
    bool segment(scw_out_t*, char*, char*, u_int, u_int&);

private:
    scw_worddict_t* pwdict;
    bool load_success;
    int segment_type;
};

#endif // SIMFC_WORDSEGMENTER_H
