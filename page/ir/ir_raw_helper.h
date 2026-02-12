#ifndef IR_RAW_HELPER_H
#define IR_RAW_HELPER_H

int ir_validate_raw_capture_file(const char *raw_path, int *token_count_out, int *last_sign_out);
int ir_append_synthetic_gap_for_odd_capture(const char *raw_path, int token_count, int last_sign);

#endif /* IR_RAW_HELPER_H */
