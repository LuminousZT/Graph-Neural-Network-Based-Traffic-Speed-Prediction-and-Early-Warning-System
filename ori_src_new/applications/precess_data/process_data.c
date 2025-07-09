/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-19     Administrator       the first version
 */
#include "precess_data.h"
#include <math.h>



// 计算拥堵度（0-100%）
float Cal_Congestion_Degree(int free_flow_count,int mild_congestion_count,int moderate_congestion_count,int severe_congestion_count )
{
    float congestion_degree = (free_flow_count * FREE_WEIGHT) +(mild_congestion_count * MILD_WEIGHT) +(moderate_congestion_count * MODERATE_WEIGHT) +(severe_congestion_count * SEVERE_WEIGHT);
//    return  congestion_degree;
//    return fmaxf(0.0f, fminf(1.0f, (congestion_degree / nodes_OUTPUTS)));
    return congestion_degree;
}


int Cal_Congestion_Degree_NO(float congestion_degree)
{
    int No_Degree;
    if(congestion_degree>0.0 && congestion_degree<=30.0)
    {
        No_Degree = 4;
    }
    else if(congestion_degree>30.0 && congestion_degree<=55.0)
    {
        No_Degree = 3;
    }
    else if(congestion_degree>55.0 && congestion_degree<=60.0)
    {
        No_Degree = 2;
    }
    else if(congestion_degree>60.0)
    {
        No_Degree = 1;
    }
    return No_Degree;
}



