#ifndef GCODE_ENGINE_H
#define GCODE_ENGINE_H
 

void gcode_engine_init(void);
void gcode_engine_once(void);



void gcode_engine_command(const char cmd_line, const burst_rcv_ctx_t * rcv_ctx);




#endif // GCODE_ENGINE_H
