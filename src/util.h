#pragma once

void degrees_to_dms(double degrees, int *dd, int *mm, double *ss);
void degrees_to_hms(double degrees, int *hh, int *mm, double *ss);
double format_angle(int hh_or_dd, int mm, double ss);
int binary_strstr(char *buffer, size_t buffer_len, char *search_bytes, size_t search_len);