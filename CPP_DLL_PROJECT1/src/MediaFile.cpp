#include "MediaFile.h"

#include <iostream>
#include <algorithm>
using namespace std;

MediaFile::MediaFile(string file, AVMediaType _type) {
    m_file = file;
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
