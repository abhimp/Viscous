/*
 * This is an implementation of Viscous protocol.
 * Copyright (C) 2017  Abhijit Mondal
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/*
 * Created On : 30.11.2016
 * Author      : Sourav Bhattacharjee
 *
 */
#include "CongestionHandler.hh"
#include "../../util/LinuxMath.hh"
#include <math.h>    /* cbrt */
#include "CubicChannelHandler.hh"
#include "CommonHeaders.hh"
/*Add div64.h incase cube root does not work.*/

using namespace std;

#define BICTCP_BETA_SCALE    1024
#define BICTCP_HZ             10
#define min(a,b) ((a > b) ? b : a)

void cc_init(struct congestion *ca) {
    cube_rtt_scale = (bic_scale * 10);

    beta_scale = 8*(BICTCP_BETA_SCALE+beta) / 3
            / (BICTCP_BETA_SCALE - beta);

    cube_factor = 1ull << (10+3*BICTCP_HZ); /* 2^40 */
}

void cc_reset(struct congestion *ca) {
    ca->cnt = 0;
    ca->last_max_cwnd = 0; /*W_last_max*/
    //ca->last_cwnd = 0;
    //ca->last_time = 0;
    ca->origin_point = 0;
    ca->K = 0;
    ca->delay_min = 0; /*dMin*/
    ca->epoch_start = 0;
    ca->ack_cnt = 0;
    ca->tcp_cwnd = 0; /*W_tcp*/
    //ca->found = 0;

}


/*void cc_friedliness() {
    tcp_cwnd = tcp_cwnd + (3 * beta) / (3 - beta) + ack_cnt / cwnd;
    ack_cnt = 0;

    if (tcp_cwnd > cwnd) {
        max_cnt = cwnd / (tcp_cwnd - cwnd);
        if (cnt > max_cnt)
            cnt = max_cnt;
    }
}*/

void cc_update(struct congestion *ca, uint32_t cwnd, uint32_t acked) {
    appInt32 delta, target, max_cnt;
    appInt64 offs, t;

    ca->ack_cnt = ca->ack_cnt + acked;

    if (ca->epoch_start == 0) {
        ca->epoch_start = getTime();
        if (cwnd < ca->last_max_cwnd) {
            ca->K = cbrt((ca->last_max_cwnd - cwnd) * cube_factor); /*replace cbrt by cubic_root*/
            ca->origin_point = ca->last_max_cwnd;
        } else {
            ca->K = 0;
            ca->origin_point = cwnd;
        }
        ca->ack_cnt = 1;
        ca->tcp_cwnd = cwnd;
    }
    t = tcp_time_stamp - ca->epoch_start;
    t = t + msecs_to_jiffies(ca->delay_min >> 3);
    t <<= BICTCP_HZ;
    do_div(t, HZ);
    if (t < ca->K)
        offs = ca->K - t;
    else
        offs = t - ca->K;
    delta = (cube_rtt_scale * offs * offs * offs) >> (10 + 3 * BICTCP_HZ);

    if (t < ca->K)                            /* below origin*/
        target = ca->origin_point - delta;
    else                                          /* above origin*/
        target = ca->origin_point + delta;

    if (target > cwnd)
        ca->cnt = cwnd / (target - cwnd);
    else
        ca->cnt = 100 * cwnd;

    if (ca->last_max_cwnd == 0 && ca->cnt > 20)
        ca->cnt = 20;

    if (tcp_friendliness){
        appInt32 scale = beta_scale;

        delta = (cwnd * scale) >> 3;
        while(ca->ack_cnt > delta){
            ca->ack_cnt = ca->ack_cnt - delta;
            ca->tcp_cwnd = max_cnt;
        }
    }
}

void ack_rcv(struct congestion *ca, appInt16 acked) {
    if (ca->delay_min)
        ca->delay_min = min(ca->delay_min, CubicChannelHandler().RTT);
    else
        ca->delay_min = CubicChannelHandler().RTT;
    if (CubicChannelHandler().CWND <= ssthresh)
        cube_factor = CubicChannelHandler().CWND + acked;
    else {
        cc_update(ca, CubicChannelHandler().CWND, acked);
        if (CubicChannelHandler().snd_cwnd_cnt > ca->cnt) {
            CubicChannelHandler().CWND = CubicChannelHandler().CWND + 1;
            CubicChannelHandler().snd_cwnd_cnt = 0;
        } else {
            CubicChannelHandler().snd_cwnd_cnt = CubicChannelHandler().snd_cwnd_cnt + acked;
            if(CubicChannelHandler().snd_cwnd_cnt >= ca->cnt){
                appInt16 delta = CubicChannelHandler().snd_cwnd_cnt / ca->cnt;
                CubicChannelHandler().snd_cwnd_cnt -=delta * ca->cnt;
                CubicChannelHandler().CWND += delta;
            }
        }

    }
}


void packet_loss(struct congestion *ca) {
    ca->epoch_start = 0;
    if (CubicChannelHandler().CWND < ca->last_max_cwnd && fast_convergence)
        ca->last_max_cwnd = CubicChannelHandler().CWND * (2 - beta) / 2;
    else
        ca->last_max_cwnd = CubicChannelHandler().CWND;
    CubicChannelHandler().CWND = CubicChannelHandler().CWND * (1 - beta);
    ssthresh = CubicChannelHandler().CWND;
}

void timeoutCalculation(appInt64 RTT){

}
