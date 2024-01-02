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
/// 获取者负责释放内存11
/// <returns></returns>
AVPacket* MediaFile::GetFirstPack() {
    if (m_packetCache.empty()) {
        LogDebug() << "无数据" << '\n';
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

void MediaFile::GetDatas()
{
    // 打开文件
    int ret = avformat_open_input(&m_ctx, m_file.c_str(), NULL, NULL);
    if (m_ctx == nullptr) {
        LogDebug() << "严重错误，open失败" << '\n';//
    }
    if (ret != 0) {
        av_strerror(ret, m_error, 200);
        av_log(NULL, AV_LOG_WARNING, "error, ret=%d, msg=%s\n", ret, m_error);
        return;
    }

    // 获取流信息到上下文结构体
    AVDictionary* info = nullptr;
    av_dict_set(&info, "nb_streams", 0, 0);
    if (avformat_find_stream_info(m_ctx, &info) < 0) {
        return;
    };

    // 记录所有目标流
    for (int i = 0; i < m_ctx->nb_streams; i++) {
        m_datas[i].first = m_ctx->streams[i];
    }

    // 读取所有数据包
    while (true) {
        AVPacket* avPacket = av_packet_alloc();
        int ret = 0;
        // 读取
        try {
            ret = av_read_frame(m_ctx, avPacket);
            if (ret == AVERROR_EOF) {
                LogDebug() << "读取文件结束" << '\n';
                break;
            }
            if (ret < 0) {
                LogDebug() << "读取数据包错误" << '\0';
                break;
            }
        }
        catch (...) {
            LogDebug() << "触发了异常" << '\n';
            LogDebug() << "读取文件结束" << '\n';
            break;
        }
        // 缓存
        m_datas.at(avPacket->stream_index).second.push_back(avPacket);
    }
}

/// <summary>
/// 打开媒体文件，获取指定类型的流信息
/// </summary>
/// <returns></returns>
bool MediaFile::Open(AVFormatContext* _dstCtx) {
    // 打开文件
    int ret = avformat_open_input(&m_ctx, m_file.c_str(), NULL, NULL);
    if (m_ctx == nullptr) {
        LogDebug() << "严重错误，open失败" << '\n';//
    }
    if (ret != 0) {
        av_strerror(ret, m_error, 200);
        av_log(NULL, AV_LOG_WARNING, "error, ret=%d, msg=%s\n", ret, m_error);
        return -1;
    }

    // 遍历指定媒体流
    AVDictionary* info = nullptr;
    av_dict_set(&info, "nb_streams", 0, 0);
    if (avformat_find_stream_info(m_ctx, &info) < 0) {
        return -1;
    };
    for (int i = 0; i < m_ctx->nb_streams; i++) {
        if (m_ctx->streams[i]->codecpar->codec_type == m_mediaType) {
            // 复制到输出流
            AVStream* outStream = avformat_new_stream(_dstCtx, NULL);
            AVStream* inputStr = m_ctx->streams[i];
            avcodec_parameters_copy(outStream->codecpar, m_ctx->streams[i]->codecpar);
            {
                m_copyStream(inputStr, outStream);
            }

            StreamPacketsReader m_reader1;
            // 记录索引
            int outputIdx = outStream->index;
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
/// 后台运行
/// 不支持多线程
/// <returns></returns>
void MediaFile::PushCache() {
    int sizeBuff = 0;
    int index = 0;

    vector<AVPacket*> packetsBuffer;

    {
        struct CustomDeleter {
            void operator()(int* ptr) {
                std::cout << "Custom deleter is called!" << std::endl;
                delete ptr;
            }
        };

        function<void(AVPacket*)> freeAvpkt = [](AVPacket* ptr) {
            //av_packet_ref
            av_packet_unref(ptr);
        };

        std::unique_ptr<int, CustomDeleter> customPtr(new int, CustomDeleter());
        std::unique_ptr<AVPacket, function<void(AVPacket*)>> customPtr1(packetsBuffer[0], freeAvpkt);
    }
    // 原始流索引
    set<int> srcStreIdx;
    for (const auto &stream : m_streamInfo) {
        srcStreIdx.insert(stream.second.srcStreamIndex);
    }
    // 读取一段数据包
    string msg = m_mediaType == AVMEDIA_TYPE_VIDEO ? "----视频读取，" : "****音频读取，";
    LogDebug() << msg.c_str();
    while (true)
    {
        AVPacket* avPacket = av_packet_alloc();
        int ret = 0;
        // 读取
        try {
            ret = av_read_frame(m_ctx, avPacket);
            if (ret < 0) {
                LogDebug() << "读取数据包错误" << '\0';
                return;
            }
            if (avPacket->dts < 0) {
                LogDebug() << "解码时间戳小于0" << '\n';
            }
            if (avPacket->size <= 0) {
                LogDebug() << "读取文件结束" << '\n';
                return;
            }
        }
        catch (...) {
            LogDebug() << "触发了异常" << '\n';
        }
        // 缓存
        for (const auto& info : m_streamInfo) {
            int outIdx = info.first;
            int srcIdx = info.second.srcStreamIndex;
            if (srcIdx == avPacket->stream_index) {
                avPacket->stream_index = outIdx;
                break;
            }
        }
        packetsBuffer.push_back(avPacket);
        // 跳出 读取完最近一个关键帧，跳出
        if (avPacket->flags == AV_PKT_FLAG_KEY && m_mediaType == AVMEDIA_TYPE_VIDEO) {
            break;
        }
        // 跳出， 音频流读满150个，退出
        else if (packetsBuffer.size() >= 150 && m_mediaType == AVMEDIA_TYPE_AUDIO) {
            break;
        }
    }
    // 排序
    std::sort(packetsBuffer.begin(), packetsBuffer.end(), [](AVPacket* a, AVPacket*b) {
        return a->dts < b->dts;
    });
    // 出参
    for (AVPacket* p : packetsBuffer) {
        m_packetCache.push(p);
    }
    LogDebug() << "长度为：：" << packetsBuffer.size() << '\n';
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
/// _StreamMap     源流序号，<源流指针，目标流序号>
bool GetStreamMap(AVFormatContext* _srcContext, map<int, pair<AVStream*, int>>& _StreamMap, AVMediaType _type, AVFormatContext* _dstContext)
{
    if (_srcContext->nb_streams <= 0) {
        return false;
    }

    // 遍历拷贝视频流
    for (int i = 0; i < _srcContext->nb_streams; i++) {
        if (_srcContext->streams[i]->codecpar->codec_type == _type) {
            // 源流
            AVStream* inStreamVideo = _srcContext->streams[i];
            _StreamMap[i].first = inStreamVideo;
            // 新建目标流
            AVStream* outStream = avformat_new_stream(_dstContext, NULL);
            _StreamMap[i].second = outStream->index;
            // 复制
            avcodec_parameters_copy(outStream->codecpar, inStreamVideo->codecpar);
            outStream->codecpar->codec_tag = 0;
        }
    }
}

/// <summary>
/// 
/// </summary>
/// _StreamMap     源流序号，<源流指针，目标流序号>
bool SetNewStream(AVFormatContext* _dstContext, map<int, pair<AVStream*, int>>& _StreamMap)
{
    for (auto i = _StreamMap.begin(); i != _StreamMap.end(); i++) {
        if (i->second.first == nullptr) {
            continue;
        }
        // 新建流到目标上下文
        AVStream* dstStream = avformat_new_stream(_dstContext, NULL);
        i->second.second = dstStream->index;
        // 拷贝源流到新建的流
        avcodec_parameters_copy(dstStream->codecpar, i->second.first->codecpar);
        dstStream->codecpar->codec_tag = 0;
    }
    return true;
}
