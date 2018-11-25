/******************************************************************************

  Copyright (C) 2015 Winner Micro electronics Co., Ltd.

 ******************************************************************************
  File Name     : alg.h
  Version       : Initial Draft
  Author        : Li Limin, lilm@winnermicro.com
  Created       : 2015/3/7
  Last Modified :
  Description   : Application layer gateway, (alg) only for apsta

  History       :
  Date          : 2015/3/7
  Author        : Li Limin, lilm@winnermicro.com
  Modification  : Created file

******************************************************************************/
#ifndef __ALG_H__
#define __ALG_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


/* ============================== configure ===================== */
/* napt age time (second) */
#define NAPT_TABLE_TIMEOUT           60

/* napt port range: 15000~19999 */
#define NAPT_LOCAL_PORT_RANGE_START  0x3A98
#define NAPT_LOCAL_PORT_RANGE_END    0x4E1F

/* napt icmp id range: 3000-65535 */
#define NAPT_ICMP_ID_RANGE_START     0xBB8
#define NAPT_ICMP_ID_RANGE_END       0xFFFF


/* napt table size */
#define NAPT_TABLE_LIMIT
#ifdef  NAPT_TABLE_LIMIT
#define NAPT_TABLE_SIZE_MAX          1000
#endif
/* ============================================================ */

#define NAPT_TMR_INTERVAL           ((NAPT_TABLE_TIMEOUT / 2) * 1000UL)

typedef enum napt_tmr_type_e {
    NAPT_TMR_TYPE_TCP,
    NAPT_TMR_TYPE_UDP,
    NAPT_TMR_TYPE_ICMP,
    NAPT_TMR_TYPE_GRE,
    NAPT_TMR_TYPE_MAX
} napt_tmr_type_t;

extern bool alg_napt_port_is_used(u16 port);

extern int alg_napt_init(void);

extern void alg_tmr(void);

extern int alg_input(const u8 *bssid, u8 *pkt_body, u32 pkt_len);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __ALG_H__ */

