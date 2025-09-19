#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// 🔹 CONFIGURAÇÕES BÁSICAS
#define NO_SYS                      0
#define LWIP_SOCKET                 1
#define LWIP_NETCONN                1

// 🔹 CORRIGIR CONFLITO DE STRUCT TIMEVAL
#define LWIP_TIMEVAL_PRIVATE        0  // ✅ USA A STRUCT TIMEVAL DO SISTEMA

// 🔹 MEMÓRIA - CONFIGURAÇÃO MÍNIMA FUNCIONAL
#define MEM_LIBC_MALLOC             0
#define MEMP_MEM_MALLOC             0
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    4096

// 🔹 TCP/IP STACK
#define LWIP_IPV4                   1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_ICMP                   1

// 🔹 PBUF
#define PBUF_LINK_HLEN              (14 + ETH_PAD_SIZE)
#define PBUF_LINK_ENCAPSULATION_HLEN 0
#define PBUF_POOL_BUFSIZE           LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_ENCAPSULATION_HLEN+PBUF_LINK_HLEN)

// 🔹 ARP
#define LWIP_ARP                    1
#define ARP_TABLE_SIZE              10
#define ARP_QUEUEING                0

// 🔹 IP options
#define IP_FORWARD                  0
#define IP_REASSEMBLY               1
#define IP_FRAG                     1
#define IP_OPTIONS_ALLOWED          1
#define IP_REASS_MAXAGE             15
#define IP_REASS_MAX_PBUFS          10
#define IP_DEFAULT_TTL              255
#define IP_SOF_BROADCAST            0
#define IP_SOF_BROADCAST_RECV       0

// 🔹 ICMP
#define LWIP_ICMP                   1
#define ICMP_TTL                   (IP_DEFAULT_TTL)
#define LWIP_BROADCAST_PING         0
#define LWIP_MULTICAST_PING         0

// 🔹 RAW
#define LWIP_RAW                    1

// 🔹 DHCP
#define LWIP_DHCP                   1
#define DHCP_DOES_ARP_CHECK         ((LWIP_DHCP) && (LWIP_ARP))

// 🔹 AUTOIP
#define LWIP_AUTOIP                 0

// 🔹 TCP
#define TCP_TTL                     (IP_DEFAULT_TTL)
#define TCP_WND                     (4 * TCP_MSS)
#define TCP_MAXRTX                  12
#define TCP_SYNMAXRTX               6
#define TCP_QUEUE_OOSEQ             (LWIP_TCP)
#define TCP_MSS                     1460
#define TCP_CALCULATE_EFF_SEND_MSS  1
#define TCP_SND_BUF                 (2 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS))
#define TCP_SNDLOWAT                LWIP_MIN(LWIP_MAX(((TCP_SND_BUF)/2), (2 * TCP_MSS) + 1), (TCP_SND_BUF) - 1)
#define TCP_SNDQUEUELOWAT           LWIP_MAX(((TCP_SND_QUEUELEN)/2), 5)
#define TCP_OOSEQ_MAX_BYTES         0
#define TCP_OOSEQ_MAX_PBUFS         0
#define TCP_LISTEN_BACKLOG          0
#define TCP_DEFAULT_LISTEN_BACKLOG  0xff

// 🔹 Pbuf options
#define PBUF_POOL_SIZE              16

// 🔹 Network Interfaces options
#define LWIP_NETIF_HOSTNAME         0
#define LWIP_NETIF_API              0
#define LWIP_NETIF_STATUS_CALLBACK  0
#define LWIP_NETIF_LINK_CALLBACK    0
#define LWIP_NETIF_REMOVE_CALLBACK  0
#define LWIP_NETIF_HWADDRHINT       0
#define LWIP_NETIF_LOOPBACK         0
#define LWIP_LOOPBACK_MAX_PBUFS     0

// 🔹 LOOPIF
#define LWIP_HAVE_LOOPIF            0

// 🔹 Thread options
#define TCPIP_THREAD_NAME              "tcpip_thread"
#define TCPIP_THREAD_STACKSIZE         1024
#define TCPIP_THREAD_PRIO              (configMAX_PRIORITIES - 2)

#define TCPIP_MBOX_SIZE                8
#define SLIPIF_THREAD_STACKSIZE        1024
#define SLIPIF_THREAD_PRIO             1
#define PPP_THREAD_STACKSIZE           1024
#define PPP_THREAD_PRIO                1
#define DEFAULT_THREAD_STACKSIZE       1024
#define DEFAULT_THREAD_PRIO            1
#define DEFAULT_RAW_RECVMBOX_SIZE      8
#define DEFAULT_UDP_RECVMBOX_SIZE      8
#define DEFAULT_TCP_RECVMBOX_SIZE      8
#define DEFAULT_ACCEPTMBOX_SIZE        8

// 🔹 Sequential layer options
#define LWIP_TCPIP_TIMEOUT             0
#define LWIP_NETCONN_SEM_PER_THREAD    0

// 🔹 Socket options
#define LWIP_SO_SNDTIMEO                1
#define LWIP_SO_RCVTIMEO                1
#define LWIP_SO_SNDRCVTIMEO_NONSTANDARD 0
#define LWIP_SO_RCVBUF                  0
#define LWIP_SO_LINGER                  0
#define SO_REUSE                        0
#define SO_REUSE_RXTOALL                0
#define LWIP_FIONREAD_LINUXMODE         0

// 🔹 Statistics options
#define LWIP_STATS                      0
#define LWIP_STATS_DISPLAY              0

// 🔹 PPP options
#define PPP_SUPPORT                     0

// 🔹 Checksum options
#define LWIP_CHECKSUM_CTRL_PER_NETIF    0
#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_GEN_ICMP6              1
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1
#define CHECKSUM_CHECK_ICMP6            1
#define LWIP_CHECKSUM_ON_COPY           0

// 🔹 IPv6 options
#define LWIP_IPV6                       0

// 🔹 Debugging options
#define LWIP_DEBUG                      0

// 🔹 Performance tracking
#define LWIP_PERF                       0

// 🔹 MEMORY POOL SIZES
#define MEMP_NUM_PBUF                   16
#define MEMP_NUM_RAW_PCB                4
#define MEMP_NUM_UDP_PCB                4
#define MEMP_NUM_TCP_PCB                5
#define MEMP_NUM_TCP_PCB_LISTEN         8
#define MEMP_NUM_TCP_SEG                16
#define MEMP_NUM_REASSDATA              5
#define MEMP_NUM_FRAG_PBUF              15
#define MEMP_NUM_ARP_QUEUE              30
#define MEMP_NUM_IGMP_GROUP             8
#define MEMP_NUM_SYS_TIMEOUT            8
#define MEMP_NUM_NETBUF                 2
#define MEMP_NUM_NETCONN                4
#define MEMP_NUM_TCPIP_MSG_API          8
#define MEMP_NUM_TCPIP_MSG_INPKT        8

#endif
