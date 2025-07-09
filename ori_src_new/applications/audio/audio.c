#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_file.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <drv_sound.h>

#define BUFSZ   20
#define SOUND_DEVICE_NAME    "sound0"    /* Audio 设备名称 */
static rt_device_t snd_dev;              /* Audio 设备句柄 */
struct rt_audio_caps caps = {0};
struct wav_info *info = NULL;

char * filepath = "/sdcard/mymusic.wav";

char * filepath1 = "/sdcard/tongchang.wav";
char * filepath2 = "/sdcard/qingdu.wav";
char * filepath3 = "/sdcard/zhongdu.wav";
char * filepath4 = "/sdcard/yanzhong.wav";

struct RIFF_HEADER_DEF
{
    char riff_id[4];     // 'R','I','F','F'
    uint32_t riff_size;
    char riff_format[4]; // 'W','A','V','E'
};

struct WAVE_FORMAT_DEF
{
    uint16_t FormatTag;
    uint16_t Channels;
    uint32_t SamplesPerSec;
    uint32_t AvgBytesPerSec;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
};

struct FMT_BLOCK_DEF
{
    char fmt_id[4];    // 'f','m','t',' '
    uint32_t fmt_size;
    struct WAVE_FORMAT_DEF wav_format;
};

struct DATA_BLOCK_DEF
{
    char data_id[4];     // 'R','I','F','F'
    uint32_t data_size;
};

struct wav_info
{
    struct RIFF_HEADER_DEF header;
    struct FMT_BLOCK_DEF   fmt_block;
    struct DATA_BLOCK_DEF  data_block;
};

int wavplay_init()
{

    rt_hw_sound_init();
    info = (struct wav_info *) rt_malloc(sizeof * info);
    if (info == RT_NULL)
        return -1;

    /* 根据设备名称查找 Audio 设备，获取设备句柄 */
    snd_dev = rt_device_find(SOUND_DEVICE_NAME);

    /* 以只写方式打开 Audio 播放设备 */
    rt_device_open(snd_dev, RT_DEVICE_OFLAG_WRONLY);

    /* 设置采样率、通道、采样位数等音频参数信息 */
    caps.main_type               = AUDIO_TYPE_OUTPUT;                           /* 输出类型（播放设备 ）*/
    caps.sub_type                = AUDIO_DSP_PARAM;                             /* 设置所有音频参数信息 */
    caps.udata.config.samplerate = info->fmt_block.wav_format.SamplesPerSec;    /* 采样率 */
    caps.udata.config.channels   = info->fmt_block.wav_format.Channels;         /* 采样通道 */
    caps.udata.config.samplebits = 2;                                          /* 采样位数 */
    rt_device_control(snd_dev, AUDIO_CTL_CONFIGURE, &caps);

    return 0;

}

int wavplay_enable(int No)
{
    int fd = -1;
    uint8_t *buffer = NULL;
    buffer = rt_malloc(BUFSZ);
    if (buffer == RT_NULL)
    {
        return -1;
    }
    switch(No)
    {

        case 0:
        {
            fd = open(filepath, O_RDONLY);
            break;
        }

        case 1:
        {
            fd = open(filepath1, O_RDONLY);
            break;
        }
        case 2:
        {
            fd = open(filepath2, O_RDONLY);
            break;
        }
        case 3:
        {
            fd = open(filepath3, O_RDONLY);
            break;
        }
        case 4:
        {
            fd = open(filepath4, O_RDONLY);
            break;
        }
        default: return -1;break;
    }

    if (fd < 0)
    {
        rt_kprintf("open file failed!\n");
        return -1;
    }
    if (read(fd, &(info->header), sizeof(struct RIFF_HEADER_DEF)) <= 0)
        return -1;
    if (read(fd, &(info->fmt_block),  sizeof(struct FMT_BLOCK_DEF)) <= 0)
        return -1;
    if (read(fd, &(info->data_block), sizeof(struct DATA_BLOCK_DEF)) <= 0)
        return -1;
    while (1)
    {
        int length;

        /* 从文件系统读取 wav 文件的音频数据 */
        length = read(fd, buffer, BUFSZ);

        if (length <= 0)
            break;

        /* 向 Audio 设备写入音频数据 */
        rt_device_write(snd_dev, 0, buffer, length);
    }
    rt_free(buffer);
    close(fd);
    return 0;
}

int wavplay_sample()
{
    int fd = -1;
    uint8_t *buffer = NULL;
    struct wav_info *info = NULL;
    struct rt_audio_caps caps = {0};


    fd = open(filepath, O_RDONLY);
    if (fd < 0)
    {
        rt_kprintf("open file failed!\n");
        goto __exit;
    }

    buffer = rt_malloc(BUFSZ);
    if (buffer == RT_NULL)
        goto __exit;

    info = (struct wav_info *) rt_malloc(sizeof * info);
    if (info == RT_NULL)
        goto __exit;

    if (read(fd, &(info->header), sizeof(struct RIFF_HEADER_DEF)) <= 0)
        goto __exit;
    if (read(fd, &(info->fmt_block),  sizeof(struct FMT_BLOCK_DEF)) <= 0)
        goto __exit;
    if (read(fd, &(info->data_block), sizeof(struct DATA_BLOCK_DEF)) <= 0)
        goto __exit;

    rt_kprintf("wav information:\n");
    rt_kprintf("samplerate %d\n", info->fmt_block.wav_format.SamplesPerSec);
    rt_kprintf("channel %d\n", info->fmt_block.wav_format.Channels);

    /* 根据设备名称查找 Audio 设备，获取设备句柄 */
    snd_dev = rt_device_find(SOUND_DEVICE_NAME);

    /* 以只写方式打开 Audio 播放设备 */
    rt_device_open(snd_dev, RT_DEVICE_OFLAG_WRONLY);

    /* 设置采样率、通道、采样位数等音频参数信息 */
    caps.main_type               = AUDIO_TYPE_OUTPUT;                           /* 输出类型（播放设备 ）*/
    caps.sub_type                = AUDIO_DSP_PARAM;                             /* 设置所有音频参数信息 */
    caps.udata.config.samplerate = info->fmt_block.wav_format.SamplesPerSec;    /* 采样率 */
    caps.udata.config.channels   = info->fmt_block.wav_format.Channels;         /* 采样通道 */
    caps.udata.config.samplebits = 16;                                          /* 采样位数 */
    rt_device_control(snd_dev, AUDIO_CTL_CONFIGURE, &caps);

    while (1)
    {
        int length;

        /* 从文件系统读取 wav 文件的音频数据 */
        length = read(fd, buffer, BUFSZ);

        if (length <= 0)
            break;

        /* 向 Audio 设备写入音频数据 */
        rt_device_write(snd_dev, 0, buffer, length);
    }

    /* 关闭 Audio 设备 */
    rt_device_close(snd_dev);

__exit:

    if (fd >= 0)
        close(fd);

    if (buffer)
        rt_free(buffer);

    if (info)
        rt_free(info);

    return 0;
}

//MSH_CMD_EXPORT(wavplay_sample,  play wav file);
