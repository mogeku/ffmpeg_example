#define SDL_MAIN_HANDLED

#include <stdio.h>

#include <iostream>
using namespace std;

#define __STDC_CONSTANT_MACROS
extern "C" {
#include <SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <stdarg.h>
#include <stdio.h>

#define MAX_BUF_LEN 1024

int g_log_debug_flag = 1;
int g_log_info_flag = 1;
int g_log_warnning_flag = 1;
int g_log_error_flag = 1;

enum LOG_LEVEL { LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR };

void set_log_flag(int log_debug_flag, int log_info_flag, int log_warnning_flag,
                  int log_error_flag) {
  g_log_debug_flag = log_debug_flag;
  g_log_info_flag = log_info_flag;
  g_log_warnning_flag = log_warnning_flag;
  g_log_error_flag = log_error_flag;
}

void output_log(LOG_LEVEL log_level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char buf[MAX_BUF_LEN] = {0};
  vsnprintf(buf, MAX_BUF_LEN - 1, fmt, args);

  switch (log_level) {
    case LOG_DEBUG:
      if (g_log_debug_flag) printf("[Log-Debug]:%s\n", buf);
      break;
    case LOG_INFO:
      if (g_log_info_flag) printf("[Log-Info]:%s\n", buf);
      break;
    case LOG_WARNING:
      if (g_log_warnning_flag) printf("[Log-Warnning]:%s\n", buf);
      break;
    case LOG_ERROR:
      if (g_log_error_flag) printf("[Log-Error]:%s\n", buf);
      break;
    default:
      break;
  }

  va_end(args);
}

static int g_frame_rate = 1;
static int g_sfp_refresh_thread_exit = 0;
static int g_sfp_refresh_thread_pause = 0;

#define SFM_REFRESH_EVENT (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT (SDL_USEREVENT + 2)

typedef struct FFmpeg_V_Param_T {
  AVFormatContext *pFormatCtx;
  AVCodecContext *pCodecCtx;
  SwsContext *pSwsCtx;
  int video_index;
} FFmpeg_V_Param;

typedef struct SDL_Param_T {
  SDL_Window *p_sdl_window;
  SDL_Renderer *p_sdl_renderer;
  SDL_Texture *p_sdl_texture;
  SDL_Rect sdl_rect;
  SDL_Thread *p_sdl_thread;
} SDL_Param;

/*
  return value:zero(success) non-zero(failure)
*/
int init_ffmpeg(FFmpeg_V_Param *p_ffmpeg_param, const char *filePath) {
  // init FFmpeg_V_Param
  p_ffmpeg_param->pFormatCtx = avformat_alloc_context();
  const AVCodec *pCodec = NULL;

  // do global initialization of network libraries
  avformat_network_init();

  // open input stream
  if (avformat_open_input(&(p_ffmpeg_param->pFormatCtx), filePath, NULL,
                          NULL) != 0) {
    output_log(LOG_ERROR, "avformat_open_input error");
    return -1;
  }

  // find stream info
  if (avformat_find_stream_info(p_ffmpeg_param->pFormatCtx, NULL) < 0) {
    output_log(LOG_ERROR, "avformat_find_stream_info error");
    return -1;
  }

  // get video pCodecParms, codec and frame rate
  for (int i = 0; i < p_ffmpeg_param->pFormatCtx->nb_streams; i++) {
    AVStream *pStream = p_ffmpeg_param->pFormatCtx->streams[i];
    if (pStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      pCodec = avcodec_find_decoder(pStream->codecpar->codec_id);
      p_ffmpeg_param->pCodecCtx = avcodec_alloc_context3(pCodec);
      avcodec_parameters_to_context(p_ffmpeg_param->pCodecCtx,
                                    pStream->codecpar);
      g_frame_rate = pStream->avg_frame_rate.num / pStream->avg_frame_rate.den;
      p_ffmpeg_param->video_index = i;
    }
  }
  if (!p_ffmpeg_param->pCodecCtx) {
    output_log(LOG_ERROR, "could not find video codecCtx");
    return -1;
  }

  // open codec
  if (avcodec_open2(p_ffmpeg_param->pCodecCtx, pCodec, NULL)) {
    output_log(LOG_ERROR, "avcodec_open2 error");
    return -1;
  }

  // get scale pixelformat context
  p_ffmpeg_param->pSwsCtx = sws_getContext(
      p_ffmpeg_param->pCodecCtx->width, p_ffmpeg_param->pCodecCtx->height,
      p_ffmpeg_param->pCodecCtx->pix_fmt, p_ffmpeg_param->pCodecCtx->width,
      p_ffmpeg_param->pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL,
      NULL, NULL);

  //   av_dump_format(p_ffmpeg_param->pFormatCtx,
  p_ffmpeg_param->video_index,
      //                  filePath, 0);
      av_dump_format(p_ffmpeg_param->pFormatCtx, 10, filePath, 0);

  return 0;
}

/*
  return value:zero(success) non-zero(failure)
*/
int release_ffmpeg(FFmpeg_V_Param *p_ffmpeg_param) {
  if (!p_ffmpeg_param) return -1;

  // realse scale pixelformat context
  if (p_ffmpeg_param->pSwsCtx) sws_freeContext(p_ffmpeg_param->pSwsCtx);

  // close codec
  if (p_ffmpeg_param->pCodecCtx) avcodec_close(p_ffmpeg_param->pCodecCtx);

  // close input stream
  if (p_ffmpeg_param->pFormatCtx)
    avformat_close_input(&(p_ffmpeg_param->pFormatCtx));

  // free AVCodecContext
  if (p_ffmpeg_param->pCodecCtx)
    avcodec_free_context(&(p_ffmpeg_param->pCodecCtx));

  // free AVFormatContext
  if (p_ffmpeg_param->pFormatCtx)
    avformat_free_context(p_ffmpeg_param->pFormatCtx);

  // free FFmpeg_V_Param
  delete p_ffmpeg_param;
  p_ffmpeg_param = NULL;

  return 0;
}

int sfp_refresh_thread(void *opaque) {
  g_sfp_refresh_thread_exit = 0;
  g_sfp_refresh_thread_pause = 0;
  while (!g_sfp_refresh_thread_exit) {
    if (!g_sfp_refresh_thread_pause) {
      SDL_Event sdl_event;
      sdl_event.type = SFM_REFRESH_EVENT;
      SDL_PushEvent(&sdl_event);
    }
    SDL_Delay(1000 / g_frame_rate);
  }
  g_sfp_refresh_thread_exit = 0;
  g_sfp_refresh_thread_pause = 0;
  SDL_Event sdl_event;
  sdl_event.type = SFM_BREAK_EVENT;
  SDL_PushEvent(&sdl_event);
  return 0;
}

int init_sdl2(SDL_Param_T *p_sdl_param, int screen_w, int screen_h) {
  if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
    output_log(LOG_ERROR, "SDL_Init error");
    return -1;
  }
  p_sdl_param->p_sdl_window = SDL_CreateWindow(
      "vPlayer_sdl", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w,
      screen_h, SDL_WINDOW_OPENGL);
  if (!p_sdl_param->p_sdl_window) {
    output_log(LOG_ERROR, "SDL_CreateWindow error");
    return -1;
  }
  p_sdl_param->p_sdl_renderer =
      SDL_CreateRenderer(p_sdl_param->p_sdl_window, -1, 0);
  p_sdl_param->p_sdl_texture =
      SDL_CreateTexture(p_sdl_param->p_sdl_renderer, SDL_PIXELFORMAT_IYUV,
                        SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);
  p_sdl_param->sdl_rect.x = 0;
  p_sdl_param->sdl_rect.y = 0;
  p_sdl_param->sdl_rect.w = screen_w;
  p_sdl_param->sdl_rect.h = screen_h;
  p_sdl_param->p_sdl_thread = SDL_CreateThread(sfp_refresh_thread, NULL, NULL);

  return 0;
}

int release_sdl2(SDL_Param_T *p_sdl_param) {
  SDL_DestroyTexture(p_sdl_param->p_sdl_texture);
  SDL_DestroyRenderer(p_sdl_param->p_sdl_renderer);
  SDL_DestroyWindow(p_sdl_param->p_sdl_window);
  SDL_Quit();
  return 0;
}

int vPlayer_sdl2(const char *filePath) {
  // ffmpeg param
  FFmpeg_V_Param *p_ffmpeg_param = NULL;
  AVPacket *packet = NULL;
  AVFrame *pFrame = NULL, *pFrameYUV = NULL;
  int out_buffer_size = 0;
  unsigned char *out_buffer = 0;

  // sdl param
  SDL_Param_T *p_sdl_param = NULL;
  SDL_Event sdl_event;

  int ret = 0;

  // init ffmpeg
  p_ffmpeg_param = new FFmpeg_V_Param();
  memset(p_ffmpeg_param, 0, sizeof(FFmpeg_V_Param));
  if (init_ffmpeg(p_ffmpeg_param, filePath)) {
    ret = -1;
    goto end;
  }
  packet = av_packet_alloc();
  pFrame = av_frame_alloc();
  pFrameYUV = av_frame_alloc();
  out_buffer_size = av_image_get_buffer_size(
      AV_PIX_FMT_YUV420P, p_ffmpeg_param->pCodecCtx->width,
      p_ffmpeg_param->pCodecCtx->height, 1);
  out_buffer = (unsigned char *)av_malloc(out_buffer_size);
  av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
                       AV_PIX_FMT_YUV420P, p_ffmpeg_param->pCodecCtx->width,
                       p_ffmpeg_param->pCodecCtx->height, 1);
  p_sdl_param = new SDL_Param_T();
  memset(p_sdl_param, 0, sizeof(SDL_Param_T));
  if (init_sdl2(p_sdl_param, p_ffmpeg_param->pCodecCtx->width,
                p_ffmpeg_param->pCodecCtx->height)) {
    ret = -1;
    goto end;
  }

  // demuxing and show
  while (true) {
    int temp_ret = 0;
    SDL_WaitEvent(&sdl_event);
    if (sdl_event.type == SFM_REFRESH_EVENT) {
      while (true) {
        if (av_read_frame(p_ffmpeg_param->pFormatCtx, packet) < 0) {
          g_sfp_refresh_thread_exit = 1;
          break;
        }
        if (packet->stream_index == p_ffmpeg_param->video_index) {
          break;
        }
      }
      if (avcodec_send_packet(p_ffmpeg_param->pCodecCtx, packet))
        g_sfp_refresh_thread_exit = 1;

      do {
        temp_ret = avcodec_receive_frame(p_ffmpeg_param->pCodecCtx, pFrame);
        if (temp_ret == AVERROR_EOF) {
          g_sfp_refresh_thread_exit = 1;
          break;
        }
        if (temp_ret == 0) {
          sws_scale(p_ffmpeg_param->pSwsCtx,
                    (const unsigned char *const *)pFrame->data,
                    pFrame->linesize, 0, p_ffmpeg_param->pCodecCtx->height,
                    pFrameYUV->data, pFrameYUV->linesize);

          SDL_UpdateTexture(p_sdl_param->p_sdl_texture,
                            &(p_sdl_param->sdl_rect), pFrameYUV->data[0],
                            pFrameYUV->linesize[0]);

          SDL_RenderClear(p_sdl_param->p_sdl_renderer);
          SDL_RenderCopy(p_sdl_param->p_sdl_renderer,
                         p_sdl_param->p_sdl_texture, NULL,
                         &(p_sdl_param->sdl_rect));
          SDL_RenderPresent(p_sdl_param->p_sdl_renderer);
        }
      } while (temp_ret != AVERROR(EAGAIN));

      // av_packet_unref(packet);
    } else if (sdl_event.type == SFM_BREAK_EVENT) {
      break;
    } else if (sdl_event.type == SDL_KEYDOWN) {
      if (sdl_event.key.keysym.sym == SDLK_SPACE)
        g_sfp_refresh_thread_pause = !g_sfp_refresh_thread_pause;
      if (sdl_event.key.keysym.sym == SDLK_q) g_sfp_refresh_thread_exit = 1;
    } else if (sdl_event.type == SDL_QUIT) {
      g_sfp_refresh_thread_exit = 1;
    }
  }

end:
  release_ffmpeg(p_ffmpeg_param);
  av_packet_free(&packet);
  av_frame_free(&pFrame);
  av_frame_free(&pFrameYUV);
  release_sdl2(p_sdl_param);
  return ret;
}

int my_ffplay(const string &mp4) {
  AVFormatContext *p_fmt_ctx = NULL;
  int ret = avformat_open_input(&p_fmt_ctx, mp4.c_str(), NULL, NULL);
  if (ret != 0) {
    cout << "avformat_open_input failed\n";
    return ret;
  }

  ret = avformat_find_stream_info(p_fmt_ctx, NULL);
  if (ret != 0) {
    cout << "avformat_open_input failed\n";
    return ret;
  }

  av_dump_format(p_fmt_ctx, 0, mp4.c_str(), 0);

  int v_idx = -1;
  for (auto i = 0; i < p_fmt_ctx->nb_streams; ++i) {
    cout << "stream codec type: " << p_fmt_ctx->streams[i]->codecpar->codec_type
         << endl;
    if (p_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      v_idx = i;
      cout << "Find a video stream, index " << v_idx << endl;
      // break;
    }
  }
  if (v_idx == -1) {
    cout << "Not find a video stream" << endl;
    return -1;
  }

  AVCodecParameters *p_codec_par = p_fmt_ctx->streams[v_idx]->codecpar;
  cout << "Video codec id: " << p_codec_par->codec_id << endl;
  const AVCodec *p_codec = avcodec_find_decoder(p_codec_par->codec_id);
  if (p_codec == NULL) {
    cout << "Not find a codec" << endl;
    return -1;
  }

  AVCodecContext *p_codec_ctx = avcodec_alloc_context3(p_codec);
  ret = avcodec_parameters_to_context(p_codec_ctx, p_codec_par);
  if (ret != 0) {
    cout << "avcodec_parameters_to_context failed " << ret << endl;
    ;
    return ret;
  }

  ret = avcodec_open2(p_codec_ctx, p_codec, NULL);
  if (ret != 0) {
    cout << "avcodec_open2 failed " << ret << endl;
    return ret;
  }

  AVFrame *p_frm_raw = av_frame_alloc();
  AVFrame *p_frm_yuv = av_frame_alloc();

  int buf_size = av_image_get_buffer_size(
      AV_PIX_FMT_YUV420P, p_codec_ctx->width, p_codec_ctx->height, 1);
  uint8_t *buffer = (uint8_t *)av_malloc(buf_size);
  av_image_fill_arrays(p_frm_yuv->data, p_frm_yuv->linesize, buffer,
                       AV_PIX_FMT_YUV420P, p_codec_ctx->width,
                       p_codec_ctx->height, 1);

  SwsContext *sws_ctx = sws_getContext(p_codec_ctx->width, p_codec_ctx->height,
                                       p_codec_ctx->pix_fmt, p_codec_ctx->width,
                                       p_codec_ctx->height, AV_PIX_FMT_YUV420P,
                                       SWS_BICUBIC, NULL, NULL, NULL);

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    cout << "SDL_Init failed: " << SDL_GetError() << endl;
    return -1;
  }

  SDL_Window *screen = SDL_CreateWindow(
      "my_ffplay Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      p_codec_ctx->width, p_codec_ctx->height, SDL_WINDOW_OPENGL);

  if (screen == NULL) {
    cout << "SDL_CreateWindow failed: " << SDL_GetError() << endl;
    return -1;
  }

  SDL_Renderer *sdl_renderer = SDL_CreateRenderer(screen, -1, 0);

  SDL_Texture *sdl_texture = SDL_CreateTexture(
      sdl_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
      p_codec_ctx->width, p_codec_ctx->height);

  SDL_Rect sdl_rect;
  sdl_rect.x = 0;
  sdl_rect.y = 0;
  sdl_rect.w = p_codec_ctx->width;
  sdl_rect.h = p_codec_ctx->height;

  AVPacket *p_packet = (AVPacket *)av_malloc(sizeof(AVPacket));

  while (av_read_frame(p_fmt_ctx, p_packet) == 0) {
    if (p_packet->stream_index == v_idx) {
      ret = avcodec_send_packet(p_codec_ctx, p_packet);
      if (ret != 0) {
        cout << "avcodec_send_packet failed " << ret << endl;
        return ret;
      }

      // do {
        ret = avcodec_receive_frame(p_codec_ctx, p_frm_raw);
        cout << ret << endl;
        // if (ret == AVERROR_EOF) {
        //   char e[255]{0};
        //   av_strerror(ret, e, 255);
        //   cout << "avcodec_receive_frame failed " << ret << ", " << e << endl;
        //   return ret;
        // }

        if (ret == 0) {
          sws_scale(sws_ctx, (const uint8_t *const *)p_frm_raw->data,
                    p_frm_raw->linesize, 0, p_codec_ctx->height,
                    p_frm_yuv->data, p_frm_yuv->linesize);

          SDL_UpdateYUVTexture(sdl_texture, &sdl_rect, p_frm_yuv->data[0],
                               p_frm_yuv->linesize[0], p_frm_yuv->data[1],
                               p_frm_yuv->linesize[1], p_frm_yuv->data[2],
                               p_frm_yuv->linesize[2]);

          SDL_RenderClear(sdl_renderer);
          SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &sdl_rect);
          SDL_RenderPresent(sdl_renderer);

          SDL_Delay(100);
        }
      // } while (ret != AVERROR(EAGAIN));
    }
    av_packet_unref(p_packet);
  }

  SDL_Quit();
  sws_freeContext(sws_ctx);
  av_free(buffer);
  av_frame_free(&p_frm_yuv);
  av_frame_free(&p_frm_raw);
  avcodec_close(p_codec_ctx);
  avformat_close_input(&p_fmt_ctx);

  return ret;
}

int main() {
  string mp4{R"(E:\Project\Git\JupyterNotebook\Mp4Analyse\data\data1.mp4)"};
  int ret = 0;

  // vPlayer_sdl2(mp4.c_str());

  ret = my_ffplay(mp4);

  return ret;
}
