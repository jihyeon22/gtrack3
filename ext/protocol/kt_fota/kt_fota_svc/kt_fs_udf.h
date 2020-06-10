#pragma once

char *kt_dm_srv_get_ip();
int kt_dm_srv_get_port();
char *kt_qty_srv_get_ip();
int kt_qty_srv_get_port();

char *kt_srv_get_cti(char *buf_out);
char *kt_srv_get_cust_tag(char *buf_out);
char *kt_srv_get_modem_qty(char *buf_out, int buf_len);
char *kt_srv_get_package_version(char *buf_out);
char *kt_srv_get_model_name();


char *kt_srv_get_device_qty_cpu(char *buf_out);
char *kt_srv_get_device_qty_mem(char *buf_out);
char *kt_srv_get_device_qty_hw(char *buf_out);
char *kt_srv_get_device_qty_err(char *buf_out);
