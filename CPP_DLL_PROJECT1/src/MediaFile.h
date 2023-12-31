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
    /// <summary>
    /// ��������Ŀ����������ȥ��
    /// </summary>
    /// <param name="_dstCtx"></param>
    /// <returns></returns>
    bool CopyStreamTo(AVFormatContext* _dstCtx);

    std::atomic<bool> m_hasReadAll = false;
    /// <summary>
    /// 
    /// </summary>
    /// ��̨����
    /// ��֧�ֶ��߳�
    /// <returns></returns>
    void PushCache();
    void GetAllPackets() {

        // ����ָ��ý����
        //avformat_find_stream_info(m_ctx, NULL);
        //for (int i = 0; i < m_ctx->nb_streams; i++) {
        //    if (m_ctx->streams[i]->codecpar->codec_type == m_mediaType) {
        //        // ���Ƶ������
        //        AVStream* newStream = avformat_new_stream(_dstCtx, NULL);
        //        avcodec_parameters_copy(newStream->codecpar, m_ctx->streams[i]->codecpar);
        //        StreamPacketsReader m_reader1;
        //        // ��¼����
        //        int outputIdx = newStream->index;
        //        m_streamInfo[outputIdx].srcStream = m_ctx->streams[i];
        //        m_streamInfo[outputIdx].srcStreamIndex = i;

        //        m_reader1.m_ctx = m_ctx;
        //        m_reader1.m_outputStrIdx = outputIdx;
        //    }
        //}
        
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
            //// ���� ��ȡ�����һ���ؼ�֡������
            //if (avPacket->flags == AV_PKT_FLAG_KEY && m_mediaType == AVMEDIA_TYPE_VIDEO) {
            //    break;
            //}
            //// ������ ��Ƶ������150�����˳�
            //else if (packetsBuffer.size() >= 150 && m_mediaType == AVMEDIA_TYPE_AUDIO) {
            //    break;
            //}
        }
        // ����
        std::sort(packetsBuffer.begin(), packetsBuffer.end(), [](AVPacket* a, AVPacket* b) {
            return a->dts < b->dts;
            });
        // ����
        for (AVPacket* p : packetsBuffer) {
            m_packetCache.push(p);
        }
        LogDebug() << "����Ϊ����" << packetsBuffer.size() << '\n';
    }
};