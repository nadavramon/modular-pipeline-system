#pragma once

const char* plugin_get_name(void);
const char* plugin_init(int queue_size);
const char* plugin_fini(void);
const char* plugin_place_work(const char* str);
const char* plugin_wait_finished(void);
void plugin_attach(const char* (*next_place_work)(const char*));
