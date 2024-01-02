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

#include "ThreadPool.h"

/// <summary>
/// 
/// </summary>
class StreamPacketsReader {
public:
    int m_outputStrIdx = -1; // ���ö������İ��������
    AVFormatContext* m_ctx = nullptr;  // ������������������
    queue<AVPacket*> m_packets;

    void pushSegement() {

    }
};

struct MediaRelation
{
    int srcStreamIndex = -1;
    AVMediaType type = AVMediaType::AVMEDIA_TYPE_UNKNOWN;
    AVStream* srcStream = nullptr;
    
};
/// <summary>
/// 
/// </summary>
bool GetNextPacket(AVStream*& srcStream, map<int, pair<AVStream*, int>> streamMap,
    AVFormatContext* context, AVPacket& avPacket);

/// <summary>
/// GetStreamMap
/// </summary>
/// _StreamMap     Դ����ţ�<Դ��ָ�룬Ŀ�������>
bool GetStreamMap(AVFormatContext* _srcContext,
    map<int, pair<AVStream*, int>>& _StreamMap,
    AVMediaType                     _type,
    AVFormatContext* _dstContext);

/// <summary>
/// 
/// </summary>
/// _StreamMap     Դ����ţ�<Դ��ָ�룬Ŀ�������>
bool SetNewStream(AVFormatContext* _dstContext,
    map<int, pair<AVStream*, int>>& _StreamMap);


/// <summary>
/// 
/// </summary>
class MediaFile {
private:
    queue<AVPacket*> m_packetCache; // ���ݰ��������

public:
    // <����ţ�<�������ݰ�>>
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
    AVMediaType m_mediaType = AVMediaType::AVMEDIA_TYPE_UNKNOWN;
    int m_mediaFrames = 0;
    int m_curFrameIdx = 0;
    char m_error[200];
    std::string m_file;
    AVFormatContext* m_ctx = nullptr;
    vector<StreamPacketsReader> m_reader;

    //queue<AVPacket*> m_queue;
    MediaFile(string file, AVMediaType _type);;
    ~MediaFile() {
        // �ͷ���Դ
        while (!m_packetCache.empty()) {
            AVPacket* i = m_packetCache.front();
            av_packet_unref(i);
            m_packetCache.pop();
        }
        if (m_ctx) {
            avformat_close_input(&m_ctx);
            m_ctx = nullptr;
        }
    };

    map<int, MediaRelation> m_streamInfo; // <�����������ţ�����������ָ��> ָ�����͵�������Ϣ

    AVPacket* FirstPack();
    /// <summary>
    /// 
    /// </summary>
    /// ��ȡ�߸����ͷ��ڴ�11
    /// <returns></returns>
    AVPacket* GetFirstPack();
    /// <summary>
    /// ��ý���ļ�����ȡָ�����͵�����Ϣ
    /// </summary>
    /// <returns></returns>
    bool Open(AVFormatContext* _dstCtx = nullptr);

    std::atomic<bool> m_hasReadAll = false;
    /// <summary>
    /// 
    /// </summary>
    /// ��̨����
    /// ��֧�ֶ��߳�
    /// <returns></returns>
    void PushCache();
    void GetAllPackets() {
        int sizeBuff = 0;
        int index = 0;

        vector<AVPacket*> packetsBuffer;
        // ԭʼ������
        set<int> srcStreIdx;
        for (const auto& stream : m_streamInfo) {
            srcStreIdx.insert(stream.second.srcStreamIndex);
        }
        // ��ȡһ�����ݰ�
        string msg = m_mediaType == AVMEDIA_TYPE_VIDEO ? "----��Ƶ��ȡ��" : "****��Ƶ��ȡ��";
        LogDebug() << msg.c_str();
        while (true)
        {
            AVPacket* avPacket = av_packet_alloc();
            int ret = 0;
            // ��ȡ
            try {
                ret = av_read_frame(m_ctx, avPacket);
                if (ret < 0) {
                    LogDebug() << "��ȡ���ݰ�����" << '\0';
                    break;
                }
                if (avPacket->dts < 0) {
                    LogDebug() << "����ʱ���С��0" << '\n';
                }
                if (avPacket->size <= 0) {
                    LogDebug() << "��ȡ�ļ�����" << '\n';
                    return;
                }
            }
            catch (...) {
                LogDebug() << "�������쳣" << '\n';
            }
            // ����
            for (const auto& info : m_streamInfo) {
                int outIdx = info.first;
                int srcIdx = info.second.srcStreamIndex;
                if (srcIdx == avPacket->stream_index) {
                    avPacket->stream_index = outIdx;
                    break;
                }
            }
            packetsBuffer.push_back(avPacket);
        }
        // ����
        //std::sort(packetsBuffer.begin(), packetsBuffer.end(), [](AVPacket* a, AVPacket* b) {
        //    return a->dts < b->dts;
        //    });
        // ����
        for (AVPacket* p : packetsBuffer) {
            m_packetCache.push(p);
        }
        LogDebug() << "����Ϊ����" << packetsBuffer.size() << '\n';

        std::vector<int> dtsVec;
        for (const auto& packet : packetsBuffer) {
            dtsVec.push_back(packet->dts);
        }
        std::vector<int> ptsVec;
        for (const auto& packet : packetsBuffer) {
            ptsVec.push_back(packet->pts);
        }

        //std::vector<int> ptsVec;
        //for (const auto& packet : packetsBuffer) {
        //    ptsVec.push_back(packet->pts);
        //    double realTime = av_q2d(timeBase) * packet.pts;
        //}

        std::vector<AVPacket*> keyPakVec;
        std::vector<int> keyPosVec;
        for (size_t i = 0; i < packetsBuffer.size(); i++) {
            AVPacket* curPkt = packetsBuffer[i];
            if (curPkt->flags & AV_PKT_FLAG_KEY) {
                keyPakVec.push_back(curPkt);
                keyPosVec.push_back(i);
            }
        }
        LogDebug() << "����Ϊ����" << packetsBuffer.size() << '\n';
    }
};