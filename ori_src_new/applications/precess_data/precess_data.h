/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-19     Administrator       the first version
 */
#ifndef APPLICATIONS_PRECESS_DATA_PRECESS_DATA_H_
#define APPLICATIONS_PRECESS_DATA_PRECESS_DATA_H_

#include <stdio.h>
#include <drv_lcd.h>
#include <string.h>

// 定义权重系数（可根据实际路网特性调整）
#define FREE_WEIGHT 0.0    // 通畅无影响
#define MILD_WEIGHT 0.1    // 轻度拥堵权重
#define MODERATE_WEIGHT 0.2 // 中度拥堵权重
#define SEVERE_WEIGHT 0.7   // 严重拥堵权重
#define nodes_OUTPUTS 185

float Cal_Congestion_Degree(int free_flow_count,int mild_congestion_count,int moderate_congestion_count,int severe_congestion_count );
int Cal_Congestion_Degree_NO(float congestion_degree);



#endif /* APPLICATIONS_PRECESS_DATA_PRECESS_DATA_H_ */
