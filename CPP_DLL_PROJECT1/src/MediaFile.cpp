#include "MediaFile.h"

#include <iostream>
#include <algorithm>
using namespace std;

MediaFile::MediaFile(string file, AVMediaType _type) {
    m_file = file;
}


void MediaFile::GetDatas()
{
    // ���ļ�
    int ret = avformat_open_input(&m_ctx, m_file.c_str(), NULL, NULL);
    if (m_ctx == nullptr) {
        LogDebug() << "���ش���openʧ��" << '\n';//
    }
    if (ret != 0) {
        av_strerror(ret, m_error, 200);
        av_log(NULL, AV_LOG_WARNING, "error, ret=%d, msg=%s\n", ret, m_error);
        return;
    }

    // ��ȡ����Ϣ�������Ľṹ��
    AVDictionary* info = nullptr;
    av_dict_set(&info, "nb_streams", 0, 0);
    if (avformat_find_stream_info(m_ctx, &info) < 0) {
        return;
    };

    // ��¼����Ŀ����
    for (int i = 0; i < m_ctx->nb_streams; i++) {
        m_datas[i].first = m_ctx->streams[i];
    }

    // ��ȡ�������ݰ�
    while (true) {
        AVPacket* avPacket = av_packet_alloc();
        int ret = 0;
        // ��ȡ
        try {
            ret = av_read_frame(m_ctx, avPacket);
            if (ret == AVERROR_EOF) {
                LogDebug() << "��ȡ�ļ�����" << '\n';
                break;
            }
            if (ret < 0) {
                LogDebug() << "��ȡ���ݰ�����" << '\0';
                break;
            }
        }
        catch (...) {
            LogDebug() << "�������쳣" << '\n';
            LogDebug() << "��ȡ�ļ�����" << '\n';
            break;
        }
        // ����
        m_datas.at(avPacket->stream_index).second.push_back(avPacket);
    }
}
