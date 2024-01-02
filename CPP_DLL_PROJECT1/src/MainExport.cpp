#include "MainExport.h"
#include <ThreadPool.h>
#include "MediaFile.h"

void CopyStream(AVStream* src, AVStream* dst) {
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

bool TestExample::testMainFunc(std::string vedioFile, std::string audioFile, std::string outputPath)
{
    MediaFile vedio(vedioFile, AVMEDIA_TYPE_VIDEO);
    vedio.GetDatas();
    MediaFile audio(audioFile, AVMEDIA_TYPE_AUDIO);
    audio.GetDatas();
    AVFormatContext* m_outFormatContext = NULL;  // 输出上下文
    int error = avformat_alloc_output_context2(&m_outFormatContext, nullptr, nullptr, outputPath.c_str());
    char errors[1024] = { '\0' };

    // <输出流序号，<输入流，数据包>>
    map<int, pair<AVStream*, vector<AVPacket*>>> inputDatas;
    vector<MediaFile*> files{&vedio, &audio};
    for (auto file : files) {
        for (auto iter = file->m_datas.begin(); iter != file->m_datas.end(); iter++) {
            // params
            AVStream* inStream = iter->second.first;
            vector<AVPacket*> pkts = iter->second.second;
            AVStream* outStream = avformat_new_stream(m_outFormatContext, NULL);
            // copy stream info
            avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
            CopyStream(inStream, outStream);
            // collect datas
            inputDatas[outStream->index].first = inStream;
            inputDatas[outStream->index].second = pkts;
        }

    }

    // test check dts/pts
    for (auto iter = inputDatas.begin(); iter != inputDatas.end(); iter++) {
        vector<AVPacket*>& pkts = iter->second.second;
        AVRational time_base = iter->second.first->time_base;

        vector<double> pRealTimes;
        vector<double> dRealTimes;
        for (size_t i = 0; i < pkts.size(); i++) {
            pRealTimes.push_back(av_q2d(time_base) * pkts[i]->pts);
            dRealTimes.push_back(av_q2d(time_base) * pkts[i]->dts);
        }
    }

    // set out-stream-index to pkts which to be written into outputfile
    // <输入流， 数据包>
    vector<pair<AVStream*, AVPacket*>> pktsToWrite;
    for (auto iter = inputDatas.begin(); iter != inputDatas.end(); iter++) {
        int outStrIdx = iter->first;
        AVStream* inStr = iter->second.first;
        vector<AVPacket*> pkts = iter->second.second;

        for (size_t i = 0; i < pkts.size(); i++) {
            pkts[i]->stream_index = outStrIdx;
            pktsToWrite.push_back(std::make_pair(inStr, pkts[i]));
        }
    }

    // sort by dts
    std::sort(pktsToWrite.begin(), pktsToWrite.end(),
        [](const pair<AVStream*, AVPacket*> &left, const pair<AVStream*, AVPacket*> &right) {
            double dtsLeft = av_q2d(left.first->time_base) * left.second->dts;
            double dtsRight = av_q2d(right.first->time_base) * right.second->dts;
            return dtsLeft < dtsRight;
        });

    // 打开文件,写入文件头
    avio_open(&m_outFormatContext->pb, outputPath.c_str(), AVIO_FLAG_WRITE);
    int ret = avformat_write_header(m_outFormatContext, NULL);
    if (ret != 0) {
        return -2;
    }

    for (size_t i = 0; i < pktsToWrite.size(); i++) {
        AVStream* str = pktsToWrite[i].first;
        AVPacket* pkt = pktsToWrite[i].second;

        ret = av_interleaved_write_frame(m_outFormatContext, pkt);
        if (0 != ret) {
            char err[] = "严重错误，数据包写入失败";
        }
        if (i % 2000 == 0) {
            char ik[5];
            ik[2] = 4;
        }
    }

    // 写入文件尾部
    ret = av_write_trailer(m_outFormatContext);
    if (ret != 0) {
        av_strerror(ret, errors, 200);
        av_log(NULL, AV_LOG_WARNING, "av_write_trailer error: ret=%d, msg=%s\n", ret, errors);
    }
    // 关闭输出文件
    avio_close(m_outFormatContext->pb);
    return vedioFile.empty();

}
