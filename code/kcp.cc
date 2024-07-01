#include "kcp.h"

namespace KCP {

const uint32_t KCP_RTO_NDL = 30;            // 没有延迟的最小重传超时时间，单位：毫秒
const uint32_t KCP_RTO_MIN = 100;           // 正常情况下的最小重传超时时间，单位：毫秒
const uint32_t KCP_RTO_DEF = 200;           // 默认的重传超时时间，单位：毫秒
const uint32_t KCP_RTO_MAX = 60000;         // 最大的重传超时时间，单位：毫秒

const uint32_t KCP_CMD_PUSH = 81;           // 命令：推送数据
const uint32_t KCP_CMD_ACK  = 82;           // 命令：确认收到数据
const uint32_t KCP_CMD_WASK = 83;           // 命令：窗口探测（请求）
const uint32_t KCP_CMD_WINS = 84;           // 命令：窗口大小（告知）

const uint32_t KCP_ASK_SEND = 1;            // 需要发送 IKCP_CMD_WASK 命令
const uint32_t KCP_ASK_TELL = 2;            // 需要发送 IKCP_CMD_WINS 命令

const uint32_t KCP_WND_SND = 32;            // 发送窗口大小，表示同时允许发送的数据包数量
const uint32_t KCP_WND_RCV = 128;           // 接收窗口大小，表示可以接收的最大数据包数量，必须大于等于最大分段大小

const uint32_t KCP_MTU_DEF = 1400;          // 默认的最大传输单元大小，单位：字节

const uint32_t KCP_ACK_FAST = 3;            // 触发快速应答的最大次数

const uint32_t KCP_INTERVAL = 100;          // 内部发送心跳消息的时间间隔，单位：毫秒

const uint32_t KCP_OVERHEAD = 24;           // 协议头部固定长度，单位：字节

const uint32_t KCP_DEADLINK = 20;           // 检测死连接的时间阈值，单位：秒

const uint32_t KCP_THRESH_INIT = 2;         // 拥塞控制阈值的初始值

const uint32_t KCP_THRESH_MIN = 2;          // 拥塞控制阈值的最小值

const uint32_t KCP_PROBE_INIT = 7000;       // 7 秒用于探测窗口大小的初始时间，单位：毫秒

const uint32_t KCP_PROBE_LIMIT = 120000;    // 最多 120 秒用于探测窗口大小的时间限制，单位：毫秒

const uint32_t KCP_FASTACK_LIMIT = 5;       // 触发快速应答的最大次数限制

Kcp::Kcp(uint32_t conv, void* user) {
	// conv = conv;
    // kcp->user = user;
    // kcp->snd_una = 0;
	// kcp->snd_nxt = 0;
	// kcp->rcv_nxt = 0;
	// kcp->ts_recent = 0;
	// kcp->ts_lastack = 0;
	// kcp->ts_probe = 0;
	// kcp->probe_wait = 0;
	// kcp->snd_wnd = KCP_WND_SND;
	// kcp->rcv_wnd = KCP_WND_RCV;
	// kcp->rmt_wnd = KCP_WND_RCV;
	// kcp->cwnd = 0;
	// kcp->incr = 0;
	// kcp->probe = 0;
	// kcp->mtu = KCP_MTU_DEF;
	// kcp->mss = kcp->mtu - KCP_OVERHEAD;
	// kcp->stream = 0;
	// user = nullptr;
}

Kcp::~Kcp() {

}

Kcp::ptr Kcp::kcp_create(uint32_t conv, void *user) {
}

// release kcp control object
void Kcp::kcp_release(Kcp::ptr kcp) {

} 


}