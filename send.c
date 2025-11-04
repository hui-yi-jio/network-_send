#include <arpa/inet.h>
#include <assert.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <threads.h>
#include <unistd.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef _Float32 f32;

volatile bool quit;
void sigh(int) { quit = 1; }

int sockfd;
struct {
  u32 id, seq, t;
  u16 type;
  u8 data[1500];
} sbuf = {2, 0, 0, 0x1919, "test"}, rbuf;

_Atomic u64 compcnt, sbytes, rbytes;
int comph(void *) {
  u64 c = 0;
  while (!quit) {
    u64 cnt = compcnt++;
    f32 t = cnt * 1e-3;
    f32 y = __builtin_sinf(t) + 1;
    sbuf.data[c] = y * 128;
    c = c == 1499 ? 0 : c + 1;
  }
  return 0;
}

int sendh(void *) {
  struct ifreq ifr = {{"enp46s0"}, {}};
  assert(ioctl(sockfd, SIOCGIFINDEX, &ifr) != -1 || close(sockfd));
  struct sockaddr_ll addr = {AF_PACKET, 0, ifr.ifr_ifindex, 0, 0, 6, "\6"};
  while (!quit) {
    sbytes += sendto(sockfd, (void *)&sbuf, 1514, 0, (struct sockaddr *)&addr,
                     sizeof(addr));
    ++sbuf.seq;
  }
  return 0;
}
int recvh(void *) {
  while (!quit) {
    u32 r = recvfrom(sockfd, (void *)&rbuf, 1514, 0, 0, 0);
    if (rbuf.id == 2)
      continue;
    rbytes += r;
  }
  return 0;
}
int main() {
  sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  assert(sockfd != -1);

  signal(SIGINT, sigh);
  signal(SIGTERM, sigh);
  thrd_t compt, sendt, recvt;
#define crthrd(fn) thrd_create(&fn##t, fn##h, 0)
  crthrd(comp);
  crthrd(send);
  crthrd(recv);

  while (!quit) {
    printf("\rseq: %10u send rate: %10fMbps\trecv rate: %10fMbps\tcnt: %10lu",
           sbuf.seq, sbytes * 8e-6, rbytes * 8e-6, compcnt);
    compcnt = 0, sbytes = 0, rbytes = 0;
    fflush(stdout);
    sleep(1);
  }
  close(sockfd);
  thrd_join(compt, 0);
  thrd_join(sendt, 0);
  thrd_join(recvt, 0);
}
