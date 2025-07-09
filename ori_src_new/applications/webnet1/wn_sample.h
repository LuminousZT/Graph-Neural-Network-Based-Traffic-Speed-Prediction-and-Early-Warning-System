/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-15     Administrator       the first version
 */
#ifndef APPLICATIONS_WEBNET_WN_SAMPLE_H_
#define APPLICATIONS_WEBNET_WN_SAMPLE_H_



#define WEBNET_USING_UPLOAD
#include "wn_session.h"

void asp_var_version(struct webnet_session* session);
void cgi_calc_handler(struct webnet_session* session);
void cgi_hello_handler(struct webnet_session* session);
void webnet_start(void);

const char *get_file_name(struct webnet_session *session);
 int upload_open(struct webnet_session *session);
 int upload_close(struct webnet_session* session);
 int upload_write(struct webnet_session* session, const void* data, rt_size_t length);
 int upload_done (struct webnet_session* session);


#endif /* APPLICATIONS_WEBNET_WN_SAMPLE_H_ */
