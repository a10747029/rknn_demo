#ifndef _CAMERA_RECORD_H_
#define _CAMERA_RECORD_H_
#include <string.h>
#include <rk_mpi.h>
#include <mpp_env.h>
#include <mpp_mem.h>
#include <mpp_log.h>
#include <mpp_time.h>
#include <mpp_common.h>
//#include <rk_type.h>

#include <utils.h>

#ifdef __cplusplus
extern "C"{
#endif

extern size_t fifo_write_len;

void record_init();
void record_put_frame(char* buf);
#ifdef __cplusplus
}
#endif

#endif
