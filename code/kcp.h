#ifndef __KCP_H__
#define __KCP_H__

#include <stdint.h>
#include <list>
#include <memory>

namespace KCP {

struct KCPSEG {
   uint32_t conv;     //会话编号，两方一致才会通信
   uint32_t cmd;      //指令类型，四种下面会说
   uint32_t frg;      //分片编号 倒数第几个seg。主要就是用来合并一块被分段的数据。
   uint32_t wnd;      //自己可用窗口大小    
   uint32_t ts;
   uint32_t sn;       //编号 确认编号或者报文编号
   uint32_t una;      //代表编号前面的所有报都收到了的标志
   uint32_t len;
   uint32_t resendts; //重传的时间戳。超过当前时间重发这个包
   uint32_t rto;      //超时重传时间，根据网络去定
   uint32_t fastack;  //快速重传机制，记录被跳过的次数，超过次数进行快速重传
   uint32_t xmit;     //重传次数
   char data[1];     //数据内容
};


class Kcp {
public:
    typedef std::shared_ptr<Kcp> ptr;
    Kcp(uint32_t conv, void* user);
    ~Kcp();

    static Kcp::ptr kcp_create(uint32_t conv, void *user);
    static void kcp_release(Kcp::ptr kcp); 

private:
    uint32_t conv;   // 标识会话
    uint32_t mtu;    // 最大传输单元,默认数据为1400,最小为50
    uint32_t mss;    // 最大分片大小,不大于mtu
    uint32_t state;  // 连接状态(0xffffffff表示断开连接)
    uint32_t snd_una;    // 第一个未确认的包
    uint32_t snd_nxt;    // 下一个待分配包的序号，这个值实际是用来分配序号的
    uint32_t rcv_nxt;    // 待接收消息序号.为了保证包的顺序,接收方会维护一个接收窗口,接收窗口有一个起始序号rcv_nxt
    		             // 以及尾序号rcv_nxt + rcv_wnd(接收窗口大小)

    uint32_t ts_recent;  
    uint32_t ts_lastack;
    uint32_t ssthresh;       // 拥塞窗口的阈值
    
    
    int32_t  rx_rttval;      // RTT的变化量,代表连接的抖动情况
    int32_t  rx_srtt;        // smoothed round trip time，平滑后的RTT；
    int32_t  rx_rto;         // 收ACK接收延迟计算出来的重传超时时间
    int32_t  rx_minrto;      // 最小重传超时时间

    uint32_t snd_wnd;        // 发送窗口大小
    uint32_t rcv_wnd;        // 接收窗口大小，本质上而言如果接收端一直不去读取数据则rcv_queue就会满(达到rcv_wnd)
    uint32_t rmt_wnd;        // 远端接收窗口大小
    uint32_t cwnd;           // 拥塞窗口大小, 动态变化
    uint32_t probe;          // 探查变量, IKCP_ASK_TELL表示告知远端窗口大小。IKCP_ASK_SEND表示请求远端告知窗口大小；

    uint32_t current;
    uint32_t interval;       // 内部flush刷新间隔，对系统循环效率有非常重要影响, 间隔小了cpu占用率高, 间隔大了响应慢
    uint32_t ts_flush;       // 下次flush刷新的时间戳
    uint32_t xmit;           // 发送segment的次数, 当segment的xmit增加时，xmit增加(重传除外)

    uint32_t nrcv_buf;       // 接收缓存中的消息数量
    uint32_t nsnd_buf;       // 发送缓存中的消息数量

    uint32_t nrcv_que;       // 接收队列中消息数量
    uint32_t nsnd_que;       // 发送队列中消息数量

    uint32_t nodelay;        // 是否启动无延迟模式。无延迟模式rtomin将设置为0，拥塞控制不启动；
    uint32_t updated;         //是 否调用过update函数的标识；

    uint32_t ts_probe;       // 下次探查窗口的时间戳；
    uint32_t probe_wait;     // 探查窗口需要等待的时间；

    uint32_t dead_link;      // 最大重传次数，被认为连接中断；
    uint32_t incr;           // 可发送的最大数据量；

    std::list<KCPSEG> snd_queue;    //发送消息的队列 
    std::list<KCPSEG> rcv_queue;    //接收消息的队列, 是已经确认可以供用户读取的数据
    std::list<KCPSEG> snd_buf;      //发送消息的缓存 和snd_queue有什么区别
    std::list<KCPSEG> rcv_buf;      //接收消息的缓存, 还不能直接供用户读取的数据
    
    uint32_t *acklist;   //待发送的ack的列表 当收到一个数据报文时，将其对应的 ACK 报文的 sn 号以及时间戳 ts 
    		             //同时加入到acklist 中，即形成如 [sn1, ts1, sn2, ts2 …] 的列表
    uint32_t ackcount;   //是当前计数, 记录 acklist 中存放的 ACK 报文的数量 
    uint32_t ackblock;   //是容量, acklist 数组的可用长度，当 acklist 的容量不足时，需要进行扩容

    void *user;     // 指针，可以任意放置代表用户的数据，也可以设置程序中需要传递的变量；
    char *buffer;   //  

    int fastresend; // 触发快速重传的重复ACK个数；
    int fastlimit;

    int nocwnd;     // 取消拥塞控制
    int stream;     // 是否采用流传输模式

    int logmask;    // 日志的类型，如IKCP_LOG_IN_DATA，方便调试

    int (*output)(const char *buf, int len, struct KCPCB *kcp, void *user);//发送消息的回调函数
    void (*writelog)(const char *log, struct KCPCB *kcp, void *user);  // 写日志的回调函数
};


#define IKCP_LOG_OUTPUT			1
#define IKCP_LOG_INPUT			2
#define IKCP_LOG_SEND			4
#define IKCP_LOG_RECV			8
#define IKCP_LOG_IN_DATA		16
#define IKCP_LOG_IN_ACK			32
#define IKCP_LOG_IN_PROBE		64
#define IKCP_LOG_IN_WINS		128
#define IKCP_LOG_OUT_DATA		256
#define IKCP_LOG_OUT_ACK		512
#define IKCP_LOG_OUT_PROBE		1024
#define IKCP_LOG_OUT_WINS		2048


}


#endif // __KCP_H__