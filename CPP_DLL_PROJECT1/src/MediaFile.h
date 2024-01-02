#pragma once
extern "C" {
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>
#include<libavformat/avio.h>
#include<libavutil/avutil.h>
}

#include <map>
#include <set>
#include <mutex>
#include <shared_mutex>
#include <queue>
#include <atomic>
using namespace std;

#include "Logger.h"

/// <summary>
/// 
/// </summary>
class MediaFile {
public:
    // <流序号，<流，数据包>>
    std::map<int, std::pair<AVStream*, std::vector<AVPacket*>>> m_datas;
    void GetDatas();
    function<void(AVStream*, AVStream*)> m_copyStream = [](AVStream* src, AVStream* dst) {
        dst->discard = src->discard;
        dst->id = src->id;
        dst->time_base = src->time_base;
        dst->start_time = src->start_time;
        dst->duration = src->duration;
        dst->nb_frames = src->nb_frames;
        dst->disposition = src->disposition;
        dst->sample_aspect_ratio = src->sample_aspect_ratio;
        dst->event_flags = src->event_flags;
        dst->r_frame_rate = src->r_frame_rate;
        dst->pts_wrap_bits = src->pts_wrap_bits;
    };
public:
    char m_error[200];
    std::string m_file;
    AVFormatContext* m_ctx = nullptr;

    MediaFile(string file, AVMediaType _type);;
    ~MediaFile() {
        if (m_ctx) {
            avformat_close_input(&m_ctx);
            m_ctx = nullptr;
        }
    };
};