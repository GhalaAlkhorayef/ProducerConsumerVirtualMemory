#ifndef LOGGER_H
#define LOGGER_H

void log_page_fault(int page_number);
void log_page_replacement(int evicted_page, int new_page);
void log_produced(int item, int index);
void log_consumed(int item, int index);
void log_buffer_state(int *buffer, int count, int in, int out);

#endif
