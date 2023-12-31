#include "MediaFile.h"

#include <iostream>
#include <algorithm>
using namespace std;



//queue<AVPacket*> m_queue;

MediaFile::MediaFile(string file, AVMediaType _type) {
    m_file = file;
    m_mediaType = _type;
}

AVPacket* MediaFile::FirstPack() {
    if (m_packetCache.empty()) {
        return nullptr;
    }
    return m_packetCache.front();
}

/// <summary>
/// 
/// </summary>
/// ��ȡ�߸����ͷ��ڴ�11
/// <returns></returns>
AVPacket* MediaFile::GetFirstPack() {
    if (m_packetCache.empty()) {
        LogDebug() << "������" << '\n';
        return nullptr;
    }

    AVPacket* pack = m_packetCache.front();
    if (pack->pts < 0) {
        int i = 0;
        i++;
    }
    m_packetCache.pop();
    m_curFrameIdx++;

    if (m_packetCache.empty()) {
        PushCache();
    }

    return pack;
}



/// <summary>
/// ��ý���ļ�����ȡָ�����͵�����Ϣ
/// </summary>
/// <returns></returns>
bool MediaFile::Open(AVFormatContext* _dstCtx) {
    // ���ļ�
    int ret = avformat_open_input(&m_ctx, m_file.c_str(), NULL, NULL);
    if (m_ctx == nullptr) {
        LogDebug() << "���ش���openʧ��" << '\n';//
    }
    if (ret != 0) {
        av_strerror(ret, m_error, 200);
        av_log(NULL, AV_LOG_WARNING, "error, ret=%d, msg=%s\n", ret, m_error);
        return -1;
    }

    // ����ָ��ý����
    avformat_find_stream_info(m_ctx, NULL);
    for (int i = 0; i < m_ctx->nb_streams; i++) {
        if (m_ctx->streams[i]->codecpar->codec_type == m_mediaType) {
            // ���Ƶ������
            AVStream* newStream = avformat_new_stream(_dstCtx, NULL);
            avcodec_parameters_copy(newStream->codecpar, m_ctx->streams[i]->codecpar);
            StreamPacketsReader m_reader1;
            // ��¼����
            int outputIdx = newStream->index;
            m_streamInfo[outputIdx].srcStream = m_ctx->streams[i];
            m_streamInfo[outputIdx].srcStreamIndex = i;

            m_reader1.m_ctx = m_ctx;
            m_reader1.m_outputStrIdx = outputIdx;
        }
    }

    GetAllPackets();
}


/// <summary>
/// 
/// </summary>
/// ��̨����
/// ��֧�ֶ��߳�
/// <returns></returns>
void MediaFile::PushCache() {
    int sizeBuff = 0;
    int index = 0;

    vector<AVPacket*> packetsBuffer;
    // ԭʼ������
    set<int> srcStreIdx;
    for (const auto &stream : m_streamInfo) {
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
                return;
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
        // ���� ��ȡ�����һ���ؼ�֡������
        if (avPacket->flags == AV_PKT_FLAG_KEY && m_mediaType == AVMEDIA_TYPE_VIDEO) {
            break;
        }
        // ������ ��Ƶ������150�����˳�
        else if (packetsBuffer.size() >= 150 && m_mediaType == AVMEDIA_TYPE_AUDIO) {
            break;
        }
    }
    // ����
    std::sort(packetsBuffer.begin(), packetsBuffer.end(), [](AVPacket* a, AVPacket*b) {
        return a->dts < b->dts;
    });
    // ����
    for (AVPacket* p : packetsBuffer) {
        m_packetCache.push(p);
    }
    LogDebug() << "����Ϊ����" << packetsBuffer.size() << '\n';
}

/// <summary>
/// 
/// </summary>
bool GetNextPacket(AVStream*& srcStream, map<int, pair<AVStream*, int>> streamMap, AVFormatContext* context, AVPacket& avPacket) {
    AVPacket* pack = nullptr;
    while (av_read_frame(context, &avPacket) == 0) {
        if (streamMap.find(avPacket.stream_index) != streamMap.end()) {
            pack = &avPacket;
            break;
        };
    };
    if (pack == nullptr) {
        srcStream = nullptr;
        return false;
    };
    srcStream = streamMap[avPacket.stream_index].first;
    return false;
}

/// <summary>
/// GetStreamMap
/// </summary>
/// _StreamMap     Դ����ţ�<Դ��ָ�룬Ŀ�������>
bool GetStreamMap(AVFormatContext* _srcContext, map<int, pair<AVStream*, int>>& _StreamMap, AVMediaType _type, AVFormatContext* _dstContext)
{
    if (_srcContext->nb_streams <= 0) {
        return false;
    }

    // ����������Ƶ��
    for (int i = 0; i < _srcContext->nb_streams; i++) {
        if (_srcContext->streams[i]->codecpar->codec_type == _type) {
            // Դ��
            AVStream* inStreamVideo = _srcContext->streams[i];
            _StreamMap[i].first = inStreamVideo;
            // �½�Ŀ����
            AVStream* outStream = avformat_new_stream(_dstContext, NULL);
            _StreamMap[i].second = outStream->index;
            // ����
            avcodec_parameters_copy(outStream->codecpar, inStreamVideo->codecpar);
            outStream->codecpar->codec_tag = 0;
        }
    }
}

/// <summary>
/// 
/// </summary>
/// _StreamMap     Դ����ţ�<Դ��ָ�룬Ŀ�������>
bool SetNewStream(AVFormatContext* _dstContext, map<int, pair<AVStream*, int>>& _StreamMap)
{
    for (auto i = _StreamMap.begin(); i != _StreamMap.end(); i++) {
        if (i->second.first == nullptr) {
            continue;
        }
        // �½�����Ŀ��������
        AVStream* dstStream = avformat_new_stream(_dstContext, NULL);
        i->second.second = dstStream->index;
        // ����Դ�����½�����
        avcodec_parameters_copy(dstStream->codecpar, i->second.first->codecpar);
        dstStream->codecpar->codec_tag = 0;
    }
    return true;
}
